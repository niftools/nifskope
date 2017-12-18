#ifndef SP_BLOCKS_H
#define SP_BLOCKS_H

#include "spellbook.h"


// Brief description is deliberately not autolinked to class Spell
/*! \file blocks.h
 * \brief Block spells header
 *
 * All classes here inherit from the Spell class.
 */

typedef enum {
	// "nifskope"
	MIME_IDX_APP = 0,
	// "nibranch" or "niblock"
	MIME_IDX_STREAM,
	// "version"
	MIME_IDX_VER,
	// "type"
	MIME_IDX_TYPE
} CopyPasteMimeTypes;

//! Copy a branch (a block and its descendents) to the clipboard
class spCopyBranch final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Copy Branch" ); }
	QString page() const override final { return Spell::tr( "Block" ); }
	QKeySequence hotkey() const override final { return QKeySequence( QKeySequence::Copy ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final;
	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final;
};

//! Paste a branch from the clipboard
class spPasteBranch final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Paste Branch" ); }
	QString page() const override final { return Spell::tr( "Block" ); }
	// Doesn't work unless the menu entry is unique
	QKeySequence hotkey() const override final { return QKeySequence( QKeySequence::Paste ); }

	QString acceptFormat( const QString & format, const NifModel * nif );

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final;

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final;
};

class spDuplicateBranch final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Duplicate Branch" ); }
	QString page() const override final { return Spell::tr( "Block" ); }
	QKeySequence hotkey() const override final { return{ Qt::CTRL + Qt::Key_D }; }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final;

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final;
};

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


//! Add a link to the specified block to a link array
/*!
* @param nif The model
* @param iParent The block containing the link array
* @param array The name of the link array
* @param link A reference to the block to insert into the link array
*/
bool addLink( NifModel * nif, const QModelIndex & iParent, const QString & array, int link );

//! Remove a link to a block from the specified link array
/*!
* @param nif The model
* @param iParent The block containing the link array
* @param array The name of the link array
* @param link A reference to the block to remove from the link array
*/
void delLink( NifModel * nif, const QModelIndex & iParent, QString array, int link );

//! Link one block to another
/*!
 * @param nif The model
 * @param index The block to link to (becomes parent)
 * @param iBlock The block to link (becomes child)
 */
void blockLink( NifModel * nif, const QModelIndex & index, const QModelIndex & iBlock );

#endif // SP_BLOCKS_H
