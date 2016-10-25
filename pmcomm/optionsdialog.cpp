#include "optionsdialog.h"

#include "appsettings.h"

#include <QRadioButton>
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QDialogButtonBox>

OptionsDialog::OptionsDialog(QWidget *parent) : QDialog(parent) {
	setWindowTitle("PMComm options");
	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	AppSettings *settings = AppSettings::getInstance();
	QFormLayout *layout = new QFormLayout(this);

	QGroupBox *temperatureBox = new QGroupBox("Temperature units");
	QHBoxLayout *temperatureLayout = new QHBoxLayout(temperatureBox);
	celsiusButton = new QRadioButton("Celsius");
	celsiusButton->setChecked(true);
	fahrenheitButton = new QRadioButton("Fahrenhiet");
	temperatureLayout->addWidget(celsiusButton);
	temperatureLayout->addWidget(fahrenheitButton);

	intervalBox = new QComboBox();
	for(int i = 2; i <= 10; i++) {
		intervalBox->addItem(QString::number(i), i);
	}
	int interval = settings->getEfficiencyAverageInterval();
	int intervalIndex = intervalBox->findData(interval);
	if(intervalIndex < 0) {
		intervalIndex = intervalBox->count();
		intervalBox->addItem(QString::number(interval), interval);
	}
	intervalBox->setCurrentIndex(intervalIndex);

	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
	connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
	connect(this, SIGNAL(accepted()), this, SLOT(saveData()));

	layout->addRow(temperatureBox);
	layout->addRow("Number of cycles to average for efficiency data", intervalBox);
	layout->addRow(buttonBox);

	if(settings->getTempUnit() == AppSettings::Fahrenheit)
		fahrenheitButton->setChecked(true);
	else
		celsiusButton->setChecked(true);
}

void OptionsDialog::saveData() {
    AppSettings *settings = AppSettings::getInstance();
    settings->setTempUnit(fahrenheitButton->isChecked() ? AppSettings::Fahrenheit : AppSettings::Celsius);
    settings->setEfficiencyAverageInterval(intervalBox->itemData(intervalBox->currentIndex()).toInt());
}
