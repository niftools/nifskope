/****************************************************************************
**
** nifscope: a tool for analyzing and editing NetImmerse files (.nif & .kf)
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/


#include "nifskope.h"

#include <QAction>
#include <QApplication>
#include <QButtonGroup>
#include <QByteArray>
#include <QCheckBox>
#include <QDataStream>
#include <QDebug>
#include <QDesktopWidget>
#include <QFile>
#include <QFileDialog>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLayout>
#include <QMenu>
#include <QMessageBox>
#include <QRadioButton>
#include <QSettings>
#include <QSplitter>
#include <QTextEdit>
#include <QTimer>
#include <QToolButton>

#include <QListView>
#include <QTreeView>

#include "nifmodel.h"
#include "nifproxy.h"

#include "glview.h"
#include "popup.h"

#define HIDEZEROCOND


/*
 * main GUI window
 */

NifSkope::NifSkope() : QWidget(), popOpts( 0 )
{
	// create a new model
	model = new NifModel( this );
	connect( model, SIGNAL( dataChanged( const QModelIndex & , const QModelIndex & ) ),
			this, SLOT( dataChanged( const QModelIndex &, const QModelIndex & ) ) );
	proxy = new NifProxyModel( this );
	proxy->setModel( model );
	
	// application specific settings
	QSettings settings( "NifTools", "NifSkope" );
	
	// layout widgets in a grid
	QGridLayout * grid = new QGridLayout;
	setLayout( grid );
	
	// load action
	QAction * aLoad = new QAction( "load", this );
	connect( aLoad, SIGNAL( triggered() ), this, SLOT( load() ) );
	
	// load button
	QToolButton * tool = new QToolButton;
	tool->setDefaultAction( aLoad );
	grid->addWidget( tool, 0, 0 );
	
	// the name of the file to load
	lineLoad = new QLineEdit( settings.value( "last load", QString( "" ) ).toString() );
	grid->addWidget( lineLoad, 0, 1, 1, 2 );
	connect( lineLoad, SIGNAL( returnPressed() ), this, SLOT( load() ) );
	
	// file selector
	tool = new QToolButton;
	tool->setDefaultAction( new QAction( "browse", this ) );
	connect( tool->defaultAction(), SIGNAL( triggered() ), this, SLOT( loadBrowse() ) );
	grid->addWidget( tool, 0, 3 );
	
	// variable split layout
	split = new QSplitter;
	grid->addWidget( split, 1, 0, 1, 4 );
	grid->setRowStretch( 1, 4 );

	// this view shows the block list
	list = new QTreeView;
	split->addWidget( list );
	QFont font = list->font();
	font.setPointSize( font.pointSize() + 3 );
	list->setFont( font );
	list->setModel( proxy );
	list->setColumnHidden( NifModel::TypeCol, true );
	list->setColumnHidden( NifModel::ArgCol, true );
	list->setColumnHidden( NifModel::Arr1Col, true );
	list->setColumnHidden( NifModel::Arr2Col, true );
	list->setColumnHidden( NifModel::CondCol, true );
	list->setColumnHidden( NifModel::Ver1Col, true );
	list->setColumnHidden( NifModel::Ver2Col, true );
	
	// this view shows the whole tree or the block details
	tree = new QTreeView;
	split->addWidget( tree );
	tree->setFont( font );
	tree->setModel( model );
	
	connect( list, SIGNAL( clicked( const QModelIndex & ) ),
			this, SLOT( selectRoot( const QModelIndex & ) ) );

#ifdef QT_OPENGL_LIB
	// open gl
	ogl = new GLView;
	ogl->setNif( model );	
	split->addWidget( ogl );
#else
	ogl = 0;
#endif

	// save action
	QAction * aSave = new QAction( "save", this );
	connect( aSave, SIGNAL( triggered() ), this, SLOT( save() ) );
	
	// save button
	tool = new QToolButton;
	tool->setDefaultAction( aSave );
	grid->addWidget( tool, 2, 0 );

	// name of the file to write to
	lineSave = new QLineEdit( settings.value( "last save", "" ).toString() );
	grid->addWidget( lineSave, 2, 1 );
	connect( lineSave, SIGNAL( returnPressed() ), this, SLOT( save() ) );
	
	// a file select button
	tool = new QToolButton;
	tool->setDefaultAction( new QAction( "browse", this ) );
	connect( tool->defaultAction(), SIGNAL( triggered() ), this, SLOT( saveBrowse() ) );
	grid->addWidget( tool, 2, 2 );
	
	//split->restoreState( settings.value( "split sizes" ).toByteArray() );
	list->setVisible( settings.value( "show list", true ).toBool() );
	tree->setVisible( settings.value( "show tree", true ).toBool() );
	if ( ogl ) ogl->setVisible( settings.value( "show opengl", true ).toBool() );

	// options
	popOpts = new Popup( "Options", this );
	{
		popOpts->setLayout( new QVBoxLayout );
		
		QGroupBox * optList = new QGroupBox( "File Block List" );
		popOpts->layout()->addWidget( optList );
		optList->setFlat( true );
		optList->setCheckable( true );
		optList->setChecked( list->isVisibleTo( this ) );
		connect( optList, SIGNAL( toggled( bool ) ), list, SLOT( setVisible( bool ) ) );
		connect( optList, SIGNAL( toggled( bool ) ), this, SLOT( clearRoot() ) );
		optList->setLayout( new QVBoxLayout );

		listMode = new QButtonGroup( this );
		connect( listMode, SIGNAL( buttonClicked( QAbstractButton * ) ), this, SLOT( setListMode( QAbstractButton * ) ) );
		
		QRadioButton * checkMode = new QRadioButton( "list" );
		optList->layout()->addWidget( checkMode );
		checkMode->setChecked( settings.value( "list mode", "hirarchy" ).toString() == checkMode->text() );
		listMode->addButton( checkMode );
		checkMode = new QRadioButton( "hirarchy" );
		optList->layout()->addWidget( checkMode );
		checkMode->setChecked( settings.value( "list mode", "hirarchy" ).toString() == checkMode->text() );
		listMode->addButton( checkMode );
		
		setListMode( listMode->checkedButton() );
		
		QGroupBox * optTree = new QGroupBox( "Detailed Tree View" );
		popOpts->layout()->addWidget( optTree );
		optTree->setFlat( true );
		optTree->setCheckable( true );
		optTree->setChecked( tree->isVisibleTo( this ) );
		connect( optTree, SIGNAL( toggled( bool ) ), tree, SLOT( setVisible( bool ) ) );
		optTree->setLayout( new QVBoxLayout );
		
		conditionZero = new QCheckBox( "hide condition zero" );
		//conditionZero->setChecked( settings.value( "hide condition zero", true ).toBool() );
		connect( conditionZero, SIGNAL( toggled( bool ) ), this, SLOT( updateConditionZero() ) );
		optTree->layout()->addWidget( conditionZero );
		conditionZero->setToolTip( "this hides every row wich conditions are not met<br>warning: toggling this function can be slow especially on large nifs" );
#ifndef HIDEZEROCOND
		conditionZero->setVisible( false );
#endif
		
#ifdef QT_OPENGL_LIB
		QGroupBox * optGL = new QGroupBox( "OpenGL Preview" );
		popOpts->layout()->addWidget( optGL );
		optGL->setFlat( true );
		optGL->setCheckable( true );
		optGL->setChecked( ogl->isVisibleTo( this ) );
		optGL->setEnabled( ogl->isValid() );
		connect( optGL, SIGNAL( toggled( bool ) ), ogl, SLOT( setVisible( bool ) ) );
		optGL->setLayout( new QVBoxLayout );
		
		optGL->layout()->addWidget( new QLabel( "texture folder" ) );
		QLineEdit * optTexdir = new QLineEdit;
		optGL->layout()->addWidget( optTexdir );
		connect( optTexdir, SIGNAL( textChanged( const QString & ) ), ogl, SLOT( setTextureFolder( const QString & ) ) );
		optTexdir->setText( settings.value( "texture folder" ).toString() );
		optTexdir->setToolTip( "put in your texture folders here<br>if you have more than one seperate them with semicolons" );
		
		QCheckBox * optLights = new QCheckBox( "lighting" );
		optGL->layout()->addWidget( optLights );
		optLights->setChecked( settings.value( "enable lighting", true ).toBool() );
		connect( optLights, SIGNAL( toggled( bool ) ), ogl, SLOT( setLighting( bool ) ) );
		ogl->setLighting( optLights->isChecked() );
		
		QCheckBox * optAxis = new QCheckBox( "draw axis" );
		optGL->layout()->addWidget( optAxis );
		optAxis->setChecked( settings.value( "draw axis", true ).toBool() );
		connect( optAxis, SIGNAL( toggled( bool ) ), ogl, SLOT( setDrawAxis( bool ) ) );
		ogl->setDrawAxis( optAxis->isChecked() );
		
		QCheckBox * optRotate = new QCheckBox( "rotate" );
		optGL->layout()->addWidget( optRotate );
		optRotate->setChecked( settings.value( "rotate", true ).toBool() );
		connect( optRotate, SIGNAL( toggled( bool ) ), ogl, SLOT( setRotate( bool ) ) );
		ogl->setRotate( optRotate->isChecked() );
#endif
		
		QGroupBox * optMisc = new QGroupBox;
		popOpts->layout()->addWidget( optMisc );
		optMisc->setFlat( true );
		QGridLayout * optGrid = new QGridLayout;
		optMisc->setLayout( optGrid );
		
		autoSettings = new QCheckBox( "save settings on exit" );
		autoSettings->setChecked( settings.value( "auto save settings", false ).toBool() );
		optGrid->addWidget( autoSettings, 0, 0, 1, 3 );
		
		QAction * aboutNifSkope = new QAction( "about nifskope", this );
		connect( aboutNifSkope, SIGNAL( triggered() ), this, SLOT( about() ) );
		tool = new QToolButton;
		tool->setDefaultAction( aboutNifSkope );
		optGrid->addWidget( tool, 1, 0 );
		
		QAction * saveOpts = new QAction( "save options", this );
		connect( saveOpts, SIGNAL( triggered() ), this, SLOT( saveOptions() ) );
		tool = new QToolButton;
		tool->setDefaultAction( saveOpts );
		optGrid->addWidget( tool, 1, 1 );
		
		QAction * about = new QAction( "about qt", this );
		connect( about, SIGNAL( triggered() ), qApp, SLOT( aboutQt() ) );
		tool = new QToolButton;
		tool->setDefaultAction( about );
		optGrid->addWidget( tool, 1, 2 );
		
	}
	tool = new QToolButton;
	tool->setDefaultAction( popOpts->popupAction() );
	grid->addWidget( tool, 2, 3 );
	
	// last but not least: set up a custom delegate to provide edit functionality
	list->setItemDelegate( model->createDelegate() );
	tree->setItemDelegate( model->createDelegate() );
	
	// tweak some display settings
	QFontMetrics m( list->font() );
	list->setIconSize( QSize( m.width( "000" ), m.lineSpacing() ) );
	list->setUniformRowHeights( true );
	list->setAlternatingRowColors( false );
	list->header()->setStretchLastSection( true );
	settings.beginReadArray( "list header" );
	for ( int c = 0; c < list->header()->count(); c++ )
	{
		settings.setArrayIndex( c );
		int s = settings.value( "size", -1 ).toInt();
		if ( s >= 0 ) list->header()->resizeSection( c, s );
	}
	settings.endArray();
	tree->setIconSize( QSize( m.width( "000" ), m.lineSpacing() ) );
	tree->setUniformRowHeights( true );
	tree->setAlternatingRowColors( false );
	tree->header()->setStretchLastSection( false );
	settings.beginReadArray( "tree header" );
	for ( int c = 0; c < tree->header()->count(); c++ )
	{
		settings.setArrayIndex( c );
		int s = settings.value( "size", -1 ).toInt();
		if ( s >= 0 ) tree->header()->resizeSection( c, s );
	}
	settings.endArray();
	
	// fetch context menu signals from model views
	list->setContextMenuPolicy( Qt::CustomContextMenu );
	connect( list, SIGNAL( customContextMenuRequested( const QPoint & ) ),
			this, SLOT( contextMenu( const QPoint & ) ) );
	tree->setContextMenuPolicy( Qt::CustomContextMenu );
	connect( tree, SIGNAL( customContextMenuRequested( const QPoint & ) ),
			this, SLOT( contextMenu( const QPoint & ) ) );
			
	// messages
	msgroup = new QGroupBox( "messages" );
	grid->addWidget( msgroup, 3, 0, 1, 4 );
	msgroup->setLayout( new QHBoxLayout );
	msgroup->setVisible( false );
	msgroup->setCheckable( true );
	msgroup->setChecked( msgroup->isVisibleTo( this ) );
	connect( msgroup, SIGNAL( toggled( bool ) ), this, SLOT( toggleMessages() ) );
	msgview = new QTextEdit;
	msgview->setReadOnly( true );
	msgroup->layout()->addWidget( msgview );
	connect( msgroup, SIGNAL( toggled( bool ) ), msgview, SLOT( clear() ) );
	
	QRect geo = settings.value( "window geometry", QRect() ).value<QRect>();
	if ( geo.isValid() )	setGeometry( geo );
}

NifSkope::~NifSkope()
{
	if ( autoSettings->isChecked() )
		saveOptions();
}

void NifSkope::saveOptions()
{
	QSettings settings( "NifTools", "NifSkope" );
	settings.setValue( "last load", lineLoad->text() );
	settings.setValue( "last save", lineSave->text() );
	settings.setValue( "show list", list->isVisibleTo( this ) );
	settings.setValue(	"list mode", ( listMode->checkedButton() ? listMode->checkedButton()->text() : "list" ) );
	settings.beginWriteArray( "list header" );
	for ( int c = 0; c < list->header()->count(); c++ )
	{
		settings.setArrayIndex( c );
		settings.setValue( "size", list->header()->sectionSize( c ) );
	}
	settings.endArray();
	settings.setValue( "show tree", tree->isVisibleTo( this ) );
	settings.setValue( "hide condition zero", conditionZero->isChecked() );
	settings.beginWriteArray( "tree header" );
	for ( int c = 0; c < tree->header()->count(); c++ )
	{
		settings.setArrayIndex( c );
		settings.setValue( "size", tree->header()->sectionSize( c ) );
	}
	settings.endArray();
#ifdef QT_OPENGL_LIB
	settings.setValue( "show opengl", ogl->isVisibleTo( this ) );
	settings.setValue( "texture folder", ogl->textureFolder() );
	settings.setValue( "enable lighting", ogl->lighting() );
	settings.setValue( "draw axis", ogl->drawAxis() );
	settings.setValue( "rotate", ogl->rotate() );
#endif
	settings.setValue( "auto save settings", autoSettings->isChecked() );
	settings.setValue( "window geometry", normalGeometry() );
	//settings.setValue( "split sizes", split->saveState() );
}

void NifSkope::about()
{
	QMessageBox::about( this, "About NifSkope",
	"NifSkope is a simple low level tool for analyzing NetImmerse '.nif' files."
	"<br><br>"
	"NifSkope is based on the NifTool's file format data base."
	"For more informations visit our site at http://niftools.sourceforge.net"
	"<br><br>"
	"Because NifSkope uses the Qt libraries it is free software. You should have received"
	" a copy of the GPL and the source code all together in one package.<br>");
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
		QAction * a = menu->addAction( "follow Link" );
		a->setEnabled( link >= 0 );
		menu->addSeparator();
	}

	{
		QMenu * m = new QMenu( "insert Block" );
		QStringList ids = model->allNiBlocks();
		ids.sort();
		foreach( QString x, ids )
			m->addAction( x );
		menu->addMenu( m );
	}
	
	if ( model->getBlockNumber( idx ) >= 0 )
	{
		menu->addAction( "remove Block" );
	}
	else
	{
		menu->addSeparator();
		menu->addAction( "update Header" );
	}
	
	if ( ! model->itemArr1( idx ).isEmpty() )
	{
		menu->addSeparator();
		QAction * a = menu->addAction( "update Array" );
		a->setEnabled( model->evalCondition( idx, true ) );
	}
	
	QAction * a = menu->exec( p );
	if ( a ) 
	{
		if ( a->text() == "follow Link" )
		{
			QModelIndex target = model->getBlock( link );
			if ( list->isVisible() )
			{
				if ( list->model() == proxy )
				{
					QModelIndex pidx = proxy->mapFrom( target, list->currentIndex() );
					list->setCurrentIndex( pidx );
					while ( pidx.isValid() )
					{
						list->expand( pidx );
						pidx = pidx.parent();
					}
				}
				else
					list->setCurrentIndex( target );
				tree->setRootIndex( target );
			}
			else
			{
				tree->setCurrentIndex( target );
				tree->expand( target );
			}
		}
		else if ( a->text() == "update Header" )
		{
			model->updateHeader();
		}
		else if ( a->text() == "update Array" )
		{
			model->updateArray( idx );
		}
		else if ( a->text() == "remove Block" )
		{
			model->removeNiBlock( model->getBlockNumber( idx ) );
			model->updateHeader();
		}
		else
		{
			model->insertNiBlock( a->text(), model->getBlockNumber( idx ) );
			model->updateHeader();
		}
	}
	delete menu;
}

void NifSkope::clearRoot()
{
	tree->setRootIndex( QModelIndex() );
}

void NifSkope::selectRoot( const QModelIndex & index )
{
	if ( ! index.isValid() ) return;
	if ( index.model() == proxy )
		tree->setRootIndex( proxy->mapTo( index ) );
	else if ( index.model() == model )
		tree->setRootIndex( index );
}

void NifSkope::setListMode( QAbstractButton * b )
{
	QModelIndex idx = list->currentIndex();
	if ( b->text() == "list" )
	{
		list->setModel( model );
		list->setItemsExpandable( false );
		list->setRootIsDecorated( false );
		list->setCurrentIndex( proxy->mapTo( idx ) );
	}
	else
	{
		proxy->reset();
		list->setModel( proxy );
		list->setItemsExpandable( true );
		list->setRootIsDecorated( true );
		QModelIndex pidx = proxy->mapFrom( idx, QModelIndex() );
		list->setCurrentIndex( pidx );
		while ( pidx.isValid() )
		{
			list->expand( pidx );
			pidx = pidx.parent();
		}
	}
}

void NifSkope::showHideRows( QModelIndex parent )
{
#ifndef HIDEZEROCOND
	return;
#endif
	bool treeVis = tree->isVisibleTo( tree->parentWidget() );
	if ( treeVis ) tree->setVisible( false );
	//qDebug( "showHideRows( %s )", str( model->itemName( parent ) ) );
	for ( int c = 0; c < model->rowCount( parent ); c++ )
	{
		QModelIndex child = model->index( c, 0, parent );
		if ( ! child.isValid() )
			qDebug( "row %i is invalid", c );
		else
		{
			if ( conditionZero->isChecked() && ! model->evalCondition( child ) )
			{
				tree->setRowHidden( c, parent, true );
			}
			else
			{
				tree->setRowHidden( c, parent, false );
				QString type = model->itemType( child );
				if ( model->rowCount( child ) > 0 && ! model->isUnconditional( type ) )
					showHideRows( child );
			}
		}
	}
	if ( treeVis ) tree->setVisible( true );
}

void NifSkope::updateConditionZero()
{
	showHideRows( QModelIndex() );
}

void NifSkope::dataChanged( const QModelIndex & idx, const QModelIndex & xdi )
{
	if ( ! conditionZero->isChecked() )
		return;
	
	if ( ! ( idx.isValid() && xdi.isValid() ) )
	{
		showHideRows( QModelIndex() );
		return;
	}
	
	QModelIndex block = idx;
	
	while ( block.parent().isValid() )
		block = block.parent();
		
	showHideRows( block );
}

void NifSkope::load()
{
	// open file
	if ( lineLoad->text().isEmpty() )
		return;
	
	QFile f( lineLoad->text() );
	if ( f.open( QIODevice::ReadOnly ) )
	{
		QDataStream stream( &f );
		model->load( stream );
#ifdef QT_OPENGL_LIB
		if ( ogl && ogl->isValid() ) ogl->compile( true );
#endif
		updateConditionZero();
		f.close();
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
		QDataStream stream( &f );
		model->save( stream );
		f.close();
	}
	else
		qWarning() << "could not write file " << lineSave->text();
}

void NifSkope::loadBrowse()
{
	// file select
	QString fn = QFileDialog::getOpenFileName( this, "Choose a file to open", lineLoad->text(), "NIFs (*.nif *.NIF)");
	if ( !fn.isEmpty() )
	{
		lineLoad->setText( fn );
		load();
	}
}

void NifSkope::saveBrowse()
{
	// file select
	QString fn = QFileDialog::getSaveFileName( this, "Choose a file to save", lineSave->text(), "NIFs (*.nif *.NIF)");
	if ( !fn.isEmpty() )
	{
		lineSave->setText( fn );
		save();
	}
}

void NifSkope::addMessage( const QString & msg )
{
	msgroup->setChecked( true );
	msgview->append( msg );
//	msgview->ensureCursorVisible( true );
}

void NifSkope::toggleMessages()
{
	QTimer::singleShot( 0, this, SLOT( delayedToggleMessages() ) );
}

void NifSkope::delayedToggleMessages()
{
	msgroup->setVisible( msgroup->isChecked() );
}

NifSkope * msgtarget = 0;

void myMessageOutput(QtMsgType type, const char *msg)
{
	switch (type)
	{
		case QtDebugMsg:
			break;
		case QtWarningMsg:
		case QtCriticalMsg:
			if ( msgtarget )
			{
				msgtarget->addMessage( msg );
				break;
			}
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
	QDir dir( app.applicationDirPath() );
	QString result = NifModel::parseXmlDescription( dir.filePath( "NifSkope.xml" ) );
	if ( ! result.isEmpty() )
	{
		QMessageBox::critical( 0, "NifSkope", result );
		return -1;
	}
	
	// set up the GUI
	NifSkope edit;
	edit.show();
	
	msgtarget = &edit;
	qInstallMsgHandler(myMessageOutput);

	// start the event loop
	return app.exec();
}
