/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005-2015, NIF File Format Library and Tools
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

#include "spellbook.h"

#include "ui/checkablemessagebox.h"

#include <QCache>
#include <QDir>
#include <QSettings>


//! \file spellbook.cpp SpellBook implementation

QList<Spell *> & SpellBook::spells()
{
	// construct-on-first-use wrapper
	static QList<Spell *> * _spells = new QList<Spell *>();
	return *_spells;
}

QList<SpellBook *> & SpellBook::books()
{
	static QList<SpellBook *> * _books = new QList<SpellBook *>();
	return *_books;
}

QMultiHash<QString, Spell *> & SpellBook::hash()
{
	static QMultiHash<QString, Spell *> * _hash = new QMultiHash<QString, Spell *>();
	return *_hash;
}

QList<Spell *> & SpellBook::instants()
{
	static QList<Spell *> * _instants = new QList<Spell *>();
	return *_instants;
}

QList<Spell *> & SpellBook::sanitizers()
{
	static QList<Spell *> * _sanitizers = new QList<Spell *>();
	return *_sanitizers;
}

SpellBook::SpellBook( NifModel * nif, const QModelIndex & index, QObject * receiver, const char * member ) : QMenu(), Nif( 0 )
{
	setTitle( "Spells" );

	// register this book in the library
	books().append( this );

	// attach this book to the specified nif
	sltNif( nif );

	// fill in the known spells
	for ( Spell * spell : spells() ) {
		newSpellRegistered( spell );
	}

	// set the current index
	sltIndex( index );

	connect( this, &SpellBook::triggered, this, &SpellBook::sltSpellTriggered );

	if ( receiver && member )
		connect( this, SIGNAL( sigIndex( const QModelIndex & ) ), receiver, member );
}

SpellBook::~SpellBook()
{
	books().removeAll( this );
}

void SpellBook::cast( NifModel * nif, const QModelIndex & index, Spell * spell )
{
	QSettings cfg;

	bool suppressConfirm = cfg.value( "Settings/Suppress Undoable Confirmation", false ).toBool();
	bool accepted = false;

	QDialogButtonBox::StandardButton response = QDialogButtonBox::Yes;

	if ( !suppressConfirm ) {
		response = CheckableMessageBox::question( this, "Confirmation", "This action cannot currently be undone. Do you want to continue?", "Do not ask me again", &accepted );

		if ( accepted )
			cfg.setValue( "Settings/Suppress Undoable Confirmation", true );
	}
	
	if ( (response == QDialogButtonBox::Yes) && spell && spell->isApplicable( nif, index ) ) {
		emit sigIndex( spell->cast( nif, index ) );
	}
}

void SpellBook::sltSpellTriggered( QAction * action )
{
	Spell * spell = Map.value( action );
	cast( Nif, Index, spell );
}

void SpellBook::sltNif( NifModel * nif )
{
	if ( Nif )
		disconnect( Nif, &NifModel::modelReset, this, static_cast<void (SpellBook::*)()>(&SpellBook::checkActions) );

	Nif = nif;
	Index = QModelIndex();

	if ( Nif )
		connect( Nif, &NifModel::modelReset, this, static_cast<void (SpellBook::*)()>(&SpellBook::checkActions) );
}

void SpellBook::sltIndex( const QModelIndex & index )
{
	if ( index.model() == Nif )
		Index = index;
	else
		Index = QModelIndex();

	checkActions();
}

void SpellBook::checkActions()
{
	checkActions( this, QString() );
}

void SpellBook::checkActions( QMenu * menu, const QString & page )
{
	bool menuEnable = false;
	for ( QAction * action : menu->actions() ) {
		if ( action->menu() ) {
			checkActions( action->menu(), action->menu()->title() );
			menuEnable |= action->menu()->isEnabled();
			action->setVisible( action->menu()->isEnabled() );
		} else {
			for ( Spell * spell : spells() ) {
				if ( action->text() == spell->name() && page == spell->page() ) {
					bool actionEnable = Nif && spell->isApplicable( Nif, Index );
					action->setVisible( actionEnable );
					action->setEnabled( actionEnable );
					menuEnable |= actionEnable;
				}
			}
		}
	}
	menu->setEnabled( menuEnable );
}

void SpellBook::newSpellRegistered( Spell * spell )
{
	if ( spell->page().isEmpty() ) {
		Map.insert( addAction( spell->icon(), spell->name() ), spell );
	} else {
		QMenu * menu = nullptr;
		for ( QAction * action : actions() ) {
			if ( action->menu() && action->menu()->title() == spell->page() ) {
				menu = action->menu();
				break;
			}
		}

		if ( !menu ) {
			menu = new QMenu( spell->page() );
			addMenu( menu );
		}

		QAction * act = menu->addAction( spell->icon(), spell->name() );
		act->setShortcut( spell->hotkey() );
		Map.insert( act, spell );
	}
}

void SpellBook::registerSpell( Spell * spell )
{
	spells().append( spell );
	hash().insertMulti( spell->name(), spell );

	if ( spell->instant() )
		instants().append( spell );

	if ( spell->sanity() )
		sanitizers().append( spell );

	for ( SpellBook * book : books() ) {
		book->newSpellRegistered( spell );
	}
}

Spell * SpellBook::lookup( const QString & id )
{
	if ( id.isEmpty() )
		return nullptr;

	QString page;
	QString name = id;

	if ( id.contains( "/" ) ) {
		QStringList split = id.split( "/" );
		page = split.value( 0 );
		name = split.value( 1 );
	}

	for ( Spell * spell : hash().values( name ) ) {
		if ( spell->page() == page )
			return spell;
	}

	return nullptr;
}

Spell * SpellBook::lookup( const QKeySequence & hotkey )
{
	if ( hotkey.isEmpty() )
		return nullptr;

	for ( Spell * spell : spells() ) {
		if ( spell->hotkey() == hotkey )
			return spell;
	}

	return nullptr;
}

Spell * SpellBook::instant( const NifModel * nif, const QModelIndex & index )
{
	for ( Spell * spell : instants() ) {
		if ( spell->isApplicable( nif, index ) )
			return spell;
	}
	return nullptr;
}

QModelIndex SpellBook::sanitize( NifModel * nif )
{
	QPersistentModelIndex ridx;

	for ( Spell * spell : sanitizers() ) {
		if ( spell->isApplicable( nif, QModelIndex() ) ) {
			QModelIndex idx = spell->cast( nif, QModelIndex() );

			if ( idx.isValid() && !ridx.isValid() )
				ridx = idx;
		}
	}

	return ridx;
}

QAction * SpellBook::exec( const QPoint & pos, QAction * act )
{
	if ( isEnabled() )
		return QMenu::exec( pos, act );

	return nullptr;
}


QList<BatchProperty> Spell::batchProperties() const
{
     QList<BatchProperty> properties;
     properties.append(BatchProperty("Block:", 0));
     return properties;
}
