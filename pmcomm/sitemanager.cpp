#include "sitemanager.h"

#include "appsettings.h"
#include "datafetcher.h"

#include <QTimer>

#include <QDebug>

static const int MAX_OUTSTANDING_REQUESTS = 2;

SiteManager::SiteManager(QSharedPointer<SiteSettings> settings, QObject *parent) : QObject(parent), settings(settings) {
	fetcher = NULL;
	battery1Present = false;
	battery2Present = false;
	version.first = -1;
	updating = false;
	shouldUpdate = false;
	errorPending = ErrorOK;
	labelsValid = false;
	forceDisconnect = false;
	nextId = 0;
	outstandingLoggedRequest.second = -1;
	downloadingLogged = false;
	memset(smallShunt, 0, sizeof(smallShunt));
	displays.insert(PM_DALARM); // Make sure the alarm state is always there

	programDataTimer = new QTimer(this);
	programDataTimer->setInterval(120000);

	connect(programDataTimer, SIGNAL(timeout()), this, SLOT(updateProgramData()));
}

DataFetcher *SiteManager::getFetcher() {
	if(forceDisconnect)
		return NULL;

	if(errorPending == ErrorAccepted) { // If we are trying to do any operation, the user decided to retry
		errorPending = ErrorCleared;
	}

	if(fetcher != NULL) {
		return fetcher;
	}

	fetcher = new DataFetcher(settings.data(), this);

	connect(fetcher, SIGNAL(connected(int, int)), this, SLOT(handleConnected(int, int)));
	connect(fetcher, SIGNAL(displayDataReady(DisplayValue &, int)), this, SLOT(handleData(DisplayValue &, int)));
    connect(fetcher, SIGNAL(displayDataError(enum PMDisplayNumber, int)), this, SLOT(displayDataError(enum PMDisplayNumber, int)));
	connect(fetcher, SIGNAL(programDataReady(QSharedPointer<ProgramValue>, int)), this, SLOT(handleProgramData(QSharedPointer<ProgramValue>, int)));
    connect(fetcher, SIGNAL(programDataFetchError(enum PMProgramNumber, int)), this, SLOT(fetchError(enum PMProgramNumber, int)));
	connect(fetcher, SIGNAL(programDataStored(enum PMProgramNumber, int)), this, SLOT(programDataStored(enum PMProgramNumber, int)));
	connect(fetcher, SIGNAL(programDataSetError(enum PMProgramNumber, int)), this, SLOT(setError(enum PMProgramNumber, int)));
	connect(fetcher, SIGNAL(resetStatus(enum PMResetType, bool, int)), this, SLOT(resetStatus(enum PMResetType, bool, int)));
	connect(fetcher, SIGNAL(loggedDataReady(QSharedPointer<LoggedValue>, QSharedPointer<LoggedValue>, int)), this, SLOT(handleLoggedData(QSharedPointer<LoggedValue>, QSharedPointer<LoggedValue>, int)));
	connect(fetcher, SIGNAL(loggedDataError(LoggedValue::LoggedDataType, int)), this, SLOT(loggedError(LoggedValue::LoggedDataType, int)));
	connect(fetcher, SIGNAL(loggedDataProgress(LoggedValue::LoggedDataType, int, int, int)), this, SLOT(handleLoggedProgress(LoggedValue::LoggedDataType, int, int, int)));

	return fetcher;
}

void SiteManager::releaseFetcher() {
	stopUpdating();
	forceDisconnect = true;

	if(fetcher == NULL)
		return;

	delete fetcher;
	fetcher = NULL;
}

void SiteManager::reconnect() {
	forceDisconnect = false;

	// Treat all pending requests as errored out
	foreach(enum PMProgramNumber program, outstandingFetchRequests.keys()) {
		fetchErrors.insert(program);
	}
	outstandingFetchRequests.clear();

	foreach(enum PMProgramNumber program, outstandingSetRequests.keys()) {
		setErrors.insert(program);
	}
	outstandingSetRequests.clear();

	foreach(enum PMResetType command, outstandingResetRequests.keys()) {
		resetErrors.insert(command);
	}
	outstandingResetRequests.clear();

	if(outstandingLoggedRequest.second != -1) {
		loggedRequested.append(outstandingLoggedRequest.first);
		outstandingLoggedRequest.second = -1;
	}

	errorPending = ErrorHappened;
	// Now, retry all of these "errored out" requests
	finishError(true, true);
}

void SiteManager::startUpdating() {
	if(updating || forceDisconnect)
		return;

	updating = true;
	shouldUpdate = true;
	updateProgramData();
	programDataTimer->start();
	requestDisplays();
	emit statusChanged(getStatus());
}

void SiteManager::stopUpdating() {
	shouldUpdate = false;
	stopUpdatingImpl();
}

void SiteManager::stopUpdatingError() {
	stopUpdatingImpl();
}

void SiteManager::stopUpdatingImpl() {
	updating = false;
	updatedDisplays.clear();
	upcomingRequests.clear();
	outstandingRequests.clear();
	programDataTimer->stop();

	emit allDisplaysUpdated();
	emit statusChanged(getStatus());
}

void SiteManager::handleConnected(int id, int version) {
	Q_UNUSED(id);

	interfaceVersion.first = version / 10;
	interfaceVersion.second = version % 10;

	emit versionUpdated();
}

void SiteManager::addDisplay(enum PMDisplayNumber display) {
	displays.insert(display);

	requestIfNeeded(display);
}

void SiteManager::removeDisplay(enum PMDisplayNumber display) {
	if(display == PM_DALARM) // Don't ever remove the alarm state
		return;

	displays.remove(display);
	upcomingRequests.removeAll(display);
	outstandingRequests.remove(display); // Do I really want to do this?
	updatedDisplays.remove(display);
}

bool SiteManager::batteryPresent(int battery) {
	if(battery == 1)
		return battery1Present;
	if(battery == 2)
		return battery2Present;
	return false;
}

DisplayValue SiteManager::getDisplay(enum PMDisplayNumber display) {
	if(currentValues.contains(display) && displayValid(display)) {
		return currentValues[display];
	}

	return DisplayValue();
}

bool SiteManager::displayValid(enum PMDisplayNumber display) {
	switch(display) {
		case PM_D22:
		case PM_D24:
		case PM_D26: return battery1Present;

		case PM_D23:
		case PM_D25:
		case PM_D27: return battery2Present;

		default: return true;
	}
}

bool SiteManager::displayUpToDate(enum PMDisplayNumber display) {
	if(!displayValid(display))
		return false;

	return updatedDisplays.contains(display);
}

QString SiteManager::ampsLabelStatic(QString prefix, QString baseLabel, int ampsChannel, enum PMAmpsLabel *ampsLabels) {
	enum PMAmpsLabel label = ampsLabels[ampsChannel - 1];

	switch(label) {
		default:
		case PM_AMPSLABEL_AMPSNUM: return QString("%1 %2 %3").arg(prefix).arg(baseLabel).arg(ampsChannel);
		case PM_AMPSLABEL_SOLAR: return QString("%1 Solar %2").arg(prefix).arg(baseLabel);
		case PM_AMPSLABEL_WIND: return QString("%1 Wind %2").arg(prefix).arg(baseLabel);
		case PM_AMPSLABEL_HYDRO: return QString("%1 Hydro %2").arg(prefix).arg(baseLabel);
		case PM_AMPSLABEL_LOAD: return QString("%1 Load %2").arg(prefix).arg(baseLabel);
		case PM_AMPSLABEL_BATTERY: return QString("%1 Battery %2").arg(prefix).arg(baseLabel);
		case PM_AMPSLABEL_BATTERY_1: return QString("%1 Battery %2 %3").arg(prefix).arg(ampsChannel).arg(baseLabel);
	}
}

QString SiteManager::voltsLabelStatic(QString prefix, int voltsChannel, enum PMBatteryLabel batteryLabel) {
	if(voltsChannel != 1) {
		batteryLabel = PM_BATLABEL_BATTERY_1;
	}

	if(batteryLabel == PM_BATLABEL_BATTERY) {
		return QString("%1 Battery Volts").arg(prefix);
	} else {
		return QString("%1 Battery %2 Volts").arg(prefix).arg(voltsChannel);
	}
}

QString SiteManager::getLabelStatic(enum PMDisplayNumber display, struct PMLabels & currLabels) {
	switch(display) {
		case PM_D1: return voltsLabelStatic("AD1:", 1, currLabels.battLabel);
		case PM_D2: return voltsLabelStatic("AD2:", 2, currLabels.battLabel);
		case PM_D3: return voltsLabelStatic("AD3: Average", 1, currLabels.battLabel);
		case PM_D4: return voltsLabelStatic("AD4: Average", 2, currLabels.battLabel);
		case PM_D7: return ampsLabelStatic("AD7:", "Amps", 1, currLabels.ampsLabels);
		case PM_D8: return ampsLabelStatic("AD8:", "Amps", 2, currLabels.ampsLabels);
		case PM_D9: return ampsLabelStatic("AD9:", "Amps", 3, currLabels.ampsLabels);
		case PM_D10: return ampsLabelStatic("AD10: Average", "Amps", 1, currLabels.ampsLabels);
		case PM_D11: return ampsLabelStatic("AD11: Average", "Amps", 2, currLabels.ampsLabels);
		case PM_D12: return ampsLabelStatic("AD12: Average", "Amps", 3, currLabels.ampsLabels);
		case PM_D13: return ampsLabelStatic("AD13:", "Amp Hours", 1, currLabels.ampsLabels);
		case PM_D14: return ampsLabelStatic("AD14:", "Amp Hours", 2, currLabels.ampsLabels);
		case PM_D15: return ampsLabelStatic("AD15:", "Amp Hours", 3, currLabels.ampsLabels);
		case PM_D16: return ampsLabelStatic("AD16: Cumulative", "Ah", 1, currLabels.ampsLabels);
		case PM_D17: return ampsLabelStatic("AD17: Cumulative", "Ah", 2, currLabels.ampsLabels);
		case PM_D18: return ampsLabelStatic("AD18:", "Watts", 1, currLabels.ampsLabels);
		case PM_D19: return ampsLabelStatic("AD19:", "Watts", 2, currLabels.ampsLabels);
		case PM_D20: return ampsLabelStatic("AD20:", "Watt Hours", 1, currLabels.ampsLabels);
		case PM_D21: return ampsLabelStatic("AD21:", "Watt Hours", 2, currLabels.ampsLabels);
		case PM_D22: return "AD22: Battery 1 Percent Full";
		case PM_D23: return "AD23: Battery 2 Percent Full";
		case PM_D24: return "AD24: Battery 1 DSC";
		case PM_D25: return "AD25: Battery 2 DSC";
		case PM_D26: return "AD26: Battery 1 DSE";
		case PM_D27: return "AD27: Battery 2 DSE";
		case PM_D28: return QString::fromUtf8("AD28: Temperature (°%1)").arg((AppSettings::getInstance()->getTempUnit() == AppSettings::Fahrenheit) ? "F" : "C");
		case PM_D29: return "AD29: Battery 1 1-cycle Efficiency";
		case PM_D30: return "AD30: Battery 1 1-cycle Self-Discharge";
		case PM_D31: return "AD31: Battery 1 4-cycle Efficiency";
		case PM_D32: return "AD32: Battery 1 4-cycle Self-Discharge";
		case PM_D33: return "AD33: Battery 1 15-cycle Efficiency";
		case PM_D34: return "AD34: Battery 1 15-cycle Self-Discharge";
		case PM_D35: return "AD35: Battery 2 1-cycle Efficiency";
		case PM_D36: return "AD36: Battery 2 1-cycle Self-Discharge";
		case PM_D37: return "AD37: Battery 2 4-cycle Efficiency";
		case PM_D38: return "AD38: Battery 2 4-cycle Self-Discharge";
		case PM_D39: return "AD39: Battery 2 15-cycle Efficiency";
		case PM_D40: return "AD40: Battery 2 15-cycle Self-Discharge";
		case PM_DALARM: return "Alarm status";

		default: return "INVALID";
	}
}

QString SiteManager::getLabelStatic(enum PMDisplayNumber display, SiteManager *manager) {
	if(manager != NULL && manager->labelsValid) {
		return getLabelStatic(display, manager->labels);
	} else {
		struct PMLabels currLabels;
		currLabels.battLabel = PM_BATLABEL_BATTERY_1;

		for(int i = 0; i < 3; i++) {
			currLabels.ampsLabels[i] = PM_AMPSLABEL_AMPSNUM;
		}
		return getLabelStatic(display, currLabels);
	}
}

QString SiteManager::getLabel(enum PMDisplayNumber display) {
	return getLabelStatic(display, this);
}

QSharedPointer<SiteSettings> SiteManager::getSettings() {
	return settings;
}

// Request displays as needed
void SiteManager::requestDisplays() {
	if(!updating)
		return;

	bool refilled = false;
	while(outstandingRequests.size() < MAX_OUTSTANDING_REQUESTS) {
		if(upcomingRequests.isEmpty()) {
			if(refilled) // Displays can't be empty, since it always contains at least PM_DALARM
				return;
			refilled = true;
			upcomingRequests = displays.toList();
			qSort(upcomingRequests);
		}

		enum PMDisplayNumber display = upcomingRequests.takeFirst();
		if(displayValid(display) && !outstandingRequests.contains(display)) {
			qDebug() << "Fetching display " << display;
			int id = generateId();
			getFetcher()->fetchDisplayData(display, id);
			outstandingRequests.insert(display, id);
		}
	}
}

void SiteManager::requestIfNeeded(enum PMDisplayNumber display) {
	if(!updating || upcomingRequests.contains(display))
		return;

	upcomingRequests.append(display);
}

void SiteManager::handleData(DisplayValue & data, int id) {
	if(errorPending == ErrorCleared) {
		errorPending = ErrorOK;
		emit statusChanged(getStatus());
	}

	enum PMDisplayNumber display = data.displayNum();

	int ampsChannel = data.ampsChannel();
	if(ampsChannel != 0) {
		data.setPrecision(smallShunt[ampsChannel - 1] ? 2 : 1);
	}

	currentValues[display] = data;
	outstandingRequests.remove(display, id);

	if(updating)
		updatedDisplays.insert(display);

	// continue to issue requests
	requestDisplays();
	emit displayUpdated(display);
}

void SiteManager::displayDataError(enum PMDisplayNumber display, int id) {
	if(outstandingRequests.contains(display, id)) {
		bool newError = errorPending != ErrorHappened;
		errorPending = ErrorHappened;
		stopUpdatingError();

		if(newError)
			emit communicationsError();
	}
}

void SiteManager::fetchProgram(enum PMProgramNumber program) {
	if(forceDisconnect)
		return;

	if(!outstandingFetchRequests.contains(program) && !fetchErrors.contains(program)) {
		int id = generateId();
		getFetcher()->fetchProgramData(program, id);
		outstandingFetchRequests.insert(program, id);
		qDebug() << "Fetching program " << program;
	}
}

void SiteManager::updateProgramData() {
	qDebug() << "Updating program data";
	// Determine if batteries are connected
	fetchProgram(PM_P14);
	fetchProgram(PM_P15);

	// Get shunt type
	fetchProgram(PM_P11_13);

	// Get correct labels
	fetchProgram(PM_P6_9);

	// Version-dependent things (currently only existence of TCP/IP settings)
	fetchProgram(PM_P_VERSION);

	// Get alarm levels
	fetchProgram(PM_P22_23);
	fetchProgram(PM_P24_25);
}

QSharedPointer<ProgramValue> SiteManager::getProgram(enum PMProgramNumber program) {
	if(programValues.contains(program))
		return programValues[program];

	return QSharedPointer<ProgramValue>();
}

void SiteManager::handleProgramData(QSharedPointer<ProgramValue> value, int id) {
	qDebug() << "Got program data";
	if(errorPending == ErrorCleared) {
		errorPending = ErrorOK;
		emit statusChanged(getStatus());
	}

	enum PMProgramNumber progNum = value->progNum();
	if(!outstandingFetchRequests.contains(progNum, id))
		return;
	outstandingFetchRequests.remove(progNum, id);

	QSharedPointer<ProgramValue> cloned = QSharedPointer<ProgramValue>(value->clone());
	programValues[progNum] = cloned;

	handleProgramChange(progNum);
}

// This is called when program data is either read or written
void SiteManager::handleProgramChange(enum PMProgramNumber progNum) {
	switch(progNum) {
		case PM_P14: // Battery 1 present
			if(static_cast<CapacityProgramValue *>(programValues[progNum].data())->batteryPresent()) {
				battery1Present = true;
				requestIfNeeded(PM_D22);
				requestIfNeeded(PM_D24);
				requestIfNeeded(PM_D26);
			} else {
				battery1Present = false;
			}
			emit displayUpdated(PM_D22);
			emit displayUpdated(PM_D24);
			emit displayUpdated(PM_D26);
			if(downloadingLogged)
				loggedData->setBatteryEnabled(1, battery1Present);
			emit batteryPresenceUpdated(1, battery1Present);
			break;

		case PM_P15: // Battery 2 present
			if(static_cast<CapacityProgramValue *>(programValues[progNum].data())->batteryPresent()) {
				battery2Present = true;
				requestIfNeeded(PM_D23);
				requestIfNeeded(PM_D25);
				requestIfNeeded(PM_D27);
			} else {
				battery2Present = false;
			}
			emit displayUpdated(PM_D23);
			emit displayUpdated(PM_D25);
			emit displayUpdated(PM_D27);
			if(downloadingLogged)
				loggedData->setBatteryEnabled(2, battery2Present);
			emit batteryPresenceUpdated(2, battery2Present);
			break;

		case PM_P6_9: // Volts/amps labels
			static_cast<LabelsProgramValue *>(programValues[progNum].data())->getLabels(labels);
			labelsValid = true;
			if(downloadingLogged)
				loggedData->setLabels(labels);
			emit labelsUpdated();
			break;

		case PM_P11_13: // Shunt types
			ShuntTypesProgramValue *shuntTypes;
			shuntTypes = static_cast<ShuntTypesProgramValue *>(programValues[progNum].data());
			for(int i = 0; i < 3; i++)
				smallShunt[i] = (shuntTypes->shuntType(i + 1) == PM_SHUNTTYPE_100A100MV);
			if(downloadingLogged)
				loggedData->setShuntTypes(smallShunt);
			break;

		case PM_P38: // Unit time
			int rawTime;
			rawTime = static_cast<TimeProgramValue *>(programValues[progNum].data())->getRawMinutes();
			if(downloadingLogged)
				loggedData->setUnitTime(rawTime);
			break;

		case PM_P_VERSION: // Input unit version
			VersionProgramValue *versionValue;
			versionValue = static_cast<VersionProgramValue *>(programValues[progNum].data());
			version = QPair<int, int>(versionValue->versionMajor(), versionValue->versionMinor());
			emit versionUpdated();
			break;

		default:
			break;		
	}

	// If we were waiting for metadata, announce that the logged data is ready
	if(downloadingLogged && loggedRequested.empty() && loggedData->metadataReady()) {
		downloadingLogged = false;
		emit statusChanged(getStatus());
		emit loggedDataDownloaded();
	}
	
	emit programUpdated(progNum);
}

void SiteManager::fetchError(enum PMProgramNumber program, int id) {
	qWarning() << "Entered fetch error" << program;
	if(outstandingFetchRequests.contains(program, id)) {
		outstandingFetchRequests.remove(program, id);
		fetchErrors.insert(program);
		bool newError = errorPending != ErrorHappened;
		errorPending = ErrorHappened;
		// We have an error
		stopUpdatingError();
		if(newError)
			emit communicationsError();
	}
}

void SiteManager::saveProgramData(QSharedPointer<ProgramValue> & value) {
	if(forceDisconnect)
		return;

	enum PMProgramNumber program = value->progNum();
	programValues[program] = value;

	int id = generateId();
	outstandingSetRequests.insert(program, id);
	getFetcher()->setProgramData(program, value, id);

	handleProgramChange(program);
}

void SiteManager::programDataStored(enum PMProgramNumber program, int id) {
	if(outstandingSetRequests.contains(program, id)) {
		outstandingSetRequests.remove(program, id);
		if(errorPending == ErrorCleared) {
			errorPending = ErrorOK;
			emit statusChanged(getStatus());
		}

		if(outstandingSetRequests.isEmpty())
			emit writingFinished();
	}
}

void SiteManager::setError(enum PMProgramNumber program, int id) {
	qWarning() << "Entered set error";
	if(outstandingSetRequests.contains(program, id)) {
		outstandingSetRequests.remove(program, id);
		setErrors.insert(program);
		bool newError = errorPending != ErrorHappened;
		errorPending = ErrorHappened;
		// We have an error
		stopUpdatingError();
		if(newError)
			emit communicationsError();
	}
}

// Tells this object what to do about a pending error
// set noError to true to immediately go back to the ErrorOK state
void SiteManager::finishError(bool retry, bool noError) {
	if(forceDisconnect)
		return;
	if(errorPending != ErrorHappened)
		return;

	if(fetcher != NULL) // This must come before setting errorPending to ErrorAccepted
		fetcher->resetFailFast(); // Make sure future operations don't fail

	if(retry) {
		if(noError)
			errorPending = ErrorOK;
		else
			errorPending = ErrorCleared;

		if(shouldUpdate)
			startUpdating();

		foreach(enum PMProgramNumber program, fetchErrors) {
			int id = generateId();
			getFetcher()->fetchProgramData(program, id);
			outstandingFetchRequests.insert(program, id);
		}
		fetchErrors.clear();

		foreach(enum PMProgramNumber program, setErrors) {
			int id = generateId();
			getFetcher()->setProgramData(program, programValues[program], id);
			outstandingSetRequests.insert(program, id);
		}
		setErrors.clear();

		foreach(enum PMResetType command, resetErrors) {
			int id = generateId();
			getFetcher()->resetPM(command, id);
			outstandingResetRequests.insert(command, id);
		}
		resetErrors.clear();

		loggedFetchNext();

		emit statusChanged(getStatus());
	} else {
		errorPending = ErrorAccepted;

		outstandingRequests.clear();
		outstandingFetchRequests.clear();
		outstandingSetRequests.clear();
		outstandingResetRequests.clear();
		outstandingLoggedRequest.second = -1;
		loggedRequested.clear();
		loggedData = QSharedPointer<LoggedData>();
		downloadingLogged = false;
		fetchErrors.clear();
		setErrors.clear();
		resetErrors.clear();
		emit statusChanged(getStatus());
		emit errorCanceled();
	}
}

SiteManager::Status SiteManager::getStatus() {
	if(errorPending != ErrorOK)
		return SiteError;

	if(downloadingLogged) // This takes precedence over "updating", since updates will be delayed
		return SiteDownloadingLogged;

	if(updating)
		return SiteUpdating;

	return SiteStopped;
}

bool SiteManager::getResetCommand(enum PMDisplayNumber display, enum PMResetType & command) {
	switch(display) {
		case PM_D13:
			command = PM_RESET_AHR1;
			break;
		case PM_D14:
			command = PM_RESET_AHR2;
			break;
		case PM_D15:
			command = PM_RESET_AHR3;
			break;
		case PM_D16:
			command = PM_RESET_CAH1;
			break;
		case PM_D17:
			command = PM_RESET_CAH2;
			break;
		case PM_D20:
			command = PM_RESET_WHR1;
			break;
		case PM_D21:
			command = PM_RESET_WHR2;
			break;
		case PM_D24:
			command = PM_RESET_BAT1_DSC;
			break;
		case PM_D25:
			command = PM_RESET_BAT2_DSC;
			break;
		case PM_D26:
			command = PM_RESET_BAT1_DSE;
			break;
		case PM_D27:
			command = PM_RESET_BAT2_DSE;
			break;
		default:
			return false;
	}
	return true;
}

bool SiteManager::resetDisplay(enum PMDisplayNumber display) {
	enum PMResetType command;
	if(!getResetCommand(display, command))
		return false;

	return resetDirect(command);
}

bool SiteManager::resetDirect(enum PMResetType command) {
	if(forceDisconnect)
		return false;

	if(!outstandingResetRequests.contains(command)) {
		int id = generateId();
		getFetcher()->resetPM(command, id);
		qDebug() << "Resetting " << command;
		outstandingResetRequests.insert(command, id);
	}
	return true;	
}

void SiteManager::resetStatus(enum PMResetType command, bool success, int id) {
	if(outstandingResetRequests.contains(command, id)) {
		outstandingResetRequests.remove(command, id);
		if(!success) {
			qWarning() << "Entered reset error";
			resetErrors.insert(command);
			bool newError = errorPending != ErrorHappened;
			errorPending = ErrorHappened;
			// We have an error
			stopUpdatingError();
			if(newError)
				emit communicationsError();
		} else {
			qDebug() << "Reset successful";
			if(command == PM_RESET_PROGRAM) { // If all program data was reset, clear the versions stored here
				programValues.clear();
				emit programDataReset();
				updateProgramData();
			}

			emit resetComplete(command);
		}
	}
}

// Returns false if logged data is already being downloaded
bool SiteManager::downloadLogged(bool periodic, bool profile, bool efficiency) {
	if(forceDisconnect || downloadingLogged)
		return false;

	loggedData = QSharedPointer<LoggedData>(new LoggedData(settings->getId()));

	if(periodic) {
		loggedRequested.append(LoggedValue::TYPE_PERIODIC);
	}
	if(profile) {
		loggedRequested.append(LoggedValue::TYPE_PROFILE);
	}
	if(efficiency) {
		loggedRequested.append(LoggedValue::TYPE_EFFICIENCY);
	}

	downloadingLogged = true;
	emit statusChanged(getStatus());

	loggedFetchNext();
	return true;
}

// Gives a partial result (doesn't immediately cancel)
void SiteManager::cancelLoggedDownload() {
	loggedRequested.clear();
	loggedRequested.append(outstandingLoggedRequest.first);
}

/* Returns true if finished */
bool SiteManager::loggedFetchNext() {
	if(forceDisconnect)
		return false;

	if(loggedRequested.empty())
		return true;

	LoggedValue::LoggedDataType type = loggedRequested.first();

	int id = generateId();
	getFetcher()->fetchLoggedData(type, id);
	outstandingLoggedRequest = QPair<LoggedValue::LoggedDataType, int>(type, id);
	qDebug() << "Fetching logged data " << type;
	return false;
}

void SiteManager::handleLoggedData(QSharedPointer<LoggedValue> value, QSharedPointer<LoggedValue> value2, int id) {
	qDebug() << "Got logged data";
	if(errorPending == ErrorCleared) {
		errorPending = ErrorOK;
		emit statusChanged(getStatus());
	}

	LoggedValue::LoggedDataType type = value->type();
	if(outstandingLoggedRequest != QPair<LoggedValue::LoggedDataType, int>(type, id))
		return;
	outstandingLoggedRequest.second = -1;
	loggedRequested.removeAll(type);

	loggedData->setLoggedValue(value);
	loggedData->setLoggedValue(value2);

	if(!loggedFetchNext()) {
		return;
	}

	updateProgramData();
	fetchProgram(PM_P38); // Time
}

QSharedPointer<LoggedData> SiteManager::retrieveLogged() {
	if(downloadingLogged)
		return QSharedPointer<LoggedData>();

	return loggedData;
}

void SiteManager::handleLoggedProgress(LoggedValue::LoggedDataType type, int id, int progress, int outof) {
	if(outstandingLoggedRequest != QPair<LoggedValue::LoggedDataType, int>(type, id))
		return;

	emit loggedDataProgress(type, progress, outof);
}

void SiteManager::loggedError(LoggedValue::LoggedDataType type, int id) {
	qWarning() << "Entered logged error";
	if(outstandingLoggedRequest == QPair<LoggedValue::LoggedDataType, int>(type, id)) {
		loggedRequested.append(type);
		outstandingLoggedRequest.second = -1;
		bool newError = errorPending != ErrorHappened;
		errorPending = ErrorHappened;
		// We have an error
		stopUpdatingError();
		if(newError)
			emit communicationsError();
	}
}

int SiteManager::generateId() {
	if(nextId > 1000000) {
		nextId = 0;
	}

	return nextId++;
}
