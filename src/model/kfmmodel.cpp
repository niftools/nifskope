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

#include "kfmmodel.h"

#include "xml/xmlconfig.h"
#include "message.h"
#include "io/nifstream.h"

#include <QStringBuilder>

//! @file kfmmodel.cpp KfmModel

const QString DOT_QSTRING(".");

KfmModel::KfmModel( QObject * parent ) : BaseModel( parent )
{
	clear();
}

QModelIndex KfmModel::getKFMroot() const
{
	if ( kfmroot )
		return createIndex( 0, 0, kfmroot );

	return QModelIndex();
}

QString KfmModel::version2string( quint32 v )
{
	if ( v == 0 )
		return QString();

	QString s = QString::number( ( v >> 24 ) & 0xff, 16 ) % DOT_QSTRING
	            % QString::number( ( v >> 16 ) & 0xff, 16 ) % DOT_QSTRING
	            % QString::number( ( v >> 8 ) & 0xff, 16 ) % DOT_QSTRING
	            % QString::number( v & 0xff, 16 );
	return s;
}

quint32 KfmModel::version2number( const QString & s )
{
	if ( s.isEmpty() )
		return 0;

	QStringList l = s.split( DOT_QSTRING );

	if ( l.count() <= 1 ) {
		bool ok;
		quint32 i = s.toUInt( &ok );
		return ( i == 0xffffffff ? 0 : i );
	}

	quint32 v = 0;

	for ( int i = 0; i < l.count(); i++ )
		v += l[i].toInt( 0, 16 ) << ( (3 - i) * 8 );

	return v;
}

bool KfmModel::evalVersionImpl( const NifItem * item ) const
{
	return item->evalVersion(version);
}

void KfmModel::clear()
{
	beginResetModel();
	fileinfo = QFileInfo();
	filename = QString();
	folder = QString();
	root->killChildren();
	version = 0x0200000b;
	auto rootData = NifData( "Kfm", "Kfm" );
	rootData.setIsCompound( true );
	rootData.setIsConditionless( true );
	insertType( root, rootData );
	kfmroot = root->child( 0 );
	endResetModel();

	if ( kfmroot )
		set<QString>( kfmroot, "Header String", ";Gamebryo KFM File Version 2.0.0.0b" );
}

/*
 *  array functions
 */

bool KfmModel::updateArraySizeImpl( NifItem * array )
{
	if ( !array->isArray() ) {
		if ( array )
			reportError( array, __func__, "The input item is not an array." );
		return false;
	}

	// Get new array size
	int nNewSize = evalArraySize( array );

	if ( nNewSize > 1024 * 1024 * 8 ) {
		reportError( array, __func__, tr( "Array size %1 is much too large." ).arg( nNewSize ) );
		return false;
	} else if ( nNewSize < 0 ) {
		reportError( array, __func__, tr( "Array size %1 is invalid." ).arg( nNewSize ) );
		return false;
	}

	int nOldSize = array->childCount();

	if ( nNewSize > nOldSize ) { // Add missing items
		NifData data( array->name(),
					  array->strType(),
					  array->templ(),
					  NifValue( NifValue::type( array->strType() ) ),
					  addConditionParentPrefix( array->arg() ),
			          addConditionParentPrefix( array->arr2() ) // arr1 in children is parent arr2
		);

		// Fill data flags
		data.setIsConditionless( true );
		data.setIsCompound( array->isCompound() );
		data.setIsArray( array->isMultiArray() );

		beginInsertRows( itemToIndex(array), nOldSize, nNewSize - 1 );
		array->prepareInsert( nNewSize - nOldSize );
		for ( int c = nOldSize; c < nNewSize; c++ )
			insertType( array, data );
		endInsertRows();
	}

	if ( nNewSize < nOldSize ) { // Remove excess items
		beginRemoveRows( itemToIndex(array), nNewSize, nOldSize - 1 );
		array->removeChildren( nNewSize, nOldSize - nNewSize );
		endRemoveRows();
	}

	return true;
}

/*
 *  basic and compound type functions
 */

void KfmModel::insertType( NifItem * parent, const NifData & data, int at )
{
	if ( data.isArray() ) {
		NifItem * array = insertBranch( parent, data, at );

		if ( evalCondition( array ) )
			updateArraySize( array );

	} else if ( data.isCompound() ) {
		NifBlockPtr compound = compounds.value( data.type() );
		if ( !compound )
			return;
		NifItem * branch = insertBranch( parent, data, at );
		branch->prepareInsert( compound->types.count() );

		if ( !data.arg().isEmpty() || !data.templ().isEmpty() ) {
			QString arg = addConditionParentPrefix( data.arg() );
			QString tmp = data.templ();

			if ( tmp == XMLTMPL ) {
				NifItem * tItem = branch;

				while ( tmp == XMLTMPL && tItem->parent() ) {
					tItem = tItem->parent();
					tmp = tItem->templ();
				}
			}

			for ( NifData & d : compound->types ) {
				if ( d.type() == XMLTMPL ) {
					d.setType( tmp );
					d.value.changeType( NifValue::type( tmp ) );
				}

				if ( d.arg() == XMLARG )  d.setArg( data.arg() );
				if ( d.arr1() == XMLARG ) d.setArr1( arg );
				if ( d.arr2() == XMLARG ) d.setArr2( arg );

				if ( d.cond().contains( XMLARG ) ) {
					QString x = d.cond(); x.replace( x.indexOf( XMLARG ), 5, arg ); d.setCond( x );
				}

				insertType( branch, d );
			}
		} else {
			for ( const NifData& d : compound->types ) {
				insertType( branch, d );
			}
		}
	} else {
		parent->insertChild( data, at );
	}
}


/*
 *  load and save
 */

bool KfmModel::setHeaderString( const QString & s, uint ver )
{
	if ( s.startsWith( ";Gamebryo KFM File Version " ) ) {
		version = version2number( s.right( s.length() - 27 ) );
		root->invalidateVersionCondition();
		root->invalidateCondition();

		if ( isVersionSupported( version ) ) {
			return true;
		} else {
			Message::critical( nullptr, tr( "Version %1 is not supported." ).arg( version2string( version ) ) );
			return false;
		}
	}
	Message::critical( nullptr, tr( "Could not open %1 because it is not a supported type." ).arg( fileinfo.fileName() ) );
	return false;
}

bool KfmModel::load( QIODevice & device )
{
	clear();

	NifIStream stream( this, &device );

	if ( !kfmroot || !load( kfmroot, stream ) ) {
		Message::critical( nullptr, tr( "The file could not be read. See Details for more information." ),
			tr( "failed to load kfm file (%1)" ).arg( version2string( version ) )
		);
		return false;
	}

	// Qt5 port:
	// begin/endResetModel() is already called above in clear()
	// May not be necessary again, but doing so to mimic Qt4 codebase.
	beginResetModel();
	endResetModel();
	return true;
}

bool KfmModel::save( QIODevice & device ) const
{
	NifOStream stream( this, &device );

	if ( !kfmroot || !save( kfmroot, stream ) ) {
		Message::critical( nullptr, tr( "Failed to write KFM file." ) );
		return false;
	}

	return true;
}

bool KfmModel::load( NifItem * parent, NifIStream & stream )
{
	if ( !parent )
		return false;

	for ( NifItem * child: parent->childIter() ) {
		child->invalidateCondition();

		if ( evalCondition( child ) ) {
			if ( child->isArray() ) {
				if ( !updateArraySize( child ) )
					return false;
				if ( !load( child, stream ) )
					return false;
			} else if ( child->childCount() > 0 ) {
				if ( !load( child, stream ) )
					return false;
			} else {
				if ( !stream.read( child->value() ) )
					return false;
			}
		}
	}

	return true;
}

bool KfmModel::save( NifItem * parent, NifOStream & stream ) const
{
	if ( !parent )
		return false;

	for ( int row = 0; row < parent->childCount(); row++ ) {
		NifItem * child = parent->child( row );

		if ( evalCondition( child ) ) {
			if ( !child->arr1().isEmpty() || !child->arr2().isEmpty() || child->childCount() > 0 ) {
				if ( !child->arr1().isEmpty() && child->childCount() != evalArraySize( child ) ) {
					Message::append( tr( "Warnings were generated while reading the blocks." ),
						tr( "%1 array size mismatch" ).arg( child->name() )
					);
				}

				if ( !save( child, stream ) )
					return false;
			} else {
				if ( !stream.write( child->value() ) )
					return false;
			}
		}
	}

	return true;
}

NifItem * KfmModel::insertBranch( NifItem * parentItem, const NifData & data, int at )
{
	return parentItem->insertChild( data, NifValue::tNone, at );
}
