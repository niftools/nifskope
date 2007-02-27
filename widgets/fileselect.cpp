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

// how long the visual save/load feedback is visible
#define FEEDBACK_TIME 1200

#include "fileselect.h"

#include <QAction>
#include <QCompleter>
#include <QContextMenuEvent>
#include <QDirModel>
#include <QFileDialog>
#include <QLayout>
#include <QMenu>
#include <QSettings>
#include <QThread>
#include <QTimer>
#include <QToolButton>

CompletionAction::CompletionAction( QObject * parent ) : QAction( "Completion of Filenames", parent )
{
	QSettings cfg( "NifTools", "NifSkope" );
	setCheckable( true );
	setChecked( cfg.value( "completion of file names", false ).toBool() );

	connect( this, SIGNAL( toggled( bool ) ), this, SLOT( sltToggled( bool ) ) );
}

CompletionAction::~CompletionAction()
{
}

void CompletionAction::sltToggled( bool )
{
	QSettings cfg( "NifTools", "NifSkope" );
	cfg.setValue( "completion of file names", isChecked() );
}

FileSelector::FileSelector( Modes mode, const QString & buttonText, QBoxLayout::Direction dir )
	: QWidget(), Mode( mode ), dirmdl( 0 ), completer( 0 )
{
	QBoxLayout * lay = new QBoxLayout( dir, this );
	lay->setMargin( 0 );
	setLayout( lay );
	
	line = new QLineEdit( this );
	// doesn't work in Qt 4.2.2
	// set the State property for the stylesheet
	// line->setProperty("State", "Neutral");

	connect( line, SIGNAL( textEdited( const QString & ) ), this, SIGNAL( sigEdited( const QString & ) ) );
	connect( line, SIGNAL( returnPressed() ), this, SLOT( activate() ) );
	
	action = new QAction( this );
	action->setText( buttonText );
	connect( action, SIGNAL( triggered() ), this, SLOT( browse() ) );
	addAction( action );
	
	QToolButton * button = new QToolButton( this );
	button->setDefaultAction( action );
	
	lay->addWidget( line );
	lay->addWidget( button );
	
	// setFocusProxy( line );
	
	line->installEventFilter( this );
	
	connect( completionAction(), SIGNAL( toggled( bool ) ), this, SLOT( setCompletionEnabled( bool ) ) );
	setCompletionEnabled( completionAction()->isChecked() );
}

QAction * FileSelector::completionAction()
{
	static QAction * action = new CompletionAction;
	return action;
}

void FileSelector::setCompletionEnabled( bool x )
{
	if ( x && ! dirmdl )
	{
		QDir::Filters fm;
		
		switch ( Mode )
		{
			case LoadFile:
				fm = QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot;
				break;
			case SaveFile:
				fm = QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot;
				break;
			case Folder:
				fm = QDir::AllDirs | QDir::NoDotAndDotDot;
				break;
		}
		
		dirmdl = new QDirModel( fltr, fm, QDir::DirsFirst | QDir::Name, this );
		dirmdl->setLazyChildCount( true );
		line->setCompleter( completer = new QCompleter( dirmdl, this ) );
	}
	else if ( ! x && dirmdl )
	{
		line->setCompleter( 0 );
		delete completer;
		completer = 0;
		delete dirmdl;
		dirmdl = 0;
	}
}

QString FileSelector::file() const
{
	return line->text();
}

void FileSelector::setFile( const QString & x )
{
	line->setText( x );
}

void FileSelector::setText( const QString & x )
{
	setFile( x );
}

void FileSelector::setState( const ActionStates & x )
{
	switch( x ) {
		case StateSuccess:
			// doesn't work in Qt 4.2.2
			// line->setProperty("State", "FileSuccess");
			line->setStyleSheet( "background-color: #efe;" );
			QTimer::singleShot( FEEDBACK_TIME, this, SLOT( setState() ) );
			break;

		case StateError:
			// line->setProperty("State", "FileError");
			line->setStyleSheet( "background-color: #fee;" );
			QTimer::singleShot( FEEDBACK_TIME, this, SLOT( setState() ) );
			break;

		default:
			// line->setProperty("State", "Neutral");
			line->setStyleSheet( "background-color: #fefefe;" );
			break;
	}
}

void FileSelector::replaceText( const QString & x )
{
	line->setCompleter( 0 );
	line->selectAll();
	line->del();
	line->insert( x );
	line->setCompleter( completer );
}

void FileSelector::setFilter( const QStringList & fltr )
{
	this->fltr = fltr;
	if ( dirmdl )
		dirmdl->setNameFilters( fltr );
}

QStringList FileSelector::filter() const
{
	return fltr;
}

void FileSelector::browse()
{
	QString x;
	
	switch ( Mode )
	{
		case Folder:
			x = QFileDialog::getExistingDirectory( this, "Choose a folder", file() );
			break;
		case LoadFile:
			x = QFileDialog::getOpenFileName( this, "Choose a file", file(), fltr.join( ";" ) );
			break;
		case SaveFile:
			x = QFileDialog::getSaveFileName( this, "Choose a file", file(), fltr.join( ";" ) );
			break;
	}
	
	if ( ! x.isEmpty() )
	{
		line->setText( x );
		activate();
	}
}

void FileSelector::activate()
{
	QFileInfo inf( file() );
	
	switch ( Mode )
	{
		case LoadFile:
			if ( ! inf.isFile() )
				return;
			break;
		case SaveFile:
			if ( inf.isDir() )
				return;
			break;
		case Folder:
			if ( ! inf.isDir() )
				return;
			break;
	}
	emit sigActivated( file() );
}

bool FileSelector::eventFilter( QObject * o, QEvent * e )
{
	if ( o == line && e->type() == QEvent::ContextMenu )
	{
		QContextMenuEvent * event = static_cast<QContextMenuEvent*>( e );
		
		QMenu * menu = line->createStandardContextMenu();
		menu->addSeparator();
		menu->addAction( completionAction() );
		menu->exec(event->globalPos());
		delete menu;
		return true;
	}
	
	return QWidget::eventFilter( o, e );
}
