/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005-2017, NIF File Format Library and Tools
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

#include "undocommands.h"

#include "nifmodel.h"


/*
 *  ChangeValueCommand
 */

ChangeValueCommand::ChangeValueCommand( const QModelIndex & index,
	const QVariant & value, const QString & valueString, const QString & valueType, NifModel * model )
	: QUndoCommand(), nif( model ), idx( index )
{
	oldValue = index.data( Qt::EditRole );
	newValue = value;

	auto oldTxt = index.data( Qt::DisplayRole ).toString();
	auto newTxt = valueString;

	if ( !newTxt.isEmpty() )
		setText( QApplication::translate( "ChangeValueCommand", "Set %1 to %2" ).arg( valueType ).arg( newTxt ) );
	else
		setText( QApplication::translate( "ChangeValueCommand", "Modify %1" ).arg( valueType ) );
}

ChangeValueCommand::ChangeValueCommand( const QModelIndex & index, const NifValue & oldVal, 
										const NifValue & newVal, const QString & valueType, NifModel * model )
	: QUndoCommand(), nif( model ), idx( index )
{
	oldValue = oldVal.toVariant();
	newValue = newVal.toVariant();

	auto oldTxt = oldVal.toString();
	auto newTxt = newVal.toString();

	if ( !newTxt.isEmpty() )
		setText( QApplication::translate( "ChangeValueCommand", "Set %1 to %2" ).arg( valueType ).arg( newTxt ) );
	else
		setText( QApplication::translate( "ChangeValueCommand", "Modify %1" ).arg( valueType ) );
}

void ChangeValueCommand::redo()
{
	//qDebug() << "Redoing";
	nif->setData( idx, newValue, Qt::EditRole );

	//qDebug() << nif->data( idx ).toString();
}

void ChangeValueCommand::undo()
{
	//qDebug() << "Undoing";
	nif->setData( idx, oldValue, Qt::EditRole );

	//qDebug() << nif->data( idx ).toString();
}


/*
 *  ToggleCheckBoxListCommand
 */

ToggleCheckBoxListCommand::ToggleCheckBoxListCommand( const QModelIndex & index,
	const QVariant & value, const QString & valueType, NifModel * model )
	: QUndoCommand(), nif( model ), idx( index )
{
	oldValue = index.data( Qt::EditRole );
	newValue = value;

	auto oldTxt = index.data( Qt::DisplayRole ).toString();

	setText( QApplication::translate( "ToggleCheckBoxListCommand", "Modify %1" ).arg( valueType ) );
}

void ToggleCheckBoxListCommand::redo()
{
	//qDebug() << "Redoing";
	nif->setData( idx, newValue, Qt::EditRole );

	//qDebug() << nif->data( idx ).toString();
}

void ToggleCheckBoxListCommand::undo()
{
	//qDebug() << "Undoing";
	nif->setData( idx, oldValue, Qt::EditRole );

	//qDebug() << nif->data( idx ).toString();
}
