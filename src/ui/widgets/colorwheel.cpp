/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005-2015, NIF File Format Library and Tools
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the NIF File Format Library and Tools project may not be
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

#include "colorwheel.h"

#include "spellbook.h"
#include "data/niftypes.h"
#include "ui/widgets/floatslider.h"

#include <QContextMenuEvent>
#include <QDialog>
#include <QIcon>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QMenu>
#include <QMouseEvent>


/* XPM */
static const char * const hsv42_xpm[] = {
	"32 32 43 1",
	"   c None",
	".	c #3200FF", "+	c #0024FF", "@	c #7600FF", "#	c #0045FF", "$	c #A000FF",
	"%	c #FF0704", "&	c #015AFF", "*	c #FF0031", "=	c #FF0058", "-	c #FF0074",
	";	c #FF0086", ">	c #006DFF", ",	c #D700FF", "'	c #FF00A3", ")	c #FF00C5",
	"!	c #FC00DE", "~	c #FC00FA", "{	c #0086FF", "]	c #FF3700", "^	c #00A7FF",
	"/	c #FF6000", "(	c #00C3FF", "_	c #00FF32", ":	c #00DDFF", "<	c #FF8D00",
	"[	c #00FF55", "}	c #23FF00", "|	c #00FF72", "1	c #00FF88", "2	c #00FFA0",
	"3	c #00FFBB", "4	c #00FFD7", "5	c #00F9F8", "6	c #FFA900", "7	c #60FF00",
	"8	c #83FF00", "9	c #FFC200", "0	c #A3FF00", "a	c #FFD800", "b	c #C0FF00",
	"c	c #FBF100", "d	c #E6FF00",
	"            7778800b            ",
	"         }}}}778800bbdd         ",
	"       }}}}}}777800bbddcc       ",
	"      }}}}}7}778800bdddcca      ",
	"     __}}}}}}77880bbddccca9     ",
	"    [___}}}}}}7780bbddcaa996    ",
	"   [[[___}}}}77880bddccaa9666   ",
	"  ||[[[___}}}}7780bddca9966<<   ",
	"  |1|[[[__}}}}7880bdcca966<<</  ",
	" 211|||[[__}}}}78bdcca966<</<// ",
	" 22211||[[[}}}778bdca96<<<///// ",
	" 3222121||[__}}70bda96<<</////] ",
	"3333322211|[__}78dc96<////]]]]]]",
	"444433332221|_}70d9<<///]]]]]]]%",
	"4444444433322|[}ba<//]]]]]]%%%%%",
	"555555454544442[c/]%%%%%%%%%%%%%",
	"5555555555:5::({!=*%%%%%%%%%%%%%",
	":5:5:::::(((^{&.$);-==******%*%%",
	":::::((((^^{>&+.@~)';-===*******",
	":(((((^^^{{>#+..@,~)';;-====*=* ",
	" (^^^^^{{>>#+..@@,~!)'';;--==== ",
	" ^^^^{{{>&##+...$$,~!)'';;---== ",
	" ^^{{{>>&##....@@$,,~!))'';;--  ",
	"  {{>>>&#++....@$$,,~~!)'''';;  ",
	"  {>>&&##++....@@$,,~~!!))'''   ",
	"   >&&##++.....@@$$,,~~!)))''   ",
	"    &#+++......@@$$,,~~~!)))    ",
	"     #+++.....@@@$$$,,~~!!!     ",
	"      ++.......@$$$,,,~~~!      ",
	"        ......@@@$$$$,,~        ",
	"         ......@@$$,,,          ",
	"            .@@@@$$             "
};

#include <math.h>

static QIconPtr icon = nullptr;

ColorWheel::ColorWheel( QWidget * parent ) : QWidget( parent )
{
	H = 0.0; S = 0.0; V = 1.0;
}

ColorWheel::ColorWheel( const QColor & c, QWidget * parent ) : QWidget( parent )
{
	H = c.hueF();
	S = c.saturationF();
	V = c.valueF();
	A = c.alphaF();

	if ( H >= 1.0 || H < 0.0 )
		H = 0.0;

	if ( S > 1.0 || S < 0.0 )
		S = 1.0;

	if ( V > 1.0 || V < 0.0 )
		V = 1.0;
}

QIcon ColorWheel::getIcon()
{
	if ( !icon )
		icon = QIconPtr( new QIcon(QPixmap( hsv42_xpm )) );

	return *icon;
}

QColor ColorWheel::getColor() const
{
	return QColor::fromHsvF( H, S, V, A );
}

void ColorWheel::setColor( const QColor & c )
{
	double h = c.hueF();
	S = c.saturationF();
	V = c.valueF();
	A = c.alphaF();

	if ( h >= 1.0 || h < 0.0 )
		h = 0.0;

	if ( S > 1.0 || S < 0.0 )
		S = 1.0;

	if ( V > 1.0 || V < 0.0 )
		V = 1.0;

	H = h;
	update();
	emit sigColor( c );
}

bool ColorWheel::getAlpha() const
{
	return isAlpha;
}

void ColorWheel::setAlpha( const bool & b )
{
	isAlpha = b;
}

void ColorWheel::setAlphaValue( const float & f )
{
	A = f;

	setColor( QColor::fromHsvF( H, S, V, A ) );
}

QSize ColorWheel::sizeHint() const
{
	if ( sHint.isValid() )
		return sHint;

	return { 200, 200 };
}

void ColorWheel::setSizeHint( const QSize & s )
{
	sHint = s;
}

QSize ColorWheel::minimumSizeHint() const
{
	return { 50, 50 };
}

int ColorWheel::heightForWidth( int width ) const
{
	if ( width < minimumSizeHint().height() )
		return minimumSizeHint().height();

	return width;
}

#ifndef M_PI
#define M_PI 3.1415926535897932385
#endif

void ColorWheel::paintEvent( QPaintEvent * e )
{
	Q_UNUSED( e );
	double s = qMin( width(), height() ) / 2.0;
	double c = s - s / 5;

	QPainter p( this );
	p.translate( width() / 2, height() / 2 );
	p.setRenderHint( QPainter::Antialiasing );
	p.setRenderHint( QPainter::HighQualityAntialiasing );

	p.setPen( Qt::NoPen );

	QConicalGradient cgrad( QPointF( 0, 0 ), 90 );
	cgrad.setColorAt( 0.00, QColor::fromHsvF( 0.0, 1.0, 1.0 ) );

	for ( double d = 0.01; d < 1.00; d += 0.01 )
		cgrad.setColorAt( d, QColor::fromHsvF( d, 1.0, 1.0 ) );

	cgrad.setColorAt( 1.00, QColor::fromHsvF( 0.0, 1.0, 1.0 ) );

	p.setBrush( QBrush( cgrad ) );
	p.drawEllipse( QRectF( -s, -s, s * 2, s * 2 ) );
	p.setBrush( palette().color( QPalette::Background ) );
	p.drawEllipse( QRectF( -c, -c, c * 2, c * 2 ) );

	double x = ( H - 0.5 ) * 2 * M_PI;

	QPointF points[3];
	points[0] = QPointF( sin( x ) * c, cos( x ) * c );
	points[1] = QPointF( sin( x + 2 * M_PI / 3 ) * c, cos( x + 2 * M_PI / 3 ) * c );
	points[2] = QPointF( sin( x + 4 * M_PI / 3 ) * c, cos( x + 4 * M_PI / 3 ) * c );

	QColor colors[3][2];
	colors[0][0] = QColor::fromHsvF( H, 1.0, 1.0, 1.0 );
	colors[0][1] = QColor::fromHsvF( H, 0.0, 0.0, 0.0 );
	colors[1][0] = QColor::fromHsvF( H, 0.0, 0.0, 1.0 );
	colors[1][1] = QColor::fromHsvF( H, 0.0, 0.0, 0.0 );
	colors[2][0] = QColor::fromHsvF( H, 0.0, 1.0, 1.0 );
	colors[2][1] = QColor::fromHsvF( H, 0.0, 1.0, 0.0 );


	p.setBrush( QColor::fromHsvF( H, 0.0, .5 ) );
	p.drawPolygon( points, 3 );

	QLinearGradient lgrad( points[0], ( points[1] + points[2] ) / 2 );
	lgrad.setColorAt( 0.0, colors[0][0] );
	lgrad.setColorAt( 1.0, colors[0][1] );
	p.setBrush( lgrad );
	p.drawPolygon( points, 3 );

	lgrad = QLinearGradient( points[1], ( points[0] + points[2] ) / 2 );
	lgrad.setColorAt( 0.0, colors[1][0] );
	lgrad.setColorAt( 1.0, colors[1][1] );
	p.setBrush( lgrad );
	p.drawPolygon( points, 3 );

	lgrad = QLinearGradient( points[2], ( points[0] + points[1] ) / 2 );
	lgrad.setColorAt( 0.0, colors[2][0] );
	lgrad.setColorAt( 1.0, colors[2][1] );
	p.setBrush( lgrad );
	p.drawPolygon( points, 3 );

	p.setPen( QColor::fromHsvF( H, 0.0, 1.0, 0.8 ) );
	p.setBrush( QColor::fromHsvF( H, 1.0, 1.0, 1.0 ) );

	double z = (s - c) / 2;
	QPointF pointH( sin( x ) * ( c + z ), cos( x ) * ( c + z ) );
	p.drawEllipse( QRectF( pointH - QPointF( z / 2, z / 2 ), QSizeF( z, z ) ) );

	p.setBrush( QColor::fromHsvF( H, S, V, 1.0 ) );
	p.rotate( ( 1.0 - H ) * 360 - 120 );
	QPointF pointSV( ( S - 0.5 ) * sqrt( c * c - c * c / 4 ) * 2 * V, V * ( c + c / 2 ) - c );
	p.drawEllipse( QRectF( pointSV - QPointF( z / 2, z / 2 ), QSizeF( z, z ) ) );
}

void ColorWheel::mousePressEvent( QMouseEvent * e )
{
	if ( e->button() != Qt::LeftButton )
		return;

	double dx = abs( e->x() - width() / 2 );
	double dy = abs( e->y() - height() / 2 );
	double d  = sqrt( dx * dx + dy * dy );

	double s = qMin( width(), height() ) / 2.0;
	double c = s - s / 5;

	if ( d > s )
		pressed = Nope;
	else if ( d > c )
		pressed = Circle;
	else
		pressed = Triangle;

	setColor( e->x(), e->y() );
}

void ColorWheel::mouseMoveEvent( QMouseEvent * e )
{
	if ( e->buttons() & Qt::LeftButton )
		setColor( e->x(), e->y() );
}

void ColorWheel::contextMenuEvent( QContextMenuEvent * e )
{
	QMenu * menu = new QMenu( this );

	for ( const QString& name : QColor::colorNames() ) {
		QAction * act = new QAction( menu );
		act->setText( name );
		QPixmap pix( 16, 16 );
		QPainter paint( &pix );
		paint.setBrush( QColor( name ) );
		paint.setPen( Qt::black );
		paint.drawRect( pix.rect().adjusted( 0, 0, -1, -1 ) );
		act->setIcon( QIcon( pix ) );
		menu->addAction( act );
	}

	QAction * hex = new QAction( tr( "Hex Edit..." ), this );
	menu->addSeparator();
	menu->addAction( hex );
	connect( hex, &QAction::triggered, this, &ColorWheel::chooseHex );

	if ( QAction * act = menu->exec( e->globalPos() ) ) {
		if ( act != hex ) {
			setColor( QColor( act->text() ) );
			emit sigColorEdited( getColor() );
		}
	}

	delete menu;
}

void ColorWheel::setColor( int x, int y )
{
	if ( pressed == Circle ) {
		QLineF l( QPointF( width() / 2.0, height() / 2.0 ), QPointF( x, y ) );
		H = l.angle( QLineF( 0, 1, 0, 0 ) ) / 360.0;

		if ( l.dx() > 0 )
			H = 1.0 - H;

		update();
		emit sigColor( getColor() );
		emit sigColorEdited( getColor() );
	} else if ( pressed == Triangle ) {
		QPointF mp( x - width() / 2, y - height() / 2 );

		QMatrix m;
		m.rotate( (H ) * 360.0 + 120 );
		QPointF p( m.map( mp ) );
		double c = qMin( width(), height() ) / 2.0;
		c -= c / 5;
		V  = ( p.y() + c ) / ( c + c / 2 );

		if ( V < 0.0 ) V = 0;
		if ( V > 1.0 ) V = 1.0;

		if ( V > 0 ) {
			double h = V * ( c + c / 2 ) / ( 2.0 * sin( 60.0 / 180.0 * M_PI ) );
			S = ( p.x() + h ) / ( h * 2 );

			if ( S < 0.0 ) S = 0.0;
			if ( S > 1.0 ) S = 1.0;
		} else {
			S = 1.0;
		}

		update();
		emit sigColor( getColor() );
		emit sigColorEdited( getColor() );
	}
}

QColor ColorWheel::choose( const QColor & c, bool alphaEnable, QWidget * parent )
{
	QDialog dlg( parent );
	dlg.setWindowTitle( "Choose a Color" );
	QGridLayout * grid = new QGridLayout;
	dlg.setLayout( grid );

	ColorWheel * hsv = new ColorWheel;
	hsv->setColor( c );
	grid->addWidget( hsv, 0, 0, 1, 2 );
	hsv->setAlpha( alphaEnable );

	AlphaSlider * alpha = new AlphaSlider;
	alpha->setColor( c );
	alpha->setValue( c.alphaF() );
	hsv->setAlphaValue( c.alphaF() );
	alpha->setOrientation( Qt::Vertical );
	grid->addWidget( alpha, 0, 2 );
	alpha->setVisible( alphaEnable );
	connect( hsv, &ColorWheel::sigColor, alpha, &AlphaSlider::setColor );
	connect( alpha, &AlphaSlider::valueChanged, hsv, &ColorWheel::setAlphaValue );

	QHBoxLayout * hbox = new QHBoxLayout;
	grid->addLayout( hbox, 1, 0, 1, 3 );
	QPushButton * ok = new QPushButton( "ok" );
	hbox->addWidget( ok );
	QPushButton * cancel = new QPushButton( "cancel" );
	hbox->addWidget( cancel );
	connect( ok, &QPushButton::clicked, &dlg, &QDialog::accept );
	connect( cancel, &QPushButton::clicked, &dlg, &QDialog::reject );

	if ( dlg.exec() == QDialog::Accepted ) {
		QColor color = hsv->getColor();
		color.setAlphaF( alpha->value() );
		return color;
	}

	return c;
}

Color3 ColorWheel::choose( const Color3 & c, QWidget * parent )
{
	if ( c.red() > 1.0 || c.green() > 1.0 || c.blue() > 1.0 )
		return c;

	return Color3( choose( c.toQColor(), false, parent ) );
}

Color4 ColorWheel::choose( const Color4 & c, QWidget * parent )
{
	if ( c.red() > 1.0 || c.green() > 1.0 || c.blue() > 1.0 || c.alpha() > 1.0 )
		return c;

	return Color4( choose( c.toQColor(), true, parent ) );
}

void ColorWheel::chooseHex()
{
	QDialog * dlg = new QDialog();
	ColorSpinBox * r, * g, * b, * a;
	QGridLayout * grid = new QGridLayout;
	dlg->setLayout( grid );

	//: Red
	grid->addWidget( new QLabel( tr( "R" ) ), 0, 0, 1, 1 );
	grid->addWidget( r = new ColorSpinBox(), 0, 1, 1, 1 );
	r->setSingleStep( 1 );
	r->setRange( 0, 255 );
	r->setValue( getColor().red() );

	//: Green
	grid->addWidget( new QLabel( tr( "G" ) ), 0, 2, 1, 1 );
	grid->addWidget( g = new ColorSpinBox(), 0, 3, 1, 1 );
	g->setSingleStep( 1 );
	g->setRange( 0, 255 );
	g->setValue( getColor().green() );

	//: Blue
	grid->addWidget( new QLabel( tr( "B" ) ), 0, 4, 1, 1 );
	grid->addWidget( b = new ColorSpinBox(), 0, 5, 1, 1 );
	b->setSingleStep( 1 );
	b->setRange( 0, 255 );
	b->setValue( getColor().blue() );

	QLabel * alphaLabel;
	//: Alpha
	grid->addWidget( alphaLabel = new QLabel( tr( "A" ) ), 0, 6, 1, 1 );
	grid->addWidget( a = new ColorSpinBox(), 0, 7, 1, 1 );
	a->setSingleStep( 1 );
	a->setRange( 0, 255 );
	a->setValue( getColor().alpha() );
	alphaLabel->setVisible( getAlpha() );
	a->setVisible( getAlpha() );

	QHBoxLayout * hBox  = new QHBoxLayout;
	QPushButton * btnOk = new QPushButton( tr( "OK" ) );
	QPushButton * btnCancel = new QPushButton( tr( "Cancel" ) );
	hBox->addWidget( btnOk );
	hBox->addWidget( btnCancel );
	grid->addLayout( hBox, 1, 0, 1, -1 );

	connect( btnOk, &QPushButton::clicked, dlg, &QDialog::accept );
	connect( btnCancel, &QPushButton::clicked, dlg, &QDialog::reject );

	if ( dlg->exec() == QDialog::Accepted ) {
		const QColor temp( r->value(), g->value(), b->value(), a->value() );
		setColor( temp );
		emit sigColorEdited( temp );
	}
}

ColorLineEdit::ColorLineEdit( QWidget * parent ) : QWidget( parent )
{
	QHBoxLayout * layout = new QHBoxLayout( this );

	setLayout( layout );

	title = new QLabel( this );
	title->setText( "Color" );
	title->setSizePolicy( QSizePolicy( QSizePolicy::Maximum, QSizePolicy::MinimumExpanding ) );

	lblColor = new QLabel( this );

	color = new QLineEdit( this );
	color->setText( "#FFFFFF" );
	color->setMaxLength( 7 );
	color->setAlignment( Qt::AlignCenter );
	color->setMaximumWidth( 60 );

	alpha = new QDoubleSpinBox( this );
	alpha->setDecimals( 4 );
	alpha->setMinimum( 0.0 );
	alpha->setMaximum( 1.0 );
	alpha->setSingleStep( 0.01 );
	alpha->setVisible( false );
	alpha->setHidden( true );

	layout->setAlignment( Qt::AlignLeft );
	layout->addWidget( title );
	layout->addWidget( lblColor );
	layout->addWidget( color );
	layout->addWidget( alpha );
}

QColor ColorLineEdit::getColor() const
{
	QColor c = QColor( color->text() );
	if ( hasAlpha )
		c.setAlphaF( alpha->value() );

	return c;
}

void ColorLineEdit::setWheel( ColorWheel * cw, const QString & str )
{
	wheel = cw;

	if ( !str.isEmpty() )
		setTitle( str );

	connect( wheel, &ColorWheel::sigColor, this, &ColorLineEdit::setColor );
	connect( wheel, &ColorWheel::sigColor, [this]() {
		lblColor->setStyleSheet( "background-color: " + wheel->getColor().name( QColor::HexArgb ) + ";" );
	} );

	connect( color, &QLineEdit::cursorPositionChanged, [this]() {
		if ( color->cursorPosition() == 0 ) {
			color->setCursorPosition( 1 );
		}
	} );

	connect( color, &QLineEdit::textEdited, [this]() {
		QString colorTxt = color->text();

		// Prevent "#" deletion
		if ( !color->text().startsWith( "#" ) ) {
			color->setText( "#" + color->text() );
		}

		QColor c = QColor( colorTxt );
		if ( hasAlpha )
			c.setAlphaF( alpha->value() );

		if ( (color->text().length() % 2 == 0) || !QColor::isValidColor( colorTxt ) )
			return;

		QColor wc = wheel->getColor();
		if ( c.toRgb() != wc.toRgb() )
			wheel->setColor( c );

		emit textEdited( colorTxt );
	} );
}

void ColorLineEdit::setTitle( const QString & str )
{
	title->setText( str );
}

void ColorLineEdit::setColor( const QColor & c )
{
	color->setText( c.name( QColor::HexRgb ) );

	if ( hasAlpha )
		alpha->setValue( c.alphaF() );

	QColor wc = wheel->getColor();
	
	// Sync color wheel
	//	Do NOT compare entire QColor, will create
	//	infinite loop between their ::setColor()
	if ( hasAlpha && c.alphaF() != wc.alphaF() )
		wheel->setColor( c );

	if ( c.red() != wc.red() || c.green() != wc.green() || c.blue() != wc.blue() )
		wheel->setColor( c );
}

void ColorLineEdit::setAlpha( float a )
{
	hasAlpha = true;

	alpha->setValue( a );
}
