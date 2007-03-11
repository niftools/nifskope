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

#include "uvedit.h"

#include "../nifmodel.h"
#include "../niftypes.h"
#include "../gl/options.h"
#include "../gl/gltex.h"
#include "../gl/gltools.h"
#include "../nvtristrip/qtwrapper.h"

#include <math.h>
#include <gl/glext.h>

#define BASESIZE 512.0
#define GRIDSIZE 16.0
#define GRIDSEGS 4
#define ZOOMUNIT 64.0

static GLshort vertArray[4][2] = {
	{ 0, 0 }, { 1, 0 },
	{ 1, 1 }, { 0, 1 }
};

static GLshort texArray[4][2] = {
	{ 0, 1 }, { 1, 1 },
	{ 1, 0 }, { 0, 0 }
};

static GLdouble glUnit = ( 1.0 / BASESIZE );
static GLdouble glGridD = GRIDSIZE * glUnit;

UVWidget::UVWidget( QWidget * parent )
	: QGLWidget( QGLFormat( QGL::SampleBuffers ), parent )
{
	textures = new TexCache( this );

	glMode = RenderMode;

	zoom = 1.2;

	pos = QPoint( 0, 0 );

	mousePos = QPoint( -1000, -1000 );

	selectionRect = QRect( 0, 0, 0, 0 );

	selectionType = NoneSel;
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
	glDepthRange( 0.0, -1.0 );
	glEnable( GL_DEPTH_TEST );

	glEnable( GL_MULTISAMPLE );
	glDisable( GL_LIGHTING );

	qglClearColor( GLOptions::bgColor() );

	bindTexture( texfile );
	
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
	makeCurrent();

	glPushAttrib( GL_ALL_ATTRIB_BITS );
	
	glMatrixMode( GL_PROJECTION );
	glPushMatrix();
	glLoadIdentity();

	setupViewport( width(), height() );

	glMatrixMode( GL_MODELVIEW );
	glPushMatrix();
	glLoadIdentity();
	
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	glPushMatrix();
	glLoadIdentity();
	
	glEnable( GL_TEXTURE_2D );

	glTranslatef( -0.5f, -0.5f, 0.0f );

	glTranslatef( -1.0f, -1.0f, 0.0f );
	for( int i = 0; i < 3; i++ )
	{
		for( int j = 0; j < 3; j++ )
		{
			if( i == 1 && j == 1 ) {
				glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
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
	
	glLineWidth( 2.0f );
	glColor4f( 0.5f, 0.5f, 0.5f, 0.5f );
	glBegin( GL_LINE_STRIP );
	for( int i = 0; i < 5; i++ )
	{
		glVertex2sv( vertArray[i % 4] );
	}
	glEnd();

	glPopMatrix();



	glPushMatrix();
	glLoadIdentity();

	glLineWidth( 0.8f );
	glBegin( GL_LINES );
	int glGridMinX	= qRound( glViewRect[0] / glGridD );
	int glGridMaxX	= qRound( glViewRect[1] / glGridD );
	int glGridMinY	= qRound( glViewRect[2] / glGridD );
	int glGridMaxY	= qRound( glViewRect[3] / glGridD );
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



	if( !selectionRect.isNull() ) {
		drawSelectionRect();
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
	glScalef( 1.0f, -1.0f, 1.0f );
	glTranslatef( -0.5f, -0.5f, 0.0f );

	Color4 nlColor( GLOptions::nlColor() );
	nlColor.setAlpha( 0.5f );
	Color4 hlColor( GLOptions::hlColor() );
	hlColor.setAlpha( 0.5f );

	glColor( nlColor );
	GLfloat zPos = 0.0f;
	
	if( glMode == RenderMode || ( glMode == SelectionMode && selectionType != TexCoordSel ) ) {
		GLint glName = texcoords.size() + faces.size();

		glLineWidth( 1.0f );

		for( int i = faces.size() - 1; i > -1; i-- )
		{
			bool selected = selectedFaces.contains( i );

			if( glMode == RenderMode ) {

				if( selected ) {
					glColor( hlColor );
					zPos = -1.0f;
				}
			}

			if( glMode == SelectionMode ) {
				glLoadName( glName );
			}

			glBegin( GL_LINES );
			for( int j = 0; j < 3; j++ )
			{
				glVertex3f( texcoords[faces[i].tc[j]][0], texcoords[faces[i].tc[j]][1], zPos );
				glVertex3f( texcoords[faces[i].tc[(j + 1) % 3]][0], texcoords[faces[i].tc[(j + 1) % 3]][1], zPos );
			}
			glEnd();
			
			if( glMode == SelectionMode ) {
				glName--;
			}

			if( glMode == RenderMode ) {
				if( selected ) {
					glColor( nlColor );
					zPos = 0.0f;
				}
			}
		}
	}

	if( glMode == RenderMode || ( glMode == SelectionMode && selectionType != FaceSel ) ) {
		GLint glName = texcoords.size();

		glPointSize( 3.5f );

		for( int i = texcoords.size() - 1; i > -1; i-- )
		{
			bool selected = selectedTexCoords.contains( i );

			if( glMode == RenderMode ) {
				if( selected ) {
					glColor( hlColor );
					zPos = -1.0f;
				}
			}

			if( glMode == SelectionMode ) {
				glLoadName( glName );
			}

			glBegin( GL_POINTS );
			glVertex2f( texcoords[i][0], texcoords[i][1] );
			glEnd();
			
			if( glMode == SelectionMode ) {
				glName--;
			}

			
			if( glMode == RenderMode ) {
				if( selected ) {
					glColor( nlColor );
					zPos = 0.0f;
				}
			}
		}
	}

	glPopMatrix();
}

void UVWidget::drawSelectionRect()
{
	glMatrixMode( GL_MODELVIEW );
	glPushMatrix();
	glLoadIdentity();

	glScaled( glUnit, -glUnit, 1.0 );
	glTranslated( -1.0 * pos.x(), -1.0 * pos.y(), 0.0 );
	glScaled( zoom, zoom, 1.0 );
	glTranslated( -0.5 * width(), -0.5 * height(), 0.0 );

	GLint selRect[4];
	selRect[0] = selectionRect.left();
	selRect[1] = selectionRect.top();
	selRect[2] = selectionRect.right();
	selRect[3] = selectionRect.bottom();

	glHighlightColor();

	glBegin( GL_LINE_LOOP );
	for( int i = 0; i < 2; i++ ) {
		for( int j = 0; j < 2; j++ ) {
			glVertex2i( selRect[( ( i + j ) * 2 ) % 4], selRect[( ( i * 2 ) + 1 ) % 4] );
		}
	}
	glEnd();

	glPopMatrix();
}

void UVWidget::setupViewport( int width, int height )
{
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();

	glViewport( 0, 0, width, height );

	glOrtho( glViewRect[0], glViewRect[1], glViewRect[2], glViewRect[3], 0.0, 100.0 );
}

void UVWidget::updateViewRect( int width, int height )
{
	GLdouble glOffX	= glUnit * zoom * 0.5 * width;
	GLdouble glOffY	= glUnit * zoom * 0.5 * height;
	GLdouble glPosX = glUnit * pos.x();
	GLdouble glPosY = glUnit * pos.y();

	glViewRect[0] = - glOffX - glPosX;
	glViewRect[1] = + glOffX - glPosX;
	glViewRect[2] = - glOffY + glPosY;
	glViewRect[3] = + glOffY + glPosY;
}

int UVWidget::indexAt( const QPoint & hitPos, int (&hitArray)[128], GLdouble dx, GLdouble dy )
{
	makeCurrent();

	glPushAttrib( GL_ALL_ATTRIB_BITS );

	GLuint buffer[512];
	glSelectBuffer( 512, buffer );

	glRenderMode( GL_SELECT );
	glMode = SelectionMode;

	glMatrixMode( GL_PROJECTION );
	glPushMatrix();
	glLoadIdentity();

	glViewport( 0, 0, width(), height() );

	GLint viewport[4];
	glGetIntegerv( GL_VIEWPORT, viewport );

	gluPickMatrix( hitPos.x(), viewport[3] - hitPos.y(), dx, dy, viewport );

	glOrtho( glViewRect[0], glViewRect[1], glViewRect[2], glViewRect[3], 0.0, 100.0 );
	
	glInitNames();
	glPushName( 0 );
	
	drawTexCoords();
	
	GLint hits = glRenderMode( GL_RENDER );
	glMode = RenderMode;

	int selection = -1;
	QList< int > hitNames;

	if( hits > 0 ) {
		for( int i = 0; i < hits; i++ )
		{
			hitNames.append( buffer[i * 4 + 3] );
		}
		qSort< QList< GLint > >( hitNames );
		selection = hitNames[selectCycle % hits];

		for( int i = 0; i < hitNames.size(); i++ )
		{
			if( i > 511 ) {
				break;
			}
			hitArray[i] = hitNames[i];
		}
	}

	glMatrixMode( GL_MODELVIEW );
	glPopMatrix();

	glMatrixMode( GL_PROJECTION );
	glPopMatrix();
	
	glPopAttrib();

	return hits;
}

bool UVWidget::bindTexture( const QString & filename )
{
	GLuint mipmaps = 0;
	GLfloat max_anisotropy = 0.0f;

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
			if ( GLOptions::antialias() )
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

	return QSize( BASESIZE, BASESIZE );
}

void UVWidget::setSizeHint( const QSize & s )
{
	sHint = s;
}

QSize UVWidget::minimumSizeHint() const
{
	return QSize( BASESIZE, BASESIZE );
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

	int hits;
	int hitArray[128];
	int selection;
	selTypes prevSelType;

	switch( e->button() ) {
		case Qt::LeftButton:
			if( dPos.manhattanLength() > 3 ) {
				selectCycle = 0;
			}

			// clear the selection lock if alt is pressed
			if( e->modifiers() == Qt::AltModifier ) {

				if( dPos.manhattanLength() < 4 ) {
					selectCycle++;
				}

				selectionType = NoneSel;
			}

			hits = indexAt( e->pos(), hitArray, 4.0, 4.0 );

			// nothing was selected
			if( hits < 1 ) {
				selectionRect.setTopLeft( e->pos() );
				selectionRect.setBottomRight( e->pos() );

				if( e->modifiers() == Qt::ControlModifier ) {
					break;
				}

				selectCycle = 0;
				selectionType = NoneSel;
				selectedTexCoords.clear();
				selectedFaces.clear();

				break;
			}

			// Store previous selection type
			prevSelType = selectionType;

			// Select the currently cycled object 
			selection = hitArray[selectCycle % hits] - 1;

			// a texcoord was selected
			if( selection < texcoords.size() ) {
				selectionType = TexCoordSel;
			}

			// a face was selected
			else {
				selection -= texcoords.size();
				selectionType = FaceSel;
			}
			
			switch( e->modifiers() )
			{
				case Qt::ShiftModifier:
					selectedTexCoords.clear();
					selectedFaces.clear();

					if( selectionType == TexCoordSel ) {
						if( !texcoords2faces.contains( selection ) ) {
							selectionType = TexCoordSel;
							selectTexCoord( selection );

							break;
						}

						selection = texcoords2faces[selection];
					}

					selectionType = ElementSel;
					selectFace( selection );
					break;
					
				case Qt::ControlModifier:
					if( prevSelType == NoneSel || prevSelType == selectionType ) {
						selectObject( selection, true );
					}
					else {
						selectionType = prevSelType;
					}
					break;

				default:
					if( prevSelType == NoneSel || prevSelType == selectionType ) {
						if( !isSelected( selection ) ) {
							selectedTexCoords.clear();
							selectedFaces.clear();

							selectObject( selection );
						}
					}
					else {
						selectionType = prevSelType;
					}
					break;
			}

			break;
	}

	updateGL();
}

void UVWidget::mouseReleaseEvent( QMouseEvent * e )
{
	int hits;
	int hitArray[128];
	QList< int > hitList;
	QRect hitRect = selectionRect.normalized();
	QRect hitDivide = hitRect;
	QRect hitConquer;
	selectionRect = QRect( 0, 0, 0, 0 );

	switch( e->button() )
	{
		case Qt::LeftButton:

			if( hitRect.isNull() || selectionType != NoneSel ) {
				break;
			}

			selectionType = TexCoordSel;

			hitDivide.setWidth( 4 );
			hitConquer = hitRect.intersected( hitDivide );
			while( !hitConquer.isEmpty() ) {
				hits = indexAt( hitConquer.center(), hitArray, hitConquer.width(), hitConquer.height() );
				for( int i = 0; i < hits; i++ )
				{
					hitList.append( hitArray[i] );
				}

				hitDivide.adjust( 4, 0, 4, 0 );
				hitConquer = hitRect.intersected( hitDivide );
			}

			if( hitList.empty() ) {
				selectionType = NoneSel;
				break;
			}
			else {
				for( int i = 0; i < hitList.size(); i++ ) {
					if( hitList[i] - 1 < texcoords.size() ) {
						selectTexCoord( hitList[i] - 1 );
					}
				}
			}
			break;
	}

	updateGL();
}

void UVWidget::mouseMoveEvent( QMouseEvent * e )
{
	QPoint dPos( e->pos() - mousePos );
	GLdouble moveX, moveY;

	switch( e->buttons()) {
		case Qt::LeftButton:
			if( selectionType != NoneSel ) {
				moveX = glUnit * zoom * dPos.x();
				moveY = glUnit * zoom * dPos.y();

				foreach( int tc, selectedTexCoords )
				{
					texcoords[tc][0] += moveX;
					texcoords[tc][1] += moveY;
				}

				updateNif();
			}
			else {
				selectionRect.setBottomRight( e->pos() );
			}

			break;

		case Qt::MidButton:
			pos += zoom * dPos;
			updateViewRect( width(), height() );
			break;

		case Qt::RightButton:
			zoom *= 1.0 + ( dPos.y() / ZOOMUNIT );

			if( zoom < 0.1 ) {
				zoom = 0.1;
			}
			else if( zoom > 10.0 ) {
				zoom = 10.0;
			}

			updateViewRect( width(), height() );

			break;

		default:
			return;
	}

	mousePos = e->pos();

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
	nif = nifModel;
	idx = nifIndex;

	textures->setNifFolder( nif->getFolder() );

	QModelIndex iData = nif->getBlock( nif->getLink( idx, "Data" ) );
	if( nif->inherits( iData, "NiTriBasedGeomData" ) )
	{
		QModelIndex iUV = nif->getIndex( nif->getBlock( iData ), "UV Sets" );
		if( !iUV.isValid() ) {
			return false;
		}

		iTexCoords = iUV.child( 0, 0 );
		if( nif->isArray( iTexCoords ) ) {
			texcoords = nif->getArray< Vector2 >( iTexCoords );
		}
		else {
			return false;
		}
		
		QVector< Triangle > tris;
		
		if( nif->isNiBlock( idx, "NiTriShape" ) ) {
			tris = nif->getArray< Triangle >( iData, "Triangles" );
		}
		else if( nif->isNiBlock( idx, "NiTriStrips" ) ) {
			QModelIndex iPoints = nif->getIndex( iData, "Points" );
			if( iPoints.isValid() )
			{
				QList< QVector< quint16 > > strips;
				for( int r = 0; r < nif->rowCount( iPoints ); r++ )
				{
					strips.append( nif->getArray< quint16 >( iPoints.child( r, 0 ) ) );
				}

				tris = triangulate( strips );
			}
			else {
				return false;
			}
		}
		else {
			return false;
		}

		QMutableVectorIterator< Triangle > itri( tris );
		while ( itri.hasNext() )
		{
			Triangle & t = itri.next();

			int fIdx = faces.size();
			faces.append( face( fIdx, t[0], t[1], t[2] ) );

			for( int i = 0; i < 3; i++ ) {
				texcoords2faces.insertMulti( t[i], fIdx );
			}
		}
	}

	foreach( qint32 l, nif->getLinkArray( idx, "Properties" ) )
	{
		QModelIndex iProp = nif->getBlock( l, "NiProperty" );
		if( iProp.isValid() ) {
			if( nif->isNiBlock( iProp, "NiTexturingProperty" ) ) {
				foreach ( qint32 sl, nif->getChildLinks( nif->getBlockNumber( iProp ) ) )
				{
					QModelIndex iTexSource = nif->getBlock( sl, "NiSourceTexture" );
					if( iTexSource.isValid() )
					{
						texfile = TexCache::find( nif->get<QString>( iTexSource, "File Name" ) , nif->getFolder() );
						return true;
					}
				}
			}
		}
	}

	return false;
}

void UVWidget::updateNif()
{
	if( iTexCoords.isValid() ) {
		nif->setArray< Vector2 >( iTexCoords, texcoords );
	}
}

bool UVWidget::isSelected( int index )
{
	switch( selectionType )
	{
		case TexCoordSel:
			return selectedTexCoords.contains( index );

		case FaceSel:
			return selectedFaces.contains( index );
			break;

		default:
			return false;
	}
}

void UVWidget::selectObject( int index, bool toggle )
{
	switch( selectionType )
	{
		case TexCoordSel:
			selectTexCoord( index, toggle );
			break;

		case FaceSel:
			selectFace( index, toggle );
			break;

		default:
			return;
	}
}

void UVWidget::selectTexCoord( int index, bool toggle )
{
	if( !selectedTexCoords.contains( index ) ) {
		selectedTexCoords.append( index );
	}
	else if( toggle ) {
		selectedTexCoords.removeAll( index );
	}
}

void UVWidget::selectFace( int index, bool toggle )
{
	if( !selectedFaces.contains( index ) ) {
		selectedFaces.append( index );

		if( selectionType == FaceSel || selectionType == ElementSel ) {
			for( int i = 0; i < 3; i++ ) {
				selectTexCoord( faces[index].tc[i] );
			}
		}

		if( selectionType == ElementSel ) {
			for( int i = 0; i < 3; i++ )
			{
				QList< int > neighbors = texcoords2faces.values( faces[index].tc[i] );
				foreach( int j, neighbors )
				{
					if( i == index ) {
						continue;
					}

					selectFace( j );
				}
			}
		}

	}
	else if( toggle ) {
		selectedFaces.removeAll( index );

		if( selectionType == FaceSel ) {
			for( int i = 0; i < 3; i++ ) {
				QList< int > tcFaces = texcoords2faces.values( faces[index].tc[i] );
				bool deselect = true;
				foreach( int j, tcFaces )
				{
					if( selectedFaces.contains( j ) ) {
						deselect = false;
						break;
					}
				}
				if( deselect ) {
					selectTexCoord( faces[index].tc[i], true );
				}
			}

			if( selectedFaces.empty() ) {
				selectionType = NoneSel;
			}
		}
	}
}
