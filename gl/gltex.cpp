/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005-2008, NIF File Format Library and Tools
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
#include "GLee.h"
#include <QtOpenGL>

#include <QDebug>
#include <QDir>
#include <QFileSystemWatcher>
#include <QListView>

#include "glscene.h"
#include "gltex.h"
#include "gltexloaders.h"
#include "options.h"
#include "fsengine/fsmanager.h"
#include "fsengine/fsengine.h"
#include <GL/glext.h>

int num_texture_units = 0;
float max_anisotropy = 0;

void initializeTextureUnits( const QGLContext * context )
{
	QString extensions( (const char *) glGetString(GL_EXTENSIONS) );
	//foreach ( QString e, extensions.split( " " ) )
	//	qWarning() << e;
	
	//if (!extensions.contains("GL_ARB_texture_compression"))
	//	qWarning() << "texture compression not supported, some textures may not load";

	// *** check disabled: software decompression is supported ***
	//if (!extensions.contains("GL_EXT_texture_compression_s3tc"))
	//	qWarning() << "S3TC texture compression not supported, some textures may not load";

	if ( GLEE_ARB_multitexture )
	{
    glGetIntegerv( GL_MAX_TEXTURE_UNITS_ARB, &num_texture_units );
    if ( num_texture_units < 1 )
      num_texture_units = 1;
		  //qWarning() << "texture units" << num_texture_units;
	}
	else
	{
    qWarning( "multitexturing not supported" );
    num_texture_units = 1;
	}
	
	if ( GLEE_EXT_texture_filter_anisotropic )
	{
		glGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, & max_anisotropy );
		//qWarning() << "maximum anisotropy" << max_anisotropy;
	}
}

bool activateTextureUnit( int stage )
{
	if ( num_texture_units <= 1 )
		return ( stage == 0 );
	
	if ( stage < num_texture_units )
	{
    if (GLEE_ARB_texture_compression )
    {
      glActiveTexture( GL_TEXTURE0 + stage );
      glClientActiveTexture( GL_TEXTURE0 + stage );	
      return true;
    }
    else
      qWarning( "texture compression not supported" );
  }
  else
    return false;
}

void resetTextureUnits()
{
	if ( num_texture_units <= 1 )
	{
		glDisable( GL_TEXTURE_2D );
		return;
	}
	
	for ( int x = num_texture_units-1; x >= 0; x-- )
	{
		glActiveTexture( GL_TEXTURE0 + x );
		glDisable( GL_TEXTURE_2D );
		glMatrixMode( GL_TEXTURE );
		glLoadIdentity();
		glMatrixMode( GL_MODELVIEW );
		glClientActiveTexture( GL_TEXTURE0 + x );
		glDisableClientState( GL_TEXTURE_COORD_ARRAY );
	}
}


/*
	TexCache
*/

TexCache::TexCache( QObject * parent ) : QObject( parent )
{
	watcher = new QFileSystemWatcher( this );
	connect( watcher, SIGNAL( fileChanged( const QString & ) ), this, SLOT( fileChanged( const QString & ) ) );
}

TexCache::~TexCache()
{
	//flush();
}

QString TexCache::find( const QString & file, const QString & nifdir )
{
	if ( file.isEmpty() )
		return QString();

#ifndef Q_OS_WIN
	/* convert nif path backslash into forward slash */
	/* also check for both original name and lower case name */
	QString filename_orig = QString(file).replace( "\\", "/" );;
	QString filename = file.toLower().replace( "\\", "/" );
#else
	QString filename = file.toLower();
#endif

	while ( filename.startsWith( "/" ) || filename.startsWith( "\\" ) )
		filename.remove( 0, 1 );
	
	QStringList extensions;
	extensions << ".tga" << ".dds" << ".bmp" << ".nif";
#ifndef Q_OS_WIN
	extensions << ".TGA" << ".DDS" << ".BMP" << ".NIF";
#endif
	bool replaceExt = false;
	if ( Options::textureAlternatives() )
	{
		foreach ( QString ext, extensions )
		{
			if ( filename.endsWith( ext ) )
			{
				extensions.removeAll( ext );
				extensions.prepend( ext );
				filename = filename.left( filename.length() - ext.length() );
#ifndef Q_OS_WIN
				filename_orig = filename_orig.left( filename_orig.length() - ext.length() );
#endif
				replaceExt = true;
				break;
			}
		}
	}
	
	// attempt to find the texture in one of the folders
	QDir dir;
	foreach ( QString ext, extensions )
	{
		if ( replaceExt ) {
			filename += ext;
#ifndef Q_OS_WIN
			filename_orig += ext;
#endif
		}
		
		foreach ( QString folder, Options::textureFolders() )
		{
			if( folder.startsWith( "./" ) || folder.startsWith( ".\\" ) ) {
				folder = nifdir + "/" + folder;
			}
			
			dir.setPath( folder );
#ifndef Q_OS_WIN
			//qWarning() << folder << filename;
#endif
			if ( dir.exists( filename ) ) {
				filename = dir.filePath( filename );
				// fix separators
#ifdef Q_OS_WIN
				filename.replace("/", "\\");
#else
				filename.replace("\\", "/");
#endif
				return filename;
			}
#ifndef Q_OS_WIN
			//qWarning() << folder << filename_orig;
			if ( dir.exists( filename_orig ) ) {
				filename = dir.filePath( filename_orig );
				// fix separators
#ifdef Q_OS_WIN
				filename.replace("/", "\\");
#else
				filename.replace("\\", "/");
#endif
				return filename;
			}
#endif
		}

		// Search through archives last and return archive annotated name
		//   which will be removed by any handlers
		foreach ( FSArchiveFile* archive, FSManager::archiveList() ) {
			if ( archive ) {
				QString fullname = archive->absoluteFilePath( filename.toLower().replace( "\\", "/" ) );
				if (!fullname.isEmpty()) {
#ifdef Q_OS_WIN
					fullname.replace("/", "\\");
#else
					fullname.replace("\\", "/");
#endif
					return fullname;
				}
			}
		}
		
		if ( replaceExt ) {
			filename = filename.left( filename.length() - ext.length() );
#ifndef Q_OS_WIN
			filename_orig = filename_orig.left( filename_orig.length() - ext.length() );
#endif
		} else
			break;
	}
	
	// fix separators
#ifdef Q_OS_WIN
	filename.replace("/", "\\");
#else
	filename.replace("\\", "/");
#endif

	if ( replaceExt )
		return filename + extensions.value( 0 );
	else
		return filename;
}

QString TexCache::stripPath( const QString & filepath, const QString & nifFolder )
{
	QString file = filepath;
	file = file.replace( "/", "\\" ).toLower();
	
	foreach ( QString base, Options::textureFolders() )
	{
		if( base.startsWith( "./" ) || base.startsWith( ".\\" ) ) {
			base = nifFolder + "/" + base;
		}
		base = base.replace( "/", "\\" ).toLower();
		
		if ( file.startsWith( base ) )
		{
			file.remove( 0, base.length() );
			break;
		}
	}
	
	if ( file.startsWith( "/" ) || file.startsWith( "\\" ) )
		file.remove( 0, 1 );
	
	return file;
}

bool TexCache::canLoad( const QString & filePath )
{
	return texCanLoad( filePath );
}

void TexCache::fileChanged( const QString & filepath )
{
	QMutableHashIterator<QString,Tex*> it( textures );
	while ( it.hasNext() )
	{
		it.next();
		
		Tex * tx = it.value();
		
		if ( tx->filepath == filepath )
		{
			if ( QFile::exists( tx->filepath ) )
			{
				tx->reload = true;
				emit sigRefresh();
			}
			else
			{
				it.remove();
				if ( tx->id )
					glDeleteTextures( 1, &tx->id );
				delete tx;
			}
		}
	}
}

int TexCache::bind( const QString & fname )
{
	Tex * tx = textures.value( fname );
	if ( ! tx )
	{
		tx = new Tex;
		tx->filename = fname;
		tx->id = 0;
		tx->mipmaps = 0;
		tx->reload = false;
		
		textures.insert( tx->filename, tx );
	}
	
	if ( tx->filepath.isEmpty() || tx->reload )
		tx->filepath = find( tx->filename, nifFolder );
	
	if ( ! tx->id || tx->reload )
	{
		if ( QFile::exists( tx->filepath ) && QFileInfo( tx->filepath ).isWritable() && (!watcher->files().contains(tx->filepath)) )
			watcher->addPath( tx->filepath );
		tx->load();
	}
	
	glBindTexture( GL_TEXTURE_2D, tx->id );
	
	return tx->mipmaps;
}

int TexCache::bind( const QModelIndex & iSource )
{
	const NifModel * nif = qobject_cast<const NifModel *>( iSource.model() );
	if ( nif && iSource.isValid() ) {
		if ( nif->get<quint8>( iSource, "Use External" ) == 0 ){
			QModelIndex iData = nif->getBlock( nif->getLink( iSource, "Pixel Data" ) );
			if (iData.isValid()) {
				Tex * tx = embedTextures.value( iData );
				if (tx == NULL){
					tx = new Tex();
					tx->id = 0;
					tx->reload = false;
					try
					{
						glGenTextures( 1, &tx->id );
						glBindTexture( GL_TEXTURE_2D, tx->id );
						embedTextures.insert( iData, tx );
						texLoad(iData, tx->format, tx->width, tx->height, tx->mipmaps);
					} catch ( QString e ) {
						tx->status = e;
					}
				}
				glBindTexture( GL_TEXTURE_2D, tx->id );
				return tx->mipmaps;
			}
		} else {
			return bind( nif->get<QString>( iSource, "File Name" ) );
		}
	}
	return 0;
}


void TexCache::Tex::load()
{
	if ( ! id )
		glGenTextures( 1, &id );
	
	width = height = mipmaps = 0;
	reload = false;
	status = QString();
	
	glBindTexture( GL_TEXTURE_2D, id );
	
	try
	{
		texLoad( filepath, format, width, height, mipmaps );
	}
	catch ( QString e )
	{
		status = e;
	}
}

void TexCache::flush()
{
	foreach ( Tex * tx, textures )
	{
		if ( tx->id )
			glDeleteTextures( 1, &tx->id );
	}
	qDeleteAll( textures );
	textures.clear();

	foreach ( Tex * tx, embedTextures )
	{
		if ( tx->id )
			glDeleteTextures( 1, &tx->id );
	}
	qDeleteAll( embedTextures );
	embedTextures.clear();
	
	if( !watcher->files().empty() ) {
		watcher->removePaths( watcher->files() );
	}
}

bool TexturingProperty::bind( int id, const QString & fname )
{
	GLuint mipmaps = 0;
	if ( id >= 0 && id <= 7 )
	{
		if ( !fname.isEmpty() )
			mipmaps = scene->bindTexture(  fname );
		else 
			mipmaps = scene->bindTexture( textures[ id ].iSource );
		if (mipmaps == 0)
			return false;
		
		if ( max_anisotropy > 0 )
		{
			if ( Options::antialias() )
				glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, max_anisotropy );
			else
				glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0 );
		}
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipmaps > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, textures[id].wrapS );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, textures[id].wrapT );
		glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
		glMatrixMode( GL_TEXTURE );
		glLoadIdentity();
		if ( textures[id].hasTransform )
		{
			glTranslatef( - textures[id].center[0], - textures[id].center[1], 0 );
			glRotatef( textures[id].rotation, 0, 0, 1 );
			glTranslatef( textures[id].center[0], textures[id].center[1], 0 );
			glScalef( textures[id].tiling[0], textures[id].tiling[1], 1 );
			glTranslatef( textures[id].translation[0], textures[id].translation[1], 0 );
		}
		glMatrixMode( GL_MODELVIEW );
		return true;
	}
	else
		return false;
}

bool checkSet( int s, const QList< QVector< Vector2 > > & texcoords )
{
	return s >= 0 && s < texcoords.count() && texcoords[s].count();
}

bool TexturingProperty::bind( int id, const QList< QVector< Vector2 > > & texcoords )
{
	if ( checkSet( textures[id].coordset, texcoords ) && bind( id ) )
	{
		glEnable( GL_TEXTURE_2D );
		glEnableClientState( GL_TEXTURE_COORD_ARRAY );
		glTexCoordPointer( 2, GL_FLOAT, 0, texcoords[ textures[id].coordset ].data() );
		return true;
	}
	else
	{
		glDisable( GL_TEXTURE_2D );
		return false;
	}
}

bool TexturingProperty::bind( int id, const QList< QVector< Vector2 > > & texcoords, int stage )
{
	return ( activateTextureUnit( stage ) && bind( id, texcoords ) );
}

bool TextureProperty::bind()
{
	if ( GLuint mipmaps = scene->bindTexture( fileName() ) )
	{
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipmaps > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR );
		glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
		glMatrixMode( GL_TEXTURE );
		glLoadIdentity();
		glMatrixMode( GL_MODELVIEW );
		return true;
	}
	return false;
}

bool TextureProperty::bind( const QList< QVector< Vector2 > > & texcoords )
{
	if ( checkSet( 0, texcoords ) && bind() )
	{
		glEnable( GL_TEXTURE_2D );
		glEnableClientState( GL_TEXTURE_COORD_ARRAY );
		glTexCoordPointer( 2, GL_FLOAT, 0, texcoords[ 0 ].data() );
		return true;
	}
	else
	{
		glDisable( GL_TEXTURE_2D );
		return false;
	}
}

void TexCache::setNifFolder( const QString & folder )
{
	nifFolder = folder;
	flush();
	emit sigRefresh();
}

//////////////////////////////////////////////////////////////////////////


bool BSShaderLightingProperty::bind( int id, const QString & fname )
{
   GLuint mipmaps = 0;
   if ( !fname.isEmpty() )
      mipmaps = scene->bindTexture(  fname );
   else 
      mipmaps = scene->bindTexture( this->fileName(id) );
   if (mipmaps == 0)
      return false;

   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipmaps > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR );
   glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
   glMatrixMode( GL_TEXTURE );
   glLoadIdentity();
   glMatrixMode( GL_MODELVIEW );
   return true;
}

bool BSShaderLightingProperty::bind( int id, const QList< QVector<Vector2> > & texcoords )
{
   if ( checkSet( 0, texcoords ) && bind(id) )
   {
      glEnable( GL_TEXTURE_2D );
      glEnableClientState( GL_TEXTURE_COORD_ARRAY );
      glTexCoordPointer( 2, GL_FLOAT, 0, texcoords[ 0 ].data() );
      return true;
   }
   else
   {
      glDisable( GL_TEXTURE_2D );
      return false;
   }
}

#endif
