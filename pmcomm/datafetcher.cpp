#include "datafetcher.h"

#include "pmdefs.h"
#include "pmconnectionwrapper.h"

#include <QThread>

DataFetcher::DataFetcher(SiteSettings *settings, QObject *parent) : QObject(parent) {
	thread = new QThread();
	if(settings->isInternet()) {
		wrapper = new PMConnectionWrapper(settings->getHostname(), settings->getPort());
	} else {
		wrapper = new PMConnectionWrapper(settings->getSerialPort());
	}
	wrapper->moveToThread(thread);

	connect(thread, SIGNAL(finished()), wrapper, SLOT(deleteLater()));
	connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));

	// Prepare types for queued connections
    qRegisterMetaType<uint16_t>("uint16_t");
    qRegisterMetaType<DisplayValue>("DisplayValue");
   	qRegisterMetaType<QSharedPointer<ProgramValue> >("QSharedPointer<ProgramValue>");
    qRegisterMetaType<enum PMDisplayNumber>("enum PMDisplayNumber");
    qRegisterMetaType<enum PMProgramNumber>("enum PMProgramNumber");
    qRegisterMetaType<enum PMResetType>("enum PMResetType");
    qRegisterMetaType<LoggedValue::LoggedDataType>("LoggedValue::LoggedDataType");
    qRegisterMetaType<QSharedPointer<LoggedValue> >("QSharedPointer<LoggedValue>");

	// Queued connections
	connect(this, SIGNAL(privateResetFailFast()), wrapper, SLOT(resetFailFast()));
	connect(this, SIGNAL(privateFetchDisplayData(enum PMDisplayNumber, int)), wrapper, SLOT(fetchDisplayData(enum PMDisplayNumber, int)));
	connect(this, SIGNAL(privateFetchProgramData(enum PMProgramNumber, int)), wrapper, SLOT(fetchProgramData(enum PMProgramNumber, int)));
	connect(this, SIGNAL(privateSetProgramData(enum PMProgramNumber, QSharedPointer<ProgramValue>, int)), wrapper, SLOT(setProgramData(enum PMProgramNumber, QSharedPointer<ProgramValue>, int)));
	connect(this, SIGNAL(privateFetchLoggedData(LoggedValue::LoggedDataType, int)), wrapper, SLOT(fetchLoggedData(LoggedValue::LoggedDataType, int)));
	connect(this, SIGNAL(privateResetPM(enum PMResetType, int)), wrapper, SLOT(resetPM(enum PMResetType, int)));

	connect(wrapper, SIGNAL(displayDataReady(DisplayValue, int)), this, SLOT(privateEmitDisplayReady(DisplayValue, int)));
	connect(wrapper, SIGNAL(displayDataError(enum PMDisplayNumber, int)), this, SLOT(privateEmitDisplayError(enum PMDisplayNumber, int)));
	connect(wrapper, SIGNAL(programDataReady(QSharedPointer<ProgramValue>, int)), this, SLOT(privateEmitProgramReady(QSharedPointer<ProgramValue>, int)));
	connect(wrapper, SIGNAL(programDataFetchError(enum PMProgramNumber, int)), this, SLOT(privateEmitProgramFetchError(enum PMProgramNumber, int)));
	connect(wrapper, SIGNAL(programDataStored(enum PMProgramNumber, int)), this, SLOT(privateEmitProgramStored(enum PMProgramNumber, int)));
	connect(wrapper, SIGNAL(programDataSetError(enum PMProgramNumber, int)), this, SLOT(privateEmitProgramSetError(enum PMProgramNumber, int)));
	connect(wrapper, SIGNAL(loggedDataReady(QSharedPointer<LoggedValue>, QSharedPointer<LoggedValue>, int)), this, SLOT(privateEmitLoggedReady(QSharedPointer<LoggedValue>, QSharedPointer<LoggedValue>, int)));
	connect(wrapper, SIGNAL(loggedDataError(LoggedValue::LoggedDataType, int)), this, SLOT(privateEmitLoggedError(LoggedValue::LoggedDataType, int)));
	connect(wrapper, SIGNAL(loggedDataProgress(LoggedValue::LoggedDataType, int, int, int)), this, SLOT(privateEmitLoggedProgress(LoggedValue::LoggedDataType, int, int, int)));
	connect(wrapper, SIGNAL(resetStatus(enum PMResetType, bool, int)), this, SLOT(privateEmitResetStatus(enum PMResetType, bool, int)));
	connect(wrapper, SIGNAL(connected(int, int)), this, SLOT(privateEmitConnected(int, int)));
	connect(wrapper, SIGNAL(connectionError(int)), this, SLOT(privateEmitConnectionError(int)));
}

DataFetcher::~DataFetcher() {
	thread->quit();
}

void DataFetcher::resetFailFast() {
	if(!thread->isRunning())
		return; // If there's no thread, there's no wrapper. The wrapper will be initialized with this already reset

	emit privateResetFailFast();
}
	
void DataFetcher::fetchDisplayData(enum PMDisplayNumber display, int id) {
	if(!thread->isRunning())
		thread->start();

	emit privateFetchDisplayData(display, id);
}

void DataFetcher::fetchProgramData(enum PMProgramNumber program, int id) {
	if(!thread->isRunning())
		thread->start();
	
	emit privateFetchProgramData(program, id);
}

void DataFetcher::setProgramData(enum PMProgramNumber program, QSharedPointer<ProgramValue> value, int id) {
	if(!thread->isRunning())
		thread->start();

	emit privateSetProgramData(program, value, id);
}

void DataFetcher::fetchLoggedData(LoggedValue::LoggedDataType type, int id) {
	if(!thread->isRunning())
		thread->start();

	emit privateFetchLoggedData(type, id);
}

void DataFetcher::resetPM(enum PMResetType command, int id) {
	if(!thread->isRunning())
		thread->start();

	emit privateResetPM(command, id);
}

void DataFetcher::privateEmitDisplayReady(DisplayValue value, int id) {
	emit displayDataReady(value, id);
}

void DataFetcher::privateEmitDisplayError(enum PMDisplayNumber display, int id) {
	emit displayDataError(display, id);
}

void DataFetcher::privateEmitProgramReady(QSharedPointer<ProgramValue> value, int id) {
	emit programDataReady(value, id);
}

void DataFetcher::privateEmitProgramFetchError(enum PMProgramNumber program, int id) {
	emit programDataFetchError(program, id);
}

void DataFetcher::privateEmitProgramStored(enum PMProgramNumber program, int id) {
	emit programDataStored(program, id);
}

void DataFetcher::privateEmitProgramSetError(enum PMProgramNumber program, int id) {
	emit programDataSetError(program, id);
}

void DataFetcher::privateEmitLoggedReady(QSharedPointer<LoggedValue> value, QSharedPointer<LoggedValue> value2, int id) {
	emit loggedDataReady(value, value2, id);
}

void DataFetcher::privateEmitLoggedError(LoggedValue::LoggedDataType type, int id) {
	emit loggedDataError(type, id);
}

void DataFetcher::privateEmitLoggedProgress(LoggedValue::LoggedDataType type, int id, int progress, int outof) {
	emit loggedDataProgress(type, id, progress, outof);
}

void DataFetcher::privateEmitResetStatus(enum PMResetType command, bool success, int id) {
	emit resetStatus(command, success, id);
}

void DataFetcher::privateEmitConnected(int id, int version) {
	emit connected(id, version);
}

void DataFetcher::privateEmitConnectionError(int id) {
	emit connectionError(id);
}
