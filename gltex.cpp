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

#ifdef QT_OPENGL_LIB

#include <QtOpenGL>

#include <QDebug>
#include <QDir>
#include <QFileInfo>

#include "glscene.h"

#ifndef APIENTRY
# define APIENTRY
#endif
typedef void (APIENTRY *pfn_glCompressedTexImage2D) ( GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const GLvoid * );
static pfn_glCompressedTexImage2D _glCompressedTexImage2D = 0;

void GLTex::initialize( const QGLContext * context )
{
	QString extensions( (const char *) glGetString(GL_EXTENSIONS) );
	
	if ( !_glCompressedTexImage2D )
	{
		if (!extensions.contains("GL_ARB_texture_compression"))
			qWarning( "need OpenGL extension GL_ARB_texture_compression for DDS textures" );
		if (!extensions.contains("GL_EXT_texture_compression_s3tc"))
			qWarning( "need OpenGL extension GL_EXT_texture_compression_s3tc for DDS textures" );
		
		_glCompressedTexImage2D = (pfn_glCompressedTexImage2D) context->getProcAddress("glCompressedTexImage2D");
		if ( !_glCompressedTexImage2D )
			_glCompressedTexImage2D = (pfn_glCompressedTexImage2D) context->getProcAddress("glCompressedTexImage2DARB");
	}
	if ( !_glCompressedTexImage2D )
		qWarning( "glCompressedTexImage2D not found" );
}

GLTex::~GLTex()
{
	release();
}

void GLTex::release()
{
	if ( id )
		glDeleteTextures( 1, &id );
	id = 0;
}


GLuint texLoadBMP( const QString & filepath );
GLuint texLoadDDS( const QString & filepath );
GLuint texLoadTGA( const QString & filepath );
GLuint texLoadRaw( QIODevice & dev, int w, int h, int m, int bitspp, int bytespp, const quint32 masks[], bool flipH = false, bool flipV = false, bool rle = false );


bool Scene::bindTexture( const QModelIndex & index )
{
	if ( ! index.isValid() )
		return false;
	
	foreach ( GLTex * tex, textures )
	{
		if ( tex->iSource == index )
		{
			if ( tex->id )
			{
				glBindTexture( GL_TEXTURE_2D, tex->id );
				return true;
			}
			return false;
		}
	}
	
	GLTex * tex = new GLTex( index, this );
	textures.append( tex );
	
	if ( tex->id )
	{
		glBindTexture( GL_TEXTURE_2D, tex->id );
		return true;
	}
	
	return false;
}

GLTex::GLTex( const QModelIndex & index, Scene * scene ) : id( 0 ), iSource( index )
{
	const NifModel * nif = static_cast<const NifModel *>( iSource.model() );
	if ( iSource.isValid() && nif )
	{
		QModelIndex iTexSource = nif->getIndex( iSource, "texture source" );
		if ( iTexSource.isValid() )
		{
			external = nif->get<bool>( iTexSource, "use external" );
			if ( external )
			{
				QString filename = nif->get<QString>( iTexSource, "file name" ).toLower();
				
				if ( filename.startsWith( "/" ) or filename.startsWith( "\\" ) )
					filename = filename.right( filename.length() - 1 );
				
				// attempt to find the texture in one of the folders
				QDir dir;
				foreach ( QString folder, scene->texfolders )
				{
					dir.setPath( folder );
					if ( dir.exists( filename ) )
						break;
					if ( dir.exists( "../" + filename ) )
					{
						dir.cd( ".." );
						break;
					}
				}
				
				if ( dir.exists( filename ) )
				{
					filepath = dir.filePath( filename );
					readOnly = !QFileInfo( filepath ).isWritable();
					loaded = QDateTime::currentDateTime();
					
					if ( filepath.endsWith( ".dds" ) )
						id = texLoadDDS( filepath );
					else if ( filepath.endsWith( ".tga" ) )
						id = texLoadTGA( filepath );
					else if ( filepath.endsWith( ".bmp" ) )
						id = texLoadBMP( filepath );
					else
						qWarning() << "could not load texture " << filepath << " (valid image formats are DDS TGA and BMP)";
				}
				else
					qWarning() << "texture" << filename << "not found";
			}
			else
			{		// internal texture
				iPixelData = nif->getBlock( nif->getLink( iTexSource, "pixel data" ), "NiPixelData" );
				if ( iPixelData.isValid() )
				{
					quint32 masks[4];
					static const char * maskNames[4] = { "red mask", "green mask", "blue mask", "alpha mask" };
					for ( int c = 0; c < 4; c++ )
						masks[c] = nif->get<int>( iPixelData, maskNames[c] );
					qint32 bitspp = nif->get<int>( iPixelData, "bits per pixel" );
					qint32 bytespp = nif->get<int>( iPixelData, "bytes per pixel" );
					qint32 mipmaps = nif->get<int>( iPixelData, "num mipmaps" );
					
					QModelIndex iMipmaps = nif->getIndex( iPixelData, "mipmaps" );
					if ( iMipmaps.isValid() && nif->rowCount( iMipmaps ) >= 1 )
					{
						qint32 width = nif->get<int>( iMipmaps.child( 0, 0 ), "width" );
						qint32 height = nif->get<int>( iMipmaps.child( 0, 0 ), "height" );
						qint32 offset = nif->get<int>( iMipmaps.child( 0, 0 ), "offset" );
						
						QByteArray pixels = nif->get<QByteArray>( iPixelData, "pixel data" );
						QBuffer buffer( &pixels );
						if ( buffer.open( QIODevice::ReadOnly ) )
						{
							buffer.seek( offset );
							id = texLoadRaw( buffer, width, height, 1, bitspp, bytespp, masks );
						}
					}
				}
			}
		}
	}
}

bool isPowerOfTwo( unsigned int x )
{
	while ( ! ( x == 0 || x & 1 ) )
		x = x >> 1;
	return ( x == 1 );
}

bool uncompressRLE( QIODevice & f, int w, int h, int bpp, quint8 * pixel )
{
	int bytespp = bpp / 8;
	
	int c = 0;
	quint8 rl;
	while ( c < w * h )
	{
		if ( !f.getChar( (char *) &rl ) )
			return false;
		
		if ( rl & 0x80 )
		{
			quint8 px[4];
			for ( int b = 0; b < bytespp; b++ )
				if ( ! f.getChar( (char *) &px[b] ) )
					return false;
			rl &= 0x7f;
			do
			{
				for ( int b = 0; b < bytespp; b++ )
					*pixel++ = px[b];
			} while ( ++c < w*h && rl-- > 0  );
		}
		else
		{
			rl &= 0x7f;
			do
			{
				for ( int b = 0; b < bytespp; b++ )
					if ( ! f.getChar( (char *) pixel++ ) )
						return false;
			} while (  ++c < w*h && rl-- > 0 );
		}
	}
	return true;
}

void convertToRGBA( const quint8 * data, int w, int h, int bytespp, const quint32 mask[], bool flipV, bool flipH, quint8 * pixl )
{
	memset( pixl, 0, w * h * 4 );

	static const int rgbashift[4] = { 0, 8, 16, 24 };
	
	for ( int a = 0; a < 4; a++ )
	{
		if ( mask[a] )
		{
			quint32 msk = mask[ a ];
			int rshift = 0;
			while ( msk != 0 && msk & 0xffffff00 )		{	msk = msk >> 1;	rshift++;	}
			int lshift = rgbashift[ a ];
			while ( msk != 0 && ( msk & 0x80 == 0 ) )	{	msk = msk << 1;	lshift++;	}
			msk = mask[ a ];
			
			const quint8 * src = data;
			const quint32 inc = ( flipH ? -1 : 1 );
			for ( int y = 0; y < h; y++ )
			{
				quint32 * dst = (quint32 *) ( pixl + 4 * ( w * ( flipV ? h - y - 1 : y ) + ( flipH ? w - 1 : 0 ) ) );
				for ( int x = 0; x < w; x++ )
				{
					*dst |= ( *( (const quint32 *) src ) & msk ) >> rshift << lshift;
					dst += inc;
					src += bytespp;
				}
			}
		}
		else
		{
			quint32 * dst = (quint32 *) pixl;
			quint32 x = 0xff << rgbashift[ a ];
			for ( int c = w * h; c > 0; c-- )
				*dst++ |= x;
		}
	}
}

GLuint texLoadRaw( QIODevice & f, int width, int height, int num_mipmaps, int bpp, int bytespp, const quint32 mask[], bool flipV, bool flipH, bool rle )
{
	if ( bpp != 32 && bpp != 24 )
	{	// check image depth
		qWarning( "texLoadRaw() : unsupported image depth %i", bpp );
		return 0;
	}
	
	GLuint tx_id;
	glGenTextures(1, &tx_id);
	glBindTexture(GL_TEXTURE_2D, tx_id);
//	glTexParameteri( GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, ( num_mipmaps > 1 ? GL_FALSE : GL_TRUE ) );
	
	
	glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
	glPixelStorei( GL_UNPACK_SWAP_BYTES, GL_FALSE );
	
	quint8 * data1 = (quint8 *) malloc( width * height * 4 );
	quint8 * data2 = (quint8 *) malloc( width * height * 4 );
	
	int w = width;
	int h = height;
	
	for ( int m = 0; m < num_mipmaps; m++ )
	{
		if ( w == 0 ) w = 1;
		if ( h == 0 ) h = 1;
		
		if ( rle )
		{
			if ( ! uncompressRLE( f, w, h, bpp, data1 ) )
				qWarning() << "texLoadRaw() : unexpected EOF";
		}
		else if ( f.read( (char *) data1, w * h * bytespp ) != w * h * bytespp )
		{
			qWarning() << "texLoadRaw() : unexpected EOF";
		}
		
		convertToRGBA( data1, w, h, bytespp, mask, flipV, flipH, data2 );
		
		glTexImage2D( GL_TEXTURE_2D, m, bpp / 8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data2 );
		w /= 2;
		h /= 2;
	}
	
	free( data2 );
	free( data1 );
	
	return tx_id;
}

#define DDSD_MIPMAPCOUNT           0x00020000
#define DDPF_FOURCC                0x00000004

// DDS format structure
struct DDSFormat {
    quint32 dwSize;
    quint32 dwFlags;
    quint32 dwHeight;
    quint32 dwWidth;
    quint32 dwLinearSize;
    quint32 dummy1;
    quint32 dwMipMapCount;
    quint32 dummy2[11];
    struct {
	quint32 dwSize;
	quint32 dwFlags;
	quint32 dwFourCC;
	quint32 dwBPP;
	quint32 dwRMask;
	quint32 dwGMask;
	quint32 dwBMask;
	quint32 dwAMask;
    } ddsPixelFormat;
};

// compressed texture pixel formats
#define FOURCC_DXT1  0x31545844
#define FOURCC_DXT2  0x32545844
#define FOURCC_DXT3  0x33545844
#define FOURCC_DXT4  0x34545844
#define FOURCC_DXT5  0x35545844

#ifndef GL_COMPRESSED_RGB_S3TC_DXT1_EXT
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT   0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT  0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT  0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT  0x83F3
#endif

/*
 *  load a (compressed) dds texture
 */
 
GLuint texLoadDDS( const QString & filename )
{
	//qDebug( "DDS" );
	
	QFile f( filename );
	if ( ! f.open( QIODevice::ReadOnly ) )
	{
		qWarning() << "texLoadDDS( " << filename << " ) : could not open file";
		return 0;
	}
	
	char tag[4];
	f.read(&tag[0], 4);
	DDSFormat ddsHeader;

	if ( strncmp(tag,"DDS ", 4) != 0 || f.read((char *) &ddsHeader, sizeof(DDSFormat)) != sizeof( DDSFormat ) )
	{
		qWarning() << "texLoadDDS( " << filename << " ) : not a DDS file";
		return 0;
	}
	
	if ( !( ddsHeader.dwFlags & DDSD_MIPMAPCOUNT ) )
		ddsHeader.dwMipMapCount = 1;
	
	if ( ! ( isPowerOfTwo( ddsHeader.dwWidth ) && isPowerOfTwo( ddsHeader.dwHeight ) ) )
	{
		qWarning() << "texLoadDDS( " << filename << " ) : image dimensions must be power of two";
		return 0;
	}

	int blockSize = 16;
	GLenum format;

	if ( ddsHeader.ddsPixelFormat.dwFlags & DDPF_FOURCC )
	{
		switch(ddsHeader.ddsPixelFormat.dwFourCC)
		{
			case FOURCC_DXT1:
				format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
				blockSize = 8;
				break;
			case FOURCC_DXT3:
				format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
				break;
			case FOURCC_DXT5:
				format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
				break;
			default:
				qWarning() << "texLoadDDS( " << filename << " ) : DDS image compression type not supported";
				return 0;
		}
	}
	else
	{
		/*
		qDebug( "UNCOMPRESSED DDS" );
		qDebug( "SIZE %i,%i,%i", ddsHeader.dwWidth, ddsHeader.dwHeight, ddsHeader.dwMipMapCount );
		qDebug( "BPP %i", ddsHeader.ddsPixelFormat.dwBPP );
		qDebug( "R %08X", ddsHeader.ddsPixelFormat.dwRMask );
		qDebug( "G %08X", ddsHeader.ddsPixelFormat.dwGMask );
		qDebug( "B %08X", ddsHeader.ddsPixelFormat.dwBMask );
		qDebug( "A %08X", ddsHeader.ddsPixelFormat.dwAMask );
		*/
		return texLoadRaw( f, ddsHeader.dwWidth, ddsHeader.dwHeight,
			ddsHeader.dwMipMapCount, ddsHeader.ddsPixelFormat.dwBPP, ddsHeader.ddsPixelFormat.dwBPP / 8,
			&ddsHeader.ddsPixelFormat.dwRMask );
	}
	
	if ( !_glCompressedTexImage2D )
	{
		qWarning() << "texLoadDDS( " << filename << " ) : DDS image compression not supported by your open gl implementation";
		return 0;
	}
	
	f.seek(ddsHeader.dwSize + 4);
	
	GLuint tx_id;
	glGenTextures(1, &tx_id);
	glBindTexture(GL_TEXTURE_2D, tx_id);
//	glTexParameteri( GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, ( ddsHeader.dwMipMapCount <= 1 ? GL_TRUE : GL_FALSE ) );
	
	//qDebug( "w %i h %i m %i", ddsHeader.dwWidth, ddsHeader.dwHeight, ddsHeader.dwMipMapCount );
	
	GLubyte * pixels = (GLubyte *) malloc(((ddsHeader.dwWidth+3)/4) * ((ddsHeader.dwHeight+3)/4) * blockSize);
	unsigned int w, h, s;
	unsigned int m = ddsHeader.dwMipMapCount;
	for(unsigned int i = 0; i < m; ++i)
	{
		w = ddsHeader.dwWidth >> i;
		h = ddsHeader.dwHeight >> i;
		if (w == 0) w = 1;
		if (h == 0) h = 1;
		s = ((w+3)/4) * ((h+3)/4) * blockSize;
		if ( f.read( (char *) pixels, s ) != s )
		{
			qWarning() << "texLoadDDS( " << filename << " ) : unexpected EOF";
			free( pixels );
			return tx_id;
		}
		_glCompressedTexImage2D( GL_TEXTURE_2D, i, format, w, h, 0, s, pixels );
	}
	
	f.close();
	
	free(pixels);
	
	return tx_id;
}

/*
 *  load a tga texture
 */
 
#define TGA_COLOR		2
#define TGA_COLOR_RLE	10

GLuint texLoadTGA( const QString & filename )
{
	QFile f( filename );
	if ( ! f.open( QIODevice::ReadOnly ) )
	{
		qWarning() << "texLoadTGA(" << filename << ") : could not open file";
		return 0;
	}
	
	// read in tga header
	quint8 hdr[18];
	qint64 readBytes = f.read((char *)hdr, 18);
	if ( readBytes != 18 )
	{
		qWarning() << "texLoadTGA(" << filename << ") : unexpected EOF";
		return 0;
	}
	if ( hdr[0] ) f.read( hdr[0] );
	
	/*
	qDebug( "xy origin %i %i", hdr[8] + 256 * hdr[9], hdr[10] + 256 * hdr[11] );
	qDebug( "depth %i", hdr[16] );
	qDebug( "alpha %02X", hdr[17] );
	*/
	unsigned int depth = hdr[16];
	unsigned int alphaValue  = hdr[17] & 15;
	bool flipV = ! ( hdr[17] & 32 );
	bool flipH = hdr[17] & 16;
	unsigned int width = hdr[12] + 256 * hdr[13];
	unsigned int height = hdr[14] + 256 * hdr[15];
	
	//qDebug() << "TGA flip V " << flipV << " H " << flipH;

	if ( ! ( isPowerOfTwo( width ) && isPowerOfTwo( height ) ) )
	{
		qWarning() << "texLoadTGA( " << filename << " ) : image dimensions must be power of two";
		return 0;
	}

	// check format and call texLoadRaw
	if ( ( depth == 32 && alphaValue == 8 ) || ( depth == 24 && alphaValue == 0 ) )
	{
		switch( hdr[2] )
		{
		case TGA_COLOR: 
		case TGA_COLOR_RLE:
			if ( depth == 32 && alphaValue == 8 )
			{
				static const quint32 TGA_RGBA_MASK[4] = { 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 };
				return texLoadRaw( f, width, height, 1, 32, 4, TGA_RGBA_MASK, flipV, flipH, hdr[2] == TGA_COLOR_RLE );
			}
			else if ( depth == 24 && alphaValue == 0 )
			{
				static const quint32 TGA_RGB_MASK[4] = { 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000 };
				return texLoadRaw( f, width, height, 1, 24, 3, TGA_RGB_MASK, flipV, flipH, hdr[2] == TGA_COLOR_RLE );
			}
			break;
		}
	}
	
	qWarning() << "texLoadTGA(" << filename << ") : image sub format not supported";
	return 0;
}

quint32 get32( quint8 * x )
{
	return *( (quint32 *) x );
}

quint16 get16( quint8 * x )
{
	return *( (quint16 *) x );
}

GLuint texLoadBMP( const QString & filename )
{
	QFile f( filename );
	if ( ! f.open( QIODevice::ReadOnly ) )
	{
		qWarning() << "texLoadBMP( " << filename << " ) : could not open file";
		return 0;
	}
	
	// read in tga header
	quint8 hdr[54];
	qint64 readBytes = f.read((char *)hdr, 54);
	
	if ( readBytes != 54 || strncmp((char*)hdr,"BM", 2) != 0)
	{
		qWarning() << "texLoadBMP( " << filename << " ) : not a BMP file";
		return 0;
	}
	
	/*
	qDebug( "size %i,%i", get32( &hdr[18] ), get32( &hdr[22] ) );
	qDebug( "plns %i", get16( &hdr[26] ) );
	qDebug( "bpp  %i", get16( &hdr[28] ) );
	qDebug( "cmpr %i", get32( &hdr[30] ) );
	qDebug( "ofs  %i", get32( &hdr[10] ) );
	*/
	
	unsigned int width = get32( &hdr[18] );
	unsigned int height = get32( &hdr[22] );
	unsigned int bpp = get16( &hdr[28] );
	unsigned int compression = get32( &hdr[30] );
	unsigned int offset = get32( &hdr[10] );
	
	if ( ! ( isPowerOfTwo( width ) && isPowerOfTwo( height ) ) )
	{
		qWarning() << "texLoadBMP( " << filename << " ) : image dimensions must be power of two";
		return 0;
	}

	if ( bpp != 24 || compression != 0 || offset != 54 )
	{
		qWarning() << "texLoadBMP( " << filename << " ) : image sub format not supported";
		return 0;
	}
	
	static const quint32 BMP_RGBA_MASK[4] = { 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000 };
	return texLoadRaw( f, width, height, 1, bpp, 3, BMP_RGBA_MASK, true );
}

#endif
