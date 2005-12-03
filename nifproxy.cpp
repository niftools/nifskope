/****************************************************************************
**
** nifscope: a tool for analyzing and editing NetImmerse files (.nif & .kf)
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/


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
	
	NifProxyItem * findItem( int b )
	{
		if ( blockNumber == b )	return this;
		NifProxyItem * item = this;
		while ( ( item = item->parentItem ) )
			if ( item->blockNumber == b ) return item;
		foreach ( item, childItems )
		{
			if ( item->blockNumber == b )
				return item;
		}
		foreach ( item, childItems )
		{
			NifProxyItem * x = item->findItem( b );
			if ( x ) return x;
		}
		return 0;
	}
	
	void findAllItems( int b, QList<NifProxyItem*> & list )
	{
		if ( blockNumber == b )
			list.append( this );
		foreach ( NifProxyItem * item, childItems )
		{
			item->findAllItems( b, list );
		}
	}
	
	void checkAssigned( QVector<bool> & assigned )
	{
		foreach ( NifProxyItem * item, childItems )
		{
			assigned[ item->block() ] = true;
			if ( item->childCount() > 0 )
				item->checkAssigned( assigned );
		}
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
	//qDebug() << "proxy set model";
    if ( nif )
	{
        disconnect(nif, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
                   this, SLOT(xDataChanged(QModelIndex,QModelIndex)));
        disconnect(nif, SIGNAL(headerDataChanged(Orientation,int,int)),
                   this, SIGNAL(headerDataChanged(Orientation,int,int)));

        //disconnect(nif, SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)),
        //           this, SLOT(xRowsAboutToBeInserted(QModelIndex,int,int)));
        disconnect(nif, SIGNAL(rowsInserted(QModelIndex,int,int)),
                   this, SLOT(xRowsInserted(QModelIndex,int,int)));
        //disconnect(nif, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
        //           this, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)));
        disconnect(nif, SIGNAL(rowsRemoved(QModelIndex,int,int)),
                   this, SLOT(xRowsRemoved(QModelIndex,int,int)));
		/*
        disconnect(nif, SIGNAL(columnsAboutToBeInserted(QModelIndex,int,int)),
                   this, SIGNAL(columnsAboutToBeInserted(QModelIndex,int,int)));
        disconnect(nif, SIGNAL(columnsInserted(QModelIndex,int,int)),
                   this, SIGNAL(columnsInserted(QModelIndex,int,int)));
        disconnect(nif, SIGNAL(columnsAboutToBeRemoved(QModelIndex,int,int)),
                   this, SIGNAL(columnsAboutToBeRemoved(QModelIndex,int,int)));
        disconnect(nif, SIGNAL(columnsRemoved(QModelIndex,int,int)),
                   this, SIGNAL(columnsRemoved(QModelIndex,int,int)));
		*/
        disconnect(nif, SIGNAL(modelReset()), this, SLOT(reset()));
        disconnect(nif, SIGNAL(layoutChanged()), this, SIGNAL(layoutChanged()));
    }

	nif = qobject_cast<NifModel *>( model );

    if ( nif )
	{
        connect(nif, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
                   this, SLOT(xDataChanged(QModelIndex,QModelIndex)));
        connect(nif, SIGNAL(headerDataChanged(Orientation,int,int)),
                   this, SIGNAL(headerDataChanged(Orientation,int,int)));

        //connect(nif, SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)),
        //        this, SLOT(xRowsAboutToBeInserted(QModelIndex,int,int)));
        connect(nif, SIGNAL(rowsInserted(QModelIndex,int,int)),
                this, SLOT(xRowsInserted(QModelIndex,int,int)));
        //connect(nif, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
        //        this, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)));
        connect(nif, SIGNAL(rowsRemoved(QModelIndex,int,int)),
                this, SLOT(xRowsRemoved(QModelIndex,int,int)));
		/*
        connect(nif, SIGNAL(columnsAboutToBeInserted(QModelIndex,int,int)),
                this, SIGNAL(columnsAboutToBeInserted(QModelIndex,int,int)));
        connect(nif, SIGNAL(columnsInserted(QModelIndex,int,int)),
                this, SIGNAL(columnsInserted(QModelIndex,int,int)));
        connect(nif, SIGNAL(columnsAboutToBeRemoved(QModelIndex,int,int)),
                this, SIGNAL(columnsAboutToBeRemoved(QModelIndex,int,int)));
        connect(nif, SIGNAL(columnsRemoved(QModelIndex,int,int)),
                this, SIGNAL(columnsRemoved(QModelIndex,int,int)));
		*/
        connect(nif, SIGNAL(modelReset()), this, SLOT(reset()));
        connect(nif, SIGNAL(layoutChanged()), this, SIGNAL(layoutChanged()));
    }

	reset();
}

void NifProxyModel::reset()
{
	//qDebug() << "proxy reset";
	root->killChildren();
	updateLinks( -1 );
	updateRoot( true );
	QAbstractItemModel::reset();
}

void NifProxyModel::updateLinks( int block )
{
	//qDebug() << "update links" << block;
	if ( block >= 0 )
	{
		childLinks[ block ].clear();
		parentLinks[ block ].clear();
		updateLinks( block, nif->getBlock( block ) );
	}
	else
	{
		childLinks.clear();
		parentLinks.clear();
		for ( int c = 0; nif && c < nif->getBlockCount(); c++ )
			updateLinks( c );
	}
}

void NifProxyModel::updateLinks( int block, const QModelIndex & parent )
{
	if ( ! parent.isValid() )	return;
	//qDebug() << "update links" << block << nif->itemName( parent );
	for ( int r = 0; r < nif->rowCount( parent ); r++ )
	{
		QModelIndex idx( parent.child( r, 0 ) );
		bool isChildLink;
		bool isLink = nif->isLink( idx, &isChildLink );
		if ( nif->rowCount( idx ) > 0 && ( isLink || nif->itemArr1( idx ).isEmpty() ) )
		{
			updateLinks( block, idx );
		}
		else if ( isLink )
		{
			int l = nif->itemLink( idx );
			if ( l >= 0 )
			{
				//qDebug() << nif->itemName( parent ) << nif->itemName( idx ) << l << isChildLink;
				if ( isChildLink )
					childLinks[block].append( l );
				else
					parentLinks[block].append( l );
			}
		}
	}
}

void NifProxyModel::updateRoot( bool fast )
{
	if ( ! ( nif && nif->getBlockCount() > 0 ) )
	{
		if ( root->childCount() > 0 )
		{
			beginRemoveRows( QModelIndex(), 0, root->childCount() - 1 );
			root->killChildren();
			endRemoveRows();
		}
		return;
	}
	
	//qDebug() << "proxy update top level";
	
	for ( int c = 0; c < nif->getBlockCount(); c++ )
	{
		bool isTopLevel = true;
		for ( int d = 0; d < nif->getBlockCount(); d++ )
		{
			if ( c != d && childLinks[ d ].contains( c ) )
			{
				isTopLevel = false;
				break;
			}
		}
		NifProxyItem * item = root->getLink( c );
		if ( isTopLevel )
		{
			if ( ! item )
			{
				int at = root->childCount();
				if ( ! fast )	beginInsertRows( QModelIndex(), at, at );
				item = root->addLink( c );
				if ( ! fast )	endInsertRows();
			}
			updateItem( item, fast );
		}
		else if ( item )
		{
			int at = root->rowLink( c );
			if ( ! fast ) beginRemoveRows( QModelIndex(), at, at );
			root->delLink( c );
			if ( ! fast ) endRemoveRows();
		}
	}
	
	QVector<bool> assigned( nif->getBlockCount(), false );
	while ( true )
	{
		root->checkAssigned( assigned );
		int c = assigned.indexOf( false );
		if ( c >= 0 )
		{
			//if ( c == 0 )	qWarning() << "no root block found";
			int at = root->childCount();
			if ( ! fast )	beginInsertRows( QModelIndex(), at, at );
			NifProxyItem * item = root->addLink( c );
			if ( ! fast )	endInsertRows();
			updateItem( item, fast );
		}
		else
			break;
	}
}

void NifProxyModel::updateItem( NifProxyItem * item, bool fast )
{
	//qDebug() << "proxy update item" << item->block();
	//if ( item->block() == -1 )
	//	return;

	QModelIndex index( createIndex( item->row(), 0, item ) );
	
	QList<int> parents( item->parentBlocks() );
	
	foreach ( int l, childLinks[item->block()] )
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
			updateItem( child, fast );
		else
			qWarning() << "infinite recursing link construct detected" << item->block() << "->" << child->block();
	}
	foreach ( int l, parentLinks[ item->block() ] )
	{
		if ( ! item->getLink( l ) )
		{
			int at = item->childCount();
			if ( ! fast )	beginInsertRows( index, at, at );
			item->addLink( l );
			if ( ! fast )	endInsertRows();
		}
	}
	if ( ! fast )
	{
		foreach( int l, item->childBlocks() )
		{
			if ( ! ( childLinks[ item->block() ].contains( l ) || parentLinks[ item->block() ].contains( l ) ) )
			{
				int at = item->rowLink( l );
				beginRemoveRows( index, at, at );
				item->delLink( l );
				endRemoveRows();
			}
		}
	}
}

int NifProxyModel::rowCount( const QModelIndex & parent ) const
{
	NifProxyItem * parentItem;
	
	if ( ! parent.isValid() )
		parentItem = root;
	else
		parentItem = static_cast<NifProxyItem*>( parent.internalPointer() );
	
	return ( parentItem ? parentItem->childCount() : 0 );
}

QModelIndex NifProxyModel::index( int row, int column, const QModelIndex & parent ) const
{
	NifProxyItem * parentItem;
	
	if ( ! parent.isValid() )
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
	if ( ! child.isValid() )
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
		qDebug() << "NifProxyModel::mapTo() called with wrong model";
		return QModelIndex();
	}
	NifProxyItem * item = static_cast<NifProxyItem*>( idx.internalPointer() );
	if ( ! item )	return QModelIndex();
	QModelIndex nifidx;
	if ( item->block() == -2 )
		nifidx = nif->getHeader();
	else
		nifidx = nif->getBlock( item->block() );
	if ( nifidx.isValid() ) nifidx = nifidx.sibling( nifidx.row(), idx.column() );
	return nifidx;
}

QModelIndex NifProxyModel::mapFrom( const QModelIndex & idx, const QModelIndex & ref ) const
{
	if ( ! ( nif && idx.isValid() ) ) return QModelIndex();
	if ( idx.model() != nif )
	{
		qDebug() << "NifProxyModel::mapFrom() called with wrong model";
		return QModelIndex();
	}
	int blockNumber = nif->getBlockNumber( idx );
	if ( blockNumber < 0 ) return QModelIndex();
	qDebug() << "proxy map from" << blockNumber;
	NifProxyItem * item = root;
	if ( ref.isValid() )
	{
		if ( ref.model() == this )
			item = static_cast<NifProxyItem*>( ref.internalPointer() );
		else
			qDebug() << "NifProxyModel::mapFrom() called with wrong ref model";
	}
	item = item->findItem( blockNumber );
	if ( item )
		return createIndex( item->row(), idx.column(), item );
	return QModelIndex();
}

QList<QModelIndex> NifProxyModel::mapFrom( const QModelIndex & idx ) const
{
	QList<QModelIndex> indices;
	
	if ( !( nif && idx.isValid() ) ) return indices;
	if ( idx.model() != nif )
	{
		qDebug() << "NifProxyModel::mapFrom() plural called with wrong model";
		return indices;
	}
	if ( idx.parent().isValid() )
		return indices;
	
	int blockNumber = nif->getBlockNumber( idx );
	if ( blockNumber < 0 )
		return indices; // FIXME header
	
	QList<NifProxyItem*> items;
	root->findAllItems( blockNumber, items );
	foreach( NifProxyItem * item, items )
	{
		indices.append( createIndex( item->row(), idx.column(), item ) );
	}
	return indices;
}

Qt::ItemFlags NifProxyModel::flags( const QModelIndex & index ) const
{
	if ( !nif ) return 0;
	return nif->flags( mapTo( index ) );
}

int NifProxyModel::columnCount( const QModelIndex & index ) const
{
	if ( !nif ) return 0;
	return nif->columnCount( mapTo( index ) );
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
	if ( !nif ) return QVariant();
	return nif->headerData( section, orient, role );
}

/*
 *  proxy slots
 */
 
void NifProxyModel::xDataChanged( const QModelIndex & begin, const QModelIndex & end )
{
	if ( begin != end )
	{
		reset();
		return;
	}
	
	//qDebug() << "proxy dataChanged";
	
	if ( nif->isLink( begin ) )
	{
		updateLinks( nif->getBlockNumber( begin ) );
		updateRoot( false );
		return;
	}
	
	QList<QModelIndex> indices = mapFrom( begin );
	foreach ( QModelIndex idx, indices )
		emit dataChanged( idx, idx );
}

void NifProxyModel::xRowsInserted( const QModelIndex & index, int, int )
{
	int block = nif->getBlockNumber( index );
	if ( block >= 0 )
	{
		updateLinks( block );
		updateRoot( false );
	}
	else
	{
		reset();
	}
}

void NifProxyModel::xRowsRemoved( const QModelIndex & index, int, int )
{
	int block = nif->getBlockNumber( index );
	if ( block >= 0 )
	{
		updateLinks( block );
		updateRoot( false );
	}
	else
	{
		reset();
	}
}
