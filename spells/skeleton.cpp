#include "../spellbook.h"

#include "skeleton.h"

#include "../gl/gltools.h"

#include "../NvTriStrip/qtwrapper.h"

#include <QFile>
#include <QGridLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>

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
		if ( nif->isNiBlock( iShape, "NiTriShape" ) )
		{
			QModelIndex iSkinInst = nif->getBlock( nif->getLink( iShape, "Skin Instance" ), "NiSkinInstance" );
			if ( iSkinInst.isValid() )
			{
				return nif->getBlock( nif->getLink( iSkinInst, "Data" ), "NiSkinData" ).isValid();
			}
		}
		return false;
	}
	
	typedef QPair<int,float> boneweight;
	
	typedef struct {
		QList<int> bones;
		QVector<Triangle> triangles;
	} Partition;
	
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
				iSkinPart = nif->getBlock( nif->getLink( iSkinData, "Skin Partition" ), "NiSkinPartition" );
			
			// read in the weights from NiSkinData
			
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
			
			// query max bones per vertex/partition
			
			SkinPartitionDialog dlg( maxBones );
			
			if ( dlg.exec() != QDialog::Accepted )
				return iBlock;
			
			int maxBonesPerPartition = dlg.maxBonesPerPartition();
			int maxBonesPerVertex = dlg.maxBonesPerVertex();
			
			// reduce vertex influences if necessary
			
			if ( maxBones > maxBonesPerVertex )
			{
				QMutableVectorIterator< QList< boneweight > > it( weights );
				int c = 0;
				while ( it.hasNext() )
				{
					QList< boneweight > & lst = it.next();
					
					if ( lst.count() > maxBonesPerVertex )
						c++;
					
					while ( lst.count() > maxBonesPerVertex )
					{
						int j = 0;
						float weight = lst.first().second;
						for ( int i = 0; i < lst.count(); i++ )
						{
							if ( lst[i].second < weight )
								j = i;
						}
						lst.removeAt( j );
					}
					
					float totalWeight = 0;
					foreach ( boneweight bw, lst )
					{
						totalWeight += bw.second;
					}
					
					for ( int b = 0; b < lst.count(); b++ )
					{	// normalize
						lst[b].second /= totalWeight;
					}
				}
				qWarning() << "reduced" << c << "vertices to" << maxBonesPerVertex << "bone influences (maximum number of bones per vertex was" << maxBones << ")";
			}
			
			maxBones = maxBonesPerVertex;
			
			// reduces bone weights so that the triangles fit into the partitions
			
			QList<Triangle> triangles = nif->getArray<Triangle>( iData, "Triangles" ).toList();
			
			QMap< int, int > match;
			bool doMatch = true;
			
			QList<int> tribones;
			
			int cnt = 0;
			
			foreach ( Triangle tri, triangles )
			{
				do
				{
					tribones.clear();
					for ( int c = 0; c < 3; c++ )
					{
						foreach ( boneweight bw, weights[tri[c]] )
						{
							if ( ! tribones.contains( bw.first ) )
								tribones.append( bw.first );
						}
					}
					
					if ( tribones.count() > maxBonesPerPartition )
					{
						// sum up the weights for each bone
						// bones with weight == 1 can't be removed
						
						QMap<int,float> sum;
						QList<int> nono;
						
						for ( int t = 0; t < 3; t++ )
						{
							if ( weights[tri[t]].count() == 1 )
								nono.append( weights[tri[t]].first().first );
							
							foreach ( boneweight bw, weights[ tri[t] ] )
								sum[ bw.first ] += bw.second;
						}
						
						// select the bone to remove
						
						float minWeight = 5.0;
						int minBone = -1;
						
						foreach ( int b, sum.keys() )
						{
							if ( ! nono.contains( b ) && sum[b] < minWeight )
							{
								minWeight = sum[b];
								minBone = b;
							}
						}
						
						if ( minBone < 0 )	// this shouldn't never happen
							throw QString( "internal error 0x01" );
						
						// do a vertex match detect
						
						if ( doMatch )
						{
							QVector<Vector3> verts = nif->getArray<Vector3>( iData, "Vertices" );
							for ( int a = 0; a < verts.count(); a++ )
							{
								match.insertMulti( a, a );
								for ( int b = a + 1; b < verts.count(); b++ )
								{
									if ( verts[a] == verts[b] && weights[a] == weights[b] )
									{
										match.insertMulti( a, b );
										match.insertMulti( b, a );
									}
								}
							}
						}
						
						// now remove that bone from all vertices of this triangle and from all matching vertices too
						
						for ( int t = 0; t < 3; t++ )
						{
							bool rem = false;
							foreach ( int v, match.values( tri[t] ) )
							{
								QList< boneweight > & bws = weights[ v ];
								QMutableListIterator< boneweight > it( bws );
								while ( it.hasNext() )
								{
									boneweight & bw = it.next();
									if ( bw.first == minBone )
									{
										it.remove();
										rem = true;
									}
								}
								
								float totalWeight = 0;
								foreach ( boneweight bw, bws )
								{
									totalWeight += bw.second;
								}
								
								if ( totalWeight == 0 )
									throw QString( "internal error 0x02" );
								
								for ( int b = 0; b < bws.count(); b++ )
								{	// normalize
									bws[b].second /= totalWeight;
								}
							}
							if ( rem )
								cnt++;
						}
					}
				} while ( tribones.count() > maxBonesPerPartition );
			}
			
			if ( cnt > 0 )
				qWarning() << "removed" << cnt << "bone influences";
			
			// split the triangles into partitions
			
			QList<Partition> parts;
			
			while ( ! triangles.isEmpty() )
			{
				Partition part;
				
				QHash<int,bool> usedVerts;
				
				bool addtriangles;
				do
				{
					QMutableListIterator<Triangle> it( triangles );
					while ( it.hasNext() )
					{
						Triangle & tri = it.next();
						
						QList<int> tribones;
						for ( int c = 0; c < 3; c++ )
						{
							foreach ( boneweight bw, weights[tri[c]] )
							{
								if ( ! tribones.contains( bw.first ) )
									tribones.append( bw.first );
							}
						}
						
						if ( part.bones.isEmpty() || containsBones( part.bones, tribones ) )
						{
							part.bones = mergeBones( part.bones, tribones );
							part.triangles.append( tri );
							usedVerts[ tri[0] ] = true;
							usedVerts[ tri[1] ] = true;
							usedVerts[ tri[2] ] = true;
							it.remove();
						}
					}
					
					addtriangles = false;
					
					if ( part.bones.count() < maxBonesPerPartition )
					{	// if we have room left in the partition then add an adjacent triangle
						it.toFront();
						while ( it.hasNext() )
						{
							Triangle & tri = it.next();
							
							if ( usedVerts.contains( tri[0] ) || usedVerts.contains( tri[1] ) || usedVerts.contains( tri[2] ) )
							{
								QList<int> tribones;
								for ( int c = 0; c < 3; c++ )
								{
									foreach ( boneweight bw, weights[tri[c]] )
									{
										if ( ! tribones.contains( bw.first ) )
											tribones.append( bw.first );
									}
								}
								
								tribones = mergeBones( part.bones, tribones );
								if ( tribones.count() <= maxBonesPerPartition )
								{
									part.bones = tribones;
									part.triangles.append( tri );
									usedVerts[ tri[0] ] = true;
									usedVerts[ tri[1] ] = true;
									usedVerts[ tri[2] ] = true;
									it.remove();
									addtriangles = true;
									//break;
								}
							}
						}
					}
				}
				while ( addtriangles );
				
				parts.append( part );
			}
			
			//qWarning() << parts.count() << "small partitions";
			
			// merge partitions
			
			bool merged;
			do
			{
				merged = false;
				for ( int p1 = 0; p1 < parts.count() && ! merged; p1++ )
				{
					if ( parts[p1].bones.count() < maxBonesPerPartition )
					{
						for ( int p2 = p1+1; p2 < parts.count() && ! merged; p2++ )
						{
							QList<int> mergedBones = mergeBones( parts[p1].bones, parts[p2].bones );
							if ( mergedBones.count() <= maxBonesPerPartition )
							{
								parts[p1].bones = mergedBones;
								parts[p1].triangles << parts[p2].triangles;
								parts.removeAt( p2 );
								merged = true;
							}
						}
					}
				}
			}
			while ( merged );
			
			//qWarning() << parts.count() << "partitions";
			
			// create the NiSkinPartition if it doesn't exist yet
			
			if ( ! iSkinPart.isValid() )
			{
				iSkinPart = nif->insertNiBlock( "NiSkinPartition", nif->getBlockNumber( iSkinData ) + 1 );
				nif->setLink( iSkinInst, "Skin Partition", nif->getBlockNumber( iSkinPart ) );
				nif->setLink( iSkinData, "Skin Partition", nif->getBlockNumber( iSkinPart ) );
			}
			
			// start writing NiSkinPartition
			
			nif->set<int>( iSkinPart, "Num Skin Partition Blocks", parts.count() );
			nif->updateArray( iSkinPart, "Skin Partition Blocks" );
			
			for ( int p = 0; p < parts.count(); p++ )
			{
				QModelIndex iPart = nif->getIndex( iSkinPart, "Skin Partition Blocks" ).child( p, 0 );
				
				QList<int> bones = parts[p].bones;
				qSort( bones );
				
				QVector<Triangle> triangles = parts[p].triangles;
				
				QVector<int> vertices;
				foreach ( Triangle tri, triangles )
				{
					for ( int t = 0; t < 3; t++ )
					{
						if ( ! vertices.contains( tri[t] ) )
							vertices.append( tri[t] );
					}
				}
				qSort( vertices );
				
				// map the vertices
				
				for ( int tri = 0; tri < triangles.count(); tri++ )
				{
					for ( int t = 0; t < 3; t++ )
					{
						triangles[tri][t] = vertices.indexOf( triangles[tri][t] );
					}
				}
				
				// strippify the triangles
				
				QList< QVector<quint16> > strips = strippify( triangles );
				int numTriangles = 0;
				foreach ( QVector<quint16> strip, strips )
					numTriangles += strip.count() - 2;
				
				// fill in counts
				
				nif->set<int>( iPart, "Num Vertices", vertices.count() );
				nif->set<int>( iPart, "Num Triangles", numTriangles );
				nif->set<int>( iPart, "Num Bones", bones.count() );
				nif->set<int>( iPart, "Num Strips", strips.count() );
				nif->set<int>( iPart, "Num Weights Per Vertex", maxBones );
				
				// fill in bone map
				
				QModelIndex iBoneMap = nif->getIndex( iPart, "Bones" );
				nif->updateArray( iBoneMap );
				nif->setArray<int>( iBoneMap, bones.toVector() );
				
				// fill in vertex map
				
				nif->set<int>( iPart, "Has Vertex Map", 1 );
				QModelIndex iVertexMap = nif->getIndex( iPart, "Vertex Map" );
				nif->updateArray( iVertexMap );
				nif->setArray<int>( iVertexMap, vertices );
				
				// fill in vertex weights
				
				nif->set<int>( iPart, "Has Vertex Weights", 1 );
				QModelIndex iVWeights = nif->getIndex( iPart, "Vertex Weights" );
				nif->updateArray( iVWeights );
				for ( int v = 0; v < nif->rowCount( iVWeights ); v++ )
				{
					QModelIndex iVertex = iVWeights.child( v, 0 );
					nif->updateArray( iVertex );
					QList< boneweight > list = weights.value( vertices[v] );
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
					QList< boneweight > list = weights.value( vertices[v] );
					for ( int b = 0; b < maxBones; b++ )
						nif->set<int>( iVertex.child( b, 0 ), list.count() > b ? bones.indexOf( list[ b ].first ) : 0 );
				}
			}
			
			// done
			
			return iShape;
		}
		catch ( QString err )
		{
			if ( ! err.isEmpty() )
				QMessageBox::warning( 0, "NifSkope", err );
			return iShape;
		}
	}
	
	static QList<int> mergeBones( QList<int> a, QList<int> b )
	{
		foreach ( int c, b )
		{
			if ( ! a.contains( c ) )
			{
				a.append( c );
			}
		}
		return a;
	}
	
	static bool containsBones( QList<int> a, QList<int> b )
	{
		foreach ( int c, b )
		{
			if ( ! a.contains( c ) )
				return false;
		}
		return true;
	}
};

REGISTER_SPELL( spSkinPartition )


SkinPartitionDialog::SkinPartitionDialog( int ) : QDialog()
{
	spnVert = new QSpinBox( this );
	spnVert->setMinimum( 1 );
	spnVert->setMaximum( 8 );
	spnVert->setValue( 4 );
	connect( spnVert, SIGNAL( valueChanged( int ) ), this, SLOT( changed() ) );
	
	spnPart = new QSpinBox( this );
	spnPart->setMinimum( 4 );
	spnPart->setMaximum( 40 );
	spnPart->setValue( 18 );
	
	QLabel * labVert = new QLabel( this );
	labVert->setText(
	"<b>Number of Bones per Vertex</b><br>"
	"Hint: Most games use 4 bones per vertex<br>"
	"Note: If the mesh contains vertices which are<br>"
	"influenced by more than x bones the number of<br>"
	"influences will be reduced for these vertices<br>"
	);
	
	QLabel * labPart = new QLabel( this );
	labPart->setText(
	"<b>Number of Bones per Partition</b><br>"
	"Hint: Oblivion uses 18 bones pp<br>"
	"CivIV (non shader meshes) 4 bones pp<br>"
	"CivIV (shader enabled meshes) 18 bones pp<br>"
	"Note: To fit the triangles into the partitions<br>"
	"some bone influences may be removed again."
	);
	
	QPushButton * btOk = new QPushButton( this );
	btOk->setText( "Ok" );
	connect( btOk, SIGNAL( clicked() ), this, SLOT( accept() ) );
	
	QPushButton * btCancel = new QPushButton( this );
	btCancel->setText( "Cancel" );
	connect( btCancel, SIGNAL( clicked() ), this, SLOT( reject() ) );
	
	QGridLayout * grid = new QGridLayout( this );
	grid->addWidget( labVert, 0, 0 );
	grid->addWidget( labPart, 1, 0 );
	grid->addWidget( spnVert, 0, 1 );
	grid->addWidget( spnPart, 1, 1 );
	grid->addWidget( btOk, 2, 0 );
	grid->addWidget( btCancel, 2, 1 );
}

void SkinPartitionDialog::changed()
{
	spnPart->setMinimum( spnVert->value() );
}

int SkinPartitionDialog::maxBonesPerVertex()
{
	return spnVert->value();
}

int SkinPartitionDialog::maxBonesPerPartition()
{
	return spnPart->value();
}


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
			Vector3 mn;
			Vector3 mx;
			
			Vector3 center;
			float radius = 0;
			
			QModelIndex iWeightList = nif->getIndex( iBoneDataList.child( b, 0 ), "Vertex Weights" );
			for ( int w = 0; w < nif->rowCount( iWeightList ); w++ )
			{
				int v = nif->get<int>( iWeightList.child( w, 0 ), "Index" );
				if ( w == 0 )
				{
					mn = verts.value( v );
					mx = verts.value( v );
				}
				else
				{
					mn.boundMin( verts.value( v ) );
					mx.boundMax( verts.value( v ) );
				}
			}
			
			mn = meshTrans * mn;
			mx = meshTrans * mx;
			
			Transform bt( boneTrans[b] );
			mn = bt.rotation.inverted() * ( mn - bt.translation ) / bt.scale;
			mx = bt.rotation.inverted() * ( mx - bt.translation ) / bt.scale;
			
			center = ( mn + mx ) / 2;
			radius = qMax( ( mn - center ).length(), ( mx - center ).length() );
			
			nif->set<Vector3>( iBoneDataList.child( b, 0 ), "Bounding Sphere Offset", center );
			nif->set<float>( iBoneDataList.child( b, 0 ), "Bounding Sphere Radius", radius );
		}
		
		return iSkinData;
	}
	
};

REGISTER_SPELL( spFixBoneBounds )
