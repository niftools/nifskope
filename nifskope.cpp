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
#include <QByteArray>
#include <QCheckBox>
#include <QDataStream>
#include <QDesktopWidget>
#include <QFile>
#include <QFileDialog>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QMenu>
#include <QMessageBox>
#include <QSettings>
#include <QSplitter>
#include <QToolButton>

#include <QListView>
#include <QTreeView>

#include "nifmodel.h"

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
	QSplitter * split = new QSplitter;
	grid->addWidget( split, 1, 0, 1, 4 );

	// this view shows the block list
	list = new QListView;
	split->addWidget( list );
	QFont font = list->font();
	font.setPointSize( font.pointSize() + 3 );
	list->setFont( font );
	list->setModel( model );
	list->setVisible( settings.value( "show list", true ).toBool() );
	
	// this view shows the whole tree or the block details
	tree = new QTreeView;
	split->addWidget( tree );
	tree->setFont( font );
	tree->setModel( model );
	tree->setVisible( settings.value( "show tree", true ).toBool() );
	
	connect( list, SIGNAL( clicked( const QModelIndex & ) ),
			tree, SLOT( setRootIndex( const QModelIndex & ) ) );

#ifdef QT_OPENGL_LIB
	// open gl
	ogl = new GLView;
	ogl->setNif( model );	
	split->addWidget( ogl );	
	ogl->setVisible( settings.value( "show opengl", true ).toBool() );
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
		connect( optList, SIGNAL( toggled( bool ) ), this, SLOT( selectRoot() ) );
		
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
	tree->setItemDelegate( model->createDelegate() );
	
	load();
}

NifSkope::~NifSkope()
{
	if ( autoSettings->isChecked() )
		saveOptions();
}

void NifSkope::about()
{
	QMessageBox::about( this, "About NifSkope",
	"NifSkope is a simple low level tool for editing NetImmerse '.nif' files.<br><br>"
	"Because it uses the Qt libraries it is free software. You should have received"
	" a copy of the GPL and the source code all together in one package.<br>");
}

void NifSkope::saveOptions()
{
	QSettings settings( "NifTools", "NifSkope" );
	settings.setValue( "last load", lineLoad->text() );
	settings.setValue( "last save", lineSave->text() );
	settings.setValue( "show list", list->isVisibleTo( this ) );
	settings.setValue( "show tree", tree->isVisibleTo( this ) );
#ifdef QT_OPENGL_LIB
	settings.setValue( "show opengl", ogl->isVisibleTo( this ) );
	settings.setValue( "texture folder", ogl->textureFolder() );
	settings.setValue( "enable lighting", ogl->lighting() );
#endif
	settings.setValue( "hide condition zero", conditionZero->isChecked() );
	settings.setValue( "auto save settings", autoSettings->isChecked() );
}

void NifSkope::selectRoot()
{
	tree->setRootIndex( QModelIndex() );
}

void NifSkope::showHideRows( QModelIndex parent )
{
#ifndef HIDEZEROCOND
	return;
#endif

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
}

void NifSkope::updateConditionZero()
{
	showHideRows( QModelIndex() );
}

void NifSkope::dataChanged( const QModelIndex & idx, const QModelIndex & xdi )
{
	if ( ! idx.isValid() || ! xdi.isValid() )
	{
		showHideRows( QModelIndex() );
		return;
	}
	
	QModelIndex block = idx;
	
	if ( block.isValid() && ( block.row() != 0 || block.column() != 0 ) )
		block = block.sibling( 0, 0 );
	
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
		if ( ogl && ogl->isValid() ) ogl->compile();
#endif
		updateConditionZero();
		f.close();
	}	
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
	
	// start the event loop
	return app.exec();
}
