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

#include "data/niftypes.h"

#include <QObject>
#include <QByteArray>
#include <QDataStream>
#include <QString>


//! @file material.h Material, ShaderMaterial, EffectMaterial

class Material : public QObject
{
	Q_OBJECT

	friend class Renderer;
	friend class BSShape;
	friend class BSShaderLightingProperty;
	friend class BSLightingShaderProperty;
	friend class BSEffectShaderProperty;

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

	// Is not JSON format or otherwise unreadable
	bool readable = false;

	// BGSM & BGEM shared variables

	quint32 version = 0;
	quint32 tileFlags = 0;
	bool bTileU = false;
	bool bTileV = false;
	float fUOffset = 0;
	float fVOffset = 0;
	float fUScale = 1.0;
	float fVScale = 1.0;
	float fAlpha = 1.0;
	quint8 bAlphaBlend = 0;
	quint32 iAlphaSrc = 0;
	quint32 iAlphaDst = 0;
	quint8 iAlphaTestRef = 255;
	quint8 bAlphaTest = 0;
	quint8 bZBufferWrite = 1;
	quint8 bZBufferTest = 1;
	quint8 bScreenSpaceReflections = 0;
	quint8 bWetnessControl_ScreenSpaceReflections = 0;
	quint8 bDecal = 0;
	quint8 bTwoSided = 0;
	quint8 bDecalNoFade = 0;
	quint8 bNonOccluder = 0;
	quint8 bRefraction = 0;
	quint8 bRefractionFalloff = 0;
	float fRefractionPower = 0;
	quint8 bEnvironmentMapping = 0;
	float fEnvironmentMappingMaskScale = 1.0;
	quint8 bGrayscaleToPaletteColor = 1.0;
};


class ShaderMaterial : public Material
{
	Q_OBJECT

	friend class Renderer;
	friend class BSShape;
	friend class BSShaderLightingProperty;
	friend class BSLightingShaderProperty;

public:
	ShaderMaterial( QString name );

protected:
	bool readFile() override final;

	quint8 bEnableEditorAlphaRef = 0;
	quint8 bRimLighting = 0;
	float fRimPower = 0;
	float fBacklightPower = 0;
	quint8 bSubsurfaceLighting = 0;
	float fSubsurfaceLightingRolloff = 0.0;
	quint8 bSpecularEnabled = 1;
	float specR = 1.0, specG = 1.0, specB = 1.0;
	Color3 cSpecularColor;
	float fSpecularMult = 0;
	float fSmoothness = 0;
	float fFresnelPower = 0;
	float fWetnessControl_SpecScale = 0;
	float fWetnessControl_SpecPowerScale = 0;
	float fWetnessControl_SpecMinvar = 0;
	float fWetnessControl_EnvMapScale = 0;
	float fWetnessControl_FresnelPower = 0;
	float fWetnessControl_Metalness = 0;
	QString sRootMaterialPath;
	quint8 bAnisoLighting = 0;
	quint8 bEmitEnabled = 0;
	float emitR = 0, emitG = 0, emitB = 0;
	Color3 cEmittanceColor;
	float fEmittanceMult = 0;
	quint8 bModelSpaceNormals = 0;
	quint8 bExternalEmittance = 0;
	quint8 bBackLighting = 0;
	quint8 bReceiveShadows = 1;
	quint8 bHideSecret = 0;
	quint8 bCastShadows = 1;
	quint8 bDissolveFade = 0;
	quint8 bAssumeShadowmask = 0;
	quint8 bGlowmap = 0;
	quint8 bEnvironmentMappingWindow = 0;
	quint8 bEnvironmentMappingEye = 0;
	quint8 bHair = 0;
	float hairR = 0, hairG = 0, hairB = 0;
	Color3 cHairTintColor;
	quint8 bTree = 0;
	quint8 bFacegen = 0;
	quint8 bSkinTint = 0;
	quint8 bTessellate = 0;
	float fDisplacementTextureBias = 0;
	float fDisplacementTextureScale = 0;
	float fTessellationPnScale = 0;
	float fTessellationBaseFactor = 0;
	float fTessellationFadeDistance = 0;
	float fGrayscaleToPaletteScale = 0;
	quint8 bSkewSpecularAlpha = 0;

};


class EffectMaterial : public Material
{
	Q_OBJECT

	friend class Renderer;
	friend class BSShape;
	friend class BSShaderLightingProperty;
	friend class BSEffectShaderProperty;

public:
	EffectMaterial( QString name );

protected:
	bool readFile() override final;

	quint8 bBloodEnabled = 0;
	quint8 bEffectLightingEnabled = 0;
	quint8 bFalloffEnabled = 0;
	quint8 bFalloffColorEnabled = 0;
	quint8 bGrayscaleToPaletteAlpha = 0;
	quint8 bSoftEnabled = 0;
	float baseR = 1.0, baseG = 1.0, baseB = 1.0;
	Color3 cBaseColor;
	float fBaseColorScale = 1.0;
	float fFalloffStartAngle = 1.0;
	float fFalloffStopAngle = 0;
	float fFalloffStartOpacity = 1.0;
	float fFalloffStopOpacity = 0;
	float fLightingInfluence = 1.0;
	quint8 iEnvmapMinLOD = 0;
	float fSoftDepth = 100.0;
};


#endif // MATERIAL_H
