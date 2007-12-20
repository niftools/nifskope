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

#ifndef GLTEXLOADERS_H
#define GLTEXLOADERS_H

//! A function for loading textures.
/*!
 * Loads a texture pointed to by filepath.
 * Returns true on success, and throws a QString otherwise.
 * The parameters format, width, height and mipmaps will be filled with information about
 * the loaded texture.
 *
 * \param filepath The full path to the texture that must be loaded.
 * \param format Contain the format, for instance "DDS (DXT3)" or "TGA", on successful load.
 * \param width Contains the texture width on successful load.
 * \param height Contains the texture height on successful load.
 * \param mipmaps Contains the number of mipmaps on successful load.
 * \return true if the load was successful, false otherwise.
 */
extern bool texLoad( const QString & filepath, QString & format, GLuint & width, GLuint & height, GLuint & mipmaps );

//! A function which checks whether the given file can be loaded.
/*!
 * The function checks whether the file exists, is readable, and whether its extension
 * is that of a supported file format (dds, tga, or bmp).
 *
 * \param filepath The full path to the texture that must be checked.
 */
extern bool texCanLoad( const QString & filepath );

#endif
