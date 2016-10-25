#include "pmconnectionwrapper.h"

#include "libpmcomm.h"

#include <QDebug>

PMConnectionWrapper::PMConnectionWrapper(QString host, uint16_t port, QObject *parent) : QObject(parent), internet(true), host(host), port(port), conn(NULL), failFast(false) {}

PMConnectionWrapper::PMConnectionWrapper(QString serialPort, QObject *parent) : QObject(parent), internet(false), serialPort(serialPort), conn(NULL), failFast(false) {}

PMConnectionWrapper::~PMConnectionWrapper() {
	disconnectPM(-1);
}

static const int N_RETRIES = 2; // Number of retries for read/write/etc operations
static const int N_CONN_RETRIES = 1; // Number of attempts to reconnect

// Fetches data for a diven display, and emits displayDataError or displayDataReady when done
void PMConnectionWrapper::fetchDisplayData(enum PMDisplayNumber display, int id) {
	if(failFast) {
		emit displayDataError(display, id);
		return;		
	}
	if(conn == NULL)
		connectPM(id);
	if(conn == NULL) {
		emit displayDataError(display, id);
		return;
	}

	struct PMDisplayValue val;
	int err = 0;
	int reconnections = N_CONN_RETRIES;
	while(true) {
		for(int j = 0; j < N_RETRIES + 1; j++) {
			err = PMReadDisplayFormatted(conn, display, &val);
			if(err >= 0)
				break;
		}
		if(err >= 0)
			break;
		if(reconnections-- == 0)
			break;
		err = connectPM(id) ? 0 : PM_ERROR_CONNECTION;
		if(err < 0)
			break;
	}
	if(err < 0) {
		if(err == PM_ERROR_CONNECTION || err == PM_ERROR_COMMUNICATION)
			failFast = true;

	 	emit displayDataError(display, id);
	} else {
	 	DisplayValue v(display, val);
	 	emit displayDataReady(v, id);
	}
}

// Fetches data for a diven program, and emits programDataFetchError or programDataReady when done
void PMConnectionWrapper::fetchProgramData(enum PMProgramNumber program, int id) {
	if(failFast) {
		emit programDataFetchError(program, id);
		return;		
	}
	if(conn == NULL)
		connectPM(id);
	if(conn == NULL) {
		emit programDataFetchError(program, id);
		return;
	}

	enum PMProgramNumber currProgram = program;
	ProgramValue *v = NULL;
	while(v == NULL || v->needsMore(currProgram)) { // Do multiple reads if necessary
		union PMProgramData result;
		int err = 0;
		int reconnections = N_CONN_RETRIES;
		while(true) {
			for(int j = 0; j < N_RETRIES + 1; j++) {
				err = PMReadProgramFormatted(conn, currProgram, &result);
				if(err >= 0)
					break;
			}
			if(err >= 0)
				break;
			if(reconnections-- == 0)
				break;
			err = connectPM(id) ? 0 : PM_ERROR_CONNECTION;
			if(err < 0)
				break;
		}
		if(err < 0) {
			if(err == PM_ERROR_CONNECTION || err == PM_ERROR_COMMUNICATION)
				failFast = true;
			emit programDataFetchError(program, id);
			delete v;
			return;

		} else if (v == NULL) {
			v = ProgramValue::makeProgramValue(program, result);
			if(v == NULL) {
				emit programDataFetchError(program, id);
				return;
			}
		} else {
			if(!v->updateValue(currProgram, result)) {
				emit programDataFetchError(program, id);
				delete v;
				return;
			}
		}
	}
	emit programDataReady(QSharedPointer<ProgramValue>(v), id);
}

// Writes data for a diven program, and emits programDataSetError or programDataStored when done
void PMConnectionWrapper::setProgramData(enum PMProgramNumber program, QSharedPointer<ProgramValue> value, int id) {
	if(failFast) {
		emit programDataSetError(program, id);
		return;		
	}
	if(conn == NULL)
		connectPM(id);
	if(conn == NULL) {
		emit programDataSetError(program, id);
		return;
	}

	enum PMProgramNumber currProgram = program;
	bool first = true;
	while(first || value->needsMore(currProgram)) {
		first = false;
		union PMProgramData toStore;
		value->toRawValue(toStore, currProgram); // Works for both initial and extended fields
		int err = 0;
		int reconnections = N_CONN_RETRIES;
		while(true) {
			for(int j = 0; j < N_RETRIES + 1; j++) {
				err = PMWriteProgramFormatted(conn, currProgram, &toStore);
				if(err >= 0)
					break;
			}
			if(err >= 0)
				break;
			if(reconnections-- == 0)
				break;
			err = connectPM(id) ? 0 : PM_ERROR_CONNECTION;
			if(err < 0)
				break;
		}
		if(err < 0) {
			if(err == PM_ERROR_CONNECTION || err == PM_ERROR_COMMUNICATION)
				failFast = true;
			emit programDataSetError(program, id);
			return;
		}
	}
	emit programDataStored(program, id);
}

// Struct used to hold state the callbacks libpmcomm makes to indicate the current progress of a logged data download
struct callbackStatus {
	PMConnectionWrapper *wrapper;
	LoggedValue::LoggedDataType type;
	int id;
};

// Callback function called by libpmcomm to indicate the current progress of a logged data download
void PM_CALLCONV PMConnectionWrapperProgressCallback(int progress, int outof, void *usrdata) {
	callbackStatus *status = (callbackStatus *) usrdata;
	status->wrapper->handleLoggedCallback(status->id, status->type, progress, outof);
}

// Class member function called by the above callback
void PMConnectionWrapper::handleLoggedCallback(int id, LoggedValue::LoggedDataType type, int progress, int outof) {
	emit loggedDataProgress(type, id, progress, outof);
}

// Fetches a specified type of logged data, and emits loggedDataError or loggedDataReady when done
// Also periodically emits loggedDataProgress to indicate how far along the download is
void PMConnectionWrapper::fetchLoggedData(LoggedValue::LoggedDataType type, int id) {
	if(failFast) {
		emit loggedDataError(type, id);
		return;		
	}
	if(conn == NULL)
		connectPM(id);
	if(conn == NULL) {
		emit loggedDataError(type, id);
		return;
	}

	callbackStatus status;
	status.wrapper = this;
	status.type = type;
	status.id = id;

	struct PMPeriodicRecord *records = NULL;
	struct PMProfileRecord *battery1Records = NULL, *battery2Records = NULL;
	struct PMEfficiencyRecord *battery1Eff = NULL;
	struct PMEfficiencyRecord *battery2Eff = NULL;
	// Only allocate efficiency buffers if needed
	if(type == LoggedValue::TYPE_EFFICIENCY) {
		battery1Eff = new struct PMEfficiencyRecord[PM_MAX_EFFICIENCY_RECORDS];
		battery2Eff = new struct PMEfficiencyRecord[PM_MAX_EFFICIENCY_RECORDS];
	}
	int nRecords1, nRecords2;

	int err = 0;
	int reconnections = N_CONN_RETRIES;
	while(true) {
		for(int j = 0; j < N_RETRIES + 1; j++) {
			if(type == LoggedValue::TYPE_PERIODIC) {
				err = PMReadPeriodicData(conn, &records, PMConnectionWrapperProgressCallback, &status);
			} else if(type == LoggedValue::TYPE_PROFILE) {
				err = PMReadProfileData(conn, &battery1Records, &battery2Records,  PMConnectionWrapperProgressCallback, &status);
			} else if(type == LoggedValue::TYPE_EFFICIENCY) {
				err = PMReadEfficiencyData(conn, &nRecords1, battery1Eff, &nRecords2, battery2Eff, PMConnectionWrapperProgressCallback, &status);
			} else {
				Q_ASSERT(0);
			}
			if(err >= 0)
				break;
		}
		if(err >= 0)
			break;
		if(reconnections-- == 0)
			break;
		err = connectPM(id) ? 0 : PM_ERROR_CONNECTION;
		if(err < 0)
			break;
	}
	if(err < 0) {
		if(err == PM_ERROR_CONNECTION || err == PM_ERROR_COMMUNICATION)
			failFast = true;
		//If there was an error, only the efficiency buffers can still be allocated
		delete[] battery1Eff;
		delete[] battery2Eff;
	 	emit loggedDataError(type, id);
	} else {
		LoggedValue *v = NULL;
		LoggedValue *v2 = NULL;
		if(type == LoggedValue::TYPE_PERIODIC) {
			v = new PeriodicLoggedValue(records); // These always initialize correctly, so no need to check isValid()
			PMFreePeriodicData(records);
		} else if(type == LoggedValue::TYPE_PROFILE) {
			v = new ProfileLoggedValue(battery1Records, 1);
			v2 = new ProfileLoggedValue(battery2Records, 2);
			PMFreeProfileData(battery1Records);
			PMFreeProfileData(battery2Records);
		} else if(type == LoggedValue::TYPE_EFFICIENCY) {
			v = new EfficiencyLoggedValue(battery1Eff, nRecords1, 1);
			v2 = new EfficiencyLoggedValue(battery2Eff, nRecords2, 2);
			delete[] battery1Eff;
			delete[] battery2Eff;
		} else {
			Q_ASSERT(0);
		}

	 	emit loggedDataReady(QSharedPointer<LoggedValue>(v), QSharedPointer<LoggedValue>(v2), id);
	}
}

// Resets the specified type of data, and emits resetStatus (with a boolean indicating success) when done
void PMConnectionWrapper::resetPM(enum PMResetType command, int id) {
	if(failFast) {
		emit resetStatus(command, false, id);
		return;		
	}
	if(conn == NULL)
		connectPM(id);
	if(conn == NULL) {
		emit resetStatus(command, false, id);
		return;
	}

	int err = 0;
	int reconnections = N_CONN_RETRIES;
	while(true) {
		for(int j = 0; j < N_RETRIES + 1; j++) {
			err = PMReset(conn, command);
			if(err >= 0)
				break;
		}
		if(err >= 0)
			break;
		if(reconnections-- == 0)
			break;
		err = connectPM(id) ? 0 : PM_ERROR_CONNECTION;
		if(err < 0)
			break;
	}
	if(err == PM_ERROR_CONNECTION || err == PM_ERROR_COMMUNICATION)
		failFast = true;
	emit resetStatus(command, err >= 0, id);
}

// Resets the flag that causes all requests to immediately fail after certain kinds of errors
void PMConnectionWrapper::resetFailFast() {
	if(failFast)
		disconnectPM(-1);
	failFast = false;
}

// Connects to the PentaMetric. Typically this doesn't need to be called directly, since it will be called
// automatically when needed. Resets failFast, and disconnects first if necessary
bool PMConnectionWrapper::connectPM(int id) {
	failFast = false;
	if(conn != NULL)
		disconnectPM(id);
	if(internet) {
		QByteArray hostBytes = host.toUtf8();
		conn = PMOpenConnectionInet(hostBytes.constData(), port);
	} else {
		QByteArray serialBytes = serialPort.toUtf8();
		conn = PMOpenConnectionSerial(serialBytes.constData());
	}
	if(conn == NULL) {
		failFast = true;
		emit connectionError(id);
		return false;
	} else {
		emit connected(id, PMGetInterfaceVersion(conn));
		return true;
	}
}

// Disconnects from the PentaMetric. This is called automatically if the connection seems to be nonresponsive,
// and only needs to be called directly when it is important to be disconnected (such as when other code is
// testing whether a connection is possible)
void PMConnectionWrapper::disconnectPM(int id) {
	Q_UNUSED(id);
	failFast = false;
	if(conn == NULL)
		return;

	PMCloseConnection(conn);
	conn = NULL;
}
