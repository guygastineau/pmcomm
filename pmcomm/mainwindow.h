#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QObject>
#include <QPointer>

#include "programdialog.h"

class SiteManager;
class DisplayPanel;
class LoggedDownloader;
class SitesList;

class QPushButton;
class QComboBox;
class QCloseEvent;

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);

signals:
	void doConnect(QString host, uint16_t port);

public slots:
	void handleError();

	void programClicked();
	void downloadClicked();
	void manageSitesClicked();
	void globalOptionsClicked();
	void downloadOptionsClicked();

	void currentSiteChanged();

	void downloadDBError();

	void managersAdded(const QList<SiteManager *> & managers);

protected:
	virtual void closeEvent(QCloseEvent *event);

private:
	DisplayPanel *displayPanel;
   	SitesList *sitesList;
    QComboBox *siteSelection;
    LoggedDownloader *downloader;

    QPointer<ProgramDialog> programDialog;

	QMap<int, SiteManager *> managers;
	int currentManager;
	bool dbErrorCurrent;

	QPushButton *programButton;
	QPushButton *downloadButton;
	QPushButton *downloadOptionsButton;

	void setSitesAvailable();
};

#endif