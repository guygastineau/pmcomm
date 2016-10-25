#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include "appsettings.h"

#include <QObject>
#include <QDialog>

class QRadioButton;
class QComboBox;

class OptionsDialog : public QDialog {
	Q_OBJECT
public:
	OptionsDialog(QWidget *parent = NULL);

	void setTempUnit(AppSettings::TempUnit unit);
	AppSettings::TempUnit getTempUnit();

public slots:
	void saveData();

private:
	QRadioButton *celsiusButton;
	QRadioButton *fahrenheitButton;
	QComboBox *intervalBox;
};





#endif