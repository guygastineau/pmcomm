#include "displaypanel.h"

#include "displaytablemodel.h"
#include "sitesettings.h"
#include "sitemanager.h"
#include "displaytablemodel.h"
#include "displaypickerdialog.h"
#include "sitepicker.h"
#include "alarmlistmodel.h"
#include "siteslist.h"
#include "displaypicker.h"
#include "appsettings.h"

#include <QPushButton>
#include <QTableView>
#include <QStateMachine>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QItemSelection>
#include <QMessageBox>
#include <QListView>

#include <QDebug>

DisplayPanel::DisplayPanel(SitesList *sitesList, QWidget *parent) : QGroupBox("Live Data", parent) {
    AppSettings *settings = AppSettings::getInstance();
    this->sitesList = sitesList;

    const QList<enum PMDisplayNumber> & displayStates = settings->getSelectedDisplays();

    model = new DisplayTableModel(this);
    model->setDisplays(displayStates);

    if(settings->getShowSelected()) {
        currentSiteUpdated();
    } else {
        model->setSites(sitesList, settings->getSelectedSites());
    }

    table = new QTableView;
    table->setModel(model);

    connect(sitesList, SIGNAL(managersAdded(const QList<SiteManager *> &)), this, SLOT(managersAdded(const QList<SiteManager *> &)));
    connect(sitesList, SIGNAL(managersRemoved(const QList<SiteManager *> &)), this, SLOT(managersRemoved(const QList<SiteManager *> &)));
    connect(sitesList, SIGNAL(currentSiteUpdated()), this, SLOT(currentSiteUpdated()));

    // For reset button
    connect(table->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)), this, SLOT(selectionChanged()));
    connect(model, SIGNAL(modelReset()), this, SLOT(selectionChanged()));
    connect(model, SIGNAL(siteStatusChanged(SiteManager *)), this, SLOT(siteStatusChanged(SiteManager *)));
    connect(model, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)), this, SLOT(displaysChanged(const QModelIndex &, const QModelIndex &)));

    QPushButton *startStopButton = new QPushButton;

    QStateMachine *startStopMachine = new QStateMachine(this);
    QState *stoppedState = new QState();
    stoppedState->assignProperty(startStopButton, "text", "Start");
    stoppedState->assignProperty(model, "updating", false);
    QState *startedState = new QState();
    startedState->assignProperty(startStopButton, "text", "Stop");
    startedState->assignProperty(model, "updating", true);
    stoppedState->addTransition(startStopButton, SIGNAL(clicked()), startedState);
    startedState->addTransition(startStopButton, SIGNAL(clicked()), stoppedState);
    startedState->addTransition(this, SIGNAL(switchStateStopped()), stoppedState);
    startStopMachine->addState(stoppedState);
    startStopMachine->addState(startedState);
    startStopMachine->setInitialState(stoppedState);
    startStopMachine->start();

    QPushButton *selectionButton = new QPushButton("Select displays...");
    connect(selectionButton, SIGNAL(clicked(bool)), this, SLOT(selectDisplaysClicked()));

    QPushButton *selectSitesButton = new QPushButton("Select sites...");
    connect(selectSitesButton, SIGNAL(clicked(bool)), this, SLOT(selectSitesClicked()));

    resetButton = new QPushButton("Reset selected");
    // resetButton->setEnabled(false);
    connect(resetButton, SIGNAL(clicked()), this, SLOT(resetButtonClicked()));

    QHBoxLayout *displayButtons = new QHBoxLayout();
    displayButtons->addWidget(startStopButton);
    displayButtons->addWidget(selectionButton);
    displayButtons->addWidget(selectSitesButton);
    displayButtons->addStretch();
    displayButtons->addWidget(resetButton);

    QGroupBox *alarmsGroup = new QGroupBox("Alarms");
    // alarmsGroup->resize(alarmsGroup->width(), 50);
    alarmsGroup->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    QVBoxLayout *alarmsLayout = new QVBoxLayout(alarmsGroup);

    alarmsModel = new AlarmListModel(this);

    QListView *alarmsList = new QListView();
    alarmsList->setModel(alarmsModel);

    // Get it to be as unobtrusive as possible and the right size
    alarmsList->viewport()->setAutoFillBackground(false);
    alarmsList->setFrameShape(QFrame::NoFrame);
    alarmsList->setSelectionMode(QAbstractItemView::NoSelection);
    alarmsList->setFocusPolicy(Qt::NoFocus);
    alarmsList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    alarmsList->setMaximumHeight(65);
    alarmsList->setWrapping(true);
    alarmsList->setGridSize(QSize(240, 15));

    alarmsLayout->addWidget(alarmsList);

    QVBoxLayout *displayLayout = new QVBoxLayout(this);
    displayLayout->addLayout(displayButtons);
    displayLayout->addWidget(table);
    displayLayout->addWidget(alarmsGroup);

    adjustResetButton();
    adjustAlarms();
}

void DisplayPanel::managersAdded(const QList<SiteManager *> & managers) {
    AppSettings *settings = AppSettings::getInstance();
    QList<int> selectedSites = settings->getSelectedSites();
    if(!settings->getShowSelected()) {
        foreach(SiteManager *manager, managers) {
            QSharedPointer<SiteSettings> site = manager->getSettings();
            selectedSites.append(site->getId()); // Always add new sites
        }
        settings->setSelectedSites(selectedSites);
        model->setSites(sitesList, selectedSites);
    }
}

void DisplayPanel::managersRemoved(const QList<SiteManager *> & managers) {
    bool sitesDirty = false;
    AppSettings *settings = AppSettings::getInstance();
    QList<int> selectedSites = settings->getSelectedSites();
    foreach(SiteManager *manager, managers) {
        int numRemoved = selectedSites.removeAll(manager->getSettings()->getId());
        if(numRemoved > 0)
            sitesDirty = true;
    }

    if(sitesDirty) {
        settings->setSelectedSites(selectedSites);
        if(!settings->getShowSelected()) {
            model->setSites(sitesList, selectedSites);
        }
        adjustResetButton(); // Make sure invalid managers aren't used
        adjustAlarms();
    }

    // Note that the selected site will never be removed, since something
    // else automatically gets selected first (unless there are zero sites)
}

void DisplayPanel::selectDisplaysClicked() {
    // displayPicker is deleted after each use so that it can later have a different manager
    if(displayPicker != NULL)
        delete displayPicker;

    AppSettings *settings = AppSettings::getInstance();
    const QList<int> & selectedSites = settings->getSelectedSites();

    if(settings->getShowSelected()) {
    	displayPicker = new DisplayPickerDialog(sitesList->getCurrentManager(), this);
   	} else if(selectedSites.size() == 0) {
   		displayPicker = new DisplayPickerDialog(NULL, this); // Default labels
   	} else {
   		displayPicker = new DisplayPickerDialog(sitesList->getManagers()[selectedSites[0]], this);
   	}

    displayPicker->getPicker()->setSelectedItems(settings->getSelectedDisplays());

    if(displayPicker->exec() == QDialog::Accepted) {
        QList<enum PMDisplayNumber> displayStates;
        displayPicker->getPicker()->selectedItems(displayStates);
        settings->setSelectedDisplays(displayStates);

        model->setDisplays(displayStates);

    }
    displayPicker->deleteLater();
}

void DisplayPanel::selectSitesClicked() {
    AppSettings *settings = AppSettings::getInstance();
    QList<int> selectedSites = settings->getSelectedSites();

    // sitePicker is not deleted and lasts the lifetime of the application
    if(sitePicker == NULL)
	   sitePicker = new SitePicker(this);
	sitePicker->setData(sitesList, selectedSites);
	sitePicker->setShowSelected(settings->getShowSelected());

	if(sitePicker->exec() == QDialog::Accepted) {
        AppSettings *settings = AppSettings::getInstance();
		sitePicker->getSelected(selectedSites);
		bool showingSelected = sitePicker->isShowSelected();
        settings->setSelectedSites(selectedSites);
        settings->setShowSelected(showingSelected);
        if(showingSelected) {
            currentSiteUpdated();
        } else {
            model->setSites(sitesList, selectedSites);
        }
        adjustAlarms();
	}
}

void DisplayPanel::currentSiteUpdated() {
    AppSettings *settings = AppSettings::getInstance();    
    if(settings->getShowSelected()) {
        QList<int> s;
        int current = sitesList->getCurrentSite();
        if(current >= 0)
            s.append(current);
        model->setSites(sitesList, s);
    } 
}

void DisplayPanel::forceStop() {
    toReset.clear();
    resetButton->setEnabled(false);

    if(displayPicker != NULL)
        displayPicker->reject();
    if(sitePicker != NULL)
        sitePicker->reject();

    emit switchStateStopped();
}

void DisplayPanel::resetButtonClicked() {
    QMessageBox resetBox(this);
    resetBox.setText("Are you sure you want to reset the selected items?");
    resetBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    if(resetBox.exec() != QMessageBox::Ok)
        return;

    QPair<SiteManager *, enum PMDisplayNumber> curr;
    foreach(curr, toReset) {
        curr.first->resetDisplay(curr.second);
    }
}

void DisplayPanel::adjustResetButton() {
    QList<QModelIndex> selectedItems = table->selectionModel()->selectedIndexes();
    toReset.clear();

    for(int i = 0; i < selectedItems.size(); i++) {
        int row = selectedItems[i].row();
        if(row == 0)
            continue;
        int col = selectedItems[i].column();

        if(model->siteAt(col)->getStatus() == SiteManager::SiteUpdating) {
            if(model->siteAt(col)->displayUpToDate(model->displayAt(row)) && isResettable(model->displayAt(row))) {
                toReset.insert(QPair<SiteManager *, enum PMDisplayNumber>(model->siteAt(col), model->displayAt(row)));
            }
        }
    }

    resetButton->setEnabled(!toReset.empty());
}

void DisplayPanel::selectionChanged() {
    adjustResetButton();
    adjustAlarms();
}

void DisplayPanel::adjustAlarms() {
    AppSettings *settings = AppSettings::getInstance();
    if(settings->getShowSelected()) {
        alarmsModel->setManager(sitesList->getCurrentManager());
        return;
    }

    QList<QModelIndex> selectedItems = table->selectionModel()->selectedIndexes();

    int site = -1;
    for(int i = 0; i < selectedItems.size(); i++) {
        int col = selectedItems[i].column();
        if(site != -1 && site != col) {
            site = -1;
            break;
        }
        site = col;
    }

    if(site != -1) {
        alarmsModel->setManager(model->siteAt(site));
    } else {
        alarmsModel->setManager(NULL);
    }
}

void DisplayPanel::siteStatusChanged(SiteManager *site) {
    Q_UNUSED(site);
    adjustResetButton();
}

void DisplayPanel::displaysChanged(const QModelIndex & topLeft, const QModelIndex & bottomRight) {
    if(!topLeft.isValid() || !bottomRight.isValid())
        return;

    if(topLeft.row() < 1)
        return;

    if(topLeft != bottomRight) {
        adjustResetButton();
    }

    enum PMDisplayNumber display = model->displayAt(topLeft.row());
    SiteManager *site = model->siteAt(topLeft.column());

    if(table->selectionModel()->isSelected(topLeft) && site->getStatus() == SiteManager::SiteUpdating && 
        site->displayUpToDate(display) && isResettable(display)) {
        toReset.insert(QPair<SiteManager *, enum PMDisplayNumber>(site, display));
    } else {
        toReset.remove(QPair<SiteManager *, enum PMDisplayNumber>(site, display));
    }
    resetButton->setEnabled(!toReset.empty());
}

bool DisplayPanel::isResettable(enum PMDisplayNumber display) {
    switch(display) {
        case PM_D13:
        case PM_D14:
        case PM_D15:
        case PM_D16:
        case PM_D17:
        case PM_D20:
        case PM_D21:
        case PM_D24:
        case PM_D25:
        case PM_D26:
        case PM_D27: return true;

        default: return false;
    }
} 

