# Uncomment for debugging
# CONFIG += debug

# Application version (NEED TO CHANGE BOTH)
VERSION = 2.0
FULL_VERSION = "2.0 Beta 2"



# Configs below here shouldn't typically need modification

cache()

# Use QextSerialPort
include(3rdparty/qextserialport/src/qextserialport.pri)

# Needed to get widgets in Qt5
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
QT += sql

# Application name
TARGET = PMComm

# Define the preprocessor macro to get the application version in our application
DEFINES += APP_VERSION=\"\\\"$$FULL_VERSION\\\"\"

# Disable debug output (it will still print if CONFIG += debug is present above)
CONFIG(release, debug|release) {
    DEFINES += QT_NO_DEBUG_OUTPUT=1
}

mac {
	# Minimum mac version (Snow Leopard)
	QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.6

	# Icon (Mac)
	ICON = pmcomm.icns

	QMAKE_INFO_PLIST = Info.plist
}

win32 {
	# Enable console on windows when in debug mode
	CONFIG(debug, debug|release) {
	    CONFIG += console
	}

	# Icon (Windows)
	RC_FILE = pmcomm.rc
}

# Get libpmcomm
INCLUDEPATH += ../libpmcomm/include
LIBS += -L"../libpmcomm/" -lpmcomm

# Set up installation
isEmpty(PREFIX) {
	PREFIX = /usr/local
}

target.path = $$PREFIX/bin/
INSTALLS += target

SOURCES  += mainwindow.cpp \
			datafetcher.cpp \
			displayvalue.cpp \
			programvalue.cpp \
			loggedvalue.cpp \
			pmconnectionwrapper.cpp \
			sitemanager.cpp \
			displaytablemodel.cpp \
			programdialog.cpp \
			programpanes.cpp \
			displaypicker.cpp \
			sitepicker.cpp \
			siteslist.cpp \
			sitesettings.cpp \
			displaypanel.cpp \
			sitesdialog.cpp \
			appsettings.cpp \
			optionsdialog.cpp \
			loggeddownloaddialog.cpp \
			loggeddownloader.cpp \
			loggeddata.cpp \
			downloadoptionsdialog.cpp \
			displaypickerdialog.cpp \
			ipv4validator.cpp \
			alarmlistmodel.cpp \
            main.cpp
HEADERS  += mainwindow.h \
			displayvalue.h \
			programvalue.h \
			loggedvalue.h \
			pmconnectionwrapper.h \
			sitemanager.h \
			displaytablemodel.h \
			datafetcher.h \
			programdialog.h \
			programpanes.h \
			displaypicker.h \
			sitepicker.h \
			siteslist.h \
			sitesettings.h \
			displaypanel.h \
			sitesdialog.h \
			appsettings.h \
			optionsdialog.h \
			loggeddownloaddialog.h \
			loggeddownloader.h \
			loggeddata.h \
			downloadoptionsdialog.h \
			displaypickerdialog.h \
			ipv4validator.h \
			alarmlistmodel.h
