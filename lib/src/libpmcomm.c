#include "libpmcomm.h"
#include "displayformat.h"
#include "programformat.h"
#include "pmconnection.h"
#include "periodicdata.h"
#include "profiledata.h"
#include "efficiencydata.h"

#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

/* Returns the address for a given display number */
static int displayAddr(struct PMConnection *conn, enum PMDisplayNumber display) {
	switch(display) {
		case PM_D1: return 1;
		case PM_D2: return 2;
		case PM_D3: return 3;
		case PM_D4: return 4;
		case PM_D7: return 5;
		case PM_D8: return 6;
		case PM_D9: return 7;
		case PM_D10: return 8;
		case PM_D11: return 9;
		case PM_D12: return 10;
		case PM_D13: return 12;
		case PM_D14: return 13;
		case PM_D15: return 15;
		case PM_D16: return 18;
		case PM_D17: return 19;
		case PM_D18: return 23;
		case PM_D19: return 24;
		case PM_D20: return 21;
		case PM_D21: return 22;
		case PM_D22: return 26;
		case PM_D23: return 27;
		case PM_D24: return 28;
		case PM_D25: return 29;
		case PM_D26: return 30;
		case PM_D27: return 31;
		case PM_D28: return 25;
		case PM_D29_34: return 218;
		case PM_DALARM: return 37;
		case PM_D35_40: return 215;
		default: return -1;
	}
}

/* Returns the PentaMetric memory address for a given program number PROG */
static int programAddr(struct PMConnection *conn, enum PMProgramNumber prog) {
	switch(prog) {
		case PM_P1: return 0xff;
		case PM_P2: return 0xfe;
		case PM_P3: return 0xfd;
		case PM_P4: return 0xfc;
		case PM_P5: return 0xfb;
		case PM_P6_9: return 0xe1;
		case PM_P11_13: return 0xf6;
		case PM_P_VERSION: return 0xf7;
		case PM_P14: return 0xf2;
		case PM_P15: return 0xf1;
		case PM_P16: return 0xf3;
		case PM_P17_20: return 0xf0;
		case PM_P22_23: return 0xce;
		case PM_P24_25: return 0xcd;
		case PM_P26: return 0xec;
		case PM_P27: return 0xea;
		case PM_P28: return 0xeb;
		case PM_P29: return 0xe9;
		case PM_P30_31: return 0xd4;
		case PM_P32: return 0xe8;
		case PM_P33: return 0xe7;
		case PM_P34_35: return 0xe5;
		case PM_P36: return 0xe3;
		case PM_P37: return 0xe2;
		case PM_P38: return 0xf9;
		case PM_P39: return 0xcf;
		case PM_P40: return 0xd0;
		case PM_P41_42: return 0xd2;
		case PM_P43: return 0xd1;
		case PM_P_TCP: return 0x90;
		case PM_P_TCP_2: return 0x91;
		case PM_P_TCP_NETBIOS: return 0x92;
		case PM_P_TCP_PASSWORD: return 0x93;
		default: return -1;
	}
}

/* Returns the number of bytes that should be read/written from a given address ADDR */
int PMDataLen(struct PMConnection *conn, int addr) {
	switch(addr) {
		// Displays
		case 1:
		case 2:
		case 3:
		case 4: return 2;
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
		case 11:
		case 12:
		case 13: return 3;
		case 15: return 4;
		case 18:
		case 19:
		case 23:
		case 24: return 3;
		case 21:
		case 22: return 4;
		case 26:
		case 27: return 1;
		case 28:
		case 29:
		case 30:
		case 31: return 2;
		case 25: return 1;
		case 218: return 12;
		case 37: return 2;
		case 215: return 12;
		
		// Reset
		case 39: return 1;
		
		// Programs:
		case 0xff:
		case 0xfe:
		case 0xfd:
		case 0xfc:
		case 0xfb: return 5;
		case 0xe1:
		case 0xf6: return 3;
		case 0xf7: return 1;
		case 0xf2:
		case 0xf1: return 2;
		case 0xf3: return 1;
		case 0xf0: return 7;
		case 0xce:
		case 0xcd: return 2;
		case 0xec: return 3;
		case 0xea: return 2;
		case 0xeb: return 3;
		case 0xe9: return 2;
		case 0xd4: return 6;
		case 0xe8:
		case 0xe7:
		case 0xe5: return 3;
		case 0xe3:
		case 0xe2: return 1;
		case 0xf9: return 2;
		case 0x24: return 1;
		case 0xcf: return 3;
		case 0xd0: return 1;
		case 0xd2: return 2;
		case 0xd1: return 1;

		case 0x90:
		case 0x91:
		case 0x92:
		case 0x93: return 16; // Even though all bytes aren't used, these have 16 allocated

		// Special cases for logged data pointers:
		case 0x1d1: return 3;
		case 0x1d2: return 4;
		
		// Invalid:
		default: return -1;
	}
}

/* Computes and returns the checksum for buffer BUF of length LEN. */
static unsigned char computeCSum(int len, void *buf) {
	unsigned char csum = 0;
	int i;
	for(i = 0; i < len; i++) {
		csum += ((unsigned char *)buf)[i];
	}
	return 255 - csum;
}

/* Maximum number of pages to read per read command.Should not be set higher than 4. */
#define MAX_PAGES_READ 1

/* Performs a long read operation, reading PAGESLEN pages starting at BASEPAGE into BUF.
   BUF must be large enough to handle PAGESLEN * 256 bytes.Returns 0 on success, < 0 on error. */
static int PMReadLong(struct PMConnection *conn, int basepage, int pageslen, void *buf, PMProgressCallback callback, void *usrdata) {
	// Long read
	unsigned char *response = malloc(MAX_PAGES_READ * 256 + 2);
	if(response == NULL)
		return PM_ERROR_ENOMEM;

	int useCookie = IsConnectionInet(conn) ? 1 : 0;

	int i;
	for(i = 0; i < pageslen; i += MAX_PAGES_READ) {
		int npages = pageslen - i;
		if(npages > MAX_PAGES_READ) {
			npages = MAX_PAGES_READ;
		}

		if(callback) {
			callback(i, pageslen, usrdata);
		}
			
		unsigned char request[5];

		int datalen = 4;
		if(useCookie) {
			request[0] = 0;
			datalen = 5;
		}

		request[0 + useCookie] = 0xc1;
		request[1 + useCookie] = basepage + i;
		request[2 + useCookie] = npages;
		request[3 + useCookie] = computeCSum(3 + useCookie, request);
		
		int status = sendBytes(conn, datalen, request);
		if(status < 0) {
			free(response);
			return status;
		}
		
		int bytestoget = npages * 256 + useCookie + 1;
		status = receiveBytes(conn, bytestoget, response);
		if(status < 0) {
			free(response);
			return status;
		}
		
		if(computeCSum(bytestoget, response) != 0) {
			free(response);
			return PM_ERROR_BADRESPONSE;
		}
		
		memcpy(buf, response + useCookie, npages * 256);
		buf = (char *) buf + npages * 256;
	}

	free(response);
	return 0;
}

/* Performs a short read at location ADDR into BUF.BUF must be large
   enough to hold the correct number of bytes (as returned by PMDataLen()); 16 bytes
   is always sufficient.Returns 0 on success, < 0 on error. */
static int PMReadRaw(struct PMConnection *conn, int addr, void *buf) {
	int len = PMDataLen(conn, addr);
	if(len < 0)
		return PM_ERROR_BADREQUEST;
	
	int useCookie = IsConnectionInet(conn) ? 1 : 0;
	
	unsigned char request[5];
	unsigned char response[18];

	int datalen = 4;
	if(useCookie) {
		request[0] = 0;
		datalen = 5;
	}

	request[0 + useCookie] = 0x81;
	request[1 + useCookie] = addr;
	request[2 + useCookie] = len;
	request[3 + useCookie] = computeCSum(3 + useCookie, request);
	
	// send
	int error = sendBytes(conn, datalen, request);
	if(error < 0) {
		return error;
	}
	
	// receive
	datalen = len + 1 + useCookie;
	error = receiveBytes(conn, datalen, response);
	if(error < 0) {
		return error;
	}
	
	if(computeCSum(datalen, response) != 0) {
		return PM_ERROR_BADRESPONSE;
	}
	
	if(useCookie && (response[0] != request[0])) {
		return PM_ERROR_BADRESPONSE;
	}
	
	memcpy(buf, response + useCookie, len);
	return 0;
}

/* Performs a short write to location ADDR from BUF.BUF must contain at least
   the correct number of bytes for location ADDR (as returned by PMDataLen()).
   Returns 0 on success, < 0 on error. */
static int PMWriteRaw(struct PMConnection *conn, int addr, void *buf) {
	// Short write
	int len = PMDataLen(conn, addr);
	if(len < 0)
		return PM_ERROR_BADREQUEST;
		
	int useCookie = IsConnectionInet(conn) ? 1 : 0;
	
	unsigned char request[21];
	unsigned char response[2];
	
	int datalen = len + 4;
	if(useCookie) {
		request[0] = 0;
		datalen = len + 5;
	}

	request[0 + useCookie] = 0x1;
	request[1 + useCookie] = addr;
	request[2 + useCookie] = len;
	memcpy(request + 3 + useCookie, buf, len);
	request[len + 3 + useCookie] = computeCSum(len + 3 + useCookie, request);
	
	// send
	int status = sendBytes(conn, datalen, request);
	if(status < 0) {
		return status;
	}
	
	// receive
	datalen = 1 + useCookie;
	status = receiveBytes(conn, datalen, response);
	if(status < 0) {
		return status;
	}
	
	if(response[useCookie] != request[len + 3 + useCookie]) {
		return PM_ERROR_BADRESPONSE;
	}
	
	if(useCookie && (response[0] != request[0])) {
		return PM_ERROR_BADRESPONSE;
	}

	return 0;
}

/* Reads and formats the data for display DISPLAY and stores the result in *RESULT. All data is represented
   as an integer; see the additional documentation for a description of the formats.
   Returns 0 on success, <0 on error */
PMCOMM_API int PM_CALLCONV PMReadDisplayFormatted(struct PMConnection *conn, enum PMDisplayNumber display, struct PMDisplayValue *result) {
	int addr = displayAddr(conn, display);
	if(addr < 0)
		return PM_ERROR_BADREQUEST;

	enum PMDisplayFormat format;
	format = PMDataDisplayFormat(conn, addr);
	if(format == PM_FORMAT_INVALID)
		return PM_ERROR_BADREQUEST;
	
	unsigned char buf[16];
	int error = PMReadRaw(conn, addr, buf);
	if(error < 0)
		return error;

	bool largeShunt = false;
		
	error = PMFormatDisplayData(buf, (unsigned char *) &largeShunt, &result->val, format);
	return error;
}

/* Reads and formats the data for program PROG and stores the result in *RESULT. Data is represented
   as a PMProgramData union, defined in pmdefs.h; see the additional documentation for a description of the formats.
   Returns 0 on success, <0 on error */
PMCOMM_API int PM_CALLCONV PMReadProgramFormatted(struct PMConnection *conn, enum PMProgramNumber prog, union PMProgramData *result) {
	int addr = programAddr(conn, prog);
	if(addr < 0)
		return PM_ERROR_BADREQUEST;

	enum PMProgramFormat format = PMProgramDataFormat(conn, addr);
	if(format == PM_PROGFORMAT_INVALID)
		return PM_ERROR_BADREQUEST;

	unsigned char buf[16], buf2[16];
	int error = PMReadRaw(conn, addr, buf);
	if(error < 0)
		return error;

	if(prog == PM_P38) {
		// Get minutes
		error = PMReadRaw(conn, 0x24, buf2);
		if(error < 0)
			return error;
	}

	error = PMFormatProgramData(buf, buf2, result, format);
	if(error < 0)
		return PM_ERROR_DATAFORMAT;
	return 0;
}

/* Writes the data into program PROG using the input in *INPUT. Data is represented as a PMProgramData union,
   defined in pmdefs.h; see the additional documentation for a description of the formats.
   Returns 0 on success, <0 on error */
PMCOMM_API int PM_CALLCONV PMWriteProgramFormatted(struct PMConnection *conn, enum PMProgramNumber prog, union PMProgramData *input) {
	int addr = programAddr(conn, prog);
	if(addr < 0)
		return PM_ERROR_BADREQUEST;

	enum PMProgramFormat format = PMProgramDataFormat(conn, addr);
	if(format == PM_PROGFORMAT_INVALID)
		return PM_ERROR_BADREQUEST;

	unsigned char buf[16], buf2[16];
	memset(buf, 0, 16);
	memset(buf2, 0, 16);
	int error;
	if(prog == PM_P43) {
		error = PMReadRaw(conn, 0xd1, buf2);
		if(error < 0)
			return error;
	}

	error = PMEncodeProgramData(buf, buf2, input, format);
	if(error < 0)
		return PM_ERROR_DATAFORMAT;

	if(prog == PM_P38) {
		// Set minutes
		error = PMWriteRaw(conn, 0x24, buf2);
		if(error < 0)
			return error;
	}

	error = PMWriteRaw(conn, addr, buf);
	return error;
}

/* Reads efficiency for a battery.  BATTERY2 should be TRUE for battery 2 data, FALSE for battery 1 data.
   RESULTS must be array of three PMEfficiency structs into which the results are placed.
   Returns 0 on success, <0 on error */
PMCOMM_API int PM_CALLCONV PMReadEfficiencyFormatted(struct PMConnection *conn, bool battery2, struct PMEfficiency *results) {
	int addr = 218;
	if(battery2)
		addr = 215;

	unsigned char buf[16];
	int error = PMReadRaw(conn, addr, buf);
	if(error < 0)
		return error;

	error = PMFormatDisplayEfficiencyData(buf, results);
	return error;
}

/* Returns the value to write into address 0x27 to reset the type of data indicated by RESRET. */
static int getResetValue(enum PMResetType reset) {
	switch(reset) {
		case PM_RESET_AHR1: return 0x09;
		case PM_RESET_AHR2: return 0x0a;
		case PM_RESET_AHR3: return 0x0b;
		case PM_RESET_CAH1: return 0xb0;
		case PM_RESET_CAH2: return 0xb1;
		case PM_RESET_WHR1: return 0x11;
		case PM_RESET_WHR2: return 0x12;
		case PM_RESET_BAT1_DSC: return 0x19;
		case PM_RESET_BAT2_DSC: return 0x1a;
		case PM_RESET_BAT1_DSE: return 0x1b;
		case PM_RESET_BAT2_DSE: return 0x1c;

		case PM_RESET_PERIODIC: return 0x72;
		case PM_RESET_DISCHARGE: return 0x82;
		case PM_RESET_BAT1_EFF: return 0x90;
		case PM_RESET_BAT2_EFF: return 0x91;
		case PM_RESET_PROGRAM: return 0xa5;

		default: return -1;
	}
}

PMCOMM_API int PM_CALLCONV PMReset(struct PMConnection *conn, enum PMResetType reset) {
	int value = getResetValue(reset);
	if(value < 0)
		return PM_ERROR_BADREQUEST;

	unsigned char buf[1];
	buf[0] = value;

	int error = PMWriteRaw(conn, 0x27, buf);
	return error;
}

/* These aid in debugging logged data formatting. They should not be enabled normally since they will
   try to read or write from the current working directory of the program. */
// #define DEBUG_FILE_WRITE // writes raw data to a file for debugging
// #define DEBUG_FILE_READ // reads raw data back from a file for debugging (instead of reading normally)

PMCOMM_API int PM_CALLCONV PMReadPeriodicData(struct PMConnection *conn, struct PMPeriodicRecord **records, PMProgressCallback callback, void *usrdata) {
	*records = NULL;

	unsigned char *buffer = malloc(29 * 256);
	if(buffer == NULL)
		return PM_ERROR_ENOMEM;

	unsigned char ptrBuffer[16];

#ifdef DEBUG_FILE_READ
	int error = 0;
	FILE *rawFile = fopen("periodic.raw", "rb");
	if(!rawFile) error = -1;
	if(!error) error = fread(buffer, 256, 29, rawFile) == 0;
	fclose(rawFile);

	FILE *ptrFile = fopen("periodicptr.raw", "rb");
	if(!ptrFile) error = -1;
	if(!error) error = fread(ptrBuffer, 4, 1, ptrFile) == 0;
	fclose(ptrFile);

	if(error != 0) {
		free(buffer);
		return PM_ERROR_OTHER;
	}
#else
	int error = PMReadRaw(conn, 0x1d2, ptrBuffer);
	if(error < 0) {
		free(buffer);
		return error;
	}

	error = PMReadLong(conn, 0x3, 29, buffer, callback, usrdata);
	if(error < 0) {
		free(buffer);
		return error;
	}

#ifdef DEBUG_FILE_WRITE
	FILE *rawFile = fopen("periodic.raw", "wb");
	if(!rawFile) error = -1;
	if(!error) error = fwrite(buffer, 256, 29, rawFile) == 0;
	fclose(rawFile);

	FILE *ptrFile = fopen("periodicptr.raw", "wb");
	if(!ptrFile) error = -1;
	if(!error) error = fwrite(ptrBuffer, 4, 1, ptrFile) == 0;
	fclose(ptrFile);

	if(error != 0) {
		free(buffer);
		return PM_ERROR_OTHER;
	}
#endif // DEBUG_FILE_WRITE
#endif // DEBUG_FILE_READ

	error = PMFormatPeriodicData(buffer, ptrBuffer, records);
	free(buffer);
	return error;
}

PMCOMM_API int PM_CALLCONV PMReadProfileData(struct PMConnection *conn, struct PMProfileRecord **battery1Records, struct PMProfileRecord **battery2Records, PMProgressCallback callback, void *usrdata) {
	*battery1Records = NULL;
	*battery2Records = NULL;

	unsigned char *buffer = malloc(16 * 256);
	if(buffer == NULL)
		return PM_ERROR_ENOMEM;

	unsigned char ptrBuffer[16];
#ifdef DEBUG_FILE_READ
	int error = 0;
	FILE *rawFile = fopen("profile.raw", "rb");
	if(!rawFile) error = -1;
	if(!error) error = fread(buffer, 256, 16, rawFile) == 0;
	fclose(rawFile);

	FILE *ptrFile = fopen("profileptr.raw", "rb");
	if(!ptrFile) error = -1;
	if(!error) error = fread(ptrBuffer, 3, 1, ptrFile) == 0;
	fclose(ptrFile);

	if(error) {
		free(buffer);
		return PM_ERROR_OTHER;
	}
#else
	int error = PMReadRaw(conn, 0x1d1, ptrBuffer);
	if(error < 0) {
		free(buffer);
		return error;
	}

	error = PMReadLong(conn, 0x20, 16, buffer, callback, usrdata);
	if(error < 0) {
		free(buffer);
		return error;
	}

#ifdef DEBUG_FILE_WRITE
	FILE *ptrFile = fopen("profileptr.raw", "wb");
	if(!ptrFile) error = -1;
	if(!error) error = fwrite(ptrBuffer, 3, 1, ptrFile) == 0;
	fclose(ptrFile);

	FILE *rawFile = fopen("profile.raw", "wb");
	if(!rawFile) error = -1;
	if(!error) error = fwrite(buffer, 256, 16, rawFile) == 0;
	fclose(rawFile);

	if(error != 0) {
		free(buffer);
		return PM_ERROR_OTHER;
	}
#endif // DEBUG_FILE_WRITE
#endif // DEBUG_FILE_READ

	error = PMFormatProfileData(buffer, ptrBuffer, battery1Records, battery2Records);
	free(buffer);
	return error;
}

struct effStatus {
	PMProgressCallback callback;
	void *usrdata;
	int totalPages;
	int basePages;
};

static void PM_CALLCONV effCallback(int progress, int outof, void *usrdata) {
	struct effStatus *status = usrdata;
	status->callback(status->basePages + progress, status->totalPages, status->usrdata);
}

PMCOMM_API int PM_CALLCONV PMReadEfficiencyData(struct PMConnection *conn, int *nRecords1, struct PMEfficiencyRecord *battery1, int *nRecords2, struct PMEfficiencyRecord *battery2, PMProgressCallback callback, void *usrdata) {
	unsigned char *buffer = malloc(2048); // Big enough for all data for each battery
	if(buffer == NULL)
		return PM_ERROR_ENOMEM;

	struct effStatus status;
	status.callback = callback;
	status.usrdata = usrdata;
	status.totalPages = 1 + (battery1 ? 8 : 0) + (battery2 ? 8 : 0);
	status.basePages = 0;

	// Read the pointers
	int error = PMReadLong(conn, 0x2f, 1, buffer, callback ? effCallback : NULL, &status);
	if(error < 0) {
		free(buffer);
		return error;
	}

#ifdef DEBUG_FILE_WRITE
	FILE *ptrFile = fopen("efficiencyptr.raw", "wb");
	if(!ptrFile) error = -1;
	if(!error) error = fwrite(buffer + 0xfc, 4, 1, ptrFile) == 0;
	fclose(ptrFile);

	if(error != 0) {
		free(buffer);
		return PM_ERROR_OTHER;
	}
#endif

	uint16_t battery1ptr = buffer[0xfc] | (buffer[0xfd] << 8);
	uint16_t battery2ptr = buffer[0xfe] | (buffer[0xff] << 8);

	if(battery1) {
		error = PMReadLong(conn, 0x30, 8, buffer, callback ? effCallback : NULL, &status);
		status.basePages = 8;
		if(error < 0) {
			free(buffer);
			return error;
		}

#ifdef DEBUG_FILE_WRITE
		FILE *rawFile = fopen("efficiency.raw", "wb");
		if(!rawFile) error = -1;
		if(!error) error = fwrite(buffer, 256, 8, rawFile) == 0;
		fclose(rawFile);

		if(error != 0) {
			free(buffer);
			return PM_ERROR_OTHER;
		}
#endif

		error = PMFormatEfficiencyData(buffer, battery1ptr, nRecords1, battery1);
		if(error < 0) {
			free(buffer);
			return error;
		}
	}

	if(battery2) {
		error = PMReadLong(conn, 0x38, 8, buffer, callback ? effCallback : NULL, &status);
		if(error < 0) {
			free(buffer);
			return error;
		}
		error = PMFormatEfficiencyData(buffer, battery2ptr, nRecords2, battery2);
		if(error < 0) {
			free(buffer);
			return error;
		}
	}

	free(buffer);
	return 0;
}
