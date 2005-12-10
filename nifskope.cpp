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
#include "nifview.h"

#include "glview.h"
#include "popup.h"

void saveHeader( const QString & name, QSettings & settings, QHeaderView * header )
{
	QByteArray b;
	QDataStream d( &b, QIODevice::WriteOnly );
	d << header->count();
	for ( int c = 0; c < header->count(); c++ )
		d << header->sectionSize( c );
	settings.setValue( name, b );
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

/*
 * main GUI window
 */

NifSkope::NifSkope() : QWidget(), popOpts( 0 )
{
	// create a new model
	model = new NifModel( this );
	
	// create a new hirarchical proxy model
	proxy = new NifProxyModel( this );
	proxy->setModel( model );
	
	// application specific settings
	QSettings settings( "NifTools", "NifSkope" );
	
	// layout widgets in a grid
	QGridLayout * grid = new QGridLayout;
	setLayout( grid );
	
	// load action
	QToolButton * tool = new QToolButton;
	tool->setDefaultAction( new QAction( "load", this ) );
	connect( tool->defaultAction(), SIGNAL( triggered() ), this, SLOT( loadBrowse() ) );
	grid->addWidget( tool, 0, 0 );

	// the name of the file to load
	lineLoad = new QLineEdit( settings.value( "last load", QString( "" ) ).toString() );
	grid->addWidget( lineLoad, 0, 1, 1, 2 );
	connect( lineLoad, SIGNAL( returnPressed() ), this, SLOT( load() ) );
	
	// variable split layout
	split = new QSplitter;
	grid->addWidget( split, 1, 0, 1, 4 );
	grid->setRowStretch( 1, 4 );

	// this view shows the block list
	list = new NifTreeView;
	split->addWidget( list );
	QFont font = list->font();
	font.setPointSize( font.pointSize() + 3 );
	list->setFont( font );
	list->setVisible( settings.value( "show list", true ).toBool() );
	
	// this view shows the whole tree or the block details
	tree = new NifTreeView;
	split->addWidget( tree );
	tree->setFont( font );
	tree->setModel( model );
	tree->setVisible( settings.value( "show tree", false ).toBool() );
	
	connect( list, SIGNAL( clicked( const QModelIndex & ) ),
			this, SLOT( select( const QModelIndex & ) ) );

#ifdef QT_OPENGL_LIB
	// open gl
	ogl = new GLView;
	ogl->setVisible( settings.value( "show opengl", true ).toBool() );
	ogl->setNif( model );	
	split->addWidget( ogl );
	connect( ogl, SIGNAL( clicked( const QModelIndex & ) ),
			this, SLOT( select( const QModelIndex & ) ) );
#else
	ogl = 0;
#endif

	// save button
	grid->addWidget( (tool = new QToolButton), 2, 0 );
	tool->setDefaultAction( new QAction( "save", this ) );
	connect( tool->defaultAction(), SIGNAL( triggered() ), this, SLOT( saveBrowse() ) );

	// name of the file to write to
	lineSave = new QLineEdit( settings.value( "last save", "" ).toString() );
	grid->addWidget( lineSave, 2, 1 );
	connect( lineSave, SIGNAL( returnPressed() ), this, SLOT( save() ) );
	
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
		conditionZero->setChecked( settings.value( "hide condition zero", true ).toBool() );
		connect( conditionZero, SIGNAL( toggled( bool ) ), tree, SLOT( setEvalConditions( bool ) ) );
		optTree->layout()->addWidget( conditionZero );
		conditionZero->setToolTip( "checking this option makes the tree view better readable by displaying<br>only the rows where the condition and version is true" );
		tree->setEvalConditions( conditionZero->isChecked() );
		
#ifdef QT_OPENGL_LIB
		QGroupBox * optGL = new QGroupBox( "OpenGL Preview" );
		popOpts->layout()->addWidget( optGL );
		optGL->setFlat( true );
		optGL->setCheckable( true );
		optGL->setChecked( ogl->isVisibleTo( this ) );
		optGL->setEnabled( ogl->isValid() );
		connect( optGL, SIGNAL( toggled( bool ) ), ogl, SLOT( setVisible( bool ) ) );
		optGL->setLayout( new QVBoxLayout );
		
		QCheckBox * optTextures = new QCheckBox( "textures" );
		optTextures->setToolTip( "enable texturing" );
		optGL->layout()->addWidget( optTextures );
		optTextures->setChecked( settings.value( "enable textures", true ).toBool() );
		connect( optTextures, SIGNAL( toggled( bool ) ), ogl, SLOT( setTexturing( bool ) ) );
		ogl->setTexturing( optTextures->isChecked() );
		
		QLineEdit * optTexdir = new QLineEdit;
		optGL->layout()->addWidget( optTexdir );
		connect( optTexdir, SIGNAL( textChanged( const QString & ) ), ogl, SLOT( setTextureFolder( const QString & ) ) );
		optTexdir->setText( settings.value( "texture folder" ).toString() );
		optTexdir->setToolTip( "put in your texture folders here<br>if you have more than one seperate them with semicolons" );
		
		QCheckBox * optLights = new QCheckBox( "lighting" );
		optLights->setToolTip( "toggle lighting on or off" );
		optGL->layout()->addWidget( optLights );
		optLights->setChecked( settings.value( "enable lighting", true ).toBool() );
		connect( optLights, SIGNAL( toggled( bool ) ), ogl, SLOT( setLighting( bool ) ) );
		ogl->setLighting( optLights->isChecked() );
		
		QCheckBox * optAlpha = new QCheckBox( "blending" );
		optLights->setToolTip( "toggle alpha blending" );
		optGL->layout()->addWidget( optAlpha );
		optAlpha->setChecked( settings.value( "enable blending", true ).toBool() );
		connect( optAlpha, SIGNAL( toggled( bool ) ), ogl, SLOT( setBlending( bool ) ) );
		ogl->setBlending( optAlpha->isChecked() );
		
		QCheckBox * optAxis = new QCheckBox( "draw axis" );
		optGL->layout()->addWidget( optAxis );
		optAxis->setChecked( settings.value( "draw axis", true ).toBool() );
		connect( optAxis, SIGNAL( toggled( bool ) ), ogl, SLOT( setDrawAxis( bool ) ) );
		ogl->setDrawAxis( optAxis->isChecked() );
		
		QCheckBox * optRotate = new QCheckBox( "rotate" );
		optRotate->setToolTip( "slowly rotate the object around the z axis" );
		optGL->layout()->addWidget( optRotate );
		optRotate->setChecked( settings.value( "rotate", true ).toBool() );
		connect( optRotate, SIGNAL( toggled( bool ) ), ogl, SLOT( setRotate( bool ) ) );
		ogl->setRotate( optRotate->isChecked() );
		
		ogl->compile();
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
	grid->addWidget( tool, 2, 2 );
	
	// last but not least: set up a custom delegate to provide edit functionality
	list->setItemDelegate( model->createDelegate() );
	tree->setItemDelegate( model->createDelegate() );
	
	// tweak some display settings
	QFontMetrics m( list->font() );
	list->setIconSize( QSize( m.width( "000" ), m.lineSpacing() ) );
	list->header()->setStretchLastSection( false );
	restoreHeader( "list sizes", settings, list->header() );
	
	list->setColumnHidden( NifModel::TypeCol, true );
	list->setColumnHidden( NifModel::ArgCol, true );
	list->setColumnHidden( NifModel::Arr1Col, true );
	list->setColumnHidden( NifModel::Arr2Col, true );
	list->setColumnHidden( NifModel::CondCol, true );
	list->setColumnHidden( NifModel::Ver1Col, true );
	list->setColumnHidden( NifModel::Ver2Col, true );
	
	tree->setIconSize( QSize( m.width( "000" ), m.lineSpacing() ) );
	tree->header()->setStretchLastSection( false );
	restoreHeader( "tree sizes", settings, tree->header() );
	
	// fetch context menu signals from model views
	connect( list, SIGNAL( customContextMenuRequested( const QPoint & ) ),
			this, SLOT( contextMenu( const QPoint & ) ) );
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
	
	//split->restoreState( settings.value( "split sizes" ).toByteArray() );

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
	saveHeader( "list sizes", settings, list->header() );
	settings.setValue( "show tree", tree->isVisibleTo( this ) );
	settings.setValue( "hide condition zero", conditionZero->isChecked() );
	saveHeader( "tree sizes", settings, tree->header() );
#ifdef QT_OPENGL_LIB
	settings.setValue( "show opengl", ogl->isVisibleTo( this ) );
	settings.setValue( "texture folder", ogl->textureFolder() );
	settings.setValue( "enable textures", ogl->texturing() );
	settings.setValue( "enable lighting", ogl->lighting() );
	settings.setValue( "enable blending", ogl->blending() );
	settings.setValue( "draw axis", ogl->drawAxis() );
	settings.setValue( "rotate", ogl->rotate() );
#endif
	settings.setValue( "auto save settings", autoSettings->isChecked() );
	settings.setValue( "window geometry", normalGeometry() );
	//settings.setValue( "split sizes", split->saveState() );
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
	
	if ( sender() == list && list->model() == proxy )
	{
		menu->addSeparator();
		menu->addAction( "expand all" );
		menu->addAction( "collapse all" );
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
					list->setCurrentIndexExpanded( pidx );
				}
				else
					list->setCurrentIndexExpanded( target );
				tree->setRootIndex( target );
			}
			else
			{
				tree->setCurrentIndexExpanded( target );
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
		else if ( a->text() == "expand all" )
		{
			list->setAllExpanded( QModelIndex(), true );
		}
		else if ( a->text() == "collapse all" )
		{
			list->setAllExpanded( QModelIndex(), false );
		}
		else if ( a->text() == "remove Block" )
		{
			model->removeNiBlock( model->getBlockNumber( idx ) );
			model->updateHeader();
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
}

void NifSkope::setListMode( QAbstractButton * b )
{
	QModelIndex idx = list->currentIndex();
	if ( b->text() == "list" )
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
		return;
	
	QFile f( lineLoad->text() );
	if ( f.open( QIODevice::ReadOnly ) )
	{
		model->load( f );
#ifdef QT_OPENGL_LIB
		if ( ogl && ogl->isValid() ) ogl->compile( true );
#endif
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
		model->save( f );
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
	QString result = NifModel::parseXmlDescription( QDir( app.applicationDirPath() ).filePath( "NifSkope.xml" ) );
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

    if (app.argc() > 1)
        edit.load(QString(app.argv()[app.argc() - 1]));

	// start the event loop
	return app.exec();
}
