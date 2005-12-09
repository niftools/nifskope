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

#include "nifview.h"

#include "nifmodel.h"

NifTreeView::NifTreeView() : QTreeView()
{
	nif = 0;
	EvalConditions = false;
	
	setUniformRowHeights( true );
	setAlternatingRowColors( true );
	setContextMenuPolicy( Qt::CustomContextMenu );
}

NifTreeView::~NifTreeView()
{
}

void NifTreeView::setModel( QAbstractItemModel * model )
{
	if ( nif && EvalConditions )
		disconnect( nif, SIGNAL( dataChanged( const QModelIndex &, const QModelIndex & ) ), this, SLOT( updateConditions() ) );

	nif = qobject_cast<NifModel*>( model );
	
	QTreeView::setModel( model );
	
	if ( nif && EvalConditions )
		connect( nif, SIGNAL( dataChanged( const QModelIndex &, const QModelIndex & ) ), this, SLOT( updateConditions() ) );
}

void NifTreeView::setCurrentIndexExpanded( const QModelIndex & index )
{
	if ( ! index.isValid() )
		setCurrentIndex( index );
	
	if ( index.model() != model() )	return;
	
	QModelIndex idx = index;
	while ( idx.isValid() )
	{
		expand( idx );
		idx = idx.parent();
	}
	
	setCurrentIndex( index );
}

void NifTreeView::setEvalConditions( bool c )
{
	if ( EvalConditions == c )
		return;
	
	EvalConditions = c;
	
	if ( nif )
	{
		if ( EvalConditions )
			connect( nif, SIGNAL( dataChanged( const QModelIndex &, const QModelIndex & ) ), this, SLOT( updateConditions() ) );
		else
			disconnect( nif, SIGNAL( dataChanged( const QModelIndex &, const QModelIndex & ) ), this, SLOT( updateConditions() ) );
	}
	
	doItemsLayout();
}

bool NifTreeView::isRowHidden( int r, const QModelIndex & index ) const
{
	return ( EvalConditions && index.isValid() && nif && index.model() == nif && !nif->evalCondition( index.child( r, 0 ) ) );
}

void NifTreeView::setAllExpanded( const QModelIndex & index, bool e )
{
	if ( ! ( model() && index.model() == model() ) )	return;
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

void NifTreeView::updateConditions()
{
	if ( EvalConditions )
		doItemsLayout();
}

