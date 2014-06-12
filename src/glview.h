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

#include "nifmodel.h"
#include "widgets/floatedit.h"
#include "widgets/floatslider.h"

#include <QGLWidget> // Inherited
#include <QDateTime>
#include <QPersistentModelIndex>

#include <math.h>


//! \file glview.h GLView class

class Scene;

class QAction;
class QActionGroup;
class QComboBox;
class QGLFormat;
class QMenu;
class QOpenGLContext;
class QOpenGLFunctions;
class QSettings;
class QToolBar;
class QTimer;

//! The model view window
class GLView final : public QGLWidget
{
	Q_OBJECT

	//! Constructor
	GLView( const QGLFormat & format, const QGLWidget * shareWidget = 0 );
	//! Destructor
	~GLView();

public:
	//! Static instance
	static GLView * create();

	QOpenGLContext * glContext;
	QOpenGLFunctions * glFuncs;

	Scene * getScene();
	void updateShaders();

	void center();
	void move( float, float, float );
	void rotate( float, float, float );
	void zoom( float );

	void setDistance( float );
	void setPosition( float, float, float );
	void setPosition( Vector3 );
	void setRotation( float, float, float );
	void setZoom( float );

	QModelIndex indexAt( const QPoint & p, int cycle = 0 );

	// UI

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
	QAction * aColorKeyDebug;
	QAction * aAnimate;
	QAction * aAnimPlay;
	QAction * aAnimLoop;
	QAction * aAnimSwitch;

	QToolBar * tAnim;
	QToolBar * tView;

	QComboBox * animGroups;
	FloatSlider * sldTime;

	QMenu * createMenu() const;
	QList<QToolBar *> toolbars() const;

	QSize minimumSizeHint() const override final { return { 50, 50 }; }
	QSize sizeHint() const override final { return { 400, 400 }; }

	// Settings

	void save( QSettings & );
	void restore( const QSettings & );

public slots:
	void setNif( NifModel * );
	void setCurrentIndex( const QModelIndex & );

	void sltTime( float );
	void sltSequence( const QString & );
	void sltSaveUserView();

signals:
	void clicked( const QModelIndex & );
	void paintUpdate();
	void sigTime( float t, float mn, float mx );

protected:
	//! Sets up the OpenGL rendering context, defines display lists, etc.
	void initializeGL() override final;
	//! Sets up the OpenGL viewport, projection, etc.
	void resizeGL( int width, int height ) override final;
#ifdef USE_GL_QPAINTER
	void paintEvent( QPaintEvent * ) override final;
#else
	//! Renders the OpenGL scene.
	void paintGL() override final;
#endif
	void glProjection( int x = -1, int y = -1 );

	// QWidget Event Handlers

	void dragEnterEvent( QDragEnterEvent * ) override final;
	void dragLeaveEvent( QDragLeaveEvent * ) override final;
	void dragMoveEvent( QDragMoveEvent * ) override final;
	void dropEvent( QDropEvent * ) override final;
	void focusOutEvent( QFocusEvent * ) override final;
	void keyPressEvent( QKeyEvent * ) override final;
	void keyReleaseEvent( QKeyEvent * ) override final;
	void mouseDoubleClickEvent( QMouseEvent * ) override final;
	void mouseMoveEvent( QMouseEvent * ) override final;
	void mousePressEvent( QMouseEvent * ) override final;
	void mouseReleaseEvent( QMouseEvent * ) override final;
	void wheelEvent( QWheelEvent * ) override final;

protected slots:
	void saveImage();

private:
	NifModel * model;
	Scene * scene;

	class TexCache * textures;

	float time;
	QTime lastTime;
	QTimer * timer;

	int fpscnt;
	float fpsact;
	float fpsacc;

	float Dist;
	Vector3 Pos;
	Vector3 Rot;
	GLdouble Zoom;
	GLdouble axis;
	Transform viewTrans;
	int zInc;
	
	QHash<int, bool> kbd;
	QPoint lastPos;
	QPoint pressPos;
	Vector3 mouseMov;
	Vector3 mouseRot;
	int cycleSelect;

	QPersistentModelIndex iDragTarget;
	QString fnDragTex, fnDragTexOrg;

	bool doCompile;
	bool doCenter;
	bool doMultisampling;

	QAction * checkedViewAction() const;
	void uncheckViewAction();

private slots:
	void advanceGears();
	void checkActions();
	void viewAction( QAction * );

	void dataChanged( const QModelIndex &, const QModelIndex & );
	void modelChanged();
	void modelLinked();
	void modelDestroyed();

	void sceneUpdate();
};

#endif
