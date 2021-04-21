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

#include "material.h"

#include <fsengine/fsengine.h>

#include <QBuffer>
#include <QDataStream>
#include <QDebug>
#include <QDir>
#include <QSettings>


//! @file material.cpp BGSM/BGEM file I/O

#define BGSM 0x4D534742
#define BGEM 0x4D454742

Material::Material( QString name, Game::GameMode game )
{
	localPath = toLocalPath( name.replace( "\\", "/" ) );
	if ( localPath.startsWith( "data/", Qt::CaseInsensitive ) ) {
		localPath.remove( 0, 5 );
	}
	data = find( localPath, game );

	fileExists = !data.isEmpty();
}

bool Material::openFile()
{
	if ( data.isEmpty() )
		return false;

	QBuffer f( &data );
	if ( f.open( QIODevice::ReadOnly ) ) {
		in.setDevice( &f );
		in.setByteOrder( QDataStream::LittleEndian );
		in.setFloatingPointPrecision( QDataStream::SinglePrecision );

		quint32 magic;
		in >> magic;

		if ( magic != BGSM && magic != BGEM )
			return false;

		return readFile();
	}

	return false;
}

bool Material::readFile()
{
	in >> version;

	in >> tileFlags;
	bTileU = (tileFlags & 0x2) != 0;
	bTileV = (tileFlags & 0x1) != 0;

	in >> fUOffset >> fVOffset >> fUScale >> fVScale;
	in >> fAlpha;
	in >> bAlphaBlend >> iAlphaSrc >> iAlphaDst;
	in >> iAlphaTestRef;
	in >> bAlphaTest >> bZBufferWrite >> bZBufferTest;
	in >> bScreenSpaceReflections >> bWetnessControl_ScreenSpaceReflections;
	in >> bDecal >> bTwoSided >> bDecalNoFade >> bNonOccluder;
	in >> bRefraction >> bRefractionFalloff >> fRefractionPower;
	in >> bEnvironmentMapping;
	if ( version < 10 )
		in >> fEnvironmentMappingMaskScale;

	in >> bGrayscaleToPaletteColor;

	if ( version >= 6 )
		in >> ucMaskWrites;

	return in.status() == QDataStream::Ok;
}

QByteArray Material::find( QString path, Game::GameMode game )
{
	QSettings settings;

	QString filename;
	QDir dir;
	for ( QString folder : Game::GameManager::folders(game) ) {
		dir.setPath( folder );

		if ( dir.exists( path ) ) {
			filename = QDir::fromNativeSeparators( dir.filePath( path ) );

			QFile f( filename );
			if ( f.open( QIODevice::ReadOnly ) )
				return f.readAll();
		}
	}

	for ( FSArchiveFile * archive : Game::GameManager::opened_archives(game) ) {
		if ( archive ) {
			filename = QDir::fromNativeSeparators( path.toLower() );
			if ( archive->hasFile( filename ) ) {
				QByteArray outData;
				archive->fileContents( filename, outData );

				if ( !outData.isEmpty() ) {
					return outData;
				}
			}
		}
	}

	return QByteArray();
}

QString Material::toLocalPath( QString path ) const
{
	QFileInfo finfo( path );

	QString p = path;
	if ( finfo.isAbsolute() ) {
		int idx = path.indexOf( "materials", 0, Qt::CaseInsensitive );

		p = path.right( path.length() - idx );
	}

	return p;
}

bool Material::isValid() const
{
	return readable && !data.isEmpty();
}

QStringList Material::textures() const
{
	return textureList;
}

QString Material::getPath() const
{
	return localPath;
}


ShaderMaterial::ShaderMaterial( QString name, Game::GameMode game ) : Material( name, game )
{
	if ( fileExists )
		readable = openFile();
}

bool ShaderMaterial::readFile()
{
	Material::readFile();

	size_t numTex = (version >= 17) ? 10 : 9;
	for ( int i = 0; i < numTex; i++ ) {
		char * str;
		in >> str;
		textureList << QString( str );
	}

	in >> bEnableEditorAlphaRef;
	if ( version >= 8 ) {
		in >> bTranslucency >> bTranslucencyThickObject >> bTranslucencyMixAlbedoWithSubsurfaceCol;
		in >> subR >> subG >> subB;
		cTranslucencySubsurfaceColor.setRGB( subR, subG, subB );
		in >> fTranslucencyTransmissiveScale >> fTranslucencyTurbulence;
	}
	else
		in >> bRimLighting >> fRimPower >> fBacklightPower >> bSubsurfaceLighting >> fSubsurfaceLightingRolloff;

	in >> bSpecularEnabled;
	in >> specR >> specG >> specB;
	cSpecularColor.setRGB( specR, specG, specB );
	in >> fSpecularMult >> fSmoothness;
	in >> fFresnelPower;
	in >> fWetnessControl_SpecScale >> fWetnessControl_SpecPowerScale >> fWetnessControl_SpecMinvar;
	if ( version < 10 )
		in >> fWetnessControl_EnvMapScale;

	in >> fWetnessControl_FresnelPower >> fWetnessControl_Metalness;

	if ( version > 2 )
		in >> bPBR;

	if ( version >= 9 )
		in >> bCustomPorosity >> fPorosityValue;

	char * rootMaterialStr;
	in >> rootMaterialStr;
	sRootMaterialPath = QString( rootMaterialStr );

	in >> bAnisoLighting >> bEmitEnabled;

	if ( bEmitEnabled )
		in >> emitR >> emitG >> emitB;
	cEmittanceColor.setRGB( emitR, emitG, emitB );

	in >> fEmittanceMult >> bModelSpaceNormals;
	in >> bExternalEmittance;
	if ( version >= 12 )
		in >> fLumEmittance;
	if ( version >= 13 )
		in >> bUseAdaptativeEmissive >> fAdaptativeEmissive_ExposureOffset >> fAdaptativeEmissive_FinalExposureMin >> fAdaptativeEmissive_FinalExposureMax;

	if ( version < 8 )
		in >> bBackLighting;
	in >> bReceiveShadows >> bHideSecret >> bCastShadows;
	in >> bDissolveFade >> bAssumeShadowmask >> bGlowmap;

	if ( version < 7 )
		in >> bEnvironmentMappingWindow >> bEnvironmentMappingEye;
	in >> bHair >> hairR >> hairG >> hairB;
	cHairTintColor.setRGB( hairR, hairG, hairB );

	in >> bTree >> bFacegen >> bSkinTint >> bTessellate;
	if ( version == 1 )
		in >> fDisplacementTextureBias >> fDisplacementTextureScale >>
		fTessellationPnScale >> fTessellationBaseFactor >> fTessellationFadeDistance;
	in >> fGrayscaleToPaletteScale >> bSkewSpecularAlpha;

	if ( version >= 3 ) {
		in >> bTerrain;
		if ( bTerrain ) {
			if ( version == 3 )
				in.skipRawData(4);
			in >> fTerrainThresholdFalloff >> fTerrainTilingDistance >> fTerrainRotationAngle;
		}
	}

	return in.status() == QDataStream::Ok;
}

EffectMaterial::EffectMaterial( QString name, Game::GameMode game ) : Material( name, game )
{
	if ( fileExists )
		readable = openFile();
}

bool EffectMaterial::readFile()
{
	Material::readFile();

	size_t numTex = (version >= 10) ? 8 : 5;
	for ( int i = 0; i < numTex; i++ ) {
		char * str;
		in >> str;
		textureList << QString( str );
	}

	if ( version >= 10 ) {
		in >> bEnvironmentMapping;
		in >> fEnvironmentMappingMaskScale;
	}

	in >> bBloodEnabled >> bEffectLightingEnabled;
	in >> bFalloffEnabled >> bFalloffColorEnabled;
	in >> bGrayscaleToPaletteAlpha >> bSoftEnabled;
	in >> baseR >> baseG >> baseB;

	cBaseColor.setRGB( baseR, baseG, baseB );

	in >> fBaseColorScale;
	in >> fFalloffStartAngle >> fFalloffStopAngle;
	in >> fFalloffStartOpacity >> fFalloffStopOpacity;
	in >> fLightingInfluence >> iEnvmapMinLOD >> fSoftDepth;

	if ( version >= 11 ) {
		in >> emitR >> emitG >> emitB;
		cEmittanceColor.setRGB( emitR, emitG, emitB );
		
		if ( version >= 15 ) {
			in >> fAdaptativeEmissive_ExposureOffset >> fAdaptativeEmissive_FinalExposureMin >> fAdaptativeEmissive_FinalExposureMax;

			if ( version >= 16 )
				in >> bGlowmap;

			if ( version >= 20 )
				in >> bEffectPbrSpecular;
		}
	}

	return in.status() == QDataStream::Ok;
}
