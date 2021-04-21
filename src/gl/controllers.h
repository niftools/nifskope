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

#ifndef CONTROLLERS_H
#define CONTROLLERS_H

#include "gl/glcontroller.h" // Inherited
#include "data/niftypes.h"

#include <QPointer>


//! @file controllers.h Controller subclasses

class Shape;
class Node;

//! Controller for `NiControllerManager` blocks
class ControllerManager final : public Controller
{
public:
	ControllerManager( Node * node, const QModelIndex & index );

	void updateTime( float ) override final {}

	bool update( const NifModel * nif, const QModelIndex & index ) override final;

	void setSequence( const QString & seqname ) override final;

protected:
	QPointer<Node> target;
};


//! Controller for `NiKeyframeController` blocks
class KeyframeController final : public Controller
{
public:
	KeyframeController( Node * node, const QModelIndex & index );

	void updateTime( float time ) override final;

	bool update( const NifModel * nif, const QModelIndex & index ) override final;

protected:
	QPointer<Node> target;

	QPersistentModelIndex iTranslations, iRotations, iScales;

	int lTrans, lRotate, lScale;
};


//! Controller for `NiTransformController` blocks
class TransformController final : public Controller
{
public:
	TransformController( Node * node, const QModelIndex & index );

	void updateTime( float time ) override final;

	void setInterpolator( const QModelIndex & idx ) override final;

protected:
	QPointer<Node> target;
	QPointer<TransformInterpolator> interpolator;
};


//! Controller for `NiMultiTargetTransformController` blocks
class MultiTargetTransformController final : public Controller
{
	typedef QPair<QPointer<Node>, QPointer<TransformInterpolator> > TransformTarget;

public:
	MultiTargetTransformController( Node * node, const QModelIndex & index );

	void updateTime( float time ) override final;

	bool update( const NifModel * nif, const QModelIndex & index ) override final;

	bool setInterpolatorNode( Node * node, const QModelIndex & idx );

protected:
	QPointer<Node> target;
	QList<TransformTarget> extraTargets;
};


//! Controller for `NiVisController` blocks
class VisibilityController final : public Controller
{
public:
	VisibilityController( Node * node, const QModelIndex & index );

	void updateTime( float time ) override final;

	bool update( const NifModel * nif, const QModelIndex & index ) override final;

protected:
	QPointer<Node> target;

	int visLast;
};


//! Controller for `NiGeomMorpherController` blocks
class MorphController final : public Controller
{
	//! A representation of Mesh geometry morphs
	struct MorphKey
	{
		QPersistentModelIndex iFrames;
		QVector<Vector3> verts;
		int index;
	};

public:
	MorphController( Shape * mesh, const QModelIndex & index );
	~MorphController();

	void updateTime( float time ) override final;

	bool update( const NifModel * nif, const QModelIndex & index ) override final;

protected:
	QPointer<Shape> target;
	QVector<MorphKey *>  morph;
};


//! Controller for `NiUVController` blocks
class UVController final : public Controller
{
public:
	UVController( Shape * mesh, const QModelIndex & index );

	~UVController();

	void updateTime( float time ) override final;

	bool update( const NifModel * nif, const QModelIndex & index ) override final;

protected:
	QPointer<Shape> target;

	int luv = 0;
};


class Particles;

//! Controller for `NiParticleSystemController` and other blocks
class ParticleController final : public Controller
{
	struct Particle
	{
		Vector3 position;
		Vector3 velocity;
		Vector3 unknown;
		float lifetime = 0;
		float lifespan = 0;
		float lasttime = 0;
		short y = 0;
		short vertex = 0;

		Particle()
		{
		}
	};
	QVector<Particle> list;
	struct Gravity
	{
		float force;
		int type;
		Vector3 position;
		Vector3 direction;
	};
	QVector<Gravity> grav;

	QPointer<Particles> target;

	float emitStart = 0, emitStop = 0, emitRate = 0, emitLast = 0, emitAccu = 0, emitMax = 0;
	QPointer<Node> emitNode;
	Vector3 emitRadius;

	float spd = 0, spdRnd = 0;
	float ttl = 0, ttlRnd = 0;

	float inc = 0, incRnd = 0;
	float dec = 0, decRnd = 0;

	float size = 0;
	float grow = 0;
	float fade = 0;

	float localtime = 0;

	QList<QPersistentModelIndex> iExtras;
	QPersistentModelIndex iColorKeys;

public:
	ParticleController( Particles * particles, const QModelIndex & index );

	bool update( const NifModel * nif, const QModelIndex & index ) override final;

	void updateTime( float time ) override final;

	void startParticle( Particle & p );

	void moveParticle( Particle & p, float deltaTime );

	void sizeParticle( Particle & p, float & size );

	void colorParticle( Particle & p, Color4 & color );
};


class AlphaProperty;
class MaterialProperty;
class TexturingProperty;
class TextureProperty;
class BSEffectShaderProperty;
class BSLightingShaderProperty;

//! Controller for alpha values in a MaterialProperty
class AlphaController final : public Controller
{
public:
	AlphaController( MaterialProperty * prop, const QModelIndex & index );

	AlphaController( AlphaProperty * prop, const QModelIndex & index );

	void updateTime( float time ) override final;

protected:
	QPointer<MaterialProperty> materialProp;
	QPointer<AlphaProperty> alphaProp;

	int lAlpha = 0;
};


//! Controller for color values in a MaterialProperty
class MaterialColorController final : public Controller
{
public:
	MaterialColorController( MaterialProperty * prop, const QModelIndex & index );

	void updateTime( float time ) override final;

	bool update( const NifModel * nif, const QModelIndex & index ) override final;

protected:
	QPointer<MaterialProperty> target; //!< The MaterialProperty being controlled

	int lColor = 0;                        //!< Last interpolation time
	int tColor = tAmbient;                 //!< The color slot being controlled

	//! Color slots that can be controlled
	enum
	{
		tAmbient = 0,
		tDiffuse = 1,
		tSpecular = 2,
		tSelfIllum = 3
	};
};


//! Controller for source textures in a TexturingProperty
class TexFlipController final : public Controller
{
public:
	TexFlipController( TexturingProperty * prop, const QModelIndex & index );

	TexFlipController( TextureProperty * prop, const QModelIndex & index );

	void updateTime( float time ) override final;

	bool update( const NifModel * nif, const QModelIndex & index ) override final;

protected:
	QPointer<TexturingProperty> target;
	QPointer<TextureProperty> oldTarget;

	float flipDelta = 0;
	int flipSlot = 0;

	int flipLast = 0;

	QPersistentModelIndex iSources;
};


//! Controller for transformations in a TexturingProperty
class TexTransController final : public Controller
{
public:
	TexTransController( TexturingProperty * prop, const QModelIndex & index );

	void updateTime( float time ) override final;

	bool update( const NifModel * nif, const QModelIndex & index ) override final;

protected:
	QPointer<TexturingProperty> target;

	int texSlot = 0;
	int texOP = 0;

	int lX = 0;
};

namespace EffectFloat
{
	enum Variable
	{
		Emissive_Multiple = 0,
		Falloff_Start_Angle = 1,
		Falloff_Stop_Angle = 2,
		Falloff_Start_Opacity = 3,
		Falloff_Stop_Opacity = 4,
		Alpha = 5,
		U_Offset = 6,
		U_Scale = 7,
		V_Offset = 8,
		V_Scale = 9
	};
}


//! Controller for float values in a BSEffectShaderProperty
class EffectFloatController final : public Controller
{
public:
	EffectFloatController( BSEffectShaderProperty * prop, const QModelIndex & index );

	void updateTime( float time ) override final;

	bool update( const NifModel * nif, const QModelIndex & index ) override final;

protected:
	QPointer<BSEffectShaderProperty> target;

	EffectFloat::Variable variable = EffectFloat::Emissive_Multiple;
};


//! Controller for color values in a BSEffectShaderProperty
class EffectColorController final : public Controller
{
public:
	EffectColorController( BSEffectShaderProperty * prop, const QModelIndex & index );

	void updateTime( float time ) override final;

	bool update( const NifModel * nif, const QModelIndex & index ) override final;

protected:
	QPointer<BSEffectShaderProperty> target;

	int variable = 0;
};

namespace LightingFloat
{
	enum Variable
	{
		Refraction_Strength = 0,
		Reflection_Strength = 8,
		Glossiness = 9,
		Specular_Strength = 10,
		Emissive_Multiple = 11,
		Alpha = 12,
		U_Offset = 20,
		U_Scale = 21,
		V_Offset = 22,
		V_Scale = 23
	};
}


//! Controller for float values in a BSEffectShaderProperty
class LightingFloatController final : public Controller
{
public:
	LightingFloatController( BSLightingShaderProperty * prop, const QModelIndex & index );

	void updateTime( float time ) override final;

	bool update( const NifModel * nif, const QModelIndex & index ) override final;

protected:
	QPointer<BSLightingShaderProperty> target;

	LightingFloat::Variable variable = LightingFloat::Refraction_Strength;
};


//! Controller for color values in a BSEffectShaderProperty
class LightingColorController final : public Controller
{
public:
	LightingColorController( BSLightingShaderProperty * prop, const QModelIndex & index );

	void updateTime( float time ) override final;

	bool update( const NifModel * nif, const QModelIndex & index ) override final;

protected:
	QPointer<BSLightingShaderProperty> target;

	int variable = 0;
};


#endif // CONTROLLERS_H
