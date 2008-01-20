/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005-2008, NIF File Format Library and Tools
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

#include "dds_api.h"
#include "Stream.h"
#include "DirectDrawSurface.h"
#include <stdio.h> // printf

int is_a_dds(unsigned char *mem) // note: use at most first 8 bytes
{
	/* heuristic check to see if mem contains a DDS file */
	/* header.fourcc == FOURCC_DDS */
	if ((mem[0] != 'D') || (mem[1] != 'D') || (mem[2] != 'S') || (mem[3] != ' ')) return(0);
	/* header.size == 124 */
	if ((mem[4] != 124) || mem[5] || mem[6] || mem[7]) return(0);
	return(1);
}

Image * load_dds(unsigned char *mem, int size)
{
	DirectDrawSurface dds(mem, size); /* reads header */
	
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
	
	/* load first face, first mipmap */
	Image *img = new Image();	
	dds.mipmap(img, 0, 0);
	return img;
}
