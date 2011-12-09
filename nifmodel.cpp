/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005-2010, NIF File Format Library and Tools
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

#include "nifmodel.h"
#include "niftypes.h"
#include "options.h"
#include "config.h"
#include "spellbook.h"

#include <QByteArray>
#include <QColor>
#include <QDebug>
#include <QFile>
#include <QTime>
#include <QSettings>
#include <QtEndian>

//! \file nifmodel.cpp NifModel implementation, NifModelEval

NifModel::NifModel( QObject * parent ) : BaseModel( parent )
{
	clear();
}

QString NifModel::version2string( quint32 v )
{
	if ( v == 0 )	return QString();
	QString s;
	if ( v < 0x0303000D ) {
		//This is an old-style 2-number version with one period
		s = QString::number( ( v >> 24 ) & 0xff, 10 ) + "."
		+ QString::number( ( v >> 16 ) & 0xff, 10 );

		quint32 sub_num1 = ((v >> 8) & 0xff);
		quint32 sub_num2 = (v & 0xff);
		if ( sub_num1 > 0 || sub_num2 > 0 ) {
			s = s + QString::number( sub_num1, 10 );
		}

		if ( sub_num2 > 0 ) {
			s = s + QString::number( sub_num2, 10 );
		}
	} else {
		//This is a new-style 4-number version with 3 periods
		s = QString::number( ( v >> 24 ) & 0xff, 10 ) + "."
			+ QString::number( ( v >> 16 ) & 0xff, 10 ) + "."
			+ QString::number( ( v >> 8 ) & 0xff, 10 ) + "."
			+ QString::number( v & 0xff, 10 );
	}
	return s;
}

quint32 NifModel::version2number( const QString & s )
{
	if ( s.isEmpty() )	return 0;

	if ( s.contains( "." ) )
	{
		QStringList l = s.split( "." );

		quint32 v = 0;

		if ( l.count() > 4 ) {
			//Should probaby post a warning here or something.  Version # has more than 3 dots in it.
			return 0;
		} else if ( l.count() == 2 ) {
			//This is an old style version number.  Take each digit following the first one at a time.
			//The first one is the major version
			v += l[0].toInt() << (3 * 8);

			if ( l[1].size() >= 1 ) {
				v += l[1].mid(0, 1).toInt() << (2 * 8);
			}
			if ( l[1].size() >= 2 ) {
				v += l[1].mid(1, 1).toInt() << (1 * 8);
			}
			if ( l[1].size() >= 3 ) {
				v += l[1].mid(2, -1).toInt();
			}
			return v;
		} else {
			//This is a new style version number with dots separating the digits
			for ( int i = 0; i < 4 && i < l.count(); i++ ) {
				v += l[i].toInt( 0, 10 ) << ( (3-i) * 8 );
			}
			return v;
		}

	} else {
		bool ok;
		quint32 i = s.toUInt( &ok );
		return ( i == 0xffffffff ? 0 : i );
	}
}

// Helper class for evaluating condition expressions
class NifModelEval
{
public:
	const NifModel * model;
	const NifItem * item;
	NifModelEval(const NifModel * model, const NifItem * item) {
		this->model = model;
		this->item = item;
	}

	QVariant operator()(const QVariant &v) const {
		if ( v.type() == QVariant::String ) {
			QString left = v.toString();
			NifItem * i = const_cast<NifItem*>(item);
			i = model->getItem( i, left );
			if (i) {
				if ( i->value().isCount() )
					return QVariant( i->value().toCount() );
				else if ( i->value().isFileVersion() )
					return QVariant( i->value().toFileVersion() );
			}
			QVariant(0);
		}
		return v;
	}
};

bool NifModel::evalVersion( NifItem * item, bool chkParents ) const
{
	if ( item == root )
		return true;

	if ( chkParents && item->parent() )
		if ( ! evalVersion( item->parent(), true ) )
			return false;

	if ( !item->evalVersion( version ) )
		return false;

	QString vercond = item->vercond();
	if ( vercond.isEmpty() )
		return true;

	QString vercond2 = item->verexpr().toString();

	NifModelEval functor(this, getHeaderItem());
	return item->verexpr().evaluateBool(functor);
}

void NifModel::clear()
{
	folder = QString();
	root->killChildren();
	insertType( root, NifData( "NiHeader", "Header" ) );
	insertType( root, NifData( "NiFooter", "Footer" ) );
	version = version2number( Options::startupVersion() );
	if( !isVersionSupported(version) ) {
		msg( Message() << tr("Unsupported 'Startup Version' %1 specified, reverting to 20.0.0.5").arg( Options::startupVersion() ).toAscii() );
		version = 0x14000005;
	}
	reset();
	NifItem * item = getItem( getHeaderItem(), "Version" );
	if ( item ) item->value().setFileVersion( version );

	QString header_string;
	if ( version <= 0x0A000100 ) {
		header_string = "NetImmerse File Format, Version ";
	} else {
		header_string = "Gamebryo File Format, Version ";
	}

	header_string += version2string(version);

	set<QString>( getHeaderItem(), "Header String", header_string );

	if ( version == 0x14000005 ) {
		//Just set this if version is 20.0.0.5 for now.  Probably should be a separate option.
		set<int>( getHeaderItem(), "User Version", 11 );
		set<int>( getHeaderItem(), "User Version 2", 11 );
	}
	//set<int>( getHeaderItem(), "Unknown Int 3", 11 );

	if ( version < 0x0303000D ) {
		QVector<QString> copyright(3);
		copyright[0] = "Numerical Design Limited, Chapel Hill, NC 27514";
		copyright[1] = "Copyright (c) 1996-2000";
		copyright[2] = "All Rights Reserved";

		setArray<QString>( getHeader(), "Copyright", copyright );
	}
	lockUpdates = false;
	needUpdates = utNone;
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
	if (lockUpdates) {
		needUpdates = UpdateType(needUpdates | utFooter);
		return;
	}

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
	if (lockUpdates) {
		needUpdates = UpdateType(needUpdates | utHeader);
		return;
	}

	NifItem * header = getHeaderItem();

	set<int>( header, "Num Blocks", getBlockCount() );
	NifItem * idxBlockTypes = getItem( header, "Block Types" );
	NifItem * idxBlockTypeIndices = getItem( header, "Block Type Index" );
	NifItem * idxBlockSize = getItem( header, "Block Size" );
	if ( idxBlockTypes && idxBlockTypeIndices )
	{
		QStringList blocktypes;
		QList<int>	blocktypeindices;

		for ( int r = 1; r < root->childCount() - 1; r++ )
		{
			NifItem * block = root->child( r );

			/*
			if ( ! blocktypes.contains( block->name() ) )
				blocktypes.append( block->name() );
			blocktypeindices.append( blocktypes.indexOf( block->name() ) );
			*/

			// NiMesh hack
			QString blockName = block->name();
#ifndef QT_NO_DEBUG
			qWarning() << "Updating header with " << blockName;
#endif
			if ( blockName == "NiDataStream" )
			{
				blockName = QString("NiDataStream\x01%1\x01%2").arg( block->child( "Usage" )->value().get<int>() ).arg( block->child( "Access" )->value().get<int>() );
#ifndef QT_NO_DEBUG
				qWarning() << "Changing blockname to " << blockName;
#endif
			}

			if ( ! blocktypes.contains( blockName ) )
				blocktypes.append( blockName );
			blocktypeindices.append( blocktypes.indexOf( blockName ) );

			if (version >= 0x14020000 && idxBlockSize )
				updateArrays(block, false);
		}

		set<int>( header, "Num Block Types", blocktypes.count() );

		// Setting fast update to true to workaround bug where deleting blocks
		//	causes a crash.  This probably means anyone listening to updates
		//	for these two arrays will not work.
		updateArrayItem( idxBlockTypes, false );
		updateArrayItem( idxBlockTypeIndices, true );
		if (version >= 0x14020000 && idxBlockSize ) {
			updateArrayItem( idxBlockSize, false );
		}

		for ( int r = 0; r < idxBlockTypes->childCount(); r++ )
			set<QString>( idxBlockTypes->child( r ), blocktypes.value( r ) );

		for ( int r = 0; r < idxBlockTypeIndices->childCount(); r++ )
			set<int>( idxBlockTypeIndices->child( r ), blocktypeindices.value( r ) );

		// for version 20.2.0.? and above the block size is stored in the header
		if (version >= 0x14020000 && idxBlockSize )
			for ( int r = 0; r < idxBlockSize->childCount(); r++ )
				set<quint32>( idxBlockSize->child( r ), blockSize( getBlockItem( r ) ) );

		// For 20.1 and above strings are saved in the header.  Max String Length must be updated.
		if (version >= 0x14010003) {
			int maxlen = 0;
			int nstrings = get<uint>(header, "Num Strings");
			QModelIndex iArray = getIndex( getHeader(), "Strings" );
			if ( nstrings > 0 && iArray.isValid() ) {
				for (int row=0; row<nstrings; ++row) {
					int len = get<QString>( iArray.child(row,0) ).length();
					if (len > maxlen)
						maxlen = len;
				}
			}
			set<uint>(header, "Max String Length", maxlen);
		}
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

bool NifModel::updateByteArrayItem( NifItem * array, bool fast )
{
	const int ArrayConvertSize = 0; // Currently disabled.  Use nifskopetype="blob" instead

	int calcRows = getArraySize( array );
	int rows = array->childCount();

	// exit early and let default handling delete all rows
	if (calcRows == 0 || !array->arr2().isEmpty())
		return false;

	// Transition from large array to smaller array which now requires real rows
	if ( calcRows < ArrayConvertSize ) {
		if (rows == 1) {
			if ( NifItem *child = array->child(0) ) {
				if ( child->value().type() == NifValue::tBlob ) {
					QByteArray bm = get<QByteArray>( child );

					// Delete the blob row after grabbing the data
					if ( ! fast )	beginRemoveRows( createIndex( array->row(), 0, array ), 0, rows - 1 );
					array->removeChildren( 0, rows );
					if ( ! fast )	endRemoveRows();

					// Now insert approprate rows and replace data from byte array to preserve some of the data.
					if ( calcRows > 0 )
					{
						NifData data( array->name(), array->type(), array->temp(), NifValue( NifValue::type( array->type() ) ), parentPrefix( array->arg() ), parentPrefix( array->arr2() ), QString(), QString(), 0, 0 );
						if ( ! fast )	beginInsertRows( createIndex( array->row(), 0, array ), 0, calcRows-1 );
						array->prepareInsert( calcRows );
						for ( int i = 0; i < calcRows; i++ ) {
							insertType( array, data );
							if ( NifItem *c = array->child(i) )
								c->value().set<quint8>( bm[i] );
						}
						if ( ! fast )	endInsertRows();
					}
				}
			}
		}
		return false;
	}


	// Create dummy row for holding the blob data
	QByteArray bytes; bytes.resize( calcRows );

	// Grab data from existing rows if appropriate and then purge
	if (rows > 1) {
		for (int i=0;i<rows;i++) {
			if ( NifItem *child = array->child(0) ) {
				bytes[i] = get<quint8>( child );
			}
		}
		if ( ! fast )	beginRemoveRows( createIndex( array->row(), 0, array ), 0, rows - 1 );
		array->removeChildren( 0, rows );
		if ( ! fast )	endRemoveRows();
		rows = 0;
	}

	// Create the dummy row for holding the byte array
	if ( rows == 0 ) {
		NifData data( array->name(), array->type(), array->temp(), NifValue( NifValue::tBlob ), parentPrefix( array->arg() ), QString(), QString(), QString(), 0, 0 );
		if ( ! fast )	beginInsertRows( createIndex( array->row(), 0, array ), 0, 1 );
		array->prepareInsert( 1 );
		insertType( array, data );
		if ( ! fast )	endInsertRows();
	}
	if ( NifItem *child = array->child(0) ) {
		QByteArray* bm = (child->value().type() == NifValue::tBlob) ? get<QByteArray*>( child ) : NULL;
		if (bm == NULL) {
			set<QByteArray>( child, bytes );
		} else if (bm->size() == 0) {
			*bm = bytes;
		} else {
			bm->resize(calcRows);
		}
	}
	return true;
}

bool NifModel::updateArrayItem( NifItem * array, bool fast )
{
	if ( array->arr1().isEmpty() )
		return false;

	int d1 = getArraySize( array );


	// Special case for very large arrays that are opaque in nature.
	//  Typical array handling has very poor performance with these arrays
	if ( NifValue::type( array->type() ) == NifValue::tBlob )  {
		if ( updateByteArrayItem(array, fast) )
			return true;
	}

	if ( d1 > 1024 * 1024 * 8 ) {
		msg( Message() << tr("array %1 much too large. %2 bytes requested").arg(array->name()).arg(d1) );
		return false;
	} else if ( d1 < 0 ) {
		msg( Message() << tr("array %1 invalid").arg(array->name()) );
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

bool NifModel::updateArrays( NifItem * parent, bool fast )
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

				if ( ! updateArrays( child, false ) )
					return false;
			}
			else if ( child->childCount() > 0 )
			{
				if ( ! updateArrays( child, fast ) )
					return false;
			}
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
		msg( Message() << tr("unknown block %1").arg(identifier) );
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

void NifModel::updateStrings(NifModel *src, NifModel* tgt, NifItem *item)
{
	if ( NULL == item )
		return;
	NifValue::Type vt = item->value().type();
	if ( vt == NifValue::tStringIndex || vt == NifValue::tSizedString || item->type() == "string" )
	{
		QString str = src->string( src->createIndex(0, 0, item) );
		tgt->assignString( tgt->createIndex(0, 0, item), str, false );
	}
	for (int i=0; i<item->childCount(); ++i)
	{
		updateStrings(src, tgt, item->child(i));
	}
}

QMap<qint32,qint32> NifModel::moveAllNiBlocks( NifModel * targetnif, bool update )
{
	int bcnt = getBlockCount();

	bool doStringUpdate = (  this->getVersionNumber() >= 0x14010003 || targetnif->getVersionNumber() >= 0x14010003 );

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
		NifItem *item = targetnif->root->child( targetnif->getBlockCount() - i );
		targetnif->mapLinks( item , map );
		if (doStringUpdate)
			updateStrings( this, targetnif, item );
	}

	if (update)
	{
		updateLinks();
		updateHeader();
		updateFooter();
		emit linksChanged();

		targetnif->updateLinks();
		targetnif->updateHeader();
		targetnif->updateFooter();
		emit targetnif->linksChanged();
	}
	return map;
}

void NifModel::reorderBlocks( const QVector<qint32> & order )
{
	if ( getBlockCount() <= 1 )
		return;

	if ( order.count() != getBlockCount() )
	{
		msg( Message() << tr("NifModel::reorderBlocks() - invalid argument") );
		return;
	}

	QMap<qint32,qint32> linkMap;
	QMap<qint32,qint32> blockMap;

	for ( qint32 n = 0; n < order.count(); n++ )
	{
		if ( blockMap.contains( order[n] ) || order[n] < 0 || order[n] >= getBlockCount() )
		{
			msg( Message() << tr("NifModel::reorderBlocks() - invalid argument") );
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
	Q_UNUSED(at);
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
		msg( Message() << tr("unknown ancestor %1").arg(identifier) );
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
		{
			NifItem *item = parent->insertChild( data, at );
			// Kludge for string conversion.
			//  Ensure that the string type is correct for the nif version
			if ( item->value().type() == NifValue::tString || item->value().type() == NifValue::tFilePath ) {
				item->value().changeType( version < 0x14010003 ? NifValue::tSizedString : NifValue::tStringIndex );
			}
		}
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

	bool ndr = role == NifSkopeDisplayRole;
	if (role == NifSkopeDisplayRole)
		role = Qt::DisplayRole;

	if ( column == ValueCol && item->parent() == root && item->type() == "NiBlock" )
	{
		QModelIndex buddy;
		if ( item->name() == "NiSourceTexture" || item->name() == "NiImage" )
			buddy = getIndex( index, "File Name" );
		else if ( item->name() == "NiStringExtraData" )
			buddy = getIndex( index, "String Data" );
		//else if ( item->name() == "NiTransformInterpolator" && role == Qt::DisplayRole)
		//	return QString(tr("TODO: find out who is referring me"));
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
		  if (version >= 0x14010003)
		  {
			  buddy = getIndex( index, "Node Name" );
			  if ( buddy.isValid() )
				  buddy = buddy.sibling( buddy.row(), index.column() );
			  if ( buddy.isValid() )
				  return data( buddy, role );
		  }
		  else if (version <= 0x14000005)
		  {
			  buddy = getIndex( index, "Node Name Offset" );
			  if ( buddy.isValid() )
				  buddy = buddy.sibling( buddy.row(), index.column() );
			  if ( buddy.isValid() )
				  return data( buddy, role );
		  }
		}
	}

	switch ( role )
	{
		case Qt::DisplayRole:
		{
			switch ( column )
			{
				case NameCol:
				{
					if (ndr)
						return item->name();
					QString a = "";
					if ( itemType( index ) == "NiBlock" )
						a = QString::number( getBlockNumber( index ) ) + " ";
					return a + item->name();
				}	break;
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
					QString type = item->type();
					const NifValue& value = item->value();
					if (value.type() == NifValue::tString || value.type() == NifValue::tFilePath)
					{
						return QString(this->string(index)).replace("\n", " ").replace("\r", " ");
					}
					else if ( item->value().type() == NifValue::tStringOffset )
					{
						int ofs = item->value().get<int>();
						if ( ofs < 0 || ofs == 0x0000FFFF )
							return QString( "<empty>" );
						NifItem * palette = getItemX( item, "String Palette" );
						int link = ( palette ? palette->value().toLink() : -1 );
						if ( ( palette = getBlockItem( link ) ) && ( palette = getItem( palette, "Palette" ) ) )
						{
							QByteArray bytes = palette->value().get<QByteArray>();
							if ( ofs < bytes.count() )
								return QString( & bytes.data()[ofs] );
							else
								return tr("<offset invalid>");
						}
						else
						{
							return tr("<palette not found>");
						}
					}
					else if ( item->value().type() == NifValue::tStringIndex )
					{
						int idx = item->value().get<int>();
						if (idx == -1)
							return QString();
						NifItem * header = getHeaderItem();
						QModelIndex stringIndex = createIndex( header->row(), 0, header );
						QString string = get<QString>( this->index( idx, 0, getIndex( stringIndex, "Strings" ) ) );
						if ( idx >= 0 )
							return QString( "%2 [%1]").arg( idx ).arg( string );
						return tr("%1 - <index invalid>").arg( idx );
						  }
					else if ( item->value().type() == NifValue::tBlockTypeIndex )
					{
						int idx = item->value().get<int>();
						int offset = idx & 0x7FFF;
						NifItem * blocktypes = getItemX( item, "Block Types" );
						NifItem * blocktyp = ( blocktypes ? blocktypes->child( offset ) : 0 );
						if ( blocktyp )
							return QString( "%2 [%1]").arg( idx ).arg( blocktyp->value().get<QString>() );
						else
							return tr("%1 - <index invalid>").arg( idx );
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
								return tr("%1 <invalid>").arg( lnk );
							}
						}
						else
							return tr("None");
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
					{
						return item->value().toString().replace("\n", " ").replace("\r", " ");
					}
				}	break;
				case ArgCol:	return item->arg();
				case Arr1Col:	return item->arr1();
				case Arr2Col:	return item->arr2();
				case CondCol:	return item->cond();
				case Ver1Col:	return version2string( item->ver1() );
				case Ver2Col:	return version2string( item->ver2() );
				case VerCondCol: return item->vercond();
				default:		return QVariant();
			}
		}
		case Qt::DecorationRole:
		{
			switch ( column )
			{
				case NameCol:
 					// (QColor, QIcon or QPixmap) as stated in the docs
					/*if ( itemType( index ) == "NiBlock" )
						return QString::number( getBlockNumber( index ) );*/
					return QVariant();
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
				case ValueCol:
				{
					const NifValue& value = item->value();
					if (value.type() == NifValue::tString || value .type() == NifValue::tFilePath)
					{
						return string(index);
					}
					return item->value().toVariant();
				}
				case ArgCol:	return item->arg();
				case Arr1Col:	return item->arr1();
				case Arr2Col:	return item->arr2();
				case CondCol:	return item->cond();
				case Ver1Col:	return version2string( item->ver1() );
				case Ver2Col:	return version2string( item->ver2() );
				case VerCondCol: return item->vercond();
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
						QString tip = QString( "<p><b>%1</b></p><p>%2</p>" )
							.arg( item->name() )
							.arg( QString(item->text()).replace( "<", "&lt;" ).replace( "\n", "<br/>") );

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
						case NifValue::tWord:
						case NifValue::tShort:
						case NifValue::tBool:
						case NifValue::tInt:
						case NifValue::tUInt:
							{
								return tr("dec: %1\nhex: 0x%2")
									.arg( item->value().toString() )
									.arg( item->value().toCount(), 8, 16, QChar( '0' ) );
							}
						case NifValue::tFloat:
							{
								return tr("float: %1\nhex: 0x%2")
									.arg( NumOrMinMax( item->value().toFloat(), 'g', 8 ) )
									.arg( item->value().toCount(), 8, 16, QChar( '0' ) );
							}
						case NifValue::tFlags:
							{
								quint16 f = item->value().toCount();
								return tr("dec: %1\nhex: 0x%2\nbin: 0b%3")
									.arg( f )
									.arg( f, 4, 16, QChar( '0' ) )
									.arg( f, 16, 2, QChar( '0' ) );
							}
						case NifValue::tStringIndex:
							return QString( "0x%1" )
								.arg( item->value().toCount(), 8, 16, QChar( '0' ) );
						case NifValue::tStringOffset:
							return QString( "0x%1" )
								.arg( item->value().toCount(), 8, 16, QChar( '0' ) );
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
								return QString( "R %1\nG %2\nB %3" )
									.arg( c[0] )
									.arg( c[1] )
									.arg( c[2] );
							}
						case NifValue::tColor4:
							{
								Color4 c = item->value().get<Color4>();
								return QString( "R %1\nG %2\nB %3\nA %4" )
									.arg( c[0] )
									.arg( c[1] )
									.arg( c[2] )
									.arg( c[3] );
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
			// "notify" about an invalid index in "Triangles"
			// TODO: checkbox, "show invalid only"
			if ( column == ValueCol && item->value().type() == NifValue::tTriangle ) {
				NifItem *nv = getItemX( item, "Num Vertices" );
				quint32 nvc = nv->value().toCount();
				Triangle t = item->value().get<Triangle>();
				if (t[0] >= nvc || t[1] >= nvc || t[2] >= nvc)
					return QColor::fromRgb(240, 210, 210);
			}

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
		else if ( item->name() == "NiStringExtraData" )
			buddy = getIndex( index, "String Data" );
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
			{
				QString type = item->type();
				NifValue& val = item->value();
				if (val.type() == NifValue::tString || val.type() == NifValue::tFilePath)
				{
					val.changeType( version < 0x14010003 ? NifValue::tSizedString : NifValue::tStringIndex );
					assignString(index, value.toString(), true);
				}
				else
				{
					item->value().fromVariant( value );
					if ( isLink( index ) && getBlockOrHeader( index ) != getFooter() )
					{
						updateLinks();
						updateFooter();
						emit linksChanged();
					}
				}
			}	break;
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
		case NifModel::VerCondCol:
			item->setVerCond( value.toString() );
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

	if ( row >= 0 && ((row+count) <= item->childCount()) )
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
 	//msg( DbgMsg() << s );
	if ( ! ( s.startsWith( "NetImmerse File Format" ) || s.startsWith( "Gamebryo" ) // official
		|| s.startsWith( "NDSNIF" ) // altantica
		|| s.startsWith( "NS" ) // neosteam
		|| s.startsWith( "Joymaster HS1 Object Format - (JMI)" ) // howling sword, uses .jmi extension
		) )
	{
		msg( Message() << tr("this is not a NIF") );
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
			msg( Message() << tr("version %1 (%2) is not supported yet").arg(version2string( version )).arg(v) );
			return false;
		}

		return true;
	}
	else if ( s.startsWith( "NS" ) )
	{
		// Dodgy version for NeoSteam
		version = 0x0a010000;
		return true;
	}
	else
	{
		msg( Message() << tr("invalid header string") );
		return false;
	}
}

bool NifModel::load( QIODevice & device )
{
	NIFSKOPE_QSETTINGS(cfg);
	bool ignoreSize = false;
	ignoreSize = cfg.value("ignore block size", false).toBool();

	clear();

	NifIStream stream( this, &device );

	// read header
	NifItem * header = NULL;
	header = getHeaderItem();
	// bugfix: force user versions to zero (if the template was version
	// 20.0.0.5 then they have been set to non-zero, and the read function
	// will not reset them to zero on older files...)
	if (header)
	{
		set<int>( header, "User Version 2", 0 );
		set<int>( header, "User Version", 0 );
	}
	if ( !header || !load( header, stream, true ) )
	{
		msg( Message() << tr("failed to load file header (version %1, %2)").arg(version, 0, 16).arg(version2string(version)));
		return false;
	}

	int numblocks = 0;
	numblocks = get<int>( header, "Num Blocks" );
	//qDebug( "numblocks %i", numblocks );

	emit sigProgress( 0, numblocks );
	QTime t = QTime::currentTime();

	qint64 curpos = 0;
	try
	{
		curpos = device.pos();
		if ( version >= 0x0303000d )
		{
			// read in the NiBlocks
			QString prevblktyp;
			for ( int c = 0; c < numblocks; c++ )
			{
				emit sigProgress( c + 1, numblocks );

				if ( device.atEnd() )
					throw tr("unexpected EOF during load");

				QString blktyp;
				quint32 size = UINT_MAX;
				try
				{
					if ( version >= 0x0a000000 )
					{
						// block types are stored in the header for versions above 10.x.x.x
						//	the upper bit or the blocktypeindex seems to be related to PhysX
						int blktypidx = get<int>( index( c, 0, getIndex( createIndex( header->row(), 0, header ), "Block Type Index" ) ) );
						blktyp = get<QString>( index( blktypidx & 0x7FFF, 0, getIndex( createIndex( header->row(), 0, header ), "Block Types" ) ) );
						// note: some 10.0.1.0 version nifs from Oblivion in certain distributions seem to be missing
						//		 these four bytes on the havok blocks
						//		 (see for instance meshes/architecture/basementsections/ungrdltraphingedoor.nif)
						if ((version < 0x0a020000) && (!blktyp.startsWith("bhk"))) {
						  int dummy;
						  device.read( (char *) &dummy, 4 );
						  if (dummy != 0)
							 msg(Message() << tr("non-zero block separator (%1) preceeding block %2").arg(dummy).arg(blktyp));
						};

						// for version 20.2.0.? and above the block size is stored in the header
						if (!ignoreSize && version >= 0x14020000)
							size = get<quint32>( index( c, 0, getIndex( createIndex( header->row(), 0, header ), "Block Size" ) ) );
					}
					else
					{
						int len;
						device.read( (char *) &len, 4 );
						if ( len < 2 || len > 80 )
							throw tr("next block does not start with a NiString");
						blktyp = device.read( len );
					}

					// Hack for NiMesh data streams
					qint32 dataStreamUsage = -1;
					qint32 dataStreamAccess = -1;
					if ( blktyp.startsWith( "NiDataStream\x01" ) )
					{
						QStringList splitStream = blktyp.split("\x01");
						blktyp = splitStream[0];
						bool ok;
						dataStreamUsage = splitStream[1].toInt( &ok );
						if ( ! ok )
						{
							throw tr("Unknown NiDataStream");
						}
						dataStreamAccess = splitStream[2].toInt( &ok );
						if ( ! ok )
						{
							throw tr("Unknown NiDataStream");
						}
#ifndef QT_NO_DEBUG
						qWarning() << "Loaded NiDataStream with usage " << dataStreamUsage << " access " << dataStreamAccess;
#endif
					}

					if ( isNiBlock( blktyp ) )
					{
						//msg( DbgMsg() << "loading block" << c << ":" << blktyp );
						QModelIndex newBlock = insertNiBlock( blktyp, -1, true );
						if ( ! load( root->child( c+1 ), stream, true ) )
						{
							NifItem * child = root->child( c );
							throw tr("failed to load block number %1 (%2) previous block was %3").arg( c ).arg( blktyp ).arg( child ? child->name() : prevblktyp );
						}
						// NiMesh hack
						if ( blktyp == "NiDataStream" )
						{
							set<qint32>( newBlock, "Usage", dataStreamUsage );
							set<qint32>( newBlock, "Access", dataStreamAccess );
						}
					}
					else
					{
						msg( Message() << tr("warning: block %1 (%2) not inserted!").arg(c).arg(blktyp) );
						throw tr("encountered unknown block (%1)").arg( blktyp );
					}
				}
				catch ( QString err )
				{
					// version 20.3.0.3 can mostly recover from some failures because it store block sizes
					// XXX FIXME: if isNiBlock returned false, block numbering will be screwed up!!
					if (size == UINT_MAX)
						throw err;
				}
				// Check device position and emit warning if location is not expected
				if (size != UINT_MAX)
					{
					qint64 pos = device.pos();
					if ((curpos + size) != pos)
					{
						// unable to seek to location... abort
						if (device.seek(curpos + size))
							msg( Message() << tr("device position incorrect after block number %1 (%2) at 0x%3 ended at 0x%4 (expected 0x%5)").arg( c ).arg( blktyp ).arg(QString::number(curpos, 16)).arg(QString::number(pos, 16)).arg(QString::number(curpos+size, 16)).toAscii() );
						else
							throw tr("failed to reposition device at block number %1 (%2) previous block was %3").arg( c ).arg( blktyp ).arg( root->child( c )->name() );

						curpos = device.pos();
					}
					else
					{
						curpos = pos;
					}
				}
				prevblktyp = blktyp;
			}

			// read in the footer
			if ( !load( getFooterItem(), stream, true ) )
				throw tr("failed to load file footer");
		}
		else
		{	// versions below 3.3.0.13
			QMap<qint32,qint32> linkMap;

			try {
				for ( qint32 c = 0; true; c++ )
				{
					emit sigProgress( c + 1, 0 );

					if ( device.atEnd() )
						throw tr("unexpected EOF during load");

					int len;
					device.read( (char *) &len, 4 );
					if ( len < 0 || len > 80 )
						throw tr("next block does not start with a NiString");

					QString blktyp = device.read( len );

					if ( blktyp == "End Of File" )
					{
						break;
					}
					else if ( blktyp == "Top Level Object" )
					{
						device.read( (char *) &len, 4 );
						if ( len < 0 || len > 80 )
							throw tr("next block does not start with a NiString");
						blktyp = device.read( len );
					}

					qint32 p;
					device.read( (char *) &p, 4 );
					p -= 1;
					if ( p != c )
						linkMap.insert( p, c );

					if ( isNiBlock( blktyp ) )
					{
						//msg( DbgMsg() << "loading block" << c << ":" << blktyp );
						insertNiBlock( blktyp, -1, true );
						if ( ! load( root->child( c+1 ), stream, true ) )
							throw tr("failed to load block number %1 (%2) previous block was %3").arg( c ).arg( blktyp ).arg( root->child( c )->name() );
					}
					else
						throw tr("encountered unknown block (%1)").arg( blktyp );
				}
			}
			catch ( QString err ) {
				//If this is an old file we should still map the links, even if it failed
				mapLinks( linkMap );

				//Re-throw exception so that the error is printed
				throw err;
			}

			//Also map links if nothing went wrong
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

	// Force update header and footer prior to save
	if ( NifModel* mdl = const_cast<NifModel*>(this) ) {
		mdl->updateHeader();
		mdl->updateFooter();
	}
	
	emit sigProgress( 0, rowCount( QModelIndex() ) );
	
	for ( int c = 0; c < rowCount( QModelIndex() ); c++ )
	{
		emit sigProgress( c+1, rowCount( QModelIndex() ) );

		//msg( DbgMsg() << "saving block" << c << ":" << itemName( index( c, 0 ) ) );
#ifndef QT_NO_DEBUG
		qWarning() << "saving block " << c << ": " << itemName( index( c, 0 ) );
#endif
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
			msg( Message() << tr("failed to write block %1(%2)").arg(itemName( index( c, 0 ) )).arg(c-1) );
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
		msg( Message() << tr("failed to open file") << fname );
		return false;
	}

	NifIStream stream( this, &f );

	// read header
	NifItem * header = getHeaderItem();
	if ( !header || !load( header, stream, true ) )
	{
		msg( Message() << tr("failed to load file header (version %1)").arg(version) );
		return false;
	}

	return true;
}

bool NifModel::earlyRejection( const QString & filepath, const QString & blockId, quint32 version )
{
	NifModel nif;
	if ( nif.loadHeaderOnly( filepath ) == false ) {
		//File failed to read entierly
		return false;
	}

	bool ver_match = false;
	if ( version == 0 )
	{
		ver_match = true;
	}
	else if ( version != 0 && nif.getVersionNumber() == version )
	{
		ver_match = true;
	}

	bool blk_match = false;
	if ( blockId.isEmpty() == true || version < 0x0A000100 )
	{
		blk_match = true;
	}
	else
	{
		foreach ( QString s, nif.getArray<QString>( nif.getHeader(), "Block Types" ) )
		{
			if ( inherits( s, blockId ) )
			{
				blk_match = true;
				break;
			}
		}
	}

	return (ver_match && blk_match);
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
	NifItem * target = static_cast<NifItem *>( index.internalPointer() );
	if ( target && index.isValid() && index.model() == this )
	{
		int ofs = 0;
		for ( int c = 0; c < root->childCount(); c++ )
		{
			if ( c > 0 && c <= getBlockCount() )
			{
				if ( version > 0x0a000000 )
				{
					if ( version < 0x0a020000 )
					{
						ofs += 4;
					}
				}
				else
				{
					if ( version < 0x0303000d )
					{
						if ( rootLinks.contains( c - 1 ) )
						{
							QString string = "Top Level Object";
							ofs += 4 + string.length();
						}
					}

					QString string = itemName( this->NifModel::index( c, 0 ) );
					ofs += 4 + string.length();

					if ( version < 0x0303000d )
					{
						ofs += 4;
					}
				}
			}

			if ( fileOffset( root->child( c ), target, stream, ofs ) )
				return ofs;
		}
	}
	return -1;
}

int NifModel::blockSize( const QModelIndex & index ) const
{
	NifSStream stream( this );
	return blockSize( static_cast<NifItem *>( index.internalPointer() ), stream );
}

int NifModel::blockSize( NifItem * parent ) const
{
	NifSStream stream( this );
	return blockSize( parent, stream );
}

int NifModel::blockSize( NifItem * parent, NifSStream& stream ) const
{
	int size = 0;
	if ( ! parent )
		return 0;

	for ( int row = 0; row < parent->childCount(); row++ )
	{
		NifItem * child = parent->child( row );

		if ( child->isAbstract() )
		{
#ifndef QT_NO_DEBUG
			// qWarning() << "Not counting abstract item " << child->name();
#endif
			continue;
		}

		if ( evalCondition( child ) )
		{
			if ( ! child->arr1().isEmpty() || ! child->arr2().isEmpty() || child->childCount() > 0 )
			{
				if ( ! child->arr1().isEmpty() && child->childCount() != getArraySize( child ) ) {
					if ( ( NifValue::type( child->type() ) == NifValue::tBlob ) ) {
						// special byte
					} else {
						msg( Message() << tr("block %1 %2 array size mismatch").arg(getBlockNumber( parent )).arg(child->name()) );
					}
				}

				size += blockSize( child, stream );
			}
			else
			{
				size += stream.size( child->value() );
			}
		}
	}
	return size;
}

bool NifModel::load( NifItem * parent, NifIStream & stream, bool fast )
{
	if ( ! parent ) return false;

	for ( int row = 0; row < parent->childCount(); row++ )
	{
		NifItem * child = parent->child( row );

		if ( child->isAbstract() )
		{
#ifndef QT_NO_DEBUG
			// qWarning() << "Not loading abstract item " << child->name();
#endif
			continue;
		}

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
		
		// these values are always little-endian
		if( (child->name() == "Num Blocks") || (child->name() == "User Version") || (child->name() == "User Version 2") )
		{
			if( version >= 0x14000004 && get<quint8>( getHeaderItem(), "Endian Type" ) == 0 )
			{
				child->value().setCount( qFromBigEndian( child->value().toCount() ) );
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

		if ( child->isAbstract() )
		{
#ifndef QT_NO_DEBUG
			qWarning() << "Not saving abstract item " << child->name();
#endif
			continue;
		}

		if ( evalCondition( child ) )
		{
			if ( ! child->arr1().isEmpty() || ! child->arr2().isEmpty() || child->childCount() > 0 )
			{
				if ( ! child->arr1().isEmpty() && child->childCount() != getArraySize( child ) ) {
					if ( ( NifValue::type( child->type() ) == NifValue::tBlob ) ) {
						// special byte
					} else {
						msg( Message() << tr("block %1 %2 array size mismatch").arg(getBlockNumber( parent )).arg(child->name()) );
					}
				}

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
	if ( parent == target )
		return true;

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
	if (lockUpdates) {
		needUpdates = UpdateType(needUpdates | utLinks);
		return;
	}

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


		int n = getBlockCount();
		QByteArray hasrefs(n, 0);
		for ( int c = 0; c < n; c++ )
		{
			foreach( int d, childLinks.value( c ) )
			{
				if (d >=0 && d < n)
					hasrefs[d] = 1;
			}
		}
		for ( int c = 0; c < n; c++ )
		{
			if (!hasrefs[c])
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
			msg( Message() << tr("infinite recursive link construct detected %1 -> %2").arg(block).arg(child) );
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

QString NifModel::string( const QModelIndex & index, bool extraInfo ) const
{
	NifValue v = getValue( index );
	if (v.type() == NifValue::tSizedString)
		return BaseModel::get<QString>( index );

	if (getVersionNumber() >= 0x14010003)
	{
		QModelIndex iIndex;
		int idx = -1;
		if (v.type() == NifValue::tStringIndex)
			idx = get<int>( index );
		else if (!v.isValid())
			idx = get<int>( getIndex( index, "Index" ) );
		if (idx == -1)
		{
			return QString();
		}
		else if ( idx >= 0 )
		{
			NifItem * header = this->getHeaderItem();
			QModelIndex stringIndex = createIndex( header->row(), 0, header );
			if ( stringIndex.isValid() )
			{
				QString string = BaseModel::get<QString>( this->index( idx, 0, getIndex( stringIndex, "Strings" ) ) );
				if (extraInfo)
					string = QString( "%2 [%1]").arg( idx ).arg( string );
				return string;
			}
			else
			{
				return tr( "%1 - <index invalid>" ).arg( idx );
			}
		}
	}
	else
	{
		if (v.type() == NifValue::tNone)
		{
			QModelIndex iIndex = getIndex( index, "String" );
			if (iIndex.isValid())
				return BaseModel::get<QString>( iIndex );
		}
		else
		{
			return BaseModel::get<QString>( index );
		}
	}
	return QString();
}

QString NifModel::string( const QModelIndex & index, const QString & name, bool extraInfo ) const
{
	return string( getIndex(index, name), extraInfo );
}

bool NifModel::assignString( const QModelIndex & index, const QString & string, bool replace)
{
	NifItem * item = static_cast<NifItem*>( index.internalPointer() );
	if ( ! ( index.isValid() && item && index.model() == this ) )	return false;
	return assignString(item, string, replace);
}

bool NifModel::assignString( NifItem * item, const QString & string, bool replace )
{
	NifValue& v = item->value();

	if (getVersionNumber() >= 0x14010003)
	{
		NifItem * pItem;
		int idx = -1;
		if (v.type() == NifValue::tStringIndex)
			idx = get<int>( pItem = item );
		else if (!v.isValid()) {
			pItem = getItem( item, "Index" );
			if (!pItem) return false;
			idx = get<int>( pItem );
		} else if (v.type() == NifValue::tSizedString && item->type() == "string") {
			pItem = item;
			idx = -1;
		}
		else
			return BaseModel::set<QString>( item, string );

		QModelIndex header = getHeader();
		int nstrings = get<int>( header, "Num Strings" );
		if ( string.isEmpty() )
		{
			if (replace && idx >= 0 && idx < nstrings)
			{
				// TODO: Can we remove the string safely here?
			}
			v.changeType(NifValue::tStringIndex);
			return set<int>( item, 0xffffffff );
		}
		// Simply replace the string
		QModelIndex iArray = getIndex( header, "Strings" );
		if (replace && idx >= 0 && idx < nstrings) {
			return BaseModel::set<QString>(iArray.child(idx, 0), string);
		}

		QVector<QString> stringVector = getArray<QString>( iArray );
		idx = stringVector.indexOf(string);
		// Already exists.  Just update the Index
		if (idx >= 0 && idx < stringVector.size())
		{
			v.changeType(NifValue::tStringIndex);
			return set<int>( pItem, idx );
		}
		else // append string to end of list
		{
			set<uint>( header, "Num Strings", nstrings+1);
			updateArray(header, "Strings");
			QModelIndex iArray = getIndex( header, "Strings" );
			BaseModel::set<QString>(iArray.child(nstrings, 0), string);

			v.changeType(NifValue::tStringIndex);
			return set<int>( pItem, nstrings );
		}
	}
	else // handle the older simpler strings
	{
		if (v.type() == NifValue::tNone)
		{
			return BaseModel::set<QString>( getItem( item, "String" ), string );
		}
		else if (v.type() == NifValue::tStringIndex)
		{
			NifValue v(NifValue::tString);
			v.set(string);
			return setItemValue(item, v);
			//return BaseModel::set<QString>( index, string );
		}
		else
		{
			return BaseModel::set<QString>( item, string );
		}
	}
	return false;
}

bool NifModel::assignString( const QModelIndex & index, const QString & name, const QString & string, bool replace )
{
	return assignString( getIndex(index, name), string, replace );
}


// convert a block from one type to another
void NifModel::convertNiBlock( const QString & identifier, const QModelIndex& index , bool fast )
{
	QString btype = getBlockName( index );
	if (btype == identifier)
		return;

	if ( !inherits( btype, identifier ) && !inherits(identifier, btype) ) {
		msg( Message() << tr("blocktype %1 and %2 are not related").arg(btype).arg(identifier) );
		return;
	}
	NifItem * branch = static_cast<NifItem*>( index.internalPointer() );
	NifBlock * srcBlock = blocks.value( btype );
	NifBlock * dstBlock = blocks.value( identifier );
	if ( srcBlock && dstBlock && branch )
	{
		branch->setName( identifier );

		if ( inherits( btype, identifier ) ) {
			// Remove any level between the two types
			for (QString ancestor = btype; !ancestor.isNull() && ancestor != identifier; ) {
				NifBlock * block = blocks.value( ancestor );
				if (!block) break;

				int n = block->types.count();
				if (n > 0) removeRows( branch->childCount() - n,  n, index );
				ancestor = block->ancestor;
			}
		} else if ( inherits(identifier, btype) ) {
			// Add any level between the two types
			QStringList types;
			for (QString ancestor = identifier; !ancestor.isNull() && ancestor != btype; ) {
				NifBlock * block = blocks.value( ancestor );
				if (!block) break;
				types.insert(0, ancestor);
				ancestor = block->ancestor;
			}
			foreach(QString ancestor, types)
			{
				NifBlock * block = blocks.value( ancestor );
				if (!block) break;
				int cn = branch->childCount();
				int n = block->types.count();
				if (n > 0) {
					beginInsertRows( index, cn, cn+n-1 );
					branch->prepareInsert( n );
					foreach ( NifData data, block->types )
						insertType( branch, data );
					endInsertRows();
				}
			}
		}

		if ( ! fast )
		{
			updateHeader();
			updateLinks();
			updateFooter();
			emit linksChanged();
		}
	}
}

bool NifModel::holdUpdates(bool value)
{
	bool retval = lockUpdates;
	if (lockUpdates == value)
		return retval;
	lockUpdates = value;
	if (!lockUpdates)
	{
		updateModel(needUpdates);
		needUpdates = utNone;
	}
	return retval;
}

void NifModel::updateModel( UpdateType value )
{
	if (value & utHeader) updateHeader();
	if (value & utLinks)  updateLinks();
	if (value & utFooter) updateFooter();
	if (value & utLinks)  emit linksChanged();
}

