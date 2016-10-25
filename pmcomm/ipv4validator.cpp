#include "ipv4validator.h"

QValidator::State IPv4Validator::validate(QString & input, int & pos) const {
	Q_UNUSED(pos);
	QStringList elements = input.split(".");
	if(elements.size() > 4)
		return Invalid;

	bool emptyGroup = false;
	for(int i = 0; i < elements.size(); i++) {
		if(elements[i].isEmpty() || elements[i] == " ") {
			emptyGroup = true;
			continue;
		}
		bool ok;
		int value = elements[i].toInt(&ok);
		if(!ok || value < 0 || value > 255)
			return Invalid;
	}
	if(elements.size() < 4 || emptyGroup)
		return Intermediate;
	return Acceptable;
}