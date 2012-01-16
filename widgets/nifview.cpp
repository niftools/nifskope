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

#include "nifview.h"

#include "../basemodel.h"
#include "../nifproxy.h"
#include "../spellbook.h"

#include <QKeyEvent>

NifTreeView::NifTreeView() : QTreeView()
{
	nif = 0;
	EvalConditions = false;
	RealTimeEval = false;
	
	setUniformRowHeights( true );
	setAlternatingRowColors( true );
	setContextMenuPolicy( Qt::CustomContextMenu );
	setHorizontalScrollBarPolicy( Qt::ScrollBarAsNeeded );

	connect( this, SIGNAL( expanded( const QModelIndex & ) ), this, SLOT( scrollExpand( const QModelIndex & ) ) );
}

NifTreeView::~NifTreeView()
{
}

void NifTreeView::setModel( QAbstractItemModel * model )
{
	if ( nif )
		disconnect( nif, SIGNAL( dataChanged( const QModelIndex &, const QModelIndex & ) ), this, SLOT( updateConditions( const QModelIndex &, const QModelIndex & ) ) );

	nif = qobject_cast<BaseModel*>( model );
	
	QTreeView::setModel( model );
	
	if ( nif && EvalConditions && RealTimeEval )
		connect( nif, SIGNAL( dataChanged( const QModelIndex &, const QModelIndex & ) ), this, SLOT( updateConditions( const QModelIndex &, const QModelIndex & ) ) );
}

void NifTreeView::setRootIndex( const QModelIndex & index )
{
	QModelIndex root = index;
	if ( root.isValid() && root.column() != 0 )
		root = root.sibling( root.row(), 0 );
	QTreeView::setRootIndex( root );
}

void NifTreeView::clearRootIndex()
{
	setRootIndex( QModelIndex() );
}

void NifTreeView::setEvalConditions( bool c )
{
	if ( EvalConditions == c )
		return;
	
	EvalConditions = c;
	
	if ( nif && EvalConditions && RealTimeEval )
	{
		connect( nif, SIGNAL( dataChanged( const QModelIndex &, const QModelIndex & ) ), this, SLOT( updateConditions( const QModelIndex &, const QModelIndex & ) ) );
	}
	else
	{
		disconnect( nif, SIGNAL( dataChanged( const QModelIndex &, const QModelIndex & ) ), this, SLOT( updateConditions( const QModelIndex &, const QModelIndex & ) ) );
	}

	// refresh
	updateConditionRecurse( rootIndex() );
	doItemsLayout();
}

void NifTreeView::setRealTime( bool c )
{
	if ( RealTimeEval == c )
		return;

	RealTimeEval = c;

	if ( nif && EvalConditions && RealTimeEval )
	{
		connect( nif, SIGNAL( dataChanged( const QModelIndex &, const QModelIndex & ) ), this, SLOT( updateConditions( const QModelIndex &, const QModelIndex & ) ) );
	}
	else
	{
		disconnect( nif, SIGNAL( dataChanged( const QModelIndex &, const QModelIndex & ) ), this, SLOT( updateConditions( const QModelIndex &, const QModelIndex & ) ) );
	}

	updateConditionRecurse( rootIndex() );
	doItemsLayout();
}

bool NifTreeView::isRowHidden( int r, const QModelIndex & index ) const
{
	return ( EvalConditions && index.isValid() && nif && index.model() == nif && !nif->evalCondition( index.child( r, 0 ) ) );
}

void NifTreeView::setAllExpanded( const QModelIndex & index, bool e )
{
	if ( ! model() )	return;
	for ( int r = 0; r < model()->rowCount( index ); r++ )
	{
		QModelIndex child = model()->index( r, 0, index );
		if ( model()->hasChildren( child ) )
		{
			setExpanded( child, e );
			setAllExpanded( child, e );
		}
	}
}

QStyleOptionViewItem NifTreeView::viewOptions() const
{
	QStyleOptionViewItem opt = QTreeView::viewOptions();
	opt.showDecorationSelected = true;
	return opt;
}

void NifTreeView::drawBranches( QPainter * painter, const QRect & rect, const QModelIndex & index ) const
{
	if ( rootIsDecorated() )
		QTreeView::drawBranches( painter, rect, index );
}

void NifTreeView::updateConditions( const QModelIndex & topLeft, const QModelIndex & bottomRight )
{
	updateConditionRecurse( topLeft.parent() );
	doItemsLayout();
}

void NifTreeView::updateConditionRecurse( const QModelIndex & index )
{
	/*
	if ( model()->rowCount( index ) == 0 )
	{
		setRowHidden( index.row(), index.parent(), (EvalConditions && ! nif->evalVersion( index, true ) ) );
	}
	*/
	//else
	//{
	for ( int r = 0; r < model()->rowCount( index ); r++ )
	{
		QModelIndex child = model()->index( r, 0, index );
		updateConditionRecurse( child );
	}
	setRowHidden( index.row(), index.parent(), (EvalConditions && ! nif->evalVersion( index, false ) ) );
	//}
}

void NifTreeView::keyPressEvent( QKeyEvent * e )
{
	Spell * spell = SpellBook::lookup( QKeySequence( e->modifiers() + e->key() ) );
	
	if ( spell )
	{
		NifModel * nif = 0;
		NifProxyModel * proxy = 0;
		
		QPersistentModelIndex oldidx;
		
		if ( model()->inherits( "NifModel" ) )
		{
			nif = static_cast<NifModel *>( model() );
			oldidx = currentIndex();
		}
		else if ( model()->inherits( "NifProxyModel" ) )
		{
			proxy = static_cast<NifProxyModel*>( model() );
			nif = static_cast<NifModel *>( proxy->model() );
			oldidx = proxy->mapTo( currentIndex() );
		}
		
		if ( nif && spell->isApplicable( nif, oldidx ) )
		{
			selectionModel()->setCurrentIndex( QModelIndex(), QItemSelectionModel::Clear | QItemSelectionModel::Rows );

			QModelIndex newidx = spell->cast( nif, oldidx );
			if ( proxy )
				newidx = proxy->mapFrom( newidx, oldidx );

			// grab selection from the selection model as it tends to be more accurate
			if (!newidx.isValid())
			{
				if (oldidx.isValid())
					newidx = ( proxy ) ? proxy->mapFrom( oldidx, oldidx ) : ((QModelIndex)oldidx);
				else
					newidx = selectionModel()->currentIndex();
			}
			if (newidx.isValid())
			{
				selectionModel()->setCurrentIndex(newidx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
				scrollTo(newidx, EnsureVisible);
				emit clicked(newidx);
			}
			return;
		}
	}
	
	QTreeView::keyPressEvent( e );
}

void NifTreeView::currentChanged( const QModelIndex & current, const QModelIndex & last )
{
	QTreeView::currentChanged( current, last );
	if ( nif && EvalConditions )
	{
		updateConditionRecurse( rootIndex() );
	}
	emit sigCurrentIndexChanged( currentIndex() );
}

void NifTreeView::scrollExpand( const QModelIndex & index )
{
	// this is a compromise between scrolling to the top, and scrolling the last child to the bottom
	scrollTo( index, PositionAtCenter );
}
