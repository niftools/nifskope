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

#ifndef GLPROPERTY_H
#define GLPROPERTY_H

#include "icontrollable.h" // Inherited
#include "data/niftypes.h"

#include <QHash>
#include <QPersistentModelIndex>
#include <QString>


//! @file glproperty.h Property, PropertyList

typedef unsigned int GLenum;
typedef int GLint;
typedef unsigned int GLuint;
typedef float GLfloat;


class Material;
class NifModel;

//! Controllable properties attached to nodes and meshes
class Property : public IControllable
{
	friend class PropertyList;

protected:
	//! Protected constructor; see IControllable()
	Property( Scene * scene, const QModelIndex & index ) : IControllable( scene, index ) {}

	int ref = 0;

public:
	/*! Creates a Property based on the specified index of the specified model
	 *
	 * @param scene The Scene the property is in
	 * @param nif	The model
	 * @param index The index NiProperty
	 */
	static Property * create( Scene * scene, const NifModel * nif, const QModelIndex & index );

	enum Type
	{
		Alpha, ZBuffer, MaterialProp, Texturing, Texture, Specular, Wireframe, VertexColor, Stencil, ShaderLighting
	};

	virtual Type type() const = 0;
	virtual QString typeId() const = 0;

	template <typename T> static Type _type();
	template <typename T> T * cast()
	{
		if ( type() == _type<T>() )
			return static_cast<T *>( this );

		return nullptr;
	}
};

//! Associate a Property subclass with a Property::Type
#define REGISTER_PROPERTY( CLASSNAME, TYPENAME ) template <> inline Property::Type Property::_type<CLASSNAME>() { return Property::TYPENAME; }

//! A list of [Properties](@ref Property)
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

	return nullptr;
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

	bool hasAlphaBlend() const { return alphaBlend; }
	bool hasAlphaTest() const { return alphaTest; }

	GLfloat alphaThreshold = 0;

	friend void glProperty( AlphaProperty * );

protected:
	void setController( const NifModel * nif, const QModelIndex & controller ) override final;
	void updateImpl( const NifModel * nif, const QModelIndex & block ) override final;

	bool alphaBlend = false, alphaTest = false, alphaSort = false;
	GLenum alphaSrc = 0, alphaDst = 0, alphaFunc = 0;
};

REGISTER_PROPERTY( AlphaProperty, Alpha )

//! A Property that specifies depth testing
class ZBufferProperty final : public Property
{
public:
	ZBufferProperty( Scene * scene, const QModelIndex & index ) : Property( scene, index ) {}

	Type type() const override final { return ZBuffer; }
	QString typeId() const override final { return "NiZBufferProperty"; }

	bool test() const { return depthTest; }
	bool mask() const { return depthMask; }

	friend void glProperty( ZBufferProperty * );

protected:
	bool depthTest = false;
	bool depthMask = false;
	GLenum depthFunc = 0;

	void updateImpl( const NifModel * nif, const QModelIndex & block ) override final;
};

REGISTER_PROPERTY( ZBufferProperty, ZBuffer )

//! Number of textures; base + dark + detail + gloss + glow + bump + 4 decals
#define numTextures 10

//! A Property that specifies (multi-)texturing
class TexturingProperty final : public Property
{
	friend class TexFlipController;
	friend class TexTransController;

	//! The properties of each texture slot
	struct TexDesc
	{
		QPersistentModelIndex iSource;
		GLenum filter = 0;
		GLint wrapS = 0, wrapT = 0;
		int coordset = 0;
		float maxAniso = 1.0;

		bool hasTransform = false;

		Vector2 translation;
		Vector2 tiling;
		float rotation = 0;
		Vector2 center;
	};

public:
	TexturingProperty( Scene * scene, const QModelIndex & index ) : Property( scene, index ) {}

	Type type() const override final { return Texturing; }
	QString typeId() const override final { return "NiTexturingProperty"; }

	friend void glProperty( TexturingProperty * );

	bool bind( int id, const QString & fname = QString() );

	bool bind( int id, const QVector<QVector<Vector2> > & texcoords );
	bool bind( int id, const QVector<QVector<Vector2> > & texcoords, int stage );

	QString fileName( int id ) const;
	int coordSet( int id ) const;

	static int getId( const QString & id );

protected:
	TexDesc textures[numTextures];

	void setController( const NifModel * nif, const QModelIndex & controller ) override final;
	void updateImpl( const NifModel * nif, const QModelIndex & block ) override final;
};

REGISTER_PROPERTY( TexturingProperty, Texturing )

//! A Property that specifies a texture
class TextureProperty final : public Property
{
	friend class TexFlipController;

public:
	TextureProperty( Scene * scene, const QModelIndex & index ) : Property( scene, index ) {}

	Type type() const override final { return Texture; }
	QString typeId() const override final { return "NiTextureProperty"; }

	friend void glProperty( TextureProperty * );

	bool bind();
	bool bind( const QVector<QVector<Vector2> > & texcoords );

	QString fileName() const;

protected:
	QPersistentModelIndex iImage;

	void setController( const NifModel * nif, const QModelIndex & controller ) override final;
	void updateImpl( const NifModel * nif, const QModelIndex & block ) override final;
};

REGISTER_PROPERTY( TextureProperty, Texture )

//! A Property that specifies a material
class MaterialProperty final : public Property
{
	friend class AlphaController;
	friend class MaterialColorController;

public:
	MaterialProperty( Scene * scene, const QModelIndex & index ) : Property( scene, index ) {}

	Type type() const override final { return MaterialProp; }
	QString typeId() const override final { return "NiMaterialProperty"; }

	friend void glProperty( class MaterialProperty *, class SpecularProperty * );

	GLfloat alphaValue() const { return alpha; }

protected:
	Color4 ambient, diffuse, specular, emissive;
	GLfloat shininess = 0, alpha = 0;

	void setController( const NifModel * nif, const QModelIndex & controller ) override final;
	void updateImpl( const NifModel * nif, const QModelIndex & block ) override final;
};

REGISTER_PROPERTY( MaterialProperty, MaterialProp )

//! A Property that specifies specularity
class SpecularProperty final : public Property
{
public:
	SpecularProperty( Scene * scene, const QModelIndex & index ) : Property( scene, index ) {}

	Type type() const override final { return Specular; }
	QString typeId() const override final { return "NiSpecularProperty"; }

	friend void glProperty( class MaterialProperty *, class SpecularProperty * );

protected:
	bool spec = false;

	void updateImpl( const NifModel * nif, const QModelIndex & index ) override final;
};

REGISTER_PROPERTY( SpecularProperty, Specular )

//! A Property that specifies wireframe drawing
class WireframeProperty final : public Property
{
public:
	WireframeProperty( Scene * scene, const QModelIndex & index ) : Property( scene, index ) {}

	Type type() const override final { return Wireframe; }
	QString typeId() const override final { return "NiWireframeProperty"; }

	friend void glProperty( WireframeProperty * );

protected:
	bool wire = false;

	void updateImpl( const NifModel * nif, const QModelIndex & index ) override final;
};

REGISTER_PROPERTY( WireframeProperty, Wireframe )

//! A Property that specifies vertex color handling
class VertexColorProperty final : public Property
{
public:
	VertexColorProperty( Scene * scene, const QModelIndex & index ) : Property( scene, index ) {}

	Type type() const override final { return VertexColor; }
	QString typeId() const override final { return "NiVertexColorProperty"; }

	friend void glProperty( VertexColorProperty *, bool vertexcolors );

protected:
	int lightmode = 0;
	int vertexmode = 0;

	void updateImpl( const NifModel * nif, const QModelIndex & index ) override final;
};

REGISTER_PROPERTY( VertexColorProperty, VertexColor )

namespace Stencil
{
	enum TestFunc
	{
		TEST_NEVER,
		TEST_LESS,
		TEST_EQUAL,
		TEST_LESSEQUAL,
		TEST_GREATER,
		TEST_NOTEQUAL,
		TEST_GREATEREQUAL,
		TEST_ALWAYS,
		TEST_MAX
	};

	enum Action
	{
		ACTION_KEEP,
		ACTION_ZERO,
		ACTION_REPLACE,
		ACTION_INCREMENT,
		ACTION_DECREMENT,
		ACTION_INVERT,
		ACTION_MAX
	};

	enum DrawMode
	{
		DRAW_CCW_OR_BOTH,
		DRAW_CCW,
		DRAW_CW,
		DRAW_BOTH,
		DRAW_MAX
	};
}

//! A Property that specifies stencil testing
class StencilProperty final : public Property
{
public:
	StencilProperty( Scene * scene, const QModelIndex & index ) : Property( scene, index ) {}

	Type type() const override final { return Stencil; }
	QString typeId() const override final { return "NiStencilProperty"; }

	friend void glProperty( StencilProperty * );

protected:
	enum
	{
		ENABLE_MASK = 0x0001,
		FAIL_MASK = 0x000E,
		FAIL_POS = 1,
		ZFAIL_MASK = 0x0070,
		ZFAIL_POS = 4,
		ZPASS_MASK = 0x0380,
		ZPASS_POS = 7,
		DRAW_MASK = 0x0C00,
		DRAW_POS = 10,
		TEST_MASK = 0x7000,
		TEST_POS = 12
	};

	bool stencil = false;

	GLenum func = 0;
	GLuint ref = 0;
	GLuint mask = 0xffffffff;

	GLenum failop = 0;
	GLenum zfailop = 0;
	GLenum zpassop = 0;

	bool cullEnable = false;
	GLenum cullMode = 0;

	void updateImpl( const NifModel * nif, const QModelIndex & index ) override final;
};

REGISTER_PROPERTY( StencilProperty, Stencil )


namespace ShaderFlags
{
	enum SF1 : unsigned int
	{
		SLSF1_Specular = 1,	    // 0!
		SLSF1_Skinned = 1 << 1,  // 1
		SLSF1_Temp_Refraction = 1 << 2,  // 2
		SLSF1_Vertex_Alpha = 1 << 3,  // 3
		SLSF1_Greyscale_To_PaletteColor = 1 << 4,  // 4
		SLSF1_Greyscale_To_PaletteAlpha = 1 << 5,  // 5
		SLSF1_Use_Falloff = 1 << 6,  // 6
		SLSF1_Environment_Mapping = 1 << 7,  // 7
		SLSF1_Recieve_Shadows = 1 << 8,  // 8
		SLSF1_Cast_Shadows = 1 << 9,  // 9
		SLSF1_Facegen_Detail_Map = 1 << 10,  // 10
		SLSF1_Parallax = 1 << 11,  // 11
		SLSF1_Model_Space_Normals = 1 << 12,  // 12
		SLSF1_Non_Projective_Shadows = 1 << 13,  // 13
		SLSF1_Landscape = 1 << 14,  // 14
		SLSF1_Refraction = 1 << 15,  // 15!
		SLSF1_Fire_Refraction = 1 << 16,  // 16
		SLSF1_Eye_Environment_Mapping = 1 << 17,  // 17
		SLSF1_Hair_Soft_Lighting = 1 << 18,  // 18
		SLSF1_Screendoor_Alpha_Fade = 1 << 19,  // 19
		SLSF1_Localmap_Hide_Secret = 1 << 20,  // 20
		SLSF1_FaceGen_RGB_Tint = 1 << 21,  // 21
		SLSF1_Own_Emit = 1 << 22,  // 22
		SLSF1_Projected_UV = 1 << 23,  // 23
		SLSF1_Multiple_Textures = 1 << 24,  // 24
		SLSF1_Remappable_Textures = 1 << 25,  // 25
		SLSF1_Decal = 1 << 26,  // 26
		SLSF1_Dynamic_Decal = 1 << 27,  // 27
		SLSF1_Parallax_Occlusion = 1 << 28,  // 28
		SLSF1_External_Emittance = 1 << 29,  // 29
		SLSF1_Soft_Effect = 1 << 30,  // 30
		SLSF1_ZBuffer_Test = (unsigned int)(1 << 31),  // 31
	};

	enum SF2 : unsigned int
	{
		SLSF2_ZBuffer_Write = 1,	  // 0!
		SLSF2_LOD_Landscape = 1 << 1,  // 1
		SLSF2_LOD_Objects = 1 << 2,  // 2
		SLSF2_No_Fade = 1 << 3,  // 3
		SLSF2_Double_Sided = 1 << 4,  // 4
		SLSF2_Vertex_Colors = 1 << 5,  // 5
		SLSF2_Glow_Map = 1 << 6,  // 6
		SLSF2_Assume_Shadowmask = 1 << 7,  // 7
		SLSF2_Packed_Tangent = 1 << 8,  // 8
		SLSF2_Multi_Index_Snow = 1 << 9,  // 9
		SLSF2_Vertex_Lighting = 1 << 10,  // 10
		SLSF2_Uniform_Scale = 1 << 11,  // 11
		SLSF2_Fit_Slope = 1 << 12,  // 12
		SLSF2_Billboard = 1 << 13,  // 13
		SLSF2_No_LOD_Land_Blend = 1 << 14,  // 14
		SLSF2_EnvMap_Light_Fade = 1 << 15,  // 15!
		SLSF2_Wireframe = 1 << 16,  // 16
		SLSF2_Weapon_Blood = 1 << 17,  // 17
		SLSF2_Hide_On_Local_Map = 1 << 18,  // 18
		SLSF2_Premult_Alpha = 1 << 19,  // 19
		SLSF2_Cloud_LOD = 1 << 20,  // 20
		SLSF2_Anisotropic_Lighting = 1 << 21,  // 21
		SLSF2_No_Transparency_Multisampling = 1 << 22,  // 22
		SLSF2_Unused01 = 1 << 23,  // 23
		SLSF2_Multi_Layer_Parallax = 1 << 24,  // 24
		SLSF2_Soft_Lighting = 1 << 25,  // 25
		SLSF2_Rim_Lighting = 1 << 26,  // 26
		SLSF2_Back_Lighting = 1 << 27,  // 27
		SLSF2_Unused02 = 1 << 28,  // 28
		SLSF2_Tree_Anim = 1 << 29,  // 29
		SLSF2_Effect_Lighting = 1 << 30,  // 30
		SLSF2_HD_LOD_Objects = (unsigned int)(1 << 31),  // 31
	};

	enum ShaderType : unsigned int
	{
		ST_Default,
		ST_EnvironmentMap,
		ST_GlowShader,
		ST_Heightmap,
		ST_FaceTint,
		ST_SkinTint,
		ST_HairTint,
		ST_ParallaxOccMaterial,
		ST_WorldMultitexture,
		ST_WorldMap1,
		ST_Unknown10,
		ST_MultiLayerParallax,
		ST_Unknown12,
		ST_WorldMap2,
		ST_SparkleSnow,
		ST_WorldMap3,
		ST_EyeEnvmap,
		ST_Unknown17,
		ST_WorldMap4,
		ST_WorldLODMultitexture
	};

	enum BSShaderCRC32 : unsigned int
	{
		// Lighting + Effect
		// SF1
		ZBUFFER_TEST = 1740048692,
		SKINNED = 3744563888,
		ENVMAP = 2893749418,
		VERTEX_ALPHA = 2333069810,
		GRAYSCALE_TO_PALETTE_COLOR = 442246519,
		DECAL = 3849131744,
		DYNAMIC_DECAL = 1576614759,
		HAIRTINT = 1264105798,
		SKIN_TINT = 1483897208,
		EMIT_ENABLED = 2262553490,
		REFRACTION = 1957349758,
		REFRACTION_FALLOFF = 902349195,
		RGB_FALLOFF = 3448946507,
		EXTERNAL_EMITTANCE = 2150459555,
		// SF2
		ZBUFFER_WRITE = 3166356979,
		GLOWMAP = 2399422528,
		TWO_SIDED = 759557230,
		VERTEXCOLORS = 348504749,
		NOFADE = 2994043788,
		LOD_OBJECTS = 2896726515,
		// Lighting only
		PBR = 731263983,
		FACE = 314919375,
		CAST_SHADOWS = 1563274220,
		MODELSPACENORMALS = 2548465567,
		TRANSFORM_CHANGED = 3196772338,
		INVERTED_FADE_PATTERN = 3030867718,
		// Effect only
		EFFECT_LIGHTING = 3473438218,
		FALLOFF = 3980660124,
		SOFT_EFFECT = 3503164976,
		GRAYSCALE_TO_PALETTE_ALPHA = 2901038324,
		WEAPON_BLOOD = 2078326675,
		NO_EXPOSURE = 3707406987
	};

	static QMap<uint, uint64_t> CRC_TO_FLAG = {
		// SF1
		{CAST_SHADOWS, SLSF1_Cast_Shadows},
		{ZBUFFER_TEST, SLSF1_ZBuffer_Test},
		{SKINNED, SLSF1_Skinned},
		{ENVMAP, SLSF1_Environment_Mapping},
		{VERTEX_ALPHA, SLSF1_Vertex_Alpha},
		{FACE, SLSF1_Facegen_Detail_Map},
		{GRAYSCALE_TO_PALETTE_COLOR, SLSF1_Greyscale_To_PaletteColor},
		{GRAYSCALE_TO_PALETTE_ALPHA, SLSF1_Greyscale_To_PaletteAlpha},
		{DECAL, SLSF1_Decal},
		{DYNAMIC_DECAL, SLSF1_Dynamic_Decal},
		{EMIT_ENABLED, SLSF1_Own_Emit},
		{REFRACTION, SLSF1_Refraction},
		{SKIN_TINT, SLSF1_FaceGen_RGB_Tint},
		{RGB_FALLOFF, SLSF1_Recieve_Shadows},
		{EXTERNAL_EMITTANCE, SLSF1_External_Emittance},
		{MODELSPACENORMALS, SLSF1_Model_Space_Normals},
		{FALLOFF, SLSF1_Use_Falloff},
		{SOFT_EFFECT, SLSF1_Soft_Effect},
		// SF2
		{ZBUFFER_WRITE, (uint64_t)SLSF2_ZBuffer_Write << 32},
		{GLOWMAP, (uint64_t)SLSF2_Glow_Map << 32},
		{TWO_SIDED, (uint64_t)SLSF2_Double_Sided << 32},
		{VERTEXCOLORS, (uint64_t)SLSF2_Vertex_Colors << 32},
		{NOFADE, (uint64_t)SLSF2_No_Fade << 32},
		{WEAPON_BLOOD, (uint64_t)SLSF2_Weapon_Blood << 32},
		{TRANSFORM_CHANGED, (uint64_t)SLSF2_Assume_Shadowmask << 32},
		{EFFECT_LIGHTING, (uint64_t)SLSF2_Effect_Lighting << 32},
		{LOD_OBJECTS, (uint64_t)SLSF2_LOD_Objects << 32},

		// TODO
		{PBR, 0},
		{REFRACTION_FALLOFF, 0},
		{INVERTED_FADE_PATTERN, 0},
		{HAIRTINT, 0},
		{NO_EXPOSURE, 0},
	};
}

enum TexClampMode : unsigned int
{
	CLAMP_S_CLAMP_T	= 0,
	CLAMP_S_WRAP_T	= 1,
	WRAP_S_CLAMP_T	= 2,
	WRAP_S_WRAP_T 	= 3,
	MIRRORED_S_MIRRORED_T = 4
};


struct UVScale
{
	float x;
	float y;

	UVScale() { reset(); }	
	void reset() { x = y = 1.0f; }
	void set( float _x, float _y ) { x = _x; y = _y; }
	void set( const Vector2 & v) { x = v[0]; y = v[1]; }
};

struct UVOffset
{
	float x;
	float y;

	UVOffset() { reset(); }
	void reset() { x = y = 0.0f; }
	void set(float _x, float _y) { x = _x; y = _y; }
	void set(const Vector2 & v) { x = v[0]; y = v[1]; }
};


//! A Property that specifies shader lighting (Bethesda-specific)
class BSShaderLightingProperty : public Property
{
public:
	BSShaderLightingProperty( Scene * scene, const QModelIndex & index ) : Property( scene, index ) { }
	~BSShaderLightingProperty();

	Type type() const override final { return ShaderLighting; }
	QString typeId() const override { return "BSShaderLightingProperty"; }

	friend void glProperty( BSShaderLightingProperty * );

	void clear() override;

	bool bind( int id, const QString & fname = QString(), TexClampMode mode = TexClampMode::WRAP_S_WRAP_T );
	bool bind( int id, const QVector<QVector<Vector2> > & texcoords );

	bool bindCube( int id, const QString & fname = QString() );

	//! Checks if the params of the shader depend on data from block
	bool isParamBlock( const QModelIndex & block ) { return ( block == iBlock || block == iTextureSet ); }

	QString fileName( int id ) const;
	//int coordSet( int id ) const;

	static int getId( const QString & id );

	//! Has Shader Flag 1
	bool hasSF1( ShaderFlags::SF1 flag ) { return flags1 & flag; };
	//! Has Shader Flag 2
	bool hasSF2( ShaderFlags::SF2 flag ) { return flags2 & flag; };

	void setFlags1( const NifModel * nif );
	void setFlags2( const NifModel * nif );

	bool hasVertexColors = false;
	bool hasVertexAlpha = false; // TODO Gavrant: it's unused

	bool depthTest = false;
	bool depthWrite = false;
	bool isDoubleSided = false;
	bool isVertexAlphaAnimation = false;

	UVScale uvScale;
	UVOffset uvOffset;
	TexClampMode clampMode = CLAMP_S_CLAMP_T;

	Material * getMaterial() const { return material; }

protected:
	ShaderFlags::SF1 flags1 = ShaderFlags::SLSF1_ZBuffer_Test;
	ShaderFlags::SF2 flags2 = ShaderFlags::SLSF2_ZBuffer_Write;

	//QVector<QString> textures;
	QPersistentModelIndex iTextureSet;
	QPersistentModelIndex iWetMaterial;

	Material * material = nullptr;
	void setMaterial( Material * newMaterial );

	void updateImpl( const NifModel * nif, const QModelIndex & block ) override;
	virtual void resetParams();
};

REGISTER_PROPERTY( BSShaderLightingProperty, ShaderLighting )


//! A Property that inherits BSLightingShaderProperty (Skyrim-specific)
class BSLightingShaderProperty final : public BSShaderLightingProperty
{
public:
	BSLightingShaderProperty( Scene * scene, const QModelIndex & index ) : BSShaderLightingProperty( scene, index ) { }

	QString typeId() const override final { return "BSLightingShaderProperty"; }

	//! Is Shader Type
	bool isST( ShaderFlags::ShaderType st ) { return shaderType == st; };

	bool hasGlowMap = false;
	bool hasEmittance = false;
	bool hasSoftlight = false;
	bool hasBacklight = false;
	bool hasRimlight = false;
	bool hasSpecularMap = false;
	bool hasMultiLayerParallax = false;
	bool hasEnvironmentMap = false;
	bool useEnvironmentMask = false;
	bool hasHeightMap = false;
	bool hasRefraction = false;
	bool hasDetailMask = false;
	bool hasTintMask = false;
	bool hasTintColor = false;
	bool greyscaleColor = false;

	Color3 emissiveColor;
	float emissiveMult = 1.0;

	Color3 specularColor;
	float specularGloss = 80.0;
	float specularStrength = 1.0;

	Color3 tintColor;

	float alpha = 1.0;

	float lightingEffect1 = 0.0;
	float lightingEffect2 = 1.0;

	float environmentReflection = 0.0;

	// Multi-layer properties
	float innerThickness = 1.0;
	UVScale innerTextureScale;
	float outerRefractionStrength = 0.0;
	float outerReflectionStrength = 1.0;

	float fresnelPower = 5.0;
	float paletteScale = 1.0;
	float rimPower = 2.0;
	float backlightPower = 0.0;

protected:
	void setController( const NifModel * nif, const QModelIndex & controller ) override final;
	void updateImpl( const NifModel * nif, const QModelIndex & block ) override;
	void resetParams() override;
	void updateParams( const NifModel * nif );
	void setTintColor( const NifModel* nif, const QString & itemName );

	ShaderFlags::ShaderType shaderType = ShaderFlags::ST_Default;
};

REGISTER_PROPERTY( BSLightingShaderProperty, ShaderLighting )


//! A Property that inherits BSEffectShaderProperty (Skyrim-specific)
class BSEffectShaderProperty final : public BSShaderLightingProperty
{
public:
	BSEffectShaderProperty( Scene * scene, const QModelIndex & index ) : BSShaderLightingProperty( scene, index ) { }

	QString typeId() const override final { return "BSEffectShaderProperty"; }

	float getAlpha() const { return emissiveColor.alpha(); }

	bool hasSourceTexture = false;
	bool hasGreyscaleMap = false;
	bool hasEnvironmentMap = false;
	bool hasNormalMap = false;
	bool hasEnvironmentMask = false;
	bool useFalloff = false;
	bool hasRGBFalloff = false;

	bool greyscaleColor = false;
	bool greyscaleAlpha = false;

	bool hasWeaponBlood = false;

	struct Falloff
	{
		float startAngle = 1.0f;
		float stopAngle = 0.0f;

		float startOpacity = 1.0f;
		float stopOpacity = 0.0f;

		float softDepth = 1.0f;
	};
	Falloff falloff;

	float lumEmittance = 0.0;

	Color4 emissiveColor;
	float emissiveMult = 1.0;

	float lightingInfluence = 0.0;
	float environmentReflection = 0.0;

protected:
	void setController( const NifModel * nif, const QModelIndex & controller ) override final;
	void updateImpl( const NifModel * nif, const QModelIndex & block ) override;
	void resetParams() override;
	void updateParams( const NifModel * nif );
};

REGISTER_PROPERTY( BSEffectShaderProperty, ShaderLighting )


namespace WaterShaderFlags
{
	enum SF1 : unsigned int
	{
		SWSF1_UNKNOWN0 = 1,
		SWSF1_Bypass_Refraction_Map = 1 << 1,
		SWSF1_Water_Toggle = 1 << 2,
		SWSF1_UNKNOWN3 = 1 << 3,
		SWSF1_UNKNOWN4 = 1 << 4,
		SWSF1_UNKNOWN5 = 1 << 5,
		SWSF1_Highlight_Layer_Toggle = 1 << 6,
		SWSF1_Enabled = 1 << 7
	};
}

//! A Property that inherits BSWaterShaderProperty (Skyrim-specific)
class BSWaterShaderProperty final : public BSShaderLightingProperty
{
public:
	BSWaterShaderProperty( Scene * scene, const QModelIndex & index ) : BSShaderLightingProperty( scene, index ) { }

	QString typeId() const override final { return "BSWaterShaderProperty"; }

	unsigned int getWaterShaderFlags() const;

	void setWaterShaderFlags( unsigned int );

protected:
	WaterShaderFlags::SF1 waterShaderFlags = WaterShaderFlags::SF1(0);
};

REGISTER_PROPERTY( BSWaterShaderProperty, ShaderLighting )


#endif
