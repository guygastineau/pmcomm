#include "programdialog.h"

#include "programpanes.h"
#include "sitemanager.h"
#include "appsettings.h"

#include <QDialogButtonBox>
#include <QListWidget>
#include <QStackedWidget>
#include <QLabel>
#include <QSettings>
#include <QFileDialog>

// The way to get standard locations changed between Qt4 and Qt5
#if QT_VERSION >= 0x050000
#include <QStandardPaths>
#else
#include <QDesktopServices>
#endif

#include <QDebug>

ProgramDialog::ProgramDialog(SiteManager *manager, QWidget *parent, Qt::WindowFlags f) : QDialog(parent, f) {
	setWindowTitle("Program the PentaMetric");
	setMinimumSize(900, 500);
	exportingDialog = NULL;
	savingDialog = NULL;
	allDirty = false;
	shouldClose = false;
	this->manager = manager;
	manager->updateProgramData();
	QGridLayout *layout = new QGridLayout(this);

	programsList = new QListWidget;
	programsList->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
	programsList->setMinimumWidth(300);
	controlsArea = new QStackedWidget;

	QList<enum PMProgramNumber> progNumbers;

	progNumbers << PM_P1 << PM_P2 << PM_P3 << PM_P4 << PM_P5 << PM_P6_9 << PM_P11_13 << PM_P14 << PM_P15 << 
	PM_P16 << PM_P22_23 << PM_P24_25 << PM_P26 << PM_P27 << PM_P28 << PM_P29 << PM_P30_31 << PM_P32 << PM_P33 <<
	PM_P34_35 << PM_P36 << PM_P37 << PM_P38 << PM_P39 << PM_P43 << PM_P45_49 << PM_P_TCP;

	int index = 0;
	foreach(enum PMProgramNumber num, progNumbers) {
		ProgramPane *pane = ProgramPane::makeProgramPane(num, manager);
		pane->setEnabled(!pane->needsRead());
		panes[num] = pane;
		programNumbers[index] = num;

		programsList->addItem(pane->getName());

		int battery = pane->getBatteryNum();
		if(num != PM_P14 && num != PM_P15 && battery != 0 && !manager->batteryPresent(battery))
			setRowEnabled(index, false);
		if(num == PM_P_TCP)
			setRowEnabled(index, false);

		QWidget *withHelp = new QWidget;
		QVBoxLayout *withHelpLayout = new QVBoxLayout(withHelp);
		withHelpLayout->addWidget(pane, 1);

		// Add the help text
		QString helpString = pane->helpText();
		if(helpString != "") {
			QLabel *helpLabel = new QLabel(helpString);
			helpLabel->setWordWrap(true);
			withHelpLayout->addWidget(helpLabel);
		}
		withHelpLayout->addStretch();

		controlsArea->addWidget(withHelp);

		index++;
	}

	connect(static_cast<CapacityProgramPane *>(panes[PM_P14]), SIGNAL(batteryPresenceUpdated(int, bool)), this, SLOT(batteryPresenceUpdated(int)));
	connect(static_cast<CapacityProgramPane *>(panes[PM_P15]), SIGNAL(batteryPresenceUpdated(int, bool)), this, SLOT(batteryPresenceUpdated(int)));

	connect(manager, SIGNAL(programUpdated(enum PMProgramNumber)), this, SLOT(programUpdated(enum PMProgramNumber)));
	connect(manager, SIGNAL(writingFinished()), this, SLOT(writingFinished()));
	connect(manager, SIGNAL(errorCanceled()), this, SLOT(errorFinish()));
	connect(manager, SIGNAL(batteryPresenceUpdated(int, bool)), this, SLOT(batteryPresenceUpdated(int)));
	connect(manager, SIGNAL(versionUpdated()), this, SLOT(firmwareVersionUpdated()));
	connect(manager, SIGNAL(programDataReset()), this, SLOT(programDataReset()));

	connect(programsList, SIGNAL(currentRowChanged(int)), this, SLOT(programChanged(int)), Qt::QueuedConnection);
	connect(this, SIGNAL(finished(int)), this, SLOT(closeDialogs()));
	programsList->setCurrentRow(0);

	QHBoxLayout *bottomLayout = new QHBoxLayout();
	inputUnitVersion = new QLabel();
	interfaceUnitVersion = new QLabel();
	bottomLayout->addWidget(inputUnitVersion);
	bottomLayout->addSpacing(20);
	bottomLayout->addWidget(interfaceUnitVersion);

	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::SaveAll | QDialogButtonBox::Close);
	QPushButton *importButton = new QPushButton("Import settings...");
	connect(importButton, SIGNAL(clicked()), this, SLOT(importClicked()));
	buttonBox->addButton(importButton, QDialogButtonBox::ActionRole);
	QPushButton *exportButton = new QPushButton("Export settings...");
	connect(exportButton, SIGNAL(clicked()), this, SLOT(exportClicked()));
	buttonBox->addButton(exportButton, QDialogButtonBox::ActionRole);
	connect(buttonBox, SIGNAL(accepted()), this, SLOT(saveAndClose()));
	connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
	bottomLayout->addWidget(buttonBox);

	layout->addWidget(programsList, 0, 0);
	layout->addWidget(controlsArea, 0, 1);
	layout->addLayout(bottomLayout, 1, 0, 1, 2);

	firmwareVersionUpdated();
}

void ProgramDialog::closeDialogs() {
	delete savingDialog;
	savingDialog = NULL;
	delete exportingDialog;
	exportingDialog = NULL;
}

void ProgramDialog::programChanged(int index) {
	int currIndex = controlsArea->currentIndex();

	if(index < 0)
		return;

	// Verify the data from the previous pane
	ProgramPane *currPane = panes[programNumbers[currIndex]];
	if(index != currIndex && isPaneDirty(currPane) && !currPane->isValid()) {
		qWarning() << "Bad program data!";
		programsList->setCurrentRow(currIndex);
		panes[programNumbers[currIndex]]->handleInvalid();
		return;
	}

	controlsArea->setCurrentIndex(index);
	enum PMProgramNumber progNum = programNumbers[index];

	if(panes[progNum]->needsRead() && (!manager->getProgram(progNum) || !isPaneDirty(panes[progNum])))
		manager->fetchProgram(progNum);
}

void ProgramDialog::programUpdated(enum PMProgramNumber program) {
	if(panes.contains(program)) {
		panes[program]->setEnabled(true);
		panes[program]->setData(QSharedPointer<ProgramValue>(manager->getProgram(program)->clone()));
		panes[program]->loadData();
	}

	if(exportWaitingList.contains(program)) {
		exportWaitingList.remove(program);
		exportValues[program] = manager->getProgram(program);
		if(exportWaitingList.empty())
			finishExport();
	}
}

void ProgramDialog::batteryPresenceUpdated(int battery) {
	ProgramPane *pane = NULL;
	if(battery == 1)
		pane = panes[PM_P14];
	else if(battery == 2)
		pane = panes[PM_P15];
	else
		return;

	bool present = manager->batteryPresent(battery);
	if(isPaneDirty(pane) && pane->isValid()) {
		present = static_cast<CapacityProgramPane *>(pane)->batteryEnabled();
	}

	foreach(int i, programNumbers.keys()) {
		enum PMProgramNumber progNum = programNumbers[i];

		if(progNum != PM_P14 && progNum != PM_P15 && panes[progNum]->getBatteryNum() == battery) {
			setRowEnabled(i, present);
		}
	}
}

void ProgramDialog::firmwareVersionUpdated() {
	QPair<int, int> & v = manager->firmwareVersion();
	bool versionValid = v.first >= 0;
	bool version2 = v.first >= 2;
	// Need to find the right row, since there isn't a data structure for that
	if(versionValid) {
		foreach(int i, programNumbers.keys()) {
			if(programNumbers[i] == PM_P_TCP)
				setRowEnabled(i, version2);
		}
		static_cast<ShuntTypesProgramPane *>(panes[PM_P11_13])->setAmps2SumEnabled(version2);
	}

	QString inputVersion("?");
	if(v.first == 0) // Treat version 0 as 1
		inputVersion = "1.0";
	if(v.first >= 1)
		inputVersion = QString("%1.%2").arg(v.first).arg(v.second);
	inputUnitVersion->setText(QString("Input Unit version: %1").arg(inputVersion));

	QString interfaceVersion("None");
	QPair<int, int> & interfaceV = manager->interfaceFirmwareVersion();
	if(interfaceV.first != 0 || interfaceV.second != 0)
		interfaceVersion = QString("%1.%2").arg(interfaceV.first).arg(interfaceV.second);
	interfaceUnitVersion->setText(QString("Ethernet Interface version: %1").arg(interfaceVersion));
}

void ProgramDialog::setRowEnabled(int row, bool enabled) {
	QListWidgetItem *currItem = programsList->item(row);

	Qt::ItemFlags flags = currItem->flags();
	if(enabled)
		flags |= Qt::ItemIsEnabled;
	else
		flags &= ~Qt::ItemIsEnabled;
	currItem->setFlags(flags);
}
	
void ProgramDialog::programDataReset() {
	int currIndex = controlsArea->currentIndex();
	if(currIndex < 0)
		return;

	enum PMProgramNumber progNum = programNumbers[currIndex];

	if(panes[progNum]->needsRead())
		manager->fetchProgram(progNum);
}

void ProgramDialog::writingFinished() {
	delete savingDialog;
	savingDialog = NULL;

	if(shouldClose)
		accept();
}

void ProgramDialog::errorFinish() {
	accept();
}

void ProgramDialog::reject() {
	closePrograms();
}

bool ProgramDialog::closePrograms() {
	if(!isVisible())
		return true;

	bool dirty = false;
	foreach(enum PMProgramNumber num, panes.keys()) {
		if(isPaneDirty(panes[num])) {
			dirty = true;
			break;
		}
	}

	if(!dirty) {
		QDialog::reject();
		return true;
	}

	QMessageBox saveBox(this);
	saveBox.setText("Would you like to save your changes?");
	saveBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
	saveBox.setWindowModality(Qt::WindowModal);
	switch(saveBox.exec()) {
		case QMessageBox::Yes:
			saveAndClose(true);
			return false;
		case QMessageBox::No:
			QDialog::reject();
			break;
		case QMessageBox::Cancel:
			return false;
	}
	return true;
}

void ProgramDialog::saveAndClose(bool close) {
	int currIndex = controlsArea->currentIndex();

	// Verify the data from the previous pane
	if(!panes[programNumbers[currIndex]]->isValid()) {
		qWarning() << "Bad data in program pane!";
		programsList->setCurrentRow(currIndex);
		panes[programNumbers[currIndex]]->handleInvalid();
		return;
	}

	shouldClose = close; // Remember whether we want to close
	bool anyDirty = false;
	foreach(enum PMProgramNumber num, panes.keys()) {
		// Avoid saving to locations that shouldn't be written on old units
		if(isPaneDirty(panes[num])) {
			 if(num == PM_P_TCP && manager->firmwareVersion().first < 2)
			 	continue;

			panes[num]->saveData();
			QSharedPointer<ProgramValue> value = panes[num]->getData();
			manager->saveProgramData(value);
			anyDirty = true;
		}
	}
	allDirty = false;

	if(shouldClose && !anyDirty) {
		accept();
	}

	if(anyDirty) {
		// Put up the dialog
		savingDialog = new QDialog(this);
		savingDialog->setWindowFlags(savingDialog->windowFlags() & ~Qt::WindowCloseButtonHint);
		savingDialog->setWindowModality(Qt::WindowModal);
		savingDialog->setMinimumSize(200, 75);
		QHBoxLayout *layout = new QHBoxLayout(savingDialog);
		layout->addWidget(new QLabel("Saving, please wait..."));
		savingDialog->show();
	}
}

// Determines if a pane is dirty
bool ProgramDialog::isPaneDirty(ProgramPane *pane) {
	return pane->isDirty() || (allDirty && pane->getData());
}

void ProgramDialog::exportAllToFile(QString & name) {
	// Make sure the data is valid
	int currIndex = controlsArea->currentIndex();
	if(!panes[programNumbers[currIndex]]->isValid()) {
		qWarning() << "Bad data in program pane!";
		programsList->setCurrentRow(currIndex);
		panes[programNumbers[currIndex]]->handleInvalid();
		return;
	}

	// Put up the dialog
	exportingDialog = new QDialog(this);
	exportingDialog->setWindowFlags(exportingDialog->windowFlags() & ~Qt::WindowCloseButtonHint);
	exportingDialog->setWindowModality(Qt::WindowModal);
	exportingDialog->setMinimumSize(200, 75);
	QHBoxLayout *layout = new QHBoxLayout(exportingDialog);
	layout->addWidget(new QLabel("Exporting, please wait..."));
	exportingDialog->show();

	exportWaitingList.clear();
	exportValues.clear();
	exportName = name;

	foreach(enum PMProgramNumber num, panes.keys()) {
		if(num == PM_P45_49) // Skip resets
			continue;

		if(isPaneDirty(panes[num])) {
			panes[num]->saveData();
			exportValues[num] = panes[num]->getData();
		} else {
			exportWaitingList.insert(num);
			manager->fetchProgram(num);
		}
	}

	if(exportWaitingList.empty())
		finishExport();
}

bool ProgramDialog::importAllFromFile(QString & name) {
	QSettings importFile(name, QSettings::IniFormat);
	importFile.beginGroup("Programs");

	QPair<int, int> & version = manager->firmwareVersion();

	QList<QSharedPointer<ProgramValue> > newValues;
	foreach(QString key, importFile.allKeys()) {
		enum PMProgramNumber num = ProgramValue::getNumber(key);
		if(num == PM_P45_49) // Skip resets
			continue;
		if(num == PM_P38) // Skip time
			continue;

		if(!panes.contains(num)) { // Invalid setting
			importFile.endGroup();
			return false;
		}

		QSharedPointer<ProgramValue> value(ProgramValue::makeProgramValue(num, importFile.value(key)));
		if(value == NULL) {
			importFile.endGroup();
			return false;
		}
		if(num == PM_P11_13 && version.first >= 0 && version.first < 2) // Disable amps summing on old (but valid) versions
			value.staticCast<ShuntTypesProgramValue>()->setAmps2Sum(false);

		newValues.append(value);
	}
	importFile.endGroup();

	if(newValues.empty() || importFile.status() != QSettings::NoError) // Make sure something was found
		return false;

	// If here, no error occurred
	QSharedPointer<ProgramValue> value;
	foreach(value, newValues) {
		enum PMProgramNumber num = value->progNum();
		panes[num]->setEnabled(true);
		panes[num]->setData(value); // Set it as the model
		panes[num]->loadData(); // Load it into the widgets
	}

	allDirty = true;
	batteryPresenceUpdated(1);
	batteryPresenceUpdated(2);
	return true;
}

void ProgramDialog::finishExport() {
	QFile exportF(exportName);
	if(exportF.exists())
		exportF.remove(); // The QFileDialog already warns the user their file will be overwritten

	QSettings exportFile(exportName, QSettings::IniFormat);
	exportFile.beginGroup("Programs");

	foreach(enum PMProgramNumber num, exportValues.keys()) {
		exportFile.setValue(ProgramValue::getName(num), exportValues[num]->toVariant());
	}

	exportFile.endGroup();
	exportFile.sync();

	delete exportingDialog;
	exportingDialog = NULL;

	if(exportFile.status() == QSettings::NoError) {
		QMessageBox successMsg(QMessageBox::Information, "Success", "Settings were exported successfully", QMessageBox::Ok, this);
		successMsg.setWindowModality(Qt::WindowModal);
		successMsg.exec();
	} else {
		QMessageBox errorMsg(QMessageBox::Warning, "Error", "Unable to save settings", QMessageBox::Ok, this);
		errorMsg.setWindowModality(Qt::WindowModal);
		errorMsg.exec();
	}
}

void ProgramDialog::exportClicked() {
	AppSettings *settings = AppSettings::getInstance();

	QString defaultPath = settings->getSettingsExportPath();
	qDebug() << "Default path:" << defaultPath;
	defaultPath.append("/pmsettings.ini");

	QFileDialog exportDialog(this, "Save settings file", defaultPath);
	exportDialog.setAcceptMode(QFileDialog::AcceptSave);
	exportDialog.setFileMode(QFileDialog::AnyFile);
	exportDialog.setDefaultSuffix("ini");

	if(exportDialog.exec()) {
		settings->setSettingsExportPath(exportDialog.directory().absolutePath());
		QString path = exportDialog.selectedFiles()[0];

		exportAllToFile(path);
	}
}

void ProgramDialog::importClicked() {
	AppSettings *settings = AppSettings::getInstance();

	QString defaultPath = settings->getSettingsExportPath();
	qDebug() << "Default path:" << defaultPath;

	QFileDialog importDialog(this, "Choose settings file", defaultPath, "Settings files (*.ini)");
	importDialog.setFileMode(QFileDialog::ExistingFile);

	if(importDialog.exec()) {
		settings->setSettingsExportPath(importDialog.directory().absolutePath());
		bool success = importAllFromFile(importDialog.selectedFiles()[0]);
		if(success) {
			QMessageBox successMsg(QMessageBox::Information, "Success", "Settings were imported successfully", QMessageBox::Ok, this);
			successMsg.setWindowModality(Qt::WindowModal);
			successMsg.exec();
		} else {
			QMessageBox errorMsg(QMessageBox::Warning, "Error", "Unable to import settings. Please make sure the file you chose is valid", QMessageBox::Ok, this);
			errorMsg.setWindowModality(Qt::WindowModal);
			errorMsg.exec();
		}
	}
}
