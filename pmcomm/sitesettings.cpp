#include "sitesettings.h"

#include "appsettings.h"

SiteSettings::SiteSettings(int id, QString hostName, unsigned short port) {
	this->id = id;
	internet = true;

	this->hostName = hostName;
	this->port = port;

	downloadInterval = 0;
}
SiteSettings::SiteSettings(int id, QString serialPort) {
	this->id = id;
	internet = false;

	this->serialPort = serialPort;

	downloadInterval = 0;
}

void SiteSettings::setId(int id) {
	this->id = id;
}

QString SiteSettings::getName() const {
	return name;
}

void SiteSettings::setName(QString name) {
	this->name = name;
}

int SiteSettings::getId() const {
	return id;
}

bool SiteSettings::isInternet() const {
	return internet;
}

QString SiteSettings::getHostname() const {
	return hostName;
}

unsigned short SiteSettings::getPort() const {
	return port;
}

QString SiteSettings::getSerialPort() const {
	return serialPort;
}

int SiteSettings::getDownloadInterval() const {
	return downloadInterval;
}
void SiteSettings::setDownloadInterval(int secs) {
	downloadInterval = secs;
}

void SiteSettings::saveChanges() {
	AppSettings *appSettings = AppSettings::getInstance();
	appSettings->saveSites();
}

// Checks if the settings are too similar to other
bool SiteSettings::conflictsWith(const SiteSettings & other) const {
	if(internet && other.internet) {
		if(hostName == other.hostName && port == other.port)
			return true;
	}

	if(!internet && !other.internet) {
		if(serialPort == other.serialPort)
			return true;
	}

	return false;
}
