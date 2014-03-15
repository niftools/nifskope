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
class spCollapseArray : public Spell
{
public:
	QString name() const { return Spell::tr( "Collapse" ); }
	QString page() const { return Spell::tr( "Array" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index );
	QModelIndex cast( NifModel * nif, const QModelIndex & index );
	// moved from sanitize.cpp
	QModelIndex numCollapser( NifModel * nif, QModelIndex & iNumElem, QModelIndex & iArray );
};

#endif // SP_MISC_H
