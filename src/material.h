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

#ifndef MATERIAL_H
#define MATERIAL_H

#include "niftypes.h"

#include <QObject>
#include <QByteArray>
#include <QDataStream>
#include <QString>


class Material : public QObject
{
	Q_OBJECT

	friend class Renderer;
	friend class BSShape;
	friend class BSShaderLightingProperty;

public:
	Material( QString name );

	bool isValid() const;
	QStringList textures() const;
	QString getPath() const;

protected:
	virtual bool readFile();
	QByteArray find( QString path );
	QString toLocalPath( QString path ) const;


	QStringList textureList;

	bool fileExists = false;
	QString localPath;
	//QString absolutePath;

	QDataStream in;
	QByteArray data;

	// BGSM & BGEM shared variables

	quint32 version = 0;
	quint32 tileFlags = 0;
	bool bTileU = false;
	bool bTileV = false;
	float fUOffset;
	float fVOffset;
	float fUScale;
	float fVScale;
	float fAlpha;
	quint8 bAlphaBlend;
	quint32 iAlphaSrc;
	quint32 iAlphaDst;
	quint8 iAlphaTestRef;
	quint8 bAlphaTest;
	quint8 bZBufferWrite;
	quint8 bZBufferTest;
	quint8 bScreenSpaceReflections;
	quint8 bWetnessControl_ScreenSpaceReflections;
	quint8 bDecal;
	quint8 bTwoSided;
	quint8 bDecalNoFade;
	quint8 bNonOccluder;
	quint8 bRefraction;
	quint8 bRefractionFalloff;
	float fRefractionPower;
	quint8 bEnvironmentMapping;
	float fEnvironmentMappingMaskScale;
	quint8 bGrayscaleToPaletteColor;
};


class ShaderMaterial : public Material
{
	Q_OBJECT

	friend class Renderer;
	friend class BSShape;
	friend class BSShaderLightingProperty;

public:
	ShaderMaterial( QString name );

protected:
	bool readFile() override final;

	quint8 bEnableEditorAlphaRef;
	quint8 bRimLighting;
	float fRimPower;
	float fBacklightPower;
	quint8 bSubsurfaceLighting;
	float fSubsurfaceLightingRolloff;
	quint8 bSpecularEnabled;
	float specR, specG, specB;
	Color3 cSpecularColor;
	float fSpecularMult;
	float fSmoothness;
	float fFresnelPower;
	float fWetnessControl_SpecScale;
	float fWetnessControl_SpecPowerScale;
	float fWetnessControl_SpecMinvar;
	float fWetnessControl_EnvMapScale;
	float fWetnessControl_FresnelPower;
	float fWetnessControl_Metalness;
	QString sRootMaterialPath;
	quint8 bAnisoLighting;
	quint8 bEmitEnabled;
	float emitR = 0, emitG = 0, emitB = 0;
	Color3 cEmittanceColor;
	float fEmittanceMult;
	quint8 bModelSpaceNormals;
	quint8 bExternalEmittance;
	quint8 bBackLighting;
	quint8 bReceiveShadows;
	quint8 bHideSecret;
	quint8 bCastShadows;
	quint8 bDissolveFade;
	quint8 bAssumeShadowmask;
	quint8 bGlowmap;
	quint8 bEnvironmentMappingWindow;
	quint8 bEnvironmentMappingEye;
	quint8 bHair;
	float hairR, hairG, hairB;
	Color3 cHairTintColor;
	quint8 bTree;
	quint8 bFacegen;
	quint8 bSkinTint;
	quint8 bTessellate;
	float fDisplacementTextureBias;
	float fDisplacementTextureScale;
	float fTessellationPnScale;
	float fTessellationBaseFactor;
	float fTessellationFadeDistance;
	float fGrayscaleToPaletteScale;
	quint8 bSkewSpecularAlpha;

};


class EffectMaterial : public Material
{
	Q_OBJECT

	friend class Renderer;
	friend class BSShape;
	friend class BSShaderLightingProperty;

public:
	EffectMaterial( QString name );

protected:
	bool readFile() override final;

	quint8 bBloodEnabled;
	quint8 bEffectLightingEnabled;
	quint8 bFalloffEnabled;
	quint8 bFalloffColorEnabled;
	quint8 bGrayscaleToPaletteAlpha;
	quint8 bSoftEnabled;
	float baseR = 0, baseG = 0, baseB = 0;
	Color3 cBaseColor;
	float fBaseColorScale;
	float fFalloffStartAngle;
	float fFalloffStopAngle;
	float fFalloffStartOpacity;
	float fFalloffStopOpacity;
	float fLightingInfluence;
	quint8 iEnvmapMinLOD;
	float fSoftDepth;
};


#endif // MATERIAL_H
