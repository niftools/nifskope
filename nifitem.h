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

#ifndef NIFITEM_H
#define NIFITEM_H

#include "nifvalue.h"
#include "nifexpr.h"

#include <QSharedData>
#include <QVector>

//! \file nifitem.h NifItem, NifBlock, NifData, NifSharedData

//! Shared data for NifData.
/**
 * See QSharedDataPointer for details on data sharing in Qt;
 * <a href="http://doc.trolltech.org/latest/shared.html">shared classes</a>
 * give pointer efficiency to classes.
 */
class NifSharedData : public QSharedData
{
	friend class NifData;
	
	//! Constructor.
	NifSharedData( const QString & n, const QString & t, const QString & tt, const QString & a, const QString & a1, const QString & a2, const QString & c, quint32 v1, quint32 v2, bool abs )
		: QSharedData(), name( n ), type( t ), temp( tt ), arg( a ), arr1( a1 ), arr2( a2 ), cond( c ), ver1( v1 ), ver2( v2 ), condexpr(c), isAbstract( abs ) {}
	
	//! Constructor.
	NifSharedData( const QString & n, const QString & t )
		: QSharedData(), name( n ), type( t ), ver1( 0 ), ver2( 0 ), isAbstract( false ) {}
	
	//! Constructor.
	NifSharedData( const QString & n, const QString & t, const QString & txt )
		: QSharedData(), name( n ), type( t ), ver1( 0 ), ver2( 0 ), text( txt ), isAbstract( false ) {}
	
	//! Constructor.
	NifSharedData()
		: QSharedData(), ver1( 0 ), ver2( 0 ), isAbstract( false ) {}
	
	//! Name.
	QString  name;
	//! Type.
	QString  type;
	//! Template type.
	QString  temp;
	//! Argument.
	QString  arg;
	//! First array length.
	QString  arr1;
	//! Second array length.
	QString  arr2;
	//! Condition.
	QString  cond;
	//! Earliest version.
	quint32  ver1;
	//! Latest version.
	quint32  ver2;
	//! Description text.
	QString  text;
	//! Condition as an expression.
	Expression condexpr;
	//! Version condition.
	QString  vercond;
	//! Version condition as an expression.
	Expression verexpr;
	//! Abstract flag.
	bool isAbstract;
};

//! The data and NifValue stored by a NifItem
class NifData
{
public:
	//! Constructor.
	NifData( const QString & name, const QString & type, const QString & temp, const NifValue & val, const QString & arg, const QString & arr1, const QString & arr2, const QString & cond, quint32 ver1, quint32 ver2, bool isAbstract = false )
		: d( new NifSharedData( name, type, temp, arg, arr1, arr2, cond, ver1, ver2, isAbstract ) ), value( val ) {}
	
	//! Constructor.
	NifData( const QString & name, const QString & type = QString(), const QString & text = QString() )
		: d( new NifSharedData( name, type, text ) ) {}
	
	//! Constructor.
	NifData()
		: d( new NifSharedData() ) {}
	
	//! Get the name of the data.
	inline const QString & name() const	{ return d->name; }
	//! Get the type of the data.
	inline const QString & type() const	{ return d->type; }
	//! Get the template type of the data.
	inline const QString & temp() const	{ return d->temp; }
	//! Get the argument of the data.
	inline const QString & arg() const	{ return d->arg; }
	//! Get the first array length of the data.
	inline const QString & arr1() const	{ return d->arr1; }
	//! Get the second array length of the data.
	inline const QString & arr2() const	{ return d->arr2; }
	//! Get the condition attribute of the data.
	inline const QString & cond() const	{ return d->cond; }
	//! Get the earliest version of the data.
	inline quint32 ver1() const			{ return d->ver1; }
	//! Get the latest version of the data.
	inline quint32 ver2() const			{ return d->ver2; }
	//! Get the text description of the data.
	inline const QString & text() const	{ return d->text; }
	//! Get the condition attribute of the data, as an expression.
	inline const Expression & condexpr() const	{ return d->condexpr; }
	//! Get the version condition attribute of the data.
	inline const QString & vercond() const	{ return d->vercond; }
	//! Get the version condition attribute of the data, as an expression.
	inline const Expression & verexpr() const	{ return d->verexpr; }
	//! Get the abstract attribute of the data.
	inline const bool & isAbstract() const	{ return d->isAbstract; }
	
	//! Sets the name of the data.
	void setName( const QString & name )	{ d->name = name; }
	//! Sets the type of the data.
	void setType( const QString & type )	{ d->type = type; }
	//! Sets the template type of the data.
	void setTemp( const QString & temp )	{ d->temp = temp; }
	//! Sets the argument of the data.
	void setArg( const QString & arg )		{ d->arg = arg; }
	//! Sets the first array length of the data.
	void setArr1( const QString & arr1 )	{ d->arr1 = arr1; }
	//! Sets the second array length of the data.
	void setArr2( const QString & arr2 )	{ d->arr2 = arr2; }
	//! Sets the condition attribute of the data.
	void setCond( const QString & cond )	{ 
		d->cond = cond; 
		d->condexpr = Expression(cond); 
	}
	//! Sets the earliest version of the data.
	void setVer1( quint32 ver1 )			{ d->ver1 = ver1; }
	//! Sets the latest version of the data.
	void setVer2( quint32 ver2 )			{ d->ver2 = ver2; }
	//! Sets the text description of the data.
	void setText( const QString & text )	{ d->text = text; }
	//! Sets the version condition attribute of the data.
	void setVerCond( const QString & cond )	{ 
		d->vercond = cond; 
		d->verexpr = Expression(cond); 
	}
	//! Sets the abstract attribute of the data.
	void setAbstract( bool & isAbstract)	{ d->isAbstract = isAbstract; }

protected:
	//! The internal shared data.
	QSharedDataPointer<NifSharedData> d;

public:
	//! The value stored with the data.
	NifValue value;
};

//! A block representing a niobject in XML.
struct NifBlock
{
	//! Identifier.
	QString id;
	//! Ancestor.
	QString ancestor;
	//! Description text.
	QString text;
	//! Abstract flag.
	bool abstract;
	//! Data present.
	QList<NifData> types;
};

//! An item which contains NifData
class NifItem
{
public:
	//! Constructor.
	NifItem( NifItem * parent )
		: parentItem( parent ) {}
	
	//! Constructor.
	NifItem( const NifData & data, NifItem * parent )
		: itemData( data ), parentItem( parent ) {}
	
	//! Destructor.
	~NifItem()
	{
		qDeleteAll( childItems );
	}
	
	//! Return the parent item.
	NifItem * parent() const
	{
		return parentItem;
	}
	
	//! Return the row that this item is at.
	int row() const
	{
		if ( parentItem )
			return parentItem->childItems.indexOf( const_cast<NifItem*>(this) );
		return 0;
	}
	
	//! Allocate memory to insert child items
	/*!
	 * \param e The number of items to be inserted
	 */
	void prepareInsert( int e )
	{
		childItems.reserve( childItems.count() + e );
	}
	
	//! Insert child data item
	/*!
	 * \param data The data to insert
	 * \param at The position to insert at; append if not specified
	 * \return An item containing the inserted data
	 */
	NifItem * insertChild( const NifData & data, int at = -1 )
	{
		NifItem * item = new NifItem( data, this );
		if ( at < 0 || at > childItems.count() )
			childItems.append( item );
		else
			childItems.insert( at, item );
		return item;
	}
	
	//! Insert child item
	/*!
	 * \param child The data to insert
	 * \param at The position to insert at; append if not specified
	 * \return The row the child was inserted at
	 */
	int insertChild( NifItem * child, int at = -1 )
	{
		child->parentItem = this;
		if ( at < 0 || at > childItems.count() )
			childItems.append( child );
		else
			childItems.insert( at, child );
		return child->row();
	}
	
	//! Take child item at row
	/*!
	 * \param row The row to take the item from
	 * \return The child item that was removed
	 */
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
	
	//! Remove child item at row
	/*!
	 * \param row The row to remove the item from
	 */
	void removeChild( int row )
	{
		NifItem * item = child( row );
		if ( item )
		{
			childItems.remove( row );
			delete item;
		}
	}
	
	//! Remove several child items
	/*!
	 * \param row The row to start from
	 * \param count The number of rows to delete
	 */
	void removeChildren( int row, int count )
	{
		for ( int c = row; c < row + count; c++ )
		{
			NifItem * item = childItems.value( c );
			if ( item ) delete item;
		}
		childItems.remove( row, count );
	}
	
	//! Return the child item at the specified row
	NifItem * child( int row )
	{
		return childItems.value( row );
	}
	
	//! Return the child item with the specified name
	NifItem * child( const QString & name )
	{
		foreach ( NifItem * child, childItems )
			if ( child->name() == name )
				return child;
		return 0;
	}
	
	//! Return a count of the number of child items
	int childCount()
	{
		return childItems.count();
	}
	
	//! Remove all child items
	void killChildren()
	{
		qDeleteAll( childItems );
		childItems.clear();
	}
	
	//! Return the value of the item data (const version)
	inline const NifValue & value() const	{ return itemData.value; }
	//! Return the value of the item data
	inline NifValue & value()	{ return itemData.value; }
	
	//! Return the name of the data
	inline QString  name() const	{	return itemData.name();	}
	//! Return the type of the data
	inline QString  type() const	{	return itemData.type();	}
	//! Return the template type of the data
	inline QString  temp() const	{	return itemData.temp();	}
	//! Return the argument attribute of the data
	inline QString  arg() const	{	return itemData.arg();		}
	//! Return the first array length of the data
	inline QString  arr1() const	{	return itemData.arr1();	}
	//! Return the second array length of the data
	inline QString  arr2() const	{	return itemData.arr2();	}
	//! Return the condition attribute of the data
	inline QString  cond() const	{	return itemData.cond();	}
	//! Return the earliest version attribute of the data
	inline quint32  ver1() const	{	return itemData.ver1();	}
	//! Return the latest version attribute of the data
	inline quint32  ver2() const	{	return itemData.ver2();	}
	//! Return the description text of the data
	inline QString  text() const	{	return itemData.text();	}
	//! Return the condition attribute of the data, as an expression
	inline const Expression& condexpr() const	{	return itemData.condexpr();	}
	//! Return the version condition attribute of the data
	inline QString  vercond() const	{	return itemData.vercond();	}
	//! Return the version condition attribute of the data, as an expression
	inline const Expression& verexpr() const	{	return itemData.verexpr();	}
	//! Return the abstract attribute of the data
	inline const bool & isAbstract() const	{ return itemData.isAbstract(); }
	
	//! Set the name
	inline void setName( const QString & name )	{	itemData.setName( name );	}
	//! Set the type
	inline void setType( const QString & type )	{	itemData.setType( type );	}
	//! Set the template type
	inline void setTemp( const QString & temp )	{	itemData.setTemp( temp );	}
	//! Set the argument attribute
	inline void setArg( const QString & arg )		{	itemData.setArg( arg );		}
	//! Set the first array length
	inline void setArr1( const QString & arr1 )	{	itemData.setArr1( arr1 );	}
	//! Set the second array length
	inline void setArr2( const QString & arr2 )	{	itemData.setArr2( arr2 );	}
	//! Set the condition attribute
	inline void setCond( const QString & cond )	{	itemData.setCond( cond );	}
	//! Set the earliest version attribute
	inline void setVer1( int v1 )					{	itemData.setVer1( v1 );		}
	//! Set the latest version attribute
	inline void setVer2( int v2 )					{	itemData.setVer2( v2 );		}
	//! Set the description text
	inline void setText( const QString & text )	{	itemData.setText( text );	}
	//! Set the version condition attribute
	inline void setVerCond( const QString & cond )	{	itemData.setVerCond( cond );	}
	
	//! Determine if this item is present in the specified version
	inline bool evalVersion( quint32 v )
	{
		return ( ( ver1() == 0 || ver1() <= v ) && ( ver2() == 0 || v <= ver2() ) );
	}
	
	//! Get the child items as an array
	template <typename T> QVector< T > getArray() const
	{
		QVector<T> array;
		foreach ( NifItem * child, childItems )
			array.append( child->itemData.value.get< T >() );
		return array;
	}
	
	//! Set the child items from an array
	template <typename T> void setArray( const QVector< T > & array )
	{
		int x = 0;
		foreach ( NifItem * child, childItems )
			child->itemData.value.set< T >( array.value( x++ ) );
	}

private:
	//! The data held by the item
	NifData itemData;
	//! The parent of this item
	NifItem * parentItem;
	//! The child items
	QVector<NifItem*> childItems;
};

#endif
