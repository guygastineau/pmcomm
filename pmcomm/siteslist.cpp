#include "siteslist.h"

#include "appsettings.h"
#include "sitemanager.h"

#include <QDebug>

SitesList::SitesList(QObject *parent) : QAbstractListModel(parent) {
    AppSettings *settings = AppSettings::getInstance();
    sites = settings->getSites();

    QList<SiteManager *> managersToAdd;
    QSharedPointer<SiteSettings> site;
    foreach(site, sites->values()) {
        int id = site->getId();
        SiteManager *manager = new SiteManager(site, this);
        managersToAdd.append(manager);
        managers[id] = manager;

        alphabetized.append(qMakePair(site->getName(), id));
    }
    qSort(alphabetized);
}

QMap<int, QSharedPointer<SiteSettings> > & SitesList::getSites() {
    return *sites;
}

const QMap<int, SiteManager *> & SitesList::getManagers() {
    return managers;
}

void SitesList::addSites(const QList<QSharedPointer<SiteSettings> > & settingsList) {
    QList<SiteManager *> managersToAdd;

    QSharedPointer<SiteSettings> settings;
    foreach(settings, settingsList) {
        int id = settings->getId();
        if(id < 0) {
            if(!sites->empty()) {
                id = sites->keys()[sites->size() - 1] + 1; // QMap is guaranteed to be sorted by key
            } else {
                id = 0;
            }
            settings->setId(id);
        }

        (*sites)[id] = settings;
        SiteManager *manager = new SiteManager(settings, this);
        managers[settings->getId()] = manager;
        managersToAdd.append(manager);

        QPair<QString, int> toInsertAlph(settings->getName(), id);
        int i;
        for(i = 0; i < alphabetized.size(); i++) {
            if(toInsertAlph < alphabetized[i])
                break;
        }
        if(!alphabetized.empty()) {
            beginInsertRows(QModelIndex(), i, i);
            alphabetized.insert(i, toInsertAlph);
            endInsertRows();
        } else {
            alphabetized.append(toInsertAlph);
            emit dataChanged(createIndex(0, 0), createIndex(0, 0));
            getCurrentSite(); // Makes the first index valid if needed
        }
    }
    emit managersAdded(managersToAdd);
}

void SitesList::removeSites(const QList<int> & idList) {
    QList<SiteManager *> managersToRemove;

    foreach(int id, idList) {
        managersToRemove.append(managers[id]);
        managers.remove(id);
        sites->remove(id);

        int row = siteRowById(id);
        if(alphabetized.size() > 1) {
            beginRemoveRows(QModelIndex(), row, row);
            alphabetized.removeAt(row);
            endRemoveRows();

            if(id == getCurrentSite()) // Make sure the current site exists
                setCurrentSite(0);
        } else {
            alphabetized.clear();
            setCurrentSite(-1);
            emit dataChanged(createIndex(0, 0), createIndex(0, 0));
            emit currentRowChanged(0);
        }
    }
    emit managersRemoved(managersToRemove);

    foreach(SiteManager *manager, managersToRemove) {
        delete manager; // As far as I can tell, this is always safe
    }
}

void SitesList::updateSite(QSharedPointer<SiteSettings> site) {
    int id = site->getId();
    *(*sites)[id] = *site; // Important to copy the data, not the pointer

    int newOffset = 0; // For moving to higher index, need to subtract 1
    int oldPos = -1;
    int newPos = -1;
    QPair<QString, int> toInsertAlph(site->getName(), id);
    for(int i = 0; i < alphabetized.size(); i++) {
        if(newPos < 0 && (toInsertAlph < alphabetized[i])) {
            newPos = i;
            if(oldPos >= 0)
                newOffset = -1;
        }
        if(alphabetized[i].second == id)
            oldPos = i;
    }
    if(newPos < 0) { // Make sure it gets put somewhere
        newPos = alphabetized.size();
        newOffset = -1;
    }

    bool needToMove = (oldPos != newPos) && (oldPos + 1 != newPos);
    if(needToMove)
        beginMoveRows(QModelIndex(), oldPos, oldPos, QModelIndex(), newPos);

    alphabetized.removeAt(oldPos);
    alphabetized.insert(newPos + newOffset, toInsertAlph);

    if(needToMove)
        endMoveRows();
}

void SitesList::setSites(QList<QSharedPointer<SiteSettings> > & settingsList) {
    AppSettings::getInstance()->saveSites();

    QList<QSharedPointer<SiteSettings> > toAdd;
    QSet<int> toRemove = sites->keys().toSet();

    QSharedPointer<SiteSettings> site;
    foreach(site, settingsList) {
        int id = site->getId();
        if(!sites->contains(id)) {
            toAdd.append(site);
        } else {
            updateSite(site);
            toRemove.remove(id);
        }
    }
   
    addSites(toAdd);
    removeSites(toRemove.toList());
    AppSettings::getInstance()->saveSites();

    emit dataChanged(createIndex(0, 0), createIndex(alphabetized.size() - 1, 0));
}

int SitesList::getCurrentSite() {
    AppSettings *settings = AppSettings::getInstance();
    int current = settings->getCurrentSite();

    if(alphabetized.size() == 0) {
        if(current != -1)
            setCurrentSite(-1);
        return -1;
    }

    if(current < 0 || !sites->contains(current)) {
        int newCurrent = alphabetized[0].second;
        setCurrentSite(newCurrent);
        return newCurrent;
    }

    return current;
}

void SitesList::setCurrentSite(int id) {
    AppSettings *settings = AppSettings::getInstance();
    int prev = settings->getCurrentSite();
    settings->setCurrentSite(id);
    if(prev != id) {
        emit currentSiteUpdated();
        emit currentRowChanged(siteRowById(id));
    }
}

void SitesList::setCurrentSiteByRow(int row) {
    if(row < 0 || row >= alphabetized.size())
        return;

    setCurrentSite(alphabetized[row].second);
}

SiteManager *SitesList::getCurrentManager() {
    int curr = getCurrentSite();
    if(curr < 0)
        return NULL;
    return managers[curr];
}

int SitesList::siteRowById(int id) {
   for(int i = 0; i < alphabetized.size(); i++) {
        if(alphabetized[i].second == id) {
            return i;
        }
    }
    return -1;
}

int SitesList::rowCount(const QModelIndex & parent) const {
    Q_UNUSED(parent);

    if(alphabetized.size() == 0)
        return 1;

    return alphabetized.size();
}

QVariant SitesList::data(const QModelIndex & index, int role) const {
    if(!index.isValid())
        return QVariant();

    if(alphabetized.size() == 0 && index.row() == 0 && role == Qt::DisplayRole) {
        return "(No sites)";
    }

    if(index.row() >= alphabetized.size() || index.row() < 0)
        return QVariant();

    if(role == Qt::DisplayRole) {
        return alphabetized[index.row()].first;
    }
    return QVariant();
}
