#ifndef SP_TRANSFORM_H
#define SP_TRANSFORM_H

#include "spellbook.h"

class spApplyTransformation final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Apply" ); }
	QString page() const override final { return Spell::tr( "Transform" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final;
	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final;
};

#endif
