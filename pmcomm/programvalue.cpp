#include "programvalue.h"

// This function returns the appropriate subclass of ProgramValue for each program
ProgramValue *ProgramValue::makeProgramValue(enum PMProgramNumber prog, union PMProgramData &val) {
	switch(prog) {
		case PM_P1:
		case PM_P2:
		case PM_P3:
		case PM_P4:
		case PM_P5: return new SwitchSelectProgramValue(prog, val);
		case PM_P6_9: return new LabelsProgramValue(prog, val);
		case PM_P11_13: return new ShuntTypesProgramValue(prog, val);
		case PM_P_VERSION: return new VersionProgramValue(prog, val);
		case PM_P14:
		case PM_P15: return new CapacityProgramValue(prog, val);
		case PM_P16: return new FilterTimeProgramValue(prog, val);
		case PM_P22_23:
		case PM_P24_25: return new AlarmLevelProgramValue(prog, val);
		case PM_P26:
		case PM_P28: return new LowAlarmProgramValue(prog, val);
		case PM_P27:
		case PM_P29: return new HighAlarmProgramValue(prog, val);
		case PM_P30_31: return new RelayProgramValue(prog, val);
		case PM_P32:
		case PM_P33: return new ChargedProgramValue(prog, val);
		case PM_P34_35: return new EfficiencyProgramValue(prog, val);
		case PM_P36:
		case PM_P37: return new ChargeTimeProgramValue(prog, val);
		case PM_P38: return new TimeProgramValue(prog, val);
		case PM_P39: return new PeriodicProgramValue(prog, val);
		case PM_P43: return new ProfileProgramValue(prog, val);
		case PM_P_TCP: return new InternetProgramValue(prog, val);

		default: return NULL;
	}
}

ProgramValue *ProgramValue::makeProgramValue(enum PMProgramNumber prog, const QVariant & val) {
	ProgramValue *value = privMakeProgFromVariant(prog, val);
	if(value == NULL)
		return NULL;

	if(!value->valid) {
		delete value;
		return NULL;
	}
	return value;
}

ProgramValue *ProgramValue::privMakeProgFromVariant(enum PMProgramNumber prog, const QVariant & val) {
	switch(prog) {
		case PM_P1:
		case PM_P2:
		case PM_P3:
		case PM_P4:
		case PM_P5: return new SwitchSelectProgramValue(prog, val);
		case PM_P6_9: return new LabelsProgramValue(prog, val);
		case PM_P11_13: return new ShuntTypesProgramValue(prog, val);
		case PM_P14:
		case PM_P15: return new CapacityProgramValue(prog, val);
		case PM_P16: return new FilterTimeProgramValue(prog, val);
		case PM_P22_23:
		case PM_P24_25: return new AlarmLevelProgramValue(prog, val);
		case PM_P26:
		case PM_P28: return new LowAlarmProgramValue(prog, val);
		case PM_P27:
		case PM_P29: return new HighAlarmProgramValue(prog, val);
		case PM_P30_31: return new RelayProgramValue(prog, val);
		case PM_P32:
		case PM_P33: return new ChargedProgramValue(prog, val);
		case PM_P34_35: return new EfficiencyProgramValue(prog, val);
		case PM_P36:
		case PM_P37: return new ChargeTimeProgramValue(prog, val);
		case PM_P38: return new TimeProgramValue(prog, val);
		case PM_P39: return new PeriodicProgramValue(prog, val);
		case PM_P43: return new ProfileProgramValue(prog, val);
		case PM_P_TCP: return new InternetProgramValue(prog, val);

		default: return NULL;
	}	
}

#define GET_NAME_MACRO(pnum) case pnum: return #pnum;

QString ProgramValue::getName(enum PMProgramNumber prog) {
	switch(prog) {
		GET_NAME_MACRO(PM_P1)
		GET_NAME_MACRO(PM_P2)
		GET_NAME_MACRO(PM_P3)
		GET_NAME_MACRO(PM_P4)
		GET_NAME_MACRO(PM_P5)
		GET_NAME_MACRO(PM_P6_9)
		GET_NAME_MACRO(PM_P11_13)
		GET_NAME_MACRO(PM_P_VERSION)
		GET_NAME_MACRO(PM_P14)
		GET_NAME_MACRO(PM_P15)
		GET_NAME_MACRO(PM_P16)
		GET_NAME_MACRO(PM_P22_23)
		GET_NAME_MACRO(PM_P24_25)
		GET_NAME_MACRO(PM_P26)
		GET_NAME_MACRO(PM_P28)
		GET_NAME_MACRO(PM_P27)
		GET_NAME_MACRO(PM_P29)
		GET_NAME_MACRO(PM_P30_31)
		GET_NAME_MACRO(PM_P32)
		GET_NAME_MACRO(PM_P33)
		GET_NAME_MACRO(PM_P34_35)
		GET_NAME_MACRO(PM_P36)
		GET_NAME_MACRO(PM_P37)
		GET_NAME_MACRO(PM_P38)
		GET_NAME_MACRO(PM_P39)
		GET_NAME_MACRO(PM_P43)
		GET_NAME_MACRO(PM_P_TCP)

		default: return "INVALID";
	}
}

#define GET_NUMBER_MACRO(pnum) if(name == #pnum) return pnum;

enum PMProgramNumber ProgramValue::getNumber(QString & name) {
	GET_NUMBER_MACRO(PM_P1)
	GET_NUMBER_MACRO(PM_P2)
	GET_NUMBER_MACRO(PM_P3)
	GET_NUMBER_MACRO(PM_P4)
	GET_NUMBER_MACRO(PM_P5)
	GET_NUMBER_MACRO(PM_P6_9)
	GET_NUMBER_MACRO(PM_P11_13)
	GET_NUMBER_MACRO(PM_P_VERSION)
	GET_NUMBER_MACRO(PM_P14)
	GET_NUMBER_MACRO(PM_P15)
	GET_NUMBER_MACRO(PM_P16)
	GET_NUMBER_MACRO(PM_P22_23)
	GET_NUMBER_MACRO(PM_P24_25)
	GET_NUMBER_MACRO(PM_P26)
	GET_NUMBER_MACRO(PM_P28)
	GET_NUMBER_MACRO(PM_P27)
	GET_NUMBER_MACRO(PM_P29)
	GET_NUMBER_MACRO(PM_P30_31)
	GET_NUMBER_MACRO(PM_P32)
	GET_NUMBER_MACRO(PM_P33)
	GET_NUMBER_MACRO(PM_P34_35)
	GET_NUMBER_MACRO(PM_P36)
	GET_NUMBER_MACRO(PM_P37)
	GET_NUMBER_MACRO(PM_P38)
	GET_NUMBER_MACRO(PM_P39)
	GET_NUMBER_MACRO(PM_P43)
	GET_NUMBER_MACRO(PM_P_TCP)

	return PM_PINVALID;
}
