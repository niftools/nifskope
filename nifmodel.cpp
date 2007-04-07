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

#include "spellbook.h"

#include <QByteArray>
#include <QColor>
#include <QFile>
#include <QTime>

NifModel::NifModel( QObject * parent ) : BaseModel( parent )
{
	clear();
}

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
	
	if ( s.contains( "." ) )
	{
		QStringList l = s.split( "." );
		quint32 v = 0;
		for ( int i = 0; i < 4 && i < l.count(); i++ )
			v += l[i].toInt( 0, 10 ) << ( (3-i) * 8 );
		return v;
	}
	else
	{
		bool ok;
		quint32 i = s.toUInt( &ok );
		return ( i == 0xffffffff ? 0 : i );
	}
}

bool NifModel::evalVersion( NifItem * item, bool chkParents ) const
{
	if ( item == root )
		return true;
	
	if ( chkParents && item->parent() )
		if ( ! evalVersion( item->parent(), true ) )
			return false;
	
	return item->evalVersion( version );
}

void NifModel::clear()
{
	folder = QString();
	root->killChildren();
	insertType( root, NifData( "NiHeader", "Header" ) );
	insertType( root, NifData( "NiFooter", "Footer" ) );
	version = version2number( "20.0.0.5" );
	reset();
	NifItem * item = getItem( getHeaderItem(), "Version" );
	if ( item ) item->value().setFileVersion( version );
	
	set<QString>( getHeaderItem(), "Header String", "Gamebryo File Format, Version 20.0.0.5" );
	set<int>( getHeaderItem(), "User Version", 11 );
	set<int>( getHeaderItem(), "Unknown Int 3", 11 );
}

/*
 *  footer functions
 */
 
NifItem * NifModel::getFooterItem() const
{
	return root->child( root->childCount() - 1 );
}

QModelIndex NifModel::getFooter() const
{
	NifItem * footer = getFooterItem();
	if ( footer )
		return createIndex( footer->row(), 0, footer );
	else
		return QModelIndex();
}

void NifModel::updateFooter()
{
	NifItem * footer = getFooterItem();
	if ( ! footer ) return;
	NifItem * roots = getItem( footer, "Roots" );
	if ( ! roots ) return;
	set<int>( footer, "Num Roots", rootLinks.count() );
	updateArrayItem( roots, false );
	for ( int r = 0; r < roots->childCount(); r++ )
		roots->child( r )->value().setLink( rootLinks.value( r ) );
}

/*
 *  header functions
 */

QModelIndex NifModel::getHeader() const
{
	QModelIndex header = index( 0, 0 );
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
		
		for ( int r = 1; r < root->childCount() - 1; r++ )
		{
			NifItem * block = root->child( r );
			
			if ( ! blocktypes.contains( block->name() ) )
				blocktypes.append( block->name() );
			blocktypeindices.append( blocktypes.indexOf( block->name() ) );
		}
		
		set<int>( header, "Num Block Types", blocktypes.count() );
		
		updateArrayItem( idxBlockTypes, false );
		updateArrayItem( idxBlockTypeIndices, false );
		
		for ( int r = 0; r < idxBlockTypes->childCount(); r++ )
			set<QString>( idxBlockTypes->child( r ), blocktypes.value( r ) );
		
		for ( int r = 0; r < idxBlockTypeIndices->childCount(); r++ )
			set<int>( idxBlockTypeIndices->child( r ), blocktypeindices.value( r ) );
	}
}

/*
 *  search and find
 */

NifItem * NifModel::getItem( NifItem * item, const QString & name ) const
{
	if ( name.startsWith( "HEADER/" ) )
		return getItem( getHeaderItem(), name.right( name.length() - 7 ) );
	
	if ( ! item || item == root )		return 0;
	
	int slash = name.indexOf( "/" );
	if ( slash > 0 )
	{
		QString left = name.left( slash );
		QString right = name.right( name.length() - slash - 1 );
		
		if ( left == ".." )
			return getItem( item->parent(), right );
		else
			return getItem( getItem( item, left ), right );
	}
	
	for ( int c = 0; c < item->childCount(); c++ )
	{
		NifItem * child = item->child( c );
		
		if ( child->name() == name && evalCondition( child ) )
			return child;
	}
	
	return 0;
}

/*
 *  array functions
 */
 
static QString parentPrefix( const QString & x )
{
	for ( int c = 0; c < x.length(); c++ )
		if ( ! x[c].isNumber() )
			return QString( "../" ) + x;
	return x;
}

bool NifModel::updateArrayItem( NifItem * array, bool fast )
{
	if ( array->arr1().isEmpty() )
		return false;
	
	int d1 = getArraySize( array );
	if ( d1 > 1024 * 1024 * 8 )
	{
		msg( Message() << "array" << array->name() << "much too large" );
		return false;
	}

	int rows = array->childCount();
	if ( d1 > rows )
	{
		NifData data( array->name(), array->type(), array->temp(), NifValue( NifValue::type( array->type() ) ), parentPrefix( array->arg() ), parentPrefix( array->arr2() ), QString(), QString(), 0, 0 );
		
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
		NifItem * parent = array;
		while ( parent->parent() && parent->parent() != root )
			parent = parent->parent();
		if ( parent != getFooterItem() )
		{
			updateLinks();
			updateFooter();
			emit linksChanged();
		}
	}
	return true;
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
		
		if ( at >= 0 )
			at++;
		else
			at = getBlockCount() + 1;
		
		if ( ! fast ) beginInsertRows( QModelIndex(), at, at );
		NifItem * branch = insertBranch( root, NifData( identifier, "NiBlock", block->text ), at );
		if ( ! fast ) endInsertRows();
		
		if ( ! block->ancestor.isEmpty() )
			insertAncestor( branch, block->ancestor );
		
		branch->prepareInsert( block->types.count() );
		
		foreach ( NifData data, block->types )
			insertType( branch, data );
		
		if ( ! fast )
		{
			updateHeader();
			updateLinks();
			updateFooter();
			emit linksChanged();
		}
		return createIndex( branch->row(), 0, branch );
	}
	else
	{
		msg( Message() << "unknown block " << identifier );
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
	updateFooter();
	emit linksChanged();
}

void NifModel::moveNiBlock( int src, int dst )
{
	if ( src < 0 || src >= getBlockCount() )
		return;
	
	beginRemoveRows( QModelIndex(), src+1, src+1 );
	NifItem * block = root->takeChild( src+1 );
	endRemoveRows();
	
	if ( dst >= 0 )
		dst++;
	
	beginInsertRows( QModelIndex(), dst, dst );
	dst = root->insertChild( block, dst ) - 1;
	endInsertRows();
	
	QMap<qint32,qint32> map;
	if ( src < dst )
		for ( int l = src; l <= dst; l++ )
			map.insert( l, l-1 );
	else
		for ( int l = dst; l <= src; l++ )
			map.insert( l, l+1 );
	
	map.insert( src, dst );
	
	mapLinks( root, map );
	
	updateLinks();
	updateHeader();
	updateFooter();
	emit linksChanged();
}

QMap<qint32,qint32> NifModel::moveAllNiBlocks( NifModel * targetnif )
{
	int bcnt = getBlockCount();
	
	QMap<qint32,qint32> map;
	
	beginRemoveRows( QModelIndex(), 1, bcnt );
	targetnif->beginInsertRows( QModelIndex(), targetnif->getBlockCount(), targetnif->getBlockCount() + bcnt - 1 );
	
	for ( int i = 0; i < bcnt; i++ )
	{
		map.insert( i, targetnif->root->insertChild( root->takeChild( 1 ), targetnif->root->childCount() - 1 ) - 1 );
	}
	
	endRemoveRows();
	targetnif->endInsertRows();
	
	for ( int i = 0; i < bcnt; i++ )
	{
		targetnif->mapLinks( targetnif->root->child( targetnif->getBlockCount() - i ), map );
	}
	
	updateLinks();
	updateHeader();
	updateFooter();
	emit linksChanged();
	
	targetnif->updateLinks();
	targetnif->updateHeader();
	targetnif->updateFooter();
	emit targetnif->linksChanged();
	
	return map;
}

void NifModel::reorderBlocks( const QVector<qint32> & order )
{
	if ( getBlockCount() <= 1 )
		return;
	
	if ( order.count() != getBlockCount() )
	{
		msg( Message() << "NifModel::reorderBlocks() - invalid argument" );
		return;
	}
	
	QMap<qint32,qint32> linkMap;
	QMap<qint32,qint32> blockMap;
	
	for ( qint32 n = 0; n < order.count(); n++ )
	{
		if ( blockMap.contains( order[n] ) || order[n] < 0 || order[n] >= getBlockCount() )
		{
			msg( Message() << "NifModel::reorderBlocks() - invalid argument" );
			return;
		}
		blockMap.insert( order[n], n );
		if ( order[n] != n )
			linkMap.insert( n, order[n] );
	}
	
	if ( linkMap.isEmpty() )
		return;
	
	// take all the blocks
	beginRemoveRows( QModelIndex(), 1, root->childCount() - 2 );
	QList<NifItem*> temp;
	for ( qint32 n = 0; n < order.count(); n++ )
		temp.append( root->takeChild( 1 ) );
	endRemoveRows();
	
	// then insert them again in the new order
	beginInsertRows( QModelIndex(), 1, temp.count() ); 
	foreach ( qint32 n, blockMap )
		root->insertChild( temp[ n ], root->childCount()-1 );
	endInsertRows();
	
	mapLinks( root, linkMap );
	updateLinks();
	emit linksChanged();
	
	updateHeader();
	updateFooter();
}

void NifModel::mapLinks( const QMap<qint32,qint32> & map )
{
	mapLinks( root, map );
	updateLinks();
	emit linksChanged();
	
	updateHeader();
	updateFooter();
}

QString NifModel::getBlockName( const QModelIndex & idx ) const
{
	const NifItem * block = static_cast<NifItem*>( idx.internalPointer() );
	if( block ) {
		return block->name();
	}

	return QString( "" );
}

QString NifModel::getBlockType( const QModelIndex & idx ) const
{
	const NifItem * block = static_cast<NifItem*>( idx.internalPointer() );
	if( block ) {
		return block->type();
	}

	return QString( "" );
}

int NifModel::getBlockNumber( const QModelIndex & idx ) const
{
	if ( ! ( idx.isValid() && idx.model() == this ) )
		return -1;
	
	const NifItem * block = static_cast<NifItem*>( idx.internalPointer() );
	while ( block && block->parent() != root )
		block = block->parent();
	
	if ( ! block )
		return -1;
	
	int num = block->row() - 1;
	if ( num >= getBlockCount() )	num = -1;
	return num;
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

int NifModel::getBlockNumber( NifItem * block ) const
{
	while ( block && block->parent() != root )
		block = block->parent();
	
	if ( ! block )
		return -1;
	
	int num = block->row() - 1;
	if ( num >= getBlockCount() )	num = -1;
	return num;
}

QModelIndex NifModel::getBlock( int x, const QString & name ) const
{
	if ( x < 0 || x >= getBlockCount() )
		return QModelIndex();
	x += 1; //the first block is the NiHeader
	QModelIndex idx = index( x, 0 );
	if ( inherits( idx, name ) )
		return idx;
	else
		return QModelIndex();
}

bool NifModel::isNiBlock( const QModelIndex & index, const QString & name ) const
{
	NifItem * item = static_cast<NifItem *>( index.internalPointer() );
	if ( index.isValid() && item && item->parent() == root && getBlockNumber( item ) >= 0 )
	{
		if ( name.isEmpty() )
			return true;
		else
			return item->name() == name;
	}
	return false;
}

NifItem * NifModel::getBlockItem( int x ) const
{
	if ( x < 0 || x >= getBlockCount() )	return 0;
	return root->child( x+1 );
}

int NifModel::getBlockCount() const
{
	return rowCount() - 2;
}


/*
 *  ancestor functions
 */

void NifModel::insertAncestor( NifItem * parent, const QString & identifier, int at )
{
	NifBlock * ancestor = blocks.value( identifier );
	if ( ancestor )
	{
		if ( ! ancestor->ancestor.isEmpty() )
			insertAncestor( parent, ancestor->ancestor );
		//parent->insertChild( NifData( identifier, "Abstract" ) );
		parent->prepareInsert( ancestor->types.count() );
		foreach ( NifData data, ancestor->types )
			insertType( parent, data );
	}
	else
		msg( Message() << "unknown ancestor " << identifier );
}

bool NifModel::inherits( const QString & name, const QString & aunty )
{
    if ( name == aunty ) return true;
	NifBlock * type = blocks.value( name );
	if ( type && ( type->ancestor == aunty || inherits( type->ancestor, aunty ) ) )
		return true;
	return false;
}

bool NifModel::inherits( const QModelIndex & idx, const QString & aunty ) const
{
	int x = getBlockNumber( idx );
	if ( x < 0 )
		return false;
	return inherits( itemName( index( x+1, 0 ) ), aunty );
}


/*
 *  basic and compound type functions
 */
 
void NifModel::insertType( const QModelIndex & parent, const NifData & data, int at )
{
	NifItem * item = static_cast<NifItem *>( parent.internalPointer() );
	if ( parent.isValid() && item && item != root )
		insertType( item, data, at );
}

void NifModel::insertType( NifItem * parent, const NifData & data, int at )
{
	if ( ! data.arr1().isEmpty() )
	{
		NifItem * array = insertBranch( parent, data, at );
		
		if ( evalCondition( array ) )
			updateArrayItem( array, true );
		return;
	}

	NifBlock * compound = compounds.value( data.type() );
	if ( compound )
	{
		NifItem * branch = insertBranch( parent, data, at );
		branch->prepareInsert( compound->types.count() );
		foreach ( NifData d, compound->types )
			insertType( branch, d );
	}
	else
	{
		if ( data.type() == "TEMPLATE" || data.temp() == "TEMPLATE" )
		{
			QString tmp = parent->temp();
			NifItem * tItem = parent;
			while ( tmp == "TEMPLATE" && tItem->parent() )
			{
				tItem = tItem->parent();
				tmp = tItem->temp();
			}
			NifData d( data );
			if ( d.type() == "TEMPLATE" )
			{
				d.value.changeType( NifValue::type( tmp ) );
				d.setType( tmp );
			}
			if ( d.temp() == "TEMPLATE" )
				d.setTemp( tmp );
			insertType( parent, d, at );
		}
		else
			parent->insertChild( data, at );
	}
}


/*
 *  item value functions
 */

bool NifModel::setItemValue( NifItem * item, const NifValue & val )
{
	item->value() = val;
	emit dataChanged( createIndex( item->row(), ValueCol, item ), createIndex( item->row(), ValueCol, item ) );
	if ( itemIsLink( item ) )
	{
		NifItem * parent = item;
		while ( parent->parent() && parent->parent() != root )
			parent = parent->parent();
		if ( parent != getFooterItem() )
		{
			updateLinks();
			updateFooter();
			emit linksChanged();
		}
	}
	return true;
}

/*
 *  QAbstractModel interface
 */

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
		if ( item->name() == "NiSourceTexture" || item->name() == "NiImage" )
			buddy = getIndex( index, "File Name" );
		else
			buddy = getIndex( index, "Name" );
		if ( buddy.isValid() )
			buddy = buddy.sibling( buddy.row(), index.column() );
		if ( buddy.isValid() )
			return data( buddy, role );
	} 
   else if ( column == ValueCol && item->parent() != root && item->type() == "ControllerLink" && role == Qt::DisplayRole )
   {
      QModelIndex buddy;
      if ( item->name() == "Controlled Blocks" )
      {
         buddy = getIndex( index, "Node Name Offset" );
         if ( buddy.isValid() )
            buddy = buddy.sibling( buddy.row(), index.column() );
         if ( buddy.isValid() )
            return data( buddy, role );
      }
   }


	switch ( role )
	{
		case Qt::DisplayRole:
		{
			switch ( column )
			{
				case NameCol:	return item->name();
				case TypeCol:
				{
					if ( ! item->temp().isEmpty() )
					{
						NifItem * i = item;
						while ( i && i->temp() == "TEMPLATE" )
							i = i->parent();
						return QString( "%1<%2>" ).arg( item->type() ).arg( i ? i->temp() : QString() );
					}
					else
					{
						return item->type();
					}
				}	break;
				case ValueCol:
				{
					if ( item->value().type() == NifValue::tStringOffset )
					{
						int ofs = item->value().get<int>();
						if ( ofs < 0 )
							return QString( "<empty>" );
						NifItem * palette = getItemX( item, "String Palette" );
						int link = ( palette ? palette->value().toLink() : -1 );
						if ( ( palette = getBlockItem( link ) ) && ( palette = getItem( palette, "Palette" ) ) )
						{
							QByteArray bytes = palette->value().get<QByteArray>();
							if ( ofs < bytes.count() )
								return QString( & bytes.data()[ofs] );
							else
								return QString( "<offset invalid>" );
						}
						else
						{
							return QString( "<palette not found>" );
						}
					}
					else if ( item->value().type() == NifValue::tBlockTypeIndex )
					{
						int idx = item->value().get<int>();
						NifItem * blocktypes = getItemX( item, "Block Types" );
						NifItem * blocktyp = ( blocktypes ? blocktypes->child( idx ) : 0 );
						if ( blocktyp )
							return QString( "%2 [%1]").arg( idx ).arg( blocktyp->value().get<QString>() );
						else
							return QString( "%1 - <index invalid>" ).arg( idx );
					}
					else if ( item->value().isLink() )
					{
						int lnk = item->value().toLink();
						if ( lnk >= 0 )
						{
							QModelIndex block = getBlock( lnk );
							if ( block.isValid() )
							{
								QModelIndex block_name = getIndex( block, "Name" );
								if ( block_name.isValid() && ! get<QString>( block_name ).isEmpty() )
									return QString( "%1 (%2)" ).arg( lnk ).arg( get<QString>( block_name ) );
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
					else if ( item->value().isCount() )
					{
						QString optId = NifValue::enumOptionName( item->type(), item->value().toCount() );
						if ( optId.isEmpty() )
							return item->value().toString();
						else
							return QString( "%1" ).arg( optId );
					}
					else
						return item->value().toString();
				}	break;
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
				case NameCol:
				{
					if ( item->parent() && ! item->parent()->arr1().isEmpty() )
					{
						return QString( "array index: %1" ).arg( item->row() );
					}
					else
					{
						QString tip = QString( "<p><b>%1</b></p><p>%2</p>" ).arg( item->name() ).arg( item->text() );
						
						if ( NifBlock * blk = blocks.value( item->name() ) )
						{
							tip += "<p>Ancestors:<ul>";
							while ( blocks.contains( blk->ancestor ) )
							{
								tip += QString( "<li>%1</li>" ).arg( blk->ancestor );
								blk = blocks.value( blk->ancestor );
							}
							tip += "</ul></p>";
						}
						return tip;
					}
				}	break;
				case TypeCol:
					return NifValue::typeDescription( item->type() );
				case ValueCol:
				{
					switch ( item->value().type() )
					{
						case NifValue::tByte:
							{
								quint8 b = item->value().toCount();
								return QString( "dec: %1<br>hex: 0x%2" ).arg( b ).arg( b, 2, 16, QChar( '0' ) );
							}
						case NifValue::tWord:
						case NifValue::tShort:
							{
								quint16 s = item->value().toCount();
								return QString( "dec: %1<br>hex: 0x%2" ).arg( s ).arg( s, 4, 16, QChar( '0' ) );
							}
						case NifValue::tBool:
						case NifValue::tInt:
						case NifValue::tUInt:
							{
								quint32 i = item->value().toCount();
								return QString( "dec: %1<br>hex: 0x%2" ).arg( i ).arg( i, 8, 16, QChar( '0' ) );
							}
						case NifValue::tFloat:
							{
								float f = item->value().toFloat();
								return QString( "float: %1<br>data: 0x%2" ).arg( f ).arg( *( (unsigned int*) &f ), 8, 16, QChar( '0' ) );
							}
						case NifValue::tFlags:
							{
								quint16 f = item->value().toCount();
								return QString( "dec: %1<br>hex: 0x%2<br>bin: 0b%3" ).arg( f ).arg( f, 4, 16, QChar( '0' ) ).arg( f, 16, 2, QChar( '0' ) );
							}
						case NifValue::tStringOffset:
							return QString( "0x%1" ).arg( item->value().toCount(), 8, 16, QChar( '0' ) );
						case NifValue::tVector3:
							return item->value().get<Vector3>().toHtml();
						case NifValue::tMatrix:
							return item->value().get<Matrix>().toHtml();
						case NifValue::tMatrix4:
							return item->value().get<Matrix4>().toHtml();
						case NifValue::tQuat:
						case NifValue::tQuatXYZW:
							return item->value().get<Quat>().toHtml();
						case NifValue::tColor3:
							{
								Color3 c = item->value().get<Color3>();
								return QString( "R %1<br>G %2<br>B %3" ).arg( c[0] ).arg( c[1] ).arg( c[2] );
							}
						case NifValue::tColor4:
							{
								Color4 c = item->value().get<Color4>();
								return QString( "R %1<br>G %2<br>B %3<br>A %4" ).arg( c[0] ).arg( c[1] ).arg( c[2] ).arg( c[3] );
							}
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
				Spell * spell = SpellBook::instant( this, index );
				if ( spell )
					return spell->page() + "/" + spell->name();
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
		if ( item->name() == "NiSourceTexture" || item->name() == "NiImage" )
			buddy = getIndex( index, "File Name" );
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
			if ( item->parent() && item->parent() == root )
				updateHeader();
			break;
		case NifModel::TypeCol:
			item->setType( value.toString() );
			break;
		case NifModel::ValueCol:
			item->value().fromVariant( value );
			if ( isLink( index ) && getBlockOrHeader( index ) != getFooter() )
			{
				updateLinks();
				updateFooter();
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

	// reverse buddy lookup
	if ( index.column() == ValueCol )
	{
		if ( item->name() == "File Name" )
		{
			NifItem * parent = item->parent();
			if ( parent && ( parent->name() == "Texture Source" || parent->name() == "NiImage" ) )
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

void NifModel::reset()
{
	updateLinks();
	BaseModel::reset();
}

bool NifModel::removeRows( int row, int count, const QModelIndex & parent )
{
	NifItem * item = static_cast<NifItem*>( parent.internalPointer() );
	if ( ! ( parent.isValid() && parent.model() == this && item ) )
		return false;
	
	if ( row >= 0 && row+count < item->childCount() )
	{
		bool link = false;
		for ( int r = row; r < row + count; r++ )
			link |= itemIsLink( item->child( r ) );
		
		beginRemoveRows( parent, row, row+count );
		item->removeChildren( row, count );
		endRemoveRows();
		
		if ( link )
		{
			updateLinks();
			updateFooter();
			emit linksChanged();
		}
		return true;
	}
	else
		return false;
}


/*
 *  load and save
 */

bool NifModel::setHeaderString( const QString & s )
{
 	msg( DbgMsg() << s );
	if ( ! ( s.startsWith( "NetImmerse File Format" ) || s.startsWith( "Gamebryo" ) ) )
	{
		msg( Message() << "this is not a NIF" );
		return false;
	}
	
	int p = s.indexOf( "Version", 0, Qt::CaseInsensitive );
	if ( p >= 0 )
	{
		QString v = s;
		
		v.remove( 0, p + 8 );
		
		for ( int i = 0; i < v.length(); i++ )
		{
			if ( v[i].isDigit() || v[i] == QChar( '.' ) )
				continue;
			else
			{
				v = v.left( i );
			}
		}
		
		version = version2number( v );
		
		if ( ! isVersionSupported( version ) )
		{
			msg( Message() << "version" << version2string( version ) << "(" << v << ")" << "is not supported yet" );
			return false;
		}
		
		return true;
	}
	else
	{
		msg( Message() << "invalid header string" );
		return false;
	}
}

bool NifModel::load( QIODevice & device )
{
	clear();
	
	NifIStream stream( this, &device );

	// read header
	NifItem * header = getHeaderItem();
	if ( !header || !load( header, stream, true ) )
	{
		msg( Message() << "failed to load file header (version" << version << ")" );
		return false;
	}
	
	int numblocks = get<int>( header, "Num Blocks" );
	qDebug( "numblocks %i", numblocks );
	
	emit sigProgress( 0, numblocks );
	QTime t = QTime::currentTime();
	
	try
	{
		if ( version >= 0x0303000d )
		{
			// read in the NiBlocks
			for ( int c = 0; c < numblocks; c++ )
			{
				emit sigProgress( c + 1, numblocks );
				
				if ( device.atEnd() )
					throw QString( "unexpected EOF during load" );
				
				QString blktyp;
				
				if ( version >= 0x0a000000 )
				{
					// block types are stored in the header for versions above 10.x.x.x
					int blktypidx = get<int>( index( c, 0, getIndex( createIndex( header->row(), 0, header ), "Block Type Index" ) ) );
					blktyp = get<QString>( index( blktypidx, 0, getIndex( createIndex( header->row(), 0, header ), "Block Types" ) ) );
					
					if ( version < 0x0a020000 )
						device.read( 4 );
				}
				else
				{
					int len;
					device.read( (char *) &len, 4 );
					if ( len < 2 || len > 80 )
						throw QString( "next block does not start with a NiString" );
					blktyp = device.read( len );
				}
				
				if ( isNiBlock( blktyp ) )
				{
					msg( DbgMsg() << "loading block" << c << ":" << blktyp );
					insertNiBlock( blktyp, -1, true );
					if ( ! load( root->child( c+1 ), stream, true ) ) 
						throw QString( "failed to load block number %1 (%2) previous block was %3" ).arg( c ).arg( blktyp ).arg( root->child( c )->name() );
				}
				else
					throw QString( "encountered unknown block (%1)" ).arg( blktyp );
			}
			
			// read in the footer
			if ( !load( getFooterItem(), stream, true ) )
				throw QString( "failed to load file footer" );
		}
		else
		{	// versions below 3.3.0.13
			QMap<qint32,qint32> linkMap;
			
			for ( qint32 c = 0; true; c++ )
			{
				emit sigProgress( c + 1, 0 );
				
				if ( device.atEnd() )
					throw QString( "unexpected EOF during load" );
				
				int len;
				device.read( (char *) &len, 4 );
				if ( len < 0 || len > 80 )
					throw QString( "next block does not start with a NiString" );
				
				QString blktyp = device.read( len );
				
				if ( blktyp == "End Of File" )
				{
					break;
				}
				else if ( blktyp == "Top Level Object" )
				{
					device.read( (char *) &len, 4 );
					if ( len < 0 || len > 80 )
						throw QString( "next block does not start with a NiString" );
					blktyp = device.read( len );
				}
				
				qint32 p;
				device.read( (char *) &p, 4 );
				p -= 1;
				if ( p != c )
					linkMap.insert( p, c );
				
				if ( isNiBlock( blktyp ) )
				{
					msg( DbgMsg() << "loading block" << c << ":" << blktyp );
					insertNiBlock( blktyp, -1, true );
					if ( ! load( root->child( c+1 ), stream, true ) ) 
						throw QString( "failed to load block number %1 (%2) previous block was %3" ).arg( c ).arg( blktyp ).arg( root->child( c )->name() );
				}
				else
					throw QString( "encountered unknown block (%1)" ).arg( blktyp );
			}
			mapLinks( linkMap );
		}
	}
	catch ( QString err )
	{
		msg( Message() << err.toAscii().constData() );
		reset();
		return false;
	}
	//msg( Message() << t.msecsTo( QTime::currentTime() ) );
	reset(); // notify model views that a significant change to the data structure has occurded
	return true;
}

bool NifModel::save( QIODevice & device ) const
{
	NifOStream stream( this, &device );

	emit sigProgress( 0, rowCount( QModelIndex() ) );
	
	for ( int c = 0; c < rowCount( QModelIndex() ); c++ )
	{
		emit sigProgress( c+1, rowCount( QModelIndex() ) );
		
		msg( DbgMsg() << "saving block" << c << ":" << itemName( index( c, 0 ) ) );
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
				if ( version < 0x0303000d )
				{
					if ( rootLinks.contains( c - 1 ) )
					{
						QString string = "Top Level Object";
						int len = string.length();
						device.write( (char *) &len, 4 );
						device.write( (const char *) string.toAscii(), len );
					}
				}
				
				QString string = itemName( index( c, 0 ) );
				int len = string.length();
				device.write( (char *) &len, 4 );
				device.write( (const char *) string.toAscii(), len );
				
				if ( version < 0x0303000d )
				{
					device.write( (char *) &c, 4 );
				}
			}
		}
		if ( !save( root->child( c ), stream ) )
		{
			msg( Message() << "failed to write block" << itemName( index( c, 0 ) ) << "(" << c-1 << ")" );
			return false;
		}
	}
	
	if ( version < 0x0303000d )
	{
		QString string = "End Of File";
		int len = string.length();
		device.write( (char *) &len, 4 );
		device.write( (const char *) string.toAscii(), len );
	}
	
	return true;
}

bool NifModel::load( QIODevice & device, const QModelIndex & index )
{
	NifItem * item = static_cast<NifItem*>( index.internalPointer() );
	if ( item && index.isValid() && index.model() == this )
	{
		NifIStream stream( this, &device );
		bool ok = load( item, stream, false );
		updateLinks();
		updateFooter();
		emit linksChanged();
		return ok;
	}
	reset();
	return false;
}

bool NifModel::loadAndMapLinks( QIODevice & device, const QModelIndex & index, const QMap<qint32,qint32> & map )
{
	NifItem * item = static_cast<NifItem*>( index.internalPointer() );
	if ( item && index.isValid() && index.model() == this )
	{
		NifIStream stream( this, & device );
		bool ok = load( item, stream, false );
		mapLinks( item, map );
		updateLinks();
		updateFooter();
		emit linksChanged();
		return ok;
	}
	reset();
	return false;
}

bool NifModel::loadHeaderOnly( const QString & fname )
{
	clear();
	
	QFile f( fname );
	if ( ! f.open( QIODevice::ReadOnly ) )
	{
		msg( Message() << "failed to open file" << fname );
		return false;
	}
	
	NifIStream stream( this, &f );
	
	// read header
	NifItem * header = getHeaderItem();
	if ( !header || !load( header, stream, true ) )
	{
		msg( Message() << "failed to load file header (version" << version << ")" );
		return false;
	}
	
	return true;
}

bool NifModel::checkForBlock( const QString & filepath, const QString & blockId )
{
	NifModel nif;
	if ( nif.loadHeaderOnly( filepath ) )
	{
		foreach ( QString s, nif.getArray<QString>( nif.getHeader(), "Block Types" ) )
		{
			if ( inherits( s, blockId ) )
				return true;
		}
	}
	return false;
}
 
bool NifModel::save( QIODevice & device, const QModelIndex & index ) const
{
	NifOStream stream( this, &device );
	NifItem * item = static_cast<NifItem*>( index.internalPointer() );
	return ( item && index.isValid() && index.model() == this && save( item, stream ) );
}

int NifModel::fileOffset( const QModelIndex & index ) const
{
	NifSStream stream( this );
	NifItem * item = static_cast<NifItem *>( index.internalPointer() );
	if ( item && index.isValid() && index.model() == this )
	{
		int ofs = 0;
		if ( fileOffset( root, item, stream, ofs ) )
			return ofs;
	}
	return -1;
}

bool NifModel::load( NifItem * parent, NifIStream & stream, bool fast )
{
	if ( ! parent ) return false;
	
	for ( int row = 0; row < parent->childCount(); row++ )
	{
		NifItem * child = parent->child( row );
		
		if ( evalCondition( child ) )
		{
			if ( ! child->arr1().isEmpty() )
			{
				if ( ! updateArrayItem( child, fast ) )
					return false;
				
				if ( ! load( child, stream, fast ) )
					return false;
			}
			else if ( child->childCount() > 0 )
			{
				if ( ! load( child, stream, fast ) )
					return false;
			}
			else
			{
				if ( ! stream.read( child->value() ) )
					return false;
			}
		}
	}
	return true;
}

bool NifModel::save( NifItem * parent, NifOStream & stream ) const
{
	if ( ! parent ) return false;
	
	for ( int row = 0; row < parent->childCount(); row++ )
	{
		NifItem * child = parent->child( row );
		if ( evalCondition( child ) )
		{
			if ( ! child->arr1().isEmpty() || ! child->arr2().isEmpty() || child->childCount() > 0 )
			{
				if ( ! child->arr1().isEmpty() && child->childCount() != getArraySize( child ) )
					msg( Message() << "block" << getBlockNumber( parent ) << child->name() << "array size mismatch" );
				
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

bool NifModel::fileOffset( NifItem * parent, NifItem * target, NifSStream & stream, int & ofs ) const
{
	for ( int row = 0; row < parent->childCount(); row++ )
	{
		NifItem * child = parent->child( row );
		if ( child == target )
			return true;
		
		if ( evalCondition( child ) )
		{
			if ( ! child->arr1().isEmpty() || ! child->arr2().isEmpty() || child->childCount() > 0 )
			{
				if ( fileOffset( child, target, stream, ofs ) )
					return true;
			}
			else
			{
				ofs += stream.size( child->value() );
			}
		}
	}
	return false;
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
			msg( Message() << "infinite recursive link construct detected" << block << "->" << child );
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
		int l = parent->value().toLink();
		if ( l >= 0 && ( ( delta != 0 && l >= block ) || l == block ) )
		{
			if ( delta == 0 )
				parent->value().setLink( -1 );
			else
				parent->value().setLink( l + delta );
		}
	}
}

void NifModel::mapLinks( NifItem * parent, const QMap<qint32,qint32> & map )
{
	if ( ! parent ) return;
	
	if ( parent->childCount() > 0 )
	{
		for ( int c = 0; c < parent->childCount(); c++ )
			mapLinks( parent->child( c ), map );
	}
	else
	{
		int l = parent->value().toLink();
		if ( l >= 0 )
		{
			if ( map.contains( l ) )
				parent->value().setLink( map[ l ] );
		}
	}
}
	
qint32 NifModel::getLink( const QModelIndex & index ) const
{
	NifItem * item = static_cast<NifItem*>( index.internalPointer() );
	if ( ! ( index.isValid() && item && index.model() == this ) )	return -1;
	return item->value().toLink();
}

qint32 NifModel::getLink( const QModelIndex & parent, const QString & name ) const
{
	NifItem * parentItem = static_cast<NifItem*>( parent.internalPointer() );
	if ( ! ( parent.isValid() && parentItem && parent.model() == this ) )
		return -1;
	
	NifItem * item = getItem( parentItem, name );
	if ( item )
		return item->value().toLink();
	
	return -1;
}

QVector<qint32> NifModel::getLinkArray( const QModelIndex & iArray ) const
{
	QVector<qint32> links;
	NifItem * item = static_cast<NifItem*>( iArray.internalPointer() );
	if ( isArray( iArray ) && item && iArray.model() == this )
	{
		for ( int c = 0; c < item->childCount(); c++ )
		{
			if ( itemIsLink( item->child( c ) ) )
			{
				links.append( item->child( c )->value().toLink() );
			}
			else
			{
				links.clear();
				break;
			}
		}
	}
	return links;
}

QVector<qint32> NifModel::getLinkArray( const QModelIndex & parent, const QString & name ) const
{
	return getLinkArray( getIndex( parent, name ) );
}

bool NifModel::setLink( const QModelIndex & parent, const QString & name, qint32 l )
{
	NifItem * parentItem = static_cast<NifItem*>( parent.internalPointer() );
	if ( ! ( parent.isValid() && parentItem && parent.model() == this ) )
		return false;
	
	NifItem * item = getItem( parentItem, name );
	if ( item && item->value().setLink( l ) )
	{
		emit dataChanged( createIndex( item->row(), ValueCol, item ), createIndex( item->row(), ValueCol, item ) );
		NifItem * parent = item;
		while ( parent->parent() && parent->parent() != root )
			parent = parent->parent();
		if ( parent != getFooterItem() )
		{
			updateLinks();
			updateFooter();
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
		emit dataChanged( createIndex( item->row(), ValueCol, item ), createIndex( item->row(), ValueCol, item ) );
		NifItem * parent = item;
		while ( parent->parent() && parent->parent() != root )
			parent = parent->parent();
		if ( parent != getFooterItem() )
		{
			updateLinks();
			updateFooter();
			emit linksChanged();
		}
		return true;
	}
	else
		return false;
}

bool NifModel::setLinkArray( const QModelIndex & iArray, const QVector<qint32> & links )
{
	NifItem * item = static_cast<NifItem*>( iArray.internalPointer() );
	if ( isArray( iArray ) && item && iArray.model() == this )
	{
		bool ret = true;
		for ( int c = 0; c < item->childCount() && c < links.count(); c++ )
		{
			ret &= item->child( c )->value().setLink( links[c] );
		}
		ret &= item->childCount() == links.count();
		int x = item->childCount() - 1;
		if ( x >= 0 )
			emit dataChanged( createIndex( 0, ValueCol, item->child( 0 ) ), createIndex( x, ValueCol, item->child( x ) ) );
		NifItem * parent = item;
		while ( parent->parent() && parent->parent() != root )
			parent = parent->parent();
		if ( parent != getFooterItem() )
		{
			updateLinks();
			updateFooter();
			emit linksChanged();
		}
		return ret;
	}
	return false;
}

bool NifModel::setLinkArray( const QModelIndex & parent, const QString & name, const QVector<qint32> & links )
{
	return setLinkArray( getIndex( parent, name ), links );
}

bool NifModel::isLink( const QModelIndex & index, bool * isChildLink ) const
{
	NifItem * item = static_cast<NifItem*>( index.internalPointer() );
	if ( ! ( index.isValid() && item && index.model() == this ) )	return false;
	return itemIsLink( item, isChildLink );
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
