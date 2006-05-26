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
#include <QTimer>


#include <math.h>

#include "nifmodel.h"
#include "gl/glscene.h"
#include "gl/gltex.h"

#include "spellbook.h"
#include "widgets/colorwheel.h"

#define FPS 25

#define FOV 45.0

#define MOV_SPD 350
#define ROT_SPD 45

#define ZOOM_MIN 1.0
#define ZOOM_MAX 1000.0

GLView::GLView()
	: QGLWidget()
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
	
	aViewPerspective = new QAction( "Perspective", grpView );
	aViewPerspective->setToolTip( "Perspective View Transformation or Orthogonal View Transformation" );
	aViewPerspective->setCheckable( true );
	aViewPerspective->setShortcut( Qt::Key_F9 );
	grpView->addAction( aViewPerspective );
	
	
	aShading = new QAction( "&Enable Shaders", this );
	aShading->setToolTip( "enable shading" );
	aShading->setCheckable( true );
	aShading->setChecked( true );
	aShading->setShortcut( Qt::Key_F12 );
	connect( aShading, SIGNAL( toggled( bool ) ), this, SLOT( updateShaders() ) );
	addAction( aShading );
	
	aTexturing = new QAction( "&Texturing", this );
	aTexturing->setToolTip( "enable texturing" );
	aTexturing->setCheckable( true );
	aTexturing->setChecked( true );
	connect( aTexturing, SIGNAL( toggled( bool ) ), this, SLOT( checkActions() ) );
	addAction( aTexturing );
	
	aBlending = new QAction( "&Blending", this );
	aBlending->setToolTip( "enable alpha blending" );
	aBlending->setCheckable( true );
	aBlending->setChecked( true );
	connect( aBlending, SIGNAL( toggled( bool ) ), this, SLOT( checkActions() ) );
	addAction( aBlending );
	
	aDrawAxis = new QAction( "&Draw Axes", this );
	aDrawAxis->setToolTip( "draw xyz-Axes" );
	aDrawAxis->setCheckable( true );
	aDrawAxis->setChecked( true );
	connect( aDrawAxis, SIGNAL( toggled( bool ) ), this, SLOT( checkActions() ) );
	addAction( aDrawAxis );
	
	aDrawNodes = new QAction( "Draw &Nodes", this );
	aDrawNodes->setToolTip( "draw bones/nodes" );
	aDrawNodes->setCheckable( true );
	aDrawNodes->setChecked( true );
	connect( aDrawNodes, SIGNAL( toggled( bool ) ), this, SLOT( checkActions() ) );
	addAction( aDrawNodes );
	
	aDrawHavok = new QAction( "Draw Havok", this );
	aDrawHavok->setToolTip( "draw the havok shapes" );
	aDrawHavok->setCheckable( true );
	aDrawHavok->setChecked( false );
	connect( aDrawHavok, SIGNAL( toggled( bool ) ), this, SLOT( checkActions() ) );
	addAction( aDrawHavok );
	
	aDrawHidden = new QAction( "&Show Hidden", this );
	aDrawHidden->setToolTip( "if checked nodes and meshes are allways displayed<br>wether they are hidden ( flags & 1 ) or not" );
	aDrawHidden->setCheckable( true );
	aDrawHidden->setChecked( false );
	connect( aDrawHidden, SIGNAL( toggled( bool ) ), this, SLOT( checkActions() ) );
	addAction( aDrawHidden );
	
	aDrawStats = new QAction( "&Show Stats", this );
	aDrawStats->setToolTip( "display some statistics about the selected node" );
	aDrawStats->setCheckable( true );
	aDrawStats->setChecked( false );
	connect( aDrawStats, SIGNAL( toggled( bool ) ), this, SLOT( checkActions() ) );
	addAction( aDrawStats );
	
	aOnlyTextured = new QAction( "&Hide non textured", this );
	aOnlyTextured->setToolTip( "This options selects wether meshes without textures will be visible or not" );
	aOnlyTextured->setCheckable( true );
	aOnlyTextured->setChecked( false );
	connect( aOnlyTextured, SIGNAL( toggled( bool ) ), this, SLOT( checkActions() ) );
	addAction( aOnlyTextured );
	
	aHighlight = new QAction( "&Highlight Selected", this );
	aHighlight->setToolTip( "highlight selected meshes and nodes" );
	aHighlight->setCheckable( true );
	aHighlight->setChecked( true );
	connect( aHighlight, SIGNAL( toggled( bool ) ), this, SLOT( checkActions() ) );
	addAction( aHighlight );
	
	aRotate = new QAction( "&Rotate", this );
	aRotate->setToolTip( "slowly rotate the object around the z axis" );
	aRotate->setCheckable( true );
	aRotate->setChecked( true );
	connect( aRotate, SIGNAL( toggled( bool ) ), this, SLOT( checkActions() ) );
	addAction( aRotate );
	
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
	
	aBenchmark = new QAction( "Benchmark FPS", this );
	aBenchmark->setCheckable( true );
	aBenchmark->setChecked( false );
	aBenchmark->setVisible( true );
	connect( aBenchmark, SIGNAL( toggled( bool ) ), this, SLOT( checkActions() ) );
	addAction( aBenchmark );

	aTexFolder = new QAction( "Set Texture &Folder...", this );
	aTexFolder->setToolTip( "tell me where your textures are" );
	connect( aTexFolder, SIGNAL( triggered() ), this, SLOT( selectTexFolder() ) );
	addAction( aTexFolder );
	
	aBgColor = new QAction( "Set Background Color...", this );
	connect( aBgColor, SIGNAL( triggered() ), this, SLOT( selectBgColor() ) );
	addAction( aBgColor );
	
	aHlColor = new QAction( "Set Highlight Color...", this );
	connect( aHlColor, SIGNAL( triggered() ), this, SLOT( selectHlColor() ) );
	addAction( aHlColor );
	
	aCullExp = new QAction( "Cull Nodes by Name...", this );
	connect( aCullExp, SIGNAL( triggered() ), this, SLOT( adjustCullExp() ) );
	addAction( aCullExp );
}

GLView::~GLView()
{
	makeCurrent();
	delete scene;
}

void GLView::initializeGL()
{
	initializeTextureUnits( context() );
	
	qglClearColor( bgcolor );

	glShadeModel( GL_SMOOTH );
	
	if ( ! scene->renderer.initialize( context() ) )
	{
		aShading->setChecked( false );
		aShading->setDisabled( true );
	}
	else
		scene->renderer.updateShaders();

	static const GLfloat L0position[4] = { 0.0, 0.0, 1.0, 0.0f };
	static const GLfloat L0ambient[4] = { 0.4f, 0.4f, 0.4f, 1.0f };
	static const GLfloat L0diffuse[4] = { 0.8f, 0.8f, 0.8f, 1.0f };
	static const GLfloat L0specular[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glLightfv( GL_LIGHT0, GL_POSITION, L0position );
	glLightfv( GL_LIGHT0, GL_AMBIENT, L0ambient );
	glLightfv( GL_LIGHT0, GL_DIFFUSE, L0diffuse );
	glLightfv( GL_LIGHT0, GL_SPECULAR, L0specular );
	glLightModeli( GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE );
	glEnable( GL_LIGHT0 );
	glEnable( GL_LIGHTING );
	
	// check for errors
	
	GLenum err;
	while ( ( err = glGetError() ) != GL_NO_ERROR )
		qDebug() << "GL ERROR (init) : " << (const char *) gluErrorString( err );
}

void GLView::updateShaders()
{
	makeCurrent();
	scene->renderer.updateShaders();
	scene->shading = aShading->isChecked();
	update();
}

void GLView::glProjection( int x, int y )
{
	// check for errors
	
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
	if ( aDrawAxis->isChecked() )
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
	if ( ! ( isVisible() && height() ) )	return;
	
	qglClearColor( bgcolor );
	
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
		doCompile = false;
	}
	
	// center the model
	
	if ( doCenter )
	{
		viewAction( checkedViewAction() );
		doCenter = false;
	}
	
	// transform the scene
	
	Transform viewTrans;
	viewTrans.rotation.fromEuler( Rot[0] / 180.0 * PI, Rot[1] / 180.0 * PI, Rot[2] / 180.0 * PI );
	viewTrans.translation = viewTrans.rotation * Pos;
	if ( ! aViewWalk->isChecked() )
		viewTrans.translation[2] -= Dist * 2;
	
	scene->transform( viewTrans, time );
	
	// setup projection mode

	glProjection();
	
	// draw the axis
	
	if ( aDrawAxis->isChecked() )
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
		qWarning() << "GL ERROR (paint): " << (const char *) gluErrorString( err );

	if ( aBenchmark->isChecked() || aDrawStats->isChecked() )
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
		glColor( Color3( scene->hlcolor ) );
		
		int ls = QFontMetrics( font() ).lineSpacing();
		int y = 1;
		
		if ( aBenchmark->isChecked() )
		{
			renderText( 0, y++ * ls, QString( "FPS %1" ).arg( int( fpsact ) ), font() );
			y++;
		}
		
		if ( aDrawStats->isChecked() )
		{
			QString stats = scene->textStats();
			QStringList lines = stats.split( "\n" );
			foreach ( QString line, lines )
				renderText( 0, y++ * ls, line, font() );
		}
	}
}

QModelIndex GLView::indexAt( const QPoint & pos )
{
	if ( ! ( model && isVisible() && height() ) )
		return QModelIndex();
	
	makeCurrent();

	glProjection( pressPos.x(), pressPos.y() );

	GLuint	buffer[512];
	glSelectBuffer( 512, buffer );

	if ( aDrawHavok->isChecked() )
	{
		glRenderMode( GL_SELECT );	
		glInitNames();
		glPushName( 0 );
		
		scene->drawHavok();
		
		GLint hits = glRenderMode( GL_RENDER );
		if ( hits > 0 )
		{
			int	choose = buffer[ 3 ];
			int	depth = buffer[ 1 ];
			for ( int loop = 1; loop < hits; loop++ )
			{
				if ( buffer[ loop * 4 + 1 ] < GLuint( depth ) )
				{
					choose = buffer[ loop * 4 + 3 ];
					depth = buffer[ loop * 4 + 1 ];
				}       
			}
			return model->getBlock( choose );
		}
	}
	
	if ( aDrawNodes->isChecked() )
	{
		glRenderMode( GL_SELECT );	
		glInitNames();
		glPushName( 0 );
		
		scene->drawNodes();
		
		GLint hits = glRenderMode( GL_RENDER );
		if ( hits > 0 )
		{
			int	choose = buffer[ 3 ];
			int	depth = buffer[ 1 ];
			for ( int loop = 1; loop < hits; loop++ )
			{
				if ( buffer[ loop * 4 + 1 ] < GLuint( depth ) )
				{
					choose = buffer[ loop * 4 + 3 ];
					depth = buffer[ loop * 4 + 1 ];
				}       
			}
			return model->getBlock( choose );
		}
	}
	
	glRenderMode( GL_SELECT );	
	glInitNames();
	glPushName( 0 );
	
	scene->drawShapes();
	
	GLint hits = glRenderMode( GL_RENDER );
	if ( hits > 0 )
	{
		int	choose = buffer[ 3 ];
		int	depth = buffer[ 1 ];
		for ( int loop = 1; loop < hits; loop++ )
		{
			if ( buffer[ loop * 4 + 1 ] < GLuint( depth ) )
			{
				choose = buffer[ loop * 4 + 3];
				depth = buffer[ loop * 4 + 1];
			}       
		}
		return model->getBlock( choose );
	}
	
	return QModelIndex();
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
	
	scene->currentNode = model->getBlockNumber( index );
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

void GLView::selectTexFolder()
{
	if ( ! model ) return;
	Spell * spell = SpellBook::lookup( "Texture/Folders" );
	if ( spell && spell->isApplicable( model, QModelIndex() ) )
		spell->cast( model, QModelIndex() );
}

void GLView::selectBgColor()
{
	bgcolor = ColorWheel::choose( bgcolor, false, this );
	update();
}

void GLView::selectHlColor()
{
	scene->hlcolor = ColorWheel::choose( scene->hlcolor, false, this );
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

void GLView::adjustCullExp()
{
	bool ok;
	QString exp = QInputDialog::getText( this, "Cull Nodes by Name", "Enter a regular expression:", QLineEdit::Normal, scene->expCull.pattern(), &ok );
	if ( ok )
		scene->expCull = QRegExp( exp );
	update();
}


void GLView::viewAction( QAction * act )
{
	BoundSphere bs = scene->bounds();
	if ( aDrawAxis->isChecked() )
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
		aRotate->setChecked( false );
	}

	if ( ! act )
	{
		act = checkedViewAction();
	}
	
	if ( act != aViewWalk )
		setPosition( Vector3() - bs.center );
	
	if ( act == aViewTop )
	{
		setRotation( 0, 0, 0 );
		aViewWalk->setChecked( false );
		aViewTop->setChecked( true );
		aViewFront->setChecked( false );
		aViewSide->setChecked( false );
	}
	else if ( act == aViewFront )
	{
		setRotation( - 90, 0, 0 );
		aViewWalk->setChecked( false );
		aViewTop->setChecked( false );
		aViewFront->setChecked( true );
		aViewSide->setChecked( false );
	}
	else if ( act == aViewSide )
	{
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
	
	if ( aBenchmark->isChecked() )
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
		if ( aAnimLoop->isChecked() && time > scene->timeMax() )
			time = scene->timeMin();
		
		emit sigTime( time, scene->timeMin(), scene->timeMax() );
		update();
	}
	
	if ( aRotate->isChecked() )
	{
		rotate( 0, 0, 3 * dT );
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
	pressPos = event->pos();
}

void GLView::mouseReleaseEvent( QMouseEvent *event )
{
	if ( ! ( model && ( pressPos - event->pos() ).manhattanLength() <= 3 ) )
		return;
	
	QModelIndex idx = indexAt( event->pos() );
	scene->currentNode = model->getBlockNumber( idx );
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
	doCompile = true;
	if ( ! aViewWalk->isChecked() )
		doCenter = true;
	update();
}

void GLView::wheelEvent( QWheelEvent * event )
{
	if ( aViewWalk->isChecked() )
		mouseMov += Vector3( 0, 0, event->delta() );
	else
		setDistance( Dist * ( event->delta() > 0 ? 1.2 : 0.8 ) );
}

void GLView::checkActions()
{
	scene->texturing = aTexturing->isChecked();
	scene->blending = aBlending->isChecked();
	scene->showHavok = aDrawHavok->isChecked();
	scene->highlight = aHighlight->isChecked();
	scene->showHidden = aDrawHidden->isChecked();
	scene->showNodes = aDrawNodes->isChecked();
	scene->animate = aAnimate->isChecked();
	scene->onlyTextured = aOnlyTextured->isChecked();
	
	lastTime = QTime::currentTime();
	
	if ( aBenchmark->isChecked() )
		timer->setInterval( 0 );
	else
		timer->setInterval( 1000 / FPS );
	
	update();
}

void GLView::save( QSettings & settings )
{
	//settings.beginGroup( "OpenGL" );
	settings.setValue( "texture folder", TexCache::texfolders.join( ";" ) );
	settings.setValue( "enable textures", aTexturing->isChecked() );
	settings.setValue( "enable blending", aBlending->isChecked() );
	settings.setValue( "enable shading", aShading->isChecked() );
	settings.setValue( "draw havok", aDrawHavok->isChecked() );
	settings.setValue( "highlight meshes", aHighlight->isChecked() );
	settings.setValue( "draw axis", aDrawAxis->isChecked() );
	settings.setValue( "draw nodes", aDrawNodes->isChecked() );
	settings.setValue( "draw hidden", aDrawHidden->isChecked() );
	settings.setValue( "draw stats", aDrawStats->isChecked() );
	settings.setValue( "rotate", aRotate->isChecked() );
	settings.setValue( "enable animations", aAnimate->isChecked() );
	settings.setValue( "play animation", aAnimPlay->isChecked() );
	settings.setValue( "loop animation", aAnimLoop->isChecked() );
	settings.setValue( "bg color", bgcolor );
	settings.setValue( "hl color", scene->hlcolor );
	settings.setValue( "cull nodes by name", scene->expCull );
	settings.setValue( "draw only textured meshes", scene->onlyTextured );
	
	settings.setValue( "perspective", aViewPerspective->isChecked() );
	//settings.endGroup();
}

void GLView::restore( QSettings & settings )
{
	//settings.beginGroup( "OpenGL" );
	TexCache::texfolders = settings.value( "texture folder" ).toString().split( ";" );
	aTexturing->setChecked( settings.value( "enable textures", true ).toBool() );
	aShading->setChecked( settings.value( "enable shading", true ).toBool() && aShading->isEnabled() );
	aDrawHavok->setChecked( settings.value( "draw havok", true ).toBool() );
	aBlending->setChecked( settings.value( "enable blending", true ).toBool() );
	aHighlight->setChecked( settings.value( "highlight meshes", true ).toBool() );
	aDrawAxis->setChecked( settings.value( "draw axis", false ).toBool() );
	aDrawNodes->setChecked( settings.value( "draw nodes", false ).toBool() );
	aDrawHidden->setChecked( settings.value( "draw hidden", false ).toBool() );
	aDrawStats->setChecked( settings.value( "draw stats", false ).toBool() );
	aRotate->setChecked( settings.value( "rotate", true ).toBool() );
	aAnimate->setChecked( settings.value( "enable animations", true ).toBool() );
	aAnimPlay->setChecked( settings.value( "play animation", true ).toBool() );
	aAnimLoop->setChecked( settings.value( "loop animation", true ).toBool() );
	bgcolor = settings.value( "bg color", palette().color( QPalette::Active, QPalette::Background ) ).value<QColor>();
	scene->hlcolor = settings.value( "hl color", QColor::fromRgbF( 0.0, 1.0, 0.0 ) ).value<QColor>();
	scene->expCull = settings.value( "cull nodes by name", QRegExp(
		"^collidee|^shadowcaster|^\\!LoD_cullme|^footprint"
	) ).value<QRegExp>();
	aOnlyTextured->setChecked( settings.value( "draw only textured meshes", false ).toBool() );
	
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
