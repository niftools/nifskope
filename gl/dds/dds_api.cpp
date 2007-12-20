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

#include <dds_api.h>
#include <Stream.h>
#include <DirectDrawSurface.h>
#include <stdio.h> // printf
#include <QByteArray.h>

int imb_is_a_dds(unsigned char *mem) // note: use at most first 32 bytes
{
	/* heuristic check to see if mem contains a DDS file */
	/* header.fourcc == FOURCC_DDS */
	if ((mem[0] != 'D') || (mem[1] != 'D') || (mem[2] != 'S') || (mem[3] != ' ')) return(0);
	/* header.size == 124 */
	if ((mem[4] != 124) || mem[5] || mem[6] || mem[7]) return(0);
	return(1);
}

int load_dds(unsigned char *mem, int size)
{
	DirectDrawSurface dds(mem, size); /* reads header */
	unsigned char bits_per_pixel;
	Image img;
	unsigned int numpixels = 0;
	int col;
	unsigned char *cp = (unsigned char *) &col;
	Color32 pixel;
	Color32 *pixels = 0;

	/* check if DDS is valid and supported */
	if (!dds.isValid()) {
		printf("DDS: not valid; header follows\n");
		dds.printInfo();
		return(0);
	}
	if (!dds.isSupported()) {
		printf("DDS: format not supported\n");
		return(0);
	}
	if ((dds.width() > 65535) || (dds.height() > 65535)) {
		printf("DDS: dimensions too large\n");
		return(0);
	}

	/* convert DDS into ImBuf */
	if (dds.hasAlpha()) bits_per_pixel = 32;
	else bits_per_pixel = 24;

	dds.mipmap(&img, 0, 0); /* load first face, first mipmap */
	pixels = img.pixels();
	numpixels = dds.width() * dds.height();
	cp[3] = 0xff; /* default alpha if alpha channel is not present */

	for (unsigned int i = 0; i < numpixels; i++) {
		pixel = pixels[i];
		cp[0] = pixel.r; /* set R component of col */
		cp[1] = pixel.g; /* set G component of col */
		cp[2] = pixel.b; /* set B component of col */
		if (bits_per_pixel == 32)
			cp[3] = pixel.a; /* set A component of col */
	}
	
	return(1);
}
