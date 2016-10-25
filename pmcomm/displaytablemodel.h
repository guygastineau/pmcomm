#ifndef DISPLAYTABLEMODEL_H
#define DISPLAYTABLEMODEL_H

#include "pmdefs.h"
#include "sitemanager.h"

#include <QAbstractTableModel>
#include <QVariant>
#include <QModelIndex>
#include <QList>
#include <QObject>

class SiteManager;
class SitesList;

class DisplayTableModel : public QAbstractTableModel {
	Q_OBJECT
	Q_PROPERTY(bool updating READ isUpdating WRITE setUpdating);

public:
	DisplayTableModel(QObject *parent = NULL);

	SiteManager *siteAt(int column);
	void removeSiteAt(int column);
	void setSites(SitesList *allSites, const QList<int> & selectedSites);

	enum PMDisplayNumber displayAt(int row);
	void addDisplayAt(enum PMDisplayNumber display, int row);
	void removeDisplayAt(int row);
	void setDisplays(const QList<enum PMDisplayNumber> & display);

	bool isUpdating();
	void setUpdating(bool on);

	int rowCount(const QModelIndex & parent = QModelIndex()) const;
	int columnCount(const QModelIndex & parent = QModelIndex()) const;
	QVariant data(const QModelIndex &index, int role) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

public slots:
	void displayUpdated(PMDisplayNumber display);
	void programUpdated(PMProgramNumber program);
	void tempUnitChanged();
	void columnUpdated();
	void labelsUpdated();

	void statusChanged(SiteManager::Status status);

signals:
	void siteStatusChanged(SiteManager *site);

private:
	QList<SiteManager *> sites;
	QList<enum PMDisplayNumber> displays;

	int alarmMasks[2];

	bool updating;
};

#endif
