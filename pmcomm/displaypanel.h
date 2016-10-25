#ifndef DISPLAYPANEL_H
#define DISPLAYPANEL_H

#include "pmdefs.h"

#include <QGroupBox>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QModelIndex>
#include <QSet>
#include <QPair>

class SiteManager;
class DisplayTableModel;
class DisplayPickerDialog;
class SitePicker;
class AlarmListModel;
class SitesList;

class QPushButton;
class QTableView;

class DisplayPanel : public QGroupBox {
	Q_OBJECT

public:
	DisplayPanel(SitesList *sitesList, QWidget *parent = NULL);

public slots:
	void selectDisplaysClicked();
	void selectSitesClicked();
	void resetButtonClicked();

	void selectionChanged();
	void siteStatusChanged(SiteManager *site);

	void currentSiteUpdated();
	void managersAdded(const QList<SiteManager *> & managers);
	void managersRemoved(const QList<SiteManager *> & managers);

	void displaysChanged(const QModelIndex & topLeft, const QModelIndex & bottomRight);

	void forceStop();

signals:
	void switchStateStopped();

private:
	SitesList *sitesList;

	DisplayTableModel *model;
    QTableView *table;
    AlarmListModel *alarmsModel;
	QPushButton *resetButton;

	QPointer<DisplayPickerDialog> displayPicker;
	QPointer<SitePicker> sitePicker;

    QSet<QPair<SiteManager *, enum PMDisplayNumber> > toReset;
	void adjustResetButton();
	bool isResettable(enum PMDisplayNumber display);

	void adjustAlarms();
};



#endif
