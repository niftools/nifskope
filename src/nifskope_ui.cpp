/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005-2015, NIF File Format Library and Tools
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
#include "ui/settingsdialog.h"

#include "glview.h"
#include "gl/glscene.h"
#include "kfmmodel.h"
#include "nifmodel.h"
#include "nifproxy.h"
#include "spellbook.h"
#include "batchprocessor.h"
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
#include <QCheckBox>
#include <QComboBox>
#include <QDebug>
#include <QDockWidget>
#include <QFileDialog>
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
#include <QWidgetAction>

#include <QProcess>
#include <QStyleFactory>


//! @file nifskope_ui.cpp UI logic for %NifSkope's main window.

NifSkope * NifSkope::createWindow( const QString & fname )
{
	NifSkope * skope = new NifSkope;
	skope->setAttribute( Qt::WA_DeleteOnClose );
	skope->restoreUi();
	skope->show();
	skope->raise();

	// Example Dark style
	//QApplication::setStyle( QStyleFactory::create( "Fusion" ) );
	//QPalette darkPalette;
	//darkPalette.setColor( QPalette::Window, QColor( 53, 53, 53 ) );
	//darkPalette.setColor( QPalette::WindowText, Qt::white );
	//darkPalette.setColor( QPalette::Base, QColor( 25, 25, 25 ) );
	//darkPalette.setColor( QPalette::AlternateBase, QColor( 53, 53, 53 ) );
	//darkPalette.setColor( QPalette::ToolTipBase, Qt::white );
	//darkPalette.setColor( QPalette::ToolTipText, Qt::white );
	//darkPalette.setColor( QPalette::Text, Qt::white );
	//darkPalette.setColor( QPalette::Button, QColor( 53, 53, 53 ) );
	//darkPalette.setColor( QPalette::ButtonText, Qt::white );
	//darkPalette.setColor( QPalette::BrightText, Qt::red );
	//darkPalette.setColor( QPalette::Link, QColor( 42, 130, 218 ) );
	//darkPalette.setColor( QPalette::Highlight, QColor( 42, 130, 218 ) );
	//darkPalette.setColor( QPalette::HighlightedText, Qt::black );
	//qApp->setPalette( darkPalette );
	//qApp->setStyleSheet( "QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }" );

	if ( !fname.isEmpty() ) {
		skope->loadFile( fname );
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

	// Undo/Redo
	undoAction = nif->undoStack->createUndoAction( this, tr( "&Undo" ) );
	undoAction->setShortcut( QKeySequence::Undo );
	undoAction->setIcon( QIcon( ":btn/undo" ) );
	redoAction = nif->undoStack->createRedoAction( this, tr( "&Redo" ) );
	redoAction->setShortcut( QKeySequence::Redo );
	redoAction->setIcon( QIcon( ":btn/redo" ) );

	ui->tFile->addAction( undoAction );
	ui->tFile->addAction( redoAction );

	connect( undoAction, &QAction::triggered, [this]( bool ) {
		ogl->update();
	} );

	connect( redoAction, &QAction::triggered, [this]( bool ) {
		ogl->update();
	} );

	//ui->aSave->setShortcut( QKeySequence::Save ); // Bad idea, goes against previous shortcuts
	//ui->aSaveAs->setShortcut( QKeySequence::SaveAs ); // Bad idea, goes against previous shortcuts
	ui->aWindow->setShortcut( QKeySequence::New );

	connect( ui->aOpen, &QAction::triggered, this, &NifSkope::openDlg );
	connect( ui->aSave, &QAction::triggered, this, &NifSkope::save );  
	connect( ui->aSaveAs, &QAction::triggered, this, &NifSkope::saveAsDlg );

	// TODO: Assure Actions and Scene state are synced
	// Set Data for Actions to pass onto Scene when clicking
	/*	
		ShowAxes = 0x1,
		ShowGrid = 0x2,
		ShowNodes = 0x4,
		ShowCollision = 0x8,
		ShowConstraints = 0x10,
		ShowMarkers = 0x20,
		DoDoubleSided = 0x40, // Not implemented
		DoVertexColors = 0x80,
		DoSpecular = 0x100,
		DoGlow = 0x200,
		DoTexturing = 0x400,
		DoBlending = 0x800,   // Not implemented
		DoMultisampling = 0x1000, // Not implemented
		DoLighting = 0x2000,
		DoCubeMapping = 0x4000,
		DisableShaders = 0x8000
	*/

	ui->aShowAxes->setData( Scene::ShowAxes );
	ui->aShowNodes->setData( Scene::ShowNodes );
	ui->aShowCollision->setData( Scene::ShowCollision );
	ui->aShowConstraints->setData( Scene::ShowConstraints );
	ui->aShowMarkers->setData( Scene::ShowMarkers );

	ui->aTextures->setData( Scene::DoTexturing );
	ui->aVertexColors->setData( Scene::DoVertexColors );
	ui->aSpecular->setData( Scene::DoSpecular );
	ui->aGlow->setData( Scene::DoGlow );
	ui->aCubeMapping->setData( Scene::DoCubeMapping );
	ui->aLighting->setData( Scene::DoLighting );
	ui->aDisableShading->setData( Scene::DisableShaders );

	auto agroup = [this]( QVector<QAction *> actions, bool exclusive ) {
		QActionGroup * ag = new QActionGroup( this );
		for ( auto a : actions ) {
			ag->addAction( a );
		}

		ag->setExclusive( exclusive );

		return ag;
	};

	showActions = agroup( { ui->aShowAxes, ui->aShowNodes, ui->aShowCollision, ui->aShowConstraints, ui->aShowMarkers }, false );
	connect( showActions, &QActionGroup::triggered, ogl->getScene(), &Scene::updateSceneOptionsGroup );

	shadingActions = agroup( { ui->aTextures, ui->aVertexColors, ui->aSpecular, ui->aGlow, ui->aCubeMapping, ui->aLighting, ui->aDisableShading }, false );
	connect( shadingActions, &QActionGroup::triggered, ogl->getScene(), &Scene::updateSceneOptionsGroup );

	// Setup blank QActions for Recent Files menus
	for ( int i = 0; i < NumRecentFiles; ++i ) {
		recentFileActs[i] = new QAction( this );
		recentFileActs[i]->setVisible( false );
		connect( recentFileActs[i], &QAction::triggered, this, &NifSkope::openRecentFile );
	}


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
	ui->aBoundsDebug->setDisabled( true );
	ui->aBoundsDebug->setVisible( false );
#else
	QAction * debugNone = new QAction( this );

	QActionGroup * debugActions = agroup( { debugNone, ui->aColorKeyDebug, ui->aBoundsDebug }, false );
	connect( ui->aColorKeyDebug, &QAction::triggered, [this]( bool checked ) {
		if ( checked )
			ogl->setDebugMode( GLView::DbgColorPicker );
		else
			ogl->setDebugMode( GLView::DbgNone );
		
		ogl->update();
	} );

	connect( ui->aBoundsDebug, &QAction::triggered, [this]( bool checked ) {
		if ( checked )
			ogl->setDebugMode( GLView::DbgBounds );
		else
			ogl->setDebugMode( GLView::DbgNone );

		ogl->update();
	} );

	connect( debugActions, &QActionGroup::triggered, [=]( QAction * action ) {
		for ( auto a : debugActions->actions() ) {
			if ( a == action )
				continue;

			a->setChecked( false );
		}
	} );
#endif

	connect( ui->aSilhouette, &QAction::triggered, [this]( bool checked ) {
		//ui->aDisableShading->setChecked( checked );
		ogl->setVisMode( Scene::VisSilhouette, checked );
	} );

	connect( ui->aVisNormals, &QAction::triggered, [this]( bool checked ) {
		ogl->setVisMode( Scene::VisNormalsOnly, checked );
	} );

	connect( ogl, &GLView::clicked, this, &NifSkope::select );
	connect( ogl, &GLView::sceneTimeChanged, inspect, &InspectView::updateTime );
	connect( ogl, &GLView::paintUpdate, inspect, &InspectView::refresh );
	connect( ogl, &GLView::viewpointChanged, [this]() {
		ui->aViewTop->setChecked( false );
		ui->aViewFront->setChecked( false );
		ui->aViewLeft->setChecked( false );
		ui->aViewUser->setChecked( false );

		ogl->setOrientation( GLView::ViewDefault, false );
	} );

	connect( graphicsView, &GLGraphicsView::customContextMenuRequested, this, &NifSkope::contextMenu );

	// Update Inspector widget with current index
	connect( tree, &NifTreeView::sigCurrentIndexChanged, inspect, &InspectView::updateSelection );

	
	// Hide new Settings from Release for the time being
#ifdef QT_NO_DEBUG
	ui->aSettings->setVisible( false );
#endif
}

void NifSkope::initDockWidgets()
{
	dRefr = ui->RefrDock;
	dList = ui->ListDock;
	dTree = ui->TreeDock;
	dHeader = ui->HeaderDock;
	dInsp = ui->InspectDock;
	dKfm = ui->KfmDock;

	// Tabify List and Header
	tabifyDockWidget( dList, dHeader );
	// Raise List above Header
	dList->raise();

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

#ifndef FSENGINE
	if ( ui->aResources ) {
		ui->mOptions->removeAction( ui->aResources );
	}
#endif

	// Populate Toolbars menu with all enabled toolbars
	for ( QObject * o : children() ) {
		QToolBar * tb = qobject_cast<QToolBar *>(o);
		if ( tb && tb->objectName() != "tFile" ) {
			// Do not add tFile to the list
			ui->mToolbars->addAction( tb->toggleViewAction() );
		}
	}

	// Insert SpellBook class before Help
	ui->menubar->insertMenu( ui->menubar->actions().at( 3 ), book );

	ui->mOptions->insertActions( ui->aResources, Options::actions() );

	// Insert Import/Export menus
	mExport = ui->menuExport;
	mImport = ui->menuImport;

	fillImportExportMenus();
	connect( mExport, &QMenu::triggered, this, &NifSkope::sltImportExport );
	connect( mImport, &QMenu::triggered, this, &NifSkope::sltImportExport );

	for ( int i = 0; i < NumRecentFiles; ++i )
		ui->mRecentFiles->addAction( recentFileActs[i] );


	// Load & Save
	QMenu * mSave = new QMenu( this );
	mSave->setObjectName( "mSave" );

	mSave->addAction( ui->aSave );
	mSave->addAction( ui->aSaveAs );

	QMenu * mOpen = new QMenu( this );
	mOpen->setObjectName( "mOpen" );

	mOpen->addAction( ui->aOpen );

	aRecentFilesSeparator = mOpen->addSeparator();
	for ( int i = 0; i < NumRecentFiles; ++i )
		mOpen->addAction( recentFileActs[i] );

	// Append Menu to tFile actions
	for ( auto child : ui->tFile->findChildren<QToolButton *>() ) {

		if ( child->defaultAction() == ui->aSaveMenu ) {
			child->setMenu( mSave );
			child->setPopupMode( QToolButton::InstantPopup );
			child->setStyleSheet( "padding-left: 2px; padding-right: 10px;" );
		}

		if ( child->defaultAction() == ui->aOpenMenu ) {
			child->setMenu( mOpen );
			child->setPopupMode( QToolButton::InstantPopup );
			child->setStyleSheet( "padding-left: 2px; padding-right: 10px;" );
		}
	}

	updateRecentFileActions();

	// Lighting Menu
	// TODO: Split off into own widget
	auto mLight = lightingWidget();

	// Append Menu to tFile actions
	for ( auto child : ui->tRender->findChildren<QToolButton *>() ) {

		if ( child->defaultAction() == ui->aLightMenu ) {
			child->setMenu( mLight );
			child->setPopupMode( QToolButton::InstantPopup );
			child->setStyleSheet( "padding-left: 2px; padding-right: 10px;" );
		}
	}

}


void NifSkope::initToolBars()
{
	// Disable without NIF loaded
	ui->tRender->setEnabled( false );
	ui->tRender->setContextMenuPolicy( Qt::ActionsContextMenu );

	// Status Bar
	ui->statusbar->setContentsMargins( 0, 0, 0, 0 );
	ui->statusbar->addPermanentWidget( progress );
	
	// TODO: Split off into own widget
	ui->statusbar->addPermanentWidget( filePathWidget( this ) );


	// Render

	QActionGroup * grpView = new QActionGroup( this );
	grpView->addAction( ui->aViewTop );
	grpView->addAction( ui->aViewFront );
	grpView->addAction( ui->aViewLeft );
	grpView->addAction( ui->aViewWalk );
	grpView->setExclusive( true );


	// Animate
	connect( ui->aAnimate, &QAction::toggled, ui->tAnim, &QToolBar::setVisible );
	connect( ui->tAnim, &QToolBar::visibilityChanged, ui->aAnimate, &QAction::setChecked );

	/*enum AnimationStates
	{
		AnimDisabled = 0x0,
		AnimEnabled = 0x1,
		AnimPlay = 0x2,
		AnimLoop = 0x4,
		AnimSwitch = 0x8
	};*/

	ui->aAnimate->setData( GLView::AnimEnabled );
	ui->aAnimPlay->setData( GLView::AnimPlay );
	ui->aAnimLoop->setData( GLView::AnimLoop );
	ui->aAnimSwitch->setData( GLView::AnimSwitch );

	connect( ui->aAnimate, &QAction::toggled, ogl, &GLView::updateAnimationState );
	connect( ui->aAnimPlay, &QAction::triggered, ogl, &GLView::updateAnimationState );
	connect( ui->aAnimLoop, &QAction::toggled, ogl, &GLView::updateAnimationState );
	connect( ui->aAnimSwitch, &QAction::toggled, ogl, &GLView::updateAnimationState );

	// Animation timeline slider
	auto animSlider = new FloatSlider( Qt::Horizontal, true, true );
	auto animSliderEdit = new FloatEdit( ui->tAnim );

	animSlider->addEditor( animSliderEdit );
	animSlider->setParent( ui->tAnim );
	animSlider->setMinimumWidth( 75 );
	animSlider->setMaximumWidth( 150 );
	animSlider->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::MinimumExpanding );

	connect( ogl, &GLView::sceneTimeChanged, animSlider, &FloatSlider::set );
	connect( ogl, &GLView::sceneTimeChanged, animSliderEdit, &FloatEdit::set );
	connect( animSlider, &FloatSlider::valueChanged, ogl, &GLView::setSceneTime );
	connect( animSlider, &FloatSlider::valueChanged, animSliderEdit, &FloatEdit::setValue );
	connect( animSliderEdit, static_cast<void (FloatEdit::*)(float)>(&FloatEdit::sigEdited), ogl, &GLView::setSceneTime );
	connect( animSliderEdit, static_cast<void (FloatEdit::*)(float)>(&FloatEdit::sigEdited), animSlider, &FloatSlider::setValue );
	
	// Animations
	animGroups = new QComboBox( ui->tAnim );
	animGroups->setMinimumWidth( 60 );
	animGroups->setSizeAdjustPolicy( QComboBox::AdjustToContents );
	animGroups->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Minimum );
	connect( animGroups, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::activated), ogl, &GLView::setSceneSequence );

	ui->tAnim->addWidget( animSlider );
	animGroupsAction = ui->tAnim->addWidget( animGroups );

	connect( ogl, &GLView::sequencesDisabled, ui->tAnim, &QToolBar::hide );
	connect( ogl, &GLView::sequenceStopped, ui->aAnimPlay, &QAction::toggle );
	connect( ogl, &GLView::sequenceChanged, [this]( const QString & seqname ) {
		animGroups->setCurrentIndex( ogl->getScene()->animGroups.indexOf( seqname ) );
	} );
	connect( ogl, &GLView::sequencesUpdated, [this]() {
		ui->tAnim->show();

		animGroups->clear();
		animGroups->addItems( ogl->getScene()->animGroups );
		animGroups->setCurrentIndex( ogl->getScene()->animGroups.indexOf( ogl->getScene()->animGroup ) );

		if ( animGroups->count() == 0 ) {
			animGroupsAction->setVisible( false );
			ui->aAnimSwitch->setVisible( false );
		} else {
			ui->aAnimSwitch->setVisible( animGroups->count() != 1 );
			animGroupsAction->setVisible( true );
			animGroups->adjustSize();
		}
	} );


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

	connect( lodSlider, &QSlider::valueChanged, ogl->getScene(), &Scene::updateLodLevel );
	connect( lodSlider, &QSlider::valueChanged, Options::get(), &Options::sigChanged );
	connect( nif, &NifModel::lodSliderChanged, [tLOD]( bool enabled ) { tLOD->setEnabled( enabled ); tLOD->setVisible( enabled ); } );
}

void NifSkope::initConnections()
{
	connect( nif, &NifModel::beginUpdateHeader, this, &NifSkope::enableUi );

	connect( this, &NifSkope::beginLoading, this, &NifSkope::onLoadBegin );
	connect( this, &NifSkope::beginSave, this, &NifSkope::onSaveBegin );

	connect( this, &NifSkope::completeLoading, this, &NifSkope::onLoadComplete );
	connect( this, &NifSkope::completeSave, this, &NifSkope::onSaveComplete );
}


QMenu * NifSkope::lightingWidget()
{
	QMenu * mLight = new QMenu( this );
	mLight->setObjectName( "mLight" );

	mLight->setStyleSheet(
		R"qss(#mLight { background: #f5f5f5; padding: 8px; border: 1px solid #CCC; })qss"
	);

	auto onOffWidget = new QWidget;
	onOffWidget->setContentsMargins( 0, 0, 0, 0 );
	auto onOffLayout = new QHBoxLayout;
	onOffLayout->setContentsMargins( 0, 0, 0, 0 );
	onOffWidget->setLayout( onOffLayout );

	auto chkLighting = new QToolButton( mLight );
	chkLighting->setObjectName( "chkLighting" );
	chkLighting->setCheckable( true );
	chkLighting->setChecked( true );
	chkLighting->setDefaultAction( ui->aLighting );
	chkLighting->setIconSize( QSize( 18, 18 ) );
	chkLighting->setToolButtonStyle( Qt::ToolButtonTextBesideIcon );
	//chkLighting->setText( "    " + ui->aLighting->text() ); // Resets during toggle
	//chkLighting->setStatusTip( ui->aLighting->statusTip() ); Doesn't work
	chkLighting->setStyleSheet( R"qss( 
		#chkLighting { padding: 5px; } 
	)qss" );

	auto lightingOptionsWidget = new QWidgetAction( mLight );

	auto optionsWidget = new QWidget;
	optionsWidget->setContentsMargins( 0, 0, 0, 0 );
	auto optionsLayout = new QHBoxLayout;
	optionsLayout->setContentsMargins( 0, 0, 0, 0 );
	optionsWidget->setLayout( optionsLayout );


	auto chkFrontal = new QToolButton( mLight );
	chkFrontal->setObjectName( "chkFrontal" );
	chkFrontal->setCheckable( true );
	chkFrontal->setChecked( true );
	chkFrontal->setDefaultAction( ui->aFrontalLight );
	//chkFrontal->setIconSize( QSize( 16, 16 ) );
	//chkFrontal->setToolButtonStyle( Qt::ToolButtonTextBesideIcon );
	//chkFrontal->setStatusTip( ui->aFrontalLight->statusTip() ); Doesn't work
	chkFrontal->setStyleSheet( R"qss( #chkFrontal { padding: 5px; } )qss" );

	onOffLayout->addWidget( chkLighting );
	onOffLayout->addWidget( chkFrontal );

	// Slider lambda
	auto sld = [this]( QWidget * parent, const QString & img, int min, int max, int val ) {

		auto slider = new QSlider( Qt::Horizontal, parent );
		slider->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Maximum );
		slider->setRange( min, max );
		slider->setSingleStep( max / 4 );
		slider->setTickInterval( max / 2 );
		slider->setTickPosition( QSlider::TicksBelow );
		slider->setValue( val );
		slider->setStyleSheet( "background: transparent url(" + img + ");" + R"qss(
				background-repeat: no-repeat;
				background-position: left;
				background-origin: margin;
				background-clip: margin;
				margin-left: 20px;
			)qss" 
		);

		return slider;
	};

	// Lighting position, uses -720 to 720 and GLView divides it by 4
	//	because QSlider uses integers and we'd like less choppy motion
	auto sldDeclination = sld( chkFrontal, ":/btn/lightVertical", -720, 720, 0 );
	auto sldPlanarAngle = sld( chkFrontal, ":/btn/lightHorizontal", -720, 720, 0 );
	auto sldBrightness = sld( chkFrontal, ":/btn/sun", 0, 1440, 720 );
	auto sldAmbient = sld( chkFrontal, ":/btn/cloud", 0, 1440, 720 );

	sldDeclination->setDisabled( true );
	sldPlanarAngle->setDisabled( true );

	// Button lambda
	auto btn = [this]( QAction * act, const QString & name, QMenu * menu ) {
	
		auto button = new QToolButton( menu );
		button->setObjectName( name );
		button->setCheckable( true );
		button->setChecked( true );
		button->setDefaultAction( act );
		button->setIconSize( QSize( 16, 16 ) );
		//button->setStatusTip( ui->aTextures->statusTip() ); Doesn't work
		button->setStyleSheet( R"qss( padding: 3px 2px 2px 3px; )qss" );

		return button;
	};

	auto chkTextures = btn( ui->aTextures, "chkTextures", mLight );
	auto chkVertexColors = btn( ui->aVertexColors, "chkVertexColors", mLight );
	auto chkNormalsOnly = btn( ui->aVisNormals, "chkNormalsOnly", mLight );

	optionsLayout->addWidget( chkTextures );
	optionsLayout->addWidget( chkVertexColors );
	optionsLayout->addWidget( chkNormalsOnly );

	lightingOptionsWidget->setDefaultWidget( optionsWidget );

	auto lightingGroup = new QGroupBox( mLight );
	lightingGroup->setObjectName( "lightingGroup" );
	lightingGroup->setContentsMargins( 0, 0, 0, 0 );
	lightingGroup->setStyleSheet( R"qss( #lightingGroup { padding: 0; border: none; } )qss" );
	//lightingGroup->setDisabled( true );

	auto lightingGroupVbox = new QVBoxLayout;
	lightingGroupVbox->setContentsMargins( 0, 0, 0, 0 );
	lightingGroupVbox->setSpacing( 0 );
	lightingGroup->setLayout( lightingGroupVbox );

	lightingGroupVbox->addWidget( sldBrightness );
	lightingGroupVbox->addWidget( sldAmbient );
	lightingGroupVbox->addWidget( sldDeclination );
	lightingGroupVbox->addWidget( sldPlanarAngle );

	// Disable lighting sliders when Frontal
	connect( chkFrontal, &QToolButton::toggled, sldDeclination, &QSlider::setDisabled );
	connect( chkFrontal, &QToolButton::toggled, sldPlanarAngle, &QSlider::setDisabled );

	// Disable Frontal checkbox (and sliders) when no lighting
	connect( chkLighting, &QToolButton::toggled, chkFrontal, &QToolButton::setEnabled );
	connect( chkLighting, &QToolButton::toggled, ui->aVisNormals, &QAction::setEnabled );
	connect( chkLighting, &QToolButton::toggled, [sldDeclination, sldPlanarAngle, chkFrontal]( bool checked ) {
		if ( !chkFrontal->isChecked() ) {
			// Don't enable the sliders if Frontal is checked
			sldDeclination->setEnabled( checked );
			sldDeclination->setEnabled( checked );
		}
	} );

	// Inform ogl of changes
	//connect( chkLighting, &QCheckBox::toggled, ogl, &GLView::lightingToggled );
	connect( sldBrightness, &QSlider::valueChanged, ogl, &GLView::setBrightness );
	connect( sldAmbient, &QSlider::valueChanged, ogl, &GLView::setAmbient );
	connect( sldDeclination, &QSlider::valueChanged, ogl, &GLView::setDeclination );
	connect( sldPlanarAngle, &QSlider::valueChanged, ogl, &GLView::setPlanarAngle );
	connect( chkFrontal, &QToolButton::toggled, ogl, &GLView::setFrontalLight );


	// Set up QWidgetActions so they can be added to a QMenu
	auto lightingWidget = new QWidgetAction( mLight );
	lightingWidget->setDefaultWidget( onOffWidget );

	auto lightingAngleWidget = new QWidgetAction( mLight );
	lightingAngleWidget->setDefaultWidget( lightingGroup );

	mLight->addAction( lightingWidget );
	mLight->addAction( lightingAngleWidget );
	mLight->addAction( lightingOptionsWidget );

	return mLight;
}


QWidget * NifSkope::filePathWidget( QWidget * parent )
{
	// Show Filepath of loaded NIF
	auto filepathWidget = new QWidget( this );
	filepathWidget->setObjectName( "filepathStatusbarWidget" );
	auto filepathLayout = new QHBoxLayout( filepathWidget );
	filepathWidget->setLayout( filepathLayout );
	filepathLayout->setContentsMargins( 0, 0, 0, 0 );
	auto labelFilepath = new QLabel( "", filepathWidget );
	labelFilepath->setMinimumHeight( 16 );

	filepathLayout->addWidget( labelFilepath );

	// Navigate to Filepath
	auto navigateToFilepath = new QPushButton( "", filepathWidget );
	navigateToFilepath->setFlat( true );
	navigateToFilepath->setIcon( QIcon( ":btn/loadDark" ) );
	navigateToFilepath->setIconSize( QSize( 16, 16 ) );
	navigateToFilepath->setStatusTip( tr( "Show in Explorer" ) );

	filepathLayout->addWidget( navigateToFilepath );

	filepathWidget->setVisible( false );

	// Show Filepath on successful NIF load
	connect( this, &NifSkope::completeLoading, [this, filepathWidget, labelFilepath]( bool success, QString & fname ) {
		filepathWidget->setVisible( success );
		labelFilepath->setText( fname );
		//ui->statusbar->showMessage( tr("File Loaded Successfully"), 3000 );
	} );

	// Change Filepath on successful NIF save
	connect( this, &NifSkope::completeSave, [this, filepathWidget, labelFilepath]( bool success, QString & fname ) {
		filepathWidget->setVisible( success );
		labelFilepath->setText( fname );
		//ui->statusbar->showMessage( tr("File Saved Successfully"), 3000 );
	} );

	// Navigate to NIF in Explorer
	connect( navigateToFilepath, &QPushButton::clicked, [this]() {
#ifdef Q_OS_WIN
		QStringList args;
		args << "/select," << QDir::toNativeSeparators( currentFile );
		QProcess::startDetached( "explorer", args );
#endif
	} );


	return filepathWidget;
}


void NifSkope::openDlg()
{
	// Grab most recent filepath if blank window
	auto path = nif->getFileInfo().absolutePath();
	path = (path.isEmpty()) ? recentFileActs[0]->data().toString() : path;

	if ( !saveConfirm() )
		return;

	QStringList files = QFileDialog::getOpenFileNames( this, tr( "Open File" ), path, fileFilters() );
	if ( !files.isEmpty() )
		openFiles( files );
}

void NifSkope::onLoadBegin()
{
	setEnabled( false );
	ui->tAnim->setEnabled( false );

	progress->setVisible( true );
	progress->reset();
}

void NifSkope::onLoadComplete( bool success, QString & fname )
{
	QApplication::restoreOverrideCursor();

	// Re-enable window
	setEnabled( true ); // IMPORTANT!

	int timeout = 2500;
	if ( success ) {
		// Expand BSShaderTextureSet by default
		auto indices = nif->match( nif->index( 0, 0 ), Qt::DisplayRole, "Textures", -1, Qt::MatchRecursive );
		for ( auto i : indices ) {
			tree->expand( i );
		}

		// Scroll panel back to top
		tree->scrollTo( nif->index( 0, 0 ) );

		select( nif->getHeader() );

		header->setRootIndex( nif->getHeader() );
		ogl->setOrientation( GLView::ViewFront );

		enableUi();

	} else {
		// File failed to load
		Message::critical( this, tr( "Failed to load %1" ).arg( fname ) );

		nif->clear();
		kfm->clear();
		timeout = 0;

		// Remove from Current Files
		clearCurrentFile();

		// Reset
		currentFile = "";
		setWindowFilePath( "" );
		progress->reset();
	}

	// Mark window as unmodified
	setWindowModified( false );
	nif->undoStack->clear();

	// Hide Progress Bar
	QTimer::singleShot( timeout, progress, SLOT( hide() ) );
}


void NifSkope::saveAsDlg()
{
	QString filename = QFileDialog::getSaveFileName( this, tr( "Save File" ), nif->getFileInfo().absoluteFilePath(),
		fileFilters( false ),
		new QString( fileFilter( nif->getFileInfo().suffix() ) )
	);

	if ( filename.isEmpty() )
		return;

	saveFile( filename );
}

void NifSkope::onSaveBegin()
{
	setEnabled( false );
}

void NifSkope::onSaveComplete( bool success, QString & fname )
{
	setEnabled( true );

	if ( success ) {
		// Update if Save As results in filename change
		setWindowFilePath( currentFile );
		// Mark window as unmodified
		nif->undoStack->setClean();
		setWindowModified( false );
	}
}

bool NifSkope::saveConfirm()
{
	if ( isWindowModified() || !nif->undoStack->isClean() ) {
		QMessageBox::StandardButton response;
		response = QMessageBox::question( this,
			tr( "Save Confirmation" ),
			tr( "<h3><b>You have unsaved changes to %1.</b></h3>Would you like to save them now?" ).arg( nif->getFileInfo().completeBaseName() ),
			QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::No );

		if ( response == QMessageBox::Yes ) {
			saveAsDlg();
			return true;
		} else if ( response == QMessageBox::No ) {
			return true;
		} else if ( response == QMessageBox::Cancel ) {
			return false;
		}
	}

	return true;
}

void NifSkope::enableUi()
{
	// Re-enable toolbars, actions, and menus
	ui->aSaveMenu->setEnabled( true );
	ui->aSave->setEnabled( true );
	ui->aSaveAs->setEnabled( true );
	ui->aHeader->setEnabled( true );

	ui->mRender->setEnabled( true );
	ui->tAnim->setEnabled( true );
	animGroups->clear();


	ui->tRender->setEnabled( true );

	// We only need to enable the UI once, disconnect
	disconnect( nif, &NifModel::beginUpdateHeader, this, &NifSkope::enableUi );
}

void NifSkope::saveUi() const
{
	QSettings settings;
	// TODO: saveState takes a version number which can be incremented between releases if necessary
	settings.setValue( "UI/Window State", saveState( 0x073 ) );
	settings.setValue( "UI/Window Geometry", saveGeometry() );

	settings.setValue( "File/Auto Sanitize", aSanitize->isChecked() );

	settings.setValue( "UI/List Mode", (gListMode->checkedAction() == aList ? "list" : "hierarchy") );
	settings.setValue( "UI/Hide Mismatched Rows", aCondition->isChecked() );
	settings.setValue( "UI/Realtime Condition Updating", aRCondition->isChecked() );

	settings.setValue( "UI/List Header", list->header()->saveState() );
	settings.setValue( "UI/Tree Header", tree->header()->saveState() );
	settings.setValue( "UI/Header Header", header->header()->saveState() );
	settings.setValue( "UI/Kfmtree Header", kfmtree->header()->saveState() );

	settings.setValue( "GLView/Enable Animations", ui->aAnimate->isChecked() );
	//settings.setValue( "GLView/Play Animation", ui->aAnimPlay->isChecked() );
	//settings.setValue( "GLView/Loop Animation", ui->aAnimLoop->isChecked() );
	//settings.setValue( "GLView/Switch Animation", ui->aAnimSwitch->isChecked() );
	settings.setValue( "GLView/Perspective", ui->aViewPerspective->isChecked() );

	Options::get()->save();
}


void NifSkope::restoreUi()
{
	QSettings settings;
	restoreGeometry( settings.value( "UI/Window Geometry" ).toByteArray() );
	restoreState( settings.value( "UI/Window State" ).toByteArray(), 0x073 );

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
	header->header()->restoreState( settings.value( "UI/Header Header" ).toByteArray() );
	kfmtree->header()->restoreState( settings.value( "UI/Kfmtree Header" ).toByteArray() );

	ui->aAnimate->setChecked( settings.value( "GLView/Enable Animations", true ).toBool() );
	//ui->aAnimPlay->setChecked( settings.value( "GLView/Play Animation", true ).toBool() );
	//ui->aAnimLoop->setChecked( settings.value( "GLView/Loop Animation", true ).toBool() );
	//ui->aAnimSwitch->setChecked( settings.value( "GLView/Switch Animation", true ).toBool() );

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
	header->setFont( font );
	header->setIconSize( QSize( metrics.width( "000" ), metrics.lineSpacing() ) );
	kfmtree->setFont( font );
	kfmtree->setIconSize( QSize( metrics.width( "000" ), metrics.lineSpacing() ) );
	ogl->setFont( font );
}

void NifSkope::resizeDone()
{
	isResizing = false;

	// Unhide GLView, update GLGraphicsView
	ogl->show();
	graphicsScene->setSceneRect( graphicsView->rect() );
	graphicsView->fitInView( graphicsScene->sceneRect() );

	ogl->setUpdatesEnabled( true );
	ogl->setDisabled( false );
	ogl->getScene()->animate = true;
	ogl->update();
	ogl->resizeGL( centralWidget()->width(), centralWidget()->height() );
}


bool NifSkope::eventFilter( QObject * o, QEvent * e )
{
	// TODO: This doesn't seem to be doing anything extra
	//if ( e->type() == QEvent::Polish ) {
	//	QTimer::singleShot( 0, this, SLOT( overrideViewFont() ) );
	//}

	// Filter GLGraphicsView
	auto obj = qobject_cast<GLGraphicsView *>(o);
	if ( !obj )
		return QMainWindow::eventFilter( o, e );

	// Turn off animation
	// Grab framebuffer
	// Begin resize timer
	// Block all Resize Events to GLView
	if ( e->type() == QEvent::Resize ) {
		// Hide GLView
		ogl->hide();

		if ( !isResizing  && !resizeTimer->isActive() ) {
			ogl->getScene()->animate = false;
			ogl->updateGL();

			if ( buf.isNull() ) {
				// Init initial buffer with solid color
				//	Otherwise becomes random colors on release builds
				buf = QImage( 10, 10, QImage::Format_ARGB32 );
				buf.fill( Options::bgColor() );
			} else {
				buf = ogl->grabFrameBuffer();
			}

			ogl->setUpdatesEnabled( false );
			ogl->setDisabled( true );

			isResizing = true;
			resizeTimer->start( 300 );
		}

		return true;
	}

	// Paint stored framebuffer over GLGraphicsView while resizing
	if ( !buf.isNull() && isResizing && e->type() == QEvent::Paint ) {
		QPainter painter;
		painter.begin( static_cast<QWidget *>(o) );
		painter.drawImage( QRect( 0, 0, painter.device()->width(), painter.device()->height() ), buf );
		painter.end();

		return true;
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
	} else if ( sender() == header ) {
		idx = header->indexAt( pos );
		p = header->mapToGlobal( pos );
	} else if ( sender() == graphicsView ) {
		idx = ogl->indexAt( pos );
		p = graphicsView->mapToGlobal( pos );
	} else {
		return;
	}

	while ( idx.model() && idx.model()->inherits( "NifProxyModel" ) ) {
		idx = qobject_cast<const NifProxyModel *>(idx.model())->mapTo( idx );
	}

	SpellBook contextBook( nif, idx, this, SLOT( select( const QModelIndex & ) ) );

	if ( !idx.isValid() || nif->flags( idx ) & Qt::ItemIsEditable )
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
		reload();
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

void NifSkope::on_aBatchProcessor_triggered()
{
    BatchProcessor::create();
}

void NifSkope::on_aHeader_triggered()
{
	if ( tree )
		tree->clearRootIndex();

	select( nif->getHeader() );
}


void NifSkope::on_tRender_actionTriggered( QAction * action )
{

}

void NifSkope::on_aViewTop_triggered( bool checked )
{
	if ( checked ) {
		ogl->setOrientation( GLView::ViewTop );
	}
}

void NifSkope::on_aViewFront_triggered( bool checked )
{
	if ( checked ) {
		ogl->setOrientation( GLView::ViewFront );
	}
}

void NifSkope::on_aViewLeft_triggered( bool checked )
{
	if ( checked ) {
		ogl->setOrientation( GLView::ViewLeft );
	}
}

void NifSkope::on_aViewCenter_triggered()
{
	ogl->center();
}

void NifSkope::on_aViewFlip_triggered( bool checked )
{
	ogl->flipOrientation();
}

void NifSkope::on_aViewPerspective_toggled( bool checked )
{
	ogl->setProjection( checked );
}

void NifSkope::on_aViewWalk_triggered( bool checked )
{
	if ( checked ) {
		ogl->setOrientation( GLView::ViewWalk );
	}
}


void NifSkope::on_aViewUserSave_triggered( bool checked )
{ 
	Q_UNUSED( checked );
	ogl->saveUserView();
	ui->aViewUser->setChecked( true );
}


void NifSkope::on_aViewUser_toggled( bool checked )
{
	if ( checked ) {
		ogl->setOrientation( GLView::ViewUser, false );
		ogl->loadUserView();
	}
}

void NifSkope::on_aSettings_triggered()
{
	settingsDlg->show();
	settingsDlg->raise();
	settingsDlg->activateWindow();
}
