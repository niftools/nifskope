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

#include "nifmodel.h"

#include "message.h"
#include "spellbook.h"
#include "data/niftypes.h"
#include "io/nifstream.h"

#include <QByteArray>
#include <QColor>
#include <QDebug>
#include <QFile>
#include <QSettings>



//! @file nifmodel.cpp The NIF data model.

NifModel::NifModel( QObject * parent ) : BaseModel( parent )
{
	updateSettings();

	clear();
}

void NifModel::updateSettings()
{
	QSettings settings;

	settings.beginGroup( "Settings/NIF/Startup Defaults" );

	cfg.startupVersion = settings.value( "Version", "20.0.0.5" ).toString();
	cfg.userVersion = settings.value( "User Version", "11" ).toInt();
	cfg.userVersion2 = settings.value( "User Version 2", "11" ).toInt();

	settings.endGroup();
}

QString NifModel::version2string( quint32 v )
{
	if ( v == 0 )
		return QString();

	QString s;

	if ( v < 0x0303000D ) {
		//This is an old-style 2-number version with one period
		s = QString::number( ( v >> 24 ) & 0xff, 10 ) + "."
		    + QString::number( ( v >> 16 ) & 0xff, 10 );

		quint32 sub_num1 = ( (v >> 8) & 0xff );
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
	if ( s.isEmpty() )
		return 0;

	if ( s.contains( "." ) ) {
		QStringList l = s.split( "." );

		quint32 v = 0;

		if ( l.count() > 4 ) {
			// Should probaby post a warning here or something.  Version # has more than 3 dots in it.
			return 0;
		} else if ( l.count() == 2 ) {
			// This is an old style version number.  Take each digit following the first one at a time.
			// The first one is the major version
			v += l[0].toInt() << (3 * 8);

			if ( l[1].size() >= 1 ) {
				v += l[1].mid( 0, 1 ).toInt() << (2 * 8);
			}

			if ( l[1].size() >= 2 ) {
				v += l[1].mid( 1, 1 ).toInt() << (1 * 8);
			}

			if ( l[1].size() >= 3 ) {
				v += l[1].mid( 2, -1 ).toInt();
			}

			return v;
		}

		// This is a new style version number with dots separating the digits
		for ( int i = 0; i < 4 && i < l.count(); i++ ) {
			v += l[i].toInt( 0, 10 ) << ( (3 - i) * 8 );
		}

		return v;
	}

	bool ok;
	quint32 i = s.toUInt( &ok );
	return ( i == 0xffffffff ? 0 : i );
}

bool NifModel::evalVersion( NifItem * item, bool chkParents ) const
{
	if ( item->isVercondValid() )
		return item->versionCondition();

	item->setVersionCondition( item == root );
	if ( item->versionCondition() )
		return true;

	if ( chkParents && item->parent() ) {
		// Set false if parent is false and early reject
		item->setVersionCondition( evalVersion( item->parent(), true ) );
		if ( !item->versionCondition() )
			return false;
	}

	// Early reject for ver1/ver2
	item->setVersionCondition( item->evalVersion( version ) );
	if ( !item->versionCondition() )
		return false;

	// Early reject for vercond
	item->setVersionCondition( item->vercond().isEmpty() );
	if ( item->versionCondition() )
		return true;

	// If there is a vercond, evaluate it
	NifModelEval functor( this, getHeaderItem() );
	item->setVersionCondition( item->verexpr().evaluateBool( functor ) );

	return item->versionCondition();
}

void NifModel::clear()
{
	beginResetModel();
	fileinfo = QFileInfo();
	filename = QString();
	folder = QString();
	root->killChildren();

	NifData headerData = NifData( "NiHeader", "Header" );
	NifData footerData = NifData( "NiFooter", "Footer" );
	headerData.setIsCompound( true );
	headerData.setIsConditionless( true );
	footerData.setIsCompound( true );
	footerData.setIsConditionless( true );

	insertType( root, headerData );
	insertType( root, footerData );
	version = version2number( cfg.startupVersion );

	if ( !supportedVersions.isEmpty() && !isVersionSupported( version ) ) {
		Message::warning( nullptr, tr( "Unsupported 'Startup Version' %1 specified, reverting to 20.0.0.5" ).arg( cfg.startupVersion ) );
		version = 0x14000005;
	}
	endResetModel();

	NifItem * item = getItem( getHeaderItem(), "Version" );

	if ( item )
		item->value().setFileVersion( version );

	QString header_string;

	if ( version <= 0x0A000100 ) {
		header_string = "NetImmerse File Format, Version ";
	} else {
		header_string = "Gamebryo File Format, Version ";
	}

	header_string += version2string( version );

	set<QString>( getHeaderItem(), "Header String", header_string );

	if ( version >= 0x14000005 ) {
		set<int>( getHeaderItem(), "User Version", cfg.userVersion );
		set<int>( getHeaderItem(), "User Version 2", cfg.userVersion2 );
	}

	//set<int>( getHeaderItem(), "Unknown Int 3", 11 );

	if ( version < 0x0303000D ) {
		QVector<QString> copyright( 3 );
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

	return QModelIndex();
}

void NifModel::updateFooter()
{
	if ( lockUpdates ) {
		needUpdates = UpdateType( needUpdates | utFooter );
		return;
	}

	NifItem * footer = getFooterItem();

	if ( !footer )
		return;

	NifItem * roots = getItem( footer, "Roots" );

	if ( !roots )
		return;

	set<int>( footer, "Num Roots", rootLinks.count() );
	updateArrayItem( roots );

	for ( int r = 0; r < roots->childCount(); r++ )
		roots->child( r )->value().setLink( rootLinks.value( r ) );
}

/*
 *  header functions
 */

QString NifModel::extractRTTIArgs( const QString & RTTIName, NiMesh::DataStreamMetadata & metadata ) const
{
	QStringList nameAndArgs = RTTIName.split( "\x01" );
	Q_ASSERT( nameAndArgs.size() >= 1 );

	if ( nameAndArgs[0] == QLatin1String( "NiDataStream" ) ) {
		Q_ASSERT( nameAndArgs.size() == 3 );
		metadata.usage = NiMesh::DataStreamUsage( nameAndArgs[1].toInt() );
		metadata.access = NiMesh::DataStreamAccess( nameAndArgs[2].toInt() );
	}

	return nameAndArgs[0];
}

QString NifModel::createRTTIName( const QModelIndex & iBlock ) const
{
	return createRTTIName( static_cast<NifItem *>(iBlock.internalPointer()) );
}

QString NifModel::createRTTIName( const NifItem * block ) const
{
	if ( !block )
		return {};

	QString blockName = block->name();
	if ( blockName == QLatin1String( "NiDataStream" ) ) {
		blockName = QString( "NiDataStream\x01%1\x01%2" )
			.arg( block->child( "Usage" )->value().get<quint32>() )
			.arg( block->child( "Access" )->value().get<quint32>() );
	}

	return blockName;
}

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
	emit beginUpdateHeader();

	if ( lockUpdates ) {
		needUpdates = UpdateType( needUpdates | utHeader );
		return;
	}

	NifItem * header = getHeaderItem();

	set<int>( header, "Num Blocks", getBlockCount() );
	NifItem * idxBlockTypes = getItem( header, "Block Types" );
	NifItem * idxBlockTypeIndices = getItem( header, "Block Type Index" );
	NifItem * idxBlockSize = getItem( header, "Block Size" );
	// 20.3.1.2 Custom Version
	NifItem * idxBlockTypeHashes = nullptr;
	if ( version == 0x14030102 )
		idxBlockTypeHashes = getItem( header, "Block Type Hashes" );

	// Update Block Types, Block Type Index, and Block Size
	if ( (idxBlockTypes || idxBlockTypeHashes) && idxBlockTypeIndices ) {
		QVector<QString> blocktypes;
		QVector<int> blocktypeindices;
		QVector<int> blocksizes;

		for ( int r = 1; r < root->childCount() - 1; r++ ) {
			NifItem * block = root->child( r );
			QString blockName = createRTTIName( block );

			int bTypeIdx = blocktypes.indexOf( blockName );
			if ( bTypeIdx < 0 ) {
				blocktypes.append( blockName );
				bTypeIdx = blocktypes.count() - 1;
			}
			
			blocktypeindices.append( bTypeIdx );

			if ( version >= 0x14020000 && idxBlockSize ) {
				updateArrays( block );
				blocksizes.append( blockSize( block ) );
			}

		}

		set<int>( header, "Num Block Types", blocktypes.count() );

		if ( idxBlockTypes )
			updateArrayItem( idxBlockTypes );
		updateArrayItem( idxBlockTypeIndices );

		if ( version >= 0x14020000 && idxBlockSize ) {
			updateArrayItem( idxBlockSize );
		}

		setState( Processing );
		idxBlockTypeIndices->setArray<int>( blocktypeindices );
		if ( idxBlockTypes )
			idxBlockTypes->setArray<QString>( blocktypes );
		if ( blocksizes.count() )
			idxBlockSize->setArray<int>( blocksizes );
		// 20.3.1.2 Custom Version
		if ( idxBlockTypeHashes ) {
			QVector<quint32> blocktypehashes;
			for ( const auto & t : blocktypes )
				blocktypehashes << DJB1Hash( t.toStdString().c_str() );

			updateArrayItem( idxBlockTypeHashes );
			idxBlockTypeHashes->setArray<quint32>( blocktypehashes );
		}

		restoreState();

		// For 20.1 and above strings are saved in the header.  Max String Length must be updated.
		if ( version >= 0x14010003 ) {
			int maxlen = 0;
			int nstrings = get<uint>( header, "Num Strings" );
			QModelIndex iArray = getIndex( getHeader(), "Strings" );

			if ( nstrings > 0 && iArray.isValid() ) {
				for ( int row = 0; row < nstrings; ++row ) {
					int len = get<QString>( iArray.child( row, 0 ) ).length();

					if ( len > maxlen )
						maxlen = len;
				}
			}

			set<uint>( header, "Max String Length", maxlen );
		}
	}
}

/*
 *  search and find
 */

NifItem * NifModel::getItem( NifItem * item, const QString & name ) const
{
	if ( !item || item == root )
		return nullptr;

	if ( item->isArray() || item->parent()->isArray() ) {
		int slash = name.indexOf( QLatin1String("\\") );
		if ( slash > 0 ) {
			QString left = name.left( slash );
			QString right = name.right( name.length() - slash - 1 );

			// Resolve ../ for arr1, arr2, arg passing
			if ( left == ".." )
				return getItem( item->parent(), right );

			return getItem( getItem( item, left ), right );
		}
	}

	for ( auto child : item->children() ) {
		if ( child && child->name() == name && evalCondition( child ) )
			return child;
	}

	return nullptr;
}

/*
 *  array functions
 */

static QString parentPrefix( const QString & x )
{
	for ( int c = 0; c < x.length(); c++ ) {
		if ( !x[c].isNumber() )
			return QString( "..\\" ) + x;
	}

	return x;
}

bool NifModel::updateByteArrayItem( NifItem * array )
{
	// New row count
	int rows = getArraySize( array );
	if ( rows == 0 )
		return false;

	// Create byte array for holding blob data
	QByteArray bytes;
	bytes.resize( rows );

	// Previous row count
	int itemRows = array->childCount();

	// Grab data from existing rows if appropriate and then purge
	if ( itemRows > 1 ) {
		for ( int i = 0; i < itemRows; i++ ) {
			if ( NifItem * child = array->child( 0 ) ) {
				bytes[i] = get<quint8>( child );
			}
		}

		beginRemoveRows( createIndex( array->row(), 0, array ), 0, itemRows - 1 );
		array->removeChildren( 0, itemRows );
		endRemoveRows();

		itemRows = 0;
	}

	// Create the dummy row for holding the byte array
	if ( itemRows == 0 ) {
		NifData data( array->name(), array->type(), array->temp(), NifValue( NifValue::tBlob ), parentPrefix( array->arg() ) );
		data.setBinary( true );

		beginInsertRows( createIndex( array->row(), 0, array ), 0, 1 );

		array->prepareInsert( 1 );
		insertType( array, data );

		endInsertRows();
	}

	// Update the byte array
	if ( NifItem * child = array->child( 0 ) ) {
		QByteArray * bm = (child->isBinary()) ? get<QByteArray *>( child ) : nullptr;
		if ( !bm ) {
			set<QByteArray>( child, bytes );
		} else if ( bm->size() == 0 ) {
			*bm = bytes;
		} else {
			bm->resize( rows );
		}
	}

	return true;
}

bool NifModel::updateArrayItem( NifItem * array )
{
	if ( !isArray( array ) )
		return false;

	// New row count
	int rows = getArraySize( array );

	// Binary array handling
	if ( array->isBinary() ) {
		if ( updateByteArrayItem( array ) )
			return true;
	}

	// Error handling
	if ( rows > 1024 * 1024 * 8 ) {
		auto m = tr( "[%1] Array %2 much too large. %3 bytes requested" ).arg( getBlockNumber( array ) )
			.arg( array->name() ).arg( rows );
		if ( msgMode == UserMessage ) {
			Message::append( nullptr, tr( readFail ), m, QMessageBox::Critical );
		} else {
			testMsg( m );
		}

		return false;
	} else if ( rows < 0 ) {
		auto m = tr( "[%1] Array %2 invalid" ).arg( getBlockNumber( array ) ).arg( array->name() );
		if ( msgMode == UserMessage ) {
			Message::append( nullptr, tr( readFail ), m, QMessageBox::Critical );
		} else {
			testMsg( m );
		}

		return false;
	}

	// Previous row count
	int itemRows = array->childCount();

	// Add item children
	if ( rows > itemRows ) {
		NifData data( array->name(),
					  array->type(),
					  array->temp(),
					  NifValue( NifValue::type( array->type() ) ),
					  parentPrefix( array->arg() ),
					  parentPrefix( array->arr2() ) // arr1 in children is parent arr2
		);

		// Fill data flags
		data.setIsConditionless( true );
		data.setIsCompound( array->isCompound() );
		data.setIsArray( array->isMultiArray() );

		beginInsertRows( createIndex( array->row(), 0, array ), itemRows, rows - 1 );

		array->prepareInsert( rows - itemRows );

		for ( int c = itemRows; c < rows; c++ )
			insertType( array, data );

		endInsertRows();
	}

	// Remove item children
	if ( rows < itemRows ) {
		beginRemoveRows( createIndex( array->row(), 0, array ), rows, itemRows - 1 );

		array->removeChildren( rows, itemRows - rows );

		endRemoveRows();
	}

	if ( (state != Loading) && (rows != itemRows) && (isCompound( array->type() ) || NifValue::isLink( NifValue::type( array->type() ) )) ) {
		NifItem * parent = array;

		while ( parent->parent() && parent->parent() != root )
			parent = parent->parent();

		if ( parent != getFooterItem() ) {
			updateLinks();
			updateFooter();
			emit linksChanged();
		}
	}

	return true;
}

bool NifModel::updateArrays( NifItem * parent )
{
	if ( !parent )
		return false;

	for ( auto child : parent->children() ) {
		if ( evalCondition( child ) ) {
			if ( isArray( child ) ) {
				if ( !updateArrayItem( child ) || !updateArrays( child ) )
					return false;
			} else if ( child->childCount() > 0 ) {
				if ( !updateArrays( child ) )
					return false;
			}
		}
	}

	return true;
}

/*
 *  block functions
 */

QModelIndex NifModel::insertNiBlock( const QString & identifier, int at )
{
	NifBlockPtr block = blocks.value( identifier );

	if ( block ) {
		if ( at < 0 || at > getBlockCount() )
			at = -1;

		if ( at >= 0 )
			adjustLinks( root, at, 1 );

		if ( at >= 0 )
			at++;
		else
			at = getBlockCount() + 1;

		beginInsertRows( QModelIndex(), at, at );

		NifItem * branch = insertBranch( root, NifData( identifier, "NiBlock", block->text ), at );
		branch->setCondition( true );

		endInsertRows();

		if ( !block->ancestor.isEmpty() )
			insertAncestor( branch, block->ancestor );

		branch->prepareInsert( block->types.count() );

		for ( const NifData& data : block->types ) {
			insertType( branch, data );
		}

		if ( state != Loading ) {
			updateHeader();
			updateLinks();
			updateFooter();
			emit linksChanged();
		}

		return createIndex( branch->row(), 0, branch );
	}

	if ( msgMode == UserMessage ) {
		Message::critical( nullptr, tr( "Could not insert NiBlock." ), tr( "unknown block %1" ).arg( identifier ) );
	} else {
		testMsg( tr( "unknown block %1" ) );
	}
	
	return QModelIndex();
}

void NifModel::removeNiBlock( int blocknum )
{
	if ( blocknum < 0 || blocknum >= getBlockCount() )
		return;

	adjustLinks( root, blocknum, 0 );
	adjustLinks( root, blocknum, -1 );
	beginRemoveRows( QModelIndex(), blocknum + 1, blocknum + 1 );
	root->removeChild( blocknum + 1 );
	endRemoveRows();
	updateLinks();
	updateFooter();
	emit linksChanged();
}

void NifModel::moveNiBlock( int src, int dst )
{
	if ( src < 0 || src >= getBlockCount() )
		return;

	beginRemoveRows( QModelIndex(), src + 1, src + 1 );
	NifItem * block = root->takeChild( src + 1 );
	endRemoveRows();

	if ( dst >= 0 )
		dst++;

	beginInsertRows( QModelIndex(), dst, dst );
	dst = root->insertChild( block, dst ) - 1;
	endInsertRows();

	QMap<qint32, qint32> map;

	if ( src < dst ) {
		for ( int l = src; l <= dst; l++ )
			map.insert( l, l - 1 );
	} else {
		for ( int l = dst; l <= src; l++ )
			map.insert( l, l + 1 );
	}


	map.insert( src, dst );

	mapLinks( root, map );

	updateLinks();
	updateHeader();
	updateFooter();
	emit linksChanged();
}

void NifModel::updateStrings( NifModel * src, NifModel * tgt, NifItem * item )
{
	if ( !item )
		return;

	NifValue::Type vt = item->value().type();

	if ( vt == NifValue::tStringIndex || vt == NifValue::tSizedString || item->type() == "string" ) {
		QString str = src->string( src->createIndex( 0, 0, item ) );
		tgt->assignString( tgt->createIndex( 0, 0, item ), str, false );
	}

	for ( auto child : item->children() ) {
		updateStrings( src, tgt, child );
	}
}

QMap<qint32, qint32> NifModel::moveAllNiBlocks( NifModel * targetnif, bool update )
{
	int bcnt = getBlockCount();

	bool doStringUpdate = (  this->getVersionNumber() >= 0x14010003 || targetnif->getVersionNumber() >= 0x14010003 );

	QMap<qint32, qint32> map;

	beginRemoveRows( QModelIndex(), 1, bcnt );
	targetnif->beginInsertRows( QModelIndex(), targetnif->getBlockCount(), targetnif->getBlockCount() + bcnt - 1 );

	for ( int i = 0; i < bcnt; i++ ) {
		map.insert( i, targetnif->root->insertChild( root->takeChild( 1 ), targetnif->root->childCount() - 1 ) - 1 );
	}

	endRemoveRows();
	targetnif->endInsertRows();

	for ( int i = 0; i < bcnt; i++ ) {
		NifItem * item = targetnif->root->child( targetnif->getBlockCount() - i );
		targetnif->mapLinks( item, map );

		if ( doStringUpdate )
			updateStrings( this, targetnif, item );
	}

	if ( update ) {
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

	QString err = tr( "NifModel::reorderBlocks() - invalid argument" );

	if ( order.count() != getBlockCount() ) {
		if ( msgMode == UserMessage ) {
			Message::critical( nullptr, err );
		} else {
			testMsg( err );
		}
		return;
	}

	QMap<qint32, qint32> linkMap;
	QMap<qint32, qint32> blockMap;

	for ( qint32 n = 0; n < order.count(); n++ ) {
		if ( blockMap.contains( order[n] ) || order[n] < 0 || order[n] >= getBlockCount() ) {
			if ( msgMode == UserMessage ) {
				Message::critical( nullptr, err );
			} else {
				testMsg( err );
			}
			
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
	QList<NifItem *> temp;

	for ( qint32 n = 0; n < order.count(); n++ )
		temp.append( root->takeChild( 1 ) );

	endRemoveRows();

	// then insert them again in the new order
	beginInsertRows( QModelIndex(), 1, temp.count() );
	for ( const auto n : blockMap ) {
		root->insertChild( temp[ n ], root->childCount() - 1 );
	}
	endInsertRows();

	mapLinks( root, linkMap );
	updateLinks();
	emit linksChanged();

	updateHeader();
	updateFooter();
}

void NifModel::mapLinks( const QMap<qint32, qint32> & map )
{
	mapLinks( root, map );
	updateLinks();
	emit linksChanged();

	updateHeader();
	updateFooter();
}

QString NifModel::getBlockName( const QModelIndex & idx ) const
{
	const NifItem * block = static_cast<NifItem *>( idx.internalPointer() );

	if ( block ) {
		return block->name();
	}

	return {};
}

QString NifModel::getBlockType( const QModelIndex & idx ) const
{
	const NifItem * block = static_cast<NifItem *>( idx.internalPointer() );

	if ( block ) {
		return block->type();
	}

	return {};
}

int NifModel::getBlockNumber( const QModelIndex & idx ) const
{
	if ( !( idx.isValid() && idx.model() == this ) )
		return -1;

	const NifItem * block = static_cast<NifItem *>( idx.internalPointer() );

	while ( block && block->parent() != root )
		block = block->parent();

	if ( !block )
		return -1;

	int num = block->row() - 1;

	if ( num >= getBlockCount() )
		num = -1;

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

	if ( !block )
		return -1;

	int num = block->row() - 1;

	if ( num >= getBlockCount() )
		num = -1;

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

	return QModelIndex();
}

bool NifModel::isNiBlock( const QModelIndex & index, const QString & name ) const
{
	NifItem * item = static_cast<NifItem *>( index.internalPointer() );

	if ( index.isValid() && item && item->parent() == root && getBlockNumber( item ) >= 0 ) {
		if ( name.isEmpty() )
			return true;

		return item->name() == name;
	}

	return false;
}

bool NifModel::isNiBlock( const QModelIndex & index, const QStringList & names ) const
{
	for ( const QString & name : names ) {
		if ( isNiBlock( index, name ) )
			return true;
	}

	return false;
}

NifItem * NifModel::getBlockItem( int x ) const
{
	if ( x < 0 || x >= getBlockCount() )
		return nullptr;

	return root->child( x + 1 );
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
	setState( Inserting );

	Q_UNUSED( at );
	NifBlockPtr ancestor = blocks.value( identifier );

	if ( ancestor ) {
		if ( !ancestor->ancestor.isEmpty() )
			insertAncestor( parent, ancestor->ancestor );

		//parent->insertChild( NifData( identifier, "Abstract" ) );
		parent->prepareInsert( ancestor->types.count() );
		for ( const NifData& data : ancestor->types ) {
			insertType( parent, data );
		}
	} else {
		if ( msgMode == UserMessage ) {
			Message::warning( nullptr, tr( "Cannot insert parent." ), tr( "unknown parent %1" ).arg( identifier ) );
		} else {
			testMsg( tr( "unknown parent %1" ).arg( identifier ) );
		}
		
	}

	restoreState();
}

bool NifModel::inherits( const QString & name, const QString & aunty ) const
{
	if ( name == aunty )
		return true;

	NifBlockPtr type = blocks.value( name );

	if ( type && ( type->ancestor == aunty || inherits( type->ancestor, aunty ) ) )
		return true;

	return false;
}

bool NifModel::inherits( const QString & name, const QStringList & ancestors ) const
{
	for ( const auto & a : ancestors ) {
		if ( inherits( name, a ) )
			return true;
	}

	return false;
}

bool NifModel::inherits( const QModelIndex & idx, const QString & aunty ) const
{
	int x = getBlockNumber( idx );

	if ( x < 0 )
		return false;

	return inherits( itemName( index( x + 1, 0 ) ), aunty );
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
	setState( Inserting );

	if ( data.isArray() ) {
		NifItem * item = insertBranch( parent, data, at );
	} else if ( data.isCompound() ) {
		NifBlockPtr compound = compounds.value( data.type() );
		if ( !compound )
			return;
		NifItem * branch = insertBranch( parent, data, at );
		branch->prepareInsert( compound->types.count() );
		for ( const NifData & d : compound->types ) {
			insertType( branch, d );
		}
	} else if ( data.isMixin() ) {
		NifBlockPtr compound = compounds.value( data.type() );
		if ( !compound )
			return;
		parent->prepareInsert( compound->types.count() );
		for ( const NifData & d : compound->types ) {
			insertType( parent, d );
		}
	} else if ( data.isTemplated() ) {
		QLatin1String tmpl( "TEMPLATE" );
		QString tmp = parent->temp();
		NifItem * tItem = parent;

		while ( tmp == tmpl && tItem->parent() ) {
			tItem = tItem->parent();
			tmp = tItem->temp();
		}

		NifData d( data );

		if ( d.type() == tmpl ) {
			d.value.changeType( NifValue::type( tmp ) );
			d.setType( tmp );
			// The templates are now filled
			d.setTemplated( false );
		}

		if ( d.temp() == tmpl )
			d.setTemp( tmp );

		insertType( parent, d, at );
	} else {
		NifItem * item = parent->insertChild( data, at );

		// Kludge for string conversion.
		//  Ensure that the string type is correct for the nif version
		if ( item->value().type() == NifValue::tString || item->value().type() == NifValue::tFilePath ) {
			item->value().changeType( version < 0x14010003 ? NifValue::tSizedString : NifValue::tStringIndex );
		}
	}

	restoreState();
}


/*
 *  item value functions
 */

bool NifModel::setItemValue( NifItem * item, const NifValue & val )
{
	item->value() = val;
	emit dataChanged( createIndex( item->row(), ValueCol, item ), createIndex( item->row(), ValueCol, item ) );

	if ( itemIsLink( item ) ) {
		NifItem * parent = item;

		while ( parent->parent() && parent->parent() != root )
			parent = parent->parent();

		if ( parent != getFooterItem() ) {
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
	if ( index != idx )
		return data( index, role );

	NifItem * item = static_cast<NifItem *>( index.internalPointer() );

	if ( !( index.isValid() && item && index.model() == this ) )
		return QVariant();

	int column = index.column();

	bool ndr = role == NifSkopeDisplayRole;

	if ( role == NifSkopeDisplayRole )
		role = Qt::DisplayRole;

	switch ( role ) {
	case Qt::DisplayRole:
		{
			switch ( column ) {
			case NameCol:
				{
					if ( ndr )
						return item->name();

					QString a = "";

					if ( itemType( index ) == "NiBlock" )
						a = QString::number( getBlockNumber( index ) ) + " ";

					return a + item->name();
				}
				break;
			case TypeCol:
				{
					if ( !item->temp().isEmpty() ) {
						NifItem * i = item;

						while ( i && i->temp() == "TEMPLATE" )
							i = i->parent();

						return QString( "%1<%2>" ).arg( item->type(), i ? i->temp() : QString() );
					}

					return item->type();
				}
				break;
			case ValueCol:
				{
					const NifValue & value = item->value();

					if ( value.type() == NifValue::tString || value.type() == NifValue::tFilePath ) {
						return QString( this->string( index ) ).replace( "\n", " " ).replace( "\r", " " );
					}
					else if ( value.type() == NifValue::tStringOffset )
					{
						int ofs = value.get<int>();
						if ( ofs < 0 || ofs == 0x0000FFFF )
							return QString( "<empty>" );

						NifItem * palette = getItemX( item, "String Palette" );
						int link = ( palette ? palette->value().toLink() : -1 );

						if ( ( palette = getBlockItem( link ) ) && ( palette = getItem( palette, "Palette" ) ) ) {
							QByteArray bytes = palette->value().get<QByteArray>();

							if ( !(ofs < bytes.count()) )
								return tr( "<offset invalid>" );

							return QString( &bytes.data()[ofs] );
						}

						return tr( "<palette not found>" );
					}
					else if ( value.type() == NifValue::tStringIndex )
					{
						int idx = value.get<int>();
						if ( idx == -1 )
							return QString();

						NifItem * header = getHeaderItem();
						QModelIndex stringIndex = createIndex( header->row(), 0, header );
						QString string = get<QString>( this->index( idx, 0, getIndex( stringIndex, "Strings" ) ) );

						if ( idx < 0 )
							return tr( "%1 - <index invalid>" ).arg( idx );

						return QString( "%2 [%1]" ).arg( idx ).arg( string );
					}
					else if ( value.type() == NifValue::tBlockTypeIndex )
					{

						int idx = value.get<int>();
						int offset = idx & 0x7FFF;
						NifItem * blocktypes = getItemX( item, "Block Types" );
						NifItem * blocktyp = ( blocktypes ? blocktypes->child( offset ) : 0 );

						if ( !blocktyp )
							return tr( "%1 - <index invalid>" ).arg( idx );

						return QString( "%2 [%1]" ).arg( idx ).arg( blocktyp->value().get<QString>() );
					}
					else if ( value.isLink() )
					{
						int lnk = value.toLink();

						if ( lnk >= 0 ) {
							QModelIndex block = getBlock( lnk );

							if ( !block.isValid() )
								return tr( "%1 <invalid>" ).arg( lnk );

							QModelIndex block_name = getIndex( block, "Name" );

							if ( block_name.isValid() && !get<QString>( block_name ).isEmpty() )
								return QString( "%1 (%2)" ).arg( lnk ).arg( get<QString>( block_name ) );

							return QString( "%1 [%2]" ).arg( lnk ).arg( itemName( block ) );
						}

						return tr( "None" );
					}
					else if ( value.isCount() )
					{
						QString optId = NifValue::enumOptionName( item->type(), value.toCount() );

						if ( optId.isEmpty() )
							return value.toString();

						return QString( "%1" ).arg( optId );
					}

					return value.toString().replace( "\n", " " ).replace( "\r", " " );
				}
				break;
			case ArgCol:
				return item->arg();
			case Arr1Col:
				return item->arr1();
			case Arr2Col:
				return item->arr2();
			case CondCol:
				return item->cond();
			case Ver1Col:
				return version2string( item->ver1() );
			case Ver2Col:
				return version2string( item->ver2() );
			case VerCondCol:
				return item->vercond();
			default:
				return QVariant();
			}
		}
	case Qt::DecorationRole:
		{
			switch ( column ) {
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
			switch ( column ) {
			case NameCol:
				return item->name();
			case TypeCol:
				return item->type();
			case ValueCol:
				{
					const NifValue & value = item->value();

					if ( value.type() == NifValue::tString || value.type() == NifValue::tFilePath ) {
						return string( index );
					}

					return item->value().toVariant();
				}
			case ArgCol:
				return item->arg();
			case Arr1Col:
				return item->arr1();
			case Arr2Col:
				return item->arr2();
			case CondCol:
				return item->cond();
			case Ver1Col:
				return version2string( item->ver1() );
			case Ver2Col:
				return version2string( item->ver2() );
			case VerCondCol:
				return item->vercond();
			default:
				return QVariant();
			}
		}
	case Qt::ToolTipRole:
		{
			switch ( column ) {
			case NameCol:
				{
					if ( item->parent() && isArray( item->parent() ) ) {
						return QString( "array index: %1" ).arg( item->row() );
					} else {
						QString tip = QString( "<p><b>%1</b></p><p>%2</p>" )
						              .arg( item->name() )
						              .arg( QString( item->text() ).replace( "<", "&lt;" ).replace( "\n", "<br/>" ) );

						if ( NifBlockPtr blk = blocks.value( item->name() ) ) {
							tip += "<p>Ancestors:<ul>";

							while ( blocks.contains( blk->ancestor ) ) {
								tip += QString( "<li>%1</li>" ).arg( blk->ancestor );
								blk  = blocks.value( blk->ancestor );
							}

							tip += "</ul></p>";
						}

						return tip;
					}
				}
				break;
			case TypeCol:
				return NifValue::typeDescription( item->type() );
			case ValueCol:
				{
					switch ( item->value().type() ) {
					case NifValue::tByte:
					case NifValue::tWord:
					case NifValue::tShort:
					case NifValue::tBool:
					case NifValue::tInt:
					case NifValue::tUInt:
					case NifValue::tULittle32:
						{
							return tr( "dec: %1\nhex: 0x%2" )
							       .arg( item->value().toString() )
							       .arg( item->value().toCount(), 8, 16, QChar( '0' ) );
						}
					case NifValue::tFloat:
					case NifValue::tHfloat:
						{
							return tr( "float: %1\nhex: 0x%2" )
							       .arg( NumOrMinMax( item->value().toFloat(), 'g', 8 ) )
							       .arg( item->value().toCount(), 8, 16, QChar( '0' ) );
						}
					case NifValue::tFlags:
						{
							quint16 f = item->value().toCount();
							return tr( "dec: %1\nhex: 0x%2\nbin: 0b%3" )
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
					case NifValue::tHalfVector3:
						return item->value().get<HalfVector3>().toHtml();
					case NifValue::tByteVector3:
						return item->value().get<ByteVector3>().toHtml();
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
					case NifValue::tByteColor4:
						{
							Color4 c = item->value().get<ByteColor4>();
							return QString( "R %1\nG %2\nB %3\nA %4" )
								.arg( c[0] )
								.arg( c[1] )
								.arg( c[2] )
								.arg( c[3] );
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
				}
				break;
			default:
				break;
			}
		}
		return QVariant();
	case Qt::BackgroundColorRole:
		{
			// "notify" about an invalid index in "Triangles"
			// TODO: checkbox, "show invalid only"
			if ( column == ValueCol && item->value().type() == NifValue::tTriangle ) {
				NifItem * nv = findItemX( item, "Num Vertices" );

				if ( !nv ) {
					qDebug() << "Num Vertices is null";
					return QVariant();
				}

				quint32 nvc = nv->value().toCount();
				Triangle t  = item->value().get<Triangle>();

				if ( t[0] >= nvc || t[1] >= nvc || t[2] >= nvc )
					return QColor::fromRgb( 240, 210, 210 );
			}

			if ( column == ValueCol && item->value().isColor() ) {
				return item->value().toColor();
			}
		}
		return QVariant();
	case Qt::UserRole:
		{
			if ( column == ValueCol ) {
				SpellPtr spell = SpellBook::instant( this, index );

				if ( spell )
					return spell->page() + "/" + spell->name();
			}
		}
		return QVariant();
	default:
		return QVariant();
	}
}

bool NifModel::setData( const QModelIndex & index, const QVariant & value, int role )
{
	NifItem * item = static_cast<NifItem *>( index.internalPointer() );

	if ( !( index.isValid() && role == Qt::EditRole && index.model() == this && item ) )
		return false;

	// Set Buddy
	QModelIndex idx = buddy( index );
	if ( index != idx )
		return setData( idx, value, role );

	switch ( index.column() ) {
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
			NifValue & val = item->value();

			if ( val.type() == NifValue::tString || val.type() == NifValue::tFilePath ) {
				val.changeType( version < 0x14010003 ? NifValue::tSizedString : NifValue::tStringIndex );
				assignString( index, value.toString(), true );
			} else {
				item->value().setFromVariant( value );

				if ( isLink( index ) && getBlockOrHeader( index ) != getFooter() ) {
					updateLinks();
					updateFooter();
					emit linksChanged();
				}
			}
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
	case NifModel::VerCondCol:
		item->setVerCond( value.toString() );
	default:
		return false;
	}

	// reverse buddy lookup
	if ( index.column() == ValueCol ) {
		if ( item->name() == "File Name" ) {
			NifItem * parent = item->parent();

			if ( parent && ( parent->name() == "Texture Source" || parent->name() == "NiImage" ) ) {
				parent = parent->parent();

				if ( parent && parent->type() == "NiBlock" && parent->name() == "NiSourceTexture" )
					emit dataChanged( createIndex( parent->row(), ValueCol, parent ), createIndex( parent->row(), ValueCol, parent ) );
			}
		} else if ( item->name() == "Name" ) {
			NifItem * parent = item->parent();

			if ( parent && parent->type() == "NiBlock" )
				emit dataChanged( createIndex( parent->row(), ValueCol, parent ), createIndex( parent->row(), ValueCol, parent ) );
		}
	}

	if ( state == Default ) {
		// Reassess conditions for reliant data only when modifying value
		invalidateDependentConditions( item );
		// update original index
		emit dataChanged( index, index );
	}

	return true;
}

void NifModel::reset()
{
	beginResetModel();
	resetState();
	updateLinks();
	endResetModel();
}

bool NifModel::removeRows( int row, int count, const QModelIndex & parent )
{
	NifItem * item = static_cast<NifItem *>( parent.internalPointer() );

	if ( !( parent.isValid() && parent.model() == this && item ) )
		return false;

	if ( row >= 0 && ( (row + count) <= item->childCount() ) ) {
		bool link = false;

		for ( int r = row; r < row + count; r++ )
			link |= itemIsLink( item->child( r ) );

		beginRemoveRows( parent, row, row + count );
		item->removeChildren( row, count );
		endRemoveRows();

		if ( link ) {
			updateLinks();
			updateFooter();
			emit linksChanged();
		}

		return true;
	}

	return false;
}

QModelIndex NifModel::buddy( const QModelIndex & index ) const
{
	NifItem * item = static_cast<NifItem *>(index.internalPointer());
	if ( !item )
		return QModelIndex();

	QModelIndex buddy;
	if ( index.column() == ValueCol && item->parent() == root && item->type() == "NiBlock" ) {

		if ( item->name() == "NiSourceTexture" || item->name() == "NiImage" ) {
			buddy = getIndex( index, "File Name" );
		} else if ( item->name() == "NiStringExtraData" ) {
			buddy = getIndex( index, "String Data" );
		} else {
			buddy = getIndex( index, "Name" );
		}

		if ( buddy.isValid() )
			buddy = buddy.sibling( buddy.row(), ValueCol );

		if ( buddy.isValid() )
			return buddy;
	} else if ( index.column() == ValueCol && item->parent() != root ) {

		if ( item->type() == "ControlledBlock" && item->name() == "Controlled Blocks" ) {
			if ( version >= 0x14010003 ) {
				buddy = getIndex( index, "Node Name" );
			} else if ( version <= 0x14000005 ) {
				buddy = getIndex( index, "Node Name Offset" );
			}

			if ( buddy.isValid() )
				buddy = buddy.sibling( buddy.row(), ValueCol );

			return buddy;
		}
	}

	return index;
}


/*
 *  load and save
 */

bool NifModel::setHeaderString( const QString & s )
{
	if ( !( s.startsWith( "NetImmerse File Format" ) || s.startsWith( "Gamebryo" ) // official
	        || s.startsWith( "NDSNIF" )                                            // altantica
	        || s.startsWith( "NS" )                                                // neosteam
	        || s.startsWith( "Joymaster HS1 Object Format - (JMI)" )               // howling sword, uses .jmi extension
	     ) )
	{
		auto m = tr( "Could not open %1 because it is not a supported type." ).arg( fileinfo.fileName() );
		if ( msgMode == UserMessage ) {
			Message::critical( nullptr, m );
		} else {
			testMsg( m );
		}

		return false;
	}

	int p = s.indexOf( "Version", 0, Qt::CaseInsensitive );

	if ( p >= 0 ) {
		QString v = s;

		v.remove( 0, p + 8 );

		for ( int i = 0; i < v.length(); i++ ) {
			if ( v[i].isDigit() || v[i] == QChar( '.' ) ) {
				continue;
			} else {
				v = v.left( i );
			}
		}

		version = version2number( v );

		if ( !isVersionSupported( version ) ) {
			auto m = tr( "Version %1 (%2) is not supported." ).arg( version2string( version ), v );
			if ( msgMode == UserMessage ) {
				Message::critical( nullptr, m );
			} else {
				testMsg( m );
			}
			
			return false;
		}

		return true;
	} else if ( s.startsWith( "NS" ) ) {
		// Dodgy version for NeoSteam
		version = 0x0a010000;
		return true;
	}

	if ( msgMode == UserMessage ) {
		Message::critical( nullptr, tr( "Invalid header string" ) );
	} else {
		testMsg( tr( "Invalid header string" ) );
	}
	
	return false;
}

bool NifModel::load( QIODevice & device )
{
	QSettings settings;
	bool ignoreSize = settings.value( "Ignore Block Size", true ).toBool();

	clear();

	NifIStream stream( this, &device );

	if ( state != Loading )
		setState( Loading );

	// read header
	NifItem * header = getHeaderItem();
	if ( !header || !loadHeader( header, stream ) ) {
		auto m = tr( "failed to load file header (version %1, %2)" ).arg( version, 0, 16 ).arg( version2string( version ) );
		if ( msgMode == UserMessage ) {
			Message::critical( nullptr, tr( "The file could not be read. See Details for more information." ), m );
		} else {
			testMsg( m );
		}

		resetState();
		return false;
	}

	int numblocks = 0;
	numblocks = get<int>( header, "Num Blocks" );
	//qDebug( "numblocks %i", numblocks );

	emit sigProgress( 0, numblocks );
	//QTime t = QTime::currentTime();

	qint64 curpos = 0;
	try
	{
		curpos = device.pos();

		if ( version >= 0x0303000d ) {
			// read in the NiBlocks
			QString prevblktyp;

			for ( int c = 0; c < numblocks; c++ ) {
				emit sigProgress( c + 1, numblocks );

				if ( device.atEnd() )
					throw tr( "unexpected EOF during load" );

				QString blktyp;
				quint32 size = UINT_MAX;
				try
				{
					if ( version >= 0x0a000000 ) {
						// block types are stored in the header for versions above 10.x.x.x
						//	the upper bit or the blocktypeindex seems to be related to PhysX
						int blktypidx = get<int>( index( c, 0, getIndex( createIndex( header->row(), 0, header ), "Block Type Index" ) ) );
						blktyp = get<QString>( index( blktypidx & 0x7FFF, 0, getIndex( createIndex( header->row(), 0, header ), "Block Types" ) ) );
						
						// 20.3.1.2 Custom Version
						if ( version == 0x14030102 ) {
							auto hash = get<quint32>(
								index( blktypidx & 0x7FFF, 0, getIndex( createIndex( header->row(), 0, header ),
																	   "Block Type Hashes" ) )
							);

							if ( blockHashes.contains( hash ) )
								blktyp = blockHashes[hash]->id;
							else
								throw tr( "Block Hash not found." );
						}

						// note: some 10.0.1.0 version nifs from Oblivion in certain distributions seem to be missing
						//		 these four bytes on the havok blocks
						//		 (see for instance meshes/architecture/basementsections/ungrdltraphingedoor.nif)
						if ( (version < 0x0a020000) && ( !blktyp.startsWith( "bhk" ) ) ) {
							int dummy;
							device.read( (char *)&dummy, 4 );

							if ( dummy != 0 ) {
								auto m = tr( "non-zero block separator (%1) preceeding block %2" ).arg( dummy ).arg( blktyp );
								if ( msgMode == UserMessage ) {
									Message::append( tr( "Warnings were generated while reading NIF file." ), m );
								} else {
									testMsg( m );
								}
							}
						}

						// for version 20.2.0.? and above the block size is stored in the header
						if ( !ignoreSize && version >= 0x14020000 )
							size = get<quint32>( index( c, 0, getIndex( createIndex( header->row(), 0, header ), "Block Size" ) ) );
					} else {
						int len;
						device.read( (char *)&len, 4 );

						if ( len < 2 || len > 80 )
							throw tr( "next block (%1) does not start with a NiString" ).arg( c );

						blktyp = device.read( len );
					}

					// Hack for NiMesh data streams
					NiMesh::DataStreamMetadata metadata = {};

					if ( blktyp.startsWith( "NiDataStream\x01" ) )
						blktyp = extractRTTIArgs( blktyp, metadata );

					if ( isNiBlock( blktyp ) ) {
						//qDebug() << "loading block" << c << ":" << blktyp );
						QModelIndex newBlock = insertNiBlock( blktyp, -1 );

						if ( !loadItem( root->child( c + 1 ), stream ) ) {
							NifItem * child = root->child( c );
							throw tr( "failed to load block number %1 (%2) previous block was %3" ).arg( c ).arg( blktyp ).arg( child ? child->name() : prevblktyp );
						}

						// NiMesh hack
						if ( blktyp == "NiDataStream" ) {
							set<quint32>( newBlock, "Usage", metadata.usage );
							set<quint32>( newBlock, "Access", metadata.access );
						}
					} else {
						auto m = tr( "warning: block %1 (%2) not inserted!" ).arg( c ).arg( blktyp );
						if ( msgMode == UserMessage ) {
							Message::append( tr( "Warnings were generated while reading NIF file." ), m );
						} else {
							testMsg( m );
						}

						throw tr( "encountered unknown block (%1)" ).arg( blktyp );
					}
				}
				catch ( QString & err )
				{
					// version 20.3.0.3 can mostly recover from some failures because it store block sizes
					// XXX FIXME: if isNiBlock returned false, block numbering will be screwed up!!
					if ( size == UINT_MAX )
						throw err;
				}

				// Check device position and emit warning if location is not expected
				if ( size != UINT_MAX ) {
					qint64 pos = device.pos();

					if ( (curpos + size) != pos ) {
						// unable to seek to location... abort
						if ( device.seek( curpos + size ) ) {
							auto m = tr( "device position incorrect after block number %1 (%2) at 0x%3 ended at 0x%4 (expected 0x%5)" )
								.arg( c )
								.arg( blktyp )
								.arg( QString::number( curpos, 16 ) )
								.arg( QString::number( pos, 16 ) )
								.arg( QString::number( curpos + size, 16 )
							);

							if ( msgMode == UserMessage ) {
								Message::append( tr( "Warnings were generated while reading NIF file." ), m );
							} else {
								testMsg( m );
							}
						}
						else {
							throw tr( "failed to reposition device at block number %1 (%2) previous block was %3" ).arg( c ).arg( blktyp ).arg( root->child( c )->name() );
						}
						curpos = device.pos();
					} else {
						curpos = pos;
					}
				}

				prevblktyp = blktyp;
			}

			// read in the footer
			// Disabling the throw because it hinders decoding when the XML is wrong,
			// and prevents any data whatsoever from loading.
			loadItem( getFooterItem(), stream );
			//if ( !loadItem( getFooterItem(), stream ) )
			//	throw tr( "failed to load file footer" );
		} else {
			// versions below 3.3.0.13
			QMap<qint32, qint32> linkMap;

			try {
				for ( qint32 c = 0; true; c++ ) {
					emit sigProgress( c + 1, 0 );

					if ( device.atEnd() )
						throw tr( "unexpected EOF during load" );

					int len;
					device.read( (char *)&len, 4 );

					if ( len < 0 || len > 80 )
						throw tr( "next block (%1) does not start with a NiString" ).arg( c );

					QString blktyp = device.read( len );

					if ( blktyp == "End Of File" ) {
						break;
					} else if ( blktyp == "Top Level Object" ) {
						device.read( (char *)&len, 4 );

						if ( len < 0 || len > 80 )
							throw tr( "next block (%1) does not start with a NiString" ).arg( c );

						blktyp = device.read( len );
					}

					qint32 p;
					device.read( (char *)&p, 4 );
					p -= 1;

					if ( p != c )
						linkMap.insert( p, c );

					if ( isNiBlock( blktyp ) ) {
						//qDebug() << "loading block" << c << ":" << blktyp );
						insertNiBlock( blktyp, -1 );

						if ( !loadItem( root->child( c + 1 ), stream ) )
							throw tr( "failed to load block number %1 (%2) previous block was %3" ).arg( c ).arg( blktyp ).arg( root->child( c )->name() );
					} else {
						throw tr( "encountered unknown block (%1)" ).arg( blktyp );
					}
				}
			}
			catch ( QString & err ) {
				//If this is an old file we should still map the links, even if it failed
				mapLinks( linkMap );

				//Re-throw exception so that the error is printed
				throw err;
			}

			//Also map links if nothing went wrong
			mapLinks( linkMap );
		}
	}
	catch ( QString & err )
	{
		if ( msgMode == UserMessage ) {
			Message::append( nullptr, tr( readFail ), err, QMessageBox::Critical );
		} else {
			testMsg( err );
		}
		
		reset();
		return false;
	}

	//qDebug() << t.msecsTo( QTime::currentTime() );
	reset(); // notify model views that a significant change to the data structure has occurded
	return true;
}

bool NifModel::save( QIODevice & device ) const
{
	NifOStream stream( this, &device );

	setState( Saving );

	// Force update header and footer prior to save
	if ( NifModel * mdl = const_cast<NifModel *>(this) ) {
		mdl->updateHeader();
		mdl->updateFooter();
	}

	emit sigProgress( 0, rowCount( QModelIndex() ) );

	for ( int c = 0; c < rowCount( QModelIndex() ); c++ ) {
		emit sigProgress( c + 1, rowCount( QModelIndex() ) );

		//qDebug() << "saving block " << c << ": " << itemName( index( c, 0 ) );

		if ( itemType( index( c, 0 ) ) == "NiBlock" ) {
			if ( version > 0x0a000000 ) {
				if ( version < 0x0a020000 ) {
					int null = 0;
					device.write( (char *)&null, 4 );
				}
			} else {
				if ( version < 0x0303000d ) {
					if ( rootLinks.contains( c - 1 ) ) {
						QString string = "Top Level Object";
						int len = string.length();
						device.write( (char *)&len, 4 );
						device.write( string.toLatin1().constData(), len );
					}
				}

				QString string = itemName( index( c, 0 ) );
				int len = string.length();
				device.write( (char *)&len, 4 );
				device.write( string.toLatin1().constData(), len );

				if ( version < 0x0303000d ) {
					device.write( (char *)&c, 4 );
				}
			}
		}

		if ( !saveItem( root->child( c ), stream ) ) {
			Message::critical( nullptr, tr( "Failed to write block %1 (%2)." ).arg( itemName( index( c, 0 ) ) ).arg( c - 1 ) );
			resetState();
			return false;
		}
	}

	if ( version < 0x0303000d ) {
		QString string = "End Of File";
		int len = string.length();
		device.write( (char *)&len, 4 );
		device.write( string.toLatin1().constData(), len );
	}

	resetState();
	return true;
}

bool NifModel::loadIndex( QIODevice & device, const QModelIndex & index )
{
	NifItem * item = static_cast<NifItem *>( index.internalPointer() );

	if ( item && index.isValid() && index.model() == this ) {
		NifIStream stream( this, &device );
		bool ok = loadItem( item, stream );
		updateLinks();
		updateFooter();
		emit linksChanged();
		return ok;
	}

	reset();
	return false;
}

bool NifModel::loadAndMapLinks( QIODevice & device, const QModelIndex & index, const QMap<qint32, qint32> & map )
{
	NifItem * item = static_cast<NifItem *>( index.internalPointer() );

	if ( item && index.isValid() && index.model() == this ) {
		NifIStream stream( this, &device );
		bool ok = loadItem( item, stream );
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

	if ( !f.open( QIODevice::ReadOnly ) ) {
		Message::critical( nullptr, tr( "Failed to open %1" ).arg( fname ) );
		return false;
	}

	NifIStream stream( this, &f );

	// read header
	NifItem * header = getHeaderItem();

	if ( !header || !loadHeader( header, stream ) ) {
		if ( msgMode == UserMessage ) {
			Message::critical( nullptr, tr( "Failed to load file header." ), tr( "Version %1" ).arg( version ) );
		} else {
			testMsg( tr( "Failed to load file header version %1" ).arg( version ) );
		}
		return false;
	}

	return true;
}

bool NifModel::earlyRejection( const QString & filepath, const QString & blockId, quint32 v )
{
	NifModel nif;

	if ( nif.loadHeaderOnly( filepath ) == false ) {
		//File failed to read entierly
		return false;
	}

	bool ver_match = false;

	if ( v == 0 ) {
		ver_match = true;
	} else if ( v != 0 && nif.getVersionNumber() == v ) {
		ver_match = true;
	}

	bool blk_match = false;

	if ( blockId.isEmpty() == true || v < 0x0A000100 ) {
		blk_match = true;
	} else {
		const auto & types = nif.getArray<QString>( nif.getHeader(), "Block Types" );
		for ( const QString& s : types ) {
			if ( inherits( s, blockId ) ) {
				blk_match = true;
				break;
			}
		}
	}

	return (ver_match && blk_match);
}

bool NifModel::saveIndex( QIODevice & device, const QModelIndex & index ) const
{
	NifOStream stream( this, &device );
	NifItem * item = static_cast<NifItem *>( index.internalPointer() );
	return ( item && index.isValid() && index.model() == this && saveItem( item, stream ) );
}

int NifModel::fileOffset( const QModelIndex & index ) const
{
	NifSStream stream( this );
	NifItem * target = static_cast<NifItem *>( index.internalPointer() );

	if ( target && index.isValid() && index.model() == this ) {
		int ofs = 0;

		for ( int c = 0; c < root->childCount(); c++ ) {
			if ( c > 0 && c <= getBlockCount() ) {
				if ( version > 0x0a000000 ) {
					if ( version < 0x0a020000 ) {
						ofs += 4;
					}
				} else {
					if ( version < 0x0303000d ) {
						if ( rootLinks.contains( c - 1 ) ) {
							QString string = "Top Level Object";
							ofs += 4 + string.length();
						}
					}

					QString string = itemName( this->NifModel::index( c, 0 ) );
					ofs += 4 + string.length();

					if ( version < 0x0303000d ) {
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

int NifModel::blockSize( NifItem * parent, NifSStream & stream ) const
{
	int size = 0;

	if ( !parent )
		return 0;

	for ( int row = 0; row < parent->childCount(); row++ ) {
		NifItem * child = parent->child( row );

		if ( child->isAbstract() ) {
			//qDebug() << "Not counting abstract item " << child->name();
			continue;
		}

		if ( evalCondition( child ) ) {
			if ( isArray( child ) || !child->arr2().isEmpty() || child->childCount() > 0 ) {
				if ( isArray( child ) && child->childCount() != getArraySize( child ) ) {
					if ( child->isBinary() ) {
						// special byte
					} else {
						auto m = tr( "block %1 %2 array size mismatch" ).arg( getBlockNumber( parent ) ).arg( child->name() );
						if ( msgMode == UserMessage ) {
							Message::append( tr( "Warnings were generated while reading the blocks." ), m );
						} else {
							testMsg( m );
						}
					}
				}

				size += blockSize( child, stream );
			} else {
				size += stream.size( child->value() );
			}
		}
	}

	return size;
}

bool NifModel::loadItem( NifItem * parent, NifIStream & stream )
{
	if ( !parent )
		return false;

	for ( auto child : parent->children() ) {
		if ( !child->isConditionless() )
			child->invalidateCondition();

		if ( child->isAbstract() ) {
			//qDebug() << "Not loading abstract item " << child->name();
			continue;
		}

		if ( evalCondition( child ) ) {
			if ( isArray( child ) ) {
				if ( !updateArrayItem( child ) || !loadItem( child, stream ) )
					return false;
			} else if ( child->childCount() > 0 ) {
				if ( !loadItem( child, stream ) )
					return false;
			} else {
				if ( !stream.read( child->value() ) )
					return false;
			}
		}
	}

	return true;
}

bool NifModel::loadHeader( NifItem * header, NifIStream & stream )
{
	// Load header separately and invalidate conditions before reading
	//	Compensates for < 20.0.0.5 User Version/User Version 2 program defaults issue
	if ( !header )
		return false;

	set<int>( header, "User Version", 0 );
	set<int>( header, "User Version 2", 0 );

	invalidateConditions( header, false );
	
	return loadItem( header, stream );
}

bool NifModel::saveItem( NifItem * parent, NifOStream & stream ) const
{
	if ( !parent )
		return false;

	for ( auto child : parent->children() ) {
		if ( child->isAbstract() ) {
			qDebug() << "Not saving abstract item " << child->name();
			continue;
		}

		if ( evalCondition( child ) ) {
			if ( isArray( child ) || !child->arr2().isEmpty() || child->childCount() > 0 ) {
				if ( isArray( child ) && child->childCount() != getArraySize( child ) ) {
					if ( child->isBinary() ) {
						// special byte
					} else {
						Message::append( tr( "Warnings were generated while reading the blocks." ),
							tr( "block %1 %2 array size mismatch" ).arg( getBlockNumber( parent ) ).arg( child->name() )
						);
					}
				}

				if ( !saveItem( child, stream ) )
					return false;
			} else {
				if ( !stream.write( child->value() ) )
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

	for ( auto child : parent->children() ) {
		if ( child == target )
			return true;

		if ( evalCondition( child ) ) {
			if ( isArray( child ) || !child->arr2().isEmpty() || child->childCount() > 0 ) {
				if ( fileOffset( child, target, stream, ofs ) )
					return true;
			} else {
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

bool NifModel::evalCondition( NifItem * item, bool chkParents ) const
{
	if ( !evalVersion( item, chkParents ) ) {
		// Version is global and cond is not so set false and abort
		item->setCondition( false );
		return false;
	}

	if ( item->isConditionValid() )
		return item->condition();

	// Early reject
	if ( item->isConditionless() ) {
		item->setCondition( true );
		return item->condition();
	}
	
	// Store row conditions for certain compound arrays
	NifItem * parent = item->parent();
	if ( isFixedCompound( parent->type() ) ) {
		NifItem * arrRoot = parent->parent();
		if ( isArray( arrRoot ) ) {
			// This is a compound in the compound array
			if ( parent->row() == 0 ) {
				// This is the first compound in the compound array
				if ( item->row() == 0 ) {
					// First row of first compound, initialize condition cache
					arrRoot->resetArrayConditions();
				}
				// Cache condition on array root
				arrRoot->updateArrayCondition( BaseModel::evalCondition( item ), item->row() );
			}
			auto arrCond = arrRoot->arrayConditions();
			auto itemRow = item->row();
			if ( arrCond.count() > itemRow )
				item->setCondition( arrCond.at( itemRow ) );
			return item->condition();
		}
	}

	item->setCondition( BaseModel::evalCondition( item, chkParents ) );

	return item->condition();
}

void NifModel::invalidateConditions( NifItem * item, bool refresh )
{
	for ( NifItem * c : item->children() ) {
		c->invalidateCondition();
		c->invalidateVersionCondition();
		if ( refresh )
			c->setCondition( BaseModel::evalCondition( c ) );

		if ( isArray( c ) || c->childCount() > 0 ) {
			invalidateConditions( c );
		}
	}

	// Reset conditions cached on array root for fixed condition compounds
	if ( isArray( item ) && isFixedCompound( item->type() ) ) {
		item->resetArrayConditions();
	}
}

void NifModel::invalidateConditions( const QModelIndex & index, bool refresh )
{
	auto item = static_cast<NifItem *>(index.internalPointer());
	if ( item )
		invalidateConditions( item, refresh );
}

void NifModel::invalidateDependentConditions( NifItem * item )
{
	if ( !item )
		return;

	NifItem * p = item->parent();
	if ( !p || p == root )
		return;

	QString name = item->name();
	for ( int i = item->row(); i < p->childCount(); i++ ) {
		auto c = p->children().at( i );
		// String check for Name in cond or arg
		//	Note: May cause some false positives but this is OK
		if ( c->cond().contains( name ) ) {
			c->invalidateCondition();
			c->setCondition( BaseModel::evalCondition( c ) );
		}

		if ( (c->cond().contains( name ) || c->arg().contains( name )) && c->childCount() > 0 )
			invalidateConditions( c, true );
	}
}

void NifModel::invalidateDependentConditions( const QModelIndex & index )
{
	auto item = static_cast<NifItem *>(index.internalPointer());
	if ( item )
		invalidateDependentConditions( item );
}


/*
 *  link functions
 */

void NifModel::updateLinks( int block )
{
	if ( lockUpdates ) {
		needUpdates = UpdateType( needUpdates | utLinks );
		return;
	}

	if ( block >= 0 ) {
		childLinks[ block ].clear();
		parentLinks[ block ].clear();
		updateLinks( block, getBlockItem( block ) );
	} else {
		rootLinks.clear();
		childLinks.clear();
		parentLinks.clear();

		// Run updateLinks() for each block
		for ( int c = 0; c < getBlockCount(); c++ )
			updateLinks( c );

		// Run checkLinks() for each block
		for ( int c = 0; c < getBlockCount(); c++ ) {
			QStack<int> stack;
			checkLinks( c, stack );
		}


		int n = getBlockCount();
		QByteArray hasrefs( n, 0 );

		for ( int c = 0; c < n; c++ ) {
			for ( const auto d : childLinks.value( c ) ) {
				if ( d >= 0 && d < n )
					hasrefs[d] = 1;
			}
		}

		for ( int c = 0; c < n; c++ ) {
			if ( !hasrefs[c] )
				rootLinks.append( c );
		}
	}
}

void NifModel::updateLinks( int block, NifItem * parent )
{
	if ( !parent )
		return;

	auto links = parent->getLinkRows();
	for ( int l : links ) {
		NifItem * c = parent->child( l );
		if ( !c )
			continue;
	
		if ( c->childCount() > 0 ) {
			updateLinks( block, c );
			continue;
		}
	
		int i = c->value().toLink();
		if ( i >= 0 ) {
			if ( c->value().type() == NifValue::tUpLink ) {
				if ( !parentLinks[block].contains( i ) )
					parentLinks[block].append( i );
			} else {
				if ( !childLinks[block].contains( i ) )
					childLinks[block].append( i );
			}
		}
	}
	
	auto linkparents = parent->getLinkAncestorRows();
	for ( int p : linkparents ) {
		NifItem * c = parent->child( p );
		if ( c && c->childCount() > 0 )
			updateLinks( block, c );
	}
}

void NifModel::checkLinks( int block, QStack<int> & parents )
{
	parents.push( block );
	foreach ( const auto child, childLinks.value( block ) ) {
		if ( parents.contains( child ) ) {
			auto m = tr( "infinite recursive link construct detected %1 -> %2" ).arg( block ).arg( child );
			if ( msgMode == UserMessage ) {
				Message::append( tr( "Warnings were generated while reading NIF file." ), m );
			} else {
				testMsg( m );
			}

			childLinks[block].removeAll( child );
		} else {
			checkLinks( child, parents );
		}
	}
	parents.pop();
}

void NifModel::adjustLinks( NifItem * parent, int block, int delta )
{
	if ( !parent )
		return;

	if ( parent->childCount() > 0 ) {
		for ( auto child : parent->children() )
			adjustLinks( child, block, delta );
	} else {
		int l = parent->value().toLink();

		if ( l >= 0 && ( ( delta != 0 && l >= block ) || l == block ) ) {
			if ( delta == 0 )
				parent->value().setLink( -1 );
			else
				parent->value().setLink( l + delta );
		}
	}
}

void NifModel::mapLinks( NifItem * parent, const QMap<qint32, qint32> & map )
{
	if ( !parent )
		return;

	if ( parent->childCount() > 0 ) {
		for ( auto child : parent->children() )
			mapLinks( child, map );
	} else {
		int l = parent->value().toLink();

		if ( l >= 0 ) {
			if ( map.contains( l ) )
				parent->value().setLink( map[ l ] );
		}
	}
}

qint32 NifModel::getLink( const QModelIndex & index ) const
{
	NifItem * item = static_cast<NifItem *>( index.internalPointer() );

	if ( !( index.isValid() && item && index.model() == this ) )
		return -1;

	return item->value().toLink();
}

qint32 NifModel::getLink( const QModelIndex & parent, const QString & name ) const
{
	NifItem * parentItem = static_cast<NifItem *>( parent.internalPointer() );

	if ( !( parent.isValid() && parentItem && parent.model() == this ) )
		return -1;

	NifItem * item = getItem( parentItem, name );

	if ( item )
		return item->value().toLink();

	return -1;
}

QVector<qint32> NifModel::getLinkArray( const QModelIndex & iArray ) const
{
	QVector<qint32> links;
	NifItem * item = static_cast<NifItem *>( iArray.internalPointer() );

	if ( isArray( iArray ) && item && iArray.model() == this ) {
		for ( auto child : item->children() ) {
			if ( itemIsLink( child ) ) {
				links.append( child->value().toLink() );
			} else {
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
	NifItem * parentItem = static_cast<NifItem *>( parent.internalPointer() );

	if ( !( parent.isValid() && parentItem && parent.model() == this ) )
		return false;

	NifItem * item = getItem( parentItem, name );

	if ( item && item->value().setLink( l ) ) {
		emit dataChanged( createIndex( item->row(), ValueCol, item ), createIndex( item->row(), ValueCol, item ) );
		NifItem * parent = item;

		while ( parent->parent() && parent->parent() != root )
			parent = parent->parent();

		if ( parent != getFooterItem() ) {
			updateLinks();
			updateFooter();
			emit linksChanged();
		}

		return true;
	}

	return false;
}

bool NifModel::setLink( const QModelIndex & index, qint32 l )
{
	NifItem * item = static_cast<NifItem *>( index.internalPointer() );

	if ( !( index.isValid() && item && index.model() == this ) )
		return false;

	if ( item && item->value().setLink( l ) ) {
		emit dataChanged( createIndex( item->row(), ValueCol, item ), createIndex( item->row(), ValueCol, item ) );
		NifItem * parent = item;

		while ( parent->parent() && parent->parent() != root )
			parent = parent->parent();

		if ( parent != getFooterItem() ) {
			updateLinks();
			updateFooter();
			emit linksChanged();
		}

		return true;
	}

	return false;
}

bool NifModel::setLinkArray( const QModelIndex & iArray, const QVector<qint32> & links )
{
	NifItem * item = static_cast<NifItem *>( iArray.internalPointer() );

	if ( isArray( iArray ) && item && iArray.model() == this ) {
		bool ret = true;

		for ( int c = 0; c < item->childCount() && c < links.count(); c++ ) {
			ret &= item->child( c )->value().setLink( links[c] );
		}

		ret &= item->childCount() == links.count();
		int x = item->childCount() - 1;

		if ( x >= 0 )
			emit dataChanged( createIndex( 0, ValueCol, item->child( 0 ) ), createIndex( x, ValueCol, item->child( x ) ) );

		NifItem * parent = item;

		while ( parent->parent() && parent->parent() != root )
			parent = parent->parent();

		if ( parent != getFooterItem() ) {
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
	NifItem * item = static_cast<NifItem *>( index.internalPointer() );

	if ( !( index.isValid() && item && index.model() == this ) )
		return false;

	return itemIsLink( item, isChildLink );
}

int NifModel::getParent( int block ) const
{
	int parent = -1;

	for ( int b = 0; b < getBlockCount(); b++ ) {
		if ( childLinks.value( b ).contains( block ) ) {
			parent = b;
			break;
		}
	}

	return parent;
}

int NifModel::getParent( const QModelIndex & index ) const
{
	return getParent( getBlockNumber( index ) );
}

QString NifModel::string( const QModelIndex & index, bool extraInfo ) const
{
	NifValue v = getValue( index );

	if ( v.type() == NifValue::tSizedString )
		return BaseModel::get<QString>( index );

	if ( getVersionNumber() >= 0x14010003 ) {
		QModelIndex iIndex;
		int idx = -1;

		if ( v.type() == NifValue::tStringIndex )
			idx = get<int>( index );
		else if ( !v.isValid() )
			idx = get<int>( getIndex( index, "Index" ) );

		if ( idx < 0 )
			return QString();

		NifItem * header = this->getHeaderItem();
		QModelIndex stringIndex = createIndex( header->row(), 0, header );

		if ( !stringIndex.isValid() )
			return tr( "%1 - <index invalid>" ).arg( idx );

		QString string = BaseModel::get<QString>( this->index( idx, 0, getIndex( stringIndex, "Strings" ) ) );

		if ( extraInfo )
			string = QString( "%2 [%1]" ).arg( idx ).arg( string );

		return string;
	}

	if ( v.type() != NifValue::tNone )
		return BaseModel::get<QString>( index );

	QModelIndex iIndex = getIndex( index, "String" );
	if ( iIndex.isValid() )
		return BaseModel::get<QString>( iIndex );

	return QString();
}

QString NifModel::string( const QModelIndex & index, const QString & name, bool extraInfo ) const
{
	return string( getIndex( index, name ), extraInfo );
}

bool NifModel::assignString( const QModelIndex & index, const QString & string, bool replace )
{
	NifItem * item = static_cast<NifItem *>( index.internalPointer() );

	if ( !( index.isValid() && item && index.model() == this ) )
		return false;

	return assignString( item, string, replace );
}

bool NifModel::assignString( NifItem * item, const QString & string, bool replace )
{
	NifValue & v = item->value();

	if ( getVersionNumber() >= 0x14010003 ) {
		NifItem * pItem;
		int idx = -1;

		switch ( v.type() ) {
		case NifValue::tNone:
			pItem = getItem( item, "Index" );
			if ( !pItem )
				return false;

			idx = get<int>( pItem );
			break;
		case NifValue::tStringIndex:
			idx = get<int>( pItem = item );
			break;
		case NifValue::tSizedString:
			if ( item->type() == "string" ) {
				pItem = item;
				idx = -1;
				break;
			} // fall through
		default:
			return BaseModel::set<QString>( item, string );
		}

		QModelIndex header = getHeader();
		int nstrings = get<int>( header, "Num Strings" );

		if ( string.isEmpty() ) {
			if ( replace && idx >= 0 && idx < nstrings ) {
				// TODO: Can we remove the string safely here?
			}

			v.changeType( NifValue::tStringIndex );
			return set<int>( item, 0xffffffff );
		}

		QModelIndex iArray = getIndex( header, "Strings" );

		// Simply replace the string
		if ( replace && idx >= 0 && idx < nstrings ) {
			return BaseModel::set<QString>( iArray.child( idx, 0 ), string );
		}

		QVector<QString> stringVector = getArray<QString>( iArray );
		idx = stringVector.indexOf( string );

		// Already exists.  Just update the Index
		if ( idx >= 0 && idx < stringVector.size() ) {
			v.changeType( NifValue::tStringIndex );
			return set<int>( pItem, idx );
		}

		// Append string to end of list
		set<uint>( header, "Num Strings", nstrings + 1 );
		updateArray( header, "Strings" );
		BaseModel::set<QString>( iArray.child( nstrings, 0 ), string );

		v.changeType( NifValue::tStringIndex );
		return set<int>( pItem, nstrings );
	} // endif getVersionNumber() >= 0x14010003

	// Handle the older simpler strings
	if ( v.type() == NifValue::tNone ) {
		return BaseModel::set<QString>( getItem( item, "String" ), string );
	} else if ( v.type() == NifValue::tStringIndex ) {
		NifValue v( NifValue::tString );
		v.set( string );
		return setItemValue( item, v );
		//return BaseModel::set<QString>( index, string );
	}

	return BaseModel::set<QString>( item, string );
}

bool NifModel::assignString( const QModelIndex & index, const QString & name, const QString & string, bool replace )
{
	return assignString( getIndex( index, name ), string, replace );
}


// convert a block from one type to another
void NifModel::convertNiBlock( const QString & identifier, const QModelIndex & index )
{
	QString btype = getBlockName( index );

	if ( btype == identifier )
		return;

	if ( !inherits( btype, identifier ) && !inherits( identifier, btype ) ) {
		Message::critical( nullptr, tr( "Cannot convert NiBlock." ), tr( "blocktype %1 and %2 are not related" ).arg( btype, identifier ) );
		return;
	}

	NifItem * branch = static_cast<NifItem *>( index.internalPointer() );
	NifBlockPtr srcBlock = blocks.value( btype );
	NifBlockPtr dstBlock = blocks.value( identifier );

	if ( srcBlock && dstBlock && branch ) {
		branch->setName( identifier );

		if ( inherits( btype, identifier ) ) {
			// Remove any level between the two types
			for ( QString ancestor = btype; !ancestor.isNull() && ancestor != identifier; ) {
				NifBlockPtr block = blocks.value( ancestor );

				if ( !block )
					break;

				int n = block->types.count();

				if ( n > 0 )
					removeRows( branch->childCount() - n,  n, index );

				ancestor = block->ancestor;
			}
		} else if ( inherits( identifier, btype ) ) {
			// Add any level between the two types
			QStringList types;

			for ( QString ancestor = identifier; !ancestor.isNull() && ancestor != btype; ) {
				NifBlockPtr block = blocks.value( ancestor );

				if ( !block )
					break;

				types.insert( 0, ancestor );
				ancestor = block->ancestor;
			}

			for ( const QString& ancestor : types ) {
				NifBlockPtr block = blocks.value( ancestor );

				if ( !block )
					break;

				int cn = branch->childCount();
				int n  = block->types.count();

				if ( n > 0 ) {
					beginInsertRows( index, cn, cn + n - 1 );
					branch->prepareInsert( n );
					const auto & types = block->types;
					for ( const NifData& data : types ) {
						insertType( branch, data );
					}
					endInsertRows();
				}
			}
		}

		if ( state != Loading ) {
			updateHeader();
			updateLinks();
			updateFooter();
			emit linksChanged();
		}
	}
}

bool NifModel::holdUpdates( bool value )
{
	bool retval = lockUpdates;

	if ( lockUpdates == value )
		return retval;

	lockUpdates = value;

	if ( !lockUpdates ) {
		updateModel( needUpdates );
		needUpdates = utNone;
	}

	return retval;
}

void NifModel::updateModel( UpdateType value )
{
	if ( value & utHeader )
		updateHeader();

	if ( value & utLinks )
		updateLinks();

	if ( value & utFooter )
		updateFooter();

	if ( value & utLinks )
		emit linksChanged();
}


/*
 *  NifModelEval
 */

NifModelEval::NifModelEval( const NifModel * model, const NifItem * item )
{
	this->model = model;
	this->item = item;
}

QVariant NifModelEval::operator()( const QVariant & v ) const
{
	if ( v.type() == QVariant::String ) {
		QString left = v.toString();
		NifItem * i = const_cast<NifItem *>(item);
		i = model->getItem( i, left );

		if ( i ) {
			if ( i->value().isCount() )
				return QVariant( i->value().toCount() );
			else if ( i->value().isFileVersion() )
				return QVariant( i->value().toFileVersion() );
		}

		return QVariant( 0 );
	}

	return v;
}
