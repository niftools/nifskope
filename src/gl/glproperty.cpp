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

void AlphaProperty::update( const NifModel * nif, const QModelIndex & block )
{
	Property::update( nif, block );

	if ( iBlock.isValid() && iBlock == block ) {
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
		alphaThreshold = nif->get<int>( iBlock, "Threshold" ) / 255.0;

		alphaSort = ( flags & 0x2000 ) == 0;

		// Temporary Weapon Blood fix for FO4
		if ( nif->getUserVersion2() == 130 )
			alphaTest |= (flags == 20547);
	}
}

void AlphaProperty::setController( const NifModel * nif, const QModelIndex & controller )
{
	if ( nif->itemName( controller ) == "BSNiAlphaPropertyTestRefController" ) {
		Controller * ctrl = new AlphaController( this, controller );
		ctrl->update( nif, controller );
		controllers.append( ctrl );
	}
}

void AlphaProperty::setThreshold( float threshold )
{
	alphaThreshold = threshold;
}

void glProperty( AlphaProperty * p )
{
	if ( p && p->alphaBlend && (p->scene->options & Scene::DoBlending) ) {
		glDisable( GL_POLYGON_OFFSET_FILL );
		glEnable( GL_BLEND );
		glBlendFunc( p->alphaSrc, p->alphaDst );
	} else {
		glDisable( GL_BLEND );
	}

	if ( p && p->alphaTest && (p->scene->options & Scene::DoBlending) ) {
		glDisable( GL_POLYGON_OFFSET_FILL );
		glEnable( GL_ALPHA_TEST );
		glAlphaFunc( p->alphaFunc, p->alphaThreshold );
	} else {
		glDisable( GL_ALPHA_TEST );
	}
}

void ZBufferProperty::update( const NifModel * nif, const QModelIndex & block )
{
	Property::update( nif, block );

	if ( iBlock.isValid() && iBlock == block ) {
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

void TexturingProperty::update( const NifModel * nif, const QModelIndex & property )
{
	Property::update( nif, property );

	if ( iBlock.isValid() && iBlock == property ) {
		static const char * texnames[numTextures] = {
			"Base Texture", "Dark Texture", "Detail Texture", "Gloss Texture", "Glow Texture", "Bump Map Texture", "Decal 0 Texture", "Decal 1 Texture", "Decal 2 Texture", "Decal 3 Texture"
		};

		for ( int t = 0; t < numTextures; t++ ) {
			QModelIndex iTex = nif->getIndex( property, texnames[t] );

			if ( iTex.isValid() ) {
				textures[t].iSource  = nif->getBlock( nif->getLink( iTex, "Source" ), "NiSourceTexture" );
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
			glRotatef( (textures[id].rotation * 180.0 / PI ), 0, 0, 1 );
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
		QModelIndex iSource  = textures[ id ].iSource;
		const NifModel * nif = qobject_cast<const NifModel *>( iSource.model() );

		if ( nif && iSource.isValid() ) {
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
	if ( nif->itemName( iController ) == "NiFlipController" ) {
		Controller * ctrl = new TexFlipController( this, iController );
		ctrl->update( nif, iController );
		controllers.append( ctrl );
	} else if ( nif->itemName( iController ) == "NiTextureTransformController" ) {
		Controller * ctrl = new TexTransController( this, iController );
		ctrl->update( nif, iController );
		controllers.append( ctrl );
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
	if ( p && (p->scene->options & Scene::DoTexturing) && p->bind( 0 ) ) {
		glEnable( GL_TEXTURE_2D );
	}
}

/*
    TextureProperty
*/

void TextureProperty::update( const NifModel * nif, const QModelIndex & property )
{
	Property::update( nif, property );

	if ( iBlock.isValid() && iBlock == property ) {
		iImage = nif->getBlock( nif->getLink( iBlock, "Image" ), "NiImage" );
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
	const NifModel * nif = qobject_cast<const NifModel *>( iImage.model() );

	if ( nif && iImage.isValid() )
		return nif->get<QString>( iImage, "File Name" );

	return QString();
}


void TextureProperty::setController( const NifModel * nif, const QModelIndex & iController )
{
	if ( nif->itemName( iController ) == "NiFlipController" ) {
		Controller * ctrl = new TexFlipController( this, iController );
		ctrl->update( nif, iController );
		controllers.append( ctrl );
	}
}

void glProperty( TextureProperty * p )
{
	if ( p && (p->scene->options & Scene::DoTexturing) && p->bind() ) {
		glEnable( GL_TEXTURE_2D );
	}
}

/*
    MaterialProperty
*/

void MaterialProperty::update( const NifModel * nif, const QModelIndex & index )
{
	Property::update( nif, index );

	if ( iBlock.isValid() && iBlock == index ) {
		alpha = nif->get<float>( index, "Alpha" );

		if ( alpha < 0.0 )
			alpha = 0.0;

		if ( alpha > 1.0 )
			alpha = 1.0;

		ambient  = Color4( nif->get<Color3>( index, "Ambient Color" ) );
		diffuse  = Color4( nif->get<Color3>( index, "Diffuse Color" ) );
		specular = Color4( nif->get<Color3>( index, "Specular Color" ) );
		emissive = Color4( nif->get<Color3>( index, "Emissive Color" ) );

		// OpenGL needs shininess clamped otherwise it generates GL_INVALID_VALUE
		shininess = std::min( std::max( nif->get<float>( index, "Glossiness" ), 0.0f ), 128.0f );
	}
}

void MaterialProperty::setController( const NifModel * nif, const QModelIndex & iController )
{
	if ( nif->itemName( iController ) == "NiAlphaController" ) {
		Controller * ctrl = new AlphaController( this, iController );
		ctrl->update( nif, iController );
		controllers.append( ctrl );
	} else if ( nif->itemName( iController ) == "NiMaterialColorController" ) {
		Controller * ctrl = new MaterialColorController( this, iController );
		ctrl->update( nif, iController );
		controllers.append( ctrl );
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

void SpecularProperty::update( const NifModel * nif, const QModelIndex & block )
{
	Property::update( nif, block );

	if ( iBlock.isValid() && iBlock == block ) {
		spec = nif->get<int>( iBlock, "Flags" ) != 0;
	}
}

void WireframeProperty::update( const NifModel * nif, const QModelIndex & block )
{
	Property::update( nif, block );

	if ( iBlock.isValid() && iBlock == block ) {
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

void VertexColorProperty::update( const NifModel * nif, const QModelIndex & block )
{
	Property::update( nif, block );

	if ( iBlock.isValid() && iBlock == block ) {
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

void StencilProperty::update( const NifModel * nif, const QModelIndex & block )
{
	using namespace Stencil;
	Property::update( nif, block );

	if ( iBlock.isValid() && iBlock == block ) {
		static const GLenum funcMap[8] = {
			GL_NEVER, GL_GEQUAL, GL_NOTEQUAL, GL_GREATER, GL_LEQUAL, GL_EQUAL, GL_LESS, GL_ALWAYS
		};

		static const GLenum opMap[6] = {
			GL_KEEP, GL_ZERO, GL_REPLACE, GL_INCR, GL_DECR, GL_INVERT
		};

		int drawMode = 0;
		if ( nif->checkVersion( 0, 0x14000005 ) ) {
			drawMode = nif->get<int>( iBlock, "Draw Mode" );
			func = funcMap[std::max(nif->get<quint32>( iBlock, "Stencil Function" ), (quint32)TEST_MAX - 1 )];
			failop = opMap[std::max( nif->get<quint32>( iBlock, "Fail Action" ), (quint32)ACTION_MAX - 1 )];
			zfailop = opMap[std::max( nif->get<quint32>( iBlock, "Z Fail Action" ), (quint32)ACTION_MAX - 1 )];
			zpassop = opMap[std::max( nif->get<quint32>( iBlock, "Pass Action" ), (quint32)ACTION_MAX - 1 )];
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

void BSShaderLightingProperty::update( const NifModel * nif, const QModelIndex & property )
{
	Property::update( nif, property );

	if ( iBlock.isValid() && iBlock == property ) {
		iTextureSet = nif->getBlock( nif->getLink( iBlock, "Texture Set" ), "BSShaderTextureSet" );

		// handle niobject name="BSEffectShaderProperty...
		if ( !iTextureSet.isValid() )
			iSourceTexture = iBlock;

		iWetMaterial = nif->getIndex( iBlock, "Wet Material" );
	}
}

void glProperty( BSShaderLightingProperty * p )
{
	if ( p && (p->scene->options & Scene::DoTexturing) && p->bind( 0 ) ) {
		glEnable( GL_TEXTURE_2D );
	}
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

QString BSShaderLightingProperty::fileName( int id ) const
{
	const NifModel * nif;

	// Fallout 4
	nif = qobject_cast<const NifModel *>(iWetMaterial.model());
	if ( nif ) {
		// BSLSP
		auto m = static_cast<ShaderMaterial *>(material);
		if ( m && m->isValid() ) {
			auto tex = m->textures();
			if ( tex.count() == 9 ) {
				switch ( id ) {
				case 0: // Diffuse
					if ( !tex[0].isEmpty() )
						return tex[0];
					break;
				case 1: // Normal
					if ( !tex[1].isEmpty() )
						return tex[1];
					break;
				case 2: // Glow
					if ( m->bGlowmap && !tex[5].isEmpty() )
						return tex[5];
					break;
				case 3: // Greyscale
					if ( m->bGrayscaleToPaletteColor && !tex[3].isEmpty() )
						return tex[3];
					break;
				case 4: // Cubemap
					if ( m->bEnvironmentMapping && !tex[4].isEmpty() )
						return tex[4];
					break;
				case 5: // Env Mask
					if ( m->bEnvironmentMapping && !tex[5].isEmpty() )
						return tex[5];
					break;
				case 7: // Specular
					if ( m->bSpecularEnabled && !tex[2].isEmpty() )
						return tex[2];
					break;
				}
			}
		}
	}

	nif = qobject_cast<const NifModel *>(iTextureSet.model());
	if ( nif && iTextureSet.isValid() ) {
		int nTextures = nif->get<int>( iTextureSet, "Num Textures" );
		QModelIndex iTextures = nif->getIndex( iTextureSet, "Textures" );

		if ( id >= 0 && id < nTextures )
			return nif->get<QString>( iTextures.child( id, 0 ) );
	} else {
		// handle niobject name="BSEffectShaderProperty...
		auto m = static_cast<EffectMaterial *>(material);

		nif = qobject_cast<const NifModel *>(iSourceTexture.model());
		if ( !m && nif && iSourceTexture.isValid() ) {
			switch ( id ) {
			case 0:
				return nif->get<QString>( iSourceTexture, "Source Texture" );
			case 1:
				return nif->get<QString>( iSourceTexture, "Greyscale Texture" );
			case 2:
				return nif->get<QString>( iSourceTexture, "Env Map Texture" );
			case 3:
				return nif->get<QString>( iSourceTexture, "Normal Texture" );
			case 4:
				return nif->get<QString>( iSourceTexture, "Env Mask Texture" );
			}
		} else if ( m && m->isValid() ) {
			auto tex = m->textures();
			return tex[id];
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

QPersistentModelIndex BSShaderLightingProperty::getTextureSet() const
{
	return iTextureSet;
}

unsigned int BSShaderLightingProperty::getFlags1() const
{
	return (unsigned int)flags1;
}

unsigned int BSShaderLightingProperty::getFlags2() const
{
	return (unsigned int)flags2;
}

void BSShaderLightingProperty::setFlags1( unsigned int val )
{
	flags1 = ShaderFlags::SF1( val );
}

void BSShaderLightingProperty::setFlags2( unsigned int val )
{
	flags2 = ShaderFlags::SF2( val );
}

UVScale BSShaderLightingProperty::getUvScale() const
{
	return uvScale;
}

UVOffset BSShaderLightingProperty::getUvOffset() const
{
	return uvOffset;
}

void BSShaderLightingProperty::setUvScale( float x, float y )
{
	uvScale.x = x;
	uvScale.y = y;
}

void BSShaderLightingProperty::setUvOffset( float x, float y )
{
	uvOffset.x = x;
	uvOffset.y = y;
}

TexClampMode BSShaderLightingProperty::getClampMode() const
{
	return clampMode;
}

void BSShaderLightingProperty::setClampMode( uint mode )
{
	clampMode = TexClampMode( mode );
}

Material * BSShaderLightingProperty::mat() const
{
	return material;
}

/*
	BSLightingShaderProperty
*/

void BSLightingShaderProperty::update( const NifModel * nif, const QModelIndex & property )
{
	BSShaderLightingProperty::update( nif, property );

	if ( name.endsWith( ".bgsm", Qt::CaseInsensitive ) )
		material = new ShaderMaterial( name );

	if ( material && !material->isValid() )
		material = nullptr;

	if ( material && name.isEmpty() ) {
		delete material;
		material = nullptr;
	}

}

void BSLightingShaderProperty::updateParams( const NifModel * nif, const QModelIndex & prop )
{
	ShaderMaterial * m = nullptr;
	if ( mat() && mat()->isValid() )
		m = static_cast<ShaderMaterial *>(mat());

	auto stream = nif->getUserVersion2();
	auto textures = nif->getArray<QString>( getTextureSet(), "Textures" );

	setShaderType( nif->get<unsigned int>( prop, "Skyrim Shader Type" ) );
	setFlags1( nif->get<unsigned int>( prop, "Shader Flags 1" ) );
	setFlags2( nif->get<unsigned int>( prop, "Shader Flags 2" ) );

	hasVertexAlpha = hasSF1( ShaderFlags::SLSF1_Vertex_Alpha );
	hasVertexColors = hasSF2( ShaderFlags::SLSF2_Vertex_Colors );

	if ( !m ) {
		isDoubleSided = hasSF2( ShaderFlags::SLSF2_Double_Sided );
		depthTest = hasSF1( ShaderFlags::SLSF1_ZBuffer_Test );
		depthWrite = hasSF2( ShaderFlags::SLSF2_ZBuffer_Write );

		alpha = nif->get<float>( prop, "Alpha" );

		auto scale = nif->get<Vector2>( prop, "UV Scale" );
		auto offset = nif->get<Vector2>( prop, "UV Offset" );

		setUvScale( scale[0], scale[1] );
		setUvOffset( offset[0], offset[1] );
		setClampMode( nif->get<uint>( prop, "Texture Clamp Mode" ) );

		// Specular
		if ( hasSF1( ShaderFlags::SLSF1_Specular ) ) {
			auto spC = nif->get<Color3>( prop, "Specular Color" );
			auto spG = nif->get<float>( prop, "Glossiness" );
			// FO4
			if ( spG == 0.0 )
				spG = nif->get<float>( prop, "Smoothness" );
			auto spS = nif->get<float>( prop, "Specular Strength" );
			setSpecular( spC, spG, spS );
		} else {
			setSpecular( Color3( 0, 0, 0 ), 0, 0 );
		}

		// Emissive
		setEmissive( nif->get<Color3>( prop, "Emissive Color" ), nif->get<float>( prop, "Emissive Multiple" ) );

		hasEmittance = hasSF1( ShaderFlags::SLSF1_Own_Emit );
		hasGlowMap = getShaderType() & ShaderFlags::ST_GlowShader && hasSF2( ShaderFlags::SLSF2_Glow_Map ) && !textures.value( 2, "" ).isEmpty();


		// Version Dependent settings
		if ( stream < 130 ) {
			lightingEffect1 = nif->get<float>( prop, "Lighting Effect 1" );
			lightingEffect2 = nif->get<float>( prop, "Lighting Effect 2" );

			innerThickness = nif->get<float>( prop, "Parallax Inner Layer Thickness" );
			outerRefractionStrength = nif->get<float>( prop, "Parallax Refraction Scale" );
			outerReflectionStrength = nif->get<float>( prop, "Parallax Envmap Strength" );
			auto innerScale = nif->get<Vector2>( prop, "Parallax Inner Layer Texture Scale" );
			setInnerTextureScale( innerScale[0], innerScale[1] );

			hasSpecularMap = hasSF1( ShaderFlags::SLSF1_Specular ) && !textures.value( 7, "" ).isEmpty();
			hasHeightMap = isST( ShaderFlags::ST_Heightmap ) && hasSF1( ShaderFlags::SLSF1_Parallax ) && !textures.value( 3, "" ).isEmpty();
			hasBacklight = hasSF2( ShaderFlags::SLSF2_Back_Lighting );
			hasRimlight = hasSF2( ShaderFlags::SLSF2_Rim_Lighting );
			hasSoftlight = hasSF2( ShaderFlags::SLSF2_Soft_Lighting );
			hasModelSpaceNormals = hasSF1( ShaderFlags::SLSF1_Model_Space_Normals );
			hasMultiLayerParallax = hasSF2( ShaderFlags::SLSF2_Multi_Layer_Parallax );

			hasRefraction = hasSF1( ShaderFlags::SLSF1_Refraction );
			hasFireRefraction = hasSF1( ShaderFlags::SLSF1_Fire_Refraction );

			hasTintColor = false;
			hasTintMask = isST( ShaderFlags::ST_FaceTint );
			hasDetailMask = hasTintMask;

			QString tint;
			if ( isST( ShaderFlags::ST_HairTint ) )
				tint = "Hair Tint Color";
			else if ( isST( ShaderFlags::ST_SkinTint ) )
				tint = "Skin Tint Color";

			if ( !tint.isEmpty() ) {
				hasTintColor = true;
				setTintColor( nif->get<Color3>( prop, tint ) );
			}
		} else {
			hasSpecularMap = hasSF1( ShaderFlags::SLSF1_Specular );
			greyscaleColor = hasSF1( ShaderFlags::SLSF1_Greyscale_To_PaletteColor );
			paletteScale = nif->get<float>( prop, "Grayscale to Palette Scale" );
			lightingEffect1 = nif->get<float>( prop, "Subsurface Rolloff" );
			backlightPower = nif->get<float>( prop, "Backlight Power" );
			fresnelPower = nif->get<float>( prop, "Fresnel Power" );
		}

		// Environment Map, Mask and Reflection Scale
		hasEnvironmentMap = isST( ShaderFlags::ST_EnvironmentMap ) && hasSF1( ShaderFlags::SLSF1_Environment_Mapping );
		hasEnvironmentMap |= isST( ShaderFlags::ST_EyeEnvmap ) && hasSF1( ShaderFlags::SLSF1_Eye_Environment_Mapping );
		if ( stream == 100 )
			hasEnvironmentMap |= hasMultiLayerParallax;

		hasCubeMap = (
			isST( ShaderFlags::ST_EnvironmentMap )
			|| isST( ShaderFlags::ST_EyeEnvmap )
			|| isST( ShaderFlags::ST_MultiLayerParallax )
			)
			&& hasEnvironmentMap
			&& !textures.value( 4, "" ).isEmpty();

		useEnvironmentMask = hasEnvironmentMap && !textures.value( 5, "" ).isEmpty();

		if ( isST( ShaderFlags::ST_EnvironmentMap ) )
			environmentReflection = nif->get<float>( prop, "Environment Map Scale" );
		else if ( isST( ShaderFlags::ST_EyeEnvmap ) )
			environmentReflection = nif->get<float>( prop, "Eye Cubemap Scale" );

	} else {
		alpha = m->fAlpha;

		setUvScale( m->fUScale, m->fVScale );
		setUvOffset( m->fUOffset, m->fVOffset );
		setSpecular( m->cSpecularColor, m->fSmoothness, m->fSpecularMult );
		setEmissive( m->cEmittanceColor, m->fEmittanceMult );

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

		hasSpecularMap = m->bSpecularEnabled && !m->textureList[2].isEmpty();
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

		hasEnvironmentMap = m->bEnvironmentMapping;
		hasCubeMap = m->bEnvironmentMapping && !m->textureList[4].isEmpty();
		useEnvironmentMask = hasEnvironmentMap && !m->bGlowmap && !m->textureList[5].isEmpty();
		environmentReflection = m->fEnvironmentMappingMaskScale;

		if ( hasSoftlight )
			setLightingEffect1( m->fSubsurfaceLightingRolloff );
	}
}

void BSLightingShaderProperty::setController( const NifModel * nif, const QModelIndex & iController )
{
	if ( nif->itemName( iController ) == "BSLightingShaderPropertyFloatController" ) {
		Controller * ctrl = new LightingFloatController( this, iController );
		ctrl->update( nif, iController );
		controllers.append( ctrl );
	} else if ( nif->itemName( iController ) == "BSLightingShaderPropertyColorController" ) {
		Controller * ctrl = new LightingColorController( this, iController );
		ctrl->update( nif, iController );
		controllers.append( ctrl );
	}
}

void BSLightingShaderProperty::setShaderType( unsigned int t )
{
	shaderType = ShaderFlags::ShaderType( t );
}

ShaderFlags::ShaderType BSLightingShaderProperty::getShaderType()
{
	return shaderType;
}

void BSLightingShaderProperty::setEmissive( const Color3 & color, float mult )
{
	emissiveColor = color;
	emissiveMult = mult;
}

void BSLightingShaderProperty::setSpecular( const Color3 & color, float gloss, float strength )
{
	specularColor = color;
	specularGloss = gloss;
	specularStrength = strength;
}

Color3 BSLightingShaderProperty::getEmissiveColor() const
{
	return emissiveColor;
}

Color3 BSLightingShaderProperty::getSpecularColor() const
{
	return specularColor;
}

float BSLightingShaderProperty::getEmissiveMult() const
{
	return emissiveMult;
}

float BSLightingShaderProperty::getLightingEffect1() const
{
	return lightingEffect1;
}

float BSLightingShaderProperty::getLightingEffect2() const
{
	return lightingEffect2;
}

void BSLightingShaderProperty::setLightingEffect1( float val )
{
	lightingEffect1 = val;
}

void BSLightingShaderProperty::setLightingEffect2( float val )
{
	lightingEffect2 = val;
}

float BSLightingShaderProperty::getSpecularGloss() const
{
	return specularGloss;
}

float BSLightingShaderProperty::getSpecularStrength() const
{
	return specularStrength;
}

float BSLightingShaderProperty::getInnerThickness() const
{
	return innerThickness;
}

UVScale BSLightingShaderProperty::getInnerTextureScale() const
{
	return innerTextureScale;
}

float BSLightingShaderProperty::getOuterRefractionStrength() const
{
	return outerRefractionStrength;
}

float BSLightingShaderProperty::getOuterReflectionStrength() const
{
	return outerReflectionStrength;
}

void BSLightingShaderProperty::setInnerThickness( float thickness )
{
	innerThickness = thickness;
}

void BSLightingShaderProperty::setInnerTextureScale( float x, float y )
{
	innerTextureScale.x = x;
	innerTextureScale.y = y;
}

void BSLightingShaderProperty::setOuterRefractionStrength( float strength )
{
	outerRefractionStrength = strength;
}

void BSLightingShaderProperty::setOuterReflectionStrength( float strength )
{
	outerReflectionStrength = strength;
}

float BSLightingShaderProperty::getEnvironmentReflection() const
{
	return environmentReflection;
}

void BSLightingShaderProperty::setEnvironmentReflection( float strength )
{
	environmentReflection = strength;
}

float BSLightingShaderProperty::getAlpha() const
{
	return alpha;
}

void BSLightingShaderProperty::setAlpha( float opacity )
{
	alpha = opacity;
}

Color3 BSLightingShaderProperty::getTintColor() const
{
	return tintColor;
}

void BSLightingShaderProperty::setTintColor( const Color3 & c )
{
	tintColor = c;
}

/*
	BSEffectShaderProperty
*/

void BSEffectShaderProperty::update( const NifModel * nif, const QModelIndex & property )
{
	BSShaderLightingProperty::update( nif, property );

	if ( name.endsWith( ".bgem", Qt::CaseInsensitive ) )
		material = new EffectMaterial( name );

	if ( material && !material->isValid() )
		material = nullptr;

	if ( material && name.isEmpty() ) {
		delete material;
		material = nullptr;
	}
}

void BSEffectShaderProperty::updateParams( const NifModel * nif, const QModelIndex & prop )
{
	EffectMaterial * m = nullptr;
	if ( mat() && mat()->isValid() )
		m = static_cast<EffectMaterial *>(mat());

	auto stream = nif->getUserVersion2();

	setFlags1( nif->get<unsigned int>( prop, "Shader Flags 1" ) );
	setFlags2( nif->get<unsigned int>( prop, "Shader Flags 2" ) );

	hasVertexAlpha = hasSF1( ShaderFlags::SLSF1_Vertex_Alpha );
	hasVertexColors = hasSF2( ShaderFlags::SLSF2_Vertex_Colors );

	if ( !m ) {
		setEmissive( nif->get<Color4>( prop, "Emissive Color" ), nif->get<float>( prop, "Emissive Multiple" ) );

		hasSourceTexture = !nif->get<QString>( prop, "Source Texture" ).isEmpty();
		hasGreyscaleMap = !nif->get<QString>( prop, "Greyscale Texture" ).isEmpty();

		greyscaleAlpha = hasSF1( ShaderFlags::SLSF1_Greyscale_To_PaletteAlpha );
		greyscaleColor = hasSF1( ShaderFlags::SLSF1_Greyscale_To_PaletteColor );
		useFalloff = hasSF1( ShaderFlags::SLSF1_Use_Falloff );

		depthTest = hasSF1( ShaderFlags::SLSF1_ZBuffer_Test );
		depthWrite = hasSF2( ShaderFlags::SLSF2_ZBuffer_Write );
		isDoubleSided = hasSF2( ShaderFlags::SLSF2_Double_Sided );

		if ( stream < 130 ) {
			hasWeaponBlood = hasSF2( ShaderFlags::SLSF2_Weapon_Blood );
		} else {
			hasEnvMap = !nif->get<QString>( prop, "Env Map Texture" ).isEmpty();
			hasNormalMap = !nif->get<QString>( prop, "Normal Texture" ).isEmpty();
			hasEnvMask = !nif->get<QString>( prop, "Env Mask Texture" ).isEmpty();

			environmentReflection = nif->get<float>( prop, "Environment Map Scale" );

			// Receive Shadows -> RGB Falloff for FO4
			hasRGBFalloff = hasSF1( ShaderFlags::SF1( 1 << 8 ) );
		}

		auto scale = nif->get<Vector2>( prop, "UV Scale" );
		auto offset = nif->get<Vector2>( prop, "UV Offset" );

		setUvScale( scale[0], scale[1] );
		setUvOffset( offset[0], offset[1] );
		setClampMode( nif->get<quint8>( prop, "Texture Clamp Mode" ) );

		if ( hasSF2( ShaderFlags::SLSF2_Effect_Lighting ) )
			lightingInfluence = (float)nif->get<quint8>( prop, "Lighting Influence" ) / 255.0;

		auto startA = nif->get<float>( prop, "Falloff Start Angle" );
		auto stopA = nif->get<float>( prop, "Falloff Stop Angle" );
		auto startO = nif->get<float>( prop, "Falloff Start Opacity" );
		auto stopO = nif->get<float>( prop, "Falloff Stop Opacity" );
		auto soft = nif->get<float>( prop, "Soft Falloff Depth" );

		setFalloff( startA, stopA, startO, stopO, soft );

	} else {

		setEmissive( Color4( m->cBaseColor, m->fAlpha ), m->fBaseColorScale );

		hasSourceTexture = !m->textureList[0].isEmpty();
		hasGreyscaleMap = !m->textureList[1].isEmpty();
		hasEnvMap = !m->textureList[2].isEmpty();
		hasNormalMap = !m->textureList[3].isEmpty();
		hasEnvMask = !m->textureList[4].isEmpty();

		environmentReflection = m->fEnvironmentMappingMaskScale;

		greyscaleAlpha = m->bGrayscaleToPaletteAlpha;
		greyscaleColor = m->bGrayscaleToPaletteColor;
		useFalloff = m->bFalloffEnabled;
		hasRGBFalloff = m->bFalloffColorEnabled;

		depthTest = m->bZBufferTest;
		depthWrite = m->bZBufferWrite;
		isDoubleSided = m->bTwoSided;

		setUvScale( m->fUScale, m->fVScale );
		setUvOffset( m->fUOffset, m->fVOffset );

		if ( m->bTileU && m->bTileV )
			clampMode = TexClampMode::WRAP_S_WRAP_T;
		else if ( m->bTileU )
			clampMode = TexClampMode::WRAP_S_CLAMP_T;
		else if ( m->bTileV )
			clampMode = TexClampMode::CLAMP_S_WRAP_T;
		else
			clampMode = TexClampMode::CLAMP_S_CLAMP_T;

		if ( m->bEffectLightingEnabled )
			lightingInfluence = m->fLightingInfluence;

		setFalloff( m->fFalloffStartAngle, m->fFalloffStopAngle,
						   m->fFalloffStartOpacity, m->fFalloffStopOpacity, m->fSoftDepth );
	}
}

void BSEffectShaderProperty::setController( const NifModel * nif, const QModelIndex & iController )
{
	if ( nif->itemName( iController ) == "BSEffectShaderPropertyFloatController" ) {
		Controller * ctrl = new EffectFloatController( this, iController );
		ctrl->update( nif, iController );
		controllers.append( ctrl );
	} else if ( nif->itemName( iController ) == "BSEffectShaderPropertyColorController" ) {
		Controller * ctrl = new EffectColorController( this, iController );
		ctrl->update( nif, iController );
		controllers.append( ctrl );
	}
}

void BSEffectShaderProperty::setEmissive( const Color4 & color, float mult )
{
	emissiveColor = color;
	emissiveMult = mult;
}

Color4 BSEffectShaderProperty::getEmissiveColor() const
{
	return emissiveColor;
}

float BSEffectShaderProperty::getEmissiveMult() const
{
	return emissiveMult;
}

float BSEffectShaderProperty::getAlpha() const
{
	return emissiveColor.alpha();
}

void BSEffectShaderProperty::setFalloff( float startA, float stopA, float startO, float stopO, float soft )
{
	falloff.startAngle = startA;
	falloff.stopAngle = stopA;
	falloff.startOpacity = startO;
	falloff.stopOpacity = stopO;
	falloff.softDepth = soft;
}

float BSEffectShaderProperty::getEnvironmentReflection() const
{
	return environmentReflection;
}

void BSEffectShaderProperty::setEnvironmentReflection( float strength )
{
	environmentReflection = strength;
}

float BSEffectShaderProperty::getLightingInfluence() const
{
	return lightingInfluence;
}

void BSEffectShaderProperty::setLightingInfluence( float strength )
{
	lightingInfluence = strength;
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

