#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include "ui/settingspane.h"

#include <QDebug>
#include <QListWidget>
#include <QSettings>
#include <QStatusBar>


//! @file settingsdialog.cpp SettingsDialog

SettingsDialog::SettingsDialog( QWidget * parent ) :
    QDialog( parent ),
    ui( new Ui::SettingsDialog )
{
    ui->setupUi( this );

	content = ui->content;
	categories = ui->categoryList;

	setWindowTitle( tr( "Settings" ) );
	setWindowFlags( Qt::Tool | Qt::WindowStaysOnTopHint );
	installEventFilter( this );

	content->addWidget( new SettingsGeneral( this ) );
	content->addWidget( new SettingsRender( this ) );
	content->addWidget( new SettingsResources( this ) );

	categories->setCurrentRow( 0 );

	btnSave = ui->submit->button( QDialogButtonBox::Save );
	btnSave->setEnabled( false );

	btnApply = ui->submit->button( QDialogButtonBox::Apply );
	btnApply->setEnabled( false );

	btnCancel = ui->submit->button( QDialogButtonBox::Cancel );
	btnCancel->setEnabled( true );
	btnCancel->setText( tr("Close") );

	QSettings settings;

	settingsVersion = settings.value( "Settings/Version" );
	if ( settingsVersion.isNull() ) {
		// First time install, save defaults to registry
		save();
		settings.setValue( "Settings/Version", 1.0 );
	}

	connect( ui->categoryList, &QListWidget::currentItemChanged, this, &SettingsDialog::changePage );
	connect( ui->submit, &QDialogButtonBox::accepted, this, &SettingsDialog::save );
	connect( ui->submit, &QDialogButtonBox::clicked, [this]( QAbstractButton * btn ) {
		if ( btn == btnApply )
			apply();
	} );
	connect( ui->submit, &QDialogButtonBox::rejected, this, &SettingsDialog::cancel );
	connect( ui->restoreAllDefaults, &QPushButton::clicked, this, &SettingsDialog::restoreDefaults );
}

SettingsDialog::~SettingsDialog()
{
}


void SettingsDialog::registerPage( QWidget * parent, const QString & text )
{
	auto p = qobject_cast<SettingsDialog *>(parent);
	if ( p )
		p->categories->addItem( text );
}

void SettingsDialog::apply()
{
	emit saveSettings();
	emit update3D();

	btnSave->setEnabled( false );
	btnApply->setEnabled( false );
	btnCancel->setText( tr("Close") );
}

void SettingsDialog::save()
{
	apply();
	close();
}

void SettingsDialog::cancel()
{
	emit loadSettings();

	close();

	btnSave->setEnabled( false );
	btnApply->setEnabled( false );
	btnCancel->setText( tr("Close") );
}

void SettingsDialog::restoreDefaults()
{
	auto tmpDlg = std::unique_ptr<SettingsDialog>( new SettingsDialog );
	tmpDlg->save();

	loadSettings();
}

void SettingsDialog::modified()
{
	btnSave->setEnabled( true );
	btnApply->setEnabled( true );
	btnCancel->setText( tr("Cancel") );
}

void SettingsDialog::changePage( QListWidgetItem * current, QListWidgetItem * previous )
{
	if ( !current )
		current = previous;

	content->setCurrentIndex( categories->row( current ) );
}

bool SettingsDialog::eventFilter( QObject * o, QEvent * e )
{
	return QDialog::eventFilter( o, e );
}

void SettingsDialog::showEvent( QShowEvent * e )
{
	emit loadSettings();

	QDialog::showEvent( e );
}
