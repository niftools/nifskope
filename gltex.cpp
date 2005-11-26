/****************************************************************************
**
** nifscope: a tool for analyzing and editing NetImmerse files (.nif & .kf)
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/


#ifdef QT_OPENGL_LIB

#include "glview.h"

#include <QtOpenGL>

#include <QDebug>
#include <QDir>
#include <QFileInfo>

void GLView::flushTextureCache()
{
	makeCurrent();
	textures.clear();
}

GLuint GLView::compileTexture( QString filename )
{
	GLTex * tex = textures.object( filename );
	if ( tex )
	{	// check if texture is in cache
		QFileInfo file( tex->filepath );
		if ( file.exists() )
		{
			if ( file.lastModified() < tex->loaded )
			{
				return tex->id;
			}
		}
		textures.remove( filename );
	}

	// attempt to find the texture in one of the folders
	QStringList list = texfolder.split( ";" );
	QDir dir;
	for ( int c = 0; c < list.count(); c++ )
	{
		dir.setPath( list[c] );
		if ( dir.exists( filename ) )
			break;
		if ( dir.exists( "../" + filename ) )
		{
			dir.cd( ".." );
			break;
		}
	}
	
	if ( ! dir.exists( filename ) )
	{
		qWarning() << "texture " << filename << " not found";
		return 0;
	}
	
	tex = GLTex::create( dir.filePath( filename ), context() );
	if ( tex )
	{
		textures.insert( filename, tex );
		return tex->id;
	}
	else
		return 0;
}

void uncompressRLE( quint8 * data, int w, int h, int bpp, quint8 * pixel )
{
	int bytespp = bpp / 8;
	
	int c = 0;
	while ( c < w * h )
	{
		quint8 rl = *data++;
		if ( rl & 0x80 )
		{
			quint8 px[4];
			for ( int b = 0; b < bytespp; b++ )
				px[b] = *data++;
			rl &= 0x7f;
			do
			{
				for ( int b = 0; b < bytespp; b++ )
					*pixel++ = px[b];
			} while ( rl-- > 0 && ++c < w*h );
		}
		else
		{
			rl &= 0x7f;
			do
			{
				for ( int b = 0; b < bytespp; b++ )
					*pixel++ = *data++;
			} while ( rl-- > 0 && ++c < w*h );
		}
	}
}

void convertToRGBA( quint8 * data, int w, int h, int bytespp, const quint32 mask[], bool flipV, bool flipH, quint8 * pixl )
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

GLuint texLoadRaw( QFile & f, int width, int height, int num_mipmaps, int bpp, int bytespp, const quint32 mask[], bool flipV = false, bool flipH = false, bool rle = false)
{
	if ( bpp != 32 && bpp != 24 )
	{	// check image depth
		qDebug( "texLoadRaw() : unsupported image depth %i", bpp );
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
		
		f.read( (char *) data1, w * h * bytespp );
		
		if ( rle )
		{
			uncompressRLE( data1, w, h, bpp, data2 );
			quint8 * xchg = data2;
			data2 = data1;
			data1 = xchg;
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

#ifndef APIENTRY
# define APIENTRY
#endif
typedef void (APIENTRY *pfn_glCompressedTexImage2D) ( GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const GLvoid * );
static pfn_glCompressedTexImage2D qt_glCompressedTexImage2D = 0;

/*
 *  load a (compressed) dds texture
 */
 
GLuint texLoadDDS( const QString & filename, const QGLContext * context )
{
	//qDebug( "DDS" );
	if ( !qt_glCompressedTexImage2D )
	{
		QString extensions( (const char *) glGetString(GL_EXTENSIONS) );
		if (!extensions.contains("GL_ARB_texture_compression"))
		{
			qWarning( "need OpenGL extension GL_ARB_texture_compression for DDS textures" );
			return 0;
		}
		if (!extensions.contains("GL_EXT_texture_compression_s3tc"))
		{
			qWarning( "need OpenGL extension GL_EXT_texture_compression_s3tc for DDS textures" );
			return 0;
		}
		qt_glCompressedTexImage2D = (pfn_glCompressedTexImage2D) context->getProcAddress("glCompressedTexImage2D");
	}
	if ( !qt_glCompressedTexImage2D )
	{
		qWarning( "glCompressedTexImage2D not found" );
		return 0;
	}
	
	QFile f( filename );
	if ( ! f.open( QIODevice::ReadOnly ) )
	{
		qWarning() << "texLoadDDS( " << filename << " ) : cannot open";
		return 0;
	}
	
	char tag[4];
	f.read(&tag[0], 4);
	if (strncmp(tag,"DDS ", 4) != 0)
	{
		qWarning() << "texLoadDDS( " << filename << " ) : not a DDS file";
		return 0;
	}
	
	DDSFormat ddsHeader;
	f.read((char *) &ddsHeader, sizeof(DDSFormat));
	
	if ( !( ddsHeader.dwFlags & DDSD_MIPMAPCOUNT ) )
		ddsHeader.dwMipMapCount = 1;
	
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
	
	f.seek(ddsHeader.dwSize + 4);
	
	GLuint tx_id;
	glGenTextures(1, &tx_id);
	glBindTexture(GL_TEXTURE_2D, tx_id);
//	glTexParameteri( GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, ( ddsHeader.dwMipMapCount <= 1 ? GL_TRUE : GL_FALSE ) );
	
	//qDebug( "w %i h %i m %i", ddsHeader.dwWidth, ddsHeader.dwHeight, ddsHeader.dwMipMapCount );
	
	GLubyte * pixels = (GLubyte *) malloc(((ddsHeader.dwWidth+3)/4) * ((ddsHeader.dwHeight+3)/4) * blockSize);
	int w, h, s;
	int m = ddsHeader.dwMipMapCount;
	for(int i = 0; i < m; ++i)
	{
		w = ddsHeader.dwWidth >> i;
		h = ddsHeader.dwHeight >> i;
		if (w == 0) w = 1;
		if (h == 0) h = 1;
		s = ((w+3)/4) * ((h+3)/4) * blockSize;
		f.read( (char *) pixels, s );
		qt_glCompressedTexImage2D( GL_TEXTURE_2D, i, format, w, h, 0, s, pixels );
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
		qDebug( "texLoadTGA() : could not open file %s", (const char *) filename.toAscii() );
		return 0;
	}
	
	// read in tga header
	quint8 hdr[18];
	qint64 readBytes = f.read((char *)hdr, 18);
	if ( readBytes != 18 )
		return 0;
	if ( hdr[0] ) f.read( hdr[0] );
	
	/*
	qDebug( "xy origin %i %i", hdr[8] + 256 * hdr[9], hdr[10] + 256 * hdr[11] );
	qDebug( "depth %i", hdr[16] );
	qDebug( "alpha %02X", hdr[17] );
	*/
	int depth = hdr[16];
	int alphaValue  = hdr[17] & 15;
	bool flipV = ! ( hdr[17] & 32 );
	bool flipH = hdr[17] & 16;
	int width = hdr[12] + 256 * hdr[13];
	int height = hdr[14] + 256 * hdr[15];
	
	//qDebug() << "TGA flip V " << flipV << " H " << flipH;

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
	
	qWarning( "texLoadTGA() : unsupported image format ( bpp %i, alpha %i, format %i )", depth, alphaValue, hdr[2] );
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
	if ( readBytes != 54 )
		return 0;
	
	if (strncmp((char*)hdr,"BM", 2) != 0)
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
	
	int width = get32( &hdr[18] );
	int height = get32( &hdr[22] );
	int bpp = get16( &hdr[28] );
	int compression = get32( &hdr[30] );
	int offset = get32( &hdr[10] );
	
	if ( bpp != 24 || compression != 0 || offset != 54 )
	{
		qWarning() << "texLoadBMP( " << filename << " ) : image format not supported";
		return 0;
	}
	
	static const quint32 BMP_RGBA_MASK[4] = { 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000 };
	return texLoadRaw( f, width, height, 1, bpp, 3, BMP_RGBA_MASK, true );
}


GLTex::GLTex()
{
	id = 0;
}

GLTex::~GLTex()
{
	if ( id )		glDeleteTextures( 1, &id );
}
	
GLTex * GLTex::create( const QString & filepath, const QGLContext * context )
{
	GLuint id = 0;
	
	if ( filepath.toLower().endsWith( ".dds" ) )
		id = texLoadDDS( filepath, context );
	else if ( filepath.toLower().endsWith( ".tga" ) )
		id = texLoadTGA( filepath );
	else if ( filepath.toLower().endsWith( ".bmp" ) )
		id = texLoadBMP( filepath );
	else
	{
		qWarning() << "could not load texture " << filepath << " (valid image formats are DDS TGA and BMP)";
		return 0;
	}
	
	if ( id )
	{
		GLTex * tex = new GLTex;
		tex->id = id;
		tex->filepath = filepath;
		tex->loaded = QDateTime::currentDateTime();
		return tex;
	}
	
	return 0;
}

#endif
