#ifndef DISPLAYPICKER_H
#define DISPLAYPICKER_H

#include "pmdefs.h"

#include <QObject>
#include <QTreeWidget>

class SiteManager;

class QListWidget;

class DisplayPicker : public QWidget {
	Q_OBJECT

public:
	DisplayPicker(SiteManager *manager, QWidget *parent = 0);

	void setSelectedItems(const QList<enum PMDisplayNumber> & items);
	void selectedItems(QList<enum PMDisplayNumber> & items);

	void addDefaultDisplays();
	void addEfficiencyDisplays();

public slots:
	void updateLabels();
	void treeItemChanged(QTreeWidgetItem *item);

private:
	void addPossibleDisplays(QList<QPair<QString, QList<enum PMDisplayNumber> > > & categories);
	void createListItem(enum PMDisplayNumber display);

	SiteManager *manager;
	QTreeWidget *displaysTree;
	QListWidget *selectedList;
};

#endif