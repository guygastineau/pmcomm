#include "periodicdata.h"

#include <stdlib.h>
#include <stdio.h>

int PMFormatScientific(unsigned char *data, struct PMScientificValue *result) {
	result->mantissa = data[0] | ((data[1] & 0x3) << 8);
	if(data[1] & 0x80)
		result->mantissa *= -1;

	result->exponent = ((data[1] >> 4) & 0x7) - 3;
	return 0;
}

static int formatTemp(unsigned char *data, int8_t *min, int8_t *max) {
	*min = data[0];
	*max = data[1];
	return 0;
}

static int formatVolts(unsigned char *data, uint16_t *result) {
	*result = data[0] | ((data[1] & 0x7) << 8);
	*result /= 2;
	return 0;
}

static int formatBatteryState(unsigned char *data, struct PMPeriodicBatteryState *result) {
	result->percent = data[0] & 0x7f;
	result->charged = !!(data[0] & 0x80);
	return 0;
}

static int readRecord(unsigned char *data, uint16_t format, int *bytesRead, struct PMPeriodicRecord **records) {
	struct PMPeriodicRecord *record = malloc(sizeof(struct PMPeriodicRecord));
	if(record == NULL)
		return PM_ERROR_ENOMEM;

	record->next = *records;
	*records = record;

	record->validData = 0;
	record->measTime = (data[0] | (data[1] << 8)) * 180 + data[2];
	*bytesRead = 3;

	if(format & PM_PERIODIC_AHR1_VALID) {
		if(PMFormatScientific(data + *bytesRead, &record->ahr1) < 0)
			return PM_ERROR_DATAFORMAT;
		*bytesRead += 2;
	}
	if(format & PM_PERIODIC_AHR2_VALID) {
		if(PMFormatScientific(data + *bytesRead, &record->ahr2) < 0)
			return PM_ERROR_DATAFORMAT;
		*bytesRead += 2;
	}
	if(format & PM_PERIODIC_AHR3_VALID) {
		if(PMFormatScientific(data + *bytesRead, &record->ahr3) < 0)
			return PM_ERROR_DATAFORMAT;
		*bytesRead += 2;
	}
	if(format & PM_PERIODIC_WHR1_VALID) {
		if(PMFormatScientific(data + *bytesRead, &record->whr1) < 0)
			return PM_ERROR_DATAFORMAT;
		*bytesRead += 2;
	}
	if(format & PM_PERIODIC_WHR2_VALID) {
		if(PMFormatScientific(data + *bytesRead, &record->whr2) < 0)
			return PM_ERROR_DATAFORMAT;
		*bytesRead += 2;
	}
	if(format & PM_PERIODIC_TEMP_VALID) {
		if(formatTemp(data + *bytesRead, &record->minTemp, &record->maxTemp) < 0)
			return PM_ERROR_DATAFORMAT;
		*bytesRead += 2;
	}
	if(format & PM_PERIODIC_VOLTS1_VALID) {
		if(formatVolts(data + *bytesRead, &record->volts1) < 0)
			return PM_ERROR_DATAFORMAT;
		*bytesRead += 2;
	}
	if(format & PM_PERIODIC_AMPS1_VALID) {
		if(PMFormatScientific(data + *bytesRead, &record->amps1) < 0)
			return PM_ERROR_DATAFORMAT;
		*bytesRead += 2;
	}
	if(format & PM_PERIODIC_VOLTS2_VALID) {
		if(formatVolts(data + *bytesRead, &record->volts2) < 0)
			return PM_ERROR_DATAFORMAT;
		*bytesRead += 2;
	}
	if(format & PM_PERIODIC_BATTSTATE_VALID) {
		if(formatBatteryState(data + *bytesRead, &record->bat1Percent) < 0)
			return PM_ERROR_DATAFORMAT;
		if(formatBatteryState(data + *bytesRead + 1, &record->bat2Percent) < 0)
			return PM_ERROR_DATAFORMAT;
		*bytesRead += 2;
	}

	record->validData = format;
	return 0;
}

/* Reads a section.  Returns 0 when done, 1 if more data is available, or <0 on error */
static int readSection(unsigned char *buffer, uint16_t *base, uint16_t writePtr, struct PMPeriodicRecord **records) {
	uint16_t readBase = *base;

	// Get format
	uint16_t format = buffer[(readBase & 0xffc0) + 1] | (buffer[(readBase & 0xffc0) + 2] << 8);

	// Compute the top of the current section
	uint16_t topOfPage = 0;
	if((readBase & 0xffc0) == (writePtr & 0xffc0)) {
		topOfPage = 0x3f & writePtr;
	} else {
		uint16_t nextPage = 0;
		if(readBase != 0x1cc0) {
			nextPage = (readBase & 0xffc0) + 0x40;
		}
		topOfPage = buffer[nextPage];
	}

	// Increment past format
	readBase += 3;

	int error = 0;
	// Read all records in the section
	while(1) {
		int bytesRead;
		error = readRecord(buffer + readBase, format, &bytesRead, records);
		if(error < 0) {
			return error;
		}

		// Figure out what to do next
		if((readBase & 0xffc0) == (writePtr & 0xffc0)) {
			if(readBase < writePtr) {
				readBase += bytesRead;
			} else {
				return 0;
			}
		} else {
			if((readBase & 0x3f) < topOfPage) {
				readBase += bytesRead;
			} else {
				// Update the base pointer
				if((readBase & 0xffc0) >= 0x1cc0) {
					*base = 0;
				} else {
					*base = (*base & 0xffc0) + 0x40;
				}
				return 1;
			}
		}
	}	
}

PMCOMM_API void PM_CALLCONV PMFreePeriodicData(struct PMPeriodicRecord *records) {
	while(records != NULL) {
		struct PMPeriodicRecord *next = records->next;
		free(records);
		records = next;
	}
}

int PMFormatPeriodicData(unsigned char *buffer, unsigned char *ptrBuffer, struct PMPeriodicRecord **records) {
	*records = NULL;
	uint16_t writePtr = ptrBuffer[2] | ((ptrBuffer[3] & 0x3f) << 8);
	if(writePtr < 0x300 || writePtr > 0x1fff)
		return 0; // TODO: should this be an error?

	writePtr -= 0x300;

	bool memFull = buffer[0x1cc0];

	if(writePtr == 0x1cc0) {
		return 0;
	}

	uint16_t readBase = 0;
	// Compute where to start reading valid data
	if(memFull && (writePtr & 0xffc0) < 0x1cc0) {
		readBase = (writePtr & 0xffc0) + 0x40;
	}

	int error;
	while(1) {
		error = readSection(buffer, &readBase, writePtr, records);
		if(error < 0) {
			PMFreePeriodicData(*records);
			return error;
		}

		if(error == 0)
			break;
	}
	return 0;
}
