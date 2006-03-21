#include "../spellbook.h"

#include "../widgets/colorwheel.h"

class spChooseColor : public Spell
{
public:
	QString name() const { return "Choose"; }
	QString page() const { return "Color"; }
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


