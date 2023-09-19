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

#include "glproperty.h"

#include "message.h"
#include "gl/controllers.h"
#include "gl/glscene.h"
#include "gl/gltex.h"
#include "io/material.h"
#include "model/nifmodel.h"

#include <QOpenGLContext>


//! @file glproperty.cpp Encapsulation of NiProperty blocks defined in nif.xml

//! Helper function that checks texture sets
bool checkSet( int s, const QVector<QVector<Vector2> > & texcoords )
{
	return s >= 0 && s < texcoords.count() && texcoords[s].count();
}

Property * Property::create( Scene * scene, const NifModel * nif, const QModelIndex & index )
{
	Property * property = 0;

	if ( nif->isNiBlock( index, "NiAlphaProperty" ) ) {
		property = new AlphaProperty( scene, index );
	} else if ( nif->isNiBlock( index, "NiZBufferProperty" ) ) {
		property = new ZBufferProperty( scene, index );
	} else if ( nif->isNiBlock( index, "NiTexturingProperty" ) ) {
		property = new TexturingProperty( scene, index );
	} else if ( nif->isNiBlock( index, "NiTextureProperty" ) ) {
		property = new TextureProperty( scene, index );
	} else if ( nif->isNiBlock( index, "NiMaterialProperty" ) ) {
		property = new MaterialProperty( scene, index );
	} else if ( nif->isNiBlock( index, "NiSpecularProperty" ) ) {
		property = new SpecularProperty( scene, index );
	} else if ( nif->isNiBlock( index, "NiWireframeProperty" ) ) {
		property = new WireframeProperty( scene, index );
	} else if ( nif->isNiBlock( index, "NiVertexColorProperty" ) ) {
		property = new VertexColorProperty( scene, index );
	} else if ( nif->isNiBlock( index, "NiStencilProperty" ) ) {
		property = new StencilProperty( scene, index );
	} else if ( nif->isNiBlock( index, "BSLightingShaderProperty" ) ) {
		property = new BSLightingShaderProperty( scene, index );
	} else if ( nif->isNiBlock( index, "BSShaderLightingProperty" ) ) {
		property = new BSShaderLightingProperty( scene, index );
	} else if ( nif->isNiBlock( index, "BSEffectShaderProperty" ) ) {
		property = new BSEffectShaderProperty( scene, index );
	} else if ( nif->isNiBlock( index, "BSWaterShaderProperty" ) ) {
		property = new BSWaterShaderProperty( scene, index );
	} else if ( nif->isNiBlock( index, "BSShaderNoLightingProperty" ) ) {
		property = new BSShaderLightingProperty( scene, index );
	} else if ( nif->isNiBlock( index, "BSShaderPPLightingProperty" ) ) {
		property = new BSShaderLightingProperty( scene, index );
	} else if ( index.isValid() ) {
#ifndef QT_NO_DEBUG
		NifItem * item = static_cast<NifItem *>( index.internalPointer() );

		if ( item )
			qCWarning( nsNif ) << tr( "Unknown property: %1" ).arg( item->name() );
		else
			qCWarning( nsNif ) << tr( "Unknown property: I can't determine its name" );
#endif
	}

	if ( property )
		property->update( nif, index );

	return property;
}

PropertyList::PropertyList()
{
}

PropertyList::PropertyList( const PropertyList & other )
{
	operator=( other );
}

PropertyList::~PropertyList()
{
	clear();
}

void PropertyList::clear()
{
	for ( Property * p : properties ) {
		if ( --p->ref <= 0 )
			delete p;
	}
	properties.clear();
}

PropertyList & PropertyList::operator=( const PropertyList & other )
{
	clear();
	for ( Property * p : other.properties ) {
		add( p );
	}
	return *this;
}

bool PropertyList::contains( Property * p ) const
{
	if ( !p )
		return false;

	QList<Property *> props = properties.values( p->type() );
	return props.contains( p );
}

void PropertyList::add( Property * p )
{
	if ( p && !contains( p ) ) {
		++p->ref;
		properties.insert( p->type(), p );
	}
}

void PropertyList::del( Property * p )
{
	if ( !p )
		return;

	QHash<Property::Type, Property *>::iterator i = properties.find( p->type() );

	while ( p && i != properties.end() && i.key() == p->type() ) {
		if ( i.value() == p ) {
			i = properties.erase( i );

			if ( --p->ref <= 0 )
				delete p;
		} else {
			++i;
		}
	}
}

Property * PropertyList::get( const QModelIndex & index ) const
{
	if ( !index.isValid() )
		return 0;

	for ( Property * p : properties ) {
		if ( p->index() == index )
			return p;
	}
	return 0;
}

void PropertyList::validate()
{
	QList<Property *> rem;
	for ( Property * p : properties ) {
		if ( !p->isValid() )
			rem.append( p );
	}
	for ( Property * p : rem ) {
		del( p );
	}
}

void PropertyList::merge( const PropertyList & other )
{
	for ( Property * p : other.properties ) {
		if ( !properties.contains( p->type() ) )
			add( p );
	}
}

void AlphaProperty::updateImpl( const NifModel * nif, const QModelIndex & index )
{
	Property::updateImpl( nif, index );

	if ( index == iBlock ) {
		unsigned short flags = nif->get<int>( iBlock, "Flags" );

		alphaBlend = flags & 1;

		static const GLenum blendMap[16] = {
			GL_ONE, GL_ZERO, GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR,
			GL_DST_COLOR, GL_ONE_MINUS_DST_COLOR, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
			GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA, GL_SRC_ALPHA_SATURATE, GL_ONE,
			GL_ONE, GL_ONE, GL_ONE, GL_ONE
		};

		alphaSrc = blendMap[ ( flags >> 1 ) & 0x0f ];
		alphaDst = blendMap[ ( flags >> 5 ) & 0x0f ];

		static const GLenum testMap[8] = {
			GL_ALWAYS, GL_LESS, GL_EQUAL, GL_LEQUAL, GL_GREATER, GL_NOTEQUAL, GL_GEQUAL, GL_NEVER
		};

		alphaTest = flags & ( 1 << 9 );
		alphaFunc = testMap[ ( flags >> 10 ) & 0x7 ];
		alphaThreshold = float( nif->get<int>( iBlock, "Threshold" ) ) / 255.0;

		alphaSort = ( flags & 0x2000 ) == 0;

		// Temporary Weapon Blood fix for FO4
		if ( nif->getBSVersion() >= 130 )
			alphaTest |= (flags == 20547);
	}
}

void AlphaProperty::setController( const NifModel * nif, const QModelIndex & controller )
{
	auto contrName = nif->itemName(controller);
	if ( contrName == "BSNiAlphaPropertyTestRefController" ) {
		Controller * ctrl = new AlphaController( this, controller );
		registerController(nif, ctrl);
	}
}

void glProperty( AlphaProperty * p )
{
	if ( p && p->alphaBlend && p->scene->hasOption(Scene::DoBlending) ) {
		glEnable( GL_BLEND );
		glBlendFunc( p->alphaSrc, p->alphaDst );
	} else {
		glDisable( GL_BLEND );
	}

	if ( p && p->alphaTest && p->scene->hasOption(Scene::DoBlending) ) {
		glEnable( GL_ALPHA_TEST );
		glAlphaFunc( p->alphaFunc, p->alphaThreshold );
	} else {
		glDisable( GL_ALPHA_TEST );
	}
}

void ZBufferProperty::updateImpl( const NifModel * nif, const QModelIndex & index )
{
	Property::updateImpl( nif, index );

	if ( index == iBlock ) {
		int flags = nif->get<int>( iBlock, "Flags" );
		depthTest = flags & 1;
		depthMask = flags & 2;
		static const GLenum depthMap[8] = {
			GL_ALWAYS, GL_LESS, GL_EQUAL, GL_LEQUAL, GL_GREATER, GL_NOTEQUAL, GL_GEQUAL, GL_NEVER
		};

		// This was checking version 0x10000001 ?
		if ( nif->checkVersion( 0x04010012, 0x14000005 ) ) {
			depthFunc = depthMap[ nif->get < int > ( iBlock, "Function" ) & 0x07 ];
		} else if ( nif->checkVersion( 0x14010003, 0 ) ) {
			depthFunc = depthMap[ (flags >> 2 ) & 0x07 ];
		} else {
			depthFunc = GL_LEQUAL;
		}
	}
}

void glProperty( ZBufferProperty * p )
{
	if ( p ) {
		if ( p->depthTest ) {
			glEnable( GL_DEPTH_TEST );
			glDepthFunc( p->depthFunc );
		} else {
			glDisable( GL_DEPTH_TEST );
		}

		glDepthMask( p->depthMask ? GL_TRUE : GL_FALSE );
	} else {
		glEnable( GL_DEPTH_TEST );
		glDepthFunc( GL_LESS );
		glDepthMask( GL_TRUE );
		glDepthFunc( GL_LEQUAL );
	}
}

/*
    TexturingProperty
*/

void TexturingProperty::updateImpl( const NifModel * nif, const QModelIndex & index )
{
	Property::updateImpl( nif, index );

	if ( index == iBlock ) {
		static const char * texnames[numTextures] = {
			"Base Texture", "Dark Texture", "Detail Texture", "Gloss Texture", "Glow Texture", "Bump Map Texture", "Decal 0 Texture", "Decal 1 Texture", "Decal 2 Texture", "Decal 3 Texture"
		};

		for ( int t = 0; t < numTextures; t++ ) {
			QModelIndex iTex = nif->getIndex( iBlock, texnames[t] );

			if ( iTex.isValid() ) {
				textures[t].iSource  = nif->getBlockIndex( nif->getLink( iTex, "Source" ), "NiSourceTexture" );
				textures[t].coordset = nif->get<int>( iTex, "UV Set" );
				
				int filterMode = 0, clampMode = 0;
				if ( nif->checkVersion( 0, 0x14010002 ) ) {
					filterMode = nif->get<int>( iTex, "Filter Mode" );
					clampMode  = nif->get<int>( iTex, "Clamp Mode" );
				} else if ( nif->checkVersion( 0x14010003, 0 ) ) {
					auto flags = nif->get<ushort>( iTex, "Flags" );
					filterMode = ((flags & 0x0F00) >> 0x08);
					clampMode  = ((flags & 0xF000) >> 0x0C);
					textures[t].coordset = (flags & 0x00FF);
				}

				float af = 1.0;
				float max_af = get_max_anisotropy();
				// Let User Settings decide for trilinear
				if ( filterMode == GL_LINEAR_MIPMAP_LINEAR )
					af = max_af;

				// Override with value in NIF for 20.5+
				if ( nif->checkVersion( 0x14050004, 0 ) )
					af = std::min( max_af, (float)nif->get<ushort>( iTex, "Max Anisotropy" ) );

				textures[t].maxAniso = std::max( 1.0f, std::min( af, max_af ) );

				// See OpenGL docs on glTexParameter and GL_TEXTURE_MIN_FILTER option
				// See also http://gregs-blog.com/2008/01/17/opengl-texture-filter-parameters-explained/
				switch ( filterMode ) {
				case 0:
					textures[t].filter = GL_NEAREST;
					break;             // nearest
				case 1:
					textures[t].filter = GL_LINEAR;
					break;             // bilinear
				case 2:
					textures[t].filter = GL_LINEAR_MIPMAP_LINEAR;
					break;             // trilinear
				case 3:
					textures[t].filter = GL_NEAREST_MIPMAP_NEAREST;
					break;             // nearest from nearest
				case 4:
					textures[t].filter = GL_NEAREST_MIPMAP_LINEAR;
					break;             // interpolate from nearest
				case 5:
					textures[t].filter = GL_LINEAR_MIPMAP_NEAREST;
					break;             // bilinear from nearest
				default:
					textures[t].filter = GL_LINEAR;
					break;
				}

				switch ( clampMode ) {
				case 0:
					textures[t].wrapS = GL_CLAMP;
					textures[t].wrapT = GL_CLAMP;
					break;
				case 1:
					textures[t].wrapS = GL_CLAMP;
					textures[t].wrapT = GL_REPEAT;
					break;
				case 2:
					textures[t].wrapS = GL_REPEAT;
					textures[t].wrapT = GL_CLAMP;
					break;
				default:
					textures[t].wrapS = GL_REPEAT;
					textures[t].wrapT = GL_REPEAT;
					break;
				}

				textures[t].hasTransform = nif->get<int>( iTex, "Has Texture Transform" );

				if ( textures[t].hasTransform ) {
					textures[t].translation = nif->get<Vector2>( iTex, "Translation" );
					textures[t].tiling = nif->get<Vector2>( iTex, "Scale" );
					textures[t].rotation = nif->get<float>( iTex, "Rotation" );
					textures[t].center = nif->get<Vector2>( iTex, "Center" );
				} else {
					// we don't really need to set these since they won't be applied in bind() unless hasTransform is set
					textures[t].translation = Vector2();
					textures[t].tiling = Vector2( 1.0, 1.0 );
					textures[t].rotation = 0.0;
					textures[t].center = Vector2( 0.5, 0.5 );
				}
			} else {
				textures[t].iSource = QModelIndex();
			}
		}
	}
}

bool TexturingProperty::bind( int id, const QString & fname )
{
	GLuint mipmaps = 0;

	if ( id >= 0 && id <= (numTextures - 1) ) {
		if ( !fname.isEmpty() )
			mipmaps = scene->bindTexture( fname );
		else
			mipmaps = scene->bindTexture( textures[ id ].iSource );

		if ( mipmaps == 0 )
			return false;

		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, textures[id].maxAniso );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipmaps > 1 ? textures[id].filter : GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, textures[id].wrapS );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, textures[id].wrapT );
		glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
		glMatrixMode( GL_TEXTURE );
		glLoadIdentity();

		if ( textures[id].hasTransform ) {
			// Sign order is important here: get it backwards and we rotate etc.
			// around (-center, -center)
			glTranslatef( textures[id].center[0], textures[id].center[1], 0 );

			// rotation appears to be in radians
			glRotatef( rad2deg( textures[id].rotation ), 0, 0, 1 );
			// It appears that the scaling here is relative to center
			glScalef( textures[id].tiling[0], textures[id].tiling[1], 1 );
			glTranslatef( textures[id].translation[0], textures[id].translation[1], 0 );

			glTranslatef( -textures[id].center[0], -textures[id].center[1], 0 );
		}

		glMatrixMode( GL_MODELVIEW );
		return true;
	}

	return false;
}

bool TexturingProperty::bind( int id, const QVector<QVector<Vector2> > & texcoords )
{
	if ( checkSet( textures[id].coordset, texcoords ) && bind( id ) ) {
		glEnable( GL_TEXTURE_2D );
		glEnableClientState( GL_TEXTURE_COORD_ARRAY );
		glTexCoordPointer( 2, GL_FLOAT, 0, texcoords[ textures[id].coordset ].data() );
		return true;
	} else {
		glDisable( GL_TEXTURE_2D );
		return false;
	}
}

bool TexturingProperty::bind( int id, const QVector<QVector<Vector2> > & texcoords, int stage )
{
	return ( activateTextureUnit( stage ) && bind( id, texcoords ) );
}

QString TexturingProperty::fileName( int id ) const
{
	if ( id >= 0 && id <= (numTextures - 1) ) {
		QModelIndex iSource = textures[id].iSource;
		auto nif = NifModel::fromValidIndex(iSource);
		if ( nif ) {
			return nif->get<QString>( iSource, "File Name" );
		}
	}

	return QString();
}

int TexturingProperty::coordSet( int id ) const
{
	if ( id >= 0 && id <= (numTextures - 1) ) {
		return textures[id].coordset;
	}

	return -1;
}


//! Set the appropriate Controller
void TexturingProperty::setController( const NifModel * nif, const QModelIndex & iController )
{
	auto contrName = nif->itemName(iController);
	if ( contrName == "NiFlipController" ) {
		Controller * ctrl = new TexFlipController( this, iController );
		registerController(nif, ctrl);
	} else if ( contrName == "NiTextureTransformController" ) {
		Controller * ctrl = new TexTransController( this, iController );
		registerController(nif, ctrl);
	}
}

int TexturingProperty::getId( const QString & texname )
{
	const static QHash<QString, int> hash{
		{ "base",   0 },
		{ "dark",   1 },
		{ "detail", 2 },
		{ "gloss",  3 },
		{ "glow",   4 },
		{ "bumpmap", 5 },
		{ "decal0", 6 },
		{ "decal1", 7 },
		{ "decal2", 8 },
		{ "decal3", 9 }
	};

	return hash.value( texname, -1 );
}

void glProperty( TexturingProperty * p )
{
	if ( p && p->scene->hasOption(Scene::DoTexturing) && p->bind(0) ) {
		glEnable( GL_TEXTURE_2D );
	}
}

/*
    TextureProperty
*/

void TextureProperty::updateImpl( const NifModel * nif, const QModelIndex & index )
{
	Property::updateImpl( nif, index );

	if ( index == iBlock ) {
		iImage = nif->getBlockIndex( nif->getLink( iBlock, "Image" ), "NiImage" );
	}
}

bool TextureProperty::bind()
{
	if ( GLuint mipmaps = scene->bindTexture( fileName() ) ) {
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipmaps > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR );
		glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
		glMatrixMode( GL_TEXTURE );
		glLoadIdentity();
		glMatrixMode( GL_MODELVIEW );
		return true;
	}

	return false;
}

bool TextureProperty::bind( const QVector<QVector<Vector2> > & texcoords )
{
	if ( checkSet( 0, texcoords ) && bind() ) {
		glEnable( GL_TEXTURE_2D );
		glEnableClientState( GL_TEXTURE_COORD_ARRAY );
		glTexCoordPointer( 2, GL_FLOAT, 0, texcoords[ 0 ].data() );
		return true;
	} else {
		glDisable( GL_TEXTURE_2D );
		return false;
	}
}

QString TextureProperty::fileName() const
{
	auto nif = NifModel::fromValidIndex(iImage);
	if ( nif )
		return nif->get<QString>( iImage, "File Name" );

	return QString();
}


void TextureProperty::setController( const NifModel * nif, const QModelIndex & iController )
{
	auto contrName = nif->itemName(iController);
	if ( contrName == "NiFlipController" ) {
		Controller * ctrl = new TexFlipController( this, iController );
		registerController(nif, ctrl);
	}
}

void glProperty( TextureProperty * p )
{
	if ( p && p->scene->hasOption(Scene::DoTexturing) && p->bind() ) {
		glEnable( GL_TEXTURE_2D );
	}
}

/*
    MaterialProperty
*/

void MaterialProperty::updateImpl( const NifModel * nif, const QModelIndex & index )
{
	Property::updateImpl( nif, index );

	if ( index == iBlock ) {
		alpha = nif->get<float>( iBlock, "Alpha" );
		if ( alpha < 0.0 )
			alpha = 0.0;
		if ( alpha > 1.0 )
			alpha = 1.0;

		ambient  = Color4( nif->get<Color3>( iBlock, "Ambient Color" ) );
		diffuse  = Color4( nif->get<Color3>( iBlock, "Diffuse Color" ) );
		specular = Color4( nif->get<Color3>( iBlock, "Specular Color" ) );
		emissive = Color4( nif->get<Color3>( iBlock, "Emissive Color" ) );

		// OpenGL needs shininess clamped otherwise it generates GL_INVALID_VALUE
		shininess = std::min( std::max( nif->get<float>( iBlock, "Glossiness" ), 0.0f ), 128.0f );
	}
}

void MaterialProperty::setController( const NifModel * nif, const QModelIndex & iController )
{
	auto contrName = nif->itemName(iController);
	if ( contrName == "NiAlphaController" ) {
		Controller * ctrl = new AlphaController( this, iController );
		registerController(nif, ctrl);
	} else if ( contrName == "NiMaterialColorController" ) {
		Controller * ctrl = new MaterialColorController( this, iController );
		registerController(nif, ctrl);
	}
}


void glProperty( MaterialProperty * p, SpecularProperty * s )
{
	if ( p ) {
		glMaterial( GL_FRONT_AND_BACK, GL_AMBIENT, p->ambient.blend( p->alpha ) );
		glMaterial( GL_FRONT_AND_BACK, GL_DIFFUSE, p->diffuse.blend( p->alpha ) );
		glMaterial( GL_FRONT_AND_BACK, GL_EMISSION, p->emissive.blend( p->alpha ) );

		if ( !s || s->spec ) {
			glMaterialf( GL_FRONT_AND_BACK, GL_SHININESS, p->shininess );
			glMaterial( GL_FRONT_AND_BACK, GL_SPECULAR, p->specular.blend( p->alpha ) );
		} else {
			glMaterialf( GL_FRONT_AND_BACK, GL_SHININESS, 0.0 );
			glMaterial( GL_FRONT_AND_BACK, GL_SPECULAR, Color4( 0.0, 0.0, 0.0, p->alpha ) );
		}
	} else {
		Color4 a( 0.4f, 0.4f, 0.4f, 1.0f );
		Color4 d( 0.8f, 0.8f, 0.8f, 1.0f );
		Color4 s( 1.0f, 1.0f, 1.0f, 1.0f );
		glMaterialf( GL_FRONT_AND_BACK, GL_SHININESS, 33.0f );
		glMaterial( GL_FRONT_AND_BACK, GL_AMBIENT, a );
		glMaterial( GL_FRONT_AND_BACK, GL_DIFFUSE, d );
		glMaterial( GL_FRONT_AND_BACK, GL_SPECULAR, s );
	}
}

void SpecularProperty::updateImpl( const NifModel * nif, const QModelIndex & index )
{
	Property::updateImpl( nif, index );

	if ( index == iBlock ) {
		spec = nif->get<int>( iBlock, "Flags" ) != 0;
	}
}

void WireframeProperty::updateImpl( const NifModel * nif, const QModelIndex & index )
{
	Property::updateImpl( nif, index );

	if ( index == iBlock ) {
		wire = nif->get<int>( iBlock, "Flags" ) != 0;
	}
}

void glProperty( WireframeProperty * p )
{
	if ( p && p->wire ) {
		glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		glLineWidth( 1.0 );
	} else {
		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	}
}

void VertexColorProperty::updateImpl( const NifModel * nif, const QModelIndex & index )
{
	Property::updateImpl( nif, index );

	if ( index == iBlock ) {
		if ( nif->checkVersion( 0, 0x14010001 ) ) {
			vertexmode = nif->get<int>( iBlock, "Vertex Mode" );
			// 0 : source ignore
			// 1 : source emissive
			// 2 : source ambient + diffuse
			lightmode = nif->get<int>( iBlock, "Lighting Mode" );
			// 0 : emissive
			// 1 : emissive + ambient + diffuse
		} else {
			auto flags = nif->get<quint16>( iBlock, "Flags" );
			vertexmode = (flags & 0x0030) >> 4;
			lightmode = (flags & 0x0008) >> 3;
		}
	}
}

void glProperty( VertexColorProperty * p, bool vertexcolors )
{
	// FIXME

	if ( !vertexcolors ) {
		glDisable( GL_COLOR_MATERIAL );
		glColor( Color4( 1.0, 1.0, 1.0, 1.0 ) );
		return;
	}

	if ( p ) {
		//if ( p->lightmode )
		{
			switch ( p->vertexmode ) {
			case 0:
				glDisable( GL_COLOR_MATERIAL );
				glColor( Color4( 1.0, 1.0, 1.0, 1.0 ) );
				return;
			case 1:
				glEnable( GL_COLOR_MATERIAL );
				glColorMaterial( GL_FRONT_AND_BACK, GL_EMISSION );
				return;
			case 2:
			default:
				glEnable( GL_COLOR_MATERIAL );
				glColorMaterial( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE );
				return;
			}
		}
		//else
		//{
		//	glDisable( GL_LIGHTING );
		//	glDisable( GL_COLOR_MATERIAL );
		//}
	} else {
		glEnable( GL_COLOR_MATERIAL );
		glColorMaterial( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE );
	}
}

void StencilProperty::updateImpl( const NifModel * nif, const QModelIndex & index )
{
	using namespace Stencil;
	Property::updateImpl( nif, index );

	if ( index == iBlock ) {
		static const GLenum funcMap[8] = {
			GL_NEVER, GL_GEQUAL, GL_NOTEQUAL, GL_GREATER, GL_LEQUAL, GL_EQUAL, GL_LESS, GL_ALWAYS
		};

		static const GLenum opMap[6] = {
			GL_KEEP, GL_ZERO, GL_REPLACE, GL_INCR, GL_DECR, GL_INVERT
		};

		int drawMode = 0;
		if ( nif->checkVersion( 0, 0x14000005 ) ) {
			drawMode = nif->get<int>( iBlock, "Draw Mode" );
			func = funcMap[std::min(nif->get<quint32>( iBlock, "Stencil Function" ), (quint32)TEST_MAX - 1 )];
			failop = opMap[std::min( nif->get<quint32>( iBlock, "Fail Action" ), (quint32)ACTION_MAX - 1 )];
			zfailop = opMap[std::min( nif->get<quint32>( iBlock, "Z Fail Action" ), (quint32)ACTION_MAX - 1 )];
			zpassop = opMap[std::min( nif->get<quint32>( iBlock, "Pass Action" ), (quint32)ACTION_MAX - 1 )];
			stencil = (nif->get<quint8>( iBlock, "Stencil Enabled" ) & ENABLE_MASK);
		} else {
			auto flags = nif->get<int>( iBlock, "Flags" );
			drawMode = (flags & DRAW_MASK) >> DRAW_POS;
			func = funcMap[(flags & TEST_MASK) >> TEST_POS];
			failop = opMap[(flags & FAIL_MASK) >> FAIL_POS];
			zfailop = opMap[(flags & ZFAIL_MASK) >> ZFAIL_POS];
			zpassop = opMap[(flags & ZPASS_MASK) >> ZPASS_POS];
			stencil = (flags & ENABLE_MASK);
		}

		switch ( drawMode ) {
		case DRAW_CW:
			cullEnable = true;
			cullMode = GL_FRONT;
			break;
		case DRAW_BOTH:
			cullEnable = false;
			cullMode = GL_BACK;
			break;
		case DRAW_CCW:
		default:
			cullEnable = true;
			cullMode = GL_BACK;
			break;
		}

		ref = nif->get<quint32>( iBlock, "Stencil Ref" );
		mask = nif->get<quint32>( iBlock, "Stencil Mask" );
	}
}

void glProperty( StencilProperty * p )
{
	if ( p ) {
		if ( p->cullEnable )
			glEnable( GL_CULL_FACE );
		else
			glDisable( GL_CULL_FACE );

		glCullFace( p->cullMode );

		if ( p->stencil ) {
			glEnable( GL_STENCIL_TEST );
			glStencilFunc( p->func, p->ref, p->mask );
			glStencilOp( p->failop, p->zfailop, p->zpassop );
		} else {
			glDisable( GL_STENCIL_TEST );
		}
	} else {
		glEnable( GL_CULL_FACE );
		glCullFace( GL_BACK );
		glDisable( GL_STENCIL_TEST );
	}
}

/*
    BSShaderLightingProperty
*/

BSShaderLightingProperty::~BSShaderLightingProperty()
{
	if ( material )
		delete material;
}

void BSShaderLightingProperty::updateImpl( const NifModel * nif, const QModelIndex & index )
{
	Property::updateImpl( nif, index );

	if ( index == iBlock ) {
		iTextureSet = nif->getBlockIndex( nif->getLink( iBlock, "Texture Set" ), "BSShaderTextureSet" );
		iWetMaterial = nif->getIndex( iBlock, "Root Material" );
	}
}

void BSShaderLightingProperty::resetParams()
{
	flags1 = ShaderFlags::SLSF1_ZBuffer_Test;
	flags2 = ShaderFlags::SLSF2_ZBuffer_Write;

	uvScale.reset();
	uvOffset.reset();
	clampMode = CLAMP_S_CLAMP_T;

	hasVertexColors = false;
	hasVertexAlpha = false;

	depthTest = false;
	depthWrite = false;
	isDoubleSided = false;
	isVertexAlphaAnimation = false;
}

void glProperty( BSShaderLightingProperty * p )
{
	if ( p && p->scene->hasOption(Scene::DoTexturing) && p->bind(0) ) {
		glEnable( GL_TEXTURE_2D );
	}
}

void BSShaderLightingProperty::clear()
{
	Property::clear();

	setMaterial(nullptr);
}

void BSShaderLightingProperty::setMaterial( Material * newMaterial )
{
	if (newMaterial && !newMaterial->isValid()) {
		delete newMaterial;
		newMaterial = nullptr;
	}
	if ( material && material != newMaterial ) {
		delete material;
	}
	material = newMaterial;
}

bool BSShaderLightingProperty::bind( int id, const QString & fname, TexClampMode mode )
{
	GLuint mipmaps = 0;

	if ( !fname.isEmpty() )
		mipmaps = scene->bindTexture( fname );
	else
		mipmaps = scene->bindTexture( this->fileName( id ) );

	if ( mipmaps == 0 )
		return false;


	switch ( mode )
	{
	case TexClampMode::CLAMP_S_CLAMP_T:
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
		break;
	case TexClampMode::CLAMP_S_WRAP_T:
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
		break;
	case TexClampMode::WRAP_S_CLAMP_T:
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
		break;
	case TexClampMode::MIRRORED_S_MIRRORED_T:
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT );
		break;
	case TexClampMode::WRAP_S_WRAP_T:
	default:
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
		break;
	}

	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, get_max_anisotropy() );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipmaps > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR );
	glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	glMatrixMode( GL_TEXTURE );
	glLoadIdentity();
	glMatrixMode( GL_MODELVIEW );
	return true;
}

bool BSShaderLightingProperty::bind( int id, const QVector<QVector<Vector2> > & texcoords )
{
	if ( checkSet( 0, texcoords ) && bind( id ) ) {
		glEnable( GL_TEXTURE_2D );
		glEnableClientState( GL_TEXTURE_COORD_ARRAY );
		glTexCoordPointer( 2, GL_FLOAT, 0, texcoords[ 0 ].data() );
		return true;
	}

	glDisable( GL_TEXTURE_2D );
	return false;
}

bool BSShaderLightingProperty::bindCube( int id, const QString & fname )
{
	Q_UNUSED( id );

	GLuint result = 0;

	if ( !fname.isEmpty() )
		result = scene->bindTexture( fname );

	if ( result == 0 )
		return false;

	glEnable( GL_TEXTURE_CUBE_MAP_SEAMLESS );

	glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
	glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	glMatrixMode( GL_TEXTURE );
	glLoadIdentity();
	glMatrixMode( GL_MODELVIEW );

	return true;
}

enum
{
	BGSM1_DIFFUSE = 0,
	BGSM1_NORMAL,
	BGSM1_SPECULAR,
	BGSM1_G2P,
	BGSM1_ENV,
	BGSM20_GLOW = 4,
	BGSM1_GLOW = 5,
	BGSM1_ENVMASK = 5,
	BGSM20_REFLECT,
	BGSM20_LIGHTING,

	BGSM1_MAX = 9,
	BGSM20_MAX = 10
};

QString BSShaderLightingProperty::fileName( int id ) const
{
	const NifModel * nif;

	// Fallout 4
	nif = NifModel::fromValidIndex(iWetMaterial);
	if ( nif ) {
		// BSLSP
		auto m = static_cast<ShaderMaterial *>(material);
		if ( m && m->isValid() ) {
			auto tex = m->textures();
			if ( tex.count() >= BGSM1_MAX ) {
				switch ( id ) {
				case 0: // Diffuse
					if ( !tex[BGSM1_DIFFUSE].isEmpty() )
						return tex[BGSM1_DIFFUSE];
					break;
				case 1: // Normal
					if ( !tex[BGSM1_NORMAL].isEmpty() )
						return tex[BGSM1_NORMAL];
					break;
				case 2: // Glow
					if ( tex.count() == BGSM1_MAX && m->bGlowmap && !tex[BGSM1_GLOW].isEmpty() )
						return tex[BGSM1_GLOW];

					if ( tex.count() == BGSM20_MAX && m->bGlowmap && !tex[BGSM20_GLOW].isEmpty() )
						return tex[BGSM20_GLOW];
					break;
				case 3: // Greyscale
					if ( m->bGrayscaleToPaletteColor && !tex[BGSM1_G2P].isEmpty() )
						return tex[BGSM1_G2P];
					break;
				case 4: // Cubemap
					if ( tex.count() == BGSM1_MAX && m->bEnvironmentMapping && !tex[BGSM1_ENV].isEmpty() )
						return tex[BGSM1_ENV];
					break;
				case 5: // Env Mask
					if ( m->bEnvironmentMapping && !tex[BGSM1_ENVMASK].isEmpty() )
						return tex[BGSM1_ENVMASK];
					break;
				case 7: // Specular
					if ( m->bSpecularEnabled && !tex[BGSM1_SPECULAR].isEmpty() )
						return tex[BGSM1_SPECULAR];
					break;
				}
			}
			if ( tex.count() >= BGSM20_MAX ) {
				switch ( id ) {
				case 8:
					if ( m->bSpecularEnabled && !tex[BGSM20_REFLECT].isEmpty() )
						return tex[BGSM20_REFLECT];
					break;
				case 9:
					if ( m->bSpecularEnabled && !tex[BGSM20_LIGHTING].isEmpty() )
						return tex[BGSM20_LIGHTING];
					break;
				}
			}
		}

		return QString();
	}

	// From iTextureSet
	nif = NifModel::fromValidIndex(iTextureSet);
	if ( nif ) {
		if ( id >= 0 && id < nif->get<int>(iTextureSet, "Num Textures") ) {
			QModelIndex iTextures = nif->getIndex(iTextureSet, "Textures");
			return nif->get<QString>( iTextures.child(id, 0) );
		}

		return QString();
	}

	// From material
	auto m = static_cast<EffectMaterial*>(material);
	if ( m ) {
		if (m->isValid()) {
			auto tex = m->textures();
			return tex[id];
		}

		return QString();
	}

	// Handle niobject name="BSEffectShaderProperty...
	nif = NifModel::fromIndex( iBlock );
	if ( nif ) {
		switch ( id ) {
		case 0:
			return nif->get<QString>( iBlock, "Source Texture" );
		case 1:
			return nif->get<QString>( iBlock, "Greyscale Texture" );
		case 2:
			return nif->get<QString>( iBlock, "Env Map Texture" );
		case 3:
			return nif->get<QString>( iBlock, "Normal Texture" );
		case 4:
			return nif->get<QString>( iBlock, "Env Mask Texture" );
		case 6:
			return nif->get<QString>( iBlock, "Reflectance Texture" );
		case 7:
			return nif->get<QString>( iBlock, "Lighting Texture" );
		}
	}

	return QString();
}

int BSShaderLightingProperty::getId( const QString & id )
{
	const static QHash<QString, int> hash{
		{ "base",   0 },
		{ "dark",   1 },
		{ "detail", 2 },
		{ "gloss",  3 },
		{ "glow",   4 },
		{ "bumpmap", 5 },
		{ "decal0", 6 },
		{ "decal1", 7 }
	};

	return hash.value( id, -1 );
}

void BSShaderLightingProperty::setFlags1( const NifModel * nif )
{
	if ( nif->getBSVersion() >= 151 ) {
		auto sf1 = nif->getArray<quint32>( iBlock, "SF1" );
		auto sf2 = nif->getArray<quint32>( iBlock, "SF2" );
		sf1.append( sf2 );

		uint64_t flags = 0;
		for ( auto sf : sf1 ) {
			flags |= ShaderFlags::CRC_TO_FLAG.value( sf, 0 );
		}
		flags1 = ShaderFlags::SF1( (uint32_t)flags );
	} else {
		flags1 = ShaderFlags::SF1( nif->get<unsigned int>(iBlock, "Shader Flags 1") );
	}
}

void BSShaderLightingProperty::setFlags2( const NifModel * nif )
{
	if ( nif->getBSVersion() >= 151 ) {
		auto sf1 = nif->getArray<quint32>( iBlock, "SF1" );
		auto sf2 = nif->getArray<quint32>( iBlock, "SF2" );
		sf1.append( sf2 );

		uint64_t flags = 0;
		for ( auto sf : sf1 ) {
			flags |= ShaderFlags::CRC_TO_FLAG.value( sf, 0 );
		}
		flags2 = ShaderFlags::SF2( (uint32_t)(flags >> 32) );
	} else {
		flags2 = ShaderFlags::SF2( nif->get<unsigned int>(iBlock, "Shader Flags 2") );
	}
}

/*
	BSLightingShaderProperty
*/

void BSLightingShaderProperty::updateImpl( const NifModel * nif, const QModelIndex & index )
{
	BSShaderLightingProperty::updateImpl( nif, index );

	if ( index == iBlock ) {
		setMaterial(name.endsWith(".bgsm", Qt::CaseInsensitive) ? new ShaderMaterial(name, scene->game) : nullptr);
		updateParams(nif);
	}
	else if ( index == iTextureSet ) {
		updateParams(nif);
	}
}

void BSLightingShaderProperty::resetParams()
{
	BSShaderLightingProperty::resetParams();

	hasGlowMap = false;
	hasEmittance = false;
	hasSoftlight = false;
	hasBacklight = false;
	hasRimlight = false;
	hasSpecularMap = false;
	hasMultiLayerParallax = false;
	hasEnvironmentMap = false;
	useEnvironmentMask = false;
	hasHeightMap = false;
	hasRefraction = false;
	hasDetailMask = false;
	hasTintMask = false;
	hasTintColor = false;
	greyscaleColor = false;

	emissiveColor = Color3(0, 0, 0);
	emissiveMult = 1.0;

	specularColor = Color3(0, 0, 0);
	specularGloss = 0;
	specularStrength = 0;

	tintColor = Color3(0, 0, 0);

	alpha = 1.0;

	lightingEffect1 = 0.0;
	lightingEffect2 = 1.0;

	environmentReflection = 0.0;

	// Multi-layer properties
	innerThickness = 1.0;
	innerTextureScale.reset();
	outerRefractionStrength = 0.0;
	outerReflectionStrength = 1.0;

	fresnelPower = 5.0;
	paletteScale = 1.0;
	rimPower = 2.0;
	backlightPower = 0.0;
}

void BSLightingShaderProperty::updateParams( const NifModel * nif )
{
	resetParams();

	setFlags1( nif );
	setFlags2( nif );

	if ( nif->getBSVersion() >= 151 ) {
		shaderType = ShaderFlags::ShaderType::ST_EnvironmentMap;
		hasVertexAlpha = true;
		hasVertexColors = true;
	} else {
		shaderType = ShaderFlags::ShaderType( nif->get<unsigned int>(iBlock, "Shader Type") );
		hasVertexAlpha = hasSF1(ShaderFlags::SLSF1_Vertex_Alpha);
		hasVertexColors = hasSF2(ShaderFlags::SLSF2_Vertex_Colors);
	}
	isVertexAlphaAnimation = hasSF2(ShaderFlags::SLSF2_Tree_Anim);

	ShaderMaterial * m = ( material && material->isValid() ) ? static_cast<ShaderMaterial*>(material) : nullptr;
	if ( m ) {
		alpha = m->fAlpha;

		uvScale.set(m->fUScale, m->fVScale);
		uvOffset.set(m->fUOffset, m->fVOffset);

		specularColor = Color3(m->cSpecularColor);
		specularGloss = m->fSmoothness;
		specularStrength = m->fSpecularMult;

		emissiveColor = Color3(m->cEmittanceColor);
		emissiveMult = m->fEmittanceMult;

		if ( m->bTileU && m->bTileV )
			clampMode = TexClampMode::WRAP_S_WRAP_T;
		else if ( m->bTileU )
			clampMode = TexClampMode::WRAP_S_CLAMP_T;
		else if ( m->bTileV )
			clampMode = TexClampMode::CLAMP_S_WRAP_T;
		else
			clampMode = TexClampMode::CLAMP_S_CLAMP_T;

		fresnelPower = m->fFresnelPower;
		greyscaleColor = m->bGrayscaleToPaletteColor;
		paletteScale = m->fGrayscaleToPaletteScale;

		hasSpecularMap = m->bSpecularEnabled && (!m->textureList[2].isEmpty()
			|| (nif->getBSVersion() >= 151 && !m->textureList[7].isEmpty()));
		hasGlowMap = m->bGlowmap;
		hasEmittance = m->bEmitEnabled;
		hasBacklight = m->bBackLighting;
		hasRimlight = m->bRimLighting;
		hasSoftlight = m->bSubsurfaceLighting;
		rimPower = m->fRimPower;
		backlightPower = m->fBacklightPower;
		isDoubleSided = m->bTwoSided;
		depthTest = m->bZBufferTest;
		depthWrite = m->bZBufferWrite;

		hasEnvironmentMap = m->bEnvironmentMapping || m->bPBR;
		useEnvironmentMask = hasEnvironmentMap && !m->bGlowmap && !m->textureList[5].isEmpty();
		environmentReflection = m->fEnvironmentMappingMaskScale;

		if ( hasSoftlight )
			lightingEffect1 = m->fSubsurfaceLightingRolloff;

	} else { // m == nullptr

		auto textures = nif->getArray<QString>(iTextureSet, "Textures");

		isDoubleSided = hasSF2( ShaderFlags::SLSF2_Double_Sided );
		depthTest = hasSF1( ShaderFlags::SLSF1_ZBuffer_Test );
		depthWrite = hasSF2( ShaderFlags::SLSF2_ZBuffer_Write );

		alpha = nif->get<float>( iBlock, "Alpha" );

		uvScale.set( nif->get<Vector2>(iBlock, "UV Scale") );
		uvOffset.set( nif->get<Vector2>(iBlock, "UV Offset") );
		clampMode = TexClampMode( nif->get<uint>( iBlock, "Texture Clamp Mode" ) );

		// Specular
		if ( hasSF1( ShaderFlags::SLSF1_Specular ) ) {
			specularColor = nif->get<Color3>(iBlock, "Specular Color");
			specularGloss = nif->get<float>( iBlock, "Glossiness" );
			if ( specularGloss == 0.0f ) // FO4
				specularGloss = nif->get<float>( iBlock, "Smoothness" );
			specularStrength = nif->get<float>(iBlock, "Specular Strength");
		}

		// Emissive
		emissiveColor = nif->get<Color3>( iBlock, "Emissive Color" );
		emissiveMult = nif->get<float>( iBlock, "Emissive Multiple" );

		hasEmittance = hasSF1( ShaderFlags::SLSF1_Own_Emit );
		hasGlowMap = isST(ShaderFlags::ST_GlowShader) && hasSF2( ShaderFlags::SLSF2_Glow_Map ) && !textures.value( 2, "" ).isEmpty();

		// Version Dependent settings
		if ( nif->getBSVersion() < 130 ) {
			lightingEffect1 = nif->get<float>( iBlock, "Lighting Effect 1" );
			lightingEffect2 = nif->get<float>( iBlock, "Lighting Effect 2" );

			innerThickness = nif->get<float>( iBlock, "Parallax Inner Layer Thickness" );
			outerRefractionStrength = nif->get<float>( iBlock, "Parallax Refraction Scale" );
			outerReflectionStrength = nif->get<float>( iBlock, "Parallax Envmap Strength" );
			innerTextureScale.set( nif->get<Vector2>(iBlock, "Parallax Inner Layer Texture Scale") );

			hasSpecularMap = hasSF1( ShaderFlags::SLSF1_Specular ) && !textures.value( 7, "" ).isEmpty();
			hasHeightMap = isST( ShaderFlags::ST_Heightmap ) && hasSF1( ShaderFlags::SLSF1_Parallax ) && !textures.value( 3, "" ).isEmpty();
			hasBacklight = hasSF2( ShaderFlags::SLSF2_Back_Lighting );
			hasRimlight = hasSF2( ShaderFlags::SLSF2_Rim_Lighting );
			hasSoftlight = hasSF2( ShaderFlags::SLSF2_Soft_Lighting );
			hasMultiLayerParallax = hasSF2( ShaderFlags::SLSF2_Multi_Layer_Parallax );
			hasRefraction = hasSF1( ShaderFlags::SLSF1_Refraction );

			hasTintMask = isST( ShaderFlags::ST_FaceTint );
			hasDetailMask = hasTintMask;

			if ( isST( ShaderFlags::ST_HairTint ) )
				setTintColor( nif, "Hair Tint Color" );
			else if ( isST( ShaderFlags::ST_SkinTint ) )
				setTintColor( nif, "Skin Tint Color" );
		} else {
			hasSpecularMap = hasSF1( ShaderFlags::SLSF1_Specular );
			greyscaleColor = hasSF1( ShaderFlags::SLSF1_Greyscale_To_PaletteColor );
			paletteScale = nif->get<float>( iBlock, "Grayscale to Palette Scale" );
			lightingEffect1 = nif->get<float>( iBlock, "Subsurface Rolloff" );
			backlightPower = nif->get<float>( iBlock, "Backlight Power" );
			fresnelPower = nif->get<float>( iBlock, "Fresnel Power" );
		}

		// Environment Map, Mask and Reflection Scale
		hasEnvironmentMap = 
			( isST(ShaderFlags::ST_EnvironmentMap) && hasSF1(ShaderFlags::SLSF1_Environment_Mapping) )
			|| ( isST(ShaderFlags::ST_EyeEnvmap) && hasSF1(ShaderFlags::SLSF1_Eye_Environment_Mapping) )
			|| ( nif->getBSVersion() == 100 && hasMultiLayerParallax );
		
		useEnvironmentMask = hasEnvironmentMap && !textures.value( 5, "" ).isEmpty();

		if ( isST( ShaderFlags::ST_EnvironmentMap ) )
			environmentReflection = nif->get<float>( iBlock, "Environment Map Scale" );
		else if ( isST( ShaderFlags::ST_EyeEnvmap ) )
			environmentReflection = nif->get<float>( iBlock, "Eye Cubemap Scale" );
	}
}

void BSLightingShaderProperty::setController( const NifModel * nif, const QModelIndex & iController )
{
	auto contrName = nif->itemName(iController);
	if ( contrName == "BSLightingShaderPropertyFloatController" ) {
		Controller * ctrl = new LightingFloatController( this, iController );
		registerController(nif, ctrl);
	} else if ( contrName == "BSLightingShaderPropertyColorController" ) {
		Controller * ctrl = new LightingColorController( this, iController );
		registerController(nif, ctrl);
	}
}

void BSLightingShaderProperty::setTintColor( const NifModel* nif, const QString & itemName )
{
	hasTintColor = true;
	tintColor = nif->get<Color3>(iBlock, itemName);
}

/*
	BSEffectShaderProperty
*/

void BSEffectShaderProperty::updateImpl( const NifModel * nif, const QModelIndex & index )
{
	BSShaderLightingProperty::updateImpl( nif, index );

	if ( index == iBlock ) {
		setMaterial(name.endsWith(".bgem", Qt::CaseInsensitive) ? new EffectMaterial(name, scene->game) : nullptr);
		updateParams(nif);
	}
	else if ( index == iTextureSet )
		updateParams(nif);
}

void BSEffectShaderProperty::resetParams() 
{
	BSShaderLightingProperty::resetParams();

	hasSourceTexture = false;
	hasGreyscaleMap = false;
	hasEnvironmentMap = false;
	hasEnvironmentMask = false;
	hasNormalMap = false;
	useFalloff = false;
	hasRGBFalloff = false;

	greyscaleColor = false;
	greyscaleAlpha = false;

	hasWeaponBlood = false;

	falloff.startAngle = 1.0f;
	falloff.stopAngle = 0.0f;
	falloff.startOpacity = 1.0f;
	falloff.stopOpacity = 0.0f;
	falloff.softDepth = 1.0f;

	lumEmittance = 0.0;

	emissiveColor = Color4(0, 0, 0, 0);
	emissiveMult = 1.0;

	lightingInfluence = 0.0;
	environmentReflection = 0.0;
}

void BSEffectShaderProperty::updateParams( const NifModel * nif )
{
	resetParams();

	setFlags1( nif );
	setFlags2( nif );

	hasVertexAlpha = hasSF1( ShaderFlags::SLSF1_Vertex_Alpha );
	hasVertexColors = hasSF2( ShaderFlags::SLSF2_Vertex_Colors );
	isVertexAlphaAnimation = hasSF2(ShaderFlags::SLSF2_Tree_Anim);

	EffectMaterial * m = ( material && material->isValid() ) ? static_cast<EffectMaterial*>(material) : nullptr;
	if ( m ) {
		hasSourceTexture = !m->textureList[0].isEmpty();
		hasGreyscaleMap = !m->textureList[1].isEmpty();
		hasEnvironmentMap = !m->textureList[2].isEmpty();
		hasNormalMap = !m->textureList[3].isEmpty();
		hasEnvironmentMask = !m->textureList[4].isEmpty();

		environmentReflection = m->fEnvironmentMappingMaskScale;

		greyscaleAlpha = m->bGrayscaleToPaletteAlpha;
		greyscaleColor = m->bGrayscaleToPaletteColor;
		useFalloff = m->bFalloffEnabled;
		hasRGBFalloff = m->bFalloffColorEnabled;

		depthTest = m->bZBufferTest;
		depthWrite = m->bZBufferWrite;
		isDoubleSided = m->bTwoSided;

		lumEmittance = m->fLumEmittance;

		uvScale.set(m->fUScale, m->fVScale);
		uvOffset.set(m->fUOffset, m->fVOffset);

		if ( m->bTileU && m->bTileV )
			clampMode = TexClampMode::WRAP_S_WRAP_T;
		else if ( m->bTileU )
			clampMode = TexClampMode::WRAP_S_CLAMP_T;
		else if ( m->bTileV )
			clampMode = TexClampMode::CLAMP_S_WRAP_T;
		else
			clampMode = TexClampMode::CLAMP_S_CLAMP_T;

		emissiveColor = Color4(m->cBaseColor, m->fAlpha);
		emissiveMult = m->fBaseColorScale;

		if ( m->bEffectLightingEnabled )
			lightingInfluence = m->fLightingInfluence;

		falloff.startAngle = m->fFalloffStartAngle;
		falloff.stopAngle = m->fFalloffStopAngle;
		falloff.startOpacity = m->fFalloffStartOpacity;
		falloff.stopOpacity = m->fFalloffStopOpacity;
		falloff.softDepth = m->fSoftDepth;

	} else { // m == nullptr
		
		hasSourceTexture = !nif->get<QString>( iBlock, "Source Texture" ).isEmpty();
		hasGreyscaleMap = !nif->get<QString>( iBlock, "Greyscale Texture" ).isEmpty();

		greyscaleAlpha = hasSF1( ShaderFlags::SLSF1_Greyscale_To_PaletteAlpha );
		greyscaleColor = hasSF1( ShaderFlags::SLSF1_Greyscale_To_PaletteColor );
		useFalloff = hasSF1( ShaderFlags::SLSF1_Use_Falloff );

		depthTest = hasSF1( ShaderFlags::SLSF1_ZBuffer_Test );
		depthWrite = hasSF2( ShaderFlags::SLSF2_ZBuffer_Write );
		isDoubleSided = hasSF2( ShaderFlags::SLSF2_Double_Sided );

		if ( nif->getBSVersion() < 130 ) {
			hasWeaponBlood = hasSF2( ShaderFlags::SLSF2_Weapon_Blood );
		} else {
			hasEnvironmentMap = !nif->get<QString>( iBlock, "Env Map Texture" ).isEmpty();
			hasEnvironmentMask = !nif->get<QString>(iBlock, "Env Mask Texture").isEmpty();
			hasNormalMap = !nif->get<QString>( iBlock, "Normal Texture" ).isEmpty();

			environmentReflection = nif->get<float>( iBlock, "Environment Map Scale" );

			// Receive Shadows -> RGB Falloff for FO4
			hasRGBFalloff = hasSF1( ShaderFlags::SF1( 1 << 8 ) );
		}

		uvScale.set( nif->get<Vector2>(iBlock, "UV Scale") );
		uvOffset.set( nif->get<Vector2>(iBlock, "UV Offset") );
		clampMode = TexClampMode( nif->get<quint8>( iBlock, "Texture Clamp Mode" ) );

		emissiveColor = nif->get<Color4>(iBlock, "Base Color");
		emissiveMult = nif->get<float>(iBlock, "Base Color Scale");

		if ( hasSF2( ShaderFlags::SLSF2_Effect_Lighting ) )
			lightingInfluence = (float)nif->get<quint8>( iBlock, "Lighting Influence" ) / 255.0;

		falloff.startAngle = nif->get<float>( iBlock, "Falloff Start Angle" );
		falloff.stopAngle = nif->get<float>( iBlock, "Falloff Stop Angle" );
		falloff.startOpacity = nif->get<float>( iBlock, "Falloff Start Opacity" );
		falloff.stopOpacity = nif->get<float>( iBlock, "Falloff Stop Opacity" );
		falloff.softDepth = nif->get<float>( iBlock, "Soft Falloff Depth" );
	}
}

void BSEffectShaderProperty::setController( const NifModel * nif, const QModelIndex & iController )
{
	auto contrName = nif->itemName(iController);
	if ( contrName == "BSEffectShaderPropertyFloatController" ) {
		Controller * ctrl = new EffectFloatController( this, iController );
		registerController(nif, ctrl);
	} else if ( contrName == "BSEffectShaderPropertyColorController" ) {
		Controller * ctrl = new EffectColorController( this, iController );
		registerController(nif, ctrl);
	}
}

/*
	BSWaterShaderProperty
*/

unsigned int BSWaterShaderProperty::getWaterShaderFlags() const
{
	return (unsigned int)waterShaderFlags;
}

void BSWaterShaderProperty::setWaterShaderFlags( unsigned int val )
{
	waterShaderFlags = WaterShaderFlags::SF1( val );
}

