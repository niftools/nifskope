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

#include <popup.h>

#include <QAction>
#include <QApplication>
#include <QDesktopWidget>
#include <QMessageBox>

Popup::Popup( const QString & title, QWidget * parent )
	: QFrame( parent, Qt::Popup )
{
	aPopup = new QAction( title, this );
 	aPopup->setCheckable( true );
 	aPopup->setChecked( false );
	connect( aPopup, SIGNAL( triggered( bool ) ), this, SLOT( popup( bool ) ) );
	
	setFrameShape( QFrame::Box );
	
	setVisible( false );
}

Popup::~Popup()
{
}

void Popup::popup( bool x )
{
	if ( x )
		popup();
	else
		hide();
}

void Popup::popup()
{
	popup( QCursor::pos() );
}

void Popup::popup( const QPoint & pos )
{
	QPoint p = pos;
	
	int w = sizeHint().width();
	int h = sizeHint().height();
	
	if ( p.x() + w > QApplication::desktop()->width() )
		p.setX( p.x() - w );
	if ( p.x() < 0 )
		p.setX( 0 );
	if ( p.y() + h > QApplication::desktop()->height() )
		p.setY( p.y() - h );
	if ( p.y() < 0 )
		p.setY( 0 );
	
	move( p );
	
	show();
}

void Popup::showEvent( QShowEvent * e )
{
 	aPopup->setChecked( true );
	QFrame::showEvent( e );
}

void Popup::hideEvent( QHideEvent * e )
{
	emit aboutToHide();
 	aPopup->setChecked( false );
	QFrame::hideEvent( e );
}
