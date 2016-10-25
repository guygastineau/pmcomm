#include "loggedvalue.h"

#include "sitemanager.h"
#include "loggeddata.h"
#include "appsettings.h"

#include <QString>
#include <QDateTime>
#include <QSqlQuery>
#include <QVariant>
#include <QFile>
#include <QIODevice>
#include <QTextStream>
#include <cmath> // pow()
#include <cstdlib> // llabs()

static const unsigned int TIME_MATCH_THRESHOLD = 3600 * 4; // Four hours

LoggedValue::LoggedValue() {
	context = NULL;
	battery = 0;
	initialized = false;
}

// Sets a pointer to the LoggedData object that owns this LoggedValue
void LoggedValue::setContext(LoggedData *loggedData) {
	context = loggedData;
}

int LoggedValue::getBattery() {
	return battery;
}

// LoggedDisplayValue constructor for int-valued types 
LoggedDisplayValue::LoggedDisplayValue(enum PMDisplayNumber display, int value) : DisplayValue() {
	disp = display;
	precision = 0;
	intValue = value;
	scientificValue.mantissa = value;
	scientificValue.exponent = 0;
}

// LoggedDisplayValue constructor for scientific-valued types 
LoggedDisplayValue::LoggedDisplayValue(enum PMDisplayNumber display, struct PMScientificValue & value) : DisplayValue(), scientificValue(value) {
	disp = display;
	precision = 0;
	intValue = 0;
}

// Converts the LoggedDisplayValue to floating point
double LoggedDisplayValue::toDouble() {
	if(disp >= PM_D7 && disp <= PM_D21)
		return scientificValue.mantissa * pow((double) 10, (double) (scientificValue.exponent));

	return DisplayValue::toDouble();
}

// Constructs a PeriodicLoggedValue from a linked list of PMPeriodicRecord
// structs, as returned by libpmcomm
PeriodicLoggedValue::PeriodicLoggedValue(struct PMPeriodicRecord *records) {
	loggedType = TYPE_PERIODIC;

	while(records) {
		rs.prepend(*records);
		records = records->next;
	}
	initialized = true;
}

// Constructs a PeriodicLoggedValue from a particular download in the database
PeriodicLoggedValue::PeriodicLoggedValue(QSqlDatabase db, int downloadid) {
	loggedType = TYPE_PERIODIC;

	QSqlQuery query(db);
	bool success = query.prepare("SELECT meastime,"
		"ahr1man,ahr1exp,ahr2man,ahr2exp,ahr3man,ahr3exp,"
		"whr1man,whr1exp,whr2man,whr2exp,minTemp,maxTemp,"
		"volts1,volts2,amps1man,amps1exp,bat1percent,bat1charged,"
		"bat2percent,bat2charged FROM periodic WHERE downloadid = "
		":downloadid ORDER BY recordid ASC;");

	if(!success)
		return;

	query.bindValue(":downloadid", downloadid);
	if(!query.exec())
		return;

	while(query.next()) {
		PMPeriodicRecord curr;
		memset(&curr, 0, sizeof(PMPeriodicRecord));

		curr.measTime = query.value(0).toInt();
		if(!query.value(1).isNull()) {
			curr.ahr1.mantissa = query.value(1).toInt();
			curr.ahr1.exponent = query.value(2).toInt();
			curr.validData |= PM_PERIODIC_AHR1_VALID;
		}
		if(!query.value(3).isNull()) {
			curr.ahr2.mantissa = query.value(3).toInt();
			curr.ahr2.exponent = query.value(4).toInt();
			curr.validData |= PM_PERIODIC_AHR2_VALID;
		}
		if(!query.value(5).isNull()) {
			curr.ahr3.mantissa = query.value(5).toInt();
			curr.ahr3.exponent = query.value(6).toInt();
			curr.validData |= PM_PERIODIC_AHR3_VALID;
		}
		if(!query.value(7).isNull()) {
			curr.whr1.mantissa = query.value(7).toInt();
			curr.whr1.exponent = query.value(8).toInt();
			curr.validData |= PM_PERIODIC_WHR1_VALID;
		}
		if(!query.value(9).isNull()) {
			curr.whr2.mantissa = query.value(9).toInt();
			curr.whr2.exponent = query.value(10).toInt();
			curr.validData |= PM_PERIODIC_WHR2_VALID;
		}
		if(!query.value(11).isNull()) {
			curr.maxTemp = query.value(11).toInt();
			curr.minTemp = query.value(12).toInt();
			curr.validData |= PM_PERIODIC_TEMP_VALID;
		}
		if(!query.value(13).isNull()) {
			curr.volts1 = query.value(13).toInt();
			curr.validData |= PM_PERIODIC_VOLTS1_VALID;
		}
		if(!query.value(14).isNull()) {
			curr.amps1.mantissa = query.value(14).toInt();
			curr.amps1.exponent = query.value(15).toInt();
			curr.validData |= PM_PERIODIC_AMPS1_VALID;
		}
		if(!query.value(16).isNull()) {
			curr.volts1 = query.value(16).toInt();
			curr.validData |= PM_PERIODIC_VOLTS2_VALID;
		}
		if(!query.value(17).isNull()) {
			curr.bat1Percent.percent = query.value(17).toInt();
			curr.bat1Percent.charged = query.value(18).toBool();
			curr.bat2Percent.percent = query.value(19).toInt();
			curr.bat2Percent.charged = query.value(20).toBool();
			curr.validData |= PM_PERIODIC_BATTSTATE_VALID;
		}	

		rs.append(curr);
	}
	initialized = true;
}

// Determines if two periodic records are exactly equal
bool PeriodicLoggedValue::rowsEqual(struct PMPeriodicRecord & one, struct PMPeriodicRecord & two) {
	if(one.validData != two.validData || one.measTime != two.measTime)
		return false;
	if((one.validData & PM_PERIODIC_AHR1_VALID) && (one.ahr1.mantissa != two.ahr1.mantissa || one.ahr1.exponent != two.ahr1.exponent))
		return false;
	if((one.validData & PM_PERIODIC_AHR2_VALID) && (one.ahr2.mantissa != two.ahr2.mantissa || one.ahr2.exponent != two.ahr2.exponent))
		return false;
	if((one.validData & PM_PERIODIC_AHR3_VALID) && (one.ahr3.mantissa != two.ahr3.mantissa || one.ahr3.exponent != two.ahr3.exponent))
		return false;
	if((one.validData & PM_PERIODIC_WHR1_VALID) && (one.whr1.mantissa != two.whr1.mantissa || one.whr1.exponent != two.whr1.exponent))
		return false;
	if((one.validData & PM_PERIODIC_WHR2_VALID) && (one.whr2.mantissa != two.whr2.mantissa || one.whr2.exponent != two.whr2.exponent))
		return false;
	if((one.validData & PM_PERIODIC_TEMP_VALID) && (one.minTemp != two.minTemp || one.maxTemp != two.maxTemp))
		return false;
	if((one.validData & PM_PERIODIC_VOLTS1_VALID) && (one.volts1 != two.volts1))
		return false;
	if((one.validData & PM_PERIODIC_VOLTS2_VALID) && (one.volts2 != two.volts2))
		return false;
	if((one.validData & PM_PERIODIC_AMPS1_VALID) && (one.amps1.mantissa != two.amps1.mantissa || one.amps1.exponent != two.amps1.exponent))
		return false;
	if((one.validData & PM_PERIODIC_BATTSTATE_VALID) && (one.bat1Percent.percent != two.bat1Percent.percent || one.bat1Percent.charged != two.bat1Percent.charged))
		return false;
	if((one.validData & PM_PERIODIC_BATTSTATE_VALID) && (one.bat2Percent.percent != two.bat2Percent.percent || one.bat2Percent.charged != two.bat2Percent.charged))
		return false;
	return true;
}

// Writes the headers for periodic data to a file
void PeriodicLoggedValue::writeHeadersToFile(QTextStream & loggedfile) {
	Q_ASSERT(context);

	PMLabels & labels = context->labels;
	loggedfile << "Date Time,";
	loggedfile << SiteManager::getLabelStatic(PM_D13, labels) << ",";
	loggedfile << SiteManager::getLabelStatic(PM_D14, labels) << ",";
	loggedfile << SiteManager::getLabelStatic(PM_D15, labels) << ",";
	loggedfile << SiteManager::getLabelStatic(PM_D20, labels) << ",";
	loggedfile << SiteManager::getLabelStatic(PM_D21, labels) << ",";
	loggedfile << SiteManager::getLabelStatic(PM_D28, labels) << " Min,";
	loggedfile << SiteManager::getLabelStatic(PM_D28, labels) << " Max,";
	loggedfile << SiteManager::getLabelStatic(PM_D3, labels) << ",";
	loggedfile << SiteManager::getLabelStatic(PM_D10, labels) << ",";
	loggedfile << SiteManager::getLabelStatic(PM_D4, labels) << ",";
	loggedfile << SiteManager::getLabelStatic(PM_D22, labels) << ",";
	loggedfile << "Battery 1 Charged,";
	loggedfile << SiteManager::getLabelStatic(PM_D23, labels) << ",";
	loggedfile << "Battery 2 Charged,";
	loggedfile << endl;
}

// Writes a set of PeriodicLoggedValue objects, which should comprise a continuous record,
// to a file
bool PeriodicLoggedValue::writeGroupToFile(QList<PeriodicLoggedValue *> values, QString filename) {
	QFile lf(filename);
	if(!lf.open(QIODevice::WriteOnly | QIODevice::Text))
		return false;
	QTextStream loggedfile(&lf);

	values.last()->writeHeadersToFile(loggedfile);

	for(int i = 0; i < values.size(); i++) {
		int startRow = 0;
		bool extraRow = false;
		if(i != 0) {
			startRow = values[i]->findMatchingRow(*values[i - 1], extraRow);
		}

		int endRow = values[i]->rs.size();
		if(startRow < endRow && extraRow) {
			loggedfile << ",,,,,,,,,,,,,," << endl;
		}

		values[i]->writeRowsToFile(loggedfile, startRow, endRow);
	}

	return loggedfile.status() == QTextStream::Ok;
}

// Finds the row in this object that corresponds to the last row in previous.
// If the times don't match up to show that the record was continuous (i.e. power was
// lost), extraRow is set to true
int PeriodicLoggedValue::findMatchingRow(PeriodicLoggedValue & previous, bool & extraRow) {
	Q_ASSERT(context);
	Q_ASSERT(previous.context);

	if(previous.rs.empty())
		return 0;

	struct PMPeriodicRecord & prev = previous.rs.last();
	int startRow = 0;

	uint64_t prevMeasTime = previous.context->realTime + ((int) prev.measTime - previous.context->unitTime) * 60;

	// Search for matching time
	extraRow = true; // True if a spacer row is to be inserted
	uint64_t bestTimeDiff = 0;
	bool approxMatch = false;
	for(int row = 0; row < rs.size(); row++) {
		struct PMPeriodicRecord & curr = rs[row];
		uint64_t currMeasTime = context->realTime + ((int) curr.measTime - context->unitTime) * 60;
		uint64_t timeDiff = llabs(currMeasTime - prevMeasTime);
		bool timeMatch = timeDiff <= TIME_MATCH_THRESHOLD;
		if(rowsEqual(curr, prev)) {
			startRow = row + 1;
			extraRow = !timeMatch;
			break;
		}
		if(timeMatch && (!approxMatch || timeDiff < bestTimeDiff)) {
			approxMatch = true;
			bestTimeDiff = timeDiff;
			startRow = row + (currMeasTime > prevMeasTime ? 0 : 1);
		}
	}

	return startRow;
}

// Writes the data in this object to a file
bool PeriodicLoggedValue::writeToFile(QString filename) {
	QFile lf(filename);
	if(!lf.open(QIODevice::WriteOnly | QIODevice::Text))
		return false;
	QTextStream loggedfile(&lf);

	writeHeadersToFile(loggedfile);

	writeRowsToFile(loggedfile, 0, rs.size());

	return loggedfile.status() == QTextStream::Ok;
}

// Writes specified rows of this object to a file
void PeriodicLoggedValue::writeRowsToFile(QTextStream & loggedfile, int startRow, int endRow) {
	Q_ASSERT(context);

	for(int i = startRow; i < endRow; i++) {
		struct PMPeriodicRecord & curr = rs[i];

		int64_t measTime = context->realTime + ((int) curr.measTime - context->unitTime) * 60;
		QDateTime localTime = QDateTime::fromTime_t(measTime).toLocalTime();
		QString formattedTime = localTime.toString("MM/dd/yyyy hh:mm");

		loggedfile << formattedTime;
		loggedfile << ",";
	    if(curr.validData & PM_PERIODIC_AHR1_VALID) {
	    	LoggedDisplayValue value(PM_D13, curr.ahr1);
	     	loggedfile << value.toString();
	    }
	    loggedfile << ",";
	    if(curr.validData & PM_PERIODIC_AHR2_VALID) {
	    	LoggedDisplayValue value(PM_D14, curr.ahr2);
	     	loggedfile << value.toString();
	    }
	    loggedfile << ",";
	    if(curr.validData & PM_PERIODIC_AHR3_VALID) {
	    	LoggedDisplayValue value(PM_D15, curr.ahr3);
	     	loggedfile << value.toString();
	    }
	    loggedfile << ",";
	    if(curr.validData & PM_PERIODIC_WHR1_VALID) {
	    	LoggedDisplayValue value(PM_D20, curr.whr1);
	     	loggedfile << value.toString();
	    }
	    loggedfile << ",";
	    if(curr.validData & PM_PERIODIC_WHR2_VALID) {
	    	LoggedDisplayValue value(PM_D21, curr.whr2);
	     	loggedfile << value.toString();
	    }
	    loggedfile << ",";
	    if(curr.validData & PM_PERIODIC_TEMP_VALID) {
	    	LoggedDisplayValue valueMin(PM_D28, curr.minTemp);
	     	loggedfile << valueMin.toString() << ",";
	    	LoggedDisplayValue valueMax(PM_D28, curr.maxTemp);
	     	loggedfile << valueMax.toString();
	    } else {
	    	loggedfile << ",";
	    }
	    loggedfile << ",";
	    if(curr.validData & PM_PERIODIC_VOLTS1_VALID) {
	    	LoggedDisplayValue value(PM_D3, curr.volts1);
	     	loggedfile << value.toString();
	    }
	    loggedfile << ",";
	    if(curr.validData & PM_PERIODIC_AMPS1_VALID) {
	    	LoggedDisplayValue value(PM_D10, curr.amps1);
	    	value.setPrecision((context->shuntTypes & 1) ? 2 : 1);
	     	loggedfile << value.toString();
	    }
	    loggedfile << ",";
	    if(curr.validData & PM_PERIODIC_VOLTS2_VALID) {
	    	LoggedDisplayValue value(PM_D4, curr.volts2);
	     	loggedfile << value.toString();
	    }
	    loggedfile << ",";
	    if(curr.validData & PM_PERIODIC_BATTSTATE_VALID) {
	    	LoggedDisplayValue valueB1(PM_D22, curr.bat1Percent.percent);
	     	loggedfile << valueB1.toString() << ",";
	     	loggedfile << (curr.bat1Percent.charged ? "TRUE" : "FALSE") << ",";
	    	LoggedDisplayValue valueB2(PM_D23, curr.bat2Percent.percent);
	     	loggedfile << valueB2.toString() << ",";
	     	loggedfile << (curr.bat2Percent.charged ? "TRUE" : "FALSE");
	    } else {
	    	loggedfile << ",,,";
	    }
	    loggedfile << endl;
	}
}

// Writes the data in this object to the database using a specified downloadid
bool PeriodicLoggedValue::writeToDB(QSqlDatabase db, int downloadid) {
	QSqlQuery query(db);
	bool success = query.prepare("INSERT INTO periodic (downloadid,recordid,meastime,"
		"ahr1man,ahr1exp,ahr2man,ahr2exp,ahr3man,ahr3exp,"
		"whr1man,whr1exp,whr2man,whr2exp,minTemp,maxTemp,"
		"volts1,volts2,amps1man,amps1exp,bat1percent,bat1charged,"
		"bat2percent,bat2charged) VALUES (:downloadid,:recordid,:meastime,"
		":ahr1man,:ahr1exp,:ahr2man,:ahr2exp,:ahr3man,:ahr3exp,"
		":whr1man,:whr1exp,:whr2man,:whr2exp,:minTemp,:maxTemp,"
		":volts1,:volts2,:amps1man,:amps1exp,:bat1percent,:bat1charged,"
		":bat2percent,:bat2charged);");

	if(!success)
		return false;

	for(int i = 0; i < rs.size(); i++) {
		struct PMPeriodicRecord  & curr = rs[i];

		query.bindValue(":downloadid", downloadid);
		query.bindValue(":recordid", i+1);
		query.bindValue(":meastime", curr.measTime);

    	QVariant ahr1man = (curr.validData & PM_PERIODIC_AHR1_VALID) ? QVariant(curr.ahr1.mantissa) : QVariant();
    	QVariant ahr1exp = (curr.validData & PM_PERIODIC_AHR1_VALID) ? QVariant(curr.ahr1.exponent) : QVariant();
     	QVariant ahr2man = (curr.validData & PM_PERIODIC_AHR2_VALID) ? QVariant(curr.ahr2.mantissa) : QVariant();
    	QVariant ahr2exp = (curr.validData & PM_PERIODIC_AHR2_VALID) ? QVariant(curr.ahr2.exponent) : QVariant();
    	QVariant ahr3man = (curr.validData & PM_PERIODIC_AHR3_VALID) ? QVariant(curr.ahr3.mantissa) : QVariant();
    	QVariant ahr3exp = (curr.validData & PM_PERIODIC_AHR3_VALID) ? QVariant(curr.ahr3.exponent) : QVariant();
    	QVariant whr1man = (curr.validData & PM_PERIODIC_WHR1_VALID) ? QVariant(curr.whr1.mantissa) : QVariant();
    	QVariant whr1exp = (curr.validData & PM_PERIODIC_WHR1_VALID) ? QVariant(curr.whr1.exponent) : QVariant();
    	QVariant whr2man = (curr.validData & PM_PERIODIC_WHR2_VALID) ? QVariant(curr.whr2.mantissa) : QVariant();
    	QVariant whr2exp = (curr.validData & PM_PERIODIC_WHR2_VALID) ? QVariant(curr.whr2.exponent) : QVariant();
    	QVariant maxTemp = (curr.validData & PM_PERIODIC_TEMP_VALID) ? QVariant(curr.maxTemp) : QVariant();
    	QVariant minTemp = (curr.validData & PM_PERIODIC_TEMP_VALID) ? QVariant(curr.minTemp) : QVariant();
    	QVariant volts1 = (curr.validData & PM_PERIODIC_VOLTS1_VALID) ? QVariant(curr.volts1) : QVariant();
    	QVariant amps1man = (curr.validData & PM_PERIODIC_AMPS1_VALID) ? QVariant(curr.amps1.mantissa) : QVariant();
    	QVariant amps1exp = (curr.validData & PM_PERIODIC_AMPS1_VALID) ? QVariant(curr.amps1.exponent) : QVariant();
    	QVariant volts2 = (curr.validData & PM_PERIODIC_VOLTS2_VALID) ? QVariant(curr.volts2) : QVariant();
    	QVariant bat1percent = (curr.validData & PM_PERIODIC_BATTSTATE_VALID) ? QVariant(curr.bat1Percent.percent) : QVariant();
    	QVariant bat1charged = (curr.validData & PM_PERIODIC_BATTSTATE_VALID) ? QVariant((int) curr.bat1Percent.charged) : QVariant();
    	QVariant bat2percent = (curr.validData & PM_PERIODIC_BATTSTATE_VALID) ? QVariant(curr.bat2Percent.percent) : QVariant();
    	QVariant bat2charged = (curr.validData & PM_PERIODIC_BATTSTATE_VALID) ? QVariant((int) curr.bat2Percent.charged) : QVariant();

		query.bindValue(":ahr1man", ahr1man);
		query.bindValue(":ahr1exp", ahr1exp);
		query.bindValue(":ahr2man", ahr2man);
		query.bindValue(":ahr2exp", ahr2exp);
		query.bindValue(":ahr3man", ahr3man);
		query.bindValue(":ahr3exp", ahr3exp);
		query.bindValue(":whr1man", whr1man);
		query.bindValue(":whr1exp", whr1exp);
		query.bindValue(":whr2man", whr2man);
		query.bindValue(":whr2exp", whr2exp);
		query.bindValue(":maxTemp", maxTemp);
		query.bindValue(":minTemp", minTemp);
		query.bindValue(":volts1", volts1);
		query.bindValue(":amps1man", amps1man);
		query.bindValue(":amps1exp", amps1exp);
		query.bindValue(":volts2", volts2);
		query.bindValue(":bat1percent", bat1percent);
		query.bindValue(":bat1charged", bat1charged);
		query.bindValue(":bat2percent", bat1percent);
		query.bindValue(":bat2charged", bat1charged);

		if(!query.exec())
			return false;
	}

	return true;
}

// Constructs a ProfileLoggedValue from a linked list of PMProfileRecord
// structs and the battery, as returned by libpmcomm
ProfileLoggedValue::ProfileLoggedValue(struct PMProfileRecord *records, int battery) {
	loggedType = TYPE_PROFILE;
	this->battery = battery;
	while(records) {
		rs.prepend(*records);
		records = records->next;
	}
	initialized = true;	
}

// Constructs a ProfileLoggedValue from a particular download in the database
// for a particular battery
ProfileLoggedValue::ProfileLoggedValue(QSqlDatabase db, int downloadid, int battery) {
	loggedType = TYPE_PROFILE;
	this->battery = battery;

	QSqlQuery query(db);
	bool success = query.prepare("SELECT day,percent,volts,ampsman,ampsexp "
		"FROM profile WHERE downloadid = :downloadid AND battery = :battery "
		"ORDER BY recordid ASC;");

	if(!success)
		return;

	query.bindValue(":downloadid", downloadid);
	query.bindValue(":battery", battery);
	if(!query.exec())
		return;

	while(query.next()) {
		PMProfileRecord curr;
		memset(&curr, 0, sizeof(PMProfileRecord));

		curr.day = query.value(0).toInt();
		curr.percentFull = query.value(1).toInt();
		curr.volts = query.value(2).toInt();
		curr.amps.mantissa = query.value(3).toInt();
		curr.amps.exponent = query.value(4).toInt();
		rs.append(curr);
	}
	initialized = true;
}

// Determines if two profile records are exactly equal
bool ProfileLoggedValue::rowsEqual(struct PMProfileRecord & one, struct PMProfileRecord & two) {
	if(one.day != two.day)
		return false;
	if(one.percentFull != two.percentFull)
		return false;
	if(one.volts != two.volts)
		return false;
	if(one.amps.mantissa != two.amps.mantissa || one.amps.exponent != two.amps.exponent)
		return false;
	return true;
}

// Writes the headers for profile data to a file
void ProfileLoggedValue::writeHeadersToFile(QTextStream & loggedfile) {
	loggedfile << "Day,Percent Full,Filtered Volts,Filtered Amps" << endl;
}

// Writes the data in this object to a file
bool ProfileLoggedValue::writeToFile(QString filename) {
	QFile lf(filename);
	if(!lf.open(QIODevice::WriteOnly | QIODevice::Text))
		return false;
	QTextStream loggedfile(&lf);

	writeHeadersToFile(loggedfile);

	writeRowsToFile(loggedfile, 0, rs.size());
	
	return loggedfile.status() == QTextStream::Ok;
}

// Writes a set of ProfileLoggedValue objects, which should comprise a continuous record,
// to a file
bool ProfileLoggedValue::writeGroupToFile(QList<ProfileLoggedValue *> values, QString filename) {
	QFile lf(filename);
	if(!lf.open(QIODevice::WriteOnly | QIODevice::Text))
		return false;
	QTextStream loggedfile(&lf);

	values.last()->writeHeadersToFile(loggedfile);

	for(int i = 0; i < values.size(); i++) {
		int startRow = 0;
		bool extraRow = false;
		if(i != 0) {
			startRow = values[i]->findMatchingRow(*values[i - 1], extraRow);
		}

		int endRow = values[i]->rs.size();
		if(startRow < endRow && extraRow) {
			loggedfile << ",,," << endl;
		}

		values[i]->writeRowsToFile(loggedfile, startRow, endRow);
	}

	return loggedfile.status() == QTextStream::Ok;
}

// Finds the row in this object that corresponds to the last row in previous.
// If the rows don't match exactly, extraRow is set to true
int ProfileLoggedValue::findMatchingRow(ProfileLoggedValue & previous, bool & extraRow) {
	Q_ASSERT(context);
	Q_ASSERT(previous.context);

	if(previous.rs.empty())
		return 0;

	struct PMProfileRecord & prev = previous.rs.last();
	int startRow = 0;

	// Search for matching row
	extraRow = true; // True if a spacer row is to be inserted
	for(int row = 0; row < rs.size(); row++) {
		struct PMProfileRecord & curr = rs[row];
		if(rowsEqual(curr, prev)) {
			startRow = row + 1;
			extraRow = false;
			break;
		}
	}

	return startRow;
}

// Writes specified rows of this object to a file
void ProfileLoggedValue::writeRowsToFile(QTextStream & loggedfile, int startRow, int endRow) {
	for(int i = startRow; i < endRow; i++) {
	    struct PMProfileRecord & curr = rs[i];

	    loggedfile << (int) curr.day << ",";
	    LoggedDisplayValue percentVal(PM_D22, curr.percentFull);
	    loggedfile << percentVal.toString() << ",";
    	LoggedDisplayValue voltsVal(PM_D3, curr.volts);
     	loggedfile << voltsVal.toString() << ",";
     	LoggedDisplayValue ampsVal(PM_D10, curr.amps);
     	ampsVal.setPrecision((context->shuntTypes & (battery == 1 ? 1 : 2)) ? 2 : 1);
	    loggedfile << ampsVal.toString();
	    loggedfile << endl;
	}	
}

// Writes the data in this object to the database using a specified downloadid
bool ProfileLoggedValue::writeToDB(QSqlDatabase db, int downloadid) {
	QSqlQuery query(db);
	bool success = query.prepare("INSERT INTO profile (downloadid,recordid,battery,"
		"day,percent,volts,ampsman,ampsexp) VALUES (:downloadid,:recordid,:battery,"
		":day,:percent,:volts,:ampsman,:ampsexp);");

	if(!success)
		return false;

	for(int i = 0; i < rs.size(); i++) {
		struct PMProfileRecord & curr = rs[i];

		query.bindValue(":downloadid", downloadid);
		query.bindValue(":recordid", i+i);
		query.bindValue(":battery", battery);
		query.bindValue(":day", curr.day);
	    query.bindValue(":percent", curr.percentFull);
	    query.bindValue(":volts", curr.volts);
	    query.bindValue(":ampsman", curr.amps.mantissa);
	    query.bindValue(":ampsexp", curr.amps.exponent);

		if(!query.exec())
			return false;
	}

	return true;	
}

// Constructs an EfficiencyLoggedValue from an array list of PMEfficiencyRecord
// structs and the battery, as returned by libpmcomm
EfficiencyLoggedValue::EfficiencyLoggedValue(struct PMEfficiencyRecord *records, int nRecords, int battery) {
	loggedType = TYPE_EFFICIENCY;
	this->battery = battery;

	for(int i = 0; i < nRecords; i++) {
		rs.append(records[i]);
	}
}

// Constructs an EfficiencyLoggedValue from a particular download in the database
// for a particular battery
EfficiencyLoggedValue::EfficiencyLoggedValue(QSqlDatabase db, int downloadid, int battery) {
	loggedType = TYPE_EFFICIENCY;
	this->battery = battery;

	QSqlQuery query(db);
	bool success = query.prepare("SELECT endtime,valid,length,"
		"discharge,charge,net,efficiency,selfdischarge FROM efficiency "
		"WHERE downloadid = :downloadid AND battery = :battery ORDER BY recordid ASC;");

	if(!success)
		return;

	query.bindValue(":downloadid", downloadid);
	query.bindValue(":battery", battery);
	if(!query.exec())
		return;

	while(query.next()) {
		PMEfficiencyRecord curr;
		memset(&curr, 0, sizeof(PMEfficiencyRecord));

		curr.endTime = query.value(0).toInt();
		curr.validData = query.value(1).toBool();
		curr.cycleMinutes = query.value(2).toInt();
		curr.ahrDischarge = query.value(3).toInt();
		curr.ahrCharge = query.value(4).toInt();
		curr.ahrNet = query.value(5).toInt();
		curr.efficiency = query.value(6).toDouble();
		curr.selfDischarge = query.value(7).toDouble();
		rs.append(curr);
	}
	initialized = true;
}

// Determines if two efficiency records are exactly equal
bool EfficiencyLoggedValue::rowsEqual(struct PMEfficiencyRecord & one, struct PMEfficiencyRecord & two) {
	if(one.validData != two.validData || one.endTime != two.endTime)
		return false;
	if(one.ahrCharge != two.ahrCharge)
		return false;
	if(one.ahrDischarge != two.ahrDischarge)
		return false;
	if(one.cycleMinutes != two.cycleMinutes)
		return false;
	return true;
}

// Writes the headers for efficiency data to a file
void EfficiencyLoggedValue::writeHeadersToFile(QTextStream & loggedfile) {
	loggedfile << "Date Time,Valid,Cycle Hrs,Dis AHrs,Chrg AHrs,Net AHrs,Chrg Eff,Self DisChrg";
	int interval = AppSettings::getInstance()->getEfficiencyAverageInterval();
	loggedfile << "," << interval << " Cycle Chrg Eff," << interval << " Cycle Self DisChrg" << endl;
}

// Writes specified rows of this object to a file
void EfficiencyLoggedValue::writeRowsToFile(QTextStream & loggedfile, int startRow, int endRow, QList<struct PMEfficiencyRecord *> & lastFewRows) {
	int avgInterval = AppSettings::getInstance()->getEfficiencyAverageInterval();
	for(int i = startRow; i < endRow; i++) {
		struct PMEfficiencyRecord & curr = rs[i];

		while(lastFewRows.size() >= avgInterval)
			lastFewRows.removeFirst();

		int64_t measTime = context->realTime + ((int) curr.endTime - context->unitTime) * 60;
		QDateTime localTime = QDateTime::fromTime_t(measTime).toLocalTime();
		QString formattedTime = localTime.toString("MM/dd/yyyy hh:mm");

		loggedfile << formattedTime << ",";
		loggedfile << (curr.validData ? "TRUE" : "FALSE") << ",";
		if(curr.validData) {
			loggedfile << fixed << qSetRealNumberPrecision(2) << curr.cycleMinutes / 60.0 << ",";
			loggedfile << fixed << qSetRealNumberPrecision(2) << curr.ahrDischarge / -100.0 << ",";
			loggedfile << fixed << qSetRealNumberPrecision(2) << curr.ahrCharge / 100.0 << ",";
			loggedfile << fixed << qSetRealNumberPrecision(2) << curr.ahrNet / 100.0 << ",";
			loggedfile << fixed << qSetRealNumberPrecision(2) << curr.efficiency << ",";
			loggedfile << fixed << qSetRealNumberPrecision(2) << curr.selfDischarge << ",";

			// Append only if the row is good
			lastFewRows.append(&rs[i]);
			if(lastFewRows.size() == avgInterval) {
				// Compute the average
				int totalMinutes = 0;
				int totalDischarge = 0;
				int totalCharge = 0;
				foreach(struct PMEfficiencyRecord *currAvg, lastFewRows) {
					totalMinutes += currAvg->cycleMinutes;
					totalDischarge += currAvg->ahrDischarge;
					totalCharge += currAvg->ahrCharge;
				}
				double avgEfficiency = (double) totalDischarge * 100.0 / totalCharge;
				int totalNet = totalCharge - totalDischarge;
				double avgDischarege = ((double) totalNet / 100) / (totalMinutes / 60.0);

				loggedfile << fixed << qSetRealNumberPrecision(2) << avgEfficiency << ",";
				loggedfile << fixed << qSetRealNumberPrecision(2) << avgDischarege;
			} else {
				loggedfile << ",";
			}
		} else {
			loggedfile << ",,,,,,,";
		}
		loggedfile << endl;
	}
}

// Writes the data in this object to a file
bool EfficiencyLoggedValue::writeToFile(QString filename) {
	QFile lf(filename);
	if(!lf.open(QIODevice::WriteOnly | QIODevice::Text))
		return false;
	QTextStream loggedfile(&lf);

	writeHeadersToFile(loggedfile);
	
	QList<struct PMEfficiencyRecord *> dummy;
	writeRowsToFile(loggedfile, 0, rs.size(), dummy);

	return loggedfile.status() == QTextStream::Ok;
}

// Writes a set of EfficiencyLoggedValue objects, which should comprise a continuous record,
// to a file
bool EfficiencyLoggedValue::writeGroupToFile(QList<EfficiencyLoggedValue *> values, QString filename) {
	QFile lf(filename);
	if(!lf.open(QIODevice::WriteOnly | QIODevice::Text))
		return false;
	QTextStream loggedfile(&lf);

	values.last()->writeHeadersToFile(loggedfile);

	QList<struct PMEfficiencyRecord *> lastFewRows; // Used to allow averaging over multiple downloads
	for(int i = 0; i < values.size(); i++) {
		int startRow = 0;
		bool extraRow = false;
		if(i != 0) {
			startRow = values[i]->findMatchingRow(*values[i - 1], extraRow);
		}

		int endRow = values[i]->rs.size();
		if(startRow < endRow && extraRow) {
			loggedfile << ",,,,,,,,," << endl;
			lastFewRows.clear(); // Don't keep data around if there is a gap
		}

		values[i]->writeRowsToFile(loggedfile, startRow, endRow, lastFewRows);
	}

	return loggedfile.status() == QTextStream::Ok;
}

// Finds the row in this object that corresponds to the last row in previous.
// If the times don't match up to show that the record was continuous (i.e. power was
// lost), extraRow is set to true
int EfficiencyLoggedValue::findMatchingRow(EfficiencyLoggedValue & previous, bool & extraRow) {
	Q_ASSERT(context);
	Q_ASSERT(previous.context);

	if(previous.rs.empty())
		return 0;

	struct PMEfficiencyRecord & prev = previous.rs.last();
	int startRow = 0;

	uint64_t prevMeasTime = previous.context->realTime + ((int) prev.endTime - previous.context->unitTime) * 60;

	// Search for matching time
	extraRow = true; // True if a spacer row is to be inserted
	uint64_t bestTimeDiff = 0;
	bool approxMatch = false;
	for(int row = 0; row < rs.size(); row++) {
		struct PMEfficiencyRecord & curr = rs[row];
		uint64_t currMeasTime = context->realTime + ((int) curr.endTime - context->unitTime) * 60;
		uint64_t timeDiff = llabs(currMeasTime - prevMeasTime);
		bool timeMatch = timeDiff <= TIME_MATCH_THRESHOLD;
		if(rowsEqual(curr, prev)) {
			startRow = row + 1;
			extraRow = !timeMatch;
			break;
		}
		if(timeMatch && (!approxMatch || timeDiff < bestTimeDiff)) {
			approxMatch = true;
			bestTimeDiff = timeDiff;
			startRow = row + (currMeasTime > prevMeasTime ? 0 : 1);
		}
	}

	return startRow;
}

// Writes the data in this object to the database using a specified downloadid
bool EfficiencyLoggedValue::writeToDB(QSqlDatabase db, int downloadid) {
	QSqlQuery query(db);
	bool success = query.prepare("INSERT INTO efficiency (downloadid,recordid,battery,"
		"endtime,valid,length,discharge,charge,net,"
		"efficiency,selfdischarge) VALUES (:downloadid,:recordid,:battery,"
		":endtime,:valid,:length,:discharge,:charge,:net,"
		":efficiency,:selfdischarge);");

	if(!success)
		return false;

	for(int i = 0; i < rs.size(); i++) {
		struct PMEfficiencyRecord  & curr = rs[i];

	    query.bindValue(":downloadid", downloadid);
	    query.bindValue(":recordid", i+1);
	    query.bindValue(":battery", battery);
	    query.bindValue(":endtime", curr.endTime);
	    query.bindValue(":valid", (int) curr.validData);
	    query.bindValue(":length", curr.cycleMinutes);
	    query.bindValue(":discharge", curr.ahrDischarge);
	    query.bindValue(":charge", curr.ahrCharge);
	    query.bindValue(":net", curr.ahrNet);
	    query.bindValue(":efficiency", curr.efficiency);
	    query.bindValue(":selfdischarge", curr.selfDischarge);

		if(!query.exec())
			return false;
	}

	return true;
}

