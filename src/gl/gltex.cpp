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

#include "gltex.h"

#include "message.h"
#include "gl/glscene.h"
#include "gl/gltexloaders.h"
#include "model/nifmodel.h"

#include <fsengine/fsengine.h>
#include <fsengine/fsmanager.h>

#include <QDebug>
#include <QDir>
#include <QFileSystemWatcher>
#include <QListView>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QSettings>

#include <algorithm>


//! @file gltex.cpp TexCache management

#ifdef WIN32
PFNGLACTIVETEXTUREARBPROC glActiveTextureARB = nullptr;
PFNGLCLIENTACTIVETEXTUREARBPROC glClientActiveTextureARB = nullptr;
#endif

//! Number of texture units
GLint num_texture_units = 0;

//! Maximum anisotropy
float max_anisotropy = 1.0f;
void set_max_anisotropy()
{
	static QSettings settings;
	max_anisotropy = std::min( std::pow( settings.value( "Settings/Render/General/Anisotropic Filtering", 4.0 ).toFloat(), 2.0f ),
								max_anisotropy );
}

float get_max_anisotropy()
{
	return max_anisotropy;
}

void initializeTextureUnits( const QOpenGLContext * context )
{
	if ( context->hasExtension( "GL_ARB_multitexture" ) ) {
		glGetIntegerv( GL_MAX_TEXTURE_IMAGE_UNITS_ARB, &num_texture_units );

		if ( num_texture_units < 1 )
			num_texture_units = 1;

		//qDebug() << "texture units" << num_texture_units;
	} else {
		qCWarning( nsGl ) << QObject::tr( "Multitexturing not supported." );
		num_texture_units = 1;
	}

	if ( context->hasExtension( "GL_EXT_texture_filter_anisotropic" ) ) {
		glGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_anisotropy );
		set_max_anisotropy();
		//qDebug() << "maximum anisotropy" << max_anisotropy;
	}

#ifdef WIN32
	if ( !glActiveTextureARB )
		glActiveTextureARB = (PFNGLACTIVETEXTUREARBPROC)context->getProcAddress( "glActiveTextureARB" );

	if ( !glClientActiveTextureARB )
		glClientActiveTextureARB = (PFNGLCLIENTACTIVETEXTUREARBPROC)context->getProcAddress( "glClientActiveTextureARB" );
#endif

	initializeTextureLoaders( context );
}

bool activateTextureUnit( int stage )
{
	if ( num_texture_units <= 1 )
		return ( stage == 0 );

	if ( stage < num_texture_units ) {

		glActiveTextureARB( GL_TEXTURE0 + stage );
		glClientActiveTextureARB( GL_TEXTURE0 + stage );
		return true;
	}

	return false;
}

void resetTextureUnits( int numTex )
{
	if ( num_texture_units <= 1 ) {
		glDisable( GL_TEXTURE_2D );
		return;
	}

	for ( int x = numTex - 1; x >= 0; x-- ) {
		glActiveTextureARB( GL_TEXTURE0 + x );
		glDisable( GL_TEXTURE_2D );
		glMatrixMode( GL_TEXTURE );
		glLoadIdentity();
		glMatrixMode( GL_MODELVIEW );
		glClientActiveTextureARB( GL_TEXTURE0 + x );
		glDisableClientState( GL_TEXTURE_COORD_ARRAY );
	}
}


/*
 *  TexCache
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

	if ( QFile( file ).exists() )
		return file;

	QSettings settings;

	QString filename = QDir::toNativeSeparators( file );

	QStringList extensions;
	extensions << ".dds";
	bool replaceExt = false;

	bool textureAlternatives = settings.value( "Settings/Resources/Alternate Extensions", false ).toBool();
	if ( textureAlternatives ) {
		extensions << ".tga" << ".bmp" << ".nif" << ".texcache";
		for ( const QString ext : QStringList{ extensions } )
		{
			if ( filename.endsWith( ext, Qt::CaseInsensitive ) ) {
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

		auto appdir = QDir::currentPath();
		
		// First search NIF root
		dir.setPath( nifdir );
		if ( dir.exists( filename ) ) {
			return dir.filePath( filename );
		}

		// Next search NifSkope dir
		dir.setPath( appdir );
		if ( dir.exists( filename ) ) {
			return dir.filePath( filename );
		}

		
		QStringList folders = settings.value( "Settings/Resources/Folders", QStringList() ).toStringList();

		for ( QString folder : folders ) {
			// TODO: Always search nifdir without requiring a relative entry
			// in folders?  Not too intuitive to require ".\" in your texture folder list
			// even if it is added by default.
			if ( folder.startsWith( "./" ) || folder.startsWith( ".\\" ) ) {
				folder = nifdir + "/" + folder;
			}

			dir.setPath( folder );

			if ( dir.exists( filename ) ) {
				filename = dir.filePath( filename );
				filename = QDir::toNativeSeparators( filename );
				return filename;
			}
		}

		// Search through archives last, and load any requested textures into memory.
		for ( FSArchiveFile * archive : FSManager::archiveList() ) {
			if ( archive ) {
				filename = QDir::fromNativeSeparators( filename.toLower() );
				if ( archive->hasFile( filename ) ) {
					QByteArray outData;
					archive->fileContents( filename, outData );

					if ( !outData.isEmpty() ) {
						data = outData;
						filename = QDir::toNativeSeparators( filename );
						return filename;
					}
				}
			}
		}

		// For Skyrim and FO4 which occasionally leave the textures off
		if ( !filename.startsWith( "textures", Qt::CaseInsensitive ) ) {
			QRegularExpression re( "textures[\\\\/]", QRegularExpression::CaseInsensitiveOption );
			int texIdx = filename.indexOf( re );
			if ( texIdx > 0 ) {
				filename.remove( 0, texIdx );
			} else {
				while ( filename.startsWith( "/" ) || filename.startsWith( "\\" ) )
					filename.remove( 0, 1 );

				if ( !filename.startsWith( "textures", Qt::CaseInsensitive ) && !filename.startsWith( "shaders", Qt::CaseInsensitive ) )
					filename.prepend( "textures\\" );
			}

			return find( filename, nifdir, data );
		}

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

	QSettings settings;
	QStringList folders = settings.value( "Settings/Resources/Folders", QStringList() ).toStringList();

	for ( QString base : folders ) {
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

bool TexCache::isSupported( const QString & filePath )
{
	return texIsSupported( filePath );
}

void TexCache::fileChanged( const QString & filepath )
{
	QMutableHashIterator<QString, Tex *> it( textures );

	while ( it.hasNext() ) {
		it.next();

		Tex * tx = it.value();

		if ( tx && tx->filepath == filepath ) {
			// Remove from watcher now to prevent multiple signals
			watcher->removePath( tx->filepath );
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

		if ( !isSupported( fname ) )
			tx->id = 0xFFFFFFFF;
	}

	if ( tx->id == 0xFFFFFFFF )
		return 0;

	QByteArray outData;

	if ( tx->filepath.isEmpty() || tx->reload )
		tx->filepath = find( tx->filename, nifFolder, outData );

	if ( !outData.isEmpty() || tx->reload ) {
		tx->data = outData;
	}

	if ( !tx->id || tx->reload ) {
		if ( QFile::exists( tx->filepath ) && QFileInfo( tx->filepath ).isWritable()
			 && ( !watcher->files().contains( tx->filepath ) ) )
			watcher->addPath( tx->filepath );

		tx->load();
	} else {
		if ( !tx->target )
			tx->target = GL_TEXTURE_2D;

		glBindTexture( tx->target, tx->id );
	}

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
						texLoad( iData, tx->format, tx->target, tx->width, tx->height, tx->mipmaps, tx->id );
					}
					catch ( QString & e ) {
						tx->status = e;
					}
				} else {
					glBindTexture( GL_TEXTURE_2D, tx->id );
				}

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
			//qDebug() << "TexCache::importFile: Texture has filename (from NIF) " << filename;
			Tex * tx = textures.value( filename );
			return tx->savePixelData( nif, iSource, iData );
		}
	}

	return false;
}


/*
*  TexCache::Tex
*/

void TexCache::Tex::load()
{
	if ( !id )
		glGenTextures( 1, &id );

	width  = height = mipmaps = 0;
	reload = false;
	status = QString();

	if ( target )
		glBindTexture( target, id );

	try
	{
		texLoad( filepath, format, target, width, height, mipmaps, data, id );
	}
	catch ( QString & e )
	{
		status = e;
	}
}

bool TexCache::Tex::saveAsFile( const QModelIndex & index, QString & savepath )
{
	texLoad( index, format, target, width, height, mipmaps, id );

	if ( savepath.toLower().endsWith( ".tga" ) ) {
		return texSaveTGA( index, savepath, width, height );
	}

	return texSaveDDS( index, savepath, width, height, mipmaps );
}

bool TexCache::Tex::savePixelData( NifModel * nif, const QModelIndex & iSource, QModelIndex & iData )
{
	Q_UNUSED( iSource );
	// gltexloaders function goes here
	//qDebug() << "TexCache::Tex:savePixelData: Packing" << iSource << "from file" << filepath << "to" << iData;
	return texSaveNIF( nif, filepath, iData );
}
