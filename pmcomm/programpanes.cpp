#include "programpanes.h"

ProgramPane *ProgramPane::makeProgramPane(enum PMProgramNumber prog, SiteManager *manager, QWidget *parent) {
	switch(prog) {
		case PM_P1: return new SwitchSelectProgramPane("P1: Switch Select #1", manager, parent);
		case PM_P2: return new SwitchSelectProgramPane("P2: Switch Select #2", manager, parent);
		case PM_P3: return new SwitchSelectProgramPane("P3: Switch Select #3", manager, parent);
		case PM_P4: return new SwitchSelectProgramPane("P4: Switch Select #4", manager, parent);
		case PM_P5: return new SwitchSelectProgramPane("P5: Switch Select #5", manager, parent);
		case PM_P6_9: return new LabelsProgramPane("P6-9: Amp/Volt Labels", parent);
		case PM_P11_13: return new ShuntTypesProgramPane("P11-13: Shunt Selection", parent);
		case PM_P14: return new CapacityProgramPane("P14: Batt1 Enable/Capacity", 1, parent);
		case PM_P15: return new CapacityProgramPane("P15: Batt2 Enable/Capacity", 2, parent);
		case PM_P16: return new FilterTimeProgramPane("P16: Filter Time", parent);
		case PM_P22_23: return new AlarmLevelProgramPane("P22-23: Alarm Enable Battery 1", 1, parent);
		case PM_P24_25: return new AlarmLevelProgramPane("P24-25: Alarm Enable Battery 2", 2, parent);
		case PM_P26: return new LowAlarmProgramPane("P26: Battery 1 Low Alarm Setpoint", 1, parent);
		case PM_P27: return new HighAlarmProgramPane("P27: Battery 1 High Alarm Setpoint", 1, parent);
		case PM_P28: return new LowAlarmProgramPane("P28: Battery 2 Low Alarm Setpoint", 2, parent);
		case PM_P29: return new HighAlarmProgramPane("P29: Battery 2 High Alarm Setpoint", 2, parent);
		case PM_P30_31: return new RelayProgramPane("P30-31: Relay On/Off Setpoint", parent);
		case PM_P32: return new ChargedCriteriaProgramPane("P32: Battery 1 Charged Criteria", 1, parent);
		case PM_P33: return new ChargedCriteriaProgramPane("P33: Battery 2 Charged Criteria", 2, parent);
		case PM_P34_35: return new EfficiencyProgramPane("P34-35: Battery Efficiency Compensation", parent);
		case PM_P36: return new EqualizeTimeProgramPane("P36: Time Between Equalize", "equalize", parent);
		case PM_P37: return new ChargeTimeProgramPane("P37: Time Between Charge", "charge", parent);
		case PM_P38: return new TimeProgramPane("P38: Date/Time", parent);
		case PM_P39: return new PeriodicProgramPane("P39-42: Periodic Logged Data", parent);
		case PM_P43: return new ProfileProgramPane("P43: Discharge Profile Data", parent);
		case PM_P45_49: return new ResetProgramPane("P45-49: Erase / Initialize Data", manager, parent);
		case PM_P_TCP: return new InternetProgramPane("TCP/IP Settings", parent);

		default: return NULL;
	}
}

QString SwitchSelectProgramPane::helpText() {
	return "These select up to 5 display items to be accessed by EACH of the top row of 5 switches on the PentaMetic Display Unit. "
	"The 5 switches are numbered left to right, from #1 to #5.";
}

QString LabelsProgramPane::helpText() {
	return "These allow selection of more meaningful names to identify some display items. The \"Amps\" labels also affect the "
	"corresponding \"Watt Hours\", \"Amp Hours\", and \"Watts\" displays.";
}

QString ShuntTypesProgramPane::helpText() {
	return "The check box \"Read Amps 2 as...\" means that this choice will cause Amps channel 2 to sum the Amps from shunts 1 and 2.  "
	"The Amps 1 channel will still measure just amps for shunt 1.";
}

QString CapacityProgramPane::helpText() {
	return "The \"capacity\" value should be used when a battery is being measured with this channel. The \"percent full\" display (AD22-23) "
	"is based on this assumed value of amp hour capacity.\n\n"
	"\"Treat channel as battery\" means that: (1) the \"Percent full\" battery data will be available for that channel, (2) DISCHARGING "
	"amps will accumulate at their exact rate, however CHARGING amps will accumulate at a lesser percentage determined by \"efficiency factor\" " 
	"(P34-P35), and (3) Amp Hours will be reset to zero and Percent Full to 100% when the \"charge criteria\" P31-32 are satisfied.  See "
	"Secction 7 of the PentaMetric instructions has full details.\n\n"
	"If this option not selected positive amps and negative amps will accumulate amp hours at their exact rates with no resets.";
}

QString FilterTimeProgramPane::helpText() {
	return "The displays AD3,4,10,11,12 are filtered versions (more sluggish) of the regular volts and amps displays (AD1,2,7,8,9).  "
	"A higher filer time makes them more highly filtered (slower to respond).  These filtered values are used for the voltage and "
	"current setpoint values in P31-32 to eliminate the effect of brief changes in the volts or amps values. When set to 0 both "
	"filtered and regular displays are identical.";
}

QString AlarmLevelProgramPane::helpText() {
	return "The \"audible\" alarm option provides an audible alarm when using the PentaMetric display unit";
}

QString LowAlarmProgramPane::helpText() {
	return "";
}

QString HighAlarmProgramPane::helpText() {
	return "";
}

QString RelayProgramPane::helpText() {
	return "This controls when the relay output turns on and off. In case of conflict between Volts and %Full, the Volts setting will be used";
}

QString ChargedCriteriaProgramPane::helpText() {
	return "These define when the battery is considered \"charged\".  This resets the battery amp hours to zero and the \"Battery Percent Full\" "
	"to 100%.  It also defines the beginning and end of charging cycles for the logged efficiency data. They only apply when "
	"\"treat channel as battery” is checked in P14-15.";
}

QString EfficiencyProgramPane::helpText() {
	return "This applies to battery measured \"Amp Aours\" AD13-14 only when \"Treat channel as battery\" is checked in P14-15. It is intended to "
	"compensate for self discharge when calculating \"Percent Full\". If \"Efficiency factor\" is set to 94% then charging amp hours will accumulate "
	"at 94% of the actual amp rate, whereas the discharge rate is always done at 100% to require extra charging. The \"self discharge\" amps is a "
	"value of amps that causes a constant discharge of this amount to be accumulated in the amp hour value at all times.  Normally both of these "
	"will not be used simultaneously.";
}

QString EqualizeTimeProgramPane::helpText() {
	return "This is only intended to be used with the \"Time to Equalize\" alarm P22-25. Its only function is to define the time interval from when "
	"the \"time to equalize\" is MANUALLY reset to when the alarm activates.";
}

QString ChargeTimeProgramPane::helpText() {
	return "This is used with the \"Time to Charge\" alarm P22-25.  The time in \"Days since charged\" displays (AD24-25) automatically reset when "
	"the \"charged\" criteria (P32-33) are met. The alarm occurs when the AD24-25 time exceeds the value entered here. to remind the user to not go "
	"too long between a full charge of the batteries.";
}

QString TimeProgramPane::helpText() {
	return "The date and time set here is used when displaying logged data on the PentaMetric display unit. This time is NOT used for logged data "
	"downloaded through PMComm, since the time is correctly adjusted based on the PC clock as long as the clock in the PentaMetric continues to run. "
	"Since changing this makes the PentaMetric's clock change, it may cause times for data stored before change to be incorrect. Changing the time "
	"frequently is therefore not recommended.";
}

QString PeriodicProgramPane::helpText() {
	return "This is used to configure which data is logged periodically and how often. When memory is full, new data overwrites the oldest data";
}

QString ProfileProgramPane::helpText() {
	return "As batteries charge or discharge, the \"battery discharge profile data\" logs battery volts and amps each time the \"percent full\" "
	"goes up or down by 5% (or 10%). This can show, when discharging, at what percent full batteries begin to show more rapid voltage dropoff "
	"indicating low battery energy left.";
}

QString ResetProgramPane::helpText() {
	return "For the logged data items (first four) above: when memory is full, new data overwrites the oldest data. These buttons allow you to "
	"erase ALL old data. \"Reset programs to defaults\" causes all program values (not logged data) to return to original factory defaults.";
}

QString InternetProgramPane::helpText() {
	return "To automatically get an IP address for the PentaMetric interface, check the \"Use DHCP for configuration\" box. Note that this can "
	"make it difficult to connect to the PentaMetric unless you configure your DHCP server (typically your router in home/small office networks) "
	"to always give out the same IP address every time; this setting is typically called something like \"DHCP reservation\". Otherwise, you can "
	"manually configure an IP address and other settings.\n\n"
	"If you specify a name in the \"NetBIOS Name\" box, you can connect from PMComm using this name instead of the IP address (only works when "
	"running PMComm on Windows). \"Listen on port\" specifies the TCP port on which the PentaMetric will listen for connections, which must match "
	"the port configured in PMComm. The default of 1701 is suggested unless you have a reason to change it.\n\n"
	"AFTER YOU CHANGE THESE SETTINGS YOU MUST ENSURE THAT JUMPER J3 ON THE INTERFACE UNIT IS SET CORRECTLY (SEE THE MANUAL), AND YOU MUST ALSO "
	"CHANGE THE SETTINGS PMCOMM USES TO CONNECT (LOCATED UNDER \"MANAGE SITES\").";
}



