#ifndef DISPLAYVALUE_H
#define DISPLAYVALUE_H

#include "pmdefs.h"

#include <QString>

/* This class represents a value corresponding to one of the PentaMetric displays.
   It provides a nicer interface than the struct PMDisplayValue
 */
class DisplayValue {

public:
	DisplayValue();
	DisplayValue(enum PMDisplayNumber disp, struct PMDisplayValue &val);
	virtual ~DisplayValue() {}

	virtual double toDouble();
	virtual int getRawIntValue() { return intValue; }

	QString toString();

	enum PMDisplayNumber displayNum();
	bool valid();

	void setPrecision(int p);

	int ampsChannel();

protected:
	enum PMDisplayNumber disp;
	int intValue;

	int digitsTotal();
	int digitsDP();

	int precision;
};

#endif