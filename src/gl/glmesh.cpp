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

#include "glmesh.h"
#include "config.h"
#include "settings.h"

#include "controllers.h"
#include "glscene.h"
#include "gltools.h"

#include <QBuffer>
#include <QDebug>
#include <QSettings>

#include <QOpenGLFunctions>


//! @file glmesh.cpp Scene management for visible meshes such as NiTriShapes.


Shape::Shape( Scene * s, const QModelIndex & b ) : Node( s, b )
{
}

Mesh::Mesh( Scene * s, const QModelIndex & b ) : Shape( s, b )
{
}

bool Mesh::isBSLODPresent = false;

void Mesh::clear()
{
	Node::clear();

	iData = iSkin = iSkinData = iSkinPart = iTangentData = QModelIndex();
	bslsp = nullptr;
	bsesp = nullptr;

	verts.clear();
	norms.clear();
	colors.clear();
	coords.clear();
	tangents.clear();
	bitangents.clear();
	triangles.clear();
	tristrips.clear();
	weights.clear();
	partitions.clear();
	sortedTriangles.clear();
	indices.clear();
	transVerts.clear();
	transNorms.clear();
	transColors.clear();
	transColorsNoAlpha.clear();
	transTangents.clear();
	transBitangents.clear();

	isBSLODPresent = false;
	isDoubleSided = false;
}

void Shape::setController( const NifModel * nif, const QModelIndex & iController )
{
	if ( nif->itemName( iController ) == "NiGeomMorpherController" ) {
		Controller * ctrl = new MorphController( this, iController );
		ctrl->update( nif, iController );
		controllers.append( ctrl );
	} else if ( nif->itemName( iController ) == "NiUVController" ) {
		Controller * ctrl = new UVController( this, iController );
		ctrl->update( nif, iController );
		controllers.append( ctrl );
	} else {
		Node::setController( nif, iController );
	}
}

void Mesh::update( const NifModel * nif, const QModelIndex & index )
{
	Node::update( nif, index );

	if ( !iBlock.isValid() || !index.isValid() )
		return;

	if ( !isBSLODPresent ) {

		if ( nif->isNiBlock( iBlock, "BSLODTriShape" ) ) {
			isBSLODPresent = true;
		}

		emit nif->lodSliderChanged( isBSLODPresent );
	}

	updateData |= ( iData == index ) || ( iTangentData == index );
	updateSkin |= ( iSkin == index );
	updateSkin |= ( iSkinData == index );
	updateSkin |= ( iSkinPart == index );

	isVertexAlphaAnimation = false;
	isDoubleSided = false;

	if ( nif->checkVersion( 0x14020007, 0 ) && nif->inherits( iBlock, "NiTriBasedGeom" ) )
	{
		QVector<qint32> props = nif->getLinkArray( iBlock, "Properties" ) + nif->getLinkArray( iBlock, "BS Properties" );
	
		for ( int i = 0; i < props.count(); i++ ) {
			QModelIndex iProp = nif->getBlock( props[i], "BSLightingShaderProperty" );
	
			if ( iProp.isValid() ) {

				if ( !bslsp )
					bslsp = properties.get<BSLightingShaderProperty>();

				if ( !bslsp )
					break;

				auto hasSF1 = [this]( ShaderFlags::SF1 flag ) {
					return bslsp->getFlags1() & flag;
				};

				auto hasSF2 = [this]( ShaderFlags::SF2 flag ) {
					return bslsp->getFlags2() & flag;
				};

				auto isST = [this]( ShaderFlags::ShaderType st ) {
					return bslsp->getShaderType() == st;
				};


				auto shaderType = nif->get<unsigned int>( iProp, "Skyrim Shader Type" );

				auto sf1 = nif->get<unsigned int>( iProp, "Shader Flags 1" );
				auto sf2 = nif->get<unsigned int>( iProp, "Shader Flags 2" );

				// Get all textures for shader property
				auto textures = nif->getArray<QString>( bslsp->getTextureSet(), "Textures" );

				bslsp->setShaderType( shaderType );
				bslsp->setFlags1( sf1 );
				bslsp->setFlags2( sf2 );

				bslsp->setAlpha( nif->get<float>( iProp, "Alpha" ) );

				depthTest = hasSF1( ShaderFlags::SLSF1_ZBuffer_Test );
				depthWrite = hasSF2( ShaderFlags::SLSF2_ZBuffer_Write );

				// Mesh alpha override
				translucent = (bslsp->getAlpha() < 1.0);

				// Draw mesh second
				drawSecond |= translucent;

				// Specular
				if ( hasSF1( ShaderFlags::SLSF1_Specular ) ) {
					auto spC = nif->get<Color3>( iProp, "Specular Color" );
					auto spG = nif->get<float>( iProp, "Glossiness" );
					auto spS = nif->get<float>( iProp, "Specular Strength" );
					bslsp->setSpecular( spC, spG, spS );
				} else {
					bslsp->setSpecular( Color3(0, 0, 0), 0, 0 );
				}

				// Emissive
				auto emC = nif->get<Color3>( iProp, "Emissive Color" );
				auto emM = nif->get<float>( iProp, "Emissive Multiple" );
				bslsp->setEmissive( emC, emM );

				bslsp->hasEmittance = hasSF1( ShaderFlags::SLSF1_Own_Emit );

				// Set glow map if shader meets requirements
				if ( bslsp->getShaderType() & ShaderFlags::ST_GlowShader ) {
					bslsp->hasGlowMap = hasSF2( ShaderFlags::SLSF2_Glow_Map ) && !textures.value( 2, "" ).isEmpty();
				}

				bslsp->hasVertexAlpha = hasSF1( ShaderFlags::SLSF1_Vertex_Alpha );
				bslsp->hasVertexColors = hasSF2( ShaderFlags::SLSF2_Vertex_Colors );
				bslsp->hasBacklight = hasSF2( ShaderFlags::SLSF2_Back_Lighting );
				bslsp->hasRimlight  = hasSF2( ShaderFlags::SLSF2_Rim_Lighting );
				bslsp->hasSoftlight = hasSF2( ShaderFlags::SLSF2_Soft_Lighting );
				bslsp->hasModelSpaceNormals = hasSF1( ShaderFlags::SLSF1_Model_Space_Normals );
				bslsp->hasSpecularMap = hasSF1( ShaderFlags::SLSF1_Specular ) && !textures.value( 7, "" ).isEmpty();
				bslsp->hasMultiLayerParallax = hasSF2( ShaderFlags::SLSF2_Multi_Layer_Parallax );

				bslsp->hasEnvironmentMap = isST( ShaderFlags::ST_EnvironmentMap ) && hasSF1( ShaderFlags::SLSF1_Environment_Mapping );
				bslsp->hasEnvironmentMap |= isST( ShaderFlags::ST_EyeEnvmap ) && hasSF1( ShaderFlags::SLSF1_Eye_Environment_Mapping );
				bslsp->hasEnvironmentMap |= bslsp->hasMultiLayerParallax;
				bslsp->hasCubeMap = (
						isST( ShaderFlags::ST_EnvironmentMap ) 
						|| isST( ShaderFlags::ST_EyeEnvmap )
						|| isST( ShaderFlags::ST_MultiLayerParallax )
					)
					&& bslsp->hasEnvironmentMap
					&& !textures.value( 4, "" ).isEmpty();

				bslsp->useEnvironmentMask = bslsp->hasEnvironmentMap && !textures.value( 5, "" ).isEmpty();

				bslsp->hasHeightMap = isST( ShaderFlags::ST_Heightmap );
				bslsp->hasHeightMap |= hasSF1( ShaderFlags::SLSF1_Parallax ) && !textures.value( 3, "" ).isEmpty();

				bslsp->hasRefraction = hasSF1( ShaderFlags::SLSF1_Refraction );
				bslsp->hasFireRefraction = hasSF1( ShaderFlags::SLSF1_Fire_Refraction );

				bslsp->hasTintColor = false;
				bslsp->hasTintMask = isST( ShaderFlags::ST_FaceTint );
				bslsp->hasDetailMask = bslsp->hasTintMask;

				QString tint;
				if ( isST( ShaderFlags::ST_HairTint ) )
					tint = "Hair Tint Color";
				else if ( isST( ShaderFlags::ST_SkinTint ) )
					tint = "Skin Tint Color";

				if ( !tint.isEmpty() ) {
					bslsp->hasTintColor = true;
					bslsp->setTintColor( nif->get<Color3>( iProp, tint ) );
				}

				// Mesh alpha override
				translucent |= bslsp->hasRefraction;


				auto le1 = nif->get<float>( iProp, "Lighting Effect 1" );
				auto le2 = nif->get<float>( iProp, "Lighting Effect 2" );

				bslsp->setLightingEffect1( le1 );
				bslsp->setLightingEffect2( le2 );

				auto uvScale = nif->get<Vector2>( iProp, "UV Scale" );
				auto uvOffset = nif->get<Vector2>( iProp, "UV Offset" );

				bslsp->setUvScale( uvScale[0], uvScale[1] );
				bslsp->setUvOffset( uvOffset[0], uvOffset[1] );

				auto clampMode = nif->get<uint>( iProp, "Texture Clamp Mode" );

				bslsp->setClampMode( clampMode );

				float envReflection = 0;
				if ( isST( ShaderFlags::ST_EnvironmentMap ) ) {
					envReflection = nif->get<float>( iProp, "Environment Map Scale" );
				} else if ( isST( ShaderFlags::ST_EyeEnvmap ) ) {
					envReflection = nif->get<float>( iProp, "Eye Cubemap Scale" );
				}

				bslsp->setEnvironmentReflection( envReflection );

				auto innerThickness = nif->get<float>( iProp, "Parallax Inner Layer Thickness" );
				auto innerScale = nif->get<Vector2>( iProp, "Parallax Inner Layer Texture Scale" );
				auto outerRefraction = nif->get<float>( iProp, "Parallax Refraction Scale" );
				auto outerReflection = nif->get<float>( iProp, "Parallax Envmap Strength" );

				bslsp->setInnerThickness( innerThickness );
				bslsp->setInnerTextureScale( innerScale[0], innerScale[1] );
				bslsp->setOuterRefractionStrength( outerRefraction );
				bslsp->setOuterReflectionStrength( outerReflection );
				
				isDoubleSided = hasSF2( ShaderFlags::SLSF2_Double_Sided );
				
				isVertexAlphaAnimation = hasSF2( ShaderFlags::SLSF2_Tree_Anim );
				//break;

			} else {
				iProp = nif->getBlock( props[i], "BSEffectShaderProperty" );
	
				if ( iProp.isValid() ) {
					if ( !bsesp )
						bsesp = properties.get<BSEffectShaderProperty>();

					if ( !bsesp )
						break;

					auto hasSF1 = [this]( ShaderFlags::SF1 flag ) {
						return bsesp->getFlags1() & flag;
					};

					auto hasSF2 = [this]( ShaderFlags::SF2 flag ) {
						return bsesp->getFlags2() & flag;
					};

					auto sf1 = nif->get<unsigned int>( iProp, "Shader Flags 1" );
					auto sf2 = nif->get<unsigned int>( iProp, "Shader Flags 2" );

					bsesp->setFlags1( sf1 );
					bsesp->setFlags2( sf2 );

					depthTest = hasSF1( ShaderFlags::SLSF1_ZBuffer_Test );
					depthWrite = hasSF2( ShaderFlags::SLSF2_ZBuffer_Write );

					auto emC = nif->get<Color4>( iProp, "Emissive Color" );
					auto emM = nif->get<float>( iProp, "Emissive Multiple" );
					bsesp->setEmissive( emC, emM );

					// For BSESP, let Alpha prop handle things
					bool hasAlphaProp = findProperty<AlphaProperty>();

					// Mesh alpha override
					translucent = (bsesp->getAlpha() < 1.0) && !hasAlphaProp;

					// Draw mesh second
					drawSecond |= translucent;

					bsesp->hasSourceTexture = !nif->get<QString>( iProp, "Source Texture" ).isEmpty();
					bsesp->hasGreyscaleMap = !nif->get<QString>( iProp, "Greyscale Texture" ).isEmpty();
					bsesp->greyscaleAlpha = hasSF1( ShaderFlags::SLSF1_Greyscale_To_PaletteAlpha );
					bsesp->greyscaleColor = hasSF1( ShaderFlags::SLSF1_Greyscale_To_PaletteColor );

					bsesp->useFalloff = hasSF1( ShaderFlags::SLSF1_Use_Falloff );

					bsesp->vertexAlpha = hasSF1( ShaderFlags::SLSF1_Vertex_Alpha );
					bsesp->vertexColors = hasSF2( ShaderFlags::SLSF2_Vertex_Colors );

					bsesp->hasWeaponBlood = hasSF2( ShaderFlags::SLSF2_Weapon_Blood );

					auto uvScale = nif->get<Vector2>( iProp, "UV Scale" );
					auto uvOffset = nif->get<Vector2>( iProp, "UV Offset" );

					bsesp->setUvScale( uvScale[0], uvScale[1] );
					bsesp->setUvOffset( uvOffset[0], uvOffset[1] );

					auto clampMode = nif->get<uint>( iProp, "Texture Clamp Mode" );

					bsesp->setClampMode( clampMode );

					auto startA = nif->get<float>( iProp, "Falloff Start Angle" );
					auto stopA = nif->get<float>( iProp, "Falloff Stop Angle" );
					auto startO = nif->get<float>( iProp, "Falloff Start Opacity" );
					auto stopO = nif->get<float>( iProp, "Falloff Stop Opacity" );
					auto soft = nif->get<float>( iProp, "Soft Falloff Depth" );

					bsesp->setFalloff( startA, stopA, startO, stopO, soft );

					isDoubleSided = hasSF2( ShaderFlags::SLSF2_Double_Sided );

					bsesp->doubleSided = isDoubleSided;
				}
			}
		}
	}


	if ( iBlock == index ) {
		// NiMesh presents a problem because we are almost guaranteed to have multiple "data" blocks
		// for eg. vertices, indices, normals, texture data etc.
#ifndef QT_NO_DEBUG

		if ( nif->checkVersion( 0x14050000, 0 ) && nif->inherits( iBlock, "NiMesh" ) ) {
			qDebug() << nif->get<ushort>( iBlock, "Num Submeshes" ) << " submeshes";
			iData = nif->getIndex( iBlock, "Datas" );

			if ( iData.isValid() ) {
				qDebug() << "Got " << nif->rowCount( iData ) << " rows of data";
				updateData = true;
			} else {
				qDebug() << "Did not find data in NiMesh ???";
			}

			return;
		}

#endif

		for ( const auto link : nif->getChildLinks( id() ) ) {
			QModelIndex iChild = nif->getBlock( link );

			if ( !iChild.isValid() )
				continue;

			QString name = nif->itemName( iChild );

			if ( nif->inherits( iChild, "NiTriShapeData" ) || nif->inherits( iChild, "NiTriStripsData" ) ) {
				if ( !iData.isValid() ) {
					iData  = iChild;
					updateData = true;
				} else if ( iData != iChild ) {
					Message::append( tr( "Warnings were generated while updating meshes." ),
						tr( "Block %1 has multiple data blocks" ).arg( id() )
					);
				}
			} else if ( nif->inherits( iChild, "NiSkinInstance" ) ) {
				if ( !iSkin.isValid() ) {
					iSkin  = iChild;
					updateSkin = true;
				} else if ( iSkin != iChild ) {
					Message::append( tr( "Warnings were generated while updating meshes." ),
						tr( "Block %1 has multiple skin instances" ).arg( id() )
					);
				}
			}
		}
	}

	updateBounds |= updateData;
}


bool Mesh::isHidden() const
{
	return ( Node::isHidden()
	         || ( !(scene->options & Scene::ShowHidden) /*&& Options::onlyTextured()*/
	              && !properties.get<TexturingProperty>()
	              && !properties.get<BSShaderLightingProperty>()
	         )
	);
}

bool compareTriangles( const QPair<int, float> & tri1, const QPair<int, float> & tri2 )
{
	return ( tri1.second < tri2.second );
}

void Mesh::transform()
{
	const NifModel * nif = static_cast<const NifModel *>( iBlock.model() );

	if ( !nif || !iBlock.isValid() ) {
		clear();
		return;
	}

	if ( updateData ) {
		updateData = false;

		// update for NiMesh
		if ( nif->checkVersion( 0x14050000, 0 ) && nif->inherits( iBlock, "NiMesh" ) ) {
#ifndef QT_NO_DEBUG
			// do stuff
			qDebug() << "Entering NiMesh decoding...";
			// mesh primitive type
			QString meshPrimitiveType = NifValue::enumOptionName( "MeshPrimitiveType", nif->get<uint>( iData, "Primitive Type" ) );
			qDebug() << "Mesh uses" << meshPrimitiveType;

			for ( int i = 0; i < nif->rowCount( iData ); i++ ) {
				// each data reference is to a single data stream
				quint32 stream = nif->getLink( iData.child( i, 0 ), "Stream" );
				qDebug() << "Data stream: " << stream;
				// can have multiple submeshes, unsure of exact meaning
				ushort numSubmeshes = nif->get<ushort>( iData.child( i, 0 ), "Num Submeshes" );
				qDebug() << "Submeshes: " << numSubmeshes;
				QPersistentModelIndex submeshMap = nif->getIndex( iData.child( i, 0 ), "Submesh To Region Map" );

				for ( int j = 0; j < numSubmeshes; j++ ) {
					qDebug() << "Submesh" << j << "maps to region" << nif->get<ushort>( submeshMap.child( j, 0 ) );
				}

				// each stream can have multiple components, and each has a starting index
				QMap<uint, QString> componentIndexMap;
				int numComponents = nif->get<int>( iData.child( i, 0 ), "Num Components" );
				qDebug() << "Components: " << numComponents;
				// semantics determine the usage
				QPersistentModelIndex componentSemantics = nif->getIndex( iData.child( i, 0 ), "Component Semantics" );

				for ( int j = 0; j < numComponents; j++ ) {
					QString name = nif->get<QString>( componentSemantics.child( j, 0 ), "Name" );
					uint index = nif->get<uint>( componentSemantics.child( j, 0 ), "Index" );
					qDebug() << "Component" << name << "at position" << index << "of component" << j << "in stream" << stream;
					componentIndexMap.insert( j, QString( "%1 %2" ).arg( name ).arg( index ) );
				}

				// now the data stream itself...
				QPersistentModelIndex dataStream = nif->getBlock( stream );
				QByteArray streamData = nif->get<QByteArray>( nif->getIndex( dataStream, "Data" ).child( 0, 0 ) );
				QBuffer streamBuffer( &streamData );
				streamBuffer.open( QIODevice::ReadOnly );
				// probably won't use this
				QDataStream streamReader( &streamData, QIODevice::ReadOnly );
				// we should probably check the header here, but we expect things to be little endian
				streamReader.setByteOrder( QDataStream::LittleEndian );
				// each region exists within the data stream at the specified index
				quint32 numRegions = nif->get<quint32>( dataStream, "Num Regions" );
				QPersistentModelIndex regions = nif->getIndex( dataStream, "Regions" );
				quint32 totalIndices = 0;

				if ( regions.isValid() ) {
					qDebug() << numRegions << " regions in this stream";

					for ( quint32 j = 0; j < numRegions; j++ ) {
						qDebug() << "Start index: " << nif->get<quint32>( regions.child( j, 0 ), "Start Index" );
						qDebug() << "Num indices: " << nif->get<quint32>( regions.child( j, 0 ), "Num Indices" );
						totalIndices += nif->get<quint32>( regions.child( j, 0 ), "Num Indices" );
					}

					qDebug() << totalIndices << "total indices in" << numRegions << "regions";
				}

				uint numStreamComponents = nif->get<uint>( dataStream, "Num Components" );
				qDebug() << "Stream has" << numStreamComponents << "components";
				QPersistentModelIndex streamComponents = nif->getIndex( dataStream, "Component Formats" );
				// stream components are interleaved, so we need to know their type before we read them
				QList<uint> typeList;

				for ( uint j = 0; j < numStreamComponents; j++ ) {
					uint compFormat  = nif->get<uint>( streamComponents.child( j, 0 ) );
					QString compName = NifValue::enumOptionName( "ComponentFormat", compFormat );
					qDebug() << "Component format is" << compName;
					qDebug() << "Stored as a" << compName.split( "_" )[1];
					typeList.append( compFormat - 1 );

					// this can probably wait until we're reading the stream values
					QString compNameIndex = componentIndexMap.value( j );
					QString compType = compNameIndex.split( " " )[0];
					uint startIndex  = compNameIndex.split( " " )[1].toUInt();
					qDebug() << "Component" << j << "contains" << compType << "starting at index" << startIndex;

					// try and sort out texcoords here...
					if ( compType == "TEXCOORD" ) {
						QVector<Vector2> tempCoords;
						coords.append( tempCoords );
						qDebug() << "Assigning coordinate set" << startIndex;
					}
				}

				// for each component
				// get the length
				// get the underlying type (will probably need OpenEXR to deal with float16 types)
				// read that type in, k times, where k is the length of the vector
				// start index will not be 0 if eg. multiple UV maps, but hopefully we don't have multiple components

				for ( uint j = 0; j < totalIndices; j++ ) {
					for ( uint k = 0; k < numStreamComponents; k++ ) {
						int typeLength = ( ( typeList[k] & 0x000F0000 ) >> 0x10 );
						int typeSize = ( ( typeList[k] & 0x00000F00 ) >> 0x08 );
						qDebug() << "Reading" << typeLength << "values" << typeSize << "bytes";

						NifIStream tempInput( new NifModel, &streamBuffer );
						QList<NifValue> values;
						NifValue tempValue;

						// if we had the right types, we could read in Vector etc. and not have the mess below
						switch ( ( typeList[k] & 0x00000FF0 ) >> 0x04 ) {
						case 0x10:
							tempValue.changeType( NifValue::tByte );
							break;
						case 0x21:
							tempValue.changeType( NifValue::tShort );
							break;
						case 0x42:
							tempValue.changeType( NifValue::tInt );
							break;
						case 0x43:
							tempValue.changeType( NifValue::tFloat );
							break;
						}

						for ( int l = 0; l < typeLength; l++ ) {
							tempInput.read( tempValue );
							values.append( tempValue );
							qDebug() << tempValue.toString();
						}

						QString compType = componentIndexMap.value( k ).split( " " )[0];
						qDebug() << "Will store this value in" << compType;

						// the mess begins...
						if ( NifValue::enumOptionName( "ComponentFormat", (typeList[k] + 1 ) ) == "F_FLOAT32_3" ) {
							Vector3 tempVect3( values[0].toFloat(), values[1].toFloat(), values[2].toFloat() );

							if ( compType == "POSITION" ) {
								verts.append( tempVect3 );
							} else if ( compType == "NORMAL" ) {
								norms.append( tempVect3 );
							}
						} else if ( compType == "INDEX" ) {
							indices.append( values[0].toCount() );
						} else if ( compType == "TEXCOORD" ) {
							Vector2 tempVect2( values[0].toFloat(), values[1].toFloat() );
							quint32 coordSet = componentIndexMap.value( k ).split( " " )[1].toUInt();
							qDebug() << "Need to append" << tempVect2 << "to texcoords" << coordSet;
							QVector<Vector2> currentSet = coords[coordSet];
							currentSet.append( tempVect2 );
							coords[coordSet] = currentSet;
						}
					}
				}

				// build triangles, strips etc.
				if ( meshPrimitiveType == "MESH_PRIMITIVE_TRIANGLES" ) {
					for ( int k = 0; k < indices.size(); ) {
						Triangle tempTri( indices[k], indices[k + 1], indices[k + 2] );
						qDebug() << "Inserting triangle" << tempTri;
						triangles.append( tempTri );
						k = k + 3;
					}
				}
			}

#endif
		} else {

			verts  = nif->getArray<Vector3>( iData, "Vertices" );
			norms  = nif->getArray<Vector3>( iData, "Normals" );
			colors = nif->getArray<Color4>( iData, "Vertex Colors" );

			// Detect if "Has Vertex Colors" is set to Yes in NiTriShape
			//	Used to compare against SLSF2_Vertex_Colors
			hasVertexColors = true;
			if ( colors.length() == 0 ) {
				hasVertexColors = false;
			}

			if ( isVertexAlphaAnimation ) {
				for ( int i = 0; i < colors.count(); i++ )
					colors[i].setRGBA( colors[i].red(), colors[i].green(), colors[i].blue(), 1 );
			}

			tangents   = nif->getArray<Vector3>( iData, "Tangents" );
			bitangents = nif->getArray<Vector3>( iData, "Bitangents" );

			if ( norms.count() < verts.count() )
				norms.clear();

			if ( colors.count() < verts.count() )
				colors.clear();

			coords.clear();
			QModelIndex uvcoord = nif->getIndex( iData, "UV Sets" );

			if ( !uvcoord.isValid() )
				uvcoord = nif->getIndex( iData, "UV Sets 2" );

			if ( uvcoord.isValid() ) {
				for ( int r = 0; r < nif->rowCount( uvcoord ); r++ ) {
					QVector<Vector2> tc = nif->getArray<Vector2>( uvcoord.child( r, 0 ) );

					if ( tc.count() < verts.count() )
						tc.clear();

					coords.append( tc );
				}
			}

			if ( nif->itemName( iData ) == "NiTriShapeData" ) {
				// check indexes
				// TODO: check other indexes as well
				QVector<Triangle> ftriangles = nif->getArray<Triangle>( iData, "Triangles" );
				triangles.clear();
				int inv_idx = 0;
				int inv_cnt = 0;

				for ( int i = 0; i < ftriangles.count(); i++ ) {
					Triangle t = ftriangles[i];
					inv_idx = 0;

					for ( int j = 0; j < 3; j++ ) {
						if ( t[j] >= verts.count() ) {
							inv_idx = 1;
							break;
						}
					}

					if ( !inv_idx )
						triangles.append( t );
				}

				inv_cnt = ftriangles.count() - triangles.count();
				ftriangles.clear();

				if ( inv_cnt > 0 ) {
					int block_idx = nif->getBlockNumber( nif->getIndex( iData, "Triangles" ) );
					Message::append( tr( "Warnings were generated while rendering mesh." ),
						tr( "Block %1: %2 invalid indices in NiTriShapeData.Triangles" ).arg( block_idx ).arg( inv_cnt )
					);
				}

				tristrips.clear();
			} else if ( nif->itemName( iData ) == "NiTriStripsData" ) {
				tristrips.clear();
				QModelIndex points = nif->getIndex( iData, "Points" );

				if ( points.isValid() ) {
					for ( int r = 0; r < nif->rowCount( points ); r++ )
						tristrips.append( nif->getArray<quint16>( points.child( r, 0 ) ) );
				} else {
					Message::append( tr( "Warnings were generated while rendering mesh." ),
						tr( "Block %1: Invalid 'Points' array in %2" )
						.arg( nif->getBlockNumber( iData ) )
						.arg( nif->itemName( iData ) )
					);
				}

				triangles.clear();
			} else {
				triangles.clear();
				tristrips.clear();
			}

			QModelIndex iExtraData = nif->getIndex( iBlock, "Extra Data List" );

			if ( iExtraData.isValid() ) {
				for ( int e = 0; e < nif->rowCount( iExtraData ); e++ ) {
					QModelIndex iExtra = nif->getBlock( nif->getLink( iExtraData.child( e, 0 ) ), "NiBinaryExtraData" );

					if ( nif->get<QString>( iExtra, "Name" ) == "Tangent space (binormal & tangent vectors)" ) {
						iTangentData = iExtra;
						QByteArray data = nif->get<QByteArray>( iExtra, "Binary Data" );

						if ( data.count() == verts.count() * 4 * 3 * 2 ) {
							tangents.resize( verts.count() );
							bitangents.resize( verts.count() );
							Vector3 * t = (Vector3 *)data.data();

							for ( int c = 0; c < verts.count(); c++ )
								tangents[c] = *t++;

							for ( int c = 0; c < verts.count(); c++ )
								bitangents[c] = *t++;
						}
					}
				}
			}
		}
	}

	if ( updateSkin ) {
		updateSkin = false;
		weights.clear();
		partitions.clear();

		iSkinData = nif->getBlock( nif->getLink( iSkin, "Data" ), "NiSkinData" );

		skeletonRoot  = nif->getLink( iSkin, "Skeleton Root" );
		skeletonTrans = Transform( nif, iSkinData );

		bones = nif->getLinkArray( iSkin, "Bones" );

		QModelIndex idxBones = nif->getIndex( iSkinData, "Bone List" );
		unsigned char hvw = nif->get<unsigned char>( iSkinData, "Has Vertex Weights" );
		int vcnt = hvw ? 0 : verts.count();

		if ( idxBones.isValid() /*&& hvw*/ ) {
			for ( int b = 0; b < nif->rowCount( idxBones ) && b < bones.count(); b++ ) {
				weights.append( BoneWeights( nif, idxBones.child( b, 0 ), bones[ b ], vcnt ) );
			}
		}

		iSkinPart = nif->getBlock( nif->getLink( iSkin, "Skin Partition" ), "NiSkinPartition" );

		if ( !iSkinPart.isValid() )
			// nif versions < 10.2.0.0 have skin partition linked in the skin data block
			iSkinPart = nif->getBlock( nif->getLink( iSkinData, "Skin Partition" ), "NiSkinPartition" );

		if ( iSkinPart.isValid() ) {
			QModelIndex idx = nif->getIndex( iSkinPart, "Skin Partition Blocks" );

			for ( int i = 0; i < nif->rowCount( idx ) && idx.isValid(); i++ ) {
				partitions.append( SkinPartition( nif, idx.child( i, 0 ) ) );
			}
		}
	}

	Node::transform();
}

void Mesh::transformShapes()
{
	if ( isHidden() )
		return;

	Node::transformShapes();

	transformRigid = true;

	if ( weights.count() ) {
		transformRigid = false;

		transVerts.resize( verts.count() );
		transVerts.fill( Vector3() );
		transNorms.resize( norms.count() );
		transNorms.fill( Vector3() );
		transTangents.resize( tangents.count() );
		transTangents.fill( Vector3() );
		transBitangents.resize( bitangents.count() );
		transBitangents.fill( Vector3() );

		Node * root = findParent( skeletonRoot );

		if ( partitions.count() ) {
			foreach ( const SkinPartition part, partitions ) {
				QVector<Transform> boneTrans( part.boneMap.count() );

				for ( int t = 0; t < boneTrans.count(); t++ ) {
					Node * bone = root ? root->findChild( bones.value( part.boneMap[t] ) ) : 0;
					boneTrans[ t ] = viewTrans() * skeletonTrans;

					if ( bone )
						boneTrans[ t ] = boneTrans[ t ] * bone->localTrans( skeletonRoot ) * weights.value( part.boneMap[t] ).trans;

					//if ( bone ) boneTrans[ t ] = bone->viewTrans() * weights.value( part.boneMap[t] ).trans;
				}

				for ( int v = 0; v < part.vertexMap.count(); v++ ) {
					int vindex = part.vertexMap[ v ];

					if ( vindex < 0 || vindex > transVerts.count() )
						break;

					if ( transVerts[vindex] == Vector3() ) {
						for ( int w = 0; w < part.numWeightsPerVertex; w++ ) {
							QPair<int, float> weight = part.weights[ v * part.numWeightsPerVertex + w ];
							Transform trans = boneTrans.value( weight.first );

							if ( verts.count() > vindex )
								transVerts[vindex] += trans * verts[ vindex ] * weight.second;

							if ( norms.count() > vindex )
								transNorms[vindex] += trans.rotation * norms[ vindex ] * weight.second;

							if ( tangents.count() > vindex )
								transTangents[vindex] += trans.rotation * tangents[ vindex ] * weight.second;

							if ( bitangents.count() > vindex )
								transBitangents[vindex] += trans.rotation * bitangents[ vindex ] * weight.second;
						}
					}
				}
			}
		} else {
			int x = 0;
			foreach ( const BoneWeights bw, weights ) {
				Transform trans = viewTrans() * skeletonTrans;
				Node * bone = root ? root->findChild( bw.bone ) : 0;

				if ( bone )
					trans = trans * bone->localTrans( skeletonRoot ) * bw.trans;

				if ( bone )
					weights[x++].tcenter = bone->viewTrans() * bw.center;
				else
					x++;

				Matrix natrix = trans.rotation;
				for ( const VertexWeight& vw : bw.weights ) {
					if ( transVerts.count() > vw.vertex )
						transVerts[ vw.vertex ] += trans * verts[ vw.vertex ] * vw.weight;

					if ( transNorms.count() > vw.vertex )
						transNorms[ vw.vertex ] += natrix * norms[ vw.vertex ] * vw.weight;

					if ( transTangents.count() > vw.vertex )
						transTangents[ vw.vertex ] += natrix * tangents[ vw.vertex ] * vw.weight;

					if ( transBitangents.count() > vw.vertex )
						transBitangents[ vw.vertex ] += natrix * bitangents[ vw.vertex ] * vw.weight;
				}
			}
		}

		for ( int n = 0; n < transNorms.count(); n++ )
			transNorms[n].normalize();

		for ( int t = 0; t < transTangents.count(); t++ )
			transTangents[t].normalize();

		for ( int t = 0; t < transBitangents.count(); t++ )
			transBitangents[t].normalize();

		boundSphere = BoundSphere( transVerts );
		boundSphere.applyInv( viewTrans() );
		updateBounds = false;
	} else {
		transVerts = verts;
		transNorms = norms;
		transTangents = tangents;
		transBitangents = bitangents;
	}

	/*
	//Commented this out because this appears from my tests to be an
	//incorrect understanding of the flag we previously called "sort."
	//Tests have shown that none of the games or official scene viewers
	//ever sort triangles, regarless of the path.  The triangles are always
	//drawn in the order they exist in the triangle array.

	AlphaProperty * alphaprop = findProperty<AlphaProperty>();

	if ( alphaprop && alphaprop->sort() )
	{



	    QVector< QPair< int, float > > triOrder( triangles.count() );
	    if ( transformRigid )
	    {
	        Transform vt = viewTrans();
	        Vector3 ref = vt.rotation.inverted() * ( Vector3() - vt.translation ) / vt.scale;
	        int t = 0;
	        foreach ( Triangle tri, triangles )
	        {
	            triOrder[t] = QPair<int, float>( t, 0 - ( ref - verts.value( tri.v1() ) ).squaredLength() - ( ref - verts.value( tri.v2() ) ).squaredLength() - ( ref - verts.value( tri.v1() ) ).squaredLength() );
	            t++;
	        }
	    }
	    else
	    {
	        int t = 0;
	        foreach ( Triangle tri, triangles )
	        {
	            QPair< int, float > tp;
	            tp.first = t;
	            tp.second = transVerts.value( tri.v1() )[2] + transVerts.value( tri.v2() )[2] + transVerts.value( tri.v3() )[2];
	            triOrder[t++] = tp;
	        }
	    }
	    qSort( triOrder.begin(), triOrder.end(), compareTriangles );
	    sortedTriangles.resize( triangles.count() );
	    for ( int t = 0; t < triangles.count(); t++ )
	        sortedTriangles[t] = triangles[ triOrder[t].first ];
	}
	else
	{
	*/
	sortedTriangles = triangles;
	//}

	MaterialProperty * matprop = findProperty<MaterialProperty>();
	if ( matprop && matprop->alphaValue() != 1.0 ) {
		float a = matprop->alphaValue();
		transColors.resize( colors.count() );

		for ( int c = 0; c < colors.count(); c++ )
			transColors[c] = colors[c].blend( a );
	} else {
		transColors = colors;
	}

	//if ( !bslsp )
	//	bslsp = properties.get<BSLightingShaderProperty>();

	if ( bslsp ) {
		transColorsNoAlpha.resize( colors.count() );
		if ( !(bslsp->getFlags1() & ShaderFlags::SLSF1_Vertex_Alpha) ) {
			for ( int c = 0; c < colors.count(); c++ )
				transColorsNoAlpha[c] = Color4( transColors[c].red(), transColors[c].green(), transColors[c].blue(), 1.0f );
		} else {
			transColorsNoAlpha.clear();
		}
	}

	//if ( !bsesp )
	//	bsesp = properties.get<BSEffectShaderProperty>();

	//if ( bsesp ) {
	//	transColorsNoAlpha.resize( colors.count() );
	//	if ( !(bsesp->getFlags1() & ShaderFlags::SLSF1_Vertex_Alpha) ) {
	//		//qDebug() << "No VA" << name;
	//		for ( int c = 0; c < colors.count(); c++ )
	//			transColorsNoAlpha[c] = Color4( transColors[c].red(), transColors[c].green(), transColors[c].blue(), 1.0f );
	//	} else {
	//		transColorsNoAlpha.clear();
	//	}
	//}
}

BoundSphere Mesh::bounds() const
{
	if ( updateBounds ) {
		updateBounds = false;
		boundSphere = BoundSphere( verts );
	}

	return worldTrans() * boundSphere;
}

void Mesh::drawShapes( NodeList * secondPass, bool presort )
{
	if ( isHidden() )
		return;

	// TODO: Only run this if BSXFlags has "EditorMarkers present" flag
	if ( !(scene->options & Scene::ShowMarkers) && name.startsWith( "EditorMarker" ) )
		return;

	auto nif = static_cast<const NifModel *>(iBlock.model());
	
	if ( Node::SELECTING ) {
		int s_nodeId = ID2COLORKEY( nodeId );
		glColor4ubv( (GLubyte *)&s_nodeId );
	}

	// BSOrderedNode
	presorted |= presort;

	// Draw translucent meshes in second pass

	AlphaProperty * aprop = findProperty<AlphaProperty>();

	drawSecond |= (aprop && aprop->blend());

	if ( secondPass && drawSecond ) {
		secondPass->add( this );
		return;
	}

	// TODO: Option to hide Refraction and other post effects

	// rigid mesh? then pass the transformation on to the gl layer

	if ( transformRigid ) {
		glPushMatrix();
		glMultMatrix( viewTrans() );
	}

	//if ( !Node::SELECTING ) {
	//	qDebug() << viewTrans().translation;
		//qDebug() << Vector3( nif->get<Vector4>( iBlock, "Translation" ) );
	//}

	// Debug axes
	//drawAxes(Vector3(), 35.0);

	// setup array pointers

	// Render polygon fill slightly behind alpha transparency and wireframe
	glEnable( GL_POLYGON_OFFSET_FILL );
	glPolygonOffset( 1.0f, 2.0f );

	glEnableClientState( GL_VERTEX_ARRAY );
	glVertexPointer( 3, GL_FLOAT, 0, transVerts.data() );

	if ( !Node::SELECTING ) {
		if ( transNorms.count() ) {
			glEnableClientState( GL_NORMAL_ARRAY );
			glNormalPointer( GL_FLOAT, 0, transNorms.data() );
		}

		// Do VCs if legacy or if either bslsp or bsesp is set
		bool doVCs = (!bslsp && !bsesp) 
			|| (bslsp && (bslsp->getFlags2() & ShaderFlags::SLSF2_Vertex_Colors))
			|| (bsesp && (bsesp->getFlags2() & ShaderFlags::SLSF2_Vertex_Colors));

		if ( transColors.count()
			&& ( scene->options & Scene::DoVertexColors )
			&& doVCs )
		{
			glEnableClientState( GL_COLOR_ARRAY );
			glColorPointer( 4, GL_FLOAT, 0, (transColorsNoAlpha.count()) ? transColorsNoAlpha.data() : transColors.data() );
		} else {
			if ( !hasVertexColors && (bslsp && bslsp->hasVertexColors) ) {
				// Correctly blacken the mesh if SLSF2_Vertex_Colors is still on
				//	yet "Has Vertex Colors" is not.
				glColor( Color3( 0.0f, 0.0f, 0.0f ) );
			} else {
				glColor( Color3( 1.0f, 1.0f, 1.0f ) );
			}
		}
	}

	// TODO: Hotspot.  See about optimizing this.
	if ( !Node::SELECTING )
		shader = scene->renderer->setupProgram( this, shader );

	if ( isDoubleSided ) {
		glDisable( GL_CULL_FACE );
	}

	auto bsLOD = nif->getBlock( iBlock, "BSLODTriShape" );
	if ( !bsLOD.isValid() ) {

		// render the triangles
		if ( sortedTriangles.count() )
			glDrawElements( GL_TRIANGLES, sortedTriangles.count() * 3, GL_UNSIGNED_SHORT, sortedTriangles.data() );

	} else {
		auto lod0 = nif->get<uint>( bsLOD, "Level 0 Size" );
		auto lod1 = nif->get<uint>( bsLOD, "Level 1 Size" );
		auto lod2 = nif->get<uint>( bsLOD, "Level 2 Size" );

		auto lod0tris = sortedTriangles.mid( 0, lod0 );
		auto lod1tris = sortedTriangles.mid( lod0, lod1 );
		auto lod2tris = sortedTriangles.mid( lod0 + lod1, lod2 );

		// If Level2, render all
		// If Level1, also render Level0
		switch ( scene->lodLevel ) {
		case Scene::Level2:
			if ( lod2tris.count() )
				glDrawElements( GL_TRIANGLES, lod2tris.count() * 3, GL_UNSIGNED_SHORT, lod2tris.data() );
		case Scene::Level1:
			if ( lod1tris.count() )
				glDrawElements( GL_TRIANGLES, lod1tris.count() * 3, GL_UNSIGNED_SHORT, lod1tris.data() );
		case Scene::Level0:
		default:
			if ( lod0tris.count() )
				glDrawElements( GL_TRIANGLES, lod0tris.count() * 3, GL_UNSIGNED_SHORT, lod0tris.data() );
			break;
		}
	}

	// render the tristrips
	for ( int s = 0; s < tristrips.count(); s++ )
		glDrawElements( GL_TRIANGLE_STRIP, tristrips[s].count(), GL_UNSIGNED_SHORT, tristrips[s].data() );

	if ( isDoubleSided ) {
		glEnable( GL_CULL_FACE );
	}

	if ( !Node::SELECTING )
		scene->renderer->stopProgram();

	glDisableClientState( GL_VERTEX_ARRAY );
	glDisableClientState( GL_NORMAL_ARRAY );
	glDisableClientState( GL_COLOR_ARRAY );

	glDisable( GL_POLYGON_OFFSET_FILL );

	if ( transformRigid )
		glPopMatrix();
}

void Mesh::drawSelection() const
{
	if ( scene->options & Scene::ShowNodes )
		Node::drawSelection();

	if ( isHidden() )
		return;

	if ( scene->currentBlock != iBlock && scene->currentBlock != iData && scene->currentBlock != iSkinPart
	     && ( !iTangentData.isValid() || scene->currentBlock != iTangentData ) )
	{
		return;
	}


	if ( transformRigid ) {
		glPushMatrix();
		glMultMatrix( viewTrans() );
	}

	glDisable( GL_LIGHTING );
	glDisable( GL_COLOR_MATERIAL );
	glDisable( GL_TEXTURE_2D );
	glDisable( GL_NORMALIZE );
	glEnable( GL_DEPTH_TEST );
	glDepthMask( GL_FALSE );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glDisable( GL_ALPHA_TEST );

	glDisable( GL_CULL_FACE );

	glLineWidth( 1.0 );
	glPointSize( 3.5 );

	QString n;
	int i = -1;

	if ( scene->currentBlock == iBlock || scene->currentIndex == iData ) {
		n = "Faces";
	} else if ( scene->currentBlock == iData || scene->currentBlock == iSkinPart ) {
		n = scene->currentIndex.data( NifSkopeDisplayRole ).toString();

		QModelIndex iParent = scene->currentIndex.parent();

		if ( iParent.isValid() && iParent != iData ) {
			n = iParent.data( NifSkopeDisplayRole ).toString();
			i = scene->currentIndex.row();
		}
	} else if ( scene->currentBlock == iTangentData ) {
		n = "TSpace";
	}

	glDepthFunc( GL_LEQUAL );
	glNormalColor();

	glPolygonMode( GL_FRONT_AND_BACK, GL_POINT );

	// TODO: Reenable as an alternative to MSAA when MSAA is not supported
	//glEnable( GL_POINT_SMOOTH );
	//glHint( GL_POINT_SMOOTH_HINT, GL_NICEST );

	if ( n == "Vertices" || n == "Normals" || n == "Vertex Colors"
	     || n == "UV Sets" || n == "Tangents" || n == "Bitangents" )
	{
		glBegin( GL_POINTS );

		for ( int j = 0; j < transVerts.count(); j++ )
			glVertex( transVerts.value( j ) );

		glEnd();

		if ( i >= 0 ) {
			glDepthFunc( GL_ALWAYS );
			glHighlightColor();
			glBegin( GL_POINTS );
			glVertex( transVerts.value( i ) );
			glEnd();
		}
	}

	if ( n == "Points" ) {
		glBegin( GL_POINTS );
		const NifModel * nif = static_cast<const NifModel *>( iData.model() );
		QModelIndex points = nif->getIndex( iData, "Points" );

		if ( points.isValid() ) {
			for ( int j = 0; j < nif->rowCount( points ); j++ ) {
				QModelIndex iPoints = points.child( j, 0 );

				for ( int k = 0; k < nif->rowCount( iPoints ); k++ ) {
					glVertex( transVerts.value( nif->get<quint16>( iPoints.child( k, 0 ) ) ) );
				}
			}
		}

		glEnd();

		if ( i >= 0 ) {
			glDepthFunc( GL_ALWAYS );
			glHighlightColor();
			glBegin( GL_POINTS );
			QModelIndex iPoints = points.child( i, 0 );

			if ( nif->isArray( scene->currentIndex ) ) {
				for ( int j = 0; j < nif->rowCount( iPoints ); j++ ) {
					glVertex( transVerts.value( nif->get<quint16>( iPoints.child( j, 0 ) ) ) );
				}
			} else {
				iPoints = scene->currentIndex.parent();
				glVertex( transVerts.value( nif->get<quint16>( iPoints.child( i, 0 ) ) ) );
			}

			glEnd();
		}
	}

	glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

	// TODO: Reenable as an alternative to MSAA when MSAA is not supported
	//glEnable( GL_LINE_SMOOTH );
	//glHint( GL_LINE_SMOOTH_HINT, GL_NICEST );

	if ( n == "Normals" || n == "TSpace" ) {
		float normalScale = bounds().radius / 20;

		if ( normalScale < 0.1f )
			normalScale = 0.1f;

		glBegin( GL_LINES );

		for ( int j = 0; j < transVerts.count() && j < transNorms.count(); j++ ) {
			glVertex( transVerts.value( j ) );
			glVertex( transVerts.value( j ) + transNorms.value( j ) * normalScale );
		}

		if ( n == "TSpace" ) {
			for ( int j = 0; j < transVerts.count() && j < transTangents.count() && j < transBitangents.count(); j++ ) {
				glVertex( transVerts.value( j ) );
				glVertex( transVerts.value( j ) + transTangents.value( j ) * normalScale );
				glVertex( transVerts.value( j ) );
				glVertex( transVerts.value( j ) + transBitangents.value( j ) * normalScale );
			}
		}

		glEnd();

		if ( i >= 0 ) {
			glDepthFunc( GL_ALWAYS );
			glHighlightColor();
			glBegin( GL_LINES );
			glVertex( transVerts.value( i ) );
			glVertex( transVerts.value( i ) + transNorms.value( i ) * normalScale );
			glEnd();
		}
	}

	if ( n == "Tangents" ) {
		float normalScale = bounds().radius / 20;
		normalScale /= 2.0f;

		if ( normalScale < 0.1f )
			normalScale = 0.1f;

		glBegin( GL_LINES );

		for ( int j = 0; j < transVerts.count() && j < transTangents.count(); j++ ) {
			glVertex( transVerts.value( j ) );
			glVertex( transVerts.value( j ) + transTangents.value( j ) * normalScale * 2 );
			glVertex( transVerts.value( j ) );
			glVertex( transVerts.value( j ) - transTangents.value( j ) * normalScale / 2 );
		}

		glEnd();

		if ( i >= 0 ) {
			glDepthFunc( GL_ALWAYS );
			glHighlightColor();
			glBegin( GL_LINES );
			glVertex( transVerts.value( i ) );
			glVertex( transVerts.value( i ) + transTangents.value( i ) * normalScale * 2 );
			glVertex( transVerts.value( i ) );
			glVertex( transVerts.value( i ) - transTangents.value( i ) * normalScale / 2 );
			glEnd();
		}
	}

	if ( n == "Bitangents" ) {
		float normalScale = bounds().radius / 20;
		normalScale /= 2.0f;

		if ( normalScale < 0.1f )
			normalScale = 0.1f;

		glBegin( GL_LINES );

		for ( int j = 0; j < transVerts.count() && j < transBitangents.count(); j++ ) {
			glVertex( transVerts.value( j ) );
			glVertex( transVerts.value( j ) + transBitangents.value( j ) * normalScale * 2 );
			glVertex( transVerts.value( j ) );
			glVertex( transVerts.value( j ) - transBitangents.value( j ) * normalScale / 2 );
		}

		glEnd();

		if ( i >= 0 ) {
			glDepthFunc( GL_ALWAYS );
			glHighlightColor();
			glBegin( GL_LINES );
			glVertex( transVerts.value( i ) );
			glVertex( transVerts.value( i ) + transBitangents.value( i ) * normalScale * 2 );
			glVertex( transVerts.value( i ) );
			glVertex( transVerts.value( i ) - transBitangents.value( i ) * normalScale / 2 );
			glEnd();
		}
	}

	if ( n == "Faces" || n == "Triangles" ) {
		glLineWidth( 1.5f );

		for ( const Triangle& tri : triangles ) {
			glBegin( GL_TRIANGLES );
			glVertex( transVerts.value( tri.v1() ) );
			glVertex( transVerts.value( tri.v2() ) );
			glVertex( transVerts.value( tri.v3() ) );
			//glVertex( transVerts.value( tri.v1() ) );
			glEnd();
		}

		if ( i >= 0 ) {
			glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
			glDepthFunc( GL_ALWAYS );
			glHighlightColor();
			Triangle tri = triangles.value( i );
			glBegin( GL_TRIANGLES );
			glVertex( transVerts.value( tri.v1() ) );
			glVertex( transVerts.value( tri.v2() ) );
			glVertex( transVerts.value( tri.v3() ) );
			//glVertex( transVerts.value( tri.v1() ) );
			glEnd();
			glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		}
	}

	if ( n == "Faces" || n == "Strips" || n == "Strip Lengths" ) {
		glLineWidth( 1.5f );

		for ( const QVector<quint16>& strip : tristrips ) {
			quint16 a = strip.value( 0 );
			quint16 b = strip.value( 1 );

			for ( int v = 2; v < strip.count(); v++ ) {
				quint16 c = strip[v];

				if ( a != b && b != c && c != a ) {
					glBegin( GL_LINE_STRIP );
					glVertex( transVerts.value( a ) );
					glVertex( transVerts.value( b ) );
					glVertex( transVerts.value( c ) );
					glVertex( transVerts.value( a ) );
					glEnd();
				}

				a = b;
				b = c;
			}
		}

		if ( i >= 0 && !tristrips.isEmpty() ) {
			QVector<quint16> strip = tristrips[i];

			quint16 a = strip.value( 0 );
			quint16 b = strip.value( 1 );

			for ( int v = 2; v < strip.count(); v++ ) {
				quint16 c = strip[v];

				if ( a != b && b != c && c != a ) {
					glDepthFunc( GL_ALWAYS );
					glHighlightColor();
					glBegin( GL_LINE_STRIP );
					glVertex( transVerts.value( a ) );
					glVertex( transVerts.value( b ) );
					glVertex( transVerts.value( c ) );
					glVertex( transVerts.value( a ) );
					glEnd();
				}

				a = b;
				b = c;
			}
		}
	}

	if ( n == "Skin Partition Blocks" ) {

		for ( int c = 0; c < partitions.count(); c++ ) {
			if ( c == i )
				glHighlightColor();
			else
				glNormalColor();

			QVector<int> vmap = partitions[c].vertexMap;

			for ( const Triangle& tri : partitions[c].triangles ) {
				glBegin( GL_LINE_STRIP );
				glVertex( transVerts.value( vmap.value( tri.v1() ) ) );
				glVertex( transVerts.value( vmap.value( tri.v2() ) ) );
				glVertex( transVerts.value( vmap.value( tri.v3() ) ) );
				glVertex( transVerts.value( vmap.value( tri.v1() ) ) );
				glEnd();
			}
			for ( const QVector<quint16>& strip : partitions[c].tristrips ) {
				quint16 a = vmap.value( strip.value( 0 ) );
				quint16 b = vmap.value( strip.value( 1 ) );

				for ( int v = 2; v < strip.count(); v++ ) {
					quint16 c = vmap.value( strip[v] );

					if ( a != b && b != c && c != a ) {
						glBegin( GL_LINE_STRIP );
						glVertex( transVerts.value( a ) );
						glVertex( transVerts.value( b ) );
						glVertex( transVerts.value( c ) );
						glVertex( transVerts.value( a ) );
						glEnd();
					}

					a = b;
					b = c;
				}
			}
		}
	}

	if ( transformRigid )
		glPopMatrix();
}

QString Mesh::textStats() const
{
	return Node::textStats() + QString( "\nshader: %1\n" ).arg( shader );
}
