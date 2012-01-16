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

#include "Color.h"
#include "Image.h"

#include <stdio.h> // printf

Image::Image() : m_width(0), m_height(0), m_format(Format_RGB), m_data(0)
{
}

Image::~Image()
{
	free();
}

void Image::allocate(uint w, uint h)
{
	free();
	m_width = w;
	m_height = h;
	m_data = new Color32[w * h];
}

void Image::free()
{
	if (m_data) delete [] m_data;
	m_data = 0;
}


uint Image::width() const
{
	return m_width;
}

uint Image::height() const
{
	return m_height;
}

const Color32 * Image::scanline(uint h) const
{
	if (h >= m_height) {
		printf("DDS: scanline beyond dimensions of image");
		return m_data;
	}
	return m_data + h * m_width;
}

Color32 * Image::scanline(uint h)
{
	if (h >= m_height) {
		printf("DDS: scanline beyond dimensions of image");
		return m_data;
	}
	return m_data + h * m_width;
}

const Color32 * Image::pixels() const
{
	return m_data;
}

Color32 * Image::pixels()
{
	return m_data;
}

const Color32 & Image::pixel(uint idx) const
{
	if (idx >= m_width * m_height) {
		printf("DDS: pixel beyond dimensions of image");
		return m_data[0];
	}
	return m_data[idx];
}

Color32 & Image::pixel(uint idx)
{
	if (idx >= m_width * m_height) {
		printf("DDS: pixel beyond dimensions of image");
		return m_data[0];
	}
	return m_data[idx];
}


Image::Format Image::format() const
{
	return m_format;
}

void Image::setFormat(Image::Format f)
{
	m_format = f;
}

