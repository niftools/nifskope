#include "3ds.h"

#include "../spellbook.h"

#include "../NvTriStrip/qtwrapper.h"

#include "../gl/gltex.h"

#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QString>

class sp3dsImportModel : public Spell
{
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
		
		objMaterial() : name( "Untextured" ), alpha( 1.0f ), glossiness( 15.0f ) {}
	};
	
	struct objMatFace {
		QString matName;
		QVector< short > subFaces;
	};

	// The 3ds file can be made up of several objects
	struct objMesh {
		QString name;					// The object name
		QVector<Vector3> vertices;		// The array of vertices
		QVector<Vector3> normals;		// The array of the normals for the vertices
		QVector<Vector2> texcoords;		// The array of texture coordinates for the vertices
		QVector<objFace> faces;			// The array of face indices
		QVector<objMatFace> matfaces;	// The array of materials for this mesh
		Vector3 pos;					// The position to move the object to
		Vector3 rot;					// The angles to rotate the object

		objMesh() : pos( 0.0f, 0.0f, 0.0f ), rot( 0.0f, 0.0f, 0.0f ) {}
	};

public:
	QString name() const { return Spell::tr("Import .3ds Model"); }
	QString page() const { return Spell::tr("Import"); }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return nif && ! index.isValid();
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & )
	{
		float ObjScale;
		QVector< objMesh > ObjMeshes;
		QMap< QString, objMaterial > ObjMaterials;

		// read the file
		
		QSettings settings( "NifTools", "NifSkope" );
		settings.beginGroup( "spells" );
		settings.beginGroup( page() );
		settings.beginGroup( name() );
		
		QString fname = QFileDialog::getOpenFileName( 0, Spell::tr("Choose a .3ds file to import"), settings.value( Spell::tr("File Name") ).toString(), "*.3ds" );
		if ( fname.isEmpty() ) {
			return QModelIndex();
		}
		
		QFile fobj( fname );
		if ( !fobj.open( QIODevice::ReadOnly ) )
		{
			qWarning() << Spell::tr("Could not open %1 for read access").arg( fobj.fileName() );
			return QModelIndex();
		}
		
		Chunk * FileChunk = Chunk::LoadFile( &fobj );
		if( !FileChunk ) {
			qWarning() << Spell::tr("Could not get 3ds data");
			return QModelIndex();
		}

		Chunk * Model = FileChunk->getChild( M3DMAGIC );
		if( !Model ) {
			qWarning() << Spell::tr("Could not get 3ds model");
			return QModelIndex();
		}

		Chunk * ModelData = Model->getChild( MDATA );
		if( !ModelData ) {
			qWarning() << Spell::tr("Could not get 3ds model data");
			return QModelIndex();
		}

		Chunk * MasterScale = ModelData->getChild( MASTER_SCALE );
		if( MasterScale ) {
			ObjScale = *( (float *) MasterScale->getData() );
			MasterScale->clearData();
		}
		else {
			ObjScale = 1.0f;
		}

		QList< Chunk * > Materials = ModelData->getChildren( MATERIAL );
		QList< Chunk * > Meshes = ModelData->getChildren( NAMED_OBJECT );

		foreach( Chunk * mat, Materials )
		{
			objMaterial newMat;

			// material name
			Chunk * matName = mat->getChild( MAT_NAME );
			if( matName ) {
				newMat.name = GetNameFromChunk( matName );
			}

			// material colors
			Chunk * matColor = mat->getChild( MAT_AMBIENT );
			if( matColor ) {
				newMat.Ka = GetColorFromChunk( matColor );
			}

			matColor = mat->getChild( MAT_DIFFUSE );
			if( matColor ) {
				newMat.Kd = GetColorFromChunk( matColor );
			}

			matColor = mat->getChild( MAT_SPECULAR );
			if( matColor ) {
				newMat.Ks = GetColorFromChunk( matColor );
			}

			// material textures
			Chunk * matTexture = mat->getChild( MAT_TEXMAP );
			if( matTexture ) {
				Chunk * matTexProperty = matTexture->getChild( MAT_MAPNAME );
				if( matTexProperty ) {
					newMat.map_Kd = GetNameFromChunk( matTexProperty );
				}
			}

			// material alpha
			Chunk * matAlpha = mat->getChild( MAT_TRANSPARENCY );
			if( matAlpha ) {
				newMat.alpha = 1.0f - GetPercentageFromChunk( matAlpha );
			}

			ObjMaterials.insert( newMat.name, newMat );
		}

		foreach( Chunk * mesh, Meshes )
		{
			objMesh newMesh;

			newMesh.name = GetNameFromChunk( mesh );

			foreach( Chunk * TriObj, mesh->getChildren( N_TRI_OBJECT ) )
			{
				Chunk * PointArray = TriObj->getChild( POINT_ARRAY );
				if( PointArray ) {
					Chunk::ChunkData * rawData = PointArray->getData();

					short * nPoints = (short *) rawData;

					rawData += sizeof( short );

					for( int i = 0; i < *nPoints; i++ )
					{
						Chunk::ChunkTypeFloat3 * v = (Chunk::ChunkTypeFloat3 *) rawData;
						rawData += sizeof( Chunk::ChunkTypeFloat3 );

						newMesh.vertices.append( Vector3( v->x, v->y, v->z ) );
						newMesh.normals.append( Vector3( 0.0f, 0.0f, 0.0f ) );
					}

					PointArray->clearData();
				}

				Chunk * FaceArray = TriObj->getChild( FACE_ARRAY );
				if( FaceArray ) {
					Chunk::ChunkData * rawData = FaceArray->getData();

					short * nPoints = (short *) rawData;

					rawData += sizeof( short );

					for( int i = 0; i < *nPoints; i++ )
					{
						Chunk::ChunkTypeFaceArray * f = (Chunk::ChunkTypeFaceArray *) rawData;
						rawData += sizeof( Chunk::ChunkTypeFaceArray );

						objFace newFace;

						newFace.v1 = f->vertex1;
						newFace.v2 = f->vertex2;
						newFace.v3 = f->vertex3;

						newFace.dblside = !(f->flags & FACE_FLAG_ONESIDE);

						Vector3 n1 = newMesh.vertices[newFace.v2] - newMesh.vertices[newFace.v1];
						Vector3 n2 = newMesh.vertices[newFace.v3] - newMesh.vertices[newFace.v1];
						Vector3 FaceNormal = Vector3::crossproduct(n1, n2);
						FaceNormal.normalize();
						newMesh.normals[newFace.v1] += FaceNormal;
						newMesh.normals[newFace.v2] += FaceNormal;
						newMesh.normals[newFace.v3] += FaceNormal;

						newMesh.faces.append( newFace );

					}

					FaceArray->clearData();

					objMatFace newMatFace;

					Chunk * MatFaces = FaceArray->getChild( MSH_MAT_GROUP );
					if( MatFaces ) {
						Chunk::ChunkData * rawData = MatFaces->getData();

						QString MatName;

						while( *rawData ) {
							MatName.append( *rawData );
							rawData++;
						}
						rawData++;

						newMatFace.matName = MatName;

						short * nFaces = (short *) rawData;
						rawData += sizeof( short );

						for( int i = 0; i < *nFaces; i++ ) {
							short * FaceNum = (short *) rawData;
							rawData += sizeof( short );

							newMatFace.subFaces.append( *FaceNum );
						}

						MatFaces->clearData();

						newMesh.matfaces.append( newMatFace );
					}
				}

				Chunk * TexVerts = TriObj->getChild( TEX_VERTS );
				if( TexVerts ) {
					Chunk::ChunkData * rawData = PointArray->getData();

					short * nVerts = (short *) rawData;

					rawData += sizeof( short );

					for( int i = 0; i < *nVerts; i++ )
					{
						Chunk::ChunkTypeFloat2 * v = (Chunk::ChunkTypeFloat2 *) rawData;
						rawData += sizeof( Chunk::ChunkTypeFloat2 );

						newMesh.texcoords.append( Vector2( v->x, v->y ) );
					}

					TexVerts->clearData();
				}

			}

			for( int i = 0; i < newMesh.normals.size(); i++ )
			{
				newMesh.normals[i].normalize();
			}

			ObjMeshes.append( newMesh );
		}

		fobj.close();
		
		for(int objIndex = 0; objIndex < ObjMeshes.size(); objIndex++) {
			objMesh * mesh = &ObjMeshes[objIndex];

			// create group node
			QPersistentModelIndex iNode = nif->insertNiBlock( "NiNode" );
			nif->set<QString>( iNode, "Name", mesh->name );
			
			// create a NiTriShape foreach material in the object
			int shapecount = 0;
			for( int i = 0; i < mesh->matfaces.size(); i++ )
			{
				if ( !ObjMaterials.contains( mesh->matfaces[i].matName ) ) {
					qWarning() << Spell::tr("Material '%1' not found in list!").arg( mesh->matfaces[i].matName );
				}

				objMaterial * mat = &ObjMaterials[mesh->matfaces[i].matName];

				QPersistentModelIndex iShape = nif->insertNiBlock( "NiTriShape" );
				nif->set<QString>( iShape, "Name", QString( "%1:%2" ).arg( nif->get<QString>( iNode, "Name" ) ).arg( shapecount++ ) );
				addLink( nif, iNode, "Children", nif->getBlockNumber( iShape ) );
				
				QModelIndex iMaterial = nif->insertNiBlock( "NiMaterialProperty" );
				nif->set<QString>( iMaterial, "Name", mat->name );
				nif->set<Color3>( iMaterial, "Ambient Color", mat->Ka );
				nif->set<Color3>( iMaterial, "Diffuse Color", mat->Kd );
				nif->set<Color3>( iMaterial, "Specular Color", mat->Ks );
				nif->set<Color3>( iMaterial, "Emissive Color", Color3( 0, 0, 0 ) );
				nif->set<float>( iMaterial, "Alpha", mat->alpha );
				nif->set<float>( iMaterial, "Glossiness", mat->glossiness );
				
				addLink( nif, iShape, "Properties", nif->getBlockNumber( iMaterial ) );
				
				if ( !mat->map_Kd.isEmpty() )
				{
					QModelIndex iTexProp = nif->insertNiBlock( "NiTexturingProperty" );
					addLink( nif, iShape, "Properties", nif->getBlockNumber( iTexProp ) );
					
					nif->set<int>( iTexProp, "Has Base Texture", 1 );
					QModelIndex iBaseMap = nif->getIndex( iTexProp, "Base Texture" );
					nif->set<int>( iBaseMap, "Clamp Mode", 3 );
					nif->set<int>( iBaseMap, "Filter Mode", 2 );
					
					QModelIndex iTexSource = nif->insertNiBlock( "NiSourceTexture" );
					nif->setLink( iBaseMap, "Source", nif->getBlockNumber( iTexSource ) );
					
					nif->set<int>( iTexSource, "Pixel Layout", nif->getVersion() == "20.0.0.5" ? 6 : 5 );
					nif->set<int>( iTexSource, "Use Mipmaps", 2 );
					nif->set<int>( iTexSource, "Alpha Format", 3 );
					nif->set<int>( iTexSource, "Unknown Byte", 1 );
					nif->set<int>( iTexSource, "Unknown Byte 2", 1 );
					
					nif->set<int>( iTexSource, "Use External", 1 );
					nif->set<QString>( iTexSource, "File Name", mat->map_Kd );
				}
				
				QModelIndex iData = nif->insertNiBlock( "NiTriShapeData" );
				nif->setLink( iShape, "Data", nif->getBlockNumber( iData ) );
				
				QVector< Triangle > triangles;
				QVector< objPoint > points;
				
				foreach( short faceIndex, mesh->matfaces[i].subFaces )
				{
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
				nif->set<int>( iData, "Has UV Sets", 1 );
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
				foreach ( Vector3 v,  mesh->vertices )
					center += v;
				if (  mesh->vertices.count() > 0 ) center /=  mesh->vertices.count();
				nif->set<Vector3>( iData, "Center", center );
				float radius = 0;
				foreach ( Vector3 v,  mesh->vertices )
				{
					float d = ( center - v ).length();
					if ( d > radius ) radius = d;
				}
				nif->set<float>( iData, "Radius", radius );
				
				nif->set<int>( iData, "Unknown Short 2", 0x4000 );
			}
		}
		
		settings.setValue( "File Name", fname );
		
		nif->reset();
		return QModelIndex();
	}

	void addLink( NifModel * nif, QModelIndex iBlock, QString name, qint32 link )
	{
		QModelIndex iArray = nif->getIndex( iBlock, name );
		QModelIndex iSize = nif->getIndex( iBlock, QString( "Num %1" ).arg( name ) );
		int numIndices = nif->get<int>( iSize );
		nif->set<int>( iSize, numIndices + 1 );
		nif->updateArray( iArray );
		nif->setLink( iArray.child( numIndices, 0 ), link );
	}

private:
	Color3 GetColorFromChunk( Chunk * cnk )
	{
		float r = 1.0f;
		float g = 1.0f;
		float b = 1.0f;

		Chunk * ColorChunk;

		ColorChunk = cnk->getChild( COLOR_F );
		if( !ColorChunk ) {
			ColorChunk = cnk->getChild( LIN_COLOR_F );
		}
		if( ColorChunk ) {
			Chunk::ChunkTypeFloat3 * colorData =
				(Chunk::ChunkTypeFloat3 *) ColorChunk->getData();

			r = colorData->x;
			g = colorData->y;
			b = colorData->z;

			ColorChunk->clearData();
		}

		ColorChunk = cnk->getChild( COLOR_24 );
		if( !ColorChunk ) {
			ColorChunk = cnk->getChild( LIN_COLOR_24 );
		}
		if( ColorChunk ) {
			Chunk::ChunkTypeChar3 * colorData =
				(Chunk::ChunkTypeChar3 *) ColorChunk->getData();

			if( colorData ) {
				r = (float)( colorData->x ) / 255.0f;
				g = (float)( colorData->y ) / 255.0f;
				b = (float)( colorData->z ) / 255.0f;
			}
			
			ColorChunk->clearData();
		}		

		return Color3( r, g, b );
	}

	float GetPercentageFromChunk( Chunk * cnk )
	{
		float f = 0.0f;

		Chunk * PercChunk = cnk->getChild( FLOAT_PERCENTAGE );
		if( PercChunk ) {
			Chunk::ChunkTypeFloat * floatData =
				(Chunk::ChunkTypeFloat *) PercChunk->getData();

			if( floatData ) {
				f = *floatData;
			}

			PercChunk->clearData();
		}

		PercChunk = cnk->getChild( INT_PERCENTAGE );
		if( PercChunk ) {
			Chunk::ChunkTypeShort * intData =
				(Chunk::ChunkTypeShort *) PercChunk->getData();

			if( intData ) {
				f = (float)( (short)( *intData ) / 255.0f );
			}

			PercChunk->clearData();
		}

		return f;
	}

	QString GetNameFromChunk( Chunk * cnk )
	{
		QString str;

		char * _str = (char *)cnk->getData();
		str = _str;
		cnk->clearData();

		return str;
	}
};

REGISTER_SPELL( sp3dsImportModel )
