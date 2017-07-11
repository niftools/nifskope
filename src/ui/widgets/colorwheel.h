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

#ifndef COLORWHEEL_H
#define COLORWHEEL_H

#include <QWidget>   // Inherited
#include <QSpinBox>  // Inherited

#include <QColor>
#include <QRegularExpression>
#include <QSlider>


//! \file colorwheel.h ColorWheel, ColorSpinBox

class Color3;
class Color4;

//! A color selection widget using the HSV model
class ColorWheel final : public QWidget
{
	Q_OBJECT

public:
	static QColor choose( const QColor & color, bool alpha = true, QWidget * parent = nullptr );
	static Color3 choose( const Color3 & color, QWidget * parent = nullptr );
	static Color4 choose( const Color4 & color, QWidget * parent = nullptr );

	static QIcon getIcon();

	ColorWheel( QWidget * parent = nullptr );
	ColorWheel( const QColor & c, QWidget * parent = nullptr );

	Q_PROPERTY( QColor color READ getColor WRITE setColor NOTIFY sigColor USER true )

	QColor getColor() const;
	bool getAlpha() const;

	QSize sizeHint() const override final;
	QSize minimumSizeHint() const override final;

	void setSizeHint( const QSize & s );

	int heightForWidth( int width ) const override final;

signals:
	void sigColor( const QColor & );
	void sigColorEdited( const QColor & );

public slots:
	void setColor( const QColor & );
	void setAlpha( const bool & );
	void setAlphaValue( const float & );
	void chooseHex();

protected:
	void paintEvent( QPaintEvent * e ) override final;
	void mousePressEvent( QMouseEvent * e ) override final;
	void mouseMoveEvent( QMouseEvent * e ) override final;
	void contextMenuEvent( QContextMenuEvent * e ) override final;

	void setColor( int x, int y );

private:
	double H = 0, S = 0, V = 0, A = 0;

	bool isAlpha = false;

	enum
	{
		Nope, Circle, Triangle
	} pressed = Nope;

	QSize sHint;
};

class QLabel;
class QLineEdit;
class QDoubleSpinBox;
class QPushButton;

class ColorLineEdit final : public QWidget
{
	Q_OBJECT

public:
	ColorLineEdit( QWidget * parent = nullptr );

	QColor getColor() const;

	void setWheel( ColorWheel *, const QString & = "" );
	void setTitle( const QString & );
	void setColor( const QColor & );

signals:
	void textEdited( const QString & );

public slots:
	void setAlpha( float );

private:
	ColorWheel * wheel;
	QLabel * title;
	QLabel * lblColor;
	QLineEdit * color;
	QDoubleSpinBox * alpha;
	QPushButton * btn;

	bool hasAlpha = false;
};

class ColorSpinBox final : public QSpinBox
{
public:
	ColorSpinBox( QWidget * parent ) : QSpinBox( parent ) {}
	ColorSpinBox() : QSpinBox() {}

protected:

	QString textFromValue( int d ) const override final
	{
		return QString::number( d, 16 );
	}

	int valueFromText( const QString & text ) const override final
	{
		bool ok;
		return ( text.toInt( &ok, 16 ) );
	}

	QValidator::State validate( QString & input, int & pos ) const override final
	{
		return QRegularExpressionValidator( QRegularExpression( "[0-9A-Fa-f]{0,2}" ), 0 ).validate( input, pos );
	}
};

#endif
