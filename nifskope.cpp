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
#include <QButtonGroup>
#include <QByteArray>
#include <QCheckBox>
#include <QDebug>
#include <QDesktopWidget>
#include <QDockWidget>
#include <QFontDialog>
#include <QFile>
#include <QFileDialog>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QSlider>
#include <QSpinBox>
#include <QTextEdit>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QUrl>

#include <QListView>
#include <QTreeView>

#include "kfmmodel.h"
#include "nifmodel.h"
#include "nifproxy.h"
#include "nifview.h"

#include "glview.h"
#include "spellbook.h"

#include "widgets/floatslider.h"

/*
 * main GUI window
 */

void NifSkope::about()
{
	QString text =
	"<p>NifSkope is a simple low level tool for analyzing NetImmerse '.nif' files.</p>"
	"<p>NifSkope is based on the NifTool's file format data base. "
	"For more informations visit our site at http://niftools.sourceforge.net</p>"
	"<p>Because NifSkope uses the Qt libraries it is free software. "
	"The source is available via SVN at https://svn.sourceforge.net/svnroot/niftools/trunk/nifskope</p>";

    QMessageBox mb( "About NifSkope 0.8.8", text, QMessageBox::Information, QMessageBox::Ok + QMessageBox::Default, 0, 0, this);
    mb.setIconPixmap( QPixmap( ":/res/nifskope.png" ) );
    mb.exec();
}

NifSkope::NifSkope() : QMainWindow()
{
	// create a new nif
	nif = new NifModel( this );
	connect( nif, SIGNAL( sigMessage( const Message & ) ), this, SLOT( dispatchMessage( const Message & ) ) );
	
	// create a new hierarchical proxy nif
	proxy = new NifProxyModel( this );
	proxy->setModel( nif );

	// create a new kfm model
	kfm = new KfmModel( this );
	connect( kfm, SIGNAL( sigMessage( const Message & ) ), this, SLOT( dispatchMessage( const Message & ) ) );
	
	
	// this view shows the block list
	list = new NifTreeView;
	list->setModel( proxy );
	list->setItemDelegate( nif->createDelegate() );

	connect( list, SIGNAL( clicked( const QModelIndex & ) ),
		this, SLOT( select( const QModelIndex & ) ) );
	connect( list, SIGNAL( customContextMenuRequested( const QPoint & ) ),
		this, SLOT( contextMenu( const QPoint & ) ) );
	

	// this view shows the whole nif file or the block details
	tree = new NifTreeView;
	tree->setModel( nif );
	tree->setItemDelegate( nif->createDelegate() );
	tree->header()->setStretchLastSection( false );

	connect( tree, SIGNAL( clicked( const QModelIndex & ) ),
		this, SLOT( select( const QModelIndex & ) ) );
	connect( tree, SIGNAL( customContextMenuRequested( const QPoint & ) ),
		this, SLOT( contextMenu( const QPoint & ) ) );


	// this view shows the whole kfm file
	kfmtree = new NifTreeView;
	kfmtree->setModel( kfm );
	kfmtree->setItemDelegate( kfm->createDelegate() );
	kfmtree->header()->setStretchLastSection( false );

	connect( kfmtree, SIGNAL( customContextMenuRequested( const QPoint & ) ),
		this, SLOT( contextMenu( const QPoint & ) ) );


	// open gl
	setCentralWidget( ogl = new GLView );
	ogl->setNif( nif );
	connect( ogl, SIGNAL( clicked( const QModelIndex & ) ),
			this, SLOT( select( const QModelIndex & ) ) );
	connect( ogl, SIGNAL( customContextMenuRequested( const QPoint & ) ),
		this, SLOT( contextMenu( const QPoint & ) ) );


	// actions

	aLoad = new QAction( "&Load", this );
	connect( aLoad, SIGNAL( triggered() ), this, SLOT( loadBrowse() ) );	
	aSave = new QAction( "&Save", this );
	connect( aSave, SIGNAL( triggered() ), this, SLOT( saveBrowse() ) );
	aSanitize = new QAction( "&Auto Sanitize before Save", this );
	aSanitize->setCheckable( true );
	aSanitize->setChecked( true );
	aLoadXML = new QAction( "Reload &XML", this );
	connect( aLoadXML, SIGNAL( triggered() ), this, SLOT( loadXML() ) );
	aReload = new QAction( "&Reload XML + Nif", this );
	aReload->setShortcut( Qt::ALT + Qt::Key_X );
	connect( aReload, SIGNAL( triggered() ), this, SLOT( reload() ) );
	aWindow = new QAction( "&New Window", this );
	connect( aWindow, SIGNAL( triggered() ), this, SLOT( sltWindow() ) );
	aQuit = new QAction( "&Quit", this );
	connect( aQuit, SIGNAL( triggered() ), qApp, SLOT( quit() ) );
	
	aList = new QAction( "List", this );
	aList->setCheckable( true );
	aList->setChecked( list->model() == nif );

	aHierarchy = new QAction( "Hierarchy", this );
	aHierarchy->setCheckable( true );
	aHierarchy->setChecked( list->model() == proxy );
	
	gListMode = new QActionGroup( this );
	connect( gListMode, SIGNAL( triggered( QAction * ) ), this, SLOT( setListMode() ) );
	gListMode->addAction( aList );
	gListMode->addAction( aHierarchy );
	gListMode->setExclusive( true );
	
	aSelectFont = new QAction( "Select Font ...", this );
	connect( aSelectFont, SIGNAL( triggered() ), this, SLOT( sltSelectFont() ) );
	
	aNifSkope = new QAction( "About &NifSkope", this );
	connect( aNifSkope, SIGNAL( triggered() ), this, SLOT( about() ) );
	
	aAboutQt = new QAction( "About &Qt", this );
	connect( aAboutQt, SIGNAL( triggered() ), qApp, SLOT( aboutQt() ) );
	

	// dock widgets
	
	dList = new QDockWidget( "Block List" );
	dList->setObjectName( "ListDock" );
	dList->setWidget( list );
	dList->toggleViewAction()->setShortcut( Qt::Key_F1 );
	connect( dList->toggleViewAction(), SIGNAL( toggled( bool ) ), this, SLOT( clearRoot() ) );
	
	dTree = new QDockWidget( "Block Details" );
	dTree->setObjectName( "TreeDock" );
	dTree->setWidget( tree );	
	dTree->toggleViewAction()->setShortcut( Qt::Key_F2 );
	dTree->toggleViewAction()->setChecked( false );
	dTree->setVisible( false );

	dKfm = new QDockWidget( "KFM" );
	dKfm->setObjectName( "KfmDock" );
	dKfm->setWidget( kfmtree );	
	dKfm->toggleViewAction()->setShortcut( Qt::Key_F3 );
	dKfm->toggleViewAction()->setChecked( false );
	dKfm->setVisible( false );

	addDockWidget( Qt::LeftDockWidgetArea, dList );
	addDockWidget( Qt::BottomDockWidgetArea, dTree );
	addDockWidget( Qt::RightDockWidgetArea, dKfm );


	// tool bar
	
	tool = new QToolBar( "Load & Save" );
	tool->setObjectName( "toolbar" );
	tool->setAllowedAreas( Qt::TopToolBarArea | Qt::BottomToolBarArea );
	
	tool->addAction( aLoad );
	tool->addWidget( lineLoad = new QLineEdit );
	connect( lineLoad, SIGNAL( returnPressed() ), this, SLOT( load() ) );
	
	tool->addWidget( lineSave = new QLineEdit );
	connect( lineSave, SIGNAL( returnPressed() ), this, SLOT( save() ) );
	
	tool->addAction( aSave );	
	
	addToolBar( Qt::TopToolBarArea, tool );
	
	
	// animation tool bar
	
	tAnim = new QToolBar( "Animation" );
	tAnim->setObjectName( "AnimTool" );
	tAnim->setAllowedAreas( Qt::TopToolBarArea | Qt::BottomToolBarArea );
	
	tAnim->addAction( ogl->aAnimPlay );
	
	connect( ogl->aAnimate, SIGNAL( toggled( bool ) ), tAnim->toggleViewAction(), SLOT( setChecked( bool ) ) );
	connect( ogl->aAnimate, SIGNAL( toggled( bool ) ), tAnim, SLOT( setVisible( bool ) ) );
	connect( tAnim->toggleViewAction(), SIGNAL( toggled( bool ) ), ogl->aAnimate, SLOT( setChecked( bool ) ) );
	
	FloatSlider * sldTime = new FloatSlider( Qt::Horizontal );
	tAnim->addWidget( sldTime );
	connect( ogl, SIGNAL( sigTime( float, float, float ) ), sldTime, SLOT( set( float, float, float ) ) );
	connect( sldTime, SIGNAL( valueChanged( float ) ), ogl, SLOT( sltTime( float ) ) );
	
	tAnim->addAction( ogl->aAnimLoop );
	
	addToolBar( Qt::TopToolBarArea, tAnim );
	
	// menu

	QMenu * mFile = new QMenu( "&File" );
	mFile->addAction( aLoad );
	mFile->addAction( aSave );
	mFile->addSeparator();
	mFile->addAction( aSanitize );
	mFile->addSeparator();
	mFile->addAction( aWindow );
	mFile->addSeparator();
	mFile->addAction( aLoadXML );
	mFile->addAction( aReload );
	mFile->addSeparator();
	mFile->addAction( aQuit );
	
	QMenu * mView = new QMenu( "&View" );
	mView->addAction( dList->toggleViewAction() );
	mView->addAction( dTree->toggleViewAction() );
	mView->addAction( dKfm->toggleViewAction() );
	mView->addSeparator();
	QMenu * mTools = new QMenu( "&Toolbars" );
	mView->addMenu( mTools );
	mTools->addAction( tool->toggleViewAction() );
	mTools->addAction( tAnim->toggleViewAction() );
	mView->addSeparator();
	QMenu * mViewList = new QMenu( "&Block List Options" );
	mView->addMenu( mViewList );
	mViewList->addAction( aHierarchy );
	mViewList->addAction( aList );
	mView->addAction( aSelectFont );
	
	QMenu * mOpts = new QMenu( "&Render" );
	foreach ( QAction * a, ogl->grpView->actions() )
		mOpts->addAction( a );
	mOpts->addSeparator();
	foreach ( QAction * a, ogl->actions() )
		mOpts->addAction( a );
	
	QMenu * mAbout = new QMenu( "&About" );
	mAbout->addAction( aNifSkope );
	mAbout->addAction( aAboutQt );
	
	menuBar()->addMenu( mFile );
	menuBar()->addMenu( mView );
	menuBar()->addMenu( mOpts );
	menuBar()->addMenu( new SpellBook( nif, QModelIndex(), this, SLOT( select( const QModelIndex & ) ) ) );
	menuBar()->addMenu( mAbout );
}

NifSkope::~NifSkope()
{
	QSettings settings( "NifTools", "NifSkope" );
	save( settings );
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
	lineLoad->setText( settings.value( "last load", QString( "" ) ).toString() );
	lineSave->setText( settings.value( "last save", QString( "" ) ).toString() );
	aSanitize->setChecked( settings.value( "auto sanitize", true ).toBool() );
	
	restoreHeader( "list sizes", settings, list->header() );
	if ( settings.value( "list mode", "hirarchy" ).toString() == "list" )
		aList->setChecked( true );
	else
		aHierarchy->setChecked( true );
	setListMode();

	restoreHeader( "tree sizes", settings, tree->header() );
	
	restoreHeader( "kfmtree sizes", settings, kfmtree->header() );

	ogl->restore( settings );	

	restoreState( settings.value( "window state" ).toByteArray(), 0x073 );
	
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
	QModelIndex idx = index;
	
	if ( idx.model() == proxy )
		idx = proxy->mapTo( index );
	
	if ( ! idx.isValid() || idx.model() != nif ) return;
	
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
}

void NifSkope::setListMode()
{
	QModelIndex idx = list->currentIndex();
	QAction * a = gListMode->checkedAction();
	if ( !a || a == aList )
	{
		if ( list->model() != nif )
		{
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
			list->setColumnHidden( NifModel::TempCol, true );
		}
	}
	else
	{
		if ( list->model() != proxy )
		{
			list->setModel( proxy );
			list->setItemsExpandable( true );
			list->setRootIsDecorated( true );
			QModelIndex pidx = proxy->mapFrom( idx, QModelIndex() );
			list->setCurrentIndexExpanded( pidx );
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
		if ( ! kfm->loadFromFile( nifname ) )
			qWarning() << "failed to load kfm from file" << nifname;
		nifname = kfm->get<QString>( kfm->getKFMroot(), "NIF File Name" );
		if ( ! nifname.isEmpty() )
			nifname.prepend( kfm->getFolder() + "/" );
	}
	
	bool a = ogl->aAnimate->isChecked();
	bool r = ogl->aRotate->isChecked();
	ogl->aAnimate->setChecked( false );
	ogl->aRotate->setChecked( false );
	
	if ( nifname.isEmpty() )
	{
		nif->clear();
		setWindowTitle( "NifSkope" );
	}
	else
	{
		ProgDlg prog;
		prog.setLabelText( "loading nif..." );
		prog.setRange( 0, 1 );
		prog.setValue( 0 );
		prog.setMinimumDuration( 2100 );
		connect( nif, SIGNAL( sigProgress( int, int ) ), & prog, SLOT( sltProgress( int, int ) ) );
		
		if ( ! nif->loadFromFile( nifname ) )
			qWarning() << "failed to load nif from file " << nifname;
		
		setWindowTitle( "NifSkope - " + nifname.right( nifname.length() - nifname.lastIndexOf( '/' ) - 1 ) );
	}
	
	ogl->aAnimate->setChecked( a );
	ogl->aRotate->setChecked( r );
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
		if ( ! kfm->saveToFile( nifname ) )
			qWarning() << "failed to write kfm file" << nifname;
	}
	else
	{
		if ( aSanitize->isChecked() )
		{
			QModelIndex idx = SpellBook::sanitize( nif );
			if ( idx.isValid() )
				select( idx );
		}
		if ( ! nif->saveToFile( nifname ) )
			qWarning() << "failed to write nif file " << nifname;
		setWindowTitle( "NifSkope - " + nifname.right( nifname.length() - nifname.lastIndexOf( '/' ) - 1 ) );
	}
	setEnabled( true );
}

void NifSkope::loadBrowse()
{
	// file select
	QString fn = QFileDialog::getOpenFileName( this, "Choose a file to open", lineLoad->text(), "NIFs (*.nif *.kf *.kfa *.kfm)");
	if ( !fn.isEmpty() )
	{
		lineLoad->setText( fn );
		load();
	}
}

void NifSkope::saveBrowse()
{
	// file select
	QString fn = QFileDialog::getSaveFileName( this, "Choose a file to save", lineSave->text(), "NIFs (*.nif *.kf *.kfa)");
	if ( !fn.isEmpty() )
	{
		lineSave->setText( fn );
		save();
	}
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

void NifSkope::dispatchMessage( const Message & msg )
{
	switch ( msg.type() )
	{
		case QtCriticalMsg:
			qCritical( msg );
			break;
		case QtFatalMsg:
			qFatal( msg );
			break;
		case QtWarningMsg:
			qWarning( msg );
			break;
		case QtDebugMsg:
		default:
			qDebug( msg );
			break;
	}
}

QTextEdit * msgtarget = 0;

void myMessageOutput(QtMsgType type, const char *msg)
{
	switch (type)
	{
		case QtDebugMsg:
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
			QMessageBox::critical( 0, "Fatal Error", msg );
			abort();
	}
}

int main( int argc, char * argv[] )
{
	// set up the QtApplication
	QApplication app( argc, argv );
	
	qRegisterMetaType<Message>( "Message" );
	
	qInstallMsgHandler(myMessageOutput);
	
	NifModel::loadXML();
	KfmModel::loadXML();
	
	NifSkope * skope = NifSkope::createWindow();
	
    if ( app.argc() > 1 )
        skope->load( QString( app.argv()[ app.argc() - 1 ] ) );

	// start the event loop
	return app.exec();
}
