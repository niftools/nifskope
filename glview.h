/****************************************************************************
**
** nifscope: a tool for analyzing and editing NetImmerse files (.nif & .kf)
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/


#ifndef GLVIEW
#define GLVIEW

#ifdef QT_OPENGL_LIB

#include <QGLWidget>
#include <QCache>
#include <QDateTime>
#include <QFile>
#include <QStack>
#include <QTimer>

#include "nifmodel.h"

#include "glmath.h"

class Scene;

class GLView : public QGLWidget
{
	Q_OBJECT

public:
	GLView();
	~GLView();
	
	int xRotation() const { return xRot; }
	int yRotation() const { return yRot; }
	int zRotation() const { return zRot; }
	
	bool texturing() const;
	bool blending() const;
	bool lighting() const { return lightsOn; }
	bool drawAxis() const { return drawaxis; }
	bool rotate() const { return timer->isActive(); }
	
	QString textureFolder() const;
	
	QSize minimumSizeHint() const { return QSize( 50, 50 ); }
	QSize sizeHint() const { return QSize( 400, 400 ); }

	void compile( bool center = false );

public slots:
	void setNif( NifModel * );

	void setXRotation(int angle);
	void setYRotation(int angle);
	void setZRotation(int angle);
	void setZoom( int zoom );
	void setXTrans( int );
	void setYTrans( int );
	
	void setBlending( bool );
	void setDrawAxis( bool );
	void setLighting( bool );
	void setRotate( bool );
	void setTexturing( bool );
	
	void setTextureFolder( const QString & );

signals: 
	void xRotationChanged(int angle);
	void yRotationChanged(int angle);
	void zRotationChanged(int angle);
	void zoomChanged( int zoom );
	void clicked( const QModelIndex & );

protected:
	void initializeGL();
	void paintGL();
	void resizeGL(int width, int height);
	void mousePressEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void mouseDoubleClickEvent( QMouseEvent * );
	void mouseMoveEvent(QMouseEvent *event);
	void wheelEvent( QWheelEvent * event );

private slots:
	void advanceGears();
	
	void dataChanged();

private:
	void normalizeAngle(int *angle);
	
	GLuint click_tex;
	
	int xRot;
	int yRot;
	int zRot;
	int zoom;
	
	int zInc;
	
	int xTrans;
	int yTrans;
	
	bool updated;
	bool doCompile;
	bool doCenter;
	
	QPoint lastPos;
	QPoint pressPos;
	
	NifModel * model;
	
	Scene * scene;
	
	bool lightsOn;
	bool drawaxis;	

	Vector boundMin, boundMax;
	
	QTimer * timer;
};

#else
class GLView {};
#endif


#endif
