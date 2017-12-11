/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005-2015, NIF File Format Library and Tools
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

#ifndef GLTEXLOADERS_H
#define GLTEXLOADERS_H

#ifdef _MSC_VER
#pragma warning(push, 0)
#endif

#include <gli.hpp>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

class QOpenGLContext;
class QByteArray;
class QModelIndex;
class QString;

typedef unsigned int GLuint;
typedef unsigned int GLenum;

//! Initialize the GL functions necessary for texture loading
extern void initializeTextureLoaders( const QOpenGLContext * context );
//! Create texture with glTexStorage2D using GLI
extern GLuint GLI_create_texture( gli::texture& texture, GLenum& target, GLuint& id );
//! Fallback for systems that do not have glTexStorage2D
extern GLuint GLI_create_texture_fallback( gli::texture& texture, GLenum & target, GLuint& id );
//! Rewrite of gli::load_dds to not crash on invalid textures
extern gli::texture load_if_valid( const char * data, unsigned int size );

//! @file gltexloaders.h Texture loading functions header

/*! A function for loading textures.
 *
 * Loads a texture pointed to by filepath.
 * Returns true on success, and throws a QString otherwise.
 * The parameters format, width, height and mipmaps will be filled with information about
 * the loaded texture.
 *
 * @param filepath	The full path to the texture that must be loaded.
 * @param format	Contain the format, for instance "DDS (DXT3)" or "TGA", on successful load.
 * @param width		Contains the texture width on successful load.
 * @param height	Contains the texture height on successful load.
 * @param mipmaps	Contains the number of mipmaps on successful load.
 * @return			True if the load was successful, false otherwise.
 */
extern bool texLoad( const QString & filepath, QString & format, GLenum & target, GLuint & width, GLuint & height, GLuint & mipmaps, GLuint & id );
extern bool texLoad( const QString & filepath, QString & format, GLenum & target, GLuint & width, GLuint & height, GLuint & mipmaps, QByteArray & data, GLuint & id );

/*! A function for loading textures.
 *
 * Loads a texture pointed to by model index.
 * Returns true on success, and throws a QString otherwise.
 * The parameters format, width, height and mipmaps will be filled with information about
 * the loaded texture.
 *
 * @param iData		Reference to pixel data block
 * @param format	Contain the format, for instance "DDS (DXT3)" or "TGA", on successful load.
 * @param width		Contains the texture width on successful load.
 * @param height	Contains the texture height on successful load.
 * @param mipmaps	Contains the number of mipmaps on successful load.
 * @return			True if the load was successful, false otherwise.
 */
extern bool texLoad( const QModelIndex & iData, QString & format, GLenum & target, GLuint & width, GLuint & height, GLuint & mipmaps, GLuint & id );

/*! A function which checks whether the given file can be loaded.
 *
 * The function checks whether the file exists, is readable, and whether its extension
 * is that of a supported file format (dds, tga, or bmp).
 *
 * @param filepath The full path to the texture that must be checked.
 */
extern bool texCanLoad( const QString & filepath );

/*! A function which checks whether the given file is supported.
*
* The function checks whether its extension
* is that of a supported file format (dds, tga, or bmp).
*
* @param filepath The full path to the texture that must be checked.
*/
extern bool texIsSupported( const QString & filepath );

/*! Save pixel data to a DDS file
 *
 * @param index		Reference to pixel data
 * @param filepath	The filepath to write
 * @param width		The width of the texture
 * @param height	The height of the texture
 * @param mipmaps	The number of mipmaps present
 * @return			True if the save was successful, false otherwise
 */
bool texSaveDDS( const QModelIndex & index, const QString & filepath, const GLuint & width, const GLuint & height, const GLuint & mipmaps );

/*! Save pixel data to a TGA file
 *
 * @param index		Reference to pixel data
 * @param filepath	The filepath to write
 * @param width		The width of the texture
 * @param height	The height of the texture
 * @return			True if the save was successful, false otherwise
 */
bool texSaveTGA( const QModelIndex & index, const QString & filepath, const GLuint & width, const GLuint & height );

/*! Save a file to pixel data
 *
 * @param filepath	The source texture to convert
 * @param iData		The pixel data to write
 */
bool texSaveNIF( class NifModel * nif, const QString & filepath, QModelIndex & iData );

#endif
