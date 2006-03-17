#include "spellbook.h"

#include "../NvTriStrip/NvTriStrip.h"

#include <QMessageBox>

QList< QVector<quint16> > strippify( QVector<Triangle> triangles )
{
	unsigned short * data = (unsigned short *) malloc( triangles.count() * 3 * sizeof( unsigned short ) );
	for ( int t = 0; t < triangles.count(); t++ )
	{
		data[ t * 3 + 0 ] = triangles[t][0];
		data[ t * 3 + 1 ] = triangles[t][1];
		data[ t * 3 + 2 ] = triangles[t][2];
	}
	
	PrimitiveGroup * groups = 0;
	unsigned short numGroups = 0;
	
	SetStitchStrips( false );
	//SetCacheSize( 64 );
	GenerateStrips( data, triangles.count()*3, &groups, &numGroups );
	free( data );
	
	QList< QVector<quint16> > strips;

	if ( !groups )
		return strips;
	
	for ( int g = 0; g < numGroups; g++ )
	{
		if ( groups[g].type == PT_STRIP )
		{
			QVector< quint16 > strip( groups[g].numIndices, 0 );
			for ( quint32 s = 0; s < groups[g].numIndices; s++ )
			{
				strip[s] = groups[g].indices[s];
			}
			strips.append( strip );
		}
	}
	
	delete [] groups;
	
	return strips;
}

QVector<Triangle> triangulate( QList< QVector<quint16> > strips )
{
	QVector<Triangle> tris;
	foreach( QVector<quint16> strip, strips )
	{
		quint16 a, b = strip.value( 0 ), c = strip.value( 1 );
		bool flip = false;
		for ( int s = 2; s < strip.count(); s++ )
		{
			a = b;
			b = c;
			c = strip.value( s );
			if ( a != b && b != c && c != a )
			{
				if ( ! flip )
					tris.append( Triangle( a, b, c ) );
				else
					tris.append( Triangle( a, c, b ) );
			}
			flip = ! flip;
		}
	}
	return tris;
}

template <typename T> void copyArray( NifModel * nif, const QModelIndex & iDst, const QModelIndex & iSrc )
{
	if ( iDst.isValid() && iSrc.isValid() )
	{
		nif->updateArray( iDst );
		nif->setArray<T>( iDst, nif->getArray<T>( iSrc ) );
	}
}

template <typename T> void copyArray( NifModel * nif, const QModelIndex & iDst, const QModelIndex & iSrc, const QString & name )
{
	copyArray<T>( nif, nif->getIndex( iDst, name ), nif->getIndex( iSrc, name ) );
}

template <typename T> void copyValue( NifModel * nif, const QModelIndex & iDst, const QModelIndex & iSrc, const QString & name )
{
	nif->set<T>( iDst, name, nif->get<T>( iSrc, name ) );
}

class spStrippify : public Spell
{
	QString name() const { return "Strippify"; }
	QString page() const { return "Mesh"; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return ( ! nif->getVersion().startsWith( "4." ) ) && nif->itemType( index ) == "NiBlock" && nif->itemName( index ) == "NiTriShape";
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		QPersistentModelIndex idx = index;
		QPersistentModelIndex iData = nif->getBlock( nif->getLink( idx, "Data" ), "NiTriShapeData" );
		
		if ( ! iData.isValid() )	return idx;
		
		QVector<Triangle> triangles;
		QModelIndex iTriangles = nif->getIndex( iData, "Triangles" );
		if ( iTriangles.isValid() )
		{
			int skip = 0;
			for ( int t = 0; t < nif->rowCount( iTriangles ); t++ )
			{
				Triangle tri = nif->get<Triangle>( iTriangles.child( t, 0 ) );
				if ( tri[0] != tri[1] && tri[1] != tri[2] && tri[2] != tri[0] )
					triangles.append( tri );
				else
					skip++;
			}
			//qWarning() << "num triangles" << triangles.count() << "skipped" << skip;
		}
		else
			return idx;
		
		QList< QVector<quint16> > strips = strippify ( triangles );
		
		if ( strips.count() <= 0 )
			return idx;
		
		nif->insertNiBlock( "NiTriStripsData", nif->getBlockNumber( idx )+1 );
		QModelIndex iStripData = nif->getBlock( nif->getBlockNumber( idx ) + 1, "NiTriStripsData" );
		if ( iStripData.isValid() )
		{
			copyValue<int>( nif, iStripData, iData, "Num Vertices" );
			
			nif->set<int>( iStripData, "Has Vertices", 1 );
			copyArray<Vector3>( nif, iStripData, iData, "Vertices" );
			
			copyValue<int>( nif, iStripData, iData, "Has Normals" );
			copyArray<Vector3>( nif, iStripData, iData, "Normals" );
			
			copyValue<int>( nif, iStripData, iData, "Has Vertex Colors" );
			copyArray<Color4>( nif, iStripData, iData, "Vertex Colors" );
			
			copyValue<int>( nif, iStripData, iData, "Has UV" );
			copyValue<int>( nif, iStripData, iData, "Num UV Sets" );
			copyValue<int>( nif, iStripData, iData, "Num UV Sets 2" );
			QModelIndex iDstUV = nif->getIndex( iStripData, "UV Sets" );
			QModelIndex iSrcUV = nif->getIndex( iData, "UV Sets" );
			if ( iDstUV.isValid() && iSrcUV.isValid() )
			{
				nif->updateArray( iDstUV );
				for ( int r = 0; r < nif->rowCount( iDstUV ); r++ )
				{
					copyArray<Vector2>( nif, iDstUV.child( r, 0 ), iSrcUV.child( r, 0 ) );
				}
			}
			iDstUV = nif->getIndex( iStripData, "UV Sets 2" );
			iSrcUV = nif->getIndex( iData, "UV Sets 2" );
			if ( iDstUV.isValid() && iSrcUV.isValid() )
			{
				nif->updateArray( iDstUV );
				for ( int r = 0; r < nif->rowCount( iDstUV ); r++ )
				{
					copyArray<Vector2>( nif, iDstUV.child( r, 0 ), iSrcUV.child( r, 0 ) );
				}
			}
			
			copyValue<Vector3>( nif, iStripData, iData, "Center" );
			copyValue<float>( nif, iStripData, iData, "Radius" );
			
			nif->set<int>( iStripData, "Has Points", 1 );
			
			QModelIndex iLengths = nif->getIndex( iStripData, "Strip Lengths" );
			QModelIndex iPoints = nif->getIndex( iStripData, "Points" );
			
			if ( iLengths.isValid() && iPoints.isValid() )
			{
				nif->set<int>( iStripData, "Num Strips", strips.count() );
				nif->updateArray( iLengths );
				nif->updateArray( iPoints );
				int x = 0;
				int z = 0;
				foreach ( QVector<quint16> strip, strips )
				{
					nif->set<int>( iLengths.child( x, 0 ), strip.count() );
					QModelIndex iStrip = iPoints.child( x, 0 );
					if ( iStrip.isValid() )
					{
						nif->updateArray( iStrip );
						for ( int y = 0; y < strip.count(); y++ )
						{
							nif->set<int>( iStrip.child( y, 0 ), strip[y] );
						}
					}
					x++;
					z += strip.count() - 2;
				}
				nif->set<int>( iStripData, "Num Triangles", z );
				
				nif->setData( idx.sibling( idx.row(), NifModel::NameCol ), "NiTriStrips" );
				int lnk = nif->getLink( idx, "Data" );
				nif->setLink( idx, "Data", nif->getBlockNumber( iStripData ) );
				nif->removeNiBlock( lnk );
			}
		}
		return idx;
	}
};

REGISTER_SPELL( spStrippify )

class spTriangulate : public Spell
{
	QString name() const { return "Triangulate"; }
	QString page() const { return "Mesh"; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return ( nif->itemType( index ) == "NiBlock" && nif->itemName( index ) == "NiTriStrips" );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		QPersistentModelIndex idx = index;
		QPersistentModelIndex iStripData = nif->getBlock( nif->getLink( idx, "Data" ), "NiTriStripsData" );
		
		if ( ! iStripData.isValid() )	return idx;
		
		QList< QVector<quint16> > strips;

		QModelIndex iPoints = nif->getIndex( iStripData, "Points" );
		
		if ( ! iPoints.isValid() ) return idx;
		
		for ( int s = 0; s < nif->rowCount( iPoints ); s++ )
		{
			QVector<quint16> strip;
			QModelIndex iStrip = iPoints.child( s, 0 );
			for ( int p = 0; p < nif->rowCount( iStrip ); p++ )
				strip.append( nif->get<int>( iStrip.child( p, 0 ) ) );
			strips.append( strip );
		}
		
		QVector<Triangle> triangles = triangulate ( strips );
		
		nif->insertNiBlock( "NiTriShapeData", nif->getBlockNumber( idx ) + 1 );
		QModelIndex iTriData = nif->getBlock( nif->getBlockNumber( idx ) + 1, "NiTriShapeData" );
		if ( iTriData.isValid() )
		{
			copyValue<int>( nif, iTriData, iStripData, "Num Vertices" );
			
			nif->set<int>( iTriData, "Has Vertices", 1 );
			copyArray<Vector3>( nif, iTriData, iStripData, "Vertices" );
			
			copyValue<int>( nif, iTriData, iStripData, "Has Normals" );
			copyArray<Vector3>( nif, iTriData, iStripData, "Normals" );
			
			copyValue<int>( nif, iTriData, iStripData, "Has Vertex Colors" );
			copyArray<Color4>( nif, iTriData, iStripData, "Vertex Colors" );
			
			copyValue<int>( nif, iTriData, iStripData, "Has UV" );
			copyValue<int>( nif, iTriData, iStripData, "Num UV Sets" );
			copyValue<int>( nif, iTriData, iStripData, "Num UV Sets 2" );
			QModelIndex iDstUV = nif->getIndex( iTriData, "UV Sets" );
			QModelIndex iSrcUV = nif->getIndex( iStripData, "UV Sets" );
			if ( iDstUV.isValid() && iSrcUV.isValid() )
			{
				nif->updateArray( iDstUV );
				for ( int r = 0; r < nif->rowCount( iDstUV ); r++ )
				{
					copyArray<Vector2>( nif, iDstUV.child( r, 0 ), iSrcUV.child( r, 0 ) );
				}
			}
			iDstUV = nif->getIndex( iTriData, "UV Sets 2" );
			iSrcUV = nif->getIndex( iStripData, "UV Sets 2" );
			if ( iDstUV.isValid() && iSrcUV.isValid() )
			{
				nif->updateArray( iDstUV );
				for ( int r = 0; r < nif->rowCount( iDstUV ); r++ )
				{
					copyArray<Vector2>( nif, iDstUV.child( r, 0 ), iSrcUV.child( r, 0 ) );
				}
			}
			
			copyValue<Vector3>( nif, iTriData, iStripData, "Center" );
			copyValue<float>( nif, iTriData, iStripData, "Radius" );
			
			nif->set<int>( iTriData, "Num Triangles", triangles.count() );
			nif->set<int>( iTriData, "Num Triangle Points", triangles.count() * 3 );
			nif->set<int>( iTriData, "Has Triangles", 1 );
			
			QModelIndex iTriangles = nif->getIndex( iTriData, "Triangles" );
			if ( iTriangles.isValid() )
			{
				nif->updateArray( iTriangles );
				nif->setArray<Triangle>( iTriangles, triangles );
			}
			
			nif->setData( idx.sibling( idx.row(), NifModel::NameCol ), "NiTriShape" );
			int lnk = nif->getLink( idx, "Data" );
			nif->setLink( idx, "Data", nif->getBlockNumber( iTriData ) );
			nif->removeNiBlock( lnk );
		}
		return idx;
	}
};

REGISTER_SPELL( spTriangulate )
