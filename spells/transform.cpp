#include "../spellbook.h"

#include "../glscene.h"

class spApplyTransformation : public Spell
{
public:
	QString name() const { return "Apply"; }
	QString page() const { return "Transform"; }
	
	bool isApplicable( NifModel * nif, const QModelIndex & index )
	{
		return ( nif->itemType( index ) == "NiBlock" && nif->getLink( index, "Controller" ) == -1 && nif->getLink( index, "Skin Instance" ) == -1 &&
			( ( nif->inherits( nif->itemName( index ), "AParentNode" ) && nif->get<int>( index, "Flags" ) & 8 )
				|| nif->itemName( index ) == "NiTriShape" || nif->itemName( index ) == "NiTriStrips" ) );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		if ( nif->inherits( nif->itemName( index ), "AParentNode" ) )
		{
			Transform tp( nif, index );
			bool ok = false;
			foreach ( int l, nif->getChildLinks( nif->getBlockNumber( index ) ) )
			{
				QModelIndex iChild = nif->getBlock( l );
				if ( iChild.isValid() && nif->inherits( nif->itemName( iChild ), "ANode" ) )
				{
					Transform tc( nif, iChild );
					tc = tp * tc;
					tc.writeBack( nif, iChild );
					ok = true;
				}
			}
			if ( ok )
			{
				tp = Transform();
				tp.writeBack( nif, index );
			}
		}
		else
		{
			QModelIndex iData;
			if ( nif->itemName( index ) == "NiTriShape") 
				iData = nif->getBlock( nif->getLink( index, "Data" ), "NiTriShapeData" );
			else if ( nif->itemName( index ) == "NiTriStrips" ) 
				iData = nif->getBlock( nif->getLink( index, "Data" ), "NiTriStripsData" );
			
			if ( iData.isValid() )
			{
				Transform t( nif, index );
				QModelIndex iVertices = nif->getIndex( iData, "Vertices" );
				if ( iVertices.isValid() )
				{
					for ( int v = 0; v < nif->rowCount( iVertices ); v++ )
						nif->setItemData<Vector3>( iVertices.child( v, 0 ), t * nif->itemData<Vector3>( iVertices.child( v, 0 ) ) );
					
					QModelIndex iNormals = nif->getIndex( iData, "Normals" );
					if ( iNormals.isValid() )
					{
						for ( int n = 0; n < nif->rowCount( iNormals ); n++ )
							nif->setItemData<Vector3>( iNormals.child( n, 0 ), t.rotation * nif->itemData<Vector3>( iNormals.child( n, 0 ) ) );
					}
				}
				QModelIndex iCenter = nif->getIndex( iData, "Center" );
				if ( iCenter.isValid() )
					nif->setItemData<Vector3>( iCenter, t.rotation * nif->itemData<Vector3>( iCenter ) + t.translation );
				QModelIndex iRadius = nif->getIndex( iData, "Radius" );
				if ( iRadius.isValid() )
					nif->setItemData<float>( iRadius, t.scale * nif->itemData<float>( iRadius ) );
				t = Transform();
				t.writeBack( nif, index );
			}
		}
		return index;
	}
};

REGISTER_SPELL( spApplyTransformation )

class spClearTransformation : public Spell
{
public:
	QString name() const { return "Clear"; }
	QString page() const { return "Transform"; }
	
	bool isApplicable( NifModel * nif, const QModelIndex & index )
	{
		return ( nif->itemType( index ) == "NiBlock" && nif->inherits( nif->itemName( index ), "ANode" ) );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		Transform tp;
		tp.writeBack( nif, index );
		return index;
	}
};

REGISTER_SPELL( spClearTransformation )
