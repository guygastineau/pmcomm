#ifndef LOGGEDDOWNLOADER_H
#define LOGGEDDOWNLOADER_H

#include "pmdefs.h"
#include "loggeddata.h"
#include "sitemanager.h"

#include <QObject>
#include <QSqlDatabase>
#include <QMap>
#include <QList>
#include <QPair>
#include <QSharedPointer>

class SiteManager;
class SitesList;

class QTimer;

class LoggedDownloader : public QObject {
	Q_OBJECT

public:
	LoggedDownloader(SitesList *sitesList, QObject *parent = NULL);
	~LoggedDownloader();

	bool isInitialized() { return initialized; }

	QList<QPair<int, int64_t> > downloadsForSite(int site, bool & success);
	QList<QSharedPointer<LoggedData> > retrieveLoggedSet(QList<int> & sites, bool & success);
	bool deleteLoggedSet(QList<int> & ids);

	void checkIfDownloadNeeded();

public slots:
	void checkTime();
	void downloadFinished();
	void errorCanceled();
	void siteStatusChanged(SiteManager::Status newStatus);

	bool downloadNow(SiteManager *manager);

	void managersAdded(const QList<SiteManager *> & managers);
	void managersRemoved(const QList<SiteManager *> & managers);


signals:
	void downloadDBError();

private:
	void retrieveDownloadTimes();

	struct DownloadStats {
		int64_t retryTime;
		int64_t lastDownloadTime;
		bool forceReconnect; // Set to true to ensure reconnection
	};

	QSqlDatabase db;

	SitesList *sitesList;
	SiteManager *currManager;
	QMap<int, DownloadStats> stats;

	QTimer *downloadTimer;
	bool initialized;
};


#endif