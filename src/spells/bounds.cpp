#include "spellbook.h"

#include "ui/widgets/nifeditors.h"


// Brief description is deliberately not autolinked to class Spell
/*! \file bounds.cpp
 * \brief Bounding box editing spells (spEditBounds)
 *
 * All classes here inherit from the Spell class.
 */

//! Edit a bounding box
class spEditBounds final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Edit" ); }
	QString page() const override final { return Spell::tr( "Bounds" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		return (nif->itemName( index ) == "BSBound")
		       || (nif->itemName( index.parent() ) == "BSBound" )
		       || (nif->get<bool>( index, "Has Bounding Box" ) == true)
		       || (nif->itemName( index ) == "Bounding Box")
		       || (nif->itemName( index.parent() ) == "Bounding Box");
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		NifBlockEditor * edit = new NifBlockEditor( nif, nif->getBlock( index ) );

		if ( nif->get<bool>( index, "Has Bounding Box" ) == true || nif->itemName( index ) == "Bounding Box" || nif->itemName( index.parent() ) == "Bounding Box" ) {
			QModelIndex iBound;

			if ( nif->itemName( index ) == "Bounding Box" ) {
				iBound = index;
			} else if ( nif->itemName( index.parent() ) == "Bounding Box" ) {
				iBound = index.parent();
			} else {
				iBound = nif->getIndex( index, "Bounding Box" );
			}

			edit->add( new NifVectorEdit( nif, nif->getIndex( iBound, "Translation" ) ) );
			edit->add( new NifRotationEdit( nif, nif->getIndex( iBound, "Rotation" ) ) );
			edit->add( new NifVectorEdit( nif, nif->getIndex( iBound, "Radius" ) ) );
		} else if ( nif->itemName( index ) == "BSBound" || nif->itemName( index.parent() ) == "BSBound" ) {
			QModelIndex iBound;

			if ( nif->itemName( index ) == "BSBound" ) {
				iBound = index;
			} else if ( nif->itemName( index.parent() ) == "BSBound" ) {
				iBound = index.parent();
			}

			edit->add( new NifVectorEdit( nif, nif->getIndex( iBound, "Center" ) ) );
			edit->add( new NifVectorEdit( nif, nif->getIndex( iBound, "Dimensions" ) ) );
		}

		edit->show();
		return index;
	}
};

REGISTER_SPELL( spEditBounds )
