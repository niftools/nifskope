#include "spellbook.h"

#include "widgets/colorwheel.h"


// Brief description is deliberately not autolinked to class Spell
/*! \file color.cpp
 * \brief Color editing spells (spChooseColor)
 *
 * All classes here inherit from the Spell class.
 */

//! Choose a color using a ColorWheel
class spChooseColor : public Spell
{
public:
	QString name() const { return Spell::tr("Choose"); }
	QString page() const { return Spell::tr("Color"); }
	QIcon icon() const { return ColorWheel::getIcon(); }
	bool instant() const { return true; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return nif->getValue( index ).isColor();
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		if ( nif->getValue( index ).type() == NifValue::tColor3 )
			nif->set<Color3>( index, ColorWheel::choose( nif->get<Color3>( index ) ) );
		else if ( nif->getValue( index ).type() == NifValue::tColor4 )
			nif->set<Color4>( index, ColorWheel::choose( nif->get<Color4>( index ) ) );
		return index;
	}
};

REGISTER_SPELL( spChooseColor )

