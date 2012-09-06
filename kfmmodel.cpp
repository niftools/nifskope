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

#include "kfmmodel.h"

KfmModel::KfmModel( QObject * parent ) : BaseModel( parent )
{
	clear();
}

QModelIndex KfmModel::getKFMroot() const
{
	if ( kfmroot )
		return createIndex( 0, 0, kfmroot );
	else
		return QModelIndex();
}

QString KfmModel::version2string( quint32 v )
{
	if ( v == 0 )	return QString();
	QString s = QString::number( ( v >> 24 ) & 0xff, 16 ) + "."
		+ QString::number( ( v >> 16 ) & 0xff, 16 ) + "."
		+ QString::number( ( v >> 8 ) & 0xff, 16 ) + "."
		+ QString::number( v & 0xff, 16 );
	return s;
}	

quint32 KfmModel::version2number( const QString & s )
{
	if ( s.isEmpty() )	return 0;
	QStringList l = s.split( "." );
	if ( l.count() <= 1 )
	{
		bool ok;
		quint32 i = s.toUInt( &ok );
		return ( i == 0xffffffff ? 0 : i );
	}
	quint32 v = 0;
	for ( int i = 0; i < l.count(); i++ )
		v += l[i].toInt( 0, 16 ) << ( (3-i) * 8 );
	return v;
}

bool KfmModel::evalVersion( NifItem * item, bool chkParents ) const
{
	if ( item == root )
		return true;
	
	if ( chkParents && item->parent() )
		if ( ! evalVersion( item->parent(), true ) )
			return false;
	
	return item->evalVersion( version );
}

void KfmModel::clear()
{
	folder = QString();
	root->killChildren();
	insertType( root, NifData( "Kfm", "Kfm" ) );
	kfmroot = root->child( 0 );
	version = 0x0200000b;
	reset();
	if ( kfmroot )
		set<QString>( kfmroot, "Header String", ";Gamebryo KFM File Version 2.0.0.0b" );
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

bool KfmModel::updateArrayItem( NifItem * array, bool fast )
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
	return true;
}

/*
 *  basic and compound type functions
 */

void KfmModel::insertType( NifItem * parent, const NifData & data, int at )
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
		if ( ! data.arg().isEmpty() || ! data.temp().isEmpty() )
		{
			QString arg = parentPrefix( data.arg() );
			QString tmp = data.temp();
			if ( tmp == "TEMPLATE" )
			{
				NifItem * tItem = branch;
				while ( tmp == "TEMPLATE" && tItem->parent() )
				{
					tItem = tItem->parent();
					tmp = tItem->temp();
				}
			}
			foreach ( NifData d, compound->types )
			{
				if ( d.type() == "TEMPLATE" )
				{
					d.setType( tmp );
					d.value.changeType( NifValue::type( tmp ) );
				}
				if ( d.arg() == "ARG" )	d.setArg( data.arg() );
				if ( d.arr1() == "ARG" ) d.setArr1( arg );
				if ( d.arr2() == "ARG" ) d.setArr2( arg );
				if ( d.cond().contains( "ARG" ) ) { QString x = d.cond(); x.replace( x.indexOf( "ARG" ), 5, arg ); d.setCond( x ); }
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

bool KfmModel::setItemValue( NifItem * item, const NifValue & val )
{
	item->value() = val;
	emit dataChanged( createIndex( item->row(), ValueCol, item ), createIndex( item->row(), ValueCol, item ) );
	return true;
}

/*
 *  load and save
 */

bool KfmModel::setHeaderString( const QString & s )
{
	//msg( DbgMsg() << s << s.right( s.length() - 27 ) );
	if ( s.startsWith( ";Gamebryo KFM File Version " ) )
	{
		version = version2number( s.right( s.length() - 27 ) );
		if ( isVersionSupported( version ) )
		{
			return true;
		}
		else
		{
			msg( Message() << tr("version") << version2string( version ) << tr("not supported yet") );
			return false;
		}
	}
	else
	{
		msg( Message() << tr("this is not a KFM") );
		return false;
	}
}

bool KfmModel::load( QIODevice & device )
{
	clear();
	
	NifIStream stream( this, &device );

	if ( !kfmroot || !load( kfmroot, stream, true ) )
	{
		msg( Message() << tr("failed to load kfm file (%1)").arg(version));
		return false;
	}
	
	reset();
	return true;
}

bool KfmModel::save( QIODevice & device ) const
{
	NifOStream stream( this, &device );

	if ( ! kfmroot || save( kfmroot, stream ) )
	{
		msg( Message() << tr("failed to write kfm file") );
		return false;
	}
	return true;
}

bool KfmModel::load( NifItem * parent, NifIStream & stream, bool fast )
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

bool KfmModel::save( NifItem * parent, NifOStream & stream ) const
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
					msg( Message() << child->name() << tr("array size mismatch") );
				
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

NifItem * KfmModel::insertBranch( NifItem * parentItem, const NifData & data, int at )
{
	NifItem * item = parentItem->insertChild( data, at );
	item->value().changeType( NifValue::tNone );
	return item;
}
