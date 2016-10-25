#include "efficiencydata.h"

#include <string.h>
#include <stdio.h>

static int formatRecord(unsigned char *data, struct PMEfficiencyRecord *record, uint32_t *ahc, uint32_t *ahd, uint32_t *prevTime, bool firstRow) {
	memset(record, 0, sizeof(*record));
	record->validData = (data[2] & 0x80) && !firstRow;
	record->endTime = data[0] + (data[1] | ((data[2] & 0x7f) << 8)) * 180;

	uint32_t ahrCharged = data[3] | (data[4] << 8) | (data[5] << 16);
	uint32_t ahrDischarged = data[6] | (data[7] << 8) | (data[8] << 16);

	if(record->validData) {
		record->ahrCharge = (*ahc - ahrCharged) & 0xffffff;
		record->ahrDischarge = (ahrDischarged - *ahd) & 0xffffff;

		record->ahrNet = record->ahrCharge - record->ahrDischarge;
		record->cycleMinutes = record->endTime - *prevTime;

		record->efficiency = (double) record->ahrDischarge  * 100.0 / record->ahrCharge;
		record->selfDischarge = ((double) record->ahrNet / 100) / (record->cycleMinutes / 60.0);
	}

	*ahc = ahrCharged;
	*ahd = ahrDischarged;
	*prevTime = record->endTime;
	return 0;
}

int PMFormatEfficiencyData(unsigned char *buffer, uint16_t pointer, int *nRecords, struct PMEfficiencyRecord *records) {
	*nRecords = 0;

	bool memFull = pointer & 0x8000;
	pointer &= 0x7fff;
	if(pointer > 223)
		return 0; // TODO: should this be an error?

	// Check if data is available
	if(pointer == 0 && !memFull)
		return 0;

	int readPtr = 1;
	if(memFull) {
		if(pointer == 223) {
			readPtr = 0;
		} else {
			readPtr = pointer + 1;
		}
	}

	int error = 0;
	uint32_t ahc, ahd, prevTime;
	bool firstRow = true;
	while(1) {
		int readAddr = (readPtr / 7) * 0x40 + (readPtr % 7) * 9;

		error = formatRecord(buffer + readAddr, records + *nRecords, &ahc, &ahd, &prevTime, firstRow);
		if(error < 0)
			return error;

		(*nRecords)++;
		if(readPtr == pointer) {
			return 0;
		} else {
			if(readPtr == 223) {
				readPtr = 0;
			} else {
				readPtr++;
			}
		}
		firstRow = false;
	}
}

