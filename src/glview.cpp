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

#include "glview.h"
#include "config.h"
#include "options.h"

#include "ui_nifskope.h"
#include "nifskope.h"
#include "nifmodel.h"
#include "gl/glscene.h"
#include "gl/gltex.h"
#include "widgets/fileselect.h"
#include "widgets/floatedit.h"
#include "widgets/floatslider.h"

#include <QActionGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDebug>
#include <QDialog>
#include <QDir>
#include <QLabel>
#include <QKeyEvent>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QPushButton>
#include <QRadioButton>
#include <QSettings>
#include <QSpinBox>
#include <QTimer>
#include <QToolBar>

#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLFramebufferObject>
#include <QGLFormat>

// TODO: Determine the necessity of this
// Appears to be used solely for gluErrorString
// There may be some Qt alternative
#ifdef __APPLE__
	#include <OpenGL/glu.h>
#else
	#include <GL/glu.h>
#endif


// NOTE: The FPS define is a frame limiter,
//	NOT the guaranteed FPS in the viewport.
//	Also the QTimer is integer milliseconds 
//	so 60 will give you 1000/60 = 16, not 16.666
//	therefore it's really 62.5FPS
#define FPS 60


#define FOV 45.0
#define MOV_SPD 350
#define ROT_SPD 45
#define ZOOM_MIN 1.0
#define ZOOM_MAX 1000.0

#ifndef M_PI
#define M_PI 3.1415926535897932385
#endif


//! \file glview.cpp GLView implementation

GLView * GLView::create( NifSkope * window )
{
	static QList<QPointer<GLView> > views;

	QGLWidget * share = nullptr;
	for ( const QPointer<GLView>& v : views ) {
		if ( v )
			share = v;
	}

	QGLFormat fmt;
	fmt.setDoubleBuffer( true );
	fmt.setRgba( true );
	fmt.setSamples( Options::antialias() ? 16 : 0 );

	// All new windows after the first window will share a format
	if ( share ) {
		fmt = share->format();
	} else {
		fmt.setSampleBuffers( Options::antialias() );
	}
		
	// OpenGL version
	fmt.setVersion( 2, 1 );
	// Ignored if version < 3.2
	//fmt.setProfile(QGLFormat::CoreProfile);

	// V-Sync
	fmt.setSwapInterval( 1 );

	views.append( QPointer<GLView>( new GLView( fmt, window, share ) ) );

	return views.last();
}

GLView::GLView( const QGLFormat & format, QWidget * p, const QGLWidget * shareWidget )
	: QGLWidget( format, p, shareWidget )
{
	setFocusPolicy( Qt::ClickFocus );
	setAttribute( Qt::WA_PaintOnScreen );
	setAttribute( Qt::WA_NoSystemBackground );
	setAutoFillBackground( false );
	setAcceptDrops( true );
	setContextMenuPolicy( Qt::CustomContextMenu );

	// Make the context current on this window
	makeCurrent();

	// Create an OpenGL context
	glContext = context()->contextHandle();

	// Obtain a functions object and resolve all entry points
	glFuncs = glContext->functions();

	if ( !glFuncs ) {
		qWarning( "Could not obtain OpenGL functions" );
		exit( 1 );
	}

	glFuncs->initializeOpenGLFunctions();

	view = ViewDefault;
	animState = AnimEnabled;
	debugMode = DbgNone;

	Zoom = 1.0;

	doCenter  = false;
	doCompile = false;

	model = nullptr;

	time = 0.0;
	lastTime = QTime::currentTime();

	textures = new TexCache( this );

	scene = new Scene( textures, glContext, glFuncs );
	connect( textures, &TexCache::sigRefresh, this, static_cast<void (GLView::*)()>(&GLView::update) );
	connect( scene, &Scene::sceneUpdated, this, static_cast<void (GLView::*)()>(&GLView::update) );

	timer = new QTimer( this );
	timer->setInterval( 1000 / FPS );
	timer->start();
	connect( timer, &QTimer::timeout, this, &GLView::advanceGears );

	lightVisTimeout = 1500;
	lightVisTimer = new QTimer( this );
	lightVisTimer->setSingleShot( true );
	connect( lightVisTimer, &QTimer::timeout, [this]() { unsetVisMode( VisLightPos ); update(); } );

	connect( Options::get(), &Options::sigFlush3D, textures, &TexCache::flush );
	connect( Options::get(), &Options::sigChanged, this, static_cast<void (GLView::*)()>(&GLView::update) );
	connect( Options::get(), &Options::materialOverridesChanged, this, &GLView::sceneUpdate );
}

GLView::~GLView()
{
	delete scene;
}


/* 
 * Scene
 */

Scene * GLView::getScene()
{
	return scene;
}

void GLView::sceneUpdate()
{
	scene->update( model, QModelIndex() );
	update();
}

void GLView::updateAnimationState()
{
	QAction * action = qobject_cast<QAction *>(sender());
	if ( action ) {
		auto opt = AnimationState( action->data().toInt() );

		if ( action->isChecked() )
			animState |= opt;
		else
			animState &= ~opt;

		scene->animate = (animState & AnimEnabled);
		lastTime = QTime::currentTime();

		update();
	}
}


/*
 *  OpenGL
 */

void GLView::initializeGL()
{
	GLenum err;
	
	if ( scene->options & Scene::DoMultisampling ) {
		if ( !glContext->hasExtension( "GL_EXT_framebuffer_multisample" ) ) {
			scene->options &= ~Scene::DoMultisampling;
			//qWarning() << "System does not support multisampling";
		} /* else {
			GLint maxSamples;
			glGetIntegerv( GL_MAX_SAMPLES, &maxSamples );
			qDebug() << "Max samples:" << maxSamples;
		}*/
	}

	initializeTextureUnits( glContext );

	if ( scene->renderer->initialize() )
		updateShaders();

	// Initial viewport values
	//	Made viewport and aspect member variables.
	//	They were being updated every single frame instead of only when resizing.
	//glGetIntegerv( GL_VIEWPORT, viewport );
	aspect = (GLdouble)width() / (GLdouble)height();

	// Check for errors
	while ( ( err = glGetError() ) != GL_NO_ERROR )
		qDebug() << tr( "glview.cpp - GL ERROR (init) : " ) << (const char *)gluErrorString( err );
}

void GLView::updateShaders()
{
	makeCurrent();
	scene->updateShaders();
	update();
}

void GLView::glProjection( int x, int y )
{
	Q_UNUSED( x ); Q_UNUSED( y );

	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();

	BoundSphere bs = scene->view * scene->bounds();

	if ( scene->options & Scene::ShowAxes ) {
		bs |= BoundSphere( scene->view * Vector3(), axis );

		// Include grid in bounds
		bs.radius = (bs.radius > 1024.0f) ? bs.radius : 1024.0f;
	}

	GLdouble nr = fabs( bs.center[2] ) - bs.radius * 1.2;
	GLdouble fr = fabs( bs.center[2] ) + bs.radius * 1.2;

	if ( perspectiveMode || (view == ViewWalk) ) {
		// Perspective View
		if ( nr < 1.0 )
			nr = 1.0;
		if ( fr < 2.0 )
			fr = 2.0;

		if ( nr > fr ) {
			// add: swap them when needed
			GLfloat tmp = nr;
			nr = fr;
			fr = tmp;
		}

		if ( (fr - nr) < 0.00001f ) {
			// add: ensure distance
			nr = 1.0;
			fr = 2.0;
		}

		GLdouble h2 = tan( ( FOV / Zoom ) / 360 * M_PI ) * nr;
		GLdouble w2 = h2 * aspect;
		glFrustum( -w2, +w2, -h2, +h2, nr, fr );
	} else {
		// Orthographic View
		GLdouble h2 = Dist / Zoom;
		GLdouble w2 = h2 * aspect;
		glOrtho( -w2, +w2, -h2, +h2, nr, fr );
	}

	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();
}


// TODO: Temp materials
static GLfloat mat_half[] = { 0.5f, 0.5f, 0.5f, 1.0f };
static GLfloat mat_full[] = { 1.0f, 1.0f, 1.0f, 1.0f };
static GLfloat mat_specular[] = { 0, 0, 0, 1.0f };
static GLfloat mat_shininess[] = { 0 };

#ifdef USE_GL_QPAINTER
void GLView::paintEvent( QPaintEvent * event )
{
	makeCurrent();

	QPainter painter;
	painter.begin( this );
	painter.setRenderHint( QPainter::TextAntialiasing );
#else
void GLView::paintGL()
{
#endif
	// Save GL state
	glPushAttrib( GL_ALL_ATTRIB_BITS );
	glMatrixMode( GL_PROJECTION );
	glPushMatrix();
	glMatrixMode( GL_MODELVIEW );
	glPushMatrix();

	// Clear Viewport
	//glViewport( 0, 0, width(), height() );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
	
	
	// Compile the model
	if ( doCompile ) {
		textures->setNifFolder( model->getFolder() );
		scene->make( model );
		scene->transform( Transform(), scene->timeMin() );
		axis = (scene->bounds().radius <= 0) ? 1 : scene->bounds().radius * 1.0; // fix: the axis appearance when there is no scene yet

		if ( scene->timeMin() != scene->timeMax() ) {
			if ( time < scene->timeMin() || time > scene->timeMax() )
				time = scene->timeMin();

			emit sequencesUpdated();

		} else if ( scene->timeMax() == 0 ) {
			// No Animations in this NIF
			emit sequencesDisabled( true );
		}
		emit sigTime( time, scene->timeMin(), scene->timeMax() );
		doCompile = false;
	}

	// Center the model
	if ( doCenter ) {
		setCenter();
		doCenter = false;
	}

	// Transform the scene
	Matrix ap;

	// TODO: Redo for new Settings class
	//if ( Options::upAxis() == Options::YAxis ) {
	//	ap( 0, 0 ) = 0; ap( 0, 1 ) = 0; ap( 0, 2 ) = 1;
	//	ap( 1, 0 ) = 1; ap( 1, 1 ) = 0; ap( 1, 2 ) = 0;
	//	ap( 2, 0 ) = 0; ap( 2, 1 ) = 1; ap( 2, 2 ) = 0;
	//} else if ( Options::upAxis() == Options::XAxis ) {
	//	ap( 0, 0 ) = 0; ap( 0, 1 ) = 1; ap( 0, 2 ) = 0;
	//	ap( 1, 0 ) = 0; ap( 1, 1 ) = 0; ap( 1, 2 ) = 1;
	//	ap( 2, 0 ) = 1; ap( 2, 1 ) = 0; ap( 2, 2 ) = 0;
	//}

	Transform viewTrans;
	viewTrans.rotation.fromEuler( Rot[0] / 180.0 * PI, Rot[1] / 180.0 * PI, Rot[2] / 180.0 * PI );
	viewTrans.rotation = viewTrans.rotation * ap;
	viewTrans.translation = viewTrans.rotation * Pos;

	if ( view != ViewWalk )
		viewTrans.translation[2] -= Dist * 2;

	scene->transform( viewTrans, time );

	// Setup projection mode
	glProjection();
	glLoadIdentity();

	// Draw the axes and grid
	if ( scene->options & Scene::ShowAxes ) {
		glDisable( GL_ALPHA_TEST );
		glDisable( GL_BLEND );
		glDisable( GL_LIGHTING );
		glDisable( GL_COLOR_MATERIAL );
		glEnable( GL_DEPTH_TEST );
		glDepthMask( GL_TRUE );
		glDepthFunc( GL_LESS );
		glDisable( GL_TEXTURE_2D );
		glDisable( GL_NORMALIZE );
		glLineWidth( 2.0f );

		glPushMatrix();
		glLoadMatrix( viewTrans );

		drawAxes( Vector3(), axis );

		// TODO: Configurable grid in Settings
		// 1024 game units, major lines every 128, minor lines every 64
		drawGrid( 1024, 128, 2 );

		// Debug scene bounds
#ifndef QT_NO_DEBUG
		if ( debugMode == DbgBounds ) {
			BoundSphere bs = scene->bounds();
			bs |= BoundSphere( Vector3(), axis );
			drawSphere( bs.center, bs.radius );
		}
#endif
		glPopMatrix();
	}


	if ( scene->options & Scene::DoLighting ) {
		// Setup light
		Vector4 lightDir( 0.0, 0.0, 1.0, 0.0 );

		if ( !frontalLight ) {
			float decl = declination / 180.0 * PI;
			Vector3 v( sin( decl ), 0, cos( decl ) );
			Matrix m; m.fromEuler( 0, 0, planarAngle / 180.0 * PI );
			v = m * v;
			lightDir = Vector4( viewTrans.rotation * v, 0.0 );

			glEnable( GL_BLEND );
			glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
			glEnable( GL_DEPTH_TEST );
			glDepthMask( GL_TRUE );
			glDepthFunc( GL_LESS );
			glLineWidth( 2.0f );
			glColor4f( 1.0f, 1.0f, 1.0f, 0.5f );


			if ( visMode & VisLightPos ) {
				glPushMatrix();
				glLoadMatrix( viewTrans );

				// Scale the distance a bit
				float l = axis + 64.0;
				l = (l < 128) ? axis * 1.5 : l;
				l = (l > 2048) ? axis * 0.66 : l;
				l = (l > 1024) ? axis * 0.75 : l;

				drawDashLine( Vector3( 0, 0, 0 ), v * l, 30 );
				drawSphere( v * l, axis / 10 );
				glPopMatrix();
				glDisable( GL_BLEND );
			}
		}

		glShadeModel( GL_SMOOTH );
		glEnable( GL_LIGHTING );
		glEnable( GL_LIGHT0 );
		glLightfv( GL_LIGHT0, GL_AMBIENT, mat_half );
		glLightfv( GL_LIGHT0, GL_DIFFUSE, mat_full );
		glLightfv( GL_LIGHT0, GL_SPECULAR, mat_specular );
		glLightfv( GL_LIGHT0, GL_POSITION, lightDir.data() );

		// Necessary?
		glLightModeli( GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE );
	} else {
		float a = 0.5f; // Ambient brightness
		float d = 0.5f; // Diffuse brightness
		if ( scene->options & Scene::UseTextures ) {
			a = 1.0f;
			// Blow the vertex colors out until I can fix the shader
			//	This gives me a very high ColorD value in the shaders
			//	to compare to, so that I can turn off normals when ColorD.r > 1.0
			d = 1000.0f;
		} else if ( scene->options & Scene::ShowVertexColors ) {
			a = 0.0f;
			d = 0.1f;
		}

		GLfloat mat_diff[] = { d, d, d, 1.0f };
		GLfloat mat_amb[] = { a, a, a, 1.0f };

		glShadeModel( GL_SMOOTH );
		glEnable( GL_LIGHTING );
		glEnable( GL_LIGHT0 );
		glLightModelfv( GL_LIGHT_MODEL_AMBIENT, mat_full );
		glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, mat_full );
		//glMaterialfv( GL_FRONT_AND_BACK, GL_EMISSION, mat_full );
		glLightfv( GL_LIGHT0, GL_AMBIENT, mat_amb );
		glLightfv( GL_LIGHT0, GL_DIFFUSE, mat_diff );
		glLightfv( GL_LIGHT0, GL_SPECULAR, mat_specular );
		
		//glMaterialfv( GL_FRONT, GL_AMBIENT, mat_full );
		//glMaterialfv( GL_FRONT, GL_DIFFUSE, mat_amb );
		//glMaterialfv( GL_FRONT, GL_SPECULAR, mat_specular );
	}

	if ( scene->options & Scene::DoMultisampling )
		glEnable( GL_MULTISAMPLE_ARB );

#ifndef QT_NO_DEBUG
	// Color Key debug
	if ( debugMode == DbgColorPicker ) {
		glDisable( GL_MULTISAMPLE );
		glDisable( GL_LINE_SMOOTH );
		glDisable( GL_TEXTURE_2D );
		glDisable( GL_BLEND );
		glDisable( GL_DITHER );
		glDisable( GL_LIGHTING );
		glShadeModel( GL_FLAT );
		glDisable( GL_FOG );
		glDisable( GL_MULTISAMPLE_ARB );
		glEnable( GL_DEPTH_TEST );
		glDepthFunc( GL_LEQUAL );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		Node::SELECTING = 1;
	} else {
		Node::SELECTING = 0;
	}
#endif

	// Draw the model
	scene->draw();

	// Restore GL state
	glPopAttrib();
	glMatrixMode( GL_MODELVIEW );
	glPopMatrix();
	glMatrixMode( GL_PROJECTION );
	glPopMatrix();

	// Check for errors
	GLenum err;
	while ( ( err = glGetError() ) != GL_NO_ERROR )
		qDebug() << tr( "glview.cpp - GL ERROR (paint): " ) << (const char *)gluErrorString( err );

	emit paintUpdate();


#ifdef USE_GL_QPAINTER
	painter.end();
#endif
}


void GLView::resizeGL( int width, int height )
{
	makeCurrent();
	aspect = (GLdouble)width / (GLdouble)height;
	glViewport( 0, 0, width, height );
	qglClearColor( Options::bgColor() );

	update();
}

void GLView::resizeEvent( QResizeEvent * e )
{
	// This function should never be called.
	// Moved to NifSkope::eventFilter()
}

void GLView::flagsChanged()
{
	sceneUpdate();
	update();
}

void GLView::frontalLightToggled( bool frontal )
{
	frontalLight = frontal;
	update();
}

void GLView::declinationChanged( int decl )
{
	declination = float( decl / 2 );
	lightVisTimer->start( lightVisTimeout );
	setVisMode( VisLightPos );
	update();
}

void GLView::planarAngleChanged( int angle )
{
	planarAngle = float( angle / 2 );
	lightVisTimer->start( lightVisTimeout );
	setVisMode( VisLightPos );
	update();
}

void GLView::setDebugMode( DebugMode mode )
{
	debugMode = mode;
}

void GLView::setVisMode( VisMode mode )
{
	visMode |= mode;
}

void GLView::unsetVisMode( VisMode mode )
{
	visMode &= ~mode;
}


typedef void (Scene::* DrawFunc)( void );

int indexAt( /*GLuint *buffer,*/ NifModel * model, Scene * scene, QList<DrawFunc> drawFunc, int cycle, const QPoint & pos, int & furn )
{
	Q_UNUSED( model ); Q_UNUSED( cycle );
	// Color Key O(1) selection
	//	Open GL 3.0 says glRenderMode is deprecated
	//	ATI OpenGL API implementation of GL_SELECT corrupts NifSkope memory
	//
	// Create FBO for sharp edges and no shading.
	// Texturing, blending, dithering, lighting and smooth shading should be disabled.
	// The FBO can be used for the drawing operations to keep the drawing operations invisible to the user.

	GLint viewport[4];
	glGetIntegerv( GL_VIEWPORT, viewport );

	// Create new FBO with multisampling disabled
	QOpenGLFramebufferObjectFormat fboFmt;
	fboFmt.setTextureTarget( GL_TEXTURE_2D );
	fboFmt.setInternalTextureFormat( GL_RGB32F_ARB );
	fboFmt.setAttachment( QOpenGLFramebufferObject::Attachment::CombinedDepthStencil );

	QOpenGLFramebufferObject fbo( viewport[2], viewport[3], fboFmt );
	fbo.bind();

	glEnable( GL_LIGHTING );
	glDisable( GL_MULTISAMPLE );
	glDisable( GL_MULTISAMPLE_ARB );
	glDisable( GL_LINE_SMOOTH );
	glDisable( GL_POINT_SMOOTH );
	glDisable( GL_POLYGON_SMOOTH );
	glDisable( GL_TEXTURE_1D );
	glDisable( GL_TEXTURE_2D );
	glDisable( GL_TEXTURE_3D );
	glDisable( GL_BLEND );
	glDisable( GL_DITHER );
	glDisable( GL_FOG );
	glDisable( GL_LIGHTING );
	glShadeModel( GL_FLAT );
	glEnable( GL_DEPTH_TEST );
	glDepthFunc( GL_LEQUAL );
	glClearColor( 0, 0, 0, 1 );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	// Rasterize the scene
	Node::SELECTING = 1;
	for ( DrawFunc df : drawFunc ) {
		(scene->*df)();
	}
	Node::SELECTING = 0;

	fbo.release();

	QImage img( fbo.toImage() );
	QColor pixel = img.pixel( pos );

#ifndef QT_NO_DEBUG
	img.save( "fbo.png" );
#endif

	// Encode RGB to Int
	int a = 0;
	a |= pixel.red()   << 0;
	a |= pixel.green() << 8;
	a |= pixel.blue()  << 16;

	// Decode:
	// R = (id & 0x000000FF) >> 0
	// G = (id & 0x0000FF00) >> 8
	// B = (id & 0x00FF0000) >> 16

	int choose = COLORKEY2ID( a );

	// Pick BSFurnitureMarker
	if ( choose > 0 ) {
		auto furnBlock = model->getBlock( model->index( 3, 0, model->getBlock( choose & 0x0ffff ) ), "BSFurnitureMarker" );

		if ( furnBlock.isValid() ) {
			furn = choose >> 16;
			choose &= 0x0ffff;
		}
	}

	//qDebug() << "Key:" << a << " R" << pixel.red() << " G" << pixel.green() << " B" << pixel.blue();
	return choose;
}

QModelIndex GLView::indexAt( const QPoint & pos, int cycle )
{
	if ( !(model && isVisible() && height()) )
		return QModelIndex();

	makeCurrent();

	glPushAttrib( GL_ALL_ATTRIB_BITS );
	glMatrixMode( GL_PROJECTION );
	glPushMatrix();
	glMatrixMode( GL_MODELVIEW );
	glPushMatrix();

	glViewport( 0, 0, width(), height() );
	glProjection( pos.x(), pos.y() );

	QList<DrawFunc> df;

	if ( scene->options & Scene::ShowCollision )
		df << &Scene::drawHavok;

	if ( scene->options & Scene::ShowNodes )
		df << &Scene::drawNodes;

	if ( scene->options & Scene::ShowMarkers )
		df << &Scene::drawFurn;

	df << &Scene::drawShapes;

	int choose = -1, furn = -1;
	choose = ::indexAt( model, scene, df, cycle, pos, /*out*/ furn );

	glPopAttrib();
	glMatrixMode( GL_MODELVIEW );
	glPopMatrix();
	glMatrixMode( GL_PROJECTION );
	glPopMatrix();

	QModelIndex chooseIndex;

	if ( choose != -1 ) {
		// Block Index
		chooseIndex = model->getBlock( choose );

		if ( furn != -1 ) {
			// Furniture Row @ Block Index
			chooseIndex = model->index( furn, 0, model->index( 3, 0, chooseIndex ) );
		}			
	}

	return chooseIndex;
}

void GLView::center()
{
	doCenter = true;
	update();
}

void GLView::move( float x, float y, float z )
{
	Pos += Matrix::euler( Rot[0] / 180 * PI, Rot[1] / 180 * PI, Rot[2] / 180 * PI ).inverted() * Vector3( x, y, z );
	updateViewpoint();
	update();
}

void GLView::rotate( float x, float y, float z )
{
	Rot += Vector3( x, y, z );
	updateViewpoint();
	update();
}

void GLView::zoom( float z )
{
	Zoom *= z;

	if ( Zoom < ZOOM_MIN )
		Zoom = ZOOM_MIN;

	if ( Zoom > ZOOM_MAX )
		Zoom = ZOOM_MAX;

	update();
}

void GLView::setCenter()
{
	qDebug() << "Setting center";

	Node * node = scene->getNode( model, scene->currentBlock );

	if ( node != 0 ) {
		BoundSphere bs = node->bounds();

		this->setPosition( -bs.center );

		if ( bs.radius > 0 ) {
			setDistance( bs.radius * 1.5f );
		}
	} else {
		BoundSphere bs = scene->bounds();

		//if ( scene->options & Scene::ShowAxes )
		//	bs |= BoundSphere( Vector3(), axis );

		if ( bs.radius < 1 )
			bs.radius = 1;

		setDistance( bs.radius );
		setZoom( 1.0 );

		setPosition( Vector3() - bs.center );

		setOrientation( view );
	}

}

void GLView::setDistance( float x )
{
	Dist = x;
	update();
}

void GLView::setPosition( float x, float y, float z )
{
	Pos = { x, y, z };
	update();
}

void GLView::setPosition( Vector3 v )
{
	Pos = v;
	update();
}

void GLView::setProjection( bool isPersp )
{
	perspectiveMode = isPersp;
	update();
}

void GLView::setRotation( float x, float y, float z )
{
	Rot = { x, y, z };
	update();
}

void GLView::setZoom( float z )
{
	Zoom = z;
	update();
}


void GLView::flipOrientation()
{
	ViewState tmp = ViewDefault;

	switch ( view ) {
	case ViewTop:
		tmp = ViewBottom;
		break;
	case ViewBottom:
		tmp = ViewTop;
		break;
	case ViewLeft:
		tmp = ViewRight;
		break;
	case ViewRight:
		tmp = ViewLeft;
		break;
	case ViewFront:
		tmp = ViewBack;
		break;
	case ViewBack:
		tmp = ViewFront;
		break;
	case ViewUser:
	default:
	{
		// TODO: Flip any other view also?
	}
		break;
	}

	setOrientation( tmp, false );
}

void GLView::setOrientation( GLView::ViewState state, bool recenter )
{
	if ( state == view )
		return;

	switch ( state ) {
	case ViewBottom:
		setRotation( 180, 0, 0 ); // Bottom
		break;
	case ViewTop:
		setRotation( 0, 0, 0 ); // Top
		break;
	case ViewBack:
		setRotation( -90, 0, 0 ); // Back
		break;
	case ViewFront:
		setRotation( -90, 0, 180 ); // Front
		break;
	case ViewRight:
		setRotation( -90, 0, 90 ); // Right
		break;
	case ViewLeft:
		setRotation( -90, 0, -90 ); // Left
		break;
	default:
		break;
	}

	view = state;

	// Recenter
	if ( recenter )
		center();
}

void GLView::updateViewpoint()
{
	switch ( view ) {
	case ViewTop:
	case ViewBottom:
	case ViewLeft:
	case ViewRight:
	case ViewFront:
	case ViewBack:
	case ViewUser:
		emit viewpointChanged();
		break;
	default:
		break;
	}
}


/*
 *  NifModel
 */

void GLView::setNif( NifModel * nif )
{
	if ( model ) {
		// disconnect( model ) may not work with new Qt5 syntax...
		// it says the calls need to remain symmetric to the connect() ones.
		// Otherwise, use QMetaObject::Connection
		disconnect( model );
	}

	model = nif;

	if ( model ) {
		connect( model, &NifModel::dataChanged, this, &GLView::dataChanged );
		connect( model, &NifModel::linksChanged, this, &GLView::modelLinked );
		connect( model, &NifModel::modelReset, this, &GLView::modelChanged );
		connect( model, &NifModel::destroyed, this, &GLView::modelDestroyed );
	}

	doCompile = true;
}

void GLView::setCurrentIndex( const QModelIndex & index )
{
	if ( !( model && index.model() == model ) )
		return;

	scene->currentBlock = model->getBlock( index );
	scene->currentIndex = index.sibling( index.row(), 0 );

	update();
}

QModelIndex parent( QModelIndex ix, QModelIndex xi )
{
	ix = ix.sibling( ix.row(), 0 );
	xi = xi.sibling( xi.row(), 0 );

	while ( ix.isValid() ) {
		QModelIndex x = xi;

		while ( x.isValid() ) {
			if ( ix == x )
				return ix;

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

	if ( idx == xdi ) {
		if ( idx.column() != 0 )
			ix = idx.sibling( idx.row(), 0 );
	} else {
		ix = ::parent( idx, xdi );
	}

	if ( ix.isValid() ) {
		scene->update( model, idx );
		update();
	} else {
		modelChanged();
	}
}

void GLView::modelChanged()
{
	doCompile = true;
	doCenter  = true;
	update();
}

void GLView::modelLinked()
{
	doCompile = true; //scene->update( model, QModelIndex() );
	update();
}

void GLView::modelDestroyed()
{
	setNif( nullptr );
}


/*
 * UI
 */

void GLView::sltTime( float t )
{
	time = t;
	update();
	emit sigTime( time, scene->timeMin(), scene->timeMax() );
}

void GLView::sltSequence( const QString & seqname )
{
	// Update UI
	QAction * action = qobject_cast<QAction *>(sender());
	if ( !action ) {
		// Called from self and not UI
		emit sequenceChanged( seqname );
	}
	
	scene->setSequence( seqname );
	time = scene->timeMin();
	emit sigTime( time, scene->timeMin(), scene->timeMax() );
	update();
}

// TODO: Multiple user views, ala Recent Files
void GLView::saveUserView()
{
	QSettings cfg;
	cfg.beginGroup( "GLView" );
	cfg.beginGroup( "User View" );
	cfg.setValue( "RotX", Rot[0] );
	cfg.setValue( "RotY", Rot[1] );
	cfg.setValue( "RotZ", Rot[2] );
	cfg.setValue( "PosX", Pos[0] );
	cfg.setValue( "PosY", Pos[1] );
	cfg.setValue( "PosZ", Pos[2] );
	cfg.setValue( "Dist", Dist );
	cfg.endGroup();
	cfg.endGroup();
}

void GLView::loadUserView()
{
	QSettings cfg;
	cfg.beginGroup( "GLView" );
	cfg.beginGroup( "User View" );
	setRotation( cfg.value( "RotX" ).toDouble(), cfg.value( "RotY" ).toDouble(), cfg.value( "RotZ" ).toDouble() );
	setPosition( cfg.value( "PosX" ).toDouble(), cfg.value( "PosY" ).toDouble(), cfg.value( "PosZ" ).toDouble() );
	setDistance( cfg.value( "Dist" ).toDouble() );
	cfg.endGroup();
	cfg.endGroup();
}

void GLView::advanceGears()
{
	QTime t  = QTime::currentTime();
	float dT = lastTime.msecsTo( t ) / 1000.0;
	dT = (dT < 0) ? 0 : ((dT > 1.0) ? 1.0 : dT);

	lastTime = t;

	if ( !isVisible() )
		return;

	if ( ( animState & AnimEnabled ) && ( animState & AnimPlay )
		&& scene->timeMin() != scene->timeMax() )
	{
		time += dT;

		if ( time > scene->timeMax() ) {
			if ( ( animState & AnimSwitch ) && !scene->animGroups.isEmpty() ) {
				int ix = scene->animGroups.indexOf( scene->animGroup );
	
				if ( ++ix >= scene->animGroups.count() )
					ix -= scene->animGroups.count();
	
				sltSequence( scene->animGroups.value( ix ) );
			} else if ( animState & AnimLoop ) {
				time = scene->timeMin();
			} else {
				// Animation has completed and is not looping
				//	or cycling through animations.
				// Reset time and state and then inform UI it has stopped.
				time = scene->timeMin();
				animState &= ~AnimPlay;
				emit sequenceStopped();
			}
		} else {
			// Animation is not done yet
		}

		emit sigTime( time, scene->timeMin(), scene->timeMax() );
		update();
	}

	// TODO: Some kind of input class for choosing the appropriate
	// keys based on user preferences of what app they would like to
	// emulate for the control scheme
	// Rotation
	if ( kbd[ Qt::Key_Up ] )    rotate( -ROT_SPD * dT, 0, 0 );
	if ( kbd[ Qt::Key_Down ] )  rotate( +ROT_SPD * dT, 0, 0 );
	if ( kbd[ Qt::Key_Left ] )  rotate( 0, 0, -ROT_SPD * dT );
	if ( kbd[ Qt::Key_Right ] ) rotate( 0, 0, +ROT_SPD * dT );

	// Movement
	if ( kbd[ Qt::Key_A ] ) move( +MOV_SPD * dT, 0, 0 );
	if ( kbd[ Qt::Key_D ] ) move( -MOV_SPD * dT, 0, 0 );
	if ( kbd[ Qt::Key_W ] ) move( 0, 0, +MOV_SPD * dT );
	if ( kbd[ Qt::Key_S ] ) move( 0, 0, -MOV_SPD * dT );
	//if ( kbd[ Qt::Key_F ] ) move( 0, +MOV_SPD * dT, 0 );
	//if ( kbd[ Qt::Key_R ] ) move( 0, -MOV_SPD * dT, 0 );

	// Zoom
	if ( kbd[ Qt::Key_Q ] ) setDistance( Dist * 1.0 / 1.1 );
	if ( kbd[ Qt::Key_E ] ) setDistance( Dist * 1.1 );

	// Focal Length
	if ( kbd[ Qt::Key_PageUp ] )   zoom( 1.1f );
	if ( kbd[ Qt::Key_PageDown ] ) zoom( 1 / 1.1f );

	if ( mouseMov[0] != 0 || mouseMov[1] != 0 || mouseMov[2] != 0 ) {
		move( mouseMov[0], mouseMov[1], mouseMov[2] );
		mouseMov = Vector3();
	}

	if ( mouseRot[0] != 0 || mouseRot[1] != 0 || mouseRot[2] != 0 ) {
		rotate( mouseRot[0], mouseRot[1], mouseRot[2] );
		mouseRot = Vector3();
	}
}


// TODO: Separate widget
void GLView::saveImage()
{
	QDialog dlg;
	QGridLayout * lay = new QGridLayout;
	dlg.setWindowTitle( tr( "Save View" ) );
	dlg.setLayout( lay );
	dlg.setMinimumWidth( 400 );

	QString date = QDateTime::currentDateTime().toString( "yyyyMMdd_HH-mm-ss" );
	QString name = model->getFilename();

	QString nifFolder = model->getFolder();
	QString filename = name + (!name.isEmpty() ? "_" : "") + date + ".jpg";

	// Default: NifSkope directory
	// TODO: User-configurable default screenshot path in Options
	QString nifskopePath = "screenshots/" + filename;
	// Absolute: NIF directory
	QString nifPath = nifFolder + (!nifFolder.isEmpty() ? "/" : "") + filename;

	FileSelector * file = new FileSelector( FileSelector::SaveFile, tr( "File" ), QBoxLayout::LeftToRight );
	file->setFilter( { "Images (*.jpg *.png *.bmp)", "JPEG (*.jpg)", "PNG (*.png)", "BMP (*.bmp)" } );
	file->setFile( nifskopePath );
	lay->addWidget( file, 0, 0, 1, -1 );
	
	QRadioButton * nifskopeDir = new QRadioButton( tr( "NifSkope Directory" ), this );
	nifskopeDir->setChecked( true );
	nifskopeDir->setToolTip( tr( "Save to NifSkope screenshots directory" ) );

	QRadioButton * niffileDir = new QRadioButton( tr( "NIF Directory" ), this );
	niffileDir->setChecked( false );
	niffileDir->setDisabled( nifFolder.isEmpty() );
	niffileDir->setToolTip( tr( "Save to NIF file directory" ) );

	lay->addWidget( nifskopeDir, 1, 0, 1, 1 );
	lay->addWidget( niffileDir, 1, 1, 1, 1 );

	// Save JPEG Quality
	QSettings cfg;
	int jpegQuality = cfg.value( "JPEG/Quality", 91 ).toInt();
	cfg.setValue( "JPEG/Quality", jpegQuality );

	QHBoxLayout * pixBox = new QHBoxLayout;
	pixBox->setAlignment( Qt::AlignRight );
	QSpinBox * pixQuality = new QSpinBox;
	pixQuality->setRange( -1, 100 );
	pixQuality->setSingleStep( 10 );
	pixQuality->setValue( jpegQuality );
	pixQuality->setSpecialValueText( tr( "Auto" ) );
	pixQuality->setMaximumWidth( pixQuality->minimumSizeHint().width() );
	pixBox->addWidget( new QLabel( tr( "JPEG Quality" ) ) );
	pixBox->addWidget( pixQuality );
	lay->addLayout( pixBox, 1, 2, Qt::AlignRight );

	QHBoxLayout * hBox  = new QHBoxLayout;
	QPushButton * btnOk = new QPushButton( tr( "Save" ) );
	QPushButton * btnCancel = new QPushButton( tr( "Cancel" ) );
	hBox->addWidget( btnOk );
	hBox->addWidget( btnCancel );
	lay->addLayout( hBox, 2, 0, 1, -1 );

	// Set FileSelector to NifSkope dir (relative)
	connect( nifskopeDir, &QRadioButton::clicked, [&]()
		{
			file->setText( nifskopePath );
			file->setFile( nifskopePath );
		}
	);
	// Set FileSelector to NIF File dir (absolute)
	connect( niffileDir, &QRadioButton::clicked, [&]()
		{
			file->setText( nifPath );
			file->setFile( nifPath );
		}
	);

	// Validate on OK
	connect( btnOk, &QPushButton::clicked, [&]() 
		{
			// Save JPEG Quality
			cfg.setValue( "JPEG/Quality", pixQuality->value() );

			// TODO: Set up creation of screenshots directory in Options
			if ( nifskopeDir->isChecked() ) {
				QDir workingDir;
				workingDir.mkpath( "screenshots" );
			}

			QImage img = grabFrameBuffer();
			if ( img.save( file->file(), 0, pixQuality->value() ) ) {
				dlg.accept();
			} else {
				qWarning() << "Could not save to file. Please check the filepath and extension are valid.";
			}
		}
	);
	connect( btnCancel, &QPushButton::clicked, &dlg, &QDialog::reject );

	if ( dlg.exec() != QDialog::Accepted )
		return;
}


/* 
 * QWidget Event Handlers 
 */

void GLView::dragEnterEvent( QDragEnterEvent * e )
{
	if ( e->mimeData()->hasUrls() && e->mimeData()->urls().count() == 1 ) {
		QUrl url = e->mimeData()->urls().first();

		if ( url.scheme() == "file" ) {
			QString fn = url.toLocalFile();

			if ( textures->canLoad( fn ) ) {
				fnDragTex = textures->stripPath( fn, model->getFolder() );
				e->accept();
				return;
			}
		}
	}

	e->ignore();
}

void GLView::dragLeaveEvent( QDragLeaveEvent * e )
{
	Q_UNUSED( e );

	if ( iDragTarget.isValid() ) {
		model->set<QString>( iDragTarget, fnDragTexOrg );
		iDragTarget = QModelIndex();
		fnDragTex = fnDragTexOrg = QString();
	}
}

void GLView::dragMoveEvent( QDragMoveEvent * e )
{
	if ( iDragTarget.isValid() ) {
		model->set<QString>( iDragTarget, fnDragTexOrg );
		iDragTarget  = QModelIndex();
		fnDragTexOrg = QString();
	}

	QModelIndex iObj = model->getBlock( indexAt( e->pos() ), "NiAVObject" );

	if ( iObj.isValid() ) {
		for ( const auto l : model->getChildLinks( model->getBlockNumber( iObj ) ) ) {
			QModelIndex iTxt = model->getBlock( l, "NiTexturingProperty" );

			if ( iTxt.isValid() ) {
				QModelIndex iSrc = model->getBlock( model->getLink( iTxt, "Base Texture/Source" ), "NiSourceTexture" );

				if ( iSrc.isValid() ) {
					iDragTarget = model->getIndex( iSrc, "File Name" );

					if ( iDragTarget.isValid() ) {
						fnDragTexOrg = model->get<QString>( iDragTarget );
						model->set<QString>( iDragTarget, fnDragTex );
						e->accept();
						return;
					}
				}
			}
		}
	}

	e->ignore();
}

void GLView::dropEvent( QDropEvent * e )
{
	iDragTarget = QModelIndex();
	fnDragTex = fnDragTexOrg = QString();
	e->accept();
}

void GLView::focusOutEvent( QFocusEvent * )
{
	kbd.clear();
}

void GLView::keyPressEvent( QKeyEvent * event )
{
	switch ( event->key() ) {
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
	//case Qt::Key_R:
	//case Qt::Key_F:
	case Qt::Key_Q:
	case Qt::Key_E:
		kbd[event->key()] = true;
		break;
	case Qt::Key_Escape:
		doCompile = true;

		if ( view == ViewWalk )
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
	switch ( event->key() ) {
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
	//case Qt::Key_R:
	//case Qt::Key_F:
	case Qt::Key_Q:
	case Qt::Key_E:
		kbd[event->key()] = false;
		break;
	default:
		event->ignore();
		break;
	}
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

void GLView::mouseMoveEvent( QMouseEvent * event )
{
	int dx = event->x() - lastPos.x();
	int dy = event->y() - lastPos.y();

	if ( event->buttons() & Qt::LeftButton ) {
		mouseRot += Vector3( dy * .5, 0, dx * .5 );
	} else if ( event->buttons() & Qt::MidButton ) {
		float d = axis / (qMax( width(), height() ) + 1);
		mouseMov += Vector3( dx * d, -dy * d, 0 );
	} else if ( event->buttons() & Qt::RightButton ) {
		setDistance( Dist - (dx + dy) * (axis / (qMax( width(), height() ) + 1)) );
	}

	lastPos = event->pos();
}

void GLView::mousePressEvent( QMouseEvent * event )
{
	lastPos = event->pos();

	if ( (pressPos - event->pos()).manhattanLength() <= 3 )
		cycleSelect++;
	else
		cycleSelect = 0;

	pressPos = event->pos();
}

void GLView::mouseReleaseEvent( QMouseEvent * event )
{
	if ( !(model && (pressPos - event->pos()).manhattanLength() <= 3) )
		return;

	QModelIndex idx = indexAt( event->pos(), cycleSelect );
	scene->currentBlock = model->getBlock( idx );
	scene->currentIndex = idx.sibling( idx.row(), 0 );

	if ( idx.isValid() ) {
		emit clicked( idx );
	}

	update();
}

void GLView::wheelEvent( QWheelEvent * event )
{
	if ( view == ViewWalk )
		mouseMov += Vector3( 0, 0, event->delta() );
	else
		setDistance( Dist * (event->delta() < 0 ? 1.0 / 0.8 : 0.8) );
}
