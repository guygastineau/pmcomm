#ifndef ALARMLISTMODEL_H
#define ALARMLISTMODEL_H

#include "pmdefs.h"
#include "sitemanager.h"

#include <QObject>
#include <QAbstractListModel>
#include <QVariant>
#include <QList>

class AlarmListModel : public QAbstractListModel {
	Q_OBJECT

public:
	AlarmListModel(QObject *parent = NULL);
	void setManager(SiteManager *newManager);

	int rowCount(const QModelIndex & parent = QModelIndex()) const;
	QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;

	static int getMask(enum PMAlarmIndex alarm, int battery);
	static QString getAlarmLabel(enum PMAlarmIndex alarm, int battery);

public slots:
	void displayUpdated(PMDisplayNumber display);
	void programUpdated(PMProgramNumber program);
	void statusChanged(SiteManager::Status status);

private:
	int getStatusFromMask(int mask) const;

	SiteManager *manager;
	int alarmStatus;
	bool alarmStatusValid; // Indicates if rawAlarmStatus is valid
	bool statusDidChange; // Set to true when a status change requires updating the view

	QList<enum PMAlarmIndex> alarmsEnabled[2]; // Alarms for each battery
};



#endif
