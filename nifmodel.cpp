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
#include "niftypes.h"

#include <QApplication>
#include <QByteArray>
#include <QColor>
#include <QDebug>
#include <QProgressDialog>
#include <QTime>

#include <qendian.h>


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
	reset();
	NifItem * item = getItem( getHeaderItem(), "Version" );
	if ( item ) item->value().setFileVersion( version );
	item = getItem( getHeaderItem(), "Header String" );
	if ( item ) item->value().set<QString>( "NetImmerse File Format, Version 4.0.0.2" );
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
	NifItem * header = getHeaderItem();
	
	set<int>( header, "Num Blocks", getBlockCount() );
	
	NifItem * idxBlockTypes = getItem( header, "Block Types" );
	NifItem * idxBlockTypeIndices = getItem( header, "Block Type Index" );
	if ( idxBlockTypes && idxBlockTypeIndices )
	{
		QStringList blocktypes;
		QList<int>	blocktypeindices;
	
		for ( int r = 1; r < root->childCount(); r++ )
		{
			NifItem * block = root->child( r );

			if ( ! blocktypes.contains( block->name() ) )
				blocktypes.append( block->name() );
			blocktypeindices.append( blocktypes.indexOf( block->name() ) );
		}
		
		set<int>( header, "Num Block Types", blocktypes.count() );
		
		updateArray( idxBlockTypes, false );
		updateArray( idxBlockTypeIndices, false );
		
		for ( int r = 0; r < idxBlockTypes->childCount(); r++ )
			idxBlockTypes->child( r )->value().set<QString>( blocktypes.value( r ) );
		
		for ( int r = 0; r < idxBlockTypeIndices->childCount(); r++ )
			idxBlockTypeIndices->child( r )->value().setCount( blocktypeindices.value( r ) );
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
			d1 = dim1->value().toCount();
		else
		{
			NifItem * item = dim1->child( array->row() );
			if ( item )
				d1 = item->value().toCount();
			else
				qCritical() << "failed to get array size for array " << array->name();
		}
	}
	if ( d1 < 0 )
	{
		qWarning() << "invalid array size for array" << array->name();
		d1 = 0;
	}
	return d1;
}

QString inBrakets( const QString & x )
{
	for ( int c = 0; c < x.length(); c++ )
		if ( ! x[c].isNumber() )
			return QString( "(%1)" ).arg( x );
	return x;
}

bool NifModel::updateArray( NifItem * array, bool fast )
{
	if ( array->arr1().isEmpty() )
		return false;
	
	int d1 = getArraySize( array );
	if ( d1 > 1024 * 1024 * 8 )
	{
		qWarning() << "array" << array->name() << "much too large";
		return false;
	}

	int rows = array->childCount();
	if ( d1 > rows )
	{
		NifData data( array->name(), array->type(), NifValue( NifValue::type( array->type() ) ), inBrakets( array->arg() ), inBrakets( array->arr2() ), QString(), QString(), 0, 0 );
		
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
	if ( !fast && d1 != rows && ( isCompound( array->type() ) || NifValue::isLink( NifValue::type( array->type() ) ) ) )
	{
		updateLinks();
		emit linksChanged();
	}
	return true;
}

bool NifModel::updateArray( const QModelIndex & array, bool fast )
{
	NifItem * item = static_cast<NifItem*>( array.internalPointer() );
	if ( ! ( array.isValid() && item && array.model() == this ) )
		return false;
	return updateArray( item, fast );
}

/*
 *  block functions
 */

QModelIndex NifModel::insertNiBlock( const QString & identifier, int at, bool fast )
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
		{
			updateHeader();
			updateLinks();
			emit linksChanged();
		}
		return createIndex( branch->row(), 0, branch );
	}
	else
	{
		qCritical() << "unknown block " << identifier;
		return QModelIndex();
	}
}

void NifModel::removeNiBlock( int blocknum )
{
	if ( blocknum < 0 || blocknum >= getBlockCount() )
		return;
	
	adjustLinks( root, blocknum, 0 );
	adjustLinks( root, blocknum, -1 );
	beginRemoveRows( QModelIndex(), blocknum+1, blocknum+1 );
	root->removeChild( blocknum + 1 );
	endRemoveRows();
	updateHeader();
	updateLinks();
	emit linksChanged();
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

QModelIndex NifModel::getBlock( const QModelIndex & idx, const QString & id ) const
{
	return getBlock( getBlockNumber( idx ), id );
}

QModelIndex NifModel::getBlockOrHeader( const QModelIndex & idx ) const
{
	QModelIndex block = idx;
	while ( block.isValid() && block.parent().isValid() )
		block = block.parent();
	return block;
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
	if ( x < 0 )
		return QModelIndex();
	x += 1; //the first block is the NiHeader
	QModelIndex idx = index( x, 0 );
	if ( ! name.isEmpty() )
		return ( itemName( idx ) == name ? idx : QModelIndex() );
	else
		return idx;
}

NifItem * NifModel::getBlockItem( int x ) const
{
	if ( x < 0 )	return 0;
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

bool NifModel::inherits( const QModelIndex & index, const QString & aunty )
{
	QModelIndex block = getBlock( index );
	if ( block.isValid() )
		return inherits( itemName( block ), aunty );
	else
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
				if ( d.arr1().contains( "(ARG)" ) ) { QString x = d.arr1(); x.replace( x.indexOf( "(ARG)" ), 5, arg ); d.setArr1( x ); }
				if ( d.arr2().contains( "(ARG)" ) ) { QString x = d.arr2(); x.replace( x.indexOf( "(ARG)" ), 5, arg ); d.setArr2( x ); }
				if ( d.cond().contains( "(ARG)" ) ) { QString x = d.cond(); x.replace( x.indexOf( "(ARG)" ), 5, arg ); d.setCond( x ); }
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

NifValue NifModel::itemValue( const QModelIndex & index ) const
{
	NifItem * item = static_cast<NifItem*>( index.internalPointer() );
	if ( ! ( index.isValid() && item && index.model() == this ) )	return NifValue();
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
	return item->value().toLink();
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
		*isChildLink = ( item->value().type() == NifValue::tLink );
	return item->value().isLink();
}

bool NifModel::itemIsLink( const QModelIndex & index, bool * isChildLink ) const
{
	NifItem * item = static_cast<NifItem*>( index.internalPointer() );
	if ( ! ( index.isValid() && item && index.model() == this ) )	return false;
	return itemIsLink( item, isChildLink );
}

bool NifModel::setItemValue( NifItem * item, const NifValue & val )
{
	item->value() = val;
	emit dataChanged( createIndex( item->row(), ValueCol, item ), createIndex( item->row(), ValueCol, item ) );
	if ( itemIsLink( item ) )
	{
		updateLinks();
		emit linksChanged();
	}
	return true;
}

bool NifModel::setItemValue( const QModelIndex & index, const NifValue & val )
{
	NifItem * item = static_cast<NifItem*>( index.internalPointer() );
	if ( ! ( index.isValid() && item && index.model() == this ) )	return false;
	return setItemValue( item, val );
}

bool NifModel::setValue( const QModelIndex & parent, const QString & name, const NifValue & val )
{
	if ( ! parent.isValid() )
	{
		qWarning() << "setValue(" << name << ") reached top level";
		return false;
	}
	
	if ( name.startsWith( "(" ) && name.endsWith( ")" ) )
		return setValue( parent.parent(), name.mid( 1, name.length() - 2 ).trimmed(), val );
	
	for ( int c = 0; c < rowCount( parent ); c++ )
	{
		QModelIndex child = parent.child( c, 0 );
		if ( itemName( child ) == name && evalCondition( child ) )
		{
			setItemValue( child, val );
			return true;
		}
		if ( rowCount( child ) > 0 && itemArr1( child ).isEmpty() )
		{
			if ( setValue( child, name, val ) ) return true;
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

QVariant NifModel::data( const QModelIndex & idx, int role ) const
{
	QModelIndex index = buddy( idx );
	
	NifItem * item = static_cast<NifItem*>( index.internalPointer() );
	if ( ! ( index.isValid() && item && index.model() == this ) )
		return QVariant();
	
	int column = index.column();
	
	if ( column == ValueCol && item->parent() == root && item->type() == "NiBlock" )
	{
		QModelIndex buddy;
		if ( item->name() == "NiSourceTexture" )
			buddy = getIndex( getIndex( index, "Texture Source" ), "File Name" );
		else
			buddy = getIndex( index, "Name" );
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
					if ( item->value().isLink() )
					{
						int lnk = item->value().toLink();
						if ( lnk >= 0 )
						{
							QModelIndex block = getBlock( lnk );
							if ( block.isValid() )
							{
								QModelIndex block_name = getIndex( block, "Name" );
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
						else
							return QString( "None" );
					}
					else
						return item->value().toVariant();
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
				case ValueCol:	return item->value().toVariant();
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
			switch ( column )
			{
				case ValueCol:
				{
					switch ( item->value().type() )
					{
						case NifValue::tMatrix:
							return item->value().get<Matrix>().toHtml();
						case NifValue::tQuat:
							return item->value().get<Quat>().toHtml();
						default:
							break;
					}
				}	break;
				default:
					break;
			}
		}	return QVariant();
		case Qt::BackgroundColorRole:
		{
			if ( column == ValueCol && item->value().isColor() )
			{
				return item->value().toColor();
			}
		}	return QVariant();
		case Qt::UserRole:
		{
			if ( column == ValueCol )
			{
				if ( item->value().isColor() )
					return "Color/Choose";
				else if ( item->name() == "File Name" )
					return "Texture/Choose";
			}
		}	return QVariant();
		default:
			return QVariant();
	}
}

bool NifModel::setData( const QModelIndex & index, const QVariant & value, int role )
{
	NifItem * item = static_cast<NifItem*>( index.internalPointer() );
	if ( ! ( index.isValid() && role == Qt::EditRole && index.model() == this && item ) )
		return false;
	
	// buddy lookup
	if ( index.column() == ValueCol && item->parent() == root && item->type() == "NiBlock" )
	{
		QModelIndex buddy;
		if ( item->name() == "NiSourceTexture" )
			buddy = getIndex( getIndex( index, "Texture Source" ), "File Name" );
		else
			buddy = getIndex( index, "Name" );
		if ( buddy.isValid() )
			buddy = buddy.sibling( buddy.row(), index.column() );
		if ( buddy.isValid() )
			return setData( buddy, value, role );
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
			item->value().fromVariant( value );
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

	// reverse buddy lookup (perhaps I'd better think of a way how to make the mapping of values to the block level more efficient)
	if ( index.column() == ValueCol )
	{
		if ( item->name() == "File Name" )
		{
			NifItem * parent = item->parent();
			if ( parent && parent->name() == "Texture Source" )
			{
				parent = parent->parent();
				if ( parent && parent->type() == "NiBlock" && parent->name() == "NiSourceTexture" )
					emit dataChanged( createIndex( parent->row(), ValueCol, parent ), createIndex( parent->row(), ValueCol, parent ) );
			}
		}
		else if ( item->name() == "Name" )
		{
			NifItem * parent = item->parent();
			if ( parent && parent->type() == "NiBlock" )
				emit dataChanged( createIndex( parent->row(), ValueCol, parent ), createIndex( parent->row(), ValueCol, parent ) );
		}
	}

	// update original index
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
	
	quint64 filepos = device.pos();
	
	QByteArray version_string;
	
	{	// read magic version string
		version_string.clear();
		int c = 0;
		char chr = 0;
		while ( c++ < 80 && device.getChar( &chr ) && chr != '\n' )
			version_string.append( chr );
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
	
	// verify version number
	if ( ! supportedVersions.contains( version ) )
	{
		qCritical() << "version" << version2string( version ) << "is not supported yet";
		clear();
		return false;
	}
	
	// now start reading from the beginning of the file
	device.seek( filepos );
	NifStream stream( version, &device );

	// read header
	if ( !load( getHeaderItem(), stream, true ) )
	{
		qCritical() << "failed to load file header (version" << version << ")";
		clear();
		return false;
	}
	
	QModelIndex header = index( 0, 0 );
	
	int numblocks = get<int>( header, "Num Blocks" );
	qDebug( "numblocks %i", numblocks );
	
	qApp->processEvents();

	QProgressDialog prog;
	prog.setLabelText( "loading nif..." );
	prog.setRange( 0, numblocks );
	prog.setValue( 0 );
	prog.setMinimumDuration( 2100 );
	
	//QTime t = QTime::currentTime();

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
				
				int blktypidx = itemValue( index( c, 0, getIndex( header, "Block Type Index" ) ) ).toCount();
				blktyp = itemValue( index( blktypidx, 0, getIndex( header, "Block Types" ) ) ).toString();
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
				if ( ! load( root->child( c+1 ), stream, true ) ) 
					throw QString( "failed to load block number %1 (%2)" ).arg( c ).arg( blktyp );
			}
			else
				throw QString( "encountered unknown block (%1)" ).arg( blktyp );
		}
	}
	catch ( QString err )
	{
		qCritical() << (const char *) err.toAscii();
		reset();
		return false;
	}
	//qDebug() << t.msecsTo( QTime::currentTime() );
	reset(); // notify model views that a significant change to the data structure has occurded
	return true;
}

bool NifModel::save( QIODevice & device )
{
	NifStream stream( version, &device );
	
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
		if ( !save( root->child( c ), stream ) )
		{
			qCritical() << "failed to write block" << itemName( index( c, 0 ) ) << "(" << c-1 << ")";
			return false;
		}
	}
	
	int rcnt = rootLinks.count();
	device.write( (char *) &rcnt, 4 );
	foreach ( int rlnk, rootLinks )
		device.write( (char *) &rlnk, 4 );
	
	return true;
}

bool NifModel::load( QIODevice & device, const QModelIndex & index )
{
	NifItem * item = static_cast<NifItem*>( index.internalPointer() );
	if ( item && index.isValid() && index.model() == this )
	{
		NifStream stream( version, &device );
		load( item, stream, false );
		updateLinks();
		emit linksChanged();
		return true;
	}
	reset();
	return false;
}

bool NifModel::save( QIODevice & device, const QModelIndex & index )
{
	NifStream stream( version, &device );
	NifItem * item = static_cast<NifItem*>( index.internalPointer() );
	return ( item && index.isValid() && index.model() == this && save( item, stream ) );
}

bool NifModel::load( NifItem * parent, NifStream & stream, bool fast )
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
				if ( ! updateArray( child, fast ) )
					return false;
				
				if ( !load( child, stream, fast ) )
					return false;
			}
			else if ( child->childCount() > 0 )
			{
				if ( !load( child, stream, fast ) )
					return false;
			}
			else
			{
				if ( ! stream.read( child->value() ) )
					return false;
				//qDebug() << "  loaded item" << child->name() << child->value().type() << child->value().toQString();
			}
		}
	}
	return true;
}

bool NifModel::save( NifItem * parent, NifStream & stream )
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
				if ( ! isChildLink && child->value().toLink() < 0 )
					qWarning() << "block" << getBlockNumber( parent ) << child->name() << "unassigned parent link";
				else if ( child->value().toLink() >= getBlockCount() )
					qWarning() << "block" << getBlockNumber( parent ) << child->name() << "invalid link";
			}
			
			if ( ! child->arr1().isEmpty() || ! child->arr2().isEmpty() || child->childCount() > 0 )
			{
				if ( ! child->arr1().isEmpty() && child->childCount() != getArraySize( child ) )
					qWarning() << "block" << getBlockNumber( parent ) << child->name() << "array size mismatch";
				
				if ( !save( child, stream ) )
					return false;
			}
			else
			{
				if ( ! stream.write( child->value() ) )
					return false;
			}
		}
	}
	return true;
}

NifItem * NifModel::insertBranch( NifItem * parentItem, const NifData & data, int at )
{
	NifItem * item = parentItem->insertChild( data, at );
	item->value().changeType( NifValue::tNone );
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
			updateLinks( block, child );
		}
		else if ( islink )
		{
			int l = child->value().toLink();
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
				parent->value().setLink( -1 );
			else
				parent->value().setLink( l + delta );
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
	}
	
	//qDebug() << "item" << name << "not found";
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

int NifModel::getInt( NifItem * parent, const QString & nameornumber ) const
{
	if ( nameornumber.isEmpty() )
		return 0;
	
	bool ok;
	int i = nameornumber.toInt( &ok );
	if ( ok )	return i;
	
	NifItem * item = getItem( parent, nameornumber );
	if ( item )
		return item->value().toCount();

	qWarning() << "failed to get int for" << nameornumber;
	return 0;
}

int NifModel::getLink( const QModelIndex & parent, const QString & name ) const
{
	NifItem * parentItem = static_cast<NifItem*>( parent.internalPointer() );
	if ( ! ( parent.isValid() && parentItem && parent.model() == this ) )
		return -1;
	
	NifItem * item = getItem( parentItem, name );
	if ( item )
		return item->value().toLink();
	
	return -1;
}

bool NifModel::setLink( const QModelIndex & parent, const QString & name, qint32 l )
{
	NifItem * parentItem = static_cast<NifItem*>( parent.internalPointer() );
	if ( ! ( parent.isValid() && parentItem && parent.model() == this ) )
		return false;
	
	NifItem * item = getItem( parentItem, name );
	if ( item && item->value().setLink( l ) )
	{
		if ( itemIsLink( item ) )
		{
			updateLinks();
			emit linksChanged();
		}
		return true;
	}
	else
		return false;
}

bool NifModel::setLink( const QModelIndex & index, qint32 l )
{
	NifItem * item = static_cast<NifItem*>( index.internalPointer() );
	if ( ! ( index.isValid() && item && index.model() == this ) )
		return false;

	if ( item && item->value().setLink( l ) )
	{
		if ( itemIsLink( item ) )
		{
			updateLinks();
			emit linksChanged();
		}
		return true;
	}
	else
		return false;
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
	
	static const char * const exp[] = { "!=", "==", ">=", "<=", "<", ">" };
	static const int num_exp = 6;
	
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
		qCritical() << "could not eval condition" << cond;
		return false;
	}
	
	int l = getInt( item->parent(), left );
	int r = getInt( item->parent(), right );
	
	switch ( c )
	{
		case 0: return l != r;
		case 1: return l == r;
		case 2: return l >= r;
		case 3: return l <= r;
		case 4: return l > r;
		case 5: return l < r;
		default:
			qCritical() << "could not eval condition" << cond;
			return false;
	}
}

bool NifModel::evalCondition( const QModelIndex & index, bool chkParents ) const
{
	NifItem * item = static_cast<NifItem*>( index.internalPointer() );
	if ( index.isValid() && index.model() == this && item )
		return evalCondition( item, chkParents );
	return false;
}

int NifModel::getParent( int block ) const
{
	int parent = -1;
	for ( int b = 0; b < getBlockCount(); b++ )
	{
		if ( childLinks.value( b ).contains( block ) )
		{
			if ( parent < 0 )
				parent = b;
			else
				return -1;
		}
	}
	return parent;
}
