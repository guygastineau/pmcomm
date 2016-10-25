#ifndef SITEMANAGER_H
#define SITEMANAGER_H

#include "pmdefs.h"
#include "loggedvalue.h"
#include "loggeddata.h"
#include "sitesettings.h"
#include "programvalue.h"

#include <QObject>
#include <QString>
#include <QSet>
#include <QSharedPointer>
#include <QMultiMap>
#include <QPair>

class DataFetcher;

class QTimer;

class SiteManager : public QObject {
	Q_OBJECT

public:
	SiteManager(QSharedPointer<SiteSettings> settings, QObject *parent = NULL);

	QSharedPointer<SiteSettings> getSettings();

	void startUpdating();
	void stopUpdating();
	void releaseFetcher();
	void reconnect();

	enum Status {
		SiteStopped,
		SiteUpdating,
		SiteDownloadingLogged,
		SiteError
	};
	Status getStatus();

	void addDisplay(enum PMDisplayNumber display);
	void removeDisplay(enum PMDisplayNumber display);

	DisplayValue getDisplay(enum PMDisplayNumber display);
	bool displayUpToDate(enum PMDisplayNumber display);

	QSharedPointer<ProgramValue> getProgram(enum PMProgramNumber program);

	bool downloadLogged(bool periodic, bool profile, bool efficiency);
	QSharedPointer<LoggedData> retrieveLogged();

	static QString getLabelStatic(enum PMDisplayNumber display, SiteManager *manager);
	static QString getLabelStatic(enum PMDisplayNumber display, struct PMLabels & labels);
	QString getLabel(enum PMDisplayNumber display);

	bool resetDisplay(enum PMDisplayNumber display);
	bool resetDirect(enum PMResetType command);

	bool batteryPresent(int battery);
	QPair<int, int> & firmwareVersion() { return version; }
	QPair<int, int> & interfaceFirmwareVersion() { return interfaceVersion; }

public slots:
	void fetchProgram(enum PMProgramNumber program);
	void saveProgramData(QSharedPointer<ProgramValue> & value);
   	void updateProgramData();
	void finishError(bool retry, bool noError = false);
	void cancelLoggedDownload();

	// These aren't meant to be called directly (only as a slot invocation)
    void handleData(DisplayValue & value, int id);
    void displayDataError(enum PMDisplayNumber display, int id);
    void handleProgramData(QSharedPointer<ProgramValue> data, int id);
   	void fetchError(enum PMProgramNumber program, int id);
	void programDataStored(enum PMProgramNumber program, int id);
	void setError(enum PMProgramNumber program, int id);
	void resetStatus(enum PMResetType command, bool success, int id);
	void handleConnected(int id, int version);
	void handleLoggedData(QSharedPointer<LoggedValue> value, QSharedPointer<LoggedValue> value2, int id);
	void handleLoggedProgress(LoggedValue::LoggedDataType type, int id, int progress, int outof);
	void loggedError(LoggedValue::LoggedDataType type, int id);

signals:
	void displayUpdated(PMDisplayNumber display);
	void allDisplaysUpdated();

	void labelsUpdated();
	void versionUpdated();

	void programUpdated(PMProgramNumber program);
	void writingFinished();

	void resetComplete(enum PMResetType command);
	void batteryPresenceUpdated(int battery, bool present);
	void programDataReset();

	void communicationsError();
	void errorCanceled();

	void statusChanged(SiteManager::Status newStatus);

	void loggedDataDownloaded();
	void loggedDataProgress(LoggedValue::LoggedDataType type, int progress, int outof);

private:
	DataFetcher *getFetcher();

	bool displayValid(enum PMDisplayNumber display);

	void stopUpdatingError();
	void stopUpdatingImpl();
	void requestDisplays();
	void requestIfNeeded(enum PMDisplayNumber display);

	bool getResetCommand(enum PMDisplayNumber display, enum PMResetType & command);

	static QString ampsLabelStatic(QString prefix, QString baseLabel, int ampsChannel, enum PMAmpsLabel *ampsLabels);
	static QString voltsLabelStatic(QString prefix, int voltsChannel, enum PMBatteryLabel batteryLabel);

	void handleProgramChange(enum PMProgramNumber progNum);
	int generateId();

	bool loggedFetchNext();

	enum ErrorStatus {
		ErrorOK, // No errors present
		ErrorHappened, // Error has happened
		ErrorCleared, // Error is supposedly fixed, but data still needs to come in to go back to ErrorOK
		ErrorAccepted // The user has chosen to not continue. Attempting any operation will put the state back to ErrorCleared
	};

	int nextId;

	DataFetcher *fetcher;
	QSharedPointer<SiteSettings> settings;
	// QString siteName;
	QMap<enum PMDisplayNumber, DisplayValue> currentValues;
	QSet<enum PMDisplayNumber> updatedDisplays;
	QSet<enum PMDisplayNumber> displays;
	QMultiMap<enum PMDisplayNumber, int> outstandingRequests;
	QList<enum PMDisplayNumber> upcomingRequests;

	QPair<int, int> version;
	QPair<int, int> interfaceVersion;

	bool battery1Present;
	bool battery2Present;
	bool smallShunt[3];

	struct PMLabels labels;
	bool labelsValid;

	bool updating;
	QTimer *programDataTimer;

	QMap<enum PMProgramNumber, QSharedPointer<ProgramValue> > programValues;
	QMultiMap<enum PMProgramNumber, int> outstandingFetchRequests; // These are really just being used as regular maps
	QSet<enum PMProgramNumber> fetchErrors;
	QMultiMap<enum PMProgramNumber, int> outstandingSetRequests;
	QSet<enum PMProgramNumber> setErrors;
	QMultiMap<enum PMResetType, int> outstandingResetRequests;
	QSet<enum PMResetType> resetErrors;

	ErrorStatus errorPending;
	bool shouldUpdate;

	bool downloadingLogged;
	QList<LoggedValue::LoggedDataType> loggedRequested;
	QSharedPointer<LoggedData> loggedData;
	QPair<LoggedValue::LoggedDataType, int> outstandingLoggedRequest;

	bool forceDisconnect;
};

Q_DECLARE_METATYPE(SiteManager::Status);

#endif