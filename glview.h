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
#include <QQueue>

#include "nifmodel.h"
#include "gltransform.h"

class Scene;

class QActionGroup;
class QSettings;
class QTimer;

class GLView : public QGLWidget
{
	Q_OBJECT

public:
	GLView();
	~GLView();
	
	QModelIndex indexAt( const QPoint & p );
	
	void move( float, float, float );
	void setPosition( float, float, float );
	void setPosition( Vector3 );
	
	void setDistance( float );
	
	void rotate( float, float, float );
	void setRotation( float, float, float );
	
	void zoom( float );
	void setZoom( float );
	
	bool highlight() const;
	bool drawNodes() const;
	
	QString textureFolder() const;
	
	QSize minimumSizeHint() const { return QSize( 50, 50 ); }
	QSize sizeHint() const { return QSize( 400, 400 ); }

	void center();
	
	QActionGroup * grpView;
	
	QAction * aViewWalk;
	QAction * aViewTop;
	QAction * aViewFront;
	QAction * aViewSide;
	QAction * aViewPerspective;

	QAction * aAnimate;
	QAction * aAnimPlay;
	
	QActionGroup * grpSettings;
	
	QAction * aTexturing;
	QAction * aBlending;
	QAction * aLighting;
	QAction * aDrawAxis;
	QAction * aDrawNodes;
	QAction * aDrawHidden;
	QAction * aHighlight;
	QAction * aRotate;
	QAction * aBenchmark;
	
	QAction * aTexFolder;
	QAction * aBgColor;

	QAction * aOnlyTextured;
	QAction * aCullExp;
	
	void	save( QSettings & );
	void	restore( QSettings & );

public slots:
	void setNif( NifModel * );

	void setTextureFolder( const QString & );
	void setCurrentIndex( const QModelIndex & );

	void sltFrame( int );

	void selectTexFolder();
	void selectBgColor();
	void adjustCullExp();
	
signals: 
	void clicked( const QModelIndex & );
	
	void sigFrame( int f, int mn, int mx );
	
protected:
	void initializeGL();
	void paintGL();
	int  pickGL( int x, int y );
	void resizeGL( int width, int height );
	void glProjection( int x = -1, int y = -1 );

	
	void mousePressEvent( QMouseEvent * );
	void mouseReleaseEvent( QMouseEvent * );
	void mouseDoubleClickEvent( QMouseEvent * );
	void mouseMoveEvent( QMouseEvent * );
	void wheelEvent( QWheelEvent * );
	void keyPressEvent( QKeyEvent * );
	void keyReleaseEvent( QKeyEvent * );
	void focusOutEvent( QFocusEvent * );
	
private slots:
	void advanceGears();
	
	void modelChanged();
	void modelLinked();
	void modelDestroyed();
	void dataChanged( const QModelIndex &, const QModelIndex & );
	
	void checkActions();
	void viewAction( QAction * );

private:
	QAction * checkedViewAction() const;
	void uncheckViewAction();

	Vector3 Pos;
	Vector3 Rot;
	float	Dist;
	
	GLdouble Zoom;
	
	GLdouble axis;
	
	Transform viewTrans;
	
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
	
	QColor bgcolor;
	
	QHash<int,bool> kbd;
	Vector3 mouseMov;
	Vector3 mouseRot;
	
	int		fpscnt;
	float	fpsact;
	float	fpsacc;
};

#endif
