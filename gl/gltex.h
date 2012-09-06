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

#ifndef GLTEX_H
#define GLTEX_H

#include <QGLContext>

#include "../niftypes.h"

class QAction;
class QFileSystemWatcher;

class GroupBox;

//! \file gltex.h TexCache etc. header

//! A class for handling OpenGL textures.
/*!
 * This class stores information on all loaded textures, and watches the texture files.
 */
class TexCache : public QObject
{
	Q_OBJECT

	//! A structure for storing information on a single texture.
	struct Tex
	{
		//! The texture file name.
		QString filename;
		//! The texture file path.
		QString filepath;
		//! ID for use with GL texture functions
		GLuint id;
		//! Width of the texture
		GLuint width;
		//! Height of the texture
		GLuint height;
		//! Number of mipmaps present
		GLuint mipmaps;
		//! Determine whether the texture needs reloading
		bool reload;
		//! Format of the texture
		QString format;
		//! Status messages
		QString status;
		
		//! Load the texture 
		void load();

		//! Save the texture as a file
		bool saveAsFile( const QModelIndex & index, QString & savepath );
		//! Save the texture as pixel data
		bool savePixelData( NifModel * nif, const QModelIndex & iSource, QModelIndex & iData );
	};
	
public:
	//! Constructor
	TexCache( QObject * parent = 0 );
	//! Destructor
	~TexCache();
	
	//! Bind a texture from filename
	int bind( const QString & fname );
	//! Bind a texture from pixel data
	int bind( const QModelIndex & iSource );
	
	//! Debug function for getting info about a texture
	QString info( const QModelIndex & iSource );
	
	//! Export pixel data to a file
	bool exportFile( const QModelIndex & iSource, QString & filepath );
	//! Import pixel data from a file (not implemented yet)
	bool importFile( NifModel * nif, const QModelIndex & iSource, QModelIndex & iData );
	
	//! Find a texture based on its filename
	static QString find( const QString & file, const QString & nifFolder );
	//! Remove the path from a filename
	static QString stripPath( const QString & file, const QString & nifFolder );
	//! Checks whether the given file can be loaded
	static bool canLoad( const QString & file );
	
signals:
	void sigRefresh();

public slots:
	void flush();
	
	//! Set the folder to read textures from
	/*!
	 * If this is not set, relative paths won't resolve. The standard usage
	 * is to give NifModel::getFolder() as the argument.
	 */
	void setNifFolder( const QString & );
	
protected slots:
	void fileChanged( const QString & filepath );
	
protected:
	QHash<QString,Tex*>	textures;
	QHash<QModelIndex,Tex*>	embedTextures;
	QFileSystemWatcher * watcher;
	
	QString nifFolder;
};

float get_max_anisotropy();

void initializeTextureUnits( const QGLContext * );	

bool activateTextureUnit( int x );
void resetTextureUnits();	

#endif
