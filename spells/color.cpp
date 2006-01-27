#include "color.h"

#include "../spellbook.h"

#include <QDialog>
#include <QLayout>
#include <QPainter>
#include <QPushButton>
#include <QMouseEvent>

class spChooseColor : public Spell
{
public:
	QString name() const { return "Choose"; }
	QString page() const { return "Color"; }
	QIcon icon() const { return ColorWheel::getIcon(); }
	
	bool isApplicable( NifModel * nif, const QModelIndex & index )
	{
		return nif->itemValue( index ).isColor();
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		if ( nif->itemValue( index ).type() == NifValue::tColor3 )
			nif->setItemData<Color3>( index, Color3( ColorWheel::choose( nif->itemData<Color3>( index ).toQColor() ) ) );
		else if ( nif->itemValue( index ).type() == NifValue::tColor4 )
			nif->setItemData<Color4>( index, Color4( ColorWheel::choose( nif->itemData<Color4>( index ).toQColor() ) ) );
		return index;
	}
};

REGISTER_SPELL( spChooseColor )


/* XPM */
static const char * const hsv42_xpm[] = {
"32 32 43 1",
" 	c None",
".	c #3200FF","+	c #0024FF","@	c #7600FF","#	c #0045FF","$	c #A000FF",
"%	c #FF0704","&	c #015AFF","*	c #FF0031","=	c #FF0058","-	c #FF0074",
";	c #FF0086",">	c #006DFF",",	c #D700FF","'	c #FF00A3",")	c #FF00C5",
"!	c #FC00DE","~	c #FC00FA","{	c #0086FF","]	c #FF3700","^	c #00A7FF",
"/	c #FF6000","(	c #00C3FF","_	c #00FF32",":	c #00DDFF","<	c #FF8D00",
"[	c #00FF55","}	c #23FF00","|	c #00FF72","1	c #00FF88","2	c #00FFA0",
"3	c #00FFBB","4	c #00FFD7","5	c #00F9F8","6	c #FFA900","7	c #60FF00",
"8	c #83FF00","9	c #FFC200","0	c #A3FF00","a	c #FFD800","b	c #C0FF00",
"c	c #FBF100","d	c #E6FF00",
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
"            .@@@@$$             "};

#include <math.h>

QIcon * ColorWheel::icon = 0;

ColorWheel::ColorWheel( QWidget * parent ) : QWidget( parent )
{
	H = 0.0; S = 0.0; V = 1.0;
}

QIcon ColorWheel::getIcon()
{
	if ( ! icon )
		icon = new QIcon(  hsv42_xpm  );
	return *icon;
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


