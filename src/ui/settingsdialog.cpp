#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include "settings.h"

#include <QDebug>
#include <QListWidget>


//SettingsDialog * SettingsDialog::get()
//{
//	static SettingsDialog * settings = new SettingsDialog();
//
//	return settings;
//}

SettingsDialog::SettingsDialog( QWidget * parent ) :
    QDialog( parent ),
    ui( new Ui::SettingsDialog )
{
    ui->setupUi( this );

	categories = ui->categoryList;

	setWindowTitle( tr( "Settings" ) );
	setWindowFlags( Qt::Tool | Qt::WindowStaysOnTopHint );
	installEventFilter( this );

	//for ( int i = 0; i < categories->count(); i++ ) {
		//ui->contentWidget->addWidget( new SettingsPane( this ) );
	//}


	ui->contentWidget->addWidget( new SettingsGeneral( this ) );
	//ui->contentWidget->addWidget( new SettingsPane( this ) );
	//ui->contentWidget->addWidget( new SettingsPane( this ) );

	categories->setCurrentRow( 0 );

	connect( ui->categoryList, &QListWidget::currentItemChanged, this, &SettingsDialog::changePage );
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::changePage( QListWidgetItem * current, QListWidgetItem * previous )
{
	if ( !current )
		current = previous;

	ui->contentWidget->setCurrentIndex( categories->row( current ) );
}