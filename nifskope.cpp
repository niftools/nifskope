/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005-2008, NIF File Format Library and Tools
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the NIF File Format Library and Tools projectmay not be
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

// MinGW hack to ensure that GetLongPathNameW is defined
#ifdef WIN32 
#  ifdef __GNUC__
#    define WINVER 0x0500
#  endif
#endif

#include "nifskope.h"
#include "config.h"

#include <QAction>
#include <QApplication>
#include <QByteArray>
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

#include "kfmmodel.h"
#include "nifmodel.h"
#include "nifproxy.h"
#include "widgets/nifview.h"
#include "widgets/refrbrowser.h"
#include "widgets/inspect.h"

#include "glview.h"
#include "spellbook.h"
#include "widgets/fileselect.h"
#include "widgets/copyfnam.h"
#include "widgets/xmlcheck.h"

#ifdef WIN32
#  define WINDOWS_LEAN_AND_MEAN
#  include "windows.h"
#endif


#ifdef FSENGINE

#include "fsengine/fsmanager.h"

FSManager * fsmanager = 0;

#endif

// hackish workaround to undefine symbols from extra includes
#undef None
#undef Bool

void NifSkope::copySettings(QSettings & cfg, const QSettings & oldcfg, const QString name) const
{
	if ((!cfg.contains(name)) && oldcfg.contains(name)) {
		//qDebug() << "copying nifskope setting" << name;
		cfg.setValue(name, oldcfg.value(name));
	};
};

void NifSkope::migrateSettings() const
{
	// load current nifskope settings
	NIFSKOPE_QSETTINGS(cfg);
	// check for older nifskope settings
	for (QStringList::ConstIterator it = NIFSKOPE_OLDERVERSIONS.begin(); it != NIFSKOPE_OLDERVERSIONS.end(); ++it ) {
		QSettings oldcfg( "NifTools", *it );
		// check for non-binary missing keys and copy them from old settings
		QStringList keys = oldcfg.allKeys();
		for (QStringList::ConstIterator key = keys.begin(); key != keys.end(); ++key) {
			//qDebug() << "checking" << *key << oldcfg.value(*key).type(); 
			switch (oldcfg.value(*key).type()) {
				case QVariant::Bool:
				case QVariant::Int:
				case QVariant::UInt:
				case QVariant::Double:
				case QVariant::String:
				case QVariant::StringList:
					// copy settings for these types
					copySettings(cfg, oldcfg, *key);
				default:
					; // do nothing
			};
		};
	};
};

/*
 * main GUI window
 */

void NifSkope::about()
{
	QString text =
	"<p style='white-space:pre'>NifSkope is a tool for analyzing and editing NetImmerse '.nif' files.</p>"
	"<p>NifSkope is based on NifTool's XML file format specification. "
	"For more informations visit our site at <a href='http://niftools.sourceforge.net'>http://niftools.sourceforge.net</a></p>"
	"<p>NifSkope is free software available under a BSD license. "
	"The source is available via <a href='https://niftools.svn.sourceforge.net/svnroot/niftools/trunk/nifskope'>svn</a>"
	" on <a href='http://sourceforge.net'>SourceForge</a>.</p>"
	"<p>The most recent version of NifSkope can always be downloaded from the <a href='http://sourceforge.net/project/showfiles.php?group_id=149157'>"
	"NifTools SourceForge Project page</a>.</p>"
	"<center><img src=':/img/havok_logo' /></center>"
	"<p>NifSkope uses Havok(R) for the generation of mopp code. "
	"(C)Copyright 1999-2008 Havok.com Inc. (and its Licensors). "
	"All Rights Reserved. "
	"See <a href='http://www.havok.com'>www.havok.com</a> for details.</p>";

	QMessageBox mb( tr("About NifSkope "NIFSKOPE_VERSION" (revision "NIFSKOPE_REVISION")"), text, QMessageBox::Information,
		QMessageBox::Ok + QMessageBox::Default, 0, 0, this);
	mb.setIconPixmap( QPixmap( ":/res/nifskope.png" ) );
	mb.exec();
}

NifSkope::NifSkope()
	: QMainWindow(), selecting( false ), initialShowEvent( true )
{
	// migrate settings from older versions of NifSkope
	migrateSettings();

	// create a new nif
	nif = new NifModel( this );
	connect( nif, SIGNAL( sigMessage( const Message & ) ), this, SLOT( dispatchMessage( const Message & ) ) );
	
	SpellBook * book = new SpellBook( nif, QModelIndex(), this, SLOT( select( const QModelIndex & ) ) );
	
	// create a new hierarchical proxy nif
	proxy = new NifProxyModel( this );
	proxy->setModel( nif );

	// create a new kfm model
	kfm = new KfmModel( this );
	connect( kfm, SIGNAL( sigMessage( const Message & ) ), this, SLOT( dispatchMessage( const Message & ) ) );
	
	
	// this view shows the block list
	list = new NifTreeView;
	list->setModel( nif );
	list->setItemDelegate( nif->createDelegate( book ) );
	list->header()->setStretchLastSection( true );
	list->header()->setMinimumSectionSize( 100 );
	list->installEventFilter( this );

	connect( list, SIGNAL( sigCurrentIndexChanged( const QModelIndex & ) ),
		this, SLOT( select( const QModelIndex & ) ) );
	connect( list, SIGNAL( customContextMenuRequested( const QPoint & ) ),
		this, SLOT( contextMenu( const QPoint & ) ) );

	// this view shows the whole nif file or the block details
	tree = new NifTreeView;
	tree->setModel( nif );
	tree->setItemDelegate( nif->createDelegate( book ) );
	tree->header()->setStretchLastSection( false );
	tree->installEventFilter( this );

	connect( tree, SIGNAL( sigCurrentIndexChanged( const QModelIndex & ) ),
		this, SLOT( select( const QModelIndex & ) ) );
	connect( tree, SIGNAL( customContextMenuRequested( const QPoint & ) ),
		this, SLOT( contextMenu( const QPoint & ) ) );


	// this view shows the whole kfm file
	kfmtree = new NifTreeView;
	kfmtree->setModel( kfm );
	kfmtree->setItemDelegate( kfm->createDelegate() );
	kfmtree->header()->setStretchLastSection( false );
	kfmtree->installEventFilter( this );

	connect( kfmtree, SIGNAL( customContextMenuRequested( const QPoint & ) ),
		this, SLOT( contextMenu( const QPoint & ) ) );

	// this browser shows the reference of current node
	refrbrwsr = new ReferenceBrowser;

	refrbrwsr->setNifModel( nif );
	connect( tree, SIGNAL( sigCurrentIndexChanged( const QModelIndex & ) ),
		refrbrwsr, SLOT( browse( const QModelIndex & ) ) );

#ifdef EDIT_ON_ACTIVATE
	connect( list, SIGNAL( activated( const QModelIndex & ) ),
		list, SLOT( edit( const QModelIndex & ) ) );
	connect( tree, SIGNAL( activated( const QModelIndex & ) ),
		tree, SLOT( edit( const QModelIndex & ) ) );
	connect( kfmtree, SIGNAL( activated( const QModelIndex & ) ),
		kfmtree, SLOT( edit( const QModelIndex & ) ) );
#endif


	// open gl
	setCentralWidget( ogl = GLView::create() );
	ogl->setNif( nif );
	connect( ogl, SIGNAL( clicked( const QModelIndex & ) ),
			this, SLOT( select( const QModelIndex & ) ) );
	connect( ogl, SIGNAL( customContextMenuRequested( const QPoint & ) ),
		this, SLOT( contextMenu( const QPoint & ) ) );

#ifndef DISABLE_INSPECTIONVIEWER
   // this browser shows the state of the current selected item
   //   currently for showing transform state of nodes at current time
   inspect = new InspectView;
   inspect->setNifModel( nif );
   inspect->setScene( ogl->getScene() );
   connect( tree, SIGNAL( sigCurrentIndexChanged( const QModelIndex & ) ),
      inspect, SLOT( updateSelection( const QModelIndex & ) ) );
   connect( ogl, SIGNAL( sigTime( float, float, float ) ),
      inspect, SLOT( updateTime( float, float, float ) ) );
   connect( ogl, SIGNAL( paintUpdate() ), inspect, SLOT( refresh() ) );
#endif
	// actions

	aSanitize = new QAction( tr("&Auto Sanitize before Save"), this );
	aSanitize->setCheckable( true );
	aSanitize->setChecked( true );
	aLoadXML = new QAction( tr("Reload &XML"), this );
	connect( aLoadXML, SIGNAL( triggered() ), this, SLOT( loadXML() ) );
	aReload = new QAction( tr("&Reload XML + Nif"), this );
	aReload->setShortcut( Qt::ALT + Qt::Key_X );
	connect( aReload, SIGNAL( triggered() ), this, SLOT( reload() ) );
	aWindow = new QAction( tr("&New Window"), this );
	connect( aWindow, SIGNAL( triggered() ), this, SLOT( sltWindow() ) );
	aShredder = new QAction( tr("XML Checker" ), this );
	connect( aShredder, SIGNAL( triggered() ), this, SLOT( sltShredder() ) );
	aQuit = new QAction( tr("&Quit"), this );
	connect( aQuit, SIGNAL( triggered() ), qApp, SLOT( quit() ) );
	
	aList = new QAction( tr("Show Blocks in List"), this );
	aList->setCheckable( true );
	aList->setChecked( list->model() == nif );

	aHierarchy = new QAction( tr("Show Blocks in Tree"), this );
	aHierarchy->setCheckable( true );
	aHierarchy->setChecked( list->model() == proxy );
	
	gListMode = new QActionGroup( this );
	connect( gListMode, SIGNAL( triggered( QAction * ) ), this, SLOT( setListMode() ) );
	gListMode->addAction( aList );
	gListMode->addAction( aHierarchy );
	gListMode->setExclusive( true );
	
	aSelectFont = new QAction( tr("Select Font ..."), this );
	connect( aSelectFont, SIGNAL( triggered() ), this, SLOT( sltSelectFont() ) );


	/* help menu */

	aHelpWebsite = new QAction( tr("NifSkope Documentation && &Tutorials"), this );
	aHelpWebsite->setData( QUrl("http://niftools.sourceforge.net/wiki/index.php/NifSkope") );
	connect( aHelpWebsite, SIGNAL( triggered() ), this, SLOT( openURL() ) );

	aHelpForum = new QAction( tr("NifSkope Help && Bug Report &Forum"), this );
	aHelpForum->setData( QUrl("http://niftools.sourceforge.net/forum/viewforum.php?f=24") );
	connect( aHelpForum, SIGNAL( triggered() ), this, SLOT( openURL() ) );

	aNifToolsWebsite = new QAction( tr("NifTools &Wiki"), this );
	aNifToolsWebsite->setData( QUrl("http://niftools.sourceforge.net") );
	connect( aNifToolsWebsite, SIGNAL( triggered() ), this, SLOT( openURL() ) );

	aNifToolsDownloads = new QAction( tr("NifTools &Downloads"), this );
	aNifToolsDownloads->setData( QUrl("http://sourceforge.net/project/showfiles.php?group_id=149157") );
	connect( aNifToolsDownloads, SIGNAL( triggered() ), this, SLOT( openURL() ) );

	aNifSkope = new QAction( tr("About &NifSkope"), this );
	connect( aNifSkope, SIGNAL( triggered() ), this, SLOT( about() ) );
	
	aAboutQt = new QAction( tr("About &Qt"), this );
	connect( aAboutQt, SIGNAL( triggered() ), qApp, SLOT( aboutQt() ) );

#ifdef FSENGINE
	if ( fsmanager )
	{
		aResources = new QAction( tr("Resource Files"), this );
		connect( aResources, SIGNAL( triggered() ), fsmanager, SLOT( selectArchives() ) );
	}
	else
	{
		aResources = 0;
	}
#endif
	

	// dock widgets

	dRefr = new QDockWidget( tr("Interactive Help") );
	dRefr->setObjectName( "RefrDock" );
	dRefr->setWidget( refrbrwsr );
	dRefr->toggleViewAction()->setShortcut( Qt::Key_F1 );
	dRefr->toggleViewAction()->setChecked( false );
	dRefr->setVisible( false );
	
	dList = new QDockWidget( tr("Block List") );
	dList->setObjectName( "ListDock" );
	dList->setWidget( list );
	dList->toggleViewAction()->setShortcut( Qt::Key_F2 );
	connect( dList->toggleViewAction(), SIGNAL( toggled( bool ) ), tree, SLOT( clearRootIndex() ) );
	
	dTree = new QDockWidget( tr("Block Details") );
	dTree->setObjectName( "TreeDock" );
	dTree->setWidget( tree );	
	dTree->toggleViewAction()->setShortcut( Qt::Key_F3 );
	dTree->toggleViewAction()->setChecked( false );
	dTree->setVisible( false );

	dKfm = new QDockWidget( tr("KFM") );
	dKfm->setObjectName( "KfmDock" );
	dKfm->setWidget( kfmtree );	
	dKfm->toggleViewAction()->setShortcut( Qt::Key_F4 );
	dKfm->toggleViewAction()->setChecked( false );
	dKfm->setVisible( false );

#ifndef DISABLE_INSPECTIONVIEWER
   dInsp = new QDockWidget( tr("Inspect") );
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
	tool = new QToolBar( tr("Load & Save") );
	tool->setObjectName( "toolbar" );
	tool->setAllowedAreas( Qt::TopToolBarArea | Qt::BottomToolBarArea );
		
	QStringList fileExtensions( QStringList() << "*.nif" << "*.kf" << "*.kfa" << "*.kfm" << "*.nifcache" << "*.texcache" );

	// create the load portion of the toolbar
	aLineLoad = tool->addWidget( lineLoad = new FileSelector( FileSelector::LoadFile, tr("&Load..."), QBoxLayout::RightToLeft ) );
	lineLoad->setFilter( fileExtensions );
	connect( lineLoad, SIGNAL( sigActivated( const QString & ) ), this, SLOT( load() ) );
	
	// add the Load<=>Save filename copy widget
	CopyFilename * cpFilename = new CopyFilename( this );
	cpFilename->setObjectName( "fileCopyWidget" );
	connect( cpFilename, SIGNAL( leftTriggered() ),
		this, SLOT( copyFileNameSaveLoad() ) );
	connect( cpFilename, SIGNAL( rightTriggered() ),
		this, SLOT( copyFileNameLoadSave() ) );
	aCpFileName = tool->addWidget( cpFilename );
	
	// create the save portion of the toolbar
	aLineSave = tool->addWidget( lineSave = new FileSelector( FileSelector::SaveFile, tr("&Save As..."), QBoxLayout::LeftToRight ) );
	lineSave->setFilter( fileExtensions );
	connect( lineSave, SIGNAL( sigActivated( const QString & ) ), this, SLOT( save() ) );

#ifdef Q_OS_LINUX
	// extra whitespace for linux
	QWidget * extraspace = new QWidget();
	extraspace->setFixedWidth(5);
	tool->addWidget( extraspace );
#endif

	addToolBar( Qt::TopToolBarArea, tool );
	// end Load & Save toolbar
	
	// begin OpenGL toolbars
	foreach ( QToolBar * tb, ogl->toolbars() ) {
		addToolBar( Qt::TopToolBarArea, tb );
	}
	// end OpenGL toolbars

	/* ********* */
	
	// menu

	// assemble the File menu
	QMenu * mFile = new QMenu( tr("&File") );
	mFile->addActions( lineLoad->actions() );
	mFile->addActions( lineSave->actions() );
	mFile->addSeparator();
	mFile->addMenu( mImport = new QMenu( tr("Import") ) );
	mFile->addMenu( mExport = new QMenu( tr("Export") ) );
	mFile->addSeparator();
	mFile->addAction( aSanitize );
	mFile->addSeparator();
	mFile->addAction( aWindow );
	mFile->addSeparator();
	mFile->addAction( aLoadXML );
	mFile->addAction( aReload );
	mFile->addAction( aShredder );

#ifdef FSENGINE
	if ( aResources )
	{
		mFile->addSeparator();
		mFile->addAction( aResources );
	}
#endif
	mFile->addSeparator();
	mFile->addAction( aQuit );
	
	QMenu * mView = new QMenu( tr("&View") );
	mView->addAction( dRefr->toggleViewAction() );
	mView->addAction( dList->toggleViewAction() );
	mView->addAction( dTree->toggleViewAction() );
	mView->addAction( dKfm->toggleViewAction() );
   mView->addAction( dInsp->toggleViewAction() );
	mView->addSeparator();
	QMenu * mTools = new QMenu( tr("&Toolbars") );
	mView->addMenu( mTools );
	foreach ( QObject * o, children() )
	{
		QToolBar * tb = qobject_cast<QToolBar*>( o );
		if ( tb )
			mTools->addAction( tb->toggleViewAction() );
	}
	mView->addSeparator();
	mView->addAction( aHierarchy );
	mView->addAction( aList );
	mView->addSeparator();
	mView->addAction( aSelectFont );
	
	QMenu * mAbout = new QMenu( tr("&Help") );
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
	connect( mExport, SIGNAL( triggered( QAction * ) ), this, SLOT( sltImportExport( QAction * ) ) );
	connect( mImport, SIGNAL( triggered( QAction * ) ), this, SLOT( sltImportExport( QAction * ) ) );
}

NifSkope::~NifSkope()
{
}

void NifSkope::closeEvent( QCloseEvent * e )
{
	NIFSKOPE_QSETTINGS(settings);
	save( settings );
	
	QMainWindow::closeEvent( e );
}

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
	for ( int c = 0; c < header->count(); c++ )
	{
		d >> s;
		header->resizeSection( c, s );
	}
}

void NifSkope::restore( const QSettings & settings )
{
	restoreGeometry( settings.value( "window geometry" ).toByteArray() );
	restoreState( settings.value( "window state" ).toByteArray(), 0x073 );
	
	lineLoad->setText( settings.value( "last load", QString( "" ) ).toString() );
	lineSave->setText( settings.value( "last save", QString( "" ) ).toString() );
	aSanitize->setChecked( settings.value( "auto sanitize", true ).toBool() );
	
	if ( settings.value( "list mode", "hirarchy" ).toString() == "list" )
		aList->setChecked( true );
	else
		aHierarchy->setChecked( true );
	setListMode();
	
	restoreHeader( "list sizes", settings, list->header() );
	restoreHeader( "tree sizes", settings, tree->header() );
	restoreHeader( "kfmtree sizes", settings, kfmtree->header() );

	ogl->restore( settings );	

	QVariant fontVar = settings.value( "viewFont" );
	if ( fontVar.canConvert<QFont>() )
		setViewFont( fontVar.value<QFont>() );
}

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
	settings.setValue( "window state", saveState( 0x073 ) );
	settings.setValue( "window geometry", saveGeometry() );

	settings.setValue( "last load", lineLoad->text() );
	settings.setValue( "last save", lineSave->text() );
	settings.setValue( "auto sanitize", aSanitize->isChecked() );
	
	settings.setValue(	"list mode", ( gListMode->checkedAction() == aList ? "list" : "hirarchy" ) );
	saveHeader( "list sizes", settings, list->header() );

	saveHeader( "tree sizes", settings, tree->header() );
	
	saveHeader( "kfmtree sizes", settings, kfmtree->header() );

	ogl->save( settings );
}

void NifSkope::contextMenu( const QPoint & pos )
{
	QModelIndex idx;
	QPoint p = pos;
	if ( sender() == tree )
	{
		idx = tree->indexAt( pos );
		p = tree->mapToGlobal( pos );
	}
	else if ( sender() == list )
	{
		idx = list->indexAt( pos );
		p = list->mapToGlobal( pos );
	}
	else if ( sender() == ogl )
	{
		idx = ogl->indexAt( pos );
		p = ogl->mapToGlobal( pos );
	}
	else
		return;
	
	while ( idx.model() && idx.model()->inherits( "NifProxyModel" ) )
	{
		idx = qobject_cast<const NifProxyModel*>( idx.model() )->mapTo( idx );
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
	
	if ( sender() != ogl )
	{
		ogl->setCurrentIndex( idx );
	}

	if ( sender() != list )
	{
		if ( list->model() == proxy )
		{
			QModelIndex pidx = proxy->mapFrom( nif->getBlock( idx ), list->currentIndex() );
			list->setCurrentIndex( pidx );
		}
		else if ( list->model() == nif )
		{
			list->setCurrentIndex( nif->getBlockOrHeader( idx ) );
		}
	}
	
	if ( sender() != tree )
	{
		if ( dList->isVisible() )
		{
			QModelIndex root = nif->getBlockOrHeader( idx );
			if ( tree->rootIndex() != root )
				tree->setRootIndex( root );
			tree->setCurrentIndex( idx.sibling( idx.row(), 0 ) );
		}
		else
		{
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
	if ( !a || a == aList )
	{
		if ( list->model() != nif )
		{
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
	}
	else
	{
		if ( list->model() != proxy )
		{
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
	
	if ( niffile.suffix().compare( "kfm", Qt::CaseInsensitive ) == 0 )
	{
		lineLoad->rstState();
		lineSave->rstState();
		if ( !kfm->loadFromFile( niffile.filePath() ) ) {
			qWarning() << tr("failed to load kfm from '%1'").arg( niffile.filePath() );
			lineLoad->setState( FileSelector::stError );
		}
		else {
			lineLoad->setState( FileSelector::stSuccess );
			lineLoad->setText( niffile.filePath() );
			lineSave->setText( niffile.filePath() );
		}
		
		niffile.setFile( kfm->getFolder(),
			kfm->get<QString>( kfm->getKFMroot(), "NIF File Name" ) );
	}
	
	ogl->tAnim->setEnabled( false );
	
	if ( !niffile.isFile() )
	{
		nif->clear();
		lineLoad->setState( FileSelector::stError );
		setWindowTitle( "NifSkope" );
	}
	else
	{
		ProgDlg prog;
		prog.setLabelText( tr("loading nif...") );
		prog.setRange( 0, 1 );
		prog.setValue( 0 );
		prog.setMinimumDuration( 2100 );
		connect( nif, SIGNAL( sigProgress( int, int ) ), & prog, SLOT( sltProgress( int, int ) ) );
		
		lineLoad->rstState();
		lineSave->rstState();
		if ( !nif->loadFromFile( niffile.filePath() ) ) {
			qWarning() << tr("failed to load nif from '%1'").arg( niffile.filePath() );
			lineLoad->setState( FileSelector::stError );
		}
		else {
			lineLoad->setState( FileSelector::stSuccess );
			lineLoad->setText( niffile.filePath() );
			lineSave->setText( niffile.filePath() );
		}
		
		setWindowTitle( "NifSkope - " + niffile.fileName() );
	}
	
	ogl->tAnim->setEnabled( true );
	ogl->center();
	
	setEnabled( true );
	
	// work around for what is apparently a Qt 4.4.0 bug: force toolbar actions to enable again 
	aLineLoad->setEnabled( true );
	aLineSave->setEnabled( true );
	aCpFileName->setEnabled( true );
    ogl->animGroups->setEnabled( true );
    ogl->sldTime->setEnabled( true );
}

void ProgDlg::sltProgress( int x, int y )
{
	setRange( 0, y );
	setValue( x );
	qApp->processEvents();
}

void NifSkope::save()
{
	// write to file
	setEnabled( false );
	
	QString nifname = lineSave->text();
	
	if ( nifname.endsWith( ".KFM", Qt::CaseInsensitive ) )
	{
		lineSave->rstState();
		if ( ! kfm->saveToFile( nifname ) ) {
			qWarning() << tr("failed to write kfm file") << nifname;
			lineSave->setState(FileSelector::stError);
		}
		else {
			lineSave->setState(FileSelector::stSuccess);
		}
	}
	else
	{
		lineSave->rstState();
		
		if ( aSanitize->isChecked() )
		{
			QModelIndex idx = SpellBook::sanitize( nif );
			if ( idx.isValid() )
				select( idx );
		}
		
		if ( ! nif->saveToFile( nifname ) ) {
			qWarning() << tr("failed to write nif file ") << nifname;
			lineSave->setState(FileSelector::stError);
		}
		else {
			lineSave->setState(FileSelector::stSuccess);
		}

		setWindowTitle( "NifSkope - " + nifname.right( nifname.length() - nifname.lastIndexOf( '/' ) - 1 ) );
	}
	setEnabled( true );

	// work around for what is apparently a Qt 4.4.0 bug: force toolbar actions to enable again 
	aLineLoad->setEnabled( true );
	aLineSave->setEnabled( true );
	aCpFileName->setEnabled( true );
    ogl->animGroups->setEnabled( true );
    ogl->sldTime->setEnabled( true );
}

void NifSkope::copyFileNameLoadSave()
{
	if(lineLoad->text().isEmpty()) {
		return;
	}

	lineSave->replaceText( lineLoad->text() );
}

void NifSkope::copyFileNameSaveLoad()
{
	if(lineSave->text().isEmpty()) {
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
	if( !sender() ) return;

	QAction * aURL = qobject_cast<QAction*>( sender() );
	if( !aURL ) return;

	QUrl URL = aURL->data().toUrl();
	if( !URL.isValid() ) return;

	QDesktopServices::openUrl( URL );
}


NifSkope * NifSkope::createWindow( const QString & fname )
{
	NifSkope * skope = new NifSkope;
	skope->setAttribute( Qt::WA_DeleteOnClose );
	NIFSKOPE_QSETTINGS(settings);
	skope->restore( settings );
	skope->show();
	
	skope->raise();
	
	if ( ! fname.isEmpty() )
	{
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
	if ( NifModel::loadXML() )
	{
		load();
	}
}

void NifSkope::sltSelectFont()
{
	bool ok;
	QFont fnt = QFontDialog::getFont( & ok, list->font(), this );
	if ( ! ok )
		return;
	setViewFont( fnt );
	QSettings settings;
	settings.setValue( "viewFont", fnt );
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
	if ( e->type() == QEvent::Polish )
	{
		QTimer::singleShot( 0, this, SLOT( overrideViewFont() ) );
	}
	return QMainWindow::eventFilter( o, e );
}

void NifSkope::overrideViewFont()
{
	QSettings settings;
	QVariant var = settings.value( "viewFont" );
	if ( var.canConvert<QFont>() )
	{
		setViewFont( var.value<QFont>() );
	}
}

void NifSkope::dispatchMessage( const Message & msg )
{
	switch ( msg.type() )
	{
		case QtCriticalMsg:
			qCritical() << msg;
			break;
		case QtFatalMsg:
			qFatal( QString( msg ).toAscii().data() );
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

QTextEdit * msgtarget = 0;


#ifdef WIN32
class QDefaultHandlerCriticalSection
{
	CRITICAL_SECTION cs;
public:
	QDefaultHandlerCriticalSection() { InitializeCriticalSection(&cs); }
	~QDefaultHandlerCriticalSection() { DeleteCriticalSection(&cs); }
	void lock() { EnterCriticalSection(&cs); }
	void unlock() { LeaveCriticalSection(&cs); }
};

static void qDefaultMsgHandler(QtMsgType t, const char* str)
{
	Q_UNUSED(t);
	// OutputDebugString is not threadsafe.
	// cannot use QMutex here, because qWarning()s in the QMutex
	// implementation may cause this function to recurse
	static QDefaultHandlerCriticalSection staticCriticalSection;
	if (!str) str = "(null)";
	staticCriticalSection.lock();
	QT_WA({
		QString s(QString::fromLocal8Bit(str));
		s += QLatin1String("\n");
		OutputDebugStringW((TCHAR*)s.utf16());
	}, {
		QByteArray s(str);
		s += "\n";
		OutputDebugStringA(s.data());
	})
	staticCriticalSection.unlock();
}
#else
void qDefaultMsgHandler(QtMsgType t, const char* str)
{
	if (!str) str = "(null)";
	printf( "%s\n", str );
}
#endif

void myMessageOutput(QtMsgType type, const char *msg)
{
	switch (type)
	{
		case QtDebugMsg:
			qDefaultMsgHandler(type, msg);
			break;
		case QtWarningMsg:
			// workaround for Qt 4.2.2
			if ( QString( "edit: editing failed" ) == msg )
				return;
		case QtCriticalMsg:
			if ( ! msgtarget )
			{
				msgtarget = new QTextEdit;
				msgtarget->setWindowFlags( Qt::Tool | Qt::WindowStaysOnTopHint );
			}
			if ( ! msgtarget->isVisible() )
			{
				msgtarget->clear();
				msgtarget->show();
			}
			msgtarget->append( msg );
			qDefaultMsgHandler( type, msg );
			break;
		case QtFatalMsg:
			qDefaultMsgHandler( type, msg );
			QMessageBox::critical( 0, QMessageBox::tr("Fatal Error"), msg );
			abort();
	}
}


/*
 *  IPC socket
 */

IPCsocket * IPCsocket::create()
{
	QUdpSocket * udp = new QUdpSocket();
	
	if ( udp->bind( QHostAddress( QHostAddress::LocalHost ), NIFSKOPE_IPC_PORT, QUdpSocket::DontShareAddress ) )
	{
		IPCsocket * ipc = new IPCsocket( udp );
		QDesktopServices::setUrlHandler( "nif", ipc, "openNif" );
		return ipc;
	}
	
	return 0;
}

void IPCsocket::sendCommand( const QString & cmd )
{
	QUdpSocket udp;
	udp.writeDatagram( (const char *) cmd.data(), cmd.length() * sizeof( QChar ), QHostAddress( QHostAddress::LocalHost ), NIFSKOPE_IPC_PORT );
}

IPCsocket::IPCsocket( QUdpSocket * s ) : QObject(), socket( s )
{
	QObject::connect( socket, SIGNAL( readyRead() ), this, SLOT( processDatagram() ) );

#ifdef FSENGINE
	if ( ! fsmanager )
		fsmanager = FSManager::get();
#endif
}

IPCsocket::~IPCsocket()
{
	delete socket;
}

void IPCsocket::processDatagram()
{
	while ( socket->hasPendingDatagrams() )
	{
		QByteArray data;
		data.resize( socket->pendingDatagramSize() );
		QHostAddress host;
		quint16 port = 0;
		
		socket->readDatagram( data.data(), data.size(), &host, &port );
		if ( host == QHostAddress( QHostAddress::LocalHost ) && ( data.size() % sizeof( QChar ) ) == 0 )
		{
			QString cmd;
			cmd.setUnicode( (QChar *) data.data(), data.size() / sizeof( QChar ) );
			execCommand( cmd );
		}
	}
}

void IPCsocket::execCommand( const QString & cmd )
{
	if ( cmd.startsWith( "NifSkope::open" ) )
	{
		openNif( cmd.right( cmd.length() - 15 ) );
	}
}

void IPCsocket::openNif( const QUrl & url )
{
	NifSkope::createWindow( url.toString( url.scheme() == "nif" ? QUrl::RemoveScheme : QUrl::None ) );
}


// Qt does not use the System Locale consistency so this basically forces all floating
//   numbers into C format but leaves all other local specific settings.
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
};

/*
 *  main
 */

int main( int argc, char * argv[] )
{
	NifSystemLocale mLocale;

	// set up the Qt Application
	QApplication app( argc, argv );
	app.setOrganizationName( "NifTools" );
	app.setApplicationName( "NifSkope" );
	app.setOrganizationDomain( "niftools.sourceforge.net" );
	
	// install message handler
	qRegisterMetaType<Message>( "Message" );
#ifndef NO_MESSAGEHANDLER
	qInstallMsgHandler( myMessageOutput );
#endif
	
	// if there is a style sheet present then load it
	QDir qssDir;
	bool qssFound;
	// look for stylesheet in application directory
        qssDir.setPath(QApplication::applicationDirPath());
	qssFound = qssDir.exists("style.qss");
	// look for stylesheet in linux nifskope data directory
        if (!qssFound)
	{
		qssDir.setPath("/usr/share/nifskope");
		qssFound = qssDir.exists("style.qss");
	}
	// load the style sheet if present
	if (qssFound)
	{
		QFile style( qssDir.filePath( "style.qss" ) );
		if ( style.open( QFile::ReadOnly ) )
		{
			app.setStyleSheet( style.readAll() );
			style.close();
		}
	}

	// set the translation
	QString locale = QLocale::system().name();

	QTranslator translator;
	translator.load( QString( ":lang/" ) + locale );
	app.installTranslator( &translator );
	 
	NifModel::loadXML();
	KfmModel::loadXML();

	QString fname;
	bool reuseSession = true;
	for (int i=1; i<argc; ++i)
	{
		char *arg = argv[i];
		if (arg && (arg[0] == '-' || arg[0] == '/'))
		{
			switch(arg[1])
			{
			case 'i': case 'I':
				reuseSession = false;
				break;
			}
		}
		else
		{
			fname = QDir::current().filePath( arg );
		}
	}
	
   if ( !fname.isEmpty() )
	{
		//Getting a NIF file name from the OS
		fname = QDir::current().filePath( app.argv()[ app.argc() - 1 ] );

#ifdef WIN32
		//Windows passes an ugly 8.3 file path as an argument, so use a WinAPI function to fix that
		wchar_t full[MAX_PATH];
		wchar_t * temp_name = new wchar_t[fname.size() + 1];

		fname.toWCharArray( temp_name );
		temp_name[fname.size()] = 0; //The above function doesn't seem to write a null character, so add it.

		//Ensure that input is a full path, even if a partial one was given on the command line
		DWORD ret = GetFullPathNameW( temp_name, MAX_PATH, full, NULL );

		if ( ret != 0 )
		{
			delete [] temp_name;
			temp_name = new wchar_t[MAX_PATH];

			//Finally get the full long file name version of the path
			ret = GetLongPathNameW( full, temp_name, MAX_PATH );
			
			//Copy the name back to the QString variable that Qt uses
			if ( ret != 0 )
			{
				//GetLongPath succeeded
				fname = QString::fromWCharArray( temp_name);
			} else
			{
				//GetLongPath failed, use result from GetFullPathName function
				fname = QString::fromWCharArray( full );
			}
		}

		delete [] temp_name;
#endif
	}

	if ( !reuseSession ) {
		NifSkope::createWindow( fname );
		return app.exec();
	}

	if ( IPCsocket * ipc = IPCsocket::create() )
	{
		ipc->execCommand( QString( "NifSkope::open %1" ).arg( fname ) );
		return app.exec();
	}
	else
	{
		IPCsocket::sendCommand( QString( "NifSkope::open %1" ).arg( fname ) );
		return 0;
	}
}
