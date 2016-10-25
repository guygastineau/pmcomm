#ifndef PROGRAMDIALOG_H
#define PROGRAMDIALOG_H

#include "pmdefs.h"
#include "programvalue.h"

#include <QObject>
#include <QDialog>
#include <QMap>
#include <QSet>

class ProgramPane;
class SiteManager;

class QListWidget;
class QStackedWidget;
class QLabel;
class QMessageBox;

class ProgramDialog : public QDialog {
	Q_OBJECT

public:
	ProgramDialog(SiteManager *manager, QWidget *parent = 0, Qt::WindowFlags f = 0);

	void exportAllToFile(QString & name);
	bool importAllFromFile(QString & name);

public slots:
	void programChanged(int index);

	void programUpdated(enum PMProgramNumber program);
	void writingFinished();
	void errorFinish();

	void batteryPresenceUpdated(int battery);
	void firmwareVersionUpdated();
	void programDataReset();

	void saveAndClose(bool close = false);
	bool closePrograms(); // Tries to close; returns true if actually closed

	void reject();

	void importClicked();
	void exportClicked();

	void closeDialogs();

private:
	void setRowEnabled(int row, bool enabled);
	bool isPaneDirty(ProgramPane *pane);

	SiteManager *manager;

	QListWidget *programsList;
	QStackedWidget *controlsArea;
	QLabel *inputUnitVersion;
	QLabel *interfaceUnitVersion;
	QMap<int, enum PMProgramNumber> programNumbers; // Allows converting indexes into program numbers

	QMap<enum PMProgramNumber, ProgramPane *> panes;

	bool shouldClose;
	QDialog *savingDialog;

	// Import/export stuff
	QString exportName;
	QSet<enum PMProgramNumber> exportWaitingList;
	QMap<enum PMProgramNumber, QSharedPointer<ProgramValue> > exportValues;
	bool allDirty; // All need to be written

	void finishExport();
	QDialog *exportingDialog;
};

#endif
