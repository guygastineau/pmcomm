#include "displayformat.h"

#include <stdint.h>
#include <stdbool.h>

enum PMDisplayFormat PMDataDisplayFormat(struct PMConnection *conn, int addr) {
	switch(addr) {
		case 1:
		case 2:
		case 3:
		case 4: return PM_FORMAT_1;
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
		case 10: return PM_FORMAT_2;
		case 12:
		case 13: return PM_FORMAT_3;
		case 15: return PM_FORMAT_4;
		case 18:
		case 19: return PM_FORMAT_2B;
		case 23:
		case 24: return PM_FORMAT_2;
		case 21:
		case 22: return PM_FORMAT_5;
		case 26:
		case 27: return PM_FORMAT_6;
		case 28:
		case 29:
		case 30:
		case 31: return PM_FORMAT_7;
		case 25: return PM_FORMAT_8;
		case 218: return PM_FORMAT_EFF;
		case 37: return PM_FORMAT_ALARM;
		case 215: return PM_FORMAT_EFF;
		
		default: return PM_FORMAT_INVALID;
	}
}

int PMFormatDisplayData(unsigned char *data, unsigned char *data2, int32_t *result, enum PMDisplayFormat format) {
	switch(format) {
		case PM_FORMAT_1:
			*result = (data[0] >> 1) | ((data[1] & 7) << 7);
			return 0;

		case PM_FORMAT_2:
		case PM_FORMAT_2B:
			*result = data[0] | (data[1] << 8) | (data[2] << 16);
			if(data[2] & 0x80) {
				*result |= 0xff000000;
			}
			*result *= -1;
			if(data2[0]) {
				// Round
				*result = ((*result + 5) / 10) * 10;
			}
			return 0;

		case PM_FORMAT_3:
			*result = data[0] | (data[1] << 8) | (data[2] << 16);
			if(data[2] & 0x80) {
				*result |= 0xff000000;
			}
			*result *= -1;
			return 0;

		case PM_FORMAT_4:
			*result = (data[0] >> 7) | (data[1] << 1) | (data[2] << 9) | (data[3] << 17);
			if(data[3] & 0x80) {
				*result |= 0xfe000000;
			}
			*result *= -1;
			return 0;

		case PM_FORMAT_5:
			*result = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
			*result *= -1;
			return 0;

		case PM_FORMAT_6:
			*result = data[0];
			return 0;

		case PM_FORMAT_7:
			*result = data[0] | (data[1] << 8);
			return 0;

		case PM_FORMAT_8:
			*result = ((signed char *)data)[0];
			return 0;

		case PM_FORMAT_EFF:
			return PM_ERROR_BADREQUEST;

		case PM_FORMAT_ALARM:
			*result = (data[0] & 0x9f) | ((data[1] & 0x1f) << 8);
			return 0;
		
		default:
			return PM_ERROR_BADREQUEST;
	}
}

int PMFormatDisplayEfficiencyData(unsigned char *data, struct PMEfficiency *results) {
	int i;
	for(i = 0; i < 3; i++) {
		results[i].selfdischarge = data[4 * i] | (data[4 * i + 1] & 0x3);
		if(data[4 * i + 1] & 0x2)
			results[i].selfdischarge |= 0xfc;

		results[i].percent = data[4 * i + 2];
		results[i].numCycles = data[4 * i + 3];
	}
	return 0;
}
