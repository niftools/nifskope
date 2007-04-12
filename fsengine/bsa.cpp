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


#include "bsa.h"

#include <QByteArray>
#include <QDateTime>
#include <QDebug>

/* Default header data */
#define OB_BSAHEADER_FILEID	0x00415342 /* "BSA\0" */
#define OB_BSAHEADER_VERSION	0x67

/* Archive flags */
#define OB_BSAARCHIVE_PATHNAMES	1
#define OB_BSAARCHIVE_FILENAMES	2
#define OB_BSAARCHIVE_COMPRESSFILES	4

/* File flags */
#define OB_BSAFILE_NIF	0x0001
#define OB_BSAFILE_DDS	0x0002
#define OB_BSAFILE_XML	0x0004
#define OB_BSAFILE_WAV	0x0008
#define OB_BSAFILE_MP3	0x0010
#define OB_BSAFILE_TXT	0x0020
#define OB_BSAFILE_HTML	0x0020
#define OB_BSAFILE_BAT	0x0020
#define OB_BSAFILE_SCC	0x0020
#define OB_BSAFILE_SPT	0x0040
#define OB_BSAFILE_TEX	0x0080
#define OB_BSAFILE_FNT	0x0080
#define OB_BSAFILE_CTL	0x0100

/* Bitmasks for the size field in the header */
#define OB_BSAFILE_SIZEMASK       0x3fffffff
#define OB_BSAFILE_FLAGMASK       0xC0000000

/* Record flags */
#define OB_BSAFILE_FLAG_COMPRESS  0xC0000000

struct OBBSAHeader
{
	quint32		FolderRecordOffset;
	quint32		ArchiveFlags;
	quint32		FolderCount;
	quint32		FileCount;
	quint32		FolderNameLength;
	quint32		FileNameLength;
	quint32		FileFlags;

	friend QDebug operator<<( QDebug dbg, const OBBSAHeader & head )
	{
		return dbg << "BSAHeader:"
			<< "\n  folder offset" << head.FolderRecordOffset
			<< "\n  archive flags" << head.ArchiveFlags
			<< "\n  folder Count" << head.FolderCount
			<< "\n  file Count" << head.FileCount
			<< "\n  folder name length" << head.FolderNameLength
			<< "\n  file name length" << head.FileNameLength
			<< "\n  file flags" << head.FileFlags;
	}
	
};

struct OBBSAFileInfo
{
	quint64 hash;
	quint32 sizeFlags;
	quint32 offset;
};

struct OBBSAFolderInfo
{
	quint64 hash;
	quint32 fileCount;
	quint32 offset;
};


struct MWBSAHeader
{
	quint32 HashOffset;
	quint32 FileCount;
};

struct MWBSAFileSizeOffset
{
	quint32 size;
	quint32 offset;
};


quint32 BSA::BSAFile::size() const
{
	return sizeFlags & OB_BSAFILE_SIZEMASK;
}
bool BSA::BSAFile::compressed() const
{
	return sizeFlags & OB_BSAFILE_FLAG_COMPRESS;
}

static bool BSAReadString( QAbstractFileEngine & bsa, QString & s )
{
	quint8 len;
	if ( bsa.read( (char *) & len, 1 ) != 1 )
		return false;
	
	QByteArray b( len, 0 );
	if ( bsa.read( b.data(), len ) == len )
	{
		s = b;
		return true;
	}
	else
		return false;
}


BSA::BSA( const QString & filename )
	: FSArchiveFile(), bsa( filename ), status( "initialized" )
{
	bsaPath = bsa.fileName( QAbstractFileEngine::AbsoluteName );
	bsaBase = bsa.fileName( QAbstractFileEngine::AbsolutePathName );
	bsaName = bsa.fileName( QAbstractFileEngine::BaseName );
	
	if ( ! bsa.caseSensitive() )
	{
		bsaPath = bsaPath.toLower();
		bsaBase = bsaBase.toLower();
		bsaName = bsaName.toLower();
	}
	
	qDebug() << "BSA" << bsaName << bsaBase << bsaPath;
}

BSA::~BSA()
{
	close();
}

bool BSA::canOpen( const QString & fn )
{
	QFSFileEngine f( fn );
	if ( f.open( QIODevice::ReadOnly ) )
	{
		quint32 magic, version;
		
		if ( f.read( (char *) & magic, sizeof( magic ) ) != 4 )
			return false;
		
		if ( magic == 0x00415342 )
		{
			if ( f.read( (char *) & version, sizeof( version ) ) != 4 )
				return false;
			
			return ( version == 0x67 );
		}
		else
			return magic == 0x00000100;
	}
	
	return false;
}

bool BSA::open()
{
	QMutexLocker lock( & bsaMutex );
	
	try
	{
		if ( ! bsa.open( QIODevice::ReadOnly ) )
			throw QString( "file open" );
		
		quint32 magic, version;
		
		bsa.read( (char*) &magic, sizeof( magic ) );
		
		if ( magic == 0x00415342 )
		{
			bsa.read( (char*) &version, sizeof( version ) );
			
			if ( version != 0x67 )
				throw QString( "file version" );
			
			OBBSAHeader header;
			
			if ( bsa.read( (char *) & header, sizeof( header ) ) != sizeof( header ) )
				throw QString( "header size" );
			
			//qWarning() << bsaName << header;
			
			if ( ( header.ArchiveFlags & OB_BSAARCHIVE_PATHNAMES ) == 0 || ( header.ArchiveFlags & OB_BSAARCHIVE_FILENAMES ) == 0 )
				throw QString( "header flags" );
			
			compressToggle = header.ArchiveFlags & OB_BSAARCHIVE_COMPRESSFILES;
			
			if ( ! bsa.seek( header.FolderRecordOffset + header.FolderNameLength + header.FolderCount * ( 1 + sizeof( OBBSAFolderInfo ) ) + header.FileCount * sizeof( OBBSAFileInfo ) ) )
				throw QString( "file name seek" );
			
			QByteArray fileNames( header.FileNameLength, 0 );
			if ( bsa.read( fileNames.data(), header.FileNameLength ) != header.FileNameLength )
				throw QString( "file name read" );
			quint32 fileNameIndex = 0;
			
			// qDebug() << file.pos() - header.FileNameLength << fileNames;
			
			if ( ! bsa.seek( header.FolderRecordOffset ) )
				throw QString( "folder info seek" );
			
			QVector<OBBSAFolderInfo> folderInfos( header.FolderCount );
			if ( bsa.read( (char *) folderInfos.data(), header.FolderCount * sizeof( OBBSAFolderInfo ) ) != header.FolderCount * sizeof( OBBSAFolderInfo ) )
				throw QString( "folder info read" );
			
			quint32 totalFileCount = 0;
			
			foreach ( OBBSAFolderInfo folderInfo, folderInfos )
			{
				/*
				qDebug() << file.pos() << folderInfos[c].offset;
				if ( folderInfos[c].offset < header.FileNameLength || ! file.seek( folderInfos[c].offset - header.FileNameLength ) )
					throw QString( "folder content seek" );
				*/
				
				QString folderName;
				if ( ! BSAReadString( bsa, folderName ) || folderName.isEmpty() )
					throw QString( "folder name read" );
				
				// qDebug() << folderName;
				
				BSAFolder * folder = insertFolder( folderName );
				
				quint32 fcnt = folderInfo.fileCount;
				totalFileCount += fcnt;
				QVector<OBBSAFileInfo> fileInfos( fcnt );
				if ( bsa.read( (char *) fileInfos.data(), fcnt * sizeof( OBBSAFileInfo ) ) != fcnt * sizeof( OBBSAFileInfo ) )
					throw QString( "file info read" );
				
				foreach ( OBBSAFileInfo fileInfo, fileInfos )
				{
					if ( fileNameIndex >= header.FileNameLength )
						throw QString( "file name size" );
					
					QString fileName = ( fileNames.data() + fileNameIndex );
					fileNameIndex += fileName.length() + 1;
					
					insertFile( folder, fileName, fileInfo.sizeFlags, fileInfo.offset );
				}
			}
			
			if ( totalFileCount != header.FileCount )
				throw QString( "file count" );
		}
		else if ( magic == 0x00000100 )
		{
			MWBSAHeader header;
			
			if ( bsa.read( (char *) & header, sizeof( header ) ) != sizeof( header ) )
				throw QString( "header" );
			
			quint32 dataOffset = 12 + header.HashOffset + header.FileCount * 8;
			
			QVector<MWBSAFileSizeOffset> sizeOffset( header.FileCount );
			if ( bsa.read( (char *) sizeOffset.data(), header.FileCount * sizeof( MWBSAFileSizeOffset ) ) != header.FileCount * sizeof( MWBSAFileSizeOffset ) )
				throw QString( "file size/offset" );
			
			QVector<quint32> nameOffset( header.FileCount );
			if ( bsa.read( (char *) nameOffset.data(), header.FileCount * sizeof( quint32 ) ) != header.FileCount * sizeof( quint32 ) )
				throw QString( "file name offset" );
			
			QByteArray fileNames;
			fileNames.resize( header.HashOffset - 12 * header.FileCount );
			if ( bsa.read( (char *) fileNames.data(), header.HashOffset - 12 * header.FileCount ) != header.HashOffset - 12 * header.FileCount )
				throw QString( "file names" );
			
			for ( quint32 c = 0; c < header.FileCount; c++ )
			{
				QString fname = fileNames.data() + nameOffset[ c ];
				QString dname;
				int x = fname.lastIndexOf( "\\" );
				if ( x > 0 )
				{
					dname = fname.left( x );
					fname = fname.remove( 0, x + 1 );
				}
				
				// qDebug() << "inserting" << dname << fname;
				
				insertFile( insertFolder( dname ), fname, sizeOffset[ c ].size, dataOffset + sizeOffset[ c ].offset );
			}
		}
		else
			throw QString( "file magic" );
	}
	catch ( QString e )
	{
		status = e;
		return false;
	}
	
	status = "loaded successful";
	
	return true;
}

void BSA::close()
{
	QMutexLocker lock( & bsaMutex );
	
	bsa.close();
	qDeleteAll( root.children );
	qDeleteAll( root.files );
	root.children.clear();
	root.files.clear();
	folders.clear();
}

qint64 BSA::fileSize( const QString & fn ) const
{
	// note: lazy size count (not accurate for compressed files)
	if ( const BSAFile * file = getFile( fn ) )
	{
		return file->size();
	}
	return 0;
}

bool BSA::fileContents( const QString & fn, QByteArray & content )
{
	if ( const BSAFile * file = getFile( fn ) )
	{
		QMutexLocker lock( & bsaMutex );
		if ( bsa.seek( file->offset ) )
		{
			content.resize( file->size() );
			if ( bsa.read( content.data(), file->size() ) == file->size() )
			{
				if ( file->compressed() ^ compressToggle )
				{
					quint8 x = content[0];
					content[0] = content[3];
					content[3] = x;
					x = content[1];
					content[1] = content[2];
					content[2] = x;
					content = qUncompress( content );
				}
				return true;
			}
		}
	}
	return false;
}

QStringList BSA::entryList( const QString & fn, QDir::Filters filters ) const
{
	if ( const BSAFolder * folder = getFolder( fn ) )
	{
		QStringList entries;
		
		// todo: filter
		
		if ( filters.testFlag( QDir::Dirs ) || filters.testFlag( QDir::AllDirs ) )
		{
			entries += folder->children.keys();
		}
		
		if ( filters.testFlag( QDir::Files ) )
		{
			entries += folder->files.keys();
		}
		
		return entries;
	}
	return QStringList();
}

bool BSA::stripBasePath( QString & p ) const
{
	QString base = bsaPath;
	
	if ( p.startsWith( base ) )
	{
		p.remove( 0, base.length() );
		if ( p.startsWith( "/" ) )
			p.remove( 0, 1 );
		return true;
	}
	return false;
}

BSA::BSAFolder * BSA::insertFolder( QString name )
{
	if ( name.isEmpty() )
		return & root;
	
	name = name.replace( "\\", "/" ).toLower();
	
	BSAFolder * folder = folders.value( name );
	if ( ! folder )
	{
		// qDebug() << "inserting" << name;
		
		folder = new BSAFolder;
		folders.insert( name, folder );
		
		int p = name.lastIndexOf( "/" );
		if ( p >= 0 )
		{
			folder->parent = insertFolder( name.left( p ) );
			folder->parent->children.insert( name.right( name.length() - p - 1 ), folder );
		}
		else
		{
			folder->parent = & root;
			root.children.insert( name, folder );
		}
	}
	
	return folder;
}

BSA::BSAFile * BSA::insertFile( BSAFolder * folder, QString name, quint32 sizeFlags, quint32 offset )
{
	name = name.toLower();
	
	BSAFile * file = new BSAFile;
	file->sizeFlags = sizeFlags;
	file->offset = offset;
	
	folder->files.insert( name, file );
	return file;
}

const BSA::BSAFolder * BSA::getFolder( QString fn ) const
{
	if ( fn.isEmpty() )
		return & root;
	else
		return folders.value( fn );
}

const BSA::BSAFile * BSA::getFile( QString fn ) const
{
	QString folderName;
	QString fileName = fn;
	int p = fn.lastIndexOf( "/" );
	if ( p >= 0 )
	{
		folderName = fn.left( p );
		fileName = fn.right( fn.length() - p - 1 );
	}
	
	if ( const BSAFolder * folder = getFolder( folderName ) )
		return folder->files.value( fileName );
	else
		return 0;
}

bool BSA::hasFile( const QString & fn ) const
{
	return getFile( fn );
}

bool BSA::hasFolder( const QString & fn ) const
{
	return getFolder( fn );
}

uint BSA::ownerId( const QString &, QAbstractFileEngine::FileOwner type ) const
{
	return bsa.ownerId( type );
}

QString BSA::owner( const QString &, QAbstractFileEngine::FileOwner type ) const
{
	return bsa.owner( type );
}

QDateTime BSA::fileTime( const QString &, QAbstractFileEngine::FileTime type ) const
{
	return bsa.fileTime( type );
}
