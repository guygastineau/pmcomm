#ifndef LOGGEDVALUE_H
#define LOGGEDVALUE_H

#include "displayvalue.h"
#include "pmdefs.h"

#include <QList>
#include <QSqlDatabase>

class LoggedData;

class QTextStream;

/* This class represents a group of logged data records of a particular
   type, e.g. a bunch of periodic data records or a bunch of efficiency
   records for battery 1. Its subclasses provide specialized functionality
   for each type of logged data.

   Logged values can be constructed from a struct PMPeriodicRecord (from
   libpmcomm) or read out of the database, and can subsequently be written
   to a csv file or into the database.

   Each subclass has a static writeGroupToFile() method that writes
   a set of LoggedValue objects (i.e. a set of sets of records)
   to a file. This allows exporting continuous records out of the database
   into csv files.
   
   Each subclass also has writeRowsToFile() method that writes a particular
   group of records, starting at startRow and ending at endRow (indexed from 0)
   to a file. This is used by writeGroupToFile() to ensure that only
   non-overlapping data actually makes it into the file.
 */
class LoggedValue {
public:
	enum LoggedDataType {
		TYPE_PERIODIC,
		TYPE_PROFILE,
		TYPE_EFFICIENCY
	};

	LoggedValue();
	virtual ~LoggedValue() {}

	virtual bool writeToFile(QString filename) = 0;
	virtual bool writeToDB(QSqlDatabase db, int downloadid) = 0;

	// This function writes the header line (with the names of the columns)
	virtual void writeHeadersToFile(QTextStream & loggedfile) = 0;
	int getBattery();
	LoggedDataType type() { return loggedType; }

	void setContext(LoggedData *loggedData);

	bool isValid() { return initialized; }

protected:
	LoggedDataType loggedType;
	LoggedData *context;
	int battery;

	bool initialized; // True if the constructor succeeded
};

/* This subclass of DisplayValue is designed to represent values in the logged
   data that correspond to certain displays. This allows reusing the formatting
   code for displays here, since the behavior should be roughly the same.

   In particular, it handles PMScientificValue values with movable decimal points,
   which aren't supported by the normal DisplayValue class.
 */
class LoggedDisplayValue : public DisplayValue {
public:
	LoggedDisplayValue(enum PMDisplayNumber display, int value);
	LoggedDisplayValue(enum PMDisplayNumber display, struct PMScientificValue & value);

	double toDouble();
private:
	struct PMScientificValue scientificValue;
};

/* This class represents a group of periodic data records
 */
class PeriodicLoggedValue : public LoggedValue {
public:
	PeriodicLoggedValue(struct PMPeriodicRecord *records);
	PeriodicLoggedValue(QSqlDatabase db, int downloadid);

	bool writeToFile(QString filename);
	static bool writeGroupToFile(QList<PeriodicLoggedValue *> values, QString filename);
	bool writeToDB(QSqlDatabase db, int downloadid);

private:
	void writeHeadersToFile(QTextStream & loggedfile);
	void writeRowsToFile(QTextStream & loggedfile, int startRow, int endRow);
	int findMatchingRow(PeriodicLoggedValue & previous, bool & extraRow);
	static bool rowsEqual(struct PMPeriodicRecord & one, struct PMPeriodicRecord & two);
	QList<struct PMPeriodicRecord> rs;
};

class ProfileLoggedValue : public LoggedValue {
public:
	ProfileLoggedValue(struct PMProfileRecord *records, int battery);
	ProfileLoggedValue(QSqlDatabase db, int downloadid, int battery);

	bool writeToFile(QString filename);
	static bool writeGroupToFile(QList<ProfileLoggedValue *> values, QString filename);
	bool writeToDB(QSqlDatabase db, int downloadid);

private:
	void writeHeadersToFile(QTextStream & loggedfile);
	void writeRowsToFile(QTextStream & loggedfile, int startRow, int endRow);
	int findMatchingRow(ProfileLoggedValue & previous, bool & extraRow);
	static bool rowsEqual(struct PMProfileRecord & one, struct PMProfileRecord & two);
	QList<struct PMProfileRecord> rs;
};

class EfficiencyLoggedValue : public LoggedValue {
public:
	EfficiencyLoggedValue(struct PMEfficiencyRecord *records, int nRecords, int battery);
	EfficiencyLoggedValue(QSqlDatabase db, int downloadid, int battery);

	bool writeToFile(QString filename);
	static bool writeGroupToFile(QList<EfficiencyLoggedValue *> values, QString filename);
	bool writeToDB(QSqlDatabase db, int downloadid);

private:
	void writeHeadersToFile(QTextStream & loggedfile);
	void writeRowsToFile(QTextStream & loggedfile, int startRow, int endRow, QList<struct PMEfficiencyRecord *> & lastFewRows);
	int findMatchingRow(EfficiencyLoggedValue & previous, bool & extraRow);
	static bool rowsEqual(struct PMEfficiencyRecord & one, struct PMEfficiencyRecord & two);
	QList<struct PMEfficiencyRecord> rs;
};


#endif