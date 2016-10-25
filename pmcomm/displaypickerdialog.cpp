#include "displaypickerdialog.h"

#include "displaypicker.h"
#include "sitemanager.h"

#include <QVBoxLayout>
#include <QDialogButtonBox>

DisplayPickerDialog::DisplayPickerDialog(SiteManager *manager, QWidget *parent) : QDialog(parent) {
	setWindowTitle("Select displays");
	setWindowModality(Qt::WindowModal);
	setMinimumSize(600, 500);
	QVBoxLayout *layout = new QVBoxLayout(this);

	picker = new DisplayPicker(manager);
	picker->addDefaultDisplays();
	layout->addWidget(picker);

	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
	connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

	layout->addWidget(buttonBox);
}