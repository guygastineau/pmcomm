/* This file contains all of the constant and type definitions required to use libpmcomm.
   Brief documentation is provied here; the README will eventually contain more detailed
   documentation.
 */

#ifndef PMDEFS_H
#define PMDEFS_H

#include <stdint.h>
#include <stdbool.h>

/* Include the next line for calling from Visual Basic (or other windows programs expecting __stdcall, i.e. WINAPI calling convention) */
#define USE_STDCALL

/* Get symbols exported correctly */
#ifdef _WIN32
  #ifdef BUILDING_LIBPMCOMM
    #ifdef __GNUC__
      #define PMCOMM_API __attribute__ ((dllexport))
    #else
      #define PMCOMM_API __declspec(dllexport) // Note: actually gcc seems to also supports this syntax.
    #endif
  #else
    #ifdef __GNUC__
      #define PMCOMM_API __attribute__ ((dllimport))
    #else
      #define PMCOMM_API __declspec(dllimport) // Note: actually gcc seems to also supports this syntax.
    #endif
  #endif
  // Set up the calling convention properly
  #ifdef USE_STDCALL
	#define PM_CALLCONV __stdcall
  #else
	#define PM_CALLCONV
  #endif
#else
  #if __GNUC__ >= 4
    #define PMCOMM_API __attribute__ ((visibility ("default")))
  #else
    #define PMCOMM_API
  #endif
  #define PM_CALLCONV
#endif

/* Forward declaration for connections; users should only access pointers to this struct */
struct PMConnection;

/* The displays that can be passed to PMReadDisplayFormatted() */
enum PMDisplayNumber {
	PM_DINVALID = 0,

	PM_D1 = 1,
	PM_D2,
	PM_D3,
	PM_D4,
	PM_D7 = 7,
	PM_D8,
	PM_D9,
	PM_D10,
	PM_D11,
	PM_D12,
	PM_D13,
	PM_D14,
	PM_D15,
	PM_D16,
	PM_D17,
	PM_D18,
	PM_D19,
	PM_D20,
	PM_D21,
	PM_D22,
	PM_D23,
	PM_D24,
	PM_D25,
	PM_D26,
	PM_D27,
	PM_D28,

	PM_D29, /* This block is for use ONLY when programming PM_P1 through PM_P5, not for other API calls such as PMReadDisplayFormatted() */
	PM_D30,
	PM_D31,
	PM_D32,
	PM_D33,
	PM_D34,
	PM_D35,
	PM_D36,
	PM_D37,
	PM_D38,
	PM_D39,
	PM_D40, /* End of block valid only for PM_P1 through PM_P5 */

	PM_D29_34 = 256, /* Note that these are not valid for programming PM_P1 through PM_P5 */
	PM_DALARM,
	PM_D35_40
};

/* The programs that can be passed to PMReadProgramFormatted() and PMWriteProgramFormatted() */
enum PMProgramNumber {
	PM_PINVALID = 0,

	PM_P1,
	PM_P2,
	PM_P3,
	PM_P4,
	PM_P5,
	PM_P6_9,
	PM_P11_13,
	PM_P_VERSION,
	PM_P14,
	PM_P15,
	PM_P16,
	PM_P17_20,
	PM_P22_23,
	PM_P24_25,
	PM_P26,
	PM_P27,
	PM_P28,
	PM_P29,
	PM_P30_31,
	PM_P32,
	PM_P33,
	PM_P34_35,
	PM_P36,
	PM_P37,
	PM_P38,
	PM_P39,
	PM_P40,
	PM_P41_42,
	PM_P43,

	PM_P45_49, // Placeholder for resets.  This should NOT be used with PM[Read Write]ProgramFormatted()

	PM_P_TCP,
	PM_P_TCP_2,
	PM_P_TCP_NETBIOS,
	PM_P_TCP_PASSWORD
};

/* Type for display data, as returned by PMReadDisplayFormatted() */
struct PMDisplayValue {
	int32_t val;
	int decimalPlaces; // Valid only for amps measurements, may expand in the future
};

/* Reset types, for use with PMReset() */
enum PMResetType {
	PM_RESET_AHR1,
	PM_RESET_AHR2,
	PM_RESET_AHR3,

	PM_RESET_CAH1,
	PM_RESET_CAH2,

	PM_RESET_WHR1,
	PM_RESET_WHR2,

	PM_RESET_BAT1_DSC,
	PM_RESET_BAT2_DSC,
	PM_RESET_BAT1_DSE,
	PM_RESET_BAT2_DSE,

	PM_RESET_PERIODIC,
	PM_RESET_DISCHARGE,
	PM_RESET_BAT1_EFF,
	PM_RESET_BAT2_EFF,
	PM_RESET_PROGRAM
};

/* Masks for determining which alarms are active (when reading alarm data)
   Must be bitwise ANDed with the alarm value read from the PentaMetric. */
#define PM_ALARMVAL_BAT1_LOW (1)
#define PM_ALARMVAL_BAT1_CHARGED (1 << 1)
#define PM_ALARMVAL_BAT1_HIGH (1 << 2)
#define PM_ALARMVAL_BAT1_NEEDCHARGE (1 << 3)
#define PM_ALARMVAL_BAT1_NEEDEQUALIZE (1 << 4)

#define PM_ALARMVAL_RELAY_ON (1 << 7)

#define PM_ALARMVAL_BAT2_LOW (1 << 8)
#define PM_ALARMVAL_BAT2_CHARGED (1 << 9)
#define PM_ALARMVAL_BAT2_HIGH (1 << 10)
#define PM_ALARMVAL_BAT2_NEEDCHARGE (1 << 11)
#define PM_ALARMVAL_BAT2_NEEDEQUALIZE (1 << 12)

/* Battery label constants (P6).  Partially implemented. */
enum PMBatteryLabel {
	PM_BATLABEL_BATTERY,
	PM_BATLABEL_BATTERY_1
};

/* Amps label constants (P7-9) */
enum PMAmpsLabel {
	PM_AMPSLABEL_AMPSNUM,
	PM_AMPSLABEL_SOLAR,
	PM_AMPSLABEL_WIND,
	PM_AMPSLABEL_HYDRO,
	PM_AMPSLABEL_LOAD,
	PM_AMPSLABEL_BATTERY,
	PM_AMPSLABEL_BATTERY_1
};

/* Structure for all labels (P6-9) */
struct PMLabels {
	enum PMBatteryLabel battLabel;
	enum PMAmpsLabel ampsLabels[3];
};

/* Shunt type constants (P11-13) */
enum PMShuntType {
	PM_SHUNTTYPE_100A100MV,
	PM_SHUNTTYPE_500A50MV
};

/* Structure for shunt settings (P11-13) */
struct PMShunts {
	enum PMShuntType types[3];
	bool amps2Sum; /* True if amps 2 has amps 1 added into it */
};

/* Filter time constants (P16) */
enum PMFilterTime {
	PM_FILTERTIME_0,
	PM_FILTERTIME_30SEC,
	PM_FILTERTIME_2MIN,
	PM_FILTERTIME_8MIN,
	PM_FILTERTIME_32MIN
};

/* Array indeces into which PMAlarmLevel values should be put for
   each type of alarm (P22-25)
 */
enum PMAlarmIndex {
	PM_ALARM_LOBAT,
	PM_ALARM_CHARGED,
	PM_ALARM_HIBAT,
	PM_ALARM_NEEDCHARGE,
	PM_ALARM_NEEDEQUALIZE
};

/* Alarm level constants (P22-25) */
enum PMAlarmLevel {
	PM_ALARMLEVEL_OFF,
	PM_ALARMLEVEL_VISUAL,
	PM_ALARMLEVEL_VISUAL_AUDIBLE
};

/* Alarm setpoints structure for high and low battery alarms (P26-29)
   Also used for relay on/off setpoints (P30-31)
   */
struct PMAlarmSetpoint {
	uint16_t voltage; // Tenths of a volt
	uint8_t percent; // Percent (0-100)
};

/* Charged critera structure (P32-33) */
struct PMChargedCriteria {
	uint16_t voltage; // Tenths of a volt
	uint8_t current; // Integer amps (0-100)
};

/* Efficency structure for reading (D29-34), AND for setting efficiency (P34-35) */
struct PMEfficiency {
	int16_t selfdischarge; // Hundredths of an amp (0-9.99)
	uint8_t percent; // Percent efficiency
	uint8_t numCycles; // Number of cycles averaged for this measurement; for display only
};

/* First half of main TCP settings (PTCP) */
struct PMTCPSettings {
	bool dhcpEnabled;
	uint8_t ipAddress[4];
	uint8_t gatewayAddress[4];
	uint8_t subnetMask[4];
	uint8_t primaryDNSServer[4];
};

/* Second half of main TCP settings (PTCP_2) */
struct PMTCPSettings2 {
	uint8_t secondaryDNSServer[4];
	uint16_t listenPort;
};

/* Masks for selecting periodic data to log (P39-42)  Must be bitwise ORed together */
#define PM_PERIODIC_AHR1 (1)
#define PM_PERIODIC_AHR2 (1 << 1)
#define PM_PERIODIC_AHR3 (1 << 2)
#define PM_PERIODIC_WHR1 (1 << 3)
#define PM_PERIODIC_WHR2 (1 << 4)
#define PM_PERIODIC_TEMP (1 << 5)
#define PM_PERIODIC_VOLTS1 (1 << 6)
#define PM_PERIODIC_AMPS1 (1 << 7)
#define PM_PERIODIC_VOLTS2 (1 << 8)
#define PM_PERIODIC_PERCENTFULL (1 << 9)

/* Masks for selecting discharge profile options (P43)  Must be bitwise ORed together */
#define PM_INTERVAL_BAT1 (1 << 5) // Log battery 1
#define PM_INTERVAL_BAT2 (1 << 6) // Log battery 1
#define PM_INTERVAL_5PERCENT (1 << 7) // Take measurements every 5% (as opposed to every 10%)

/* Type for program data, as returned by PMReadProgramFormatted() and accepted by
   PMWriteProgramFormatted().  Use the appropriate field for the data being read/written;
   for example, access the .batCapacity field when reading or writing program P14.
 */
union PMProgramData {
	enum PMDisplayNumber switchDisplay[5]; 		// P1-5 (number of the display to show, needs documentation)
	struct PMLabels labels; 					// P6-9
	struct PMShunts shunts;		 				// P11-13
	uint8_t version; 							// Input unit firmware version (e.g. 16 represents version 1.6)
	uint16_t batCapacity; 						// P14-15 (amp-hours)
	enum PMFilterTime filterTime; 				// P16
	enum PMAlarmLevel alarmLevel[5]; 			// P22-25
	struct PMAlarmSetpoint lowAlarmSetpoint; 	// P26,28
	uint16_t highAlarmSetpoint; 				// P27,29 (tenths of a volt)
	struct PMAlarmSetpoint relaySetpoint[2]; 	// P30-31
	struct PMChargedCriteria chargedCriteria; 	// P32-33
	struct PMEfficiency efficiency; 			// P34-35
	uint8_t chargeTime; 						// P36-37 (days)
	uint32_t timeMinutes; 						// P38-39 (minutes)
	uint16_t minutesPerMeasurement; 			// P40 (minutes)
	uint16_t periodicLoggedItems; 				// P39-42 (bitwise OR of PM_PERIODIC_... constants, above)
	uint8_t fivePercentOptions; 				// P43 (bitwise OR of PM_INTERVAL_... constants, above)
	struct PMTCPSettings tcp;					// PTCP
	struct PMTCPSettings2 tcp2;					// PTCP_2
	char tcpString[17];							// PTCP_NETBIOS, PTCP_PASSWORD (null-terminated)
};

/* Type for logged data stored in a format similar to scientific notation.  To convert
   to a double, use a function like this:

double convertScientific(struct PMScientificValue *value) {
  return value->mantissa * pow((double) 10, (double) (value->exponent));
}

 */
struct PMScientificValue {
	int16_t mantissa;
	int8_t exponent;
};

/* Structure to represent the percent full and charged state in periodic records */
struct PMPeriodicBatteryState {
	uint8_t percent;
	bool charged; // True when meets charged criteria
};

/* Structure representing a single record of periodic data.  Only some data may be
   valid; bitwise AND the values from the below masks with the validData field
   to determine whether a given field is in fact valid.  The next record is pointed
   to by the next pointer, resulting in the data being presented as a linked list.
  */
struct PMPeriodicRecord {
	uint16_t validData; // Bitfield (see comment above)
	uint32_t measTime; // Time, in same format as P38

	struct PMScientificValue ahr1;
	struct PMScientificValue ahr2;
	struct PMScientificValue ahr3;
	struct PMScientificValue whr1;
	struct PMScientificValue whr2;
	int8_t minTemp;
	int8_t maxTemp;
	uint16_t volts1;
	uint16_t volts2;
	struct PMScientificValue amps1;
	struct PMPeriodicBatteryState bat1Percent;
	struct PMPeriodicBatteryState bat2Percent;

	struct PMPeriodicRecord *next;
};

/* Masks for determining which periodic data is valid in a given record.  Bitwise
   AND with the validData field to determine if a field is valid */
#define PM_PERIODIC_AHR1_VALID 0x1
#define PM_PERIODIC_AHR2_VALID (0x1 << 1)
#define PM_PERIODIC_AHR3_VALID (0x1 << 2)
#define PM_PERIODIC_WHR1_VALID (0x1 << 3)
#define PM_PERIODIC_WHR2_VALID (0x1 << 4)
#define PM_PERIODIC_TEMP_VALID (0x1 << 5)
#define PM_PERIODIC_VOLTS1_VALID (0x1 << 6)
#define PM_PERIODIC_AMPS1_VALID (0x1 << 7)
#define PM_PERIODIC_VOLTS2_VALID (0x1 << 8)
#define PM_PERIODIC_BATTSTATE_VALID (0x1 << 9)

/* Efficiency data record structure */
struct PMEfficiencyRecord {
	uint32_t endTime; // End of the cycle
	bool validData; // True if the following data is valid (complete cycle)

	uint32_t ahrCharge; // Amp-hours charged (positive integer)
	uint32_t ahrDischarge; // Amp-hours discharged (positive integer)
	int32_t ahrNet; // Difference in the above

	uint32_t cycleMinutes; // Length of the cycle

	double efficiency; // Computed efficiency as a percentage
	double selfDischarge; // Self-discharge current averaged over the cycle
};

/* Discharge profile record structure.  The next record is pointed to by
   the next pointer, resulting in the data being presented as a linked list.
 */
struct PMProfileRecord {
	uint8_t day;
	uint8_t percentFull;
	uint16_t volts;
	struct PMScientificValue amps;

	struct PMProfileRecord *next;
};

/* Callback for download progress. Fraction completed is progress/outof. usrdata
   is a user pointer. */
typedef void (PM_CALLCONV *PMProgressCallback)(int progress, int outof, void *usrdata);

/* Error values returned on failure. All errors are less than zero.
*/
#define PM_ERROR_OTHER (-1) /* Other, unknown errors */
#define PM_ERROR_CONNECTION (-2) /* Cannot connect */
#define PM_ERROR_COMMUNICATION (-3) /* Error communicating once connected */
#define PM_ERROR_BADRESPONSE (-4) /* Bad response */
#define PM_ERROR_DATAFORMAT (-5) /* Data cannot be converted on either read (PentaMetric has bad data) or write (client supplied bad data) */
#define PM_ERROR_BADREQUEST (-6) /* Client made an invalid call (e.g. write to invalid address) */
#define PM_ERROR_ENOMEM (-7) /* No memory could be allocated */

#endif