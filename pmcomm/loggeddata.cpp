#include "loggeddata.h"

#include <QSqlQuery>
#include <QDateTime>
#include <QVariant>
#include <QString>

bool LoggedData::setupDB(QSqlDatabase db) {
	QSqlQuery query(db);
	bool success = query.exec("CREATE TABLE IF NOT EXISTS downloadinfo ("
		"downloadid INTEGER PRIMARY KEY,"
		"siteid INTEGER NOT NULL,"
		"unittime INTEGER NOT NULL,"
		"realtime INTEGER NOT NULL,"
		"datatypes INTEGER NOT NULL,"
		"shunttypes INTEGER,"
		"batlabel INTEGER,"
		"amps1label INTEGER,"
		"amps2label INTEGER,"
		"amps3label INTEGER);");

	success = success && query.exec("CREATE TABLE IF NOT EXISTS periodic ("
		"downloadid INTEGER NOT NULL,"
		"recordid INTEGER NOT NULL,"
		"meastime INTEGER NOT NULL,"
		"ahr1man INTEGER,"
		"ahr1exp INTEGER,"
		"ahr2man INTEGER,"
		"ahr2exp INTEGER,"
		"ahr3man INTEGER,"
		"ahr3exp INTEGER,"
		"whr1man INTEGER,"
		"whr1exp INTEGER,"
		"whr2man INTEGER,"
		"whr2exp INTEGER,"
		"minTemp INTEGER,"
		"maxTemp INTEGER,"
		"volts1 INTEGER,"
		"volts2 INTEGER,"
		"amps1man INTEGER,"
		"amps1exp INTEGER,"
		"bat1percent INTEGER,"
		"bat1charged INTEGER,"
		"bat2percent INTEGER,"
		"bat2charged INTEGER);");

	success = success && query.exec("CREATE TABLE IF NOT EXISTS profile ("
		"downloadid INTEGER NOT NULL,"
		"recordid INTEGER NOT NULL,"
		"battery INTEGER NOT NULL,"
		"day INTEGER,"
		"percent INTEGER,"
		"volts INTEGER,"
		"ampsman INTEGER,"
		"ampsexp INTEGER);");

	success = success && query.exec("CREATE TABLE IF NOT EXISTS efficiency ("
		"downloadid INTEGER NOT NULL,"
		"recordid INTEGER NOT NULL,"
		"battery INTEGER NOT NULL,"
		"endtime INTEGER NOT NULL,"
		"valid INTEGER NOT NULL,"
		"length INTEGER,"
		"discharge INTEGER,"
		"charge INTEGER,"
		"net INTEGER,"
		"efficiency REAL,"
		"selfdischarge REAL);");

	return success;
}

LoggedData::LoggedData(int siteId) {
	this->siteId = siteId;
	unitTime = -1;
	labelsValid = false;
	shuntsValid = false;
	batteryMask = ~0;
	types = 0;
}

void LoggedData::setBatteryEnabled(int battery, bool enabled) {
	unsigned int bits = 0;
	if(battery == 1)
		bits = LOGGEDBITS_PROFILE1 | LOGGEDBITS_EFFICIENCY1;
	else if(battery == 2)
		bits = LOGGEDBITS_PROFILE2 | LOGGEDBITS_EFFICIENCY2;		

	if(enabled)
		batteryMask |= bits;
	else
		batteryMask &= ~bits;
}

int LoggedData::enabledMask() {
	return types & batteryMask;
}

void LoggedData::setLoggedValue(QSharedPointer<LoggedValue> value) {
	if(value == NULL)
		return;

	value->setContext(this);
	QSharedPointer<ProfileLoggedValue> pValue;
	QSharedPointer<EfficiencyLoggedValue> eValue;

	switch(value->type()) {
		case LoggedValue::TYPE_PERIODIC:
			periodic = value.staticCast<PeriodicLoggedValue>();
			types |= LOGGEDBITS_PERIODIC;
			break;
		case LoggedValue::TYPE_PROFILE:
			pValue = value.staticCast<ProfileLoggedValue>();
			if(pValue->getBattery() == 1) {
				profile1 = pValue;
				types |= LOGGEDBITS_PROFILE1;
			} else {
				profile2 = pValue;
				types |= LOGGEDBITS_PROFILE2;
			}
			break;
		case LoggedValue::TYPE_EFFICIENCY:
			eValue = value.staticCast<EfficiencyLoggedValue>();
			if(eValue->getBattery() == 1) {
				efficiency1 = eValue;
				types |= LOGGEDBITS_EFFICIENCY1;
			} else {
				efficiency2 = eValue;
				types |= LOGGEDBITS_EFFICIENCY2;
			}
			break;
		default:
			break;
	}
}

int64_t LoggedData::getDownloadTime() {
	return realTime;
}

void LoggedData::setUnitTime(int unitTimeMinutes) {
	realTime = QDateTime::currentDateTime().toTime_t(); // Record the real time that the unit time was read
	unitTime = unitTimeMinutes;
}

void LoggedData::setLabels(struct PMLabels & labels) {
	this->labels = labels;
	labelsValid = true;
}

void LoggedData::setShuntTypes(bool *smallShunt) {
	shuntTypes = 0;
	for(int i = 0; i < 3; i++) {
		shuntTypes |= (smallShunt[i] ? 1 : 0) << i;
	}
	shuntsValid = true;
}

bool LoggedData::metadataReady() {
	return unitTime >= 0 && labelsValid && shuntsValid;
}

LoggedData::LoggedData(QSqlDatabase db, int downloadid) {
	downloadId = downloadid;
	siteId = -1;
	unitTime = -1;
	labelsValid = false;
	shuntsValid = false;
	batteryMask = ~0;
	initialized = false;

	QSqlQuery query(db);
	query.prepare("SELECT siteid,unittime,realtime,datatypes,shunttypes,"
		"batlabel,amps1label,amps2label,amps3label FROM "
		"downloadinfo WHERE downloadid = :downloadid;");

	query.bindValue(":downloadid", downloadid);

	bool success = query.exec();
	if(!success || !query.next())
		return;

	siteId = query.value(0).toInt();
	unitTime = query.value(1).toInt();
	realTime = query.value(2).toLongLong();
	types = query.value(3).toInt();
	shuntTypes = query.value(4).toInt();
	labels.battLabel = (enum PMBatteryLabel) query.value(5).toInt();
	for(int i = 0; i < 3; i++) {
		labels.ampsLabels[i] = (enum PMAmpsLabel) query.value(6+i).toInt();
	}

	labelsValid = true;
	shuntsValid = true;
	initialized = true;
}

bool LoggedData::readDataFromDB(QSqlDatabase db) {
	if(downloadId < 1)
		return false;

	int tmask = enabledMask();
	if(tmask & LOGGEDBITS_PERIODIC) {
		periodic = QSharedPointer<PeriodicLoggedValue>(new PeriodicLoggedValue(db, downloadId));
		if(!periodic->isValid())
			return false;
		periodic->setContext(this);
	}
	if(tmask & LOGGEDBITS_PROFILE1) {
		profile1 = QSharedPointer<ProfileLoggedValue>(new ProfileLoggedValue(db, downloadId, 1));
		if(!profile1->isValid())
			return false;
		profile1->setContext(this);
	}
	if(tmask & LOGGEDBITS_PROFILE2) {
		profile2 = QSharedPointer<ProfileLoggedValue>(new ProfileLoggedValue(db, downloadId, 2));
		if(!profile2->isValid())
			return false;
		profile2->setContext(this);
	}
	if(tmask & LOGGEDBITS_EFFICIENCY1) {
		efficiency1 = QSharedPointer<EfficiencyLoggedValue>(new EfficiencyLoggedValue(db, downloadId, 1));
		if(!efficiency1->isValid())
			return false;
		efficiency1->setContext(this);
	}
	if(tmask & LOGGEDBITS_EFFICIENCY2) {
		efficiency2 = QSharedPointer<EfficiencyLoggedValue>(new EfficiencyLoggedValue(db, downloadId, 2));
		if(!efficiency2->isValid())
			return false;
		efficiency2->setContext(this);
	}
	return true;
}

int LoggedData::createDownloadRecordDB(QSqlDatabase db) {
	if(!metadataReady())
		return -1;

	QSqlQuery query(db);
	query.prepare("INSERT INTO downloadinfo (downloadid, siteid, unittime, realtime, datatypes, "
		"shunttypes, batlabel, amps1label, amps2label, amps3label) "
		"VALUES (NULL, :siteid, :unittime, :realtime, :datatypes, :shunttypes, :batlabel, :amps1label, :amps2label, :amps3label);");

	query.bindValue(":siteid", siteId);
	query.bindValue(":unittime", unitTime);
	query.bindValue(":realtime", (qint64) realTime);
	query.bindValue(":datatypes", enabledMask());
	query.bindValue(":shunttypes", shuntTypes);
	query.bindValue(":batlabel", (int) labels.battLabel);

	for(int i = 0; i < 3; i++) {
		QString placeholder = QString(":amps%1label").arg(i+1);
		query.bindValue(placeholder, (int) labels.ampsLabels[i]);
	}

	bool success = query.exec();
	if(!success)
		return -1;

	return query.lastInsertId().toInt();
}

int LoggedData::writeToDB(QSqlDatabase db) {
	QSqlQuery query(db);
	if(!query.exec("BEGIN IMMEDIATE;"))
		return -1;

	bool success = true;
	downloadId = createDownloadRecordDB(db);
	if(downloadId < 1)
		success = false;

	int tmask = enabledMask();
	if(tmask & LOGGEDBITS_PERIODIC)
		success = success && periodic->writeToDB(db, downloadId);
	if(tmask & LOGGEDBITS_PROFILE1)
		success = success && profile1->writeToDB(db, downloadId);
	if(tmask & LOGGEDBITS_PROFILE2)
		success = success && profile2->writeToDB(db, downloadId);
	if(tmask & LOGGEDBITS_EFFICIENCY1)
		success = success && efficiency1->writeToDB(db, downloadId);
	if(tmask & LOGGEDBITS_EFFICIENCY2)
		success = success && efficiency2->writeToDB(db, downloadId);

	success = success && query.exec("COMMIT;");

	if(success)
		return downloadId;

	query.exec("ROLLBACK;");
	return -1;
}

bool LoggedData::writeToFiles(QString baseName) {
	bool success = true;
	int tmask = enabledMask();
	if(tmask & LOGGEDBITS_PERIODIC) {
		QString filename = QString("%1_PeriodicData.csv").arg(baseName);
		success = success && periodic->writeToFile(filename);
	}

	if(tmask & LOGGEDBITS_PROFILE1) {
		QString filename = QString("%1_Bat1DischProfile.csv").arg(baseName);
		success = success && profile1->writeToFile(filename);
	}
	if(tmask & LOGGEDBITS_PROFILE2) {
		QString filename = QString("%1_Bat2DischProfile.csv").arg(baseName);
		success = success && profile2->writeToFile(filename);
	}

	if(tmask & LOGGEDBITS_EFFICIENCY1) {
		QString filename = QString("%1_Bat1CycleEfficy.csv").arg(baseName);
		success = success && efficiency1->writeToFile(filename);
	}
	if(tmask & LOGGEDBITS_EFFICIENCY2) {
		QString filename = QString("%1_Bat2CycleEfficy.csv").arg(baseName);
		success = success && efficiency2->writeToFile(filename);
	}

	return success;
}

bool LoggedData::writeRecordsToFiles(QList<QSharedPointer<LoggedData> > & data, QString baseName) {
	QList<PeriodicLoggedValue *> periodicList;
	QList<ProfileLoggedValue *> profile1List, profile2List;
	QList<EfficiencyLoggedValue *> efficiency1List, efficiency2List;

	for(int i = 0; i < data.size(); i++) {
		int tmask = data[i]->enabledMask();
		if(tmask & LOGGEDBITS_PERIODIC) {
			periodicList.append(data[i]->periodic.data());
		}
		if(tmask & LOGGEDBITS_PROFILE1) {
			profile1List.append(data[i]->profile1.data());
		}
		if(tmask & LOGGEDBITS_PROFILE2) {
			profile2List.append(data[i]->profile2.data());
		}
		if(tmask & LOGGEDBITS_EFFICIENCY1) {
			efficiency1List.append(data[i]->efficiency1.data());
		}
		if(tmask & LOGGEDBITS_EFFICIENCY2) {
			efficiency1List.append(data[i]->efficiency1.data());
		}
	}

	bool success = true;
	if(!periodicList.empty()) {
		QString filename = QString("%1_PeriodicData-export.csv").arg(baseName);
		success = success && PeriodicLoggedValue::writeGroupToFile(periodicList, filename);
	}
	if(!profile1List.empty()) {
		QString filename = QString("%1_Bat1DischProfile-export.csv").arg(baseName);
		success = success && ProfileLoggedValue::writeGroupToFile(profile1List, filename);
	}
	if(!profile2List.empty()) {
		QString filename = QString("%1_Bat2DischProfile-export.csv").arg(baseName);
		success = success && ProfileLoggedValue::writeGroupToFile(profile2List, filename);
	}
	if(!efficiency1List.empty()) {
		QString filename = QString("%1_Bat1CycleEfficy-export.csv").arg(baseName);
		success = success && EfficiencyLoggedValue::writeGroupToFile(efficiency1List, filename);
	}
	if(!efficiency2List.empty()) {
		QString filename = QString("%1_Bat2CycleEfficy-export.csv").arg(baseName);
		success = success && EfficiencyLoggedValue::writeGroupToFile(efficiency2List, filename);
	}

	return success;	
}

bool LoggedData::deleteFromDB(QSqlDatabase db, int id) {
	QSqlQuery query(db);
	if(!query.exec("BEGIN IMMEDIATE;"))
		return false;

	query.prepare("DELETE FROM downloadinfo WHERE downloadid = :downloadid;");
	query.bindValue(":downloadid", id);
	bool success = query.exec();
	query.prepare("DELETE FROM periodic WHERE downloadid = :downloadid;");
	query.bindValue(":downloadid", id);
	success = success && query.exec();
	query.prepare("DELETE FROM profile WHERE downloadid = :downloadid;");
	query.bindValue(":downloadid", id);
	success = success && query.exec();
	query.prepare("DELETE FROM efficiency WHERE downloadid = :downloadid;");
	query.bindValue(":downloadid", id);
	success = success && query.exec();

	success = success && query.exec("COMMIT;");
	if(!success)
		query.exec("ROLLBACK;");

	return success;
}

