#include "spellbook.h"

#include "blocks.h"

#include "lib/nvtristripwrapper.h"

#include <climits>


// TODO: Move these to blocks.h / misc.h / wherever
template <typename T> void copyArray( NifModel * nif, const QModelIndex & iDst, const QModelIndex & iSrc )
{
	if ( iDst.isValid() && iSrc.isValid() ) {
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


class spStrippify final : public Spell
{
	QString name() const override final { return Spell::tr( "Stripify" ); }
	QString page() const override final { return Spell::tr( "Mesh" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		return nif->checkVersion( 0x0a000000, 0 ) && nif->isNiBlock( index, "NiTriShape" );
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		QPersistentModelIndex idx = index;
		QPersistentModelIndex iData = nif->getBlock( nif->getLink( idx, "Data" ), "NiTriShapeData" );

		if ( !iData.isValid() )
			return idx;

		QVector<Triangle> triangles;
		QModelIndex iTriangles = nif->getIndex( iData, "Triangles" );

		if ( !iTriangles.isValid() )
			return idx;

		int skip = 0;

		for ( int t = 0; t < nif->rowCount( iTriangles ); t++ ) {
			Triangle tri = nif->get<Triangle>( iTriangles.child( t, 0 ) );

			if ( tri[0] != tri[1] && tri[1] != tri[2] && tri[2] != tri[0] )
				triangles.append( tri );
			else
				skip++;
		}

		//qDebug() << "num triangles" << triangles.count() << "skipped" << skip;


		QVector<QVector<quint16> > strips = stripify( triangles, true );

		if ( strips.count() <= 0 )
			return idx;

		uint numTriangles = 0;
		for ( const QVector<quint16>& strip : strips ) {
			numTriangles += strip.count() - 2;
		}

		if ( numTriangles > USHRT_MAX * 2 ) {
			Message::append( tr( "Strippify failed on one or more blocks." ),
				tr( "Block %1: Too many triangles (%2) to strippify this shape." )
				.arg( nif->getBlockNumber( idx ) )
				.arg( numTriangles )
			);
			return idx;
		}

		QModelIndex iStripData = nif->insertNiBlock( "NiTriStripsData", nif->getBlockNumber( idx ) + 1 );

		if ( iStripData.isValid() ) {
			copyValue<int>( nif, iStripData, iData, "Num Vertices" );

			nif->set<int>( iStripData, "Has Vertices", 1 );
			copyArray<Vector3>( nif, iStripData, iData, "Vertices" );

			copyValue<int>( nif, iStripData, iData, "Has Normals" );
			copyArray<Vector3>( nif, iStripData, iData, "Normals" );

			copyValue<int>( nif, iStripData, iData, "TSpace Flag" );
			copyArray<Vector3>( nif, iStripData, iData, "Bitangents" );
			copyArray<Vector3>( nif, iStripData, iData, "Tangents" );

			copyValue<int>( nif, iStripData, iData, "Has Vertex Colors" );
			copyArray<Color4>( nif, iStripData, iData, "Vertex Colors" );

			copyValue<int>( nif, iStripData, iData, "Has UV" );
			copyValue<int>( nif, iStripData, iData, "Num UV Sets" );
			copyValue<int>( nif, iStripData, iData, "Vector Flags" );
			copyValue<int>( nif, iStripData, iData, "BS Vector Flags" );
			copyValue<int>( nif, iStripData, iData, "Num UV Sets 2" );
			QModelIndex iDstUV = nif->getIndex( iStripData, "UV Sets" );
			QModelIndex iSrcUV = nif->getIndex( iData, "UV Sets" );

			if ( iDstUV.isValid() && iSrcUV.isValid() ) {
				nif->updateArray( iDstUV );

				for ( int r = 0; r < nif->rowCount( iDstUV ); r++ ) {
					copyArray<Vector2>( nif, iDstUV.child( r, 0 ), iSrcUV.child( r, 0 ) );
				}
			}

			iDstUV = nif->getIndex( iStripData, "UV Sets 2" );
			iSrcUV = nif->getIndex( iData, "UV Sets 2" );

			if ( iDstUV.isValid() && iSrcUV.isValid() ) {
				nif->updateArray( iDstUV );

				for ( int r = 0; r < nif->rowCount( iDstUV ); r++ ) {
					copyArray<Vector2>( nif, iDstUV.child( r, 0 ), iSrcUV.child( r, 0 ) );
				}
			}

			copyValue<Vector3>( nif, iStripData, iData, "Center" );
			copyValue<float>( nif, iStripData, iData, "Radius" );

			nif->set<int>( iStripData, "Num Strips", strips.count() );
			nif->set<int>( iStripData, "Has Points", 1 );

			QModelIndex iLengths = nif->getIndex( iStripData, "Strip Lengths" );
			QModelIndex iPoints  = nif->getIndex( iStripData, "Points" );

			if ( iLengths.isValid() && iPoints.isValid() ) {
				nif->updateArray( iLengths );
				nif->updateArray( iPoints );
				int x = 0;
				for ( const QVector<quint16>& strip : strips ) {
					nif->set<int>( iLengths.child( x, 0 ), strip.count() );
					QModelIndex iStrip = iPoints.child( x, 0 );
					nif->updateArray( iStrip );
					nif->setArray<quint16>( iStrip, strip );
					x++;
				}
				nif->set<int>( iStripData, "Num Triangles", numTriangles );

				nif->setData( idx.sibling( idx.row(), NifModel::NameCol ), "NiTriStrips" );
				int lnk = nif->getLink( idx, "Data" );
				nif->setLink( idx, "Data", nif->getBlockNumber( iStripData ) );
				nif->removeNiBlock( lnk );
			}
		}

		// Move the triangles over 65535 into their own shape by
		// splitting the two strips between two NiTriStrips
		if ( numTriangles > USHRT_MAX ) {
			spDuplicateBranch dupe;

			// Copy the entire NiTriStrips branch
			auto iStrip2 = dupe.cast( nif, idx );
			auto iStrip2Data = nif->getBlock( nif->getLink( iStrip2, "Data" ), "NiTriStripsData" );
			if ( !iStrip2Data.isValid() || strips.count() != 2 )
				return QModelIndex();

			// Update Original Shape
			nif->set<int>( iStripData, "Num Strips", 1 );
			nif->set<int>( iStripData, "Has Points", 1 );

			QModelIndex iLengths = nif->getIndex( iStripData, "Strip Lengths" );
			QModelIndex iPoints = nif->getIndex( iStripData, "Points" );

			auto stripsA = strips.at(0);
			if ( iLengths.isValid() && iPoints.isValid() ) {
				nif->updateArray( iLengths );
				nif->set<quint16>( iLengths.child( 0, 0 ), stripsA.count() );
				nif->updateArray( iPoints );
				nif->updateArray( iPoints.child( 0, 0 ) );
				nif->setArray<quint16>( iPoints.child( 0, 0 ), stripsA );
				nif->set<quint16>( iStripData, "Num Triangles", stripsA.count() - 2 );
			}
			
			// Update New Shape
			nif->set<int>( iStrip2Data, "Num Strips", 1 );
			nif->set<int>( iStrip2Data, "Has Points", 1 );

			iLengths = nif->getIndex( iStrip2Data, "Strip Lengths" );
			iPoints = nif->getIndex( iStrip2Data, "Points" );

			auto stripsB = strips.at(1);
			if ( iLengths.isValid() && iPoints.isValid() ) {
				nif->updateArray( iLengths );
				nif->set<quint16>( iLengths.child( 0, 0 ), stripsB.count() );
				nif->updateArray( iPoints );
				nif->updateArray( iPoints.child( 0, 0 ) );
				nif->setArray<quint16>( iPoints.child( 0, 0 ), stripsB );
				nif->set<quint16>( iStrip2Data, "Num Triangles", stripsB.count() - 2 );
			}
		}

		return idx;
	}
};

REGISTER_SPELL( spStrippify )


class spStrippifyAll final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Stripify all TriShapes" ); }
	QString page() const override final { return Spell::tr( "Optimize" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		return nif->checkVersion( 0x0a000000, 0 ) && !index.isValid();
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & ) override final
	{
		QList<QPersistentModelIndex> iTriShapes;

		for ( int l = 0; l < nif->getBlockCount(); l++ ) {
			QModelIndex idx = nif->getBlock( l, "NiTriShape" );

			if ( idx.isValid() )
				iTriShapes << idx;
		}

		spStrippify Stripper;

		for ( const QModelIndex& idx : iTriShapes ) {
			Stripper.castIfApplicable( nif, idx );
		}

		return QModelIndex();
	}
};

REGISTER_SPELL( spStrippifyAll )


class spTriangulate final : public Spell
{
	QString name() const override final { return Spell::tr( "Triangulate" ); }
	QString page() const override final { return Spell::tr( "Mesh" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		return nif->isNiBlock( index, "NiTriStrips" );
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		QPersistentModelIndex idx = index;
		QPersistentModelIndex iStripData = nif->getBlock( nif->getLink( idx, "Data" ), "NiTriStripsData" );

		if ( !iStripData.isValid() )
			return idx;

		QVector<QVector<quint16> > strips;

		QModelIndex iPoints = nif->getIndex( iStripData, "Points" );

		if ( !iPoints.isValid() )
			return idx;

		for ( int s = 0; s < nif->rowCount( iPoints ); s++ ) {
			QVector<quint16> strip;
			QModelIndex iStrip = iPoints.child( s, 0 );

			for ( int p = 0; p < nif->rowCount( iStrip ); p++ )
				strip.append( nif->get<int>( iStrip.child( p, 0 ) ) );

			strips.append( strip );
		}

		QVector<Triangle> triangles = triangulate( strips );

		nif->insertNiBlock( "NiTriShapeData", nif->getBlockNumber( idx ) + 1 );
		QModelIndex iTriData = nif->getBlock( nif->getBlockNumber( idx ) + 1, "NiTriShapeData" );

		if ( iTriData.isValid() ) {
			copyValue<int>( nif, iTriData, iStripData, "Num Vertices" );

			nif->set<int>( iTriData, "Has Vertices", 1 );
			copyArray<Vector3>( nif, iTriData, iStripData, "Vertices" );

			copyValue<int>( nif, iTriData, iStripData, "Has Normals" );
			copyArray<Vector3>( nif, iTriData, iStripData, "Normals" );

			copyValue<int>( nif, iTriData, iStripData, "TSpace Flag" );
			copyArray<Vector3>( nif, iTriData, iStripData, "Bitangents" );
			copyArray<Vector3>( nif, iTriData, iStripData, "Tangents" );

			copyValue<int>( nif, iTriData, iStripData, "Has Vertex Colors" );
			copyArray<Color4>( nif, iTriData, iStripData, "Vertex Colors" );

			copyValue<int>( nif, iTriData, iStripData, "Has UV" );
			copyValue<int>( nif, iTriData, iStripData, "Num UV Sets" );
			copyValue<int>( nif, iTriData, iStripData, "Vector Flags" );
			copyValue<int>( nif, iTriData, iStripData, "BS Vector Flags" );
			copyValue<int>( nif, iTriData, iStripData, "Num UV Sets 2" );
			QModelIndex iDstUV = nif->getIndex( iTriData, "UV Sets" );
			QModelIndex iSrcUV = nif->getIndex( iStripData, "UV Sets" );

			if ( iDstUV.isValid() && iSrcUV.isValid() ) {
				nif->updateArray( iDstUV );

				for ( int r = 0; r < nif->rowCount( iDstUV ); r++ ) {
					copyArray<Vector2>( nif, iDstUV.child( r, 0 ), iSrcUV.child( r, 0 ) );
				}
			}

			iDstUV = nif->getIndex( iTriData, "UV Sets 2" );
			iSrcUV = nif->getIndex( iStripData, "UV Sets 2" );

			if ( iDstUV.isValid() && iSrcUV.isValid() ) {
				nif->updateArray( iDstUV );

				for ( int r = 0; r < nif->rowCount( iDstUV ); r++ ) {
					copyArray<Vector2>( nif, iDstUV.child( r, 0 ), iSrcUV.child( r, 0 ) );
				}
			}

			copyValue<Vector3>( nif, iTriData, iStripData, "Center" );
			copyValue<float>( nif, iTriData, iStripData, "Radius" );

			nif->set<int>( iTriData, "Num Triangles", triangles.count() );
			nif->set<int>( iTriData, "Num Triangle Points", triangles.count() * 3 );
			nif->set<int>( iTriData, "Has Triangles", 1 );

			QModelIndex iTriangles = nif->getIndex( iTriData, "Triangles" );

			if ( iTriangles.isValid() ) {
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


class spTriangulateAll final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Triangulate All Strips" ); }
	QString page() const override final { return Spell::tr( "Batch" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		return !index.isValid();
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & ) override final
	{
		QList<QPersistentModelIndex> triStrips;

		for ( int l = 0; l < nif->getBlockCount(); l++ ) {
			QModelIndex idx = nif->getBlock( l, "NiTriStrips" );
			if ( idx.isValid() )
				triStrips << idx;
		}

		spTriangulate tri;
		for ( const QModelIndex& idx : triStrips )
			tri.castIfApplicable( nif, idx );

		return QModelIndex();
	}
};

REGISTER_SPELL( spTriangulateAll )


class spStichStrips final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Stich Strips" ); }
	QString page() const override final { return Spell::tr( "Mesh" ); }

	static QModelIndex getStripsData( const NifModel * nif, const QModelIndex & index )
	{
		if ( nif->isNiBlock( index, "NiTriStrips" ) )
			return nif->getBlock( nif->getLink( index, "Data" ), "NiTriStripsData" );

		return nif->getBlock( index, "NiTriStripsData" );
	}

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		QModelIndex iData = getStripsData( nif, index );
		return iData.isValid() && nif->get<int>( iData, "Num Strips" ) > 1;
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		QModelIndex iData = getStripsData( nif, index );
		QModelIndex iLength = nif->getIndex( iData, "Strip Lengths" );
		QModelIndex iPoints = nif->getIndex( iData, "Points" );

		if ( !( iLength.isValid() && iPoints.isValid() ) )
			return index;

		QList<QVector<quint16> > strips;

		for ( int r = 0; r < nif->rowCount( iPoints ); r++ )
			strips += nif->getArray<quint16>( iPoints.child( r, 0 ) );

		if ( strips.isEmpty() )
			return index;

		QVector<quint16> strip = strips.first();
		strips.pop_front();

		for ( const QVector<quint16>& s : strips ) {
			// TODO: optimize this
			if ( strip.count() & 1 )
				strip << strip.last() << s.first() << s.first() << s;
			else
				strip << strip.last() << s.first() << s;
		}

		nif->set<int>( iData, "Num Strips", 1 );
		nif->updateArray( iLength );
		nif->set<int>( iLength.child( 0, 0 ), strip.size() );
		nif->updateArray( iPoints );
		nif->updateArray( iPoints.child( 0, 0 ) );
		nif->setArray<quint16>( iPoints.child( 0, 0 ), strip );

		return index;
	}
};

REGISTER_SPELL( spStichStrips )


class spUnstichStrips final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Unstich Strips" ); }
	QString page() const override final { return Spell::tr( "Mesh" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		QModelIndex iData = spStichStrips::getStripsData( nif, index );
		return iData.isValid() && nif->get<int>( iData, "Num Strips" ) == 1;
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		QModelIndex iData = spStichStrips::getStripsData( nif, index );
		QModelIndex iLength = nif->getIndex( iData, "Strip Lengths" );
		QModelIndex iPoints = nif->getIndex( iData, "Points" );

		if ( !( iLength.isValid() && iPoints.isValid() ) )
			return index;

		QVector<quint16> strip = nif->getArray<quint16>( iPoints.child( 0, 0 ) );

		if ( strip.size() <= 3 )
			return index;

		QList<QVector<quint16> > strips;
		QVector<quint16> scratch;

		quint16 a = strip[0];
		quint16 b = strip[1];
		bool flip = false;

		for ( int s = 2; s < strip.size(); s++ ) {
			quint16 c = strip[s];

			if ( a != b && b != c && c != a ) {
				if ( scratch.isEmpty() ) {
					if ( flip )
						scratch << a << a << b;
					else
						scratch << a << b;
				}

				scratch << c;
			} else if ( !scratch.isEmpty() ) {
				strips << scratch;
				scratch.clear();
			}

			a = b;
			b = c;
			flip = !flip;
		}

		if ( !scratch.isEmpty() )
			strips << scratch;

		nif->set<int>( iData, "Num Strips", strips.size() );
		nif->updateArray( iLength );
		nif->updateArray( iPoints );

		for ( int r = 0; r < strips.count(); r++ ) {
			nif->set<int>( iLength.child( r, 0 ), strips[r].size() );
			nif->updateArray( iPoints.child( r, 0 ) );
			nif->setArray<quint16>( iPoints.child( r, 0 ), strips[r] );
		}

		return index;
	}
};

REGISTER_SPELL( spUnstichStrips )

