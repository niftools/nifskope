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

#include "fileselect.h"

#include <QAction>
#include <QFileDialog>
#include <QLayout>
#include <QLineEdit>
#include <QToolButton>

FileSelector::FileSelector( Modes mode, const QString & buttonText, QBoxLayout::Direction dir )
	: QWidget(), Mode( mode )
{
	QBoxLayout * lay = new QBoxLayout( dir );
	lay->setMargin( 0 );
	setLayout( lay );
	
	line = new QLineEdit;
	connect( line, SIGNAL( textEdited( const QString & ) ), this, SIGNAL( sigEdited( const QString & ) ) );
	connect( line, SIGNAL( returnPressed() ), this, SLOT( activate() ) );
	
	QAction * action = new QAction( this );
	action->setText( buttonText );
	connect( action, SIGNAL( activated() ), this, SLOT( browse() ) );
	
	button = new QToolButton;
	button->setDefaultAction( action );
	
	lay->addWidget( line );
	lay->addWidget( button );
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

void FileSelector::browse()
{
	QString x;
	
	switch ( Mode )
	{
		case Folder:
			x = QFileDialog::getExistingDirectory( this, "Choose a folder", file() );
			break;
		case LoadFile:
			x = QFileDialog::getOpenFileName( this, "Choose a file", file(), fltr );
			break;
		case SaveFile:
			x = QFileDialog::getSaveFileName( this, "Choose a file", file(), fltr );
			break;
	}
	
	if ( ! x.isEmpty() )
	{
		line->setText( x );
		emit sigActivated( x );
	}
}

void FileSelector::activate()
{
	emit sigActivated( file() );
}
