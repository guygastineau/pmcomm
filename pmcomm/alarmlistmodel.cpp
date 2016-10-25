#include "alarmlistmodel.h"

#include <QBrush>

AlarmListModel::AlarmListModel(QObject *parent) : QAbstractListModel(parent) {
	alarmStatusValid = false;
	statusDidChange = false;
	manager = NULL;
}

int AlarmListModel::rowCount(const QModelIndex & parent) const {
	Q_UNUSED(parent);

	return alarmsEnabled[0].size() + alarmsEnabled[1].size() + 1; // 1 more for relay
}

QVariant AlarmListModel::data(const QModelIndex &index, int role) const {
	if(!index.isValid())
		return QVariant();

	if(index.column() != 0)
		return QVariant();

	int numRows = rowCount();
	if(index.row() < 0 || index.row() >= numRows)
		return QVariant();

	if(role == Qt::ForegroundRole) { // Display in gray when not up to date
		if(!manager || !manager->displayUpToDate(PM_DALARM))
			return QBrush(Qt::gray);
	}

	// if(role != Qt::DisplayRole)
	// 	return QVariant();

	int row = index.row();
	if(!manager) {
		return QVariant("(Select a site to view its active alarms)");
	}

	if(row == rowCount() - 1) { // Check if relay
		int status = getStatusFromMask(PM_ALARMVAL_RELAY_ON);
		if(role == Qt::ForegroundRole && status == 1) {
			return QBrush(Qt::darkGreen);
		}

		if(role != Qt::DisplayRole)
			return QVariant();

		QString statusLabel;
		if(status < 0)
			statusLabel = QString::fromUtf8("—");
		else if(status)
			statusLabel = "ON";
		else
			statusLabel = "Off";
		return QString("Relay status: %1").arg(statusLabel);
	}

	int battery = 1; // Determine the battery
	if(row >= alarmsEnabled[0].size()) {
		battery = 2;
		row -= alarmsEnabled[0].size();
	}

	int status = getStatusFromMask(getMask(alarmsEnabled[battery - 1][row], battery));
	if(role == Qt::ForegroundRole && status == 1) {
		return QBrush(Qt::red);
	}

	if(role != Qt::DisplayRole)
		return QVariant();

	QString alarmLabel = getAlarmLabel(alarmsEnabled[battery - 1][row], battery);
	QString statusLabel;
	if(status < 0)
		statusLabel = QString::fromUtf8("—");
	else if(status)
		statusLabel = "ACTIVE";
	else
		statusLabel = "Inactive";
	return QString("%1: %2").arg(alarmLabel).arg(statusLabel);
}

// 0 is off, 1 is on, -1 is invalid
int AlarmListModel::getStatusFromMask(int mask) const {
	if(!alarmStatusValid)
		return -1;

	return (alarmStatus & mask) ? 1 : 0;
}

int AlarmListModel::getMask(enum PMAlarmIndex alarm, int battery) {
	if(battery == 1) {
		switch(alarm) {
			case PM_ALARM_LOBAT: return PM_ALARMVAL_BAT1_LOW;
			case PM_ALARM_CHARGED: return PM_ALARMVAL_BAT1_CHARGED;
			case PM_ALARM_HIBAT: return PM_ALARMVAL_BAT1_HIGH;
			case PM_ALARM_NEEDCHARGE: return PM_ALARMVAL_BAT1_NEEDCHARGE;
			case PM_ALARM_NEEDEQUALIZE: return PM_ALARMVAL_BAT1_NEEDEQUALIZE;
			default: return 0;
		}
	} else {
		switch(alarm) {
			case PM_ALARM_LOBAT: return PM_ALARMVAL_BAT2_LOW;
			case PM_ALARM_CHARGED: return PM_ALARMVAL_BAT2_CHARGED;
			case PM_ALARM_HIBAT: return PM_ALARMVAL_BAT2_HIGH;
			case PM_ALARM_NEEDCHARGE: return PM_ALARMVAL_BAT2_NEEDCHARGE;
			case PM_ALARM_NEEDEQUALIZE: return PM_ALARMVAL_BAT2_NEEDEQUALIZE;
			default: return 0;
		}		
	}
}

QString AlarmListModel::getAlarmLabel(enum PMAlarmIndex alarm, int battery) {
	QString batteryString = QString("Battery %1").arg(battery);
	switch(alarm) {
		case PM_ALARM_LOBAT: return QString("%1 Low Voltage").arg(batteryString);
		case PM_ALARM_CHARGED: return QString("%1 Charged").arg(batteryString);
		case PM_ALARM_HIBAT: return QString("%1 High Voltage").arg(batteryString);
		case PM_ALARM_NEEDCHARGE: return QString("Time to Charge %1").arg(batteryString);
		case PM_ALARM_NEEDEQUALIZE: return QString("Time to Equalize %1").arg(batteryString);
		default: return "INVALID";
	}
}

// Set to NULL for no site
void AlarmListModel::setManager(SiteManager *newManager) {
	beginResetModel();
	if(manager != NULL) {
		disconnect(manager, 0, this, 0);
	}

	manager = newManager;
	alarmStatusValid = false;
	alarmsEnabled[0].clear();
	alarmsEnabled[1].clear();

	if(manager != NULL) {
		connect(manager, SIGNAL(displayUpdated(PMDisplayNumber)), this, SLOT(displayUpdated(PMDisplayNumber)));
		connect(manager, SIGNAL(programUpdated(PMProgramNumber)), this, SLOT(programUpdated(PMProgramNumber)));
		connect(manager, SIGNAL(statusChanged(SiteManager::Status)), this, SLOT(statusChanged(SiteManager::Status)));
	}

	endResetModel();

	displayUpdated(PM_DALARM);
	programUpdated(PM_P22_23);
	programUpdated(PM_P24_25);

	statusDidChange = true;
}

void AlarmListModel::statusChanged(SiteManager::Status status) {
	Q_UNUSED(status);

	statusDidChange = true;
	emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, 0));
}

void AlarmListModel::displayUpdated(PMDisplayNumber display) {
	if(display != PM_DALARM)
		return;

	if(!manager)
		return;

	DisplayValue val = manager->getDisplay(PM_DALARM);
	if(!val.valid())
		return;

	int rawValue = val.getRawIntValue();
	if(!alarmStatusValid || statusDidChange || alarmStatus != rawValue) {
		alarmStatus = rawValue;
		alarmStatusValid = true;
		statusDidChange = false;
		emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, 0));
	}
}

void AlarmListModel::programUpdated(PMProgramNumber program) {
	if(program != PM_P22_23 && program != PM_P24_25)
		return;

	if(!manager)
		return;

	AlarmLevelProgramValue *prog = static_cast<AlarmLevelProgramValue *>(manager->getProgram(program).data());
	if(prog == NULL)
		return;

	int battery = 0;
	if(program == PM_P24_25)
		battery = 1;

	QList<enum PMAlarmIndex> newAlarmsEnabled;
	for(int i = 0; i <= PM_ALARM_NEEDEQUALIZE; i++) {
		enum PMAlarmLevel level = prog->getLevelForAlarm((enum PMAlarmIndex) i);
		if(level == PM_ALARMLEVEL_VISUAL || level == PM_ALARMLEVEL_VISUAL_AUDIBLE) {
			newAlarmsEnabled.append((enum PMAlarmIndex) i);
		}
	}

	if(alarmsEnabled[battery] != newAlarmsEnabled) {
		beginResetModel();
		alarmsEnabled[battery] = newAlarmsEnabled;
		endResetModel();
	}
}


