#ifndef SETTINGSPANE_H
#define SETTINGSPANE_H

#include <QWidget>
#include <QSettings>

#include <memory>


//! @file settingspane.h SettingsPane, SettingsGeneral, SettingsRender, SettingsResources

class FSManager;
class SettingsDialog;
class QStringListModel;

namespace Ui {
class SettingsGeneral;
class SettingsRender;
class SettingsResources;
}

class SettingsPane : public QWidget
{
	Q_OBJECT

public:
	explicit SettingsPane( QWidget * parent = nullptr );
	virtual ~SettingsPane();

	virtual void read() = 0;
	virtual void write() = 0;
	virtual void setDefault() = 0;

public slots:
	virtual void modifyPane();

signals:
	void paneModified();

protected:
	void readPane( QWidget * w, QSettings & settings );
	void writePane( QWidget * w, QSettings & settings );
	bool isModified();
	void setModified( bool );

	SettingsDialog * dlg;

	bool modified = false;
};

class SettingsGeneral : public SettingsPane
{
	Q_OBJECT

public:
	explicit SettingsGeneral( QWidget * parent = nullptr );
	~SettingsGeneral();

	void read() override final;
	void write() override final;
	void setDefault() override final;

private:
	std::unique_ptr<Ui::SettingsGeneral> ui;
};

class SettingsRender : public SettingsPane
{
	Q_OBJECT

public:
	explicit SettingsRender( QWidget * parent = nullptr );
	~SettingsRender();

	void read() override final;
	void write() override final;
	void setDefault() override final;

private:
	std::unique_ptr<Ui::SettingsRender> ui;
};

class SettingsResources : public SettingsPane
{
	Q_OBJECT

public:
	explicit SettingsResources( QWidget * parent = nullptr );
	~SettingsResources();

	void read() override final;
	void write() override final;
	void setDefault() override final;

public slots:
	void on_btnFolderAdd_clicked();
	void on_btnFolderRemove_clicked();
	void on_btnFolderDown_clicked();
	void on_btnFolderUp_clicked();
	void on_btnFolderAutoDetect_clicked();

	void on_btnArchiveAdd_clicked();
	void on_btnArchiveRemove_clicked();
	void on_btnArchiveDown_clicked();
	void on_btnArchiveUp_clicked();
	void on_btnArchiveAutoDetect_clicked();

private:
	std::unique_ptr<Ui::SettingsResources> ui;

	FSManager * archiveMgr;

	QStringListModel * folders;
	QStringListModel * archives;
};


#endif // SETTINGSPANE_H
