#include "displayvalue.h"

#include "appsettings.h"

// #include <sstream>
// #include <iomanip>
#include <limits>
// #include <string>
#include <cmath>

// Default constructor constructs an invalid value
DisplayValue::DisplayValue() : disp(PM_DINVALID), intValue(0), precision(0) {}

// Normal constructor. The precision must be set separately
DisplayValue::DisplayValue(enum PMDisplayNumber disp, struct PMDisplayValue &val) : disp(disp), intValue(val.val) {
	precision = 0;
}

// Gets the display number this corresponds to
enum PMDisplayNumber DisplayValue::displayNum() {
	return disp;
}

void DisplayValue::setPrecision(int p) {
	precision = p;
}

// Gets the amps channel that relates to this value, or 0 if none is related
int DisplayValue::ampsChannel() {
	switch(disp) {
		case PM_D7:
		case PM_D10:
		case PM_D16:
		case PM_D18: return 1;
		case PM_D8:
		case PM_D11:
		case PM_D17:
		case PM_D19: return 2;
		case PM_D9:
		case PM_D12: return 3;
		default: return 0;
	}
}

/* Formats the value as a string.  This is somewhat complex, since none of the default
 * floating-point formats allow specifying both a total number of significant digits
 * and a number of digits past the decimal point.
 */
QString DisplayValue::toString() {
	// Special cases
	if(disp == PM_D28 && intValue == -21) // Temperature
		return "Invalid";

	if((disp == PM_D24 || disp == PM_D25) && intValue == 65535) // Days since charged
		return "?";

	double value = toDouble();
	int digTotal = digitsTotal(); // Total number of sig figs
	int digDP = digitsDP(); // Number of digits after the decimal point

	// For temperature
	if(disp == PM_D28) {
		if(AppSettings::getInstance()->getTempUnit() == AppSettings::Fahrenheit)
			value = value * 1.8 + 32;
	}

	// Format manually, since the standard formatting functions don't do quite the right thing
	if(std::isnan(value))
		return "Error";

	bool negative = value < 0; // Check if the number is negative
	value = std::fabs(value);

	// Loop until we have the desired precision
	while(true) {
		// Get an int representation of the part we are interested in, rounded properly
		int asInt = value * std::pow(10.0, digDP) + 0.5;

		QString formatted;
		int digitsUsed = 0;
		// Format the number, continuing until we have passed the decimal point and gotten
		// the whole number
		for(int digit = 0; digit <= digDP || asInt != 0; digit++) {
			if(digit == digDP && digit != 0)
				formatted.prepend('.');
			formatted.prepend(char((asInt % 10) + '0'));
			if(asInt != 0) // Don't count zeros to the left of valid data
				digitsUsed++;
			asInt = asInt / 10;
		}
		// digTotal == 0 means unlimited digits
		if(digTotal == 0 || digitsUsed <= digTotal) {
			if(digDP < 0) { // If we need to add trailing zeros
				formatted.append(QString(-digDP, '0'));
			}

			if(negative)
				formatted.prepend('-');
			return formatted;
		}
		digDP--; // This can go negative, which indicates insignificant trailing zeros
	}
}

// Determines if the display is initialized to a non-invalid display
bool DisplayValue::valid() {
	return disp != PM_DINVALID;
}

// Something to make it clear that the result is invalid
#define BAD_VALUE_NAN std::numeric_limits<double>::quiet_NaN()

// Raw floating-point value. This won't typically be used directly, since the precision
// calculation is tricky
double DisplayValue::toDouble() {
	switch (disp) {
		case PM_D1:
		case PM_D2:
		case PM_D3:
		case PM_D4: return intValue / 10.0;
		case PM_D7:
		case PM_D8:
		case PM_D9:
		case PM_D10:
		case PM_D11:
		case PM_D12:
		case PM_D13:
		case PM_D14:
		case PM_D15: return intValue / 100.0;
		case PM_D16:
		case PM_D17: return intValue;
		case PM_D18:
		case PM_D19:
		case PM_D20:
		case PM_D21: return intValue / 100.0;
		case PM_D22:
		case PM_D23: return intValue;
		case PM_D24:
		case PM_D25:
		case PM_D26:
		case PM_D27: return intValue / 100.0;
		case PM_D28: return intValue;
		case PM_D29_34: return BAD_VALUE_NAN;
		case PM_DALARM: return BAD_VALUE_NAN;
		case PM_D35_40: return BAD_VALUE_NAN;

		default: return BAD_VALUE_NAN;
	}
}

// Returns the total number of significant digits. 0 means unlimited
int DisplayValue::digitsTotal() {
	switch (disp) {
		case PM_D1:
		case PM_D2:
		case PM_D3:
		case PM_D4: return 4;
		case PM_D7:
		case PM_D8:
		case PM_D9:
		case PM_D10:
		case PM_D11:
		case PM_D12: return 3;
		case PM_D13:
		case PM_D14:
		case PM_D15:
		case PM_D16:
		case PM_D17: return 0;
		case PM_D18:
		case PM_D19: return 3;
		case PM_D20:
		case PM_D21: return 0;
		case PM_D22:
		case PM_D23: return 3;
		case PM_D24:
		case PM_D25:
		case PM_D26:
		case PM_D27: return 0;
		case PM_D28: return 0;
		case PM_D29_34: return 0;
		case PM_DALARM: return 0;
		case PM_D35_40: return 0;

		default: return 0;
	}
}

// Returns the number of valid digits to the right of the decimal point
int DisplayValue::digitsDP() {
	switch (disp) {
		case PM_D1:
		case PM_D2:
		case PM_D3:
		case PM_D4: return 1;
		case PM_D7:
		case PM_D8:
		case PM_D9:
		case PM_D10:
		case PM_D11:
		case PM_D12: return precision;
		case PM_D13:
		case PM_D14:
		case PM_D15: return 2;
		case PM_D16:
		case PM_D17: return 0;
		case PM_D18:
		case PM_D19: return 1;
		case PM_D20:
		case PM_D21: return 0;
		case PM_D22:
		case PM_D23: return 0;
		case PM_D24:
		case PM_D25:
		case PM_D26:
		case PM_D27: return 2;
		case PM_D28: return 0;
		case PM_D29_34: return 0;
		case PM_DALARM: return 0;
		case PM_D35_40: return 0;

		default: return 0;
	}
}
