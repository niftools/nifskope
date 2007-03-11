#ifndef SP_BLOCKS_H
#define SP_BLOCKS_H

#include "../spellbook.h"

class spRemoveBranch : public Spell
{
public:
	QString name() const { return "Remove Branch"; }
	QString page() const { return "Block"; }
	QKeySequence hotkey() const { return QKeySequence( Qt::CTRL + Qt::Key_Delete ); }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index );
	QModelIndex cast( NifModel * nif, const QModelIndex & index );
};

#endif
