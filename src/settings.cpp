#include "settings.h"

#include "ui/settingsdialog.h"

#include "ui_settingsgeneral.h"

#include <QDebug>
#include <QListWidget>



SettingsGeneral::SettingsGeneral( QWidget * parent ) :
	QWidget( parent ),
	ui( new Ui::SettingsGeneral )
{
	ui->setupUi( this );

	auto p = qobject_cast<SettingsDialog *>(parent);

	p->categories->addItem( ui->label->text() );

}

SettingsGeneral::~SettingsGeneral()
{
	delete ui;
}


