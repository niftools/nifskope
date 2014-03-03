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

/*
 * This file is based on a similar file from the NVIDIA texture tools
 * (http://nvidia-texture-tools.googlecode.com/)
 *
 * Original license from NVIDIA follows.
 */

// This code is in the public domain -- castanyo@yahoo.es

#ifndef _DDS_IMAGE_H
#define _DDS_IMAGE_H

#include "Common.h"
#include "Color.h"

/// 32 bit RGBA image.
class Image
{
public:
	
	enum Format 
	{
		Format_RGB,
		Format_ARGB,
	};
	
	Image();
	~Image();
	
	void allocate(uint w, uint h);
	/*
	bool load(const char * name);
	
	void wrap(void * data, uint w, uint h);
	void unwrap();
	*/
	
	uint width() const;
	uint height() const;
	
	const Color32 * scanline(uint h) const;
	Color32 * scanline(uint h);
	
	const Color32 * pixels() const;
	Color32 * pixels();
	
	const Color32 & pixel(uint idx) const;
	Color32 & pixel(uint idx);
	
	const Color32 & pixel(uint x, uint y) const;
	Color32 & pixel(uint x, uint y);
	
	Format format() const;
	void setFormat(Format f);
	
private:
	void free();
	
private:
	uint m_width;
	uint m_height;
	Format m_format;
	Color32 * m_data;
};


inline const Color32 & Image::pixel(uint x, uint y) const
{
	return pixel(y * width() + x);
}

inline Color32 & Image::pixel(uint x, uint y)
{
	return pixel(y * width() + x);
}

#endif // _DDS_IMAGE_H
