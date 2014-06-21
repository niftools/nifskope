#ifndef SP_BLOCKS_H
#define SP_BLOCKS_H

#include "spellbook.h"


// Brief description is deliberately not autolinked to class Spell
/*! \file blocks.h
 * \brief Block spells header
 *
 * All classes here inherit from the Spell class.
 */

//! Remove a branch (a block and its descendents)
class spRemoveBranch final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Remove Branch" ); }
	QString page() const override final { return Spell::tr( "Block" ); }
	QKeySequence hotkey() const override final { return{ Qt::CTRL + Qt::Key_Delete }; }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final;
	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final;
};

//! Link one block to another
/*!
 * @param nif The model
 * @param index The block to link to (becomes parent)
 * @param iBlock The block to link (becomes child)
 */
void blockLink( NifModel * nif, const QModelIndex & index, const QModelIndex & iBlock );

#endif // SP_BLOCKS_H
