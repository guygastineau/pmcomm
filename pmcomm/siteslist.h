#ifndef SITESLIST_H
#define SITESLIST_H

#include "sitesettings.h"

#include <QObject>
#include <QAbstractListModel>
#include <QSharedPointer>

class SiteManager;

/* This class keeps track of the sites and managers that are
   configured and which site is currently the default.
   It also emits signals to indicate changes in this state.
 */
class SitesList : public QAbstractListModel {
	Q_OBJECT

public:
	// Constructs a SitesList. This initializes the
	// list from the AppSettings and constructs the
	// corresponding managers
	SitesList(QObject *parent = NULL);

	// This can also be used to modify the list of sites, as long as updateSites() is called afterward
	QMap<int, QSharedPointer<SiteSettings> > & getSites();

	// Gets the list of managers
	const QMap<int, SiteManager *> & getManagers();

	// Sets the list of sites. Handles adding and removing as needed
	void setSites(QList<QSharedPointer<SiteSettings> > & settingsList);

	// For use as a model in the model/view framework.
	// It will display sites alphabetically
	int rowCount(const QModelIndex & parent) const;
	QVariant data(const QModelIndex & index, int role) const;

	// Returns the current site id, or -1 if there isn't one
	int getCurrentSite();

	void setCurrentSite(int id);

	// Returns the index in the alphabetized list of the specified site
	int siteRowById(int id);

	SiteManager *getCurrentManager();

public slots:
	// Notifies this when the current site should change, as indexed by the alphabetized list
	void setCurrentSiteByRow(int row);

signals:
	// Used to notify other objects that a manager was created so that
	// any initialization (particuarly connecting signals) can be done
	void managersAdded(const QList<SiteManager *> & managers);

	void managersRemoved(const QList<SiteManager *> & managers);

	void currentSiteUpdated();

	// Emitted when the current site changes for use by views
	void currentRowChanged(int row);

private:

	// Adds the specified site. This site must not have been present previously
	void addSites(const QList<QSharedPointer<SiteSettings> > & settingsList);

	// Removes the site with the specified id.
	void removeSites(const QList<int> & idList);

	void updateSite(QSharedPointer<SiteSettings> site);

	QMap<int, SiteManager *> managers;
	QMap<int, QSharedPointer<SiteSettings> > *sites;

	QList<QPair<QString, int> > alphabetized;
};






#endif
