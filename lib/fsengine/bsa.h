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

#ifndef BSA_H
#define BSA_H

#include "fsengine.h"

#include <QStandardItemModel>
#include <QSortFilterProxyModel>

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QMutex>

#include <memory>

using namespace std;


/* Default header data */
#define MW_BSAHEADER_FILEID  0x00000100 //!< Magic for Morrowind BSA
#define OB_BSAHEADER_FILEID  0x00415342 //!< Magic for Oblivion BSA, the literal string "BSA\0".
#define F4_BSAHEADER_FILEID  0x58445442	//!< Magic for Fallout 4 BA2, the literal string "BTDX".
#define OB_BSAHEADER_VERSION 0x67 //!< Version number of an Oblivion BSA
#define F3_BSAHEADER_VERSION 0x68 //!< Version number of a Fallout 3 BSA
#define SSE_BSAHEADER_VERSION 0x69 //!< Version number of a Skyrim SE BSA
#define F4_BSAHEADER_VERSION 0x01 //!< Version number of a Fallout 4 BA2

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

	friend QDebug operator<<(QDebug dbg, const OBBSAHeader & head)
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

//! Shared info format for a folder inside a BSA
struct BSAFolderInfo
{
	quint64 hash; //!< Hash of the folder name
	quint32 fileCount; //!< Number of files in folder
	quint32 unk;
	quint64 offset; //!< Offset to name of this folder
};

//! Info for a folder inside an Oblivion BSA
struct OBBSAFolderInfo
{
	quint64 hash; //!< Hash of the folder name
	quint32 fileCount; //!< Number of files in folder
	quint32 offset; //!< Offset to name of this folder
};

//! Info for a folder inside an Skyrim SE BSA
struct SEBSAFolderInfo
{
	quint64 hash; //!< Hash of the folder name
	quint32 fileCount; //!< Number of files in folder
	quint32 unk;
	quint64 offset; //!< Offset to name of this folder
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

#pragma pack(push, 4)
struct F4BSAHeader
{
	char type[4];		// 08 GNRL=General, DX10=Textures
	quint32 numFiles;   // 0C
	quint64 nameTableOffset; // 10 - relative to start of file
};

// 24
struct F4GeneralInfo
{
	quint32 nameHash;		// 00
	char ext[4];			// 04 - extension
	quint32 dirHash;		// 08
	quint32 unk0C;			// 0C - flags? 00100100
	quint64 offset;			// 10 - relative to start of file
	quint32 packedSize;		// 18 - packed length (zlib)
	quint32 unpackedSize;	// 1C - unpacked length
	quint32 unk20;			// 20 - BAADF00D
};
#pragma pack(pop)

// 18
struct F4TexInfo
{
	quint32	nameHash;		// 00
	char	ext[4];			// 04
	quint32	dirHash;		// 08
	quint8	unk0C;			// 0C
	quint8	numChunks;		// 0D
	quint16	chunkHeaderSize;// 0E - size of one chunk header
	quint16	height;			// 10
	quint16	width;			// 12
	quint8	numMips;		// 14
	quint8	format;			// 15 - DXGI_FORMAT
	quint16	unk16;			// 16 - 0800
};

// 18
struct F4TexChunk
{
	quint64	offset;			// 00
	quint32	packedSize;		// 08
	quint32	unpackedSize;	// 0C
	quint16	startMip;		// 10
	quint16	endMip;			// 12
	quint32	unk14;			// 14 - BAADFOOD
};

struct F4Tex
{
	F4TexInfo			header;
	QVector<F4TexChunk>	chunks;
};


class BSAModel;
class BSAProxyModel;

//! \file bsa.h BSA file, BSAIterator

//! A Bethesda Software Archive file
/*!
 * See <a href="http://www.uesp.net/wiki/Main_Page">The UESP</a> for descriptions of the
 * <a href="http://www.uesp.net/wiki/Tes3Mod:BSA_File_Format">Morrowind format</a> and the
 * <a href="http://www.uesp.net/wiki/Tes4Mod:BSA_File_Format">Oblivion format</a>.
 *
 * \sa MWBSAHeader, MWBSAFileSizeOffset, OBBSAHeader, OBBSAFileInfo, OBBSAFolderInfo
 */
class BSA final : public FSArchiveFile
{
public:
	//! Constructor; creates a %BSA from the given file path.
	BSA( const QString & filePath );
	//! Destructor; closes the file.
	~BSA();
	
	//! Opens the %BSA file
	bool open() override final;
	//! Closes the %BSA file
	void close() override final;
	
	//! Returns BSA::bsaPath.
	QString path() const override final { return bsaPath; }
	//! Returns BSA::bsaBase.
	QString base() const override final { return bsaBase; }
	//! Returns BSA::bsaName.
	QString name() const override final { return bsaName; }
	
	//! Whether the specified folder exists or not
	bool hasFolder( const QString & ) const override final;
	
	//! Whether the specified file exists or not
	bool hasFile( const QString & ) const override final;
	//! Returns the size of the file per BSAFile::size().
	qint64 fileSize( const QString & ) const override final;
	//! Returns the contents of the specified file
	/*!
	* \param fn The filename to get the contents for
	* \param content Reference to the byte array that holds the file contents
	* \return True if successful
	*/
	bool fileContents( const QString &, QByteArray & ) override final;
	
	//! See QFileInfo::ownerId().
	uint ownerId( const QString & ) const override final;
	//! See QFileInfo::owner().
	QString owner( const QString & ) const override final;
	//! See QFileInfo::created().
	QDateTime fileTime( const QString & ) const override final;
	//! See QFileInfo::absoluteFilePath().
	QString getAbsoluteFilePath( const QString & ) const override final;
	
	//! Whether the given file can be opened as a %BSA or not
	static bool canOpen( const QString & );
	
	//! Returns BSA::status.
	QString statusText() const { return status; }

	//! A file inside a BSA
	struct BSAFile
	{
		// Skyrim and earlier
		quint32 sizeFlags = 0; //!< The size of the file in the BSA

		// Fallout 4
		quint32 packedLength = 0;
		quint32 unpackedLength = 0;

		quint64 offset = 0; //!< The offset of the file in the BSA

		//! The size of the file inside the BSA
		quint32 size() const;

		//! Whether the file is compressed inside the BSA
		bool compressed() const;

		F4Tex tex = {};
	};
	
	//! A folder inside a BSA
	struct BSAFolder
	{
		//! Constructor
		BSAFolder() : parent( 0 ) {}
		//! Destructor
		~BSAFolder() { qDeleteAll( children ); qDeleteAll( files ); }

		QString name;
		BSAFolder * parent; //!< The parent item
		QHash<QString, BSAFolder*> children; //!< A map of child folders
		QHash<QString, BSAFile*> files; //!< A map of files inside the folder
	};
	
	//! Recursive function to generate the tree structure of folders inside a %BSA
	BSAFolder * insertFolder( QString name );
	//! Inserts a file into the structure of a %BSA
	BSAFile * insertFile( BSAFolder * folder, QString name, quint32 sizeFlags, quint32 offset );

	BSAFile * insertFile( BSAFolder * folder, QString name, quint32 packed, quint32 unpacked, quint64 offset, F4Tex dds = F4Tex() );
	
	//! Gets the specified folder, or the root folder if not found
	const BSAFolder * getFolder( QString fn ) const;
	//! Gets the specified file, or null if not found
	const BSAFile * getFile( QString fn ) const;

	bool scan( const BSA::BSAFolder *, QStandardItem *, QString );
	bool fillModel( BSAModel *, const QString & );

protected:
	
	//! The %BSA file
	QFile bsa;
	//! File info for the %BSA
	QFileInfo bsaInfo;

	quint32 version = 0;

	//! Mutual exclusion handler
	QMutex bsaMutex;
	
	//! The absolute name of the file, e.g. "d:/temp/test.bsa"
	QString bsaPath;
	//! The base path of the file, e.g. "d:/temp"
	QString bsaBase;
	//! The name of the file, e.g. "test.bsa"
	QString bsaName;
	
	//! Map of folders inside a %BSA
	QHash<QString, BSAFolder *> folders;
	//! The root folder
	BSAFolder * root;

	//! Map of files inside a %BSA
	QHash<QString, BSAFile *> files;
	
	//! Error string for exception handling
	QString status;

	qint64 numFiles = 0;
	int filesScanned = 0;
	
	//! Whether the %BSA is compressed
	bool compressToggle = false;
	//! Whether Fallout 3 names are prefixed with an extra string
	bool namePrefix = false;
};


class BSAModel : public QStandardItemModel
{
	Q_OBJECT

public:
	BSAModel( QObject * parent = nullptr );

	void init();

	Qt::ItemFlags flags( const QModelIndex & index ) const override;
};


class BSAProxyModel : public QSortFilterProxyModel
{
	Q_OBJECT

public:
	BSAProxyModel( QObject * parent = nullptr );

	void setFiletypes( QStringList types );

	void resetFilter();

public slots:
	void setFilterByNameOnly( bool nameOnly );

protected:
	bool filterAcceptsRow( int sourceRow, const QModelIndex & sourceParent ) const;
	bool lessThan( const QModelIndex & left, const QModelIndex & right ) const;

private:
	QStringList filetypes;
	bool filterByNameOnly = false;
};

#endif
