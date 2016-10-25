#include "profiledata.h"
#include "periodicdata.h"

#include <stdlib.h>

static uint16_t incrementPtr(uint16_t toIncrement) {
	if(toIncrement >= 0xfb6) {
		toIncrement = 0;
	} else {
		if((toIncrement & 0x3f) < 0x37) {
			toIncrement += 5;
		} else {
			toIncrement = (toIncrement & 0xffc0) + 0x40;
		}
	}
	return toIncrement;
}

static int formatRecord(unsigned char *data, struct PMProfileRecord **battery1Records, struct PMProfileRecord **battery2Records) {
	struct PMProfileRecord *record = malloc(sizeof(struct PMProfileRecord));
	if(record == NULL)
		return PM_ERROR_ENOMEM;

	bool battery2 = (data[0] & 0x80);
	record->percentFull = (data[0] & 0x1f) * 5;
	record->volts = (data[1] >> 1) | ((data[2] & 7) << 7);
	record->day = (data[2] >> 3) & 0x1f;
	int error = PMFormatScientific(data + 3, &record->amps);
	if(error < 0) {
		free(record);
		return PM_ERROR_DATAFORMAT;
	}

	struct PMProfileRecord **destList = battery1Records;
	if(battery2)
		destList = battery2Records;

	record->next = *destList;
	*destList = record;

	return 0;
}

PMCOMM_API void PM_CALLCONV PMFreeProfileData(struct PMProfileRecord *records) {
	while(records != NULL) {
		struct PMProfileRecord *next = records->next;
		free(records);
		records = next;
	}
}

int PMFormatProfileData(unsigned char *buffer, unsigned char *ptrBuffer, struct PMProfileRecord **battery1Records, struct PMProfileRecord **battery2Records) {
	*battery1Records = NULL;
	*battery2Records = NULL;

	uint16_t writePtr = ptrBuffer[1] | (ptrBuffer[2] << 8);
	bool memFull = writePtr & 0x8000;
	writePtr &= 0x3fff;
	if(writePtr < 0x2000 || writePtr > 0x2fbf)
		return 0; // TODO: should this be an error?
	
	writePtr -= 0x2000;

	if(!memFull && writePtr == 0) {
		return 0;
	}

	uint16_t readPtr = 5;
	if(memFull) {
		readPtr = incrementPtr(writePtr);
	}

	int error;
	while(1) {
		error = formatRecord(buffer + readPtr, battery1Records, battery2Records);
		if(error < 0) {
			PMFreeProfileData(*battery1Records);
			PMFreeProfileData(*battery2Records);
			return error;
		}

		if(readPtr == writePtr) {
			return 0;
		} else {
			readPtr = incrementPtr(readPtr);
		}
	}
	return 0;
}
