#ifndef LOGGEDDOWNLOADDIALOG_H
#define LOGGEDDOWNLOADDIALOG_H

#include <QObject>
#include <QDialog>
#include "loggedvalue.h"

class LoggedProgressDialog;
class SiteManager;

class QLabel;
class QLineEdit;
class QCheckBox;
class QProgressBar;
class QDialogButtonBox;

class LoggedDownloadDialog : public QDialog {
	Q_OBJECT

public:
	LoggedDownloadDialog(SiteManager *manager, QWidget *parent = 0);

public slots:
	void downloadClicked();
	void browseClicked();

	void downloaded();
	void errorCanceled();
	void cancelDownload();

private:
	SiteManager *manager;
	QLineEdit *pathBox;
	QLineEdit *nameBox;

	QCheckBox *periodicBox;
	QCheckBox *profileBox;
	QCheckBox *efficiencyBox;

	LoggedProgressDialog *progress;
	bool canceled;
};

class LoggedProgressDialog : public QDialog {
	Q_OBJECT

public:
	LoggedProgressDialog(QWidget *parent = 0);

public slots:
	void setProgress(LoggedValue::LoggedDataType type, int progress, int outof);
	void cancelDownload();

signals:
	void downloadCanceled();

private:
	bool canceling;
	QDialogButtonBox *buttonBox;
	QLabel *status;
	QProgressBar *progressBar;
};


#endif