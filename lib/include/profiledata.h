#ifndef PROFILEDATA_H
#define PROFILEDATA_H

#include "pmdefs.h"

#include <stdint.h>

int PMFormatProfileData(unsigned char *buffer, unsigned char *ptrBuffer, struct PMProfileRecord **battery1Records, struct PMProfileRecord **battery2Records);

#endif