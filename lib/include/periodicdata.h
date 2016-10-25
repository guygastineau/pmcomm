#ifndef PERIODICDATA_H
#define PERIODICDATA_H

#include "pmdefs.h"

int PMFormatPeriodicData(unsigned char *buffer, unsigned char *ptrBuffer, struct PMPeriodicRecord **records);
int PMFormatScientific(unsigned char *data, struct PMScientificValue *result);

#endif