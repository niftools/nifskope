#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

class QListWidget;
class QListWidgetItem;

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit SettingsDialog( QWidget * parent = nullptr );
    ~SettingsDialog();

	//static SettingsDialog * get();

	QListWidget * categories;

public slots:
	void changePage( QListWidgetItem * current, QListWidgetItem * previous );
    
private:
    Ui::SettingsDialog * ui;
};

#endif // SETTINGSDIALOG_H
