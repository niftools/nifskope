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
