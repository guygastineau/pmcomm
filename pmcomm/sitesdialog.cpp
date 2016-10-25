#include "sitesdialog.h"

#include "siteslist.h"
#include "datafetcher.h"

#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QRadioButton>
#include <QStackedWidget>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QMessageBox>
#include <QListWidget>
#include <QLabel>
#include <QComboBox>
#include "qextserialenumerator.h" // from qextserialport

#include <QDebug>

#define READY_TEST_LABEL "Ready for test"
#define TEST_BUTTON_LABEL "Test connection"

SitesDialog::SitesDialog(SitesList *sitesList, QWidget *parent) : QDialog(parent) {
	setWindowTitle("Manage sites");
	setWindowModality(Qt::ApplicationModal);
	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
	this->sitesList = sitesList;
	fetcher = NULL;
	QGridLayout *layout = new QGridLayout(this);

	sitesListWidget = new QListWidget;
	sitesListWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

	QSharedPointer<SiteSettings> site;
	foreach(site, sitesList->getSites().values()) {
		QListWidgetItem *item = new QListWidgetItem(site->getName(), sitesListWidget);
		item->setData(Qt::UserRole, QVariant::fromValue(site));
	}
	sitesListWidget->sortItems();
	currentRow = -1;

	connect(sitesListWidget, SIGNAL(currentRowChanged(int)), this, SLOT(siteChanged(int)));

	controlsArea = new QGroupBox("Site Settings");
	QFormLayout *controlsLayout = new QFormLayout(controlsArea);

	siteName = new QLineEdit();
	controlsLayout->addRow("Site name", siteName);

	connect(siteName, SIGNAL(textEdited(const QString &)), this, SLOT(nameEdited(const QString &)));

	QGroupBox *connectionTypeBox = new QGroupBox("Connection Type");
	QVBoxLayout *typeOptions = new QVBoxLayout(connectionTypeBox);
	QHBoxLayout *connectionTypeLayout = new QHBoxLayout();
	serialConnection = new QRadioButton("Serial/USB");
	serialConnection->setChecked(true);
	internetConnection = new QRadioButton("TCP/IP (Ethernet)");
	connectionTypeLayout->addWidget(serialConnection);
	connectionTypeLayout->addWidget(internetConnection);
	typeOptions->addLayout(connectionTypeLayout);

	connect(internetConnection, SIGNAL(toggled(bool)), this, SLOT(typeChanged(bool)));

	typeSpecificOptions = new QStackedWidget();
	QWidget *serialSettings = new QWidget();
	QFormLayout *serialLayout = new QFormLayout(serialSettings);

	serialPort = new QComboBox();
	serialPort->setEditable(true);
	fillSerialPorts();
	serialLayout->addRow("Serial port", serialPort);

	typeSpecificOptions->addWidget(serialSettings);

	QWidget *internetSettings = new QWidget();
	QFormLayout *internetLayout = new QFormLayout(internetSettings);

	internetHost = new QLineEdit();
	internetLayout->addRow("Host/IP Address", internetHost);

	internetPort = new QLineEdit();
	QValidator *portValidator = new QIntValidator(0, 65535, this);
	internetPort->setValidator(portValidator);
	internetLayout->addRow("Port", internetPort);

	typeSpecificOptions->addWidget(internetSettings);
	typeOptions->addWidget(typeSpecificOptions);
	controlsLayout->addRow(connectionTypeBox);

	testButton = new QPushButton(TEST_BUTTON_LABEL);
	controlsLayout->addRow(testButton);
	testStatus = new QLabel(READY_TEST_LABEL);
	controlsLayout->addRow("Test status:", testStatus);

	connect(testButton, SIGNAL(clicked()), this, SLOT(testButtonClicked()));

	QHBoxLayout *addRemoveLayout = new QHBoxLayout();
	QPushButton *addButton = new QPushButton("Add");
	QPushButton *removeButton = new QPushButton("Remove");
	addRemoveLayout->addWidget(addButton);
	addRemoveLayout->addWidget(removeButton);

	connect(addButton, SIGNAL(clicked()), this, SLOT(addClicked()));
	connect(removeButton, SIGNAL(clicked()), this, SLOT(removeClicked()));

	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
	connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

	layout->addWidget(sitesListWidget, 0, 0);
	layout->addLayout(addRemoveLayout, 1, 0);
	layout->addWidget(controlsArea, 0, 1, 2, 1);
	layout->addWidget(buttonBox, 2, 0, 1, 2);

	if(sitesListWidget->count() > 0)
		sitesListWidget->setCurrentRow(0);
	else
		clearControls();
}

// Fills in the list of available serial ports
void SitesDialog::fillSerialPorts() {
	QList<QextPortInfo> ports = QextSerialEnumerator::getPorts();
	foreach(QextPortInfo port, ports) {
#ifdef Q_OS_LINUX // I don't know why qextserialport behaves differently here
		QString portName = port.physName;
#else
		QString portName = port.portName;
#endif
		if(portName != "") {
			serialPort->addItem(portName);
		}
	}
}

void SitesDialog::setSitesWithDBData(QSet<int> & sitesWithDBData) {
	this->sitesWithDBData = sitesWithDBData;
}

void SitesDialog::typeChanged(bool isInternet) {
	typeSpecificOptions->setCurrentIndex(isInternet ? 1 : 0);
}

void SitesDialog::siteChanged(int newRow) {
	if(fetcher != NULL) {
		testStatus->setText(READY_TEST_LABEL);
		resetTest();
	}

	if(currentRow >= 0)
		saveSiteData(currentRow);

	currentRow = newRow;
	// qDebug() << "Current row: " << currentRow;
	if(currentRow >= 0)
		loadSiteData(currentRow);
}

void SitesDialog::nameEdited(const QString & text) {
	int row = sitesListWidget->currentRow();
	if(row < 0)
		return;

	sitesListWidget->item(row)->setData(Qt::DisplayRole, text);	
}

void SitesDialog::loadSiteData(int row) {
	serialPort->clear(); // Totally reload the serial port box (regardless of whether we're using it now)
	fillSerialPorts();

	QSharedPointer<SiteSettings> settings = sitesListWidget->item(row)->data(Qt::UserRole).value<QSharedPointer<SiteSettings> >();
	if(settings->isInternet()) {
		internetConnection->setChecked(true);
		internetHost->setText(settings->getHostname());
		internetPort->setText(QString::number(settings->getPort()));
		if(serialPort->count() != 0) { // Choose the first port by default
			serialPort->setCurrentIndex(0);
		} else { // No ports available
			serialPort->setCurrentIndex(-1);
		}
	} else {
		serialConnection->setChecked(true);
		QString serialName = settings->getSerialPort();
		int serialIndex = serialPort->findText(serialName);
		if(serialIndex >= 0) { // Found
			serialPort->setCurrentIndex(serialIndex);
		} else { // Not found
			serialPort->setEditText(serialName);
		}

#ifdef Q_OS_WIN // NetBIOS only works correctly on Windows
		internetHost->setText("PENTAMETRIC1");
#else
		internetHost->setText("169.254.1.1");
#endif
		internetPort->setText(QString::number(1701));
	}

	siteName->setText(settings->getName());	
}

void SitesDialog::saveSiteData(int row) {
	QSharedPointer<SiteSettings> oldSettings = sitesListWidget->item(row)->data(Qt::UserRole).value<QSharedPointer<SiteSettings> >();

	SiteSettings *newSettings = NULL;
	if(internetConnection->isChecked()) { // Internet
		newSettings = new SiteSettings(oldSettings->getId(), internetHost->text(), internetPort->text().toInt());
	} else {
		newSettings = new SiteSettings(oldSettings->getId(), serialPort->currentText());
	}

	newSettings->setName(siteName->text());

	sitesListWidget->item(row)->setData(Qt::UserRole, QVariant::fromValue(QSharedPointer<SiteSettings>(newSettings)));
}

void SitesDialog::accept() {
	if(sitesListWidget->currentRow() >= 0)
		saveSiteData(sitesListWidget->currentRow());

	QList<QSharedPointer<SiteSettings> > newSites;
	for(int i = 0; i < sitesListWidget->count(); i++) {
		QSharedPointer<SiteSettings> settings = sitesListWidget->item(i)->data(Qt::UserRole).value<QSharedPointer<SiteSettings> >();

		QSharedPointer<SiteSettings> otherSite;
		foreach(otherSite, newSites) {
			if(settings->conflictsWith(*otherSite)) {
				sitesListWidget->setCurrentRow(i);

    			QMessageBox errorBox(this);
    			errorBox.setText(QString("Site \"%1\" has the same connection settings as site \"%2\"").arg(settings->getName()).arg(otherSite->getName()));
    			errorBox.setStandardButtons(QMessageBox::Ok);
    			errorBox.exec();
        		return;
			}
			if(settings->getName() == otherSite->getName()) {
				sitesListWidget->setCurrentRow(i);

    			QMessageBox errorBox(this);
    			errorBox.setText(QString("There are two sites named \"%1\"").arg(settings->getName()));
    			errorBox.setStandardButtons(QMessageBox::Ok);
    			errorBox.exec();
        		return;
			}
		}

		newSites.append(settings);
	}
	sitesList->setSites(newSites);

	QDialog::accept();
}

void SitesDialog::addClicked() {
	QSharedPointer<SiteSettings> defaultSettings = QSharedPointer<SiteSettings>(new SiteSettings(-1, "COM1"));
	defaultSettings->setName("New Site");

	int newRow = sitesListWidget->count();
	if(currentRow >= 0) // Avoid clobbering the new value
		saveSiteData(currentRow);
	currentRow = -1;

	QListWidgetItem *item = new QListWidgetItem(defaultSettings->getName(), sitesListWidget);
	item->setData(Qt::UserRole, QVariant::fromValue(defaultSettings));

	sitesListWidget->setCurrentRow(newRow);
	controlsArea->setEnabled(true);
}

void SitesDialog::removeClicked() {
	int row = sitesListWidget->currentRow();
	if(row < 0) {
		return;
	}

	QSharedPointer<SiteSettings> oldSettings = sitesListWidget->item(row)->data(Qt::UserRole).value<QSharedPointer<SiteSettings> >();
	if(sitesWithDBData.contains(oldSettings->getId())) {
		QMessageBox warningBox(this);
		warningBox.setText("This site has logged data stored in the PMComm database. If you remove the site, this data will be deleted. Please export it first if you want to keep it. Do you want to remove the site anyway?");
		warningBox.addButton(QMessageBox::Cancel);
		QAbstractButton *deleteButton = warningBox.addButton("Remove anyway", QMessageBox::DestructiveRole);
		warningBox.exec();
		if(warningBox.clickedButton() != deleteButton)
			return;
	}
	
	delete sitesListWidget->item(row);
	currentRow = -1;
	if(sitesListWidget->count() > 0) {
		siteChanged(sitesListWidget->currentRow());
	} else {
		clearControls();
	}
}

void SitesDialog::clearControls() {
	controlsArea->setEnabled(false);
	internetHost->setText("");
	internetPort->setText("");
	serialPort->setCurrentIndex(-1);
	siteName->setText("");
	serialConnection->setChecked(true);
}

void SitesDialog::testButtonClicked() {
	if(fetcher != NULL) {
		// "Cancel" button
		testStatus->setText("Canceled");
		resetTest();
		return;
	}

	int row = sitesListWidget->currentRow();
	if(row < 0)
		return;
	saveSiteData(row);

	testStatus->setText("Testing...");
	testButton->setText("Cancel test");

	fetcher = new DataFetcher(sitesListWidget->item(row)->data(Qt::UserRole).value<QSharedPointer<SiteSettings> >().data(), this);

	connect(fetcher, SIGNAL(programDataReady(QSharedPointer<ProgramValue>, int)), this, SLOT(programDataReady(QSharedPointer<ProgramValue>)));
	connect(fetcher, SIGNAL(programDataFetchError(enum PMProgramNumber, int)), this, SLOT(connectionError()));
	connect(fetcher, SIGNAL(connectionError(int)), this, SLOT(connectionError()));

	fetcher->fetchProgramData(PM_P_VERSION, 0);
}

void SitesDialog::programDataReady(QSharedPointer<ProgramValue> value) {
	Q_UNUSED(value);
	if(fetcher == NULL)
		return;

	testStatus->setText("Success!");
	resetTest();
}

void SitesDialog::connectionError() {
	if(fetcher == NULL)
		return;

	testStatus->setText("FAILED");
	resetTest();	
}

void SitesDialog::resetTest() {
	disconnect(fetcher, 0, this, 0);
	testButton->setText(TEST_BUTTON_LABEL);

	fetcher->deleteLater();
	fetcher = NULL;
}

