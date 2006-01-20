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
#include <QMessageBox>
#include <QRadioButton>
#include <QSettings>
#include <QSlider>
#include <QSplitter>
#include <QTextEdit>
#include <QTimer>
#include <QToolButton>

#include <QListView>
#include <QTreeView>

#include "nifmodel.h"
#include "nifproxy.h"
#include "nifview.h"

#include "glview.h"
#include "popup.h"

/*
 * main GUI window
 */

NifSkope::NifSkope() : QMainWindow()
{
	// create a new model
	model = new NifModel( this );
	
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


	// actions

	aLoad = new QAction( "&Load", this );
	connect( aLoad, SIGNAL( triggered() ), this, SLOT( loadBrowse() ) );	
	aSave = new QAction( "&Save", this );
	connect( aSave, SIGNAL( triggered() ), this, SLOT( saveBrowse() ) );
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
	
	dList = new QDockWidget( "File Block List" );
	dList->setObjectName( "ListDock" );
	dList->setWidget( list );
	connect( dList->toggleViewAction(), SIGNAL( toggled( bool ) ), this, SLOT( clearRoot() ) );
	
	dTree = new QDockWidget( "Detailed Tree View" );
	dTree->setObjectName( "TreeDock" );
	dTree->setWidget( tree );	

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
	
	foreach ( QAction * a, ogl->animActions() )
		tAnim->addAction( a );
	
	connect( ogl->aAnimate, SIGNAL( toggled( bool ) ), tAnim->toggleViewAction(), SLOT( setChecked( bool ) ) );
	connect( ogl->aAnimate, SIGNAL( toggled( bool ) ), tAnim, SLOT( setVisible( bool ) ) );
	connect( tAnim->toggleViewAction(), SIGNAL( toggled( bool ) ), ogl->aAnimate, SLOT( setChecked( bool ) ) );
	
	sldTime = new QSlider;
	sldTime->setOrientation( Qt::Horizontal );
	tAnim->addWidget( sldTime );
	connect( ogl, SIGNAL( sigFrame( int, int, int ) ), this, SLOT( setFrame( int, int, int ) ) );
	connect( sldTime, SIGNAL( valueChanged( int ) ), ogl, SLOT( sltFrame( int ) ) );
	
	addToolBar( Qt::TopToolBarArea, tAnim );
	
	// menu

	QMenu * mFile = new QMenu( "&File" );
	mFile->addAction( aLoad );
	mFile->addAction( aSave );
	mFile->addSeparator();
	mFile->addAction( aWindow );
	mFile->addSeparator();
	mFile->addAction( aQuit );
	
	QMenu * mView = new QMenu( "&View" );
	mView->addAction( dTree->toggleViewAction() );
	mView->addAction( dList->toggleViewAction() );
	mView->addSeparator();
	mView->addAction( tool->toggleViewAction() );
	mView->addAction( tAnim->toggleViewAction() );
	
	QMenu * mOpts = new QMenu( "&Options" );
	foreach ( QAction * a, ogl->actions() )
		mOpts->addAction( a );
	mOpts->addSeparator();
	mOpts->addAction( aHierarchy );
	mOpts->addAction( aList );
	mOpts->addSeparator();
	mOpts->addAction( aCondition );
	
	QMenu * mTools = new QMenu( "&Tools" );
	mTools->addAction( aToolSkel );
	
	QMenu * mAbout = new QMenu( "&About" );
	mAbout->addAction( aNifSkope );
	mAbout->addAction( aAboutQt );
	
	menuBar()->addMenu( mFile );
	menuBar()->addMenu( mView );
	menuBar()->addMenu( mOpts );
	menuBar()->addMenu( mTools );
	menuBar()->addMenu( mAbout );
}

void NifSkope::setFrame( int f, int mn, int mx )
{
	sldTime->setRange( mn, mx );
	sldTime->setValue( f );
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

void NifSkope::setCurrentBlock( const QModelIndex & index )
{
	QModelIndex block = index;
	if ( block.model() == proxy )
		block = proxy->mapTo( index );
	
	if ( list->isVisible() )
	{
		if ( list->model() == proxy )
		{
			QModelIndex pidx = proxy->mapFrom( block, list->currentIndex() );
			list->setCurrentIndexExpanded( pidx );
		}
		else
			list->setCurrentIndexExpanded( block );
		tree->setRootIndex( block );
	}
	else
	{
		tree->setCurrentIndexExpanded( block );
	}
	
	ogl->setCurrentIndex( block );
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
	
	if ( idx.model() == proxy )
		idx = proxy->mapTo( idx );
	
	QMenu * menu = new QMenu( this );
	
	int link = model->itemLink( idx );
	{
		QAction * a = menu->addAction( "Follow Link" );
		a->setEnabled( link >= 0 );
		menu->addSeparator();
	}

	{
		QMenu * m = new QMenu( "Insert Block" );
		QStringList ids = model->allNiBlocks();
		ids.sort();
		foreach( QString x, ids )
			m->addAction( x );
		menu->addMenu( m );
	}
	
	if ( model->getBlockNumber( idx ) >= 0 )
	{
		menu->addAction( "Remove Block" );
	}
	else
	{
		menu->addSeparator();
		menu->addAction( "Update Header" );
	}
	
	if ( ! model->itemArr1( idx ).isEmpty() )
	{
		menu->addSeparator();
		QAction * a = menu->addAction( "Update Array" );
		a->setEnabled( model->evalCondition( idx, true ) );
	}
	
	if ( sender() == list && list->model() == proxy )
	{
		menu->addSeparator();
		menu->addAction( "Expand All" );
		menu->addAction( "Collapse All" );
	}
	
	if ( idx.isValid() && model->isCompound( model->itemType( idx ) ) )
	{
		menu->addSeparator();
		menu->addAction( "Copy" );
		const QMimeData * mime = QApplication::clipboard()->mimeData();
		if ( mime )
		{
			foreach ( QString form, mime->formats() )
			{
				if ( form.startsWith( "nifskope/compound/" ) )
				{
					QString type = form.right( form.length() - 18 );
					if ( type == model->itemType( idx ) )
					{
						menu->addAction( "Paste" );
					}
				}
			}
		}
	}
	
	if ( idx.isValid() && model->itemType( idx ) == "NiBlock" && model->isNiBlock( model->itemName( idx ) ) )
	{
		menu->addSeparator();
		menu->addAction( "Copy" );
		const QMimeData * mime = QApplication::clipboard()->mimeData();
		if ( mime )
		{
			foreach ( QString form, mime->formats() )
			{
				if ( form.startsWith( "nifskope/niblock/" ) )
				{
					QString name = form.right( form.length() - 17 );
					if ( name == model->itemName( idx ) )
					{
						menu->addAction( "Paste" );
					}
				}
			}
		}
	}
	
	QAction * a = menu->exec( p );
	if ( a ) 
	{
		if ( a->text() == "Follow Link" )
		{
			setCurrentBlock( model->getBlock( link ) );
		}
		else if ( a->text() == "Update Header" )
		{
			model->updateHeader();
		}
		else if ( a->text() == "Update Array" )
		{
			model->updateArray( idx );
		}
		else if ( a->text() == "Expand All" )
		{
			list->setAllExpanded( QModelIndex(), true );
		}
		else if ( a->text() == "Collapse All" )
		{
			list->setAllExpanded( QModelIndex(), false );
		}
		else if ( a->text() == "Remove Block" )
		{
			model->removeNiBlock( model->getBlockNumber( idx ) );
			model->updateHeader();
		}
		else if ( a->text() == "Copy" )
		{
			QByteArray data;
			QBuffer buffer( & data );
			if ( buffer.open( QIODevice::WriteOnly ) && model->save( buffer, idx ) )
			{
				QMimeData * mime = new QMimeData;
				if ( model->isCompound( model->itemType( idx ) ) )
					mime->setData( QString( "nifskope/compound/%1" ).arg( model->itemType( idx ) ), data );
				else
					mime->setData( QString( "nifskope/niblock/%1" ).arg( model->itemName( idx ) ), data );
				QApplication::clipboard()->setMimeData( mime );
			}
		}
		else if ( a->text() == "Paste" )
		{
			const QMimeData * mime = QApplication::clipboard()->mimeData();
			if ( mime )
			{
				foreach ( QString form, mime->formats() )
				{
					if ( form.startsWith( "nifskope/compound/" ) )
					{
						QString type = form.right( form.length() - 18 );
						if ( type == model->itemType( idx ) )
						{
							QByteArray data = mime->data( form );
							QBuffer buffer( & data );
							if ( buffer.open( QIODevice::ReadOnly ) )
							{
								model->load( buffer, idx );
							}
						}
						break;
					}
					else if ( form.startsWith( "nifskope/niblock/" ) )
					{
						QString name = form.right( form.length() - 17 );
						if ( name == model->itemName( idx ) && model->itemType( idx ) == "NiBlock" )
						{
							QByteArray data = mime->data( form );
							QBuffer buffer( & data );
							if ( buffer.open( QIODevice::ReadOnly ) )
							{
								model->load( buffer, idx );
							}
						}
						break;
					}
				}
			}
		}
		else
		{
			model->insertNiBlock( a->text(), model->getBlockNumber( idx )+1 );
			model->updateHeader();
		}
	}
	delete menu;
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
	if ( ! index.isValid() ) return;
	
	if ( sender() == list )
	{
		QModelIndex root = index;
		if ( index.model() == proxy )
			root = proxy->mapTo( index );
		if ( root.isValid() && root.column() != 0 )
			root = root.sibling( root.row(), 0 );
		tree->setRootIndex( root );
		ogl->setCurrentIndex( root );
	}
	else if ( sender() == ogl )
	{
		if ( list->model() == proxy )
		{
			QModelIndex pidx = proxy->mapFrom( index, QModelIndex() );
			list->setCurrentIndexExpanded( pidx );
		}
		else if ( list->model() == model )
			list->setCurrentIndexExpanded( index );
		
		if ( list->isVisible() )
			tree->setRootIndex( index );
		else
			tree->setCurrentIndexExpanded( index );
	}
	else if ( sender() == tree )
	{
		ogl->setCurrentIndex( index );
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
	
	QFile f( lineLoad->text() );
	if ( f.open( QIODevice::ReadOnly ) )
	{
		setEnabled( false );
		bool a = ogl->aAnimate->isChecked();
		bool r = ogl->aRotate->isChecked();
		ogl->aAnimate->setChecked( false );
		ogl->aRotate->setChecked( false );
		model->load( f );
		ogl->aAnimate->setChecked( a );
		ogl->aRotate->setChecked( r );
		ogl->center();
		f.close();
		setEnabled( true );
	}
	else
		qWarning() << "could not open file " << lineLoad->text();
}

void NifSkope::save()
{
	// write to file
	QFile f( lineSave->text() );
	if ( f.open( QIODevice::WriteOnly ) )
	{
		setEnabled( false );
		model->save( f );
		setEnabled( true );
		f.close();
	}
	else
		qWarning() << "could not write file " << lineSave->text();
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
	skope->resize( skope->sizeHint() );
	skope->show();
	return skope;
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
	
	// read in XML fileformat description
	QString result = NifModel::parseXmlDescription( QDir( app.applicationDirPath() ).filePath( "nif.xml" ) );
	if ( ! result.isEmpty() )
	{
		QMessageBox::critical( 0, "NifSkope", result );
		return -1;
	}
	
	qInstallMsgHandler(myMessageOutput);
	
	NifSkope * skope = NifSkope::createWindow();

    if (app.argc() > 1)
        skope->load(QString(app.argv()[app.argc() - 1]));

	// start the event loop
	return app.exec();
}

QDataStream & operator<<( QDataStream & ds, const Transform & tf )
{
	for ( int c = 0; c < 3; c++ )
		ds << tf.translation[c];
	for ( int c = 0; c < 3; c++ )
		for ( int d = 0; d < 3; d++ )
			ds << tf.rotation(c,d);
	ds << tf.scale;
	return ds;
}

QDataStream & operator>>( QDataStream & ds, Transform & tf )
{
	for ( int c = 0; c < 3; c++ )
		ds >> tf.translation[c];
	for ( int c = 0; c < 3; c++ )
		for ( int d = 0; d < 3; d++ )
			ds >> tf.rotation(c,d);
	ds >> tf.scale;
	return ds;
}

void NifSkope::sltToolSkel()
{
	QMap<QString, Transform> bones;
	QMap<QString, Transform> skins;
//#define SKEL_SAVE
#define SKEL_RESTORE
#ifdef SKEL_SAVE
	for ( int b = 0; b < model->getBlockCount(); b++ )
	{
		QModelIndex iBlock = model->getBlock( b );
		QString blockType = model->itemName( iBlock );
		
		if ( blockType == "NiNode" && model->get<QString>( iBlock, "Name" ).startsWith( "Bip01" ) )
		{
			qWarning() << b << model->get<QString>( iBlock, "Name" );
			bones.insert( model->get<QString>( iBlock, "Name" ), Transform( model, iBlock ) );
		}
		else if ( blockType == "NiSkinInstance" )
		{
			qWarning() << b << blockType;
			
			QModelIndex iBones = model->getIndex( iBlock, "Bones" );
			if ( iBones.isValid() )
				iBones = model->getIndex( iBones, "Bones" );
			
			QList<QString> names;
			if ( iBones.isValid() )
			{
				for ( int r = 0; r < model->rowCount( iBones ); r++ )
				{
					QModelIndex iBone = model->getBlock( model->itemValue( iBones.child( r, 0 ) ).toLink(), "NiNode" );
					if ( iBone.isValid() )
						names.append( model->get<QString>( iBone, "Name" ) );
					else
						names.append( "" );
				}
			}
			
			QModelIndex iData = model->getBlock( model->getLink( iBlock, "Data" ), "NiSkinData" );
			if ( iData.isValid() )
			{
				qWarning() << b << blockType;
				
				QModelIndex iBones = model->getIndex( iData, "Bone List" );
				if ( iBones.isValid() )
				{
					for ( int r = 0; r < model->rowCount( iBones ); r++ )
					{
						QString name = names.value( r );
						if ( ! name.isEmpty() )
						{
							skins.insert( name, Transform( model, iBones.child( r, 0 ) ) );
						}
					}
				}
			}
		}
	}
	QFile f( "f:\\nif\\skel.dat" );
	if ( f.open( QIODevice::WriteOnly ) )
	{
		QDataStream ds( &f );
		ds << bones;
		ds << skins;
		f.close();
	}
#endif
#ifdef SKEL_RESTORE
	QFile f( "f:\\nif\\skel.dat" );
	if ( f.open( QIODevice::ReadOnly ) )
	{
		QDataStream ds( &f );
		ds >> bones >> skins;
		f.close();
		qWarning() << bones.count() << skins.count();
	}
	
	for ( int b = 0; b < model->getBlockCount(); b++ )
	{
		QModelIndex iBlock = model->getBlock( b );
		QString blockType = model->itemName( iBlock );
		
		if ( blockType == "NiNode" )
		{
			QString name = model->get<QString>( iBlock, "Name" );
			if ( bones.contains( name ) )
			{
				qWarning() << b << name;
				model->set( iBlock, "Rotation", bones[name].rotation );
				model->set( iBlock, "Translation", bones[name].translation );
				model->set( iBlock, "Scale", bones[name].scale );
			}
		}
		else if ( blockType == "NiSkinInstance" )
		{
			qWarning() << b << blockType;
			
			QModelIndex iBones = model->getIndex( iBlock, "Bones" );
			if ( iBones.isValid() )
				iBones = model->getIndex( iBones, "Bones" );
			
			QList<QString> names;
			if ( iBones.isValid() )
			{
				for ( int r = 0; r < model->rowCount( iBones ); r++ )
				{
					QModelIndex iBone = model->getBlock( model->itemValue( iBones.child( r, 0 ) ).toLink(), "NiNode" );
					if ( iBone.isValid() )
						names.append( model->get<QString>( iBone, "Name" ) );
					else
						names.append( "" );
				}
			}
			
			QModelIndex iData = model->getBlock( model->getLink( iBlock, "Data" ), "NiSkinData" );
			if ( iData.isValid() )
			{
				qWarning() << model->getBlockNumber( iData ) << model->itemName( iData );
				
				QModelIndex iBones = model->getIndex( iData, "Bone List" );
				if ( iBones.isValid() )
				{
					for ( int r = 0; r < model->rowCount( iBones ); r++ )
					{
						QString name = names.value( r );
						if ( skins.contains( name ) )
						{
							model->set( iBones.child( r, 0 ), "Rotation", skins[name].rotation );
							model->set( iBones.child( r, 0 ), "Translation", skins[name].translation );
							model->set( iBones.child( r, 0 ), "Scale", skins[name].scale );
						}
					}
				}
			}
		}
	}
	model->reset();
#endif
}
