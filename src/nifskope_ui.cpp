/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005-2014, NIF File Format Library and Tools
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
3. The name of the NIF File Format Library and Tools project may not be
used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

***** END LICENCE BLOCK *****/

#include "nifskope.h"
#include "version.h"
#include "options.h"

#include "ui_nifskope.h"
#include "ui/about_dialog.h"

#include "glview.h"
#include "kfmmodel.h"
#include "nifmodel.h"
#include "nifproxy.h"
#include "spellbook.h"
#include "widgets/copyfnam.h"
#include "widgets/fileselect.h"
#include "widgets/floatslider.h"
#include "widgets/floatedit.h"
#include "widgets/nifview.h"
#include "widgets/refrbrowser.h"
#include "widgets/inspect.h"
#include "widgets/xmlcheck.h"

#ifdef FSENGINE
#include <fsengine/fsmanager.h>
#endif

#include <QAction>
#include <QApplication>
#include <QByteArray>
#include <QComboBox>
#include <QDebug>
#include <QDockWidget>
#include <QFontDialog>
#include <QHeaderView>
#include <QMenu>
#include <QMenuBar>
#include <QProgressBar>
#include <QMessageBox>
#include <QSettings>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>


NifSkope * NifSkope::createWindow( const QString & fname )
{
	NifSkope * skope = new NifSkope;
	skope->setAttribute( Qt::WA_DeleteOnClose );
	skope->restoreUi();
	skope->show();
	skope->raise();

	if ( !fname.isEmpty() ) {
		skope->lineLoad->setFile( fname );
		QTimer::singleShot( 0, skope, SLOT( load() ) );
	}

	return skope;
}

void NifSkope::initActions()
{
	aSanitize = ui->aSanitize;
	aList = ui->aList;
	aHierarchy = ui->aHierarchy;
	aCondition = ui->aCondition;
	aRCondition = ui->aRCondition;
	aSelectFont = ui->aSelectFont;

	ui->aWindow->setShortcut( QKeySequence::New );

	aList->setChecked( list->model() == nif );
	aHierarchy->setChecked( list->model() == proxy );

	// Allow only List or Tree view to be selected at once
	gListMode = new QActionGroup( this );
	gListMode->addAction( aList );
	gListMode->addAction( aHierarchy );
	gListMode->setExclusive( true );
	connect( gListMode, &QActionGroup::triggered, this, &NifSkope::setListMode );


	connect( aCondition, &QAction::toggled, aRCondition, &QAction::setEnabled );
	connect( aRCondition, &QAction::toggled, tree, &NifTreeView::setRealTime );
	connect( aRCondition, &QAction::toggled, kfmtree, &NifTreeView::setRealTime );

	// use toggled to enable startup values to take effect
	connect( aCondition, &QAction::toggled, tree, &NifTreeView::setEvalConditions );
	connect( aCondition, &QAction::toggled, kfmtree, &NifTreeView::setEvalConditions );

	connect( ui->aAboutNifSkope, &QAction::triggered, aboutDialog, &AboutDialog::show );
	connect( ui->aAboutQt, &QAction::triggered, qApp, &QApplication::aboutQt );

#ifdef FSENGINE
	auto fsm = FSManager::get();
	if ( fsm ) {
		connect( ui->aResources, &QAction::triggered, fsm, &FSManager::selectArchives );
	}
#endif

	connect( ui->aPrintView, &QAction::triggered, ogl, &GLView::saveImage );

#ifdef QT_NO_DEBUG
	ui->aColorKeyDebug->setDisabled( true );
	ui->aColorKeyDebug->setVisible( false );
#else
	connect( ui->aColorKeyDebug, &QAction::triggered, ogl, &GLView::paintGL );
#endif


	connect( ogl, &GLView::clicked, this, &NifSkope::select );
	connect( ogl, &GLView::customContextMenuRequested, this, &NifSkope::contextMenu );
	connect( ogl, &GLView::sigTime, inspect, &InspectView::updateTime );
	connect( ogl, &GLView::paintUpdate, inspect, &InspectView::refresh );
	connect( ogl, &GLView::viewpointChanged, [this]() {
		ui->aViewTop->setChecked( false );
		ui->aViewFront->setChecked( false );
		ui->aViewLeft->setChecked( false );
		ui->aViewUser->setChecked( false );

		ogl->setOrientation( GLView::viewDefault, false );
	} );

	// Update Inspector widget with current index
	connect( tree, &NifTreeView::sigCurrentIndexChanged, inspect, &InspectView::updateSelection );

	// Remove Progress Bar after the mesh has finished loading
	connect( ogl, &GLView::paintUpdate, [this]() {
		if ( progress && (progress->value() == progress->maximum()) ) {
		
			ui->statusbar->removeWidget( progress );
			delete progress;
			progress = nullptr;
		}
			
	} );
}

void NifSkope::initDockWidgets()
{
	dRefr = ui->RefrDock;
	dList = ui->ListDock;
	dTree = ui->TreeDock;
	dInsp = ui->InspectDock;
	dKfm = ui->KfmDock;

	// Hide certain docks by default
	dRefr->toggleViewAction()->setChecked( false );
	dInsp->toggleViewAction()->setChecked( false );
	dKfm->toggleViewAction()->setChecked( false );

	dRefr->setVisible( false );
	dInsp->setVisible( false );
	dKfm->setVisible( false );

	// Set Inspect widget
	dInsp->setWidget( inspect );

	connect( dList->toggleViewAction(), &QAction::triggered, tree, &NifTreeView::clearRootIndex );

}

void NifSkope::initMenu()
{
	// Disable without NIF loaded
	ui->mRender->setEnabled( false );

	// Insert the Load/Save actions into the File menu
	ui->mFile->insertActions( ui->aFileLoadDummy, lineLoad->actions() );
	ui->mFile->insertActions( ui->aFileSaveDummy, lineSave->actions() );

#ifndef FSENGINE
	if ( ui->aResources ) {
		ui->mFile->removeAction( ui->aResources );
	}
#endif

	// Populate Toolbars menu with all enabled toolbars
	for ( QObject * o : children() ) {
		QToolBar * tb = qobject_cast<QToolBar *>(o);
		if ( tb )
			ui->mToolbars->addAction( tb->toggleViewAction() );
	}

	// Insert SpellBook class before Help
	ui->menubar->insertMenu( ui->menubar->actions().at( 3 ), book );

	ui->mOptions->addActions( Options::actions() );

	// Insert Import/Export menus
	mExport = ui->menuExport;
	mImport = ui->menuImport;

	fillImportExportMenus();
	connect( mExport, &QMenu::triggered, this, &NifSkope::sltImportExport );
	connect( mImport, &QMenu::triggered, this, &NifSkope::sltImportExport );
}

void NifSkope::initToolBars()
{
	// Disable without NIF loaded
	ui->tRender->setEnabled( false );

	// Load & Save toolbar

	QStringList fileExtensions{
		"All Files (*.nif *.kf *.kfa *.kfm *.nifcache *.texcache *.pcpatch *.jmi)",
		"NIF (*.nif)", "Keyframe (*.kf)", "Keyframe Animation (*.kfa)", "Keyframe Motion (*.kfm)",
		"NIFCache (*.nifcache)", "TEXCache (*.texcache)", "PCPatch (*.pcpatch)", "JMI (*.jmi)"
	};

	// Load
	aLineLoad = ui->toolbar->addWidget( lineLoad = new FileSelector( FileSelector::LoadFile, tr( "&Load..." ), QBoxLayout::RightToLeft, QKeySequence::Open ) );
	lineLoad->setFilter( fileExtensions );
	connect( lineLoad, &FileSelector::sigActivated, this, static_cast<void (NifSkope::*)()>(&NifSkope::load) );

	// Load<=>Save
	CopyFilename * cpFilename = new CopyFilename( this );
	cpFilename->setObjectName( "fileCopyWidget" );
	connect( cpFilename, &CopyFilename::leftTriggered, this, &NifSkope::copyFileNameSaveLoad );
	connect( cpFilename, &CopyFilename::rightTriggered, this, &NifSkope::copyFileNameLoadSave );
	aCpFileName = ui->toolbar->addWidget( cpFilename );

	// Save
	aLineSave = ui->toolbar->addWidget( lineSave = new FileSelector( FileSelector::SaveFile, tr( "&Save As..." ), QBoxLayout::LeftToRight, QKeySequence::Save ) );
	lineSave->setFilter( fileExtensions );
	connect( lineSave, &FileSelector::sigActivated, this, static_cast<void (NifSkope::*)()>(&NifSkope::save) );


	// Render

	QActionGroup * grpView = new QActionGroup( this );
	grpView->addAction( ui->aViewTop );
	grpView->addAction( ui->aViewFront );
	grpView->addAction( ui->aViewLeft );
	grpView->addAction( ui->aViewWalk );


	// Animate

	connect( ui->aAnimate, &QAction::toggled, ui->tAnim, &QToolBar::setVisible );
	connect( ui->tAnim, &QToolBar::visibilityChanged, ui->aAnimate, &QAction::setChecked );

	// Animation timeline slider
	auto sldTime = new FloatSlider( Qt::Horizontal, true, true );
	sldTime->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Maximum );
	connect( ogl, &GLView::sigTime, sldTime, &FloatSlider::set );
	connect( sldTime, &FloatSlider::valueChanged, ogl, &GLView::sltTime );


	FloatEdit * edtTime = new FloatEdit;
	edtTime->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Maximum );

	connect( ogl, &GLView::sigTime, edtTime, &FloatEdit::set );
	connect( edtTime, static_cast<void (FloatEdit::*)(float)>(&FloatEdit::sigEdited), ogl, &GLView::sltTime );
	connect( sldTime, &FloatSlider::valueChanged, edtTime, &FloatEdit::setValue );
	connect( edtTime, static_cast<void (FloatEdit::*)(float)>(&FloatEdit::sigEdited), sldTime, &FloatSlider::setValue );
	sldTime->addEditor( edtTime );

	// Animations
	auto animGroups = new QComboBox();
	animGroups->setMinimumWidth( 120 );
	connect( animGroups, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::activated), ogl, &GLView::sltSequence );

	ui->tAnim->addWidget( sldTime );
	ui->tAnim->addWidget( animGroups );


	// LOD Toolbar
	QToolBar * tLOD = ui->tLOD;

	QSettings cfg;
	int lodLevel = cfg.value( "GLView/LOD Level", 2 ).toInt();
	cfg.setValue( "GLView/LOD Level", lodLevel );

	QSlider * lodSlider = new QSlider( Qt::Horizontal );
	lodSlider->setFocusPolicy( Qt::StrongFocus );
	lodSlider->setTickPosition( QSlider::TicksBelow );
	lodSlider->setTickInterval( 1 );
	lodSlider->setSingleStep( 1 );
	lodSlider->setMinimum( 0 );
	lodSlider->setMaximum( 2 );
	lodSlider->setValue( lodLevel );

	tLOD->addWidget( lodSlider );
	tLOD->setEnabled( false );

	connect( lodSlider, &QSlider::valueChanged, []( int value ) {
		QSettings cfg;
		cfg.setValue( "GLView/LOD Level", value );
	}
	);
	connect( lodSlider, &QSlider::valueChanged, Options::get(), &Options::sigChanged );
	connect( nif, &NifModel::lodSliderChanged, [tLOD]( bool enabled ) { tLOD->setEnabled( enabled ); } );
}


void NifSkope::saveUi() const
{
	QSettings settings;
	// TODO: saveState takes a version number which can be incremented between releases if necessary
	settings.setValue( "UI/Window State", saveState( 0x073 ) );
	settings.setValue( "UI/Window Geometry", saveGeometry() );

	settings.setValue( "File/Last Load", lineLoad->text() );
	settings.setValue( "File/Last Save", lineSave->text() );
	settings.setValue( "File/Auto Sanitize", aSanitize->isChecked() );

	settings.setValue( "UI/List Mode", (gListMode->checkedAction() == aList ? "list" : "hierarchy") );
	settings.setValue( "UI/Hide Mismatched Rows", aCondition->isChecked() );
	settings.setValue( "UI/Realtime Condition Updating", aRCondition->isChecked() );

	settings.setValue( "UI/List Header", list->header()->saveState() );
	settings.setValue( "UI/Tree Header", tree->header()->saveState() );
	settings.setValue( "UI/Kfmtree Header", kfmtree->header()->saveState() );

	settings.setValue( "GLView/Enable Animations", ui->aAnimate->isChecked() );
	settings.setValue( "GLView/Play Animation", ui->aAnimPlay->isChecked() );
	settings.setValue( "GLView/Loop Animation", ui->aAnimLoop->isChecked() );
	settings.setValue( "GLView/Switch Animation", ui->aAnimSwitch->isChecked() );
	settings.setValue( "GLView/Perspective", ui->aViewPerspective->isChecked() );

	Options::get()->save();
}


void NifSkope::restoreUi()
{
	QSettings settings;
	restoreGeometry( settings.value( "UI/Window Geometry" ).toByteArray() );
	restoreState( settings.value( "UI/Window State" ).toByteArray(), 0x073 );

	lineLoad->setText( settings.value( "File/Last Load", QString( "" ) ).toString() );
	lineSave->setText( settings.value( "File/Last Save", QString( "" ) ).toString() );
	aSanitize->setChecked( settings.value( "File/Auto Sanitize", true ).toBool() );

	if ( settings.value( "UI/List Mode", "hierarchy" ).toString() == "list" )
		aList->setChecked( true );
	else
		aHierarchy->setChecked( true );

	setListMode();

	aCondition->setChecked( settings.value( "UI/Hide Mismatched Rows", false ).toBool() );
	aRCondition->setChecked( settings.value( "UI/Realtime Condition Updating", false ).toBool() );

	list->header()->restoreState( settings.value( "UI/List Header" ).toByteArray() );
	tree->header()->restoreState( settings.value( "UI/Tree Header" ).toByteArray() );
	kfmtree->header()->restoreState( settings.value( "UI/Kfmtree Header" ).toByteArray() );

	ui->aAnimate->setChecked( settings.value( "GLView/Enable Animations", true ).toBool() );
	ui->aAnimPlay->setChecked( settings.value( "GLView/Play Animation", true ).toBool() );
	ui->aAnimLoop->setChecked( settings.value( "GLView/Loop Animation", true ).toBool() );
	ui->aAnimSwitch->setChecked( settings.value( "GLView/Switch Animation", true ).toBool() );

	auto isPersp = settings.value( "GLView/Perspective", true ).toBool();
	ui->aViewPerspective->setChecked( isPersp );

	ogl->setProjection( isPersp );

	QVariant fontVar = settings.value( "UI/View Font" );

	if ( fontVar.canConvert<QFont>() )
		setViewFont( fontVar.value<QFont>() );

	// Modify UI settings that cannot be set in Designer
	tabifyDockWidget( ui->InspectDock, ui->KfmDock );
}

void NifSkope::setViewFont( const QFont & font )
{
	list->setFont( font );
	QFontMetrics metrics( list->font() );
	list->setIconSize( QSize( metrics.width( "000" ), metrics.lineSpacing() ) );
	tree->setFont( font );
	tree->setIconSize( QSize( metrics.width( "000" ), metrics.lineSpacing() ) );
	kfmtree->setFont( font );
	kfmtree->setIconSize( QSize( metrics.width( "000" ), metrics.lineSpacing() ) );
	ogl->setFont( font );
}

bool NifSkope::eventFilter( QObject * o, QEvent * e )
{
	if ( e->type() == QEvent::Polish ) {
		QTimer::singleShot( 0, this, SLOT( overrideViewFont() ) );
	}

	return QMainWindow::eventFilter( o, e );
}


/*
* Slots
* **********************
*/


void NifSkope::contextMenu( const QPoint & pos )
{
	QModelIndex idx;
	QPoint p = pos;

	if ( sender() == tree ) {
		idx = tree->indexAt( pos );
		p = tree->mapToGlobal( pos );
	} else if ( sender() == list ) {
		idx = list->indexAt( pos );
		p = list->mapToGlobal( pos );
	} else if ( sender() == ogl ) {
		idx = ogl->indexAt( pos );
		p = ogl->mapToGlobal( pos );
	} else {
		return;
	}

	while ( idx.model() && idx.model()->inherits( "NifProxyModel" ) ) {
		idx = qobject_cast<const NifProxyModel *>(idx.model())->mapTo( idx );
	}

	SpellBook contextBook( nif, idx, this, SLOT( select( const QModelIndex & ) ) );
	contextBook.exec( p );
}

void NifSkope::overrideViewFont()
{
	QSettings settings;
	QVariant var = settings.value( "UI/View Font" );

	if ( var.canConvert<QFont>() ) {
		setViewFont( var.value<QFont>() );
	}
}


/*
* Automatic Slots
* **********************
*/


void NifSkope::on_aLoadXML_triggered()
{
	NifModel::loadXML();
	KfmModel::loadXML();
}

void NifSkope::on_aReload_triggered()
{
	if ( NifModel::loadXML() ) {
		load();
	}
}

void NifSkope::on_aSelectFont_triggered()
{
	bool ok;
	QFont fnt = QFontDialog::getFont( &ok, list->font(), this );

	if ( !ok )
		return;

	setViewFont( fnt );
	QSettings settings;
	settings.setValue( "UI/View Font", fnt );
}

void NifSkope::on_aWindow_triggered()
{
	createWindow();
}

void NifSkope::on_aShredder_triggered()
{
	TestShredder::create();
}

void NifSkope::on_aResetBlockDetails_triggered()
{
	if ( tree )
		tree->clearRootIndex();
}


void NifSkope::on_tRender_actionTriggered( QAction * action )
{

}

void NifSkope::on_aViewTop_triggered( bool wasChecked )
{
	if ( wasChecked ) {
		ogl->setOrientation( GLView::viewTop );
	}
}

void NifSkope::on_aViewFront_triggered( bool wasChecked )
{
	if ( wasChecked ) {
		ogl->setOrientation( GLView::viewFront );
	}
}

void NifSkope::on_aViewLeft_triggered( bool wasChecked )
{
	if ( wasChecked ) {
		ogl->setOrientation( GLView::viewLeft );
	}
}

void NifSkope::on_aViewCenter_triggered()
{
	qDebug() << "Centering";
	ogl->center();
}

void NifSkope::on_aViewFlip_triggered( bool wasChecked )
{
	qDebug() << "Flipping";
	ogl->flipOrientation();

}

void NifSkope::on_aViewPerspective_toggled( bool wasChecked ) {
	ogl->setProjection( wasChecked );

	//if ( wasChecked )
	//	ui->aViewPerspective->setText( tr( "Perspective" ) );
	//else
	//	ui->aViewPerspective->setText( tr( "Orthographic" ) );
}

void NifSkope::on_aViewWalk_triggered( bool wasChecked )
{
	if ( wasChecked ) {
		ogl->setOrientation( GLView::viewWalk );
	}
}


void NifSkope::on_aViewUserSave_triggered( bool wasChecked )
{ 
	qDebug() << "aViewUserSave_triggered";
	ogl->saveUserView();
	ui->aViewUser->setChecked( true );
}


void NifSkope::on_aViewUser_toggled( bool wasChecked )
{
	if ( wasChecked ) {
		ogl->setOrientation( GLView::viewUser, false );
		ogl->loadUserView();
	}
}


