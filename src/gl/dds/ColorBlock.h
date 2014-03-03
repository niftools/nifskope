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

#ifndef _DDS_COLORBLOCK_H
#define _DDS_COLORBLOCK_H

#include "Color.h"
#include "Image.h"

/// Uncompressed 4x4 color block.
struct ColorBlock
{
	ColorBlock();
	ColorBlock(const ColorBlock & block);
	ColorBlock(const Image * img, uint x, uint y);
	
	void init(const Image * img, uint x, uint y);
	
	void swizzleDXT5n();
	void splatX();
	void splatY();
	
	bool isSingleColor() const;
	uint countUniqueColors() const;
	Color32 averageColor() const;
	bool hasAlpha() const;
	
	void diameterRange(Color32 * start, Color32 * end) const;
	void luminanceRange(Color32 * start, Color32 * end) const;
	void boundsRange(Color32 * start, Color32 * end) const;
	void boundsRangeAlpha(Color32 * start, Color32 * end) const;
	
	void sortColorsByAbsoluteValue();

	float volume() const;

	// Accessors
	const Color32 * colors() const;

	Color32 color(uint i) const;
	Color32 & color(uint i);
	
	Color32 color(uint x, uint y) const;
	Color32 & color(uint x, uint y);
	
private:
	
	Color32 m_color[4*4];
	
};


/// Get pointer to block colors.
inline const Color32 * ColorBlock::colors() const
{
	return m_color;
}

/// Get block color.
inline Color32 ColorBlock::color(uint i) const
{
	return m_color[i];
}

/// Get block color.
inline Color32 & ColorBlock::color(uint i)
{
	return m_color[i];
}

/// Get block color.
inline Color32 ColorBlock::color(uint x, uint y) const
{
	return m_color[y * 4 + x];
}

/// Get block color.
inline Color32 & ColorBlock::color(uint x, uint y)
{
	return m_color[y * 4 + x];
}

#endif // _DDS_COLORBLOCK_H
