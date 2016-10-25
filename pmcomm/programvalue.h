#ifndef PROGRAMVALUE_H
#define PROGRAMVALUE_H

#include "pmdefs.h"

#include <QStringList>
#include <QtAlgorithms>
#include <QVariant>

// The location of types for QVariant changed between Qt4 and Qt5
#if QT_VERSION >= 0x050000
#include <QMetaType>
#define QVARIANT_LIST QMetaType::QVariantList
#else
#define QVARIANT_LIST QVariant::List
#endif

/* This class represents data corresponding to a single PentaMetric program mode, and
   provides a wrapper for the less user-friendly union PMProgramData. Each type of program
   value has its own subclass. These are decleared below

   More than one union PMProgramData may be part of the same ProgramValue. This is done
   to group related pieces of data (such as all of the settings for the periodic logged
   data)
 */
class ProgramValue {
public:
	// Constructs and returns the appropriate ProgramValue subclass
	static ProgramValue *makeProgramValue(enum PMProgramNumber prog, union PMProgramData &val);

	// Same as above, but starting from QVariant
	static ProgramValue *makeProgramValue(enum PMProgramNumber prog, const QVariant & value);

	// Returns a copy of the ProgramValue
	virtual ProgramValue *clone() = 0;

	// Convert back and forth between names and numbers
	static QString getName(enum PMProgramNumber prog);
	static enum PMProgramNumber getNumber(QString & name);

	virtual ~ProgramValue() {}

	// Extracts the raw union PMProgramData, but only for the first (default) block of data
	void toRawValue(union PMProgramData &val) { val = data; }
	// Extracts the raw union PMProgramData for any block of data
	void toRawValue(union PMProgramData &val, enum PMProgramNumber curr) { if(curr == prog) toRawValue(val); else toRawExtended(curr, val); }

	// Indicates if next is the last block of data, and if not, updates next to be the PMProgramNumber of the next block
	virtual bool needsMore(enum PMProgramNumber & next) { Q_UNUSED(next); return false; }

	// Adds in later blocks of data, of the type specified by curr. Returns true on success, false on failure
	virtual bool updateValue(enum PMProgramNumber curr, union PMProgramData & val) { Q_UNUSED(curr); Q_UNUSED(val); return false; }

	// Retrieves later blocks of data. The two-arg version of toRawValue() does this as well, so this shouldn't need to be
	// called manually
	virtual void toRawExtended(enum PMProgramNumber curr, union PMProgramData & val) { Q_UNUSED(curr); Q_UNUSED(val); Q_ASSERT(0); }

	enum PMProgramNumber progNum() { return prog; }

	// Converts this to a QVariant, currently just by storing the binary data
	virtual QVariant toVariant() = 0;

protected:
	// These shouldn't be constructed directly; call makeProgramValue() instead
	ProgramValue(enum PMProgramNumber prog, union PMProgramData & val) : prog(prog), data(val) { valid = true; }
	ProgramValue(enum PMProgramNumber prog) : prog(prog) { valid = false; }

	// Represent the corresponding program number (for the first block), and the associated data
	enum PMProgramNumber prog;
	union PMProgramData data;

	static ProgramValue *privMakeProgFromVariant(enum PMProgramNumber prog, const QVariant & val);

	// Used by the second form of makeProgramValue() to keep track of whether the constructor succeeded
	bool valid;
};

class SwitchSelectProgramValue : public ProgramValue {
public:
	SwitchSelectProgramValue(enum PMProgramNumber prog, union PMProgramData &val) : ProgramValue(prog, val) {}
	ProgramValue *clone() { return new SwitchSelectProgramValue(*this); }
	SwitchSelectProgramValue(enum PMProgramNumber prog, const QVariant & val) : ProgramValue(prog) {
		if(!val.canConvert(QVARIANT_LIST))
			return;
		QList<QVariant> v = val.toList();
		if(v.size() > 5)
			return;

		memset(data.switchDisplay, 0, sizeof(data.switchDisplay));
		for(int i = 0; i < v.size(); i++) {
			bool ok;
			data.switchDisplay[i] = (enum PMDisplayNumber) v[i].toInt(&ok); // Note that the actual values aren't checked
			if(!ok)
				return;
		}

		valid = true;
	}
	QVariant toVariant() {
		QList<QVariant> v;
		for(int i = 0; i < 5; i++) {
			if(data.switchDisplay[i] == PM_DINVALID)
				break;
			v.append((int) data.switchDisplay[i]);
		}

		return v;
	}

	void getDisplays(QList<enum PMDisplayNumber> & displays) {
		for(int i = 0; i < 5; i++) {
			if(data.switchDisplay[i] == PM_DINVALID) // Stop at the first zero
				break;

			displays.append(data.switchDisplay[i]);
		}
	}

	void setDisplays(const QList<enum PMDisplayNumber> & displays) {
		for(int i = 0; i < 5; i++) {
			if(i < displays.size())
				data.switchDisplay[i] = displays[i];
			else
				data.switchDisplay[i] = PM_DINVALID;
		}		
	}
};

class LabelsProgramValue : public ProgramValue {
public:
	LabelsProgramValue(enum PMProgramNumber prog, union PMProgramData &val) : ProgramValue(prog, val) {}
	ProgramValue *clone() { return new LabelsProgramValue(*this); }
	LabelsProgramValue(enum PMProgramNumber prog, const QVariant & val) : ProgramValue(prog) {
		if(!val.canConvert(QVARIANT_LIST))
			return;
		QList<QVariant> v = val.toList();
		if(v.size() != 4)
			return;

		bool ok;
		data.labels.battLabel = (enum PMBatteryLabel) v[0].toInt(&ok);
		if(!ok)
			return;

		for(int i = 0; i < 3; i++) {
			data.labels.ampsLabels[i] = (enum PMAmpsLabel) v[i + 1].toInt(&ok); // Note that the actual values aren't checked
			if(!ok)
				return;
		}

		valid = true;
	}
	QVariant toVariant() {
		QList<QVariant> v;
		v.append((int) data.labels.battLabel);
		for(int i = 0; i < 3; i++) {
			v.append((int) data.labels.ampsLabels[i]);
		}

		return v;
	}

	void getLabels(PMLabels &l) {
		l = data.labels;
		if(l.ampsLabels[2] == PM_AMPSLABEL_BATTERY_1)
			l.ampsLabels[2] = PM_AMPSLABEL_BATTERY;
	}

	void setLabels(PMLabels &l) {
		data.labels = l;
	}
};

class ShuntTypesProgramValue : public ProgramValue {
public:
	ShuntTypesProgramValue(enum PMProgramNumber prog, union PMProgramData &val) : ProgramValue(prog, val) {}
	ProgramValue *clone() { return new ShuntTypesProgramValue(*this); }
	ShuntTypesProgramValue(enum PMProgramNumber prog, const QVariant & val) : ProgramValue(prog) {
		if(!val.canConvert(QVARIANT_LIST))
			return;
		QList<QVariant> v = val.toList();
		if(v.size() != 4)
			return;

		data.shunts.amps2Sum = v[0].toBool();

		for(int i = 0; i < 3; i++) {
			bool ok;
			data.shunts.types[i] = (enum PMShuntType) v[i + 1].toInt(&ok); // Note that the actual values aren't checked
			if(!ok)
				return;
		}

		valid = true;
	}
	QVariant toVariant() {
		QList<QVariant> v;
		v.append(data.shunts.amps2Sum);
		for(int i = 0; i < 3; i++) {
			v.append((int) data.shunts.types[i]);
		}

		return v;
	}

	enum PMShuntType shuntType(int channel) {
		return data.shunts.types[channel - 1];
	}

	bool isAmps2Sum() {
		return data.shunts.amps2Sum;
	}

	void setShuntType(int channel, enum PMShuntType type) {
		data.shunts.types[channel - 1] = type;
	}

	void setAmps2Sum(bool sum) {
		data.shunts.amps2Sum = sum;
	}
};

class VersionProgramValue : public ProgramValue {
public:
	VersionProgramValue(enum PMProgramNumber prog, union PMProgramData &val) : ProgramValue(prog, val) {}
	ProgramValue *clone() { return new VersionProgramValue(*this); }
	QVariant toVariant() {
		return data.version;
	}

	int versionMajor() {
		return data.version / 10;
	}

	int versionMinor() {
		return data.version % 10;
	}
};

class CapacityProgramValue : public ProgramValue {
public:
	CapacityProgramValue(enum PMProgramNumber prog, union PMProgramData &val) : ProgramValue(prog, val) {}
	ProgramValue *clone() { return new CapacityProgramValue(*this); }
	CapacityProgramValue(enum PMProgramNumber prog, const QVariant & val) : ProgramValue(prog) {
		bool ok;
		data.batCapacity = val.toInt(&ok); // Note that the actual values aren't checked
		if(!ok)
			return;

		valid = true;
	}
	QVariant toVariant() {
		return data.batCapacity;
	}

	bool batteryPresent() {
		return data.batCapacity != 0;
	}

	int getCapacity() {
		return data.batCapacity;
	}

	void setCapacity(int c) {
		data.batCapacity = c;
	}
};

class FilterTimeProgramValue : public ProgramValue {
public:
	FilterTimeProgramValue(enum PMProgramNumber prog, union PMProgramData &val) : ProgramValue(prog, val) {}
	ProgramValue *clone() { return new FilterTimeProgramValue(*this); }
	FilterTimeProgramValue(enum PMProgramNumber prog, const QVariant & val) : ProgramValue(prog) {
		bool ok;
		data.filterTime = (enum PMFilterTime) val.toInt(&ok); // Note that the actual values aren't checked
		if(!ok)
			return;

		valid = true;
	}
	QVariant toVariant() {
		return data.filterTime;
	}

	enum PMFilterTime filterTime() {
		return data.filterTime;
	}

	void setFilterTime(enum PMFilterTime ftime) {
		data.filterTime = ftime;
	}
};

class AlarmLevelProgramValue : public ProgramValue {
public:
	AlarmLevelProgramValue(enum PMProgramNumber prog, union PMProgramData &val) : ProgramValue(prog, val) {}
	ProgramValue *clone() { return new AlarmLevelProgramValue(*this); }
	AlarmLevelProgramValue(enum PMProgramNumber prog, const QVariant & val) : ProgramValue(prog) {
		if(!val.canConvert(QVARIANT_LIST))
			return;
		QList<QVariant> v = val.toList();
		if(v.size() != 5)
			return;

		for(int i = 0; i < 5; i++) {
			bool ok;
			data.alarmLevel[i] = (enum PMAlarmLevel) v[i].toInt(&ok); // Note that the actual values aren't checked
			if(!ok)
				return;
		}

		valid = true;
	}
	QVariant toVariant() {
		QList<QVariant> v;
		for(int i = 0; i < 5; i++) {
			v.append((int) data.alarmLevel[i]);
		}

		return v;
	}

	enum PMAlarmLevel getLevelForAlarm(enum PMAlarmIndex alarm) {
		return data.alarmLevel[alarm];
	}

	void setLevelForAlarm(enum PMAlarmIndex alarm, enum PMAlarmLevel level) {
		data.alarmLevel[alarm] = level;
	}
};

class LowAlarmProgramValue : public ProgramValue {
public:
	LowAlarmProgramValue(enum PMProgramNumber prog, union PMProgramData &val) : ProgramValue(prog, val) {}
	ProgramValue *clone() { return new LowAlarmProgramValue(*this); }
	LowAlarmProgramValue(enum PMProgramNumber prog, const QVariant & val) : ProgramValue(prog) {
		if(!val.canConvert(QVARIANT_LIST))
			return;
		QList<QVariant> v = val.toList();
		if(v.size() != 2)
			return;

		bool ok;
		data.lowAlarmSetpoint.voltage = v[0].toInt(&ok);
		if(!ok)
			return;
		data.lowAlarmSetpoint.percent = v[1].toInt(&ok);
		if(!ok)
			return;

		valid = true;
	}
	QVariant toVariant() {
		QList<QVariant> v;
		v.append(data.lowAlarmSetpoint.voltage);
		v.append(data.lowAlarmSetpoint.percent);

		return v;
	}

	double getVoltage() {
		return data.lowAlarmSetpoint.voltage / 10.0;
	}

	int getPercent() {
		return data.lowAlarmSetpoint.percent;
	}

	void setVoltage(double voltage) {
		data.lowAlarmSetpoint.voltage = voltage * 10.0 + 0.5;
	}

	void setPercent(int percent) {
		data.lowAlarmSetpoint.percent = percent;
	}
};

class HighAlarmProgramValue : public ProgramValue {
public:
	HighAlarmProgramValue(enum PMProgramNumber prog, union PMProgramData &val) : ProgramValue(prog, val) {}
	ProgramValue *clone() { return new HighAlarmProgramValue(*this); }
	HighAlarmProgramValue(enum PMProgramNumber prog, const QVariant & val) : ProgramValue(prog) {
		bool ok;
		data.highAlarmSetpoint = val.toInt(&ok); // Note that the actual values aren't checked
		if(!ok)
			return;

		valid = true;
	}
	QVariant toVariant() {
		return data.highAlarmSetpoint;
	}

	double getVoltage() {
		return data.highAlarmSetpoint / 10.0;
	}

	void setVoltage(double voltage) {
		data.highAlarmSetpoint = voltage * 10.0 + 0.05;
	}
};

class RelayProgramValue : public ProgramValue {
public:
	RelayProgramValue(enum PMProgramNumber prog, union PMProgramData &val) : ProgramValue(prog, val) {}
	ProgramValue *clone() { return new RelayProgramValue(*this); }
	RelayProgramValue(enum PMProgramNumber prog, const QVariant & val) : ProgramValue(prog) {
		if(!val.canConvert(QVARIANT_LIST))
			return;
		QList<QVariant> v = val.toList();
		if(v.size() != 4)
			return;

		for(int i = 0; i < 2; i++) {
			bool ok;
			data.relaySetpoint[i].voltage = v[i * 2].toInt(&ok); // Note that the actual values aren't checked
			if(!ok)
				return;
			data.relaySetpoint[i].percent = v[i * 2 + 1].toInt(&ok); // Note that the actual values aren't checked
			if(!ok)
				return;
		}

		valid = true;
	}
	QVariant toVariant() {
		QList<QVariant> v;
		for(int i = 0; i < 2; i++) {
			v.append(data.relaySetpoint[i].voltage);
			v.append(data.relaySetpoint[i].percent);
		}

		return v;
	}

	double getVolts(bool high) {
		return data.relaySetpoint[index(high)].voltage / 10.0;
	}

	int getPercent(bool high) {
		return data.relaySetpoint[index(high)].percent;
	}

	bool isReversed() {
		return data.relaySetpoint[0].voltage > data.relaySetpoint[1].voltage;
	}

	void setVolts(double low, double high, bool reversed) { // Volts must be set together!
		bool shouldSwap = (reversed != isReversed());
		int lowIndex = index(false);
		int highIndex = index(true);

		data.relaySetpoint[lowIndex].voltage = low * 10.0 + 0.5;
		data.relaySetpoint[highIndex].voltage = high * 10.0 + 0.5;

		if(shouldSwap) {
			qSwap(data.relaySetpoint[0], data.relaySetpoint[1]);
		}
	}

	void setPercent(bool high, int percent) {
		data.relaySetpoint[index(high)].percent = percent;
	}

private:
	int index(bool high) {
		if(isReversed())
			high = !high;
		return high ? 1 : 0;
	}
};

class ChargedProgramValue : public ProgramValue {
public:
	ChargedProgramValue(enum PMProgramNumber prog, union PMProgramData &val) : ProgramValue(prog, val) {}
	ProgramValue *clone() { return new ChargedProgramValue(*this); }
	ChargedProgramValue(enum PMProgramNumber prog, const QVariant & val) : ProgramValue(prog) {
		if(!val.canConvert(QVARIANT_LIST))
			return;
		QList<QVariant> v = val.toList();
		if(v.size() != 2)
			return;

		bool ok;
		data.chargedCriteria.voltage = v[0].toInt(&ok);
		if(!ok)
			return;
		data.chargedCriteria.current = v[1].toInt(&ok);
		if(!ok)
			return;

		valid = true;
	}
	QVariant toVariant() {
		QList<QVariant> v;
		v.append(data.chargedCriteria.voltage);
		v.append(data.chargedCriteria.current);

		return v;
	}

	double getVoltage() {
		return data.chargedCriteria.voltage / 10.0;
	}

	int getCurrent() {
		return data.chargedCriteria.current;
	}

	void setVoltage(double voltage) {
		data.chargedCriteria.voltage = voltage * 10.0 + 0.5;
	}

	void setCurrent(int current) {
		data.chargedCriteria.current = current;
	}
};

class EfficiencyProgramValue : public ProgramValue {
public:
	EfficiencyProgramValue(enum PMProgramNumber prog, union PMProgramData &val) : ProgramValue(prog, val) {}
	ProgramValue *clone() { return new EfficiencyProgramValue(*this); }
	EfficiencyProgramValue(enum PMProgramNumber prog, const QVariant & val) : ProgramValue(prog) {
		if(!val.canConvert(QVARIANT_LIST))
			return;
		QList<QVariant> v = val.toList();
		if(v.size() != 2)
			return;

		bool ok;
		data.efficiency.selfdischarge = v[0].toInt(&ok);
		if(!ok)
			return;
		data.efficiency.percent = v[1].toInt(&ok);
		if(!ok)
			return;

		valid = true;
	}
	QVariant toVariant() {
		QList<QVariant> v;
		v.append(data.efficiency.selfdischarge);
		v.append(data.efficiency.percent);

		return v;
	}

	double getSelfDischarge() {
		return data.efficiency.selfdischarge / 100.0;
	}

	int getEfficiency() {
		return data.efficiency.percent;
	}

	void setSelfDischarge(double current) {
		data.efficiency.selfdischarge = current * 100.0 + 0.5;
	}

	void setEfficiency(int percent) {
		data.efficiency.percent = percent;
	}
};

class ChargeTimeProgramValue : public ProgramValue {
public:
	ChargeTimeProgramValue(enum PMProgramNumber prog, union PMProgramData &val) : ProgramValue(prog, val) {}
	ProgramValue *clone() { return new ChargeTimeProgramValue(*this); }
	ChargeTimeProgramValue(enum PMProgramNumber prog, const QVariant & val) : ProgramValue(prog) {
		bool ok;
		data.chargeTime = val.toInt(&ok); // Note that the actual values aren't checked
		if(!ok)
			return;

		valid = true;
	}
	QVariant toVariant() {
		return data.chargeTime;
	}

	int getDays() {
		return data.chargeTime;
	}

	void setDays(int days) {
		data.chargeTime = days;
	}
};

class TimeProgramValue : public ProgramValue {
public:
	TimeProgramValue(enum PMProgramNumber prog, union PMProgramData &val) : ProgramValue(prog, val) {}
	ProgramValue *clone() { return new TimeProgramValue(*this); }
	TimeProgramValue(enum PMProgramNumber prog, const QVariant & val) : ProgramValue(prog) {
		bool ok;
		data.timeMinutes = val.toInt(&ok); // Note that the actual values aren't checked
		if(!ok)
			return;

		valid = true;
	}
	QVariant toVariant() {
		return data.timeMinutes;
	}

	int getDay() {
		return data.timeMinutes / 1440;
	}

	int getHour() {
		return (data.timeMinutes % 1440) / 60;
	}

	int getMinute() {
		return (data.timeMinutes % 1440) % 60;
	}

	unsigned int getRawMinutes() {
		return data.timeMinutes;
	}

	void setDay(int day) {
		data.timeMinutes = (data.timeMinutes % 1440) + day * 1440;
	}

	void setHour(int hour) {
		data.timeMinutes = (data.timeMinutes / 1440) * 1440 + (data.timeMinutes % 1440) % 60 + hour * 60;
	}

	void setMinute(int minute) {
		data.timeMinutes = (data.timeMinutes / 60) * 60 + minute;
	}
};

class PeriodicProgramValue : public ProgramValue {
public:
	PeriodicProgramValue(enum PMProgramNumber prog, union PMProgramData &val) : ProgramValue(prog, val) {}
	ProgramValue *clone() { return new PeriodicProgramValue(*this); }
	PeriodicProgramValue(enum PMProgramNumber prog, const QVariant & val) : ProgramValue(prog) {
		if(!val.canConvert(QVARIANT_LIST))
			return;
		QList<QVariant> v = val.toList();
		if(v.size() != 3)
			return;

		bool ok;
		data.timeMinutes = v[0].toInt(&ok);
		if(!ok)
			return;
		data40.minutesPerMeasurement = v[1].toInt(&ok);
		if(!ok)
			return;
		data41_42.periodicLoggedItems = v[2].toInt(&ok);
		if(!ok)
			return;

		valid = true;
	}
	QVariant toVariant() {
		QList<QVariant> v;
		v.append(data.timeMinutes);
		v.append(data40.minutesPerMeasurement);
		v.append(data41_42.periodicLoggedItems);

		return v;
	}

	bool needsMore(enum PMProgramNumber & next) {
		switch(next) {
			case PM_P39:
				next = PM_P40;
				return true;
			case PM_P40:
				next = PM_P41_42;
				return true;
			case PM_P41_42:
			default:
				return false;
		}
	}
	void toRawExtended(enum PMProgramNumber curr, union PMProgramData &val) {
		switch(curr) {
			case PM_P40:
				val = data40;
				break;
			case PM_P41_42:
				val = data41_42;
				break;
			default:
				break;
		}
	}
	bool updateValue(enum PMProgramNumber curr, union PMProgramData & val) {
		switch(curr) {
			case PM_P40:
				data40 = val;
				return true;
			case PM_P41_42:
				data41_42 = val;
				return true;
			default:
				return false;
		}
	}

	int getStartHour() {
		return data.timeMinutes / 60;
	}

	int getStartMinute() {
		return data.timeMinutes % 60;
	}

	void setStartHour(int hour) {
		data.timeMinutes = data.timeMinutes % 60 + hour * 60;
	}

	void setStartMinute(int minute) {
		data.timeMinutes = (data.timeMinutes / 60) * 60 + minute;
	}

	int getIntervalMinutes() {
		return data40.minutesPerMeasurement;
	}

	void setIntervalMinutes(int minutes) {
		data40.minutesPerMeasurement = minutes;
	}

	int getEnabled() {
		return data41_42.periodicLoggedItems;
	}

	void setEnabled(int enabled) {
		data41_42.periodicLoggedItems = enabled;
	}
	
private:
	union PMProgramData data40;
	union PMProgramData data41_42;
};

class ProfileProgramValue : public ProgramValue {
public:
	ProfileProgramValue(enum PMProgramNumber prog, union PMProgramData &val) : ProgramValue(prog, val) {}
	ProgramValue *clone() { return new ProfileProgramValue(*this); }
	ProfileProgramValue(enum PMProgramNumber prog, const QVariant & val) : ProgramValue(prog) {
		bool ok;
		data.fivePercentOptions = val.toInt(&ok); // Note that the actual values aren't checked
		if(!ok)
			return;

		valid = true;
	}
	QVariant toVariant() {
		return data.fivePercentOptions;
	}

	bool isBatteryOn(int num) { // 1 or 2
		return (((num == 2) ? PM_INTERVAL_BAT2 : PM_INTERVAL_BAT1) & data.fivePercentOptions) != 0;
	}

	bool isFivePercentInterval() {
		return (data.fivePercentOptions & PM_INTERVAL_5PERCENT) != 0;
	}

	void setBatteryOn(int num, bool on) { // 1 or 2
		uint8_t mask = (num == 2) ? PM_INTERVAL_BAT2 : PM_INTERVAL_BAT1;
		data.fivePercentOptions = (data.fivePercentOptions & ~mask) | (on ? mask : 0);
	}

	void setFivePercentInterval(bool five) {
		data.fivePercentOptions = (data.fivePercentOptions & ~PM_INTERVAL_5PERCENT) | (five ? PM_INTERVAL_5PERCENT : 0);
	}
};

class InternetProgramValue : public ProgramValue {
public:
	InternetProgramValue(enum PMProgramNumber prog, union PMProgramData &val) : ProgramValue(prog, val) {}
	ProgramValue *clone() { return new InternetProgramValue(*this); }
	InternetProgramValue(enum PMProgramNumber prog, const QVariant & val) : ProgramValue(prog) {
		if(!val.canConvert(QVARIANT_LIST))
			return;
		QList<QVariant> v = val.toList();
		if(v.size() != 8)
			return;

		data.tcp.dhcpEnabled = v[0].toBool();
		QByteArray ipArr = v[1].toByteArray();
		if(ipArr.size() != 4)
			return;
		memcpy(data.tcp.ipAddress, ipArr.data(), 4);
		ipArr = v[2].toByteArray();
		if(ipArr.size() != 4)
			return;
		memcpy(data.tcp.gatewayAddress, ipArr.data(), 4);
		ipArr = v[3].toByteArray();
		if(ipArr.size() != 4)
			return;
		memcpy(data.tcp.subnetMask, ipArr.data(), 4);
		ipArr = v[4].toByteArray();
		if(ipArr.size() != 4)
			return;
		memcpy(data.tcp.primaryDNSServer, ipArr.data(), 4);
		ipArr = v[5].toByteArray();
		if(ipArr.size() != 4)
			return;
		memcpy(data.tcp2.secondaryDNSServer, ipArr.data(), 4);
		bool ok;
		data.tcp2.listenPort = v[6].toInt(&ok);
		if(!ok)
			return;
		QByteArray arr = v[7].toByteArray();
		strncpy(dataNetbios.tcpString, arr.data(), 16);

		valid = true;
	}
	QVariant toVariant() {
		QList<QVariant> v;
		v.append((bool) data.tcp.dhcpEnabled);
		v.append(QByteArray((char *) data.tcp.ipAddress, 4));
		v.append(QByteArray((char *) data.tcp.gatewayAddress, 4));
		v.append(QByteArray((char *) data.tcp.subnetMask, 4));
		v.append(QByteArray((char *) data.tcp.primaryDNSServer, 4));
		v.append(QByteArray((char *) data.tcp2.secondaryDNSServer, 4));
		v.append(data.tcp2.listenPort);
		v.append(QString(dataNetbios.tcpString).toUtf8());

		return v;
	}

	bool needsMore(enum PMProgramNumber & next) {
		switch(next) {
			case PM_P_TCP:
				next = PM_P_TCP_2;
				return true;
			case PM_P_TCP_2:
				next = PM_P_TCP_NETBIOS;
				return true;
			case PM_P_TCP_NETBIOS:
				next = PM_P_TCP_PASSWORD;
				return true;
			case PM_P_TCP_PASSWORD:
			default:
				return false;
		}
	}

	void toRawExtended(enum PMProgramNumber curr, union PMProgramData &val) {
		switch(curr) {
			case PM_P_TCP_2:
				val = dataTcp2;
				break;
			case PM_P_TCP_NETBIOS:
				val = dataNetbios;
				break;
			case PM_P_TCP_PASSWORD:
				val = dataPassword;
				break;
			default:
				break;
		}
	}

	bool updateValue(enum PMProgramNumber curr, union PMProgramData & val) {
		switch(curr) {
			case PM_P_TCP_2:
				dataTcp2 = val;
				return true;
			case PM_P_TCP_NETBIOS:
				dataNetbios = val;
				return true;
			case PM_P_TCP_PASSWORD:
				dataPassword = val;
				return true;
			default:
				return false;
		}
	}

	bool isDhcpEnabled() {
		return data.tcp.dhcpEnabled;
	}

	QString getIPAddress() {
		return formatIPAddress(data.tcp.ipAddress);
	}

	QString getGatewayAddress() {
		return formatIPAddress(data.tcp.gatewayAddress);
	}

	QString getSubnetMask() {
		return formatIPAddress(data.tcp.subnetMask);
	}

	QString getPrimaryDNSServer() {
		return formatIPAddress(data.tcp.primaryDNSServer);
	}

	QString getSecondaryDNSServer() {
		return formatIPAddress(dataTcp2.tcp2.secondaryDNSServer);
	}

	uint16_t getListenPort() {
		return dataTcp2.tcp2.listenPort;
	}

	QString getNetBIOSName() {
		return QString(dataNetbios.tcpString);
	}

	void setDhcpEnabled(bool enabled) {
		data.tcp.dhcpEnabled = enabled;
	}

	bool setIPAddress(const QString & address) {
		return parseIPAddress(data.tcp.ipAddress, address);
	}

	bool setGatewayAddress(const QString & address) {
		return parseIPAddress(data.tcp.gatewayAddress, address);
	}

	bool setSubnetMask(const QString & mask) {
		return isSubnetMaskValid(mask, data.tcp.subnetMask);
	}

	// Checks for validity (and parses, if parsed is not null)
	bool isSubnetMaskValid(const QString & mask, uint8_t *maskVal = NULL) {
		uint8_t tempBuf[4];
		if(maskVal == NULL)
			maskVal = tempBuf;

		if(!parseIPAddress(maskVal, mask))
			return false;

		bool foundOne = false; // First one already found
		for(int i = 0; i < 4; i++) {
			for(int j = 0; j < 8; j++) {
				bool isOne = (maskVal[3 - i] & (1 << j)) != 0;
				if(!foundOne && isOne) {
					foundOne = true;
				} else if(foundOne && !isOne) { // Went back to 0
					return false;
				}
			}
		}
		return true;		
	}

	bool setPrimaryDNSServer(const QString & address) {
		return parseIPAddress(data.tcp.primaryDNSServer, address);
	}

	bool setSecondaryDNSServer(const QString & address) {
		return parseIPAddress(dataTcp2.tcp2.secondaryDNSServer, address);
	}

	void setListenPort(uint16_t port) {
		dataTcp2.tcp2.listenPort = port;
	}

	bool setNetBIOSName(const QString & name) {
		QByteArray data = name.toUtf8();
		if(data.size() > 16)
			return false;
		strncpy(dataNetbios.tcpString, data.constData(), 16);
		return true;
	}

	bool setPassword(const QString & name) {
		QByteArray data = name.toUtf8();
		if(data.size() > 16)
			return false;
		strncpy(dataPassword.tcpString, data.constData(), 16);
		return true;
	}	

private:
	QString formatIPAddress(uint8_t *bytes) {
		return QString("%1.%2.%3.%4").arg((int) bytes[0]).arg((int) bytes[1]).arg((int) bytes[2]).arg((int) bytes[3]);
	}

	// Bytes is undefined if this returns false
	bool parseIPAddress(uint8_t *bytes, const QString & str) {
		QStringList components = str.split(".");
		if(components.size() != 4)
			return false;
		for(int i = 0; i < 4; i++) {
			bool ok;
			unsigned int converted = components[i].toUInt(&ok, 10);
			if(!ok || converted > 255)
				return false;
			bytes[i] = converted;
		}
		return true;
	}

	union PMProgramData dataTcp2;
	union PMProgramData dataNetbios;
	union PMProgramData dataPassword;
};

#endif
