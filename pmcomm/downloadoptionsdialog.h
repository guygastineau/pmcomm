#ifndef DOWNLOADOPTIONSDIALOG_H
#define DOWNLOADOPTIONSDIALOG_H

#include "sitesettings.h"

#include <QObject>
#include <QDialog>
#include <QList>
#include <QSharedPointer>

class SiteSettings;
class LoggedDownloader;

class QComboBox;
class QListWidget;
class QLineEdit;

class DownloadOptionsDialog : public QDialog {
	Q_OBJECT
public:
	DownloadOptionsDialog(QSharedPointer<SiteSettings> settings, LoggedDownloader *downloader, QWidget *parent = NULL);

public slots:
	void accept();
	void deleteButtonClicked();
	void exportSelectedClicked();
	void exportAllClicked();

private:
	void refreshDownloadList();
	QList<int> getSelectedIds();
	QList<int> getAllIds();
	void handleExport(QList<int> & ids);
	QSharedPointer<SiteSettings> settings;
	LoggedDownloader *downloader;
	QListWidget *downloadList;

	QComboBox *downloadFrequency;
};

class LoggedExportDialog : public QDialog {
	Q_OBJECT
public:
	LoggedExportDialog(LoggedDownloader *downloader, QList<int> & ids, QWidget *parent = NULL);

public slots:
	void exportClicked();
	void browseClicked();

private:
	LoggedDownloader *downloader;
	QList<int> ids;

	QLineEdit *nameBox;
	QLineEdit *pathBox;
};


#endif