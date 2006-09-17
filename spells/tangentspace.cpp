#include "../spellbook.h"

#include "../NvTriStrip/qtwrapper.h"

#include <QDebug>

#define TOLERANCE ( PI * 1 / 180 )

static QDebug operator<<( QDebug dbg, const Vector3 & v )
{
	return dbg << QString( "( %1, %2, %3 )" ).arg( v[0], 0, 'f', 1 ).arg( v[1], 0, 'f', 1 ).arg( v[2], 0, 'f', 1 ).toAscii().data();
}

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
		
		bool isSkinned = nif->getLink( iShape, "Skin Instance" ) >= 0;
		
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
		
		QMultiHash<int,int> vmap;
		
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
			
			if ( fabs( r ) <= 10e-5 )	continue;
			
			r = 1.0 / r;
			
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
			{	// no duplication, just smoothing
				int i = tri[j];
				
				tan[i] += tdir;
				bin[i] += sdir;
			}
			
			/*
			for ( int j = 0; j < 3; j++ )
			{	// store flow direction and duplicate vertices if nescessarry
				int i = tri[j];
				
				tan[i] += tdir;
				bin[i] += sdir;
				
				QList<int> indices = vmap.values( i );
				if ( indices.isEmpty() )
				{
					tan[i] = tdir;
					bin[i] = sdir;
					vmap.insert( i, i );
				}
				else if ( ! isSkinned )
				{	// dunno exactly, let's skip the duplication if it's a skinned mesh and hope for the best
					int x;
					for ( x = 0; x < indices.count(); x++ )
					{
						if ( matches( tan[indices[x]], tdir, TOLERANCE ) && matches( bin[indices[x]], sdir, TOLERANCE ) )
						{
							tri[j] = indices[x];
							break;
						}
					}
					if ( x >= indices.count() )
					{
						vmap.insert( i, verts.count() );
						tri[j] = verts.count();
						verts.append( verts[i] );
						norms.append( norms[i] );
						texco.append( texco[i] );
						if ( vxcol.count() )
							vxcol.append( vxcol[i] );
						tan.append( tdir );
						bin.append( sdir );
						dups++;
					}
				}
			}
			*/
		}
		
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
			}
			
			else
			{
				t = ( t - n * Vector3::dotproduct( n, t ) );
				t.normalize();
				//b = Vector3::crossproduct( n, t );
				b = ( b - n * Vector3::dotproduct( n, b ) );
				b.normalize();
			}
			
			//qDebug() << n << t << b;
			//qDebug() << "";
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
		return iShape;
	}
	
	bool matches( const Vector3 & a, const Vector3 & b, float tolerance )
	{
		return Vector3::angle( a, b ) < tolerance;
	}
};

REGISTER_SPELL( spTangentSpace )
