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
#include <fsengine/fsmanager.h>

#include <QBuffer>
#include <QDataStream>
#include <QDebug>
#include <QDir>
#include <QSettings>


//! @file material.cpp BGSM/BGEM file I/O

#define BGSM 0x4D534742
#define BGEM 0x4D454742

Material::Material( QString name )
{
	localPath = toLocalPath( name.replace( "\\", "/" ) );
	if ( localPath.startsWith( "data/", Qt::CaseInsensitive ) ) {
		localPath.remove( 0, 5 );
	}
	data = find( localPath );

	fileExists = !data.isEmpty();
}

bool Material::readFile()
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

		in >> version >> tileFlags;

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
		in >> bEnvironmentMapping >> fEnvironmentMappingMaskScale;
		in >> bGrayscaleToPaletteColor;

		return in.status() == QDataStream::Ok;
	}

	return false;
}

QByteArray Material::find( QString path )
{
	QSettings settings;
	QStringList folders = settings.value( "Settings/Resources/Folders", QStringList() ).toStringList();

	QString filename;
	QDir dir;
	for ( QString folder : folders ) {
		dir.setPath( folder );

		if ( dir.exists( path ) ) {
			filename = QDir::fromNativeSeparators( dir.filePath( path ) );

			QFile f( filename );
			if ( f.open( QIODevice::ReadOnly ) )
				return f.readAll();
		}
	}

	for ( FSArchiveFile * archive : FSManager::archiveList() ) {
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


ShaderMaterial::ShaderMaterial( QString name ) : Material( name )
{
	if ( fileExists )
		readable = readFile();
}

bool ShaderMaterial::readFile()
{
	if ( data.isEmpty() || !Material::readFile() )
		return false;

	QBuffer f( &data );
	if ( f.open( QIODevice::ReadOnly ) ) {
		in.setDevice( &f );
		in.setByteOrder( QDataStream::LittleEndian );
		in.setFloatingPointPrecision( QDataStream::SinglePrecision );

		in.skipRawData( 63 );

		for ( int i = 0; i < 9; i++ ) {
			char * str;
			in >> str;
			textureList << QString( str );
		}

		in >> bEnableEditorAlphaRef >> bRimLighting;
		in >> fRimPower >> fBacklightPower;
		in >> bSubsurfaceLighting >> fSubsurfaceLightingRolloff;
		in >> bSpecularEnabled;
		in >> specR >> specG >> specB;
		cSpecularColor.setRGB( specR, specG, specB );
		in >> fSpecularMult >> fSmoothness >> fFresnelPower;
		in >> fWetnessControl_SpecScale >> fWetnessControl_SpecPowerScale >> fWetnessControl_SpecMinvar;
		in >> fWetnessControl_EnvMapScale >> fWetnessControl_FresnelPower >> fWetnessControl_Metalness;

		char * rootMaterialStr;
		in >> rootMaterialStr;
		sRootMaterialPath = QString( rootMaterialStr );

		in >> bAnisoLighting >> bEmitEnabled;

		if ( bEmitEnabled )
			in >> emitR >> emitG >> emitB;
		cEmittanceColor.setRGB( emitR, emitG, emitB );

		in >> fEmittanceMult >> bModelSpaceNormals;
		in >> bExternalEmittance >> bBackLighting;
		in >> bReceiveShadows >> bHideSecret >> bCastShadows;
		in >> bDissolveFade >> bAssumeShadowmask >> bGlowmap;
		in >> bEnvironmentMappingWindow >> bEnvironmentMappingEye;
		in >> bHair >> hairR >> hairG >> hairB;
		cHairTintColor.setRGB( hairR, hairG, hairB );

		in >> bTree >> bFacegen >> bSkinTint >> bTessellate;
		in >> fDisplacementTextureBias >> fDisplacementTextureScale;
		in >> fTessellationPnScale >> fTessellationBaseFactor >> fTessellationFadeDistance;
		in >> fGrayscaleToPaletteScale >> bSkewSpecularAlpha;

		return in.status() == QDataStream::Ok;
	}

	return false;
}

EffectMaterial::EffectMaterial( QString name ) : Material( name )
{
	if ( fileExists )
		readable = readFile();
}

bool EffectMaterial::readFile()
{
	if ( data.isEmpty() || !Material::readFile() )
		return false;

	QBuffer f( &data );
	if ( f.open( QIODevice::ReadOnly ) ) {
		in.setDevice( &f );
		in.setByteOrder( QDataStream::LittleEndian );
		in.setFloatingPointPrecision( QDataStream::SinglePrecision );

		in.skipRawData( 63 );

		for ( int i = 0; i < 5; i++ ) {
			char * str;
			in >> str;
			textureList << QString( str );
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

		return in.status() == QDataStream::Ok;
	}

	return false;
}
