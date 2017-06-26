#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#endif


#include "3ds.h"

#include "spellbook.h"
#include "gl/gltex.h"

#include "lib/nvtristripwrapper.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QString>

#define tr( x ) QCoreApplication::tr( "3dsImport", x )


struct objPoint
{
	int v, t, n;

	bool operator==( const objPoint & other ) const
	{
		return v == other.v && t == other.t && n == other.n;
	}
};

struct objFace
{
	int v1, v2, v3;
	bool dblside;
};

struct objMaterial
{
	QString name;
	Color3 Ka, Kd, Ks;
	float alpha, glossiness;
	QString map_Kd;

	objMaterial() : name( "Untextured" ), alpha( 1.0f ), glossiness( 15.0f )
	{
	}
};

struct objMatFace
{
	QString matName;
	QVector<short> subFaces;
};

// The 3ds file can be made up of several objects
struct objMesh
{
	QString name;                   // The object name
	QVector<Vector3> vertices;      // The array of vertices
	QVector<Vector3> normals;       // The array of the normals for the vertices
	QVector<Vector2> texcoords;     // The array of texture coordinates for the vertices
	QVector<objFace> faces;         // The array of face indices
	QVector<objMatFace> matfaces;   // The array of materials for this mesh
	Vector3 pos;                    // The position to move the object to
	Vector3 rot;                    // The angles to rotate the object

	objMesh() : pos( 0.0f, 0.0f, 0.0f ), rot( 0.0f, 0.0f, 0.0f )
	{
	}
};

struct objKeyframe
{
	Vector3 pos;
	float rotAngle = 0;
	Vector3 rotAxis;
	float scale = 0;

	objKeyframe()
	{
	}
};

struct objKfSequence
{
	short objectId = 0;
	QString objectName;
	long startTime = 0, endTime = 0, curTime = 0;
	Vector3 pivot;
	QMap<short, objKeyframe> frames;

	objKfSequence()
	{
	}
};

static void addLink( NifModel * nif, const QModelIndex & iBlock, const QString & name, qint32 link )
{
	QModelIndex iArray = nif->getIndex( iBlock, name );
	QModelIndex iSize  = nif->getIndex( iBlock, QString( "Num %1" ).arg( name ) );
	int numIndices = nif->get<int>( iSize );
	nif->set<int>( iSize, numIndices + 1 );
	nif->updateArray( iArray );
	nif->setLink( iArray.child( numIndices, 0 ), link );
}

static Color3 GetColorFromChunk( Chunk * cnk )
{
	float r = 1.0f;
	float g = 1.0f;
	float b = 1.0f;

	Chunk * ColorChunk;

	ColorChunk = cnk->getChild( COLOR_F );

	if ( !ColorChunk ) {
		ColorChunk = cnk->getChild( LIN_COLOR_F );
	}

	if ( ColorChunk ) {
		r = ColorChunk->read<float>();
		g = ColorChunk->read<float>();
		b = ColorChunk->read<float>();
	}

	ColorChunk = cnk->getChild( COLOR_24 );

	if ( !ColorChunk ) {
		ColorChunk = cnk->getChild( LIN_COLOR_24 );
	}

	if ( ColorChunk ) {
		r = (float)( ColorChunk->read<unsigned char>() ) / 255.0f;
		g = (float)( ColorChunk->read<unsigned char>() ) / 255.0f;
		b = (float)( ColorChunk->read<unsigned char>() ) / 255.0f;
	}

	return Color3( r, g, b );
}

static float GetPercentageFromChunk( Chunk * cnk )
{
	float f = 0.0f;

	Chunk * PercChunk = cnk->getChild( FLOAT_PERCENTAGE );

	if ( PercChunk ) {
		f = PercChunk->read<float>();
	}

	PercChunk = cnk->getChild( INT_PERCENTAGE );

	if ( PercChunk ) {
		f = (float)( PercChunk->read<unsigned short>() / 255.0f );
	}

	return f;
}


void import3ds( NifModel * nif, const QModelIndex & index )
{
	//--Determine how the file will import, and be sure the user wants to continue--//

	// If no existing node is selected, create a group node.  Otherwise use selected node
	QPersistentModelIndex iRoot, iNode, iShape, iMaterial, iData, iTexProp, iTexSource;
	QModelIndex iBlock = nif->getBlock( index );

	//Be sure the user hasn't clicked on a NiTriStrips object
	if ( iBlock.isValid() && nif->itemName( iBlock ) == "NiTriStrips" ) {
		int result = QMessageBox::information( 0, tr( "Import OBJ" ), tr( "You cannot import an OBJ file over a NiTriStrips object.  Please convert it to a NiTriShape object first by right-clicking and choosing Mesh > Triangulate" ) );
		return;
	}

	if ( iBlock.isValid() && nif->itemName( index ) == "NiNode" ) {
		iNode = index;
	} else if ( iBlock.isValid() && nif->itemName( index ) == "NiTriShape" ) {
		iShape = index;
		//Find parent of NiTriShape
		int par_num = nif->getParent( nif->getBlockNumber( index ) );

		if ( par_num != -1 ) {
			iNode = nif->getBlock( par_num );
		}

		//Find material, texture, and data objects
		QList<int> children = nif->getChildLinks( nif->getBlockNumber( iShape ) );

		for ( const auto child : children ) {
			if ( child != -1 ) {
				QModelIndex temp = nif->getBlock( child );
				QString type = nif->itemName( temp );

				if ( type == "NiMaterialProperty" ) {
					iMaterial = temp;
				} else if ( type == "NiTriShapeData" ) {
					iData = temp;
				} else if ( (type == "NiTexturingProperty") || (type == "NiTextureProperty") ) {
					iTexProp = temp;

					//Search children of texture property for texture sources/images
					QList<int> chn = nif->getChildLinks( nif->getBlockNumber( iTexProp ) );

					for ( const auto c : chn ) {
						QModelIndex temp = nif->getBlock( c );
						QString type = nif->itemName( temp );

						if ( (type == "NiSourceTexture") || (type == "NiImage") ) {
							iTexSource = temp;
						}
					}
				}
			}
		}
	}

	QString question;

	if ( iNode.isValid() == true ) {
		if ( iShape.isValid() == true ) {
			question = tr( "NiTriShape selected.  The first imported mesh will replace the selected one." );
		} else {
			question = tr( "NiNode selected.  Meshes will be attached to the selected node." );
		}
	} else {
		question = tr( "No NiNode or NiTriShape selected.  Meshes will be imported to the root of the file." );
	}

	int result = QMessageBox::question( 0, tr( "Import 3DS" ), question, QMessageBox::Ok, QMessageBox::Cancel );

	if ( result == QMessageBox::Cancel ) {
		return;
	}


	//--Read the file--//

	float ObjScale;
	QVector<objMesh> ObjMeshes;
	QMap<QString, objMaterial> ObjMaterials;
	QMap<QString, objKfSequence> ObjKeyframes;

	QSettings settings;
	settings.beginGroup( "Import-Export" );
	settings.beginGroup( "3DS" );

	QString fname = QFileDialog::getOpenFileName( qApp->activeWindow(), tr( "Choose a .3ds file to import" ), settings.value( tr( "File Name" ) ).toString(), "3DS (*.3ds)" );

	if ( fname.isEmpty() ) {
		return;
	}

	QFile fobj( fname );

	if ( !fobj.open( QIODevice::ReadOnly ) ) {
		qCCritical( nsIo ) << tr( "Failed to read %1" ).arg( fobj.fileName() );
		return;
	}

	Chunk * FileChunk = Chunk::LoadFile( &fobj );

	if ( !FileChunk ) {
		qCCritical( nsIo ) << tr( "Could not get 3ds data" );
		return;
	}

	Chunk * Model = FileChunk->getChild( M3DMAGIC );

	if ( !Model ) {
		qCCritical( nsIo ) << tr( "Could not get 3ds model" );
		return;
	}

	Chunk * ModelData = Model->getChild( MDATA );

	if ( !ModelData ) {
		qCCritical( nsIo ) << tr( "Could not get 3ds model data" );
		return;
	}

	Chunk * MasterScale = ModelData->getChild( MASTER_SCALE );

	if ( MasterScale ) {
		ObjScale = MasterScale->read<float>();
	} else {
		ObjScale = 1.0f;
	}

	QList<Chunk *> Materials = ModelData->getChildren( MATERIAL );
	QList<Chunk *> Meshes = ModelData->getChildren( NAMED_OBJECT );

	for ( Chunk * mat : Materials ) {
		objMaterial newMat;

		// material name
		Chunk * matName = mat->getChild( MAT_NAME );

		if ( matName ) {
			newMat.name = matName->readString();
		}

		// material colors
		Chunk * matColor = mat->getChild( MAT_AMBIENT );

		if ( matColor ) {
			newMat.Ka = GetColorFromChunk( matColor );
		}

		matColor = mat->getChild( MAT_DIFFUSE );

		if ( matColor ) {
			newMat.Kd = GetColorFromChunk( matColor );
		}

		matColor = mat->getChild( MAT_SPECULAR );

		if ( matColor ) {
			newMat.Ks = GetColorFromChunk( matColor );
		}

		// material textures
		Chunk * matTexture = mat->getChild( MAT_TEXMAP );

		if ( matTexture ) {
			Chunk * matTexProperty = matTexture->getChild( MAT_MAPNAME );

			if ( matTexProperty ) {
				newMat.map_Kd = matTexProperty->readString();
			}
		}

		// material alpha
		Chunk * matAlpha = mat->getChild( MAT_TRANSPARENCY );

		if ( matAlpha ) {
			newMat.alpha = 1.0f - GetPercentageFromChunk( matAlpha );
		}

		ObjMaterials.insert( newMat.name, newMat );
	}

	for ( Chunk * mesh : Meshes ) {
		objMesh newMesh;

		newMesh.name = mesh->readString();

		for ( Chunk * TriObj : mesh->getChildren( N_TRI_OBJECT ) ) {
			Chunk * PointArray = TriObj->getChild( POINT_ARRAY );

			if ( PointArray ) {
				unsigned short nPoints = PointArray->read<unsigned short>();

				for ( unsigned short i = 0; i < nPoints; i++ ) {
					float x, y, z;
					x = PointArray->read<float>();
					y = PointArray->read<float>();
					z = PointArray->read<float>();

					newMesh.vertices.append( { x, y, z } );
					newMesh.normals.append( { 0.0f, 0.0f, 1.0f } );
				}
			}

			Chunk * FaceArray = TriObj->getChild( FACE_ARRAY );

			if ( FaceArray ) {
				unsigned short nFaces = FaceArray->read<unsigned short>();

				for ( unsigned short i = 0; i < nFaces; i++ ) {
					Chunk::ChunkTypeFaceArray f;

					f.vertex1 = FaceArray->read<unsigned short>();
					f.vertex2 = FaceArray->read<unsigned short>();
					f.vertex3 = FaceArray->read<unsigned short>();
					f.flags = FaceArray->read<unsigned short>();

					objFace newFace;

					newFace.v1 = f.vertex1;
					newFace.v2 = f.vertex2;
					newFace.v3 = f.vertex3;

					newFace.dblside = !(f.flags & FACE_FLAG_ONESIDE);

					Vector3 n1 = newMesh.vertices[newFace.v2] - newMesh.vertices[newFace.v1];
					Vector3 n2 = newMesh.vertices[newFace.v3] - newMesh.vertices[newFace.v1];
					Vector3 FaceNormal = Vector3::crossproduct( n1, n2 );
					FaceNormal.normalize();
					newMesh.normals[newFace.v1] += FaceNormal;
					newMesh.normals[newFace.v2] += FaceNormal;
					newMesh.normals[newFace.v3] += FaceNormal;

					newMesh.faces.append( newFace );
				}

				objMatFace newMatFace;

				for ( Chunk * MatFaces : FaceArray->getChildren( MSH_MAT_GROUP ) ) {
					//Chunk * MatFaces = FaceArray->getChild( MSH_MAT_GROUP );
					if ( MatFaces ) {
						newMatFace.matName = MatFaces->readString();

						unsigned short nFaces = MatFaces->read<unsigned short>();

						for ( unsigned short i = 0; i < nFaces; i++ ) {
							unsigned short FaceNum = MatFaces->read<unsigned short>();
							newMatFace.subFaces.append( FaceNum );
						}

						newMesh.matfaces.append( newMatFace );
					}
				}
			}

			Chunk * TexVerts = TriObj->getChild( TEX_VERTS );

			if ( TexVerts ) {
				unsigned short nVerts = TexVerts->read<unsigned short>();

				for ( unsigned short i = 0; i < nVerts; i++ ) {
					float x, y;
					x = TexVerts->read<float>();
					y = TexVerts->read<float>();

					newMesh.texcoords.append( Vector2( x, -y ) );
				}
			}
		}

		for ( int i = 0; i < newMesh.normals.size(); i++ ) {
			newMesh.normals[i].normalize();
		}

		ObjMeshes.append( newMesh );
	}

	Chunk * Keyframes = Model->getChild( KFDATA );

	if ( Keyframes ) {
		if ( Chunk * KfHdr = Keyframes->getChild( KFHDR ) ) {
		}

		QList<Chunk *> KfSegs = Keyframes->getChildren( KFSEG );
		QList<Chunk *> KfCurTimes = Keyframes->getChildren( KFCURTIME );

		for ( int i = 0; i < KfSegs.size(); i++ ) {
			/*
			Chunk::ChunkData * rawData = KfSegs[i]->getData();
			newKfSeg.startTime = *( (long *) rawData );
			rawData += sizeof( long );
			newKfSeg.endTime = *( (long *) rawData );
			KfSegs[i]->clearData();

			Chunk * KfCurTime = KfCurTimes[i];

			rawData = KfCurTimes[i]->getData();
			newKfSeg.curTime = *( (long *) rawData );
			KfCurTimes[i]->clearData();
			*/
		}

		for ( Chunk * KfObj : Keyframes->getChildren( OBJECT_NODE_TAG ) ) {
			objKfSequence newKfSeq;

			if ( Chunk * NodeId = KfObj->getChild( NODE_ID ) ) {
				newKfSeq.objectId = NodeId->read<unsigned short>();
			}

			if ( Chunk * NodeHdr = KfObj->getChild( NODE_HDR ) ) {
				newKfSeq.objectName = NodeHdr->readString();

				unsigned short Flags1 = NodeHdr->read<unsigned short>();
				unsigned short Flags2 = NodeHdr->read<unsigned short>();
				unsigned short Hierarchy = NodeHdr->read<unsigned short>();
			}

			if ( Chunk * Pivot = KfObj->getChild( PIVOT ) ) {
				float x = Pivot->read<float>();
				float y = Pivot->read<float>();
				float z = Pivot->read<float>();

				newKfSeq.pivot = { x, y, z };
			}

			if ( Chunk * PosTrack = KfObj->getChild( POS_TRACK_TAG ) ) {
				unsigned short flags = PosTrack->read<unsigned short>();

				unsigned short unknown1 = PosTrack->read<unsigned short>();
				unsigned short unknown2 = PosTrack->read<unsigned short>();
				unsigned short unknown3 = PosTrack->read<unsigned short>();
				unsigned short unknown4 = PosTrack->read<unsigned short>();

				unsigned short keys = PosTrack->read<unsigned short>();

				unsigned short unknown = PosTrack->read<unsigned short>();

				for ( int key = 0; key < keys; key++ ) {
					unsigned short kfNum = PosTrack->read<unsigned short>();
					unsigned long kfUnknown = PosTrack->read<unsigned long>();
					float kfPosX = PosTrack->read<float>();
					float kfPosY = PosTrack->read<float>();
					float kfPosZ = PosTrack->read<float>();

					newKfSeq.frames[kfNum].pos = { kfPosX, kfPosY, kfPosZ };
				}
			}

			if ( Chunk * RotTrack = KfObj->getChild( ROT_TRACK_TAG ) ) {
				unsigned short flags = RotTrack->read<unsigned short>();

				unsigned short unknown1 = RotTrack->read<unsigned short>();
				unsigned short unknown2 = RotTrack->read<unsigned short>();
				unsigned short unknown3 = RotTrack->read<unsigned short>();
				unsigned short unknown4 = RotTrack->read<unsigned short>();

				unsigned short keys = RotTrack->read<unsigned short>();

				unsigned short unknown = RotTrack->read<unsigned short>();

				for ( unsigned short key = 0; key < keys; key++ ) {
					unsigned short kfNum = RotTrack->read<unsigned short>();
					unsigned long kfUnknown = RotTrack->read<unsigned long>();
					float kfRotAngle = RotTrack->read<float>();
					float kfAxisX = RotTrack->read<float>();
					float kfAxisY = RotTrack->read<float>();
					float kfAxisZ = RotTrack->read<float>();

					newKfSeq.frames[kfNum].rotAngle = kfRotAngle;
					newKfSeq.frames[kfNum].rotAxis = { kfAxisX, kfAxisY, kfAxisZ };
				}
			}

			if ( Chunk * SclTrack = KfObj->getChild( SCL_TRACK_TAG ) ) {
				unsigned short flags = SclTrack->read<unsigned short>();

				unsigned short unknown1 = SclTrack->read<unsigned short>();
				unsigned short unknown2 = SclTrack->read<unsigned short>();
				unsigned short unknown3 = SclTrack->read<unsigned short>();
				unsigned short unknown4 = SclTrack->read<unsigned short>();

				unsigned short keys = SclTrack->read<unsigned short>();

				unsigned short unknown = SclTrack->read<unsigned short>();

				for ( unsigned short key = 0; key < keys; key++ ) {
					unsigned short kfNum = SclTrack->read<unsigned short>();
					unsigned long kfUnknown = SclTrack->read<unsigned long>();
					float kfSclX = SclTrack->read<float>();
					float kfSclY = SclTrack->read<float>();
					float kfSclZ = SclTrack->read<float>();

					newKfSeq.frames[kfNum].scale = ( kfSclX + kfSclY + kfSclZ ) / 3.0f;
				}
			}

			ObjKeyframes.insertMulti( newKfSeq.objectName, newKfSeq );
		}
	}

	fobj.close();

	//--Translate file structures into NIF ones--//

	if ( iNode.isValid() == false ) {
		iNode = nif->insertNiBlock( "NiNode" );
		nif->set<QString>( iNode, "Name", "Scene Root" );
	}

	//Record root object
	iRoot = iNode;

	// create a NiTriShape foreach material in the object
	for ( int objIndex = 0; objIndex < ObjMeshes.size(); objIndex++ ) {
		objMesh * mesh = &ObjMeshes[objIndex];



		// create group node if there is more than 1 material
		bool groupNode = false;
		QPersistentModelIndex iNode = iRoot;

		if ( mesh->matfaces.size() > 1 ) {
			groupNode = true;

			iNode = nif->insertNiBlock( "NiNode" );
			nif->set<QString>( iNode, "Name", mesh->name );
			addLink( nif, iRoot, "Children", nif->getBlockNumber( iNode ) );
		}

		int shapecount = 0;

		for ( int i = 0; i < mesh->matfaces.size(); i++ ) {
			if ( !ObjMaterials.contains( mesh->matfaces[i].matName ) ) {
				Message::append( tr( "Warnings were generated during 3ds import." ),
					tr( "Material '%1' not found in list." ).arg( mesh->matfaces[i].matName ) );
			}

			objMaterial * mat = &ObjMaterials[mesh->matfaces[i].matName];

			if ( iShape.isValid() == false || objIndex != 0 ) {
				iShape = nif->insertNiBlock( "NiTriShape" );
			}

			if ( groupNode ) {
				nif->set<QString>( iShape, "Name", QString( "%1:%2" ).arg( nif->get<QString>( iNode, "Name" ) ).arg( shapecount++ ) );
				addLink( nif, iNode, "Children", nif->getBlockNumber( iShape ) );
			} else {
				nif->set<QString>( iShape, "Name", mesh->name );
				addLink( nif, iRoot, "Children", nif->getBlockNumber( iShape ) );
			}


			// add material property, for non-Skyrim versions
			if ( nif->getUserVersion() < 12 ) {
				if ( iMaterial.isValid() == false || objIndex != 0 ) {
					iMaterial = nif->insertNiBlock( "NiMaterialProperty" );
				}

				nif->set<QString>( iMaterial, "Name", mat->name );
				nif->set<Color3>( iMaterial, "Ambient Color", mat->Ka );
				nif->set<Color3>( iMaterial, "Diffuse Color", mat->Kd );
				nif->set<Color3>( iMaterial, "Specular Color", mat->Ks );
				nif->set<Color3>( iMaterial, "Emissive Color", Color3( 0, 0, 0 ) );
				nif->set<float>( iMaterial, "Alpha", mat->alpha );
				nif->set<float>( iMaterial, "Glossiness", mat->glossiness );

				addLink( nif, iShape, "Properties", nif->getBlockNumber( iMaterial ) );
			}

			if ( !mat->map_Kd.isEmpty() ) {
				if ( nif->getUserVersion() >= 12 ) {
					// Skyrim, nothing here yet
				} else if ( nif->getVersionNumber() >= 0x0303000D ) {
					//Newer versions use NiTexturingProperty and NiSourceTexture
					if ( iTexProp.isValid() == false || objIndex != 0 || nif->itemType( iTexProp ) != "NiTexturingProperty" ) {
						iTexProp = nif->insertNiBlock( "NiTexturingProperty" );
					}

					addLink( nif, iShape, "Properties", nif->getBlockNumber( iTexProp ) );

					nif->set<int>( iTexProp, "Has Base Texture", 1 );
					QModelIndex iBaseMap = nif->getIndex( iTexProp, "Base Texture" );
					nif->set<int>( iBaseMap, "Clamp Mode", 3 );
					nif->set<int>( iBaseMap, "Filter Mode", 2 );

					if ( iTexSource.isValid() == false || objIndex != 0 || nif->itemType( iTexSource ) != "NiSourceTexture" ) {
						iTexSource = nif->insertNiBlock( "NiSourceTexture" );
					}

					nif->setLink( iBaseMap, "Source", nif->getBlockNumber( iTexSource ) );

					nif->set<int>( iTexSource, "Pixel Layout", nif->getVersion() == "20.0.0.5" ? 6 : 5 );
					nif->set<int>( iTexSource, "Use Mipmaps", 2 );
					nif->set<int>( iTexSource, "Alpha Format", 3 );
					nif->set<int>( iTexSource, "Unknown Byte", 1 );
					nif->set<int>( iTexSource, "Unknown Byte 2", 1 );

					nif->set<int>( iTexSource, "Use External", 1 );
					nif->set<QString>( iTexSource, "File Name", mat->map_Kd );
				} else {
					//Older versions use NiTextureProperty and NiImage
					if ( iTexProp.isValid() == false || objIndex != 0 || nif->itemType( iTexProp ) != "NiTextureProperty" ) {
						iTexProp = nif->insertNiBlock( "NiTextureProperty" );
					}

					addLink( nif, iShape, "Properties", nif->getBlockNumber( iTexProp ) );

					if ( iTexSource.isValid() == false || objIndex != 0 || nif->itemType( iTexSource ) != "NiImage" ) {
						iTexSource = nif->insertNiBlock( "NiImage" );
					}

					nif->setLink( iTexProp, "Image", nif->getBlockNumber( iTexSource ) );

					nif->set<int>( iTexSource, "External", 1 );
					nif->set<QString>( iTexSource, "File Name", mat->map_Kd );
				}
			}

			if ( iData.isValid() == false || objIndex != 0 ) {
				iData = nif->insertNiBlock( "NiTriShapeData" );
			}

			nif->setLink( iShape, "Data", nif->getBlockNumber( iData ) );

			QVector<Triangle> triangles;
			QVector<objPoint> points;

			for ( const auto faceIndex : mesh->matfaces[i].subFaces ) {
				objFace face = mesh->faces[faceIndex];

				Triangle tri;

				tri.set( face.v1, face.v2, face.v3 );

				triangles.append( tri );
			}

			nif->set<int>( iData, "Num Vertices", mesh->vertices.count() );
			nif->set<int>( iData, "Has Vertices", 1 );
			nif->updateArray( iData, "Vertices" );
			nif->setArray<Vector3>( iData, "Vertices",  mesh->vertices );
			nif->set<int>( iData, "Has Normals", 1 );
			nif->updateArray( iData, "Normals" );
			nif->setArray<Vector3>( iData, "Normals",  mesh->normals );
			nif->set<int>( iData, "Has UV", 1 );
			nif->set<int>( iData, "Num UV Sets", 1 );
			nif->set<int>( iData, "Num UV Sets 2", 1 );
			QModelIndex iTexCo = nif->getIndex( iData, "UV Sets" );

			if ( !iTexCo.isValid() ) {
				iTexCo = nif->getIndex( iData, "UV Sets 2" );
			}

			nif->updateArray( iTexCo );
			nif->updateArray( iTexCo.child( 0, 0 ) );
			nif->setArray<Vector2>( iTexCo.child( 0, 0 ),  mesh->texcoords );

			nif->set<int>( iData, "Has Triangles", 1 );
			nif->set<int>( iData, "Num Triangles", triangles.count() );
			nif->set<int>( iData, "Num Triangle Points", triangles.count() * 3 );
			nif->updateArray( iData, "Triangles" );
			nif->setArray<Triangle>( iData, "Triangles", triangles );

			Vector3 center;
			for ( const Vector3& v : mesh->vertices ) {
				center += v;
			}

			if ( mesh->vertices.count() > 0 )
				center /= mesh->vertices.count();

			nif->set<Vector3>( iData, "Center", center );
			float radius = 0;
			for ( const Vector3& v : mesh->vertices ) {
				float d = ( center - v ).length();

				if ( d > radius )
					radius = d;
			}
			nif->set<float>( iData, "Radius", radius );

			nif->set<int>( iData, "Unknown Short 2", 0x4000 );
		}

		// set up a controller for animated objects
	}

	settings.setValue( "File Name", fname );

	settings.endGroup(); // 3DS
	settings.endGroup(); // Import-Export

	nif->reset();
	return;
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
