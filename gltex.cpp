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
#include <QFileDialog>
#include <QFileInfo>

#include "glscene.h"
#include "gltex.h"

#include <GL/glext.h>

QStringList GLTex::texfolders;

#ifndef APIENTRY
# define APIENTRY
#endif
typedef void (APIENTRY *pfn_glCompressedTexImage2D) ( GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const GLvoid * );
static pfn_glCompressedTexImage2D _glCompressedTexImage2D = 0;

static bool ext_compression = false;
static bool ext_mipmapping = false;

void GLTex::initialize( const QGLContext * context )
{
	QString extensions( (const char *) glGetString(GL_EXTENSIONS) );
	//foreach ( QString e, extensions.split( " " ) )
	//	qWarning() << e;
	/*
	if ( ! extensions.contains( "GL_ARB_texture_compression" ) )
		qWarning( "need OpenGL extension GL_ARB_texture_compression for DDS textures" );
	if ( ! extensions.contains( "GL_EXT_texture_compression_s3tc" ) )
		qWarning( "need OpenGL extension GL_EXT_texture_compression_s3tc for DDS textures" );
	*/
	_glCompressedTexImage2D = (pfn_glCompressedTexImage2D) context->getProcAddress( "glCompressedTexImage2D" );
	if ( ! _glCompressedTexImage2D )
		_glCompressedTexImage2D = (pfn_glCompressedTexImage2D) context->getProcAddress( "glCompressedTexImage2DARB" );

	ext_compression = _glCompressedTexImage2D;
	if ( ! ext_compression )
		qWarning( "texture compression not supported" );

	ext_mipmapping = extensions.contains( "GL_SGIS_generate_mipmap" );
	if ( ext_mipmapping )
		glHint( GL_GENERATE_MIPMAP_HINT_SGIS, GL_NICEST );
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
				if ( tex->external )
				{
					QFileInfo info( tex->filepath );
					if ( tex->loaded < info.lastModified() && info.lastModified().secsTo( QDateTime::currentDateTime() ) > 2 )
					{
						if ( info.isWritable() )
						{
							tex->invalidate();
							break;
						}
					}
				}
				
				glBindTexture( GL_TEXTURE_2D, tex->id );
				return true;
			}
			else
				return false;
		}
	}
	
	GLTex * tex = new GLTex( index );
	textures.append( tex );
	
	if ( tex->id )
	{
		glBindTexture( GL_TEXTURE_2D, tex->id );
		return true;
	}
	
	return false;
}

GLTex::GLTex( const QModelIndex & index ) : id( 0 ), iSource( index )
{
	const NifModel * nif = static_cast<const NifModel *>( iSource.model() );
	if ( iSource.isValid() && nif )
	{
		//qWarning() << "tex" << nif->getBlockNumber( index );
		QModelIndex iTexSource = nif->getIndex( iSource, "Texture Source" );
		if ( iTexSource.isValid() )
		{
			external = nif->get<bool>( iTexSource, "Use External" );
			if ( external )
			{
				filepath = findFile( nif->get<QString>( iTexSource, "File Name" ), nif->getFolder() );
				
				if ( QFile::exists( filepath ) )
				{
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
					qWarning() << "texture" << filepath << "not found";
			}
			else
			{		// internal texture
				iPixelData = nif->getBlock( nif->getLink( iTexSource, "Pixel Data" ), "NiPixelData" );
				if ( iPixelData.isValid() )
				{
					quint32 masks[4];
					static const char * maskNames[4] = { "Red Mask", "Green Mask", "Blue Mask", "Alpha Mask" };
					for ( int c = 0; c < 4; c++ )
						masks[c] = nif->get<int>( iPixelData, maskNames[c] );
					qint32 bitspp = nif->get<int>( iPixelData, "Bits Per Pixel" );
					qint32 bytespp = nif->get<int>( iPixelData, "Bytes Per Pixel" );
					
					QModelIndex iMipmaps = nif->getIndex( iPixelData, "Mipmaps" );
					if ( iMipmaps.isValid() && nif->rowCount( iMipmaps ) >= 1 )
					{
						qint32 width = nif->get<int>( iMipmaps.child( 0, 0 ), "Width" );
						qint32 height = nif->get<int>( iMipmaps.child( 0, 0 ), "Height" );
						qint32 offset = nif->get<int>( iMipmaps.child( 0, 0 ), "Offset" );
						
						qint32 mipmaps = 1;
						qint32 w = width;
						qint32 h = height;
						qint32 o = offset;
						for ( int c = 1; c < nif->rowCount( iMipmaps ); c++ )
						{
							o += w * h * bytespp;
							if ( w > 1 ) w /= 2;
							if ( h > 1 ) h /= 2;
							if ( o == nif->get<int>( iMipmaps.child( c, 0 ), "Offset" )
								&& w == nif->get<int>( iMipmaps.child( c, 0 ), "Width" )
								&& h == nif->get<int>( iMipmaps.child( c, 0 ), "Height" ) )
								mipmaps++;
							else
								break;
						}
						
						QByteArray pixels = nif->get<QByteArray>( iPixelData, "Pixel Data" );
						QBuffer buffer( &pixels );
						if ( buffer.open( QIODevice::ReadOnly ) )
						{
							buffer.seek( offset );
							id = texLoadRaw( buffer, width, height, mipmaps, bitspp, bytespp, masks );
						}
					}
				}
			}
		}
	}
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

bool GLTex::isValid() const
{
	return ( iSource.isValid() && ( external || iPixelData.isValid() ) );
}

void GLTex::invalidate()
{
	iSource = QModelIndex();
	iPixelData = QModelIndex();
}

QString GLTex::findFile( const QString & file, const QString & additionalFolders )
{
	if ( file.isEmpty() )
		return QString();
	
	QString filename = file.toLower();
	
	while ( filename.startsWith( "/" ) or filename.startsWith( "\\" ) )
		filename.remove( 0, 1 );
	
	QStringList extensions;
	extensions << ".tga" << ".dds" << ".bmp";
	bool replaceExt = false;
	foreach ( QString ext, extensions )
		if ( filename.endsWith( ext ) )
		{
			extensions.removeAll( ext );
			extensions.prepend( ext );
			filename = filename.left( filename.length() - ext.length() );
			replaceExt = true;
			break;
		}
	
	// attempt to find the texture in one of the folders
	QDir dir;
	foreach ( QString ext, extensions )
	{
		if ( replaceExt )
			filename += ext;
		foreach ( QString folder, additionalFolders.split( ";" ) + texfolders )
		{
			dir.setPath( folder );
			if ( dir.exists( filename ) )
				return dir.filePath( filename );
			
			if ( filename.startsWith( "textures" ) && dir.exists( "../" + filename ) )
				return dir.filePath( "../" + filename );
		}
		if ( replaceExt )
			filename = filename.left( filename.length() - ext.length() );
		else
			break;
	}
	
	if ( replaceExt )
		return filename + extensions.value( 0 );
	else
		return filename;
}

bool isPowerOfTwo( unsigned int x )
{
	while ( ! ( x == 0 || x & 1 ) )
		x = x >> 1;
	return ( x == 1 );
}

/*
too bad: mipmaps aren't very well supported by my opengl driver

void generateMipMaps( int m )
{
	GLint w = 0, h = 0;
	
	glGetTexLevelParameteriv( GL_TEXTURE_2D, m, GL_TEXTURE_WIDTH, &w );
	glGetTexLevelParameteriv( GL_TEXTURE_2D, m, GL_TEXTURE_HEIGHT, &h );
	
	qWarning() << m << w << h;

	if ( w <= 1 && h <= 1 )
		return;
	
	quint8 * data = (quint8 *) malloc( w * h * 4 );
	glGetTexImage( GL_TEXTURE_2D, m, GL_RGBA, GL_UNSIGNED_BYTE, data );
	
	do
	{
		const quint8 * src = data;
		quint8 * dst = data;
		
		quint32 xo = ( w > 1 ? 1*4 : 0 );
		quint32 yo = ( h > 1 ? w*4 : 0 );
		
		w /= 2;
		h /= 2;
		
		if ( w == 0 ) w = 1;
		if ( h == 0 ) h = 1;
		
		for ( int y = 0; y < h; y++ )
		{
			for ( int x = 0; x < w; x++ )
			{
				for ( int b = 0; b < 4; b++ )
				{
					*dst++ = ( *(src+xo) + *(src+yo) + *(src+xo+yo) + *src++ ) / 4;
				}
				src += xo;
			}
			src += yo;
		}
		
		glTexImage2D( GL_TEXTURE_2D, ++m, 4, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );
		
		qWarning() << m << w << h;
	}
	while ( w > 1 || h > 1 );
	
	free( data );
}
*/

bool uncompressRLE( QIODevice & f, int w, int h, int bytespp, quint8 * pixel )
{
	QByteArray data = f.readAll();
	
	int c = 0;
	int o = 0;
	
	quint8 rl;
	while ( c < w * h )
	{
		rl = data[o++];
		if ( rl & 0x80 )
		{
			quint8 px[4];
			for ( int b = 0; b < bytespp; b++ )
				px[b] = data[o++];
			rl &= 0x7f;
			do
			{
				for ( int b = 0; b < bytespp; b++ )
					*pixel++ = px[b];
			}
			while ( ++c < w*h && rl-- > 0  );
		}
		else
		{
			do
			{
				for ( int b = 0; b < bytespp; b++ )
					*pixel++ = data[o++];
			}
			while (  ++c < w*h && rl-- > 0 );
		}
		if ( o >= data.count() ) return false;
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
			while ( msk != 0 && ( msk & 0xffffff00 ) ) { msk = msk >> 1; rshift++; }
			int lshift = rgbashift[ a ];
			while ( msk != 0 && ( ( msk & 0x80 ) == 0 ) )	{ msk = msk << 1; lshift++; }
			msk = mask[ a ];
			
			const quint8 * src = data;
			const quint32 inc = ( flipH ? -1 : 1 );
			for ( int y = 0; y < h; y++ )
			{
				quint32 * dst = (quint32 *) ( pixl + 4 * ( w * ( flipV ? h - y - 1 : y ) + ( flipH ? w - 1 : 0 ) ) );
				if ( rshift == lshift )
				{
					for ( int x = 0; x < w; x++ )
					{
						*dst |= *( (const quint32 *) src ) & msk;
						dst += inc;
						src += bytespp;
					}
				}
				else
				{
					for ( int x = 0; x < w; x++ )
					{
						*dst |= ( *( (const quint32 *) src ) & msk ) >> rshift << lshift;
						dst += inc;
						src += bytespp;
					}
				}
			}
		}
		else if ( a == 3 )
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
	if ( bytespp * 8 != bpp )
	{
		qWarning() << "texLoadRaw() : unsupported image depth" << bpp << "/" << bytespp;
		return 0;
	}
	
	if ( bpp > 32 || bpp < 8 )
	{	// check image depth
		qWarning() << "texLoadRaw() : unsupported image depth" << bpp;
		return 0;
	}
	
	GLuint tx_id;
	glGenTextures(1, &tx_id);
	glBindTexture(GL_TEXTURE_2D, tx_id);
	
	glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
	glPixelStorei( GL_UNPACK_SWAP_BYTES, GL_FALSE );
	
	quint8 * data1 = (quint8 *) malloc( width * height * 4 );
	quint8 * data2 = (quint8 *) malloc( width * height * 4 );
	
	int w = width;
	int h = height;
	int m = 0;
	
	//do
	{
		if ( w == 0 ) w = 1;
		if ( h == 0 ) h = 1;
		
		if ( rle )
		{
			if ( ! uncompressRLE( f, w, h, bytespp, data1 ) )
				qWarning() << "texLoadRaw() : unexpected EOF";
		}
		else if ( f.read( (char *) data1, w * h * bytespp ) != w * h * bytespp )
		{
			qWarning() << "texLoadRaw() : unexpected EOF";
		}
		
		convertToRGBA( data1, w, h, bytespp, mask, flipV, flipH, data2 );
		
		glTexImage2D( GL_TEXTURE_2D, m++, 4, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data2 );
		
		//if ( w == 1 && h == 1 )
		//	break;
		
		w /= 2;
		h /= 2;
	}
	//while ( m < num_mipmaps );
	
	free( data2 );
	free( data1 );
	
	//if ( w > 1 || h > 1 )
	//	generateMipMaps( m-1 );
	
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

// thanks nvidia for providing the source code to flip dxt images

typedef struct
{
	unsigned short col0, col1;
	unsigned char row[4];
} DXTColorBlock_t;

typedef struct
{
	unsigned short row[4];
} DXT3AlphaBlock_t;

typedef struct
{
	unsigned char alpha0, alpha1;
	unsigned char row[6];
} DXT5AlphaBlock_t;

void SwapMem(void *byte1, void *byte2, int size)
{
	unsigned char *tmp=(unsigned char *)malloc(sizeof(unsigned char)*size);
	memcpy(tmp, byte1, size);
	memcpy(byte1, byte2, size);
	memcpy(byte2, tmp, size);
	free(tmp);
}

inline void SwapChar( unsigned char * x, unsigned char * y )
{
	unsigned char z = *x;
	*x = *y;
	*y = z;
}

inline void SwapShort( unsigned short * x, unsigned short * y )
{
	unsigned short z = *x;
	*x = *y;
	*y = z;
}

void flipDXT1Blocks(DXTColorBlock_t *Block, int NumBlocks)
{
	int i;
	DXTColorBlock_t *ColorBlock=Block;
	for(i=0;i<NumBlocks;i++)
	{
		SwapChar( &ColorBlock->row[0], &ColorBlock->row[3] );
		SwapChar( &ColorBlock->row[1], &ColorBlock->row[2] );
		ColorBlock++;
	}
}

void flipDXT3Blocks(DXTColorBlock_t *Block, int NumBlocks)
{
	int i;
	DXTColorBlock_t *ColorBlock=Block;
	DXT3AlphaBlock_t *AlphaBlock;
	for(i=0;i<NumBlocks;i++)
	{
		AlphaBlock=(DXT3AlphaBlock_t *)ColorBlock;
		SwapShort( &AlphaBlock->row[0], &AlphaBlock->row[3] );
		SwapShort( &AlphaBlock->row[1], &AlphaBlock->row[2] );
		ColorBlock++;
		SwapChar( &ColorBlock->row[0], &ColorBlock->row[3] );
		SwapChar( &ColorBlock->row[1], &ColorBlock->row[2] );
		ColorBlock++;
	}
}

void flipDXT5Alpha(DXT5AlphaBlock_t *Block)
{
	unsigned long *Bits, Bits0=0, Bits1=0;

	memcpy(&Bits0, &Block->row[0], sizeof(unsigned char)*3);
	memcpy(&Bits1, &Block->row[3], sizeof(unsigned char)*3);

	Bits=((unsigned long *)&(Block->row[0]));
	*Bits&=0xff000000;
	*Bits|=(unsigned char)(Bits1>>12)&0x00000007;
	*Bits|=(unsigned char)((Bits1>>15)&0x00000007)<<3;
	*Bits|=(unsigned char)((Bits1>>18)&0x00000007)<<6;
	*Bits|=(unsigned char)((Bits1>>21)&0x00000007)<<9;
	*Bits|=(unsigned char)(Bits1&0x00000007)<<12;
	*Bits|=(unsigned char)((Bits1>>3)&0x00000007)<<15;
	*Bits|=(unsigned char)((Bits1>>6)&0x00000007)<<18;
	*Bits|=(unsigned char)((Bits1>>9)&0x00000007)<<21;

	Bits=((unsigned long *)&(Block->row[3]));
	*Bits&=0xff000000;
	*Bits|=(unsigned char)(Bits0>>12)&0x00000007;
	*Bits|=(unsigned char)((Bits0>>15)&0x00000007)<<3;
	*Bits|=(unsigned char)((Bits0>>18)&0x00000007)<<6;
	*Bits|=(unsigned char)((Bits0>>21)&0x00000007)<<9;
	*Bits|=(unsigned char)(Bits0&0x00000007)<<12;
	*Bits|=(unsigned char)((Bits0>>3)&0x00000007)<<15;
	*Bits|=(unsigned char)((Bits0>>6)&0x00000007)<<18;
	*Bits|=(unsigned char)((Bits0>>9)&0x00000007)<<21;
}

void flipDXT5Blocks(DXTColorBlock_t *Block, int NumBlocks)
{
	DXTColorBlock_t *ColorBlock=Block;
	DXT5AlphaBlock_t *AlphaBlock;
	int i;

	for(i=0;i<NumBlocks;i++)
	{
		AlphaBlock=(DXT5AlphaBlock_t *)ColorBlock;

		flipDXT5Alpha(AlphaBlock);
		ColorBlock++;

		SwapChar( &ColorBlock->row[0], &ColorBlock->row[3] );
		SwapChar( &ColorBlock->row[1], &ColorBlock->row[2] );
		ColorBlock++;
	}
}

void flipDXT( GLenum glFormat, int width, int height, unsigned char * image )
{
	int linesize, j;

	DXTColorBlock_t *top;
	DXTColorBlock_t *bottom;
	int xblocks=width/4;
	int yblocks=height/4;

	switch ( glFormat)
	{
		case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT: 
			linesize=xblocks*8;
			for(j=0;j<(yblocks>>1);j++)
			{
				top=(DXTColorBlock_t *)(image+j*linesize);
				bottom=(DXTColorBlock_t *)(image+(((yblocks-j)-1)*linesize));
				flipDXT1Blocks(top, xblocks);
				flipDXT1Blocks(bottom, xblocks);
				SwapMem(bottom, top, linesize);
			}
			break;

		case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
			linesize=xblocks*16;
			for(j=0;j<(yblocks>>1);j++)
			{
				top=(DXTColorBlock_t *)(image+j*linesize);
				bottom=(DXTColorBlock_t *)(image+(((yblocks-j)-1)*linesize));
				flipDXT3Blocks(top, xblocks);
				flipDXT3Blocks(bottom, xblocks);
				SwapMem(bottom, top, linesize);
			}
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
			linesize=xblocks*16;
			for(j=0;j<(yblocks>>1);j++)
			{
				top=(DXTColorBlock_t *)(image+j*linesize);
				bottom=(DXTColorBlock_t *)(image+(((yblocks-j)-1)*linesize));
				flipDXT5Blocks(top, xblocks);
				flipDXT5Blocks(bottom, xblocks);
				SwapMem(bottom, top, linesize);
			}
			break;
		default:
			return;
	}
}


GLuint texLoadDXT( QFile & f, quint32 compression, quint32 width, quint32 height, quint32 mipmaps, bool flipV = false )
{
	int blockSize = 8;
	GLenum glFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
	
	switch( compression )
	{
		case FOURCC_DXT1:
			glFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
			blockSize = 8;
			break;
		case FOURCC_DXT3:
			glFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
			blockSize = 16;
			break;
		case FOURCC_DXT5:
			glFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
			blockSize = 16;
			break;
		default:
			qWarning() << "texLoadDXT(" << f.fileName() << ") : unsupported DXT compression type";
			return 0;
	}
	
	if ( !_glCompressedTexImage2D )
	{
		qDebug() << "texLoadDXT(" << f.fileName() << ") : DXT image compression not supported by your open gl implementation";
		return 0;
	}
	
	GLuint tx_id;
	glGenTextures( 1, &tx_id );
	glBindTexture( GL_TEXTURE_2D, tx_id );
	
	GLubyte * pixels = (GLubyte *) malloc( ( ( width + 3 ) / 4 ) * ( ( height + 3 ) / 4 ) * blockSize );
	unsigned int w = width, h = height, s;
	unsigned int m = 0;
	
	//do
	{
		if ( w == 0 ) w = 1;
		if ( h == 0 ) h = 1;
		
		s = ((w+3)/4) * ((h+3)/4) * blockSize;
		
		if ( f.read( (char *) pixels, s ) != s )
		{
			qWarning() << "texLoadDXT(" << f.fileName() << ") : unexpected EOF";
			free( pixels );
			return tx_id;
		}
		
		if ( flipV )
			flipDXT( glFormat, w, h, pixels );
		
		_glCompressedTexImage2D( GL_TEXTURE_2D, m++, glFormat, w, h, 0, s, pixels );
		
		//if ( w == 1 && h == 1 )
		//	break;
		
		w /= 2;
		h /= 2;
	}
	//while ( m < mipmaps );
	
	return tx_id;
}

/*
 *  load a (compressed) dds texture
 */
 
GLuint texLoadDDS( const QString & filename )
{
	QFile f( filename );
	if ( ! f.open( QIODevice::ReadOnly ) )
	{
		qWarning() << "texLoadDDS(" << filename << ") : could not open file";
		return 0;
	}
	
	char tag[4];
	f.read(&tag[0], 4);
	DDSFormat ddsHeader;

	if ( strncmp( tag,"DDS ", 4 ) != 0 || f.read((char *) &ddsHeader, sizeof(DDSFormat)) != sizeof( DDSFormat ) )
	{
		qWarning() << "texLoadDDS(" << filename << ") : not a DDS file";
		return 0;
	}
	
	if ( !( ddsHeader.dwFlags & DDSD_MIPMAPCOUNT ) )
		ddsHeader.dwMipMapCount = 1;
	
	if ( ! ( isPowerOfTwo( ddsHeader.dwWidth ) && isPowerOfTwo( ddsHeader.dwHeight ) ) )
	{
		qWarning() << "texLoadDDS(" << filename << ") : image dimensions must be power of two";
		return 0;
	}

	f.seek(ddsHeader.dwSize + 4);
	
	if ( ddsHeader.ddsPixelFormat.dwFlags & DDPF_FOURCC )
	{
		return texLoadDXT( f, ddsHeader.ddsPixelFormat.dwFourCC, ddsHeader.dwWidth, ddsHeader.dwHeight, ddsHeader.dwMipMapCount );
	}
	else
	{
		return texLoadRaw( f, ddsHeader.dwWidth, ddsHeader.dwHeight,
			ddsHeader.dwMipMapCount, ddsHeader.ddsPixelFormat.dwBPP, ddsHeader.ddsPixelFormat.dwBPP / 8,
			&ddsHeader.ddsPixelFormat.dwRMask );
	}
}

/*
 *  load a tga texture
 */
 
#define TGA_COLOR		2
#define TGA_GREY		3
#define TGA_COLOR_RLE	10
#define TGA_GREY_RLE	11

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
	
	unsigned int depth = hdr[16];
	//unsigned int alphaDepth  = hdr[17] & 15;
	bool flipV = ! ( hdr[17] & 32 );
	bool flipH = hdr[17] & 16;
	unsigned int width = hdr[12] + 256 * hdr[13];
	unsigned int height = hdr[14] + 256 * hdr[15];
	
	if ( ! ( isPowerOfTwo( width ) && isPowerOfTwo( height ) ) )
	{
		qWarning() << "texLoadTGA(" << filename << ") : image dimensions must be power of two";
		return 0;
	}

	// check format and call texLoadRaw
	switch( hdr[2] )
	{
	case TGA_GREY:
	case TGA_GREY_RLE:
		if ( depth == 8 )
		{
			static const quint32 TGA_L_MASK[4] = { 0xff, 0xff, 0xff, 0x00 };
			return texLoadRaw( f, width, height, 1, 8, 1, TGA_L_MASK, flipV, flipH, hdr[2] == TGA_GREY_RLE );
		}
		else if ( depth == 16 )
		{
			static const quint32 TGA_LA_MASK[4] = { 0x00ff, 0x00ff, 0x00ff, 0xff00 };
			return texLoadRaw( f, width, height, 1, 16, 2, TGA_LA_MASK, flipV, flipH, hdr[2] == TGA_GREY_RLE );
		}
		break;
	case TGA_COLOR:
	case TGA_COLOR_RLE:
		if ( depth == 32 )
		{
			static const quint32 TGA_RGBA_MASK[4] = { 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 };
			return texLoadRaw( f, width, height, 1, 32, 4, TGA_RGBA_MASK, flipV, flipH, hdr[2] == TGA_COLOR_RLE );
		}
		else if ( depth == 24 )
		{
			static const quint32 TGA_RGB_MASK[4] = { 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000 };
			return texLoadRaw( f, width, height, 1, 24, 3, TGA_RGB_MASK, flipV, flipH, hdr[2] == TGA_COLOR_RLE );
		}
		break;
	}
	
	qWarning() << "texLoadTGA(" << filename << ") : image sub format not supported" << hdr[2] << depth;
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
		qWarning() << "texLoadBMP(" << filename << ") : could not open file";
		return 0;
	}
	
	// read in bmp header
	quint8 hdr[54];
	qint64 readBytes = f.read((char *)hdr, 54);
	
	if ( readBytes != 54 || strncmp((char*)hdr,"BM", 2) != 0)
	{
		qWarning() << "texLoadBMP(" << filename << ") : not a BMP file";
		return 0;
	}
	
	unsigned int width = get32( &hdr[18] );
	unsigned int height = get32( &hdr[22] );
	unsigned int bpp = get16( &hdr[28] );
	unsigned int compression = get32( &hdr[30] );
	unsigned int offset = get32( &hdr[10] );
	
	f.seek( offset );
	
	if ( ! ( isPowerOfTwo( width ) && isPowerOfTwo( height ) ) )
	{
		qWarning() << "texLoadBMP(" << filename << ") : image dimensions must be power of two";
		return 0;
	}

	switch ( compression )
	{
		case 0:
			if ( bpp == 24 )
			{
				static const quint32 BMP_RGBA_MASK[4] = { 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000 };
				return texLoadRaw( f, width, height, 1, bpp, 3, BMP_RGBA_MASK, true );
			}
			break;
		case FOURCC_DXT5:
		case FOURCC_DXT3:
		case FOURCC_DXT1:
			return texLoadDXT( f, compression, width, height, 1, true );
	}

	qWarning() << "texLoadBMP(" << filename << ") : image sub format not supported";
	/*
	qWarning( "size %i,%i", width, height );
	qWarning( "plns %i", get16( &hdr[26] ) );
	qWarning( "bpp  %i", bpp );
	qWarning( "cmpr %08x", compression );
	qWarning( "ofs  %i", offset );
	*/
	return 0;
}

bool GLTex::exportFile( const QString & name )
{
	if ( ! id )
		return false;
	
	QString filename = name;
	if ( ! filename.toLower().endsWith( ".tga" ) )
		filename.append( ".tga" );

	glBindTexture( GL_TEXTURE_2D, id );
	
	GLint w, h;
	
	glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w );
	glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h );
	
	quint32 s = w * h * 4;
	
	quint8 * pixl = (quint8 *) malloc( s );
	quint8 * data = (quint8 *) malloc( s );

	glPixelStorei( GL_PACK_ALIGNMENT, 1 );
	glPixelStorei( GL_PACK_SWAP_BYTES, GL_FALSE );
	glGetTexImage( GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixl );
	
	static const quint32 TGA_RGBA_MASK_INV[4] = { 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 };
	convertToRGBA( pixl, w, h, 4, TGA_RGBA_MASK_INV, true, false, data );
	
	free( pixl );
	
	QFile f( filename );
	if ( ! f.open( QIODevice::WriteOnly ) )
	{
		qWarning() << "exportTexture(" << filename << ") : could not open file";
		free( data );
		return false;
	}
	
	// write out tga header
	quint8 hdr[18];
	for ( int o = 0; o < 18; o++ ) hdr[o] = 0;

	hdr[02] = TGA_COLOR;
	hdr[12] = w % 256;
	hdr[13] = w / 256;
	hdr[14] = h % 256;
	hdr[15] = h / 256;
	hdr[16] = 32;
	hdr[17] = 8;
	
	qint64 writeBytes = f.write( (char *) hdr, 18 );
	if ( writeBytes != 18 )
	{
		qWarning() << "exportTexture(" << filename << ") : failed to write file";
		free( data );
		return false;
	}
	
	writeBytes = f.write( (char *) data, s );
	if ( writeBytes != s )
	{
		qWarning() << "exportTexture(" << filename << ") : failed to write file";
		free( data );
		return false;
	}
	
	free( data );
	return true;
}

#endif
