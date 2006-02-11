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

#include <QGLWidget>
#include <QCache>
#include <QDateTime>
#include <QFile>
#include <QStack>

#include "nifmodel.h"

class Scene;

class QSettings;
class QTimer;

class GLView : public QGLWidget
{
	Q_OBJECT

public:
	GLView();
	~GLView();
	
	QModelIndex indexAt( const QPoint & p );
	
	int xRotation() const { return xRot; }
	int yRotation() const { return yRot; }
	int zRotation() const { return zRot; }
	
	bool highlight() const;
	bool drawNodes() const;
	
	QString textureFolder() const;
	
	QSize minimumSizeHint() const { return QSize( 50, 50 ); }
	QSize sizeHint() const { return QSize( 400, 400 ); }

	void center();

	QAction * aTexturing;
	QAction * aBlending;
	QAction * aLighting;
	QAction * aDrawAxis;
	QAction * aDrawNodes;
	QAction * aDrawHidden;
	QAction * aHighlight;
	QAction * aRotate;
	QAction * aTexFolder;
	
	QAction * aAnimate;
	QAction * aAnimPlay;
	
	QList<QAction*> animActions() const;
	
	void	save( QSettings & );
	void	restore( QSettings & );

public slots:
	void setNif( NifModel * );

	void setTextureFolder( const QString & );
	void setNifFolder( const QString & );
	
	void setCurrentIndex( const QModelIndex & );

	void sltFrame( int );

	void selectTexFolder();
	
signals: 
	void clicked( const QModelIndex & );
	
	void sigFrame( int f, int mn, int mx );
	
protected:
	void initializeGL();
	void paintGL();
	int  pickGL( int x, int y );
	void resizeGL(int width, int height);
	void mousePressEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void mouseDoubleClickEvent( QMouseEvent * );
	void mouseMoveEvent(QMouseEvent *event);
	void wheelEvent( QWheelEvent * event );
	void glPerspective( int x = -1, int y = -1 );
	void glOrtho();

private slots:
	void advanceGears();
	
	void modelChanged();
	void modelLinked();
	void modelDestroyed();
	void dataChanged( const QModelIndex &, const QModelIndex & );
	
	void checkActions();

private:
	void normalizeAngle(int *angle);
	
	int xRot;
	int yRot;
	int zRot;

	GLdouble zoom;
	GLdouble xTrans;
	GLdouble yTrans;
	
	GLdouble radius;
	GLdouble axis;
	
	int zInc;
	
	bool doCompile;
	bool doCenter;
	
	QPoint lastPos;
	QPoint pressPos;
	
	NifModel * model;
	
	Scene * scene;
	
	QTimer * timer;
	
	float time;
	QTime lastTime;
};

#endif
