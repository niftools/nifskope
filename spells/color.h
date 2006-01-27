#ifndef SPELL_COLOR_H
#define SPELL_COLOR_H

#include <QColor>
#include <QWidget>

class ColorWheel : public QWidget
{
	Q_OBJECT
public:
	static QColor choose( const QColor & color, QWidget * parent = 0 );
	
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


#endif
