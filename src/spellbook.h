/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005-2012, NIF File Format Library and Tools
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the NIF File Format Library and Tools project may not be
   used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

***** END LICENCE BLOCK *****/

#ifndef SPELLBOOK_H
#define SPELLBOOK_H

#include "nifmodel.h"

#include <QCoreApplication>
#include <QMenu>

//! \file spellbook.h Spell, SpellBook and Librarian

//! Register a Spell using a Librarian
#define REGISTER_SPELL( SPELL ) static Librarian __##SPELL##__ ( new SPELL );

//! Flexible context menu magic functions.
class Spell
{
public:
	//! Constructor
	Spell() {}
	//! Destructor
	virtual ~Spell() {}
	
	//! Name of spell
	virtual QString name() const = 0;
	//! Context sub-menu that the spell appears on
	virtual QString page() const { return QString(); }
	//! Unused?
	virtual QString hint() const { return QString(); }
	//! Icon displayed in block view
	virtual QIcon icon() const { return QIcon(); }
	//! Whether the spell shows up in block list instead of a context menu
	virtual bool instant() const { return false; }
	//! Whether the spell performs a sanitizing function
	virtual bool sanity() const { return false; }
	//! Hotkey sequence
	virtual QKeySequence hotkey() const { return QKeySequence(); }

	//! Determine if/when the spell can be cast
	virtual bool isApplicable( const NifModel * nif, const QModelIndex & index ) = 0;
	
	//! Cast (apply) the spell
	virtual QModelIndex cast( NifModel * nif, const QModelIndex & index ) = 0;

	//! Cast the spell if applicable
	void castIfApplicable( NifModel * nif, const QModelIndex & index )
	{
		if ( isApplicable( nif, index ) )
			cast( nif, index );
	}
	
	//! i18n wrapper for various strings
	/*!
	 * Note that we don't use QObject::tr() because that doesn't provide
	 * context. We also don't use the QCoreApplication Q_DECLARE_TR_FUNCTIONS()
	 * macro because that won't document properly.
	 *
	 * No spells should reimplement this function.
	 */
	static inline QString tr( const char * key, const char * comment = 0 ) { return QCoreApplication::translate( "Spell", key, comment ); }
};

//! Spell menu
class SpellBook : public QMenu
{
	Q_OBJECT
public:
	//! Constructor
	SpellBook( NifModel * nif, const QModelIndex & index = QModelIndex(), QObject * receiver = 0, const char * member = 0 );
	//! Destructor
	~SpellBook();
	
	//! From QMenu: Pops up the menu so that the action <i>act</i> will be at the specified global position <i>pos</i>
	QAction * exec( const QPoint & pos, QAction * act = 0 );
	
	//! Register spell with appropriate books
	static void registerSpell( Spell * spell );
	
	//! Locate spell by name
	static Spell * lookup( const QString & id );
	//! Locate spell by hotkey
	static Spell * lookup( const QKeySequence & hotkey );
	//! Locate instant spells by datatype
	static Spell * instant( const NifModel * nif, const QModelIndex & index );
	
	//! Cast all sanitizing spells
	static QModelIndex sanitize( NifModel * nif );
	
public slots:
	void sltNif( NifModel * nif );
	
	void sltIndex( const QModelIndex & index );
	
	void cast( NifModel * nif, const QModelIndex & index, Spell * spell );
	
	void checkActions();
	
signals:
	void sigIndex( const QModelIndex & index );

protected slots:
	void sltSpellTriggered( QAction * action );

protected:
	NifModel * Nif;
	QPersistentModelIndex Index;
	QMap<QAction*,Spell*> Map;
	
	void newSpellRegistered( Spell * spell );
	void checkActions( QMenu * menu, const QString & page );
	
private:
	static QList<Spell*> & spells();
	static QList<SpellBook*> & books();
	static QMultiHash<QString, Spell*> & hash();
	static QList<Spell*> & instants();
	static QList<Spell*> & sanitizers();
};

//! SpellBook manager
class Librarian
{
public:
	//! Contructor.
	/**
	 * Registers the spell with the appropriate spellbooks.
	 *
	 * \param spell The spell to manage
	 */
	Librarian( Spell * spell )
	{
		SpellBook::registerSpell( spell );
	}
};

#endif
