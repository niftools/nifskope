#include "tangentspace.h"

#include "lib/nvtristripwrapper.h"


bool spTangentSpace::isApplicable( const NifModel * nif, const QModelIndex & index )
{
	QModelIndex iData = nif->getBlock( nif->getLink( index, "Data" ) );

	if ( nif->isNiBlock( index, "BSTriShape" ) || nif->isNiBlock( index, "BSSubIndexTriShape" ) 
		|| nif->isNiBlock( index, "BSMeshLODTriShape" ) ) {
		// TODO: Check vertex flags to verify mesh has normals and space for tangents/bitangents
		return true;
	}

	if ( !( nif->isNiBlock( index, "NiTriShape" ) && nif->isNiBlock( iData, "NiTriShapeData" ) )
	     && !( nif->isNiBlock( index, "BSLODTriShape" ) && nif->isNiBlock( iData, "NiTriShapeData" ) )
	     && !( nif->isNiBlock( index, "NiTriStrips" ) && nif->isNiBlock( iData, "NiTriStripsData" ) ) )
	{
		return false;
	}

	// early exit of normals are missing
	if ( !nif->get<bool>( iData, "Has Normals" ) )
		return false;

	if ( nif->checkVersion( 0x14000004, 0x14000005 ) && (nif->getUserVersion() == 11) )
		return true;

	// If bethesda then we will configure the settings for the mesh.
	if ( nif->getUserVersion() == 11 )
		return true;

	// 10.1.0.0 and greater can have tangents and bitangents
	if ( nif->checkVersion( 0x0A010000, 0 ) )
		return true;

	return false;
}

QModelIndex spTangentSpace::cast( NifModel * nif, const QModelIndex & iBlock )
{
	QPersistentModelIndex iShape = iBlock;
	QModelIndex iData;
	QModelIndex iPartBlock;
	if ( nif->getUserVersion2() < 100 ) {
		iData = nif->getBlock( nif->getLink( iShape, "Data" ) );
	} else {
		auto vf = nif->get<BSVertexDesc>( iShape, "Vertex Desc" );
		if ( (vf & VertexFlags::VF_SKINNED) && nif->getUserVersion2() == 100 ) {
			// Skinned SSE
			auto skinID = nif->getLink( nif->getIndex( iShape, "Skin" ) );
			auto partID = nif->getLink( nif->getBlock( skinID, "NiSkinInstance" ), "Skin Partition" );
			iPartBlock = nif->getBlock( partID, "NiSkinPartition" );
			if ( iPartBlock.isValid() )
				iData = nif->getIndex( iPartBlock, "Vertex Data" );
		} else {
			iData = nif->getIndex( iShape, "Vertex Data" );
		}
	}

	QVector<Vector3> verts;
	QVector<Vector3> norms;
	QVector<Vector2> texco;

	if ( nif->getUserVersion2() < 100 ) {
		verts = nif->getArray<Vector3>( iData, "Vertices" );
		norms = nif->getArray<Vector3>( iData, "Normals" );
	} else {
		int numVerts;
		// "Num Vertices" does not exist in the partition
		if ( iPartBlock.isValid() )
			numVerts = nif->get<uint>( iPartBlock, "Data Size" ) / nif->get<uint>( iPartBlock, "Vertex Size" );
		else
			numVerts = nif->get<int>( iShape, "Num Vertices" );

		verts.reserve( numVerts );
		norms.reserve( numVerts );
		texco.reserve( numVerts );

		for ( int i = 0; i < numVerts; i++ ) {
			auto idx = nif->index( i, 0, iData );
			verts += nif->get<Vector3>( idx, "Vertex" );
			norms += nif->get<ByteVector3>( idx, "Normal" );;
			texco += nif->get<HalfVector2>( idx, "UV" );
		}
	}

	QVector<Color4> vxcol = nif->getArray<Color4>( iData, "Vertex Colors" );
	int numUVSets = nif->get<int>( iData, "Num UV Sets" );
	int tspaceFlags = nif->get<int>( iData, "TSpace Flag" );

	if ( nif->getUserVersion2() < 100 ) {
		QModelIndex iTexCo = nif->getIndex( iData, "UV Sets" );

		if ( !iTexCo.isValid() )
			iTexCo = nif->getIndex( iData, "UV Sets 2" );

		iTexCo = iTexCo.child( 0, 0 );
		texco = nif->getArray<Vector2>( iTexCo );
	}


	QVector<Triangle> triangles;
	QModelIndex iPoints = nif->getIndex( iData, "Points" );

	if ( iPoints.isValid() ) {
		QVector<QVector<quint16> > strips;

		for ( int r = 0; r < nif->rowCount( iPoints ); r++ )
			strips.append( nif->getArray<quint16>( iPoints.child( r, 0 ) ) );

		triangles = triangulate( strips );
	} else if ( nif->getUserVersion2() < 100 ) {
		triangles = nif->getArray<Triangle>( iData, "Triangles" );
	} else if ( nif->getUserVersion2() >= 100 ) {
		if ( iPartBlock.isValid() ) {
			// Get triangles from all partitions
			auto numParts = nif->get<int>( iPartBlock, "Num Skin Partition Blocks" );
			auto iParts = nif->getIndex( iPartBlock, "Partition" );
			for ( int i = 0; i < numParts; i++ )
				triangles << nif->getArray<Triangle>( iParts.child( i, 0 ), "Triangles" );
		} else {
			triangles = nif->getArray<Triangle>( iShape, "Triangles" );
		}
	}

	if ( verts.isEmpty() || norms.count() != verts.count() || texco.count() != verts.count() || triangles.isEmpty() ) {
		Message::append( tr( "Update Tangent Spaces failed on one or more blocks." ),
			tr( "Block %1: Insufficient information to calculate tangents and bitangents. V: %2, N: %3, Tex: %4, Tris: %5" )
			.arg( nif->getBlockNumber( iBlock ) )
			.arg( verts.count() )
			.arg( norms.count() )
			.arg( texco.count() )
			.arg( triangles.count() )
		);
		return iBlock;
	}

	QVector<Vector3> tan( verts.count() );
	QVector<Vector3> bin( verts.count() );

	QMultiHash<int, int> vmap;

	//int skptricnt = 0;

	for ( int t = 0; t < triangles.count(); t++ ) {
		// for each triangle caculate the texture flow direction
		//qDebug() << "triangle" << t;

		Triangle & tri = triangles[t];

		int i1 = tri[0];
		int i2 = tri[1];
		int i3 = tri[2];

		const Vector3 & v1 = verts[i1];
		const Vector3 & v2 = verts[i2];
		const Vector3 & v3 = verts[i3];

		const Vector2 & w1 = texco[i1];
		const Vector2 & w2 = texco[i2];
		const Vector2 & w3 = texco[i3];

		Vector3 v2v1 = v2 - v1;
		Vector3 v3v1 = v3 - v1;

		Vector2 w2w1 = w2 - w1;
		Vector2 w3w1 = w3 - w1;

		float r = w2w1[0] * w3w1[1] - w3w1[0] * w2w1[1];

		/*
		if ( fabs( r ) <= 10e-10 )
		{
		    //if ( skptricnt++ < 3 )
		    //	qDebug() << t;
		    continue;
		}

		r = 1.0 / r;
		*/
		// this seems to produces better results
		r = ( r >= 0 ? +1 : -1 );

		Vector3 sdir(
		    ( w3w1[1] * v2v1[0] - w2w1[1] * v3v1[0] ) * r,
		    ( w3w1[1] * v2v1[1] - w2w1[1] * v3v1[1] ) * r,
		    ( w3w1[1] * v2v1[2] - w2w1[1] * v3v1[2] ) * r
		);

		Vector3 tdir(
		    ( w2w1[0] * v3v1[0] - w3w1[0] * v2v1[0] ) * r,
		    ( w2w1[0] * v3v1[1] - w3w1[0] * v2v1[1] ) * r,
		    ( w2w1[0] * v3v1[2] - w3w1[0] * v2v1[2] ) * r
		);

		sdir.normalize();
		tdir.normalize();

		//qDebug() << sdir << tdir;

		for ( int j = 0; j < 3; j++ ) {
			int i = tri[j];

			tan[i] += tdir;
			bin[i] += sdir;
		}
	}

	//qDebug() << "skipped triangles" << skptricnt;

	//int cnt = 0;

	for ( int i = 0; i < verts.count(); i++ ) {
		// for each vertex calculate tangent and binormal
		const Vector3 & n = norms[i];

		Vector3 & t = tan[i];
		Vector3 & b = bin[i];

		//qDebug() << n << t << b;

		if ( t == Vector3() || b == Vector3() ) {
			t[0] = n[1]; t[1] = n[2]; t[2] = n[0];
			b = Vector3::crossproduct( n, t );
			//if ( cnt++ < 3 )
			//	qDebug() << i;
		} else {
			t.normalize();
			t = ( t - n * Vector3::dotproduct( n, t ) );
			t.normalize();

			//b = Vector3::crossproduct( n, t );

			b.normalize();
			b = ( b - n * Vector3::dotproduct( n, b ) );
			b = ( b - t * Vector3::dotproduct( t, b ) );
			b.normalize();
		}

		//qDebug() << n << t << b;
		//qDebug() << "";
	}

	//qDebug() << "unassigned vertices" << cnt;

	bool isOblivion = false;

	if ( nif->checkVersion( 0x14000004, 0x14000005 ) && (nif->getUserVersion() == 11) )
		isOblivion = true;

	if ( isOblivion ) {
		QModelIndex iTSpace;
		for ( const auto link : nif->getChildLinks( nif->getBlockNumber( iShape ) ) ) {
			iTSpace = nif->getBlock( link, "NiBinaryExtraData" );

			if ( iTSpace.isValid() && nif->get<QString>( iTSpace, "Name" ) == "Tangent space (binormal & tangent vectors)" )
				break;

			iTSpace = QModelIndex();
		}

		if ( !iTSpace.isValid() ) {
			iTSpace = nif->insertNiBlock( "NiBinaryExtraData", nif->getBlockNumber( iShape ) + 1 );
			nif->set<QString>( iTSpace, "Name", "Tangent space (binormal & tangent vectors)" );
			QModelIndex iNumExtras = nif->getIndex( iShape, "Num Extra Data List" );
			QModelIndex iExtras = nif->getIndex( iShape, "Extra Data List" );

			if ( iNumExtras.isValid() && iExtras.isValid() ) {
				int numlinks = nif->get<int>( iNumExtras );
				nif->set<int>( iNumExtras, numlinks + 1 );
				nif->updateArray( iExtras );
				nif->setLink( iExtras.child( numlinks, 0 ), nif->getBlockNumber( iTSpace ) );
			}
		}

		nif->set<QByteArray>( iTSpace, "Binary Data", QByteArray( (const char *)tan.data(), tan.count() * sizeof( Vector3 ) ) + QByteArray( (const char *)bin.data(), bin.count() * sizeof( Vector3 ) ) );
	} else if ( nif->getUserVersion2() < 100 ) {
		if ( tspaceFlags == 0 )
			tspaceFlags = 0x10;

		nif->set<int>( iShape, "TSpace Flag", tspaceFlags );
		nif->set<int>( iShape, "Num UV Sets", numUVSets );
		QModelIndex iBinorms  = nif->getIndex( iData, "Bitangents" );
		QModelIndex iTangents = nif->getIndex( iData, "Tangents" );
		nif->updateArray( iBinorms );
		nif->updateArray( iTangents );
		nif->setArray( iBinorms, bin );
		nif->setArray( iTangents, tan );
	} else if ( nif->getUserVersion2() >= 100 ) {
		int numVerts;
		// "Num Vertices" does not exist in the partition
		if ( iPartBlock.isValid() )
			numVerts = nif->get<uint>( iPartBlock, "Data Size" ) / nif->get<uint>( iPartBlock, "Vertex Size" );
		else
			numVerts = nif->get<int>( iShape, "Num Vertices" );

		nif->setState( BaseModel::Processing );
		for ( int i = 0; i < numVerts; i++ ) {
			auto idx = nif->index( i, 0, iData );

			nif->set<ByteVector3>( idx, "Tangent", tan[i] );
			nif->set<float>( idx, "Bitangent X", bin[i][0] );

			auto bitYi = round( ((bin[i][1] + 1.0) / 2.0) * 255.0 );
			auto bitZi = round( ((bin[i][2] + 1.0) / 2.0) * 255.0 );
			
			nif->set<quint8>( idx, "Bitangent Y", bitYi );
			nif->set<quint8>( idx, "Bitangent Z", bitZi );
		}
		nif->restoreState();
	}

	return iShape;
}

REGISTER_SPELL( spTangentSpace )

class spAllTangentSpaces final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Update All Tangent Spaces" ); }
	QString page() const override final { return Spell::tr( "Batch" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & idx ) override final
	{
		if ( !nif || idx.isValid() )
			return false;

		// If bethesda then we will configure the settings for the mesh.
		if ( nif->getUserVersion() == 11 )
			return true;

		// 10.1.0.0 and greater can have tangents and bitangents
		if ( nif->checkVersion( 0x0A010000, 0 ) )
			return true;

		return false;
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & ) override final
	{
		QList<QPersistentModelIndex> indices;

		spTangentSpace TSpacer;

		for ( int n = 0; n < nif->getBlockCount(); n++ ) {
			QModelIndex idx = nif->getBlock( n );

			if ( TSpacer.isApplicable( nif, idx ) )
				indices << idx;
		}

		for ( const QModelIndex& idx : indices ) {
			TSpacer.castIfApplicable( nif, idx );
		}

		return QModelIndex();
	}
};

REGISTER_SPELL( spAllTangentSpaces )


class spAddAllTangentSpaces final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Add Tangent Spaces and Update" ); }
	QString page() const override final { return Spell::tr( "Batch" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & idx ) override final
	{
		return nif && !idx.isValid() && nif->checkVersion( 0x0A010000, 0 );
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & ) override final
	{
		QVector<QModelIndex> blks;
		for ( int l = 0; l < nif->getBlockCount(); l++ ) {
			QModelIndex idx = nif->getBlock( l, "NiTriShape" );
			if ( !idx.isValid() )
				continue;

			// NiTriShapeData
			auto iData = nif->getBlock( nif->getLink( idx, "Data" ) );

			// Do not do anything without proper UV/Vert/Tri data
			auto numVerts = nif->get<int>( iData, "Num Vertices" );
			auto numTris = nif->get<int>( iData, "Num Triangles" );
			bool hasUVs = nif->get<int>( iData, "Vector Flags" ) & 1;
			if ( !hasUVs || !numVerts || !numTris )
				continue;

			nif->set<int>( iData, "Vector Flags", 4097 );
			nif->updateArray( iData, "Tangents" );
			nif->updateArray( iData, "Bitangents" );

			// Add NiTriShape for spTangentSpace
			blks << idx;
		}

		spTangentSpace update;
		for ( auto& b : blks )
			update.cast( nif, b );

		return QModelIndex();
	}
};

REGISTER_SPELL( spAddAllTangentSpaces )

