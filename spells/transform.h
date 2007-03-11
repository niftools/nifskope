#ifndef SP_TRANSFORM_H
#define SP_TRANSFORM_H

#include "../spellbook.h"

class spApplyTransformation : public Spell
{
public:
	QString name() const { return "Apply"; }
	QString page() const { return "Transform"; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index );
	QModelIndex cast( NifModel * nif, const QModelIndex & index );
};

#endif
