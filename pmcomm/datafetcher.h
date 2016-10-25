#ifndef DATAFETCHER_H
#define DATAFETCHER_H

#include "displayvalue.h"
#include "loggedvalue.h"
#include "programvalue.h"
#include "sitesettings.h"

#include <QObject>
#include <QSharedPointer>

class PMConnectionWrapper;

class QThread;

/* This class is not very interesting, and may be eliminated entirely at some point.
   All it does is accept signals on a slot, and forward them to an object on a
   different thread. In particular, it forwards signals to and from a PMConnectionWrapper,
   which it managers on another thread.
 */
class DataFetcher : public QObject {
	Q_OBJECT

public:
	// Set up a fetcher for the given PentaMetric (TCP/IP)
	DataFetcher(SiteSettings *settings, QObject *parent = NULL);
	~DataFetcher();

signals:
	void displayDataReady(DisplayValue &value, int id);
	void displayDataError(enum PMDisplayNumber display, int id);

	void programDataReady(QSharedPointer<ProgramValue> value, int id);
	void programDataFetchError(enum PMProgramNumber program, int id);
	void programDataStored(enum PMProgramNumber program, int id);
	void programDataSetError(enum PMProgramNumber program, int id);

	void loggedDataReady(QSharedPointer<LoggedValue> value, QSharedPointer<LoggedValue> value2, int id);
	void loggedDataError(LoggedValue::LoggedDataType type, int id);
	void loggedDataProgress(LoggedValue::LoggedDataType type, int id, int progress, int outof);

	void resetStatus(enum PMResetType command, bool success, int id);

	void connected(int id, int version);
	void connectionError(int id);

	// Signals for communicating with wrapper
	void privateConnect(int id);
	void privateDisconnect(int id);
	void privateResetFailFast();
	void privateFetchDisplayData(enum PMDisplayNumber display, int id);
	void privateFetchProgramData(enum PMProgramNumber program, int id);
	void privateSetProgramData(enum PMProgramNumber program, QSharedPointer<ProgramValue> value, int id);
	void privateFetchLoggedData(LoggedValue::LoggedDataType type, int id);
	void privateResetPM(enum PMResetType command, int id);

public slots:
	void fetchDisplayData(enum PMDisplayNumber display, int id);

	void fetchProgramData(enum PMProgramNumber program, int id);
	void setProgramData(enum PMProgramNumber program, QSharedPointer<ProgramValue> value, int id);

	void fetchLoggedData(LoggedValue::LoggedDataType type, int id);

	void resetPM(enum PMResetType command, int id);
	
	void resetFailFast();

	// Slots for communicating with wrapper
	void privateEmitDisplayReady(DisplayValue value, int id);
	void privateEmitDisplayError(enum PMDisplayNumber display, int id);
	void privateEmitProgramReady(QSharedPointer<ProgramValue> value, int id);
	void privateEmitProgramFetchError(enum PMProgramNumber program, int id);
	void privateEmitProgramStored(enum PMProgramNumber program, int id);
	void privateEmitProgramSetError(enum PMProgramNumber program, int id);
	void privateEmitLoggedReady(QSharedPointer<LoggedValue> value, QSharedPointer<LoggedValue> value2, int id);
	void privateEmitLoggedError(LoggedValue::LoggedDataType type, int id);
	void privateEmitLoggedProgress(LoggedValue::LoggedDataType type, int id, int progress, int outof);
	void privateEmitResetStatus(enum PMResetType command, bool success, int id);
	void privateEmitConnected(int id, int version);
	void privateEmitConnectionError(int id);

private:
	QThread *thread;
	PMConnectionWrapper *wrapper;
};

#endif // DATAFETCHER_H
