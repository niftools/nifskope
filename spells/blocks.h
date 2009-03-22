#ifndef SP_BLOCKS_H
#define SP_BLOCKS_H

#include "../spellbook.h"

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

#endif
