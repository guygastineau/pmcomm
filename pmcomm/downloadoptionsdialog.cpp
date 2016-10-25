#include "downloadoptionsdialog.h"

#include "loggeddownloader.h"
#include "appsettings.h"

#include <QGridLayout>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QMessageBox>
#include <QFileDialog>
#include <QLabel>
#include <QDir>
#include <QDateTime>
#include <QComboBox>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>

DownloadOptionsDialog::DownloadOptionsDialog(QSharedPointer<SiteSettings> settings, LoggedDownloader *downloader, QWidget *parent) : QDialog(parent) {
	setWindowTitle("Logged data auto-download");
	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
	this->settings = settings;
	this->downloader = downloader;

	QGridLayout *layout = new QGridLayout(this);

	QGroupBox *downloadsGroup = new QGroupBox("Stored Data");
	QGridLayout *downloadsLayout = new QGridLayout(downloadsGroup);

	QLabel *downloadLabel = new QLabel("Data was downloaded at:", this);

	downloadList = new QListWidget;
	downloadList->setSelectionMode(QAbstractItemView::ExtendedSelection);
	refreshDownloadList();

	QPushButton *deleteButton = new QPushButton("Delete Selected");
	QPushButton *exportSelected = new QPushButton("Export Selected...");
	QPushButton *exportAll = new QPushButton("Export All...");

	connect(deleteButton, SIGNAL(clicked(bool)), this, SLOT(deleteButtonClicked()));
	connect(exportSelected, SIGNAL(clicked(bool)), this, SLOT(exportSelectedClicked()));
	connect(exportAll, SIGNAL(clicked(bool)), this, SLOT(exportAllClicked()));

	downloadsLayout->addWidget(downloadLabel, 0, 0, 1, 2);
	downloadsLayout->addWidget(downloadList, 1, 0, 1, 2);
	downloadsLayout->addWidget(deleteButton, 2, 0, 1, 1);
	downloadsLayout->addWidget(exportSelected, 2, 1, 1, 1);
	downloadsLayout->addWidget(exportAll, 3, 1, 1, 1);

	QGroupBox *settingsGroup = new QGroupBox("Download Settings");
	QVBoxLayout *settingsLayout = new QVBoxLayout(settingsGroup);
	downloadFrequency = new QComboBox();

	downloadFrequency->addItem("never", QVariant(0));
	downloadFrequency->addItem("6 hours", QVariant(360));
	downloadFrequency->addItem("12 hours", QVariant(720));
	downloadFrequency->addItem("1 day", QVariant(1440));
	downloadFrequency->addItem("2 days", QVariant(2880));
	downloadFrequency->addItem("5 days", QVariant(7200));
	downloadFrequency->addItem("10 days", QVariant(14400));
	downloadFrequency->addItem("30 days", QVariant(43200));

	int downloadInterval = settings->getDownloadInterval();
	downloadFrequency->setCurrentIndex(downloadFrequency->findData(QVariant(downloadInterval)));

	settingsLayout->addWidget(new QLabel("Auto-download interval"));
	settingsLayout->addWidget(downloadFrequency);
	settingsLayout->addStretch();


	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
	connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

	layout->addWidget(downloadsGroup, 0, 0, 2, 1);
	layout->addWidget(settingsGroup, 0, 1, 1, 1);
	layout->addWidget(buttonBox, 1, 1, 1, 1);
}

void DownloadOptionsDialog::refreshDownloadList() {
	downloadList->clear();
	bool success;
	QList<QPair<int, int64_t> > downloads = downloader->downloadsForSite(settings->getId(), success);
	for(int i = 0; i < downloads.size(); i++) {
		QDateTime localTime = QDateTime::fromTime_t(downloads[i].second).toLocalTime();
		QString formattedTime = localTime.toString("MM/dd/yyyy hh:mm");
		QListWidgetItem *newItem = new QListWidgetItem(formattedTime, downloadList);
		newItem->setData(Qt::UserRole, QVariant(downloads[i].first));
	}
}

void DownloadOptionsDialog::accept() {
	int index = downloadFrequency->currentIndex();
	if(index >= 0) {
		settings->setDownloadInterval(downloadFrequency->itemData(index).toInt());
	}

	settings->saveChanges();
	downloader->checkIfDownloadNeeded();
	QDialog::accept();
}

QList<int> DownloadOptionsDialog::getSelectedIds() {
	QList<int> result;

	QList<QListWidgetItem *> selected = downloadList->selectedItems();

	foreach(QListWidgetItem *item, selected) {
		result.append(item->data(Qt::UserRole).toInt());
	}
	return result;
}

QList<int> DownloadOptionsDialog::getAllIds() {
	QList<int> result;

	for(int i = 0; i < downloadList->count(); i++) {
		result.append(downloadList->item(i)->data(Qt::UserRole).toInt());
	}
	return result;
}

void DownloadOptionsDialog::deleteButtonClicked() {
	QMessageBox confirmation(QMessageBox::Warning, "Confirm Delete", "Are you sure you want to delete the selected download records?", QMessageBox::Yes | QMessageBox::Cancel, this);
	int ret = confirmation.exec();
	if(ret == QMessageBox::Cancel)
		return;

	QList<int> ids = getSelectedIds();
	downloader->deleteLoggedSet(ids);
	refreshDownloadList();
}

void DownloadOptionsDialog::exportSelectedClicked() {
	QList<int> ids = getSelectedIds();
	handleExport(ids);
}

void DownloadOptionsDialog::exportAllClicked() {
	QList<int> ids = getAllIds();
	handleExport(ids);
}

void DownloadOptionsDialog::handleExport(QList<int> & ids) {
	LoggedExportDialog dialog(downloader, ids, this);
	dialog.exec();
}


LoggedExportDialog::LoggedExportDialog(LoggedDownloader *downloader, QList<int> & ids, QWidget *parent) : QDialog(parent), downloader(downloader), ids(ids) {
	setWindowTitle("Export Logged Data");
	setMinimumWidth(500);
	setWindowModality(Qt::WindowModal);
    AppSettings *settings = AppSettings::getInstance();

	QVBoxLayout *layout = new QVBoxLayout(this);

	QFormLayout *optionsLayout = new QFormLayout();
	optionsLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

	QHBoxLayout *pathLayout = new QHBoxLayout();
	pathBox = new QLineEdit(QDir::toNativeSeparators(settings->getDownloadPath()));
	QPushButton *browseButton = new QPushButton("Browse...");
	connect(browseButton, SIGNAL(clicked()), this, SLOT(browseClicked()));

	pathLayout->addWidget(pathBox);
	pathLayout->addWidget(browseButton);

	QDateTime currentTime = QDateTime::currentDateTime();
	QString formattedTime = currentTime.toString("'PM'_MMM_dd_yyyy_hh_mm");

	nameBox = new QLineEdit(formattedTime);

	optionsLayout->addRow("Location", pathLayout);
	optionsLayout->addRow("File name", nameBox);

	layout->addLayout(optionsLayout);

	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
	layout->addWidget(buttonBox);

	connect(buttonBox, SIGNAL(accepted()), this, SLOT(exportClicked()));
	connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

void LoggedExportDialog::browseClicked() {
	QFileDialog bdialog(this, "Choose Download Location", pathBox->text());
	bdialog.setAcceptMode(QFileDialog::AcceptSave);
	bdialog.setOption(QFileDialog::ShowDirsOnly);
	bdialog.setFileMode(QFileDialog::Directory);

	if(bdialog.exec()) {
		QStringList selected = bdialog.selectedFiles();
		if(selected.size() >= 1) {
			pathBox->setText(selected[0]);
		}
	}
}

void LoggedExportDialog::exportClicked() {
	QString path = QDir::fromNativeSeparators(pathBox->text());
	QDir pathDir(path);
	if(!pathDir.exists()) {
		QMessageBox createDir(QMessageBox::Question, "Create Directory?", "The chosen destination directory does not exist. Would you like to create it?", QMessageBox::Yes | QMessageBox::No, this);
		if(createDir.exec() != QMessageBox::Yes)
			return;

		if(!pathDir.mkpath(".")) {
			QMessageBox failure(QMessageBox::Warning, "Error", "Failed to create the directory.", QMessageBox::Ok, this);
			failure.exec();
			return;
		}
	}
    AppSettings *settings = AppSettings::getInstance();
	settings->setDownloadPath(path);

	bool success;
	QList<QSharedPointer<LoggedData> > data = downloader->retrieveLoggedSet(ids, success);
	if(success) {
		QString baseName = QString("%1/%3").arg(path).arg(nameBox->text());
		if(LoggedData::writeRecordsToFiles(data, baseName)) {
			accept();
			return;
		}
	}
	QMessageBox failure(QMessageBox::Warning, "Error", "Unable to write output files", QMessageBox::Ok, this);
	failure.exec();
}
