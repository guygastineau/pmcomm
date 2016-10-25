#include "sitepicker.h"

#include "siteslist.h"

#include <QVBoxLayout>
#include <QListWidget>
#include <QListWidgetItem>
#include <QAbstractItemView>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QRadioButton>

SitePicker::SitePicker(QWidget *parent) : QDialog(parent) {
	setWindowTitle("Select sites");
	setWindowModality(Qt::WindowModal);

	QVBoxLayout *layout = new QVBoxLayout(this);

    QGroupBox *siteSelectionGroup = new QGroupBox("Site Selection");
    currentRadio = new QRadioButton("Currently selected site");
    currentRadio->setChecked(true);
    listRadio = new QRadioButton("Select from a list");

    QVBoxLayout *siteSelectionLayout = new QVBoxLayout(siteSelectionGroup);
    siteSelectionLayout->addWidget(currentRadio);
    siteSelectionLayout->addWidget(listRadio);
    layout->addWidget(siteSelectionGroup);

	list = new QListWidget;
	list->setSelectionMode(QAbstractItemView::SingleSelection);
	list->setDragDropMode(QAbstractItemView::InternalMove);
	list->setDragEnabled(true);
	list->setDropIndicatorShown(true);
	list->setEnabled(false);

    connect(listRadio, SIGNAL(toggled(bool)), list, SLOT(setEnabled(bool)));

	layout->addWidget(list);

	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
	connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

	layout->addWidget(buttonBox);
}

void SitePicker::setData(SitesList *sitesList, QList<int> & selectedSites) {
	list->clear();
	QSet<int> alreadyPlaced;

	for(int i = 0; i < selectedSites.size(); i++) {
		int siteId = selectedSites[i];
		QListWidgetItem *listItem = new QListWidgetItem(sitesList->getSites()[siteId]->getName(), list);
		listItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
		listItem->setCheckState(Qt::Checked);
		listItem->setData(Qt::UserRole, QVariant(siteId));
		alreadyPlaced.insert(siteId);
	}

	foreach(int siteId, sitesList->getSites().keys()) {
		if(alreadyPlaced.contains(siteId))
			continue;
		QListWidgetItem *listItem = new QListWidgetItem(sitesList->getSites()[siteId]->getName(), list);
		listItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
		listItem->setCheckState(Qt::Unchecked);
		listItem->setData(Qt::UserRole, QVariant(siteId));
	}
}

void SitePicker::setShowSelected(bool flag) {
	if(flag) {
		currentRadio->setChecked(true);
	} else {
		listRadio->setChecked(true);
	}
}

void SitePicker::getSelected(QList<int> & selectedSites) {
	selectedSites.clear();

	for(int i = 0; i < list->count(); i++) {
		QListWidgetItem *item = list->item(i);
		if(item->checkState() == Qt::Checked) {
			selectedSites.append(item->data(Qt::UserRole).toInt());
		}
	}
}

bool SitePicker::isShowSelected() {
	return currentRadio->isChecked();
}