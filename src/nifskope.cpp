/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005-2012, NIF File Format Library and Tools
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

#include "glview.h"
#include "kfmmodel.h"
#include "nifmodel.h"
#include "nifproxy.h"
#include "spellbook.h"
#include "widgets/copyfnam.h"
#include "widgets/fileselect.h"
#include "widgets/nifview.h"
#include "widgets/refrbrowser.h"
#include "widgets/inspect.h"
#include "widgets/xmlcheck.h"

#include <QAction>
#include <QApplication>
#include <QByteArray>
#include <QComboBox>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QDockWidget>
#include <QFontDialog>
#include <QFile>
#include <QFileInfo>
#include <QHeaderView>
#include <QLocale>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QTextEdit>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QTranslator>
#include <QUdpSocket>
#include <QUrl>

#include <QListView>
#include <QTreeView>

#ifdef FSENGINE
#include <fsengine/fsmanager.h>
FSManager * fsmanager = nullptr;
#endif

#ifdef WIN32
#  define WINDOWS_LEAN_AND_MEAN
#  include "windows.h"
#endif


//! \file nifskope.cpp The main file for NifSkope


/*
 * main GUI window
 */

NifSkope::NifSkope()
	: QMainWindow(), selecting( false ), initialShowEvent( true )
{
	// init UI parts
	aboutDialog = new AboutDialog( this );

	// migrate settings from older versions of NifSkope
	migrateSettings();

	// create a new nif
	nif = new NifModel( this );
	connect( nif, &NifModel::sigMessage, this, &NifSkope::dispatchMessage );

	SpellBook * book = new SpellBook( nif, QModelIndex(), this, SLOT( select( const QModelIndex & ) ) );

	// create a new hierarchical proxy nif
	proxy = new NifProxyModel( this );
	proxy->setModel( nif );

	// create a new kfm model
	kfm = new KfmModel( this );
	connect( kfm, &KfmModel::sigMessage, this, &NifSkope::dispatchMessage );


	// this view shows the block list
	list = new NifTreeView;
	list->setModel( proxy );
	list->setItemDelegate( nif->createDelegate( book ) );
	list->header()->setStretchLastSection( true );
	list->header()->setMinimumSectionSize( 100 );
	list->installEventFilter( this );

	connect( list, &NifTreeView::sigCurrentIndexChanged, this, &NifSkope::select );
	connect( list, &NifTreeView::customContextMenuRequested, this, &NifSkope::contextMenu );

	// this view shows the whole nif file or the block details
	tree = new NifTreeView;
	tree->setModel( nif );
	tree->setItemDelegate( nif->createDelegate( book ) );
	tree->header()->setStretchLastSection( false );
	tree->installEventFilter( this );

	connect( tree, &NifTreeView::sigCurrentIndexChanged, this, &NifSkope::select );
	connect( tree, &NifTreeView::customContextMenuRequested, this, &NifSkope::contextMenu );


	// this view shows the whole kfm file
	kfmtree = new NifTreeView;
	kfmtree->setModel( kfm );
	kfmtree->setItemDelegate( kfm->createDelegate() );
	kfmtree->header()->setStretchLastSection( false );
	kfmtree->installEventFilter( this );

	connect( kfmtree, &NifTreeView::customContextMenuRequested, this, &NifSkope::contextMenu );

	// this browser shows the reference of current node
	refrbrwsr = new ReferenceBrowser;

	refrbrwsr->setNifModel( nif );
	connect( tree, &NifTreeView::sigCurrentIndexChanged, refrbrwsr, &ReferenceBrowser::browse );

#ifdef EDIT_ON_ACTIVATE
	connect( list, &NifTreeView::activated,
		list, static_cast<void (NifTreeView::*)(const QModelIndex&)>(&NifTreeView::edit) );
	connect( tree, &NifTreeView::activated,
		tree, static_cast<void (NifTreeView::*)(const QModelIndex&)>(&NifTreeView::edit) );
	connect( kfmtree, &NifTreeView::activated,
		kfmtree, static_cast<void (NifTreeView::*)(const QModelIndex&)>(&NifTreeView::edit) );
#endif


	// open gl
	setCentralWidget( ogl = GLView::create() );
	ogl->setNif( nif );
	connect( ogl, &GLView::clicked, this, &NifSkope::select );
	connect( ogl, &GLView::customContextMenuRequested, this, &NifSkope::contextMenu );

#ifndef DISABLE_INSPECTIONVIEWER
	// this browser shows the state of the current selected item
	//   currently for showing transform state of nodes at current time
	inspect = new InspectView;
	inspect->setNifModel( nif );
	inspect->setScene( ogl->getScene() );
	connect( tree, &NifTreeView::sigCurrentIndexChanged, inspect, &InspectView::updateSelection);
	connect( ogl, &GLView::sigTime, inspect, &InspectView::updateTime );
	connect( ogl, &GLView::paintUpdate, inspect, &InspectView::refresh );
#endif
	// actions

	aSanitize = new QAction( tr( "&Auto Sanitize before Save" ), this );
	aSanitize->setCheckable( true );
	aSanitize->setChecked( true );
	aLoadXML = new QAction( tr( "Reload &XML" ), this );
	connect( aLoadXML, &QAction::triggered, this, &NifSkope::loadXML );
	aReload = new QAction( tr( "&Reload XML + Nif" ), this );
	aReload->setShortcut( Qt::ALT + Qt::Key_X );
	connect( aReload, &QAction::triggered, this, &NifSkope::reload );
	aWindow = new QAction( tr( "&New Window" ), this );
	aWindow->setShortcut( QKeySequence::New );
	connect( aWindow, &QAction::triggered, this, &NifSkope::sltWindow );
	aShredder = new QAction( tr( "XML Checker" ), this );
	connect( aShredder, &QAction::triggered, this, &NifSkope::sltShredder );
	aQuit = new QAction( tr( "&Quit" ), this );
	connect( aQuit, &QAction::triggered, this, &NifSkope::close );

	aList = new QAction( tr( "Show Blocks in List" ), this );
	aList->setCheckable( true );
	aList->setChecked( list->model() == nif );

	aHierarchy = new QAction( tr( "Show Blocks in Tree" ), this );
	aHierarchy->setCheckable( true );
	aHierarchy->setChecked( list->model() == proxy );

	gListMode = new QActionGroup( this );
	connect( gListMode, &QActionGroup::triggered, this, &NifSkope::setListMode );
	gListMode->addAction( aList );
	gListMode->addAction( aHierarchy );
	gListMode->setExclusive( true );

	aCondition = new QAction( tr( "Hide Version Mismatched Rows" ), this );
	aCondition->setCheckable( true );
	aCondition->setChecked( false );

	aRCondition = new QAction( tr( "Realtime Row Version Updating (slow)" ), this );
	aRCondition->setCheckable( true );
	aRCondition->setChecked( false );
	aRCondition->setEnabled( false );

	connect( aCondition, &QAction::toggled, aRCondition, &QAction::setEnabled );
	connect( aRCondition, &QAction::toggled, tree, &NifTreeView::setRealTime );
	connect( aRCondition, &QAction::toggled, kfmtree, &NifTreeView::setRealTime );

	// use toggled to enable startup values to take effect
	connect( aCondition, &QAction::toggled, tree, &NifTreeView::setEvalConditions );
	connect( aCondition, &QAction::toggled, kfmtree, &NifTreeView::setEvalConditions );

	aSelectFont = new QAction( tr( "Select Font ..." ), this );
	connect( aSelectFont, &QAction::triggered, this, &NifSkope::sltSelectFont );


	/* help menu */

	aHelpWebsite = new QAction( tr( "NifSkope Documentation && &Tutorials" ), this );
	aHelpWebsite->setData( QUrl( "http://niftools.sourceforge.net/wiki/index.php/NifSkope" ) );
	connect( aHelpWebsite, &QAction::triggered, this, &NifSkope::openURL );

	aHelpForum = new QAction( tr( "NifSkope Help && Bug Report &Forum" ), this );
	aHelpForum->setData( QUrl( "http://niftools.sourceforge.net/forum/viewforum.php?f=24" ) );
	connect( aHelpForum, &QAction::triggered, this, &NifSkope::openURL );

	aNifToolsWebsite = new QAction( tr( "NifTools &Wiki" ), this );
	aNifToolsWebsite->setData( QUrl( "http://niftools.sourceforge.net" ) );
	connect( aNifToolsWebsite, &QAction::triggered, this, &NifSkope::openURL );

	aNifToolsDownloads = new QAction( tr( "NifTools &Downloads" ), this );
	aNifToolsDownloads->setData( QUrl( "http://sourceforge.net/project/showfiles.php?group_id=149157" ) );
	connect( aNifToolsDownloads, &QAction::triggered, this, &NifSkope::openURL );

	aNifSkope = new QAction( tr( "About &NifSkope" ), this );
	// TODO: Can't seem to figure out correct cast for new signal syntax
	connect( aNifSkope, SIGNAL( triggered() ), aboutDialog, SLOT( open() ) );

	aAboutQt = new QAction( tr( "About &Qt" ), this );
	connect( aAboutQt, &QAction::triggered, qApp, &QApplication::aboutQt );

#ifdef FSENGINE

	if ( fsmanager ) {
		aResources = new QAction( tr( "Resource Files" ), this );
		connect( aResources, &QAction::triggered, fsmanager, &FSManager::selectArchives );
	} else {
		aResources = nullptr;
	}

#endif


	// dock widgets

	dRefr = new QDockWidget( tr( "Interactive Help" ) );
	dRefr->setObjectName( "RefrDock" );
	dRefr->setWidget( refrbrwsr );
	dRefr->toggleViewAction()->setShortcut( Qt::Key_F1 );
	dRefr->toggleViewAction()->setChecked( false );
	dRefr->setVisible( false );

	dList = new QDockWidget( tr( "Block List" ) );
	dList->setObjectName( "ListDock" );
	dList->setWidget( list );
	dList->toggleViewAction()->setShortcut( Qt::Key_F2 );
	connect( dList->toggleViewAction(), &QAction::triggered, tree, &NifTreeView::clearRootIndex );

	dTree = new QDockWidget( tr( "Block Details" ) );
	dTree->setObjectName( "TreeDock" );
	dTree->setWidget( tree );
	dTree->toggleViewAction()->setShortcut( Qt::Key_F3 );

	dKfm = new QDockWidget( tr( "KFM" ) );
	dKfm->setObjectName( "KfmDock" );
	dKfm->setWidget( kfmtree );
	dKfm->toggleViewAction()->setShortcut( Qt::Key_F4 );
	dKfm->toggleViewAction()->setChecked( false );
	dKfm->setVisible( false );

#ifndef DISABLE_INSPECTIONVIEWER
	dInsp = new QDockWidget( tr( "Inspect" ) );
	dInsp->setObjectName( "InspectDock" );
	dInsp->setWidget( inspect );
	//dInsp->toggleViewAction()->setShortcut( Qt::ALT + Qt::Key_Enter );
	dInsp->toggleViewAction()->setChecked( false );
	dInsp->setVisible( false );
#endif

	addDockWidget( Qt::BottomDockWidgetArea, dRefr );
	addDockWidget( Qt::LeftDockWidgetArea, dList );
	addDockWidget( Qt::BottomDockWidgetArea, dTree );
	addDockWidget( Qt::RightDockWidgetArea, dKfm );

#ifndef DISABLE_INSPECTIONVIEWER
	addDockWidget( Qt::RightDockWidgetArea, dInsp, Qt::Vertical );
#endif

	/* ******** */

	// tool bars

	// begin Load & Save toolbar
	tool = new QToolBar( tr( "Load && Save" ) );
	tool->setObjectName( "toolbar" );
	tool->setAllowedAreas( Qt::TopToolBarArea | Qt::BottomToolBarArea );

	QStringList fileExtensions{
		"All Files (*.nif *.kf *.kfa *.kfm *.nifcache *.texcache *.pcpatch *.jmi)",
		"NIF (*.nif)", "Keyframe (*.kf)", "Keyframe Animation (*.kfa)", "Keyframe Motion (*.kfm)",
		"NIFCache (*.nifcache)", "TEXCache (*.texcache)", "PCPatch (*.pcpatch)", "JMI (*.jmi)"
	};

	// create the load portion of the toolbar
	aLineLoad = tool->addWidget( lineLoad = new FileSelector( FileSelector::LoadFile, tr( "&Load..." ), QBoxLayout::RightToLeft, QKeySequence::Open ) );
	lineLoad->setFilter( fileExtensions );
	connect( lineLoad, &FileSelector::sigActivated, this, static_cast<void (NifSkope::*)()>(&NifSkope::load) );

	// add the Load<=>Save filename copy widget
	CopyFilename * cpFilename = new CopyFilename( this );
	cpFilename->setObjectName( "fileCopyWidget" );
	connect( cpFilename, &CopyFilename::leftTriggered, this, &NifSkope::copyFileNameSaveLoad );
	connect( cpFilename, &CopyFilename::rightTriggered, this, &NifSkope::copyFileNameLoadSave );
	aCpFileName = tool->addWidget( cpFilename );

	// create the save portion of the toolbar
	aLineSave = tool->addWidget( lineSave = new FileSelector( FileSelector::SaveFile, tr( "&Save As..." ), QBoxLayout::LeftToRight, QKeySequence::Save ) );
	lineSave->setFilter( fileExtensions );
	connect( lineSave, &FileSelector::sigActivated, this, static_cast<void (NifSkope::*)()>(&NifSkope::save) );

#ifdef Q_OS_LINUX
	// extra whitespace for linux
	QWidget * extraspace = new QWidget();
	extraspace->setFixedWidth( 5 );
	tool->addWidget( extraspace );
#endif

	addToolBar( Qt::TopToolBarArea, tool );
	// end Load & Save toolbar

	// begin OpenGL toolbars
	for ( QToolBar * tb : ogl->toolbars() ) {
		addToolBar( Qt::TopToolBarArea, tb );
	}
	// end OpenGL toolbars

	// begin View toolbar
	QToolBar * tView = new QToolBar( tr( "View" ) );
	tView->setObjectName( tr( "tView" ) );
	tView->setAllowedAreas( Qt::TopToolBarArea | Qt::BottomToolBarArea );
	QAction * aResetBlockDetails = new QAction( tr( "Reset Block Details" ), this );
	connect( aResetBlockDetails, &QAction::triggered, this, &NifSkope::sltResetBlockDetails );
	tView->addAction( aResetBlockDetails );
	tView->addSeparator();
	tView->addAction( dRefr->toggleViewAction() );
	tView->addAction( dList->toggleViewAction() );
	tView->addAction( dTree->toggleViewAction() );
	tView->addAction( dKfm->toggleViewAction() );
	tView->addAction( dInsp->toggleViewAction() );
	addToolBar( Qt::TopToolBarArea, tView );
	// end View toolbars

	// LOD Toolbar
	QToolBar * tLOD = new QToolBar( "LOD" );
	tLOD->setObjectName( tr( "tLOD" ) );
	tLOD->setAllowedAreas( Qt::TopToolBarArea | Qt::BottomToolBarArea );

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

	connect( lodSlider, &QSlider::valueChanged, []( int value )
		{
			QSettings cfg;
			cfg.setValue( "GLView/LOD Level", value );
		}
	);
	connect( lodSlider, &QSlider::valueChanged, Options::get(), &Options::sigChanged );
	connect( nif, &NifModel::lodSliderChanged, [tLOD]( bool enabled ) { tLOD->setEnabled( enabled ); } );

	addToolBar( Qt::TopToolBarArea, tLOD );

	/* ********* */

	// menu

	// assemble the File menu
	QMenu * mFile = new QMenu( tr( "&File" ) );
	mFile->addActions( lineLoad->actions() );
	mFile->addActions( lineSave->actions() );
	mFile->addSeparator();
	mFile->addMenu( mImport = new QMenu( tr( "Import" ) ) );
	mFile->addMenu( mExport = new QMenu( tr( "Export" ) ) );
	mFile->addSeparator();
	mFile->addAction( aSanitize );
	mFile->addSeparator();
	mFile->addAction( aWindow );
	mFile->addSeparator();
	mFile->addAction( aLoadXML );
	mFile->addAction( aReload );
	mFile->addAction( aShredder );

#ifdef FSENGINE

	if ( aResources ) {
		mFile->addSeparator();
		mFile->addAction( aResources );
	}

#endif
	mFile->addSeparator();
	mFile->addAction( aQuit );

	QMenu * mView = new QMenu( tr( "&View" ) );
	mView->addActions( tView->actions() );
	mView->addSeparator();
	QMenu * mTools = new QMenu( tr( "&Toolbars" ) );
	mView->addMenu( mTools );
	for ( QObject * o : children() ) {
		QToolBar * tb = qobject_cast<QToolBar *>( o );

		if ( tb )
			mTools->addAction( tb->toggleViewAction() );
	}
	mView->addSeparator();
	QMenu * mBlockList = new QMenu( tr( "Block List" ) );
	mView->addMenu( mBlockList );
	mBlockList->addAction( aHierarchy );
	mBlockList->addAction( aList );
	QMenu * mBlockDetails = new QMenu( tr( "Block Details" ) );
	mView->addMenu( mBlockDetails );
	mBlockDetails->addAction( aCondition );
	mBlockDetails->addAction( aRCondition );
	mBlockDetails->addAction( aResetBlockDetails );
	mView->addSeparator();
	mView->addAction( aSelectFont );

	QMenu * mAbout = new QMenu( tr( "&Help" ) );
	mAbout->addAction( dRefr->toggleViewAction() );
	mAbout->addAction( aHelpWebsite );
	mAbout->addAction( aHelpForum );
	mAbout->addSeparator();
	mAbout->addAction( aNifToolsWebsite );
	mAbout->addAction( aNifToolsDownloads );
	mAbout->addSeparator();
	mAbout->addAction( aAboutQt );
	mAbout->addAction( aNifSkope );

	menuBar()->addMenu( mFile );
	menuBar()->addMenu( mView );
	menuBar()->addMenu( ogl->createMenu() );
	menuBar()->addMenu( book );
	menuBar()->addMenu( mAbout );

	fillImportExportMenus();
	connect( mExport, &QMenu::triggered, this, &NifSkope::sltImportExport );
	connect( mImport, &QMenu::triggered, this, &NifSkope::sltImportExport );

	connect( Options::get(), &Options::sigLocaleChanged, this, &NifSkope::sltLocaleChanged );
}

NifSkope::~NifSkope()
{
}

void NifSkope::closeEvent( QCloseEvent * e )
{
	QSettings settings;
	save( settings );

	QMainWindow::closeEvent( e );
}

//! Resize views from settings
void restoreHeader( const QString & name, const QSettings & settings, QHeaderView * header )
{
	QByteArray b = settings.value( name ).value<QByteArray>();

	if ( b.isEmpty() )
		return;

	QDataStream d( &b, QIODevice::ReadOnly );
	int s;
	d >> s;

	if ( s != header->count() )
		return;

	for ( int c = 0; c < header->count(); c++ ) {
		d >> s;
		header->resizeSection( c, s );
	}
}

void NifSkope::restore( const QSettings & settings )
{
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
	restoreHeader( "UI/List Sizes", settings, list->header() );
	restoreHeader( "UI/Tree Sizes", settings, tree->header() );
	restoreHeader( "UI/Kfmtree Sizes", settings, kfmtree->header() );

	ogl->restore( settings );

	QVariant fontVar = settings.value( "UI/View Font" );

	if ( fontVar.canConvert<QFont>() )
		setViewFont( fontVar.value<QFont>() );
}

//! Save view sizes to settings
void saveHeader( const QString & name, QSettings & settings, QHeaderView * header )
{
	QByteArray b;
	QDataStream d( &b, QIODevice::WriteOnly );
	d << header->count();

	for ( int c = 0; c < header->count(); c++ )
		d << header->sectionSize( c );

	settings.setValue( name, b );
}

void NifSkope::save( QSettings & settings ) const
{
	settings.setValue( "UI/Window State", saveState( 0x073 ) );
	settings.setValue( "UI/Window Geometry", saveGeometry() );

	settings.setValue( "File/Last Load", lineLoad->text() );
	settings.setValue( "File/Last Save", lineSave->text() );
	settings.setValue( "File/Auto Sanitize", aSanitize->isChecked() );

	settings.setValue( "UI/List Mode", ( gListMode->checkedAction() == aList ? "list" : "hierarchy" ) );
	settings.setValue( "UI/Hide Mismatched Rows", aCondition->isChecked() );
	settings.setValue( "UI/Realtime Condition Updating", aRCondition->isChecked() );

	saveHeader( "UI/List Sizes", settings, list->header() );
	saveHeader( "UI/Tree Sizes", settings, tree->header() );
	saveHeader( "UI/Kfmtree Sizes", settings, kfmtree->header() );

	ogl->save( settings );

	Options::get()->save();
}

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
		idx = qobject_cast<const NifProxyModel *>( idx.model() )->mapTo( idx );
	}

	SpellBook book( nif, idx, this, SLOT( select( const QModelIndex & ) ) );
	book.exec( p );
}

void NifSkope::select( const QModelIndex & index )
{
	if ( selecting )
		return;

	QModelIndex idx = index;

	if ( idx.model() == proxy )
		idx = proxy->mapTo( index );

	if ( idx.isValid() && idx.model() != nif )
		return;

	selecting = true;

	if ( sender() != ogl ) {
		ogl->setCurrentIndex( idx );
	}

	if ( sender() != list ) {
		if ( list->model() == proxy ) {
			QModelIndex pidx = proxy->mapFrom( nif->getBlock( idx ), list->currentIndex() );
			list->setCurrentIndex( pidx );
		} else if ( list->model() == nif ) {
			list->setCurrentIndex( nif->getBlockOrHeader( idx ) );
		}
	}

	if ( sender() != tree ) {
		if ( dList->isVisible() ) {
			QModelIndex root = nif->getBlockOrHeader( idx );

			if ( tree->rootIndex() != root )
				tree->setRootIndex( root );

			tree->setCurrentIndex( idx.sibling( idx.row(), 0 ) );

			// Expand BSShaderTextureSet by default
			//if ( root.child( 1, 0 ).data().toString() == "Textures" )
			//	tree->expandAll();

		} else {
			if ( tree->rootIndex() != QModelIndex() )
				tree->setRootIndex( QModelIndex() );

			tree->setCurrentIndex( idx.sibling( idx.row(), 0 ) );
		}
	}

	selecting = false;
}

void NifSkope::setListMode()
{
	QModelIndex idx = list->currentIndex();
	QAction * a = gListMode->checkedAction();

	if ( !a || a == aList ) {
		if ( list->model() != nif ) {
			// switch to list view
			QHeaderView * head = list->header();
			int s0 = head->sectionSize( head->logicalIndex( 0 ) );
			int s1 = head->sectionSize( head->logicalIndex( 1 ) );
			list->setModel( nif );
			list->setItemsExpandable( false );
			list->setRootIsDecorated( false );
			list->setCurrentIndex( proxy->mapTo( idx ) );
			list->setColumnHidden( NifModel::NameCol, false );
			list->setColumnHidden( NifModel::TypeCol, true );
			list->setColumnHidden( NifModel::ValueCol, false );
			list->setColumnHidden( NifModel::ArgCol, true );
			list->setColumnHidden( NifModel::Arr1Col, true );
			list->setColumnHidden( NifModel::Arr2Col, true );
			list->setColumnHidden( NifModel::CondCol, true );
			list->setColumnHidden( NifModel::Ver1Col, true );
			list->setColumnHidden( NifModel::Ver2Col, true );
			list->setColumnHidden( NifModel::VerCondCol, true );
			head->resizeSection( 0, s0 );
			head->resizeSection( 1, s1 );
		}
	} else {
		if ( list->model() != proxy ) {
			// switch to hierarchy view
			QHeaderView * head = list->header();
			int s0 = head->sectionSize( head->logicalIndex( 0 ) );
			int s1 = head->sectionSize( head->logicalIndex( 1 ) );
			list->setModel( proxy );
			list->setItemsExpandable( true );
			list->setRootIsDecorated( true );
			QModelIndex pidx = proxy->mapFrom( idx, QModelIndex() );
			list->setCurrentIndex( pidx );
			// proxy model has only two columns (see columnCount in nifproxy.h)
			list->setColumnHidden( 0, false );
			list->setColumnHidden( 1, false );
			head->resizeSection( 0, s0 );
			head->resizeSection( 1, s1 );
		}
	}
}

void NifSkope::load( const QString & filepath )
{
	lineLoad->setText( filepath );
	QTimer::singleShot( 0, this, SLOT( load() ) );
}

void NifSkope::load()
{
	setEnabled( false );

	QFileInfo niffile( QDir::fromNativeSeparators( lineLoad->text() ) );
	niffile.makeAbsolute();

	if ( niffile.suffix().compare( "kfm", Qt::CaseInsensitive ) == 0 ) {
		lineLoad->rstState();
		lineSave->rstState();

		if ( !kfm->loadFromFile( niffile.filePath() ) ) {
			qWarning() << tr( "failed to load kfm from '%1'" ).arg( niffile.filePath() );
			lineLoad->setState( FileSelector::stError );
		} else {
			lineLoad->setState( FileSelector::stSuccess );
			lineLoad->setText( niffile.filePath() );
			lineSave->setText( niffile.filePath() );
		}

		niffile.setFile( kfm->getFolder(),
			kfm->get<QString>( kfm->getKFMroot(), "NIF File Name" ) );
	}

	ogl->tAnim->setEnabled( false );

	if ( !niffile.isFile() ) {
		nif->clear();
		lineLoad->setState( FileSelector::stError );
	} else {
		ProgDlg prog;
		prog.setLabelText( tr( "loading nif..." ) );
		prog.setRange( 0, 1 );
		prog.setValue( 0 );
		prog.setMinimumDuration( 2100 );
		connect( nif, &NifModel::sigProgress, &prog, &ProgDlg::sltProgress );

		lineLoad->rstState();
		lineSave->rstState();

		if ( !nif->loadFromFile( niffile.filePath() ) ) {
			qWarning() << tr( "failed to load nif from '%1'" ).arg( niffile.filePath() );
			lineLoad->setState( FileSelector::stError );
		} else {
			lineLoad->setState( FileSelector::stSuccess );
			lineLoad->setText( niffile.filePath() );
			lineSave->setText( niffile.filePath() );
		}

		setWindowTitle( niffile.fileName() );
	}

	ogl->tAnim->setEnabled( true );
	ogl->center();

	// Expand BSShaderTextureSet by default
	auto indices = nif->match( nif->index( 0, 0 ), Qt::DisplayRole, "Textures", -1, Qt::MatchRecursive );
	for ( auto i : indices ) {
		tree->expand( i );
	}

	// Scroll panel back to top
	tree->scrollTo( nif->index( 0, 0 ) );

	setEnabled( true );
}

void ProgDlg::sltProgress( int x, int y )
{
	setRange( 0, y );
	setValue( x );
	qApp->processEvents();
}

void NifSkope::sltResetBlockDetails()
{
	if ( tree )
		tree->clearRootIndex();
}

void NifSkope::save()
{
	// write to file
	setEnabled( false );

	QString nifname = lineSave->text();

	if ( nifname.endsWith( ".KFM", Qt::CaseInsensitive ) ) {
		lineSave->rstState();

		if ( !kfm->saveToFile( nifname ) ) {
			qWarning() << tr( "failed to write kfm file" ) << nifname;
			lineSave->setState( FileSelector::stError );
		} else {
			lineSave->setState( FileSelector::stSuccess );
		}
	} else {
		lineSave->rstState();

		if ( aSanitize->isChecked() ) {
			QModelIndex idx = SpellBook::sanitize( nif );

			if ( idx.isValid() )
				select( idx );
		}

		if ( !nif->saveToFile( nifname ) ) {
			qWarning() << tr( "failed to write nif file " ) << nifname;
			lineSave->setState( FileSelector::stError );
		} else {
			lineSave->setState( FileSelector::stSuccess );
		}

		// TODO: nif->getFileInfo() returns stale data
		// Instead create tmp QFileInfo from lineSave text
		// Future: updating file info stored in nif
		QFileInfo finfo( nifname );
		setWindowTitle( finfo.fileName() );
	}

	setEnabled( true );
}

void NifSkope::copyFileNameLoadSave()
{
	if ( lineLoad->text().isEmpty() ) {
		return;
	}

	lineSave->replaceText( lineLoad->text() );
}

void NifSkope::copyFileNameSaveLoad()
{
	if ( lineSave->text().isEmpty() ) {
		return;
	}

	lineLoad->replaceText( lineSave->text() );
}

void NifSkope::sltWindow()
{
	createWindow();
}

void NifSkope::sltShredder()
{
	TestShredder::create();
}

void NifSkope::openURL()
{
	if ( !sender() )
		return;

	QAction * aURL = qobject_cast<QAction *>( sender() );

	if ( !aURL )
		return;

	QUrl URL = aURL->data().toUrl();

	if ( !URL.isValid() )
		return;

	QDesktopServices::openUrl( URL );
}


NifSkope * NifSkope::createWindow( const QString & fname )
{
	NifSkope * skope = new NifSkope;
	skope->setAttribute( Qt::WA_DeleteOnClose );
	QSettings settings;
	skope->restore( settings );
	skope->show();

	skope->raise();

	if ( !fname.isEmpty() ) {
		skope->lineLoad->setFile( fname );
		QTimer::singleShot( 0, skope, SLOT( load() ) );
	}

	return skope;
}

void NifSkope::loadXML()
{
	NifModel::loadXML();
	KfmModel::loadXML();
}

void NifSkope::reload()
{
	if ( NifModel::loadXML() ) {
		load();
	}
}

void NifSkope::sltSelectFont()
{
	bool ok;
	QFont fnt = QFontDialog::getFont( &ok, list->font(), this );

	if ( !ok )
		return;

	setViewFont( fnt );
	QSettings settings;
	settings.setValue( "UI/View Font", fnt );
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

void NifSkope::overrideViewFont()
{
	QSettings settings;
	QVariant var = settings.value( "UI/View Font" );

	if ( var.canConvert<QFont>() ) {
		setViewFont( var.value<QFont>() );
	}
}

void NifSkope::dispatchMessage( const Message & msg )
{
	switch ( msg.type() ) {
	case QtCriticalMsg:
		qCritical() << msg;
		break;
	case QtFatalMsg:
		qFatal( QString( msg ).toLatin1().data() );
		break;
	case QtWarningMsg:
		qWarning() << msg;
		break;
	case QtDebugMsg:
	default:
		qDebug() << msg;
		break;
	}
}

QTextEdit * msgtarget = nullptr;


#ifdef Q_OS_WIN32
//! Windows mutex handling
class QDefaultHandlerCriticalSection
{
	CRITICAL_SECTION cs;

public:
	QDefaultHandlerCriticalSection() { InitializeCriticalSection( &cs ); }
	~QDefaultHandlerCriticalSection() { DeleteCriticalSection( &cs ); }
	void lock() { EnterCriticalSection( &cs ); }
	void unlock() { LeaveCriticalSection( &cs ); }
};

//! Application-wide debug and warning message handler internals
static void qDefaultMsgHandler( QtMsgType t, const char * str )
{
	Q_UNUSED( t );
	// OutputDebugString is not threadsafe.
	// cannot use QMutex here, because qWarning()s in the QMutex
	// implementation may cause this function to recurse
	static QDefaultHandlerCriticalSection staticCriticalSection;

	if ( !str )
		str = "(null)";

	staticCriticalSection.lock();
	QString s( QString::fromLocal8Bit( str ) );
	s += QLatin1String( "\n" );
	OutputDebugStringW( (TCHAR *)s.utf16() );
	staticCriticalSection.unlock();
}
#else
// Doxygen won't find this unless you undef Q_OS_WIN32
//! Application-wide debug and warning message handler internals
void qDefaultMsgHandler( QtMsgType t, const char * str )
{
	if ( !str )
		str = "(null)";

	printf( "%s\n", str );
}
#endif

//! Application-wide debug and warning message handler
void myMessageOutput( QtMsgType type, const QMessageLogContext &, const QString & str )
{
	QByteArray msg = str.toLocal8Bit();
	static const QString editFailed( "edit: editing failed" );
	static const QString accessWidgetRect( "QAccessibleWidget::rect" );

	switch ( type ) {
	case QtDebugMsg:
		qDefaultMsgHandler( type, msg.constData() );
		break;
	case QtWarningMsg:

		// workaround for Qt 4.2.2
		if ( editFailed == msg )
			return;
		else if ( QString( msg ).startsWith( accessWidgetRect ) )
			return;

	case QtCriticalMsg:

		if ( !msgtarget ) {
			msgtarget = new QTextEdit;
			msgtarget->setWindowFlags( Qt::Tool | Qt::WindowStaysOnTopHint );
		}

		if ( !msgtarget->isVisible() ) {
			msgtarget->clear();
			msgtarget->show();
		}

		msgtarget->append( msg );
		qDefaultMsgHandler( type, msg.constData() );
		break;
	case QtFatalMsg:
		qDefaultMsgHandler( type, msg.constData() );
		QMessageBox::critical( 0, QMessageBox::tr( "Fatal Error" ), msg );
		// TODO: the above causes stack overflow when
		// "ASSERT: "testAttribute(Qt::WA_WState_Created)" in file kernel\qapplication_win.cpp, line 3699"
		abort();
	}
}


/*
 *  IPC socket
 */

IPCsocket * IPCsocket::create()
{
	QUdpSocket * udp = new QUdpSocket();

	if ( udp->bind( QHostAddress( QHostAddress::LocalHost ), NIFSKOPE_IPC_PORT, QUdpSocket::DontShareAddress ) ) {
		IPCsocket * ipc = new IPCsocket( udp );
		QDesktopServices::setUrlHandler( "nif", ipc, "openNif" );
		return ipc;
	}

	return 0;
}

void IPCsocket::sendCommand( const QString & cmd )
{
	QUdpSocket udp;
	udp.writeDatagram( (const char *)cmd.data(), cmd.length() * sizeof( QChar ), QHostAddress( QHostAddress::LocalHost ), NIFSKOPE_IPC_PORT );
}

IPCsocket::IPCsocket( QUdpSocket * s ) : QObject(), socket( s )
{
	QObject::connect( socket, &QUdpSocket::readyRead, this, &IPCsocket::processDatagram );

#ifdef FSENGINE

	if ( !fsmanager )
		fsmanager = FSManager::get();

#endif
}

IPCsocket::~IPCsocket()
{
	delete socket;
}

void IPCsocket::processDatagram()
{
	while ( socket->hasPendingDatagrams() ) {
		QByteArray data;
		data.resize( socket->pendingDatagramSize() );
		QHostAddress host;
		quint16 port = 0;

		socket->readDatagram( data.data(), data.size(), &host, &port );

		if ( host == QHostAddress( QHostAddress::LocalHost ) && ( data.size() % sizeof( QChar ) ) == 0 ) {
			QString cmd;
			cmd.setUnicode( (QChar *)data.data(), data.size() / sizeof( QChar ) );
			execCommand( cmd );
		}
	}
}

void IPCsocket::execCommand( const QString & cmd )
{
	if ( cmd.startsWith( "NifSkope::open" ) ) {
		openNif( cmd.right( cmd.length() - 15 ) );
	}
}

void IPCsocket::openNif( const QString & url )
{
	NifSkope::createWindow( url );
}


// TODO: This class was not used. QSystemLocale became private in Qt 5.
// It appears this class was going to handle display of numbers.
/*//! System locale override
/**
 * Qt does not use the System Locale consistency so this basically forces all floating
 * numbers into C format but leaves all other local specific settings.
 *//*
class NifSystemLocale : QSystemLocale
{
    virtual QVariant query(QueryType type, QVariant in) const
    {
        switch (type)
        {
        case DecimalPoint:
            return QVariant( QLocale::c().decimalPoint() );
        case GroupSeparator:
            return QVariant( QLocale::c().groupSeparator() );
        default:
            return QVariant();
        }
    }
};*/

static QTranslator * mTranslator = nullptr;

//! Sets application locale and loads translation files
static void SetAppLocale( QLocale curLocale )
{
	QDir directory( QApplication::applicationDirPath() );

	if ( !directory.cd( "lang" ) ) {
#ifdef Q_OS_LINUX
	if ( !directory.cd( "/usr/share/nifskope/lang" ) ) {}
#endif
	}

	QString fileName = directory.filePath( "NifSkope_" ) + curLocale.name();

	if ( !QFile::exists( fileName + ".qm" ) )
		fileName = directory.filePath( "NifSkope_" ) + curLocale.name().section( '_', 0, 0 );

	if ( !QFile::exists( fileName + ".qm" ) ) {
		if ( mTranslator ) {
			qApp->removeTranslator( mTranslator );
			delete mTranslator;
			mTranslator = nullptr;
		}
	} else {
		if ( !mTranslator ) {
			mTranslator = new QTranslator();
			qApp->installTranslator( mTranslator );
		}

		mTranslator->load( fileName );
	}
}

void NifSkope::sltLocaleChanged()
{
	SetAppLocale( Options::get()->translationLocale() );

	QMessageBox mb( "NifSkope",
	                tr( "NifSkope must be restarted for this setting to take full effect." ),
	                QMessageBox::Information, QMessageBox::Ok + QMessageBox::Default, 0, 0,
	                qApp->activeWindow()
	);
	mb.setIconPixmap( QPixmap( ":/res/nifskope.png" ) );
	mb.exec();
}

QString NifSkope::getLoadFileName()
{
	return lineLoad->text();
}

/*
 *  main
 */

//! The main program
int main( int argc, char * argv[] )
{
	// set up the Qt Application
	QApplication app( argc, argv );
	app.setOrganizationName( "NifTools" );
	app.setOrganizationDomain( "niftools.org" );
	app.setApplicationName( "NifSkope " + NifSkopeVersion::rawToMajMin( NIFSKOPE_VERSION ) );
	app.setApplicationVersion( NIFSKOPE_VERSION );
	app.setApplicationDisplayName( "NifSkope " + NifSkopeVersion::rawToDisplay( NIFSKOPE_VERSION, true ) );

	// install message handler
	qRegisterMetaType<Message>( "Message" );
#ifdef QT_NO_DEBUG
	qInstallMessageHandler( myMessageOutput );
#endif

	// if there is a style sheet present then load it
	QDir qssDir( QApplication::applicationDirPath() );
	QStringList qssList( QStringList()
	                     << "style.qss"
#ifdef Q_OS_LINUX
	                     << "/usr/share/nifskope/style.qss"
#endif
	);
	QString qssName;
	for ( const QString& str : qssList ) {
		if ( qssDir.exists( str ) ) {
			qssName = qssDir.filePath( str );
			break;
		}
	}

	// load the style sheet if present
	if ( !qssName.isEmpty() ) {
		QFile style( qssName );

		if ( style.open( QFile::ReadOnly ) ) {
			app.setStyleSheet( style.readAll() );
			style.close();
		}
	}

	QSettings cfg;
	cfg.beginGroup( "Settings" );
	SetAppLocale( cfg.value( "Language", "en" ).toLocale() );
	cfg.endGroup();

	NifModel::loadXML();
	KfmModel::loadXML();

	QStack<QString> fnames;
	bool reuseSession = true;

	// EXE is being passed arguments
	for ( int i = 1; i < argc; ++i ) {
		char * arg = argv[i];

		if ( arg && arg[0] == '-' ) {
			// Command line arguments
			// TODO: See QCommandLineParser for future
			// expansion of command line abilities.
			switch ( arg[1] ) {
			case 'i':
			case 'I':
				// TODO: Figure out the point of this
				reuseSession = false;
				break;
			}
		} else {
			//qDebug() << "arg " << i << ": " << arg;
			QString fname = QDir::current().filePath( arg );

			if ( QFileInfo( fname ).exists() ) {
				fnames.push( fname );
			}
		}
	}

	// EXE is being opened directly
	if ( fnames.isEmpty() ) {
		fnames.push( QString() );
	}

	if ( fnames.count() > 0 ) {

		// TODO: Figure out the point of this
		if ( !reuseSession ) {
			//qDebug() << "NifSkope createWindow";
			NifSkope::createWindow( fnames.pop() );
			return app.exec();
		}

		if ( IPCsocket * ipc = IPCsocket::create() ) {
			//qDebug() << "IPCSocket exec";
			ipc->execCommand( QString( "NifSkope::open %1" ).arg( fnames.pop() ) );

			while ( !fnames.isEmpty() ) {
				IPCsocket::sendCommand( QString( "NifSkope::open %1" ).arg( fnames.pop() ) );
			}

			return app.exec();
		} else {
			//qDebug() << "IPCSocket send";
			IPCsocket::sendCommand( QString( "NifSkope::open %1" ).arg( fnames.pop() ) );
			return 0;
		}

	}
}


void NifSkope::migrateSettings() const
{
	// IMPORTANT:
	//	Do not make any calls to Options:: until after all migration code.
	//	Static calls to Options:: still create the options instance and inits
	//	the various widgets with incorrect values. Once you close the app, 
	//	the settings you migrated get overwritten with the default values.

	// Load current NifSkope settings
	QSettings cfg;
	// Load pre-1.2 NifSkope settings
	QSettings cfg1_1( "NifTools", "NifSkope" );

	// Current version strings
	QString curVer = NIFSKOPE_VERSION;
	QString curQtVer = QT_VERSION_STR;
	QString curDisplayVer = NifSkopeVersion::rawToDisplay( NIFSKOPE_VERSION, true );

	bool doMigration = false;

	// New Install, no need to migrate anything
	if ( !cfg.value( "Version" ).isValid() && !cfg1_1.value( "version" ).isValid() ) {
		// QSettings constructor creates an empty folder, so clear it. 
		cfg1_1.clear();

		// Set version values
		cfg.setValue( "Version", curVer );
		cfg.setValue( "Qt Version", curQtVer );
		cfg.setValue( "Display Version", curDisplayVer );

		return;
	}

	// Forward dec for prevVer which either comes from `cfg` or `cfg1_1`
	QString prevVer;
	QString prevQtVer = cfg.value( "Qt Version" ).toString();
	QString prevDisplayVer = cfg.value( "Display Version" ).toString();

	// Set full granularity for version comparisons
	NifSkopeVersion::setNumParts( 7 );

	// Check for Existing 1.1 Migration
	if ( !cfg1_1.value( "migrated" ).isValid() ) {
		// Old install has not been migrated yet

		// Get prevVer from pre-1.2 settings
		prevVer = cfg1_1.value( "version" ).toString();

		NifSkopeVersion tmp( prevVer );
		if ( tmp < "1.2.0" )
			doMigration = true;
	} else {
		// Get prevVer from post-1.2 settings
		prevVer = cfg.value( "Version" ).toString();
	}

	NifSkopeVersion oldVersion( prevVer );
	NifSkopeVersion newVersion( curVer );

	// Migrate from 1.1.x to 1.2
	if ( doMigration && (oldVersion < "1.2.0") ) {
		// Port old key values to new key names
		QHash<QString, QString>::const_iterator i;
		for ( i = migrateTo1_2.begin(); i != migrateTo1_2.end(); ++i ) {
			QVariant val = cfg1_1.value( i.key() );

			if ( val.isValid() ) {
				cfg.setValue( i.value(), val );
			}
		}

		// Set `migrated` flag in legacy QSettings
		cfg1_1.setValue( "migrated", true );
	}

	// Check NifSkope Version
	//	Assure full granularity here
	NifSkopeVersion::setNumParts( 7 );
	if ( oldVersion != newVersion ) {
		// Set new Version
		cfg.setValue( "Version", curVer );

		if ( prevDisplayVer != curDisplayVer )
			cfg.setValue( "Display Version", curDisplayVer );
	}

	// Check Qt Version
	if ( curQtVer != prevQtVer ) {
		// Check all keys and delete all QByteArrays
		// to prevent portability problems between Qt versions
		QStringList keys = cfg.allKeys();

		for ( const auto& key : keys ) {
			if ( cfg.value( key ).type() == QVariant::ByteArray ) {
				qDebug() << "Removing Qt version-specific settings" << key
					<< "while migrating settings from previous version";
				cfg.remove( key );
			}
		}

		cfg.setValue( "Qt Version", curQtVer );
	}
}
