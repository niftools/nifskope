/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005-2012, NIF File Format Library and Tools
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

#ifndef FLOATSLIDER_H
#define FLOATSLIDER_H

#include <QWidget>
#include <QFrame>

//! \file floatslider.h FloatSlider, FloatSliderEditBox, AlphaSlider

class FloatEdit;

//! Frame used by FloatSlider
class FloatSliderEditBox : public QFrame
{
	Q_OBJECT
public:
	//! Constructor
	FloatSliderEditBox( QWidget * = NULL );

	//! Adds a widget to the internal layout
	void addWidget( QWidget * );

public slots:
	void show( const QPoint & );
	void hide();
	void focusChanged( QWidget *, QWidget * );
};

//! A value slider widget for floating-point values
class FloatSlider : public QWidget
{
	Q_OBJECT
public:
	//! Constructor
	FloatSlider( Qt::Orientation = Qt::Horizontal, bool = false, bool = false );
	
	//! Gets the value of the widget
	float value() const { return val; }
	
	//! Gets the minimum value of the widget
	float minimum() const { return min; }
	//! Gets the maximum value of the widget
	float maximum() const { return max; }
	
	//! Find the orientation of the widget
	Qt::Orientation orientation() const { return ori; }
	//! Set the orientation of the widget
	void setOrientation( Qt::Orientation o );
	
	//! Add an editor widget to the float slider frame
	void addEditor( QWidget * );
	
	//! The recommended size of the widget; reimplemented from QWidget
	QSize sizeHint() const;
	//! The recommended minimum size of the widget; reimplemented from QWidget 
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
	bool showVal;
	bool editVal;

	FloatSliderEditBox * editBox;
};

//! A slider for a alpha value in a Color4.
/*!
 * Draws with a gradient of the colour that the alpha value relates to.
 */
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
