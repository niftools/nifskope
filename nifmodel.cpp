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

#include "nifmodel.h"

#include <QApplication>
#include <QByteArray>
#include <QColor>
#include <QDebug>
#include <QProgressDialog>
#include <QTime>

#include <qendian.h>

class NifItem
{
public:
	NifItem( NifItem * parent )
		: parentItem( parent ) {}
	
	NifItem( const NifData & data, NifItem * parent )
		: itemData( data ), parentItem( parent ) {}
	
	~NifItem()
	{
		qDeleteAll( childItems );
	}

	NifItem * parent() const
	{
		return parentItem;
	}
	
	int row() const
	{
		if ( parentItem )
			return parentItem->childItems.indexOf( const_cast<NifItem*>(this) );
		return 0;
	}

	void prepareInsert( int e )
	{
		childItems.reserve( childItems.count() + e );
	}
	
	NifItem * insertChild( const NifData & data, int at = -1 )
	{
		NifItem * item = new NifItem( data, this );
		if ( at < 0 || at > childItems.count() )
			childItems.append( item );
		else
			childItems.insert( at, item );
		return item;
	}
	
	void removeChild( int row )
	{
		NifItem * item = child( row );
		if ( item )
		{
			childItems.remove( row );
			delete item;
		}
	}
	
	void removeChildren( int row, int count )
	{
		for ( int c = row; c < row + count; c++ )
		{
			NifItem * item = childItems.value( c );
			if ( item ) delete item;
		}
		childItems.remove( row, count );
	}
	
	NifItem * child( int row )
	{
		return childItems.value( row );
	}
	
	NifItem * child( const QString & name )
	{
		foreach ( NifItem * child, childItems )
			if ( child->name() == name )
				return child;
		return 0;
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

	inline QString  name() const	{	return itemData.name();	}
	inline QString  type() const	{	return itemData.type();	}
	inline QVariant value() const	{	return itemData.value();	}
	inline QString  arg() const	{	return itemData.arg();		}
	inline QString  arr1() const	{	return itemData.arr1();	}
	inline QString  arr2() const	{	return itemData.arr2();	}
	inline QString  cond() const	{	return itemData.cond();	}
	inline quint32  ver1() const	{	return itemData.ver1();	}
	inline quint32  ver2() const	{	return itemData.ver2();	}
	
	inline void setName( const QString & name )	{	itemData.setName( name );	}
	inline void setType( const QString & type )	{	itemData.setType( type );	}
	inline void setValue( const QVariant & v )	{	itemData.setValue( v );		}
	inline void setArg( const QString & arg )		{	itemData.setArg( arg );		}
	inline void setArr1( const QString & arr1 )	{	itemData.setArr1( arr1 );	}
	inline void setArr2( const QString & arr2 )	{	itemData.setArr2( arr2 );	}
	inline void setCond( const QString & cond )	{	itemData.setCond( cond );	}
	inline void setVer1( int v1 )					{	itemData.setVer1( v1 );		}
	inline void setVer2( int v2 )					{	itemData.setVer2( v2 );		}
	
	inline bool evalVersion( quint32 v )
	{
		return ( ( ver1() == 0 || ver1() <= v ) && ( ver2() == 0 || v <= ver2() ) );
	}

private:
	NifData itemData;
	NifItem * parentItem;
	QVector<NifItem*> childItems;
};

QString inBrakets( const QString & x )
{
	for ( int c = 0; c < x.length(); c++ )
		if ( ! x[c].isNumber() )
			return QString( "(%1)" ).arg( x );
	return x;
}
 

NifModel::NifModel( QObject * parent ) : QAbstractItemModel( parent )
{
	root = new NifItem( 0 );
	clear();
}

NifModel::~NifModel()
{
	delete root;
}

void NifModel::clear()
{
	root->killChildren();
	insertType( root, NifData( "NiHeader", "header" ) );
	version = 0x04000002;
	version_string = "NetImmerse File Format, Version 4.0.0.2\n";
	reset();
	updateHeader();
}


/*
 *  header functions
 */

QModelIndex NifModel::getHeader() const
{
	QModelIndex header = index( 0, 0 );
	if ( itemType( header ) != "header" )
		return QModelIndex();
	return header;
}

NifItem * NifModel::getHeaderItem() const
{
	return root->child( 0 );
}

void NifModel::updateHeader()
{
	QModelIndex header = getHeader();
	
	setValue( header, "version", version );
	setValue( header, "num blocks", getBlockCount() );
	
	QModelIndex idxBlockTypes = getIndex( header, "block types" );
	QModelIndex idxBlockTypeIndices = getIndex( header, "block type index" );
	if ( idxBlockTypes.isValid() && idxBlockTypeIndices.isValid() )
	{
		QStringList blocktypes;
		QList<int>	blocktypeindices;
		for ( int r = 0; r < rowCount(); r++ )
		for ( int r = 0; r < rowCount(); r++ )
		{
			QModelIndex block = index( r, 0 );
			if ( itemType( block ) == "NiBlock" )
			{
				QString name = itemName( block );
				if ( ! blocktypes.contains( name ) )
					blocktypes.append( name );
				blocktypeindices.append( blocktypes.indexOf( name ) );
			}
		}
		setValue( header, "num block types", blocktypes.count() );
		
		updateArray( idxBlockTypes );
		updateArray( idxBlockTypeIndices );
		
		for ( int r = 0; r < rowCount( idxBlockTypes ); r++ )
			setItemValue( idxBlockTypes.child( r, 0 ), blocktypes.value( r ) );
		
		for ( int r = 0; r < rowCount( idxBlockTypeIndices ); r++ )
			setItemValue( idxBlockTypeIndices.child( r, 0 ), blocktypeindices.value( r ) );
	}
}


/*
 *  array functions
 */
 
int NifModel::getArraySize( NifItem * array ) const
{
	NifItem * parent = array->parent();
	if ( ! parent || parent == root )
		return -1;
		
	if ( array->arr1().isEmpty() )
		return 0;
	
	bool ok;
	int d1 = array->arr1().toInt( &ok );
	if ( ! ok )
	{
		NifItem * dim1 = getItem( parent, array->arr1() );
		if ( ! dim1 )
		{
			qCritical() << "failed to get array size for array" << array->name();
			return 0;
		}
		
		if ( dim1->childCount() == 0 )
			d1 = dim1->value().toInt();
		else
		{
			NifItem * item = dim1->child( array->row() );
			if ( item )
				d1 = item->value().toInt();
			else
				qCritical() << "failed to get array size for array " << array->name();
		}
	}
	if ( d1 < 0 )
	{
		qWarning() << "invalid array size for array" << array->name();
		d1 = 0;
	}
	if ( d1 > 1024 * 1024 * 8 )
	{
		qWarning() << "array" << array->name() << "much too large";
		d1 = 1024 * 1024 * 8;
	}
	return d1;
}

void NifModel::updateArray( NifItem * array, bool fast )
{
	if ( array->arr1().isEmpty() )
		return;
	
	int d1 = getArraySize( array );
	if ( d1 < 0 )	return;

	int rows = array->childCount();
	if ( d1 > rows )
	{
		NifData data( array->name(), array->type(), getTypeValue( array->type() ), inBrakets( array->arg() ), inBrakets( array->arr2() ), QString(), QString(), 0, 0 );
		
		if ( ! fast )	beginInsertRows( createIndex( array->row(), 0, array ), rows, d1-1 );
		array->prepareInsert( d1 - rows );
		for ( int c = rows; c < d1; c++ )
			insertType( array, data );
		if ( ! fast )	endInsertRows();
	}
	if ( d1 < rows )
	{
		if ( ! fast )	beginRemoveRows( createIndex( array->row(), 0, array ), d1, rows - 1 );
		array->removeChildren( d1, rows - d1 );
		if ( ! fast )	endRemoveRows();
	}
	if ( !fast && d1 != rows && itemIsLink( array ) )
	{
		updateLinks();
		emit linksChanged();
	}
}

void NifModel::updateArray( const QModelIndex & array, bool fast )
{
	NifItem * item = static_cast<NifItem*>( array.internalPointer() );
	if ( ! ( array.isValid() && item && array.model() == this ) )
		return;
	updateArray( item, fast );
}

/*
 *  block functions
 */

void NifModel::insertNiBlock( const QString & identifier, int at, bool fast )
{
	NifBlock * block = blocks.value( identifier );
	if ( block )
	{
		if ( at < 0 || at > getBlockCount() )
			at = -1;
		if ( at >= 0 )
			adjustLinks( root, at, 1 );
		
		if ( at >= 0 ) at++;
		
		if ( ! fast ) beginInsertRows( QModelIndex(), ( at >= 0 ? at : rowCount() ), ( at >= 0 ? at : rowCount() ) );
		NifItem * branch = insertBranch( root, NifData( identifier, "NiBlock" ), at );
		if ( ! fast ) endInsertRows();
		
		foreach ( QString a, block->ancestors )
			insertAncestor( branch, a );
		
		branch->prepareInsert( block->types.count() );
		
		foreach ( NifData data, block->types )
			insertType( branch, data );
		
		if ( ! fast )
			reset();
	}
	else
	{
		qCritical() << "unknown block " << identifier;
	}
}

void NifModel::removeNiBlock( int blocknum )
{
	adjustLinks( root, blocknum, 0 );
	adjustLinks( root, blocknum, -1 );
	root->removeChild( blocknum + 1 );
	reset();
}

int NifModel::getBlockNumber( const QModelIndex & idx ) const
{
	if ( ! ( idx.isValid() && idx.model() == this ) )
		return -1;
	
	const NifItem * block = static_cast<NifItem*>( idx.internalPointer() );
	while ( block && block->parent() != root )
		block = block->parent();
	
	if ( block )
		return block->row() - 1;
	else
		return -1;
}

int NifModel::getBlockNumber( NifItem * item ) const
{
	while ( item && item->parent() != root )
		item = item->parent();
	
	if ( item )
		return item->row() - 1;
	else
		return -1;
}

QModelIndex NifModel::getBlock( int x, const QString & name ) const
{
	x += 1; //the first block is the NiHeader
	QModelIndex idx = index( x, 0 );
	if ( ! name.isEmpty() )
		return ( itemName( idx ) == name ? idx : QModelIndex() );
	else
		return idx;
}

NifItem * NifModel::getBlockItem( int x ) const
{
	return root->child( x+1 );
}

int NifModel::getBlockCount() const
{
	return rowCount() - 1;
}


/*
 *  ancestor functions
 */

void NifModel::insertAncestor( NifItem * parent, const QString & identifier, int at )
{
	NifBlock * ancestor = ancestors.value( identifier );
	if ( ancestor )
	{
		foreach ( QString a, ancestor->ancestors )
			insertAncestor( parent, a );
		parent->prepareInsert( ancestor->types.count() );
		foreach ( NifData data, ancestor->types )
			insertType( parent, data );
	}
	else
	{
		qCritical() << "unknown ancestor " << identifier;
	}
}

bool NifModel::inherits( const QString & name, const QString & aunty )
{
	NifBlock * type = blocks.value( name );
	if ( ! type ) type = ancestors.value( name );
	if ( ! type ) return false;
	foreach ( QString a, type->ancestors )
	{
		if ( a == aunty || inherits( a, aunty ) ) return true;
	}
	return false;
}


/*
 *  basic and compound type functions
 */

void NifModel::insertType( NifItem * parent, const NifData & data, int at )
{
	if ( ! data.arr1().isEmpty() )
	{
		NifItem * array = insertBranch( parent, data, at );
		
		if ( evalCondition( array ) )
			updateArray( array, true );
		return;
	}

	NifBlock * compound = compounds.value( data.type() );
	if ( compound )
	{
		NifItem * branch = insertBranch( parent, data, at );
		branch->prepareInsert( compound->types.count() );
		QString arg = inBrakets( data.arg() );
		if ( ! arg.isEmpty() )
		{
			foreach ( NifData d, compound->types )
			{
				if ( d.arr1().contains( "(arg)" ) ) { QString x = d.arr1(); x.replace( x.indexOf( "(arg)" ), 5, arg ); d.setArr1( x ); }
				if ( d.arr2().contains( "(arg)" ) ) { QString x = d.arr2(); x.replace( x.indexOf( "(arg)" ), 5, arg ); d.setArr2( x ); }
				if ( d.cond().contains( "(arg)" ) ) { QString x = d.cond(); x.replace( x.indexOf( "(arg)" ), 5, arg ); d.setCond( x ); }
				insertType( branch, d );
			}
		}
		else
			foreach ( NifData d, compound->types )
				insertType( branch, d );
	}
	else
		parent->insertChild( data, at );
}


/*
 *  item value functions
 */

QString NifModel::itemName( const QModelIndex & index ) const
{
	NifItem * item = static_cast<NifItem*>( index.internalPointer() );
	if ( ! ( index.isValid() && item && index.model() == this ) )	return QString();
	return item->name();
}

QString NifModel::itemType( const QModelIndex & index ) const
{
	NifItem * item = static_cast<NifItem*>( index.internalPointer() );
	if ( ! ( index.isValid() && item && index.model() == this ) )	return QString();
	return item->type();
}

QVariant NifModel::itemValue( const QModelIndex & index ) const
{
	NifItem * item = static_cast<NifItem*>( index.internalPointer() );
	if ( ! ( index.isValid() && item && index.model() == this ) )	return QVariant();
	return item->value();
}

QString NifModel::itemArg( const QModelIndex & index ) const
{
	NifItem * item = static_cast<NifItem*>( index.internalPointer() );
	if ( ! ( index.isValid() && item && index.model() == this ) )	return QString();
	return item->arg();
}

QString NifModel::itemArr1( const QModelIndex & index ) const
{
	NifItem * item = static_cast<NifItem*>( index.internalPointer() );
	if ( ! ( index.isValid() && item && index.model() == this ) )	return QString();
	return item->arr1();
}

QString NifModel::itemArr2( const QModelIndex & index ) const
{
	NifItem * item = static_cast<NifItem*>( index.internalPointer() );
	if ( ! ( index.isValid() && item && index.model() == this ) )	return QString();
	return item->arr2();
}

QString NifModel::itemCond( const QModelIndex & index ) const
{
	NifItem * item = static_cast<NifItem*>( index.internalPointer() );
	if ( ! ( index.isValid() && item && index.model() == this ) )	return QString();
	return item->cond();
}

quint32 NifModel::itemVer1( const QModelIndex & index ) const
{
	NifItem * item = static_cast<NifItem*>( index.internalPointer() );
	if ( ! ( index.isValid() && item && index.model() == this ) )	return 0;
	return item->ver1();
}

quint32 NifModel::itemVer2( const QModelIndex & index ) const
{
	NifItem * item = static_cast<NifItem*>( index.internalPointer() );
	if ( ! ( index.isValid() && item && index.model() == this ) )	return 0;
	return item->ver2();
}

qint32 NifModel::itemLink( NifItem * item ) const
{
	return ( getDisplayHint( item->type() ) == "link" && item->value().isValid() ? item->value().toInt() : -1 );
}

qint32 NifModel::itemLink( const QModelIndex & index ) const
{
	NifItem * item = static_cast<NifItem*>( index.internalPointer() );
	if ( ! ( index.isValid() && item && index.model() == this ) )	return -1;
	return itemLink( item );
}

bool NifModel::itemIsLink( NifItem * item, bool * isChildLink ) const
{
	if ( isChildLink )
		*isChildLink = ( item->type() == "link" );
	return ( getDisplayHint( item->type() ) == "link" );
}

bool NifModel::itemIsLink( const QModelIndex & index, bool * isChildLink ) const
{
	NifItem * item = static_cast<NifItem*>( index.internalPointer() );
	if ( ! ( index.isValid() && item && index.model() == this ) )	return false;
	return itemIsLink( item, isChildLink );
}

void NifModel::setItemValue( const QModelIndex & index, const QVariant & var )
{
	NifItem * item = static_cast<NifItem*>( index.internalPointer() );
	if ( ! ( index.isValid() && item && index.model() == this ) )	return;
	item->setValue( var );
	emit dataChanged( index.sibling( index.row(), ValueCol ), index.sibling( index.row(), ValueCol ) );
	if ( itemIsLink( index ) )
	{
		updateLinks();
		emit linksChanged();
	}
}

bool NifModel::setValue( const QModelIndex & parent, const QString & name, const QVariant & var )
{
	if ( ! parent.isValid() )
	{
		qWarning( "setValue( %s ) reached top level", str( name ) );
		return false;
	}
	
	if ( name.startsWith( "(" ) && name.endsWith( ")" ) )
		return setValue( parent.parent(), name.mid( 1, name.length() - 2 ).trimmed(), var );
	
	for ( int c = 0; c < rowCount( parent ); c++ )
	{
		QModelIndex child = parent.child( c, 0 );
		if ( itemName( child ) == name && evalCondition( child ) )
		{
			setItemValue( child, var );
			return true;
		}
		if ( rowCount( child ) > 0 && itemArr1( child ).isEmpty() )
		{
			if ( setValue( child, name, var ) ) return true;
		}
	}
	
	return false;
}


/*
 *  version conversion
 */

QString NifModel::version2string( quint32 v )
{
	if ( v == 0 )	return QString();
	QString s = QString::number( ( v >> 24 ) & 0xff, 10 ) + "."
		+ QString::number( ( v >> 16 ) & 0xff, 10 ) + "."
		+ QString::number( ( v >> 8 ) & 0xff, 10 ) + "."
		+ QString::number( v & 0xff, 10 );
	return s;
}	

quint32 NifModel::version2number( const QString & s )
{
	if ( s.isEmpty() )	return 0;
	QStringList l = s.split( "." );
	if ( l.count() != 4 )
	{
		bool ok;
		quint32 i = s.toUInt( &ok );
		if ( ! ok )
			qDebug() << "version2number( " << s << " ) : conversion failed";
		return ( i == 0xffffffff ? 0 : i );
	}
	quint32 v = 0;
	for ( int i = 0; i < 4; i++ )
		v += l[i].toInt( 0, 10 ) << ( (3-i) * 8 );
	return v;
}


/*
 *  QAbstractModel interface
 */

QModelIndex NifModel::index( int row, int column, const QModelIndex & parent ) const
{
	NifItem * parentItem;
	
	if ( ! ( parent.isValid() && parent.model() == this ) )
		parentItem = root;
	else
		parentItem = static_cast<NifItem*>( parent.internalPointer() );
	
	NifItem * childItem = ( parentItem ? parentItem->child( row ) : 0 );
	if ( childItem )
		return createIndex( row, column, childItem );
	else
		return QModelIndex();
}

QModelIndex NifModel::parent( const QModelIndex & child ) const
{
	if ( ! ( child.isValid() && child.model() == this ) )
		return QModelIndex();
	
	NifItem *childItem = static_cast<NifItem*>( child.internalPointer() );
	if ( ! childItem ) return QModelIndex();
	NifItem *parentItem = childItem->parent();
	
	if ( parentItem == root || ! parentItem )
		return QModelIndex();
	
	return createIndex( parentItem->row(), 0, parentItem );
}

int NifModel::rowCount( const QModelIndex & parent ) const
{
	NifItem * parentItem;
	
	if ( ! ( parent.isValid() && parent.model() == this ) )
		parentItem = root;
	else
		parentItem = static_cast<NifItem*>( parent.internalPointer() );
	
	return ( parentItem ? parentItem->childCount() : 0 );
}

QVariant NifModel::data( const QModelIndex & index, int role ) const
{
	if ( ! ( index.isValid() && index.model() == this ) )
		return QVariant();

	NifItem * item = static_cast<NifItem*>( index.internalPointer() );
	if ( ! item ) return QVariant();
	
	int column = index.column();
	
	if ( column == ValueCol && item->parent() == root && item->type() == "NiBlock" )
	{
		QModelIndex buddy;
		if ( item->name() == "NiSourceTexture" )
			buddy = getIndex( getIndex( index, "texture source" ), "file name" );
		else
			buddy = getIndex( index, "name" );
		if ( buddy.isValid() )
			buddy = buddy.sibling( buddy.row(), index.column() );
		if ( buddy.isValid() )
			return data( buddy, role );
	}
	
	switch ( role )
	{
		case Qt::DisplayRole:
		{
			switch ( column )
			{
				case NameCol:	return item->name();
				case TypeCol:	return item->type();
				case ValueCol:
				{
					if ( ! item->value().isValid() )
						return QVariant();
					QString displayHint = getDisplayHint( item->type() );
					if ( displayHint == "float" )
						return QString::number( item->value().toDouble(), 'f', 4 );
					else if ( displayHint == "dec" )
						return QString::number( item->value().toInt(), 10 );
					else if ( displayHint == "bool" )
						return ( item->value().toUInt() != 0 ? "yes" : "no" );
					else if ( displayHint == "hex" )
						return QString::number( item->value().toUInt(), 16 ).prepend( "0x" );
					else if ( displayHint == "bin" )
						return QString::number( item->value().toUInt(), 2 ).prepend( "0b" );
					else if ( displayHint == "link" && item->value().isValid() )
					{
						int lnk = item->value().toInt();
						if ( lnk == -1 )
							return QString( "none" );
						else
						{
							QModelIndex block = getBlock( lnk );
							if ( block.isValid() )
							{
								QModelIndex block_name = getIndex( block, "name" );
								if ( block_name.isValid() && ! itemValue( block_name ).toString().isEmpty() )
									return QString( "%1 (%2)" ).arg( lnk ).arg( itemValue( block_name ).toString() );
								else
									return QString( "%1 [%2]" ).arg( lnk ).arg( itemName( block ) );
							}
							else
							{
								return QString( "%1 <invalid>" ).arg( lnk );
							}
						}
					}
					/*
					else if ( displayHint == "color" )
					{
						QColor c = item->value().value<QColor>();
						QString s = "R " + QString::number( c.redF(), 'f', 2 ) + " G " + QString::number( c.greenF(), 'f', 2 ) + " B " + QString::number( c.blueF(), 'f', 2 );
						if ( getInternalType( item->type() ) == it_color4f )
							s += " A " + QString::number( c.alphaF(), 'f', 2 );
						return s;
					}
					*/
					return item->value();
				}
				case ArgCol:	return item->arg();
				case Arr1Col:	return item->arr1();
				case Arr2Col:	return item->arr2();
				case CondCol:	return item->cond();
				case Ver1Col:	return version2string( item->ver1() );
				case Ver2Col:	return version2string( item->ver2() );
				default:		return QVariant();
			}
		}
		case Qt::DecorationRole:
		{
			switch ( column )
			{
				case NameCol:
					if ( itemType( index ) == "NiBlock" )
						return QString::number( getBlockNumber( index ) );
				default:
					return QVariant();
			}
		}
		case Qt::EditRole:
		{
			switch ( column )
			{
				case NameCol:	return item->name();
				case TypeCol:	return item->type();
				case ValueCol:	return item->value();
				case ArgCol:	return item->arg();
				case Arr1Col:	return item->arr1();
				case Arr2Col:	return item->arr2();
				case CondCol:	return item->cond();
				case Ver1Col:	return version2string( item->ver1() );
				case Ver2Col:	return version2string( item->ver2() );
				default:		return QVariant();
			}
		}
		case Qt::ToolTipRole:
		{
			QString tip;
			if ( column == TypeCol )
			{
				tip = getTypeDescription( item->type() );
				if ( ! tip.isEmpty() ) return tip;
			}
			return QVariant();
		}
		case Qt::BackgroundColorRole:
		{
			if ( column == ValueCol && ( getDisplayHint( item->type() ) == "color" ) )
				return qvariant_cast<QColor>( item->value() );
			else
				return QVariant();
		}
		default:
			return QVariant();
	}
}

bool NifModel::setData( const QModelIndex & index, const QVariant & value, int role )
{
	if ( ! ( index.isValid() && role == Qt::EditRole && index.model() == this ) )
		return false;
	
	NifItem * item = static_cast<NifItem*>( index.internalPointer() );
	if ( ! item )	return false;
	if ( index.column() == ValueCol && item->parent() == root && item->type() == "NiBlock" )
	{
		QModelIndex buddy;
		if ( item->name() == "NiSourceTexture" )
			buddy = getIndex( getIndex( index, "texture source" ), "file name" );
		else
			buddy = getIndex( index, "name" );
		if ( buddy.isValid() )
			buddy = buddy.sibling( buddy.row(), index.column() );
		if ( buddy.isValid() )
		{
			bool ret = setData( buddy, value, role );
			emit dataChanged( index, index );
			return ret;
		}
	}
	switch ( index.column() )
	{
		case NifModel::NameCol:
			item->setName( value.toString() );
			break;
		case NifModel::TypeCol:
			item->setType( value.toString() );
			break;
		case NifModel::ValueCol:
			item->setValue( value );
			if ( itemIsLink( index ) )
			{
				updateLinks();
				emit linksChanged();
			}
			break;
		case NifModel::ArgCol:
			item->setArg( value.toString() );
			break;
		case NifModel::Arr1Col:
			item->setArr1( value.toString() );
			break;
		case NifModel::Arr2Col:
			item->setArr2( value.toString() );
			break;
		case NifModel::CondCol:
			item->setCond( value.toString() );
			break;
		case NifModel::Ver1Col:
			item->setVer1( NifModel::version2number( value.toString() ) );
			break;
		case NifModel::Ver2Col:
			item->setVer2( NifModel::version2number( value.toString() ) );
			break;
		default:
			return false;
	}
	emit dataChanged( index, index );
	return true;
}

QVariant NifModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
	if ( role != Qt::DisplayRole )
		return QVariant();
	switch ( role )
	{
		case Qt::DisplayRole:
			switch ( section )
			{
				case NameCol:		return "Name";
				case TypeCol:		return "Type";
				case ValueCol:		return "Value";
				case ArgCol:		return "Argument";
				case Arr1Col:		return "Array1";
				case Arr2Col:		return "Array2";
				case CondCol:		return "Condition";
				case Ver1Col:		return "since";
				case Ver2Col:		return "until";
				default:			return QVariant();
			}
		default:
			return QVariant();
	}
}

void NifModel::reset()
{
	updateLinks();
	QAbstractItemModel::reset();
}


/*
 *  load and save
 */

bool NifModel::load( QIODevice & device )
{
	// reset model
	clear();

	{	// read magic version string
		version_string.clear();
		int c = 0;
		char chr = 0;
		while ( c < 80 && chr != '\n' )
		{
			device.read( & chr, 1 );
			version_string.append( chr );
		}
	}
	
	// verify magic id
	qDebug() << version_string;
	if ( ! ( version_string.startsWith( "NetImmerse File Format" ) || version_string.startsWith( "Gamebryo" ) ) )
	{
		qCritical( "this is not a NIF" );
		clear();
		return false;
	}
	
	// read version number
	device.read( (char *) &version, 4 );
	qDebug( "version %08X", version );
	device.seek( device.pos() - 4 );
	
	// verify version number
	if ( ! supportedVersions.contains( version ) )
	{
		qCritical() << "version" << version2string( version ) << "is not supported yet";
		clear();
		return false;
	}
	
	// read header
	if ( !load( getHeaderItem(), device ) )
	{
		qCritical() << "failed to load file header (version" << version << ")";
		clear();
		return false;
	}
	
	QModelIndex header = index( 0, 0 );
	
	int numblocks = getInt( header, "num blocks" );
	qDebug( "numblocks %i", numblocks );

	qApp->processEvents();

	QProgressDialog prog;
	prog.setLabelText( "loading nif..." );
	prog.setRange( 0, numblocks );
	prog.setValue( 0 );
	prog.setMinimumDuration( 2100 );

	// read in the NiBlocks
	try
	{
		for ( int c = 0; c < numblocks; c++ )
		{
			prog.setValue( c + 1 );
			qApp->processEvents();
			if ( prog.wasCanceled() )
			{
				clear();
				return false;
			}

			if ( device.atEnd() )
				throw QString( "unexpected EOF during load" );
			
			QString blktyp;
			
			if ( version > 0x0a000000 )
			{
				if ( version < 0x0a020000 )		device.read( 4 );
				
				int blktypidx = itemValue( index( c, 0, getIndex( header, "block type index" ) ) ).toInt();
				blktyp = itemValue( index( blktypidx, 0, getIndex( header, "block types" ) ) ).toString();
			}
			else
			{
				int len;
				device.read( (char *) &len, 4 );
				if ( len < 2 || len > 80 )
					throw QString( "next block starts not with a NiString" );
				blktyp = device.read( len );
			}
			
			if ( isNiBlock( blktyp ) )
			{
				qDebug() << "loading block" << c << ":" << blktyp;
				insertNiBlock( blktyp, -1, true );
				if ( ! load( root->child( c+1 ), device ) ) 
					throw QString( "failed to load block number %1 (%2)" ).arg( c ).arg( blktyp );
			}
			else
				throw QString( "encountered unknown block (%1)" ).arg( blktyp );
		}
	}
	catch ( QString err )
	{
		qCritical() << (const char *) err.toAscii();
	}
	reset(); // notify model views that a significant change to the data structure has occurded
	return true;
}

bool NifModel::save( QIODevice & device )
{
	// write magic version string
	device.write( version_string );
	
	qApp->processEvents();

	QProgressDialog prog;
	prog.setLabelText( "saving nif..." );
	prog.setRange( 0, rowCount( QModelIndex() ) );
	prog.setValue( 0 );
	prog.setMinimumDuration( 2100 );

	for ( int c = 0; c < rowCount( QModelIndex() ); c++ )
	{
		prog.setValue( c + 1 );
		qApp->processEvents();
		if ( prog.wasCanceled() )
			return false;
		
		qDebug() << "saving block" << c << ":" << itemName( index( c, 0 ) );
		if ( itemType( index( c, 0 ) ) == "NiBlock" )
		{
			if ( version > 0x0a000000 )
			{
				if ( version < 0x0a020000 )	
				{
					int null = 0;
					device.write( (char *) &null, 4 );
				}
			}
			else
			{
				QString string = itemName( index( c, 0 ) );
				int len = string.length();
				device.write( (char *) &len, 4 );
				device.write( (const char *) string.toAscii(), len );
			}
		}
		if ( !save( root->child( c ), device ) )
			return false;
	}
	int foot1 = 1;
	device.write( (char *) &foot1, 4 );
	int foot2 = 0;
	device.write( (char *) &foot2, 4 );
	return true;
}

bool NifModel::load( QIODevice & device, const QModelIndex & index )
{
	NifItem * item = static_cast<NifItem*>( index.internalPointer() );
	if ( item && index.isValid() && index.model() == this )
	{
		load( item, device, false );
		updateLinks();
		emit linksChanged();
		return true;
	}
	reset();
	return false;
}

bool NifModel::save( QIODevice & device, const QModelIndex & index )
{
	NifItem * item = static_cast<NifItem*>( index.internalPointer() );
	return ( item && index.isValid() && index.model() == this && save( item, device ) );
}

bool NifModel::load( NifItem * parent, QIODevice & device, bool fast )
{
	if ( ! parent ) return false;
	
	//qDebug() << "loading branch" << parent->name();
	
	for ( int row = 0; row < parent->childCount(); row++ )
	{
		NifItem * child = parent->child( row );
		if ( child && evalCondition( child ) )
		{
			if ( ! child->arr1().isEmpty() )
			{
				updateArray( child, fast );
				if ( !load( child, device ) )
					return false;
			}
			else if ( child->childCount() > 0 )
			{
				if ( !load( child, device, fast ) )
					return false;
			}
			else switch ( getInternalType( child->type() ) )
			{
				case it_uint8:
				{
					quint8 u8;
					device.read( (char *) &u8, 1 );
					child->setValue( u8 );
				} break;
				case it_uint16:
				{
					quint16 u16;
					device.read( (char *) &u16, 2 );
					child->setValue( qFromLittleEndian( u16 ) );
				} break;
				case it_uint32:
				{
					quint32 u32;
					device.read( (char *) &u32, 4 );
					child->setValue( qFromLittleEndian( u32 ) );
				} break;
				case it_int8:
				{
					qint8 s8;
					device.read( (char *) &s8, 1 );
					child->setValue( s8 );
				} break;
				case it_int16:
				{
					qint16 s16;
					device.read( (char *) &s16, 2 );
					child->setValue( qFromLittleEndian( s16 ) );
				} break;
				case it_int32:
				{
					qint32 s32;
					device.read( (char *) &s32, 4 );
					child->setValue( qFromLittleEndian( s32 ) );
				} break;
				case it_float:
				{
					float f32;
					device.read( (char *) &f32, 4 );
					child->setValue( f32 );
				} break;
				case it_color3f:
				{
					float r, g, b;
					device.read( (char *) &r, 4 );
					device.read( (char *) &g, 4 );
					device.read( (char *) &b, 4 );
					child->setValue( QColor::fromRgbF( r, g, b ) );
				} break;
				case it_color4f:
				{
					float r, g, b, a;
					device.read( (char *) &r, 4 );
					device.read( (char *) &g, 4 );
					device.read( (char *) &b, 4 );
					device.read( (char *) &a, 4 );
					child->setValue( QColor::fromRgbF( r, g, b, a ) );
				} break;
				case it_string:
				{
					int len;
					device.read( (char *) &len, 4 );
					if ( qFromLittleEndian( len ) > 4096 )
						qWarning( "maximum string length exceeded" );
					QByteArray string = device.read( qFromLittleEndian( len ) );
					string.replace( "\r", "\\r" );
					string.replace( "\n", "\\n" );
					child->setValue( QString( string ) );
				} break;
				default:
				{
					qCritical() << "block" << getBlockNumber( parent ) << child->name() << "unknown type" << child->name();
					return false;
				}
			}
		}
	}
	return true;
}

bool NifModel::save( NifItem * parent, QIODevice & device )
{
	if ( ! parent ) return false;
	
	//qDebug() << "saving branch" << parent->name();
	
	for ( int row = 0; row < parent->childCount(); row++ )
	{
		NifItem * child = parent->child( row );
		if ( child && evalCondition( child ) )
		{
			
			bool isChildLink;
			if ( itemIsLink( child, &isChildLink ) )
			{
				if ( ! isChildLink && child->value().toInt() < 0 )
					qWarning() << "block" << getBlockNumber( parent ) << child->name() << "unassigned parent link";
				else if ( child->value().toInt() >= getBlockCount() )
					qWarning() << "block" << getBlockNumber( parent ) << child->name() << "invalid link";
			}
			
			if ( ! child->arr1().isEmpty() || ! child->arr2().isEmpty() || child->childCount() > 0 )
			{
				if ( ! child->arr1().isEmpty() && child->childCount() != getArraySize( child ) )
					qWarning() << "block" << getBlockNumber( parent ) << child->name() << "array size mismatch";
				
				if ( !save( child, device ) )
					return false;
			}
			else switch ( getInternalType( child->type() ) )
			{
				case it_uint8:
				{
					quint8 u8 = (quint8) child->value().toUInt();
					device.write( (char *) &u8, 1 );
				} break;
				case it_uint16:
				{
					quint16 u16 = qToLittleEndian( (quint16) child->value().toUInt() );
					device.write( (char *) &u16, 2 );
				} break;
				case it_uint32:
				{
					quint32 u32 = qToLittleEndian( (quint32) child->value().toUInt() );
					device.write( (char *) &u32, 4 );
				} break;
				case it_int8:
				{
					qint8 s8 = (qint8) child->value().toInt();
					device.write( (char *) &s8, 4 );
				} break;
				case it_int16:
				{
					qint16 s16 = qToLittleEndian( (qint16) child->value().toInt() );
					device.write( (char *) &s16, 4 );
				} break;
				case it_int32:
				{
					qint32 s32 = qToLittleEndian( (qint32) child->value().toInt() );
					device.write( (char *) &s32, 4 );
				} break;
				case it_float:
				{
					float f32 = child->value().toDouble();
					device.write( (char *) &f32, 4 );
				} break;
				case it_color3f:
				{
					QColor rgb = child->value().value<QColor>();
					float r = rgb.redF();					
					float g = rgb.greenF();					
					float b = rgb.blueF();					
					device.write( (char *) &r, 4 );
					device.write( (char *) &g, 4 );
					device.write( (char *) &b, 4 );
				} break;
				case it_color4f:
				{
					QColor rgba = child->value().value<QColor>();
					float r = rgba.redF();					
					float g = rgba.greenF();					
					float b = rgba.blueF();					
					float a = rgba.alphaF();
					device.write( (char *) &r, 4 );
					device.write( (char *) &g, 4 );
					device.write( (char *) &b, 4 );
					device.write( (char *) &a, 4 );
				} break;
				case it_string:
				{
					QByteArray string = child->value().toString().toAscii();
					string.replace( "\\r", "\r" );
					string.replace( "\\n", "\n" );
					int len = qToLittleEndian( string.length() );
					device.write( (char *) &len, 4 );
					device.write( (const char *) string, string.length() );
				} break;
				default:
				{
					qCritical() << "block" << getBlockNumber( parent ) << child->name() << "unknown type" << child->type();
					return false;
				}
			}
		}
	}
	return true;
}

NifBasicType * NifModel::getType( const QString & name ) const
{
	int c = types.count( name );
	if ( c == 1 )
		return types.value( name );
	else if ( c > 1 )
	{
		QList<NifBasicType*> tlst = types.values( name );
		foreach ( NifBasicType * typ, tlst )
		{
			if ( ( typ->ver1 == 0 || version >= typ->ver1 ) && ( typ->ver2 == 0 || version <= typ->ver2 ) )
				return typ;
		}
	}
	
	return 0;
}

NifItem * NifModel::insertBranch( NifItem * parentItem, const NifData & data, int at )
{
	NifItem * item = parentItem->insertChild( data, at );
	item->setValue( QVariant() );
	return item;
}

/*
 *  link functions
 */

void NifModel::updateLinks( int block )
{
	if ( block >= 0 )
	{
		childLinks[ block ].clear();
		parentLinks[ block ].clear();
		updateLinks( block, getBlockItem( block ) );
	}
	else
	{
		rootLinks.clear();
		childLinks.clear();
		parentLinks.clear();
		
		for ( int c = 0; c < getBlockCount(); c++ )
			updateLinks( c );
		
		for ( int c = 0; c < getBlockCount(); c++ )
		{
			QStack<int> stack;
			checkLinks( c, stack );
		}
		
		for ( int c = 0; c < getBlockCount(); c++ )
		{
			bool isRoot = true;
			for ( int d = 0; d < getBlockCount(); d++ )
			{
				if ( c != d && childLinks.value( d ).contains( c ) )
				{
					isRoot = false;
					break;
				}
			}
			if ( isRoot )
				rootLinks.append( c );
		}
	}
}

void NifModel::updateLinks( int block, NifItem * parent )
{
	if ( ! parent ) return;
	
	for ( int r = 0; r < parent->childCount(); r++ )
	{
		NifItem * child = parent->child( r );
		
		bool ischild;
		bool islink = itemIsLink( child, &ischild );
		
		if ( child->childCount() > 0 )
		{
			if ( islink || child->arr1().isEmpty() )
				updateLinks( block, child );
		}
		else if ( islink )
		{
			int l = child->value().toInt();
			if ( l >= 0 && child->arr1().isEmpty() )
			{
				if ( ischild )
				{
					if ( !childLinks[block].contains( l ) ) childLinks[block].append( l );
				}
				else
				{
					if ( !parentLinks[block].contains( l ) ) parentLinks[block].append( l );
				}
			}
		}
	}
}

void NifModel::checkLinks( int block, QStack<int> & parents )
{
	parents.push( block );
	foreach ( int child, childLinks.value( block ) )
	{
		if ( parents.contains( child ) )
		{
			qWarning() << "infinite recursive link construct detected" << block << "->" << child;
			childLinks[block].removeAll( child );
		}
		else
		{
			checkLinks( child, parents );
		}
	}
	parents.pop();
}

void NifModel::adjustLinks( NifItem * parent, int block, int delta )
{
	if ( ! parent ) return;
	
	if ( parent->childCount() > 0 )
	{
		for ( int c = 0; c < parent->childCount(); c++ )
			adjustLinks( parent->child( c ), block, delta );
	}
	else
	{
		int l = itemLink( parent );
		if ( l >= 0 && ( ( delta != 0 && l >= block ) || l == block ) )
		{
			if ( delta == 0 )
				parent->setValue( -1 );
			else
				parent->setValue( l + delta );
		}
	}
}
	
NifItem * NifModel::getItem( NifItem * item, const QString & name ) const
{
	if ( ! item || item == root )		return 0;
	
	//qDebug() << "getItem(" << item->name() << ", " << name << ")";
	
	if ( name.startsWith( "(" ) && name.endsWith( ")" ) )
		return getItem( item->parent(), name.mid( 1, name.length() - 2 ).trimmed() );
	
	for ( int c = 0; c < item->childCount(); c++ )
	{
		NifItem * child = item->child( c );
		
		if ( child->name() == name && evalCondition( child ) )
			return child;
		//if ( child->childCount() > 0 && child->arr1().isEmpty() )
		//{
		//	NifItem * i = getItem( child, name );
		//	if ( i ) return i;
		//}
	}
	return 0;
}

QModelIndex NifModel::getIndex( const QModelIndex & parent, const QString & name ) const
{
	NifItem * parentItem = static_cast<NifItem*>( parent.internalPointer() );
	if ( ! ( parent.isValid() && parentItem && parent.model() == this ) )
		return QModelIndex();
	
	NifItem * item = getItem( parentItem, name );
	if ( item )
		return createIndex( item->row(), 0, item );
	else
		return QModelIndex();
}

QVariant NifModel::getValue( NifItem * parent, const QString & name ) const
{
	NifItem * item = getItem( parent, name );
	if ( item )
		return item->value();
	else
		return QVariant();
}

QVariant NifModel::getValue( const QModelIndex & parent, const QString & name ) const
{
	NifItem * parentItem = static_cast<NifItem*>( parent.internalPointer() );
	if ( ! ( parent.isValid() && parentItem && parent.model() == this ) )
		return QVariant();
	
	return getValue( parentItem, name );
}

int NifModel::getInt( NifItem * parent, const QString & nameornumber ) const
{
	if ( nameornumber.isEmpty() )
		return 0;
	
	bool ok;
	int i = nameornumber.toInt( &ok );
	if ( ok )	return i;
	
	QVariant v = getValue( parent, nameornumber );
	if ( ! v.canConvert( QVariant::Int ) )
		qWarning( "failed to get int for %s ('%s','%s')", str( nameornumber ), str( v.toString() ), v.typeName() );
	return v.toInt();
}

int NifModel::getInt( const QModelIndex & parent, const QString & nameornumber ) const
{
	NifItem * parentItem = static_cast<NifItem*>( parent.internalPointer() );
	if ( ! ( parent.isValid() && parentItem && parent.model() == this ) )
		return 0;
	
	return getInt( parentItem, nameornumber );
}

float NifModel::getFloat( const QModelIndex & parent, const QString & nameornumber ) const
{
	if ( nameornumber.isEmpty() )
		return 0;
	
	bool ok;
	double d = nameornumber.toDouble( &ok );
	if ( ok )	return d;
	
	QVariant v = getValue( parent, nameornumber );
	if ( ! v.canConvert( QVariant::Double ) )
		qWarning( "failed to get float value for %s ('%s','%s')", str( nameornumber ), str( v.toString() ), v.typeName() );
	return v.toDouble();
}

bool NifModel::evalCondition( NifItem * item, bool chkParents ) const
{
	if ( item == root )
		return true;
	
	if ( chkParents && item->parent() )
		if ( ! evalCondition( item->parent(), true ) )
			return false;
	
	if ( ! item->evalVersion( version ) )
		return false;
	
	QString cond = item->cond();
	
	if ( cond.isEmpty() )
		return true;
	
	//qDebug( "evalCondition( '%s' )", str( cond ) );
	
	QString left, right;
	
	static const char * const exp[] = { "!=", "==" };
	static const int num_exp = 2;
	
	int c;
	for ( c = 0; c < num_exp; c++ )
	{
		int p = cond.indexOf( exp[c] );
		if ( p > 0 )
		{
			left = cond.left( p ).trimmed();
			right = cond.right( cond.length() - p - 2 ).trimmed();
			break;
		}
	}
	
	if ( c >= num_exp )
	{
		qCritical( "could not eval condition '%s'", str( cond ) );
		return false;
	}
	
	int l = getInt( item->parent(), left );
	int r = getInt( item->parent(), right );
	
	if ( c == 0 )
	{
		//qDebug( "evalCondition '%s' (%i) != '%s' (%i) : %i", str( left ), l, str( right ), r, l != r );
		return l != r;
	}
	else
	{
		//qDebug( "evalCondition '%s' (%i) == '%s' (%i) : %i", str( left ), l, str( right ), r, l == r );
		return l == r;
	}
	
}

bool NifModel::evalCondition( const QModelIndex & index, bool chkParents ) const
{
	NifItem * item = static_cast<NifItem*>( index.internalPointer() );
	if ( index.isValid() && index.model() == this && item )
		return evalCondition( item, chkParents );
	return false;
}


