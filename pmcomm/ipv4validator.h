#include <QObject>
#include <QValidator>
#include <QStringList>

class IPv4Validator : public QValidator {
	Q_OBJECT

public:
	IPv4Validator(QObject *parent = NULL) : QValidator(parent) {}

	virtual State validate(QString & input, int & pos) const;
};