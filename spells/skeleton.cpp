#include "../spellbook.h"

#include "../gl/gltools.h"

#include "../NvTriStrip/qtwrapper.h"

#include <QFile>
#include <QMessageBox>

#define SKEL_DAT ":/res/spells/skel.dat"

QDebug operator<<( QDebug dbg, const Vector3 & v )
{
	return dbg << v[0] << v[1] << v[2];
}

typedef QMap<QString,Transform> TransMap;

class spFixSkeleton : public Spell
{
public:
	QString name() const { return "Fix Bip01"; }
	QString page() const { return "Skeleton"; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return ( nif->getVersion() == "4.0.0.2" && nif->itemType( index ) == "NiBlock" && nif->get<QString>( index, "Name" ) == "Bip01" ); //&& QFile::exists( SKEL_DAT ) );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		QFile file( SKEL_DAT );
		if ( file.open( QIODevice::ReadOnly ) )
		{
			QDataStream stream( &file );
			
			TransMap local;
			TransMap world;
			QString name;
			do
			{
				stream >> name;
				if ( !name.isEmpty() )
				{
					Transform t;
					stream >> t;
					local.insert( name, t );
					stream >> t;
					world.insert( name, t );
				}
			}
			while ( ! name.isEmpty() );
			
			TransMap bones;
			doBones( nif, index, Transform(), local, bones );
			
			foreach ( int link, nif->getChildLinks( nif->getBlockNumber( index ) ) )
			{
				QModelIndex iChild = nif->getBlock( link );
				if ( iChild.isValid() )
				{
					if ( nif->itemName( iChild ) == "NiNode" )
					{
						doNodes( nif, iChild, Transform(), world, bones );
					}
					else if ( nif->inherits( iChild, "NiTriBasedGeom" ) )
					{
						doShape( nif, iChild, Transform(), world, bones );
					}
				}
			}
			
			nif->reset();
		}
		
		return index;
	}
	
	void doBones( NifModel * nif, const QModelIndex & index, const Transform & tparent, const TransMap & local, TransMap & bones )
	{
		QString name = nif->get<QString>( index, "Name" );
		if ( name.startsWith( "Bip01" ) )
		{
			Transform tlocal( nif, index );
			bones.insert( name, tparent * tlocal );
			
			local.value( name ).writeBack( nif, index );
			
			foreach ( int link, nif->getChildLinks( nif->getBlockNumber( index ) ) )
			{
				QModelIndex iChild = nif->getBlock( link, "NiNode" );
				if ( iChild.isValid() )
					doBones( nif, iChild, tparent * tlocal, local, bones );
			}
		}
	}
	
	bool doNodes( NifModel * nif, const QModelIndex & index, const Transform & tparent, const TransMap & world, const TransMap & bones )
	{
		bool hasSkinnedChildren = false;
		
		QString name = nif->get<QString>( index, "Name" );
		if ( ! name.startsWith( "Bip01" ) )
		{
			Transform tlocal( nif, index );
			
			foreach ( int link, nif->getChildLinks( nif->getBlockNumber( index ) ) )
			{
				QModelIndex iChild = nif->getBlock( link );
				if ( iChild.isValid() )
				{
					if ( nif->itemName( iChild ) == "NiNode" )
					{
						hasSkinnedChildren |= doNodes( nif, iChild, tparent * tlocal, world, bones );
					}
					else if ( nif->inherits( iChild, "NiTriBasedGeom" ) )
					{
						hasSkinnedChildren |= doShape( nif, iChild, tparent * tlocal, world, bones );
					}
				}
			}
		}
		
		if ( hasSkinnedChildren )
		{
			Transform().writeBack( nif, index );
		}
		
		return hasSkinnedChildren;
	}
	bool doShape( NifModel * nif, const QModelIndex & index, const Transform & tparent, const TransMap & world, const TransMap & bones )
	{
		QModelIndex iShapeData = nif->getBlock( nif->getLink( index, "Data" ) );
		QModelIndex iSkinInstance = nif->getBlock( nif->getLink( index, "Skin Instance" ), "NiSkinInstance" );
		if ( ! iSkinInstance.isValid() || ! iShapeData.isValid() )
			return false;
		QStringList names;
		QModelIndex iNames = nif->getIndex( iSkinInstance, "Bones" );
		if ( iNames.isValid() )
			iNames = nif->getIndex( iNames, "Bones" );
		if ( iNames.isValid() )
			for ( int n = 0; n < nif->rowCount( iNames ); n++ )
			{
				QModelIndex iBone = nif->getBlock( nif->getLink( iNames.child( n, 0 ) ), "NiNode" );
				if ( iBone.isValid() )
					names.append( nif->get<QString>( iBone, "Name" ) );
				else
					names.append("");
			}
		QModelIndex iSkinData = nif->getBlock( nif->getLink( iSkinInstance, "Data" ), "NiSkinData" );
		if ( !iSkinData.isValid() )
			return false;
		QModelIndex iBones = nif->getIndex( iSkinData, "Bone List" );
		if ( ! iBones.isValid() )
			return false;
		
		Transform t( nif, iSkinData );
		t = tparent * t;
		t.writeBack( nif, iSkinData );
		
		for ( int b = 0; b < nif->rowCount( iBones ) && b < names.count(); b++ )
		{
			QModelIndex iBone = iBones.child( b, 0 );
			
			t = Transform( nif, iBone );
			
			t.rotation = world.value( names[ b ] ).rotation.inverted() * bones.value( names[ b ] ).rotation * t.rotation;
			t.translation = world.value( names[ b ] ).rotation.inverted() * bones.value( names[ b ] ).rotation * t.translation;
			
			t.writeBack( nif, iBone );
		}
		
		Vector3 center = nif->get<Vector3>( iShapeData, "Center" );
		nif->set<Vector3>( iShapeData, "Center", tparent * center );
		return true;
	}
};

REGISTER_SPELL( spFixSkeleton )


class spScanSkeleton : public Spell
{
public:
	QString name() const { return "Scan Bip01"; }
	QString page() const { return "Skeleton"; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return ( nif->getVersion() == "4.0.0.2" && nif->itemType( index ) == "NiBlock" && nif->get<QString>( index, "Name" ) == "Bip01" );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		QFile file( SKEL_DAT );
		if ( file.open( QIODevice::WriteOnly ) )
		{
			QDataStream stream( &file );
			scan( nif, index, Transform(), stream );
			stream << QString();
		}
		return index;
	}
	
	void scan( NifModel * nif, const QModelIndex & index, const Transform & tparent, QDataStream & stream )
	{
		QString name = nif->get<QString>( index, "Name" );
		if ( name.startsWith( "Bip01" ) )
		{
			Transform local( nif, index );
			stream << name << local << tparent * local;
			qWarning() << name;
			foreach ( int link, nif->getChildLinks( nif->getBlockNumber( index ) ) )
			{
				QModelIndex iChild = nif->getBlock( link, "NiNode" );
				if ( iChild.isValid() )
					scan( nif, iChild, tparent * local, stream );
			}
		}
	}
};

//REGISTER_SPELL( spScanSkeleton )


class spSkinPartition : public Spell
{
public:
	QString name() const { return "Make Skin Partition"; }
	QString page() const { return "Mesh"; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & iShape )
	{
		if ( nif->checkVersion( 0x0a020000, 0 ) && nif->isNiBlock( iShape, "NiTriShape" ) )
		{
			QModelIndex iSkinInst = nif->getBlock( nif->getLink( iShape, "Skin Instance" ), "NiSkinInstance" );
			if ( iSkinInst.isValid() )
			{
				return nif->getBlock( nif->getLink( iSkinInst, "Data" ), "NiSkinData" ).isValid();
			}
		}
		return false;
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & iBlock )
	{
		QPersistentModelIndex iShape = iBlock;
		try
		{
			QPersistentModelIndex iData = nif->getBlock( nif->getLink( iShape, "Data" ), "NiTriShapeData" );
			QPersistentModelIndex iSkinInst = nif->getBlock( nif->getLink( iShape, "Skin Instance" ), "NiSkinInstance" );
			QPersistentModelIndex iSkinData = nif->getBlock( nif->getLink( iSkinInst, "Data" ), "NiSkinData" );
			QModelIndex iSkinPart = nif->getBlock( nif->getLink( iSkinInst, "Skin Partition" ), "NiSkinPartition" );
			if ( ! iSkinPart.isValid() )
			{
				iSkinPart = nif->insertNiBlock( "NiSkinPartition", nif->getBlockNumber( iSkinData ) + 1 );
				nif->setLink( iSkinInst, "Skin Partition", nif->getBlockNumber( iSkinPart ) );
			}
			
			// read in the weights from NiSkinData
			
			typedef QPair<int,float> boneweight;
			
			QVector< QList< boneweight > > weights( nif->get<int>( iData, "Num Vertices" ) );
			
			QModelIndex iBoneList = nif->getIndex( iSkinData, "Bone List" );
			int numBones = nif->rowCount( iBoneList );
			for ( int bone = 0; bone < numBones; bone++ )
			{
				QModelIndex iVertexWeights = nif->getIndex( iBoneList.child( bone, 0 ), "Vertex Weights" );
				for ( int r = 0; r < nif->rowCount( iVertexWeights ); r++ )
				{
					int vertex = nif->get<int>( iVertexWeights.child( r, 0 ), "Index" );
					float weight = nif->get<float>( iVertexWeights.child( r, 0 ), "Weight" );
					if ( vertex >= weights.count() )
						throw QString( "bad NiSkinData - vertex count does not match" );
					weights[vertex].append( boneweight( bone, weight ) );
				}
			}
			
			// count min and max bones per vertex
			
			int minBones, maxBones;
			minBones = maxBones = weights.value( 0 ).count();
			foreach ( QList< boneweight > list, weights )
			{
				if ( list.count() < minBones )
					minBones = list.count();
				if ( list.count() > maxBones )
					maxBones = list.count();
			}
			
			if ( minBones <= 0 )
				throw QString( "bad NiSkinData - some vertices have no weights at all" );
			
			// official skin partitions have 4 weights per vertex. let's make sure we do the same
			if ( maxBones <= 4 )
				maxBones = 4;
			else
				throw QString( "Too many bones per vertex.<br>Consider reskinning the mesh with not more than four weights per vertex." );
			
			// strippify the triangles
			
			QVector<Triangle> triangles = nif->getArray<Triangle>( iData, "Triangles" );
			QList< QVector<quint16> > strips = strippify( triangles );
			int numTriangles = 0;
			foreach ( QVector<quint16> strip, strips )
				numTriangles += strip.count() - 2;
			
			// start writing NiSkinPartition
			
			nif->set<int>( iSkinPart, "Num Skin Partition Blocks", 1 );
			nif->updateArray( iSkinPart, "Skin Partition Blocks" );
			QModelIndex iPart = nif->getIndex( iSkinPart, "Skin Partition Blocks" ).child( 0, 0 );
			
			nif->set<int>( iPart, "Num Vertices", weights.count() );
			nif->set<int>( iPart, "Num Triangles", numTriangles );
			nif->set<int>( iPart, "Num Bones", qMax( numBones, maxBones ) );
			nif->set<int>( iPart, "Num Strips", strips.count() );
			nif->set<int>( iPart, "Num Weights Per Vertex", maxBones );
			
			// fill in bone map
			
			QModelIndex iBoneMap = nif->getIndex( iPart, "Bones" );
			nif->updateArray( iBoneMap );
			for ( int bone = 0; bone < nif->rowCount( iBoneMap ); bone++ )
				nif->set<int>( iBoneMap.child( bone, 0 ), bone < numBones ? bone : 0 );
			
			// vertex map (not needed i thought, but nvidia cards seem to rely on it)
			
			nif->set<int>( iPart, "Has Vertex Map", 1 );
			QModelIndex iVertexMap = nif->getIndex( iPart, "Vertex Map" );
			nif->updateArray( iVertexMap );
			for ( int vertex = 0; vertex < nif->rowCount( iVertexMap ); vertex++ )
				nif->set<int>( iVertexMap.child( vertex, 0 ), vertex );
			
			// fill in vertex weights
			
			nif->set<int>( iPart, "Has Vertex Weights", 1 );
			QModelIndex iVWeights = nif->getIndex( iPart, "Vertex Weights" );
			nif->updateArray( iVWeights );
			for ( int v = 0; v < nif->rowCount( iVWeights ); v++ )
			{
				QModelIndex iVertex = iVWeights.child( v, 0 );
				nif->updateArray( iVertex );
				QList< boneweight > list = weights.value( v );
				for ( int b = 0; b < maxBones; b++ )
					nif->set<float>( iVertex.child( b, 0 ), list.count() > b ? list[ b ].second : 0.0 );
			}
			
			// write the strips
			
			nif->set<int>( iPart, "Has Strips", 1 );
			QModelIndex iStripLengths = nif->getIndex( iPart, "Strip Lengths" );
			nif->updateArray( iStripLengths );
			for ( int s = 0; s < nif->rowCount( iStripLengths ); s++ )
				nif->set<int>( iStripLengths.child( s, 0 ), strips.value( s ).count() );
			
			QModelIndex iStrips = nif->getIndex( iPart, "Strips" );
			nif->updateArray( iStrips );
			for ( int s = 0; s < nif->rowCount( iStrips ); s++ )
			{
				nif->updateArray( iStrips.child( s, 0 ) );
				nif->setArray<quint16>( iStrips.child( s, 0 ), strips.value( s ) );
			}
			
			// fill in vertex bones
			
			nif->set<int>( iPart, "Has Bone Indices", 1 );
			QModelIndex iVBones = nif->getIndex( iPart, "Bone Indices" );
			nif->updateArray( iVBones );
			for ( int v = 0; v < nif->rowCount( iVBones ); v++ )
			{
				QModelIndex iVertex = iVBones.child( v, 0 );
				nif->updateArray( iVertex );
				QList< boneweight > list = weights.value( v );
				for ( int b = 0; b < maxBones; b++ )
					nif->set<int>( iVertex.child( b, 0 ), list.count() > b ? list[ b ].first : 0 );
			}
			
			// done
			
			return iShape;
		}
		catch ( QString err )
		{
			QMessageBox::warning( 0, "NifSkope", err );
			return iShape;
		}
	}
};

REGISTER_SPELL( spSkinPartition )


class spFixBoneBounds : public Spell
{
public:
	QString name() const { return "Fix Bone Bounds"; }
	QString page() const { return "Skeleton"; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return nif->isNiBlock( index, "NiSkinData" );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & iSkinData )
	{
		QModelIndex iSkinInstance = nif->getBlock( nif->getParent( nif->getBlockNumber( iSkinData ) ), "NiSkinInstance" );
		QModelIndex iMesh = nif->getBlock( nif->getParent( nif->getBlockNumber( iSkinInstance ) ) );
		QModelIndex iMeshData = nif->getBlock( nif->getLink( iMesh, "Data" ) );
		int skelRoot = nif->getLink( iSkinInstance, "Skeleton Root" );
		if ( ! nif->inherits( iMeshData, "NiTriBasedGeomData" ) || skelRoot < 0 || skelRoot != nif->getParent( nif->getBlockNumber( iMesh ) ) )
			return iSkinData;
		
		Transform meshTrans( nif, iMesh );
		
		QVector<Transform> boneTrans;
		QModelIndex iBoneMap = nif->getIndex( iSkinInstance, "Bones" );
		for ( int n = 0; n < nif->rowCount( iBoneMap ); n++ )
		{
			QModelIndex iBone = nif->getBlock( nif->getLink( iBoneMap.child( n, 0 ) ), "NiNode" );
			if ( skelRoot != nif->getParent( nif->getBlockNumber( iBone ) ) )
				return iSkinData;
			boneTrans.append( Transform( nif, iBone ) );
		}
		
		QVector<Vector3> verts = nif->getArray<Vector3>( iMeshData, "Vertices" );
		
		QModelIndex iBoneDataList = nif->getIndex( iSkinData, "Bone List" );
		for ( int b = 0; b < nif->rowCount( iBoneDataList ); b++ )
		{
			Vector3 center;
			float radius = 0;
			
			QModelIndex iWeightList = nif->getIndex( iBoneDataList.child( b, 0 ), "Vertex Weights" );
			for ( int w = 0; w < nif->rowCount( iWeightList ); w++ )
			{
				int v = nif->get<int>( iWeightList.child( w, 0 ), "Index" );
				center += verts.value( v );
			}
			
			if ( nif->rowCount( iWeightList ) > 0 )
				center /= nif->rowCount( iWeightList );
			
			for ( int w = 0; w < nif->rowCount( iWeightList ); w++ )
			{
				int v = nif->get<int>( iWeightList.child( w, 0 ), "Index" );
				float r = ( center - verts.value( v ) ).length();
				if ( r > radius )
					radius = r;
			}
			
			center = meshTrans * center;
			
			Transform bt( boneTrans[b] );
			center = bt.rotation.inverted() * ( center - bt.translation ) / bt.scale;
			
			nif->set<Vector3>( iBoneDataList.child( b, 0 ), "Center", center );
			nif->set<float>( iBoneDataList.child( b, 0 ), "Radius", radius );
		}
		
		return iSkinData;
	}
	
};

// REGISTER_SPELL( spFixBoneBounds )
