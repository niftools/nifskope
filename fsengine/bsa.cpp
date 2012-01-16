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

#include "bsa.h"

#include <QByteArray>
#include <QDateTime>
#include <QDebug>

/* Default header data */
#define MW_BSAHEADER_FILEID  0x00000100 //!< Magic for Morrowind BSA
#define OB_BSAHEADER_FILEID  0x00415342 //!< Magic for Oblivion BSA, the literal string "BSA\0".
#define OB_BSAHEADER_VERSION 0x67 //!< Version number of an Oblivion BSA
#define F3_BSAHEADER_VERSION 0x68 //!< Version number of a Fallout 3 BSA

/* Archive flags */
#define OB_BSAARCHIVE_PATHNAMES           0x0001 //!< Whether the BSA has names for paths
#define OB_BSAARCHIVE_FILENAMES           0x0002 //!< Whether the BSA has names for files
#define OB_BSAARCHIVE_COMPRESSFILES       0x0004 //!< Whether the files are compressed
#define F3_BSAARCHIVE_PREFIXFULLFILENAMES 0x0100 //!< Whether the name is prefixed to the data?

/* File flags */
#define OB_BSAFILE_NIF  0x0001 //!< Set when the BSA contains NIF files
#define OB_BSAFILE_DDS  0x0002 //!< Set when the BSA contains DDS files
#define OB_BSAFILE_XML  0x0004 //!< Set when the BSA contains XML files
#define OB_BSAFILE_WAV  0x0008 //!< Set when the BSA contains WAV files
#define OB_BSAFILE_MP3  0x0010 //!< Set when the BSA contains MP3 files
#define OB_BSAFILE_TXT  0x0020 //!< Set when the BSA contains TXT files
#define OB_BSAFILE_HTML 0x0020 //!< Set when the BSA contains HTML files
#define OB_BSAFILE_BAT  0x0020 //!< Set when the BSA contains BAT files
#define OB_BSAFILE_SCC  0x0020 //!< Set when the BSA contains SCC files
#define OB_BSAFILE_SPT  0x0040 //!< Set when the BSA contains SPT files
#define OB_BSAFILE_TEX  0x0080 //!< Set when the BSA contains TEX files
#define OB_BSAFILE_FNT  0x0080 //!< Set when the BSA contains FNT files
#define OB_BSAFILE_CTL  0x0100 //!< Set when the BSA contains CTL files

/* Bitmasks for the size field in the header */
#define OB_BSAFILE_SIZEMASK 0x3fffffff //!< Bit mask with OBBSAFileInfo::sizeFlags to get the size of the file

/* Record flags */
#define OB_BSAFILE_FLAG_COMPRESS 0xC0000000 //!< Bit mask with OBBSAFileInfo::sizeFlags to get the compression status

//! \file bsa.cpp OBBSAHeader / \link OBBSAFileInfo FileInfo\endlink / \link OBBSAFolderInfo FolderInfo\endlink; MWBSAHeader, MWBSAFileSizeOffset

//! The header of an Oblivion BSA.
/*!
 * Follows OB_BSAHEADER_FILEID and OB_BSAHEADER_VERSION.
 */
struct OBBSAHeader
{
	quint32 FolderRecordOffset; //!< Offset of beginning of folder records
	quint32 ArchiveFlags; //!< Archive flags
	quint32 FolderCount; //!< Total number of folder records (OBBSAFolderInfo)
	quint32 FileCount; //!< Total number of file records (OBBSAFileInfo)
	quint32 FolderNameLength; //!< Total length of folder names
	quint32 FileNameLength; //!< Total length of file names
	quint32 FileFlags; //!< File flags

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

//! Info for a file inside an Oblivion BSA
struct OBBSAFileInfo
{
	quint64 hash; //!< Hash of the filename
	quint32 sizeFlags; //!< Size of the data, possibly with OB_BSAFILE_FLAG_COMPRESS set
	quint32 offset; //!< Offset to raw file data
};

//! Info for a folder inside an Oblivion BSA
struct OBBSAFolderInfo
{
	quint64 hash; //!< Hash of the folder name
	quint32 fileCount; //!< Number of files in folder
	quint32 offset; //!< Offset to name of this folder
};

//! The header of a Morrowind BSA
struct MWBSAHeader
{
	quint32 HashOffset; //!< Offset of hash table minus header size (12)
	quint32 FileCount; //!< Number of files in the archive
};

//! The file size and offset of an entry in a Morrowind BSA
struct MWBSAFileSizeOffset
{
	quint32 size; //!< The size of the file
	quint32 offset; //!< The offset of the file
};

// see bsa.h
quint32 BSA::BSAFile::size() const
{
	return sizeFlags & OB_BSAFILE_SIZEMASK;
}

// see bsa.h
bool BSA::BSAFile::compressed() const
{
	return sizeFlags & OB_BSAFILE_FLAG_COMPRESS;
}

//! Reads a foldername sized string (length + null-terminated string) from the BSA
static bool BSAReadSizedString( QAbstractFileEngine & bsa, QString & s )
{
	//qDebug() << "BSA is at" << bsa.pos();
	quint8 len;
	if ( bsa.read( (char *) & len, 1 ) != 1 )
	{
		//qDebug() << "bailout on" << __FILE__ << "line" << __LINE__;
		return false;
	}
	//qDebug() << "folder string length is" << len;
	
	QByteArray b( len, char(0) );
	if ( bsa.read( b.data(), len ) == len )
	{
		s = b;
		//qDebug() << "bailout on" << __FILE__ << "line" << __LINE__;
		return true;
	}
	else
	{
		//qDebug() << "bailout on" << __FILE__ << "line" << __LINE__;
		return false;
	}
}

// see bsa.h
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

// see bsa.h
BSA::~BSA()
{
	close();
}

// see bsa.h
bool BSA::canOpen( const QString & fn )
{
	QFSFileEngine f( fn );
	if ( f.open( QIODevice::ReadOnly ) )
	{
		quint32 magic, version;
		
		if ( f.read( (char *) & magic, sizeof( magic ) ) != 4 )
			return false;
		
		//qDebug() << "Magic:" << QString::number( magic, 16 );
		if ( magic == OB_BSAHEADER_FILEID )
		{
			if ( f.read( (char *) & version, sizeof( version ) ) != 4 )
				return false;
			
			return ( version == OB_BSAHEADER_VERSION || version == F3_BSAHEADER_VERSION );
		}
		else
			return magic == MW_BSAHEADER_FILEID;
	}
	
	return false;
}

// see bsa.h
bool BSA::open()
{
	QMutexLocker lock( & bsaMutex );
	
	try
	{
		if ( ! bsa.open( QIODevice::ReadOnly ) )
			throw QString( "file open" );
		
		quint32 magic, version;
		
		bsa.read( (char*) &magic, sizeof( magic ) );
		
		if ( magic == OB_BSAHEADER_FILEID )
		{
			bsa.read( (char*) &version, sizeof( version ) );
			
			if ( version != OB_BSAHEADER_VERSION && version != F3_BSAHEADER_VERSION )
				throw QString( "file version" );
			
			OBBSAHeader header;
			
			if ( bsa.read( (char *) & header, sizeof( header ) ) != sizeof( header ) )
				throw QString( "header size" );
			
			//qWarning() << bsaName << header;
			
			if ( ( header.ArchiveFlags & OB_BSAARCHIVE_PATHNAMES ) == 0 || ( header.ArchiveFlags & OB_BSAARCHIVE_FILENAMES ) == 0 )
				throw QString( "header flags" );
			
			compressToggle = header.ArchiveFlags & OB_BSAARCHIVE_COMPRESSFILES;
			
			if (version == F3_BSAHEADER_VERSION) {
				namePrefix = header.ArchiveFlags & F3_BSAARCHIVE_PREFIXFULLFILENAMES;
			} else {
				namePrefix = false;
			}
			
			if ( ! bsa.seek( header.FolderRecordOffset + header.FolderNameLength + header.FolderCount * ( 1 + sizeof( OBBSAFolderInfo ) ) + header.FileCount * sizeof( OBBSAFileInfo ) ) )
				throw QString( "file name seek" );
			
			QByteArray fileNames( header.FileNameLength, char(0) );
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
				// useless?
				/*
				qDebug() << __LINE__ << "position" << bsa.pos() << "offset" << folderInfo.offset;
				if ( folderInfo.offset < header.FileNameLength || ! bsa.seek( folderInfo.offset - header.FileNameLength ) )
					throw QString( "folder content seek" );
				*/
				
				
				QString folderName;
				if ( ! BSAReadSizedString( bsa, folderName ) || folderName.isEmpty() )
				{
					//qDebug() << "folderName" << folderName;
					throw QString( "folder name read" );
				}
				
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
		else if ( magic == MW_BSAHEADER_FILEID )
		{
			MWBSAHeader header;
			
			if ( bsa.read( (char *) & header, sizeof( header ) ) != sizeof( header ) )
				throw QString( "header" );
			
			
			compressToggle = false;
			namePrefix = false;
			
			// header is 12 bytes, hash table is 8 bytes per entry
			quint32 dataOffset = 12 + header.HashOffset + header.FileCount * 8;
			
			// file size/offset table
			QVector<MWBSAFileSizeOffset> sizeOffset( header.FileCount );
			if ( bsa.read( (char *) sizeOffset.data(), header.FileCount * sizeof( MWBSAFileSizeOffset ) ) != header.FileCount * sizeof( MWBSAFileSizeOffset ) )
				throw QString( "file size/offset" );
			
			// filename offset table
			QVector<quint32> nameOffset( header.FileCount );
			if ( bsa.read( (char *) nameOffset.data(), header.FileCount * sizeof( quint32 ) ) != header.FileCount * sizeof( quint32 ) )
				throw QString( "file name offset" );
			
			// filenames. size is given by ( HashOffset - ( 8 * number of file/size offsets) - ( 4 * number of filenames) )
			// i.e. ( HashOffset - ( 12 * number of files ) )
			QByteArray fileNames;
			fileNames.resize( header.HashOffset - 12 * header.FileCount );
			if ( bsa.read( (char *) fileNames.data(), header.HashOffset - 12 * header.FileCount ) != header.HashOffset - 12 * header.FileCount )
				throw QString( "file names" );

			// table of 8 bytes of hash values follow, but we don't need to know what they are
			// file data follows that, which is fetched by fileContents
			
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

// see bsa.h
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

// see bsa.h
qint64 BSA::fileSize( const QString & fn ) const
{
	// note: lazy size count (not accurate for compressed files)
	if ( const BSAFile * file = getFile( fn ) )
	{
		return file->size();
	}
	return 0;
}

// see bsa.h
bool BSA::fileContents( const QString & fn, QByteArray & content )
{
	//qDebug() << "entering fileContents for" << fn;
	if ( const BSAFile * file = getFile( fn ) )
	{
		QMutexLocker lock( & bsaMutex );
		if ( bsa.seek( file->offset ) )
		{
			qint64 filesz = file->size();
			bool ok = true;
			if (namePrefix) {
				char len;
				ok = bsa.read(&len, 1);
				filesz -= len;
				if (ok) ok = bsa.seek( file->offset + 1 + len );
			}
			content.resize( filesz );
			if ( ok && bsa.read( content.data(), filesz ) == filesz )
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

// see bsa.h
QString BSA::absoluteFilePath( const QString & fn ) const
{
	if ( hasFile(fn) ) {
		return QFileInfo(this->bsaPath, fn).absoluteFilePath();
	}
	return QString();
}

// see bsa.h
QStringList BSA::entryList( const QString & fn, QDir::Filters filters ) const
{
	//qDebug() << "entered BSA::entryList with name" << fn;
	if ( const BSAFolder * folder = getFolder( fn ) )
	{
		QStringList entries;
		
		// todo: filter
		
		if ( filters.testFlag( QDir::Dirs ) || filters.testFlag( QDir::AllDirs ) )
		{
			//qDebug() << "filtering for directories";
			entries += folder->children.keys();
		}
		
		if ( filters.testFlag( QDir::Files ) )
		{
			//qDebug() << "filtering for files";
			entries += folder->files.keys();
		}
		//qDebug() << "entryList:" << entries;
		
		return entries;
	}
	return QStringList();
}

// see bsa.h
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

// see bsa.h
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

// see bsa.h
BSA::BSAFile * BSA::insertFile( BSAFolder * folder, QString name, quint32 sizeFlags, quint32 offset )
{
	name = name.toLower();
	
	BSAFile * file = new BSAFile;
	file->sizeFlags = sizeFlags;
	file->offset = offset;
	
	folder->files.insert( name, file );
	return file;
}

// see bsa.h
const BSA::BSAFolder * BSA::getFolder( QString fn ) const
{
	if ( fn.isEmpty() )
		return & root;
	else
		return folders.value( fn );
}

// see bsa.h
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

// see bsa.h
bool BSA::hasFile( const QString & fn ) const
{
	return getFile( fn );
}

// see bsa.h
bool BSA::hasFolder( const QString & fn ) const
{
	return getFolder( fn );
}

// see bsa.h
uint BSA::ownerId( const QString &, QAbstractFileEngine::FileOwner type ) const
{
	return bsa.ownerId( type );
}

// see bsa.h
QString BSA::owner( const QString &, QAbstractFileEngine::FileOwner type ) const
{
	return bsa.owner( type );
}

// see bsa.h
QDateTime BSA::fileTime( const QString &, QAbstractFileEngine::FileTime type ) const
{
	return bsa.fileTime( type );
}

// see bsa.h
// TODO: rework this to use the BSA::entryList code
BSAIterator::BSAIterator( QDir::Filters filters, const QStringList & nameFilters, BSA * archive, QString & base )
	: QAbstractFileEngineIterator( filters, nameFilters ), current( None ), archive( archive ), fileIndex( -1 ), folderIndex ( -1 )
{
	archive->ref.ref();
	//qDebug() << "Entered BSAIterator with filters" << filters << "names" << nameFilters << "base" << base; 
	//qDebug() << "Archive is" << archive->path();
	//qDebug() << "Path is" << path() << "currentFilePath is" << currentFilePath();
	
	/*
	if ( filters.testFlag( QDir::Dirs ) || filters.testFlag( QDir::AllDirs ) )
	{
		qDebug() << "Requested directories";
	}

	if ( filters.testFlag( QDir::Files ) )
	{
		qDebug() << "Requested files";
	}
	*/

	currentFolder = const_cast<BSA::BSAFolder*>( archive->getFolder( base ) );
	
	folderList += currentFolder->children.keys();
	//qDebug() << "Child folders" << folderList;
	fileList += currentFolder->files.keys();
	//qDebug() << "Child files" << fileList;

	/*
	foreach( QString bsaKey, folderList )
	{
		qDebug() << "adding files from" << bsaKey;
		foreach( QString eachFile, archive->folders.value( bsaKey )->files.keys() )
		{
			QString temp = bsaKey + "/" + eachFile;
			qDebug() << "adding" << temp;
			fileList += temp;
		}
	}
	*/
}

// see bsa.h
BSAIterator::~BSAIterator()
{
	//qDebug() << "Deleting BSAIterator";
	if ( ! archive->ref.deref() )
		delete archive;
}

// see bsa.h
bool BSAIterator::hasNext() const
{
	//qDebug() << "Entered BSAIterator::hasNext";
	//qDebug() << "fileIndex" << fileIndex << "folderIndex" << folderIndex;
	return ( fileIndex < ( fileList.size() - 1 ) || folderIndex < ( folderList.size() - 1 ) );
}

// precondition: hasNext() is true
// see bsa.h
QString BSAIterator::currentFileName() const
{
	//qDebug() << "Entered BSAIterator::currentFileName";
	if( current == File )
	{
		//qDebug() << "currentFileName: file" << fileList.at( fileIndex );
		return fileList.at( fileIndex );
	}
	else if( current == Folder )
	{
		//qDebug() << "currentFileName: folder" << folderList.at( folderIndex );
		return folderList.at( folderIndex );
	}
	else
	{
		//qDebug() << "no current file";
		return QString();
	}
}

// precondition: hasNext() is true
// postCondition: we return the name of the next file
// see bsa.h
QString BSAIterator::next()
{
	//qDebug() << "Entered BSAIterator::next";
	//qDebug() << "At" << fileIndex << "of" << fileList.size() << "files";
	//qDebug() << "At" << folderIndex << "of" << folderList.size() << "folders";
	if( folderIndex < folderList.size() - 1 )
	{
		//qDebug() << "getting next folder";
		current = Folder;
		folderIndex++;
		QString nextFolder = folderList.at( folderIndex );
		//qDebug() << "next folder is" << nextFolder;
		return nextFolder;
	}
	else if( fileIndex < fileList.size() - 1 )
	{
		//qDebug() << "getting next file";
		current = File;
		fileIndex++;
		QString nextFolder = fileList.at( fileIndex );
		//qDebug() << "next file is" << nextFolder;
		return nextFolder;
	}
	else
	{
		//qDebug() << "no next, not sure how we got here";
		current = None;
		return QString();
	}
}

// see bsa.h
QString BSAIterator::currentFilePath() const
{
	//qDebug() << "Entered BSAIterator::currentFilePath";
	return QAbstractFileEngineIterator::currentFilePath();
}

// see bsa.h
QString BSAIterator::path() const
{
	//qDebug() << "Entered BSAIterator::path";
	return archive->bsaPath;
	//return QAbstractFileEngineIterator::path();
}

