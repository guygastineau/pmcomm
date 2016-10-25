#include "loggeddownloaddialog.h"

#include "loggeddata.h"
#include "sitemanager.h"
#include "appsettings.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFormLayout>
#include <QCheckBox>
#include <QLineEdit>
#include <QFileDialog>
#include <QStringList>
#include <QDateTime>
#include <QString>
#include <QProgressBar>
#include <QLabel>
#include <QMessageBox>
#include <QDir>
#include <QLineEdit>
#include <QDialogButtonBox>

LoggedDownloadDialog::LoggedDownloadDialog(SiteManager *manager, QWidget *parent) : QDialog(parent), manager(manager) {
	canceled = false;
	setWindowTitle("Download Logged Data");
	setMinimumWidth(500);

	QVBoxLayout *layout = new QVBoxLayout(this);

	QFormLayout *optionsLayout = new QFormLayout();
	optionsLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

	QHBoxLayout *pathLayout = new QHBoxLayout();
	AppSettings *settings = AppSettings::getInstance();
	pathBox = new QLineEdit(QDir::toNativeSeparators(settings->getDownloadPath()));
	QPushButton *browseButton = new QPushButton("Browse...");
	connect(browseButton, SIGNAL(clicked()), this, SLOT(browseClicked()));

	pathLayout->addWidget(pathBox);
	pathLayout->addWidget(browseButton);

	QDateTime currentTime = QDateTime::currentDateTime();
	QString formattedTime = currentTime.toString("'PM'_MMM_dd_yyyy_hh_mm");

	nameBox = new QLineEdit(formattedTime);

	periodicBox = new QCheckBox("Periodic data");
	periodicBox->setChecked(true);
	profileBox = new QCheckBox("Battery discharge profile data");
	profileBox->setChecked(true);
	efficiencyBox = new QCheckBox("Efficiency Data");
	efficiencyBox->setChecked(true);

	optionsLayout->addRow("Location", pathLayout);
	optionsLayout->addRow("File name", nameBox);
	optionsLayout->addWidget(periodicBox);
	optionsLayout->addWidget(profileBox);
	optionsLayout->addWidget(efficiencyBox);

	layout->addLayout(optionsLayout);

	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
	layout->addWidget(buttonBox);

	connect(buttonBox, SIGNAL(accepted()), this, SLOT(downloadClicked()));
	connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

void LoggedDownloadDialog::downloadClicked() {
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

	connect(manager, SIGNAL(loggedDataDownloaded()), this, SLOT(downloaded()));
	connect(manager, SIGNAL(errorCanceled()), this, SLOT(errorCanceled()));

	if(!manager->downloadLogged(periodicBox->isChecked(), profileBox->isChecked(), efficiencyBox->isChecked())) {
		disconnect(manager, 0, this, 0);
		QMessageBox failure(QMessageBox::Warning, "Error", "Logged data is already being downloaded from this site in the background. Please wait and try again.", QMessageBox::Ok, this);
		failure.exec();
		return;
	}

	canceled = false;
	progress = new LoggedProgressDialog(this);
	progress->setWindowModality(Qt::WindowModal);

	connect(manager, SIGNAL(loggedDataProgress(LoggedValue::LoggedDataType, int, int)), progress, SLOT(setProgress(LoggedValue::LoggedDataType, int, int)));
	connect(progress, SIGNAL(downloadCanceled()), this, SLOT(cancelDownload()));

	progress->show();
}

void LoggedDownloadDialog::cancelDownload() {
	manager->cancelLoggedDownload();
	canceled = true;
}

void LoggedDownloadDialog::downloaded() {
	if(!progress)
		return;

	progress->accept();
	progress->deleteLater();
	progress = NULL;

	disconnect(manager, 0, this, 0);

	if(canceled)
		return;

	QString errorMessage("");
	QSharedPointer<LoggedData> data = manager->retrieveLogged();
	bool success = true;
	if(data == NULL) {
		errorMessage = "Unable to read data from PentaMetric";
		success = false;
	}

	if(success) {
		QString path = QDir::fromNativeSeparators(pathBox->text());
		QString baseName = QString("%1/%3").arg(path).arg(nameBox->text());
		if(!data->writeToFiles(baseName)) {
			errorMessage = "Unable to write output files";
			success = false;
		}
	}

	QMessageBox successBox(this);
	if(success) {
		successBox.setText("The download was successful.");
	} else {
		successBox.setText(errorMessage);
	}
	successBox.exec();

	if(success)
		accept();

}

void LoggedDownloadDialog::errorCanceled() {
	if(progress) {
		progress->reject();
		progress->deleteLater();
		progress = NULL;

		disconnect(manager, 0, this, 0);
	}
	reject();
}

void LoggedDownloadDialog::browseClicked() {
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


LoggedProgressDialog::LoggedProgressDialog(QWidget *parent) : QDialog(parent) {
	setMinimumWidth(300);
	canceling = false;
	QVBoxLayout *layout = new QVBoxLayout(this);

	status = new QLabel("Preparing to download...");
	progressBar = new QProgressBar();
	progressBar->setMinimum(0);
	buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel);
	connect(buttonBox, SIGNAL(rejected()), this, SLOT(cancelDownload()));

	layout->addWidget(status);
	layout->addWidget(progressBar);
	layout->addWidget(buttonBox);
}

void LoggedProgressDialog::setProgress(LoggedValue::LoggedDataType type, int progress, int outof) {
	if(!canceling) {
		switch(type) {
			case LoggedValue::TYPE_PERIODIC:
				status->setText("Downloading periodic data...");
				break;
			case LoggedValue::TYPE_PROFILE:
				status->setText("Downloading profile data...");
				break;
			case LoggedValue::TYPE_EFFICIENCY:
				status->setText("Downloading efficiency data...");
				break;
		}
	}

	progressBar->setMaximum(outof);
	progressBar->setValue(progress);
}

void LoggedProgressDialog::cancelDownload() {
	status->setText("Canceling...");
	buttonBox->button(QDialogButtonBox::Cancel)->setEnabled(false);
	canceling = true;
	emit downloadCanceled();
}




