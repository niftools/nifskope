/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005-2015, NIF File Format Library and Tools
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

#include "uvedit.h"

#include "message.h"
#include "nifskope.h"
#include "gl/gltex.h"
#include "gl/gltools.h"
#include "model/nifmodel.h"
#include "ui/settingsdialog.h"

#include "lib/nvtristripwrapper.h"

#include <QUndoStack> // QUndoCommand Inherited
#include <QActionGroup>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QInputDialog>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QPushButton>
#include <QSettings>

// TODO: Determine the necessity of this
// Appears to be used solely for gluErrorString
// There may be some Qt alternative
#ifdef __APPLE__
	#include <OpenGL/glu.h>
#else
	#include <GL/glu.h>
#endif

#define BASESIZE 512.0
#define GRIDSIZE 16.0
#define GRIDSEGS 4
#define ZOOMUNIT -64.0
#define MINZOOM 0.1
#define MAXZOOM 20.0
#define MAXSCALE 10.0
#define MAXTRANS 10.0


UVWidget * UVWidget::createEditor( NifModel * nif, const QModelIndex & idx )
{
	UVWidget * uvw = new UVWidget;
	uvw->setAttribute( Qt::WA_DeleteOnClose );

	if ( !uvw->setNifData( nif, idx ) ) {
		qCWarning( nsSpell ) << tr( "Could not load texture data for UV editor." );
		delete uvw;
		return nullptr;
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

static GLdouble glUnit  = ( 1.0 / BASESIZE );
static GLdouble glGridD = GRIDSIZE * glUnit;

QStringList UVWidget::texnames = {
	"Base Texture", "Dark Texture", "Detail Texture",
	"Gloss Texture", "Glow Texture", "Bump Map Texture",
	"Decal 0 Texture", "Decal 1 Texture", "Decal 2 Texture",
	"Decal 3 Texture"
};


UVWidget::UVWidget( QWidget * parent )
	: QGLWidget( QGLFormat( QGL::SampleBuffers ), parent, 0, Qt::Tool ), undoStack( new QUndoStack( this ) )
{
	setWindowTitle( tr( "UV Editor" ) );
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
	connect( aSelectAll, &QAction::triggered, this, &UVWidget::selectAll );
	addAction( aSelectAll );

	QAction * aSelectNone = new QAction( tr( "Select &None" ), this );
	connect( aSelectNone, &QAction::triggered, this, &UVWidget::selectNone );
	addAction( aSelectNone );

	QAction * aSelectFaces = new QAction( tr( "Select &Faces" ), this );
	connect( aSelectFaces, &QAction::triggered, this, &UVWidget::selectFaces );
	addAction( aSelectFaces );

	QAction * aSelectConnected = new QAction( tr( "Select &Connected" ), this );
	connect( aSelectConnected, &QAction::triggered, this, &UVWidget::selectConnected );
	addAction( aSelectConnected );

	QAction * aScaleSelection = new QAction( tr( "&Scale and Translate Selected" ), this );
	aScaleSelection->setShortcut( QKeySequence( "Alt+S" ) );
	connect( aScaleSelection, &QAction::triggered, this, &UVWidget::scaleSelection );
	addAction( aScaleSelection );

	QAction * aRotateSelection = new QAction( tr( "&Rotate Selected" ), this );
	aRotateSelection->setShortcut( QKeySequence( "Alt+R" ) );
	connect( aRotateSelection, &QAction::triggered, this, &UVWidget::rotateSelection );
	addAction( aRotateSelection );

	aSep = new QAction( this );
	aSep->setSeparator( true );
	addAction( aSep );

	aTextureBlend = new QAction( tr( "Texture Alpha Blending" ), this );
	aTextureBlend->setCheckable( true );
	aTextureBlend->setChecked( true );
	connect( aTextureBlend, &QAction::toggled, this, &UVWidget::updateGL );
	addAction( aTextureBlend );

	updateSettings();

	connect( NifSkope::getOptions(), &SettingsDialog::saveSettings, this, &UVWidget::updateSettings );
	connect( NifSkope::getOptions(), &SettingsDialog::update3D, this, &UVWidget::updateGL );
}

UVWidget::~UVWidget()
{
	delete textures;
	nif = nullptr;
}

void UVWidget::updateSettings()
{
	QSettings settings;
	settings.beginGroup( "Settings/Render/Colors/" );

	cfg.background = settings.value( "Background" ).value<QColor>();
	cfg.highlight = settings.value( "Highlight" ).value<QColor>();
	cfg.wireframe = settings.value( "Wireframe" ).value<QColor>();

	settings.endGroup();
}

void UVWidget::initializeGL()
{
	glMatrixMode( GL_MODELVIEW );

	initializeTextureUnits( context()->contextHandle() );

	glShadeModel( GL_SMOOTH );
	//glShadeModel( GL_LINE_SMOOTH );

	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glEnable( GL_BLEND );

	glDepthFunc( GL_LEQUAL );
	glEnable( GL_DEPTH_TEST );

	glEnable( GL_MULTISAMPLE );
	glDisable( GL_LIGHTING );

	qglClearColor( cfg.background );

	if ( !texfile.isEmpty() )
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
		qDebug() << "GL ERROR (init) : " << (const char *)gluErrorString( err );
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

	qglClearColor( cfg.background );
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

	if ( !texfile.isEmpty() )
		bindTexture( texfile );
	else
		bindTexture( texsource );

	glTranslatef( -0.5f, -0.5f, 0.0f );

	glTranslatef( -1.0f, -1.0f, 0.0f );

	for ( int i = 0; i < 3; i++ ) {
		for ( int j = 0; j < 3; j++ ) {
			if ( i == 1 && j == 1 ) {
				glColor4f( 0.75f, 0.75f, 0.75f, 1.0f );
			} else {
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
	int glGridMinX = qRound( qMin( glViewRect[0], glViewRect[1] ) / glGridD );
	int glGridMaxX = qRound( qMax( glViewRect[0], glViewRect[1] ) / glGridD );
	int glGridMinY = qRound( qMin( glViewRect[2], glViewRect[3] ) / glGridD );
	int glGridMaxY = qRound( qMax( glViewRect[2], glViewRect[3] ) / glGridD );

	for ( int i = glGridMinX; i < glGridMaxX; i++ ) {
		GLdouble glGridPos = glGridD * i;

		if ( ( i % ( GRIDSEGS * GRIDSEGS ) ) == 0 ) {
			glLineWidth( 1.4f );
			glColor4f( 1.0f, 1.0f, 1.0f, 0.4f );
		} else if ( zoom > ( GRIDSEGS * GRIDSEGS / 2.0 ) ) {
			continue;
		} else if ( ( i % GRIDSEGS ) == 0 ) {
			glLineWidth( 1.2f );
			glColor4f( 1.0f, 1.0f, 1.0f, 0.2f );
		} else if ( zoom > ( GRIDSEGS / 2.0 ) ) {
			continue;
		} else {
			glLineWidth( 0.8f );
			glColor4f( 1.0f, 1.0f, 1.0f, 0.1f );
		}

		glVertex2d( glGridPos, glViewRect[2] );
		glVertex2d( glGridPos, glViewRect[3] );
	}

	for ( int i = glGridMinY; i < glGridMaxY; i++ ) {
		GLdouble glGridPos = glGridD * i;

		if ( ( i % ( GRIDSEGS * GRIDSEGS ) ) == 0 ) {
			glLineWidth( 1.4f );
			glColor4f( 1.0f, 1.0f, 1.0f, 0.4f );
		} else if ( zoom > ( GRIDSEGS * GRIDSEGS / 2.0 ) ) {
			continue;
		} else if ( ( i % GRIDSEGS ) == 0 ) {
			glLineWidth( 1.2f );
			glColor4f( 1.0f, 1.0f, 1.0f, 0.2f );
		} else if ( zoom > ( GRIDSEGS / 2.0 ) ) {
			continue;
		} else {
			glLineWidth( 0.8f );
			glColor4f( 1.0f, 1.0f, 1.0f, 0.1f );
		}

		glVertex2d( glViewRect[0], glGridPos );
		glVertex2d( glViewRect[1], glGridPos );
	}

	glEnd();

	glPopMatrix();

	drawTexCoords();

	glDisable( GL_DEPTH_TEST );
	glDepthMask( GL_FALSE );

	if ( !selectRect.isNull() ) {
		glLoadIdentity();
		glColor( Color4( cfg.highlight ) );
		glBegin( GL_LINE_LOOP );
		glVertex( mapToContents( selectRect.topLeft() ) );
		glVertex( mapToContents( selectRect.topRight() ) );
		glVertex( mapToContents( selectRect.bottomRight() ) );
		glVertex( mapToContents( selectRect.bottomLeft() ) );
		glEnd();
	}

	if ( !selectPoly.isEmpty() ) {
		glLoadIdentity();
		glColor( Color4( cfg.highlight ) );
		glBegin( GL_LINE_LOOP );
		for ( const QPoint& p : selectPoly ) {
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

	Color4 nlColor( cfg.wireframe );
	nlColor.setAlpha( 0.5f );
	Color4 hlColor( cfg.highlight );
	hlColor.setAlpha( 0.5f );

	glLineWidth( 1.0f );
	glPointSize( 3.5f );

	glEnable( GL_BLEND );
	glEnable( GL_DEPTH_TEST );
	glDepthFunc( GL_LEQUAL );
	glDepthMask( GL_TRUE );

	float z;

	// draw triangle edges
	for ( int i = 0; i < faces.size(); i++ ) {
		glBegin( GL_LINE_LOOP );

		for ( int j = 0; j < 3; j++ ) {
			int x = faces[i].tc[j];

			if ( selection.contains( x ) ) {
				glColor( Color3( hlColor ) );
				z = 1.0f;
			} else {
				glColor( Color3( nlColor ) );
				z = 0.0f;
			}

			glVertex( Vector3( texcoords[x], z ) );
		}

		glEnd();
	}

	// draw points

	glBegin( GL_POINTS );

	for ( int i = 0; i < texcoords.size(); i++ ) {
		if ( selection.contains( i ) ) {
			glColor( Color3( hlColor ) );
			z = 1.0f;
		} else {
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
	GLdouble glOffX = glUnit * zoom * 0.5 * width;
	GLdouble glOffY = glUnit * zoom * 0.5 * height;
	GLdouble glPosX = glUnit * pos.x();
	GLdouble glPosY = glUnit * pos.y();

	glViewRect[0] = -glOffX - glPosX;
	glViewRect[1] = +glOffX - glPosX;
	glViewRect[2] = +glOffY + glPosY;
	glViewRect[3] = -glOffY + glPosY;
}

QPoint UVWidget::mapFromContents( const Vector2 & v ) const
{
	float x = ( ( v[0] - 0.5 ) - glViewRect[ 0 ] ) / ( glViewRect[ 1 ] - glViewRect[ 0 ] ) * width();
	float y = ( ( v[1] - 0.5 ) - glViewRect[ 2 ] ) / ( glViewRect[ 3 ] - glViewRect[ 2 ] ) * height() * ( -1 ) + height();

	return QPointF( x, y ).toPoint();
}

Vector2 UVWidget::mapToContents( const QPoint & p ) const
{
	float x = ( (float)p.x() / (float)width() ) * ( glViewRect[ 1 ] - glViewRect[ 0 ] ) + glViewRect[ 0 ];
	float y = ( (float)p.y() / (float)height() ) * ( glViewRect[ 2 ] - glViewRect[ 3 ] ) + glViewRect[ 3 ];
	return Vector2( x, y );
}

QVector<int> UVWidget::indices( const QPoint & p ) const
{
	return indices( QRect( p - QPoint( 2, 2 ), QSize( 5, 5 ) ) );
}

QVector<int> UVWidget::indices( const QRegion & region ) const
{
	QList<int> hits;

	for ( int i = 0; i < texcoords.count(); i++ ) {
		if ( region.contains( mapFromContents( texcoords[ i ] ) ) )
			hits << i;
	}

	return hits.toVector();
}

bool UVWidget::bindTexture( const QString & filename )
{
	GLuint mipmaps = 0;
	mipmaps = textures->bind( filename );

	if ( mipmaps ) {
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
	mipmaps = textures->bind( iSource );

	if ( mipmaps ) {
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
	if ( sHint.isValid() ) {
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

	if ( e->button() == Qt::LeftButton ) {
		QVector<int> hits = indices( mousePos );

		if ( hits.isEmpty() ) {
			if ( !e->modifiers().testFlag( Qt::ShiftModifier ) )
				selectNone();

			if ( e->modifiers().testFlag( Qt::AltModifier ) ) {
				selectPoly << e->pos();
			} else {
				selectRect.setTopLeft( mousePos );
				selectRect.setBottomRight( mousePos );
			}
		} else {
			if ( dPos.manhattanLength() > 4 )
				selectCycle = 0;
			else
				selectCycle++;

			int h = hits[ selectCycle % hits.count() ];

			if ( !e->modifiers().testFlag( Qt::ShiftModifier ) ) {
				if ( !isSelected( h ) )
					selectNone();

				select( h );
			} else {
				select( h, !isSelected( h ) );
			}

			if ( selection.isEmpty() ) {
				setCursor( QCursor( Qt::CrossCursor ) );
			} else {
				setCursor( QCursor( Qt::SizeAllCursor ) );
			}
		}
	}

	updateGL();
}

void UVWidget::mouseMoveEvent( QMouseEvent * e )
{
	QPoint dPos( e->pos() - mousePos );

	switch ( e->buttons() ) {
	case Qt::LeftButton:

		if ( !selectRect.isNull() ) {
			selectRect.setBottomRight( e->pos() );
		} else if ( !selectPoly.isEmpty() ) {
			selectPoly << e->pos();
		} else {
			auto dPosX = glUnit * zoom * dPos.x();
			auto dPosY = glUnit * zoom * dPos.y();

			if ( kbd[Qt::Key_X] )
				dPosY = 0.0;
			if ( kbd[Qt::Key_Y] )
				dPosX = 0.0;

			moveSelection( dPosX, dPosY );
		}
		break;

	case Qt::MidButton:
		pos += zoom * QPointF( dPos.x(), -dPos.y() );
		updateViewRect( width(), height() );

		setCursor( QCursor( Qt::ClosedHandCursor ) );

		break;

	case Qt::RightButton:
		zoom *= 1.0 + ( dPos.y() / ZOOMUNIT );

		if ( zoom < MINZOOM ) {
			zoom = MINZOOM;
		} else if ( zoom > MAXZOOM ) {
			zoom = MAXZOOM;
		}

		updateViewRect( width(), height() );

		setCursor( QCursor( Qt::SizeVerCursor ) );

		break;

	default:

		if ( indices( e->pos() ).count() ) {
			setCursor( QCursor( Qt::PointingHandCursor ) );
		} else {
			setCursor( QCursor( Qt::CrossCursor ) );
		}
		return;
	}

	mousePos = e->pos();

	updateGL();
}

void UVWidget::mouseReleaseEvent( QMouseEvent * e )
{
	switch ( e->button() ) {
	case Qt::LeftButton:

		if ( !selectRect.isNull() ) {
			select( QRegion( selectRect.normalized() ) );
			selectRect = QRect();
		} else if ( !selectPoly.isEmpty() ) {
			if ( selectPoly.size() > 2 ) {
				select( QRegion( QPolygon( selectPoly.toVector() ) ) );
			}

			selectPoly.clear();
		}

		break;
	default:
		break;
	}

	if ( indices( e->pos() ).count() ) {
		setCursor( QCursor( Qt::ArrowCursor ) );
	} else {
		setCursor( QCursor( Qt::CrossCursor ) );
	}

	updateGL();
}

void UVWidget::wheelEvent( QWheelEvent * e )
{
	switch ( e->modifiers() ) {
	case Qt::NoModifier:
		zoom *= 1.0 + ( e->delta() / 8.0 ) / ZOOMUNIT;

		if ( zoom < MINZOOM ) {
			zoom = MINZOOM;
		} else if ( zoom > MAXZOOM ) {
			zoom = MAXZOOM;
		}

		updateViewRect( width(), height() );

		break;
	}

	updateGL();
}

void UVWidget::keyPressEvent( QKeyEvent * e )
{
	switch ( e->key() ) {
	case Qt::Key_X:
	case Qt::Key_Y:
		kbd[e->key()] = true;
		break;
	default:
		e->ignore();
		break;
	}
}

void UVWidget::keyReleaseEvent( QKeyEvent * e )
{
	switch ( e->key() ) {
	case Qt::Key_X:
	case Qt::Key_Y:
		kbd[e->key()] = false;
		break;
	default:
		e->ignore();
		break;
	}
}

bool UVWidget::setNifData( NifModel * nifModel, const QModelIndex & nifIndex )
{
	if ( nif ) {
		// disconnect( nif ) may not work with new Qt5 syntax...
		// it says the calls need to remain symmetric to the connect() ones.
		// Otherwise, use QMetaObject::Connection
		//disconnect( nif );
		this->disconnect();
	}

	undoStack->clear();

	nif = nifModel;
	iShape = nifIndex;
	isDataOnSkin = false;

	// Version dependent actions
	if ( nif && nif->getVersionNumber() != 0x14020007 ) {
		coordSetGroup = new QActionGroup( this );
		connect( coordSetGroup, &QActionGroup::triggered, this, &UVWidget::selectCoordSet );

		coordSetSelect = new QMenu( tr( "Select Coordinate Set" ) );
		addAction( coordSetSelect->menuAction() );
		connect( coordSetSelect, &QMenu::aboutToShow, this, &UVWidget::getCoordSets );

		texSlotGroup = new QActionGroup( this );
		connect( texSlotGroup, &QActionGroup::triggered, this, &UVWidget::selectTexSlot );

		menuTexSelect = new QMenu( tr( "Select Texture Slot" ) );
		addAction( menuTexSelect->menuAction() );
		connect( menuTexSelect, &QMenu::aboutToShow, this, &UVWidget::getTexSlots );
	}

	if ( nif ) {
		connect( nif, &NifModel::modelReset, this, &UVWidget::close );
		connect( nif, &NifModel::destroyed, this, &UVWidget::close );
		connect( nif, &NifModel::dataChanged, this, &UVWidget::nifDataChanged );
		connect( nif, &NifModel::rowsRemoved, this, &UVWidget::nifDataChanged );
	}

	if ( !nif )
		return false;

	textures->setNifFolder( nif->getFolder() );

	iShapeData = nif->getBlock( nif->getLink( iShape, "Data" ) );
	if ( nif->getVersionNumber() == 0x14020007 && nif->getUserVersion2() >= 100 ) {
		iShapeData = nif->getIndex( iShape, "Vertex Data" );

		auto vf = nif->get<BSVertexDesc>( iShape, "Vertex Desc" );
		if ( (vf & VertexFlags::VF_SKINNED) && nif->getUserVersion2() == 100 ) {
			// Skinned SSE
			auto skinID = nif->getLink( nif->getIndex( iShape, "Skin" ) );
			auto partID = nif->getLink( nif->getBlock( skinID, "NiSkinInstance" ), "Skin Partition" );
			iPartBlock = nif->getBlock( partID, "NiSkinPartition" );
			if ( !iPartBlock.isValid() )
				return false;

			isDataOnSkin = true;

			iShapeData = nif->getIndex( iPartBlock, "Vertex Data" );
		}
	}

	if ( nif->inherits( iShapeData, "NiTriBasedGeomData" ) ) {
		iTexCoords = nif->getIndex( iShapeData, "UV Sets" ).child( 0, 0 );

		if ( !iTexCoords.isValid() || !nif->rowCount( iTexCoords ) ) {
			return false;
		}

		if ( !setTexCoords() ) {
			return false;
		}
	} else if ( nif->inherits( iShape, "BSTriShape" ) ) {
		int numVerts = 0;
		if ( !isDataOnSkin )
			numVerts = nif->get<int>( iShape, "Num Vertices" );
		else
			numVerts = nif->get<uint>( iPartBlock, "Data Size" ) / nif->get<uint>( iPartBlock, "Vertex Size" );

		for ( int i = 0; i < numVerts; i++ ) {
			texcoords << nif->get<Vector2>( nif->index( i, 0, iShapeData ), "UV" );
		}

		// Fake index so that isValid() checks do not fail
		iTexCoords = iShape;

		if ( !setTexCoords() )
			return false;
	}

	auto props = nif->getLinkArray( iShape, "Properties" );
	props << nif->getLink( iShape, "Shader Property" );
	for ( const auto l : props )
	{
		QModelIndex iTexProp = nif->getBlock( l, "NiTexturingProperty" );

		if ( iTexProp.isValid() ) {
			while ( currentTexSlot < texnames.size() ) {
				iTex = nif->getIndex( iTexProp, texnames[currentTexSlot] );

				if ( iTex.isValid() ) {
					QModelIndex iTexSource = nif->getBlock( nif->getLink( iTex, "Source" ) );

					if ( iTexSource.isValid() ) {
						currentCoordSet = nif->get<int>( iTex, "UV Set" );
						iTexCoords = nif->getIndex( iShapeData, "UV Sets" ).child( currentCoordSet, 0 );
						texsource  = iTexSource;

						if ( setTexCoords() )
							return true;
					}
				} else {
					currentTexSlot++;
				}
			}
		} else {
			iTexProp = nif->getBlock( l, "NiTextureProperty" );

			if ( iTexProp.isValid() ) {
				QModelIndex iTexSource = nif->getBlock( nif->getLink( iTexProp, "Image" ) );

				if ( iTexSource.isValid() ) {
					//texfile = TexCache::find( nif->get<QString>( iTexSource, "File Name" ) , nif->getFolder() );
					texsource = iTexSource;
					return true;
				}
			} else {
				// TODO: use the BSShaderTextureSet
				iTexProp = nif->getBlock( l, "BSShaderPPLightingProperty" );

				if ( !iTexProp.isValid() )
					iTexProp = nif->getBlock( l, "BSLightingShaderProperty" );

				if ( iTexProp.isValid() ) {
					QModelIndex iTexSource = nif->getBlock( nif->getLink( iTexProp, "Texture Set" ) );

					if ( iTexSource.isValid() ) {
						// Assume that a FO3 mesh never has embedded textures...
						//texsource = iTexSource;
						//return true;
						QModelIndex iTextures = nif->getIndex( iTexSource, "Textures" );

						if ( iTextures.isValid() ) {
							texfile = TexCache::find( nif->get<QString>( iTextures.child( 0, 0 ) ), nif->getFolder() );
							return true;
						}
					}
				} else {
					iTexProp = nif->getBlock( l, "BSEffectShaderProperty" );

					if ( iTexProp.isValid() ) {
						QString texture = nif->get<QString>( iTexProp, "Source Texture" );

						if ( !texture.isEmpty() ) {
							texfile = TexCache::find( texture, nif->getFolder() );
							return true;
						}
					}
				}
			}
		}
	}

	return true;
}

bool UVWidget::setTexCoords()
{
	if ( nif->inherits( iShape, "NiTriBasedGeom" ) )
		texcoords = nif->getArray<Vector2>( iTexCoords );

	QVector<Triangle> tris;

	if ( nif->isNiBlock( iShapeData, "NiTriShapeData" ) ) {
		tris = nif->getArray<Triangle>( iShapeData, "Triangles" );
	} else if ( nif->isNiBlock( iShapeData, "NiTriStripsData" ) ) {
		QModelIndex iPoints = nif->getIndex( iShapeData, "Points" );

		if ( !iPoints.isValid() )
			return false;

		for ( int r = 0; r < nif->rowCount( iPoints ); r++ ) {
			tris += triangulate( nif->getArray<quint16>( iPoints.child( r, 0 ) ) );
		}
	} else if ( nif->inherits( iShape, "BSTriShape" ) ) {
		if ( !isDataOnSkin ) {
			tris = nif->getArray<Triangle>( iShape, "Triangles" );
		} else {
			auto partIdx = nif->getIndex( iPartBlock, "Partition" );
			for ( int i = 0; i < nif->rowCount( partIdx ); i++ ) {
				tris << nif->getArray<Triangle>( nif->index( i, 0, partIdx ), "Triangles" );
			}
		}
		
	}

	if ( tris.isEmpty() )
		return false;

	QVectorIterator<Triangle> itri( tris );

	while ( itri.hasNext() ) {
		const Triangle & t = itri.next();

		int fIdx = faces.size();
		faces.append( face( fIdx, t[0], t[1], t[2] ) );

		for ( int i = 0; i < 3; i++ ) {
			texcoords2faces.insertMulti( t[i], fIdx );
		}
	}

	return true;
}

void UVWidget::updateNif()
{
	if ( nif && iTexCoords.isValid() ) {
		disconnect( nif, &NifModel::dataChanged, this, &UVWidget::nifDataChanged );
		nif->setState( BaseModel::Processing );

		if ( nif->inherits( iShapeData, "NiTriBasedGeomData" ) ) {
			nif->setArray<Vector2>( iTexCoords, texcoords );
		} else if ( nif->inherits( iShape, "BSTriShape" ) ) {
			int numVerts = 0;
			if ( !isDataOnSkin )
				numVerts = nif->get<int>( iShape, "Num Vertices" );
			else
				numVerts = nif->get<uint>( iPartBlock, "Data Size" ) / nif->get<uint>( iPartBlock, "Vertex Size" );

			for ( int i = 0; i < numVerts; i++ ) {
				nif->set<HalfVector2>( nif->index( i, 0, iShapeData ), "UV", HalfVector2( texcoords.value( i ) ) );
			}

			nif->dataChanged( iShape, iShape );
		}
		
		nif->restoreState();
		connect( nif, &NifModel::dataChanged, this, &UVWidget::nifDataChanged );
	}
}

void UVWidget::nifDataChanged( const QModelIndex & idx )
{
	if ( !nif || !iShape.isValid() || !iShapeData.isValid() || !iTexCoords.isValid() ) {
		close();
		return;
	}

	if ( nif->getBlock( idx ) == iShapeData ) {
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

class UVWSelectCommand final : public QUndoCommand
{
public:
	UVWSelectCommand( UVWidget * w, const QList<int> & nSel ) : QUndoCommand(), uvw( w ), newSelection( nSel )
	{
		setText( "Select" );
	}

	int id() const override final
	{
		return 0;
	}

	bool mergeWith( const QUndoCommand * cmd ) override final
	{
		if ( cmd->id() == id() ) {
			newSelection = static_cast<const UVWSelectCommand *>( cmd )->newSelection;
			return true;
		}

		return false;
	}

	void redo() override final
	{
		oldSelection = uvw->selection;
		uvw->selection = newSelection;
		uvw->updateGL();
	}

	void undo() override final
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
	QList<int> sel = this->selection;

	if ( yes ) {
		if ( !sel.contains( index ) )
			sel.append( index );
	} else {
		sel.removeAll( index );
	}

	undoStack->push( new UVWSelectCommand( this, sel ) );
}

void UVWidget::select( const QRegion & r, bool add )
{
	QList<int> sel( add ? this->selection : QList<int>() );
	for ( const auto s : indices( r ) ) {
		if ( !sel.contains( s ) )
			sel.append( s );
	}
	undoStack->push( new UVWSelectCommand( this, sel ) );
}

void UVWidget::selectNone()
{
	undoStack->push( new UVWSelectCommand( this, QList<int>() ) );
}

void UVWidget::selectAll()
{
	QList<int> sel;

	for ( int s = 0; s < texcoords.count(); s++ )
		sel << s;

	undoStack->push( new UVWSelectCommand( this, sel ) );
}

void UVWidget::selectFaces()
{
	QList<int> sel = this->selection;
	for ( const auto s : QList<int>( sel ) ) {
		for ( const auto f : texcoords2faces.values( s ) ) {
			for ( int i = 0; i < 3; i++ ) {
				if ( !sel.contains( faces[f].tc[i] ) )
					sel.append( faces[f].tc[i] );
			}
		}
	}
	undoStack->push( new UVWSelectCommand( this, sel ) );
}

void UVWidget::selectConnected()
{
	QList<int> sel = this->selection;
	bool more = true;

	while ( more ) {
		more = false;
		for ( const auto s : QList<int>( sel ) ) {
			for ( const auto f :texcoords2faces.values( s ) ) {
				for ( int i = 0; i < 3; i++ ) {
					if ( !sel.contains( faces[f].tc[i] ) ) {
						sel.append( faces[f].tc[i] );
						more = true;
					}
				}
			}
		}
	}

	undoStack->push( new UVWSelectCommand( this, sel ) );
}

class UVWMoveCommand final : public QUndoCommand
{
public:
	UVWMoveCommand( UVWidget * w, double dx, double dy ) : QUndoCommand(), uvw( w ), move( dx, dy )
	{
		setText( "Move" );
	}

	int id() const override final
	{
		return 1;
	}

	bool mergeWith( const QUndoCommand * cmd ) override final
	{
		if ( cmd->id() == id() ) {
			move += static_cast<const UVWMoveCommand *>( cmd )->move;
			return true;
		}

		return false;
	}

	void redo() override final
	{
		for ( const auto tc : uvw->selection ) {
			uvw->texcoords[tc] += move;
		}
		uvw->updateNif();
		uvw->updateGL();
	}

	void undo() override final
	{
		for ( const auto tc : uvw->selection ) {
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

// For mouse-driven scaling: insert state/flag for scaling mode
// get difference in mouse coords, scale everything around centre

//! A class to perform scaling of UV coordinates
class UVWScaleCommand final : public QUndoCommand
{
public:
	UVWScaleCommand( UVWidget * w, float sX, float sY ) : QUndoCommand(), uvw( w ), scaleX( sX ), scaleY( sY )
	{
		setText( "Scale" );
	}

	int id() const override final
	{
		return 2;
	}

	bool mergeWith( const QUndoCommand * cmd ) override final
	{
		if ( cmd->id() == id() ) {
			scaleX *= static_cast<const UVWScaleCommand *>( cmd )->scaleX;
			scaleY *= static_cast<const UVWScaleCommand *>( cmd )->scaleY;
			return true;
		}

		return false;
	}

	void redo() override final
	{
		Vector2 centre;
		for ( const auto i : uvw->selection ) {
			centre += uvw->texcoords[i];
		}
		centre /= uvw->selection.size();

		for ( const auto i : uvw->selection ) {
			uvw->texcoords[i] -= centre;
		}

		for ( const auto i : uvw->selection ) {
			Vector2 temp( uvw->texcoords[i] );
			uvw->texcoords[i] = Vector2( temp[0] * scaleX, temp[1] * scaleY );
		}

		for ( const auto i : uvw->selection ) {
			uvw->texcoords[i] += centre;
		}

		uvw->updateNif();
		uvw->updateGL();
	}

	void undo() override final
	{
		Vector2 centre;
		for ( const auto i : uvw->selection ) {
			centre += uvw->texcoords[i];
		}
		centre /= uvw->selection.size();

		for ( const auto i : uvw->selection ) {
			uvw->texcoords[i] -= centre;
		}

		for ( const auto i : uvw->selection ) {
			Vector2 temp( uvw->texcoords[i] );
			uvw->texcoords[i] = Vector2( temp[0] / scaleX, temp[1] / scaleY );
		}

		for ( const auto i : uvw->selection ) {
			uvw->texcoords[i] += centre;
		}

		uvw->updateNif();
		uvw->updateGL();
	}

protected:
	UVWidget * uvw;
	float scaleX, scaleY;
};

void UVWidget::scaleSelection()
{
	ScalingDialog * scaleDialog = new ScalingDialog( this );

	if ( scaleDialog->exec() == QDialog::Accepted ) {
		// order does not matter here, since we scale around the center
		// don't perform identity transforms
		if ( !( scaleDialog->getXScale() == 1.0 && scaleDialog->getYScale() == 1.0 ) ) {
			undoStack->push( new UVWScaleCommand( this, scaleDialog->getXScale(), scaleDialog->getYScale() ) );
		}

		if ( !( scaleDialog->getXMove() == 0.0 && scaleDialog->getYMove() == 0.0 ) ) {
			undoStack->push( new UVWMoveCommand( this, scaleDialog->getXMove(), scaleDialog->getYMove() ) );
		}
	}
}

ScalingDialog::ScalingDialog( QWidget * parent ) : QDialog( parent )
{
	grid = new QGridLayout;
	setLayout( grid );
	int currentRow = 0;

	grid->addWidget( new QLabel( tr( "Enter scaling factors" ) ), currentRow, 0, 1, -1 );
	currentRow++;

	grid->addWidget( new QLabel( "X: " ), currentRow, 0, 1, 1 );
	spinXScale = new QDoubleSpinBox;
	spinXScale->setValue( 1.0 );
	spinXScale->setRange( -MAXSCALE, MAXSCALE );
	grid->addWidget( spinXScale, currentRow, 1, 1, 1 );

	grid->addWidget( new QLabel( "Y: " ), currentRow, 2, 1, 1 );
	spinYScale = new QDoubleSpinBox;
	spinYScale->setValue( 1.0 );
	spinYScale->setRange( -MAXSCALE, MAXSCALE );
	grid->addWidget( spinYScale, currentRow, 3, 1, 1 );
	currentRow++;

	uniform = new QCheckBox;
	connect( uniform, &QCheckBox::toggled, this, &ScalingDialog::setUniform );
	uniform->setChecked( true );
	grid->addWidget( uniform, currentRow, 0, 1, 1 );
	grid->addWidget( new QLabel( tr( "Uniform scaling" ) ), currentRow, 1, 1, -1 );
	currentRow++;

	grid->addWidget( new QLabel( tr( "Enter translation amounts" ) ), currentRow, 0, 1, -1 );
	currentRow++;

	grid->addWidget( new QLabel( "X: " ), currentRow, 0, 1, 1 );
	spinXMove = new QDoubleSpinBox;
	spinXMove->setValue( 0.0 );
	spinXMove->setRange( -MAXTRANS, MAXTRANS );
	grid->addWidget( spinXMove, currentRow, 1, 1, 1 );

	grid->addWidget( new QLabel( "Y: " ), currentRow, 2, 1, 1 );
	spinYMove = new QDoubleSpinBox;
	spinYMove->setValue( 0.0 );
	spinYMove->setRange( -MAXTRANS, MAXTRANS );
	grid->addWidget( spinYMove, currentRow, 3, 1, 1 );
	currentRow++;

	QPushButton * ok = new QPushButton( tr( "OK" ) );
	grid->addWidget( ok, currentRow, 0, 1, 2 );
	connect( ok, &QPushButton::clicked, this, &ScalingDialog::accept );

	QPushButton * cancel = new QPushButton( tr( "Cancel" ) );
	grid->addWidget( cancel, currentRow, 2, 1, 2 );
	connect( cancel, &QPushButton::clicked, this, &ScalingDialog::reject );
}

float ScalingDialog::getXScale()
{
	return spinXScale->value();
}

float ScalingDialog::getYScale()
{
	return spinYScale->value();
}

void ScalingDialog::setUniform( bool status )
{
	// Cast QDoubleSpinBox slot
	auto dsbValueChanged = static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged);

	if ( status == true ) {
		connect( spinXScale, dsbValueChanged, spinYScale, &QDoubleSpinBox::setValue );
		spinYScale->setEnabled( false );
		spinYScale->setValue( spinXScale->value() );
	} else {
		disconnect( spinXScale, dsbValueChanged, spinYScale, &QDoubleSpinBox::setValue );
		spinYScale->setEnabled( true );
	}
}

// 1 unit corresponds to 2 grid squares
float ScalingDialog::getXMove()
{
	return spinXMove->value() / 2.0;
}

float ScalingDialog::getYMove()
{
	return spinYMove->value() / 2.0;
}

//! A class to perform rotation of UV coordinates
class UVWRotateCommand final : public QUndoCommand
{
public:
	UVWRotateCommand( UVWidget * w, float r ) : QUndoCommand(), uvw( w ), rotation( r )
	{
		setText( "Rotation" );
	}

	int id() const override final
	{
		return 3;
	}

	bool mergeWith( const QUndoCommand * cmd ) override final
	{
		if ( cmd->id() == id() ) {
			rotation += static_cast<const UVWRotateCommand *>( cmd )->rotation;
			rotation -= 360.0 * (int)( rotation / 360.0 );
			return true;
		}

		return false;
	}

	void redo() override final
	{
		Vector2 centre;
		for ( const auto i : uvw->selection ) {
			centre += uvw->texcoords[i];
		}
		centre /= uvw->selection.size();

		for ( const auto i : uvw->selection ) {
			uvw->texcoords[i] -= centre;
		}

		Matrix rotMatrix;
		rotMatrix.fromEuler( 0, 0, ( rotation * PI / 180.0 ) );

		for ( const auto i : uvw->selection ) {
			Vector3 temp( uvw->texcoords[i], 0 );
			temp = rotMatrix * temp;
			uvw->texcoords[i] = Vector2( temp[0], temp[1] );
		}

		for ( const auto i : uvw->selection ) {
			uvw->texcoords[i] += centre;
		}

		uvw->updateNif();
		uvw->updateGL();
	}

	void undo() override final
	{
		Vector2 centre;
		for ( const auto i : uvw->selection ) {
			centre += uvw->texcoords[i];
		}
		centre /= uvw->selection.size();

		for ( const auto i : uvw->selection ) {
			uvw->texcoords[i] -= centre;
		}

		Matrix rotMatrix;
		rotMatrix.fromEuler( 0, 0, -( rotation * PI / 180.0 ) );

		for ( const auto i : uvw->selection ) {
			Vector3 temp( uvw->texcoords[i], 0 );
			temp = rotMatrix * temp;
			uvw->texcoords[i] = Vector2( temp[0], temp[1] );
		}

		for ( const auto i : uvw->selection ) {
			uvw->texcoords[i] += centre;
		}

		uvw->updateNif();
		uvw->updateGL();
	}

protected:
	UVWidget * uvw;
	float rotation;
};

void UVWidget::rotateSelection()
{
	bool ok;
	float rotateFactor = QInputDialog::getDouble( this, "NifSkope", tr( "Enter rotation angle" ), 0.0, -360.0, 360.0, 2, &ok );

	if ( ok ) {
		undoStack->push( new UVWRotateCommand( this, rotateFactor ) );
	}
}

void UVWidget::getTexSlots()
{
	menuTexSelect->clear();
	validTexs.clear();

	auto props = nif->getLinkArray( iShape, "Properties" );
	props << nif->getLink( iShape, "Shader Property" );
	for ( const auto l : props )
	{
		QModelIndex iTexProp = nif->getBlock( l, "NiTexturingProperty" );

		if ( iTexProp.isValid() ) {
			for ( const QString& name : texnames ) {
				if ( nif->get<bool>( iTexProp, QString( "Has %1" ).arg( name ) ) ) {
					if ( validTexs.indexOf( name ) == -1 ) {
						validTexs << name;
						QAction * temp;
						menuTexSelect->addAction( temp = new QAction( name, this ) );
						texSlotGroup->addAction( temp );
						temp->setCheckable( true );

						if ( name == texnames[currentTexSlot] ) {
							temp->setChecked( true );
						}
					}
				}
			}
		}
	}
}

void UVWidget::selectTexSlot()
{
	QString selected = texSlotGroup->checkedAction()->text();
	currentTexSlot = texnames.indexOf( selected );

	auto props = nif->getLinkArray( iShape, "Properties" );
	props << nif->getLink( iShape, "Shader Property" );
	for ( const auto l : props )
	{
		QModelIndex iTexProp = nif->getBlock( l, "NiTexturingProperty" );

		if ( iTexProp.isValid() ) {
			iTex = nif->getIndex( iTexProp, texnames[currentTexSlot] );

			if ( iTex.isValid() ) {
				QModelIndex iTexSource = nif->getBlock( nif->getLink( iTex, "Source" ) );

				if ( iTexSource.isValid() ) {
					currentCoordSet = nif->get<int>( iTex, "UV Set" );
					iTexCoords = nif->getIndex( iShapeData, "UV Sets" ).child( currentCoordSet, 0 );
					texsource  = iTexSource;
					setTexCoords();
					updateGL();
					return;
				}
			}
		}
	}
}

void UVWidget::getCoordSets()
{
	coordSetSelect->clear();

	// TODO: Broken for newer nif.xml where NiGeometryData has been corrected
	quint8 numUvSets = nif->get<quint8>( iShapeData, "Num UV Sets" );

	for ( int i = 0; i < numUvSets; i++ ) {
		QAction * temp;
		coordSetSelect->addAction( temp = new QAction( QString( "%1" ).arg( i ), this ) );
		coordSetGroup->addAction( temp );
		temp->setCheckable( true );

		if ( i == currentCoordSet ) {
			temp->setChecked( true );
		}
	}

	coordSetSelect->addSeparator();
	aDuplicateCoords = new QAction( tr( "Duplicate current" ), this );
	coordSetSelect->addAction( aDuplicateCoords );
	connect( aDuplicateCoords, &QAction::triggered, this, &UVWidget::duplicateCoordSet );
}

void UVWidget::selectCoordSet()
{
	QString selected = coordSetGroup->checkedAction()->text();
	bool ok;
	quint8 setToUse = selected.toInt( &ok );

	if ( !ok )
		return;

	// write all changes
	updateNif();
	// change coordinate set
	changeCoordSet( setToUse );
}

void UVWidget::changeCoordSet( int setToUse )
{
	// update
	currentCoordSet = setToUse;
	nif->set<quint8>( iTex, "UV Set", currentCoordSet );
	// read new coordinate set
	iTexCoords = nif->getIndex( iShapeData, "UV Sets" ).child( currentCoordSet, 0 );
	setTexCoords();
}


void UVWidget::duplicateCoordSet()
{
	// this signal close the UVWidget
	disconnect( nif, &NifModel::dataChanged, this, &UVWidget::nifDataChanged );
	// expand the UV Sets array and duplicate the current coordinates
	quint8 numUvSets = nif->get<quint8>( iShapeData, "Num UV Sets" );
	nif->set<quint8>( iShapeData, "Num UV Sets", numUvSets + 1 );
	QModelIndex uvSets = nif->getIndex( iShapeData, "UV Sets" );
	nif->updateArray( uvSets );
	nif->setArray<Vector2>( uvSets.child( numUvSets, 0 ), nif->getArray<Vector2>( uvSets.child( currentCoordSet, 0 ) ) );
	// switch to that coordinate set
	changeCoordSet( numUvSets );
	// reconnect data changed signal
	connect( nif, &NifModel::dataChanged, this, &UVWidget::nifDataChanged );
}

