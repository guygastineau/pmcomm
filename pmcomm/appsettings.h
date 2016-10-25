#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include "pmdefs.h"

#include <QObject>
#include <QList>
#include <QVariant>
#include <QSharedPointer>

class QSettings;
class SiteSettings;
class PMConnectionWrapper;

class AppSettings : public QObject {
	Q_OBJECT
	Q_ENUMS(TempUnit)
public:
	AppSettings(QSettings *s);

	static AppSettings *getInstance() { return singletonInstance; }
	static void setSingletonInstance(AppSettings *instance) { singletonInstance = instance; }

	enum TempUnit {
		Celsius,
		Fahrenheit
	};

	void restoreWindowGeometry(QWidget *widget);
	void saveWindowGeometry(QWidget *widget);

	TempUnit getTempUnit();
	void setTempUnit(TempUnit unit);

	QMap<int, QSharedPointer<SiteSettings> > *getSites();
	void saveSites();
	int getCurrentSite();
	void setCurrentSite(int site);

	QList<int> getSelectedSites();
	void setSelectedSites(const QList<int> & selectedSites);
	bool getShowSelected();
	void setShowSelected(bool showSelected);

	QList<enum PMDisplayNumber> getSelectedDisplays();
	void setSelectedDisplays(const QList<enum PMDisplayNumber> & selectedDisplays);

	QString getSettingsExportPath();
	void setSettingsExportPath(QString path);

	QString getDownloadPath();
	void setDownloadPath(QString path);
	int getEfficiencyAverageInterval();
	void setEfficiencyAverageInterval(int interval);

signals:
	void tempUnitChanged();
	void sitesChanged();
	void currentSiteChanged();
	void selectedSitesChanged();
	void showSelectedChanged();
	void selectedDisplaysChanged();

private:
	static AppSettings *singletonInstance;

	QSettings *settings;
	QMap<int, QSharedPointer<SiteSettings> > *sites;

	QList<enum PMDisplayNumber> defaultDisplays;

	QVariant encodeDisplayNum(enum PMDisplayNumber display);
	enum PMDisplayNumber decodeDisplayNum(QVariant value);

	QVariant encodeTempUnit(TempUnit unit);
	enum TempUnit decodeTempUnit(QVariant value);
};

Q_DECLARE_METATYPE(AppSettings::TempUnit);


#endif