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


#include "fsengine.h"
#include "bsa.h"


#include <QDateTime>
#include <QDebug>
#include <QFSFileEngine>
#include <QMap>
#include <QMutex>
#include <QRegExp>
#include <QStringList>


static QMap<QString,QStringList> overlayDirs;
static QMutex overlayMutex;


class FSOverlayEngine : public QFSFileEngine
{
public:
	FSOverlayEngine( const QString & filename )
		: QFSFileEngine( filename )
	{}
	
	QStringList entryList( QDir::Filters filters, const QStringList & nameFilters ) const
	{
		QStringList entries = QFSFileEngine::entryList( filters, nameFilters );
		
		if ( filters.testFlag( QDir::Dirs ) || filters.testFlag( QDir::AllDirs ) )
		{
			QMutexLocker lock( & overlayMutex );
			
			QString dirname = fileName( AbsoluteName );
			if ( ! caseSensitive() )
				dirname = dirname.toLower();
			
			if ( overlayDirs.contains( dirname ) )
			{
				QStringList list = overlayDirs.value( dirname );
				qDebug() << dirname << "overlay" << list;
				
				foreach ( QString x, list )
				{
					if ( ! entries.contains( x ) )
					{
						entries += x;
					}
				}
			}
		}
		
		return entries;
	}
	
	FileFlags fileFlags( FileFlags type = FileInfoAll ) const
	{
		FileFlags x = QFSFileEngine::fileFlags( type );
		
		if ( type.testFlag( DirectoryType ) )
		{
			QMutexLocker lock( & overlayMutex );
			
			QString base = fileName( AbsolutePathName );
			if ( ! caseSensitive() )
				base = base.toLower();
			
			qDebug() << "base" << base;
			
			if ( overlayDirs.contains( base ) )
			{
				QString name = fileName( BaseName );
				if ( ! caseSensitive() )
					name = name.toLower();
				
				qDebug() << "name" << name;
				
				if ( overlayDirs[ base ].contains( name ) )
				{
					x |= DirectoryType;
					qDebug() << "flags |= Dir";
				}
			}
		}
		
		return x;
	}
};

QAbstractFileEngine * FSOverlayHandler::create( const QString & fileName ) const
{
	if ( fileName.startsWith( ":" ) )
		return 0;
	else
		return new FSOverlayEngine( fileName );
}



FSArchiveHandler * FSArchiveHandler::openArchive( const QString & fn )
{
	if ( BSA::canOpen( fn ) )
	{
		BSA * bsa = new BSA( fn );
		if ( bsa->open() )
			return new FSArchiveHandler( bsa );
		qDebug() << "fsengine error:" << fn << ":" << bsa->statusText();
		delete bsa;
	}
	return 0;
}


class FSArchiveEngine : public QAbstractFileEngine
{
	FSArchiveFile * archive;
	
	QString orgFilePath;
	QString relFilePath;
	
	QIODevice::OpenMode fileMode;
	QByteArray fileData;
	qint64 filePos;
	
public:
	FSArchiveEngine( FSArchiveFile * a, const QString & org, const QString & rel ) : QAbstractFileEngine()
	{
		archive = a;
		archive->ref.ref();
		
		fileMode = QIODevice::NotOpen;
		filePos = 0;
		
		orgFilePath = org;
		relFilePath = rel;
		
		qDebug() << "archive engine" << relFilePath;
	}
	
	~FSArchiveEngine()
	{
		if ( ! archive->ref.deref() )
			delete archive;
	}
	
	FileFlags fileFlags( FileFlags type = FileInfoAll ) const
	{
		FileFlags flags( 0 );
		
		if ( ( type.testFlag( FileType ) || type.testFlag( ExistsFlag ) ) && archive->hasFile( relFilePath ) )
		{
			flags |= FileType;
			flags |= ReadOtherPerm;
			flags |= ExistsFlag;
		}
		if ( ( type.testFlag( DirectoryType ) || type.testFlag( ExistsFlag ) ) && archive->hasFolder( relFilePath ) )
		{
			flags |= DirectoryType;
			flags |= ReadOtherPerm;
			flags |= ExeOtherPerm;
			flags |= ExistsFlag;
		}
		
		return flags;
	}
	
	QStringList entryList( QDir::Filters filters, const QStringList & nameFilters ) const
	{
		QStringList list;
		
		if ( filters.testFlag( QDir::Dirs ) || filters.testFlag( QDir::AllDirs ) )
		{
			// TODO: filter dir names
			list = archive->entryList( relFilePath, QDir::Dirs );
		}
		
		if ( filters.testFlag( QDir::Files ) )
		{
			if ( nameFilters.isEmpty() )
			{
				list += archive->entryList( relFilePath, QDir::Files );
			}
			else
			{
				QList<QRegExp> wildcards;
				foreach ( QString pattern, nameFilters )
					wildcards << QRegExp( pattern, Qt::CaseInsensitive, QRegExp::Wildcard );
				
				foreach ( QString fn, archive->entryList( relFilePath, QDir::Files ) )
				{
					foreach ( QRegExp e, wildcards )
					{
						if ( e.exactMatch( fn ) )
						{
							list << fn;
							break;
						}
					}
				}
			}
		}
		
		return list;
	}
	
	void setFileName( const QString & fn )
	{
		qDebug() << "bsa engine set file name" << fn;
	}
	
	QString fileName( FileName type ) const
	{
		switch ( type )
		{
			case BaseName:
				{
					QString bn = orgFilePath;
					int x = bn.lastIndexOf( "/" );
					if ( x >= 0 )
						bn.remove( 0, x + 1 );
					qDebug() << "name" << bn;
					return bn;
				}	break;
			case PathName:
				{
					QString pn = orgFilePath;
					int x = pn.lastIndexOf( "/" );
					if ( x >= 0 )
						pn = pn.left( x );
					qDebug() << "path" << pn;
					return pn;
				}	break;
			case AbsolutePathName:
				{
					QString pn = archive->path();
					if ( ! relFilePath.isEmpty() )
						pn += "/" + relFilePath;
					int x = pn.lastIndexOf( "/" );
					if ( x >= 0 )
						pn = pn.left( x );
					qDebug() << "abspath" << pn;
					return pn;
				}	break;
			default:
				break;
		}
		return orgFilePath;
	}
	
	bool isRelativePath() const
	{
		// todo
		return false;
	}
	
	bool open( QIODevice::OpenMode mode )
	{
		if ( fileMode != QIODevice::NotOpen || mode != QIODevice::ReadOnly )
			return false;
		
		if ( archive->fileContents( relFilePath, fileData ) )
		{
			fileMode = mode;
			filePos = 0;
			return true;
		}
		else
			return false;
	}
	
	bool close()
	{
		if ( fileMode != QIODevice::NotOpen )
		{
			fileMode = QIODevice::NotOpen;
			filePos = 0;
			fileData.clear();
			return true;
		}
		else
			return false;
	}
	
	qint64 size() const
	{
		if ( fileMode == QIODevice::NotOpen )
			return archive->fileSize( relFilePath );
		else
			return fileData.size();
	}
	
	bool seek( qint64 ofs )
	{
		if ( fileMode != QIODevice::NotOpen && ofs >= 0 && ofs < fileData.size() )
		{
			filePos = ofs;
			return true;
		}
		return false;
	}
	
	qint64 pos() const
	{
		return filePos;
	}
	
	qint64 readLine( char * data, qint64 maxlen )
	{
		qint64 numread = 0;
		while ( numread < maxlen && filePos < fileData.size() )
		{
			*data = fileData.at( filePos++ );
			numread++;
			if ( *data == '\n' )
				break;
			else
				data++;
		}
		return numread;
	}
	
	qint64 read( char * data, qint64 maxlen )
	{
		qint64 numread = fileData.size() - filePos;
		if ( numread > maxlen )
			numread = maxlen;
		memcpy( data, ( fileData.data() + filePos ), numread );
		filePos += numread;
		return numread;
	}
	
	qint64 write( const char * data, qint64 len )
	{
		return -1;
	}
	
	int handle() const
	{
		return 0;
	}
	
	bool flush()
	{
		return true;
	}
	
	uint ownerId( FileOwner type ) const
	{
		return archive->ownerId( relFilePath, type );
	}
	
	QString owner( FileOwner type ) const
	{
		return archive->owner( relFilePath, type );
	}
	
	QDateTime fileTime( FileTime type ) const
	{
		return archive->fileTime( relFilePath, type );
	}
	
	bool copy( const QString & newFile )
	{
		return false;
	}
	
	bool isSequential() const
	{
		return false;
	}
	
	bool caseSensitive() const
	{
		return false;
	}
	
	bool setPermissions( uint ) { return false; }
	bool setSize( qint64 ) { return false; }
	bool mkdir( const QString &, bool ) const { return false; }
	bool rmdir( const QString &, bool ) const { return false; }
	bool rename( const QString & ) { return false; }
	bool remove() { return false; }
	bool link( const QString & ) { return false; }
};


FSArchiveHandler::FSArchiveHandler( FSArchiveFile * a )
	: QAbstractFileEngineHandler()
{
	archive = a;
	archive->ref.ref();
	
	QMutexLocker lock( & overlayMutex );
	
	overlayDirs[ archive->base() ].append( archive->name() );
}

FSArchiveHandler::~FSArchiveHandler()
{
	QMutexLocker lock( & overlayMutex );
	
	overlayDirs[ archive->base() ].removeAll( archive->name() );
	
	if ( ! archive->ref.deref() )
		delete archive;
}

QAbstractFileEngine * FSArchiveHandler::create( const QString & filename ) const
{
	QString fn = filename.toLower();
	fn.replace( "\\", "/" );
	return archive->stripBasePath( fn ) ? new FSArchiveEngine( archive, filename, fn ) : 0;
}



#ifdef BSA_TEST

#include <QApplication>
#include <QCompleter>
#include <QDirModel>
#include <QLineEdit>
#include <QTreeView>

int main( int argc, char * argv[] )
{
	QApplication app( argc, argv );
	
	QString fn = "g:\\nif\\mw\\data files\\bsa.bsa"; //"f:\\data\\Oblivion - Misc.bsa";
	if ( argc > 1 )
		fn = argv[ argc - 1 ];
	
	FSOverlayHandler overlayHandler;
	
	if ( FSArchiveHandler::openArchive( fn ) )
	{
		qDebug() << fn << ":status:" << QString( "open" );
		
		QDirModel mdl;
		mdl.setLazyChildCount( true );
		mdl.setSorting( QDir::DirsFirst | QDir::Name );
		
		QTreeView view;
		view.resize( 600, 600 );
		view.move( 600, 100 );
		view.setModel( &mdl );
		view.show();
		
		QLineEdit line;
		line.setCompleter( new QCompleter( new QDirModel( QStringList() << "*.nif", QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot, QDir::DirsFirst, & line ), & line ) );
		line.show();
		
		return app.exec();
	}
	
	return 0;
}

#endif
