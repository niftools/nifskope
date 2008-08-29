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

#ifndef GLTEX_H
#define GLTEX_H

#include <QtOpenGL>

class QAction;
class QFileSystemWatcher;

class GroupBox;

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
		GLuint	id;
		GLuint	width, height, mipmaps;
		bool	reload;
		QString format;
		QString status;
		
		void load();
	};
	
public:
	TexCache( QObject * parent = 0 );
	~TexCache();
	
	int bind( const QString & fname );
	int bind( const QModelIndex& iSource );
	
	static QString find( const QString & file, const QString & nifFolder );
	static QString stripPath( const QString & file, const QString & nifFolder );
	static bool canLoad( const QString & file );
	
signals:
	void sigRefresh();

public slots:
	void flush();
	
	void setNifFolder( const QString & );
	
protected slots:
	void fileChanged( const QString & filepath );
	
protected:
	QHash<QString,Tex*>	textures;
	QHash<QModelIndex,Tex*>	embedTextures;
	QFileSystemWatcher * watcher;
	
	QString nifFolder;
};

void initializeTextureUnits( const QGLContext * );	

bool activateTextureUnit( int x );
void resetTextureUnits();	

#endif
