#ifndef SPELL_COLOR_H
#define SPELL_COLOR_H

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
	
	Q_PROPERTY( QColor color READ getColor WRITE setColor STORED false );
	
	QColor getColor() const;
	
	QSize sizeHint() const;
	QSize minimumSizeHint() const;
	
	int heightForWidth( int width ) const;

signals:
	void sigColor( const QColor & );
	
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

	static QIcon * icon;
};

class AlphaSlider : public QSlider
{
	Q_OBJECT
public:
	AlphaSlider( QWidget * parent = 0 );
	
	QSize sizeHint() const;
	
public slots:
	void setColor( const QColor & c );
	
protected:
	void paintEvent( QPaintEvent * e );
	
	QColor color0;
	QColor color1;
};

#endif
