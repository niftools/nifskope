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
#include <QColorDialog>
#include <QComboBox>
#include <QDesktopWidget>
#include <QDialog>
#include <QDirModel>
#include <QDoubleSpinBox>
#include <QGradient>
#include <QLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QToolButton>
#include <QTreeView>


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



#include <math.h>

ColorWheel::ColorWheel( QWidget * parent ) : QWidget( parent )
{
	H = 0.0; S = 0.0; V = 1.0;
}

QColor ColorWheel::getColor() const
{
	return QColor::fromHsvF( H, S, V );
}

void ColorWheel::setColor( const QColor & c )
{
	H = c.hueF();
	S = c.saturationF();
	V = c.valueF();
	if ( H >= 1.0 || H < 0.0 ) H = 0.0;
	if ( S > 1.0 || S < 0.0 ) S = 1.0;
	if ( V > 1.0 || S < 0.0 ) V = 1.0;
	update();
}

QSize ColorWheel::sizeHint() const
{
	return QSize( 250, 250 );
}

QSize ColorWheel::minimumSizeHint() const
{
	return QSize( 50, 50 );
}

int ColorWheel::heightForWidth( int width ) const
{
	if ( width < minimumSizeHint().height() )
		return minimumSizeHint().height();
	return width;
}

#ifndef pi
#define pi 3.1416
#endif

void ColorWheel::paintEvent( QPaintEvent * e )
{
	double s = qMin( width(), height() ) / 2.0;
	double c = s - s / 5;
	
	QPainter p( this );
	p.translate( width() / 2, height() / 2 );
	
	p.setPen( Qt::NoPen );
	
	QConicalGradient cgrad( QPointF( 0, 0 ), 90 );
	cgrad.setColorAt( 0.00, QColor::fromHsvF( 0.0, 1.0, 1.0 ) );
	for ( double d = 0.01; d < 1.00; d += 0.01 )
		cgrad.setColorAt( d, QColor::fromHsvF( d, 1.0, 1.0 ) );
	cgrad.setColorAt( 1.00, QColor::fromHsvF( 0.0, 1.0, 1.0 ) );
	
	p.setBrush( QBrush( cgrad ) );
	p.drawEllipse( QRectF( -s, -s, s*2, s*2 ) );
	p.setBrush( palette().color( QPalette::Background ) );
	p.drawEllipse( QRectF( -c, -c, c*2, c*2 ) );
	
	double x = ( H - 0.5 ) * 2 * pi;
	
	QPointF points[3];
	points[0] = QPointF( sin( x ) * c, cos( x ) * c );
	points[1] = QPointF( sin( x + 2 * pi / 3 ) * c, cos( x + 2 * pi / 3 ) * c );
	points[2] = QPointF( sin( x + 4 * pi / 3 ) * c, cos( x + 4 * pi / 3 ) * c );
	
	QColor colors[3][2];
	colors[0][0] = QColor::fromHsvF( H, 1.0, 1.0, 1.0 );
	colors[0][1] = QColor::fromHsvF( H, 0.0, 0.0, 0.0 );
	colors[1][0] = QColor::fromHsvF( H, 0.0, 0.0, 1.0 );
	colors[1][1] = QColor::fromHsvF( H, 0.0, 0.0, 0.0 );
	colors[2][0] = QColor::fromHsvF( H, 0.0, 1.0, 1.0 );
	colors[2][1] = QColor::fromHsvF( H, 0.0, 1.0, 0.0 );
	
	
	p.setBrush( QColor::fromHsvF( H, 0.0, .5 ) );
	p.drawPolygon( points, 3 );
	
	QLinearGradient lgrad( points[0], ( points[1]+points[2] ) / 2 );
	lgrad.setColorAt( 0.0, colors[0][0] );
	lgrad.setColorAt( 1.0, colors[0][1] );
	p.setBrush( lgrad );
	p.drawPolygon( points, 3 );
	
	lgrad = QLinearGradient( points[1], ( points[0]+points[2] ) / 2 );
	lgrad.setColorAt( 0.0, colors[1][0] );
	lgrad.setColorAt( 1.0, colors[1][1] );
	p.setBrush( lgrad );
	p.drawPolygon( points, 3 );
	
	lgrad = QLinearGradient( points[2], ( points[0]+points[1] ) / 2 );
	lgrad.setColorAt( 0.0, colors[2][0] );
	lgrad.setColorAt( 1.0, colors[2][1] );
	p.setBrush( lgrad );
	p.drawPolygon( points, 3 );
	
	p.setPen( QColor::fromHsvF( H, 0.0, 1.0, 0.8 ) );
	p.setBrush( QColor::fromHsvF( H, 1.0, 1.0, 1.0 ) );
	
	double z = (s-c)/2;
	QPointF pointH( sin( x ) * ( c + z ), cos( x ) * ( c + z ) );
	p.drawEllipse( QRectF( pointH - QPointF( z/2, z/2 ), QSizeF( z, z ) ) );
	
	p.setBrush( QColor::fromHsvF( H, S, V, 1.0 ) );
	p.rotate( ( 1.0 - H ) * 360 - 120 );
	QPointF pointSV( ( S - 0.5 ) * sqrt( c*c - c*c/4 ) * 2 * V, V * ( c + c/2 ) - c );
	p.drawEllipse( QRectF( pointSV - QPointF( z/2, z/2 ), QSizeF( z, z ) ) );
}

void ColorWheel::mousePressEvent( QMouseEvent * e )
{
	int dx = abs( e->x() - width() / 2 );
	int dy = abs( e->y() - height() / 2 );
	double d = sqrt( dx*dx + dy*dy );
	
	double s = qMin( width(), height() ) / 2;
	double c = s - s / 5;
	
	if ( d > s )
		pressed = Nope;
	if ( d > c )
		pressed = Circle;
	else
		pressed = Triangle;
	
	setColor( e->x(), e->y() );
}

void ColorWheel::mouseMoveEvent( QMouseEvent * e )
{
	setColor( e->x(), e->y() );
}

void ColorWheel::setColor( int x, int y )
{
	if ( pressed == Circle )
	{
		QLineF l( QPointF( width() / 2.0, height() / 2.0 ), QPointF( x, y ) );
		H = l.angle( QLineF( 0, 1, 0, 0 ) ) / 360.0;
		if ( l.dx() > 0 )	H = 1.0 - H;
		update();
		emit sigColor( getColor() );
	}
	else if ( pressed == Triangle )
	{
		QPointF mp( x - width() / 2, y - height() / 2 );
		
		QMatrix m;
		m.rotate( (H ) * 360.0 + 120 );
		QPointF p( m.map( mp ) );
		double c = qMin( width(), height() ) / 2;
		c -= c / 5;
		V = ( p.y() + c ) / ( c + c/2 );
		if ( V < 0.0 )	V = 0;
		if ( V > 1.0 )	V = 1.0;
		if ( V > 0 )
		{
			double h = V * ( c + c/2 ) / ( 2.0 * sin( 60.0 / 180.0 * pi ) );
			S = ( p.x() + h ) / ( h * 2 );
			if ( S < 0.0 )	S = 0.0;
			if ( S > 1.0 )	S = 1.0;
		}
		else
			S = 1.0;
		update();
		emit sigColor( getColor() );
	}
}

QColor ColorWheel::choose( const QColor & c, QWidget * parent )
{
	QDialog dlg( parent );
	dlg.setWindowTitle( "Choose a Color" );
	QGridLayout * grid = new QGridLayout;
	dlg.setLayout( grid );
	ColorWheel * hsv = new ColorWheel;
	hsv->setColor( c );
	grid->addWidget( hsv, 0, 0, 1, 2 );
	QPushButton * ok = new QPushButton( "ok" );
	grid->addWidget( ok, 1, 0 );
	QPushButton * cancel = new QPushButton( "cancel" );
	grid->addWidget( cancel, 1, 1 );
	connect( ok, SIGNAL( clicked() ), &dlg, SLOT( accept() ) );
	connect( cancel, SIGNAL( clicked() ), &dlg, SLOT( reject() ) );
	if ( dlg.exec() == QDialog::Accepted )
		return hsv->getColor();
	else
		return c;
}


QStringList selectMultipleDirs( const QString & title, const QStringList & def, QWidget * parent )
{
	QDialog dlg( parent );
	dlg.setWindowTitle( title );

	QGridLayout * grid = new QGridLayout;
	dlg.setLayout( grid );

	QDirModel * model = new QDirModel( QStringList(), QDir::Dirs, QDir::Name, &dlg );
	QTreeView * view = new QTreeView;
	view->setModel( model );
	view->setSelectionMode( QAbstractItemView::MultiSelection );
	view->setColumnHidden( 1, true );
	view->setColumnHidden( 2, true );
	view->setColumnHidden( 3, true );
	
	foreach ( QString d, def )
	{
		QModelIndex idx = model->index( d );
		if ( idx.isValid() )
		{
			view->selectionModel()->select( idx, QItemSelectionModel::Select );
			while ( idx.parent().isValid() )
			{
				idx = idx.parent();
				view->expand( idx );
			}
		}
	}

	grid->addWidget( view, 0, 0, 1, 2 );
	
	QPushButton * ok = new QPushButton( "ok" );
	grid->addWidget( ok, 1, 0 );
	QPushButton * cancel = new QPushButton( "cancel" );
	grid->addWidget( cancel, 1, 1 );
	QObject::connect( ok, SIGNAL( clicked() ), &dlg, SLOT( accept() ) );
	QObject::connect( cancel, SIGNAL( clicked() ), &dlg, SLOT( reject() ) );
	
	if ( dlg.exec() == QDialog::Accepted )
	{
		QStringList lst;
		foreach ( QModelIndex idx, view->selectionModel()->selectedIndexes() )
		{
			lst << model->filePath( idx );
		}
		return lst;
	}
	else
		return def;
}
