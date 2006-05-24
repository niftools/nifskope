#include "../spellbook.h"

#include "../NvTriStrip/qtwrapper.h"

#include <QDebug>

#define TOLERANCE ( PI * 1 / 180 )

class spTangentSpace : public Spell
{
	QString name() const { return "Update Tangent Space"; }
	QString page() const { return "Mesh"; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		QModelIndex iData = nif->getBlock( nif->getLink( index, "Data" ) );
		return ( nif->isNiBlock( index, "NiTriShape" ) && nif->isNiBlock( iData, "NiTriShapeData" ) )
			|| ( nif->isNiBlock( index, "NiTriStrips" ) && nif->isNiBlock( iData, "NiTriStripsData" ) );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & iBlock )
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
			qWarning() << "need vertices, normals, texture coordinates and faces to calculate tangents and binormals";
			return iBlock;
		}
		
		QVector<Vector3> tan( verts.count() );
		QVector<Vector3> bin( verts.count() );
		
		int dups = 0;
		
		for ( int t = 0; t < triangles.count(); t++ )
		{	// for each triangle caculate the texture flow direction
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
			
			float x1 = v2[0] - v1[0];
			float x2 = v3[0] - v1[0];
			float y1 = v2[1] - v1[1];
			float y2 = v3[1] - v1[1];
			float z1 = v2[2] - v1[2];
			float z2 = v3[2] - v1[2];
			
			float s1 = w2[0] - w1[0];
			float s2 = w3[0] - w1[0];
			float t1 = w2[1] - w1[1];
			float t2 = w3[1] - w1[1];
			
			if ( fabs( s1 * t2 - s2 * t1 ) <= 10e-5 )
				continue;
			
			float r = 1.0 / ( s1 * t2 - s2 * t1 );
			Vector3 sdir( ( t2 * x1 - t1 * x2 ) * r, ( t2 * y1 - t1 * y2 ) * r, ( t2 * z1 - t1 * z2 ) * r );
			Vector3 tdir( ( s1 * x2 - s2 * x1 ) * r, ( s1 * y2 - s2 * y1 ) * r, ( s1 * z2 - s2 * z1 ) * r );
			
			for ( int j = 0; j < 3; j++ )
			{	// store flow direction and duplicate vertices if nescessarry
				int i = tri[j];
				if ( tan[i] == Vector3() )
					tan[i] = tdir;
				else if ( ! matches( tan[i], tdir, TOLERANCE ) )
				{
					tri[j] = verts.count();
					verts.append( verts[i] );
					norms.append( norms[i] );
					texco.append( texco[i] );
					if ( vxcol.count() )
						vxcol.append( vxcol[i] );
					tan.append( tdir );
					bin.append( Vector3() );
					dups++;
				}
			}
		}
		
		for ( int i = 0; i < verts.count(); i++ )
		{	// for each vertex calculate tangent and binormal
			const Vector3 & n = norms[i];
			
			Vector3 & t = tan[i];
			Vector3 & b = bin[i];
			
			if ( t == Vector3() )
			{
				t[0] = n[1]; t[1] = n[2]; t[2] = n[0];
			}
			
			t = ( t - n * Vector3::dotproduct( n, t ) );
			t.normalize();
			b = ( Vector3::crossproduct( n, t ) );
			
			//qWarning() << QString( "tan[%1] %2 %3 %4" ).arg( i ).arg( t[0], 0, 'f', 4 ).arg( t[1], 0, 'f', 4 ).arg( t[2], 0, 'f', 4 );
			//qWarning() << QString( "bin[%1] %2 %3 %4" ).arg( i ).arg( b[0], 0, 'f', 4 ).arg( b[1], 0, 'f', 4 ).arg( b[2], 0, 'f', 4 );
		}
		
		if ( dups != 0 )
		{
			qWarning() << "duplicated" << dups << "vertices";
			
			// write back data arrays
			nif->set<int>( iData, "Num Vertices", verts.count() );
			nif->updateArray( iData, "Vertices" );
			nif->setArray<Vector3>( iData, "Vertices", verts );
			nif->updateArray( iData, "Normals" );
			nif->setArray<Vector3>( iData, "Normals", norms );
			nif->updateArray( iTexCo );
			nif->setArray<Vector2>( iTexCo, texco );
			nif->updateArray( iData, "Vertex Colors" );
			nif->setArray<Color4>( iData, "Vertex Colors", vxcol );
			
			if ( iPoints.isValid() )
			{
				QList< QVector< quint16 > > strips = strippify( triangles );
				nif->set<int>( iData, "Num Strips", strips.count() );
				nif->set<int>( iData, "Has Points", 1 );
				
				QModelIndex iLengths = nif->getIndex( iData, "Strip Lengths" );
				
				if ( iLengths.isValid() )
				{
					nif->updateArray( iLengths );
					nif->updateArray( iPoints );
					int x = 0;
					int z = 0;
					foreach ( QVector<quint16> strip, strips )
					{
						nif->set<int>( iLengths.child( x, 0 ), strip.count() );
						QModelIndex iStrip = iPoints.child( x, 0 );
						nif->updateArray( iStrip );
						nif->setArray<quint16>( iStrip, strip );
						x++;
						z += strip.count() - 2;
					}
					nif->set<int>( iData, "Num Triangles", z );
				}
			}
			else
			{
				nif->setArray<Triangle>( iData, "Triangles", triangles );
			}
		}
		
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
		/*
		nif->set<int>( iTSpace, "Size", 24 * verts.count() );
		nif->updateArray( iTSpace, "Tangents" );
		nif->setArray<Vector3>( iTSpace, "Tangents", tan );
		nif->updateArray( iTSpace, "Binormals" );
		nif->setArray<Vector3>( iTSpace, "Binormals", bin );
		*/
		return iShape;
	}
	
	bool matches( const Vector3 & a, const Vector3 & b, float tolerance )
	{
		return Vector3::angle( a, b ) < tolerance; //fabs( a[0] - b[0] ) < tolerance && fabs( a[1] - b[1] ) < tolerance && fabs( a[2] - b[2] ) < tolerance;
	}
};

REGISTER_SPELL( spTangentSpace )
