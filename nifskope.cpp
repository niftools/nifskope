/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005, NIF File Format Library and Tools
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

#include "nifskope.h"

#include <QAction>
#include <QApplication>
#include <QByteArray>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QDockWidget>
#include <QFontDialog>
#include <QFile>
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

#include "glview.h"
#include "spellbook.h"
#include "widgets/fileselect.h"
#include "widgets/copyfnam.h"


#ifdef FSENGINE

#include "fsengine/fsmanager.h"

FSManager * fsmanager = 0;

#endif


/*
 * main GUI window
 */

void NifSkope::about()
{
	QString text =
	"<p style='white-space:pre'>NifSkope is a tool for analyzing and editing NetImmerse '.nif' files.</p>"
	"<p>NifSkope is based on NifTool's file format data base. "
	"For more informations visit our site at <a href='http://www.niftools.org'>www.niftools.org</a></p>"
	"<p>Because NifSkope was build using the Qt GUI toolkit it is free software. "
	"The source is available via <a href='https://svn.sourceforge.net/svnroot/niftools/trunk/nifskope'>svn</a>"
	" on <a href='http://sourceforge.net'>SourceForge</a></p>";

    QMessageBox mb( tr("About NifSkope 0.9.6"), text, QMessageBox::Information, QMessageBox::Ok + QMessageBox::Default, 0, 0, this);
    mb.setIconPixmap( QPixmap( ":/res/nifskope.png" ) );
    mb.exec();
}

NifSkope::NifSkope()
	: QMainWindow(), selecting( false ), initialShowEvent( true )
{
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
	list->setModel( proxy );
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
	aQuit = new QAction( tr("&Quit"), this );
	connect( aQuit, SIGNAL( triggered() ), qApp, SLOT( quit() ) );
	
	aList = new QAction( tr("List"), this );
	aList->setCheckable( true );
	aList->setChecked( list->model() == nif );

	aHierarchy = new QAction( tr("Hierarchy"), this );
	aHierarchy->setCheckable( true );
	aHierarchy->setChecked( list->model() == proxy );
	
	gListMode = new QActionGroup( this );
	connect( gListMode, SIGNAL( triggered( QAction * ) ), this, SLOT( setListMode() ) );
	gListMode->addAction( aList );
	gListMode->addAction( aHierarchy );
	gListMode->setExclusive( true );
	
	aSelectFont = new QAction( tr("Select Font ..."), this );
	connect( aSelectFont, SIGNAL( triggered() ), this, SLOT( sltSelectFont() ) );
	
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
	
	dList = new QDockWidget( tr("Block List") );
	dList->setObjectName( "ListDock" );
	dList->setWidget( list );
	dList->toggleViewAction()->setShortcut( Qt::Key_F1 );
	connect( dList->toggleViewAction(), SIGNAL( toggled( bool ) ), this, SLOT( clearRoot() ) );
	
	dTree = new QDockWidget( tr("Block Details") );
	dTree->setObjectName( "TreeDock" );
	dTree->setWidget( tree );	
	dTree->toggleViewAction()->setShortcut( Qt::Key_F2 );
	dTree->toggleViewAction()->setChecked( false );
	dTree->setVisible( false );

	dKfm = new QDockWidget( tr("KFM") );
	dKfm->setObjectName( "KfmDock" );
	dKfm->setWidget( kfmtree );	
	dKfm->toggleViewAction()->setShortcut( Qt::Key_F3 );
	dKfm->toggleViewAction()->setChecked( false );
	dKfm->setVisible( false );

	addDockWidget( Qt::LeftDockWidgetArea, dList );
	addDockWidget( Qt::BottomDockWidgetArea, dTree );
	addDockWidget( Qt::RightDockWidgetArea, dKfm );

	/* ******** */

	// tool bars

	// begin Load & Save toolbar
	tool = new QToolBar( tr("Load & Save") );
	tool->setObjectName( "toolbar" );
	tool->setAllowedAreas( Qt::TopToolBarArea | Qt::BottomToolBarArea );
		
	QStringList fileExtensions( QStringList() << "*.nif" << "*.kf" << "*.kfa" << "*.kfm" );

	// create the load portion of the toolbar
	tool->addWidget( lineLoad = new FileSelector( FileSelector::LoadFile, tr("&Load..."), QBoxLayout::RightToLeft ) );
	lineLoad->setFilter( fileExtensions );
	connect( lineLoad, SIGNAL( sigActivated( const QString & ) ), this, SLOT( load() ) );
	
	// add the Load<=>Save filename copy widget
	CopyFilename * cpFilename = new CopyFilename( this );
	cpFilename->setObjectName( "fileCopyWidget" );
	connect( cpFilename, SIGNAL( leftTriggered() ),
		this, SLOT( copyFileNameSaveLoad() ) );
	connect( cpFilename, SIGNAL( rightTriggered() ),
		this, SLOT( copyFileNameLoadSave() ) );
	tool->addWidget( cpFilename );
	
	// create the save portion of the toolbar
	tool->addWidget( lineSave = new FileSelector( FileSelector::SaveFile, tr("&Save..."), QBoxLayout::LeftToRight ) );
	lineSave->setFilter( fileExtensions );
	connect( lineSave, SIGNAL( sigActivated( const QString & ) ), this, SLOT( save() ) );
	
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
	mFile->addAction( aSanitize );
	mFile->addSeparator();
	mFile->addAction( aWindow );
	mFile->addSeparator();
	mFile->addAction( aLoadXML );
	mFile->addAction( aReload );

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
	mView->addAction( dList->toggleViewAction() );
	mView->addAction( dTree->toggleViewAction() );
	mView->addAction( dKfm->toggleViewAction() );
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
	QMenu * mViewList = new QMenu( tr("&Block List Options") );
	mView->addMenu( mViewList );
	mViewList->addAction( aHierarchy );
	mViewList->addAction( aList );
	mView->addAction( aSelectFont );
	
	QMenu * mAbout = new QMenu( tr("&About") );
	mAbout->addAction( aNifSkope );
	mAbout->addAction( aAboutQt );
	
	menuBar()->addMenu( mFile );
	menuBar()->addMenu( mView );
	menuBar()->addMenu( ogl->createMenu() );
	menuBar()->addMenu( book );
	menuBar()->addMenu( mAbout );
}

NifSkope::~NifSkope()
{
}

void NifSkope::closeEvent( QCloseEvent * e )
{
	QSettings settings( "NifTools", "NifSkope" );
	save( settings );
	
	QMainWindow::closeEvent( e );
}

void restoreHeader( const QString & name, QSettings & settings, QHeaderView * header )
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

void NifSkope::restore( QSettings & settings )
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
	
	if ( idx.model() == proxy )
		idx = proxy->mapTo( idx );
	
	SpellBook book( nif, idx, this, SLOT( select( const QModelIndex & ) ) );
	book.exec( p );
}

void NifSkope::clearRoot()
{
	QModelIndex index = tree->currentIndex();
	if ( ! index.isValid() ) index = tree->rootIndex();
	tree->setRootIndex( QModelIndex() );
	tree->setCurrentIndexExpanded( index );
}

void NifSkope::select( const QModelIndex & index )
{
	if ( selecting )
		return;
	
	QModelIndex idx = index;
	
	if ( idx.model() == proxy )
		idx = proxy->mapTo( index );
	
	if ( ! idx.isValid() || idx.model() != nif ) return;
	
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
			list->setCurrentIndexExpanded( pidx );
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
			tree->setCurrentIndexExpanded( idx.sibling( idx.row(), 0 ) );
		}
		else
		{
			if ( tree->rootIndex() != QModelIndex() )
				tree->setRootIndex( QModelIndex() );
			tree->setCurrentIndexExpanded( idx.sibling( idx.row(), 0 ) );
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
			QHeaderView * head = list->header();
			int s0 = head->sectionSize( head->logicalIndex( 0 ) );
			int s1 = head->sectionSize( head->logicalIndex( 1 ) );
			list->setModel( nif );
			list->setItemsExpandable( false );
			list->setRootIsDecorated( false );
			list->setCurrentIndexExpanded( proxy->mapTo( idx ) );
			list->setColumnHidden( NifModel::TypeCol, true );
			list->setColumnHidden( NifModel::ArgCol, true );
			list->setColumnHidden( NifModel::Arr1Col, true );
			list->setColumnHidden( NifModel::Arr2Col, true );
			list->setColumnHidden( NifModel::CondCol, true );
			list->setColumnHidden( NifModel::Ver1Col, true );
			list->setColumnHidden( NifModel::Ver2Col, true );
			head->resizeSection( 0, s0 );
			head->resizeSection( 1, s1 );
		}
	}
	else
	{
		if ( list->model() != proxy )
		{
			QHeaderView * head = list->header();
			int s0 = head->sectionSize( head->logicalIndex( 0 ) );
			int s1 = head->sectionSize( head->logicalIndex( 1 ) );
			list->setModel( proxy );
			list->setItemsExpandable( true );
			list->setRootIsDecorated( true );
			QModelIndex pidx = proxy->mapFrom( idx, QModelIndex() );
			list->setCurrentIndexExpanded( pidx );
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

	QString nifname = lineLoad->text();
	
	if ( nifname.endsWith( ".KFM", Qt::CaseInsensitive ) )
	{
		if ( ! kfm->loadFromFile( nifname ) ) {
			qWarning() << tr("failed to load kfm from file") << nifname;
			lineLoad->setState( FileSelector::StateError );
		}
		else {
			lineLoad->setState( FileSelector::StateSuccess );
			lineSave->setText( lineLoad->text() );
		}

		nifname = kfm->get<QString>( kfm->getKFMroot(), "NIF File Name" );
		if ( ! nifname.isEmpty() ) {
			nifname.prepend( kfm->getFolder() + "/" );
		}
	}
	
	bool a = ogl->aAnimate->isChecked();
	ogl->aAnimate->setChecked( false );
	
	if ( nifname.isEmpty() )
	{
		nif->clear();
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
		
		if ( ! nif->loadFromFile( nifname ) ) {
			qWarning() << tr("failed to load nif from file ") << nifname;
			lineLoad->setState( FileSelector::StateError );
		}
		else {
			lineLoad->setState( FileSelector::StateSuccess );
			lineSave->setText( lineLoad->text() );
		}
		
		setWindowTitle( "NifSkope - " + nifname.right( nifname.length() - nifname.lastIndexOf( '/' ) - 1 ) );
	}
	
	ogl->aAnimate->setChecked( a );
	ogl->center();
	
	setEnabled( true );
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
		if ( ! kfm->saveToFile( nifname ) ) {
			qWarning() << tr("failed to write kfm file") << nifname;
			lineSave->setState(FileSelector::StateError);
		}
		else {
			lineSave->setState(FileSelector::StateSuccess);
		}
	}
	else
	{
		if ( aSanitize->isChecked() )
		{
			QModelIndex idx = SpellBook::sanitize( nif );
			if ( idx.isValid() )
				select( idx );
		}

		if ( ! nif->saveToFile( nifname ) ) {
			qWarning() << tr("failed to write nif file ") << nifname;
			lineSave->setState(FileSelector::StateError);
		}
		else {
			lineSave->setState(FileSelector::StateSuccess);
		}

		setWindowTitle( "NifSkope - " + nifname.right( nifname.length() - nifname.lastIndexOf( '/' ) - 1 ) );
	}
	setEnabled( true );
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

NifSkope * NifSkope::createWindow( const QString & fname )
{
	NifSkope * skope = new NifSkope;
	skope->setAttribute( Qt::WA_DeleteOnClose );
	QSettings settings( "NifTools", "NifSkope" );
	skope->restore( settings );
	skope->show();
	if ( ! fname.isEmpty() )
		skope->load( fname );
	skope->raise();
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
	QSettings settings( "NifTools", "NifSkope" );
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
}

bool NifSkope::eventFilter( QObject * o, QEvent * e )
{
	if ( e->type() == QEvent::Polish )
	{
		QTimer::singleShot( 0, this, SLOT( overrideViewFont() ) );
	}
	else if ( e->type() == QEvent::Show )
	{
		if ( initialShowEvent )
		{
			initialShowEvent = false;
			QSettings settings;
			restoreGeometry( settings.value( "window geometry" ).toByteArray() );
		}
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

void myMessageOutput(QtMsgType type, const char *msg)
{
	switch (type)
	{
		case QtDebugMsg:
			printf( "%s\n", msg );
			break;
		case QtWarningMsg:
			// workaround for Qt 4.1.3
			if ( QString( "QEventDispatcherUNIX::unregisterTimer: invalid argument" ) == msg )
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
			break;
		case QtFatalMsg:
			QMessageBox::critical( 0, QMessageBox::tr("Fatal Error"), msg );
			abort();
	}
}


/*
 *  IPC socket
 */

bool IPCsocket::nifskope( const QString & cmd )
{
	QUdpSocket * udp = new QUdpSocket();
	
	if ( udp->bind( QHostAddress( QHostAddress::LocalHost ), NIFSKOPE_IPC_PORT, QUdpSocket::DontShareAddress ) )
	{
		IPCsocket * ipc = new IPCsocket( udp );
		QDesktopServices::setUrlHandler( "nif", ipc, "openNif" );
		ipc->execCommand( cmd );
		return true;
	}
	else
	{
		udp->writeDatagram( (const char *) cmd.data(), cmd.length() * sizeof( QChar ), QHostAddress( QHostAddress::LocalHost ), NIFSKOPE_IPC_PORT );
		delete udp;
		return false;
	}
}

IPCsocket::IPCsocket( QUdpSocket * s ) : QObject(), socket( s )
{
	QObject::connect( socket, SIGNAL( readyRead() ), this, SLOT( processDatagram() ) );

#ifdef FSENGINE
	if ( ! fsmanager )
		fsmanager = new FSManager( this );
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

void IPCsocket::execCommand( QString cmd )
{
	if ( cmd.startsWith( "NifSkope::open" ) )
	{
		cmd.remove( 0, 15 );
		openNif( cmd );
	}
}

void IPCsocket::openNif( const QUrl & url )
{
	NifSkope::createWindow( url.toString( url.scheme() == "nif" ? QUrl::RemoveScheme : QUrl::None ) );
}


/*
 *  main
 */

int main( int argc, char * argv[] )
{
	// set up the Qt Application
	QApplication app( argc, argv );
	app.setOrganizationName( "NifTools" );
	app.setApplicationName( "NifSkope" );
	app.setOrganizationDomain( "niftools.org" );
	
	// install message handler
	qRegisterMetaType<Message>( "Message" );
#ifndef NO_MESSAGEHANDLER
	qInstallMsgHandler( myMessageOutput );
#endif
	
	// if there is a style sheet present then load it
	QFile style( QDir( QApplication::applicationDirPath() ).filePath( "style.qss" ) );
	if ( style.open( QFile::ReadOnly ) )
	{
		app.setStyleSheet( style.readAll() );
		style.close();
	}
	
	// set the translation
	QString locale = QLocale::system().name();

	QTranslator translator;
	translator.load( QString( ":lang/" ) + locale );
	app.installTranslator( &translator );
	 
	NifModel::loadXML();
	KfmModel::loadXML();
	
	QString fname;
    if ( app.argc() > 1 )
        fname = QDir::current().filePath( QString( app.argv()[ app.argc() - 1 ] ) );
	
	if ( IPCsocket::nifskope( QString( "NifSkope::open %1" ).arg( fname ) ) )
		// start the event loop
		return app.exec();
	else
		return 0;
}
