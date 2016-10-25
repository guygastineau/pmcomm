#ifndef LOGGEDDATA_H
#define LOGGEDDATA_H

#include "loggedvalue.h"

#include <QSharedPointer>
#include <QSqlDatabase>

class LoggedData {
public:
	LoggedData(int siteId);

	static bool setupDB(QSqlDatabase db);

	bool isInitialized() { return initialized; }

	void setLoggedValue(QSharedPointer<LoggedValue> value);
	void setUnitTime(int unitTimeMinutes);
	void setLabels(struct PMLabels & labels);
	void setShuntTypes(bool *smallShunt);
	void setBatteryEnabled(int battery, bool enabled);

	int64_t getDownloadTime();

	bool metadataReady();

	LoggedData(QSqlDatabase db, int downloadid);

	int writeToDB(QSqlDatabase db);
	bool readDataFromDB(QSqlDatabase db);

	bool writeToFiles(QString baseName);

	static bool writeRecordsToFiles(QList<QSharedPointer<LoggedData> > & data, QString baseName);
	static bool deleteFromDB(QSqlDatabase db, int id);

	friend class PeriodicLoggedValue;
	friend class ProfileLoggedValue;
	friend class EfficiencyLoggedValue;

private:
	enum LoggedTypeField {
		LOGGEDBITS_PERIODIC = 1,
		LOGGEDBITS_PROFILE1 = 2,
		LOGGEDBITS_PROFILE2 = 4,
		LOGGEDBITS_EFFICIENCY1 = 8,
		LOGGEDBITS_EFFICIENCY2 = 16
	};

	int createDownloadRecordDB(QSqlDatabase db);
	int enabledMask();

	QSharedPointer<PeriodicLoggedValue> periodic;
	QSharedPointer<ProfileLoggedValue> profile1;
	QSharedPointer<ProfileLoggedValue> profile2;	
	QSharedPointer<EfficiencyLoggedValue> efficiency1;
	QSharedPointer<EfficiencyLoggedValue> efficiency2;

	unsigned int types;
	unsigned int batteryMask;

	int siteId;
	int downloadId;
	int64_t realTime;
	int unitTime;
	struct PMLabels labels;
	bool labelsValid;
	unsigned int shuntTypes;
	bool shuntsValid;

	bool initialized;
};


#endif