#ifndef SITESDIALOG_H
#define SITESDIALOG_H

#include "programvalue.h"

#include <QDialog>
#include <QSet>

class SitesList;
class DataFetcher;

class QListWidget;
class QStackedWidget;
class QRadioButton;
class QLabel;
class QGroupBox;
class QComboBox;
class QLineEdit;

class SitesDialog : public QDialog {
	Q_OBJECT

public:
	SitesDialog(SitesList *sitesList, QWidget *parent = NULL);
	void accept();

	void setSitesWithDBData(QSet<int> & sitesWithDBData);

public slots:
	void typeChanged(bool isInternet);
	void siteChanged(int newRow);

	void addClicked();
	void removeClicked();
	void testButtonClicked();

	void programDataReady(QSharedPointer<ProgramValue> value);
	void connectionError();

	void nameEdited(const QString & text);

private:
	int minUnusedId;

	QListWidget *sitesListWidget;

	QGroupBox *controlsArea;
	QRadioButton *internetConnection, *serialConnection;
	QStackedWidget *typeSpecificOptions;
	QLineEdit *internetHost;
	QLineEdit *internetPort;
	QComboBox *serialPort;
	QLineEdit *siteName;
	QPushButton *testButton;
	QLabel *testStatus;

	SitesList *sitesList;
	QSet<int> sitesWithDBData;
	int currentRow;

	DataFetcher *fetcher;

	void fillSerialPorts();

	void saveSiteData(int id);
	void loadSiteData(int id);

	void resetTest();
	void clearControls();
};

#endif