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

#ifdef QT_OPENGL_LIB

#include "glview.h"

#include <QtOpenGL>
#include <QTimer>

#include <math.h>

#include "nifmodel.h"
#include "glscene.h"
#include "popup.h"

#define FPS 35

GLView::GLView()
	: QGLWidget()
{
	setFocusPolicy( Qt::ClickFocus );
	
	xRot = yRot = zRot = 0;
	zoom = 1.0;
	zInc = 1;
	
	doCenter = false;
	doCompile = false;
	
	model = 0;
	
	time = 0.0;
	lastTime = QTime::currentTime();
	
	scene = new Scene();

	timer = new QTimer(this);
	connect( timer, SIGNAL( timeout() ), this, SLOT( advanceGears() ) );
	
	aTexturing = new QAction( "texturing", this );
	aTexturing->setToolTip( "enable texturing" );
	aTexturing->setCheckable( true );
	aTexturing->setChecked( true );
	connect( aTexturing, SIGNAL( toggled( bool ) ), this, SLOT( checkActions() ) );
	addAction( aTexturing );
	
	aBlending = new QAction( "blending", this );
	aBlending->setToolTip( "enable alpha blending" );
	aBlending->setCheckable( true );
	aBlending->setChecked( true );
	connect( aBlending, SIGNAL( toggled( bool ) ), this, SLOT( checkActions() ) );
	addAction( aBlending );
	
	aLighting = new QAction( "lighting", this );
	aLighting->setToolTip( "enable lighting" );
	aLighting->setCheckable( true );
	aLighting->setChecked( true );
	connect( aLighting, SIGNAL( toggled( bool ) ), this, SLOT( checkActions() ) );
	addAction( aLighting );
	
	aDrawAxis = new QAction( "draw axis", this );
	aDrawAxis->setToolTip( "draw xyz-Axis" );
	aDrawAxis->setCheckable( true );
	aDrawAxis->setChecked( true );
	connect( aDrawAxis, SIGNAL( toggled( bool ) ), this, SLOT( checkActions() ) );
	addAction( aDrawAxis );
	
	aDrawNodes = new QAction( "draw nodes", this );
	aDrawNodes->setToolTip( "draw bones/nodes" );
	aDrawNodes->setCheckable( true );
	aDrawNodes->setChecked( true );
	connect( aDrawNodes, SIGNAL( toggled( bool ) ), this, SLOT( checkActions() ) );
	addAction( aDrawNodes );
	
	aDrawHidden = new QAction( "show hidden", this );
	aDrawHidden->setToolTip( "if checked nodes and meshes are allways displayed<br>wether they are hidden ( flags & 1 ) or not" );
	aDrawHidden->setCheckable( true );
	aDrawHidden->setChecked( false );
	connect( aDrawHidden, SIGNAL( toggled( bool ) ), this, SLOT( checkActions() ) );
	addAction( aDrawHidden );
	
	aHighlight = new QAction( "highlight selected", this );
	aHighlight->setToolTip( "highlight selected meshes and nodes" );
	aHighlight->setCheckable( true );
	aHighlight->setChecked( true );
	connect( aHighlight, SIGNAL( toggled( bool ) ), this, SLOT( checkActions() ) );
	addAction( aHighlight );
	
	aRotate = new QAction( "rotate", this );
	aRotate->setToolTip( "slowly rotate the object around the z axis" );
	aRotate->setCheckable( true );
	aRotate->setChecked( true );
	connect( aRotate, SIGNAL( toggled( bool ) ), this, SLOT( checkActions() ) );
	addAction( aRotate );
	
	aAnimate = new QAction( "animation", this );
	aAnimate->setToolTip( "enables evaluation of animation controllers" );
	aAnimate->setCheckable( true );
	aAnimate->setChecked( true );
	connect( aAnimate, SIGNAL( toggled( bool ) ), this, SLOT( checkActions() ) );
	addAction( aAnimate );

	aAnimPlay = new QAction( "play", this );
	aAnimPlay->setCheckable( true );
	aAnimPlay->setChecked( true );
	connect( aAnimPlay, SIGNAL( toggled( bool ) ), this, SLOT( checkActions() ) );

	aTexFolder = new QAction( "set texture folder", this );
	aTexFolder->setToolTip( "tell me where your textures are" );
	connect( aTexFolder, SIGNAL( triggered() ), this, SLOT( selectTexFolder() ) );
	addAction( aTexFolder );
}

GLView::~GLView()
{
	makeCurrent();
	delete scene;
}

void GLView::initializeGL()
{
	GLTex::initialize( context() );
	
	qglClearColor( palette().color( QPalette::Active, QPalette::Background ) );

	static const GLfloat L0position[4] = { 5.0f, 5.0f, 10.0f, 1.0f };
	static const GLfloat L0ambient[4] = { 0.4f, 0.4f, 0.4f, 1.0f };
	static const GLfloat L0diffuse[4] = { 0.8f, 0.8f, 0.8f, 1.0f };
	static const GLfloat L0specular[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glLightfv( GL_LIGHT0, GL_POSITION, L0position );
	glLightfv( GL_LIGHT0, GL_AMBIENT, L0ambient );
	glLightfv( GL_LIGHT0, GL_DIFFUSE, L0diffuse );
	glLightfv( GL_LIGHT0, GL_SPECULAR, L0specular );
	glEnable( GL_LIGHT0 );
	
	glShadeModel( GL_SMOOTH );
	glEnable( GL_POINT_SMOOTH );
	glEnable( GL_LINE_SMOOTH );

	// check for errors
	
	GLenum err;
	while ( ( err = glGetError() ) != GL_NO_ERROR )
		qDebug() << "GL ERROR (init) : " << (const char *) gluErrorString( err );
}

void GLView::glPerspective( int x, int y )
{
	GLdouble r = qMax( scene->boundRadius[0], qMax( scene->boundRadius[1], scene->boundRadius[2] ) );
	if ( r > radius )
		radius = r;

	GLint	viewport[4];
	glGetIntegerv( GL_VIEWPORT, viewport );
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if ( x >= 0 && y >= 0 )
		gluPickMatrix( (GLdouble) x, (GLdouble) (viewport[3]-y), 5.0f, 5.0f, viewport);
	gluPerspective( 45.0 / zoom, (GLdouble) viewport[2] / (GLdouble) viewport[3], 1.0 * radius, 8.0 * radius );
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef( xTrans / 20.0, yTrans / 20.0, -4.0 * radius );
}

void GLView::glOrtho()
{
	GLint	viewport[4];
	glGetIntegerv( GL_VIEWPORT, viewport );
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	::glOrtho (0.0, viewport[2], 0.0, viewport[3], -1.0, 1.0);
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
	if ( ! ( isVisible() && height() ) )	return;
	
	glEnable( GL_DEPTH_TEST );
	glDepthMask( GL_TRUE );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	
	// compile the model
	
	if ( doCompile )
	{
		scene->make( model );
		if ( time < scene->timeMin || time > scene->timeMax )
		{
			time = scene->timeMin;
			emit sigFrame( (int) ( time * FPS ), (int) ( scene->timeMin * FPS ), (int) ( scene->timeMax * FPS ) );
		}
		doCompile = false;
	}
	
	// center the model
	
	if ( doCenter )
	{
		xTrans = 0;
		yTrans = - scene->boundCenter[1];
		
		xRot = - 90*16;
		yRot = 0;
		
		zoom = 1.0;
		
		doCenter = false;
		
		radius = 0.0;
	}
	
	// transform the scene
	
	Transform viewTrans;
	viewTrans.rotation.fromEuler( xRot / 16.0 / 180 * PI, yRot / 16.0 / 180 * PI, zRot / 16.0 / 180 * PI );
	
	scene->transform( viewTrans, time );
	
	// setup projection mode

	glPerspective();
	
	// draw the axis
	
	if ( aDrawAxis->isChecked() )
	{
		glDisable( GL_BLEND );
		glDisable( GL_LIGHTING );
		glDisable( GL_COLOR_MATERIAL );
		glEnable( GL_DEPTH_TEST );
		glDepthMask( GL_TRUE );
		glDepthFunc( GL_ALWAYS );
		glDisable( GL_TEXTURE_2D );
		glDisable( GL_NORMALIZE );
		glLineWidth( 1.2 );
		
		glPushMatrix();
		viewTrans.glMultMatrix();
		
		GLfloat axis = qMax( scene->boundRadius[0], qMax( scene->boundRadius[1], scene->boundRadius[2] ) ) * 2.1;
		GLfloat arrow = axis / 10.0;
		glBegin( GL_LINES );
		glColor3f( 1.0, 0.0, 0.0 );
		glVertex3f( - axis, 0, 0 );
		glVertex3f( + axis, 0, 0 );
		glVertex3f( + axis, 0, 0 );
		glVertex3f( + axis - arrow, + arrow, 0 );
		glVertex3f( + axis, 0, 0 );
		glVertex3f( + axis - arrow, - arrow, 0 );
		glVertex3f( + axis, 0, 0 );
		glVertex3f( + axis - arrow, 0, + arrow );
		glVertex3f( + axis, 0, 0 );
		glVertex3f( + axis - arrow, 0, - arrow );
		glColor3f( 0.0, 1.0, 0.0 );
		glVertex3f( 0, - axis, 0 );
		glVertex3f( 0, + axis, 0 );
		glVertex3f( 0, + axis, 0 );
		glVertex3f( + arrow, + axis - arrow, 0 );
		glVertex3f( 0, + axis, 0 );
		glVertex3f( - arrow, + axis - arrow, 0 );
		glVertex3f( 0, + axis, 0 );
		glVertex3f( 0, + axis - arrow, + arrow );
		glVertex3f( 0, + axis, 0 );
		glVertex3f( 0, + axis - arrow, - arrow );
		glColor3f( 0.0, 0.0, 1.0 );
		glVertex3f( 0, 0, - axis );
		glVertex3f( 0, 0, + axis );
		glVertex3f( 0, 0, + axis );
		glVertex3f( 0, + arrow, + axis - arrow );
		glVertex3f( 0, 0, + axis );
		glVertex3f( 0, - arrow, + axis - arrow );
		glVertex3f( 0, 0, + axis );
		glVertex3f( + arrow, 0, + axis - arrow );
		glVertex3f( 0, 0, + axis );
		glVertex3f( - arrow, 0, + axis - arrow );
		glEnd();
		glPopMatrix();
		/*
		glColor3f( 1.0, 0.0, 1.0 );
		glBegin( GL_LINE_STRIP );
		glVertex3f( scene->boundMin[0], scene->boundMin[1], scene->boundMin[2] );
		glVertex3f( scene->boundMin[0], scene->boundMax[1], scene->boundMin[2] );
		glVertex3f( scene->boundMin[0], scene->boundMax[1], scene->boundMax[2] );
		glVertex3f( scene->boundMin[0], scene->boundMin[1], scene->boundMax[2] );
		glVertex3f( scene->boundMin[0], scene->boundMin[1], scene->boundMin[2] );
		glEnd();
		glBegin( GL_LINE_STRIP );
		glVertex3f( scene->boundMax[0], scene->boundMin[1], scene->boundMin[2] );
		glVertex3f( scene->boundMax[0], scene->boundMax[1], scene->boundMin[2] );
		glVertex3f( scene->boundMax[0], scene->boundMax[1], scene->boundMax[2] );
		glVertex3f( scene->boundMax[0], scene->boundMin[1], scene->boundMax[2] );
		glVertex3f( scene->boundMax[0], scene->boundMin[1], scene->boundMin[2] );
		glEnd();
		glBegin( GL_LINES );
		glVertex3f( scene->boundMin[0], scene->boundMin[1], scene->boundMin[2] );
		glVertex3f( scene->boundMax[0], scene->boundMin[1], scene->boundMin[2] );
		glVertex3f( scene->boundMin[0], scene->boundMax[1], scene->boundMin[2] );
		glVertex3f( scene->boundMax[0], scene->boundMax[1], scene->boundMin[2] );
		glVertex3f( scene->boundMin[0], scene->boundMax[1], scene->boundMax[2] );
		glVertex3f( scene->boundMax[0], scene->boundMax[1], scene->boundMax[2] );
		glVertex3f( scene->boundMin[0], scene->boundMin[1], scene->boundMax[2] );
		glVertex3f( scene->boundMax[0], scene->boundMin[1], scene->boundMax[2] );
		glEnd();
		*/
	}
	
	// draw the model

	if ( aLighting->isChecked() )
		glEnable( GL_LIGHTING );
	else
		glDisable( GL_LIGHTING );
	
	scene->draw();
	
	// check for errors
	
	GLenum err;
	while ( ( err = glGetError() ) != GL_NO_ERROR )
		qDebug() << "GL ERROR (paint): " << (const char *) gluErrorString( err );
}

int GLView::pickGL( int x, int y )
{
	if ( ! ( isVisible() && height() ) )
		return -1;
	
	makeCurrent();

	GLuint	buffer[512];
	glSelectBuffer( 512, buffer );

	glRenderMode( GL_SELECT );	
	glInitNames();
	glPushName( 0 );
	
	glPerspective( pressPos.x(), pressPos.y() );

	scene->draw();
	
	GLint hits = glRenderMode( GL_RENDER );
	if ( hits > 0 )
	{
		int	choose = buffer[3];
		int	depth = buffer[1];
		for (int loop = 1; loop < hits; loop++)
		{
			if (buffer[loop*4+1] < GLuint(depth))
			{
				choose = buffer[loop*4+3];
				depth = buffer[loop*4+1];
			}       
		}
		return choose;
	}
	return -1;
}

void GLView::resizeGL(int width, int height)
{
	glViewport(0, 0, width, height );
}

void GLView::setNif( NifModel * nif )
{
	scene->clear();
	
	if ( model )
	{
		disconnect( model, SIGNAL( dataChanged( const QModelIndex &, const QModelIndex & ) ), this, SLOT( dataChanged( const QModelIndex &, const QModelIndex & ) ) );
		disconnect( model, SIGNAL( linksChanged() ), this, SLOT( modelChanged() ) );
		disconnect( model, SIGNAL( modelReset() ), this, SLOT( modelChanged() ) );
		disconnect( model, SIGNAL( destroyed() ), this, SLOT( modelDestroyed() ) );
	}
	model = nif;
	if ( model )
	{
		connect( model, SIGNAL( dataChanged( const QModelIndex &, const QModelIndex & ) ), this, SLOT( dataChanged( const QModelIndex &, const QModelIndex & ) ) );
		connect( model, SIGNAL( linksChanged() ), this, SLOT( modelChanged() ) );
		connect( model, SIGNAL( modelReset() ), this, SLOT( modelChanged() ) );
		connect( model, SIGNAL( destroyed() ), this, SLOT( modelDestroyed() ) );
	}
	doCompile = true;
}

void GLView::setCurrentIndex( const QModelIndex & index )
{
	if ( ! ( model && index.model() == model ) )
		return;
	
	scene->currentNode = model->getBlockNumber( index );
	update();
}

void GLView::sltFrame( int f )
{
	time = (float) f / FPS;
	update();
}

void GLView::selectTexFolder()
{
	//QString tf = QFileDialog::getExistingDirectory( this, "select texture folder", textureFolder() );
	setTextureFolder( selectMultipleDirs( "select texture folders", scene->texfolders, this ).join( ";" ) );
}

void GLView::setTextureFolder( const QString & tf )
{
	scene->texfolders = tf.split( ";" );
	doCompile = true;
	update();
}

QString GLView::textureFolder() const
{
	return scene->texfolders.join( ";" );
}

void GLView::save( QSettings & settings )
{
	//settings.beginGroup( "OpenGL" );
	settings.setValue( "texture folder", scene->texfolders.join( ";" ) );
	settings.setValue( "enable textures", aTexturing->isChecked() );
	settings.setValue( "enable lighting", aLighting->isChecked() );
	settings.setValue( "enable blending", aBlending->isChecked() );
	settings.setValue( "highlight meshes", aHighlight->isChecked() );
	settings.setValue( "draw axis", aDrawAxis->isChecked() );
	settings.setValue( "draw nodes", aDrawNodes->isChecked() );
	settings.setValue( "draw hidden", aDrawHidden->isChecked() );
	settings.setValue( "rotate", aRotate->isChecked() );
	settings.setValue( "enable animations", aAnimate->isChecked() );
	settings.setValue( "play animation", aAnimPlay->isChecked() );
	//settings.endGroup();
}

void GLView::restore( QSettings & settings )
{
	//settings.beginGroup( "OpenGL" );
	scene->texfolders = settings.value( "texture folder" ).toString().split( ";" );
	aTexturing->setChecked( settings.value( "enable textures", true ).toBool() );
	aLighting->setChecked( settings.value( "enable lighting", true ).toBool() );
	aBlending->setChecked( settings.value( "enable blending", true ).toBool() );
	aHighlight->setChecked( settings.value( "highlight meshes", true ).toBool() );
	aDrawAxis->setChecked( settings.value( "draw axis", true ).toBool() );
	aDrawNodes->setChecked( settings.value( "draw nodes", true ).toBool() );
	aDrawHidden->setChecked( settings.value( "draw hidden", true ).toBool() );
	aRotate->setChecked( settings.value( "rotate", true ).toBool() );
	aAnimate->setChecked( settings.value( "enable animations", true ).toBool() );
	aAnimPlay->setChecked( settings.value( "play animation", true ).toBool() );
	checkActions();
	//settings.endGroup();
}

void GLView::checkActions()
{
	scene->texturing = aTexturing->isChecked();
	scene->blending = aBlending->isChecked();
	scene->highlight = aHighlight->isChecked();
	scene->drawNodes = aDrawNodes->isChecked();
	scene->drawHidden = aDrawHidden->isChecked();
	scene->animate = aAnimate->isChecked();
	lastTime = QTime::currentTime();
	if ( aRotate->isChecked() || ( aAnimate->isChecked() && aAnimPlay->isChecked() ) )
	{
		timer->start( 1000 / FPS );
	}
	else
	{
		timer->stop();
		update();
	}
}

void GLView::advanceGears()
{
	QTime t = QTime::currentTime();
	
	if ( aAnimate->isChecked() && aAnimPlay->isChecked() )
	{
		time += fabs( t.msecsTo( lastTime ) / 1000.0 );

		if ( time > scene->timeMax )
			time = scene->timeMin;
		
		emit sigFrame( (int) ( time * FPS ), (int) ( scene->timeMin * FPS ), (int) ( scene->timeMax * FPS ) );
	}
	
	lastTime = t;
	
	if ( aRotate->isChecked() )
	{
		zRot += zInc;
		normalizeAngle( &zRot );
	}
	update();
}

void GLView::dataChanged( const QModelIndex & idx, const QModelIndex & xdi )
{
	if ( doCompile )
		return;
	
	if ( idx != xdi )
		modelChanged();
	else
		scene->update( model, idx );
}

void GLView::modelChanged()
{
	qDebug( "model make triggered" );
	doCompile = true;
	update();
}

void GLView::modelDestroyed()
{
	qDebug( "destroyed" );
	setNif( 0 );
}

void GLView::normalizeAngle(int *angle)
{
	while (*angle < 0)
		*angle += 360 * 16;
	while (*angle > 360 * 16)
		*angle -= 360 * 16;
}

void GLView::mousePressEvent(QMouseEvent *event)
{
	lastPos = event->pos();
	pressPos = event->pos();
}

void GLView::mouseReleaseEvent( QMouseEvent *event )
{
	if ( ! ( isVisible() && height() && model && ( pressPos - event->pos() ).manhattanLength() <= 3 ) )
		return;
	
	int pick = pickGL( event->x(), event->y() );
	if ( pick >= 0 )
	{
		emit clicked( model->getBlock( pick ) );
		scene->currentNode = pick;
		update();
	}
}

void GLView::mouseMoveEvent(QMouseEvent *event)
{
	int dx = event->x() - lastPos.x();
	int dy = event->y() - lastPos.y();

	if (event->buttons() & Qt::LeftButton)
	{
		xRot += 8 * dy;
		yRot += 8 * dx;
		normalizeAngle( &xRot );
		normalizeAngle( &yRot );
		update();
	}
	else if (event->buttons() & Qt::RightButton)
	{
		zRot += 8 * dx;
		normalizeAngle( &zRot );
		update();
	}
	else if ( event->buttons() & Qt::MidButton)
	{
		xTrans += dx * 5;
		yTrans += dy * 5;
		update();
	}
	lastPos = event->pos();
}

void GLView::mouseDoubleClickEvent( QMouseEvent * )
{
	doCompile = true;
	update();
}

void GLView::wheelEvent( QWheelEvent * event )
{
	zoom = zoom * ( event->delta() > 0 ? 0.9 : 1.1 );
	if ( zoom < 0.25 ) zoom = 0.25;
	if ( zoom > 1000 ) zoom = 1000;
	update();
}

QList<QAction*> GLView::animActions() const
{
	QList<QAction*> actions;
	actions << aAnimPlay;
	return actions;
}

#endif
