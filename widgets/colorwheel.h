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

#ifndef COLORWHEEL_H
#define COLORWHEEL_COLOR_H

#include <QColor>
#include <QSlider>
#include <QWidget>

#include "../niftypes.h"

class ColorWheel : public QWidget
{
	Q_OBJECT
public:
	static QColor choose( const QColor & color, bool alpha = true, QWidget * parent = 0 );
	static Color3 choose( const Color3 & color, QWidget * parent = 0 );
	static Color4 choose( const Color4 & color, QWidget * parent = 0 );
	
	static QIcon getIcon();
	
	ColorWheel( QWidget * parent = 0 );
	ColorWheel( const QColor & c, QWidget * parent = 0 );
	
	Q_PROPERTY( QColor color READ getColor WRITE setColor STORED false );
	
	QColor getColor() const;
	
	QSize sizeHint() const;
	QSize minimumSizeHint() const;
	
	void setSizeHint( const QSize & s );
	
	int heightForWidth( int width ) const;

signals:
	void sigColor( const QColor & );
	void sigColorEdited( const QColor & );
	
public slots:
	void setColor( const QColor & );
	
protected:
	void paintEvent( QPaintEvent * e );
	void mousePressEvent( QMouseEvent * e );
	void mouseMoveEvent( QMouseEvent * e );

	void setColor( int x, int y );

private:
	double H, S, V;
	
	enum {
		Nope, Circle, Triangle
	} pressed;
	
	QSize sHint;

	static QIcon * icon;
};

#endif
