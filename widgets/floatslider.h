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

#ifndef FLOATSLIDER_H
#define FLOATSLIDER_H

#include <QWidget>

class FloatSlider : public QWidget
{
	Q_OBJECT
public:
	FloatSlider( Qt::Orientation o = Qt::Horizontal );
	
	float value() const { return val; }

	float minimum() const { return min; }
	float maximum() const { return max; }

	Qt::Orientation orientation() const { return ori; }
	void setOrientation( Qt::Orientation o );
	
	QSize sizeHint() const;
	QSize minimumSizeHint() const;
	
signals:
	void valueChanged( float );

public slots:
	void setValue( float val );
	void setRange( float min, float max );
	void set( float val, float min, float max );
	
protected:
	void setValueUser( float val );
	
	void paintEvent( QPaintEvent * );
	void mousePressEvent( QMouseEvent * );
	void mouseMoveEvent( QMouseEvent * );
	void mouseReleaseEvent( QMouseEvent * );
	
	float mapToValue( const QPoint & p ) const;
	
	class QStyleOptionSlider getStyleOption() const;
	
	float val, min, max;
	Qt::Orientation ori;
	
	bool pressed;
};

class AlphaSlider : public FloatSlider
{
	Q_OBJECT
public:
	AlphaSlider( Qt::Orientation o = Qt::Horizontal );
	
	QSize sizeHint() const;
	
public slots:
	void setColor( const QColor & c );
	
protected:
	void paintEvent( QPaintEvent * e );
	
	QColor color0;
	QColor color1;
};

#endif
