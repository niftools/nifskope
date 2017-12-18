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
#include "dds.h"
#include "zlib/zlib.h"
#include "lz4frame.h"

#include <QByteArray>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStringBuilder>


// see bsa.h
quint32 BSA::BSAFile::size() const
{
	if ( sizeFlags > 0 ) {
		// Skyrim and earlier
		return sizeFlags & OB_BSAFILE_SIZEMASK;
	}
	// TODO: Not correct for texture BA2s
	return (packedLength == 0) ? unpackedLength : packedLength;
}

// see bsa.h
bool BSA::BSAFile::compressed() const
{
	return sizeFlags & OB_BSAFILE_FLAG_COMPRESS;
}

//! Reads a foldername sized string (length + null-terminated string) from the BSA
static bool BSAReadSizedString( QFile & bsa, QString & s )
{
	quint8 len;
	if ( bsa.read( (char *)&len, 1 ) != 1 )
		return false;
	
	QByteArray b( len, char(0) );
	if ( bsa.read( b.data(), len ) == len ) {
		s = QString::fromLatin1( b );
		return true;
	}

	return false;
}

QByteArray gUncompress( const QByteArray & data, const int size )
{
	if ( data.size() <= 4 ) {
		qWarning( "gUncompress: Input data is truncated" );
		return QByteArray();
	}

	QByteArray result;

	int ret;
	z_stream strm;
	static const int CHUNK_SIZE = 1024;
	char out[CHUNK_SIZE];

	/* allocate inflate state */
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = size;
	strm.next_in = (Bytef*)(data.data());

	ret = inflateInit2( &strm, 15 + 32 ); // gzip decoding
	Q_ASSERT( ret == Z_OK );
	if ( ret != Z_OK )
		return QByteArray();

	// run inflate()
	do {
		strm.avail_out = CHUNK_SIZE;
		strm.next_out = (Bytef*)(out);

		ret = inflate( &strm, Z_NO_FLUSH );
		Q_ASSERT( ret != Z_STREAM_ERROR );  // state not clobbered

		switch ( ret ) {
		case Z_NEED_DICT:
			ret = Z_DATA_ERROR;     // and fall through
		case Z_DATA_ERROR:
		case Z_MEM_ERROR:
			(void)inflateEnd( &strm );
			return QByteArray();
		}

		result.append( out, CHUNK_SIZE - strm.avail_out );
	} while ( strm.avail_out == 0 );

	// clean up and return
	inflateEnd( &strm );
	return result;
}

// see bsa.h
BSA::BSA( const QString & filename )
	: FSArchiveFile(), bsa( filename ), bsaInfo( QFileInfo(filename) ), status( "initialized" )
{
	bsaPath = bsaInfo.absoluteFilePath();
	bsaBase = bsaInfo.absolutePath();
	bsaName = bsaInfo.fileName();

	root = new BSAFolder;
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
		if ( magic == F4_BSAHEADER_FILEID ) {
			if ( f.read( (char *) & version, sizeof( version ) ) != 4 )
				return false;
			return version == F4_BSAHEADER_VERSION;
		} else if ( magic == OB_BSAHEADER_FILEID ) {
			if ( f.read( (char *)&version, sizeof( version ) ) != 4 )
				return false;

			return (version == OB_BSAHEADER_VERSION || version == F3_BSAHEADER_VERSION || version == SSE_BSAHEADER_VERSION);
		} else {
			return magic == MW_BSAHEADER_FILEID;
		}

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
		
		quint32 magic;
		
		bsa.read( (char*) &magic, sizeof( magic ) );

		if ( magic == F4_BSAHEADER_FILEID ) {
			bsa.read( (char*)&version, sizeof( version ) );

			if ( version != F4_BSAHEADER_VERSION )
				throw QString( "file version" );

			F4BSAHeader header;
			if ( bsa.read( (char *)&header, sizeof( header ) ) != sizeof( header ) )
				throw QString( "header size" );

			numFiles = header.numFiles;

			auto offset = header.nameTableOffset;

			QVector<QString> filepaths;
			if ( bsa.seek( offset ) ) {
				for ( quint32 i = 0; i < numFiles; i++ ) {
					quint8 length;
					bsa.read( (char*)&length, 2 );

					QByteArray strdata( length, char( 0 ) );
					bsa.read( strdata.data(), length );

					filepaths.append( QString::fromLatin1( strdata ) );
				}
			}

			QString h = QString::fromLatin1( header.type, 4 );
			if ( h == "GNRL" ) {
				// General BA2 Format
				if ( bsa.seek( sizeof( header ) + 8 ) ) {
					for ( quint32 i = 0; i < numFiles; i++ ) {
						F4GeneralInfo finfo;
						bsa.read( (char*)&finfo, sizeof( F4GeneralInfo ) );

						QString fullpath = filepaths[i];
						fullpath.replace( "\\", "/" );

						QString filename = fullpath.right( fullpath.length() - fullpath.lastIndexOf( "/" ) - 1 );
						QString folderName = fullpath.left( fullpath.lastIndexOf( "/" ) );

						insertFile( insertFolder( folderName ), filename, finfo.packedSize, finfo.unpackedSize, finfo.offset );
					}
				}
			} else if ( h == "DX10" ) {
				// Texture BA2 Format
				if ( bsa.seek( sizeof( header ) + 8 ) ) {
					for ( quint32 i = 0; i < numFiles; i++ ) {
						F4Tex tex;
						bsa.read( (char*)&tex.header, 24 );

						QVector<F4TexChunk> texChunks;
						for ( quint32 j = 0; j < tex.header.numChunks; j++ ) {
							F4TexChunk texChunk;
							bsa.read( (char*)&texChunk, 24 );
							texChunks.append( texChunk );
						}

						tex.chunks = texChunks;

						QString fullpath = filepaths[i];
						fullpath.replace( "\\", "/" );

						QString filename = fullpath.right( fullpath.length() - fullpath.lastIndexOf( "/" ) - 1 );
						QString folderName = fullpath.left( fullpath.lastIndexOf( "/" ) );

						F4TexChunk chunk = tex.chunks[0];

						insertFile( insertFolder( folderName ), filename, chunk.packedSize, chunk.unpackedSize, chunk.offset, tex );
					}
				}
			}
		}
		else if ( magic == OB_BSAHEADER_FILEID )
		{
			bsa.read( (char*) &version, sizeof( version ) );
			
			if ( version != OB_BSAHEADER_VERSION && version != F3_BSAHEADER_VERSION && version != SSE_BSAHEADER_VERSION )
				throw QString( "file version" );
			
			OBBSAHeader header;
			
			if ( bsa.read( (char *) & header, sizeof( header ) ) != sizeof( header ) )
				throw QString( "header size" );
			
			numFiles = header.FileCount;
			
			//qDebug() << bsaName << header;
			
			if ( ( header.ArchiveFlags & OB_BSAARCHIVE_PATHNAMES ) == 0 || ( header.ArchiveFlags & OB_BSAARCHIVE_FILENAMES ) == 0 )
				throw QString( "header flags" );
			
			compressToggle = header.ArchiveFlags & OB_BSAARCHIVE_COMPRESSFILES;
			
			if (version == F3_BSAHEADER_VERSION || version == SSE_BSAHEADER_VERSION) {
				namePrefix = header.ArchiveFlags & F3_BSAARCHIVE_PREFIXFULLFILENAMES;
			}
			
			int folderSize = 0;
			if ( version != SSE_BSAHEADER_VERSION )
				folderSize = sizeof( OBBSAFolderInfo );
			else
				folderSize = sizeof( SEBSAFolderInfo );

			if ( !bsa.seek( header.FolderRecordOffset + header.FolderNameLength + header.FolderCount * (1 + folderSize) + header.FileCount * sizeof( OBBSAFileInfo ) ) )
				throw QString( "file name seek" );
			
			QByteArray fileNames( header.FileNameLength, char(0) );
			if ( bsa.read( fileNames.data(), header.FileNameLength ) != header.FileNameLength )
				throw QString( "file name read" );
			quint32 fileNameIndex = 0;
			
			//qDebug() << bsa.pos() - header.FileNameLength << fileNames;
			
			if ( ! bsa.seek( header.FolderRecordOffset ) )
				throw QString( "folder info seek" );

			quint32 totalFileCount = 0;
			bool ok = true;

			QVector<BSAFolderInfo> folderInfos;
			folderInfos.reserve( header.FolderCount );
			for ( quint32 i = 0; i < header.FolderCount; i++ ) {
				BSAFolderInfo info = {};

				// Hash
				ok &= bsa.read( (char *)&info, 8 ) == 8;
				// Filesize
				ok &= bsa.read( (char *)&info + 8, 4 ) == 4;
				if ( version == SSE_BSAHEADER_VERSION ) {
					// Unknown value & Offset
					ok &= bsa.read( (char *)&info + 12, 12 ) == 12;
				} else {
					// Offset
					// Note: this is reading a uint32 into a uint64 whose memory must be zeroed.
					ok &= bsa.read( (char *)&info + 16, 4 ) == 4;
				}

				if ( !ok )
					throw QString( "folder info read" );

				folderInfos << info;
			}

			for ( const BSAFolderInfo folderInfo : folderInfos ) {
				QString folderName;
				if ( ! BSAReadSizedString( bsa, folderName ) || folderName.isEmpty() )
				{
					//qDebug() << "folderName" << folderName;
					throw QString( "folder name read" );
				}
				
				
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

					QString fileName = QString::fromLatin1( fileNames.data() + fileNameIndex );
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
			
			numFiles = header.FileCount;
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
	catch ( QString & e )
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
	qDeleteAll( root->children );
	qDeleteAll( root->files );
	root->children.clear();
	root->files.clear();
	folders.clear();

	delete root;
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
				filesz -= len + 1;
				if (ok) ok = bsa.seek( file->offset + 1 + len );
			}

			quint32 filesize = filesz;
			if ( version == SSE_BSAHEADER_VERSION && file->sizeFlags > 0 && (file->compressed() ^ compressToggle) ) {
				ok = bsa.read( (char*)&filesize, 4 );
				filesz -= 4;
			}

			content.resize( filesz );
			if ( ok && bsa.read( content.data(), filesz ) == filesz ) {
				if ( file->sizeFlags > 0 && (file->compressed() ^ compressToggle) ) {
					// BSA
					if ( version != SSE_BSAHEADER_VERSION ) {
						content.remove( 0, 4 );
						QByteArray tmp3( content );

						content = gUncompress( tmp3, filesz - 4 );
					} else {
						QByteArray tmp;
						tmp.resize( filesize );

						LZ4F_decompressionContext_t dCtx = nullptr;
						LZ4F_createDecompressionContext( &dCtx, LZ4F_VERSION );
						size_t dstSize = filesize;
						size_t srcSize = content.size();

						LZ4F_decompressOptions_t options = {};

						LZ4F_decompress( dCtx, tmp.data(), &dstSize, content.data(), &srcSize, &options );
						LZ4F_errorCode_t error = LZ4F_freeDecompressionContext( dCtx );
						if ( error ) {
							// TODO: Message logger
							qDebug() << fn << "Error Code: " << error;
						}

						content = tmp;
					}
				} else if ( file->packedLength > 0 && !file->tex.chunks.count() ) {
					// General BA2
					QByteArray tmp( content );
					content.resize( file->unpackedLength );
					content = gUncompress( tmp, file->packedLength );
				} else if ( file->tex.chunks.count() ) {
					// Fill DDS Header
					DDS_HEADER ddsHeader = {};
					DDS_HEADER_DXT10 dx10Header = {};

					bool dx10 = false;

					ddsHeader.dwSize = sizeof( ddsHeader );
					ddsHeader.dwHeaderFlags = DDS_HEADER_FLAGS_TEXTURE | DDS_HEADER_FLAGS_LINEARSIZE | DDS_HEADER_FLAGS_MIPMAP;
					ddsHeader.dwHeight = file->tex.header.height;
					ddsHeader.dwWidth = file->tex.header.width;
					ddsHeader.dwMipMapCount = file->tex.header.numMips;
					ddsHeader.ddspf.dwSize = sizeof( DDS_PIXELFORMAT );
					ddsHeader.dwSurfaceFlags = DDS_SURFACE_FLAGS_TEXTURE | DDS_SURFACE_FLAGS_MIPMAP;

					if ( file->tex.header.unk16 == 2049 )
						ddsHeader.dwCubemapFlags = DDS_CUBEMAP_ALLFACES;

					bool supported = true;

					switch ( file->tex.header.format ) {
					case DXGI_FORMAT_BC1_UNORM:
						ddsHeader.ddspf.dwFlags = DDS_FOURCC;
						ddsHeader.ddspf.dwFourCC = MAKEFOURCC( 'D', 'X', 'T', '1' );
						ddsHeader.dwPitchOrLinearSize = file->tex.header.width * file->tex.header.height / 2;	// 4bpp
						break;

					case DXGI_FORMAT_BC2_UNORM:
						ddsHeader.ddspf.dwFlags = DDS_FOURCC;
						ddsHeader.ddspf.dwFourCC = MAKEFOURCC( 'D', 'X', 'T', '3' );
						ddsHeader.dwPitchOrLinearSize = file->tex.header.width * file->tex.header.height;	// 8bpp
						break;

					case DXGI_FORMAT_BC3_UNORM:
						ddsHeader.ddspf.dwFlags = DDS_FOURCC;
						ddsHeader.ddspf.dwFourCC = MAKEFOURCC( 'D', 'X', 'T', '5' );
						ddsHeader.dwPitchOrLinearSize = file->tex.header.width * file->tex.header.height;	// 8bpp
						break;

					case DXGI_FORMAT_BC5_UNORM:
						ddsHeader.ddspf.dwFlags = DDS_FOURCC;
						ddsHeader.ddspf.dwFourCC = MAKEFOURCC( 'A', 'T', 'I', '2' );
						ddsHeader.dwPitchOrLinearSize = file->tex.header.width * file->tex.header.height;	// 8bpp
						break;

					case DXGI_FORMAT_B8G8R8A8_UNORM:
						ddsHeader.ddspf.dwFlags = DDS_RGBA;
						ddsHeader.ddspf.dwRGBBitCount = 32;
						ddsHeader.ddspf.dwRBitMask = 0x00FF0000;
						ddsHeader.ddspf.dwGBitMask = 0x0000FF00;
						ddsHeader.ddspf.dwBBitMask = 0x000000FF;
						ddsHeader.ddspf.dwABitMask = 0xFF000000;
						ddsHeader.dwPitchOrLinearSize = file->tex.header.width * file->tex.header.height * 4;	// 32bpp
						break;

					case DXGI_FORMAT_R8_UNORM:
						ddsHeader.ddspf.dwFlags = DDS_RGB;
						ddsHeader.ddspf.dwRGBBitCount = 8;
						ddsHeader.ddspf.dwRBitMask = 0xFF;
						ddsHeader.dwPitchOrLinearSize = file->tex.header.width * file->tex.header.height;	// 8bpp
						break;

					case DXGI_FORMAT_BC7_UNORM:
					case DXGI_FORMAT_BC1_UNORM_SRGB:
					case DXGI_FORMAT_BC2_UNORM_SRGB:
					case DXGI_FORMAT_BC3_UNORM_SRGB:
					case DXGI_FORMAT_BC7_UNORM_SRGB:
						ddsHeader.ddspf.dwFlags = DDS_FOURCC;
						ddsHeader.ddspf.dwFourCC = MAKEFOURCC( 'D', 'X', '1', '0' );
						ddsHeader.dwPitchOrLinearSize = file->tex.header.width * file->tex.header.height;

						dx10 = true;
						dx10Header.dxgiFormat = DXGI_FORMAT( file->tex.header.format );
						break;

					default:
						supported = false;
						break;
					}

					if ( !supported )
						return false;

					char dds[sizeof( ddsHeader )];
					memcpy( dds, &ddsHeader, sizeof( ddsHeader ) );

					int texSize = 0; // = file->unpackedLength;
					int hdrSize = sizeof( ddsHeader ) + 4;

					content.clear();
					content.append( QByteArray::fromStdString( "DDS " ) );
					content.append( QByteArray::fromRawData( dds, sizeof( ddsHeader ) ) );
					Q_ASSERT( content.size() == hdrSize );

					if ( dx10 ) {
						dx10Header.resourceDimension = DDS_DIMENSION_TEXTURE2D;
						dx10Header.miscFlag = 0;
						dx10Header.arraySize = 1;
						dx10Header.miscFlags2 = 0;

						char dds2[sizeof( dx10Header )];
						memcpy( dds2, &dx10Header, sizeof( dx10Header ) );
						content.append( QByteArray::fromRawData( dds2, sizeof( dx10Header ) ) );
					}

					// Start at 1st chunk now
					for ( int i = 0; i < file->tex.chunks.count(); i++ ) {
						F4TexChunk chunk = file->tex.chunks[i];
						if ( bsa.seek( chunk.offset ) ) {
							QByteArray chunkData;

							if ( chunk.packedSize > 0 ) {
								chunkData.resize( chunk.packedSize );
								if ( bsa.read( chunkData.data(), chunk.packedSize ) == chunk.packedSize ) {
									chunkData = gUncompress( chunkData, chunk.packedSize );

									if ( chunkData.size() != chunk.unpackedSize )
										qCritical() << "Size does not match at " << chunk.offset;
								}
							} else if ( !(bsa.read( chunkData.data(), chunk.unpackedSize ) == chunk.unpackedSize) ) {
								qCritical() << "Size does not match at " << chunk.offset;
							}
							texSize += chunk.unpackedSize;

							content.append( chunkData );
							//Q_ASSERT( content.size() - hdrSize == texSize );
						} else {
							qCritical() << "Seek error";
						}
					}

				}

				return true;
			}
		}
	}
	return false;
}

// see bsa.h
QString BSA::getAbsoluteFilePath( const QString & fn ) const
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
		return root;
	
	name = name.replace( "\\", "/" ).toLower();
	
	BSAFolder * folder = folders.value( name );
	if ( !folder ) {
		folder = new BSAFolder;
		folder->name = name;
		folders.insert( name, folder );
		
		int p = name.lastIndexOf( "/" );
		if ( p >= 0 ) {
			folder->parent = insertFolder( name.left( p ) );
			folder->parent->children.insert( name.right( name.length() - p - 1 ), folder );
		} else {
			folder->parent = root;
			root->children.insert( name, folder );
		}
	}
	
	return folder;
}

// see bsa.h
BSA::BSAFile * BSA::insertFile( BSAFolder * folder, QString name, quint32 sizeFlags, quint32 offset )
{
	BSAFile * file = new BSAFile;
	file->sizeFlags = sizeFlags;
	file->offset = offset;
	folder->files.insert( name, file );

	files.insert( QString( folder->name % "/" % name ).toLower(), file );
	return file;
}

BSA::BSAFile * BSA::insertFile( BSAFolder * folder, QString name, quint32 packed, quint32 unpacked, quint64 offset, F4Tex dds )
{
	BSAFile * file = new BSAFile;
	file->tex = dds;
	file->packedLength = packed;
	file->unpackedLength = unpacked;
	file->offset = offset;
	folder->files.insert( name, file );

	files.insert( QString( folder->name % "/" % name ).toLower(), file );
	return file;
}

// see bsa.h
const BSA::BSAFolder * BSA::getFolder( QString fn ) const
{
	if ( fn.isEmpty() )
		return root;
	else
		return folders.value( fn );
}

// see bsa.h
const BSA::BSAFile * BSA::getFile( QString fn ) const
{
	return files.value( fn.toLower() );
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

	auto& children = folder->children;
	QHash<QString, BSAFolder *>::const_iterator i;
	for ( i = children.begin(); i != children.end(); ++i ) {
		auto f = i.value()->files;
		auto c = i.value()->children;
		if ( !f.count() && !c.count() )
			continue;

		auto folderItem = new QStandardItem( i.key() );
		auto pathDummy = new QStandardItem( "" );
		auto sizeDummy = new QStandardItem( "" );

		item->appendRow( { folderItem, pathDummy, sizeDummy } );

		// Recurse through folders
		if ( c.count() ) {
			QString fullpath = ((path.isEmpty()) ? path : path % "/") % i.key();
			scan( i.value(), folderItem, fullpath );
		}

		// List files
		if ( f.count() ) {
			QHash<QString, BSAFile *>::const_iterator it;
			for ( it = f.begin(); it != f.end(); ++it ) {
				QString fullpath = path % "/" % i.key() % "/" % it.key();

				int bytes = it.value()->size();
				QString filesize = (bytes > 1024) ? QString::number( bytes / 1024 ) + "KB" : QString::number( bytes ) + "B";

				auto fileItem = new QStandardItem( it.key() );
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
