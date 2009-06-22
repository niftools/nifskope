#ifndef SP_BLOCKS_H
#define SP_BLOCKS_H

#include "../spellbook.h"

// Brief description is deliberately not autolinked to class Spell
/*! \file blocks.h
 * \brief Block spells header
 *
 * All classes here inherit from the Spell class.
 */

//! Remove a branch (a block and its descendents)
class spRemoveBranch : public Spell
{
public:
	QString name() const { return Spell::tr("Remove Branch"); }
	QString page() const { return Spell::tr("Block"); }
	QKeySequence hotkey() const { return QKeySequence( Qt::CTRL + Qt::Key_Delete ); }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index );
	QModelIndex cast( NifModel * nif, const QModelIndex & index );
};

//! Link one block to another
/*!
 * @param nif The model
 * @param index The block to link to (becomes parent)
 * @param iBlock The block to link (becomes child)
 */
void blockLink( NifModel * nif, const QModelIndex & index, const QModelIndex & iBlock );

#endif
