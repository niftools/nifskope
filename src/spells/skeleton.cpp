#include "skeleton.h"
#include "convert.h"
#include "stripify.h"

#include "spellbook.h"
#include "gl/gltools.h"

#include "lib/nvtristripwrapper.h"

#include <QCheckBox>
#include <QFile>
#include <QGridLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>

#include <algorithm> // std::sort

#define SKEL_DAT ":/res/skel.dat"


// Brief description is deliberately not autolinked to class Spell
/*! \file skeleton.cpp
 * \brief Skeleton spells
 *
 * All classes here inherit from the Spell class.
 */

//! A map of bone names to their transforms
typedef QMap<QString, Transform> TransMap;

//! "Fix" a v4.0.0.2 skeleton
class spFixSkeleton final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Fix Bip01" ); }
	QString page() const override final { return Spell::tr( "Skeleton" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		return ( nif->getVersion() == "4.0.0.2" && nif->itemType( index ) == "NiBlock" && nif->get<QString>( index, "Name" ) == "Bip01" ); //&& QFile::exists( SKEL_DAT ) );
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		QFile file( SKEL_DAT );

		if ( file.open( QIODevice::ReadOnly ) ) {
			QDataStream stream( &file );

			TransMap local;
			TransMap world;
			QString name;

			do {
				stream >> name;

				if ( !name.isEmpty() ) {
					Transform t;
					stream >> t;
					local.insert( name, t );
					stream >> t;
					world.insert( name, t );
				}
			} while ( !name.isEmpty() );

			TransMap bones;
			doBones( nif, index, Transform(), local, bones );

			for ( const auto link : nif->getChildLinks( nif->getBlockNumber( index ) ) ) {
				QModelIndex iChild = nif->getBlock( link );

				if ( iChild.isValid() ) {
					if ( nif->itemName( iChild ) == "NiNode" ) {
						doNodes( nif, iChild, Transform(), world, bones );
					} else if ( nif->inherits( iChild, "NiTriBasedGeom" ) ) {
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

		if ( name.startsWith( "Bip01" ) ) {
			Transform tlocal( nif, index );
			bones.insert( name, tparent * tlocal );

			local.value( name ).writeBack( nif, index );

			for ( const auto link : nif->getChildLinks( nif->getBlockNumber( index ) ) ) {
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

		if ( !name.startsWith( "Bip01" ) ) {
			Transform tlocal( nif, index );

			for ( const auto link : nif->getChildLinks( nif->getBlockNumber( index ) ) ) {
				QModelIndex iChild = nif->getBlock( link );

				if ( iChild.isValid() ) {
					if ( nif->itemName( iChild ) == "NiNode" ) {
						hasSkinnedChildren |= doNodes( nif, iChild, tparent * tlocal, world, bones );
					} else if ( nif->inherits( iChild, "NiTriBasedGeom" ) ) {
						hasSkinnedChildren |= doShape( nif, iChild, tparent * tlocal, world, bones );
					}
				}
			}
		}

		if ( hasSkinnedChildren ) {
			Transform().writeBack( nif, index );
		}

		return hasSkinnedChildren;
	}
	bool doShape( NifModel * nif, const QModelIndex & index, const Transform & tparent, const TransMap & world, const TransMap & bones )
	{
		QModelIndex iShapeData = nif->getBlock( nif->getLink( index, "Data" ) );
		QModelIndex iSkinInstance = nif->getBlock( nif->getLink( index, "Skin Instance" ), "NiSkinInstance" );

		if ( !iSkinInstance.isValid() || !iShapeData.isValid() )
			return false;

		QStringList names;
		QModelIndex iNames = nif->getIndex( iSkinInstance, "Bones" );

		if ( iNames.isValid() )
			iNames = nif->getIndex( iNames, "Bones" );

		if ( iNames.isValid() )
			for ( int n = 0; n < nif->rowCount( iNames ); n++ ) {
				QModelIndex iBone = nif->getBlock( nif->getLink( iNames.child( n, 0 ) ), "NiNode" );

				if ( iBone.isValid() )
					names.append( nif->get<QString>( iBone, "Name" ) );
				else
					names.append( "" );
			}


		QModelIndex iSkinData = nif->getBlock( nif->getLink( iSkinInstance, "Data" ), "NiSkinData" );

		if ( !iSkinData.isValid() )
			return false;

		QModelIndex iBones = nif->getIndex( iSkinData, "Bone List" );

		if ( !iBones.isValid() )
			return false;

		Transform t( nif, iSkinData );
		t = tparent * t;
		t.writeBack( nif, iSkinData );

		for ( int b = 0; b < nif->rowCount( iBones ) && b < names.count(); b++ ) {
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

//! Read skeleton data for use in Fix Skeleton
class spScanSkeleton final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Scan Bip01" ); }
	QString page() const override final { return Spell::tr( "Skeleton" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		return ( nif->getVersion() == "4.0.0.2" && nif->itemType( index ) == "NiBlock" && nif->get<QString>( index, "Name" ) == "Bip01" );
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		QFile file( SKEL_DAT );

		if ( file.open( QIODevice::WriteOnly ) ) {
			QDataStream stream( &file );
			scan( nif, index, Transform(), stream );
			stream << QString();
		}

		return index;
	}

	void scan( NifModel * nif, const QModelIndex & index, const Transform & tparent, QDataStream & stream )
	{
		QString name = nif->get<QString>( index, "Name" );

		if ( name.startsWith( "Bip01" ) ) {
			Transform local( nif, index );
			stream << name << local << tparent * local;
			qDebug() << name;
			for ( const auto link : nif->getChildLinks( nif->getBlockNumber( index ) ) ) {
				QModelIndex iChild = nif->getBlock( link, "NiNode" );

				if ( iChild.isValid() )
					scan( nif, iChild, tparent * local, stream );
			}
		}
	}
};

//REGISTER_SPELL( spScanSkeleton )

//! Unknown; unused?
template <> inline bool qMapLessThanKey<Triangle>( const Triangle & s1, const Triangle & s2 )
{
	// TODO: This function makes no sense. Doxygen comments seem to think it's unused.
	int d = 0;

	if ( d == 0 ) d = (s1[0] - s2[0]);
	if ( d == 0 ) d = (s1[1] - s2[1]);
	if ( d == 0 ) d = (s1[2] - s2[2]);

	return d < 0;
}

//! Rotate a Triangle
inline void qRotate( Triangle & t )
{
	if ( t[1] < t[0] && t[1] < t[2] ) {
		t.set( t[1], t[2], t[0] );
	} else if ( t[2] < t[0] ) {
		t.set( t[2], t[0], t[1] );
	}
}

//! Make skin partition
class spSkinPartition final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Make Skin Partition" ); }
	QString page() const override final { return Spell::tr( "Mesh" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & iShape ) override final
	{
		if ( nif->isNiBlock( iShape, { "NiTriShape", "NiTriStrips" } ) ) {
			QModelIndex iSkinInst = nif->getBlock( nif->getLink( iShape, "Skin Instance" ), "NiSkinInstance" );

			if ( iSkinInst.isValid() ) {
				return nif->getBlock( nif->getLink( iSkinInst, "Data" ), "NiSkinData" ).isValid();
			}
		}

		return false;
	}

	typedef QPair<int, float> boneweight;

	//! Helper for sorting a boneweight list
	struct boneweight_equivalence
	{
		bool operator()( const boneweight & lhs, const boneweight & rhs )
		{
			if ( lhs.second == 0.0 ) {
				if ( rhs.second == 0.0 ) {
					return rhs.first < lhs.first;
				} else {
					return true;
				}

				return false;
			} else if ( rhs.second == lhs.second ) {
				return lhs.first < rhs.first;
			} else {
				return rhs.second < lhs.second;
			}
		}
	};

	//! A bone and Triangle set
	typedef struct
	{
		QList<int> bones;
		QVector<Triangle> triangles;
	} Partition;

	QModelIndex cast( NifModel * nif, const QModelIndex & iBlock ) override final
	{
		int mbpp = 0, mbpv = 0;
		bool make_strips = false;
		return cast( nif, iBlock, mbpp, mbpv, make_strips );
	}

	//! Cast with extra parameters
	QModelIndex cast( NifModel * nif, const QModelIndex & iBlock, int & maxBonesPerPartition, int & maxBonesPerVertex, bool make_strips, bool pad = false )
	{
		QPersistentModelIndex iShape = iBlock;
		QString iShapeType = "";

		if ( nif->isNiBlock( iShape, "NiTriShape" ) ) {
			iShapeType = "NiTriShape";
		} else if ( nif->isNiBlock( iShape, "NiTriStrips" ) ) {
			iShapeType = "NiTriStrips";
		}

		try
		{
			QPersistentModelIndex iData;

			if ( iShapeType == "NiTriShape" ) {
				iData = nif->getBlock( nif->getLink( iShape, "Data" ), "NiTriShapeData" );
			} else if ( iShapeType == "NiTriStrips" ) {
				iData = nif->getBlock( nif->getLink( iShape, "Data" ), "NiTriStripsData" );
			}

			QPersistentModelIndex iSkinInst = nif->getBlock( nif->getLink( iShape, "Skin Instance" ), "NiSkinInstance" );
			QPersistentModelIndex iSkinData = nif->getBlock( nif->getLink( iSkinInst, "Data" ), "NiSkinData" );
			QModelIndex iSkinPart = nif->getBlock( nif->getLink( iSkinInst, "Skin Partition" ), "NiSkinPartition" );

			if ( !iSkinPart.isValid() )
				iSkinPart = nif->getBlock( nif->getLink( iSkinData, "Skin Partition" ), "NiSkinPartition" );

			// read in the weights from NiSkinData

			int numVerts = nif->get<int>( iData, "Num Vertices" );
			QVector<QList<boneweight> > weights( numVerts );

			QModelIndex iBoneList = nif->getIndex( iSkinData, "Bone List" );
			int numBones = nif->rowCount( iBoneList );

			for ( int bone = 0; bone < numBones; bone++ ) {
				QModelIndex iVertexWeights = nif->getIndex( iBoneList.child( bone, 0 ), "Vertex Weights" );

				for ( int r = 0; r < nif->rowCount( iVertexWeights ); r++ ) {
					int vertex = nif->get<int>( iVertexWeights.child( r, 0 ), "Index" );
					float weight = nif->get<float>( iVertexWeights.child( r, 0 ), "Weight" );

					if ( vertex >= weights.count() )
						throw QString( Spell::tr( "bad NiSkinData - vertex count does not match" ) );

					weights[vertex].append( boneweight( bone, weight ) );
				}
			}

			// count min and max bones per vertex

			int minBones, maxBones;
			minBones = maxBones = weights.value( 0 ).count();
			for ( const QList<boneweight> list : weights ) {
				if ( list.count() < minBones )
					minBones = list.count();

				if ( list.count() > maxBones )
					maxBones = list.count();
			}

			if ( minBones <= 0 )
				throw QString( Spell::tr( "bad NiSkinData - some vertices have no weights at all" ) );

			// query max bones per vertex/partition

			if ( maxBonesPerPartition <= 0 || maxBonesPerVertex <= 0 ) {
				SkinPartitionDialog dlg( maxBones );

				if ( dlg.exec() != QDialog::Accepted )
					return iBlock;

				maxBonesPerPartition = dlg.maxBonesPerPartition();
				maxBonesPerVertex = dlg.maxBonesPerVertex();
				make_strips = dlg.makeStrips();
				pad = dlg.padPartitions();
			}

			// reduce vertex influences if necessary

			if ( maxBones > maxBonesPerVertex ) {
				QMutableVectorIterator<QList<boneweight> > it( weights );
				int c = 0;

				while ( it.hasNext() ) {
					QList<boneweight> & lst = it.next();
					std::sort( lst.begin(), lst.end(), boneweight_equivalence() );

					if ( lst.count() > maxBonesPerVertex )
						c++;

					while ( lst.count() > maxBonesPerVertex ) {
						lst.removeLast();
					}

					float totalWeight = 0;
					for ( const auto bw : lst ) {
						totalWeight += bw.second;
					}

					for ( int b = 0; b < lst.count(); b++ ) {
						// normalize
						lst[b].second /= totalWeight;
					}
				}

				qCWarning( nsSpell ) << Spell::tr( "Reduced %1 vertices to %2 bone influences (maximum number of bones per vertex was %3)" )
					.arg( c )
					.arg( maxBonesPerVertex )
					.arg( maxBones );
			}

			maxBones = maxBonesPerVertex;

			// reduces bone weights so that the triangles fit into the partitions

			QList<Triangle> triangles;

			if ( iShapeType == "NiTriShape" ) {
				triangles = nif->getArray<Triangle>( iData, "Triangles" ).toList();
			} else if ( iShapeType == "NiTriStrips" ) {
				// triangulate first (code copied from strippify.cpp)
				QVector<QVector<quint16> > strips;
				QModelIndex iPoints = nif->getIndex( iData, "Points" );

				for ( int s = 0; s < nif->rowCount( iPoints ); s++ ) {
					QVector<quint16> strip;
					QModelIndex iStrip = iPoints.child( s, 0 );

					for ( int p = 0; p < nif->rowCount( iStrip ); p++ ) {
						strip.append( nif->get<int>( iStrip.child( p, 0 ) ) );
					}

					strips.append( strip );
				}

				triangles = triangulate( strips ).toList();
			}

			QMap<Triangle, quint32> trimap;
			quint32 defaultPart = 0;

			if ( nif->inherits( iSkinInst, "BSDismemberSkinInstance" ) ) {
				// First find a partition to dump dangling faces.  Torso is prefered if available.
				quint32 nparts = nif->get<uint>( iSkinInst, "Num Partitions" );
				QModelIndex iPartData = nif->getIndex( iSkinInst, "Partitions" );

				for ( quint32 i = 0; i < nparts; ++i ) {
					QModelIndex iPart = iPartData.child( i, 0 );

					if ( !iPart.isValid() )
						continue;

					if ( nif->get<uint>( iPart, "Body Part" ) == 0 /* Torso */ ) {
						defaultPart = i;
						break;
					}
				}

				defaultPart = qMin( nparts - 1, defaultPart );

				// enumerate existing partitions and select faces into same partition
				quint32 nskinparts = nif->get<int>( iSkinPart, "Num Skin Partition Blocks" );
				iPartData = nif->getIndex( iSkinPart, "Skin Partition Blocks" );

				for ( quint32 i = 0; i < nskinparts; ++i ) {
					QModelIndex iPart = iPartData.child( i, 0 );

					if ( !iPart.isValid() )
						continue;

					quint32 finalPart = qMin( nparts - 1, i );

					QVector<int> vertmap = nif->getArray<int>( iPart, "Vertex Map" );

					quint8 hasFaces  = nif->get<quint8>( iPart, "Has Faces" );
					quint8 numStrips = nif->get<quint8>( iPart, "Num Strips" );
					QVector<Triangle> partTriangles;

					if ( hasFaces && numStrips == 0 ) {
						partTriangles = nif->getArray<Triangle>( iPart, "Triangles" );
					} else if ( numStrips != 0 ) {
						// triangulate first (code copied from strippify.cpp)
						QVector<QVector<quint16> > strips;
						QModelIndex iPoints = nif->getIndex( iPart, "Strips" );

						for ( int s = 0; s < nif->rowCount( iPoints ); s++ ) {
							QVector<quint16> strip;
							QModelIndex iStrip = iPoints.child( s, 0 );

							for ( int p = 0; p < nif->rowCount( iStrip ); p++ ) {
								strip.append( nif->get<int>( iStrip.child( p, 0 ) ) );
							}

							strips.append( strip );
						}

						partTriangles = triangulate( strips );
					}

					for ( int j = 0; j < partTriangles.count(); ++j ) {
						Triangle t = partTriangles[j];
						Triangle tri = t;

						if ( !vertmap.isEmpty() ) {
							tri[0] = vertmap[tri[0]];
							tri[1] = vertmap[tri[1]];
							tri[2] = vertmap[tri[2]];
						}

						qRotate( tri );
						trimap.insert( tri, finalPart );
					}
				}
			}

			QMap<int, int> match;
			bool doMatch = true;

			QList<int> tribones;

			int cnt = 0;

			for ( const Triangle& tri : triangles )
			{
				do {
					tribones.clear();

					for ( int c = 0; c < 3; c++ ) {
						for ( const auto bw : weights[tri[c]] ) {
							if ( !tribones.contains( bw.first ) )
								tribones.append( bw.first );
						}
					}

					if ( tribones.count() > maxBonesPerPartition ) {
						// sum up the weights for each bone
						// bones with weight == 1 can't be removed

						QMap<int, float> sum;
						QList<int> nono;

						for ( int t = 0; t < 3; t++ ) {
							if ( weights[tri[t]].count() == 1 )
								nono.append( weights[tri[t]].first().first );

							for ( const auto bw : weights[tri[t]] ) {
								sum[ bw.first ] += bw.second;
							}
						}

						// select the bone to remove

						float minWeight = 5.0;
						int minBone = -1;

						for ( const auto b : sum.keys() ) {
							if ( !nono.contains( b ) && sum[b] < minWeight ) {
								minWeight = sum[b];
								minBone = b;
							}
						}

						if ( minBone < 0 )  // this shouldn't never happen
							throw QString( "internal error 0x01" );

						// do a vertex match detect

						if ( doMatch ) {
							QVector<Vector3> verts = nif->getArray<Vector3>( iData, "Vertices" );

							for ( int a = 0; a < verts.count(); a++ ) {
								match.insertMulti( a, a );

								for ( int b = a + 1; b < verts.count(); b++ ) {
									if ( verts[a] == verts[b] && weights[a] == weights[b] ) {
										match.insertMulti( a, b );
										match.insertMulti( b, a );
									}
								}
							}
						}

						// now remove that bone from all vertices of this triangle and from all matching vertices too

						for ( int t = 0; t < 3; t++ ) {
							bool rem = false;
							for ( const auto v : match.values( tri[t] ) )
							{
								QList<boneweight> & bws = weights[ v ];
								QMutableListIterator<boneweight> it( bws );

								while ( it.hasNext() ) {
									boneweight & bw = it.next();

									if ( bw.first == minBone ) {
										it.remove();
										rem = true;
									}
								}

								float totalWeight = 0;
								for ( const auto bw : bws ) {
									totalWeight += bw.second;
								}

								if ( totalWeight == 0 )
									throw QString( "internal error 0x02" );

								for ( int b = 0; b < bws.count(); b++ ) {
									// normalize
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
				qCWarning( nsSpell ) << Spell::tr( "Removed %1 bone influences" ).arg( cnt );

			// split the triangles into partitions

			bool merged = true;

			QList<Partition> parts;

			if ( !trimap.isEmpty() ) {
				QMutableListIterator<Triangle> it( triangles );

				while ( it.hasNext() ) {
					Triangle tri = it.next();
					qRotate( tri );
					QMap<Triangle, quint32>::iterator partItr = trimap.find( tri );
					int partIdx = ( partItr != trimap.end() ) ? partItr.value() : defaultPart;

					if ( partIdx >= 0 ) {
						//Triangle & tri = *it;

						// Ensure enough partitions
						while ( partIdx >= int( parts.size() ) )
							parts.push_back( Partition() );

						Partition & part = parts[partIdx];

						QList<int> tribones;

						for ( int c = 0; c < 3; c++ ) {
							for ( const auto bw : weights[tri[c]] ) {
								if ( !tribones.contains( bw.first ) )
									tribones.append( bw.first );
							}
						}

						part.bones = mergeBones( part.bones, tribones );
						part.triangles.append( tri );
						it.remove();
					}
				}

				merged = false; // when explicit mapping enabled, no merging is allowed
			}

			while ( !triangles.isEmpty() ) {
				Partition part;

				QHash<int, bool> usedVerts;

				bool addtriangles;

				do {
					QMutableListIterator<Triangle> it( triangles );

					while ( it.hasNext() ) {
						Triangle & tri = it.next();

						QList<int> tribones;

						for ( int c = 0; c < 3; c++ ) {
							for ( const auto bw : weights[tri[c]] ) {
								if ( !tribones.contains( bw.first ) )
									tribones.append( bw.first );
							}
						}

						if ( part.bones.isEmpty() || containsBones( part.bones, tribones ) ) {
							part.bones = mergeBones( part.bones, tribones );
							part.triangles.append( tri );
							usedVerts[ tri[0] ] = true;
							usedVerts[ tri[1] ] = true;
							usedVerts[ tri[2] ] = true;
							it.remove();
						}
					}

					addtriangles = false;

					if ( part.bones.count() < maxBonesPerPartition ) {
						// if we have room left in the partition then add an adjacent triangle
						it.toFront();

						while ( it.hasNext() ) {
							Triangle & tri = it.next();

							if ( usedVerts.contains( tri[0] ) || usedVerts.contains( tri[1] ) || usedVerts.contains( tri[2] ) ) {
								QList<int> tribones;

								for ( int c = 0; c < 3; c++ ) {
									for ( const auto bw : weights[tri[c]] ) {
										if ( !tribones.contains( bw.first ) )
											tribones.append( bw.first );
									}
								}

								tribones = mergeBones( part.bones, tribones );

								if ( tribones.count() <= maxBonesPerPartition ) {
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
				} while ( addtriangles );

				parts.append( part );
			}

			//qDebug() << parts.count() << "small partitions";

			// merge partitions

			while ( merged ) {
				merged = false;

				for ( int p1 = 0; p1 < parts.count() && !merged; p1++ ) {
					if ( parts[p1].bones.count() < maxBonesPerPartition ) {
						for ( int p2 = p1 + 1; p2 < parts.count() && !merged; p2++ ) {
							QList<int> mergedBones = mergeBones( parts[p1].bones, parts[p2].bones );

							if ( mergedBones.count() <= maxBonesPerPartition ) {
								parts[p1].bones = mergedBones;
								parts[p1].triangles << parts[p2].triangles;
								parts.removeAt( p2 );
								merged = true;
							}
						}
					}
				}
			}

			//qDebug() << parts.count() << "partitions";

			// create the NiSkinPartition if it doesn't exist yet

			if ( !iSkinPart.isValid() ) {
				iSkinPart = nif->insertNiBlock( "NiSkinPartition", nif->getBlockNumber( iSkinData ) + 1 );
				nif->setLink( iSkinInst, "Skin Partition", nif->getBlockNumber( iSkinPart ) );
				nif->setLink( iSkinData, "Skin Partition", nif->getBlockNumber( iSkinPart ) );
			}

			// start writing NiSkinPartition

			nif->set<int>( iSkinPart, "Num Skin Partition Blocks", parts.count() );
			nif->updateArray( iSkinPart, "Skin Partition Blocks" );

			QModelIndex iBSSkinInstPartData;

			if ( nif->inherits( iSkinInst, "BSDismemberSkinInstance" ) ) {
				quint32 nparts = nif->get<uint>( iSkinInst, "Num Partitions" );
				iBSSkinInstPartData = nif->getIndex( iSkinInst, "Partitions" );

				// why is QList.count() signed? cast to squash warning
				if ( nparts != (quint32)parts.count() ) {
					qCWarning( nsSpell ) << "BSDismemberSkinInstance partition count does not match Skin Partition count.  Adjusting to fit.";
					nif->set<uint>( iSkinInst, "Num Partitions", parts.count() );
					nif->updateArray( iSkinInst, "Partitions" );
				}
			}

			QList<int> prevPartBones;

			for ( int p = 0; p < parts.count(); p++ ) {
				QModelIndex iPart = nif->getIndex( iSkinPart, "Skin Partition Blocks" ).child( p, 0 );

				QList<int> bones = parts[p].bones;
				std::sort( bones.begin(), bones.end() /*, std::less<int>()*/ );

				// set partition flags for bs skin instance if present
				if ( iBSSkinInstPartData.isValid() ) {
					if ( bones != prevPartBones ) {
						prevPartBones = bones;
						nif->set<uint>( iBSSkinInstPartData.child( p, 0 ), "Part Flag", 257 );
					}
				}

				QVector<Triangle> triangles = parts[p].triangles;

				// Create the vertex map

				int idx = 0;
				QVector<int> vidx( numVerts, -1 );
				for ( const Triangle& tri : triangles ) {
					for ( int t = 0; t < 3; t++ ) {
						int v = tri[t];

						if ( vidx[v] < 0 )
							vidx[v] = idx++;
					}
				}
				QVector<int> vertices( idx, -1 );

				for ( int i = 0; i < numVerts; ++i ) {
					int v = vidx[i];

					if ( v >= 0 ) {
						vertices[v] = i;
					}
				}

				// map the vertices

				for ( int tri = 0; tri < triangles.count(); tri++ ) {
					for ( int t = 0; t < 3; t++ ) {
						triangles[tri][t] = vertices.indexOf( triangles[tri][t] );
					}
				}

				// stripify the triangles
				QVector<QVector<quint16> > strips;
				int numTriangles = 0;

				if ( make_strips == true ) {
					strips = stripify( triangles );

					for ( const QVector<quint16>& strip : strips ) {
						numTriangles += strip.count() - 2;
					}
				} else {
					numTriangles = triangles.count();
				}

				// fill in counts
				if ( pad ) {
					while ( bones.size() < maxBonesPerPartition ) {
						bones.append( 0 );
					}
				}

				// resort the bone weights in bone order
				QMutableVectorIterator<QList<boneweight> > it( weights );

				while ( it.hasNext() ) {
					QList<boneweight> & bw = it.next();
					std::sort( bw.begin(), bw.end(), boneweight_equivalence() );
				}

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

				for ( int v = 0; v < nif->rowCount( iVWeights ); v++ ) {
					QModelIndex iVertex = iVWeights.child( v, 0 );
					nif->updateArray( iVertex );
					QList<boneweight> list = weights.value( vertices[v] );

					for ( int b = 0; b < maxBones; b++ )
						nif->set<float>( iVertex.child( b, 0 ), list.count() > b ? list[ b ].second : 0.0 );
				}

				nif->set<int>( iPart, "Has Faces", 1 );

				if ( make_strips == true ) {
					//Clear out any existing triangle data that might be left over from an existing Skin Partition
					QModelIndex iTriangles = nif->getIndex( iPart, "Triangles" );
					nif->updateArray( iTriangles );

					// write the strips
					QModelIndex iStripLengths = nif->getIndex( iPart, "Strip Lengths" );
					nif->updateArray( iStripLengths );

					for ( int s = 0; s < nif->rowCount( iStripLengths ); s++ )
						nif->set<int>( iStripLengths.child( s, 0 ), strips.value( s ).count() );

					QModelIndex iStrips = nif->getIndex( iPart, "Strips" );
					nif->updateArray( iStrips );

					for ( int s = 0; s < nif->rowCount( iStrips ); s++ ) {
						nif->updateArray( iStrips.child( s, 0 ) );
						nif->setArray<quint16>( iStrips.child( s, 0 ), strips.value( s ) );
					}
				} else {
					//Clear out any existing strip data that might be left over from an existing Skin Partition
					QModelIndex iStripLengths = nif->getIndex( iPart, "Strip Lengths" );
					nif->updateArray( iStripLengths );
					QModelIndex iStrips = nif->getIndex( iPart, "Strips" );
					nif->updateArray( iStrips );

					QModelIndex iTriangles = nif->getIndex( iPart, "Triangles" );
					nif->updateArray( iTriangles );
					nif->setArray<Triangle>( iTriangles, triangles );
				}

				// fill in vertex bones

				nif->set<int>( iPart, "Has Bone Indices", 1 );
				QModelIndex iVBones = nif->getIndex( iPart, "Bone Indices" );
				nif->updateArray( iVBones );

				for ( int v = 0; v < nif->rowCount( iVBones ); v++ ) {
					QModelIndex iVertex = iVBones.child( v, 0 );
					nif->updateArray( iVertex );
					QList<boneweight> list = weights.value( vertices[v] );

					for ( int b = 0; b < maxBones; b++ )
						nif->set<int>( iVertex.child( b, 0 ), list.count() > b ? bones.indexOf( list[ b ].first ) : 0 );
				}
			}

			// done

			return iShape;
		}
		catch ( QString & err )
		{
			if ( !err.isEmpty() )
				QMessageBox::warning( 0, "NifSkope", err );

			return iShape;
		}
	}

	static QList<int> mergeBones( QList<int> a, QList<int> b )
	{
		for ( const auto c : b ) {
			if ( !a.contains( c ) ) {
				a.append( c );
			}
		}
		return a;
	}

	static bool containsBones( QList<int> a, QList<int> b )
	{
		for ( const auto c : b ) {
			if ( !a.contains( c ) )
				return false;
		}
		return true;
	}
};

REGISTER_SPELL( spSkinPartition )

//! Make all skin partitions
class spAllSkinPartitions final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Make All Skin Partitions" ); }
	QString page() const override final { return Spell::tr( "Batch" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		return nif && !index.isValid();
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		Q_UNUSED( index );
		QList<QPersistentModelIndex> indices;

		spSkinPartition Partitioner;

		for ( int n = 0; n < nif->getBlockCount(); n++ ) {
			QModelIndex idx = nif->getBlock( n );

			if ( Partitioner.isApplicable( nif, idx ) )
				indices.append( idx );
		}

		int mbpp = 0, mbpv = 0;
		bool make_strips = false;

		for ( const QModelIndex& idx : indices ) {
			Partitioner.cast( nif, idx, mbpp, mbpv, make_strips );
		}

		qCWarning( nsSpell ) << Spell::tr( "did %1 partitions" ).arg( indices.count() );

		return QModelIndex();
	}
};

REGISTER_SPELL( spAllSkinPartitions )


SkinPartitionDialog::SkinPartitionDialog( int ) : QDialog()
{
	spnVert = new QSpinBox( this );
	spnVert->setMinimum( 1 );
	spnVert->setMaximum( 8 );
	spnVert->setValue( 4 );
	connect( spnVert, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &SkinPartitionDialog::changed );

	spnPart = new QSpinBox( this );
	spnPart->setMinimum( 4 );
	spnPart->setMaximum( 99 );
	spnPart->setValue( 18 );

	ckTStrip = new QCheckBox( "&Stripify Triangles" );
	ckTStrip->setChecked( false );
	ckTStrip->setToolTip( "Determines whether the triangles in each partition will be arranged into strips or as a list of individual triangles.  Different gaems work best with one or the other." );
	connect( ckTStrip, &QCheckBox::clicked, this, &SkinPartitionDialog::changed );

	ckPad = new QCheckBox( "&Pad Small Partitions" );
	ckPad->setChecked( false );
	ckPad->setToolTip( "Determines whether partitions that will have fewer than the selected maximum number of bones will have extra bones added to bring them up to that number." );
	connect( ckPad, &QCheckBox::clicked, this, &SkinPartitionDialog::changed );


	QLabel * labVert = new QLabel( this );
	labVert->setText( Spell::tr(
			"<b>Number of Bones per Vertex</b><br>"
			"Hint: Most games use 4 bones per vertex<br>"
			"Note: If the mesh contains vertices which are<br>"
			"influenced by more than x bones the number of<br>"
			"influences will be reduced for these vertices<br>"
		) );

	QLabel * labPart = new QLabel( this );
	labPart->setText( Spell::tr(
			"<b>Number of Bones per Partition</b><br>"
			"Hint: Oblivion uses 18 bones pp<br>"
			"CivIV (non shader meshes) 4 bones pp<br>"
			"CivIV (shader enabled meshes) 18 bones pp<br>"
			"Note: To fit the triangles into the partitions<br>"
			"some bone influences may be removed again."
		) );

	QLabel * labTStrip = new QLabel( this );
	labTStrip->setText( Spell::tr(
			"<b>Whether or not to stripify the triangles in each partition.</b><br>"
			"Hint:  Morrowind and Freedom force do not support strips.<br>"
			"Strips generally perform faster, if the game supports them."
		) );

	QLabel * labPad = new QLabel( this );
	labPad->setText( Spell::tr(
			"<b>Whether or not to pad partitions that will have fewer bones than specified above.</b><br>"
			"Hint:  Freedom Force seems to require this, but it doesn't seem to affect other games."
		) );

	QPushButton * btOk = new QPushButton( this );
	btOk->setText( Spell::tr( "Ok" ) );
	connect( btOk, &QPushButton::clicked, this, &SkinPartitionDialog::accept );

	QPushButton * btCancel = new QPushButton( this );
	btCancel->setText( Spell::tr( "Cancel" ) );
	connect( btCancel, &QPushButton::clicked, this, &SkinPartitionDialog::reject );

	QGridLayout * grid = new QGridLayout( this );
	grid->addWidget( labVert, 0, 0 );    grid->addWidget( spnVert, 0, 1 );
	grid->addWidget( labPart, 1, 0 );    grid->addWidget( spnPart, 1, 1 );
	grid->addWidget( labTStrip, 2, 0 );  grid->addWidget( ckTStrip, 2, 1 );
	grid->addWidget( labPad, 3, 0 );     grid->addWidget( ckPad, 3, 1 );
	grid->addWidget( btOk, 4, 0 );       grid->addWidget( btCancel, 4, 1 );
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

bool SkinPartitionDialog::makeStrips()
{
	return ckTStrip->isChecked();
}

bool SkinPartitionDialog::padPartitions()
{
	return ckPad->isChecked();
}

//! Fix bone bounds
class spFixBoneBounds final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Fix Bone Bounds" ); }
	QString page() const override final { return Spell::tr( "Skeleton" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		return nif->isNiBlock( index, "NiSkinData" );
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & iSkinData ) override final
	{
		QModelIndex iSkinInstance = nif->getBlock( nif->getParent( nif->getBlockNumber( iSkinData ) ), "NiSkinInstance" );
		QModelIndex iMesh = nif->getBlock( nif->getParent( nif->getBlockNumber( iSkinInstance ) ) );
		QModelIndex iMeshData = nif->getBlock( nif->getLink( iMesh, "Data" ) );
		int skelRoot = nif->getLink( iSkinInstance, "Skeleton Root" );

		if ( !nif->inherits( iMeshData, "NiTriBasedGeomData" ) || skelRoot < 0 || skelRoot != nif->getParent( nif->getBlockNumber( iMesh ) ) )
			return iSkinData;

		Transform meshTrans( nif, iMesh );

		QVector<Transform> boneTrans;
		QModelIndex iBoneMap = nif->getIndex( iSkinInstance, "Bones" );

		for ( int n = 0; n < nif->rowCount( iBoneMap ); n++ ) {
			QModelIndex iBone = nif->getBlock( nif->getLink( iBoneMap.child( n, 0 ) ), "NiNode" );

			if ( skelRoot != nif->getParent( nif->getBlockNumber( iBone ) ) )
				return iSkinData;

			boneTrans.append( Transform( nif, iBone ) );
		}

		QVector<Vector3> verts = nif->getArray<Vector3>( iMeshData, "Vertices" );

		QModelIndex iBoneDataList = nif->getIndex( iSkinData, "Bone List" );

		for ( int b = 0; b < nif->rowCount( iBoneDataList ); b++ ) {
			Vector3 mn;
			Vector3 mx;

			Vector3 center;
			float radius = 0;

			QModelIndex iWeightList = nif->getIndex( iBoneDataList.child( b, 0 ), "Vertex Weights" );

			for ( int w = 0; w < nif->rowCount( iWeightList ); w++ ) {
				int v = nif->get<int>( iWeightList.child( w, 0 ), "Index" );

				if ( w == 0 ) {
					mn = verts.value( v );
					mx = verts.value( v );
				} else {
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

			auto sphIdx = nif->getIndex( iBoneDataList.child( b, 0 ) , "Bounding Sphere" );

			nif->set<Vector3>( sphIdx, "Bounding Sphere Offset", center );
			nif->set<float>( sphIdx, "Bounding Sphere Radius", radius );
		}

		return iSkinData;
	}
};

REGISTER_SPELL( spFixBoneBounds )

//! Mirror an armature branch.
/*!
 * Renames nodes, flips translations and rotations, flips meshes, flips skin data.
 */
class spMirrorSkeleton final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Mirror armature" ); }
	QString page() const override final { return Spell::tr( "Skeleton" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		return ( nif->getVersion() == "4.0.0.2" && nif->itemType( index ) == "NiBlock" )
		       && ( ( nif->get<QString>( index, "Name" ).startsWith( "Bip01 L" ) ) || ( nif->get<QString>( index, "Name" ).startsWith( "Bip01 R" ) ) );
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		if ( nif->getLink( index, "Controller" ) != -1 ) {
			int keyframeResponse = QMessageBox::question( 0, Spell::tr( "Mirror Armature" ), Spell::tr( "Do you wish to flip or delete animation?" ), Spell::tr( "Flip" ), Spell::tr( "Delete" ), Spell::tr( "Cancel" ) );

			if ( keyframeResponse == 2 )
				return index;

			if ( keyframeResponse == 1 ) {
				// delete blocks
				int n = 0;

				while ( n < nif->getBlockCount() ) {
					QModelIndex iBlock = nif->getBlock( n );

					if ( nif->itemName( iBlock ).indexOf( "NiKeyframe" ) >= 0 )
						nif->removeNiBlock( n );
					else
						n++;
				}
			}
		}

		// TODO: Duplicate the branch
		// Perhaps have the user do this?

		// Traverse nodes starting with Bip01 L/R and "flip"
		doBones( nif, index );
		return index;
	}

	void doBones( NifModel * nif, const QModelIndex & index )
	{
		// Need correct naming scheme!
		QString name = nif->get<QString>( index, "Name" );

		if ( name.startsWith( "Bip01 L" ) || name.startsWith( "Bip01 R" ) ) {
			Transform tlocal( nif, index );

			// rename as appropriate
			if ( name.startsWith( "Bip01 L" ) )
				name.replace( QString( " L" ), QString( " R" ) );
			else
				name.replace( QString( " R" ), QString( " L" ) );

			nif->set<QString>( index, "Name", name );

			// translation is a Vector3
			// want [x,y,z] -> [x,y,-z]
			tlocal.translation = { tlocal.translation[0], tlocal.translation[1], -tlocal.translation[2] };

			// rotation is a Matrix, want to negate Y and P components - will get to R component in SkinInstance
			// convert to Euler, then [Y,P,R] -> [-Y,-P,R]
			float x, y, z;
			tlocal.rotation.toEuler( x, y, z );
			tlocal.rotation.fromEuler( -x, -y, z );

			// Apply
			tlocal.writeBack( nif, index );
		}

		// traverse
		for ( const auto link : nif->getChildLinks( nif->getBlockNumber( index ) ) ) {
			QModelIndex iChild = nif->getBlock( link );
			QString childName  = nif->get<QString>( iChild, "Name" );

			// Might as well rename children now if we can - this is less case-critical than Bip01 L/R
			if ( childName.contains( "Left ", Qt::CaseInsensitive ) ) {
				childName.replace( QString( "Left " ), QString( "Right " ), Qt::CaseInsensitive );
			} else if ( childName.contains( "Right ", Qt::CaseInsensitive ) ) {
				childName.replace( QString( "Right " ), QString( "Left " ), Qt::CaseInsensitive );
			}

			nif->set<QString>( iChild, "Name", childName );

			//qDebug() << "Checking child: " << iChild;
			if ( iChild.isValid() ) {
				if ( nif->itemName( iChild ) == "NiNode" ) {
					// repeat
					doBones( nif, iChild );
				} else if ( nif->inherits( iChild, "NiTriBasedGeom" ) ) {
					// Scale NiTriShape vertices, flip normals
					// Change SkinInstance bones
					doShapes( nif, iChild );
				} else if ( nif->inherits( iChild, "NiKeyframeController" ) ) {
					// Flip keyframe data, fun
					doKeyframes( nif, iChild );
				}
			}
		}
	}

	void doShapes( NifModel * nif, const QModelIndex & index )
	{
		//qDebug() << "Entering doShapes";
		QModelIndex iData = nif->getBlock( nif->getLink( index, "Data" ) );
		QModelIndex iSkinInstance = nif->getBlock( nif->getLink( index, "Skin Instance" ), "NiSkinInstance" );

		if ( iData.isValid() && iSkinInstance.isValid() ) {
			// from spScaleVertices
			QVector<Vector3> vertices = nif->getArray<Vector3>( iData, "Vertices" );
			QMutableVectorIterator<Vector3> it( vertices );

			while ( it.hasNext() ) {
				Vector3 & v = it.next();

				for ( int a = 0; a < 3; a++ )
					v[a] = -v[a];
			}

			nif->setArray<Vector3>( iData, "Vertices", vertices );

			// fix centre Z - don't recalculate
			Vector3 shapeCentre = nif->get<Vector3>( iData, "Center" );
			shapeCentre = { shapeCentre[0], shapeCentre[1], -shapeCentre[2] };
			nif->set<Vector3>( iData, "Center", shapeCentre );

			// from spFlipFace
			QVector<Triangle> tris = nif->getArray<Triangle>( iData, "Triangles" );

			for ( int t = 0; t < tris.count(); t++ ) {
				tris[t].flip();
			}

			nif->setArray<Triangle>( iData, "Triangles", tris );

			// from spFlipNormals
			QVector<Vector3> norms = nif->getArray<Vector3>( iData, "Normals" );

			for ( int n = 0; n < norms.count(); n++ ) {
				norms[n] = -norms[n];
			}

			nif->setArray<Vector3>( iData, "Normals", norms );

			// from spFixSkeleton - get the bones from the skin data
			// weirdness with rounding, sometimes...? probably "good enough" for 99% of cases
			QModelIndex iSkinData = nif->getBlock( nif->getLink( iSkinInstance, "Data" ), "NiSkinData" );

			if ( !iSkinData.isValid() )
				return;

			QModelIndex iBones = nif->getIndex( iSkinData, "Bone List" );

			if ( !iBones.isValid() )
				return;

			for ( int b = 0; b < nif->rowCount( iBones ); b++ ) {
				QModelIndex iBone = iBones.child( b, 0 );

				Transform tlocal( nif, iBone );

				// translation is a Vector3
				// want [x,y,z] -> [x,y,-z]
				tlocal.translation = Vector3( tlocal.translation[0], tlocal.translation[1], -tlocal.translation[2] );

				// rotation is a Matrix, want to negate Y and P components, R=180+R
				// convert to Euler, then [Y,P,R] -> [-Y,-P,R]
				float x, y, z;
				tlocal.rotation.toEuler( x, y, z );
				tlocal.rotation.fromEuler( -x, -y, PI + z );

				// Fix offset Z
				Vector3 offset = nif->get<Vector3>( iBone, "Bounding Sphere Offset" );
				offset = Vector3( offset[0], offset[1], -offset[2] );
				nif->set<Vector3>( iBone, "Bounding Sphere Offset", offset );

				// Apply
				tlocal.writeBack( nif, iBone );
			}
		}
	}

	void doKeyframes( NifModel * nif, QModelIndex & index )
	{
		// do stuff
		QModelIndex keyframeData = nif->getBlock( nif->getLink( index, "Data" ), "NiKeyframeData" );

		if ( !keyframeData.isValid() )
			return;

		QModelIndex iQuats = nif->getIndex( keyframeData, "Quaternion Keys" );

		if ( iQuats.isValid() ) {
			for ( int q = 0; q < nif->rowCount( iQuats ); q++ ) {
				QModelIndex iQuat = iQuats.child( q, 0 );

				Quat value = nif->get<Quat>( iQuat, "Value" );
				Matrix tlocal;
				tlocal.fromQuat( value );

				float x, y, z;
				tlocal.toEuler( x, y, z );
				tlocal.fromEuler( -x, -y, z );

				value = tlocal.toQuat();

				nif->set<Quat>( iQuat, "Value", value );
			}
		}

		QModelIndex iTransKeys = nif->getIndex( keyframeData, "Translations" );

		if ( iTransKeys.isValid() ) {
			iTransKeys = nif->getIndex( iTransKeys, "Keys" );

			if ( iTransKeys.isValid() ) {
				for ( int k = 0; k < nif->rowCount( iTransKeys ); k++ ) {
					QModelIndex iKey = iTransKeys.child( k, 0 );

					Vector3 value = nif->get<Vector3>( iKey, "Value" );
					value = Vector3( value[0], value[1], -value[2] );
					nif->set<Vector3>( iKey, "Value", value );
				}
			}
		}
	}
};

REGISTER_SPELL( spMirrorSkeleton )


bool spToFO42::isApplicable( const NifModel * nif, const QModelIndex & index ) {
    return nif && !index.isValid();
}

QModelIndex spToFO42::cast(NifModel * nif, const QModelIndex & index) {
    // Init new nif
    NifModel * newNif = new NifModel();
    newNif->loadFromFile("D:\\Games\\Fallout New Vegas\\FNVFo4 Converted\\test\\template.nif");

//        nif->removeNiBlock(0);
//        nif->setValue()

//        nif->loadFromFile("D:\\Games\\Fallout New Vegas\\FNVFo4 Converted\\Data\\Meshes\\new_vegas\\ammo\\9mmammo.nif");
     for ( int b = 0; b < nif->getBlockCount(); b++ ) {
//             QModelIndex iNode = nif->getBlock( b, "BSFadeNode" );
         QModelIndex iNode = nif->getBlock( b );

         if (nif->getBlockName(iNode) == "BSFadeNode") {

//             if ( iNode.isValid() ) {
             newNif->insertNiBlock( "NiNode" );
         } else if (nif->getBlockName(iNode) == "NiTriStripsData") {
             QModelIndex triShape = newNif->insertNiBlock( "BSTriShape" );

             // TODO: Vertex Colors
             // TODO: Consistency flags

             QModelIndex boundingSphere = newNif->getIndex(triShape, "Bounding Sphere");
             newNif->set<Vector3>(boundingSphere, "Center", nif->get<Vector3>( iNode, "Center"));
             newNif->set<float>(boundingSphere, "Radius", nif->get<float>( iNode, "Radius"));

             auto vf = nif->get<int>(iNode, "BS Vector Flags");
             auto newVf = newNif->get<BSVertexDesc>( triShape, "Vertex Desc" );

             if (vf & 1) {
                newVf.SetFlag(VertexFlags::VF_UV);
             }

             if (vf & 4096) {
                newVf.SetFlag(VertexFlags::VF_TANGENT);
             }

             if (nif->get<bool>( iNode, "Has Vertices")) {
                 newVf.SetFlag(VertexFlags::VF_VERTEX);
             }

             if (nif->get<bool>( iNode, "Has Normals")) {
                 newVf.SetFlag(VertexFlags::VF_NORMAL);
             }

             if (nif->get<bool>( iNode, "Has Vertex Colors")) {
                 newVf.SetFlag(VertexFlags::VF_COLORS);
             }

             spUnstichStrips().cast(nif, iNode);


             newVf.ResetAttributeOffsets(newNif->getUserVersion2());

             newNif->set<BSVertexDesc>(triShape, "Vertex Desc", newVf);

             QModelIndex points = nif->getIndex(iNode, "Points");
             uint numTriangles = 0;
             QVector<Triangle> arr = QVector<Triangle>();
             for (int i = 0; i < nif->rowCount(points); i++) {
                QVector<ushort> point = nif->getArray<ushort>(points.child(i, 0));
                for (int j = 0; j < point.count() - 2; j++) {
                    if (point[j + 1] == point[j + 2] || point[j] == point[j + 1] || point[j] == point[j + 2]) {
                        continue;
                    }

                    if (j & 1) {
                        arr.append(Triangle(point[j + 1], point[j], point[j + 2]));
                    } else {
                        arr.append(Triangle(point[j], point[j + 1], point[j + 2]));
                    }
                    numTriangles++;
                }
             }

             uint numVertices = nif->get<uint>( iNode, "Num Vertices");
//             uint numTriangles = nif->get<uint>( iNode, "Num Triangles");

//             numTriangles = 0;
             newNif->set<uint>(triShape, "Num Vertices", numVertices);
             newNif->set<uint>(triShape, "Num Triangles", numTriangles);
             newNif->set<uint>(triShape, "Data Size", newVf.GetVertexSize() * numVertices + numTriangles * 6);
             newNif->updateArray( newNif->getIndex( triShape, "Vertex Data" ) );
             newNif->updateArray( newNif->getIndex( triShape, "Triangles" ) );

             QModelIndex data = newNif->getIndex(triShape, "Vertex Data");
             QModelIndex triangles = newNif->getIndex(triShape, "Triangles");

             newNif->setArray<Triangle>(triangles, arr);
//             printf("%d Rows\n", newNif->rowCount(data));
//             for (int i = 0; i < 39; i++) {
//                 printf(newNif->getBlockName(triShape.child(i, 0)).toUtf8().constData());
//                printf("%d\n", newNif->rowCount(triShape.child(i, 0)));
//             }

//             printf(newNif->getBlockName(triangles.child(0, 0)).toUtf8().constData());
             printf("%d\n", newNif->rowCount(triangles));
             printf("%d\n", newNif->rowCount(data));
             printf("\n");


             // 0  Vertex
             // 1  Bitangent X
             // 2  Unknown Short
             // 3  Vertex
             // 4  Bitangent X // Occurs twice??
             // 5  Unknown Int
             // 6  UV
             // 7  Normal
             // 8  Bitangent Y
             // 9  Tangent
             // 10 Bitangent Z
             // 11 Vertex Colors
             // 12 Bone Weights
             // 13 Bone Indices
             // 14 Eye Data

             QVector<Vector3> verts = nif->getArray<Vector3>( iNode, "Vertices" );
             QVector<Vector3> norms = nif->getArray<Vector3>( iNode, "Normals" );
             QVector<Vector3> tangents = nif->getArray<Vector3>( iNode, "Tangents" );
             QVector<Vector3> bitangents = nif->getArray<Vector3>( iNode, "Bitangents" );

             // TODO: Fix for multiple arrays
             QVector<Vector2> uvSets = nif->getArray<Vector2>( nif->getIndex( iNode, "UV Sets"), "UV Sets");
//             QVector<ushort> points = nif->getArray<ushort>( nif->getIndex( iNode, "Points"), "Points" );
//             QVector<QModelIndex> points = nif->getArray<QModelIndex>(triShape, "Points");

             printf("UV Sets: %d\n", uvSets.count());

             printf("Tangents: %d\n", tangents.count());
//             printf("Points: %d\n", points.count());
             printf("Points: %d\n", nif->rowCount(points));
//             printf("Tangent Y 0: %f\n", byte(bitangents[0][1]));
//             printf("Tangent Z 0: %f\n", bitangents[0][2]);

             QVector<HalfVector3> newVerts = QVector<HalfVector3>(verts.count());

             // Create vertex data
             for ( int i = 0; i < verts.count(); i++ ) {
                 newNif->set<HalfVector3>( data.child( i, 0 ).child(0,0), HalfVector3(verts[i]));
                 verts.count() == norms.count()    && newNif->set<ByteVector3>( data.child( i, 0 ).child(7,0), ByteVector3(norms[i]));
                 verts.count() == tangents.count() && newNif->set<ByteVector3>( data.child( i, 0 ).child(9,0), ByteVector3(tangents[i]));
                 verts.count() == uvSets.count()   && newNif->set<HalfVector2>( data.child( i, 0 ).child(6,0), HalfVector2(uvSets[i]));
                 if (verts.count() == bitangents.count()) {
                     newNif->set<float>( data.child( i, 0 ).child(1,0),  bitangents[i][0]);

                     // TODO: Set Bitangent Y and Z
//                     newNif->set<byte>( data.child( i, 0 ).child(8,0), byte(bitangents[i][1]));
//                     newNif->set<byte>( data.child( i, 0 ).child(10,0), byte(bitangents[i][2]));
                 }

//                 if (verts.count() + 2 == points.count()) {
//                 }
             }

//             spStitchStrips::cast(nif);

//             // Create triangles
//             if (newNif->rowCount(triangles) + 2 == points.count()) {
//                 for (int i = 0; i < newNif->rowCount(triangles); i++) {
//                     if (points[i + 1] == points[i + 2] || points[i] == points[i + 1] || points[i] == points[i + 2]) {
//                         continue;
//                     }

//                     newNif->set<Triangle>( triangles.child( i, 0 ), Triangle(points[i], points[i + 1], points[i + 2]));
//                 }
//             }


//                 newNif->setArray<HalfVector3>(triShape, newVerts);

//                 newNif->setArray<HalfVector3>(triShape,)

//                 copyValue<int>( newNif, nif, triShape, iNode, "Num Triangles" );
//                 newNif.set<int>( triShape, "Num Vertices", nif->get<int>( iNode, "Num Vertices"));
//                 nif->set<int>( triShape, "Num Vertices", nif->get<int>( iNode, "Num Vertices"));
         }

     }


//        newNif.insertNiBlock( "NiNode" );
//        newNif.insertNiBlock( "BSTriShape" );

    newNif->saveToFile("D:\\Games\\Fallout New Vegas\\FNVFo4 Converted\\test\\test.nif");

    newNif->clear();

    return QModelIndex();
}

REGISTER_SPELL( spToFO42 )

