/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005-2009, NIF File Format Library and Tools
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

#include "uvedit.h"

#include "../nifmodel.h"
#include "../niftypes.h"
#include "../options.h"
#include "../gl/gltex.h"
#include "../gl/gltools.h"
#include "../NvTriStrip/qtwrapper.h"

#include <math.h>
#include <GL/glext.h>

#include <QCursor>
#include <QTimer>
#include <QUndoStack>

#define BASESIZE 512.0
#define GRIDSIZE 16.0
#define GRIDSEGS 4
#define ZOOMUNIT 64.0

UVWidget * UVWidget::createEditor( NifModel * nif, const QModelIndex & idx )
{
	UVWidget * uvw = new UVWidget;
	uvw->setAttribute( Qt::WA_DeleteOnClose );
	if ( ! uvw->setNifData( nif, idx ) )
	{
		qWarning() << tr( "Could not load texture data for UV editor." );
		delete uvw;
		return NULL;
	}
	uvw->show();
	return uvw;
}

static GLshort vertArray[4][2] = {
	{ 0, 0 }, { 1, 0 },
	{ 1, 1 }, { 0, 1 }
};

static GLshort texArray[4][2] = {
	{ 0, 0 }, { 1, 0 },
	{ 1, 1 }, { 0, 1 }
};

static GLdouble glUnit = ( 1.0 / BASESIZE );
static GLdouble glGridD = GRIDSIZE * glUnit;

UVWidget::UVWidget( QWidget * parent )
	: QGLWidget( QGLFormat( QGL::SampleBuffers ), parent, 0, Qt::Tool | Qt::WindowStaysOnTopHint ), undoStack( new QUndoStack( this ) )
{
	texnames = QStringList()
		<< "Base Texture"
		<< "Dark Texture"
		<< "Detail Texture"
		<<"Gloss Texture"
		<<"Glow Texture"
		<<"Bump Map Texture"
		<<"Decal 0 Texture"
		<<"Decal 1 Texture";

	setWindowTitle( tr("UV Editor") );
	setFocusPolicy( Qt::StrongFocus );
	
	textures = new TexCache( this );

	zoom = 1.2;

	pos = QPoint( 0, 0 );

	mousePos = QPoint( -1000, -1000 );

	setCursor( QCursor( Qt::CrossCursor ) );
	setMouseTracking( true );
	
	setContextMenuPolicy( Qt::ActionsContextMenu );
	
	QAction * aUndo = undoStack->createUndoAction( this );
	QAction * aRedo = undoStack->createRedoAction( this );
	
	aUndo->setShortcut( QKeySequence::Undo );
	aRedo->setShortcut( QKeySequence::Redo );
	
	addAction( aUndo );
	addAction( aRedo );
	
	QAction * aSep = new QAction( this );
	aSep->setSeparator( true );
	addAction( aSep );
	
	QAction * aSelectAll = new QAction( tr( "Select &All" ), this );
	aSelectAll->setShortcut( QKeySequence::SelectAll );
	connect( aSelectAll, SIGNAL( triggered() ), this, SLOT( selectAll() ) );
	addAction( aSelectAll );
	
	QAction * aSelectNone = new QAction( tr( "Select &None" ), this );
	connect( aSelectNone, SIGNAL( triggered() ), this, SLOT( selectNone() ) );
	addAction( aSelectNone );
	
	QAction * aSelectFaces = new QAction( tr( "Select &Faces" ), this );
	connect( aSelectFaces, SIGNAL( triggered() ), this, SLOT( selectFaces() ) );
	addAction( aSelectFaces );
	
	QAction * aSelectConnected = new QAction( tr( "Select &Connected" ), this );
	connect( aSelectConnected, SIGNAL( triggered() ), this, SLOT( selectConnected() ) );
	addAction( aSelectConnected );

	aSep = new QAction( this );
	aSep->setSeparator( true );
	addAction( aSep );
	
	aTextureBlend = new QAction( tr( "Texture Alpha Blending" ), this );
	aTextureBlend->setCheckable( true );
	aTextureBlend->setChecked( true );
	connect( aTextureBlend, SIGNAL( toggled( bool ) ), this, SLOT( updateGL() ) );
	addAction( aTextureBlend );

	QAction * aTexSlot = new QAction( tr( "Select Texture Slot..." ), this );
	connect( aTexSlot, SIGNAL( triggered() ), this, SLOT( selectTexSlot() ) );
	//addAction( aTexSlot );

	texSlotGroup = new QActionGroup( this );
	connect( texSlotGroup, SIGNAL( triggered( QAction * ) ), this, SLOT( selectTexSlot() ) );

	texActions = new QList<QAction *>();

	menuTexSelect = new QMenu( tr( "Select Texture Slot" ) );
	addAction( menuTexSelect->menuAction() );
	connect( menuTexSelect, SIGNAL( aboutToShow() ), this, SLOT( getTexSlots() ) );

	currentTexSlot = 0;	

	connect( Options::get(), SIGNAL( sigChanged() ), this, SLOT( updateGL() ) );
}

UVWidget::~UVWidget()
{
	delete textures;
}

void UVWidget::initializeGL()
{
	glMatrixMode( GL_MODELVIEW );

	initializeTextureUnits( context() );

	glShadeModel( GL_SMOOTH );
	glShadeModel( GL_LINE_SMOOTH );

	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glEnable( GL_BLEND );

	glDepthFunc( GL_LEQUAL );
	glEnable( GL_DEPTH_TEST );

	glEnable( GL_MULTISAMPLE );
	glDisable( GL_LIGHTING );

	qglClearColor( Options::bgColor() );

	if( !texfile.isEmpty() )
		bindTexture( texfile );
	else
		bindTexture( texsource );
	
	glEnableClientState( GL_VERTEX_ARRAY );
	glVertexPointer( 2, GL_SHORT, 0, vertArray );

	glEnableClientState( GL_TEXTURE_COORD_ARRAY );
	glTexCoordPointer( 2, GL_SHORT, 0, texArray );

	// check for errors
	GLenum err;
	while ( ( err = glGetError() ) != GL_NO_ERROR ) {
		qDebug() << "GL ERROR (init) : " << (const char *) gluErrorString( err );
	}
}

void UVWidget::resizeGL( int width, int height )
{
	updateViewRect( width, height );
}

void UVWidget::paintGL()
{
	glPushAttrib( GL_ALL_ATTRIB_BITS );
	
	glMatrixMode( GL_PROJECTION );
	glPushMatrix();
	glLoadIdentity();

	setupViewport( width(), height() );

	glMatrixMode( GL_MODELVIEW );
	glPushMatrix();
	glLoadIdentity();
	
	qglClearColor( Options::bgColor() );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	
	glDisable( GL_DEPTH_TEST );
	glDepthMask( GL_FALSE );
	
	// draw texture

	glPushMatrix();
	glLoadIdentity();
	
	glEnable( GL_TEXTURE_2D );
	
	if ( aTextureBlend->isChecked() )
		glEnable( GL_BLEND );
	else
		glDisable( GL_BLEND );

	if( !texfile.isEmpty() )
		bindTexture( texfile );
	else
		bindTexture( texsource );


	glTranslatef( -0.5f, -0.5f, 0.0f );

	glTranslatef( -1.0f, -1.0f, 0.0f );
	for( int i = 0; i < 3; i++ )
	{
		for( int j = 0; j < 3; j++ )
		{
			if( i == 1 && j == 1 ) {
				glColor4f( 0.75f, 0.75f, 0.75f, 1.0f );
			}
			else {
				glColor4f( 0.5f, 0.5f, 0.5f, 1.0f );
			}

			glDrawArrays( GL_QUADS, 0, 4 );

			glTranslatef( 1.0f, 0.0f, 0.0f );
		}

		glTranslatef( -3.0f, 1.0f, 0.0f );
	}
	glTranslatef( 1.0f, -2.0f, 0.0f );
	
	glDisable( GL_TEXTURE_2D );
	
	glPopMatrix();

	
	// draw grid

	glPushMatrix();
	glLoadIdentity();
	
	glEnable( GL_BLEND );

	glLineWidth( 0.8f );
	glBegin( GL_LINES );
	int glGridMinX	= qRound( qMin( glViewRect[0], glViewRect[1] ) / glGridD );
	int glGridMaxX	= qRound( qMax( glViewRect[0], glViewRect[1] ) / glGridD );
	int glGridMinY	= qRound( qMin( glViewRect[2], glViewRect[3] ) / glGridD );
	int glGridMaxY	= qRound( qMax( glViewRect[2], glViewRect[3] ) / glGridD );
	for( int i = glGridMinX; i < glGridMaxX; i++ )
	{
		GLdouble glGridPos = glGridD * i;

		if( ( i % GRIDSEGS ) == 0 ) {
			glLineWidth( 1.2f );
			glColor4f( 1.0f, 1.0f, 1.0f, 0.2f );
		}
		else if( zoom > 2.0 ) {
			continue;
		}

		glVertex2d( glGridPos, glViewRect[2] );
		glVertex2d( glGridPos, glViewRect[3] );

		if( ( i % GRIDSEGS ) == 0 ) {
			glLineWidth( 0.8f );
			glColor4f( 1.0f, 1.0f, 1.0f, 0.1f );
		}
	}
	for( int i = glGridMinY; i < glGridMaxY; i++ )
	{
		GLdouble glGridPos = glGridD * i;

		if( ( i % GRIDSEGS ) == 0 ) {
			glLineWidth( 1.2f );
			glColor4f( 1.0f, 1.0f, 1.0f, 0.2f );
		}
		else if( zoom > 2.0 ) {
			continue;
		}

		glVertex2d( glViewRect[0], glGridPos );
		glVertex2d( glViewRect[1], glGridPos );

		if( ( i % GRIDSEGS ) == 0 ) {
			glLineWidth( 0.8f );
			glColor4f( 1.0f, 1.0f, 1.0f, 0.1f );
		}
	}
	glEnd();

	glPopMatrix();



	drawTexCoords();

	
	glDisable( GL_DEPTH_TEST );
	glDepthMask( GL_FALSE );

	if( ! selectRect.isNull() )
	{
		glLoadIdentity();
		glHighlightColor();
		glBegin( GL_LINE_LOOP );
		glVertex( mapToContents( selectRect.topLeft() ) );
		glVertex( mapToContents( selectRect.topRight() ) );
		glVertex( mapToContents( selectRect.bottomRight() ) );
		glVertex( mapToContents( selectRect.bottomLeft() ) );
		glEnd();
	}
	
	if ( ! selectPoly.isEmpty() )
	{
		glLoadIdentity();
		glHighlightColor();
		glBegin( GL_LINE_LOOP );
		foreach ( QPoint p, selectPoly )
		{
			glVertex( mapToContents( p ) );
		}
		glEnd();
	}



	glMatrixMode( GL_MODELVIEW );
	glPopMatrix();

	glMatrixMode( GL_PROJECTION );
	glPopMatrix();

	glPopAttrib();
}

void UVWidget::drawTexCoords()
{
	glMatrixMode( GL_MODELVIEW );

	glPushMatrix();
	glLoadIdentity();

	glScalef( 1.0f, 1.0f, 1.0f );
	glTranslatef( -0.5f, -0.5f, 0.0f );

	Color4 nlColor( Options::nlColor() );
	nlColor.setAlpha( 0.5f );
	Color4 hlColor( Options::hlColor() );
	hlColor.setAlpha( 0.5f );

	glLineWidth( 1.0f );
	glPointSize( 3.5f );
	
	glEnable( GL_BLEND );
	glEnable( GL_DEPTH_TEST );
	glDepthFunc( GL_LEQUAL );
	glDepthMask( GL_TRUE );
	
	float z;
	
	// draw triangle edges
	
	for ( int i = 0; i < faces.size(); i++ )
	{
		glBegin( GL_LINE_LOOP );
		for ( int j = 0; j < 3; j++ )
		{
			int x = faces[i].tc[j];
			
			if ( selection.contains( x ) )
			{
				glColor( Color3( hlColor ) );
				z = 1.0f;
			}
			else
			{
				glColor( Color3( nlColor ) );
				z = 0.0f;
			}
			
			glVertex( Vector3( texcoords[x], z ) );
		}
		glEnd();
	}
	
	// draw points
	
	glBegin( GL_POINTS );
	for ( int i = 0; i < texcoords.size(); i++ )
	{
		if ( selection.contains( i ) )
		{
			glColor( Color3( hlColor ) );
			z = 1.0f;
		}
		else
		{
			glColor( Color3( nlColor ) );
			z = 0.0f;
		}
		glVertex( Vector3( texcoords[i], z ) );
	}
	glEnd();
	
	glPopMatrix();
}

void UVWidget::setupViewport( int width, int height )
{
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();

	glViewport( 0, 0, width, height );

	glOrtho( glViewRect[0], glViewRect[1], glViewRect[2], glViewRect[3], -10.0, +10.0 );
}

void UVWidget::updateViewRect( int width, int height )
{
	GLdouble glOffX	= glUnit * zoom * 0.5 * width;
	GLdouble glOffY	= glUnit * zoom * 0.5 * height;
	GLdouble glPosX = glUnit * pos.x();
	GLdouble glPosY = glUnit * pos.y();

	glViewRect[0] = - glOffX - glPosX;
	glViewRect[1] = + glOffX - glPosX;
	glViewRect[2] = + glOffY + glPosY;
	glViewRect[3] = - glOffY + glPosY;
}

QPoint UVWidget::mapFromContents( const Vector2 & v ) const
{
	float x = ( ( v[0] - 0.5 ) - glViewRect[ 0 ] ) / ( glViewRect[ 1 ] - glViewRect[ 0 ] ) * width();
	float y = ( ( v[1] - 0.5 ) - glViewRect[ 2 ] ) / ( glViewRect[ 3 ] - glViewRect[ 2 ] ) * height() * ( - 1 ) + height();
	
	return QPointF( x, y ).toPoint();
}

Vector2 UVWidget::mapToContents( const QPoint & p ) const
{
	float x = ( (float) p.x() / (float) width() ) * ( glViewRect[ 1 ] - glViewRect[ 0 ] ) + glViewRect[ 0 ];
	float y = ( (float) p.y() / (float) height() ) * ( glViewRect[ 2 ] - glViewRect[ 3 ] ) + glViewRect[ 3 ];
	return Vector2( x, y );
}

QVector<int> UVWidget::indices( const QPoint & p ) const
{
	return indices( QRect( p - QPoint( 2, 2 ), QSize( 5, 5 ) ) );
}

QVector<int> UVWidget::indices( const QRegion & region ) const
{
	QList<int> hits;
	
	for ( int i = 0; i < texcoords.count(); i++ )
	{
		if ( region.contains( mapFromContents( texcoords[ i ] ) ) )
			hits << i;
	}
	
	return hits.toVector();
}

bool UVWidget::bindTexture( const QString & filename )
{
	GLuint mipmaps = 0;
	GLfloat max_anisotropy = get_max_anisotropy(); // init from gltex

	QString extensions( (const char *) glGetString( GL_EXTENSIONS ) );
	
	if ( extensions.contains( "GL_EXT_texture_filter_anisotropic" ) )
	{
		glGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, & max_anisotropy );
		//qWarning() << "maximum anisotropy" << max_anisotropy;
	}

	if ( mipmaps = textures->bind( filename ) )
	{
		if ( max_anisotropy > 0.0f )
		{
			if ( Options::antialias() )
				glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, max_anisotropy );
			else
				glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f );
		}

		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipmaps > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR );
		glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

		// TODO: Add support for non-square textures

		glMatrixMode( GL_TEXTURE );
		glLoadIdentity();

		glMatrixMode( GL_MODELVIEW );
		return true;
	}
	
	return false;
}

bool UVWidget::bindTexture( const QModelIndex & iSource )
{
	GLuint mipmaps = 0;
	GLfloat max_anisotropy = get_max_anisotropy(); // init from gltex

	QString extensions( (const char *) glGetString( GL_EXTENSIONS ) );
	
	if ( extensions.contains( "GL_EXT_texture_filter_anisotropic" ) )
	{
		glGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, & max_anisotropy );
		//qWarning() << "maximum anisotropy" << max_anisotropy;
	}

	if ( mipmaps = textures->bind( iSource ) )
	{
		if ( max_anisotropy > 0.0f )
		{
			if ( Options::antialias() )
				glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, max_anisotropy );
			else
				glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f );
		}

		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipmaps > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR );
		glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

		// TODO: Add support for non-square textures

		glMatrixMode( GL_TEXTURE );
		glLoadIdentity();

		glMatrixMode( GL_MODELVIEW );
		return true;
	}
	
	return false;
 }


QSize UVWidget::sizeHint() const
{
	if( sHint.isValid() ) {
		return sHint;
	}

	return QSizeF( BASESIZE, BASESIZE ).toSize();
}

void UVWidget::setSizeHint( const QSize & s )
{
	sHint = s;
}

QSize UVWidget::minimumSizeHint() const
{
	return QSizeF( BASESIZE, BASESIZE ).toSize();
}

int UVWidget::heightForWidth( int width ) const
{
	if ( width < minimumSizeHint().height() )
		return minimumSizeHint().height();
	return width;
}

void UVWidget::mousePressEvent( QMouseEvent * e )
{
	QPoint dPos( e->pos() - mousePos );
	mousePos = e->pos();
	
	if ( e->button() == Qt::LeftButton )
	{
		QVector<int> hits = indices( mousePos );
		
		if ( hits.isEmpty() )
		{
			if ( ! e->modifiers().testFlag( Qt::ShiftModifier ) )
				selectNone();
			
			if ( e->modifiers().testFlag( Qt::AltModifier ) )
			{
				selectPoly << e->pos();
			}
			else
			{
				selectRect.setTopLeft( mousePos );
				selectRect.setBottomRight( mousePos );
			}
		}
		else
		{
			if ( dPos.manhattanLength() > 4 )
				selectCycle = 0;
			else
				selectCycle++;
			
			int h = hits[ selectCycle % hits.count() ];
			
			if ( ! e->modifiers().testFlag( Qt::ShiftModifier ) )
			{
				if ( ! isSelected( h ) )
					selectNone();
				select( h );
			}
			else
			{
				select( h, ! isSelected( h ) );
			}
			
			if ( selection.isEmpty() )
			{
				setCursor( QCursor( Qt::CrossCursor ) );
			}
			else
			{
				setCursor( QCursor( Qt::SizeAllCursor ) );
			}
		}
	}

	updateGL();
}

void UVWidget::mouseMoveEvent( QMouseEvent * e )
{
	QPoint dPos( e->pos() - mousePos );

	switch ( e->buttons() )
	{
		case Qt::LeftButton:
			if ( ! selectRect.isNull() )
			{
				selectRect.setBottomRight( e->pos() );
			}
			else if ( ! selectPoly.isEmpty() )
			{
				selectPoly << e->pos();
			}
			else
			{
				moveSelection( glUnit * zoom * dPos.x(), glUnit * zoom * dPos.y() );
			}
			break;
			
		case Qt::MidButton:
			pos += zoom * QPointF( dPos.x(), -dPos.y() );
			updateViewRect( width(), height() );
			
			setCursor( QCursor( Qt::OpenHandCursor ) );
			
			break;
			
		case Qt::RightButton:
			zoom *= 1.0 + ( dPos.y() / ZOOMUNIT );
			
			if ( zoom < 0.1 )
			{
				zoom = 0.1;
			}
			else if ( zoom > 10.0 )
			{
				zoom = 10.0;
			}
			
			updateViewRect( width(), height() );
			
			setCursor( QCursor( Qt::SizeVerCursor ) );
			
			break;
			
		default:
			if ( indices( e->pos() ).count() )
			{
				setCursor( QCursor( Qt::ArrowCursor ) );
			}
			else
			{
				setCursor( QCursor( Qt::CrossCursor ) );
			}
			return;
	}

	mousePos = e->pos();

	updateGL();
}

void UVWidget::mouseReleaseEvent( QMouseEvent * e )
{
	switch( e->button() )
	{
		case Qt::LeftButton:
			if ( ! selectRect.isNull() )
			{
				select( QRegion( selectRect.normalized() ) );
				selectRect = QRect();
			}
			else if ( ! selectPoly.isEmpty() )
			{
				if ( selectPoly.size() > 2 )
				{
					select( QRegion( QPolygon( selectPoly.toVector() ) ) );
				}
				selectPoly.clear();
			}
			break;
		default:
			break;
	}

	if ( indices( e->pos() ).count() )
	{
		setCursor( QCursor( Qt::ArrowCursor ) );
	}
	else
	{
		setCursor( QCursor( Qt::CrossCursor ) );
	}
	
	updateGL();
}

void UVWidget::wheelEvent( QWheelEvent * e )
{
	switch( e->modifiers()) {
		case Qt::NoModifier:
			zoom *= 1.0 + ( e->delta() / 8.0 ) / ZOOMUNIT;

			if( zoom < 0.1 ) {
				zoom = 0.1;
			}
			else if( zoom > 10.0 ) {
				zoom = 10.0;
			}

			updateViewRect( width(), height() );

			break;
	}

	updateGL();
}



bool UVWidget::setNifData( NifModel * nifModel, const QModelIndex & nifIndex )
{
	if ( nif )
	{
		disconnect( nif );
	}
	
	undoStack->clear();
	
	nif = nifModel;
	iShape = nifIndex;
	
	connect( nif, SIGNAL( modelReset() ), this, SLOT( close() ) );
	connect( nif, SIGNAL( destroyed() ), this, SLOT( close() ) );
	connect( nif, SIGNAL( dataChanged( const QModelIndex &, const QModelIndex & ) ), this, SLOT( nifDataChanged( const QModelIndex & ) ) );
	connect( nif, SIGNAL( rowsRemoved( const QModelIndex &, int, int ) ), this, SLOT( nifDataChanged( const QModelIndex ) ) );

	textures->setNifFolder( nif->getFolder() );

	iShapeData = nif->getBlock( nif->getLink( iShape, "Data" ) );
	if( nif->inherits( iShapeData, "NiTriBasedGeomData" ) )
	{
		iTexCoords = nif->getIndex( iShapeData, "UV Sets" ).child( 0, 0 );
		if( ! iTexCoords.isValid() || ! nif->rowCount( iTexCoords ) )
		{
			return false;
		}

		if ( ! setTexCoords() )
		{
			return false;
		}
		
	}
		
	foreach( qint32 l, nif->getLinkArray( iShape, "Properties" ) )
	{
		QModelIndex iTexProp = nif->getBlock( l, "NiTexturingProperty" );
		if( iTexProp.isValid() )
		{
			QModelIndex iTex = nif->getIndex( iTexProp, texnames[currentTexSlot] );
			if( iTex.isValid() )
			{
				QModelIndex iTexSource = nif->getBlock( nif->getLink( iTex, "Source" ) );
				if( iTexSource.isValid() )
				{
					//texfile = TexCache::find( nif->get<QString>( iTexSource, "File Name" ) , nif->getFolder() );
					//int texUVset = nif->get<int>( iTex, "UV Set" );
					//qWarning() << "Use UV set " << texUVset;
					//qWarning() << nif->getIndex( iShapeData, "UV Sets" ).child( texUVset, 0 );
					texsource = iTexSource;
					return true;
				}
			}
		}
		else 
		{
			iTexProp = nif->getBlock( l, "NiTextureProperty" );
			if( iTexProp.isValid() )
			{
				QModelIndex iTexSource = nif->getBlock( nif->getLink( iTexProp, "Image" ) );
				if( iTexSource.isValid() )
				{
					//texfile = TexCache::find( nif->get<QString>( iTexSource, "File Name" ) , nif->getFolder() );
					texsource = iTexSource;
					return true;
				}
			}
			else
			{
				iTexProp = nif->getBlock( l, "BSShaderPPLightingProperty" );
				if( iTexProp.isValid() )
				{
					QModelIndex iTexSource = nif->getBlock( nif->getLink( iTexProp, "Texture Set" ) );
					if( iTexSource.isValid() )
					{
						// Assume that a FO3 mesh never has embedded textures...
						//texsource = iTexSource;
						//return true;
						QModelIndex textures = nif->getIndex(iTexSource, "Textures");
						if (textures.isValid())
						{
							texfile = TexCache::find( nif->get<QString>( textures.child(0, 0)) , nif->getFolder() );
							return true;   
						}
					}
				}
			}
		}
	}

	return false;
}

bool UVWidget::setTexCoords()
{
	texcoords = nif->getArray< Vector2 >( iTexCoords );

	QVector< Triangle > tris;

	if( nif->isNiBlock( iShapeData, "NiTriShapeData" ) )
	{
		tris = nif->getArray< Triangle >( iShapeData, "Triangles" );
	}
	else if( nif->isNiBlock( iShapeData, "NiTriStripsData" ) )
	{
		QModelIndex iPoints = nif->getIndex( iShapeData, "Points" );
		if( iPoints.isValid() )
		{
			for( int r = 0; r < nif->rowCount( iPoints ); r++ )
			{
				tris += triangulate( nif->getArray< quint16 >( iPoints.child( r, 0 ) ) );
			}
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}

	QVectorIterator< Triangle > itri( tris );
	while ( itri.hasNext() )
	{
		const Triangle & t = itri.next();

		int fIdx = faces.size();
		faces.append( face( fIdx, t[0], t[1], t[2] ) );

		for( int i = 0; i < 3; i++ )
		{
			texcoords2faces.insertMulti( t[i], fIdx );
		}
	}
	return true;
}

void UVWidget::updateNif()
{
	if ( nif && iTexCoords.isValid() )
	{
		disconnect( nif, SIGNAL( dataChanged( const QModelIndex &, const QModelIndex & ) ), this, SLOT( nifDataChanged( const QModelIndex & ) ) );
		nif->setArray< Vector2 >( iTexCoords, texcoords );
		connect( nif, SIGNAL( dataChanged( const QModelIndex &, const QModelIndex & ) ), this, SLOT( nifDataChanged( const QModelIndex & ) ) );
	}
}

void UVWidget::nifDataChanged( const QModelIndex & idx )
{
	if ( ! nif || ! iShape.isValid() || ! iShapeData.isValid() || ! iTexCoords.isValid() )
	{
		close();
		return;
	}
	
	if ( nif->getBlock( idx ) == iShapeData )
	{
		//if ( ! setNifData( nif, iShape ) )
		{
			close();
			return;
		}
	}
}

bool UVWidget::isSelected( int index )
{
	return selection.contains( index );
}

class UVWSelectCommand : public QUndoCommand
{
public:
	UVWSelectCommand( UVWidget * w, const QList<int> & nSel ) : QUndoCommand(), uvw( w ), newSelection( nSel )
	{
		setText( "Select" );
	}
	
	int id() const
	{
		return 0;
	}
	
	bool mergeWith( const QUndoCommand * cmd )
	{
		if ( cmd->id() == id() )
		{
			newSelection = static_cast<const UVWSelectCommand *>( cmd )->newSelection;
			return true;
		}
		return false;
	}
	
	void redo()
	{
		oldSelection = uvw->selection;
		uvw->selection = newSelection;
		uvw->updateGL();
	}
	
	void undo()
	{
		uvw->selection = oldSelection;
		uvw->updateGL();
	}

protected:
	UVWidget * uvw;
	QList<int> oldSelection, newSelection;
};

void UVWidget::select( int index, bool yes )
{
	QList<int> selection = this->selection;
	if ( yes )
	{
		if ( ! selection.contains( index ) )
			selection.append( index );
	}
	else
		selection.removeAll( index );
	undoStack->push( new UVWSelectCommand( this, selection ) );
}

void UVWidget::select( const QRegion & r, bool add )
{
	QList<int> selection( add ? this->selection : QList<int>() );
	foreach ( int s, indices( r ) )
	{
		if ( ! selection.contains( s ) )
			selection.append( s );
	}
	undoStack->push( new UVWSelectCommand( this, selection ) );
}

void UVWidget::selectNone()
{
	undoStack->push( new UVWSelectCommand( this, QList<int>() ) );
}

void UVWidget::selectAll()
{
	QList<int> selection;
	for ( int s = 0; s < texcoords.count(); s++ )
		selection << s;
	undoStack->push( new UVWSelectCommand( this, selection ) );
}

void UVWidget::selectFaces()
{
	QList<int> selection = this->selection;
	foreach ( int s, selection )
	{
		foreach ( int f, texcoords2faces.values( s ) )
		{
			for ( int i = 0; i < 3; i++ )
			{
				if ( ! selection.contains( faces[f].tc[i] ) )
					selection.append( faces[f].tc[i] );
			}
		}
	}
	undoStack->push( new UVWSelectCommand( this, selection ) );
}

void UVWidget::selectConnected()
{
	QList<int> selection = this->selection;
	bool more = true;
	while ( more )
	{
		more = false;
		foreach ( int s, selection )
		{
			foreach ( int f, texcoords2faces.values( s ) )
			{
				for ( int i = 0; i < 3; i++ )
				{
					if ( ! selection.contains( faces[f].tc[i] ) )
					{
						selection.append( faces[f].tc[i] );
						more = true;
					}
				}
			}
		}
	}
	undoStack->push( new UVWSelectCommand( this, selection ) );
}

class UVWMoveCommand : public QUndoCommand
{
public:
	UVWMoveCommand( UVWidget * w, double dx, double dy ) : QUndoCommand(), uvw( w ), move( dx, dy )
	{
		setText( "Move" );
	}
	
	int id() const
	{
		return 1;
	}
	
	bool mergeWith( const QUndoCommand * cmd )
	{
		if ( cmd->id() == id() )
		{
			move += static_cast<const UVWMoveCommand*>( cmd )->move;
			return true;
		}
		return false;
	}
	
	void redo()
	{
		foreach( int tc, uvw->selection )
		{
			uvw->texcoords[tc] += move;
		}
		uvw->updateNif();
		uvw->updateGL();
	}
	
	void undo()
	{
		foreach( int tc, uvw->selection )
		{
			uvw->texcoords[tc] -= move;
		}
		uvw->updateNif();
		uvw->updateGL();
	}

protected:
	UVWidget * uvw;
	Vector2 move;
};

void UVWidget::moveSelection( double moveX, double moveY )
{
	undoStack->push( new UVWMoveCommand( this, moveX, moveY ) );
}

void UVWidget::getTexSlots()
{
	foreach( qint32 l, nif->getLinkArray( iShape, "Properties" ) )
	{
		QModelIndex iTexProp = nif->getBlock( l, "NiTexturingProperty" );
		if( iTexProp.isValid() )
		{
			foreach( QString name, texnames )
			{
				if ( nif->get<bool>( iTexProp, QString( "Has %1" ).arg( name ) ) )
				{
					if ( validTexs.indexOf( name ) == -1 )
					{
						validTexs << name;
						QAction * temp;
						menuTexSelect->addAction( temp = new QAction( name, this ) );
						texSlotGroup->addAction( temp );
						temp->setCheckable( true );
						texActions->append( temp );
						if ( name == texnames[currentTexSlot] )
						{
							temp->setChecked( true );
						}
					}
				}
			}
		}
	}
}

// This is the old one that produced a dialog box
/*
void UVWidget::selectTexSlot()
{
	QDialog dlg;
	QVBoxLayout * vbox = new QVBoxLayout;
	dlg.setLayout( vbox );

	vbox->addWidget( new QLabel( tr( "Texture Slot" ) ) );
	
	QComboBox * set = new QComboBox;
	QModelIndex iUVs = nif->getIndex( iShapeData, "UV Sets" );

	QStringList validTexs = QStringList();

	foreach( qint32 l, nif->getLinkArray( iShape, "Properties" ) )
	{
		QModelIndex iTexProp = nif->getBlock( l, "NiTexturingProperty" );
		if( iTexProp.isValid() )
		{
			foreach( QString name, texnames )
			{
				if ( nif->get<bool>( iTexProp, QString( "Has %1" ).arg( name ) ) )
				{
					set->addItem( name );
					validTexs << name;
				}
			}
		}
	}
					
	set->setCurrentIndex( validTexs.indexOf( texnames[currentTexSlot] ) );

	vbox->addWidget( set );

	QHBoxLayout * hbox = new QHBoxLayout;
	vbox->addLayout( hbox );

	QPushButton * btAccept = new QPushButton( tr("OK") );
	hbox->addWidget( btAccept );
	connect( btAccept, SIGNAL( clicked() ), &dlg, SLOT( accept() ) );

	QPushButton * btReject = new QPushButton( tr("Cancel") );
	hbox->addWidget( btReject );
	connect( btReject, SIGNAL( clicked() ), &dlg, SLOT( reject() ) );

	if ( dlg.exec() == QDialog::Accepted )
	{
		//qWarning() << "Selected texture " << set->currentIndex();
		//qWarning() << "corresponding to " << texnames.indexOf( validTexs[set->currentIndex()] );
		currentTexSlot = texnames.indexOf( validTexs[set->currentIndex()] );
		foreach( qint32 l, nif->getLinkArray( iShape, "Properties" ) )
		{
			QModelIndex iTexProp = nif->getBlock( l, "NiTexturingProperty" );
			if( iTexProp.isValid() )
			{
				QModelIndex iTex = nif->getIndex( iTexProp, texnames[currentTexSlot] );
				if( iTex.isValid() )
				{
					QModelIndex iTexSource = nif->getBlock( nif->getLink( iTex, "Source" ) );
					if( iTexSource.isValid() )
					{
						//texfile = TexCache::find( nif->get<QString>( iTexSource, "File Name" ) , nif->getFolder() );
						int texUVset = nif->get<int>( iTex, "UV Set" );
						//qWarning() << "Use UV set " << texUVset;
						iTexCoords = nif->getIndex( iShapeData, "UV Sets" ).child( texUVset, 0 );
						//qWarning() << iTexCoords;
						texsource = iTexSource;
						setTexCoords();
						updateGL();
						return;
					}
				}
			}
		}



	}

}
*/

void UVWidget::selectTexSlot()
{
	QString selected = texSlotGroup->checkedAction()->text();
	currentTexSlot = texnames.indexOf( selected );
	foreach( qint32 l, nif->getLinkArray( iShape, "Properties" ) )
	{
		QModelIndex iTexProp = nif->getBlock( l, "NiTexturingProperty" );
		if( iTexProp.isValid() )
		{
			QModelIndex iTex = nif->getIndex( iTexProp, texnames[currentTexSlot] );
			if( iTex.isValid() )
			{
				QModelIndex iTexSource = nif->getBlock( nif->getLink( iTex, "Source" ) );
				if( iTexSource.isValid() )
				{
					int texUVset = nif->get<int>( iTex, "UV Set" );
					iTexCoords = nif->getIndex( iShapeData, "UV Sets" ).child( texUVset, 0 );
					texsource = iTexSource;
					setTexCoords();
					updateGL();
					return;
				}
			}
		}
	}

}
