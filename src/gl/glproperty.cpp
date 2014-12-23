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
//#include "options.h"


#include "controllers.h"
#include "glscene.h"

#include <QOpenGLContext>


//! \file glproperty.cpp Property, subclasses

//! Helper function that checks texture sets
bool checkSet( int s, const QList<QVector<Vector2> > & texcoords )
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
		NifItem * item = static_cast<NifItem *>( index.internalPointer() );

		if ( item )
			qCWarning( nsNif ) << tr( "Unknown property: %1" ).arg( item->name() );
		else
			qCWarning( nsNif ) << tr( "Unknown property: I can't determine its name" );
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

	while ( i != properties.end() && i.key() == p->type() ) {
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
	}
}

void glProperty( AlphaProperty * p )
{
	if ( p && p->alphaBlend && (p->scene->options & Scene::DoBlending) ) {
		glEnable( GL_BLEND );
		glBlendFunc( p->alphaSrc, p->alphaDst );
	} else {
		glDisable( GL_BLEND );
	}

	if ( p && p->alphaTest && (p->scene->options & Scene::DoBlending) ) {
		//glEnable( GL_POLYGON_OFFSET_FILL );
		//glPolygonOffset( -1.0f, -1.0f );
		glDisable( GL_POLYGON_OFFSET_FILL );
		glEnable( GL_ALPHA_TEST );
		glAlphaFunc( p->alphaFunc, p->alphaThreshold );
	} else {
		glDisable( GL_ALPHA_TEST );
		//glDisable( GL_POLYGON_OFFSET_FILL );
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

				if ( nif->checkVersion( 0, 0x14000005 ) ) {
					filterMode = nif->get<int>( iTex, "Filter Mode" );
					clampMode  = nif->get<int>( iTex, "Clamp Mode" );
				} else if ( nif->checkVersion( 0x14010003, 0 ) ) {
					filterMode = ( ( nif->get<ushort>( iTex, "Flags" ) & 0x0F00 ) >> 0x08 );
					clampMode  = ( ( nif->get<ushort>( iTex, "Flags" ) & 0xF000 ) >> 0x0C );
				}

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
					textures[t].tiling = nif->get<Vector2>( iTex, "Tiling" );
					textures[t].rotation = nif->get<float>( iTex, "W Rotation" );
					textures[t].center = nif->get<Vector2>( iTex, "Center Offset" );
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

bool TexturingProperty::bind( int id, const QList<QVector<Vector2> > & texcoords )
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

bool TexturingProperty::bind( int id, const QList<QVector<Vector2> > & texcoords, int stage )
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

bool TextureProperty::bind( const QList<QVector<Vector2> > & texcoords )
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

		shininess = nif->get<float>( index, "Glossiness" );
	}

	// special case to force refresh of materials
	//bool overrideMaterials = Options::overrideMaterials();
	//
	//if ( overridden && !overrideMaterials && iBlock.isValid() ) {
	//	ambient  = Color4( nif->get<Color3>( iBlock, "Ambient Color" ) );
	//	diffuse  = Color4( nif->get<Color3>( iBlock, "Diffuse Color" ) );
	//	specular = Color4( nif->get<Color3>( iBlock, "Specular Color" ) );
	//	emissive = Color4( nif->get<Color3>( iBlock, "Emissive Color" ) );
	//} else if ( overrideMaterials ) {
	//	ambient  = Color4( Options::overrideAmbient() );
	//	diffuse  = Color4( Options::overrideDiffuse() );
	//	specular = Color4( Options::overrideSpecular() );
	//	emissive = Color4( Options::overrideEmissive() );
	//}

	//overridden = overrideMaterials;
}


AlphaController::AlphaController( MaterialProperty * prop, const QModelIndex & index )
	: Controller( index ), target( prop ), lAlpha( 0 )
{
}

void AlphaController::update( float time )
{
	if ( !( active && target ) )
		return;

	interpolate( target->alpha, iData, "Data", ctrlTime( time ), lAlpha );

	if ( target->alpha < 0 )
		target->alpha = 0;

	if ( target->alpha > 1 )
		target->alpha = 1;
}


MaterialColorController::MaterialColorController( MaterialProperty * prop, const QModelIndex & index )
	: Controller( index ), target( prop ), lColor( 0 ), tColor( tAmbient )
{
}

void MaterialColorController::update( float time )
{
	if ( !( active && target ) )
		return;

	Vector3 v3;
	interpolate( v3, iData, "Data", ctrlTime( time ), lColor );

	Color4 color( Color3( v3 ), 1.0 );

	switch ( tColor ) {
	case tAmbient:
		target->ambient = color;
		break;
	case tDiffuse:
		target->diffuse = color;
		break;
	case tSpecular:
		target->specular = color;
		break;
	case tSelfIllum:
		target->emissive = color;
		break;
	}
}

bool MaterialColorController::update( const NifModel * nif, const QModelIndex & index )
{
	if ( Controller::update( nif, index ) ) {
		if ( nif->checkVersion( 0x0A010000, 0 ) ) {
			tColor = nif->get<int>( iBlock, "Target Color" );
		} else {
			tColor = ( ( nif->get<int>( iBlock, "Flags" ) >> 4 ) & 7 );
		}

		return true;
	}

	return false;
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
		vertexmode = nif->get<int>( iBlock, "Vertex Mode" );
		// 0 : source ignore
		// 1 : source emissive
		// 2 : source ambient + diffuse
		lightmode = nif->get<int>( iBlock, "Lighting Mode" );
		// 0 : emissive
		// 1 : emissive + ambient + diffuse
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
	Property::update( nif, block );

	if ( iBlock.isValid() && iBlock == block ) {
		//static const GLenum functions[8] = { GL_NEVER, GL_LESS, GL_EQUAL, GL_LEQUAL, GL_GREATER, GL_NOTEQUAL, GL_GEQUAL, GL_ALWAYS };
		//static const GLenum operations[8] = { GL_KEEP, GL_ZERO, GL_REPLACE, GL_INCR, GL_DECR, GL_INVERT, GL_KEEP, GL_KEEP };

		// ! glFrontFace( GL_CCW )
		if ( nif->checkVersion( 0, 0x14000005 ) ) {
			switch ( nif->get<int>( iBlock, "Draw Mode" ) ) {
			case 2:
				cullEnable = true;
				cullMode = GL_FRONT;
				break;
			case 3:
				cullEnable = false;
				cullMode = GL_BACK;
				break;
			case 1:
			default:
				cullEnable = true;
				cullMode = GL_BACK;
				break;
			}
		} else {
			switch ( ( nif->get<int>( iBlock, "Flags" ) >> 10 ) & 3 ) {
			case 2:
				cullEnable = true;
				cullMode = GL_FRONT;
				break;
			case 3:
				cullEnable = false;
				cullMode = GL_BACK;
				break;
			case 1:
			default:
				cullEnable = true;
				cullMode = GL_BACK;
				break;
			}
		}
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
	} else {
		glEnable( GL_CULL_FACE );
		glCullFace( GL_BACK );
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
	}
}

void glProperty( BSShaderLightingProperty * p )
{
	if ( p && (p->scene->options & Scene::DoTexturing) && p->bind( 0 ) ) {
		glEnable( GL_TEXTURE_2D );
	}
}

bool BSShaderLightingProperty::bind( int id, const QString & fname )
{
	GLuint mipmaps = 0;

	if ( !fname.isEmpty() )
		mipmaps = scene->bindTexture( fname );
	else
		mipmaps = scene->bindTexture( this->fileName( id ) );

	if ( mipmaps == 0 )
		return false;

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

bool BSShaderLightingProperty::bind( int id, const QList<QVector<Vector2> > & texcoords )
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

QString BSShaderLightingProperty::fileName( int id ) const
{
	const NifModel * nif = qobject_cast<const NifModel *>( iTextureSet.model() );

	if ( nif && iTextureSet.isValid() ) {
		int nTextures = nif->get<int>( iTextureSet, "Num Textures" );
		QModelIndex iTextures = nif->getIndex( iTextureSet, "Textures" );

		if ( id >= 0 && id < nTextures )
			return nif->get<QString>( iTextures.child( id, 0 ) );
	} else {
		// handle niobject name="BSEffectShaderProperty...
		nif = qobject_cast<const NifModel *>(iSourceTexture.model());
		if ( id == 0 ) {
			if ( nif && iSourceTexture.isValid() )
				return nif->get<QString>( iSourceTexture, "Source Texture" );
		}

		if ( id == 1 ) {
			if ( nif && iSourceTexture.isValid() )
				return nif->get<QString>( iSourceTexture, "Greyscale Texture" );
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

unsigned int BSShaderLightingProperty::getFlags1()
{
	return (unsigned int)flags1;
}

unsigned int BSShaderLightingProperty::getFlags2()
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

UVScale BSShaderLightingProperty::getUvScale()
{
	return uvScale;
}

UVOffset BSShaderLightingProperty::getUvOffset()
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

/*
	BSLightingShaderProperty
*/

void BSLightingShaderProperty::setShaderType( unsigned int t )
{
	shaderType = ShaderType( t );
}

BSLightingShaderProperty::ShaderType BSLightingShaderProperty::getShaderType()
{
	return shaderType;
}

void BSLightingShaderProperty::setEmissive( Color3 color, float mult )
{
	emissiveColor = color;
	emissiveMult = mult;
}

void BSLightingShaderProperty::setSpecular( Color3 color, float gloss, float strength )
{
	specularColor = color;
	specularGloss = gloss;
	specularStrength = strength;
}

Color3 BSLightingShaderProperty::getEmissiveColor()
{
	return emissiveColor;
}

Color3 BSLightingShaderProperty::getSpecularColor()
{
	return specularColor;
}

float BSLightingShaderProperty::getEmissiveMult()
{
	return emissiveMult;
}

float BSLightingShaderProperty::getLightingEffect1()
{
	return lightingEffect1;
}

float BSLightingShaderProperty::getLightingEffect2()
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

float BSLightingShaderProperty::getSpecularGloss()
{
	return specularGloss;
}

float BSLightingShaderProperty::getSpecularStrength()
{
	return specularStrength;
}

/*
	BSEffectShaderProperty
*/


void BSEffectShaderProperty::setEmissive( Color4 color, float mult )
{
	emissiveColor = color;
	emissiveMult = mult;
}

Color4 BSEffectShaderProperty::getEmissiveColor()
{
	return emissiveColor;
}

float BSEffectShaderProperty::getEmissiveMult()
{
	return emissiveMult;
}

void BSEffectShaderProperty::setFalloff( float startA, float stopA, float startO, float stopO, float soft )
{
	falloff.startAngle = startA;
	falloff.stopAngle = stopA;
	falloff.startOpacity = startO;
	falloff.stopOpacity = stopO;
	falloff.softDepth = soft;
}


/*
	BSWaterShaderProperty
*/

unsigned int BSWaterShaderProperty::getWaterShaderFlags()
{
	return (unsigned int)waterShaderFlags;
}

void BSWaterShaderProperty::setWaterShaderFlags( unsigned int val )
{
	waterShaderFlags = WaterShaderFlags::SF1( val );
}

