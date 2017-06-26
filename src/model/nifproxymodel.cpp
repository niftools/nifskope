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

#include "nifproxymodel.h"

#include "message.h"
#include "model/nifmodel.h"

#include <QVector>
#include <QDebug>


//! @file nifproxymodel.cpp NifProxyItem

class NifProxyItem
{
public:
	NifProxyItem( int number, NifProxyItem * parent )
	{
		blockNumber = number;
		parentItem  = parent;
	}
	~NifProxyItem()
	{
		qDeleteAll( childItems );
	}

	NifProxyItem * getLink( int link )
	{
		for ( NifProxyItem * item : childItems ) {
			if ( item->block() == link )
				return item;
		}
		return nullptr;
	}

	int rowLink( int link )
	{
		int row = 0;
		for ( NifProxyItem * item : childItems ) {
			if ( item->block() == link )
				return row;

			row++;
		}
		return -1;
	}

	NifProxyItem * addLink( int link )
	{
		NifProxyItem * child = getLink( link );

		if ( child ) {
			return child;
		} else {
			child = new NifProxyItem( link, this );
			childItems.append( child );
			return child;
		}
	}

	void delLink( int link )
	{
		NifProxyItem * child = getLink( link );

		if ( child ) {
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
			return parentItem->childItems.indexOf( const_cast<NifProxyItem *>(this) );

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

		while ( parent && parent->parentItem ) {
			parents.append( parent->blockNumber );
			parent = parent->parentItem;
		}

		return parents;
	}

	QList<int> childBlocks() const
	{
		QList<int> blocks;
		for ( NifProxyItem * item : childItems ) {
			blocks.append( item->block() );
		}
		return blocks;
	}

	NifProxyItem * findItem( int b, bool scanParents = true )
	{
		if ( blockNumber == b )
			return this;

		for ( NifProxyItem * child : childItems ) {
			if ( child->blockNumber == b )
				return child;
		}

		for ( NifProxyItem * child : childItems ) {
			if ( NifProxyItem * x = child->findItem( b, false ) )
				return x;
		}

		if ( parentItem && scanParents ) {
			NifProxyItem * root = parentItem;

			while ( root && root->parentItem )
				root = root->parentItem;

			if ( NifProxyItem * x = root->findItem( b, false ) )
				return x;
		}

		return nullptr;
	}

	void findAllItems( int b, QList<NifProxyItem *> & list )
	{
		for ( NifProxyItem * item : childItems ) {
			item->findAllItems( b, list );
		}

		if ( blockNumber == b )
			list.append( this );
	}

	int blockNumber;
	NifProxyItem * parentItem;
	QList<NifProxyItem *> childItems;
};

NifProxyModel::NifProxyModel( QObject * parent ) : QAbstractItemModel( parent )
{
	root = new NifProxyItem( -1, 0 );
	nif = nullptr;
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
	if ( nif ) {
		disconnect( nif, &NifModel::dataChanged, this, &NifProxyModel::xDataChanged );
		disconnect( nif, &NifModel::headerDataChanged, this, &NifProxyModel::xHeaderDataChanged );
		disconnect( nif, &NifModel::rowsAboutToBeRemoved, this, &NifProxyModel::xRowsAboutToBeRemoved );
		disconnect( nif, &NifModel::linksChanged, this, &NifProxyModel::xLinksChanged );
		disconnect( nif, &NifModel::modelReset, this, &NifProxyModel::reset );
		disconnect( nif, &NifModel::layoutChanged, this, &NifProxyModel::layoutChanged );
	}

	nif = qobject_cast<NifModel *>( model );

	if ( nif ) {
		connect( nif, &NifModel::dataChanged, this, &NifProxyModel::xDataChanged );
		connect( nif, &NifModel::headerDataChanged, this, &NifProxyModel::xHeaderDataChanged );
		connect( nif, &NifModel::rowsAboutToBeRemoved, this, &NifProxyModel::xRowsAboutToBeRemoved );
		connect( nif, &NifModel::linksChanged, this, &NifProxyModel::xLinksChanged );
		connect( nif, &NifModel::modelReset, this, &NifProxyModel::reset );
		connect( nif, &NifModel::layoutChanged, this, &NifProxyModel::layoutChanged );
	}

	reset();
}

void NifProxyModel::reset()
{
	beginResetModel();
	//qDebug() << "proxy reset";
	root->killChildren();
	updateRoot( true );
	endResetModel();
}

void NifProxyModel::updateRoot( bool fast )
{
	if ( !( nif && nif->getBlockCount() > 0 ) ) {
		if ( root->childCount() > 0 ) {
			if ( !fast )
				beginRemoveRows( QModelIndex(), 0, root->childCount() - 1 );

			root->killChildren();

			if ( !fast )
				endRemoveRows();
		}

		return;
	}

	//qDebug() << "proxy update top level";

	// Make a copy to iterate over
	auto items = root->childItems;
	for ( NifProxyItem * item : items ) {
		if ( !nif->getRootLinks().contains( item->block() ) ) {
			int at = root->rowLink( item->block() );

			if ( !fast )
				beginRemoveRows( QModelIndex(), at, at );

			root->delLink( item->block() );

			if ( !fast )
				endRemoveRows();
		}
	}

	for ( const auto l : nif->getRootLinks() ) {
		NifProxyItem * item = root->getLink( l );

		if ( !item ) {
			if ( !fast )
				beginInsertRows( QModelIndex(), root->childCount(), root->childCount() );

			item = root->addLink( l );

			if ( !fast )
				endInsertRows();
		}

		updateItem( item, fast );
	}
}

void NifProxyModel::updateItem( NifProxyItem * item, bool fast )
{
	QModelIndex index( createIndex( item->row(), 0, item ) );

	QList<int> parents( item->parentBlocks() );

	for ( const auto l : item->childBlocks() ) {
		if ( !( nif->getChildLinks( item->block() ).contains( l ) || nif->getParentLinks( item->block() ).contains( l ) ) ) {
			int at = item->rowLink( l );

			if ( !fast )
				beginRemoveRows( index, at, at );

			item->delLink( l );

			if ( !fast )
				endRemoveRows();
		}
	}
	for ( const auto l : nif->getChildLinks( item->block() ) ) {
		NifProxyItem * child = item->getLink( l );

		if ( !child ) {
			int at = item->childCount();

			if ( !fast )
				beginInsertRows( index, at, at );

			child = item->addLink( l );

			if ( !fast )
				endInsertRows();
		}

		if ( !parents.contains( child->block() ) ) {
			updateItem( child, fast );
		} else {
			Message::append( tr( "Warnings were generated while reading NIF file." ),
				tr( "infinite recursive link construct detected %1 -> %2" ).arg( item->block() ).arg( child->block() )
			);
		}
	}
	for ( const auto l : nif->getParentLinks( item->block() ) ) {
		if ( !item->getLink( l ) ) {
			int at = item->childCount();

			if ( !fast )
				beginInsertRows( index, at, at );

			item->addLink( l );

			if ( !fast )
				endInsertRows();
		}
	}
}

int NifProxyModel::rowCount( const QModelIndex & parent ) const
{
	NifProxyItem * parentItem;

	if ( !( parent.isValid() && parent.model() == this ) )
		parentItem = root;
	else
		parentItem = static_cast<NifProxyItem *>( parent.internalPointer() );

	return ( parentItem ? parentItem->childCount() : 0 );
}

QModelIndex NifProxyModel::index( int row, int column, const QModelIndex & parent ) const
{
	NifProxyItem * parentItem;

	if ( !( parent.isValid() && parent.model() == this ) )
		parentItem = root;
	else
		parentItem = static_cast<NifProxyItem *>( parent.internalPointer() );

	NifProxyItem * childItem = ( parentItem ? parentItem->child( row ) : 0 );

	if ( childItem )
		return createIndex( row, column, childItem );

	return QModelIndex();
}

QModelIndex NifProxyModel::parent( const QModelIndex & child ) const
{
	if ( !( child.isValid() && child.model() == this ) )
		return QModelIndex();

	NifProxyItem * childItem  = static_cast<NifProxyItem *>( child.internalPointer() );
	NifProxyItem * parentItem = childItem->parent();

	if ( parentItem == root || !parentItem )
		return QModelIndex();

	return createIndex( parentItem->row(), 0, parentItem );
}

QModelIndex NifProxyModel::mapTo( const QModelIndex & idx ) const
{
	if ( !( nif && idx.isValid() ) )
		return QModelIndex();

	if ( idx.model() != this ) {
		qDebug() << tr( "NifProxyModel::mapTo() called with wrong model" );
		return QModelIndex();
	}

	NifProxyItem * item = static_cast<NifProxyItem *>( idx.internalPointer() );

	if ( !item )
		return QModelIndex();

	QModelIndex nifidx = nif->getBlock( item->block() );

	if ( nifidx.isValid() )
		nifidx = nifidx.sibling( nifidx.row(), ( idx.column() ? NifModel::ValueCol : NifModel::NameCol ) );

	return nifidx;
}

QModelIndex NifProxyModel::mapFrom( const QModelIndex & idx, const QModelIndex & ref ) const
{
	if ( !( nif && idx.isValid() ) )
		return QModelIndex();

	if ( idx.model() != nif ) {
		qDebug() << tr( "NifProxyModel::mapFrom() called with wrong model" );
		return QModelIndex();
	}

	int blockNumber = nif->getBlockNumber( idx );

	if ( blockNumber < 0 )
		return QModelIndex();

	NifProxyItem * item = root;

	if ( ref.isValid() ) {
		if ( ref.model() == this )
			item = static_cast<NifProxyItem *>( ref.internalPointer() );
		else
			qDebug() << tr( "NifProxyModel::mapFrom() called with wrong ref model" );
	}

	item = item->findItem( blockNumber );

	if ( item )
		return createIndex( item->row(), 0, item );

	return QModelIndex();
}

QList<QModelIndex> NifProxyModel::mapFrom( const QModelIndex & idx ) const
{
	QList<QModelIndex> indices;

	if ( !( nif && idx.isValid() && ( idx.column() == NifModel::NameCol || idx.column() == NifModel::ValueCol ) ) )
		return indices;

	if ( idx.model() != nif ) {
		qDebug() << tr( "NifProxyModel::mapFrom() plural called with wrong model" );
		return indices;
	}

	if ( idx.parent().isValid() )
		return indices;

	int blockNumber = nif->getBlockNumber( idx );

	if ( blockNumber < 0 )
		return indices;

	QList<NifProxyItem *> items;
	root->findAllItems( blockNumber, items );
	for ( NifProxyItem * item : items ) {
		indices.append( createIndex( item->row(), idx.column() != NifModel::NameCol ? 1 : 0, item ) );
	}

	return indices;
}

Qt::ItemFlags NifProxyModel::flags( const QModelIndex & index ) const
{
	if ( !nif )
		return 0;

	return nif->flags( mapTo( index ) );
}

QVariant NifProxyModel::data( const QModelIndex & index, int role ) const
{
	if ( !( nif && index.isValid() ) )
		return QVariant();

	return nif->data( mapTo( index ), role );
}

bool NifProxyModel::setData( const QModelIndex & index, const QVariant & v, int role )
{
	if ( !( nif && index.isValid() ) )
		return false;

	return nif->setData( mapTo( index ), v, role );
}

QVariant NifProxyModel::headerData( int section, Qt::Orientation orient, int role ) const
{
	if ( !nif || section < 0 || section > 1 )
		return QVariant();

	return nif->headerData( ( section ? NifModel::ValueCol : NifModel::NameCol ), orient, role );
}

/*
 *  proxy slots
 */

void NifProxyModel::xHeaderDataChanged( Qt::Orientation o, int a, int b )
{
	Q_UNUSED( a ); Q_UNUSED( b );
	emit headerDataChanged( o, 0, 1 );
}

void NifProxyModel::xDataChanged( const QModelIndex & begin, const QModelIndex & end )
{
	if ( begin == end ) {
		QList<QModelIndex> indices = mapFrom( begin );
		for ( const QModelIndex& idx : indices ) {
			emit dataChanged( idx, idx );
		}
		return;
	}
	if ( begin.parent() == end.parent() ) {
		if ( begin.row() == end.row() ) {
			int m = qMax( begin.column(), end.column() );

			for ( int c = qMin( begin.column(), end.column() ); c < m; c++ ) {
				QList<QModelIndex> indices = mapFrom( begin.sibling( begin.row(), c ) );
				for ( const QModelIndex& idx : indices ) {
					emit dataChanged( idx, idx );
				}
			}

			return;
		}
		if ( begin.column() == end.column() ) {
			int m = qMax( begin.row(), end.row() );

			for ( int r = qMin( begin.row(), end.row() ); r < m; r++ ) {
				QList<QModelIndex> indices = mapFrom( begin.sibling( r, begin.column() ) );
				for ( const QModelIndex& idx : indices ) {
					emit dataChanged( idx, idx );
				}
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
	if ( !parent.isValid() ) {
		// block removed
		for ( int c = first; c <= last; c++ ) {
			QList<NifProxyItem *> list;
			root->findAllItems( c - 1, list );
			for ( NifProxyItem * item : list ) {
				QModelIndex idx = createIndex( item->row(), 0, item );
				beginRemoveRows( idx.parent(), idx.row(), idx.row() );
				item->parentItem->childItems.removeAll( item );
				delete item;
				endRemoveRows();
			}
		}
	}
}
