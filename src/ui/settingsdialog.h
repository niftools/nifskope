#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QVariant>

#include <memory>


//! @file settingsdialog.h SettingsDialog

class QListWidget;
class QListWidgetItem;
class QPushButton;
class QStackedWidget;

namespace Ui {
class SettingsDialog;
}


class SettingsDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit SettingsDialog( QWidget * parent = nullptr );
    ~SettingsDialog();


	static void registerPage( QWidget *, const QString & );

	QStackedWidget * content;
	QListWidget * categories;

	QVariant settingsVersion;

public slots:
	void save();
	void apply();
	void cancel();
	void changePage( QListWidgetItem * current, QListWidgetItem * previous );
	void restoreDefaults();
	void modified();

signals:
	void loadSettings();
	void saveSettings();
	void localeChanged();
	void update3D();
	void flush3D();
    
private:
	std::unique_ptr<Ui::SettingsDialog> ui;

	QPushButton * btnSave;
	QPushButton * btnApply;
	QPushButton * btnCancel;

	bool eventFilter( QObject *, QEvent * ) override final;
	void showEvent( QShowEvent * ) override final;
};

#endif // SETTINGSDIALOG_H
