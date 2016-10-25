#include "displaypicker.h"

#include "sitemanager.h"

#include <QGridLayout>
#include <QListWidgetItem>
#include <QTreeWidgetItem>
#include <QTreeWidgetItemIterator>
#include <QAbstractItemView>
#include <QLabel>
#include <QStringList>
#include <QListWidget>

// Gets the list of categories that should be displayed
void DisplayPicker::addDefaultDisplays() {
	QList<QPair<QString, QList<enum PMDisplayNumber> > > categories;

	QList<enum PMDisplayNumber> battery1Items;
	battery1Items << PM_D1 << PM_D3 << PM_D7 << PM_D10 << PM_D13 << PM_D16 << PM_D18 << PM_D20 << PM_D22 << PM_D24 << PM_D26;
	categories.append(QPair<QString, QList<enum PMDisplayNumber> >("Group 1", battery1Items));

	QList<enum PMDisplayNumber> battery2Items;
	battery2Items << PM_D2 << PM_D4 << PM_D8 << PM_D11 << PM_D14 << PM_D17 << PM_D19 << PM_D21 << PM_D23 << PM_D25 << PM_D27;
	categories.append(QPair<QString, QList<enum PMDisplayNumber> >("Group 2", battery2Items));

	QList<enum PMDisplayNumber> battery3Items;
	battery3Items << PM_D9 << PM_D12 << PM_D15;
	categories.append(QPair<QString, QList<enum PMDisplayNumber> >("Group 3", battery3Items));

	QList<enum PMDisplayNumber> otherItems;
	otherItems << PM_D28 << PM_DALARM;
	categories.append(QPair<QString, QList<enum PMDisplayNumber> >("Other items", otherItems));

	addPossibleDisplays(categories);
}

// Gets the categories for efficiency data
void DisplayPicker::addEfficiencyDisplays() {
	QList<QPair<QString, QList<enum PMDisplayNumber> > > categories;

	QList<enum PMDisplayNumber> efficiency1Items;
	efficiency1Items << PM_D29 << PM_D30 << PM_D31 << PM_D32 << PM_D33 << PM_D34;
	categories.append(QPair<QString, QList<enum PMDisplayNumber> >("Battery 1 efficiency", efficiency1Items));

	QList<enum PMDisplayNumber> efficiency2Items;
	efficiency2Items << PM_D35 << PM_D36 << PM_D37 << PM_D38 << PM_D39 << PM_D40;
	categories.append(QPair<QString, QList<enum PMDisplayNumber> >("Battery 2 efficiency", efficiency2Items));

	addPossibleDisplays(categories);
}

// NULL manager uses default labels
DisplayPicker::DisplayPicker(SiteManager *manager, QWidget *parent) : QWidget(parent) {
	this->manager = manager;

	QGridLayout *layout = new QGridLayout(this);

	QLabel *leftInstructions = new QLabel("Check the boxes of the displays you want to view.");
	leftInstructions->setWordWrap(true);
	layout->addWidget(leftInstructions, 0, 0, 1, 1);

	QLabel *rightInstructions = new QLabel("Drag the displays up and down to change the order they are displayed in.");
	rightInstructions->setWordWrap(true);
	rightInstructions->setMaximumWidth(200);
	layout->addWidget(rightInstructions, 0, 1, 1, 1);

	displaysTree = new QTreeWidget();
	displaysTree->setHeaderLabel("Displays");
	connect(displaysTree, SIGNAL(itemChanged(QTreeWidgetItem *, int)), this, SLOT(treeItemChanged(QTreeWidgetItem *)));

	selectedList = new QListWidget();
	selectedList->setSelectionMode(QAbstractItemView::SingleSelection);
	selectedList->setDragDropMode(QAbstractItemView::InternalMove);
	selectedList->setDragEnabled(true);
	selectedList->setDropIndicatorShown(true);
	selectedList->setMaximumWidth(200);

	layout->addWidget(displaysTree, 1, 0, 1, 1);
	layout->addWidget(selectedList, 1, 1, 1, 1);

	if(manager != NULL)
		connect(manager, SIGNAL(labelsUpdated()), this, SLOT(updateLabels()));
}

// Fills in the tree of possible displays
void DisplayPicker::addPossibleDisplays(QList<QPair<QString, QList<enum PMDisplayNumber> > > & categories) {
	for(int i = 0; i < categories.size(); i++) {
		QPair<QString, QList<enum PMDisplayNumber> > category = categories[i];
		QTreeWidgetItem *topLevelItem = new QTreeWidgetItem(displaysTree, QStringList(category.first));
		for(int j = 0; j < category.second.size(); j++) {
			enum PMDisplayNumber display = category.second[j];
			QStringList label(SiteManager::getLabelStatic(display, manager));
			QTreeWidgetItem *treeItem = new QTreeWidgetItem(label);
			treeItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
			treeItem->setData(0, Qt::UserRole, QVariant((int) display));
			topLevelItem->addChild(treeItem);
		}
		displaysTree->expandItem(topLevelItem);
	}
}

// Add or remove items when they are checked or unchecked
void DisplayPicker::treeItemChanged(QTreeWidgetItem *item) {
	if(!item->parent()) // skip top-level items (categories)
		return;

	enum PMDisplayNumber display = (enum PMDisplayNumber) item->data(0, Qt::UserRole).toInt();

	for(int i = 0; i < selectedList->count(); i++) {
		enum PMDisplayNumber listDisplay = (enum PMDisplayNumber) selectedList->item(i)->data(Qt::UserRole).toInt();
		if(listDisplay == display) {
			if(item->checkState(0) != Qt::Checked) {
				delete selectedList->item(i);
			}
			return;
		}
	}

	// If here, not found
	if(item->checkState(0) == Qt::Checked) {
		createListItem(display);
	}
}

void DisplayPicker::createListItem(enum PMDisplayNumber display) {
	QListWidgetItem *listItem = new QListWidgetItem(SiteManager::getLabelStatic(display, manager), selectedList);	
	listItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled);
	listItem->setData(Qt::UserRole, QVariant((int) display));
}

void DisplayPicker::setSelectedItems(const QList<enum PMDisplayNumber> & items) {
	selectedList->clear();

	for(int i = 0; i < items.size(); i++) {
		enum PMDisplayNumber display = items[i];
		createListItem(display);
	}

	// Check/uncheck the boxes as needed
	QSet<enum PMDisplayNumber> selectedSet = items.toSet();
	QTreeWidgetItemIterator it(displaysTree);

	while(*it) {
		if(!(*it)->parent()) { // skip top-level items (categories)
			++it;
			continue;
		}

		enum PMDisplayNumber display = (enum PMDisplayNumber) (*it)->data(0, Qt::UserRole).toInt();
		(*it)->setCheckState(0, selectedSet.contains(display) ? Qt::Checked : Qt::Unchecked);
		++it;
	}
}

void DisplayPicker::selectedItems(QList<enum PMDisplayNumber> & items) {
	items.clear();

	for(int i = 0; i < selectedList->count(); i++) {
		enum PMDisplayNumber display = (enum PMDisplayNumber) selectedList->item(i)->data(Qt::UserRole).toInt();
		items.append(display);
	}
}

void DisplayPicker::updateLabels() {
	for(int i = 0; i < selectedList->count(); i++) {
		enum PMDisplayNumber display = (enum PMDisplayNumber) selectedList->item(i)->data(Qt::UserRole).toInt();
		selectedList->item(i)->setText(SiteManager::getLabelStatic(display, manager));
	}

	QTreeWidgetItemIterator it(displaysTree);

	while(*it) {
		if(!(*it)->parent()) { // skip top-level items (categories)
			++it;
			continue;
		}

		enum PMDisplayNumber display = (enum PMDisplayNumber) (*it)->data(0, Qt::UserRole).toInt();
		(*it)->setText(0, SiteManager::getLabelStatic(display, manager));
		++it;
	}	
}

