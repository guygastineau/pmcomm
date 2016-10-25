#ifndef PMCONNECTIONWRAPPER_H
#define PMCONNECTIONWRAPPER_H

#include "displayvalue.h"
#include "programvalue.h"
#include "loggedvalue.h"

#include <QString>
#include <QObject>
#include <QSharedPointer>

// Function used for progress callbacks
void PM_CALLCONV PMConnectionWrapperProgressCallback(int progress, int outof, void *usrdata);

/* This class provides a wrapper around the struct PMConnection that has a more C++-friendly and
   Qt-friendly interface using signals and slots. It automatically handles connecting and
   reconnecting to the PentaMetric whenever it needs to be accessed. Through the use of signals
   and slots, the interface is entirely asynchronous: another object will send a signal to
   request an operation, and receive a signal back when it is done.

   To access the PentaMetric, send a signal to the appropriate slot on this object specifying
   the information to be read/written. You can also specify an id, which can be used to link
   a particular response to a particular request. An id of -1 indicates that there was no
   associated request. When the request finishes (or fails), the corresponding signal will be
   emitted.

   Note that all types of data, except display data, are wrapped in QSharedPointer for reference
   counting.
 */
class PMConnectionWrapper : public QObject {
	Q_OBJECT

public:
	PMConnectionWrapper(QString host, uint16_t port, QObject *parent = NULL);
	PMConnectionWrapper(QString serialPort, QObject *parent = NULL);
	~PMConnectionWrapper();

// All of these signals and slots are for requesting operations and receiving the results
signals:
	void displayDataReady(DisplayValue value, int id);
	void displayDataError(enum PMDisplayNumber display, int id);

	void programDataReady(QSharedPointer<ProgramValue> value, int id);
	void programDataFetchError(enum PMProgramNumber program, int id);
	void programDataStored(enum PMProgramNumber program, int id);
	void programDataSetError(enum PMProgramNumber program, int id);

	void loggedDataReady(QSharedPointer<LoggedValue> value, QSharedPointer<LoggedValue> value2, int id);
	void loggedDataError(LoggedValue::LoggedDataType, int id);
	void loggedDataProgress(LoggedValue::LoggedDataType type, int id, int progress, int outof);

	void resetStatus(enum PMResetType command, bool success, int id);

	void connected(int id, int version);
	void connectionError(int id);

public slots:
	void fetchDisplayData(enum PMDisplayNumber display, int id);

	void fetchProgramData(enum PMProgramNumber program, int id);
	void setProgramData(enum PMProgramNumber program, QSharedPointer<ProgramValue> value, int id);

	void fetchLoggedData(LoggedValue::LoggedDataType type, int id);

	void resetPM(enum PMResetType command, int id);

	bool connectPM(int id);
	void disconnectPM(int id);

	// Resets the flag that causes all requests to immediately fail after certain kinds of errors
	void resetFailFast();

private:
	void handleLoggedCallback(int id, LoggedValue::LoggedDataType type, int progress, int outof);
	friend void PM_CALLCONV PMConnectionWrapperProgressCallback(int progress, int outof, void *usrdata);

	bool internet;
	QString host;
	uint16_t port;
	QString serialPort;
	PMConnection *conn;

	bool failFast; // set to true when further requests should be ignored. Set on connection and
				   // repeated communication errors
};

#endif

