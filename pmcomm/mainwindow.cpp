#include "mainwindow.h"

#include "sitemanager.h"
#include "programdialog.h"
#include "displaypanel.h"
#include "sitesdialog.h"
#include "optionsdialog.h"
#include "loggeddownloaddialog.h"
#include "downloadoptionsdialog.h"
#include "loggeddownloader.h"
#include "siteslist.h"

#include <QPushButton>
#include <QComboBox>
#include <QCloseEvent>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QCoreApplication>

#include <QDebug>

MainWindow::MainWindow(QWidget *parent) : QWidget(parent) {
    AppSettings *settings = AppSettings::getInstance();
    dbErrorCurrent = false;

    setWindowTitle(QString("PMComm %1").arg(QCoreApplication::applicationVersion()));
    setMinimumSize(800, 400);
    settings->restoreWindowGeometry(this);

    sitesList = new SitesList(this);

    siteSelection = new QComboBox;
    siteSelection->setModel(sitesList);

    int currentSite = sitesList->getCurrentSite();
    if(currentSite >= 0)
        siteSelection->setCurrentIndex(sitesList->siteRowById(currentSite));
    else
        siteSelection->setCurrentIndex(0); // No sites indicator

    foreach(SiteManager *manager, sitesList->getManagers()) {
        connect(manager, SIGNAL(communicationsError()), this, SLOT(handleError()));
    }

    connect(siteSelection, SIGNAL(currentIndexChanged(int)), sitesList, SLOT(setCurrentSiteByRow(int)));
    connect(sitesList, SIGNAL(currentSiteUpdated()), this, SLOT(currentSiteChanged()));
    connect(sitesList, SIGNAL(managersAdded(const QList<SiteManager *> &)), this, SLOT(managersAdded(const QList<SiteManager *> &)));

    displayPanel = new DisplayPanel(sitesList);

    downloader = new LoggedDownloader(sitesList, this);
    if(!downloader->isInitialized()) {
        delete downloader;
        downloader = NULL;
        QMessageBox errorMessage(this);
        errorMessage.setText("Could not open logged data database!");
        errorMessage.setStandardButtons(QMessageBox::Ok);
        errorMessage.exec();      
    } else {
        connect(downloader, SIGNAL(downloadDBError()), this, SLOT(downloadDBError()));
    }

    programButton = new QPushButton("Program the PentaMetric...");
    connect(programButton, SIGNAL(clicked(bool)), this, SLOT(programClicked()));

    downloadButton = new QPushButton("Download logged data (manual)...");
    connect(downloadButton, SIGNAL(clicked(bool)), this, SLOT(downloadClicked()));

    downloadOptionsButton = new QPushButton("Logged data auto-download...");
    connect(downloadOptionsButton, SIGNAL(clicked(bool)), this, SLOT(downloadOptionsClicked()));

    QGroupBox *controlsGroup = new QGroupBox("Site");
    QVBoxLayout *controlsLayout = new QVBoxLayout(controlsGroup);
    controlsLayout->addWidget(siteSelection);
    controlsLayout->addWidget(programButton);
    controlsLayout->addWidget(downloadButton);
    controlsLayout->addWidget(downloadOptionsButton);
    controlsLayout->addStretch();

    QPushButton *manageSites = new QPushButton("Manage sites...");
    QPushButton *globalOptions = new QPushButton("PMComm options...");

    connect(manageSites, SIGNAL(clicked(bool)), this, SLOT(manageSitesClicked()));
    connect(globalOptions, SIGNAL(clicked(bool)), this, SLOT(globalOptionsClicked()));

    QGroupBox *optionsGroup = new QGroupBox("Options");
    QVBoxLayout *optionsLayout = new QVBoxLayout(optionsGroup);
    optionsLayout->addWidget(manageSites);
    optionsLayout->addWidget(globalOptions);
    optionsLayout->addStretch();

    QGridLayout *ll = new QGridLayout(this);
    ll->addWidget(controlsGroup, 0, 0);
    ll->addWidget(optionsGroup, 1, 0);
    ll->addWidget(displayPanel, 0, 1, 2, 1);

    setSitesAvailable();
}

void MainWindow::handleError() {
    SiteManager *manager = (SiteManager *) sender();

    QPointer<QMessageBox> errorMessage = new QMessageBox(this);
    errorMessage->setWindowTitle("PMComm communications error");
    errorMessage->setWindowModality(Qt::ApplicationModal);
    errorMessage->setWindowFlags(errorMessage->windowFlags() | Qt::WindowStaysOnTopHint);
    QString message = QString("There was an error communicating with the PentaMetric at site \"%1\"").arg(manager->getSettings()->getName());
    errorMessage->setText(message);
    errorMessage->setInformativeText("Would you like to try again?");
    errorMessage->setStandardButtons(QMessageBox::Yes | QMessageBox::No);

    switch(errorMessage->exec()) {
        case QMessageBox::Yes:
            manager->finishError(true);
            break;

        case QMessageBox::No:
            manager->finishError(false);
            break;
    }
}

void MainWindow::downloadDBError() {
    if(dbErrorCurrent)
        return;
    dbErrorCurrent = true;
    QMessageBox errorMessage(this);
    errorMessage.setText("Failed to write to logged data database!");
    errorMessage.setStandardButtons(QMessageBox::Ok);
    errorMessage.exec();
    dbErrorCurrent = false;
}

void MainWindow::programClicked() {
    if(programDialog != NULL)
        return;

    programDialog = new ProgramDialog(sitesList->getCurrentManager(), this);
    programDialog->setAttribute(Qt::WA_DeleteOnClose);

    // This window is NOT modal
    programDialog->show();
}

void MainWindow::downloadClicked() {
    QDialog *downloadDialog = new LoggedDownloadDialog(sitesList->getCurrentManager(), this);
    downloadDialog->exec();
    downloadDialog->deleteLater();
}

void MainWindow::downloadOptionsClicked() {
    QDialog *downloadOptionsDialog = new DownloadOptionsDialog(sitesList->getCurrentManager()->getSettings(), downloader, this);
    downloadOptionsDialog->exec();
    downloadOptionsDialog->deleteLater();
}

void MainWindow::manageSitesClicked() {
    if(programDialog != NULL && !programDialog->closePrograms())
        return;

    displayPanel->forceStop();

    SitesDialog *sitesDialog = new SitesDialog(sitesList, this);

    QSet<int> sitesWithDBData; // Calculate which sites actually have data stored so we can warn the user
    foreach(SiteManager *manager, sitesList->getManagers()) {
        manager->releaseFetcher();
        if(downloader) {
            int id = manager->getSettings()->getId();
            bool success;
            if(!downloader->downloadsForSite(id, success).empty())
                sitesWithDBData.insert(id);
        }
    }

    sitesDialog->setSitesWithDBData(sitesWithDBData);
    sitesDialog->exec();

    foreach(SiteManager *manager, sitesList->getManagers()) {
        manager->reconnect();
    }   

    sitesDialog->deleteLater();
}

// No need to look at managersRemoved, since the managers get deleted afterwards anyway
void MainWindow::managersAdded(const QList<SiteManager *> & managers) {
    foreach(SiteManager *manager, managers) {
        connect(manager, SIGNAL(communicationsError()), this, SLOT(handleError()));
    }
}

void MainWindow::setSitesAvailable() {
    bool avail = (sitesList->getCurrentSite() >= 0);
    programButton->setEnabled(avail);
    downloadButton->setEnabled(avail);
    downloadOptionsButton->setEnabled(avail && downloader != NULL);
}

void MainWindow::currentSiteChanged() {
    setSitesAvailable();
}

void MainWindow::globalOptionsClicked() {
    OptionsDialog *optionsDialog = new OptionsDialog(this);
    optionsDialog->exec();

    optionsDialog->deleteLater();   
}

void MainWindow::closeEvent(QCloseEvent *event) {
    AppSettings *settings = AppSettings::getInstance();
    settings->saveWindowGeometry(this);
    event->accept();
}
