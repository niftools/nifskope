#include "../spellbook.h"

#include <QDebug>

class spMorphFrameSave : public Spell
{
public:
	QString name() const { return "Save Vertices To Frame"; }
	QString page() const { return "Morph"; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return checkMorpher( nif, index ) && listFrames( nif, index ).count() > 0;
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		QModelIndex iMeshData = getMeshData( nif, index );
		QModelIndex iMorphData = getMorphData( nif, index );
		
		QMenu menu;
		QStringList frameList = listFrames( nif, index );
		
		if ( nif->get<int>( iMeshData, "Num Vertices" ) != nif->get<int>( iMorphData, "Num Vertices" ) )
			menu.addAction( frameList.first() );
		else
			foreach ( QString f, frameList )
				menu.addAction( f );
		
		QAction * act = menu.exec( QCursor::pos() );
		if ( act )
		{
			QModelIndex iFrames = getFrameArray( nif, index );
			int selFrame = frameList.indexOf( act->text() );
			if ( selFrame == 0 )
			{
				qWarning() << "overriding base key frame, all other frames will be cleared";
				nif->set<int>( iMorphData, "Num Vertices", nif->get<int>( iMeshData, "Num Vertices" ) );
				QVector<Vector3> verts = nif->getArray<Vector3>( iMeshData, "Vertices" );
				nif->updateArray( iFrames.child( 0, 0 ), "Vectors" );
				nif->setArray( iFrames.child( 0, 0 ), "Vectors", verts );
				verts.fill( Vector3() );
				for ( int f = 1; f < nif->rowCount( iFrames ); f++ )
				{
					nif->updateArray( iFrames.child( f, 0 ), "Vectors" );
					nif->setArray<Vector3>( iFrames.child( f, 0 ), "Vectors", verts );
				}
			}
			else
			{
				QVector<Vector3> verts = nif->getArray<Vector3>( iMeshData, "Vertices" );
				QVector<Vector3> base = nif->getArray<Vector3>( iFrames.child( 0, 0 ), "Vectors" );
				QVector<Vector3> frame( base.count(), Vector3() );
				for ( int n = 0; n < base.count(); n++ )
					frame[ n ] = verts.value( n ) - base[ n ];
				nif->setArray<Vector3>( iFrames.child( selFrame, 0 ), "Vectors", frame );
			}
		}
		
		return index;
	}

	bool checkMorpher( const NifModel * nif, const QModelIndex & index )
	{
		return nif->isNiBlock( index, "NiGeomMorpherController" ) && nif->checkVersion( 0x0a010000, 0 ) && getMeshData( nif, index ).isValid();
	}
	
	QModelIndex getMeshData( const NifModel * nif, const QModelIndex & iMorpher )
	{
		QModelIndex iMesh = nif->getBlock( nif->getParent( nif->getBlockNumber( iMorpher ) ) );
		if ( nif->inherits( iMesh, "NiTriBasedGeom" ) )
		{
			QModelIndex iData = nif->getBlock( nif->getLink( iMesh, "Data" ) );
			if ( nif->inherits( iData, "NiTriBasedGeomData" ) )
				return iData;
			else
				return QModelIndex();
		}
		else
			return QModelIndex();
	}
	
	QModelIndex getMorphData( const NifModel * nif, const QModelIndex & iMorpher )
	{
		return nif->getBlock( nif->getLink( iMorpher, "Data" ), "NiMorphData" );
	}
	
	QModelIndex getFrameArray( const NifModel * nif, const QModelIndex & iMorpher )
	{
		return nif->getIndex( getMorphData( nif, iMorpher ), "Morphs" );
	}
	
	QStringList listFrames( const NifModel * nif, const QModelIndex & iMorpher )
	{
		QModelIndex iFrames = getFrameArray( nif, iMorpher );
		if ( iFrames.isValid() )
		{
			QStringList list;
			for ( int i = 0; i < nif->rowCount( iFrames ); i++ )
			{
				list << nif->get<QString>( iFrames.child( i, 0 ), "Frame Name" );
			}
			return list;
		}
		else
			return QStringList();
	}
	
};

REGISTER_SPELL( spMorphFrameSave )
