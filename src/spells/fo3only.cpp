#include "spellbook.h"


// Brief description is deliberately not autolinked to class Spell
/*! \file fo3only.cpp
 * \brief Fallout 3 specific spells (spFO3FixShapeDataName)
 *
 * All classes here inherit from the Spell class.
 */

//! Set the name of the NiGeometryData node to parent name or zero
class spFO3FixShapeDataName final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Fix Geometry Data Names" ); }
	QString page() const override final { return Spell::tr( "Sanitize" ); }
	bool sanity() const override final { return true; }

	//////////////////////////////////////////////////////////////////////////
	// Valid if nothing or NiGeometryData-based node is selected
	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		//if ( !index.isValid() )
		//	return false;

		if ( !nif->checkVersion( 0x14020007, 0x14020007 ) || (nif->getUserVersion() != 11) )
			return false;

		return !index.isValid() || nif->getBlock( index, "NiGeometryData" ).isValid();
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		if ( index.isValid() && nif->getBlock( index, "NiGeometryData" ).isValid() ) {
			nif->set<int>( index, "Unknown ID", 0 );
		} else {
			// set all blocks
			for ( int n = 0; n < nif->getBlockCount(); n++ ) {
				QModelIndex iBlock = nif->getBlock( n );

				if ( nif->getBlock( iBlock, "NiGeometryData" ).isValid() ) {
					cast( nif, iBlock );
				}
			}
		}

		return index;
	}
};

REGISTER_SPELL( spFO3FixShapeDataName )

