#include "settings.h"

#include "widgets/colorwheel.h"
#include "widgets/floatslider.h"
#include "ui/settingsdialog.h"

#include "ui_settingsgeneral.h"
#include "ui_settingsrender.h"
#include "ui_settingsresources.h"

#include <fsengine/fsengine.h>
#include <fsengine/fsmanager.h>

#include <QComboBox>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QLocale>
#include <QMessageBox>
#include <QRadioButton>
#include <QRegularExpression>
#include <QSettings>
#include <QStringListModel>
#include <QTimer>


SettingsPane::SettingsPane( QWidget * parent ) :
	QWidget( parent )
{
	dlg = qobject_cast<SettingsDialog *>(parent);
	if ( dlg ) {
		connect( dlg, &SettingsDialog::loadSettings, this, &SettingsPane::read );
		connect( dlg, &SettingsDialog::saveSettings, this, &SettingsPane::write );
		connect( this, &SettingsPane::paneModified, dlg, &SettingsDialog::modified );
	}
}

SettingsPane::~SettingsPane()
{
}

bool SettingsPane::isModified()
{
	return modified;
}

void SettingsPane::setModified( bool m )
{
	modified = m;
}

void SettingsPane::modifyPane()
{
	if ( !modified ) {
		emit paneModified();

		modified = true;
	}
}

QString humanize( QString str )
{
	str = str.replace( QRegularExpression( "(?<=[A-Z])(?=[A-Z][a-z])|(?<=[^A-Z])(?=[A-Z])|(?<=[A-Za-z])(?=[^A-Za-z])" ), " " );
	str[0] = str[0].toUpper();

	return str;
}


void SettingsPane::readPane( QWidget * w, QSettings & settings )
{
	for ( auto c : w->children() ) {

		auto f = qobject_cast<QFrame *>(c);
		if ( f ) {
			readPane( f, settings );
		}

		auto grp = qobject_cast<QGroupBox *>(c);
		if ( grp ) {
			QString name = grp->title();
			if ( !name.isEmpty() && !grp->isFlat() )
				settings.beginGroup( name );

			readPane( grp, settings );

			if ( !name.isEmpty() && !grp->isFlat() )
				settings.endGroup();
		}

		auto chk = qobject_cast<QCheckBox *>(c);
		if ( chk ) {
			QString name = (!chk->text().isEmpty()) ? chk->text() : ::humanize( chk->objectName() );
			QVariant val = settings.value( name );
			if ( !val.isNull() )
				chk->setChecked( val.toBool() );

			connect( chk, &QCheckBox::stateChanged, this, &SettingsPane::modifyPane );
		}

		auto cmb = qobject_cast<QComboBox *>(c);
		if ( cmb ) {
			QVariant val = settings.value( ::humanize( cmb->objectName() ) );
			if ( !val.isNull() )
				cmb->setCurrentIndex( val.toInt() );

			connect( cmb, &QComboBox::currentTextChanged, this, &SettingsPane::modifyPane );
		}

		auto btn = qobject_cast<QRadioButton *>(c);
		if ( btn ) {
			QString name = (!btn->text().isEmpty()) ? btn->text() : ::humanize( btn->objectName() );
			QVariant val = settings.value( name );
			if ( !val.isNull() )
				btn->setChecked( val.toBool() );

			connect( btn, &QRadioButton::clicked, this, &SettingsPane::modifyPane );
		}

		auto edt = qobject_cast<QLineEdit *>(c);
		if ( edt ) {
			QVariant val = settings.value( ::humanize( edt->objectName() ) );
			if ( !val.isNull() )
				edt->setText( val.toString() );

			auto tEmit = new QTimer( this );
			tEmit->setInterval( 500 );
			tEmit->setSingleShot( true );
			connect( tEmit, &QTimer::timeout, this, &SettingsPane::modifyPane );
			connect( edt, &QLineEdit::textEdited, [this, tEmit]() {
				if ( tEmit->isActive() ) {
					tEmit->stop();
				}

				tEmit->start();
			} );
		}

		auto clr = qobject_cast<ColorLineEdit *>(c);
		if ( clr ) {
			QVariant val = settings.value( ::humanize( clr->objectName() ) );
			if ( !val.isNull() )
				clr->setColor( val.value<QColor>() );
		}

		auto spn = qobject_cast<QSpinBox *>(c);
		if ( spn ) {
			QVariant val = settings.value( ::humanize( spn->objectName() ) );
			if ( !val.isNull() )
				spn->setValue( val.toInt() );

			connect( spn, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &SettingsPane::modifyPane );
		}

		auto dbl = qobject_cast<QDoubleSpinBox *>(c);
		if ( dbl ) {
			QVariant val = settings.value( ::humanize( dbl->objectName() ) );
			if ( !val.isNull() )
				dbl->setValue( val.toDouble() );

			connect( dbl, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &SettingsPane::modifyPane );
		}
	}
}

void SettingsPane::writePane( QWidget * w, QSettings & settings )
{
	for ( auto c : w->children() ) {

		auto f = qobject_cast<QFrame *>(c);
		if ( f ) {
			writePane( f, settings );
		}

		auto grp = qobject_cast<QGroupBox *>(c);
		if ( grp ) {
			QString name = grp->title();
			if ( !name.isEmpty() && !grp->isFlat() )
				settings.beginGroup( name );

			writePane( grp, settings );

			if ( !name.isEmpty() && !grp->isFlat() )
				settings.endGroup();
		}

		auto chk = qobject_cast<QCheckBox *>(c);
		if ( chk ) {
			QString name = (!chk->text().isEmpty()) ? chk->text() : ::humanize( chk->objectName() );
			settings.setValue( name, chk->isChecked() );
		}

		auto cmb = qobject_cast<QComboBox *>(c);
		if ( cmb ) {
			settings.setValue( ::humanize( cmb->objectName() ), cmb->currentIndex() );
		}

		auto btn = qobject_cast<QRadioButton *>(c);
		if ( btn ) {
			QString name = (!btn->text().isEmpty()) ? btn->text() : ::humanize( btn->objectName() );
			settings.setValue( name, btn->isChecked() );
		}

		auto edt = qobject_cast<QLineEdit *>(c);
		if ( edt ) {
			settings.setValue( ::humanize( edt->objectName() ), edt->text() );
		}

		auto clr = qobject_cast<ColorLineEdit *>(c);
		if ( clr ) {
			settings.setValue( ::humanize( clr->objectName() ), clr->getColor() );
		}

		auto spn = qobject_cast<QSpinBox *>(c);
		if ( spn ) {
			settings.setValue( ::humanize( spn->objectName() ), spn->value() );
		}

		auto dbl = qobject_cast<QDoubleSpinBox *>(c);
		if ( dbl ) {
			settings.setValue( ::humanize( dbl->objectName() ), dbl->value() );
		}
	}
}


/*
 * General
 */

SettingsGeneral::SettingsGeneral( QWidget * parent ) :
	SettingsPane( parent ),
	ui( new Ui::SettingsGeneral )
{
	ui->setupUi( this );
	SettingsDialog::registerPage( parent, ui->name->text() );

	QLocale locale( "en" );
	QString txtLang = QLocale::languageToString( locale.language() );

	if ( locale.country() != QLocale::AnyCountry )
		txtLang.append( " (" ).append( QLocale::countryToString( locale.country() ) ).append( ")" );

	ui->language->addItem( txtLang, locale );
	ui->language->setCurrentIndex( 0 );

	QDir directory( QApplication::applicationDirPath() );

	if ( !directory.cd( "lang" ) ) {
#ifdef Q_OS_LINUX
		directory.cd( "/usr/share/nifskope/lang" );
#endif
	}

	QRegularExpression fileRe( "NifSkope_(.*)\\.qm", QRegularExpression::CaseInsensitiveOption );

	for ( const QString file : directory.entryList( QStringList( "NifSkope_*.qm" ), QDir::Files | QDir::NoSymLinks ) )
	{
		QRegularExpressionMatch fileReMatch = fileRe.match( file );
		if ( fileReMatch.hasMatch() ) {
			QLocale fileLocale( fileReMatch.capturedTexts()[1] );
			if ( ui->language->findData( fileLocale ) < 0 ) {
				QString txtLang = QLocale::languageToString( fileLocale.language() );

				if ( fileLocale.country() != QLocale::AnyCountry )
					txtLang.append( " (" + QLocale::countryToString( fileLocale.country() ) + ")" );

				ui->language->addItem( txtLang, fileLocale );
			}
		}
	}
}

SettingsGeneral::~SettingsGeneral()
{
	delete ui;
}

void SettingsGeneral::read()
{
	QSettings settings;

	settings.beginGroup( "Settings" );

	for ( int i = 0; i < ui->general->count(); i++ ) {

		auto w = ui->general->widget( i );

		settings.beginGroup( ::humanize( w->objectName() ) );
		readPane( w, settings );
		settings.endGroup();
	}

	settings.endGroup();

	setModified( false );
}

void SettingsGeneral::write()
{
	if ( !isModified() )
		return;

	QSettings settings;

	settings.beginGroup( "Settings" );

	int langPrev = settings.value( "UI/Language", 0 ).toInt();

	for ( int i = 0; i < ui->general->count(); i++ ) {

		auto w = ui->general->widget( i );

		settings.beginGroup( ::humanize( w->objectName() ) );
		writePane( w, settings );
		settings.endGroup();
	}

	int langCur = settings.value( "UI/Language", 0 ).toInt();

	settings.endGroup();

	// Set Locale
	auto idx = ui->language->currentIndex();
	settings.setValue( "Settings/Locale", ui->language->itemData( idx ).toLocale() );

	if ( langPrev != langCur ) {
		emit dlg->localeChanged();
	}

	setModified( false );
}

void SettingsGeneral::setDefault()
{
	read();
}


/*
 * Render
 */

SettingsRender::SettingsRender( QWidget * parent ) :
	SettingsPane( parent ),
	ui( new Ui::SettingsRender )
{
	ui->setupUi( this );
	SettingsDialog::registerPage( parent, ui->name->text() );

	auto color = [this]( const QString & str, ColorWheel * w, ColorLineEdit * e, const QColor & color ) {
		e->setWheel( w, str );
		w->setColor( color );

		connect( w, &ColorWheel::sigColorEdited, this, &SettingsPane::modifyPane );
	};

	color( "Background", ui->colorBackground, ui->background, QColor( 0, 0, 0 ) );
	color( "Wireframe", ui->colorWireframe, ui->wireframe, QColor( 0, 255, 0 ) );
	color( "Highlight", ui->colorHighlight, ui->highlight, QColor( 255, 255, 0 ) );


	auto alphaSlider = [this]( ColorWheel * c, ColorLineEdit * e, QHBoxLayout * l ) {
		auto alpha = new AlphaSlider( Qt::Vertical );
		alpha->setParent( this );

		c->setAlpha( true );
		e->setAlpha( 1.0 );
		l->addWidget( alpha );

		connect( c, &ColorWheel::sigColor, alpha, &AlphaSlider::setColor );
		connect( alpha, &AlphaSlider::valueChanged, c, &ColorWheel::setAlphaValue );
		connect( alpha, &AlphaSlider::valueChanged, e, &ColorLineEdit::setAlpha );
		connect( alpha, &AlphaSlider::valueChanged, this, &SettingsPane::modifyPane );

		alpha->setColor( e->getColor() );
	};

	alphaSlider( ui->colorWireframe, ui->wireframe, ui->layAlphaWire );
	alphaSlider( ui->colorHighlight, ui->highlight, ui->layAlphaHigh );
}

SettingsRender::~SettingsRender()
{
	delete ui;
}

void SettingsRender::read()
{
	QSettings settings;

	settings.beginGroup( "Settings/Render" );

	for ( int i = 0; i < ui->render->count(); i++ ) {

		auto w = ui->render->widget( i );

		settings.beginGroup( ::humanize( w->objectName() ) );
		readPane( w, settings );
		settings.endGroup();
	}

	settings.endGroup();

	setModified( false );
}

void SettingsRender::write()
{
	if ( !isModified() )
		return;

	QSettings settings;

	settings.beginGroup( "Settings/Render" );

	for ( int i = 0; i < ui->render->count(); i++ ) {

		auto w = ui->render->widget( i );

		settings.beginGroup( ::humanize( w->objectName() ) );
		writePane( w, settings );
		settings.endGroup();
	}

	settings.endGroup();

	setModified( false );
}

void SettingsRender::setDefault()
{
	read();
}


bool regFolderPath( QStringList & gamePaths, const QString & regPath, const QString & regValue, const QString & gameFolder,
                     QStringList gameSubDirs = QStringList(), QStringList gameArchiveFilters = QStringList() )
{
	QSettings reg( regPath, QSettings::NativeFormat );
	QDir dir( reg.value( regValue ).toString() );

	if ( dir.exists() && dir.cd( gameFolder ) ) {
		gamePaths.append( dir.path() );
		if ( !gameSubDirs.isEmpty() ) {
			for ( QString & sd : gameSubDirs ) {
				gamePaths.append( dir.path() + sd );
			}
		}

		if ( !gameArchiveFilters.isEmpty() ) {
			dir.setNameFilters( gameArchiveFilters );
			dir.setFilter( QDir::Dirs );
			for ( QString & dn : dir.entryList() ) {
				gamePaths << dir.filePath( dn );

				if ( !gameSubDirs.isEmpty() ) {
					for ( QString & sd : gameSubDirs ) {
						gamePaths << dir.filePath( dn ) + sd;
					}
				}
			}
		}
		return true;
	}
	return false;
}

bool regFolderPaths( QStringList & gamePaths, const QStringList & regPaths, const QString & regValue, const QString & gameFolder,
                     QStringList gameSubDirs = QStringList(), QStringList gameArchiveFilters = QStringList() )
{
	bool result = false;
	for ( const QString & path : regPaths ) {
		result |= ::regFolderPath( gamePaths, path, regValue, gameFolder, gameSubDirs, gameArchiveFilters );
	}
	return result;
}
/*
 * Resources
 */

SettingsResources::SettingsResources( QWidget * parent ) :
	SettingsPane( parent ),
	ui( new Ui::SettingsResources )
{
	ui->setupUi( this );
	SettingsDialog::registerPage( parent, ui->name->text() );

	archiveMgr = FSManager::get();

	folders = new QStringListModel( this );
	archives = new QStringListModel( this );

	ui->foldersList->setModel( folders );
	ui->archivesList->setModel( archives );

#ifndef Q_OS_WIN32
	ui->btnArchiveAutoDetect->setHidden( true );
	ui->btnFolderAutoDetect->setHidden( true );
#endif

	// Move Up / Move Down Behavior
	connect( ui->foldersList->selectionModel(), &QItemSelectionModel::currentChanged,
		[this]( const QModelIndex & idx, const QModelIndex & last )
		{
			ui->btnFolderUp->setEnabled( idx.row() > 0 );
			ui->btnFolderDown->setEnabled( idx.row() < folders->rowCount() - 1 );
		}
	);

	// Move Up / Move Down Behavior
	connect( ui->archivesList->selectionModel(), &QItemSelectionModel::currentChanged,
		[this]( const QModelIndex & idx, const QModelIndex & last )
		{
			ui->btnArchiveUp->setEnabled( idx.row() > 0 );
			ui->btnArchiveDown->setEnabled( idx.row() < archives->rowCount() - 1 );
		}
	);
}

SettingsResources::~SettingsResources()
{
	delete ui;
}

void SettingsResources::read()
{
	QSettings settings;

	QVariant foldersVal = settings.value( "Settings/Resources/Folders" );
	if ( foldersVal.isNull() ) {
		// Temporary
		QVariant migrate = settings.value( "Render Settings/Texture Folders" );
		if ( !migrate.isNull() ) {
			foldersVal = migrate;
			settings.setValue( "Settings/Resources/Folders", migrate.toStringList() );
		}
	}

	folders->setStringList( foldersVal.toStringList() );

	QVariant archivesVal = settings.value( "Settings/Resources/Archives" );
	if ( archivesVal.isNull() ) {
		// Temporary
		QVariant migrate = settings.value( "FSEngine/Archives" );
		if ( !migrate.isNull() ) {
			archivesVal = migrate;
			settings.setValue( "Settings/Resources/Archives", migrate.toStringList() );
		}
	}

	archives->setStringList( archivesVal.toStringList() );

	ui->foldersList->setCurrentIndex( folders->index( 0, 0 ) );
	ui->archivesList->setCurrentIndex( archives->index( 0, 0 ) );

	ui->chkAlternateExt->setChecked( settings.value( "Settings/Resources/Alternate Extensions", true ).toBool() );

	setModified( false );
}

void SettingsResources::write()
{
	if ( !isModified() )
		return;

	QSettings settings;

	settings.setValue( "Settings/Resources/Folders", folders->stringList() );
	settings.setValue( "Settings/Resources/Archives", archives->stringList() );

	// Sync FSManager to Archives list
	archiveMgr->archives.clear();
	for ( const QString an : archives->stringList() ) {
		if ( !archiveMgr->archives.contains( an ) )
			if ( FSArchiveHandler * a = FSArchiveHandler::openArchive( an ) )
				archiveMgr->archives.insert( an, a );
	}

	settings.setValue( "Settings/Resources/Alternate Extensions", ui->chkAlternateExt->isChecked() );

	setModified( false );

	emit dlg->flush3D();
}

void SettingsResources::setDefault()
{
	read();
}


void moveIdxDown( QListView * view )
{
	QModelIndex idx = view->currentIndex();
	auto model = view->model();

	if ( idx.isValid() && idx.row() < model->rowCount() - 1 ) {
		QModelIndex downIdx = idx.sibling( idx.row() + 1, 0 );
		QVariant v = model->data( idx, Qt::EditRole );
		model->setData( idx, model->data( downIdx, Qt::EditRole ) );
		model->setData( downIdx, v );
		view->setCurrentIndex( downIdx );
	}
}

void moveIdxUp( QListView * view )
{
	QModelIndex idx = view->currentIndex();
	auto model = view->model();

	if ( idx.isValid() && idx.row() > 0 ) {
		QModelIndex upIdx = idx.sibling( idx.row() - 1, 0 );
		QVariant v = model->data( idx, Qt::EditRole );
		model->setData( idx, model->data( upIdx, Qt::EditRole ) );
		model->setData( upIdx, v );
		view->setCurrentIndex( upIdx );
	}
}

void SettingsResources::on_btnFolderAdd_clicked()
{
	QFileDialog dialog( this );
	dialog.setFileMode( QFileDialog::Directory );

	QString path;
	if ( dialog.exec() )
		path = dialog.selectedFiles().at( 0 );

	folders->insertRow( 0 );
	folders->setData( folders->index( 0, 0 ), path );
	ui->foldersList->setCurrentIndex( folders->index( 0, 0 ) );
	modifyPane();
}

void SettingsResources::on_btnFolderRemove_clicked()
{
	ui->foldersList->model()->removeRow( ui->foldersList->currentIndex().row() );
	modifyPane();
}

void SettingsResources::on_btnFolderDown_clicked()
{
	moveIdxDown( ui->foldersList );
	modifyPane();
}

void SettingsResources::on_btnFolderUp_clicked()
{
	moveIdxUp( ui->foldersList );
	modifyPane();
}

void SettingsResources::on_btnFolderAutoDetect_clicked()
{
	// List to hold all games paths that were detected
	QStringList list;

	// Skyrim, Fallout, Oblivion
	{
		QString beth = "HKEY_LOCAL_MACHINE\\SOFTWARE\\Bethesda Softworks\\%1";

		regFolderPaths( list,
			{ beth.arg( "Skyrim" ), beth.arg( "FalloutNV" ), beth.arg( "Fallout3" ), beth.arg( "Oblivion" ) },
			"Installed Path",
			"Data",
			{},        /* No subdirs */
			{ ".bsa" }
		);
	}

	// Morrowind
	{
		regFolderPath( list,
			"HKEY_LOCAL_MACHINE\\SOFTWARE\\Bethesda Softworks\\Morrowind",
			"Installed Path",
			"Data",
			{ "/Textures" },
			{ ".bsa" }
		);
	}

	// CIV IV
	{
		regFolderPath( list,
			"HKEY_LOCAL_MACHINE\\SOFTWARE\\Firaxis Games\\Sid Meier's Civilization 4",
			"INSTALLDIR",
			"Assets/Art/shared"
		);
	}

	// Freedom Force
	{
		QString ff = "HKEY_LOCAL_MACHINE\\SOFTWARE\\Irrational Games\\%1";
		QStringList ffSubDirs{ "./textures", "./skins/standard" };

		regFolderPaths( list,
			{ ff.arg( "FFVTTR" ), ff.arg( "Freedom Force" ) },
			"InstallDir",
			"Data/Art/library/area_specific/_textures",
			ffSubDirs
		);
	}

	QStringList archivesNew = folders->stringList() + list;
	archivesNew.removeDuplicates();

	folders->setStringList( archivesNew );
	ui->foldersList->setCurrentIndex( folders->index( 0, 0, QModelIndex() ) );

	modifyPane();
}


void SettingsResources::on_btnArchiveAdd_clicked()
{
	QStringList files = QFileDialog::getOpenFileNames(
		this,
		"Select one or more archives",
		"",
		"BSA (*.bsa)"
	);

	for ( int i = 0; i < files.count(); i++ ) {
		archives->insertRow( i );
		archives->setData( archives->index( i, 0 ), files.at( i ) );
	}

	ui->archivesList->setCurrentIndex( archives->index( 0, 0 ) );
	modifyPane();
}

void SettingsResources::on_btnArchiveRemove_clicked()
{
	ui->archivesList->model()->removeRow( ui->archivesList->currentIndex().row() );
	modifyPane();
}

void SettingsResources::on_btnArchiveDown_clicked()
{
	moveIdxDown( ui->archivesList );
	modifyPane();
}

void SettingsResources::on_btnArchiveUp_clicked()
{
	moveIdxUp( ui->archivesList );
	modifyPane();
}

void SettingsResources::on_btnArchiveAutoDetect_clicked()
{
	QStringList autoList = FSManager::autodetectArchives();

	QStringList archivesNew = archives->stringList() + autoList;
	archivesNew.removeDuplicates();

	archives->setStringList( archivesNew );

	ui->archivesList->setCurrentIndex( archives->index( 0, 0 ) );
	modifyPane();
}
