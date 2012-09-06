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

#ifndef _DDS_API_H
#define _DDS_API_H

#include "Image.h"

#define DDSD_MIPMAPCOUNT           0x00020000
#define DDPF_FOURCC                0x00000004

// DDS format structure
struct DDSFormat {
	uint32 dwSize;
	uint32 dwFlags;
	uint32 dwHeight;
	uint32 dwWidth;
	uint32 dwLinearSize;
	uint32 dummy1;
	uint32 dwMipMapCount;
	uint32 dummy2[11];
	struct {
		uint32 dwSize;
		uint32 dwFlags;
		uint32 dwFourCC;
		uint32 dwBPP;
		uint32 dwRMask;
		uint32 dwGMask;
		uint32 dwBMask;
		uint32 dwAMask;
	} ddsPixelFormat;
};

// compressed texture pixel formats
#define FOURCC_DXT1  0x31545844
#define FOURCC_DXT2  0x32545844
#define FOURCC_DXT3  0x33545844
#define FOURCC_DXT4  0x34545844
#define FOURCC_DXT5  0x35545844


//! Check whether the memory array effectively contains a DDS file.
/*!
 * Caller must make sure that mem contains at least 8 bytes.
 * \return 1 if it is a DDS file, 0 otherwise.
 */
int is_a_dds(unsigned char *mem); /* use only first 8 bytes of mem */


//! Load a DDS file.
/*!
 * \return 0 if load failed, or pointer to Image object otherwise. The caller is responsible for destructing the image object (using delete).
 */
Image * load_dds(unsigned char *mem, int size, int face = 0, int mipmap = 0);


//! Load a DDS file.
/*!
* \return 0 if load failed, or pointer to Image object otherwise. The caller is responsible for destructing the image object (using delete).
*/
Image * load_dds(const unsigned char *mem, int size, int face, int mipmap, DDSFormat* format);

#endif /* __DDS_API_H */
