#include "mainwindow.h"
#include "libpmcomm.h"
#include "appsettings.h"

#include <QApplication>
#include <QCoreApplication>
#include <QSettings>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    PMNetInitialize();

    QCoreApplication::setOrganizationName("Bogart Engineering");
    QCoreApplication::setOrganizationDomain("bogartengineering.com");
    QCoreApplication::setApplicationName("PMComm");
    QCoreApplication::setApplicationVersion(APP_VERSION);
    
    QSettings settings;
    AppSettings appSettings(&settings);
    AppSettings::setSingletonInstance(&appSettings);

    MainWindow window;
    window.show();
    return app.exec();
}
