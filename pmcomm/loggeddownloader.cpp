#include "loggeddownloader.h"

#include "siteslist.h"

#include <QString>
#include <QDir>
#include <QSqlQuery>
#include <QDateTime>
#include <QTimer>

#include <QDebug>

// The way to get standard locations changed between Qt4 and Qt5
#if QT_VERSION >= 0x050000
#include <QStandardPaths>
#else
#include <QDesktopServices>
#endif

static const int retryDelaySecs = 3600; // After this time, try to reconnect

LoggedDownloader::LoggedDownloader(SitesList *sitesList, QObject *parent) : QObject(parent) {
	initialized = false;
	currManager = NULL;
	downloadTimer = new QTimer(this);
	downloadTimer->setSingleShot(true);
	connect(downloadTimer, SIGNAL(timeout()), this, SLOT(checkTime()));

	this->sitesList = sitesList;

	connect(sitesList, SIGNAL(managersAdded(const QList<SiteManager *> &)), this, SLOT(managersAdded(const QList<SiteManager *> &)));
	connect(sitesList, SIGNAL(managersRemoved(const QList<SiteManager *> &)), this, SLOT(managersRemoved(const QList<SiteManager *> &)));

#if QT_VERSION >= 0x050000
	QString storagePath = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
#else
	QString storagePath = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
#endif

	if(!QDir("/").mkpath(storagePath))
		return;

	storagePath.append("/loggeddata.db");

	db = QSqlDatabase::addDatabase("QSQLITE", "loggeddata");
	db.setHostName("localhost");
	db.setDatabaseName(storagePath);
	if(!db.open())
		return;

	if(!LoggedData::setupDB(db))
		return;

	initialized = true;

	// Set up initial managers
	managersAdded(sitesList->getManagers().values());
}

bool LoggedDownloader::downloadNow(SiteManager *manager) {
	if(currManager != NULL)
		return false;
	bool forceReconnect = stats[manager->getSettings()->getId()].forceReconnect;
	if(!forceReconnect && (manager->getStatus() == SiteManager::SiteError)) // Don't retry if there are errors
		return false;

	qDebug() << "Downloading from site " << manager->getSettings()->getName();
	currManager = manager;

	connect(currManager, SIGNAL(loggedDataDownloaded()), this, SLOT(downloadFinished()));
	connect(currManager, SIGNAL(errorCanceled()), this, SLOT(errorCanceled()));

	return currManager->downloadLogged(true, true, true);
}

void LoggedDownloader::downloadFinished() {
	if(currManager == NULL) {
		checkTime();
		return;
	}

	disconnect(currManager, SIGNAL(loggedDataDownloaded()), this, SLOT(downloadFinished()));
	disconnect(currManager, SIGNAL(errorCanceled()), this, SLOT(errorCanceled()));

	QSharedPointer<LoggedData> data = currManager->retrieveLogged();
	if(data != NULL) {
		int recordId = data->writeToDB(db);
		if(recordId > 0) {
			int site = currManager->getSettings()->getId();
			stats[site].lastDownloadTime = data->getDownloadTime();
			stats[site].retryTime = 0;
			stats[site].forceReconnect = false;
		} else {
			emit downloadDBError();
		}
	}

	currManager = NULL;
	checkTime();
}

void LoggedDownloader::errorCanceled() {
	if(currManager == NULL) {
		checkTime();
		return;
	}

	disconnect(currManager, SIGNAL(loggedDataDownloaded()), this, SLOT(downloadFinished()));
	disconnect(currManager, SIGNAL(errorCanceled()), this, SLOT(errorCanceled()));

	int64_t currTime = QDateTime::currentDateTime().toTime_t();

	int site = currManager->getSettings()->getId();
	stats[site].retryTime = currTime + retryDelaySecs; // Wait a long while before trying to reconnect
	stats[site].forceReconnect = true;

	currManager = NULL;
	checkTime();	
}

void LoggedDownloader::checkIfDownloadNeeded() {
   	retrieveDownloadTimes(); // Refresh download time
   	downloadTimer->start(0); // Don't run now to prevent constructor from trying to download data
}

void LoggedDownloader::siteStatusChanged(SiteManager::Status newStatus) {
	SiteManager *site = (SiteManager *) sender();
	if(site == NULL)
		return;
	int siteId = site->getSettings()->getId();

	// Error won't be cleared until it is actually working for sure
	if(newStatus != SiteManager::SiteError && newStatus != SiteManager::SiteDownloadingLogged) {
		stats[siteId].retryTime = 0; // Make the site reconnect now
		stats[siteId].forceReconnect = false; // Disable, since the connection seems to be working
	}
	checkTime();
}

void LoggedDownloader::checkTime() {
	if(currManager != NULL)
		return;

	int64_t currTime, nextTime;
	int toDownloadNow;
	while(true) {
		currTime = QDateTime::currentDateTime().toTime_t();
		nextTime = -1;

		toDownloadNow = -1;

		foreach(int site, stats.keys()) {
			int interval = sitesList->getSites()[site]->getDownloadInterval() * 60;
			int64_t downloadTime = stats[site].lastDownloadTime;

			if(interval > 0) {
				downloadTime += interval;
				if(stats[site].retryTime > downloadTime) {
					downloadTime = stats[site].retryTime;
				}
				if(downloadTime <= currTime)
					toDownloadNow = site;
				else if(nextTime == -1 || downloadTime <= nextTime)
					nextTime = downloadTime;
			}
		}

		if(toDownloadNow == -1)
			break;

		SiteManager *manager = sitesList->getManagers()[toDownloadNow];
		if(manager && downloadNow(manager)) {
			return;
		}
	}

	if(nextTime != -1)
		downloadTimer->start((nextTime - currTime) * 1000);
	else
		downloadTimer->stop();
}

LoggedDownloader::~LoggedDownloader() {
	db.close();
}

void LoggedDownloader::managersAdded(const QList<SiteManager *> & managers) {
	foreach(SiteManager *manager, managers) {
		QSharedPointer<SiteSettings> site = manager->getSettings();
		int id = site->getId();

		DownloadStats s;
		s.retryTime = 0;
		s.lastDownloadTime = 0;
		s.forceReconnect = false;
		stats[id] = s;

		connect(manager, SIGNAL(statusChanged(SiteManager::Status)), this, SLOT(siteStatusChanged(SiteManager::Status)));
	}
	checkIfDownloadNeeded();
}

// Note that this permanently erases all data for these sites as well!
void LoggedDownloader::managersRemoved(const QList<SiteManager *> & managers) {
	foreach(SiteManager *manager, managers) {
		disconnect(currManager, 0, this, 0);
		if(manager == currManager) {
		 	// If we delete in the middle of the download, make sure things don't get screwed up
			currManager = NULL;
		}
		QSharedPointer<SiteSettings> site = manager->getSettings();
		int id = site->getId();
		stats.remove(id);
		
		// Delete data
		bool success;
		QList<QPair<int, int64_t> > toDelete = downloadsForSite(id, success);
		if(success) { // Not much sane we can do about a failure here
			QPair<int, int64_t> download;
			foreach(download, toDelete) {
				LoggedData::deleteFromDB(db, download.first);
			}
		}
	}
}

QList<QPair<int, int64_t> > LoggedDownloader::downloadsForSite(int site, bool & success) {
	QSqlQuery query(db);
	query.prepare("SELECT downloadid, realtime FROM downloadinfo WHERE siteid = :siteid ORDER BY realtime ASC;");
	query.bindValue(":siteid", QVariant(site));
	QList<QPair<int, int64_t> > result;
	if(!query.exec()) {
		success = false;
		return result;
	}

	while(query.next()) {
		result.append(QPair<int, int64_t>(query.value(0).toInt(), query.value(1).toLongLong()));
	}
	success = true;
	return result;
}

QList<QSharedPointer<LoggedData> > LoggedDownloader::retrieveLoggedSet(QList<int> & ids, bool & success) {
	QList<QSharedPointer<LoggedData> > result;

	foreach(int id, ids) {
		LoggedData *curr = new LoggedData(db, id);
		if(!curr->isInitialized() || !curr->readDataFromDB(db)) {
			delete curr;
			success = false;
			return result;
		}
		result.append(QSharedPointer<LoggedData>(curr));
	}

	success = true;
	return result;
}

bool LoggedDownloader::deleteLoggedSet(QList<int> & ids) {
	bool success = true;

	foreach(int id, ids) {
		success = success && LoggedData::deleteFromDB(db, id);
	}

	return success;
}

void LoggedDownloader::retrieveDownloadTimes() {
	QSqlQuery query(db);
	query.prepare("SELECT realtime FROM downloadinfo WHERE siteid = :siteid ORDER BY realtime DESC LIMIT 1;");

	foreach(int site, stats.keys()) {
		query.bindValue(":siteid", QVariant(site));
		if(!query.exec())
			break;

		if(query.next()) {
			stats[site].lastDownloadTime = query.value(0).toLongLong();
		} else {
			stats[site].lastDownloadTime = 0;
		}
		qDebug() << "Last download time for site " << site << " : " << stats[site].lastDownloadTime;
	}	
}
