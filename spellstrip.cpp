/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005, NIF File Format Library and Tools
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the NIF File Format Library and Tools projectmay not be
   used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

***** END LICENCE BLOCK *****/

#include "spellbook.h"

#include "NvTriStrip/NvTriStrip.h"

#include <QDebug>

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
	
	QList< QVector<quint16> > strips;

	SetStitchStrips( false );
	GenerateStrips( data, triangles.count()*3, &groups, &numGroups );
	free( data );
	
	if ( !groups )
	{
		return strips;
	}
	
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
		else
		{
			qDebug() << g << "is of type" << groups[g].type;
		}
	}
	
	delete [] groups;
	
	return strips;
}

template <typename T> void copyArray( NifModel * nif, const QModelIndex & iDst, const QModelIndex & iSrc, qint32 ofs = 0, qint32 num = -1 )
{
	if ( iDst.isValid() && iSrc.isValid() )
	{
		if ( num < 0 || num >= nif->rowCount( iSrc ) ) num = nif->rowCount( iSrc );
		nif->updateArray( iDst );
		for ( qint32 v = 0; v < num && ofs + v < nif->rowCount( iDst ); v++ )
		{
			nif->setItemData<T>( iDst.child( ofs + v, 0 ), nif->itemData<T>( iSrc.child( v, 0 ) ) );
		}
	}
}

template <typename T> void copyArray( NifModel * nif, const QModelIndex & iDst, const QModelIndex & iSrc, const QString & name, qint32 ofs = 0, qint32 num = -1 )
{
	copyArray<T>( nif, nif->getIndex( iDst, name ), nif->getIndex( iSrc, name ), ofs, num );
}

template <typename T> void copyValue( NifModel * nif, const QModelIndex & iDst, const QModelIndex & iSrc, const QString & name )
{
	nif->set<T>( iDst, name, nif->get<T>( iSrc, name ) );
}

class spStrippify : public Spell
{
	QString name() const { return "Strippify"; }
	QString page() const { return "Mesh"; }
	
	bool isApplicable( NifModel * nif, const QModelIndex & index )
	{
		return ( nif->itemType( index ) == "NiBlock" && nif->itemName( index ) == "NiTriShape" );
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
				Triangle tri = nif->itemData<Triangle>( iTriangles.child( t, 0 ) );
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
			
			copyValue<Vector3>( nif, iStripData, iData, "Center" );
			copyValue<float>( nif, iStripData, iData, "Radius" );
			
			QModelIndex iLengths = nif->getIndex( iStripData, "Strip Lengths" );
			QModelIndex iPoints = nif->getIndex( iStripData, "Points" );
			
			if ( iLengths.isValid() && iPoints.isValid() )
			{
				nif->set<int>( iStripData, "Num Strips", strips.count() );
				nif->updateArray( iLengths );
				nif->set<int>( iStripData, "Has Points", 1 );
				nif->updateArray( iPoints );
				int x = 0;
				int z = 0;
				foreach ( QVector<quint16> strip, strips )
				{
					nif->setItemData<int>( iLengths.child( x, 0 ), strip.count() );
					QModelIndex iStrip = iPoints.child( x, 0 );
					if ( iStrip.isValid() )
					{
						nif->updateArray( iStrip );
						for ( int y = 0; y < strip.count(); y++ )
						{
							nif->setItemData<int>( iStrip.child( y, 0 ), strip[y] );
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
