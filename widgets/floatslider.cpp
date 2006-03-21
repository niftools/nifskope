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

#include "floatslider.h"

#include <QApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QStyle>
#include <QStyleOption>

#define MAX 0xffff

FloatSlider::FloatSlider( Qt::Orientation o ) : QWidget(), val( 0.5 ), min( 0 ), max( 1.0 ), ori( o ), pressed( false )
{
	QSizePolicy sp( QSizePolicy::Expanding, QSizePolicy::Fixed );
	if ( ori == Qt::Vertical )
		sp.transpose();
	setSizePolicy( sp );
}

void FloatSlider::setValue( float v )
{
	if ( v < min )
		v = min;
	if ( v > max )
		v = max;
	
	if ( val != v )
	{
		val = v;
		update();
	}
}

void FloatSlider::setValueUser( float v )
{
	if ( v < min )
		v = min;
	if ( v > max )
		v = max;
	
	if ( val != v )
	{
		val = v;
		update();
		emit valueChanged( val );
	}
}

void FloatSlider::setRange( float mn, float mx )
{
	if ( mn > mx )
		mx = mn;
	
	if ( min != mn || max != mx )
	{
		min = mn;
		if ( val < min )
			setValue( min );
		
		max = mx;
		if ( val > max )
			setValue( max );
		
		update();
	}
}

void FloatSlider::set( float v, float mn, float mx )
{
	setRange( mn, mx );
	setValue( v );
}

void FloatSlider::setOrientation( Qt::Orientation o )
{
	if ( ori != o )
	{
		ori = o;
		QSizePolicy sp = sizePolicy();
		sp.transpose();
		setSizePolicy( sp );
		updateGeometry();
		update();
	}
}

QStyleOptionSlider FloatSlider::getStyleOption() const
{
	QStyleOptionSlider opt;
	/*
    opt.init(q);
    opt.orientation = orientation;
    opt.maximum = maximum;
    opt.minimum = minimum;
    opt.tickPosition = (QSlider::TickPosition)tickPosition;
    opt.tickInterval = tickInterval;
    opt.upsideDown = (orientation == Qt::Horizontal) ?
                     (invertedAppearance != (opt.direction == Qt::RightToLeft))
                     : (!invertedAppearance);
    opt.direction = Qt::LeftToRight; // we use the upsideDown option instead
    opt.sliderPosition = position;
    opt.sliderValue = value;
    opt.singleStep = singleStep;
    opt.pageStep = pageStep;
    if (orientation == Qt::Horizontal)
        opt.state |= QStyle::State_Horizontal;
	*/
	opt.initFrom( this );
	
    opt.subControls = QStyle::SC_None;
    opt.activeSubControls = QStyle::SC_None;

	opt.maximum = MAX;
	opt.minimum = 0;
	opt.orientation = ori;
	opt.pageStep = 10;
	opt.singleStep = 1;
	opt.sliderValue = int( ( val - min ) / ( max - min ) * MAX + .5 );
	if ( ori == Qt::Vertical )	opt.sliderValue = MAX - opt.sliderValue;
	opt.sliderPosition = opt.sliderValue;
	opt.tickPosition = QSlider::NoTicks;
	opt.upsideDown = false;
	opt.direction = Qt::LeftToRight;
    return opt;
	
}

void FloatSlider::paintEvent( QPaintEvent * e )
{
	QPainter p( this );
	QStyleOptionSlider opt = getStyleOption();
	
    opt.subControls = QStyle::SC_SliderGroove | QStyle::SC_SliderHandle;
    if ( pressed )
	{
        opt.activeSubControls = QStyle::SC_SliderHandle;
        opt.state |= QStyle::State_Sunken;
    }
	
	style()->drawComplexControl( QStyle::CC_Slider, &opt, &p, this );
}

void FloatSlider::mousePressEvent(QMouseEvent *ev)
{
    if ( max <= min || ( ev->buttons() != Qt::LeftButton ) )
	{
        ev->ignore();
        return;
    }
    ev->accept();
	
	pressed = true;
	
    setValueUser( mapToValue( ev->pos() ) );
    update();
}

void FloatSlider::mouseMoveEvent(QMouseEvent *ev)
{
    if ( max <= min || ( ev->buttons() != Qt::LeftButton ) )
	{
        ev->ignore();
        return;
    }
    ev->accept();
	
    setValueUser( mapToValue( ev->pos() ) );
	update();
}

void FloatSlider::mouseReleaseEvent(QMouseEvent *ev)
{
    if ( ev->button() != Qt::LeftButton )
	{
        ev->ignore();
        return;
    }
    ev->accept();
	
	pressed = false;
	update();
}

float FloatSlider::mapToValue( const QPoint & p ) const
{
    QStyleOptionSlider opt = getStyleOption();
    QRect gr = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove, this);
    QRect sr = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);
    int sliderMin, sliderLen, sliderPos;

    if ( ori == Qt::Horizontal )
	{
        sliderMin = gr.x() + sr.width() / 2;
        sliderLen = gr.width() - sr.width();
		sliderPos = p.x();
    }
	else
	{
        sliderMin = gr.y() + sr.height() / 2;
        sliderLen = gr.height() - sr.height();
		sliderPos = height() - p.y();
    }
	if ( sliderPos <= sliderMin )
		return min;
	if ( sliderPos >= sliderMin + sliderLen )
		return max;
    return min + ( float( sliderPos - sliderMin ) / float( sliderLen ) * ( max - min ) );
}

QSize FloatSlider::sizeHint() const
{
    QStyleOptionSlider opt = getStyleOption();
    int w = style()->pixelMetric(QStyle::PM_SliderThickness, &opt, this);
	int h = 84;
    if ( ori == Qt::Horizontal )
	{
		int x = h;
		h = w;
		w = x;
    }
    return style()->sizeFromContents( QStyle::CT_Slider, &opt, QSize( w, h ), this ).expandedTo( QApplication::globalStrut() );
}

QSize FloatSlider::minimumSizeHint() const
{
    QSize s = sizeHint();
    QStyleOptionSlider opt = getStyleOption();
    int length = style()->pixelMetric(QStyle::PM_SliderLength, &opt, this);
    if ( ori == Qt::Horizontal )
        s.setWidth(length);
    else
        s.setHeight(length);
    return s;
}

AlphaSlider::AlphaSlider( Qt::Orientation o )
	: FloatSlider( o )
{
	setRange( 0, 1.0 );
	setValue( 1.0 );
}

QSize AlphaSlider::sizeHint() const
{
	return FloatSlider::sizeHint() * 1.4;
}

void AlphaSlider::setColor( const QColor & c )
{
	color0 = c;
	color1 = c;
	color0.setAlphaF( 0.0 );
	color1.setAlphaF( 1.0 );
	
	update();
}

void AlphaSlider::paintEvent( QPaintEvent * e )
{
	int w2 = width() / 2;
	int h2 = height() / 2;
	
	QPoint points[2];
	if ( orientation() == Qt::Vertical )
	{
		points[0] = QPoint( w2, height() );
		points[1] = QPoint( w2, 0 );
	}
	else
	{
		points[0] = QPoint( 0, h2 );
		points[1] = QPoint( width(), h2 );
	}
	
	QLinearGradient agrad = QLinearGradient( points[0], points[1] );
	agrad.setColorAt( 0.0, color0 );
	agrad.setColorAt( 1.0, color1 );
	
	QPainter p;
	p.begin( this );
	p.fillRect( rect(), agrad );
	p.end();
	
	FloatSlider::paintEvent( e );
}

