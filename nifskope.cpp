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

#include "nifmodel.h"
#include "nifproxy.h"
#include "nifview.h"

#include "glview.h"
#include "spellbook.h"

/*
 * main GUI window
 */

NifSkope::NifSkope() : QMainWindow()
{
	// create a new model
	model = new NifModel( this );
	connect( model, SIGNAL( sigMessage( const Message & ) ), this, SLOT( dispatchMessage( const Message & ) ) );
	
	// create a new hierarchical proxy model
	proxy = new NifProxyModel( this );
	proxy->setModel( model );


	// this view shows the block list
	list = new NifTreeView;
	list->setModel( proxy );
	list->setItemDelegate( model->createDelegate() );

	QFont font = list->font();
	font.setPointSize( font.pointSize() + 3 );
	list->setFont( font );
	QFontMetrics metrics( list->font() );
	list->setIconSize( QSize( metrics.width( "000" ), metrics.lineSpacing() ) );
	list->header()->setStretchLastSection( false );
	
	list->setColumnHidden( NifModel::TypeCol, true );
	list->setColumnHidden( NifModel::ArgCol, true );
	list->setColumnHidden( NifModel::Arr1Col, true );
	list->setColumnHidden( NifModel::Arr2Col, true );
	list->setColumnHidden( NifModel::CondCol, true );
	list->setColumnHidden( NifModel::Ver1Col, true );
	list->setColumnHidden( NifModel::Ver2Col, true );
	
	connect( list, SIGNAL( clicked( const QModelIndex & ) ),
		this, SLOT( select( const QModelIndex & ) ) );
	connect( list, SIGNAL( customContextMenuRequested( const QPoint & ) ),
		this, SLOT( contextMenu( const QPoint & ) ) );
	

	// this view shows the whole nif file or the block details
	tree = new NifTreeView;
	tree->setModel( model );
	tree->setItemDelegate( model->createDelegate() );

	tree->setFont( font );
	tree->setIconSize( QSize( metrics.width( "000" ), metrics.lineSpacing() ) );
	tree->header()->setStretchLastSection( false );

	connect( tree, SIGNAL( clicked( const QModelIndex & ) ),
		this, SLOT( select( const QModelIndex & ) ) );
	connect( tree, SIGNAL( customContextMenuRequested( const QPoint & ) ),
		this, SLOT( contextMenu( const QPoint & ) ) );


	// open gl
	setCentralWidget( ogl = new GLView );
	ogl->setNif( model );
	connect( ogl, SIGNAL( clicked( const QModelIndex & ) ),
			this, SLOT( select( const QModelIndex & ) ) );
	connect( ogl, SIGNAL( customContextMenuRequested( const QPoint & ) ),
		this, SLOT( contextMenu( const QPoint & ) ) );


	// actions

	aLoad = new QAction( "&Load", this );
	connect( aLoad, SIGNAL( triggered() ), this, SLOT( loadBrowse() ) );	
	aSave = new QAction( "&Save", this );
	connect( aSave, SIGNAL( triggered() ), this, SLOT( saveBrowse() ) );
	aLoadXML = new QAction( "Reload &XML", this );
	connect( aLoadXML, SIGNAL( triggered() ), this, SLOT( loadXML() ) );
	aReload = new QAction( "&Reload XML + Nif", this );
	aReload->setShortcut( Qt::ALT + Qt::Key_X );
	connect( aReload, SIGNAL( triggered() ), this, SLOT( reload() ) );
	aWindow = new QAction( "&New Window", this );
	connect( aWindow, SIGNAL( triggered() ), this, SLOT( sltWindow() ) );
	aQuit = new QAction( "&Quit", this );
	connect( aQuit, SIGNAL( triggered() ), qApp, SLOT( quit() ) );
	
	aCondition = new QAction( "Hide Condition Zero", this );
	aCondition->setToolTip( "checking this option makes the tree view better readable by displaying<br>only the rows where the condition is true and version matches the file version" );
	aCondition->setCheckable( true );
	aCondition->setChecked( tree->evalConditions() );
	connect( aCondition, SIGNAL( toggled( bool ) ), tree, SLOT( setEvalConditions( bool ) ) );

	aList = new QAction( "List", this );
	aList->setCheckable( true );
	aList->setChecked( list->model() == model );

	aHierarchy = new QAction( "Hierarchy", this );
	aHierarchy->setCheckable( true );
	aHierarchy->setChecked( list->model() == proxy );
	
	gListMode = new QActionGroup( this );
	connect( gListMode, SIGNAL( triggered( QAction * ) ), this, SLOT( setListMode() ) );
	gListMode->addAction( aList );
	gListMode->addAction( aHierarchy );
	gListMode->setExclusive( true );
	
	aNifSkope = new QAction( "About &NifSkope", this );
	connect( aNifSkope, SIGNAL( triggered() ), this, SLOT( about() ) );
	
	aAboutQt = new QAction( "About &Qt", this );
	connect( aAboutQt, SIGNAL( triggered() ), qApp, SLOT( aboutQt() ) );
	
	aToolSkel = new QAction( "Skeleton", this );
	connect( aToolSkel, SIGNAL( triggered() ), this, SLOT( sltToolSkel() ) );


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

	addDockWidget( Qt::LeftDockWidgetArea, dList );
	addDockWidget( Qt::BottomDockWidgetArea, dTree );


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
	
	sldTime = new QSlider;
	sldTime->setOrientation( Qt::Horizontal );
	tAnim->addWidget( sldTime );
	connect( ogl, SIGNAL( sigFrame( int, int, int ) ), this, SLOT( setFrame( int, int, int ) ) );
	connect( sldTime, SIGNAL( valueChanged( int ) ), ogl, SLOT( sltFrame( int ) ) );
	
	addToolBar( Qt::TopToolBarArea, tAnim );
	
	// LOD tool bar
	
	tLOD = new QToolBar( "LOD distance" );
	tLOD->setObjectName( "LODtool" );
	tLOD->setAllowedAreas( Qt::TopToolBarArea | Qt::BottomToolBarArea );
	
	tLOD->addWidget( new QLabel( "LOD" ) );
	
	sldDistance = new QSlider;
	sldDistance->setOrientation( Qt::Horizontal );
	sldDistance->setRange( 0, 10000 );
	tLOD->addWidget( sldDistance );
	connect( sldDistance, SIGNAL( valueChanged( int ) ), ogl, SLOT( sltDistance( int ) ) );
	
	spnMaxDistance = new QSpinBox;
	spnMaxDistance->setRange( 0, 4000000 );
	spnMaxDistance->setValue( 10000 );
	spnMaxDistance->setPrefix( "max " );
	tLOD->addWidget( spnMaxDistance );
	connect( spnMaxDistance, SIGNAL( valueChanged( int ) ), this, SLOT( setMaxDistance( int ) ) );
	
	addToolBar( Qt::TopToolBarArea, tLOD );
	
	// menu

	QMenu * mFile = new QMenu( "&File" );
	mFile->addAction( aLoad );
	mFile->addAction( aSave );
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
	mView->addSeparator();
	QMenu * mViewList = new QMenu( "&Block List Options" );
	mView->addMenu( mViewList );
	mViewList->addAction( aHierarchy );
	mViewList->addAction( aList );
	QMenu * mViewTree = new QMenu( "&Block Detail Options" );
	mView->addMenu( mViewTree );
	mViewTree->addAction( aCondition );
	mView->addSeparator();
	QMenu * mTools = new QMenu( "&Toolbars" );
	mView->addMenu( mTools );
	mTools->addAction( tool->toggleViewAction() );
	mTools->addAction( tAnim->toggleViewAction() );
	mTools->addAction( tLOD->toggleViewAction() );
	
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
	menuBar()->addMenu( new SpellBook( model, QModelIndex(), this, SLOT( select( const QModelIndex & ) ) ) );
	menuBar()->addMenu( mAbout );
}

void NifSkope::setFrame( int f, int mn, int mx )
{
	sldTime->setRange( mn, mx );
	sldTime->setValue( f );
}

void NifSkope::setMaxDistance( int max )
{
	sldDistance->setMaximum( max );
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

	restoreHeader( "list sizes", settings, list->header() );
	if ( settings.value( "list mode", "hirarchy" ).toString() == "list" )
		aList->setChecked( true );
	else
		aHierarchy->setChecked( true );
	setListMode();

	aCondition->setChecked( settings.value( "hide condition zero", true ).toBool() );
	restoreHeader( "tree sizes", settings, tree->header() );

	ogl->restore( settings );	

	restoreState( settings.value( "window state" ).toByteArray(), 0x041 );
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
	settings.setValue( "window state", saveState( 0x041 ) );

	settings.setValue( "last load", lineLoad->text() );
	settings.setValue( "last save", lineSave->text() );
	
	settings.setValue(	"list mode", ( gListMode->checkedAction() == aList ? "list" : "hirarchy" ) );
	saveHeader( "list sizes", settings, list->header() );

	settings.setValue( "hide condition zero", aCondition->isChecked() );
	saveHeader( "tree sizes", settings, tree->header() );

	ogl->save( settings );
}

void NifSkope::about()
{
	QString text =
	"<p>NifSkope is a simple low level tool for analyzing NetImmerse '.nif' files.</p>"
	"<p>NifSkope is based on the NifTool's file format data base. "
	"For more informations visit our site at http://niftools.sourceforge.net</p>"
	"<p>Because NifSkope uses the Qt libraries it is free software. The source"
	" is available for download at our home site on sourceforge.net</p>";

    QMessageBox mb( "About NifSkope", text, QMessageBox::Information, QMessageBox::Ok + QMessageBox::Default, 0, 0, this);
    mb.setIconPixmap( QPixmap( ":/res/nifskope.png" ) );
    mb.exec();
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
	
	if ( idx.model() == proxy )
		idx = proxy->mapTo( idx );
	
	SpellBook book( model, idx, this, SLOT( select( const QModelIndex & ) ) );
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
	
	if ( ! idx.isValid() || idx.model() != model ) return;
	
	if ( sender() != ogl )
	{
		ogl->setCurrentIndex( model->getBlock( idx ) );
	}

	if ( sender() != list )
	{
		if ( list->model() == proxy )
		{
			QModelIndex pidx = proxy->mapFrom( model->getBlock( idx ), list->currentIndex() );
			list->setCurrentIndexExpanded( pidx );
		}
		else if ( list->model() == model )
		{
			list->setCurrentIndex( model->getBlockOrHeader( idx ) );
		}
	}
	
	if ( sender() != tree )
	{
		if ( dList->isVisible() )
		{
			QModelIndex root = model->getBlockOrHeader( idx );
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
		if ( list->model() != model )
		{
			list->setModel( model );
			list->setItemsExpandable( false );
			list->setRootIsDecorated( false );
			list->setCurrentIndexExpanded( proxy->mapTo( idx ) );
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
	// open file
	if ( lineLoad->text().isEmpty() )
	{
		model->clear();
		return;
	}
	
	setEnabled( false );
	bool a = ogl->aAnimate->isChecked();
	bool r = ogl->aRotate->isChecked();
	ogl->aAnimate->setChecked( false );
	ogl->aRotate->setChecked( false );
	
	ProgDlg prog;
	prog.setLabelText( "loading nif..." );
	prog.setRange( 0, 1 );
	prog.setValue( 0 );
	prog.setMinimumDuration( 2100 );
	connect( model, SIGNAL( sigProgress( int, int ) ), & prog, SLOT( sltProgress( int, int ) ) );
	
	if ( ! model->load( lineLoad->text() ) )
		qWarning() << "failed to load nif from file " << lineLoad->text();
	
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
	if ( ! model->save( lineSave->text() ) )
		qWarning() << "could not write file " << lineSave->text();
	setEnabled( true );
}

void NifSkope::loadBrowse()
{
	// file select
	QString fn = QFileDialog::getOpenFileName( this, "Choose a file to open", lineLoad->text(), "NIFs (*.nif *.kf *.kfa)");
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

NifSkope * NifSkope::createWindow()
{
	NifSkope * skope = new NifSkope;
	skope->setAttribute( Qt::WA_DeleteOnClose );
	QSettings settings( "NifTools", "NifSkope" );
	skope->restore( settings );
	skope->show();
	return skope;
}

void NifSkope::loadXML()
{
	NifModel::loadXML();
}

void NifSkope::reload()
{
	if ( NifModel::loadXML() )
	{
		load();
	}
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
	
	NifSkope * skope = NifSkope::createWindow();
	
    if ( app.argc() > 1 )
        skope->load( QString( app.argv()[ app.argc() - 1 ] ) );

	// start the event loop
	return app.exec();
}
