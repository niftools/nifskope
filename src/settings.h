#ifndef SETTINGSPANE_H
#define SETTINGSPANE_H

#include <QWidget>

class QListWidgetItem;

namespace Ui {
class SettingsGeneral;
}

class SettingsGeneral : public QWidget
{
	Q_OBJECT

public:
	explicit SettingsGeneral( QWidget * parent = 0 );
	~SettingsGeneral();

private:
	Ui::SettingsGeneral * ui;
};


#endif // SETTINGSPANE_H
