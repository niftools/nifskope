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

#include "glview.h"

#include <QtOpenGL>
#include <QActionGroup>
#include <QComboBox>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>


#include <math.h>

#include "nifmodel.h"
#include "gl/glscene.h"
#include "gl/gltex.h"
#include "gl/options.h"

#include "widgets/floatslider.h"

#define FPS 25

#define FOV 45.0

#define MOV_SPD 350
#define ROT_SPD 45

#define ZOOM_MIN 1.0
#define ZOOM_MAX 1000.0

#ifndef M_PI
#define M_PI 3.1415926535897932385
#endif 

GLView::GLView( const QGLFormat & format )
	: QGLWidget( format )
{
	setFocusPolicy( Qt::ClickFocus );
	//setContextMenuPolicy( Qt::CustomContextMenu );
	
	Zoom = 1.0;
	zInc = 1;
	
	doCenter = false;
	doCompile = false;
	
	model = 0;
	
	time = 0.0;
	lastTime = QTime::currentTime();
	
	fpsact = 0.0;
	fpsacc = 0.0;
	fpscnt = 0;
	
	scene = new Scene();

	timer = new QTimer(this);
	timer->setInterval( 1000 / FPS );
	timer->start();
	connect( timer, SIGNAL( timeout() ), this, SLOT( advanceGears() ) );
	
	
	grpView = new QActionGroup( this );
	grpView->setExclusive( false );
	connect( grpView, SIGNAL( triggered( QAction * ) ), this, SLOT( viewAction( QAction * ) ) );
	
	aViewTop = new QAction( "Top", grpView );
	aViewTop->setToolTip( "View from above" );
	aViewTop->setCheckable( true );
	aViewTop->setShortcut( Qt::Key_F5 );
	grpView->addAction( aViewTop );
	
	aViewFront = new QAction( "Front", grpView );
	aViewFront->setToolTip( "View from the front" );
	aViewFront->setCheckable( true );
	aViewFront->setChecked( true );
	aViewFront->setShortcut( Qt::Key_F6 );
	grpView->addAction( aViewFront );
	
	aViewSide = new QAction( "Side", grpView );
	aViewSide->setToolTip( "View from the side" );
	aViewSide->setCheckable( true );
	aViewSide->setShortcut( Qt::Key_F7 );
	grpView->addAction( aViewSide );
	
	aViewWalk = new QAction( "Walk", grpView );
	aViewWalk->setToolTip( "Enable walk mode" );
	aViewWalk->setCheckable( true );
	aViewWalk->setShortcut( Qt::Key_F8 );
	grpView->addAction( aViewWalk );
	
	aViewFlip = new QAction( "Flip", grpView );
	aViewFlip->setToolTip( "Flip View from Front to Back, Top to Bottom, Side to Other Side" );
	aViewFlip->setCheckable( true );
	aViewFlip->setShortcut( Qt::Key_F10 );
	grpView->addAction( aViewFlip );
	
	aViewPerspective = new QAction( "Perspective", grpView );
	aViewPerspective->setToolTip( "Perspective View Transformation or Orthogonal View Transformation" );
	aViewPerspective->setCheckable( true );
	aViewPerspective->setShortcut( Qt::Key_F9 );
	grpView->addAction( aViewPerspective );
	
	aAnimate = new QAction( "&Animations", this );
	aAnimate->setToolTip( "enables evaluation of animation controllers" );
	aAnimate->setCheckable( true );
	aAnimate->setChecked( true );
	connect( aAnimate, SIGNAL( toggled( bool ) ), this, SLOT( checkActions() ) );
	addAction( aAnimate );

	aAnimPlay = new QAction( "&Play", this );
	aAnimPlay->setCheckable( true );
	aAnimPlay->setChecked( true );
	connect( aAnimPlay, SIGNAL( toggled( bool ) ), this, SLOT( checkActions() ) );
	
	aAnimLoop = new QAction( "&Loop", this );
	aAnimLoop->setCheckable( true );
	aAnimLoop->setChecked( true );
	
	aAnimSwitch = new QAction( "&Switch", this );
	aAnimSwitch->setCheckable( true );
	aAnimSwitch->setChecked( true );
	
	// animation tool bar
	
	tAnim = new QToolBar( "Animation" );
	tAnim->setObjectName( "AnimTool" );
	tAnim->setAllowedAreas( Qt::TopToolBarArea | Qt::BottomToolBarArea );
	
	connect( aAnimate, SIGNAL( toggled( bool ) ), tAnim->toggleViewAction(), SLOT( setChecked( bool ) ) );
	connect( aAnimate, SIGNAL( toggled( bool ) ), tAnim, SLOT( setVisible( bool ) ) );
	connect( tAnim->toggleViewAction(), SIGNAL( toggled( bool ) ), aAnimate, SLOT( setChecked( bool ) ) );
	
	tAnim->addAction( aAnimPlay );
	
	FloatSlider * sldTime = new FloatSlider( Qt::Horizontal );
	connect( this, SIGNAL( sigTime( float, float, float ) ), sldTime, SLOT( set( float, float, float ) ) );
	connect( sldTime, SIGNAL( valueChanged( float ) ), this, SLOT( sltTime( float ) ) );
	tAnim->addWidget( sldTime );
	
	tAnim->addAction( aAnimLoop );

	animGroups = new QComboBox();
	animGroups->setMinimumWidth( 100 );
	connect( animGroups, SIGNAL( activated( const QString & ) ), this, SLOT( sltSequence( const QString & ) ) );
	tAnim->addWidget( animGroups );

	tAnim->addAction( aAnimSwitch );
	
	connect( GLOptions::get(), SIGNAL( sigChanged() ), this, SLOT( update() ) );
}

GLView::~GLView()
{
	makeCurrent();
	delete scene;
}

QList<QToolBar*> GLView::toolbars() const
{
	return QList<QToolBar*>() << tAnim;
}

void GLView::updateShaders()
{
	makeCurrent();
	scene->updateShaders();
	update();
}

GLView * GLView::create()
{
	QGLFormat fmt;
	fmt.setSampleBuffers( GLOptions::antialias() );
	return new GLView( fmt );
}


/*
 *  OpenGL stuff
 */

void GLView::initializeGL()
{
	initializeTextureUnits( context() );
	
	glShadeModel( GL_SMOOTH );
	
	if ( ! Renderer::initialize( context() ) )
	{
		/*
		aShading->setChecked( false );
		aShading->setDisabled( true );
		*/
	}
	else
		updateShaders();

	// check for errors
	
	GLenum err;
	while ( ( err = glGetError() ) != GL_NO_ERROR )
		qDebug() << "GL ERROR (init) : " << (const char *) gluErrorString( err );
}

void GLView::glProjection( int x, int y )
{
	GLint	viewport[4];
	glGetIntegerv( GL_VIEWPORT, viewport );
	GLdouble aspect = (GLdouble) viewport[2] / (GLdouble) viewport[3];
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if ( x >= 0 && y >= 0 )
	{
		gluPickMatrix( (GLdouble) x, (GLdouble) (viewport[3]-y), 5.0f, 5.0f, viewport);
	}
	
	BoundSphere bs = scene->view * scene->bounds();
	if ( GLOptions::drawAxes() )
		bs |= BoundSphere( scene->view * Vector3(), axis );
	
	GLdouble nr = fabs( bs.center[2] ) - bs.radius * 1.2;
	GLdouble fr = fabs( bs.center[2] ) + bs.radius * 1.2;
	
	//qWarning() << nr << fr;
	
	if ( aViewPerspective->isChecked() || aViewWalk->isChecked() )
	{
		if ( nr < 1.0 ) nr = 1.0;
		if ( fr < 2.0 ) fr = 2.0;
		GLdouble h2 = tan( ( FOV / Zoom ) / 360 * M_PI ) * nr;
		GLdouble w2 = h2 * aspect;
		glFrustum( - w2, + w2, - h2, + h2, nr, fr );
	}
	else
	{
		GLdouble h2 = Dist / Zoom;
		GLdouble w2 = h2 * aspect;
		glOrtho( - w2, + w2, - h2, + h2, nr, fr );
	}
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void GLView::center()
{
	doCenter = true;
	update();
}

void GLView::paintGL()
{
	if ( ! ( isVisible() && height() && width() ) )
		return;
	
	qglClearColor( GLOptions::bgColor() );
	
	glEnable( GL_DEPTH_TEST );
	glDepthMask( GL_TRUE );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	
	// compile the model
	
	if ( doCompile )
	{
		scene->make( model );
		scene->transform( Transform(), scene->timeMin() );
		axis = scene->bounds().radius * 1.4;
		if ( axis == 0 ) axis = 1;
		
		if ( time < scene->timeMin() || time > scene->timeMax() )
			time = scene->timeMin();
		emit sigTime( time, scene->timeMin(), scene->timeMax() );
		
		animGroups->clear();
		animGroups->addItems( scene->animGroups );
		animGroups->setCurrentIndex( scene->animGroups.indexOf( scene->animGroup ) );
		
		doCompile = false;
	}
	
	// center the model
	
	if ( doCenter )
	{
		viewAction( checkedViewAction() );
		doCenter = false;
	}
	
	// transform the scene
	
	Matrix ap;
	
	if ( GLOptions::upAxis() == GLOptions::YAxis )
	{
		ap( 0, 0 ) = 0; ap( 0, 1 ) = 0; ap( 0, 2 ) = 1;
		ap( 1, 0 ) = 1; ap( 1, 1 ) = 0; ap( 1, 2 ) = 0;
		ap( 2, 0 ) = 0; ap( 2, 1 ) = 1; ap( 2, 2 ) = 0;
	}
	else if ( GLOptions::upAxis() == GLOptions::XAxis )
	{
		ap( 0, 0 ) = 0; ap( 0, 1 ) = 1; ap( 0, 2 ) = 0;
		ap( 1, 0 ) = 0; ap( 1, 1 ) = 0; ap( 1, 2 ) = 1;
		ap( 2, 0 ) = 1; ap( 2, 1 ) = 0; ap( 2, 2 ) = 0;
	}
	
	Transform viewTrans;
	viewTrans.rotation.fromEuler( Rot[0] / 180.0 * PI, Rot[1] / 180.0 * PI, Rot[2] / 180.0 * PI );
	viewTrans.rotation = viewTrans.rotation * ap;
	viewTrans.translation = viewTrans.rotation * Pos;
	if ( ! aViewWalk->isChecked() )
		viewTrans.translation[2] -= Dist * 2;
	
	scene->transform( viewTrans, time );
	
	// setup projection mode

	glProjection();
	
	// draw the axis
	
	if ( GLOptions::drawAxes() )
	{
		glDisable( GL_ALPHA_TEST );
		glDisable( GL_BLEND );
		glDisable( GL_LIGHTING );
		glDisable( GL_COLOR_MATERIAL );
		glEnable( GL_DEPTH_TEST );
		glDepthMask( GL_TRUE );
		glDepthFunc( GL_LESS );
		glDisable( GL_TEXTURE_2D );
		glDisable( GL_NORMALIZE );
		glLineWidth( 1.2 );
		
		glPushMatrix();
		glMultMatrix( viewTrans );
		
		drawAxes( Vector3(), axis );
		
		glPopMatrix();
	}
	
	// setup light
	
	Vector4 lightDir( 0.0, 0.0, 1.0, 0.0 );
	if ( ! GLOptions::lightFrontal() )
	{
		float decl = GLOptions::lightDeclination() / 180.0 * PI;
		Vector3 v( sin( decl ), 0, cos( decl ) );
		Matrix m; m.fromEuler( 0, 0, GLOptions::lightPlanarAngle() / 180.0 * PI );
		v = m * v;
		lightDir = Vector4( viewTrans.rotation * v, 0.0 );
	}
	glLightfv( GL_LIGHT0, GL_POSITION, lightDir.data() );
	glLightfv( GL_LIGHT0, GL_AMBIENT, Color4( GLOptions::ambient() ).data() );
	glLightfv( GL_LIGHT0, GL_DIFFUSE, Color4( GLOptions::diffuse() ).data() );
	glLightfv( GL_LIGHT0, GL_SPECULAR, Color4( GLOptions::specular() ).data() );
	glLightModeli( GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE );
	glEnable( GL_LIGHT0 );
	glEnable( GL_LIGHTING );
	
	// draw the model

	scene->draw();
	
	// draw fps counter
	
	if ( fpsacc > 1.0 && fpscnt )
	{
		fpsacc /= fpscnt;
		if ( fpsacc > 0.0001 )
			fpsact = 1.0 / fpsacc;
		else
			fpsact = 10000;
		fpsacc = 0;
		fpscnt = 0;
	}

	// check for errors
	
	GLenum err;
	while ( ( err = glGetError() ) != GL_NO_ERROR )
		qDebug() << "GL ERROR (paint): " << (const char *) gluErrorString( err );

	if ( GLOptions::benchmark() || GLOptions::drawStats() )
	{
		glDisable( GL_ALPHA_TEST );
		glDisable( GL_BLEND );
		glDisable( GL_LIGHTING );
		glDisable( GL_COLOR_MATERIAL );
		glEnable( GL_DEPTH_TEST );
		glDepthMask( GL_TRUE );
		glDepthFunc( GL_LESS );
		glDisable( GL_TEXTURE_2D );
		glDisable( GL_NORMALIZE );
		glLineWidth( 1.2 );
		glNormalColor();
		
		int ls = QFontMetrics( font() ).lineSpacing();
		int y = 1;
		
		if ( GLOptions::benchmark() )
		{
			renderText( 0, y++ * ls, QString( "FPS %1" ).arg( int( fpsact ) ), font() );
			y++;
		}
		
		if ( GLOptions::drawStats() )
		{
			QString stats = scene->textStats();
			QStringList lines = stats.split( "\n" );
			foreach ( QString line, lines )
				renderText( 0, y++ * ls, line, font() );
		}
	}
}

bool compareHits( const QPair< GLuint, GLuint > & a, const QPair< GLuint, GLuint > & b )
{
	if ( a.second < b.second )
		return true;
	else
		return false;
}

typedef void (Scene::*DrawFunc)(void);
	
int indexAt( GLuint *buffer, NifModel *model, Scene *scene, QList<DrawFunc> drawFunc, int cycle )
{
	glRenderMode( GL_SELECT );	
	glInitNames();
	glPushName( 0 );
	
	foreach ( DrawFunc df, drawFunc )
		(scene->*df)();
		
	GLint hits = glRenderMode( GL_RENDER );
	if ( hits > 0 )
	{
		QList< QPair< GLuint, GLuint > > hitList;
		
		for ( int l = 0; l < hits; l++ )
		{
			hitList << QPair< GLuint, GLuint >( buffer[ l * 4 + 3 ], buffer[ l * 4 + 1 ] );
		}
		
		qSort( hitList.begin(), hitList.end(), compareHits );
		
		return hitList.value( cycle % hits ).first;
	}
	
	return -1;
}

QModelIndex GLView::indexAt( const QPoint & pos, int cycle )
{
	if ( ! ( model && isVisible() && height() ) )
		return QModelIndex();
	
	makeCurrent();

	glProjection( pressPos.x(), pressPos.y() );

	GLuint	buffer[512];
	glSelectBuffer( 512, buffer );

	int choose;
	if ( GLOptions::drawFurn() )
	{		
		choose = ::indexAt( buffer, model, scene, QList<DrawFunc>() << &Scene::drawFurn, cycle ); 
		if ( choose != -1 )
		{
			QModelIndex parent = model->index( 3, 0, model->getBlock( choose&0x0ffff ) );
			return model->index( choose>>16, 0, parent );
		}
	}
	
	QList<DrawFunc> df;
	
	if ( GLOptions::drawHavok() )
		df << &Scene::drawHavok;
	if ( GLOptions::drawNodes() )
		df << &Scene::drawNodes;
	df << &Scene::drawShapes;
	
	choose = ::indexAt( buffer, model, scene, df, cycle ); 
	if ( choose != -1 )
		return model->getBlock( choose );
	
	return QModelIndex();
}

void GLView::resizeGL(int width, int height)
{
	glViewport(0, 0, width, height );
}


/*
 *  NifModel stuff
 */
 
void GLView::setNif( NifModel * nif )
{
	if ( model )
	{
		disconnect( model );
	}
	model = nif;
	if ( model )
	{
		connect( model, SIGNAL( dataChanged( const QModelIndex &, const QModelIndex & ) ), this, SLOT( dataChanged( const QModelIndex &, const QModelIndex & ) ) );
		connect( model, SIGNAL( linksChanged() ), this, SLOT( modelLinked() ) );
		connect( model, SIGNAL( modelReset() ), this, SLOT( modelChanged() ) );
		connect( model, SIGNAL( destroyed() ), this, SLOT( modelDestroyed() ) );
	}
	doCompile = true;
}

void GLView::setCurrentIndex( const QModelIndex & index )
{
	if ( ! ( model && index.model() == model ) )
		return;
	
	scene->currentBlock = model->getBlock( index );
	scene->currentIndex = index.sibling( index.row(), 0 );
	
	update();
}

QModelIndex parent( QModelIndex ix, QModelIndex xi )
{
	ix = ix.sibling( ix.row(), 0 );
	xi = xi.sibling( xi.row(), 0 );
	
	while ( ix.isValid() )
	{
		QModelIndex x = xi;
		while ( x.isValid() )
		{
			if ( ix == x ) return ix;
			x = x.parent();
		}
		ix = ix.parent();
	}
	return QModelIndex();
}

void GLView::dataChanged( const QModelIndex & idx, const QModelIndex & xdi )
{
	if ( doCompile )
		return;
	
	QModelIndex ix = idx;
	
	if ( idx == xdi )
	{
		if ( idx.column() != 0 )
			ix = idx.sibling( idx.row(), 0 );
	}
	else
	{
		ix = ::parent( idx, xdi );
	}
	
	if ( ix.isValid() )
	{
		scene->update( model, idx );
		update();
	}
	else
		modelChanged();
}

void GLView::modelChanged()
{
	doCompile = true;
	doCenter = true;
	update();
}

void GLView::modelLinked()
{
	doCompile = true; //scene->update( model, QModelIndex() );
	update();
}

void GLView::modelDestroyed()
{
	setNif( 0 );
}


void GLView::sltTime( float t )
{
	time = t;
	update();
}

void GLView::sltSequence( const QString & seqname )
{
	animGroups->setCurrentIndex( scene->animGroups.indexOf( seqname ) );
	scene->setSequence( seqname );
	time = scene->timeMin();
	emit sigTime( time, scene->timeMin(), scene->timeMax() );
	update();
}

void GLView::setTextureFolder( const QString & tf )
{
	TexCache::texfolders = tf.split( ";" );
	doCompile = true;
	update();
}

QString GLView::textureFolder() const
{
	return TexCache::texfolders.join( ";" );
}



void GLView::viewAction( QAction * act )
{
	BoundSphere bs = scene->bounds();
	if ( GLOptions::drawAxes() )
		bs |= BoundSphere( Vector3(), axis );
	
	if ( bs.radius < 1 ) bs.radius = 1;
	setDistance( bs.radius );
	setZoom( 1.0 );
	
	if ( act == aViewWalk )
	{
		setRotation( -90, 0, 0 );
		setPosition( Vector3() - scene->bounds().center );
		setZoom( 1.0 );
		aViewWalk->setChecked( true );
		aViewTop->setChecked( false );
		aViewFront->setChecked( false );
		aViewSide->setChecked( false );
	}

	if ( ! act || act == aViewFlip )
	{
		act = checkedViewAction();
	}
	
	if ( act != aViewWalk )
		setPosition( Vector3() - bs.center );
	
	if ( act == aViewTop )
	{
		if ( aViewFlip->isChecked() )
			setRotation( 180, 0, 0 );
		else
			setRotation( 0, 0, 0 );
		aViewWalk->setChecked( false );
		aViewTop->setChecked( true );
		aViewFront->setChecked( false );
		aViewSide->setChecked( false );
	}
	else if ( act == aViewFront )
	{
		if ( aViewFlip->isChecked() )
			setRotation( -90, 0, 180 );
		else
			setRotation( -90, 0, 0 );
		aViewWalk->setChecked( false );
		aViewTop->setChecked( false );
		aViewFront->setChecked( true );
		aViewSide->setChecked( false );
	}
	else if ( act == aViewSide )
	{
		if ( aViewFlip->isChecked() )
			setRotation( - 90, 0, - 90 );
		else
			setRotation( - 90, 0, 90 );
		aViewWalk->setChecked( false );
		aViewTop->setChecked( false );
		aViewFront->setChecked( false );
		aViewSide->setChecked( true );
	}
	update();
}

QAction * GLView::checkedViewAction() const
{
	if ( aViewTop->isChecked() )
		return aViewTop;
	else if ( aViewFront->isChecked() )
		return aViewFront;
	else if ( aViewSide->isChecked() )
		return aViewSide;
	else if ( aViewWalk->isChecked() )
		return aViewWalk;
	else
		return 0;
}

void GLView::uncheckViewAction()
{
	QAction * act = checkedViewAction();
	if ( act && act != aViewWalk )
		act->setChecked( false );
}

void GLView::advanceGears()
{
	QTime t = QTime::currentTime();
	float dT = lastTime.msecsTo( t ) / 1000.0;
	
	if ( GLOptions::benchmark() )
	{
		fpsacc += dT;
		fpscnt++;
		update();
	}
	
	if ( dT < 0 )	dT = 0;
	if ( dT > 1.0 )	dT = 1.0;
	lastTime = t;
	
	if ( ! isVisible() )
		return;
	
	if ( aAnimate->isChecked() && aAnimPlay->isChecked() )
	{
		time += dT;
		if ( time > scene->timeMax() )
		{
			if ( aAnimSwitch->isChecked() && ! scene->animGroups.isEmpty() )
			{
				int ix = scene->animGroups.indexOf( scene->animGroup );
				if ( ++ix >= scene->animGroups.count() )
					ix -= scene->animGroups.count();
				sltSequence( scene->animGroups.value( ix ) );
			}
			else if ( aAnimLoop->isChecked() )
			{
				time = scene->timeMin();
			}
		}
		
		emit sigTime( time, scene->timeMin(), scene->timeMax() );
		update();
	}
	
	if ( kbd[ Qt::Key_Up ] )		rotate( - ROT_SPD * dT, 0, 0 );
	if ( kbd[ Qt::Key_Down ] )		rotate( + ROT_SPD * dT, 0, 0 );
	if ( kbd[ Qt::Key_Left ] )		rotate( 0, 0, - ROT_SPD * dT );
	if ( kbd[ Qt::Key_Right ] )	rotate( 0, 0, + ROT_SPD * dT );
	if ( kbd[ Qt::Key_A ] )		move( + MOV_SPD * dT, 0, 0 );
	if ( kbd[ Qt::Key_D ] )		move( - MOV_SPD * dT, 0, 0 );
	if ( kbd[ Qt::Key_W ] )		move( 0, 0, + MOV_SPD * dT );
	if ( kbd[ Qt::Key_S ] )		move( 0, 0, - MOV_SPD * dT );
	if ( kbd[ Qt::Key_F ] )		move( 0, + MOV_SPD * dT, 0 );
	if ( kbd[ Qt::Key_R ] )		move( 0, - MOV_SPD * dT, 0 );
	if ( kbd[ Qt::Key_Q ] )		setDistance( Dist * 0.9 );
	if ( kbd[ Qt::Key_E ] )		setDistance( Dist * 1.1 );
	if ( kbd[ Qt::Key_PageUp ] )	zoom( 1.1 );
	if ( kbd[ Qt::Key_PageDown ] )	zoom( 0.9 );
	
	if ( mouseMov[0] != 0 || mouseMov[1] != 0 || mouseMov[2] != 0 )
	{
		move( mouseMov[0], mouseMov[1], mouseMov[2] );
		mouseMov = Vector3();
	}
	
	if ( mouseRot[0] != 0 || mouseRot[1] != 0 || mouseRot[2] != 0 )
	{
		rotate( mouseRot[0], mouseRot[1], mouseRot[2] );
		mouseRot = Vector3();
	}
}

void GLView::keyPressEvent( QKeyEvent * event )
{
	switch ( event->key() )
	{
		case Qt::Key_Up:
		case Qt::Key_Down:
		case Qt::Key_Left:
		case Qt::Key_Right:
		case Qt::Key_PageUp:
		case Qt::Key_PageDown:
		case Qt::Key_A:
		case Qt::Key_D:
		case Qt::Key_W:
		case Qt::Key_S:
		case Qt::Key_R:
		case Qt::Key_F:
		case Qt::Key_Q:
		case Qt::Key_E:
			kbd[ event->key() ] = true;
			break;
		case Qt::Key_Escape:
			doCompile = true;
			if ( ! aViewWalk->isChecked() )
				doCenter = true;
			update();
			break;
		default:
			event->ignore();
			break;
	}
}

void GLView::keyReleaseEvent( QKeyEvent * event )
{
	switch ( event->key() )
	{
		case Qt::Key_Up:
		case Qt::Key_Down:
		case Qt::Key_Left:
		case Qt::Key_Right:
		case Qt::Key_PageUp:
		case Qt::Key_PageDown:
		case Qt::Key_A:
		case Qt::Key_D:
		case Qt::Key_W:
		case Qt::Key_S:
		case Qt::Key_R:
		case Qt::Key_F:
		case Qt::Key_Q:
		case Qt::Key_E:
			kbd[ event->key() ] = false;
			break;
		default:
			event->ignore();
			break;
	}
}

void GLView::focusOutEvent( QFocusEvent * )
{
	kbd.clear();
}

void GLView::mousePressEvent(QMouseEvent *event)
{
	lastPos = event->pos();
	
	if ( ( pressPos - event->pos() ).manhattanLength() <= 3 )
		cycleSelect++;
	else
		cycleSelect = 0;
	
	pressPos = event->pos();
}

void GLView::mouseReleaseEvent( QMouseEvent *event )
{
	if ( ! ( model && ( pressPos - event->pos() ).manhattanLength() <= 3 ) )
		return;
	
	QModelIndex idx = indexAt( event->pos(), cycleSelect );
	scene->currentBlock = model->getBlock( idx );
	scene->currentIndex = idx.sibling( idx.row(), 0 );
	if ( idx.isValid() )
	{
		emit clicked( idx );
	}
	update();
}

void GLView::mouseMoveEvent(QMouseEvent *event)
{
	int dx = event->x() - lastPos.x();
	int dy = event->y() - lastPos.y();

	if (event->buttons() & Qt::LeftButton)
	{
		mouseRot += Vector3( dy * .5, 0, dx * .5 );
	}
	else if ( event->buttons() & Qt::MidButton)
	{
		float d = axis / ( qMax( width(), height() ) + 1 );
		mouseMov += Vector3( dx * d, - dy * d, 0 );
	}
	lastPos = event->pos();
}

void GLView::mouseDoubleClickEvent( QMouseEvent * )
{
	/*
	doCompile = true;
	if ( ! aViewWalk->isChecked() )
		doCenter = true;
	update();
	*/
}

void GLView::wheelEvent( QWheelEvent * event )
{
	if ( aViewWalk->isChecked() )
		mouseMov += Vector3( 0, 0, event->delta() );
	else
		setDistance( Dist * ( event->delta() < 0 ? 1.2 : 0.8 ) );
}

void GLView::checkActions()
{
	scene->animate = aAnimate->isChecked();
	
	lastTime = QTime::currentTime();
	
	if ( GLOptions::benchmark() )
		timer->setInterval( 0 );
	else
		timer->setInterval( 1000 / FPS );
	
	update();
}

void GLView::save( QSettings & settings )
{
	//settings.beginGroup( "OpenGL" );
	settings.setValue( "texture folder", TexCache::texfolders.join( ";" ) );
	settings.setValue( "enable animations", aAnimate->isChecked() );
	settings.setValue( "play animation", aAnimPlay->isChecked() );
	settings.setValue( "loop animation", aAnimLoop->isChecked() );
	settings.setValue( "switch animation", aAnimSwitch->isChecked() );
	settings.setValue( "perspective", aViewPerspective->isChecked() );
	//settings.endGroup();
}

void GLView::restore( QSettings & settings )
{
	//settings.beginGroup( "OpenGL" );
	TexCache::texfolders = settings.value( "texture folder" ).toString().split( ";" );
	aAnimate->setChecked( settings.value( "enable animations", true ).toBool() );
	aAnimPlay->setChecked( settings.value( "play animation", true ).toBool() );
	aAnimLoop->setChecked( settings.value( "loop animation", true ).toBool() );
	aAnimSwitch->setChecked( settings.value( "switch animation", true ).toBool() );
	aViewPerspective->setChecked( settings.value( "perspective", true ).toBool() );
	
	checkActions();
	//settings.endGroup();
}

void GLView::move( float x, float y, float z )
{
	Pos += Matrix::euler( Rot[0] / 180 * PI, Rot[1] / 180 * PI, Rot[2] / 180 * PI ).inverted() * Vector3( x, y, z );
	update();
}

void GLView::rotate( float x, float y, float z )
{
	Rot += Vector3( x, y, z );
	uncheckViewAction();
	update();
}

void GLView::zoom( float z )
{
	Zoom *= z;
	if ( Zoom < ZOOM_MIN ) Zoom = ZOOM_MIN;
	if ( Zoom > ZOOM_MAX ) Zoom = ZOOM_MAX;
	update();
}

void GLView::setPosition( float x, float y, float z )
{
	Pos = Vector3( x, y, z );
	update();
}

void GLView::setPosition( Vector3 v )
{
	Pos = v;
	update();
}

void GLView::setRotation( float x, float y, float z )
{
	Rot = Vector3( x, y, z );
	update();
}

void GLView::setZoom( float z )
{
	Zoom = z;
	update();
}

void GLView::setDistance( float x )
{
	Dist = x;
	update();
}
