#ifndef SP_MISC_H
#define SP_MISC_H

#include "spellbook.h"


// Brief description is deliberately not autolinked to class Spell
/*! \file misc.h
 * \brief Miscellaneous spells header
 *
 * All classes here inherit from the Spell class.
 */

//! Removes empty links from a link array
class spCollapseArray final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Collapse" ); }
	QString page() const override final { return Spell::tr( "Array" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final;
	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final;
	// moved from sanitize.cpp
	QModelIndex numCollapser( NifModel * nif, QModelIndex & iNumElem, QModelIndex & iArray );
};

#endif // SP_MISC_H
