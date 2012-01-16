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

#ifndef GLVIEW
#define GLVIEW

// must come before GLee.h for linux
#include <QCache>
#include <QDateTime>
#include <QFile>
#include <QStack>
#include <QQueue>
#include <QtCore/QtCore> // extra include to avoid compile error
#include <QtGui/QtGui>   // dito

#include "gl/GLee.h"
#include <QGLWidget>

#include "nifmodel.h"
#include "widgets/floatedit.h"
#include "widgets/floatslider.h"

//! \file glview.h GLView class

class Scene;

class QActionGroup;
class QComboBox;
class QMenu;
class QSettings;
class QToolBar;
class QTimer;

//! The model view window
class GLView : public QGLWidget
{
	Q_OBJECT

	//! Constructor
	GLView( const QGLFormat & format, const QGLWidget * shareWidget = 0 );
	//! Destructor
	~GLView();
	
public:
	//! Static instance
	static GLView * create();
	
	QModelIndex indexAt( const QPoint & p, int cycle = 0 );
	
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
	
	QSize minimumSizeHint() const { return QSize( 50, 50 ); }
	QSize sizeHint() const { return QSize( 400, 400 ); }

	void center();
	
	QMenu * createMenu() const;
	QList<QToolBar*> toolbars() const;
	
	QActionGroup * grpView;
	
	QAction * aViewWalk;
	QAction * aViewTop;
	QAction * aViewFront;
	QAction * aViewSide;
	QAction * aViewFlip;
	QAction * aViewPerspective;
	QAction * aViewUser;
	QAction * aViewUserSave;
	
	QAction * aPrintView;
	
	QAction * aAnimate;
	QAction * aAnimPlay;
	QAction * aAnimLoop;
	QAction * aAnimSwitch;
	
	QToolBar * tAnim;
	QComboBox * animGroups;
	FloatSlider * sldTime;

	QToolBar * tView;
	
	void	save( QSettings & );
	void	restore( const QSettings & );

	Scene * getScene();

public slots:
	void setNif( NifModel * );

	void setCurrentIndex( const QModelIndex & );

	void sltTime( float );
	void sltSequence( const QString & );

	void updateShaders();
	
	void sltSaveUserView();
	
signals: 
	void clicked( const QModelIndex & );
	
	void sigTime( float t, float mn, float mx );

	void paintUpdate();

protected:
	//! Sets up the OpenGL rendering context, defines display lists, etc.
	void initializeGL();
	int  pickGL( int x, int y );
	//! Sets up the OpenGL viewport, projection, etc.
	void resizeGL( int width, int height );
	void glProjection( int x = -1, int y = -1 );

#ifdef USE_GL_QPAINTER
	void paintEvent( QPaintEvent * );
#else
	//! Renders the OpenGL scene.
	void paintGL();
#endif
	
	void mousePressEvent( QMouseEvent * );
	void mouseReleaseEvent( QMouseEvent * );
	void mouseDoubleClickEvent( QMouseEvent * );
	void mouseMoveEvent( QMouseEvent * );
	void wheelEvent( QWheelEvent * );
	void keyPressEvent( QKeyEvent * );
	void keyReleaseEvent( QKeyEvent * );
	void focusOutEvent( QFocusEvent * );
	
	void dragEnterEvent( QDragEnterEvent * );
	void dragMoveEvent( QDragMoveEvent * );
	void dropEvent( QDropEvent * );
	void dragLeaveEvent( QDragLeaveEvent * );
	
private slots:
	void advanceGears();
	
	void modelChanged();
	void modelLinked();
	void modelDestroyed();
	void dataChanged( const QModelIndex &, const QModelIndex & );
	
	void checkActions();
	void viewAction( QAction * );

	void sceneUpdate();

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
	
	int cycleSelect;
	
	NifModel * model;
	
	Scene * scene;
	
	QTimer * timer;
	
	float time;
	QTime lastTime;
	
	QHash<int,bool> kbd;
	Vector3 mouseMov;
	Vector3 mouseRot;
	
	int		fpscnt;
	float	fpsact;
	float	fpsacc;
	
	class TexCache * textures;
	
	QPersistentModelIndex iDragTarget;
	QString fnDragTex, fnDragTexOrg;
	
	QPoint popPos;
protected slots:
	void popMenu();
	void savePixmap();
};

#endif
