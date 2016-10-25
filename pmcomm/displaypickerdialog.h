#ifndef DISPLAYPICKERDIALOG_H
#define DISPLAYPICKERDIALOG_H

#include "pmdefs.h"

#include <QObject>
#include <QDialog>

class DisplayPicker;
class SiteManager;

class DisplayPickerDialog : public QDialog {
	Q_OBJECT

public:
	DisplayPickerDialog(SiteManager *manager, QWidget *parent = 0);
	DisplayPicker *getPicker() { return picker; }

private:
	DisplayPicker *picker;
};

#endif
