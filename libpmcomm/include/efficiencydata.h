#ifndef EFFICIENCYDATA_H
#define EFFICIENCYDATA_H

#include "pmdefs.h"

#include <stdint.h>

int PMFormatEfficiencyData(unsigned char *buffer, uint16_t pointer, int *nRecords, struct PMEfficiencyRecord *records);

#endif