#ifndef SITESETTINGS_H
#define SITESETTINGS_H

#include <QString>
#include <QSharedPointer>
#include <QMetaType>

class SiteSettings {
public:
	SiteSettings(int id, QString hostName, unsigned short port);
	SiteSettings(int id, QString serialPort);

	// Used when the id is initially unset
	void setId(int id);

	QString getName() const;
	void setName(QString name);
	int getId() const;

	bool isInternet() const;

	QString getHostname() const;
	unsigned short getPort() const;

	QString getSerialPort() const;

	int getDownloadInterval() const;
	void setDownloadInterval(int secs);

	void saveChanges();

	bool conflictsWith(const SiteSettings & other) const;

private:
	QString name;
	int id;
	bool internet;

	QString hostName;
	unsigned short port;

	QString serialPort;

	int downloadInterval;
};

Q_DECLARE_METATYPE(QSharedPointer<SiteSettings>);


#endif
