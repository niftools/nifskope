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

#include "bsa.h"

#include <QByteArray>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>

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
static bool BSAReadSizedString( QFile & bsa, QString & s )
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
	: FSArchiveFile(), bsa( filename ), bsaInfo( QFileInfo(filename) ), status( "initialized" )
{
	bsaPath = bsaInfo.absolutePath() + "/" + bsaInfo.fileName();
	bsaBase = bsaInfo.absolutePath();
	bsaName = bsaInfo.fileName();
}

// see bsa.h
BSA::~BSA()
{
	close();
}

// see bsa.h
bool BSA::canOpen( const QString & fn )
{
	QFile f( fn );
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
			
			//qDebug() << bsaName << header;
			
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
			
			//qDebug() << bsa.pos() - header.FileNameLength << fileNames;
			
			if ( ! bsa.seek( header.FolderRecordOffset ) )
				throw QString( "folder info seek" );
			
			QVector<OBBSAFolderInfo> folderInfos( header.FolderCount );
			if ( bsa.read( (char *) folderInfos.data(), header.FolderCount * sizeof( OBBSAFolderInfo ) ) != header.FolderCount * sizeof( OBBSAFolderInfo ) )
				throw QString( "folder info read" );
			
			quint32 totalFileCount = 0;
			
			for ( const OBBSAFolderInfo folderInfo : folderInfos )
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
				
				for ( const OBBSAFileInfo fileInfo : fileInfos )
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
	
	if ( const BSAFolder * folder = getFolder( folderName ) ) {
		return folder->files.value( fileName );
	}
	else {
		return nullptr;
	}
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
uint BSA::ownerId( const QString & ) const
{
	return bsaInfo.ownerId( );
}

// see bsa.h
QString BSA::owner( const QString & ) const
{
	return bsaInfo.owner( );
}

// see bsa.h
QDateTime BSA::fileTime( const QString & ) const
{
	return bsaInfo.created( );
}

bool BSA::scan( const BSA::BSAFolder * folder, QStandardItem * item, QString path )
{
	if ( !folder || folder->children.count() == 0 )
		return false;

	QHash<QString, BSA::BSAFolder *>::const_iterator i;
	for ( i = folder->children.begin(); i != folder->children.end(); i++ ) {

		if ( !i.value()->files.count() && !i.value()->children.count() )
			continue;

		auto folderItem = new QStandardItem( i.key() );
		auto pathDummy = new QStandardItem( "" );
		auto sizeDummy = new QStandardItem( "" );

		item->appendRow( { folderItem, pathDummy, sizeDummy } );

		// Recurse through folders
		if ( i.value()->children.count() ) {
			QString fullpath = ((path.isEmpty()) ? path : path + "/") + i.key();
			scan( i.value(), folderItem, fullpath );
		}

		// List files
		if ( i.value()->files.count() ) {
			QHash<QString, BSA::BSAFile *>::const_iterator f;
			for ( f = i.value()->files.begin(); f != i.value()->files.end(); f++ ) {
				QString fullpath = path + "/" + i.key() + "/" + f.key();

				int bytes = f.value()->size();
				QString filesize = (bytes > 1024) ? QString::number( bytes / 1024 ) + "KB" : QString::number( bytes ) + "B";

				auto fileItem = new QStandardItem( f.key() );
				auto pathItem = new QStandardItem( fullpath );
				auto sizeItem = new QStandardItem( filesize );

				folderItem->appendRow( { fileItem, pathItem, sizeItem } );
			}
		}
	}

	return true;
}

bool BSA::fillModel( BSAModel * bsaModel, const QString & folder )
{
	return scan( getFolder( folder ), bsaModel->invisibleRootItem(), folder );
}


BSAModel::BSAModel( QObject * parent )
	: QStandardItemModel( parent )
{

}

void BSAModel::init()
{
	setColumnCount( 3 );
	setHorizontalHeaderLabels( { "File", "Path", "Size" } );
}

Qt::ItemFlags BSAModel::flags( const QModelIndex & index ) const
{
	return QStandardItemModel::flags( index ) ^ Qt::ItemIsEditable;
}


BSAProxyModel::BSAProxyModel( QObject * parent )
	: QSortFilterProxyModel( parent )
{

}

void BSAProxyModel::setFiletypes( QStringList types )
{
	filetypes = types;
}

void BSAProxyModel::setFilterByNameOnly( bool nameOnly )
{
	filterByNameOnly = nameOnly;

	setFilterRegExp( filterRegExp() );
}

void BSAProxyModel::resetFilter()
{
	setFilterRegExp( QRegExp( "*", Qt::CaseInsensitive, QRegExp::Wildcard ) );
}

bool BSAProxyModel::filterAcceptsRow( int sourceRow, const QModelIndex & sourceParent ) const
{
	if ( !filterRegExp().isEmpty() ) {
		
		QModelIndex sourceIndex0 = sourceModel()->index( sourceRow, 0, sourceParent );
		QModelIndex sourceIndex1 = sourceModel()->index( sourceRow, 1, sourceParent );
		if ( sourceIndex0.isValid() ) {
			// If children match, parent matches
			int c = sourceModel()->rowCount( sourceIndex0 );
			for ( int i = 0; i < c; ++i ) {
				if ( filterAcceptsRow( i, sourceIndex0 ) )
					return true;
			}

			QString key0 = sourceModel()->data( sourceIndex0, filterRole() ).toString();
			QString key1 = sourceModel()->data( sourceIndex1, filterRole() ).toString();
			
			bool typeMatch = true;
			if ( filetypes.count() ) {
				typeMatch = false;
				for ( auto f : filetypes ) {
					typeMatch |= key1.endsWith( f );
				}
			}

			bool stringMatch = (filterByNameOnly) ? key0.contains( filterRegExp() ) : key1.contains( filterRegExp() );

			return typeMatch && stringMatch;
		}
	}

	return QSortFilterProxyModel::filterAcceptsRow( sourceRow, sourceParent );
}

bool BSAProxyModel::lessThan( const QModelIndex & left, const QModelIndex & right ) const
{
	QString leftString = sourceModel()->data( left ).toString();
	QString rightString = sourceModel()->data( right ).toString();

	QModelIndex leftChild = left.child( 0, 0 );
	QModelIndex rightChild = right.child( 0, 0 );

	if ( !leftChild.isValid() && rightChild.isValid() )
		return false;

	if ( leftChild.isValid() && !rightChild.isValid() )
		return true;

	return leftString < rightString;
}
