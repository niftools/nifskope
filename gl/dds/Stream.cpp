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

#include "Stream.h"

#include <stdio.h>  // printf
#include <string.h> // memcpy

unsigned int Stream::seek(unsigned int p)
{
	if (p > size) {
		printf("DDS: trying to seek beyond end of stream (corrupt file?)");
	}
	else {
		pos = p;
	}

	return pos;
}

unsigned int mem_read(Stream & mem, unsigned long long & i)
{
	if (mem.pos + 8 > mem.size) {
		printf("DDS: trying to read beyond end of stream (corrupt file?)");
		return(0);
	};
	memcpy(&i, mem.mem + mem.pos, 8); // @@ todo: make sure little endian
	mem.pos += 8;
	return(8);
}

unsigned int mem_read(Stream & mem, unsigned int & i)
{
	if (mem.pos + 4 > mem.size) {
		printf("DDS: trying to read beyond end of stream (corrupt file?)");
		return(0);
	};
	memcpy(&i, mem.mem + mem.pos, 4); // @@ todo: make sure little endian
	mem.pos += 4;
	return(4);
}

unsigned int mem_read(Stream & mem, unsigned short & i)
{
	if (mem.pos + 2 > mem.size) {
		printf("DDS: trying to read beyond end of stream (corrupt file?)");
		return(0);
	};
	memcpy(&i, mem.mem + mem.pos, 2); // @@ todo: make sure little endian
	mem.pos += 2;
	return(2);
}

unsigned int mem_read(Stream & mem, unsigned char & i)
{
	if (mem.pos + 1 > mem.size) {
		printf("DDS: trying to read beyond end of stream (corrupt file?)");
		return(0);
	};
	i = (mem.mem + mem.pos)[0];
	mem.pos += 1;
	return(1);
}

unsigned int mem_read(Stream & mem, unsigned char *i, unsigned int cnt)
{
	if (mem.pos + cnt > mem.size) {
		printf("DDS: trying to read beyond end of stream (corrupt file?)");
		return(0);
	};
	memcpy(i, mem.mem + mem.pos, cnt);
	mem.pos += cnt;
	return(cnt);
}

