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

#include <QtCore/QtCore> // extra include to avoid compile error
#include <QtGui/QtGui>   // dito
#include "GLee.h"
#include "gltexloaders.h"
#include <QtOpenGL>
#include "dds/dds_api.h"
#include "dds/DirectDrawSurface.h" // unused? check if upstream has cleaner or documented API yet
#include "../nifmodel.h"

#include <QDebug>

/*! \file gltexloaders.cpp
 * \brief Texture loading functions.
 *
 * Textures can be loaded from a filename (in any supported format) or raw
 * pixel data. They can also be exported from raw pixel data to TGA or DDS.
 *
 * Supported read formats:
 * - DDS (RAW, DXTn, 8-bit palette)
 * - TGA
 * - BMP
 * - NIF (RGB8, RGBA8, PAL8, DXT1, DXT5)
 *
 * Supported write formats:
 * - TGA (32-bit) from NIF
 * - DDS (RGB, RGBA, DXT1, DXT5) from NIF
 * - NIF from NIF (matching versions only, experimental)
 *
 */

//! Shift amounts for RGBA conversion
static const int rgbashift[4] = { 0, 8, 16, 24 };
//! Mask for TGA greyscale
static const quint32 TGA_L_MASK[4] = { 0xff, 0xff, 0xff, 0x00 };
//! Mask for TGA greyscale with alpha
static const quint32 TGA_LA_MASK[4] = { 0x00ff, 0x00ff, 0x00ff, 0xff00 };
//! Mask for TGA RGBA
static const quint32 TGA_RGBA_MASK[4] = { 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 };
//! Mask for TGA RGB
static const quint32 TGA_RGB_MASK[4] = { 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000 };
//! Mask for BMP RGBA (identical to TGA RGB)
static const quint32 BMP_RGBA_MASK[4] = { 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000 };
//! Inverse mask for RGBA
static const quint32 RGBA_INV_MASK[4] = { 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000 };

//! Unknown 8 bytes for 24-bit NiPixelData
static const quint8 unk8bytes24[] = { 96,8,130,0,0,65,0,0 };
//! Unknown 8 bytes for 32-bit NiPixelData
static const quint8 unk8bytes32[] = { 129,8,130,32,0,65,12,0 };

//! Check whether a number is a power of two.
bool isPowerOfTwo( unsigned int x )
{
	while ( ! ( x == 0 || x & 1 ) )
		x = x >> 1;
	return ( x == 1 );
}

//! Completes mipmap sequence of the current active OpenGL texture.
/*!
 * \param m Number of mipmaps that are already in the texture.
 * \return Total number of mipmaps.
 */
int generateMipMaps( int m )
{
	GLint w = 0, h = 0;
	
	// load the (m-1)'th mipmap as a basis
	glGetTexLevelParameteriv( GL_TEXTURE_2D, m-1, GL_TEXTURE_WIDTH, &w );
	glGetTexLevelParameteriv( GL_TEXTURE_2D, m-1, GL_TEXTURE_HEIGHT, &h );
	
	//qWarning() << m-1 << w << h;
	
	quint8 * data = (quint8 *) malloc( w * h * 4 );
	glGetTexImage( GL_TEXTURE_2D, m-1, GL_RGBA, GL_UNSIGNED_BYTE, data );
	
	// now generate the mipmaps until width is one or height is one.
	while ( w > 1 || h > 1 )
	{
		// the buffer overwrites itself to save memory
		const quint8 * src = data;
		quint8 * dst = data;
		
		quint32 xo = ( w > 1 ? 1*4 : 0 );
		quint32 yo = ( h > 1 ? w*4 : 0 );
		
		w /= 2;
		h /= 2;
		
		if ( w == 0 ) w = 1;
		if ( h == 0 ) h = 1;
		
		//qWarning() << m << w << h;
		
		for ( int y = 0; y < h; y++ )
		{
			for ( int x = 0; x < w; x++ )
			{
				for ( int b = 0; b < 4; b++ )
				{
					*dst++ = ( *(src+xo) + *(src+yo) + *(src+xo+yo) + *src) / 4;
					src++;
				}
				src += xo;
			}
			src += yo;
		}
		
		glTexImage2D( GL_TEXTURE_2D, m++, 4, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );
	}
	
	free( data );
	
	return m;
}

//! Converts RLE-encoded data into pixel data.
/*!
 * TGA in particular uses the PackBits format described at
 * http://en.wikipedia.org/wiki/PackBits and in the TGA spec.
 */
bool uncompressRLE( QIODevice & f, int w, int h, int bytespp, quint8 * pixel )
{
	QByteArray data = f.readAll();
	
	int c = 0; // total pixel count
	int o = 0; // data offset
	
	quint8 rl; // runlength - 1
	while ( c < w * h )
	{
		rl = data[o++];
		if ( rl & 0x80 ) // if RLE packet
		{
			quint8 px[8]; // pixel data in this packet (assume bytespp < 8)
			for ( int b = 0; b < bytespp; b++ )
				px[b] = data[o++];
			rl &= 0x7f; // strip RLE bit
			do
			{
				for ( int b = 0; b < bytespp; b++ )
					*pixel++ = px[b]; // expand pixel data (rl+1) times
			}
			while ( ++c < w*h && rl-- > 0  );
		}
		else
		{
			do
			{
				for ( int b = 0; b < bytespp; b++ )
					*pixel++ = data[o++]; // write (rl+1) raw pixels
			}
			while (  ++c < w*h && rl-- > 0 );
		}
		if ( o >= data.count() ) return false;
	}
	return true;
}

//! Convert pixels to RGBA
/*!
 * \param data Pixels to convert
 * \param w Width of the image
 * \param h Height of the image
 * \param bytespp Number of bytes per pixel
 * \param mask Bitmask for pixel data
 * \param flipV Whether to flip the data vertically
 * \param flipH Whether to flip the data horizontally
 * \param pixl Pixels to output
 */
void convertToRGBA( const quint8 * data, int w, int h, int bytespp, const quint32 mask[], bool flipV, bool flipH, quint8 * pixl )
{
	memset( pixl, 0, w * h * 4 );

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

//! Load raw pixel data
int texLoadRaw( QIODevice & f, int width, int height, int num_mipmaps, int bpp, int bytespp, const quint32 mask[], bool flipV = false, bool flipH = false, bool rle = false )
{
	if ( bytespp * 8 != bpp || bpp > 32 || bpp < 8 )
		throw QString( "unsupported image depth %1 / %2" ).arg( bpp ).arg( bytespp );
	
	glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
	glPixelStorei( GL_UNPACK_SWAP_BYTES, GL_FALSE );
	
	quint8 * data1 = (quint8 *) malloc( width * height * 4 );
	quint8 * data2 = (quint8 *) malloc( width * height * 4 );
	
	int w = width;
	int h = height;
	int m = 0;
	
	while ( m < num_mipmaps )
	{
		w = width >> m;
		h = height >> m;
		
		if ( w == 0 ) w = 1;
		if ( h == 0 ) h = 1;
		
		if ( rle )
		{
			if ( ! uncompressRLE( f, w, h, bytespp, data1 ) )
			{
				free( data2 );
				free( data1 );
				throw QString( "unexpected EOF" );
			}
		}
		else if ( f.read( (char *) data1, w * h * bytespp ) != w * h * bytespp )
		{
			
			free( data2 );
			free( data1 );
			throw QString( "unexpected EOF" );
		}
		
		convertToRGBA( data1, w, h, bytespp, mask, flipV, flipH, data2 );
		
		glTexImage2D( GL_TEXTURE_2D, m++, 4, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data2 );
		
		if ( w == 1 && h == 1 )
			break;
	}
	
	free( data2 );
	free( data1 );
	
	if ( w > 1 || h > 1 )
		m = generateMipMaps( m );
	
	return m;
}

//! Load a palettised texture
int texLoadPal( QIODevice & f, int width, int height, int num_mipmaps, int bpp, int bytespp, const quint32 colormap[], bool flipV, bool flipH, bool rle )
{
	if ( bpp != 8 || bytespp != 1 )
		throw QString( "unsupported image depth %1 / %2" ).arg( bpp ).arg( bytespp );
	
	glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
	glPixelStorei( GL_UNPACK_SWAP_BYTES, GL_FALSE );
	
	quint8 * data = (quint8 *) malloc( width * height * 1 );
	quint8 * pixl = (quint8 *) malloc( width * height * 4 );
	
	int w = width;
	int h = height;
	int m = 0;
	
	while ( m < num_mipmaps )
	{
		w = width >> m;
		h = height >> m;
		
		if ( w == 0 ) w = 1;
		if ( h == 0 ) h = 1;
		
		if ( rle )
		{
			if ( ! uncompressRLE( f, w, h, bytespp, data ) )
			{
				free( pixl );
				free( data );
				throw QString( "unexpected EOF" );
			}
		}
		else if ( f.read( (char *) data, w * h * bytespp ) != w * h * bytespp )
		{
			free( pixl );
			free( data );
			throw QString( "unexpected EOF" );
		}
		
		quint8 * src = data;
		for ( int y = 0; y < h; y++ )
		{
			quint32 * dst = (quint32 *) ( pixl + 4 * ( w * ( flipV ? h - y - 1 : y ) + ( flipH ? w - 1 : 0 ) ) );
			for ( int x = 0; x < w; x++ )
			{
				*dst++ = colormap[*src++];
			}
		}
		
		glTexImage2D( GL_TEXTURE_2D, m++, 4, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixl );
		
		if ( w == 1 && h == 1 )
			break;
	}
	
	free( pixl );
	free( data );
	
	if ( w > 1 || h > 1 )
		m = generateMipMaps( m );
	
	return m;
}

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

//! Flip DXT blocks vertically (not used in software decompression)
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

//! Load a DXT compressed DDS texture from file
/*!
 * \param f File to load from
 * \param null Format
 * \param null Block size
 * \param null Width
 * \param null Height
 * \param mipmaps The number of mipmaps to read
 * \param null Flip
 * \return The total number of mipmaps
 */
GLuint texLoadDXT( QIODevice & f, GLenum /*glFormat*/, int /*blockSize*/, quint32 /*width*/, quint32 /*height*/, quint32 mipmaps, bool /*flipV*/ = false )
{
/*
#ifdef WIN32
	if ( !_glCompressedTexImage2D )
	{
#endif
*/
	// load the pixels
	f.seek(0);
	QByteArray bytes = f.readAll();
	GLuint m = 0;
	while ( m < mipmaps )
	{
		// load face 0, mipmap m
		Image * img = load_dds((unsigned char*)bytes.data(), bytes.size(), 0, m);
		if (!img)
			return(0);
		// convert texture to OpenGL RGBA format
		unsigned int w = img->width();
		unsigned int h = img->height();
		GLubyte * pixels = new GLubyte[w * h * 4];
		Color32 * src = img->pixels();
		GLubyte * dst = pixels;
		//qWarning() << "flipV = " << flipV;
		for ( quint32 y = 0; y < h; y++ )
		{
			for ( quint32 x = 0; x < w; x++ )
			{
				*dst++ = src->r;
				*dst++ = src->g;
				*dst++ = src->b;
				*dst++ = src->a;
				src++;
			}
		}
		delete img;
		// load the texture into OpenGL
		glTexImage2D( GL_TEXTURE_2D, m++, 4, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels );
		delete [] pixels;
	}
	m = generateMipMaps( m );
	return m;
/*
#ifdef WIN32
	}
	GLubyte * pixels = (GLubyte *) malloc( ( ( width + 3 ) / 4 ) * ( ( height + 3 ) / 4 ) * blockSize );
	unsigned int w = width, h = height, s;
	unsigned int m = 0;
	
	while ( m < mipmaps )
	{
		w = width >> m;
		h = height >> m;
		
		if ( w == 0 ) w = 1;
		if ( h == 0 ) h = 1;
		
		s = ((w+3)/4) * ((h+3)/4) * blockSize;
		
		if ( f.read( (char *) pixels, s ) != s )
		{
			free( pixels );
			throw QString ( "unexpected EOF" );
		}
		
		if ( flipV )
			flipDXT( glFormat, w, h, pixels );
		
		_glCompressedTexImage2D( GL_TEXTURE_2D, m++, glFormat, w, h, 0, s, pixels );
		
		if ( w == 1 && h == 1 )
			break;
	}
	
	if ( w > 1 || h > 1 )
		return 1;
	else
		return m;
#endif
*/
}

//! Load a (possibly compressed) dds texture.
GLuint texLoadDDS( QIODevice & f, QString & texformat )
{
	char tag[4];
	f.read(&tag[0], 4);
	DDSFormat ddsHeader;

	if ( strncmp( tag,"DDS ", 4 ) != 0 || f.read((char *) &ddsHeader, sizeof(DDSFormat)) != sizeof( DDSFormat ) )
		throw QString( "not a DDS file" );
	
	texformat = "DDS";
	
	if ( !( ddsHeader.dwFlags & DDSD_MIPMAPCOUNT ) )
		ddsHeader.dwMipMapCount = 1;
	
	if ( ! ( isPowerOfTwo( ddsHeader.dwWidth ) && isPowerOfTwo( ddsHeader.dwHeight ) ) )
		throw QString( "image dimensions must be power of two" );

	f.seek(ddsHeader.dwSize + 4);
	
	if ( ddsHeader.ddsPixelFormat.dwFlags & DDPF_FOURCC )
	{
		int blockSize = 8;
		GLenum glFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
		
		switch( ddsHeader.ddsPixelFormat.dwFourCC )
		{
			case FOURCC_DXT1:
				glFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
				blockSize = 8;
				texformat += " (DXT1)";
				break;
			case FOURCC_DXT3:
				glFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
				blockSize = 16;
				texformat += " (DXT3)";
				break;
			case FOURCC_DXT5:
				glFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
				blockSize = 16;
				texformat += " (DXT5)";
				break;
			default:
				throw QString( "unknown texture compression" );
		}
		
		return texLoadDXT( f, glFormat, blockSize, ddsHeader.dwWidth, ddsHeader.dwHeight, ddsHeader.dwMipMapCount );
	}
	else if ( ddsHeader.ddsPixelFormat.dwFlags & 0x20 ) // DDPF_PALETTEINDEXED8
	{
		texformat += " (PAL)";
		
		// Palette format: 256 RGBA quads (1024 bytes), starting after header
		
		quint32 colormap[256];
		if ( f.read( (char *)colormap, 4*256 ) != 4*256 )
			throw QString( "unexpected EOF" );

		return texLoadPal( f, ddsHeader.dwWidth, ddsHeader.dwHeight, ddsHeader.dwMipMapCount,
				ddsHeader.ddsPixelFormat.dwBPP, ddsHeader.ddsPixelFormat.dwBPP / 8,
				(const quint32 *) colormap, false, false, false );
	}
	else
	{
		texformat += " (RAW)";
		
		if ( ddsHeader.ddsPixelFormat.dwRMask != 0 && ddsHeader.ddsPixelFormat.dwGMask == 0 && ddsHeader.ddsPixelFormat.dwBMask == 0 )
		{	// fixup greyscale
			ddsHeader.ddsPixelFormat.dwGMask = ddsHeader.ddsPixelFormat.dwRMask;
			ddsHeader.ddsPixelFormat.dwBMask = ddsHeader.ddsPixelFormat.dwRMask;
		}
		
		return texLoadRaw( f, ddsHeader.dwWidth, ddsHeader.dwHeight,
			ddsHeader.dwMipMapCount, ddsHeader.ddsPixelFormat.dwBPP, ddsHeader.ddsPixelFormat.dwBPP / 8,
			&ddsHeader.ddsPixelFormat.dwRMask );
	}
}

//! Load a DXT compressed texture
/*!
 * \param hdr Description of the texture
 * \param pixels The pixel data
 * \param size The size of the texture
 * \return The total number of mipmaps
 */
GLuint texLoadDXT( DDSFormat &hdr, const quint8 *pixels, uint size )
{
	int m = 0;
	while ( m < (int)hdr.dwMipMapCount )
	{
		// load face 0, mipmap m
		Image * img = load_dds(pixels, (int)size, 0, m, &hdr);
		if (!img)
			return(0);
		// convert texture to OpenGL RGBA format
		unsigned int w = img->width();
		unsigned int h = img->height();
		GLubyte * pixels = new GLubyte[w * h * 4];
		Color32 * src = img->pixels();
		GLubyte * dst = pixels;
		//qWarning() << "flipV = " << flipV;
		for ( quint32 y = 0; y < h; y++ )
		{
			for ( quint32 x = 0; x < w; x++ )
			{
				*dst++ = src->r;
				*dst++ = src->g;
				*dst++ = src->b;
				*dst++ = src->a;
				src++;
			}
		}
		delete img;
		// load the texture into OpenGL
		glTexImage2D( GL_TEXTURE_2D, m++, 4, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels );
		delete [] pixels;
	}
	m = generateMipMaps( m );
	return m;
}



// TGA constants
// Note that TGA_X_RLE = TGA_X + 8
// i.e. RLE = hdr[2] & 0x8

#define TGA_COLORMAP     1
#define TGA_COLOR        2
#define TGA_GREY         3
#define TGA_COLORMAP_RLE 9
#define TGA_COLOR_RLE    10
#define TGA_GREY_RLE     11

//! Load a TGA texture.
GLuint texLoadTGA( QIODevice & f, QString & texformat )
{
	// see http://en.wikipedia.org/wiki/Truevision_TGA for a lot of this
	texformat = "TGA";
	
	// read in tga header
	quint8 hdr[18];
	qint64 readBytes = f.read((char *)hdr, 18);
	if ( readBytes != 18 )
		throw QString( "unexpected EOF" );
	
	// ID tag, if present
	if ( hdr[0] ) f.read( hdr[0] );
	
	quint8 depth = hdr[16];
	//quint8 alphaDepth  = hdr[17] & 15;
	bool flipV = ! ( hdr[17] & 32 );
	bool flipH = hdr[17] & 16;
	quint16 width = hdr[12] + 256 * hdr[13];
	quint16 height = hdr[14] + 256 * hdr[15];
	
	if ( ! ( isPowerOfTwo( width ) && isPowerOfTwo( height ) ) )
		throw QString( "image dimensions must be power of two" );
	
	quint32 colormap[256];
	
	if ( hdr[1] )
	{	// color map present
		quint16 offset = hdr[3] + 256 * hdr[4];
		quint16 length = hdr[5] + 256 * hdr[6];
		quint8 bits = hdr[7];
		quint8 bytes = bits / 8;
		
		//qWarning() << "COLORMAP" << "offset" << offset << "length" << length << "bits" << bits << "depth" << depth;
		
		if ( bits != 32 && bits != 24 )
			throw QString( "image sub format not supported" );
		
		quint32 cnt = offset;
		quint32 col;
		while ( length-- )
		{
			col = 0;
			if ( f.read( (char *) &col, bytes ) != bytes )
				throw QString( "unexpected EOF" );
			
			if ( cnt < 256 )
			{
				switch ( bits )
				{
					case 24:
						colormap[cnt] = ( ( col & 0x00ff0000 ) >> 16 ) | ( col & 0x0000ff00 ) | ( ( col & 0xff ) << 16 ) | 0xff000000;
						break;
					case 32:
						colormap[cnt] = ( ( col & 0x00ff0000 ) >> 16 ) | ( col & 0xff00ff00 ) | ( ( col & 0xff ) << 16 );
						break;
				}
			}
			cnt++;
		}
	}

	// check format and call texLoadPal / texLoadRaw
	switch( hdr[2] )
	{
	case TGA_COLORMAP:
	case TGA_COLORMAP_RLE:
		if ( depth == 8 && hdr[1] )
		{
			texformat += " (palettized)";
			if ( hdr[2] == TGA_COLORMAP_RLE )
				texformat += " (RLE)";
			
			return texLoadPal( f, width, height, 1, depth, depth/8, colormap, flipV, flipH, hdr[2] == TGA_COLORMAP_RLE );
		}
		break;
	case TGA_GREY:
	case TGA_GREY_RLE:
		if ( depth == 8 )
		{
			texformat += " (greyscale)";
			if ( hdr[2] == TGA_GREY_RLE )
				texformat += " (RLE)";
			
			return texLoadRaw( f, width, height, 1, 8, 1, TGA_L_MASK, flipV, flipH, hdr[2] == TGA_GREY_RLE );
		}
		else if ( depth == 16 )
		{
			texformat += " (greyscale) (alpha)";
			if ( hdr[2] == TGA_GREY_RLE )
				texformat += " (RLE)";
			
			return texLoadRaw( f, width, height, 1, 16, 2, TGA_LA_MASK, flipV, flipH, hdr[2] == TGA_GREY_RLE );
		}
		break;
	case TGA_COLOR:
	case TGA_COLOR_RLE:
		if ( depth == 32 )
		{
			texformat += " (truecolor) (alpha)";
			if ( hdr[2] == TGA_GREY_RLE )
				texformat += " (RLE)";
			
			return texLoadRaw( f, width, height, 1, 32, 4, TGA_RGBA_MASK, flipV, flipH, hdr[2] == TGA_COLOR_RLE );
		}
		else if ( depth == 24 )
		{
			texformat += " (truecolor)";
			if ( hdr[2] == TGA_COLOR_RLE )
				texformat += " (RLE)";
			
			return texLoadRaw( f, width, height, 1, 24, 3, TGA_RGB_MASK, flipV, flipH, hdr[2] == TGA_COLOR_RLE );
		}
		break;
	}
	throw QString( "image sub format not supported" );
	return 0;
}

//! Return value as a 32-bit value; possibly replace with <a href="http://doc.trolltech.com/latest/qtendian.html">QtEndian</a> functions?
quint32 get32( quint8 * x )
{
	return *( (quint32 *) x );
}

//! Return value as a 16-bit value; possibly replace with <a href="http://doc.trolltech.com/latest/qtendian.html">QtEndian</a> functions?
quint16 get16( quint8 * x )
{
	return *( (quint16 *) x );
}

//! Load a BMP texture.
GLuint texLoadBMP( QIODevice & f, QString & texformat )
{
	// read in bmp header
	quint8 hdr[54];
	qint64 readBytes = f.read((char *)hdr, 54);
	
	if ( readBytes != 54 || strncmp((char*)hdr,"BM", 2) != 0)
		throw QString( "not a BMP file" );
	
	texformat = "BMP";
	
	unsigned int width = get32( &hdr[18] );
	unsigned int height = get32( &hdr[22] );
	unsigned int bpp = get16( &hdr[28] );
	unsigned int compression = get32( &hdr[30] );
	unsigned int offset = get32( &hdr[10] );
	
	f.seek( offset );
	
	if ( ! ( isPowerOfTwo( width ) && isPowerOfTwo( height ) ) )
		throw QString( "image dimensions must be power of two" );

	switch ( compression )
	{
		case 0:
			if ( bpp == 24 )
			{
				return texLoadRaw( f, width, height, 1, bpp, 3, BMP_RGBA_MASK, true );
			}
			break;
		// Since when can a BMP contain DXT compressed textures?
		case FOURCC_DXT5:
			texformat += " (DXT5)";
			return texLoadDXT( f, compression, width, height, 1, true );
		case FOURCC_DXT3:
			texformat += " (DXT3)";
			return texLoadDXT( f, compression, width, height, 1, true );
		case FOURCC_DXT1:
			texformat += " (DXT1)";
			return texLoadDXT( f, compression, width, height, 1, true );
	}

	throw QString( "unknown image sub format" );
	/*
	qWarning( "size %i,%i", width, height );
	qWarning( "plns %i", get16( &hdr[26] ) );
	qWarning( "bpp  %i", bpp );
	qWarning( "cmpr %08x", compression );
	qWarning( "ofs  %i", offset );
	*/
	return 0;
}

// (public function, documented in gltexloaders.h)
bool texLoad( const QModelIndex & iData, QString & texformat, GLuint & width, GLuint & height, GLuint & mipmaps )
{
	bool ok = false;
	const NifModel * nif = qobject_cast<const NifModel *>( iData.model() );
	if ( nif && iData.isValid() ) {
		mipmaps = nif->get<uint>(iData, "Num Mipmaps");
		QModelIndex iMipmaps = nif->getIndex( iData, "Mipmaps" );
		if ( mipmaps > 0 && iMipmaps.isValid() )
		{
			QModelIndex iMipmap = iMipmaps.child(0,0);
			width = nif->get<uint>(iMipmap, "Width");
			height = nif->get<uint>(iMipmap, "Height");
		}

		int bpp = nif->get<uint>(iData, "Bits Per Pixel");
		int bytespp = nif->get<uint>(iData, "Bytes Per Pixel");
		uint format = nif->get<uint>(iData, "Pixel Format");
		bool flipV = false;
		bool flipH = false;
		bool rle = false;

		QBuffer buf;
		
		QModelIndex iPixelData = nif->getIndex( iData, "Pixel Data" );
		if ( iPixelData.isValid() ) {
			QModelIndex iFaceData = iPixelData.child(0,0);
			if ( iFaceData.isValid() ) {
				if ( QByteArray* pdata = nif->get<QByteArray*>( iFaceData.child(0,0) ) ) {
					buf.setData(*pdata);
					buf.open(QIODevice::ReadOnly);
					buf.seek(0);
				}
			}
		}
		if (buf.size() == 0)
			return false;

		quint32 mask[4] = { 0x00000000, 0x00000000, 0x00000000, 0x00000000 };
		
		if ( nif->getVersionNumber() < 0x14000004 ) {
			mask[0] = nif->get<uint>(iData, "Red Mask");
			mask[1] = nif->get<uint>(iData, "Green Mask");
			mask[2] = nif->get<uint>(iData, "Blue Mask");
			mask[3] = nif->get<uint>(iData, "Alpha Mask");
		} else {
			QModelIndex iChannels = nif->getIndex( iData, "Channels" );
			if ( iChannels.isValid() )
			{
				for ( int i = 0; i < 4; i++ ) {
					QModelIndex iChannel = iChannels.child( i, 0 );
					uint type = nif->get<uint>(iChannel, "Type");
					uint bpc = nif->get<uint>(iChannel, "Bits Per Channel");
					int m = (1 << bpc) - 1;
					switch (type)
					{
					case 0: mask[i] = m << (bpc * 0); break; //Green
					case 1: mask[i] = m << (bpc * 1); break; //Blue
					case 2: mask[i] = m << (bpc * 2); break; //Red
					case 3: mask[i] = m << (bpc * 3); break; //Red
					}
				}
			}
		}
		DDSFormat hdr;
		hdr.dwSize = 0;
		hdr.dwFlags = DDPF_FOURCC;
		hdr.dwHeight = height;
		hdr.dwWidth = width;
		hdr.dwLinearSize = 0;
		hdr.dwMipMapCount = mipmaps;
		hdr.ddsPixelFormat.dwSize = 0;
		hdr.ddsPixelFormat.dwFlags = DDPF_FOURCC;
		hdr.ddsPixelFormat.dwFourCC = FOURCC_DXT1;
		hdr.ddsPixelFormat.dwBPP = bpp;
		hdr.ddsPixelFormat.dwRMask = mask[0];
		hdr.ddsPixelFormat.dwGMask = mask[1];
		hdr.ddsPixelFormat.dwBMask = mask[2];
		hdr.ddsPixelFormat.dwAMask = mask[3];

		texformat = "NIF";
		switch (format)
		{
		case 0: // PX_FMT_RGB8
			texformat += " (RGB8)";
			ok = (0 != texLoadRaw(buf, width, height, mipmaps, bpp, bytespp, mask, flipV, flipH, rle));
			break;
		case 1: // PX_FMT_RGBA8
			texformat += " (RGBA8)";
			ok = (0 != texLoadRaw(buf, width, height, mipmaps, bpp, bytespp, mask, flipV, flipH, rle));
			break;
		case 2: // PX_FMT_PAL8
			{
				texformat += " (PAL8)";
				// Read the NiPalette entries; change this if we change NiPalette in nif.xml
				QModelIndex iPalette = nif->getBlock( nif->getLink( iData, "Palette" ) );
				if (iPalette.isValid()) {
					QVector<quint32> map;
					uint nmap = nif->get<uint>(iPalette, "Num Entries");
					map.resize(nmap);
					QModelIndex iPaletteArray = nif->getIndex( iPalette, "Palette" );
					if ( nmap > 0 && iPaletteArray.isValid() ) {
						for (uint i=0; i<nmap; ++i) {
							QModelIndex iRGBElem = iPaletteArray.child(i,0);
							quint8 r = nif->get<quint8>(iRGBElem, "r");
							quint8 g = nif->get<quint8>(iRGBElem, "g");
							quint8 b = nif->get<quint8>(iRGBElem, "b");
							quint8 a = nif->get<quint8>(iRGBElem, "a");
							map[i] = ((quint32)((r|((quint16)g<<8))|(((quint32)b)<<16)|(((quint32)a)<<24)));
						}
					}
					ok = (0 != texLoadPal( buf, width, height, mipmaps, bpp, bytespp, map.data(), flipV, flipH, rle ));
				}
			}
			break;
		case 4: //PX_FMT_DXT1
			texformat += " (DXT1)";
			hdr.ddsPixelFormat.dwFourCC = FOURCC_DXT1;
			ok = (0 != texLoadDXT(hdr, (const unsigned char *)buf.data().data(), buf.size()));
			break;
		case 5: //PX_FMT_DXT5
			texformat += " (DXT5)";
			hdr.ddsPixelFormat.dwFourCC = FOURCC_DXT5;
			ok = (0 != texLoadDXT(hdr, (const unsigned char *)buf.data().data(), buf.size()));
			break;
		case 6: //PX_FMT_DXT5_ALT
			texformat += " (DXT5ALT)";
			hdr.ddsPixelFormat.dwFourCC = FOURCC_DXT5;
			ok = (0 != texLoadDXT(hdr, (const unsigned char *)buf.data().data(), buf.size()));
			break;
		}
		if (ok) {
			glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, (GLint *) & width );
			glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, (GLint *) & height );
		}
	}
	return ok;
}

//! Load NiPixelData or NiPersistentSrcTextureRendererData from a NifModel
GLuint texLoadNIF( QIODevice & f, QString & texformat ) {
	GLuint mipmaps = 0;

	NifModel pix;

	if ( ! pix.load( f ) )
		throw QString( "failed to load pixel data from file" );

	QPersistentModelIndex iRoot;

	foreach ( qint32 l, pix.getRootLinks() )
	{
		QModelIndex iData = pix.getBlock( l, "ATextureRenderData" );
		if ( ! iData.isValid() || iData == QModelIndex() )
			throw QString( "this is not a normal .nif file; there should be only pixel data as root blocks" );

		GLuint width, height;
		texLoad(iData, texformat, width, height, mipmaps);
	}

	return mipmaps;
}


// (public function, documented in gltexloaders.h)
bool texLoad( const QString & filepath, QString & format, GLuint & width, GLuint & height, GLuint & mipmaps )
{
	width = height = mipmaps = 0;
	
	QFile f( filepath );
	if ( ! f.open( QIODevice::ReadOnly ) )
		throw QString( "could not open file" );
	
	if ( filepath.endsWith( ".dds", Qt::CaseInsensitive ) )
		mipmaps = texLoadDDS( f, format );
	else if ( filepath.endsWith( ".tga", Qt::CaseInsensitive ) )
		mipmaps = texLoadTGA( f, format );
	else if ( filepath.endsWith( ".bmp", Qt::CaseInsensitive ) )
		mipmaps = texLoadBMP( f, format );
	else if ( filepath.endsWith( ".nif", Qt::CaseInsensitive ) || filepath.endsWith( ".texcache", Qt::CaseInsensitive ))
		mipmaps = texLoadNIF( f, format );
	else
		throw QString( "unknown texture format" );
	
	glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, (GLint *) & width );
	glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, (GLint *) & height );
	
	return mipmaps > 0;
}
	
// (public function, documented in gltexloaders.h)
bool texCanLoad( const QString & filepath )
{
	QFileInfo i( filepath );
	return i.exists() && i.isReadable() && 
		(  filepath.endsWith( ".dds", Qt::CaseInsensitive )
		|| filepath.endsWith( ".tga", Qt::CaseInsensitive )
		|| filepath.endsWith( ".bmp", Qt::CaseInsensitive )
		|| filepath.endsWith( ".nif", Qt::CaseInsensitive )
		|| filepath.endsWith( ".texcache", Qt::CaseInsensitive )
		);
}

// (public function, documented in gltexloaders.h)
bool texSaveDDS( const QModelIndex & index, const QString & filepath, GLuint & width, GLuint & height, GLuint & mipmaps )
{
	const NifModel * nif = qobject_cast<const NifModel *>( index.model() );
	quint32 format = nif->get<quint32>( index, "Pixel Format" );
	// can't dump palettised textures yet
	if ( format == 2 )
	{
		qWarning() << "Texture format not supported";
		return false;
	}

	// copy directly from mipmaps into texture
	
	QBuffer buf;
	
	QModelIndex iPixelData = nif->getIndex( index, "Pixel Data" );
	if ( iPixelData.isValid() ) {
		QModelIndex iFaceData = iPixelData.child(0,0);
		if ( iFaceData.isValid() ) {
			if ( QByteArray* pdata = nif->get<QByteArray*>( iFaceData.child(0,0) ) ) {
				buf.setData(*pdata);
				buf.open(QIODevice::ReadOnly);
				buf.seek(0);
			}
		}
	}
	
	if (buf.size() == 0)
		return false;
	
	QString filename = filepath;
	if( ! filename.toLower().endsWith( ".dds" ) )
		filename.append( ".dds" );
	
	QFile f( filename );
	if ( ! f.open( QIODevice::WriteOnly ) )
	{
		qWarning() << "texSaveDDS(" << filename << ") : could not open file";
		return false;
	}
	
	qint64 writeBytes = f.write( (char *) "DDS ", 4 );
	if ( writeBytes != 4 )
	{
		qWarning() << "texSaveDDS(" << filename << ") : could not open file";
		return false;
	}
	
	quint8 header[124]; // could probably use a bytearray or something here
	for ( int o = 0; o < 124; o++ ) header[o] = 0;
	int pos = 0; // offset
	
	// size of header
	header[pos] = 124;
	pos+=4;
	
	bool compressed = ( (format == 4) || (format == 5) || (format == 6) );
	
	// header flags
	header[pos++] = ( 1 << 0 ) // caps = 1
		| ( 1 << 1 ) // height = 1
		| ( 1 << 2 ) // width = 1
		| ( (compressed ? 0 : 1) << 3 ); // pitch, and 4 bits reserved
	
	header[pos++] = ( 1 << 4 ); // 4 bits reserved, pixelformat = 1, 3 bits reserved
	
	//quint32 mipmaps = nif->get<quint32>( index, "Num Mipmaps" );
	
	bool hasMipMaps = ( mipmaps > 1 );
	
	header[pos++] = ( (hasMipMaps ? 1 : 0) << 1 ) // 1 bit reserved, mipmapcount
			| ( (compressed ? 1 : 0) << 3 ); // 1 bit reserved, linearsize, 3 bits reserved, depth = 0
	
	pos++;
	
	// height
	qToLittleEndian( height, header+pos );
	pos+=4;
	
	// width
	qToLittleEndian( width, header+pos );
	pos+=4;

	// linear size
	// Uncompressed textures: bytes per scan line;
	// DXT1: 16-pixel blocks converted to 64 bits = 8 bytes, ie. width * height / 16 * 64 / 8
	// DXT5: don't know
	quint32 linearsize;
	switch ( format ) {
		case 0:
			linearsize = width * 3;
			break;
		case 1:
			linearsize = width * 4;
			break;
		case 4:
			linearsize = width * height / 2;
			break;
		case 5:
		case 6:
			linearsize = width * height;
			break;
		default: // how did we get here?
			linearsize = 0;
			break;
	}
	qToLittleEndian( linearsize, header+pos );
	pos+=4;
	
	// depth
	pos+=4;

	// mipmapcount
	qToLittleEndian( mipmaps, header+pos );
	pos+=4;

	// reserved[11]
	pos+=44;

	// pixel format size
	header[pos] = 32;
	pos+=4;
	
	// pixel format flags
	bool alphapixels = (format == 1 ); //|| format == 5 || format == 6);
	header[pos] = ( (alphapixels ? 1 : 0) << 0 ) // alpha pixels
		| ( (alphapixels ? 1 : 0) << 1 ) // alpha
		| ( (compressed ? 1 : 0) << 2 ) // fourcc
		| ( (compressed ? 0 : 1) << 6 ); // rgb
	pos+=4;

	// pixel format fourcc
	quint32 fourcc;
	switch ( format )
	{
		case 0:
		case 1:
			fourcc = 0;
			break;
		case 4:
			fourcc = FOURCC_DXT1;
			break;
		case 5:
		case 6:
			fourcc = FOURCC_DXT5;
			break;
		default: // again, how did we get here?
			fourcc = 0;
			break;
	}
	qToLittleEndian( fourcc, header+pos );
	pos+=4;
	
	// bitcount
	// NIF might be wrong, so hardcode based on format
	// quint32 bitcount = nif->get<quint8>( index, "Bits Per Pixel" );
	quint32 bitcount;
	switch ( format )
	{
		case 0:
		case 4:
			bitcount = 24;
			break;
		case 1:
		case 5:
		case 6:
			bitcount = 32;
			break;
		default: // what?
			bitcount = 32;
			break;
	}
	qToLittleEndian( bitcount, header+pos );
	pos+=4;
	
	// masks
	quint32 mask[4] = { 0x000000ff, 0x0000ff00, 0x00ff0000, 0x00000000 };

	if ( nif->getVersionNumber() < 0x14000004 ) {
		mask[0] = nif->get<quint32>( index, "Red Mask" );
		mask[1] = nif->get<quint32>( index, "Green Mask" );
		mask[2] = nif->get<quint32>( index, "Blue Mask" );
		mask[3] = nif->get<quint32>( index, "Alpha Mask" );
	} else {
		QModelIndex iChannels = nif->getIndex( index, "Channels" );
		if ( iChannels.isValid() )
		{
			for ( int i = 0; i < 4; i++ ) {
				QModelIndex iChannel = iChannels.child( i, 0 );
				uint type = nif->get<uint>( iChannel, "Type" );
				uint bpc = nif->get<uint>( iChannel, "Bits Per Channel" );
				int m = (1 << bpc) - 1;
				switch (type)
				{
					case 0: mask[i] = m << (bpc * 0); break; //Green
					case 1: mask[i] = m << (bpc * 1); break; //Blue
					case 2: mask[i] = m << (bpc * 2); break; //Red
					case 3: mask[i] = m << (bpc * 3); break; //Red
				}
			}
		}
	}

	/*
	if ( alphapixels )
	{
		mask[3] = 0xff000000;
	}*/

	// red mask
	qToLittleEndian( mask[0], header+pos );
	pos+=4;
	// green mask
	qToLittleEndian( mask[1], header+pos );
	pos+=4;
	// blue mask
	qToLittleEndian( mask[2], header+pos );
	pos+=4;
	// alpha mask
	qToLittleEndian( mask[3], header+pos );
	pos+=4;
	
	// caps1
	header[pos++] = ( (hasMipMaps ? 1 : 0) << 3 ); // complex
	header[pos++] = ( 1 << 4 ); // texture
	header[pos++] = ( (hasMipMaps ? 1 : 0) << 6 ); // mipmaps
	
	// write header
	writeBytes = f.write( (char *) header, 124 );
	if ( writeBytes != 124 )
	{
		qWarning() << "texSaveDDS(" << filename << ") : could not open file";
		return false;
	}
	
	// write pixel data
	writeBytes = f.write( buf.data(), buf.size() );
	if ( writeBytes != buf.size() )
	{
		qWarning() << "texSaveDDS(" << filename << ") : could not open file";
		return false;
	}
	
	return true;
}

// (public function, documented in gltexloaders.h)
bool texSaveTGA( const QModelIndex & index, const QString & filepath, GLuint & width, GLuint & height )
{
	//const NifModel * nif = qobject_cast<const NifModel *>( index.model() );
	QString filename = filepath;
	if ( ! filename.toLower().endsWith( ".tga" ) )
		filename.append( ".tga" );

	//quint32 bytespp = nif->get<quint32>( index, "Bytes Per Pixel" );
	//quint32 bpp = nif->get<quint8>( index, "Bits Per Pixel" );

	quint32 s = width * height * 4; //bytespp;
	
	quint8 * pixl = (quint8 *) malloc( s );
	quint8 * data = (quint8 *) malloc( s );

	glPixelStorei( GL_PACK_ALIGNMENT, 1 );
	glPixelStorei( GL_PACK_SWAP_BYTES, GL_FALSE );
	
	// This is very dodgy and is mainly to avoid having to run through convertToRGBA
	// It also won't work with DXT compressed or palletised textures
	/*
	if( nif->get<quint32>( index, "Pixel Format" ) == 0 ) {
		glGetTexImage( GL_TEXTURE_2D, 0, GL_BGR, GL_UNSIGNED_BYTE, data );
	}
	else if( nif->get<quint32>( index, "Pixel Format") == 1 ) {
		glGetTexImage( GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_BYTE, data );
	}*/

	glGetTexImage( GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixl );
	
	//quint32 mask[4] = { 0x00000000, 0x00000000, 0x00000000, 0x00000000 };
	/*
	if ( nif->getVersionNumber() < 0x14000004 ) {
		mask[0] = nif->get<uint>(index, "Blue Mask");
		mask[1] = nif->get<uint>(index, "Green Mask");
		mask[2] = nif->get<uint>(index, "Red Mask");
		mask[3] = nif->get<uint>(index, "Alpha Mask");
	}
	*/

	convertToRGBA( pixl, width, height, 4, TGA_RGBA_MASK, true, false, data );
	
	free( pixl );
	
	QFile f( filename );
	if ( ! f.open( QIODevice::WriteOnly ) )
	{
		qWarning() << "texSaveTGA(" << filename << ") : could not open file";
		free( data );
		return false;
	}
	
	// write out tga header
	quint8 hdr[18];
	for ( int o = 0; o < 18; o++ ) hdr[o] = 0;

	hdr[02] = TGA_COLOR;
	hdr[12] = width % 256;
	hdr[13] = width / 256;
	hdr[14] = height % 256;
	hdr[15] = height / 256;
	hdr[16] = 32; //bpp;
	hdr[17] = 8; //(bytespp == 4 ? 8 : 0)+32;
	
	quint8 footer[26];
	
	// extension area
	for ( int o = 0; o < 4; o++ ) footer[o] = 0;
	// developer area
	for ( int o = 4; o < 8; o++ ) footer[o] = 0;

	// signature
	const char * signature = "TRUEVISION-XFILE.";
	memcpy( footer+8, signature, 17 );
	footer[25] = 0;
	
	qint64 writeBytes = f.write( (char *) hdr, 18 );
	if ( writeBytes != 18 )
	{
		qWarning() << "texSaveTGA(" << filename << ") : failed to write file";
		free( data );
		return false;
	}
	
	writeBytes = f.write( (char *) data, s );
	if ( writeBytes != s )
	{
		qWarning() << "texSaveTGA(" << filename << ") : failed to write file";
		free( data );
		return false;
	}

	writeBytes = f.write( (char *) footer, 26 );
	if ( writeBytes != 26 )
	{
		qWarning() << "texSaveTGA(" << filename << ") : failed to write file";
		free( data );
		return false;
	}
	
	free( data );
	return true;
}

// (public function, documented in gltexloaders.h)
bool texSaveNIF( NifModel * nif, const QString & filepath, QModelIndex & iData )
{
	// Work out the extension and format
	// If DDS raw, DXT1 or DXT5, copy directly from texture
	//qWarning() << "texSaveNIF: saving" << filepath << "to" << iData;
	
	QFile f( filepath );
	if ( ! f.open( QIODevice::ReadOnly ) )
		throw QString( "could not open file" );
	
	if ( filepath.endsWith( ".nif", Qt::CaseInsensitive ) || filepath.endsWith( ".texcache", Qt::CaseInsensitive ) )
	{
		// NIF-to-NIF copy
		NifModel pix;
		
		if ( ! pix.load( f ) )
			throw QString( "failed to load NiPixelData from file" );
		
		// possibly update this to ATextureRenderData...
		QPersistentModelIndex iPixData;
		iPixData = pix.getBlock( 0, "NiPixelData" );
		if ( ! iPixData.isValid() )
			throw QString( "Texture .nifs should only have NiPixelData blocks" );
		
		nif->set<int>( iData, "Pixel Format", pix.get<int>( iPixData, "Pixel Format" ) );
		
		if ( nif->checkVersion( 0, 0x0A020000 ) && pix.checkVersion( 0, 0x0A020000 ) )
		{
			// copy masks
			nif->set<quint32>( iData, "Red Mask", pix.get<quint32>( iPixData, "Red Mask" ) );
			nif->set<quint32>( iData, "Green Mask", pix.get<quint32>( iPixData, "Green Mask" ) );
			nif->set<quint32>( iData, "Blue Mask", pix.get<quint32>( iPixData, "Blue Mask" ) );
			nif->set<quint32>( iData, "Alpha Mask", pix.get<quint32>( iPixData, "Alpha Mask" ) );
			nif->set<quint8>( iData, "Bits Per Pixel", pix.get<quint8>( iPixData, "Bits Per Pixel" ) );
			
			QModelIndex unknownSrc;
			QModelIndex unknownDest;
			
			// 2 sets of unknown bytes
			unknownSrc = pix.getIndex( iPixData, "Unknown 3 Bytes" );
			unknownDest = nif->getIndex( iData, "Unknown 3 Bytes" );
			
			for ( int i = 0; i < pix.rowCount( unknownSrc ); i++ )
			{
				nif->set<quint8>( unknownDest.child( i, 0 ), pix.get<quint8>( unknownSrc.child( i, 0 ) ));
			}
			
			unknownSrc = pix.getIndex( iPixData, "Unknown 8 Bytes" );
			unknownDest = nif->getIndex( iData, "Unknown 8 Bytes" );
			
			for ( int i = 0; i < pix.rowCount( unknownSrc ); i++ )
			{
				nif->set<quint8>( unknownDest.child( i, 0 ), pix.get<quint8>( unknownSrc.child( i, 0 ) ));
			}

			if ( nif->checkVersion( 0x0A010000, 0x0A020000 ) && pix.checkVersion( 0x0A010000, 0x0A020000 ) )
			{
				nif->set<quint32>( iData, "Unknown Int", pix.get<quint32>( iPixData, "Unknown Int" ) );
			}
		}
		else if ( nif->checkVersion( 0x14000004, 0 ) && pix.checkVersion( 0x14000004, 0 ) )
		{
			nif->set<quint8>( iData, "Bits Per Pixel", pix.get<quint8>( iPixData, "Bits Per Pixel" ) );
			nif->set<int>( iData, "Unknown Int 2", pix.get<int>( iPixData, "Unknown Int 2" ) );
			nif->set<quint32>( iData, "Unknown Int 3", pix.get<quint32>( iPixData, "Unknown Int 3" ) );
			nif->set<quint16>( iData, "Flags", pix.get<quint16>( iPixData, "Flags" ) );
			nif->set<quint32>( iData, "Unknown Int 4", pix.get<quint32>( iPixData, "Unknown Int 4" ) );
			
			if ( nif->checkVersion( 0x14030006, 0 ) && pix.checkVersion( 0x14030006, 0 ) )
			{
				nif->set<quint8>( iData, "Unknown Byte 1", pix.get<quint8>( iPixData, "Unknown Byte 1" ) );
			}
			
			QModelIndex srcChannels = pix.getIndex( iPixData, "Channels" );
			QModelIndex destChannels = nif->getIndex( iData, "Channels" );
			
			// copy channels
			for ( int i = 0; i < 4; i++ )
			{
#ifndef QT_NO_DEBUG
				qWarning() << "Channel" << i;
				qWarning() << pix.get<quint32>( srcChannels.child( i, 0 ), "Type" );
				qWarning() << pix.get<quint32>( srcChannels.child( i, 0 ), "Convention" );
				qWarning() << pix.get<quint8>( srcChannels.child( i, 0 ), "Bits Per Channel" );
				qWarning() << pix.get<quint8>( srcChannels.child( i, 0 ), "Unknown Byte 1" );
#endif

				nif->set<quint32>( destChannels.child( i, 0 ), "Type", pix.get<quint32>( srcChannels.child( i, 0 ), "Type" ));
				nif->set<quint32>( destChannels.child( i, 0 ), "Convention", pix.get<quint32>( srcChannels.child( i, 0 ), "Convention" ));
				nif->set<quint8>( destChannels.child( i, 0 ), "Bits Per Channel", pix.get<quint8>( srcChannels.child( i, 0 ), "Bits Per Channel" ));
				nif->set<quint8>( destChannels.child( i, 0 ), "Unknown Byte 1", pix.get<quint8>( srcChannels.child( i, 0 ), "Unknown Byte 1" ));
			}
			
			nif->set<quint32>( iData, "Num Faces", pix.get<quint32>( iPixData, "Num Faces" ) );
			
		}
		else
		{
			qWarning() << "NIF versions are incompatible, cannot copy pixel data";
			return false;
		}
		
		// ignore palette for now; what uses these? theoretically they don't exist past 4.2.2.0

/*
        <add name="Palette" type="Ref" template="NiPalette">Link to NiPalette, for 8-bit textures.</add>
    </niobject>
*/
		
		nif->set<quint32>( iData, "Num Mipmaps", pix.get<quint32>( iPixData, "Num Mipmaps" ) );
		nif->set<quint32>( iData, "Bytes Per Pixel", pix.get<quint32>( iPixData, "Bytes Per Pixel" ) );

		QModelIndex srcMipMaps = pix.getIndex( iPixData, "Mipmaps" );
		QModelIndex destMipMaps = nif->getIndex( iData, "Mipmaps" );
		nif->updateArray( destMipMaps );

		for ( int i = 0; i < pix.rowCount( srcMipMaps ); i++ )
		{
			nif->set<quint32>( destMipMaps.child( i, 0 ), "Width", pix.get<quint32>( srcMipMaps.child( i, 0 ), "Width" ) );
			nif->set<quint32>( destMipMaps.child( i, 0 ), "Height", pix.get<quint32>( srcMipMaps.child( i, 0 ), "Height" ) );
			nif->set<quint32>( destMipMaps.child( i, 0 ), "Offset", pix.get<quint32>( srcMipMaps.child( i, 0 ), "Offset" ) );
		}

		nif->set<quint32>( iData, "Num Pixels", pix.get<quint32>( iPixData, "Num Pixels" ) );

		QModelIndex srcPixelData = pix.getIndex( iPixData, "Pixel Data" );
		QModelIndex destPixelData = nif->getIndex( iData, "Pixel Data" );

		for ( int i = 0; i < pix.rowCount( srcPixelData ); i++ )
		{
			nif->updateArray( destPixelData.child( i, 0 ) );
			nif->set<QByteArray>( destPixelData.child( i, 0 ), "Pixel Data", pix.get<QByteArray>( srcPixelData.child( i, 0 ), "Pixel Data" ) );
		}

		//nif->set<>( iData, "", pix.get<>( iPixData, "" ) );
		//nif->set<>( iData, "", pix.get<>( iPixData, "" ) );


	}
	else if ( filepath.endsWith( ".bmp", Qt::CaseInsensitive ) || filepath.endsWith( ".tga", Qt::CaseInsensitive ) )
	{
		//qWarning() << "Copying from GL buffer";
		
		GLuint width, height, mipmaps;
		QString format;
		
		// fastest way to get parameters and ensure texture is active
		if ( texLoad( filepath, format, width, height, mipmaps ) )
		{
			//qWarning() << "Width" << width << "height" << height << "mipmaps" << mipmaps << "format" << format;
		}
		else
		{
			qWarning() << "Error importing texture" << filepath;
			return false;
		}
		
		// set texture as RGBA
		nif->set<quint32>( iData, "Pixel Format", 1 );
		nif->set<quint32>( iData, "Bits Per Pixel", 32 );
		if( nif->checkVersion( 0, 0x0A020000 ) )
		{
			// set masks
			nif->set<quint32>( iData, "Red Mask", RGBA_INV_MASK[0] );
			nif->set<quint32>( iData, "Green Mask", RGBA_INV_MASK[1] );
			nif->set<quint32>( iData, "Blue Mask", RGBA_INV_MASK[2] );
			nif->set<quint32>( iData, "Alpha Mask", RGBA_INV_MASK[3] );
			
			QModelIndex unknownEightBytes = nif->getIndex( iData, "Unknown 8 Bytes" );
			for ( int i = 0; i < 8; i++ )
			{
				nif->set<quint8>( unknownEightBytes.child( i, 0 ), unk8bytes32[i] );
			}
		}
		else if( nif->checkVersion( 0x14000004, 0 ) )
		{
			// set stuff
			nif->set<qint32>( iData, "Unknown Int 2", -1 ); // probably a link to something
			nif->set<quint8>( iData, "Flags", 1 );
			QModelIndex destChannels = nif->getIndex( iData, "Channels" );
			
			for ( int i = 0; i < 4; i++ )
			{
				nif->set<quint32>( destChannels.child( i, 0 ), "Type", i ); // red, green, blue, alpha
				nif->set<quint32>( destChannels.child( i, 0 ), "Convention", 0 ); // fixed
				nif->set<quint32>( destChannels.child( i, 0 ), "Bits Per Channel", 8 );
				nif->set<quint32>( destChannels.child( i, 0 ), "Unknown Byte 1", 0 );
			}
		}
		
		nif->set<quint32>( iData, "Num Mipmaps", mipmaps );
		nif->set<quint32>( iData, "Bytes Per Pixel", 4 );
		QModelIndex destMipMaps = nif->getIndex( iData, "Mipmaps" );
		nif->updateArray( destMipMaps );
		
		QByteArray pixelData;
		
		int mipmapWidth = width;
		int mipmapHeight = height;
		int mipmapOffset = 0;
		
		// generate sizes and copy mipmap to NIF
		for ( uint i = 0; i < mipmaps; i++ )
		{
			nif->set<quint32>( destMipMaps.child( i, 0 ), "Width", mipmapWidth );
			nif->set<quint32>( destMipMaps.child( i, 0 ), "Height", mipmapHeight );
			nif->set<quint32>( destMipMaps.child( i, 0 ), "Offset", mipmapOffset );
			
			quint32 mipmapSize = mipmapWidth * mipmapHeight * 4;
			
			QByteArray mipmapData;
			mipmapData.resize( mipmapSize );
			
			glGetTexImage( GL_TEXTURE_2D, i, GL_RGBA, GL_UNSIGNED_BYTE, mipmapData.data() );
			//qWarning() << "Copying mipmap" << i << "," << mipmapSize << "bytes";
			
			pixelData.append( mipmapData );
			
			//qWarning() << "Now have" << pixelData.size() << "bytes of pixel data";
			
			// generate next offset, resize
			mipmapOffset += mipmapWidth * mipmapHeight * 4;
			mipmapWidth = max( 1, mipmapWidth / 2 );
			mipmapHeight = max( 1, mipmapHeight / 2 );
		}
		
		// set total pixel size
		nif->set<quint32>( iData, "Num Pixels", mipmapOffset );
		
		QModelIndex iPixelData = nif->getIndex( iData, "Pixel Data" );
		nif->updateArray( iPixelData );
		QModelIndex iFaceData = iPixelData.child( 0, 0 );
		nif->updateArray( iFaceData );
		
		nif->set<QByteArray>( iFaceData, "Pixel Data", pixelData );
		
		// return true once perfected
		//return false;
	}
	else if ( filepath.endsWith( ".dds", Qt::CaseInsensitive ) )
	{
		//qWarning() << "Will copy from DDS data";
		DDSFormat ddsHeader;
		char tag[4];
		f.read(&tag[0], 4);
		if ( strncmp( tag,"DDS ", 4 ) != 0 || f.read((char *) &ddsHeader, sizeof(DDSFormat)) != sizeof( DDSFormat ) )
			throw QString( "not a DDS file" );

#ifndef QT_NO_DEBUG
		qWarning() << "Size: " << ddsHeader.dwSize << "Flags" << ddsHeader.dwFlags << "Height" << ddsHeader.dwHeight << "Width" << ddsHeader.dwWidth;
		qWarning() << "FourCC:" << ddsHeader.ddsPixelFormat.dwFourCC;
#endif
		if ( ddsHeader.ddsPixelFormat.dwFlags & DDPF_FOURCC )
		{
			switch ( ddsHeader.ddsPixelFormat.dwFourCC )
			{
				case FOURCC_DXT1:
					//qWarning() << "DXT1";
					nif->set<uint>( iData, "Pixel Format", 4 );
					break;
				case FOURCC_DXT5:
					//qWarning() << "DXT5";
					nif->set<uint>( iData, "Pixel Format", 5 );
					break;
				default:
					// don't know how eg. DXT3 can be stored in NIF
					qWarning() << "Unsupported DDS format:" << ddsHeader.ddsPixelFormat.dwFourCC << "FourCC";
					return false;
					break;
			}
		}
		else
		{
			//qWarning() << "RAW";
			// switch on BPP
			//nif->set<uint>( iData, "Pixel Format", 0 );
			switch( ddsHeader.ddsPixelFormat.dwBPP )
			{
				case 24:
					// RGB
					nif->set<uint>( iData, "Pixel Format", 0 );
					break;
				case 32:
					// RGBA
					nif->set<uint>( iData, "Pixel Format", 1 );
					break;
				default:
					// theoretically could have a palettised DDS in 8bpp
					qWarning() << "Unsupported DDS format:" << ddsHeader.ddsPixelFormat.dwBPP << "BPP";
					return false;
					break;
			}
		}

#ifndef QT_NO_DEBUG
		qWarning() << "BPP:" << ddsHeader.ddsPixelFormat.dwBPP;
		qWarning() << "RMask:" << ddsHeader.ddsPixelFormat.dwRMask;
		qWarning() << "GMask:" << ddsHeader.ddsPixelFormat.dwGMask;
		qWarning() << "BMask:" << ddsHeader.ddsPixelFormat.dwBMask;
		qWarning() << "AMask:" << ddsHeader.ddsPixelFormat.dwAMask;
#endif

		// Note that these might not match what's expected; hopefully the loader function is smart
		if( nif->checkVersion( 0, 0x0A020000 ) )
		{
			nif->set<uint>( iData, "Bits Per Pixel", ddsHeader.ddsPixelFormat.dwBPP );
			nif->set<uint>( iData, "Red Mask", ddsHeader.ddsPixelFormat.dwRMask );
			nif->set<uint>( iData, "Green Mask", ddsHeader.ddsPixelFormat.dwGMask );
			nif->set<uint>( iData, "Blue Mask", ddsHeader.ddsPixelFormat.dwBMask );
			nif->set<uint>( iData, "Alpha Mask", ddsHeader.ddsPixelFormat.dwAMask );

			QModelIndex unknownEightBytes = nif->getIndex( iData, "Unknown 8 Bytes" );
			for ( int i = 0; i < 8; i++ )
			{
				if ( ddsHeader.ddsPixelFormat.dwBPP == 24 )
				{
					nif->set<quint8>( unknownEightBytes.child( i, 0 ), unk8bytes24[i] );
				}
				else if ( ddsHeader.ddsPixelFormat.dwBPP == 32 )
				{
					nif->set<quint8>( unknownEightBytes.child( i, 0 ), unk8bytes32[i] );
				}
			}
		}
		else if( nif->checkVersion( 0x14000004, 0 ) )
		{
			// set stuff
			nif->set<qint32>( iData, "Unknown Int 2", -1 ); // probably a link to something
			nif->set<quint8>( iData, "Flags", 1 );
			QModelIndex destChannels = nif->getIndex( iData, "Channels" );
			// DXT1, DXT5
			if ( ddsHeader.ddsPixelFormat.dwFlags & DDPF_FOURCC )
			{
				// compressed
				nif->set<uint>( iData, "Bits Per Pixel", 0 );
				for ( int i = 0; i < 4; i++ )
				{
					if( i == 0 )
					{
						nif->set<quint32>( destChannels.child( i, 0 ), "Type", 4 ); // compressed
						nif->set<quint32>( destChannels.child( i, 0 ), "Convention", 4 ); // compressed
					}
					else
					{
						nif->set<quint32>( destChannels.child( i, 0 ), "Type", 19 ); // empty
						nif->set<quint32>( destChannels.child( i, 0 ), "Convention", 5 ); // empty
					}
					nif->set<quint8>( destChannels.child( i, 0 ), "Bits Per Channel", 0 );
					nif->set<quint8>( destChannels.child( i, 0 ), "Unknown Byte 1", 1 );
				}
			}
			else
			{
				nif->set<uint>( iData, "Bits Per Pixel", ddsHeader.ddsPixelFormat.dwBPP );
				// set RGB mask separately
				for ( int i = 0; i < 3; i++ )
				{
					if( ddsHeader.ddsPixelFormat.dwRMask == RGBA_INV_MASK[i] )
					{
						//qWarning() << "red channel" << i;
						nif->set<quint32>( destChannels.child( i, 0 ), "Type", 0 );
					}
					else if( ddsHeader.ddsPixelFormat.dwGMask == RGBA_INV_MASK[i] )
					{
						//qWarning() << "green channel" << i;
						nif->set<quint32>( destChannels.child( i, 0 ), "Type", 1 );
					}
					else if( ddsHeader.ddsPixelFormat.dwBMask == RGBA_INV_MASK[i] )
					{
						//qWarning() << "blue channel" << i;
						nif->set<quint32>( destChannels.child( i, 0 ), "Type", 2 );
					}
				}
				for ( int i = 0; i < 3; i++ )
				{
					nif->set<quint32>( destChannels.child( i, 0 ), "Convention", 0 ); // fixed
					nif->set<quint32>( destChannels.child( i, 0 ), "Bits Per Channel", 8 );
					nif->set<quint32>( destChannels.child( i, 0 ), "Unknown Byte 1", 0 );
				}
				if ( ddsHeader.ddsPixelFormat.dwBPP == 32 )
				{
					nif->set<quint32>( destChannels.child( 3, 0 ), "Type", 3 ); // alpha
					nif->set<quint32>( destChannels.child( 3, 0 ), "Convention", 0 ); // fixed
					nif->set<quint32>( destChannels.child( 3, 0 ), "Bits Per Channel", 8 );
					nif->set<quint32>( destChannels.child( 3, 0 ), "Unknown Byte 1", 0 );
				}
				else if ( ddsHeader.ddsPixelFormat.dwBPP == 24 )
				{
					nif->set<quint32>( destChannels.child( 3, 0 ), "Type", 19 ); // empty
					nif->set<quint32>( destChannels.child( 3, 0 ), "Convention", 5 ); // empty
					nif->set<quint32>( destChannels.child( 3, 0 ), "Bits Per Channel", 0 );
					nif->set<quint32>( destChannels.child( 3, 0 ), "Unknown Byte 1", 0 );
				}
			}
		}

		// generate mipmap sizes and offsets
		//qWarning() << "Mipmap count: " << ddsHeader.dwMipMapCount;
		nif->set<quint32>( iData, "Num Mipmaps", ddsHeader.dwMipMapCount );
		QModelIndex destMipMaps = nif->getIndex( iData, "Mipmaps" );
		nif->updateArray( destMipMaps );
		
		nif->set<quint32>( iData, "Bytes Per Pixel", ddsHeader.ddsPixelFormat.dwBPP / 8 );

		int mipmapWidth = ddsHeader.dwWidth;
		int mipmapHeight = ddsHeader.dwHeight;
		int mipmapOffset = 0;

		for ( quint32 i = 0; i < ddsHeader.dwMipMapCount; i ++ )
		{
			nif->set<quint32>( destMipMaps.child( i, 0 ), "Width", mipmapWidth );
			nif->set<quint32>( destMipMaps.child( i, 0 ), "Height", mipmapHeight );
			nif->set<quint32>( destMipMaps.child( i, 0 ), "Offset", mipmapOffset );
			
			if ( ddsHeader.ddsPixelFormat.dwFlags & DDPF_FOURCC )
			{
				if ( ddsHeader.ddsPixelFormat.dwFourCC == FOURCC_DXT1 )
				{
					mipmapOffset += max( 8, ( mipmapWidth * mipmapHeight / 2 ) );
				}
				else if ( ddsHeader.ddsPixelFormat.dwFourCC == FOURCC_DXT5 )
				{
					mipmapOffset += max( 16, ( mipmapWidth * mipmapHeight ) );
				}
			}
			else if ( ddsHeader.ddsPixelFormat.dwBPP == 24 )
			{
				mipmapOffset += ( mipmapWidth * mipmapHeight * 3 );
			}
			else if ( ddsHeader.ddsPixelFormat.dwBPP == 32 )
			{
				mipmapOffset += ( mipmapWidth * mipmapHeight * 4 );
			}
			mipmapWidth = max( 1, mipmapWidth / 2 );
			mipmapHeight = max( 1, mipmapHeight / 2 );
		}
		
		nif->set<quint32>( iData, "Num Pixels", mipmapOffset );
		
		// finally, copy the data...

		QModelIndex iPixelData = nif->getIndex( iData, "Pixel Data" );
		nif->updateArray( iPixelData );
		QModelIndex iFaceData = iPixelData.child( 0, 0 );
		nif->updateArray( iFaceData );
		
		f.seek( 4 + ddsHeader.dwSize );
		//qWarning() << "Reading from " << f.pos();
		
		QByteArray ddsData = f.read( mipmapOffset );
		
		//qWarning() << "Read " << ddsData.size() << " bytes of" << f.size() << ", now at" << f.pos();
		if ( ddsData.size() != mipmapOffset )
		{
			qWarning() << "Unexpected EOF";
			return false;
		}
		
		nif->set<QByteArray>( iFaceData, "Pixel Data", ddsData );

		/*
		QByteArray result = nif->get<QByteArray>( iFaceData, "Pixel Data" );
		for ( int i = 0; i < 16; i++ )
		{
			qWarning() << "Comparing byte " << i << ": result " << (quint8)result[i] << ", original " << (quint8)ddsData[i];
		}
		*/
		
		// return true once perfected
		//return false;
	}
	return true;
}
