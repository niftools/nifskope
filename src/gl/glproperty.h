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

	void update( const NifModel * nif, const QModelIndex & block ) override final;

	bool blend() const { return alphaBlend; }
	bool test() const { return alphaTest; }
	bool sort() const { return alphaSort; }
	float threshold() const { return alphaThreshold; }

	void setThreshold( float );

	friend void glProperty( AlphaProperty * );

protected:
	void setController( const NifModel * nif, const QModelIndex & controller ) override final;

	bool alphaBlend = false, alphaTest = false, alphaSort = false;
	GLenum alphaSrc = 0, alphaDst = 0, alphaFunc = 0;
	GLfloat alphaThreshold = 0;
};

REGISTER_PROPERTY( AlphaProperty, Alpha )

//! A Property that specifies depth testing
class ZBufferProperty final : public Property
{
public:
	ZBufferProperty( Scene * scene, const QModelIndex & index ) : Property( scene, index ) {}

	Type type() const override final { return ZBuffer; }
	QString typeId() const override final { return "NiZBufferProperty"; }

	void update( const NifModel * nif, const QModelIndex & block ) override final;

	bool test() const { return depthTest; }
	bool mask() const { return depthMask; }

	friend void glProperty( ZBufferProperty * );

protected:
	bool depthTest = false;
	bool depthMask = false;
	GLenum depthFunc = 0;
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

	void update( const NifModel * nif, const QModelIndex & block ) override final;

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

	void update( const NifModel * nif, const QModelIndex & block ) override final;

	friend void glProperty( TextureProperty * );

	bool bind();
	bool bind( const QVector<QVector<Vector2> > & texcoords );

	QString fileName() const;

protected:
	QPersistentModelIndex iImage;

	void setController( const NifModel * nif, const QModelIndex & controller ) override final;
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

	void update( const NifModel * nif, const QModelIndex & block ) override final;

	friend void glProperty( class MaterialProperty *, class SpecularProperty * );

	GLfloat alphaValue() const { return alpha; }

protected:
	Color4 ambient, diffuse, specular, emissive;
	GLfloat shininess = 0, alpha = 0;
	bool overridden = false;

	void setController( const NifModel * nif, const QModelIndex & controller ) override final;
};

REGISTER_PROPERTY( MaterialProperty, MaterialProp )

//! A Property that specifies specularity
class SpecularProperty final : public Property
{
public:
	SpecularProperty( Scene * scene, const QModelIndex & index ) : Property( scene, index ) {}

	Type type() const override final { return Specular; }
	QString typeId() const override final { return "NiSpecularProperty"; }

	void update( const NifModel * nif, const QModelIndex & index ) override final;

	friend void glProperty( class MaterialProperty *, class SpecularProperty * );

protected:
	bool spec = false;
};

REGISTER_PROPERTY( SpecularProperty, Specular )

//! A Property that specifies wireframe drawing
class WireframeProperty final : public Property
{
public:
	WireframeProperty( Scene * scene, const QModelIndex & index ) : Property( scene, index ) {}

	Type type() const override final { return Wireframe; }
	QString typeId() const override final { return "NiWireframeProperty"; }

	void update( const NifModel * nif, const QModelIndex & index ) override final;

	friend void glProperty( WireframeProperty * );

protected:
	bool wire = false;
};

REGISTER_PROPERTY( WireframeProperty, Wireframe )

//! A Property that specifies vertex color handling
class VertexColorProperty final : public Property
{
public:
	VertexColorProperty( Scene * scene, const QModelIndex & index ) : Property( scene, index ) {}

	Type type() const override final { return VertexColor; }
	QString typeId() const override final { return "NiVertexColorProperty"; }

	void update( const NifModel * nif, const QModelIndex & index ) override final;

	friend void glProperty( VertexColorProperty *, bool vertexcolors );

protected:
	int lightmode = 0;
	int vertexmode = 0;
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

	void update( const NifModel * nif, const QModelIndex & index ) override final;

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
	float x = 1.0f;
	float y = 1.0f;
};

struct UVOffset
{
	float x = 0.0f;
	float y = 0.0f;
};


//! A Property that specifies shader lighting (Bethesda-specific)
class BSShaderLightingProperty : public Property
{
public:
	BSShaderLightingProperty( Scene * scene, const QModelIndex & index ) : Property( scene, index ) {}

	Type type() const override final { return ShaderLighting; }
	QString typeId() const override { return "BSShaderLightingProperty"; }

	void update( const NifModel * nif, const QModelIndex & block ) override;

	friend void glProperty( BSShaderLightingProperty * );

	bool bind( int id, const QString & fname = QString(), TexClampMode mode = TexClampMode::WRAP_S_WRAP_T );
	bool bind( int id, const QVector<QVector<Vector2> > & texcoords );
	bool bind( int id, const QVector<QVector<Vector2> > & texcoords, int stage );

	bool bindCube( int id, const QString & fname = QString() );

	QString fileName( int id ) const;
	//int coordSet( int id ) const;

	static int getId( const QString & id );

	QPersistentModelIndex getTextureSet() const;

	bool hasSF1( ShaderFlags::SF1 flag ) { return getFlags1() & flag; };
	bool hasSF2( ShaderFlags::SF2 flag ) { return getFlags2() & flag; };

	unsigned int getFlags1() const;
	unsigned int getFlags2() const;

	void setFlags1( unsigned int );
	void setFlags2( unsigned int );

	UVScale getUvScale() const;
	UVOffset getUvOffset() const;

	void setUvScale( float, float );
	void setUvOffset( float, float );

	TexClampMode getClampMode() const;

	void setClampMode( uint mode );

	bool getDepthTest() { return depthTest; }
	bool getDepthWrite() { return depthWrite; }
	bool getIsDoubleSided() { return isDoubleSided; }
	bool getIsTranslucent() { return isTranslucent; }

	bool hasVertexColors = false;
	bool hasVertexAlpha = false;

	Material * mat() const;

protected:
	ShaderFlags::SF1 flags1 = ShaderFlags::SLSF1_ZBuffer_Test;
	ShaderFlags::SF2 flags2 = ShaderFlags::SLSF2_ZBuffer_Write;

	//QVector<QString> textures;
	QPersistentModelIndex iTextureSet;
	QPersistentModelIndex iSourceTexture;
	QPersistentModelIndex iWetMaterial;

	Material * material = nullptr;

	UVScale uvScale;
	UVOffset uvOffset;

	TexClampMode clampMode = CLAMP_S_CLAMP_T;

	bool depthTest = false;
	bool depthWrite = false;
	bool isDoubleSided = false;
	bool isTranslucent = false;
};

REGISTER_PROPERTY( BSShaderLightingProperty, ShaderLighting )


//! A Property that inherits BSShaderLightingProperty (Skyrim-specific)
class BSLightingShaderProperty final : public BSShaderLightingProperty
{
public:
	BSLightingShaderProperty( Scene * scene, const QModelIndex & index ) : BSShaderLightingProperty( scene, index )
	{
		emissiveMult = 1.0f;
		emissiveColor = Color3( 0, 0, 0 );
		
		specularStrength = 1.0f;
		specularColor = Color3( 1.0f, 1.0f, 1.0f );
	}

	QString typeId() const override final { return "BSLightingShaderProperty"; }

	void update( const NifModel * nif, const QModelIndex & block ) override;
	void updateParams( const NifModel * nif, const QModelIndex & prop );

	bool isST( ShaderFlags::ShaderType st ) { return getShaderType() == st; };

	Color3 getEmissiveColor() const;
	Color3 getSpecularColor() const;

	float getEmissiveMult() const;

	float getSpecularGloss() const;
	float getSpecularStrength() const;

	float getLightingEffect1() const;
	float getLightingEffect2() const;

	float getInnerThickness() const;
	UVScale getInnerTextureScale() const;
	float getOuterRefractionStrength() const;
	float getOuterReflectionStrength() const;

	float getEnvironmentReflection() const;

	float getAlpha() const;

	void setShaderType( unsigned int );

	void setEmissive( const Color3 & color, float mult = 1.0f );
	void setSpecular( const Color3 & color, float glossiness = 80.0f, float strength = 1.0f );

	void setLightingEffect1( float );
	void setLightingEffect2( float );

	void setInnerThickness( float );
	void setInnerTextureScale( float, float );
	void setOuterRefractionStrength( float );
	void setOuterReflectionStrength( float );

	void setEnvironmentReflection( float );

	void setAlpha( float );

	Color3 getTintColor() const;
	void setTintColor( const Color3 & );

	bool hasGlowMap = false;
	bool hasEmittance = false;
	bool hasSoftlight = false;
	bool hasBacklight = false;
	bool hasRimlight = false;
	bool hasModelSpaceNormals = false;
	bool hasSpecularMap = false;
	bool hasMultiLayerParallax = false;
	bool hasCubeMap = false;
	bool hasEnvironmentMap = false;
	bool useEnvironmentMask = false;
	bool hasHeightMap = false;
	bool hasRefraction = false;
	bool hasFireRefraction = false;
	bool hasDetailMask = false;
	bool hasTintMask = false;
	bool hasTintColor = false;
	bool greyscaleColor = false;

	ShaderFlags::ShaderType getShaderType();

	float fresnelPower = 5.0;
	float paletteScale = 1.0;
	float rimPower = 2.0;
	float backlightPower = 0.0;

protected:
	void setController( const NifModel * nif, const QModelIndex & controller ) override final;

	ShaderFlags::ShaderType shaderType = ShaderFlags::ST_Default;

	Color3 emissiveColor;
	Color3 specularColor;
	Color3 tintColor;

	float alpha = 1.0;

	float emissiveMult = 1.0;

	float specularGloss = 80.0;
	float specularStrength = 1.0;

	float lightingEffect1 = 0.0;
	float lightingEffect2 = 1.0;

	float environmentReflection = 0.0;

	// Multi-layer properties
	float innerThickness = 1.0;
	UVScale innerTextureScale;
	float outerRefractionStrength = 0.0;
	float outerReflectionStrength = 1.0;
};

REGISTER_PROPERTY( BSLightingShaderProperty, ShaderLighting )


//! A Property that inherits BSShaderLightingProperty (Skyrim-specific)
class BSEffectShaderProperty final : public BSShaderLightingProperty
{
public:
	BSEffectShaderProperty( Scene * scene, const QModelIndex & index ) : BSShaderLightingProperty( scene, index )
	{
	}

	QString typeId() const override final { return "BSEffectShaderProperty"; }

	void update( const NifModel * nif, const QModelIndex & block ) override;
	void updateParams( const NifModel * nif, const QModelIndex & prop );

	Color4 getEmissiveColor() const;
	float getEmissiveMult() const;

	float getAlpha() const;

	void setEmissive( const Color4 & color, float mult = 1.0f );
	void setFalloff( float, float, float, float, float );

	float getEnvironmentReflection() const;
	void setEnvironmentReflection( float );

	float getLightingInfluence() const;
	void setLightingInfluence( float );

	bool hasSourceTexture = false;
	bool hasGreyscaleMap = false;
	bool hasEnvMap = false;
	bool hasNormalMap = false;
	bool hasEnvMask = false;
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

protected:
	void setController( const NifModel * nif, const QModelIndex & controller ) override final;

	Color4 emissiveColor;
	float emissiveMult = 1.0;

	float lightingInfluence = 0.0;
	float environmentReflection = 0.0;
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

//! A Property that inherits BSShaderLightingProperty (Skyrim-specific)
class BSWaterShaderProperty final : public BSShaderLightingProperty
{
public:
	BSWaterShaderProperty( Scene * scene, const QModelIndex & index ) : BSShaderLightingProperty( scene, index )
	{
	}

	QString typeId() const override final { return "BSWaterShaderProperty"; }

	unsigned int getWaterShaderFlags() const;

	void setWaterShaderFlags( unsigned int );

protected:
	WaterShaderFlags::SF1 waterShaderFlags = WaterShaderFlags::SF1(0);
};

REGISTER_PROPERTY( BSWaterShaderProperty, ShaderLighting )


#endif
