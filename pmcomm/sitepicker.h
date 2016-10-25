#ifndef SITEPICKER_H
#define SITEPICKER_H

#include <QDialog>
#include <QObject>
#include <QList>

class SitesList;

class QListWidget;
class QRadioButton;

class SitePicker : public QDialog {
	Q_OBJECT

public:
	SitePicker(QWidget *parent = 0);

	void setData(SitesList *sitesList, QList<int> &selectedSites);
	void setShowSelected(bool flag);

	void getSelected(QList<int> & selectedSites);
	bool isShowSelected();

private:
	QListWidget *list;
    QRadioButton *currentRadio;
    QRadioButton *listRadio;
};


#endif
