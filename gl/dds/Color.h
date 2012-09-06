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

#ifndef _DDS_COLOR_H
#define _DDS_COLOR_H

/// 32 bit color stored as BGRA.
class Color32
{
public:
	Color32() { }
	Color32(const Color32 & c) : u(c.u) { }
	Color32(unsigned char R, unsigned char G, unsigned char B) { setRGBA(R, G, B, 0xFF); }
	Color32(unsigned char R, unsigned char G, unsigned char B, unsigned char A) { setRGBA( R, G, B, A); }
	//Color32(unsigned char c[4]) { setRGBA(c[0], c[1], c[2], c[3]); }
	//Color32(float R, float G, float B) { setRGBA(uint(R*255), uint(G*255), uint(B*255), 0xFF); }
	//Color32(float R, float G, float B, float A) { setRGBA(uint(R*255), uint(G*255), uint(B*255), uint(A*255)); }
	Color32(unsigned int U) : u(U) { }

	void setRGBA(unsigned char R, unsigned char G, unsigned char B, unsigned char A)
	{
		r = R;
		g = G;
		b = B;
		a = A;
	}

	void setBGRA(unsigned char B, unsigned char G, unsigned char R, unsigned char A = 0xFF)
	{
		r = R;
		g = G;
		b = B;
		a = A;
	}

	operator unsigned int () const {
		return u;
	}
	
	union {
		struct {
			unsigned char b, g, r, a;
		};
		unsigned int u;
	};
};

/// 16 bit 565 BGR color.
class Color16
{
public:
	Color16() { }
	Color16(const Color16 & c) : u(c.u) { }
	explicit Color16(unsigned short U) : u(U) { }
	
	union {
		struct {
			unsigned short b : 5;
			unsigned short g : 6;
			unsigned short r : 5;
		};
		unsigned short u;
	};
};

#endif // _DDS_COLOR_H
