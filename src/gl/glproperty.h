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

#ifndef GLPROPERTY_H
#define GLPROPERTY_H

#include "glcontrolable.h" // Inherited

#include <QHash>
#include <QPersistentModelIndex>
#include <QString>


typedef unsigned int GLenum;
typedef int GLint;
typedef unsigned int GLuint;
typedef float GLfloat;

//! \file glproperty.h Property classes

//! Controllable properties attached to nodes and meshes
class Property : public Controllable
{
protected:
	//! Protected constructor; see Controllable()
	Property( Scene * scene, const QModelIndex & index ) : Controllable( scene, index ), ref( 0 ) {}

	int ref;

	//! List of properties
	friend class PropertyList;

public:
	//! Creates a Property based on the specified index of the specified model
	/*!
	 * @param scene The Scene the property is in
	 * @param nif The model
	 * @param index The index NiProperty
	 */
	static Property * create( Scene * scene, const NifModel * nif, const QModelIndex & index );

	enum Type
	{
		Alpha, ZBuffer, Material, Texturing, Texture, Specular, Wireframe, VertexColor, Stencil, ShaderLighting
	};

	virtual Type type() const = 0;
	virtual QString typeId() const = 0;

	template <typename T> static Type _type();
	template <typename T> T * cast()
	{
		if ( type() == _type<T>() )
			return static_cast<T *>( this );

		return 0;
	}
};

//! Associate a Property subclass with a Property::Type
#define REGISTER_PROPERTY( CLASSNAME, TYPENAME ) template <> inline Property::Type Property::_type<CLASSNAME>() { return Property::TYPENAME; }

//! A list of \link Property Properties \endlink
class PropertyList final
{
public:
	PropertyList();
	PropertyList( const PropertyList & other );
	~PropertyList();

	void add( Property * );
	void del( Property * );
	bool contains( Property * ) const;

	Property * get( const QModelIndex & idx ) const;

	template <typename T> T * get() const;
	template <typename T> bool contains() const;

	void validate();

	void clear();

	PropertyList & operator=( const PropertyList & other );

	QList<Property *> list() const { return properties.values(); }

	void merge( const PropertyList & list );

protected:
	QMultiHash<Property::Type, Property *> properties;
};

template <typename T> inline T * PropertyList::get() const
{
	Property * p = properties.value( Property::_type<T>() );

	if ( p )
		return p->cast<T>();

	return 0;
}

template <typename T> inline bool PropertyList::contains() const
{
	return properties.contains( Property::_type<T>() );
}

//! A Property that specifies alpha blending
class AlphaProperty final : public Property
{
public:
	AlphaProperty( Scene * scene, const QModelIndex & index ) : Property( scene, index ) {}

	Type type() const override final { return Alpha; }
	QString typeId() const override final { return "NiAlphaProperty"; }

	void update( const NifModel * nif, const QModelIndex & block );

	bool blend() const { return alphaBlend; }
	bool test() const { return alphaTest; }
	bool sort() const { return alphaSort; }

	friend void glProperty( AlphaProperty * );

protected:
	bool alphaBlend, alphaTest, alphaSort;
	GLenum alphaSrc, alphaDst, alphaFunc;
	GLfloat alphaThreshold;
};

REGISTER_PROPERTY( AlphaProperty, Alpha )

//! A Property that specifies depth testing
class ZBufferProperty final : public Property
{
public:
	ZBufferProperty( Scene * scene, const QModelIndex & index ) : Property( scene, index ) {}

	Type type() const override final { return ZBuffer; }
	QString typeId() const override final { return "NiZBufferProperty"; }

	void update( const NifModel * nif, const QModelIndex & block );

	bool test() const { return depthTest; }
	bool mask() const { return depthMask; }

	friend void glProperty( ZBufferProperty * );

protected:
	bool depthTest;
	bool depthMask;
	GLenum depthFunc;
};

REGISTER_PROPERTY( ZBufferProperty, ZBuffer )

//! Number of textures; base + dark + detail + gloss + glow + bump + 4 decals
#define numTextures 10

//! A Property that specifies (multi-)texturing
class TexturingProperty final : public Property
{
	//! The properties of each texture slot
	struct TexDesc
	{
		QPersistentModelIndex iSource;
		GLenum filter;
		GLint wrapS, wrapT;
		int coordset;

		bool hasTransform;

		Vector2 translation;
		Vector2 tiling;
		float rotation;
		Vector2 center;
	};

public:
	TexturingProperty( Scene * scene, const QModelIndex & index ) : Property( scene, index ) {}

	Type type() const override final { return Texturing; }
	QString typeId() const override final { return "NiTexturingProperty"; }

	void update( const NifModel * nif, const QModelIndex & block );

	friend void glProperty( TexturingProperty * );

	bool bind( int id, const QString & fname = QString() );

	bool bind( int id, const QList<QVector<Vector2> > & texcoords );
	bool bind( int id, const QList<QVector<Vector2> > & texcoords, int stage );

	QString fileName( int id ) const;
	int coordSet( int id ) const;

	static int getId( const QString & id );

protected:
	TexDesc textures[numTextures];

	void setController( const NifModel * nif, const QModelIndex & controller );

	friend class TexFlipController;
	friend class TexTransController;
};

REGISTER_PROPERTY( TexturingProperty, Texturing )

//! A Property that specifies a texture
class TextureProperty final : public Property
{
public:
	TextureProperty( Scene * scene, const QModelIndex & index ) : Property( scene, index ) {}

	Type type() const override final { return Texture; }
	QString typeId() const override final { return "NiTextureProperty"; }

	void update( const NifModel * nif, const QModelIndex & block );

	friend void glProperty( TextureProperty * );

	bool bind();
	bool bind( const QList<QVector<Vector2> > & texcoords );

	QString fileName() const;

protected:
	QPersistentModelIndex iImage;

	void setController( const NifModel * nif, const QModelIndex & controller );

	friend class TexFlipController;
};

REGISTER_PROPERTY( TextureProperty, Texture )

//! A Property that specifies a material
class MaterialProperty final : public Property
{
public:
	MaterialProperty( Scene * scene, const QModelIndex & index ) : Property( scene, index ) {}

	Type type() const override final { return Material; }
	QString typeId() const override final { return "NiMaterialProperty"; }

	void update( const NifModel * nif, const QModelIndex & block );

	friend void glProperty( class MaterialProperty *, class SpecularProperty * );

	GLfloat alphaValue() const { return alpha; }

protected:
	Color4 ambient, diffuse, specular, emissive;
	GLfloat shininess, alpha;
	bool overridden;

	void setController( const NifModel * nif, const QModelIndex & controller );

	friend class AlphaController;
	friend class MaterialColorController;
};

REGISTER_PROPERTY( MaterialProperty, Material )

//! A Property that specifies specularity
class SpecularProperty final : public Property
{
public:
	SpecularProperty( Scene * scene, const QModelIndex & index ) : Property( scene, index ) {}

	Type type() const override final { return Specular; }
	QString typeId() const override final { return "NiSpecularProperty"; }

	void update( const NifModel * nif, const QModelIndex & index );

	friend void glProperty( class MaterialProperty *, class SpecularProperty * );

protected:
	bool spec;
};

REGISTER_PROPERTY( SpecularProperty, Specular )

//! A Property that specifies wireframe drawing
class WireframeProperty final : public Property
{
public:
	WireframeProperty( Scene * scene, const QModelIndex & index ) : Property( scene, index ) {}

	Type type() const override final { return Wireframe; }
	QString typeId() const override final { return "NiWireframeProperty"; }

	void update( const NifModel * nif, const QModelIndex & index );

	friend void glProperty( WireframeProperty * );

protected:
	bool wire;
};

REGISTER_PROPERTY( WireframeProperty, Wireframe )

//! A Property that specifies vertex color handling
class VertexColorProperty final : public Property
{
public:
	VertexColorProperty( Scene * scene, const QModelIndex & index ) : Property( scene, index ) {}

	Type type() const override final { return VertexColor; }
	QString typeId() const override final { return "NiVertexColorProperty"; }

	void update( const NifModel * nif, const QModelIndex & index );

	friend void glProperty( VertexColorProperty *, bool vertexcolors );

protected:
	int lightmode;
	int vertexmode;
};

REGISTER_PROPERTY( VertexColorProperty, VertexColor )

//! A Property that specifies stencil testing
class StencilProperty final : public Property
{
public:
	StencilProperty( Scene * scene, const QModelIndex & index ) : Property( scene, index ) {}

	Type type() const override final { return Stencil; }
	QString typeId() const override final { return "NiStencilProperty"; }

	void update( const NifModel * nif, const QModelIndex & index );

	friend void glProperty( StencilProperty * );

protected:
	bool stencil;

	GLenum func;
	GLint ref;
	GLuint mask;

	GLenum failop;
	GLenum zfailop;
	GLenum zpassop;

	bool cullEnable;
	GLenum cullMode;
};

REGISTER_PROPERTY( StencilProperty, Stencil )

//! A Property that specifies shader lighting (Bethesda-specific)
class BSShaderLightingProperty final : public Property
{
public:
	BSShaderLightingProperty( Scene * scene, const QModelIndex & index ) : Property( scene, index ) {}

	Type type() const override final { return ShaderLighting; }
	QString typeId() const override final { return "BSShaderLightingProperty"; }

	void update( const NifModel * nif, const QModelIndex & block );

	friend void glProperty( BSShaderLightingProperty * );

	bool bind( int id, const QString & fname = QString() );
	bool bind( int id, const QList<QVector<Vector2> > & texcoords );
	bool bind( int id, const QList<QVector<Vector2> > & texcoords, int stage );

	QString fileName( int id ) const;
	//int coordSet( int id ) const;

	static int getId( const QString & id );

	enum SF1 : unsigned int
	{
		SLSF1_Specular					  = 1,	  // 0!
		SLSF1_Skinned					  = 1<<1 ,  // 1
		SLSF1_Temp_Refraction			  = 1<<2 ,  // 2
		SLSF1_Vertex_Alpha				  = 1<<3 ,  // 3
		SLSF1_Greyscale_To_PaletteColor   = 1<<4 ,  // 4
		SLSF1_Greyscale_To_PaletteAlpha   = 1<<5 ,  // 5
		SLSF1_Use_Falloff				  = 1<<6 ,  // 6
		SLSF1_Environment_Mapping		  = 1<<7 ,  // 7
		SLSF1_Recieve_Shadows			  = 1<<8 ,  // 8
		SLSF1_Cast_Shadows				  = 1<<9 ,  // 9
		SLSF1_Facegen_Detail_Map		  = 1<<10,  // 10
		SLSF1_Parallax					  = 1<<11,  // 11
		SLSF1_Model_Space_Normals		  = 1<<12,  // 12
		SLSF1_Non_Projective_Shadows	  = 1<<13,  // 13
		SLSF1_Landscape					  = 1<<14,  // 14
		SLSF1_Refraction				  = 1<<15,  // 15!
		SLSF1_Fire_Refraction			  = 1<<16,  // 16
		SLSF1_Eye_Environment_Mapping	  = 1<<17,  // 17
		SLSF1_Hair_Soft_Lighting		  = 1<<18,  // 18
		SLSF1_Screendoor_Alpha_Fade		  = 1<<19,  // 19
		SLSF1_Localmap_Hide_Secret		  = 1<<20,  // 20
		SLSF1_FaceGen_RGB_Tint			  = 1<<21,  // 21
		SLSF1_Own_Emit					  = 1<<22,  // 22
		SLSF1_Projected_UV				  = 1<<23,  // 23
		SLSF1_Multiple_Textures			  = 1<<24,  // 24
		SLSF1_Remappable_Textures		  = 1<<25,  // 25
		SLSF1_Decal						  = 1<<26,  // 26
		SLSF1_Dynamic_Decal				  = 1<<27,  // 27
		SLSF1_Parallax_Occlusion		  = 1<<28,  // 28
		SLSF1_External_Emittance		  = 1<<29,  // 29
		SLSF1_Soft_Effect				  = 1<<30,  // 30
		SLSF1_ZBuffer_Test				  = 1<<31,  // 31
	};

	SF1 flags1; // = SF1( 0 | (1 << 7) | (1 << 8) | (1 << 21) | (1 << 30) );

	enum SF2 : unsigned int
	{
		SLSF2_ZBuffer_Write					= 1,	  // 0!
		SLSF2_LOD_Landscape					= 1<<1 ,  // 1
		SLSF2_LOD_Objects					= 1<<2 ,  // 2
		SLSF2_No_Fade						= 1<<3 ,  // 3
		SLSF2_Double_Sided					= 1<<4 ,  // 4
		SLSF2_Vertex_Colors					= 1<<5 ,  // 5
		SLSF2_Glow_Map						= 1<<6 ,  // 6
		SLSF2_Assume_Shadowmask				= 1<<7 ,  // 7
		SLSF2_Packed_Tangent				= 1<<8 ,  // 8
		SLSF2_Multi_Index_Snow				= 1<<9 ,  // 9
		SLSF2_Vertex_Lighting				= 1<<10,  // 10
		SLSF2_Uniform_Scale					= 1<<11,  // 11
		SLSF2_Fit_Slope						= 1<<12,  // 12
		SLSF2_Billboard						= 1<<13,  // 13
		SLSF2_No_LOD_Land_Blend				= 1<<14,  // 14
		SLSF2_EnvMap_Light_Fade				= 1<<15,  // 15!
		SLSF2_Wireframe						= 1<<16,  // 16
		SLSF2_Weapon_Blood					= 1<<17,  // 17
		SLSF2_Hide_On_Local_Map				= 1<<18,  // 18
		SLSF2_Premult_Alpha					= 1<<19,  // 19
		SLSF2_Cloud_LOD						= 1<<20,  // 20
		SLSF2_Anisotropic_Lighting			= 1<<21,  // 21
		SLSF2_No_Transparency_Multisampling	= 1<<22,  // 22
		SLSF2_Unused01						= 1<<23,  // 23
		SLSF2_Multi_Layer_Parallax			= 1<<24,  // 24
		SLSF2_Soft_Lighting					= 1<<25,  // 25
		SLSF2_Rim_Lighting					= 1<<26,  // 26
		SLSF2_Back_Lighting					= 1<<27,  // 27
		SLSF2_Unused02						= 1<<28,  // 28
		SLSF2_Tree_Anim						= 1<<29,  // 29
		SLSF2_Effect_Lighting				= 1<<30,  // 30
		SLSF2_HD_LOD_Objects				= 1<<31,  // 31
	};

	SF2 flags2; // = SF2( 0 | (1 << 14) );

protected:
	//QVector<QString> textures;
	QPersistentModelIndex iTextureSet;
	QPersistentModelIndex iSourceTexture;
};

REGISTER_PROPERTY( BSShaderLightingProperty, ShaderLighting )


#endif
