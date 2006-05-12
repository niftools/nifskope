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

#ifndef NIFITEM_H
#define NIFITEM_H

#include <QSharedData>
#include <QVector>

class NifSharedData : public QSharedData
{
	friend class NifData;
	
	NifSharedData( const QString & n, const QString & t, const QString & tt, const QString & a, const QString & a1, const QString & a2, const QString & c, quint32 v1, quint32 v2 )
		: QSharedData(), name( n ), type( t ), temp( tt ), arg( a ), arr1( a1 ), arr2( a2 ), cond( c ), ver1( v1 ), ver2( v2 ) {}

	NifSharedData( const QString & n, const QString & t )
		: QSharedData(), name( n ), type( t ), ver1( 0 ), ver2( 0 ) {}
	
	NifSharedData( const QString & n, const QString & t, const QString & txt )
		: QSharedData(), name( n ), type( t ), ver1( 0 ), ver2( 0 ), text( txt ) {}
	
	NifSharedData()
		: QSharedData(), ver1( 0 ), ver2( 0 ) {}
	
	QString  name;
	QString  type;
	QString  temp;
	QString  arg;
	QString  arr1;
	QString  arr2;
	QString  cond;
	quint32  ver1;
	quint32  ver2;
	QString  text;
};

class NifData
{
public:
	NifData( const QString & name, const QString & type, const QString & temp, const NifValue & val, const QString & arg, const QString & arr1, const QString & arr2, const QString & cond, quint32 ver1, quint32 ver2 )
		: d( new NifSharedData( name, type, temp, arg, arr1, arr2, cond, ver1, ver2 ) ), value( val ) {}
	
	NifData( const QString & name, const QString & type = QString(), const QString & text = QString() )
		: d( new NifSharedData( name, type, text ) ) {}
	
	NifData()
		: d( new NifSharedData() ) {}
	
	inline const QString & name() const	{ return d->name; }
	inline const QString & type() const	{ return d->type; }
	inline const QString & temp() const	{ return d->temp; }
	inline const QString & arg() const	{ return d->arg; }
	inline const QString & arr1() const	{ return d->arr1; }
	inline const QString & arr2() const	{ return d->arr2; }
	inline const QString & cond() const	{ return d->cond; }
	inline quint32 ver1() const			{ return d->ver1; }
	inline quint32 ver2() const			{ return d->ver2; }
	inline const QString & text() const	{ return d->text; }
	
	void setName( const QString & name )	{ d->name = name; }
	void setType( const QString & type )	{ d->type = type; }
	void setTemp( const QString & temp )	{ d->temp = temp; }
	void setArg( const QString & arg )		{ d->arg = arg; }
	void setArr1( const QString & arr1 )	{ d->arr1 = arr1; }
	void setArr2( const QString & arr2 )	{ d->arr2 = arr2; }
	void setCond( const QString & cond )	{ d->cond = cond; }
	void setVer1( quint32 ver1 )			{ d->ver1 = ver1; }
	void setVer2( quint32 ver2 )			{ d->ver2 = ver2; }
	void setText( const QString & text )	{ d->text = text; }

protected:
	QSharedDataPointer<NifSharedData> d;

public:
	NifValue value;
};


struct NifBlock
{
	QString				id;
	QString				text;
	QList<QString>		ancestors;
	QList<NifData>		types;
};


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
	
	int insertChild( NifItem * child, int at = -1 )
	{
		child->parentItem = this;
		if ( at < 0 || at > childItems.count() )
			childItems.append( child );
		else
			childItems.insert( at, child );
		return child->row();
	}
	
	NifItem * takeChild( int row )
	{
		NifItem * item = child( row );
		if ( item )
		{
			childItems.remove( row );
			item->parentItem = 0;
		}
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

	inline const NifValue & value() const	{ return itemData.value; }
	inline NifValue & value()	{ return itemData.value; }

	inline QString  name() const	{	return itemData.name();	}
	inline QString  type() const	{	return itemData.type();	}
	inline QString  temp() const	{	return itemData.temp();	}
	inline QString  arg() const	{	return itemData.arg();		}
	inline QString  arr1() const	{	return itemData.arr1();	}
	inline QString  arr2() const	{	return itemData.arr2();	}
	inline QString  cond() const	{	return itemData.cond();	}
	inline quint32  ver1() const	{	return itemData.ver1();	}
	inline quint32  ver2() const	{	return itemData.ver2();	}
	inline QString  text() const	{	return itemData.text();	}
	
	inline void setName( const QString & name )	{	itemData.setName( name );	}
	inline void setType( const QString & type )	{	itemData.setType( type );	}
	inline void setTemp( const QString & temp )	{	itemData.setTemp( temp );	}
	inline void setArg( const QString & arg )		{	itemData.setArg( arg );		}
	inline void setArr1( const QString & arr1 )	{	itemData.setArr1( arr1 );	}
	inline void setArr2( const QString & arr2 )	{	itemData.setArr2( arr2 );	}
	inline void setCond( const QString & cond )	{	itemData.setCond( cond );	}
	inline void setVer1( int v1 )					{	itemData.setVer1( v1 );		}
	inline void setVer2( int v2 )					{	itemData.setVer2( v2 );		}
	inline void setText( const QString & text )	{	itemData.setText( text );	}
	
	inline bool evalVersion( quint32 v )
	{
		return ( ( ver1() == 0 || ver1() <= v ) && ( ver2() == 0 || v <= ver2() ) );
	}
	
	template <typename T> QVector< T > getArray() const
	{
		QVector<T> array;
		foreach ( NifItem * child, childItems )
			array.append( child->itemData.value.get< T >() );
		return array;
	}
	
	template <typename T> void setArray( const QVector< T > & array )
	{
		int x = 0;
		foreach ( NifItem * child, childItems )
			child->itemData.value.set< T >( array.value( x++ ) );
	}

private:
	NifData itemData;
	NifItem * parentItem;
	QVector<NifItem*> childItems;
};

#endif
