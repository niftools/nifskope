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

#include "gltex.h"
#include "options.h"

#include "glscene.h"
#include "gltexloaders.h"

#ifdef FSENGINE
#include <fsengine/fsengine.h>
#include <fsengine/fsmanager.h>
#endif

#include <QDebug>
#include <QDir>
#include <QFileSystemWatcher>
#include <QListView>
#include <QOpenGLContext>
#include <QOpenGLFunctions>


//! \file gltex.cpp TexCache management

//! Number of texture units
GLint num_texture_units = 0;
//! Maximum anisotropy
float max_anisotropy = 0;

//! Accessor function for glProperty etc.
float get_max_anisotropy()
{
	return max_anisotropy;
}

void initializeTextureUnits( const QOpenGLContext * context )
{
	if ( context->hasExtension( "GL_ARB_multitexture" ) ) {
		// TODO: This will always return 4 intentionally
		// For shaders, should be GL_MAX_TEXTURE_IMAGE_UNITS_ARB, etc.

		glGetIntegerv( GL_MAX_TEXTURE_UNITS_ARB, &num_texture_units );

		if ( num_texture_units < 1 )
			num_texture_units = 1;

		//qWarning() << "texture units" << num_texture_units;
	} else {
		qWarning( "multitexturing not supported" );
		num_texture_units = 1;
	}

	if ( context->hasExtension( "GL_EXT_texture_filter_anisotropic" ) ) {
		glGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_anisotropy );
		//qWarning() << "maximum anisotropy" << max_anisotropy;
	} else {
		max_anisotropy = 0;
	}
}

bool activateTextureUnit( int stage )
{
	PFNGLACTIVETEXTUREPROC glActiveTexture;
	glActiveTexture = (PFNGLACTIVETEXTUREPROC)QOpenGLContext::currentContext()->getProcAddress( "glActiveTexture" );
	PFNGLCLIENTACTIVETEXTUREPROC glClientActiveTexture;
	glClientActiveTexture = (PFNGLCLIENTACTIVETEXTUREPROC)QOpenGLContext::currentContext()->getProcAddress( "glClientActiveTexture" );

	if ( num_texture_units <= 1 )
		return ( stage == 0 );

	// num_texture_units > 1 can only happen if GLEE_ARB_multitexture is true
	// so glActiveTexture and glClientActiveTexture are supported
	if ( stage < num_texture_units ) {
		glActiveTexture( GL_TEXTURE0 + stage );
		glClientActiveTexture( GL_TEXTURE0 + stage );
		return true;
	}

	return false;
}

void resetTextureUnits()
{
	PFNGLACTIVETEXTUREPROC glActiveTexture;
	glActiveTexture = (PFNGLACTIVETEXTUREPROC)QOpenGLContext::currentContext()->getProcAddress( "glActiveTexture" );
	PFNGLCLIENTACTIVETEXTUREPROC glClientActiveTexture;
	glClientActiveTexture = (PFNGLCLIENTACTIVETEXTUREPROC)QOpenGLContext::currentContext()->getProcAddress( "glClientActiveTexture" );

	if ( num_texture_units <= 1 ) {
		glDisable( GL_TEXTURE_2D );
		return;
	}

	// num_texture_units > 1 can only happen if GLEE_ARB_multitexture is true
	// so glActiveTexture and glClientActiveTexture are supported
	for ( int x = num_texture_units - 1; x >= 0; x-- ) {
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
	connect( watcher, &QFileSystemWatcher::fileChanged, this, &TexCache::fileChanged );
}

TexCache::~TexCache()
{
	//flush();
}

QString TexCache::find( const QString & file, const QString & nifdir )
{
	return find( file, nifdir, *(new QByteArray()) );
}

QString TexCache::find( const QString & file, const QString & nifdir, QByteArray & data )
{
	if ( file.isEmpty() )
		return QString();

	QString filename = QDir::toNativeSeparators( file );

	while ( filename.startsWith( "/" ) || filename.startsWith( "\\" ) )
		filename.remove( 0, 1 );

	QStringList extensions;
	extensions << ".tga" << ".dds" << ".bmp" << ".nif" << ".texcache";
#ifndef Q_OS_WIN
	extensions << ".TGA" << ".DDS" << ".BMP" << ".NIF" << ".TEXCACHE";
#endif
	bool replaceExt = false;

	if ( Options::textureAlternatives() ) {
		for ( const QString ext : QStringList{ extensions } )
		{
			if ( filename.endsWith( ext ) ) {
				extensions.removeAll( ext );
				extensions.prepend( ext );
				filename = filename.left( filename.length() - ext.length() );
				replaceExt = true;
				break;
			}
		}
	}

	// attempt to find the texture in one of the folders
	QDir dir;
	for ( const QString& ext : extensions ) {
		if ( replaceExt ) {
			filename += ext;
		}

		for ( QString folder : Options::textureFolders() ) {
			// TODO: Always search nifdir without requiring a relative entry
			// in folders?  Not too intuitive to require ".\" in your texture folder list
			// even if it is added by default.
			if ( folder.startsWith( "./" ) || folder.startsWith( ".\\" ) ) {
				folder = nifdir + "/" + folder;
			}

			dir.setPath( folder );

			if ( dir.exists( filename ) ) {
				filename = dir.filePath( filename );
				return filename;
			}
		}

#ifdef FSENGINE
		// Search through archives last, and load any requested textures into memory.
		for ( FSArchiveFile * archive : FSManager::archiveList() ) {
			if ( archive ) {
				filename = QDir::fromNativeSeparators( filename.toLower() );

				if ( archive->hasFile( filename ) ) {
					QByteArray outData;
					//qDebug() << "Extracting " << filename;
					archive->fileContents( filename, outData );

					if ( !outData.isEmpty() ) {
						data = outData;

						return file;
					}
				}
			}
		}
#endif
		if ( !replaceExt )
			break;

		// Remove file extension
		filename = filename.left( filename.length() - ext.length() );
	}

	// Fix separators
	filename = QDir::toNativeSeparators( filename );

	if ( replaceExt )
		return filename + extensions.value( 0 ); // Restore original file extension

	return filename;
}

/*!
 * Note: all original morrowind nifs use name.ext only for addressing the
 * textures, but most mods use something like textures/[subdir/]name.ext.
 * This is due to a feature in Morrowind resource manager: it loads name.ext,
 * textures/name.ext and textures/subdir/name.ext but NOT subdir/name.ext.
 */
QString TexCache::stripPath( const QString & filepath, const QString & nifFolder )
{
	QString file = filepath;
	file = file.replace( "/", "\\" ).toLower();
	QDir basePath;

	for ( QString base : Options::textureFolders() ) {
		if ( base.startsWith( "./" ) || base.startsWith( ".\\" ) ) {
			base = nifFolder + "/" + base;
		}

		basePath.setPath( base );
		base = basePath.absolutePath();
		base = base.replace( "/", "\\" ).toLower();
		/*
		 * note that basePath.relativeFilePath( file ) here is *not*
		 * what we want - see the above doc comments for this function
		 */

		if ( file.startsWith( base ) ) {
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
	QMutableHashIterator<QString, Tex *> it( textures );

	while ( it.hasNext() ) {
		it.next();

		Tex * tx = it.value();

		if ( tx && tx->filepath == filepath ) {
			if ( QFile::exists( tx->filepath ) ) {
				tx->reload = true;
				emit sigRefresh();
			} else {
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

	if ( !tx ) {
		tx = new Tex;
		tx->filename = fname;
		tx->id = 0;
		tx->data = QByteArray();
		tx->mipmaps = 0;
		tx->reload  = false;

		textures.insert( tx->filename, tx );
	}

	QByteArray outData;

	if ( tx->filepath.isEmpty() || tx->reload )
		tx->filepath = find( tx->filename, nifFolder, outData );

	if ( !outData.isEmpty() ) {
		tx->data = outData;
	}

	if ( !tx->id || tx->reload ) {
		if ( QFile::exists( tx->filepath ) && QFileInfo( tx->filepath ).isWritable() && ( !watcher->files().contains( tx->filepath ) ) )
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
		if ( nif->get<quint8>( iSource, "Use External" ) == 0 ) {
			QModelIndex iData = nif->getBlock( nif->getLink( iSource, "Pixel Data" ) );

			if ( iData.isValid() ) {
				Tex * tx = embedTextures.value( iData );

				if ( !tx ) {
					tx = new Tex();
					tx->id = 0;
					tx->reload = false;
					try
					{
						glGenTextures( 1, &tx->id );
						glBindTexture( GL_TEXTURE_2D, tx->id );
						embedTextures.insert( iData, tx );
						texLoad( iData, tx->format, tx->width, tx->height, tx->mipmaps );
					}
					catch ( QString e ) {
						tx->status = e;
					}
				}

				glBindTexture( GL_TEXTURE_2D, tx->id );
				return tx->mipmaps;
			}
		} else if ( !nif->get<QString>( iSource, "File Name" ).isEmpty() ) {
			return bind( nif->get<QString>( iSource, "File Name" ) );
		}
	}

	return 0;
}

void TexCache::flush()
{
	for ( Tex * tx : textures ) {
		if ( tx->id )
			glDeleteTextures( 1, &tx->id );
	}
	qDeleteAll( textures );
	textures.clear();

	for ( Tex * tx : embedTextures ) {
		if ( tx->id )
			glDeleteTextures( 1, &tx->id );
	}
	qDeleteAll( embedTextures );
	embedTextures.clear();

	if ( !watcher->files().empty() ) {
		watcher->removePaths( watcher->files() );
	}
}

void TexCache::setNifFolder( const QString & folder )
{
	nifFolder = folder;
	flush();
	emit sigRefresh();
}

QString TexCache::info( const QModelIndex & iSource )
{
	QString temp;
	const NifModel * nif = qobject_cast<const NifModel *>( iSource.model() );

	if ( nif && iSource.isValid() ) {
		if ( nif->get<quint8>( iSource, "Use External" ) == 0 ) {
			QModelIndex iData = nif->getBlock( nif->getLink( iSource, "Pixel Data" ) );

			if ( iData.isValid() ) {
				Tex * tx = embedTextures.value( iData );
				temp = QString( "Embedded texture: %1\nWidth: %2\nHeight: %3\nMipmaps: %4" )
				       .arg( tx->format )
				       .arg( tx->width )
				       .arg( tx->height )
				       .arg( tx->mipmaps );
			} else {
				temp = QString( "Embedded texture invalid" );
			}
		} else {
			QString filename = nif->get<QString>( iSource, "File Name" );
			Tex * tx = textures.value( filename );
			temp = QString( "External texture file: %1\nTexture path: %2\nFormat: %3\nWidth: %4\nHeight: %5\nMipmaps: %6" )
			       .arg( tx->filename )
			       .arg( tx->filepath )
			       .arg( tx->format )
			       .arg( tx->width )
			       .arg( tx->height )
			       .arg( tx->mipmaps );
		}
	}

	return temp;
}

bool TexCache::exportFile( const QModelIndex & iSource, QString & filepath )
{
	Tex * tx = embedTextures.value( iSource );

	if ( !tx ) {
		tx = new Tex();
		tx->id = 0;
	}

	return tx->saveAsFile( iSource, filepath );
}

bool TexCache::importFile( NifModel * nif, const QModelIndex & iSource, QModelIndex & iData )
{
	//const NifModel * nif = qobject_cast<const NifModel *>( iSource.model() );
	if ( nif && iSource.isValid() ) {
		if ( nif->get<quint8>( iSource, "Use External" ) == 1 ) {
			QString filename = nif->get<QString>( iSource, "File Name" );
			//qWarning() << "TexCache::importFile: Texture has filename (from NIF) " << filename;
			Tex * tx = textures.value( filename );
			return tx->savePixelData( nif, iSource, iData );
		}
	}

	return false;
}


//////////////////////////////////////////////////////////////////////////


void TexCache::Tex::load()
{
	if ( !id )
		glGenTextures( 1, &id );

	width  = height = mipmaps = 0;
	reload = false;
	status = QString();

	glBindTexture( GL_TEXTURE_2D, id );

	try
	{
		texLoad( filepath, format, width, height, mipmaps, data );
	}
	catch ( QString e )
	{
		status = e;
	}
}

bool TexCache::Tex::saveAsFile( const QModelIndex & index, QString & savepath )
{
	texLoad( index, format, width, height, mipmaps );

	if ( savepath.toLower().endsWith( ".tga" ) ) {
		return texSaveTGA( index, savepath, width, height );
	}

	return texSaveDDS( index, savepath, width, height, mipmaps );
}

bool TexCache::Tex::savePixelData( NifModel * nif, const QModelIndex & iSource, QModelIndex & iData )
{
	Q_UNUSED( iSource );
	// gltexloaders function goes here
	//qWarning() << "TexCache::Tex:savePixelData: Packing" << iSource << "from file" << filepath << "to" << iData;
	return texSaveNIF( nif, filepath, iData );
}
