#include "displaytablemodel.h"

#include "alarmlistmodel.h"
#include "siteslist.h"
#include "appsettings.h"

#include <QBrush>

#include <QDebug>

DisplayTableModel::DisplayTableModel(QObject *parent) : QAbstractTableModel(parent) {
	updating = false;
	memset(alarmMasks, 0, sizeof(alarmMasks));

	connect(AppSettings::getInstance(), SIGNAL(tempUnitChanged()), this, SLOT(tempUnitChanged()));
}

bool DisplayTableModel::isUpdating() {
	return updating;
}

void DisplayTableModel::setUpdating(bool on) {
	updating = on;
	if(updating) {
		foreach(SiteManager *site, sites) {
			site->startUpdating();
		}
	} else {
		foreach(SiteManager *site, sites) {
			site->stopUpdating();
		}
	}
}

int DisplayTableModel::rowCount(const QModelIndex & parent) const {
	Q_UNUSED(parent);
	return displays.size() + 1;
}

int DisplayTableModel::columnCount(const QModelIndex &parent) const {
	Q_UNUSED(parent);
	return sites.size();
}

void DisplayTableModel::tempUnitChanged() {
	int tempIndex = displays.indexOf(PM_D28);
	if(tempIndex >= 0) {
		emit headerDataChanged(Qt::Vertical, tempIndex + 1, tempIndex + 1);
		if(!sites.empty()) {
			emit dataChanged(createIndex(tempIndex + 1, 0), createIndex(tempIndex + 1, sites.size() - 1));
		}
	}
}

QVariant DisplayTableModel::data(const QModelIndex &index, int role) const {
	if(!index.isValid())
		return QVariant();

	if(index.row() >= displays.size() + 1 || index.row() < 0)
		return QVariant();

	if(index.column() >= sites.size() || index.column() < 0)
		return QVariant();

	if(role == Qt::DisplayRole) {
		SiteManager *site = sites[index.column()];
		if(index.row() == 0) {
			SiteManager::Status status = site->getStatus();
			switch(status) {
				case SiteManager::SiteStopped:
					return "Stopped";
				case SiteManager::SiteDownloadingLogged:
					return "Downloading logged data. Please wait";
				case SiteManager::SiteUpdating:
					return "Updating";
				case SiteManager::SiteError:
					return "Error";
			}
		}

		enum PMDisplayNumber display = displays[index.row() - 1];
		DisplayValue val = site->getDisplay(display);
		if(!val.valid())
			return QString::fromUtf8("—");

		if(display == PM_DALARM) {
			if(val.getRawIntValue() & (alarmMasks[0] | alarmMasks[1]))
				return "ACTIVE";
			else
				return "Inactive";		
		} else
			return val.toString();

	} else if(role == Qt::ForegroundRole) {
		SiteManager *site = sites[index.column()];

		 if(index.row() == 0) {
			SiteManager::Status status = site->getStatus();
			if(status == SiteManager::SiteDownloadingLogged)
				return QBrush(Qt::red);
			else
				return QVariant();
		 }

		if(!site->displayUpToDate(displays[index.row() - 1]))
			return QBrush(Qt::gray);

		if(index.row() != 0) {
			enum PMDisplayNumber display = displays[index.row() - 1];
			DisplayValue val = site->getDisplay(display);
			if(display == PM_DALARM && val.valid()) {
				if(val.getRawIntValue() & (alarmMasks[0] | alarmMasks[1]))
					return QBrush(Qt::red);
			}
		}
	}
	return QVariant();
}

QVariant DisplayTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
	if(section < 0)
		return QVariant();

	if(role == Qt::DisplayRole && orientation == Qt::Vertical) {
		if(section >= displays.size() + 1)
			return QVariant();

		if(section == 0)
			return "Status";

		SiteManager *site = NULL;
		if(!sites.empty())
			site = sites[0];

		return site->getLabelStatic(displays[section - 1], site);
	}
	if(role == Qt::DisplayRole && orientation == Qt::Horizontal) {
		if(section >= sites.size())
			return QVariant();

		SiteManager *site = sites[section];

		return site->getSettings()->getName();
	}
	return QVariant();
}

void DisplayTableModel::displayUpdated(PMDisplayNumber display) {
	SiteManager *site = (SiteManager *) sender();
	if(site == NULL)
		return;

    int col = sites.indexOf(site);
    int row = displays.indexOf(display);
    if(col < 0 || row < 0) // Not selected anymore
    	return;

	QModelIndex changedIdx = createIndex(row + 1, col);
	emit dataChanged(changedIdx, changedIdx);
}

// Update the alarm mask
void DisplayTableModel::programUpdated(PMProgramNumber program) {
	SiteManager *site = (SiteManager *) sender();
	if(site == NULL)
		return;

	if(program != PM_P22_23 && program != PM_P24_25)
		return;

	AlarmLevelProgramValue *prog = static_cast<AlarmLevelProgramValue *>(site->getProgram(program).data());
	if(prog == NULL)
		return;

	int battery = 1;
	if(program == PM_P24_25) battery = 2;

	alarmMasks[battery - 1] = 0;
	for(int i = 0; i <= PM_ALARM_NEEDEQUALIZE; i++) { // Skips relay
		enum PMAlarmLevel level = prog->getLevelForAlarm((enum PMAlarmIndex) i);
		if(level == PM_ALARMLEVEL_VISUAL || level == PM_ALARMLEVEL_VISUAL_AUDIBLE) {
			alarmMasks[battery - 1] |= AlarmListModel::getMask((enum PMAlarmIndex) i, battery);
		}
	}

    int col = sites.indexOf(site);
    int row = displays.indexOf(PM_DALARM);
    if(col < 0 || row < 0) // Not selected
    	return;

	QModelIndex changedIdx = createIndex(row + 1, col);
	emit dataChanged(changedIdx, changedIdx);
}

void DisplayTableModel::columnUpdated() {
	SiteManager *site = (SiteManager *) sender();
	if(site == NULL)
		return;

	int col = sites.indexOf(site);

	emit dataChanged(createIndex(1, col), createIndex(displays.size(), col));	
}

void DisplayTableModel::labelsUpdated() {
	SiteManager *site = (SiteManager *) sender();
	if(site == NULL)
		return;

    int col = sites.indexOf(site);

    if(col == 0)
    	emit headerDataChanged(Qt::Vertical, 1, displays.size());
}

void DisplayTableModel::statusChanged(SiteManager::Status status) {
	Q_UNUSED(status);

	SiteManager *site = (SiteManager *) sender();
	if(site == NULL)
		return;

    int col = sites.indexOf(site);

	QModelIndex changedIdx = createIndex(0, col);
	emit dataChanged(changedIdx, changedIdx);
	emit siteStatusChanged(site);
}

SiteManager *DisplayTableModel::siteAt(int column) {
	return sites[column];
}

void DisplayTableModel::removeSiteAt(int column) {
	beginRemoveColumns(QModelIndex(), column, column);

	SiteManager *manager = sites[column];
	manager->stopUpdating();

	disconnect(manager, 0, this, 0);

	sites.removeAt(column);

	endRemoveColumns();

	if(column == 0)
    	emit headerDataChanged(Qt::Vertical, 1, displays.size());
}

void DisplayTableModel::setSites(SitesList *allSites, const QList<int> & selectedSites) {
	beginResetModel();

	for(int i = 0; i < sites.size(); i++) {
		SiteManager *manager = sites[i];
		manager->stopUpdating();
		disconnect(manager, 0, this, 0);
	}

	sites.clear();
	const QMap<int, SiteManager *> & allManagers = allSites->getManagers();

	foreach(int site, selectedSites) {
		SiteManager *manager = allManagers[site];
		sites.append(manager);
		foreach(enum PMDisplayNumber display, displays) {
			manager->addDisplay(display);
		}

		connect(manager, SIGNAL(displayUpdated(PMDisplayNumber)), this, SLOT(displayUpdated(PMDisplayNumber)));
		connect(manager, SIGNAL(programUpdated(PMProgramNumber)), this, SLOT(programUpdated(PMProgramNumber)));
		connect(manager, SIGNAL(allDisplaysUpdated()), this, SLOT(columnUpdated()));
		connect(manager, SIGNAL(labelsUpdated()), this, SLOT(labelsUpdated()));
		connect(manager, SIGNAL(statusChanged(SiteManager::Status)), this, SLOT(statusChanged(SiteManager::Status)));
	
		if(updating) {
			manager->startUpdating();
		}
	}

	endResetModel();
}

enum PMDisplayNumber DisplayTableModel::displayAt(int row) {
	return displays[row - 1];
}

void DisplayTableModel::addDisplayAt(enum PMDisplayNumber display, int row) {
	beginInsertRows(QModelIndex(), row, row);

	displays.insert(row, display);

	foreach(SiteManager *site, sites) {
		site->addDisplay(display);
	}

	endInsertRows();
}

void DisplayTableModel::removeDisplayAt(int row) {
	beginRemoveRows(QModelIndex(), row, row);

	enum PMDisplayNumber display = displays[row];

	displays.removeAt(row);

	foreach(SiteManager *site, sites) {
		site->removeDisplay(display);
	}

	endRemoveRows();
}

void DisplayTableModel::setDisplays(const QList<enum PMDisplayNumber> & newDisplays) {
	beginResetModel();

	foreach(SiteManager *site, sites) {
		for(int i = 0; i < displays.size(); i++) {
			site->removeDisplay(displays[i]);
		}
		for(int i = 0; i < newDisplays.size(); i++) {
			site->addDisplay(newDisplays[i]);
		}
	}

	displays = newDisplays;

	endResetModel();
}


