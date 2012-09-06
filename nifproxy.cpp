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

#include "nifproxy.h"

#include "nifmodel.h"

#include <QVector>
#include <QDebug>

class NifProxyItem
{
public:
	NifProxyItem( int number, NifProxyItem * parent )
	{
		blockNumber = number;
		parentItem = parent;
	}
	~NifProxyItem()
	{
		qDeleteAll( childItems );
	}

	NifProxyItem * getLink( int link )
	{
		foreach ( NifProxyItem * item, childItems )
		{
			if ( item->block() == link )
				return item;
		}
		return 0;
	}
	
	int rowLink( int link )
	{
		int row = 0;
		foreach ( NifProxyItem * item, childItems )
		{
			if ( item->block() == link )
				return row;
			row++;
		}
		return -1;
	}
	
	NifProxyItem * addLink( int link )
	{
		NifProxyItem * child = getLink( link );
		if ( child )
		{
			return child;
		}
		else
		{
			child = new NifProxyItem( link, this );
			childItems.append( child );
			return child;
		}
	}
	
	void delLink( int link )
	{
		NifProxyItem * child = getLink( link );
		if ( child )
		{
			childItems.removeAll( child );
			delete child;
		}
	}
	
	NifProxyItem * parent() const
	{
		return parentItem;
	}
	
	NifProxyItem * child( int row )
	{
		return childItems.value( row );
	}
	
	int childCount()
	{
		return childItems.count();
	}
	
	void killChildren()
	{
		qDeleteAll( childItems );
		childItems.clear();
	}
	
	int row() const
	{
		if ( parentItem )
			return parentItem->childItems.indexOf( const_cast<NifProxyItem*>(this) );
		return 0;
	}
	
	inline int block() const
	{
		return blockNumber;
	}
	
	QList<int> parentBlocks() const
	{
		QList<int> parents;
		NifProxyItem * parent = parentItem;
		while ( parent && parent->parentItem )
		{
			parents.append( parent->blockNumber );
			parent = parent->parentItem;
		}
		return parents;
	}
	
	QList<int> childBlocks() const
	{
		QList<int> blocks;
		foreach ( NifProxyItem * item, childItems )
			blocks.append( item->block() );
		return blocks;
	}
	
	NifProxyItem * findItem( int b, bool scanParents = true )
	{
		if ( blockNumber == b )	return this;
		
		foreach ( NifProxyItem * child, childItems )
		{
			if ( child->blockNumber == b )
				return child;
		}
		
		foreach ( NifProxyItem * child, childItems )
		{
			if ( NifProxyItem * x = child->findItem( b, false ) )
				return x;
		}
		
		if ( parentItem && scanParents )
		{
			NifProxyItem * root = parentItem;
			while ( root && root->parentItem )
				root = root->parentItem;
			if ( NifProxyItem * x = root->findItem( b, false ) )
				return x;
		}
		
		return 0;
	}
	
	void findAllItems( int b, QList<NifProxyItem*> & list )
	{
		foreach ( NifProxyItem * item, childItems )
		{
			item->findAllItems( b, list );
		}
		if ( blockNumber == b )
			list.append( this );
	}
	
	int blockNumber;
	NifProxyItem * parentItem;
	QList<NifProxyItem*> childItems;
};

NifProxyModel::NifProxyModel( QObject * parent ) : QAbstractItemModel( parent )
{
	root = new NifProxyItem( -1, 0 );
	nif = 0;
}

NifProxyModel::~NifProxyModel()
{
	delete root;
}

QAbstractItemModel * NifProxyModel::model() const
{
	return nif;
}

void NifProxyModel::setModel( QAbstractItemModel * model )
{
    if ( nif )
	{
        disconnect(nif, SIGNAL(dataChanged(const QModelIndex &,const QModelIndex &)),
                   this, SLOT(xDataChanged(const QModelIndex &,const QModelIndex &)));
        disconnect(nif, SIGNAL(headerDataChanged(Qt::Orientation,int,int)),
                   this, SLOT(xHeaderDataChanged(Qt::Orientation,int,int)));
	disconnect( nif, SIGNAL( rowsAboutToBeRemoved( const QModelIndex &, int, int ) ), this, SLOT( xRowsAboutToBeRemoved( const QModelIndex &, int, int ) ) );
	disconnect( nif, SIGNAL( linksChanged() ),	this, SLOT( xLinksChanged() ) );
		
        disconnect(nif, SIGNAL(modelReset()), this, SLOT(reset()));
        disconnect(nif, SIGNAL(layoutChanged()), this, SIGNAL(layoutChanged()));
    }

	nif = qobject_cast<NifModel *>( model );

    if ( nif )
	{
        connect( nif, SIGNAL(dataChanged( const QModelIndex &, const QModelIndex & )),
                   this, SLOT(xDataChanged( const QModelIndex &, const QModelIndex & )));
        connect( nif, SIGNAL(headerDataChanged(Qt::Orientation,int,int)),
                   this, SLOT(xHeaderDataChanged(Qt::Orientation,int,int)));
	connect( nif, SIGNAL( linksChanged() ), this, SLOT( xLinksChanged() ) );
	connect( nif, SIGNAL( rowsAboutToBeRemoved( const QModelIndex &, int, int ) ), this, SLOT( xRowsAboutToBeRemoved( const QModelIndex &, int, int ) ) );
        connect( nif, SIGNAL(modelReset()), this, SLOT(reset()));
        connect( nif, SIGNAL(layoutChanged()), this, SIGNAL(layoutChanged()));
    }

	reset();
}

void NifProxyModel::reset()
{
	//qDebug() << "proxy reset";
	root->killChildren();
	updateRoot( true );
	QAbstractItemModel::reset();
}

void NifProxyModel::updateRoot( bool fast )
{
	if ( ! ( nif && nif->getBlockCount() > 0 ) )
	{
		if ( root->childCount() > 0 )
		{
			if ( ! fast ) beginRemoveRows( QModelIndex(), 0, root->childCount() - 1 );
			root->killChildren();
			if ( ! fast ) endRemoveRows();
		}
		return;
	}
	
	//qDebug() << "proxy update top level";

	foreach ( NifProxyItem * item, root->childItems )
	{
		if ( ! nif->getRootLinks().contains( item->block() ) )
		{
			int at = root->rowLink( item->block() );
			if ( ! fast ) beginRemoveRows( QModelIndex(), at, at );
			root->delLink( item->block() );
			if ( ! fast ) endRemoveRows();
		}
	}
	
	foreach ( int l, nif->getRootLinks() )
	{
		NifProxyItem * item = root->getLink( l );
		if ( ! item )
		{
			if ( ! fast )	beginInsertRows( QModelIndex(), root->childCount(), root->childCount() );
			item = root->addLink( l );
			if ( ! fast )	endInsertRows();
		}
		updateItem( item, fast );
	}
}

void NifProxyModel::updateItem( NifProxyItem * item, bool fast )
{
	QModelIndex index( createIndex( item->row(), 0, item ) );
	
	QList<int> parents( item->parentBlocks() );
	
	foreach( int l, item->childBlocks() )
	{
		if ( ! ( nif->getChildLinks( item->block() ).contains( l ) || nif->getParentLinks( item->block() ).contains( l ) ) )
		{
			int at = item->rowLink( l );
			if ( ! fast ) beginRemoveRows( index, at, at );
			item->delLink( l );
			if ( ! fast ) endRemoveRows();
		}
	}
	foreach ( int l, nif->getChildLinks(item->block()) )
	{
		NifProxyItem * child = item->getLink( l );
		if ( ! child )
		{
			int at = item->childCount();
			if ( ! fast )	beginInsertRows( index, at, at );
			child = item->addLink( l );
			if ( ! fast )	endInsertRows();
		}
		if ( ! parents.contains( child->block() ) )
		{
			updateItem( child, fast );
		}
		else
			qWarning() << tr("infinite recursing link construct detected") << item->block() << "->" << child->block();
	}
	foreach ( int l, nif->getParentLinks( item->block() ) )
	{
		if ( ! item->getLink( l ) )
		{
			int at = item->childCount();
			if ( ! fast )	beginInsertRows( index, at, at );
			item->addLink( l );
			if ( ! fast )	endInsertRows();
		}
	}
}

int NifProxyModel::rowCount( const QModelIndex & parent ) const
{
	NifProxyItem * parentItem;
	
	if ( ! ( parent.isValid() && parent.model() == this ) )
		parentItem = root;
	else
		parentItem = static_cast<NifProxyItem*>( parent.internalPointer() );
	
	return ( parentItem ? parentItem->childCount() : 0 );
}

QModelIndex NifProxyModel::index( int row, int column, const QModelIndex & parent ) const
{
	NifProxyItem * parentItem;
	
	if ( ! ( parent.isValid() && parent.model() == this ) )
		parentItem = root;
	else
		parentItem = static_cast<NifProxyItem*>( parent.internalPointer() );
	
	NifProxyItem * childItem = ( parentItem ? parentItem->child( row ) : 0 );
	if ( childItem )
		return createIndex( row, column, childItem );
	else
		return QModelIndex();
}

QModelIndex NifProxyModel::parent( const QModelIndex & child ) const
{
	if ( ! ( child.isValid() && child.model() == this ) )
		return QModelIndex();
	
	NifProxyItem * childItem = static_cast<NifProxyItem*>( child.internalPointer() );
	NifProxyItem * parentItem = childItem->parent();
	
	if ( parentItem == root || ! parentItem )
		return QModelIndex();
	return createIndex( parentItem->row(), 0, parentItem );	
}

QModelIndex NifProxyModel::mapTo( const QModelIndex & idx ) const
{
	if ( ! ( nif && idx.isValid() ) ) return QModelIndex();
	
	if ( idx.model() != this )
	{
		qDebug() << tr("NifProxyModel::mapTo() called with wrong model");
		return QModelIndex();
	}
	NifProxyItem * item = static_cast<NifProxyItem*>( idx.internalPointer() );
	if ( ! item )	return QModelIndex();
	QModelIndex nifidx = nif->getBlock( item->block() );
	if ( nifidx.isValid() ) nifidx = nifidx.sibling( nifidx.row(), ( idx.column() ? NifModel::ValueCol : NifModel::NameCol ) );
	return nifidx;
}

QModelIndex NifProxyModel::mapFrom( const QModelIndex & idx, const QModelIndex & ref ) const
{
	if ( ! ( nif && idx.isValid() ) ) return QModelIndex();
	if ( idx.model() != nif )
	{
		qDebug() << tr("NifProxyModel::mapFrom() called with wrong model");
		return QModelIndex();
	}
	int blockNumber = nif->getBlockNumber( idx );
	if ( blockNumber < 0 ) return QModelIndex();
	NifProxyItem * item = root;
	if ( ref.isValid() )
	{
		if ( ref.model() == this )
			item = static_cast<NifProxyItem*>( ref.internalPointer() );
		else
			qDebug() << tr("NifProxyModel::mapFrom() called with wrong ref model");
	}
	item = item->findItem( blockNumber );
	if ( item )
		return createIndex( item->row(), 0, item );
	return QModelIndex();
}

QList<QModelIndex> NifProxyModel::mapFrom( const QModelIndex & idx ) const
{
	QList<QModelIndex> indices;
	
	if ( !( nif && idx.isValid() && ( idx.column() == NifModel::NameCol || idx.column() == NifModel::ValueCol ) ) ) return indices;
	if ( idx.model() != nif )
	{
		qDebug() << tr("NifProxyModel::mapFrom() plural called with wrong model");
		return indices;
	}
	if ( idx.parent().isValid() )
		return indices;
	
	int blockNumber = nif->getBlockNumber( idx );
	if ( blockNumber < 0 )
		return indices;
	
	QList<NifProxyItem*> items;
	root->findAllItems( blockNumber, items );
	foreach( NifProxyItem * item, items )
		indices.append( createIndex( item->row(), idx.column() != NifModel::NameCol ? 1 : 0, item ) );
	
	return indices;
}

Qt::ItemFlags NifProxyModel::flags( const QModelIndex & index ) const
{
	if ( !nif ) return 0;
	return nif->flags( mapTo( index ) );
}

QVariant NifProxyModel::data( const QModelIndex & index, int role ) const
{
	if ( !( nif && index.isValid() ) ) return QVariant();
	return nif->data( mapTo( index ), role );
}

bool NifProxyModel::setData( const QModelIndex & index, const QVariant & v, int role )
{
	if ( !( nif && index.isValid() ) ) return false;
	return nif->setData( mapTo( index ), v, role );
}

QVariant NifProxyModel::headerData( int section, Qt::Orientation orient, int role ) const
{
	if ( !nif || section < 0 || section > 1 ) return QVariant();
	return nif->headerData( ( section ? NifModel::ValueCol : NifModel::NameCol ), orient, role );
}

/*
 *  proxy slots
 */

void NifProxyModel::xHeaderDataChanged( Qt::Orientation o, int a, int b )
{
   Q_UNUSED(a); Q_UNUSED(b);
	emit headerDataChanged( o, 0, 1 );
}
 
void NifProxyModel::xDataChanged( const QModelIndex & begin, const QModelIndex & end )
{
	if ( begin == end )
	{
		QList<QModelIndex> indices = mapFrom( begin );
		foreach ( QModelIndex idx, indices )
			emit dataChanged( idx, idx );
		return;
	}
	else if ( begin.parent() == end.parent() )
	{
		if ( begin.row() == end.row() )
		{
			int m = qMax( begin.column(), end.column() );
			for ( int c = qMin( begin.column(), end.column() ); c < m; c++ )
			{
				QList<QModelIndex> indices = mapFrom( begin.sibling( begin.row(), c ) );
				foreach ( QModelIndex idx, indices )
					emit dataChanged( idx, idx );
			}
			return;
		}
		else if ( begin.column() == end.column() )
		{
			int m = qMax( begin.row() , end.row() );
			for ( int r = qMin( begin.row(), end.row() ); r < m; r++ )
			{
				QList<QModelIndex> indices = mapFrom( begin.sibling( r, begin.column() ) );
				foreach ( QModelIndex idx, indices )
					emit dataChanged( idx, idx );
			}
			return;
		}
	}
	
	reset();
}

void NifProxyModel::xLinksChanged()
{
	updateRoot( false );
}

void NifProxyModel::xRowsAboutToBeRemoved( const QModelIndex & parent, int first, int last )
{
	if ( ! parent.isValid() )
	{	// block removed
		for ( int c = first; c <= last; c++ )
		{
			QList<NifProxyItem*> list;
			root->findAllItems( c-1, list );
			foreach ( NifProxyItem * item, list )
			{
				QModelIndex idx = createIndex( item->row(), 0, item );
				beginRemoveRows( idx.parent(), idx.row(), idx.row() );
				item->parentItem->childItems.removeAll( item );
				delete item;
				endRemoveRows();
			}
		}
	}
}
