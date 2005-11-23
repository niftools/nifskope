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
