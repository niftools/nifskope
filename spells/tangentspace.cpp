#include "tangentspace.h"

#include "../NvTriStrip/qtwrapper.h"

#include <QDebug>

bool spTangentSpace::isApplicable( const NifModel * nif, const QModelIndex & index )
{
	QModelIndex iData = nif->getBlock( nif->getLink( index, "Data" ) );
	return nif->checkVersion( 0x14000004, 0x14000005 ) &&
		( ( nif->isNiBlock( index, "NiTriShape" ) && nif->isNiBlock( iData, "NiTriShapeData" ) )
		|| ( nif->isNiBlock( index, "NiTriStrips" ) && nif->isNiBlock( iData, "NiTriStripsData" ) ) );
}

QModelIndex spTangentSpace::cast( NifModel * nif, const QModelIndex & iBlock )
{
	QPersistentModelIndex iShape = iBlock;
	
	QModelIndex iData = nif->getBlock( nif->getLink( iShape, "Data" ) );
	
	QVector<Vector3> verts = nif->getArray<Vector3>( iData, "Vertices" );
	QVector<Vector3> norms = nif->getArray<Vector3>( iData, "Normals" );
	QVector<Color4> vxcol = nif->getArray<Color4>( iData, "Vertex Colors" );
	QModelIndex iTexCo = nif->getIndex( iData, "UV Sets" );
	if ( ! iTexCo.isValid() ) iTexCo = nif->getIndex( iData, "UV Sets 2" );
	iTexCo = iTexCo.child( 0, 0 );
	QVector<Vector2> texco = nif->getArray<Vector2>( iTexCo );
	
	QVector<Triangle> triangles;
	QModelIndex iPoints = nif->getIndex( iData, "Points" );
	if ( iPoints.isValid() )
	{
		QList< QVector< quint16 > > strips;
		for ( int r = 0; r < nif->rowCount( iPoints ); r++ )
			strips.append( nif->getArray<quint16>( iPoints.child( r, 0 ) ) );
		triangles = triangulate( strips );
	}
	else
	{
		triangles = nif->getArray<Triangle>( iData, "Triangles" );
	}
	
	if ( verts.isEmpty() || norms.count() != verts.count() || texco.count() != verts.count() || triangles.isEmpty() )
	{
		qWarning() << Spell::tr( "need vertices, normals, texture coordinates and faces to calculate tangents and binormals" );
		return iBlock;
	}
	
	QVector<Vector3> tan( verts.count() );
	QVector<Vector3> bin( verts.count() );
	
	QMultiHash<int,int> vmap;
	
	//int skptricnt = 0;
	
	for ( int t = 0; t < triangles.count(); t++ )
	{	// for each triangle caculate the texture flow direction
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
			//	qWarning() << t;
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
		
		for ( int j = 0; j < 3; j++ )
		{
			int i = tri[j];
			
			tan[i] += tdir;
			bin[i] += sdir;
		}
	}
	
	//qWarning() << "skipped triangles" << skptricnt;
	
	//int cnt = 0;
	
	for ( int i = 0; i < verts.count(); i++ )
	{	// for each vertex calculate tangent and binormal
		const Vector3 & n = norms[i];
		
		Vector3 & t = tan[i];
		Vector3 & b = bin[i];
		
		//qDebug() << n << t << b;
		
		if ( t == Vector3() || b == Vector3() )
		{
			t[0] = n[1]; t[1] = n[2]; t[2] = n[0];
			b = Vector3::crossproduct( n, t );
			//if ( cnt++ < 3 )
			//	qWarning() << i;
		}
		else
		{
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
	
	//qWarning() << "unassigned vertices" << cnt;
	
	// update or create the tangent space extra data
	
	QModelIndex iTSpace;
	foreach ( qint32 link, nif->getChildLinks( nif->getBlockNumber( iShape ) ) )
	{
		iTSpace = nif->getBlock( link, "NiBinaryExtraData" );
		if ( iTSpace.isValid() && nif->get<QString>( iTSpace, "Name" ) == "Tangent space (binormal & tangent vectors)" )
			break;
		else
			iTSpace = QModelIndex();
	}
	
	if ( ! iTSpace.isValid() )
	{
		iTSpace = nif->insertNiBlock( "NiBinaryExtraData", nif->getBlockNumber( iShape ) + 1 );
		nif->set<QString>( iTSpace, "Name", "Tangent space (binormal & tangent vectors)" );
		QModelIndex iNumExtras = nif->getIndex( iShape, "Num Extra Data List" );
		QModelIndex iExtras = nif->getIndex( iShape, "Extra Data List" );
		if ( iNumExtras.isValid() && iExtras.isValid() )
		{
			int numlinks = nif->get<int>( iNumExtras );
			nif->set<int>( iNumExtras, numlinks + 1 );
			nif->updateArray( iExtras );
			nif->setLink( iExtras.child( numlinks, 0 ), nif->getBlockNumber( iTSpace ) );
		}
	}
	
	nif->set<QByteArray>( iTSpace, "Binary Data", QByteArray( (const char *) tan.data(), tan.count() * sizeof( Vector3 ) ) + QByteArray( (const char *) bin.data(), bin.count() * sizeof( Vector3 ) ) );
	return iShape;
}

REGISTER_SPELL( spTangentSpace )

class spAllTangentSpaces : public Spell
{
public:
	QString name() const { return Spell::tr( "Update All Tangent Spaces" ); }
	QString page() const { return Spell::tr( "Batch" ); }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & idx )
	{
		return nif && nif->checkVersion( 0x14000004, 0x14000005 ) && ! idx.isValid();
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & )
	{
		QList< QPersistentModelIndex > indices;
		
		spTangentSpace TSpacer;
		
		for ( int n = 0; n < nif->getBlockCount(); n++ )
		{
			QModelIndex idx = nif->getBlock( n );
			if ( TSpacer.isApplicable( nif, idx ) )
				indices << idx;
		}
		
		foreach ( QModelIndex idx, indices )
		{
			TSpacer.castIfApplicable( nif, idx );
		}
		
		return QModelIndex();
	}
};

REGISTER_SPELL( spAllTangentSpaces )

