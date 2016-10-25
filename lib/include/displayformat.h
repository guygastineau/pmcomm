#ifndef DISPLAYFORMAT_H
#define DISPLAYFORMAT_H

#include "pmdefs.h"

struct PMConnection;

enum PMDisplayFormat {
	PM_FORMAT_1,
	PM_FORMAT_2,
	PM_FORMAT_2B,
	PM_FORMAT_3,
	PM_FORMAT_4,
	PM_FORMAT_5,
	PM_FORMAT_6,
	PM_FORMAT_7,
	PM_FORMAT_8,
	PM_FORMAT_EFF,
	PM_FORMAT_ALARM,
	
	PM_FORMAT_INVALID
};

enum PMDisplayFormat PMDataDisplayFormat(struct PMConnection *conn, int addr);
int PMFormatDisplayData(unsigned char *data, unsigned char *data2, int32_t *result, enum PMDisplayFormat format);
int PMFormatDisplayEfficiencyData(unsigned char *data, struct PMEfficiency *results);

#endif