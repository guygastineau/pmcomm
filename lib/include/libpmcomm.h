/* This file contains the declarations of the functions provided by libpmcomm.  The custom
   data types and enumerations used by these functions are provided in pmdefs.h.

   the README will eventually contain more detailed documentation.
 */

#ifndef LIBPMCOMM_H
#define LIBPMCOMM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "pmdefs.h"

#include <stdint.h>
#include <stdbool.h>

/* Functions to open and close a connection */

/* Opens a connection to the PentaMetric computer interface over TCP/IP.

   hostname: Specifies the host name, in the form of a string representing an IP address, DNS hostname,
   		or (on Windows) a NetBIOS name.
   port: The TCP port to connect to on the PentaMetric computer interface.  The default is 1701.

   returns: On success, an opaque pointer representing the connection.  This pointer is required as an input to
   		almost all of the functions in the API.  On failure, the function returns NULL.  There is not
   		yet any way of determining the reason for the failure.
 */
PMCOMM_API struct PMConnection * PM_CALLCONV PMOpenConnectionInet(const char *hostname, uint16_t port);

/* Opens a connection to the PentaMetric computer interface over a serial port.

   serialport: Specifies the name of the serial port.  On Windows, this should be a string like "COM1".
   		On Unix-like systems, it should be the path to the device node, e.g. "/dev/ttyUSB0".  It
   		is necessary on Unix-like systems to ensure that the user has permissions to access the serial
   		port, which non-root users do not have by default on Linux.

   returns: On success, an opaque pointer representing the connection.  This pointer is required as an input to
   		almost all of the functions in the API.  On failure, the function returns NULL.  There is not
   		yet any way of determining the reason for the failure.
 */
PMCOMM_API struct PMConnection * PM_CALLCONV PMOpenConnectionSerial(const char *serialport);

/* Closes a connection to the PentaMetric.  This function must be called before another connection can be
   opened to the same PentaMetric.

   conn: A connection returned by one of the PMOpen...() functions
 */
PMCOMM_API void PM_CALLCONV PMCloseConnection(struct PMConnection *conn);

/* Gets the version of the Ethernet interface (returns 0 for serial interfaces).
   Returns an integer in "tenths of a version", i.e. 15 means version 1.5
 */
PMCOMM_API int PM_CALLCONV PMGetInterfaceVersion(struct PMConnection *conn);

/* Start and end using sockets (internet connections).  These do nothing except on windows. */

/* Loads the TCP/IP library on Windows by calling WSAStartup().  This needs to be done before
   connecting to a PentaMetric over TCP/IP.  Not necessary on other platforms at present.

   returns: 0 on success, nonzero on error
 */
PMCOMM_API int PM_CALLCONV PMNetInitialize();

/* Unloads the TCP/IP library on Windows by calling WSACleanup().  This should be done
   after all TCP/IP connections are finished.  Not necessary on other platforms at present.

   returns: 0 on success, nonzero on error
 */
PMCOMM_API int PM_CALLCONV PMNetUninitialize();

/* Read displays */

/* Reads one of the PentaMetric display values.

   conn: A connection returned by one of the PMOpen...() functions

   display: The display number to read.  See the declaration of enum PMDisplayNumber in pmdefs.h
        for the list of constants, which correspond to the AD numbers used by the display unit
        and listed in the PentaMetric instruction manual

   result: A pointer to a struct PMDisplayValue (see pmdefs.h) allocated by the caller, into which the requested
   		data will be placed.  Currently, only the val field is used.  The units are as follows:

   		D1-D4: Tenths of a volt
   		D7-D12: Hundredths of an amp.  Note that the last decimal digit is only significant when using the
   			100A/100mV (small) shunt.  It should not be displayed otherwise.  The shunt types can be determined
   			using PMReadProgramFormatted(), described below.
		D13-15: Hundredths of an amp-hour
		D16-17: Amp-hours
		D18-19: Hundredths of a watt.  A maximum of three decimal digits are significant, with at most one
			to the right of the decimal point with the 100A/100mV (small) shunt, and zero to the right of
			the decimal point otherwise.
		D20-21: Hundredths of a watt-hour
		D22-23: Percent full, 100 is full.  Can go a bit over 100 when charged thoroughly.
		D24-27: Hundredths of a day
		D28: Degrees C
		D29-34 and 35-40: Use PMReadEfficiencyFormatted() instead, below
		Alarms: A bitfield consisting of the constants PM_ALARMVAL_... (see pmdefs.h) bitwise ORed together
			to indicate the active alarms

   returns: 0 on success, <0 on error
 */
PMCOMM_API int PM_CALLCONV PMReadDisplayFormatted(struct PMConnection *conn, enum PMDisplayNumber display, struct PMDisplayValue *result);

/* Read and write programs */

/* Reads one of the PentaMetric program values.

   conn: A connection returned by one of the PMOpen...() functions

   prog: The program number to read.  See the declaration of enum PMProgramNumber in pmdefs.h
        for the list of constants, which correspond to the P numbers used by the display unit
        and listed in the PentaMetric instruction manual

   result: A pointer to a union PMProgramData (see pmdefs.h) allocated by the caller, into which the requested
   		data will be placed.  The field that should be accessed depends on the value of prog, as listed
   		in the comments in the union declaration.  The units are listed in the comments in pmdefs.h as
   		well.

   returns: 0 on success, <0 on error
 */
PMCOMM_API int PM_CALLCONV PMReadProgramFormatted(struct PMConnection *conn, enum PMProgramNumber prog, union PMProgramData *result);

/* Writes one of the PentaMetric program values.

   conn: A connection returned by one of the PMOpen...() functions

   prog: The program number to write.  See the declaration of enum PMProgramNumber in pmdefs.h
        for the list of constants, which correspond to the P numbers used by the display unit
        and listed in the PentaMetric instruction manual

   input: A pointer to a union PMProgramData (see pmdefs.h) allocated by the caller, from which the data will
   		be read and copied into the PentaMetric.  The field that should be accessed depends on the value of prog, as listed
   		in the comments in the union declaration.  The units are listed in the comments in pmdefs.h as
   		well.

   returns: 0 on success, <0 on error
 */
PMCOMM_API int PM_CALLCONV PMWriteProgramFormatted(struct PMConnection *conn, enum PMProgramNumber prog, union PMProgramData *input);


/* Documentation still needs to be written for the following functions. although much of the important information is present
   in the comments in pmdefs.h together with the associated data types. */
/* Read efficiency data */
PMCOMM_API int PM_CALLCONV PMReadEfficiencyFormatted(struct PMConnection *conn, bool battery2, struct PMEfficiency *results);

/* Reset displays and programs */
PMCOMM_API int PM_CALLCONV PMReset(struct PMConnection *conn, enum PMResetType reset);

/* Read periodic data */
PMCOMM_API int PM_CALLCONV PMReadPeriodicData(struct PMConnection *conn, struct PMPeriodicRecord **records, PMProgressCallback callback, void *usrdata);
PMCOMM_API void PM_CALLCONV PMFreePeriodicData(struct PMPeriodicRecord *records);

/* Read profile data */
PMCOMM_API int PM_CALLCONV PMReadProfileData(struct PMConnection *conn, struct PMProfileRecord **battery1Records, struct PMProfileRecord **battery2Records, PMProgressCallback callback, void *usrdata);
PMCOMM_API void PM_CALLCONV PMFreeProfileData(struct PMProfileRecord *records);

#define PM_MAX_EFFICIENCY_RECORDS 224

/* Read efficiency data */
PMCOMM_API int PM_CALLCONV PMReadEfficiencyData(struct PMConnection *conn, int *nRecords1, struct PMEfficiencyRecord *battery1, int *nRecords2, struct PMEfficiencyRecord *battery2, PMProgressCallback callback, void *usrdata);

#ifdef __cplusplus
}
#endif

#endif
