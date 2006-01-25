/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005, NIF File Format Library and Tools
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the NIF File Format Library and Tools projectmay not be
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

#include <QDebug>

QList<Spell*> SpellBook::spells;
QList<SpellBook*> SpellBook::books;

SpellBook::SpellBook( NifModel * nif, const QModelIndex & index, QObject * receiver, const char * member ) : QMenu(), Nif( 0 )
{
	setTitle( "Spells" );
	
	// register this book in the library
	books.append( this );

	// attach this book to the specified nif
	sltNif( nif );
	
	// fill in the known spells
	foreach ( Spell * spell, spells )
		newSpellRegistered( spell );
	
	// set the current index
	sltIndex( index );

	connect( this, SIGNAL( triggered( QAction * ) ), this, SLOT( sltSpellTriggered( QAction * ) ) );
	
	if ( receiver && member )
		connect( this, SIGNAL( sigIndex( const QModelIndex & ) ), receiver, member );
}

SpellBook::~SpellBook()
{
	books.removeAll( this );
}

void SpellBook::sltSpellTriggered( QAction * action )
{
	Spell * spell = Map.value( action );
	if ( spell && spell->isApplicable( Nif, Index ) )
		emit sigIndex( spell->cast( Nif, Index ) );
}

void SpellBook::sltNif( NifModel * nif )
{
	Nif = nif;
	Index = QModelIndex();
}

void SpellBook::sltIndex( const QModelIndex & index )
{
	if ( index.model() == Nif )
		Index = index;
	else
		Index = QModelIndex();
	checkActions( index, this, QString() );
}

void SpellBook::checkActions( const QModelIndex & index, QMenu * menu, const QString & page )
{
	bool menuEnable = false;
	foreach ( QAction * action, menu->actions() )
	{
		if ( action->menu() )
		{
			checkActions( index, action->menu(), action->menu()->title() );
			menuEnable |= action->menu()->isEnabled();
		}
		else
		{
			foreach ( Spell * spell, spells )
			{
				if ( action->text() == spell->name() && page == spell->page() )
				{
					bool actionEnable = Nif && spell->isApplicable( Nif, index );
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
	if ( spell->page().isEmpty() )
	{
		Map.insert( addAction( spell->name() ), spell );
	}
	else
	{
		QMenu * menu = 0;
		foreach ( QAction * action, actions() )
		{
			if ( action->menu() && action->menu()->title() == spell->page() )
			{
				menu = action->menu();
				break;
			}
		}
		if ( ! menu )
		{
			menu = new QMenu( spell->page() );
			addMenu( menu );
		}
		Map.insert( menu->addAction( spell->name() ), spell );
	}
}

void SpellBook::registerSpell( Spell * spell )
{
	spells.append( spell );
	foreach ( SpellBook * book, books )
	{
		book->newSpellRegistered( spell );
	}
}

#include "spells.cpp"
#include "spellstrip.cpp"

