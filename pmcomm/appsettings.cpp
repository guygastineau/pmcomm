#include "appsettings.h"

#include "sitesettings.h"

#include <QMetaObject>
#include <QMetaEnum>
#include <QWidget>
#include <QSettings>

// The way to get standard locations changed between Qt4 and Qt5
#if QT_VERSION >= 0x050000
#include <QStandardPaths>
#else
#include <QDesktopServices>
#endif

AppSettings *AppSettings::singletonInstance = NULL;

AppSettings::AppSettings(QSettings *s) {
	settings = s;
	sites = NULL;

	qRegisterMetaType<TempUnit>("AppSettings::TempUnit");

	defaultDisplays << PM_D1 << PM_D2 << PM_D7 << PM_D8 << PM_D9 << PM_D28;
}

void AppSettings::restoreWindowGeometry(QWidget *widget) {
	if(settings->contains("general/geometry")) {
		widget->restoreGeometry(settings->value("general/geometry").toByteArray());
	}
}

void AppSettings::saveWindowGeometry(QWidget *widget) {
	settings->setValue("general/geometry", widget->saveGeometry());
}

AppSettings::TempUnit AppSettings::getTempUnit() {
	return decodeTempUnit(settings->value("general/tempUnit", encodeTempUnit(Celsius)));
}

void AppSettings::setTempUnit(TempUnit unit) {
	settings->setValue("general/tempUnit", encodeTempUnit(unit));
	emit tempUnitChanged();
}

QMap<int, QSharedPointer<SiteSettings> > *AppSettings::getSites() {
	if(sites != NULL)
		return sites;

	sites = new QMap<int, QSharedPointer<SiteSettings> >();

	if(!settings->value("sites/sitesStored", false).toBool()) {
		SiteSettings *site = new SiteSettings(0, "COM1");
		site->setName("Default");
		(*sites)[0] = QSharedPointer<SiteSettings>(site);
		return sites;
	}

	int nSites = settings->beginReadArray("sites/sites");

	for(int i = 0; i < nSites; i++) {
		settings->setArrayIndex(i);
		int id = settings->value("id").toInt();
		QString name = settings->value("name").toString();
		bool isInternet = settings->value("internet", false).toBool();
		SiteSettings *site = NULL;
		if(isInternet) {
			QString hostName = settings->value("host").toString();
			unsigned short port = settings->value("port").toInt();
			site = new SiteSettings(id, hostName, port);
		} else {
			QString serialPort = settings->value("serialPort").toString();
			site = new SiteSettings(id, serialPort);
		}

		int downloadInterval = settings->value("downloadInterval").toInt();
		site->setDownloadInterval(downloadInterval);

		// site->setContext(this);
		site->setName(name);
		(*sites)[id] = QSharedPointer<SiteSettings>(site);
	}

	settings->endArray();
	return sites;
}

void AppSettings::saveSites() {
	if(sites == NULL)
		return;

	settings->beginWriteArray("sites/sites", sites->size());

	int i = 0;
	QSharedPointer<SiteSettings> site;
	foreach(site, *sites) {
		settings->setArrayIndex(i);
		settings->setValue("id", site->getId());
		settings->setValue("name", site->getName());
		bool isInternet = site->isInternet();
		settings->setValue("internet", isInternet);
		if(isInternet) {
			settings->setValue("host", site->getHostname());
			settings->setValue("port", site->getPort());
		} else {
			settings->setValue("serialPort", site->getSerialPort());
		}
		settings->setValue("downloadInterval", site->getDownloadInterval());
		i++;
	}

	settings->endArray();
	settings->setValue("sites/sitesStored", true);
	emit sitesChanged();
}

int AppSettings::getCurrentSite() {
	return settings->value("sites/currentSite", -1).toInt();
}
	
void AppSettings::setCurrentSite(int site) {
	settings->setValue("sites/currentSite", site);
	emit currentSiteChanged();
}

QList<int> AppSettings::getSelectedSites() {
	QList<int> selected;

	int nSelected = settings->beginReadArray("display/selectedSites");

	for(int i = 0; i < nSelected; i++) {
		settings->setArrayIndex(i);
		int id = settings->value("id").toInt();
		selected.append(id);
	}

	settings->endArray();
	return selected;
}

void AppSettings::setSelectedSites(const QList<int> & selectedSites) {
	settings->beginWriteArray("display/selectedSites", selectedSites.size());

	for(int i = 0; i < selectedSites.size(); i++) {
		settings->setArrayIndex(i);
		settings->setValue("id", selectedSites[i]);
	}

	settings->endArray();
	emit selectedSitesChanged();
}

bool AppSettings::getShowSelected() {
	return settings->value("display/showSelected", true).toBool();
}

void AppSettings::setShowSelected(bool showSelected) {
	settings->setValue("display/showSelected", showSelected);
	emit showSelectedChanged();
}

QList<enum PMDisplayNumber> AppSettings::getSelectedDisplays() {
	if(!settings->value("display/selectedDisplaysSaved", false).toBool()) {
		return defaultDisplays;
	}

	QList<enum PMDisplayNumber> selected;

	int nSelected = settings->beginReadArray("display/selectedDisplays");

	for(int i = 0; i < nSelected; i++)  {
		settings->setArrayIndex(i);
		enum PMDisplayNumber display = decodeDisplayNum(settings->value("display"));
		selected.append(display);
	}

	settings->endArray();
	return selected;
}

void AppSettings::setSelectedDisplays(const QList<enum PMDisplayNumber> & selectedDisplays) {
	settings->beginWriteArray("display/selectedDisplays", selectedDisplays.size());

	for(int i = 0; i < selectedDisplays.size(); i++) {
		settings->setArrayIndex(i);
		settings->setValue("display", encodeDisplayNum(selectedDisplays[i]));
	}

	settings->endArray();
	settings->setValue("display/selectedDisplaysSaved", true);
	emit selectedDisplaysChanged();
}

QString AppSettings::getDownloadPath() {
#if QT_VERSION >= 0x050000
	QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
#else
	QString defaultPath = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);
#endif
	defaultPath.append("/PentaMetric Data");

	return settings->value("logged/downloadPath", defaultPath).toString();
}

void AppSettings::setDownloadPath(QString path) {
	settings->setValue("logged/downloadPath", path);
}

QString AppSettings::getSettingsExportPath() {
#if QT_VERSION >= 0x050000
	QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
#else
	QString defaultPath = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);
#endif

	return settings->value("general/settingsExportPath", defaultPath).toString();
}

void AppSettings::setSettingsExportPath(QString path) {
	settings->setValue("general/settingsExportPath", path);
}

int AppSettings::getEfficiencyAverageInterval() {
	return settings->value("logged/efficiencyAverageInterval", 5).toInt();
}

void AppSettings::setEfficiencyAverageInterval(int interval) {
	settings->setValue("logged/efficiencyAverageInterval", interval);
}

QVariant AppSettings::encodeDisplayNum(enum PMDisplayNumber display) {
	return QVariant((int) display);
}

enum PMDisplayNumber AppSettings::decodeDisplayNum(QVariant value) {
	return (enum PMDisplayNumber) value.toInt();
}

QVariant AppSettings::encodeTempUnit(TempUnit unit) {
	const QMetaObject & meta = AppSettings::staticMetaObject;
	int index = meta.indexOfEnumerator("TempUnit");
	QMetaEnum metaEnum = meta.enumerator(index);
	return QVariant(metaEnum.valueToKey(unit));
}

AppSettings::TempUnit AppSettings::decodeTempUnit(QVariant value) {
	const QMetaObject & meta = AppSettings::staticMetaObject;
	int index = meta.indexOfEnumerator("TempUnit");
	QMetaEnum metaEnum = meta.enumerator(index);
	QByteArray v = value.toString().toUtf8();
	return (TempUnit) metaEnum.keyToValue(v.constData());
}

