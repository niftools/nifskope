#ifndef TANGENTSPACE_H
#define TANGENTSPACE_H

#include "../spellbook.h"

class spTangentSpace : public Spell
{
public:
	QString name() const { return Spell::tr("Update Tangent Space"); }
	QString page() const { return Spell::tr("Mesh"); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index );
	QModelIndex cast( NifModel * nif, const QModelIndex & iBlock );
};


#endif
