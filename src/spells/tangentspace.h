#ifndef TANGENTSPACE_H
#define TANGENTSPACE_H

#include "spellbook.h"


//! Calculates tangents and bitangents
/*!
 * Much fun reading on this can be found at
 * http://en.wikipedia.org/wiki/Frenet%E2%80%93Serret_formulas
 */
class spTangentSpace final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Update Tangent Space" ); }
	QString page() const override final { return Spell::tr( "Mesh" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final;
	QModelIndex cast( NifModel * nif, const QModelIndex & iBlock ) override final;
};


#endif
