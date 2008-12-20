#include "../spellbook.h"

#include <QDebug>

// Spell to set the name of the NiGeometryData node to parent name or zero
class spFO3FixShapeDataName : public Spell
{
public:
	QString name() const { return tr("Fix Geometry Data Names"); }
	QString page() const { return tr("Sanitize"); }
	bool sanity() const { return true; }
	
	//////////////////////////////////////////////////////////////////////////
	// Valid if nothing or NiGeometryData-based node is selected
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		//if ( !index.isValid() )
		//	return false;

		if ( !nif->checkVersion( 0x14020007, 0x14020007 ) || (nif->getUserVersion() != 11) )
			return false;

		return !index.isValid() 
			|| nif->getBlock( index, "NiGeometryData" ).isValid()
			|| nif->getBlock( index, "NiGeometry" ).isValid()
			;
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		if ( index.isValid() && 
			   ( nif->getBlock( index, "NiGeometryData" ).isValid() 
			   || nif->getBlock( index, "NiGeometry" ).isValid() 
				)
			)
		{
			QString name = nif->get<QString>(index, "Name");
			if (name.isNull() || name.isEmpty())
			{
				int id = nif->getBlockNumber(index);
				int parentid = nif->getParent(id);
				QModelIndex parent = nif->getBlock( parentid );
				name = nif->get<QString>(parent, "Name");

				if (name.isNull() || name.isEmpty()) {
					nif->set<int>(index, "Name", 0);
				} else {
					nif->set<QString>(index, "Name", name);
				}
			}
		}
		else
		{
			// set all blocks
			for ( int n = 0; n < nif->getBlockCount(); n++ ) {
				QModelIndex iBlock = nif->getBlock( n );
				if ( nif->getBlock( iBlock, "NiGeometryData" ).isValid() 
					|| nif->getBlock( iBlock, "NiGeometry" ).isValid() 
					) 
				{
					cast(nif, iBlock);
				}
			}
		}
		return index;
	}
};

REGISTER_SPELL( spFO3FixShapeDataName )
