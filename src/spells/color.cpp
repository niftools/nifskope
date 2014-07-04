#include "spellbook.h"

#include "widgets/colorwheel.h"


// Brief description is deliberately not autolinked to class Spell
/*! \file color.cpp
 * \brief Color editing spells (spChooseColor)
 *
 * All classes here inherit from the Spell class.
 */

//! Choose a color using a ColorWheel
class spChooseColor final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Choose" ); }
	QString page() const override final { return Spell::tr( "Color" ); }
	QIcon icon() const { return ColorWheel::getIcon(); }
	bool instant() const { return true; }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		return nif->getValue( index ).isColor();
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		if ( nif->getValue( index ).type() == NifValue::tColor3 )
			nif->set<Color3>( index, ColorWheel::choose( nif->get<Color3>( index ) ) );
		else if ( nif->getValue( index ).type() == NifValue::tColor4 )
			nif->set<Color4>( index, ColorWheel::choose( nif->get<Color4>( index ) ) );

		return index;
	}
};

REGISTER_SPELL( spChooseColor )

//! Set an array of Colors
class spSetAllColor final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Set All" ); }
	QString page() const override final { return Spell::tr( "Color" ); }
	QIcon icon() const { return ColorWheel::getIcon(); }
	bool instant() const { return true; }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		return nif->isArray( index ) && nif->getValue( index.child( 0, 0 ) ).isColor();
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		QModelIndex colorIdx = (nif->isArray( index )) ? index.child( 0, 0 ) : index;

		if ( nif->getValue( colorIdx ).type() == NifValue::tColor3 )
			nif->setArray<Color3>( index, ColorWheel::choose( nif->get<Color3>( colorIdx ) ) );
		else if ( nif->getValue( colorIdx ).type() == NifValue::tColor4 )
			nif->setArray<Color4>( index, ColorWheel::choose( nif->get<Color4>( colorIdx ) ) );

		return index;
	}
};

REGISTER_SPELL( spSetAllColor )
