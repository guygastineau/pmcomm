#ifndef PROGRAMPANES_H
#define PROGRAMPANES_H

#include "pmdefs.h"
#include "programvalue.h"
#include "sitemanager.h"
#include "displaypicker.h"
#include "ipv4validator.h"

#include <QLabel>
#include <QGroupBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QIntValidator>
#include <QSharedPointer>
#include <QPointer>
#include <QMessageBox>
#include <QTime>
#include <QTimeEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QListWidget>
#include <QListWidgetItem>

// This macro inserts the getter and setter for the model. This is useful so that the model
// can be a different type in each class, which obviates the need for casts everywhere
#define GET_SET_DATA_MACRO(type) QSharedPointer<ProgramValue> getData() { return model; } \
void setData(QSharedPointer<ProgramValue> value) { model = value.staticCast<type>(); }

class ProgramPane : public QGroupBox {
	Q_OBJECT

public:
	ProgramPane(const QString &title, QWidget *parent = 0) : QGroupBox(title, parent), name(title) { batteryNum = 0; }
	QString getName() { return name; }
	void setBatteryNum(int num) { batteryNum = num; }
	int getBatteryNum() { return batteryNum; }
	virtual bool needsRead() { return true; }

	static ProgramPane *makeProgramPane(enum PMProgramNumber prog, SiteManager *manager, QWidget *parent = 0);

	virtual bool isDirty() = 0;
	virtual bool isValid() { return true; }
	virtual void handleInvalid() {}

	// These actually get and set the underlying data
	virtual QSharedPointer<ProgramValue> getData() = 0;
	virtual void setData(QSharedPointer<ProgramValue> value) = 0;

	virtual QString helpText() = 0; // There is a help method for each pane even though it isn't always used

public slots:
	// These load the underlying model into the widgets and save the widget data back into the model
	virtual void loadData() = 0;
	virtual bool saveData() = 0;

protected:
	int batteryNum;
	QString name;
};

class SwitchSelectProgramPane : public ProgramPane {
	Q_OBJECT

public:
	GET_SET_DATA_MACRO(SwitchSelectProgramValue)
	SwitchSelectProgramPane(const QString &title, SiteManager *manager, QWidget *parent = 0) : ProgramPane(title, parent) {
		QVBoxLayout *layout = new QVBoxLayout(this);

		picker = new DisplayPicker(manager);
		picker->addDefaultDisplays();
		picker->addEfficiencyDisplays();
		layout->addWidget(picker);
	}

	bool isDirty() {
		if(model == NULL)
			return false;

		QList<enum PMDisplayNumber> modelDisplays;
		model->getDisplays(modelDisplays);

		QList<enum PMDisplayNumber> pickerDisplays;
		picker->selectedItems(pickerDisplays);
		return modelDisplays != pickerDisplays;
	}

	bool isValid() {
		QList<enum PMDisplayNumber> displays;
		picker->selectedItems(displays);
		return displays.size() <= 5;
	}

	void handleInvalid() {
		QPointer<QMessageBox> errorMessage = new QMessageBox(this);
		errorMessage->setText("A maximum of five displays can be selected per button");
		errorMessage->setStandardButtons(QMessageBox::Ok);
		errorMessage->setWindowModality(Qt::WindowModal);
		errorMessage->exec();	
	}

	QString helpText();

public slots:
	void loadData() {
		QList<enum PMDisplayNumber> displays;
		model->getDisplays(displays);
		picker->setSelectedItems(displays);
	}

	bool saveData() {
		QList<enum PMDisplayNumber> displays;
		picker->selectedItems(displays);
		model->setDisplays(displays);
		return true;
	}

private:
	QSharedPointer<SwitchSelectProgramValue> model;
	DisplayPicker *picker;
};

class LabelsProgramPane : public ProgramPane {
	Q_OBJECT

public:
	GET_SET_DATA_MACRO(LabelsProgramValue)
	LabelsProgramPane(const QString &title, QWidget *parent = 0) : ProgramPane(title, parent) {
		QFormLayout *layout = new QFormLayout(this);

		batteryLabel = new QComboBox;
		batteryLabel->insertItem(0, "Battery");
		batteryLabel->insertItem(1, "Battery 1");

		layout->addRow("P6: Label for Volts 1", batteryLabel);

		for(int i = 0; i < 3; i++) {
			ampsLabels[i] = new QComboBox;
			ampsLabels[i]->insertItem(0, QString("Amps %1").arg(i + 1));
			ampsLabels[i]->insertItem(1, QString("Solar"));
			ampsLabels[i]->insertItem(2, QString("Wind"));
			ampsLabels[i]->insertItem(3, QString("Hydro"));
			ampsLabels[i]->insertItem(4, QString("Load"));
			ampsLabels[i]->insertItem(5, QString("Battery"));
			if(i < 2)
				ampsLabels[i]->insertItem(6, QString("Battery %1").arg(i + 1));

			layout->addRow(QString("P%1: Label for Amps %2").arg(i + 7).arg(i + 1), ampsLabels[i]);
		}
	}

	bool isDirty() {
		if(model == NULL)
			return false;

		struct PMLabels labels;
		model->getLabels(labels);
		bool dirty = (labels.battLabel != batteryLabel->currentIndex());

		for(int i = 0; i < 3; i++) {
			dirty = dirty || (labels.ampsLabels[i] != ampsLabels[i]->currentIndex());
		}
		return dirty;
	}

	QString helpText();

public slots:
	void loadData() {
		struct PMLabels labels;
		model->getLabels(labels);

		batteryLabel->setCurrentIndex(labels.battLabel);

		for(int i = 0; i < 3; i++) {
			ampsLabels[i]->setCurrentIndex(labels.ampsLabels[i]);
		}
	}

	bool saveData() {
		struct PMLabels labels;
		labels.battLabel = (PMBatteryLabel) batteryLabel->currentIndex();

		for(int i = 0; i < 3; i++) {
			labels.ampsLabels[i] = (PMAmpsLabel) ampsLabels[i]->currentIndex();
		}
		model->setLabels(labels);
		return true;
	}

private:
	QSharedPointer<LabelsProgramValue> model;
	QComboBox *batteryLabel;
	QComboBox *ampsLabels[3];
};

class ShuntTypesProgramPane : public ProgramPane {
	Q_OBJECT

public:
	GET_SET_DATA_MACRO(ShuntTypesProgramValue)
	ShuntTypesProgramPane(const QString &title, QWidget *parent = 0) : ProgramPane(title, parent) {
		QFormLayout *layout = new QFormLayout(this);

		for(int i = 0; i < 3; i++) {
			shuntTypes[i] = new QComboBox;
			shuntTypes[i]->insertItem(0, QString("100A/100mV"));
			shuntTypes[i]->insertItem(1, QString("500A/50mV"));

			layout->addRow(QString("P%1: Shunt for Amps %2").arg(i + 11).arg(i + 1), shuntTypes[i]);

			if(i == 1) { // Battery 2
				amps2Sum = new QCheckBox;
				layout->addRow("Read Amps 2 as (shunt input 1 + shunt input 2)", amps2Sum);
			}
		}
	}

	bool isDirty() {
		if(model == NULL)
			return false;
		bool dirty = false;

		for(int i = 0; i < 3; i++) {
			dirty = dirty || (shuntTypes[i]->currentIndex() != ((model->shuntType(i + 1) == PM_SHUNTTYPE_500A50MV) ? 1 : 0));
		}
		dirty = dirty || (amps2Sum->isChecked() != model->isAmps2Sum());

		return dirty;
	}

	QString helpText();

public slots:
	void loadData() {
		for(int i = 0; i < 3; i++) {
			shuntTypes[i]->setCurrentIndex((model->shuntType(i + 1) == PM_SHUNTTYPE_500A50MV) ? 1 : 0);
		}
		amps2Sum->setChecked(model->isAmps2Sum());
	}

	bool saveData() {
		for(int i = 0; i < 3; i++) {
			model->setShuntType(i + 1, (shuntTypes[i]->currentIndex() == 1) ? PM_SHUNTTYPE_500A50MV : PM_SHUNTTYPE_100A100MV);
		}
		model->setAmps2Sum(amps2Sum->isChecked());
		return true;
	}

	void setAmps2SumEnabled(bool enabled) {
		amps2Sum->setEnabled(enabled);
		if(!enabled)
			amps2Sum->setChecked(false); // Enforce disabling this on relevant versions
	}

private:
	QSharedPointer<ShuntTypesProgramValue> model;
	QComboBox *shuntTypes[3];
	QCheckBox *amps2Sum;
};


class CapacityProgramPane : public ProgramPane {
	Q_OBJECT

public:
	GET_SET_DATA_MACRO(CapacityProgramValue)
	CapacityProgramPane(const QString &title, int batteryNum, QWidget *parent = 0) : ProgramPane(title, parent) {
		setBatteryNum(batteryNum);

		QFormLayout *layout = new QFormLayout(this);

		batteryPresent = new QCheckBox;
		layout->addRow("Treat this channel as battery", batteryPresent);

		capacityField = new QLineEdit;
		QValidator *validator = new QIntValidator(1, 9999, this);
		capacityField->setValidator(validator);
		layout->addRow("Amp Hours (1-9999)", capacityField);

		connect(batteryPresent, SIGNAL(stateChanged(int)), this, SLOT(controlsChanged()));
	}

	bool isDirty() {
		if(model == NULL)
			return false;
		if(!isValid())
			return true;
		return capacityField->text().toInt() != model->getCapacity();
	}

	bool isValid() {
		return !batteryPresent->isChecked() || capacityField->hasAcceptableInput();
	}

	void handleInvalid() {
		QPointer<QMessageBox> errorMessage = new QMessageBox(this);
		errorMessage->setText("Capacity must be between 1 and 9999 Ah when a battery is enabled");
		errorMessage->setStandardButtons(QMessageBox::Ok);
		errorMessage->setWindowModality(Qt::WindowModal);
		errorMessage->exec();	
	}

	bool batteryEnabled() {
		return batteryPresent->isChecked();
	}

	QString helpText();

public slots:
	void loadData() {
		batteryPresent->setChecked(model->batteryPresent());

		capacityField->setEnabled(model->batteryPresent());
		capacityField->setText(QString::number(model->getCapacity()));
	}

	bool saveData() {
		model->setCapacity(capacityField->text().toInt());
		return true;
	}

	void controlsChanged() {
		bool present = batteryPresent->isChecked();
		capacityField->setEnabled(present);
		if(!present) {
			capacityField->setText("0");
		}
		emit batteryPresenceUpdated(getBatteryNum(), present);
	}

signals:
	void batteryPresenceUpdated(int battery, bool present);

private:
	QSharedPointer<CapacityProgramValue> model;
	QLineEdit *capacityField;
	QCheckBox *batteryPresent;
};

class FilterTimeProgramPane : public ProgramPane {
	Q_OBJECT

public:
	GET_SET_DATA_MACRO(FilterTimeProgramValue)
	FilterTimeProgramPane(const QString &title, QWidget *parent = 0) : ProgramPane(title, parent) {
		setBatteryNum(batteryNum);

		QFormLayout *layout = new QFormLayout(this);

		QLabel *info = new QLabel("Select filter time for average amps (AD10-12) and average volts (AD3-4)");
		layout->addRow(info);
		filterTime = new QComboBox;

		filterTime->addItem("0");
		filterTime->addItem("0.5");
		filterTime->addItem("2");
		filterTime->addItem("8");
		filterTime->addItem("32");

		layout->addRow("Filter Time (minutes)", filterTime);
	}

	bool isDirty() {
		if(model == NULL)
			return false;
		return filterTime->currentIndex() != (int) model->filterTime();
	}

	QString helpText();

public slots:
	void loadData() {
		filterTime->setCurrentIndex((int) model->filterTime());
	}

	bool saveData() {
		model->setFilterTime((enum PMFilterTime) filterTime->currentIndex());
		return true;
	}

private:
	QSharedPointer<FilterTimeProgramValue> model;
	QComboBox *filterTime;
};

class AlarmLevelProgramPane : public ProgramPane {
	Q_OBJECT

public:
	GET_SET_DATA_MACRO(AlarmLevelProgramValue)
	AlarmLevelProgramPane(const QString &title, int batteryNum, QWidget *parent = 0) : ProgramPane(title, parent) {
		setBatteryNum(batteryNum);
		QFormLayout *layout = new QFormLayout(this);

		for(int i = 0; i < 5; i++) {
			levels[i] = new QComboBox;
			levels[i]->insertItem(0, QString("OFF"));
			levels[i]->insertItem(1, QString("ON VISUAL ONLY"));
			levels[i]->insertItem(2, QString("ON VISUAL/AUDIBLE"));

			QString label;
			switch(i) {
				case 0:
					label = "Low Battery";
					break;
				case 1:
					label = "Battery Charged";
					break;
				case 2:
					label = "High Battery";
					break;
				case 3:
					label = "Time to Charge";
					break;
				case 4:
					label = "Time to Equalize";
					break;
				default:
					break;
			}
			layout->addRow(label, levels[i]);
		}
	}

	bool isDirty() {
		if(model == NULL)
			return false;
		bool dirty = false;

		for(int i = 0; i < 5; i++) {
			dirty = dirty || (levels[i]->currentIndex() != (int) model->getLevelForAlarm((enum PMAlarmIndex) i));
		}
		return dirty;
	}

	QString helpText();

public slots:
	void loadData() {
		for(int i = 0; i < 5; i++) {
			levels[i]->setCurrentIndex((int) model->getLevelForAlarm((enum PMAlarmIndex) i));
		}
	}

	bool saveData() {
		for(int i = 0; i < 5; i++) {
			model->setLevelForAlarm((enum PMAlarmIndex) i, (enum PMAlarmLevel) levels[i]->currentIndex());
		}
		return true;
	}

private:
	QSharedPointer<AlarmLevelProgramValue> model;
	QComboBox *levels[5];
};

class LowAlarmProgramPane : public ProgramPane {
	Q_OBJECT

public:
	GET_SET_DATA_MACRO(LowAlarmProgramValue)
	LowAlarmProgramPane(const QString &title, int batteryNum, QWidget *parent = 0) : ProgramPane(title, parent) {
		setBatteryNum(batteryNum);

		QFormLayout *layout = new QFormLayout(this);

		voltsField = new QLineEdit;
		QValidator *validator = new QDoubleValidator(0, 102.3, 1, this);
		voltsField->setValidator(validator);
		layout->addRow("Volts less than", voltsField);

		percentField = new QLineEdit;
		validator = new QIntValidator(1, 100, this);
		percentField->setValidator(validator);
		layout->addRow("OR Percent full less than", percentField);

		percentEnabled = new QCheckBox();
		layout->addRow("Use percent full", percentEnabled);

		connect(percentEnabled, SIGNAL(stateChanged(int)), this, SLOT(controlsChanged()));
	}

	bool isDirty() {
		if(model == NULL)
			return false;
		if(!isValid())
			return true;
		return voltsField->text() != QString::number(model->getVoltage(), 'f', 1) || percentField->text().toInt() != model->getPercent();
	}

	bool isValid() {
		return (!percentEnabled->isChecked() || percentField->hasAcceptableInput()) && voltsField->hasAcceptableInput();
	}

	void handleInvalid() {
		QPointer<QMessageBox> errorMessage = new QMessageBox(this);
		errorMessage->setText("Volts must be between 0 and 102.3, and percent full must be between 1 and 100 (when enabled)");
		errorMessage->setStandardButtons(QMessageBox::Ok);
		errorMessage->setWindowModality(Qt::WindowModal);
		errorMessage->exec();	
	}

	QString helpText();

public slots:
	void loadData() {
		voltsField->setText(QString::number(model->getVoltage(), 'f', 1));

		percentField->setText(QString::number(model->getPercent()));

		percentEnabled->setChecked(model->getPercent() != 0);
	}

	bool saveData() {
		model->setVoltage(voltsField->text().toDouble());
		model->setPercent(percentField->text().toInt());
		return true;
	}

	void controlsChanged() {
		bool percent = percentEnabled->isChecked();
		percentField->setEnabled(percent);
		if(!percent) {
			percentField->setText("0");
		}
	}
private:
	QSharedPointer<LowAlarmProgramValue> model;
	QLineEdit *voltsField;
	QLineEdit *percentField;
	QCheckBox *percentEnabled;
};

class HighAlarmProgramPane : public ProgramPane {
	Q_OBJECT

public:
	GET_SET_DATA_MACRO(HighAlarmProgramValue)
	HighAlarmProgramPane(const QString &title, int batteryNum, QWidget *parent = 0) : ProgramPane(title, parent) {
		setBatteryNum(batteryNum);

		QFormLayout *layout = new QFormLayout(this);

		voltsField = new QLineEdit;
		QValidator *validator = new QDoubleValidator(0, 102.3, 1, this);
		voltsField->setValidator(validator);
		layout->addRow("Volts greater than", voltsField);
	}

	bool isDirty() {
		if(model == NULL)
			return false;
		if(!isValid())
			return true;
		return voltsField->text() != QString::number(model->getVoltage(), 'f', 1);
	}

	bool isValid() {
		return voltsField->hasAcceptableInput();
	}

	void handleInvalid() {
		QPointer<QMessageBox> errorMessage = new QMessageBox(this);
		errorMessage->setText("Volts must be between 0 and 102.3");
		errorMessage->setStandardButtons(QMessageBox::Ok);
		errorMessage->setWindowModality(Qt::WindowModal);
		errorMessage->exec();	
	}

	QString helpText();

public slots:
	void loadData() {
		voltsField->setText(QString::number(model->getVoltage(), 'f', 1));
	}

	bool saveData() {
		model->setVoltage(voltsField->text().toDouble());
		return true;
	}

private:
	QSharedPointer<HighAlarmProgramValue> model;
	QLineEdit *voltsField;
};

class RelayProgramPane : public ProgramPane {
	Q_OBJECT

public:
	GET_SET_DATA_MACRO(RelayProgramValue)
	RelayProgramPane(const QString &title, QWidget *parent = 0) : ProgramPane(title, parent) {
		QGridLayout *layout = new QGridLayout(this);

		QGroupBox *reverseBox = new QGroupBox("Relay Operation");
		QHBoxLayout *reverseLayout = new QHBoxLayout(reverseBox);
		normalButton = new QRadioButton("Normal");
		reverseButton = new QRadioButton("Reverse");
		normalButton->setChecked(true);
		reverseLayout->addWidget(normalButton);
		reverseLayout->addWidget(reverseButton);
		layout->addWidget(reverseBox, 1, 1, 1, 2);

		lowBox = new QGroupBox("Turn ON when:");
		QFormLayout *lowLayout = new QFormLayout(lowBox);
		lowVoltsBox = new QLineEdit();
		QValidator *lowVoltsValidator = new QDoubleValidator(0, 102.3, 1, this);
		lowVoltsBox->setValidator(lowVoltsValidator);
		lowLayout->addRow("Volts <=", lowVoltsBox);
		lowLayout->addRow(new QLabel("OR"));
		lowPercentBox = new QLineEdit();
		QValidator *lowPercentValidator = new QIntValidator(1, 99, this);
		lowPercentBox->setValidator(lowPercentValidator);
		lowLayout->addRow("Percent Full <=", lowPercentBox);

		lowUsePercent = new QRadioButton("Use Percent Full");
		lowUsePercent->setChecked(true);
		lowLayout->addRow(lowUsePercent);
		lowDontUsePercent = new QRadioButton("Don't use Percent Full");
		lowLayout->addRow(lowDontUsePercent);
		layout->addWidget(lowBox, 2, 1, 1, 1);

		highBox = new QGroupBox("Turn OFF when:");
		QFormLayout *highLayout = new QFormLayout(highBox);
		highVoltsBox = new QLineEdit();
		QValidator *highVoltsValidator = new QDoubleValidator(0, 102.3, 1, this);
		highVoltsBox->setValidator(highVoltsValidator);
		highLayout->addRow("Volts >=", highVoltsBox);
		highLayout->addRow(new QLabel("OR"));
		highPercentBox = new QLineEdit();
		QValidator *highPercentValidator = new QIntValidator(1, 99, this);
		highPercentBox->setValidator(highPercentValidator);
		highLayout->addRow("Percent Full >=", highPercentBox);

		highUsePercent = new QRadioButton("Use Percent Full");
		highUsePercent->setChecked(true);
		highLayout->addRow(highUsePercent);
		highDontUsePercent = new QRadioButton("Don't use Percent Full");
		highLayout->addRow(highDontUsePercent);
		highUseChargeCriteria = new QRadioButton("Use Charge Criteria (P32)");
		highLayout->addRow(highUseChargeCriteria);
		layout->addWidget(highBox, 2, 2, 1, 1);

		layout->addItem(new QSpacerItem(0, 0), 3, 1, 1, 2);
		layout->setRowStretch(3, 1);

		connect(reverseButton, SIGNAL(toggled(bool)), this, SLOT(setReversed(bool)));
		connect(lowUsePercent, SIGNAL(toggled(bool)), this, SLOT(setLowUsePercent(bool)));
		connect(highUsePercent, SIGNAL(toggled(bool)), this, SLOT(setHighUsePercent(bool)));
	}

	bool isDirty() {
		if(model == NULL)
			return false;
		if(!isValid())
			return true;

		if(reverseButton->isChecked() != model->isReversed())
			return true;
		if(lowVoltsBox->text() != QString::number(model->getVolts(false), 'f', 1))
			return true;

		int lowPercent = model->getPercent(false);
		if(lowPercent == 0 && !lowDontUsePercent->isChecked())
			return true;
		if(lowPercent != 0 && lowPercentBox->text() != QString::number(lowPercent))
			return true;

		if(highVoltsBox->text() != QString::number(model->getVolts(true), 'f', 1))
			return true;		

		int highPercent = model->getPercent(true);
		if(highPercent == 0 && !highDontUsePercent->isChecked())
			return true;
		if(highPercent == 100 && !highUseChargeCriteria->isChecked())
			return true;
		if(highPercent != 0 && highPercent != 100 && highPercentBox->text() != QString::number(highPercent))
			return true;

		return false;
	}

	bool isValid() {
		if(!lowVoltsBox->hasAcceptableInput())
			return false;
		if(lowUsePercent->isChecked() && !lowPercentBox->hasAcceptableInput())
			return false;
		if(!highVoltsBox->hasAcceptableInput())
			return false;
		if(highUsePercent->isChecked() && !highPercentBox->hasAcceptableInput())
			return false;
		if(highVoltsBox->text().toDouble() <= lowVoltsBox->text().toDouble())
			return false;
		if(lowUsePercent->isChecked() && highUsePercent->isChecked() && highPercentBox->text().toInt() <= lowPercentBox->text().toInt())
			return false;
		return true;
	}

	void handleInvalid() {
		QPointer<QMessageBox> errorMessage = new QMessageBox(this);
		errorMessage->setText("Voltages must be between 0 and 102.3, percentages must be between 1 and 99, and voltages/percentages on the left must be lower than those on the right");
		errorMessage->setStandardButtons(QMessageBox::Ok);
		errorMessage->setWindowModality(Qt::WindowModal);
		errorMessage->exec();
	}

	QString helpText();

public slots:
	void loadData() {
		setReversed(model->isReversed());

		lowVoltsBox->setText(QString::number(model->getVolts(false), 'f', 1));

		int lowPercent = model->getPercent(false);
		if(lowPercent == 0) {
			lowPercentBox->setEnabled(false);
			lowPercentBox->setText("");
			lowDontUsePercent->setChecked(true);
		} else {
			lowPercentBox->setEnabled(true);
			lowPercentBox->setText(QString::number(lowPercent));
			lowUsePercent->setChecked(true);
		}

		highVoltsBox->setText(QString::number(model->getVolts(true), 'f', 1));

		int highPercent = model->getPercent(true);
		if(highPercent == 0) {
			highPercentBox->setEnabled(false);
			lowPercentBox->setText("");
			lowDontUsePercent->setChecked(true);
		} else if(highPercent == 100) {
			highPercentBox->setEnabled(false);
			highPercentBox->setText("");
			highUseChargeCriteria->setChecked(true);
		} else {
			highPercentBox->setEnabled(true);
			highPercentBox->setText(QString::number(highPercent));
			highUsePercent->setChecked(true);
		}
	}

	bool saveData() {
		model->setVolts(lowVoltsBox->text().toDouble(), highVoltsBox->text().toDouble(), reverseButton->isChecked());
		if(lowUsePercent->isChecked()) {
			model->setPercent(false, lowPercentBox->text().toInt());
		} else {
			model->setPercent(false, 0);
		}

		if(highUsePercent->isChecked()) {
			model->setPercent(true, highPercentBox->text().toInt());
		} else if (highDontUsePercent->isChecked()) {
			model->setPercent(true, 0);
		} else {
			model->setPercent(true, 100);
		}
		return true;
	}

	void setReversed(bool reversed) {
		if(reversed) {
			reverseButton->setChecked(true);
			lowBox->setTitle("Turn OFF when:");
			highBox->setTitle("Turn ON when:");
		} else {
			normalButton->setChecked(true);
			lowBox->setTitle("Turn ON when:");
			highBox->setTitle("Turn OFF when:");
		}
	}

	void setLowUsePercent(bool use) {
		if(use) {
			lowPercentBox->setEnabled(true);
		} else {
			lowPercentBox->setEnabled(false);
			lowPercentBox->setText("");
		}
	}

	void setHighUsePercent(bool use) {
		if(use) {
			highPercentBox->setEnabled(true);
		} else {
			highPercentBox->setEnabled(false);
			highPercentBox->setText("");
		}
	}

private:
	QSharedPointer<RelayProgramValue> model;

	QRadioButton *normalButton, *reverseButton;
	QGroupBox *lowBox;
	QLineEdit *lowVoltsBox, *lowPercentBox;
	QRadioButton *lowUsePercent, *lowDontUsePercent;
	QGroupBox *highBox;
	QLineEdit *highVoltsBox, *highPercentBox;
	QRadioButton *highUsePercent, *highDontUsePercent, *highUseChargeCriteria;
};

class ChargedCriteriaProgramPane : public ProgramPane {
	Q_OBJECT

public:
	GET_SET_DATA_MACRO(ChargedProgramValue)
	ChargedCriteriaProgramPane(const QString &title, int batteryNum, QWidget *parent = 0) : ProgramPane(title, parent) {
		setBatteryNum(batteryNum);

		QFormLayout *layout = new QFormLayout(this);

		voltsField = new QLineEdit;
		QValidator *validator = new QDoubleValidator(0, 102.3, 1, this);
		voltsField->setValidator(validator);
		layout->addRow("Volts greater than", voltsField);

		ampsField = new QLineEdit;
		validator = new QIntValidator(1, 100, this);
		ampsField->setValidator(validator);
		layout->addRow("AND current less than", ampsField);

		ampsEnabled = new QCheckBox();
		layout->addRow("Use current", ampsEnabled);

		connect(ampsEnabled, SIGNAL(stateChanged(int)), this, SLOT(controlsChanged()));
	}

	bool isDirty() {
		if(model == NULL)
			return false;
		if(!isValid())
			return true;
		return voltsField->text() != QString::number(model->getVoltage(), 'f', 1) || ampsField->text().toInt() != model->getCurrent();
	}

	bool isValid() {
		return (!ampsEnabled->isChecked() || ampsField->hasAcceptableInput()) && voltsField->hasAcceptableInput();
	}

	void handleInvalid() {
		QPointer<QMessageBox> errorMessage = new QMessageBox(this);
		errorMessage->setText("Volts must be between 0 and 102.3, and amps must be between 1 and 100 (when enabled)");
		errorMessage->setStandardButtons(QMessageBox::Ok);
		errorMessage->setWindowModality(Qt::WindowModal);
		errorMessage->exec();	
	}

	QString helpText();

public slots:
	void loadData() {
		voltsField->setText(QString::number(model->getVoltage(), 'f', 1));

		ampsField->setText(QString::number(model->getCurrent()));

		ampsEnabled->setChecked(model->getCurrent() != 0);
	}

	bool saveData() {
		model->setVoltage(voltsField->text().toDouble());
		model->setCurrent(ampsField->text().toInt());
		return true;
	}

	void controlsChanged() {
		bool amps = ampsEnabled->isChecked();
		ampsField->setEnabled(amps);
		if(!amps) {
			ampsField->setText("0");
		}
	}
private:
	QSharedPointer<ChargedProgramValue> model;
	QLineEdit *voltsField;
	QLineEdit *ampsField;
	QCheckBox *ampsEnabled;
};

class EfficiencyProgramPane : public ProgramPane {
	Q_OBJECT

public:
	GET_SET_DATA_MACRO(EfficiencyProgramValue)
	EfficiencyProgramPane(const QString &title, QWidget *parent = 0) : ProgramPane(title, parent) {
		QFormLayout *layout = new QFormLayout(this);

		percentField = new QLineEdit;
		QValidator *validator = new QIntValidator(60, 100, this);
		percentField->setValidator(validator);
		layout->addRow("Efficiency Factor (%)", percentField);

		ampsField = new QLineEdit;
		validator = new QDoubleValidator(0, 9.99, 2, this);
		ampsField->setValidator(validator);
		layout->addRow("Self discharge (Amps)", ampsField);
	}

	bool isDirty() {
		if(model == NULL)
			return false;
		if(!isValid())
			return true;
		return percentField->text().toInt() != model->getEfficiency() || ampsField->text() != QString::number(model->getSelfDischarge(), 'f', 2);
	}

	bool isValid() {
		return percentField->hasAcceptableInput() && ampsField->hasAcceptableInput();
	}

	void handleInvalid() {
		QPointer<QMessageBox> errorMessage = new QMessageBox(this);
		errorMessage->setText("Efficiency must be between 60% and 100%, and self discharge must be between 0 and 9.99");
		errorMessage->setStandardButtons(QMessageBox::Ok);
		errorMessage->setWindowModality(Qt::WindowModal);
		errorMessage->exec();	
	}

	QString helpText();

public slots:
	void loadData() {
		percentField->setText(QString::number(model->getEfficiency()));

		ampsField->setText(QString::number(model->getSelfDischarge(), 'f', 2));
	}

	bool saveData() {
		model->setEfficiency(percentField->text().toInt());
		model->setSelfDischarge(ampsField->text().toDouble());
		return true;
	}

private:
	QSharedPointer<EfficiencyProgramValue> model;
	QLineEdit *percentField;
	QLineEdit *ampsField;
};

class ChargeTimeProgramPane : public ProgramPane {
	Q_OBJECT

public:
	GET_SET_DATA_MACRO(ChargeTimeProgramValue)
	ChargeTimeProgramPane(const QString &title, const QString & type, QWidget *parent = 0) : ProgramPane(title, parent) {
		QFormLayout *layout = new QFormLayout(this);

		timeType = type;
		daysField = new QLineEdit;
		QValidator *validator = new QIntValidator(0, 255, this);
		daysField->setValidator(validator);
		layout->addRow(QString("Days before %1 reminder").arg(type), daysField);
	}

	bool isDirty() {
		if(model == NULL)
			return false;
		if(!isValid())
			return true;
		return daysField->text().toInt() != model->getDays();
	}

	bool isValid() {
		return daysField->hasAcceptableInput();
	}

	void handleInvalid() {
		QPointer<QMessageBox> errorMessage = new QMessageBox(this);
		errorMessage->setText("Days must be between 0 and 255");
		errorMessage->setStandardButtons(QMessageBox::Ok);
		errorMessage->setWindowModality(Qt::WindowModal);
		errorMessage->exec();	
	}

	QString helpText();

public slots:
	void loadData() {
		daysField->setText(QString::number(model->getDays()));
	}

	bool saveData() {
		model->setDays(daysField->text().toInt());
		return true;
	}

private:
	QSharedPointer<ChargeTimeProgramValue> model;
	QLineEdit *daysField;
	QString timeType;
};

// This class exists to allow the text to be set automatically
class EqualizeTimeProgramPane : public ChargeTimeProgramPane {
public:
	EqualizeTimeProgramPane(const QString &title, const QString & type, QWidget *parent = 0) : ChargeTimeProgramPane(title, type, parent) {}

	QString helpText();
};

class TimeProgramPane : public ProgramPane {
	Q_OBJECT

public:
	GET_SET_DATA_MACRO(TimeProgramValue)
	TimeProgramPane(const QString &title, QWidget *parent = 0) : ProgramPane(title, parent) {
		QFormLayout *layout = new QFormLayout(this);

		timeField = new QTimeEdit();
		layout->addRow("Time", timeField);

		QPushButton *currentTime = new QPushButton("Set to PC Time");
		layout->addRow("", currentTime);
		connect(currentTime, SIGNAL(clicked()), this, SLOT(resetTime()));

		dayField = new QLineEdit;
		QValidator *validator = new QIntValidator(0, 8191, this);
		dayField->setValidator(validator);
		layout->addRow("Day", dayField);
	}

	bool isDirty() {
		if(model == NULL)
			return false;
		if(!isValid())
			return true;
		return dayField->text().toInt() != model->getDay() || timeField->time() != QTime(model->getHour(), model->getMinute());
	}

	bool isValid() {
		return dayField->hasAcceptableInput();
	}

	void handleInvalid() {
		QPointer<QMessageBox> errorMessage = new QMessageBox(this);
		errorMessage->setText("Day must be between 0 and 8191");
		errorMessage->setStandardButtons(QMessageBox::Ok);
		errorMessage->setWindowModality(Qt::WindowModal);
		errorMessage->exec();
	}

	QString helpText();

public slots:
	void loadData() {
		timeField->setTime(QTime(model->getHour(), model->getMinute()));
		dayField->setText(QString::number(model->getDay()));
	}

	bool saveData() {
		QTime time = timeField->time();
		model->setHour(time.hour());
		model->setMinute(time.minute());
		model->setDay(dayField->text().toInt());
		return true;
	}

	void resetTime() {
		timeField->setTime(QTime::currentTime());
	}

private:
	QSharedPointer<TimeProgramValue> model;
	QTimeEdit *timeField;
	QLineEdit *dayField;
};

class PeriodicProgramPane : public ProgramPane {
	Q_OBJECT

public:
	GET_SET_DATA_MACRO(PeriodicProgramValue)
	PeriodicProgramPane(const QString &title, QWidget *parent = 0) : ProgramPane(title, parent) {
		QFormLayout *layout = new QFormLayout(this);

		dataList = new QListWidget();
		dataList->setMinimumHeight(250);
		layout->addRow(dataList);

		QList<QPair<int, QString> > loggingChoices;
		loggingChoices <<
		qMakePair(PM_PERIODIC_AHR1, QString("AHr 1")) <<
		qMakePair(PM_PERIODIC_AHR2, QString("AHr 2")) <<
		qMakePair(PM_PERIODIC_AHR3, QString("AHr 3")) <<
		qMakePair(PM_PERIODIC_WHR1, QString("WHr 1")) <<
		qMakePair(PM_PERIODIC_WHR2, QString("WHr 2")) <<
		qMakePair(PM_PERIODIC_VOLTS1, QString("Filtered Volts 1")) <<
		qMakePair(PM_PERIODIC_VOLTS2, QString("Filtered Volts 2")) <<
		qMakePair(PM_PERIODIC_AMPS1, QString("Amps 1"))  <<
		qMakePair(PM_PERIODIC_PERCENTFULL, QString("Percent Full (both batteries)")) <<
		qMakePair(PM_PERIODIC_TEMP, QString("Temp (min/max)"));
		QPair<int, QString> currChoice;
		foreach(currChoice, loggingChoices) {
			QListWidgetItem *item = new QListWidgetItem(currChoice.second);
			item->setData(Qt::UserRole, currChoice.first);
			dataList->addItem(item);
		}

		logInterval = new QComboBox();
		logInterval->addItem("1 min", 1);
		logInterval->addItem("2 min", 2);
		logInterval->addItem("3 min", 3);
		logInterval->addItem("5 min", 5);
		logInterval->addItem("10 min", 10);
		logInterval->addItem("15 min", 15);
		logInterval->addItem("30 min", 30);
		logInterval->addItem("1 hr", 60);
		logInterval->addItem("2 hr", 120);
		logInterval->addItem("4 hr", 240);
		logInterval->addItem("6 hr", 360);
		logInterval->addItem("8 hr", 480);
		logInterval->addItem("12 hr", 720);
		logInterval->addItem("24 hr", 1440);
		logInterval->setCurrentIndex(0);
		layout->addRow("Record every", logInterval);

		logTimeField = new QTimeEdit();
		layout->addRow("First measurement at", logTimeField);

		recordLengthLabel = new QLabel();
		layout->addRow("Data capacity:", recordLengthLabel);

		connect(dataList, SIGNAL(itemChanged(QListWidgetItem *)), this, SLOT(updateCapacityLabel()));
		connect(logInterval, SIGNAL(currentIndexChanged(int)), this, SLOT(updateCapacityLabel()));
	}

	bool isDirty() {
		if(model == NULL)
			return false;

		if(getEnabledItems() != model->getEnabled())
			return true;

		if(logInterval->itemData(logInterval->currentIndex()).toInt() != model->getIntervalMinutes())
			return true;

		QTime time = logTimeField->time();
		if(time.hour() != model->getStartHour())
			return true;
		if(time.minute() != model->getStartMinute())
			return true;

		return false;
	}

public slots:
	void loadData() {
		// Avoid lots of extra calculations
		disconnect(dataList, SIGNAL(itemChanged(QListWidgetItem *)), this, SLOT(updateCapacityLabel()));
		disconnect(logInterval, SIGNAL(currentIndexChanged(int)), this, SLOT(updateCapacityLabel()));

		int enabledItems = model->getEnabled();
		for(int i = 0; i < dataList->count(); i++) {
			QListWidgetItem *item = dataList->item(i);
			item->setCheckState(enabledItems & item->data(Qt::UserRole).toInt() ? Qt::Checked : Qt::Unchecked);
		}

		int intervalMinutes = model->getIntervalMinutes();
		int intervalIndex = logInterval->findData(intervalMinutes);
		if(intervalIndex < 0) {
			intervalIndex = logInterval->count();
			logInterval->addItem(QString("%1 min").arg(intervalMinutes), intervalMinutes);
		}
		logInterval->setCurrentIndex(intervalIndex);

		logTimeField->setTime(QTime(model->getStartHour(), model->getStartMinute()));

		updateCapacityLabel();
		connect(dataList, SIGNAL(itemChanged(QListWidgetItem *)), this, SLOT(updateCapacityLabel()));
		connect(logInterval, SIGNAL(currentIndexChanged(int)), this, SLOT(updateCapacityLabel()));
	}

	bool saveData() {
		model->setEnabled(getEnabledItems());

		model->setIntervalMinutes(logInterval->itemData(logInterval->currentIndex()).toInt());

		QTime time = logTimeField->time();
		model->setStartHour(time.hour());
		model->setStartMinute(time.minute());

		return true;
	}

	void updateCapacityLabel() {
		int numEnabled;
		getEnabledItems(&numEnabled);

		int numRecords = (61 / (3 + 2 * numEnabled)) * 116;
		// This will always divide cleanly
		int recordsPerDay = 1440 / logInterval->itemData(logInterval->currentIndex()).toInt();
		double days = (double) numRecords / recordsPerDay;

		recordLengthLabel->setText(QString("%1 days").arg(QString::number(days, 'f', 1)));
	}

	QString helpText();

private:
	int getEnabledItems(int *numEnabled = NULL) {
		if(numEnabled)
			*numEnabled = 0;
		int flags = 0;
		for(int i = 0; i < dataList->count(); i++) {
			QListWidgetItem *item = dataList->item(i);
			if(item->checkState() == Qt::Checked) {
				flags |= dataList->item(i)->data(Qt::UserRole).toInt();
				if(numEnabled)
					(*numEnabled)++;
			}
		}
		return flags;
	}

	QSharedPointer<PeriodicProgramValue> model;

	QListWidget *dataList;
	QComboBox *logInterval;
	QTimeEdit *logTimeField;
	QLabel *recordLengthLabel;
};

class ProfileProgramPane : public ProgramPane {
	Q_OBJECT

public:
	GET_SET_DATA_MACRO(ProfileProgramValue)
	ProfileProgramPane(const QString &title, QWidget *parent = 0) : ProgramPane(title, parent) {
		QFormLayout *layout = new QFormLayout(this);

		QGroupBox *intervalBox = new QGroupBox();
		QVBoxLayout *intervalLayout = new QVBoxLayout(intervalBox);
		fivePercent = new QRadioButton("5% change");
		tenPercent = new QRadioButton("10% change");
		tenPercent->setChecked(true);
		intervalLayout->addWidget(fivePercent);
		intervalLayout->addWidget(tenPercent);
		layout->addRow("Record interval", intervalBox);

		battery1 = new QCheckBox();
		battery2 = new QCheckBox();
		layout->addRow("Record battery 1 data", battery1);
		layout->addRow("Record battery 2 data", battery2);
	}

	bool isDirty() {
		if(model == NULL)
			return false;
		return fivePercent->isChecked() != model->isFivePercentInterval() ||
				battery1->isChecked() != model->isBatteryOn(1) ||
				battery2->isChecked() != model->isBatteryOn(2);
	}

	QString helpText();

public slots:
	void loadData() {
		if(model->isFivePercentInterval())
			fivePercent->setChecked(true);
		else
			tenPercent->setChecked(true);

		battery1->setChecked(model->isBatteryOn(1));
		battery2->setChecked(model->isBatteryOn(2));
	}

	bool saveData() {
		model->setFivePercentInterval(fivePercent->isChecked());
		model->setBatteryOn(1, battery1->isChecked());
		model->setBatteryOn(2, battery2->isChecked());
		return true;
	}

private:
	QSharedPointer<ProfileProgramValue> model;
	QRadioButton *fivePercent;
	QRadioButton *tenPercent;
	QCheckBox *battery1;
	QCheckBox *battery2;
};

class ResetProgramPane : public ProgramPane {
	Q_OBJECT

public:
	QSharedPointer<ProgramValue> getData() { return QSharedPointer<ProgramValue>(); } // Dummy value
	void setData(QSharedPointer<ProgramValue> value) { Q_UNUSED(value); }
	ResetProgramPane(const QString &title, SiteManager *manager, QWidget *parent = 0) : ProgramPane(title, parent) {
		this->manager = manager;
		connect(manager, SIGNAL(resetComplete(enum PMResetType)), this, SLOT(resetComplete(enum PMResetType)));

		QFormLayout *layout = new QFormLayout(this);

		periodicData = new QPushButton("Clear periodic logged data");
		fivePercentData = new QPushButton("Clear discharge profile logged data");
		battery1CycleData = new QPushButton("Clear Battery 1 cycle data");
		battery2CycleData = new QPushButton("Clear Battery 2 cycle data");
		initializePrograms = new QPushButton("Reset all programs to defaults");

		layout->addRow(periodicData);
		layout->addRow(fivePercentData);
		layout->addRow(battery1CycleData);
		layout->addRow(battery2CycleData);
		layout->addRow(initializePrograms);

		connect(periodicData, SIGNAL(clicked()), this, SLOT(buttonClicked()));
		connect(fivePercentData, SIGNAL(clicked()), this, SLOT(buttonClicked()));
		connect(battery1CycleData, SIGNAL(clicked()), this, SLOT(buttonClicked()));
		connect(battery2CycleData, SIGNAL(clicked()), this, SLOT(buttonClicked()));
		connect(initializePrograms, SIGNAL(clicked()), this, SLOT(buttonClicked()));

		status = new QLabel("Ready for reset");

		layout->addRow("Status:", status);
	}

	bool isDirty() {
		return false;
	}

	bool needsRead() {
		return false;
	}

	QString helpText();

public slots:
	void loadData() { }

	bool saveData() { return true; }

	void buttonClicked() {
		if(resets.empty())
			status->setText("Ready for reset");

		QPushButton *button = static_cast<QPushButton *>(sender());

		QPointer<QMessageBox> confirmation = new QMessageBox(this);
		confirmation->setText(QString("%1 ?").arg(button->text()));
		confirmation->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
		confirmation->setWindowModality(Qt::WindowModal);
		if(confirmation->exec() != QMessageBox::Yes) {
			delete confirmation;
			return;
		}
		delete confirmation;

		enum PMResetType reset;
		if(button == periodicData) {
			reset = PM_RESET_PERIODIC;
		} else if(button == fivePercentData) {
			reset = PM_RESET_DISCHARGE;
		} else if(button == battery1CycleData) {
			reset = PM_RESET_BAT1_EFF;
		} else if(button == battery2CycleData) {
			reset = PM_RESET_BAT2_EFF;
		} else if(button == initializePrograms) {
			reset = PM_RESET_PROGRAM;
		} else {
			return;
		}

		if(resets.contains(reset))
			return;

		resets.insert(reset);
		manager->resetDirect(reset);
		status->setText("Resetting...");
	}

	void resetComplete(enum PMResetType reset) {
		if(!resets.contains(reset))
			return;

		resets.remove(reset);
		if(resets.empty()) {
			status->setText("Success!");
		}
	}

private:
	SiteManager *manager;
	QPushButton *periodicData;
	QPushButton *fivePercentData;
	QPushButton *battery1CycleData;
	QPushButton *battery2CycleData;
	QPushButton *initializePrograms;
	QLabel *status;

	QSet<enum PMResetType> resets;
};

class InternetProgramPane : public ProgramPane {
	Q_OBJECT

public:
	GET_SET_DATA_MACRO(InternetProgramValue)
	InternetProgramPane(const QString &title, QWidget *parent = 0) : ProgramPane(title, parent) {
		QFormLayout *layout = new QFormLayout(this);

		dhcpBox = new QCheckBox();
		layout->addRow("Use DHCP for configuration", dhcpBox);

		manualGroupBox = new QGroupBox("Manual configuration");
		QFormLayout *manualLayout = new QFormLayout(manualGroupBox);
		ipBox = createIPBox();
		manualLayout->addRow("IP Address", ipBox);
		gatewayBox = createIPBox();
		manualLayout->addRow("Gateway", gatewayBox);
		subnetMaskBox = createIPBox();
		manualLayout->addRow("Subnet Mask", subnetMaskBox);
		layout->addRow(manualGroupBox);

		netBIOSBox = new QLineEdit();
		QRegExp netBIOSExp("[a-zA-Z0-9\\!\\@\\#\\$\\%\\^\\&\\(\\)\\-\\_\\'\\{\\}\\.\\~]{1,15}");
		QValidator *netBIOSValidator = new QRegExpValidator(netBIOSExp, this);
		netBIOSBox->setValidator(netBIOSValidator);
		layout->addRow("NetBIOS Name", netBIOSBox);

		portBox = new QLineEdit();
		QValidator *portValidator = new QIntValidator(0, 65535, this);
		portBox->setValidator(portValidator);
		layout->addRow("Listen on port", portBox);

		QPushButton *resetButton = new QPushButton("Reset to defaults");
		layout->addRow(resetButton);

		connect(dhcpBox, SIGNAL(toggled(bool)), this, SLOT(DHCPStateChanged(bool)));
		connect(resetButton, SIGNAL(clicked()), this, SLOT(resetButtonClicked()));
	}

	bool isDirty() {
		if(model == NULL)
			return false;
		if(!isValid())
			return true;

		if(dhcpBox->isChecked() != model->isDhcpEnabled())
			return true;

		if(ipBox->text() != model->getIPAddress())
			return true;
		if(gatewayBox->text() != model->getGatewayAddress())
			return true;
		if(subnetMaskBox->text() != model->getSubnetMask())
			return true;
		if(netBIOSBox->text() != model->getNetBIOSName())
			return true;
		if(portBox->text() != QString::number(model->getListenPort()))
			return true;

		return false;
	}

	bool isValid() {
		if(model == NULL)
			return false;

		return model->isSubnetMaskValid(subnetMaskBox->text());
	}

	void handleInvalid() {
		QPointer<QMessageBox> errorMessage = new QMessageBox(this);
		errorMessage->setText("Ensure that the subnet mask is valid");
		errorMessage->setStandardButtons(QMessageBox::Ok);
		errorMessage->setWindowModality(Qt::WindowModal);
		errorMessage->exec();
	}

	QString helpText();

public slots:
	void loadData() {
		dhcpBox->setChecked(model->isDhcpEnabled());
		ipBox->setText(model->getIPAddress());
		gatewayBox->setText(model->getGatewayAddress());
		subnetMaskBox->setText(model->getSubnetMask());

		netBIOSBox->setText(model->getNetBIOSName());
		portBox->setText(QString::number(model->getListenPort()));
	}

	bool saveData() {
		model->setDhcpEnabled(dhcpBox->isChecked());
		model->setIPAddress(ipBox->text());
		model->setGatewayAddress(gatewayBox->text());
		model->setSubnetMask(subnetMaskBox->text());

		model->setNetBIOSName(netBIOSBox->text());
		model->setListenPort(portBox->text().toInt());
		return true;
	}

	void DHCPStateChanged(bool enabled) {
		manualGroupBox->setEnabled(!enabled);
	}

	void resetButtonClicked() {
		dhcpBox->setChecked(true);
		ipBox->setText("0.0.0.0");
		gatewayBox->setText("0.0.0.0");
		subnetMaskBox->setText("255.255.0.0");
		netBIOSBox->setText("PENTAMETRIC1");
		portBox->setText("1701");
	}

private:
	QLineEdit *createIPBox() {
		QLineEdit *box = new QLineEdit();
		QValidator *validator = new IPv4Validator(this);
		box->setValidator(validator);
		return box;
	}

	QSharedPointer<InternetProgramValue> model;

	QCheckBox *dhcpBox;
	QGroupBox *manualGroupBox;
	QLineEdit *ipBox, *gatewayBox, *subnetMaskBox;
	QLineEdit *netBIOSBox;
	QLineEdit *portBox;
};

#endif