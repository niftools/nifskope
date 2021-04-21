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

#ifndef NIFITEM_H
#define NIFITEM_H

#include "data/nifvalue.h"
#include "xml/nifexpr.h"

#include <QSharedData> // Inherited
#include <QPointer>
#include <QString>
#include <QVector>


//! @file nifitem.h NifItem, NifBlock, NifData, NifSharedData

/*! Shared data for NifData.
 *
 * @see QSharedDataPointer
 */
class NifSharedData final : public QSharedData
{
	friend class NifData;

public:
	enum DataFlag
	{
		None = 0x0,
		Abstract = 0x1,
		Binary = 0x2,
		Templated = 0x4,
		Compound = 0x8,
		Array = 0x10,
		MultiArray = 0x20,
		Conditionless = 0x40,
		Mixin = 0x80
	};

	typedef QFlags<DataFlag> DataFlags;

private:

	NifSharedData( const QString & n, const QString & t, const QString & tt, const QString & a, const QString & a1,
				   const QString & a2, const QString & c, quint32 v1, quint32 v2, NifSharedData::DataFlags f )
		: QSharedData(), name( n ), type( t ), temp( tt ), arg( a ), arr1( a1 ), arr2( a2 ),
		cond( c ), ver1( v1 ), ver2( v2 ), condexpr( c ), arr1expr( a1 ), flags( f )
	{
	}

	NifSharedData( const QString & n, const QString & t )
		: QSharedData(), name( n ), type( t ) {}

	NifSharedData( const QString & n, const QString & t, const QString & txt )
		: QSharedData(), name( n ), type( t ), text( txt ) {}

	NifSharedData()
		: QSharedData() {}

	//! Name.
	QString name;
	//! Type.
	QString type;
	//! Template type.
	QString temp;
	//! Argument.
	QString arg;
	//! First array length.
	QString arr1;
	//! Second array length.
	QString arr2;
	//! Condition.
	QString cond;
	//! Earliest version.
	quint32 ver1 = 0;
	//! Latest version.
	quint32 ver2 = 0;
	//! Description text.
	QString text;
	//! Condition as an expression.
	NifExpr condexpr;
	//! First array length as an expression.
	NifExpr arr1expr;
	//! Version condition.
	QString vercond;
	//! Version condition as an expression.
	NifExpr verexpr;

	DataFlags flags = None;
};

Q_DECLARE_OPERATORS_FOR_FLAGS( NifSharedData::DataFlags );

//! The data and NifValue stored by a NifItem
class NifData
{
public:
	NifData( const QString & name, const QString & type, const QString & temp, const NifValue & val, const QString & arg,
			 const QString & arr1 = QString(), const QString & arr2 = QString(), const QString & cond = QString(),
			 quint32 ver1 = 0, quint32 ver2 = 0, NifSharedData::DataFlags flag = NifSharedData::None )
		: d( new NifSharedData( name, type, temp, arg, arr1, arr2, cond, ver1, ver2, flag ) ), value( val )
	{}

	NifData( const QString & name, const QString & type = QString(), const QString & text = QString() )
		: d( new NifSharedData( name, type, text ) ) {}

	NifData()
		: d( new NifSharedData() ) {}

	//! Get the name of the data.
	inline const QString & name() const { return d->name; }
	//! Get the type of the data.
	inline const QString & type() const { return d->type; }
	//! Get the template type of the data.
	inline const QString & temp() const { return d->temp; }
	//! Get the argument of the data.
	inline const QString & arg() const { return d->arg; }
	//! Get the first array length of the data.
	inline const QString & arr1() const { return d->arr1; }
	//! Get the second array length of the data.
	inline const QString & arr2() const { return d->arr2; }
	//! Get the condition attribute of the data.
	inline const QString & cond() const { return d->cond; }
	//! Get the earliest version of the data.
	inline quint32 ver1() const { return d->ver1; }
	//! Get the latest version of the data.
	inline quint32 ver2() const { return d->ver2; }
	//! Get the text description of the data.
	inline const QString & text() const { return d->text; }
	//! Get the condition attribute of the data, as an expression.
	inline const NifExpr & condexpr() const { return d->condexpr; }
	//! Get the first array length of the data, as an expression.
	inline const NifExpr & arr1expr() const { return d->arr1expr; }
	//! Get the version condition attribute of the data.
	inline const QString & vercond() const { return d->vercond; }
	//! Get the version condition attribute of the data, as an expression.
	inline const NifExpr & verexpr() const { return d->verexpr; }
	//! Get the abstract attribute of the data.
	inline bool isAbstract() const { return d->flags & NifSharedData::Abstract; }
	//! Is the data binary. Binary means the data is being treated as one blob.
	inline bool isBinary() const { return d->flags & NifSharedData::Binary; }
	//! Is the data templated. Templated means the type is dynamic.
	inline bool isTemplated() const { return d->flags & NifSharedData::Templated; }
	//! Is the data a compound. Compound means the data type is a compound block.
	inline bool isCompound() const { return d->flags & NifSharedData::Compound; }
	//! Is the data an array. Array means the data on this row repeats.
	inline bool isArray() const { return d->flags & NifSharedData::Array; }
	//! Is the data a multi-array. Multi-array means the item's children are also arrays.
	inline bool isMultiArray() const { return d->flags & NifSharedData::MultiArray; }
	//! Is the data conditionless. Conditionless means no expression evaluation is necessary.
	inline bool isConditionless() const { return d->flags & NifSharedData::Conditionless; }
	//! Is the data a mixin. Mixin is a specialized compound which creates no nesting.
	inline bool isMixin() const { return d->flags & NifSharedData::Mixin; }

	//! Sets the name of the data.
	void setName( const QString & name ) { d->name = name; }
	//! Sets the type of the data.
	void setType( const QString & type ) { d->type = type; }
	//! Sets the template type of the data.
	void setTemp( const QString & temp ) { d->temp = temp; }
	//! Sets the argument of the data.
	void setArg( const QString & arg ) { d->arg = arg; }
	//! Sets the first array length of the data.
	void setArr1( const QString & arr1 )
	{
		d->arr1 = arr1;
		d->arr1expr = NifExpr( arr1 );
	}
	//! Sets the second array length of the data.
	void setArr2( const QString & arr2 ) { d->arr2 = arr2; }
	//! Sets the condition attribute of the data.
	void setCond( const QString & cond )
	{
		d->cond = cond;
		d->condexpr = NifExpr( cond );
	}
	//! Sets the earliest version of the data.
	void setVer1( quint32 ver1 ) { d->ver1 = ver1; }
	//! Sets the latest version of the data.
	void setVer2( quint32 ver2 ) { d->ver2 = ver2; }
	//! Sets the text description of the data.
	void setText( const QString & text ) { d->text = text; }
	//! Sets the version condition attribute of the data.
	void setVerCond( const QString & cond )
	{
		d->vercond = cond;
		d->verexpr = NifExpr( cond );
	}

	inline void setFlag( NifSharedData::DataFlags flag, bool val )
	{
		(val) ? d->flags |= flag : d->flags &= ~flag;
	}
	//! Sets the abstract attribute of the data.
	inline void setAbstract( bool flag ) { setFlag( NifSharedData::Abstract, flag ); }
	//! Sets the binary data flag. Binary means the data is being treated as one blob.
	inline void setBinary( bool flag ) { setFlag( NifSharedData::Binary, flag ); }
	//! Sets the templated data flag. Templated means the type is dynamic.
	inline void setTemplated( bool flag ) { setFlag( NifSharedData::Templated, flag ); }
	//! Sets the compound data flag. Compound means the data type is a compound block.
	inline void setIsCompound( bool flag ) { setFlag( NifSharedData::Compound, flag ); }
	//! Sets the array data flag. Array means the data on this row repeats.
	inline void setIsArray( bool flag ) { setFlag( NifSharedData::Array, flag ); }
	//! Sets the multi-array data flag. Multi-array means the item's children are also arrays.
	inline void setIsMultiArray( bool flag ) { setFlag( NifSharedData::MultiArray, flag ); }
	//! Sets the conditionless data flag. Conditionless means no expression evaluation is necessary.
	inline void setIsConditionless( bool flag ) { setFlag( NifSharedData::Conditionless, flag ); }
	//! Sets the mixin data flag. Mixin is a specialized compound which creates no nesting.
	inline void setIsMixin( bool flag ) { setFlag( NifSharedData::Mixin, flag ); }

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
	bool abstract = false;
	//! Data present.
	QList<NifData> types;
};

//! An item which contains NifData
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

	//! Return the parent item.
	NifItem * parent() const
	{
		return parentItem;
	}

	//! Return the row that this item is at.
	int row() const
	{
		if ( !parentItem )
			return 0;

		if ( rowIdx < 0 )
			rowIdx = parentItem->childItems.indexOf( const_cast<NifItem *>(this) );

		return rowIdx;
	}

	/*! Allocate memory to insert child items
	 *
	 * @param e The number of items to be inserted
	 */
	void prepareInsert( int e )
	{
		childItems.reserve( childItems.count() + e );
	}

	//! Get child items
	const QVector<NifItem *> & children()
	{
		return childItems;
	}

	/*! Insert child data item
	 *
	 * @param data	The data to insert
	 * @param at	The position to insert at; append if not specified
	 * @return		An item containing the inserted data
	 */
	NifItem * insertChild( const NifData & data, int at = -1 )
	{
		NifItem * item = new NifItem( data, this );

		if ( data.isConditionless() )
			item->setCondition( true );

		if ( at < 0 || at > childItems.count() ) {
			childItems.append( item );
		} else {
			if ( at < childItems.size() - 1 )
				invalidateRowCounts();
			childItems.insert( at, item );
		}

		populateLinksUp( item );

		return item;
	}

	/*! Insert child item
	 *
	 * @param child The data to insert
	 * @param at	The position to insert at; append if not specified
	 * @return		The row the child was inserted at
	 */
	int insertChild( NifItem * child, int at = -1 )
	{
		child->parentItem = this;

		if ( at < 0 || at > childItems.count() ) {
			childItems.append( child );
		} else {
			invalidateRowCounts();
			childItems.insert( at, child );
		}

		populateLinksUp( child );
		
		return child->row();
	}

	//! Inform the parent and its ancestors of any links
	void populateLinksUp( NifItem * item )
	{
		if ( item->value().type() == NifValue::tLink || item->value().type() == NifValue::tUpLink ) {
			// Add this child's row to the item's link vector
			linkRows << item->row();
	
			// Inform the parent that this item's rows have links
			auto p = parentItem;
			auto c = this;
			while ( p ) {
				// Add this item's row to the parent item
				if ( !p->linkAncestorRows.contains( c->row() ) )
					p->linkAncestorRows << c->row();
	
				// Recurse up
				c = p;
				p = p->parentItem;
			}
		}
	}

	/*! Take child item at row
	 *
	 * @param row	The row to take the item from
	 * @return		The child item that was removed
	 */
	NifItem * takeChild( int row )
	{
		NifItem * item = child( row );
		invalidateRowCounts();
		if ( item ) {
			childItems.remove( row );
			item->parentItem = 0;
		}

		return item;
	}

	/*! Remove child item at row
	 *
	 * @param row The row to remove the item from
	 */
	void removeChild( int row )
	{
		NifItem * item = child( row );
		invalidateRowCounts();
		if ( item ) {
			childItems.remove( row );
			delete item;
		}
	}

	/*! Remove several child items
	 *
	 * @param row	The row to start from
	 * @param count The number of rows to delete
	 */
	void removeChildren( int row, int count )
	{
		invalidateRowCounts();
		for ( int c = row; c < row + count; c++ ) {
			NifItem * item = childItems.value( c );
			if ( item )
				delete item;
		}

		childItems.remove( row, count );
	}

	//! Return the child item at the specified row
	NifItem * child( int row )
	{
		return childItems.value( row );
	}

	//! Return the child item at the specified row
	const NifItem * child( int row ) const
	{
		return childItems.value( row );
	}

	//! Return the child item with the specified name
	NifItem * child( const QString & name )
	{
		for ( NifItem * child : childItems ) {
			if ( child->name() == name )
				return child;
		}
		return nullptr;
	}

	//! Return the child item with the specified name
	const NifItem * child( const QString & name ) const
	{
		for ( const NifItem * child : childItems ) {
			if ( child->name() == name )
				return child;
		}
		return nullptr;
	}

	//! Return a count of the number of child items
	int childCount() const
	{
		return childItems.count();
	}

	//! Remove all child items
	void killChildren()
	{
		qDeleteAll( childItems );
		childItems.clear();
	}

	const QVector<ushort> & getLinkAncestorRows() const
	{
		return linkAncestorRows;
	}
	
	const QVector<ushort> & getLinkRows() const
	{
		return linkRows;
	}

	//! Conditions for each child in the array (if fixed)
	const QVector<bool> & arrayConditions()
	{
		return arrConds;
	}

	//! Reset array conditions based on size of children
	void resetArrayConditions()
	{
		if ( childItems.isEmpty() )
			return;

		arrConds.clear();
		arrConds.resize( childItems.at( 0 )->childCount() );
		arrConds.fill( false );
	}

	//! Reset array conditions based on provided size
	void resetArrayConditions( int size )
	{
		arrConds.clear();
		arrConds.resize( size );
		arrConds.fill( false );
	}

	//! Update array condition at specified index
	void updateArrayCondition( bool cond, int at )
	{
		if ( arrConds.count() > at )
			arrConds[at] = cond;
	}

	//! Cached result of cond expression
	bool condition()
	{
		return conditionStatus == 1;
	}

	//! Cached result of vercond expression
	bool versionCondition()
	{
		return vercondStatus == 1;
	}

	//! Has the condition been cached
	bool isConditionValid()
	{
		return conditionStatus != -1;
	}

	//! Has the version condition been cached
	bool isVercondValid()
	{
		return vercondStatus != -1;
	}

	//! Cache the cond expression
	void setCondition( bool status )
	{
		conditionStatus = status;
	}

	//! Cache the vercond expression
	void setVersionCondition( bool status )
	{
		vercondStatus = status;
	}

	//! Invalidate the cached cond expression
	void invalidateCondition()
	{
		conditionStatus = -1;
	}

	//! Invalidate the cached vercond expression
	void invalidateVersionCondition()
	{
		vercondStatus = -1;
	}

	//! Invalidate the cached row index
	void invalidateRow()
	{
		rowIdx = -1;
	}

	//! Invalidate the cached row index for this item and its children
	void invalidateRowCounts()
	{
		invalidateRow();
		for ( NifItem * c : childItems ) {
			c->invalidateRow();
		}
	}

	//! Invalidate the cached row index for this item and its children starting at the given index
	void invalidateRowCounts( int at )
	{
		if ( at < childCount() ) {
			invalidateRow();
			for ( int i = at; i < childCount(); i++ ) {
				childItems.value( i )->invalidateRow();
			}
		} else {
			invalidateRowCounts();
		}

	}

	//! Return the value of the item data (const version)
	inline const NifValue & value() const { return itemData.value; }
	//! Return the value of the item data
	inline NifValue & value() { return itemData.value; }

	//! Return the name of the data
	inline QString name() const {   return itemData.name(); }
	//! Return the type of the data
	inline QString type() const {   return itemData.type(); }
	//! Return the template type of the data
	inline QString temp() const {   return itemData.temp(); }
	//! Return the argument attribute of the data
	inline QString arg()  const {   return itemData.arg();  }
	//! Return the first array length of the data
	inline QString arr1() const {   return itemData.arr1(); }
	//! Return the second array length of the data
	inline QString arr2() const {   return itemData.arr2(); }
	//! Return the condition attribute of the data
	inline QString cond() const {   return itemData.cond(); }
	//! Return the earliest version attribute of the data
	inline quint32 ver1() const {   return itemData.ver1(); }
	//! Return the latest version attribute of the data
	inline quint32 ver2() const {   return itemData.ver2(); }
	//! Return the description text of the data
	inline QString text() const {   return itemData.text(); }

	//! Return the condition attribute of the data, as an expression
	inline const NifExpr & condexpr() const {   return itemData.condexpr(); }
	//! Return the arr1 attribute of the data, as an expression
	inline const NifExpr & arr1expr() const {   return itemData.arr1expr(); }
	//! Return the version condition attribute of the data
	inline QString vercond() const {   return itemData.vercond();  }
	//! Return the version condition attribute of the data, as an expression
	inline const NifExpr & verexpr() const {   return itemData.verexpr();  }
	//! Return the abstract attribute of the data
	inline bool isAbstract() const { return itemData.isAbstract(); }
	//! Is the item data binary. Binary means the data is being treated as one blob.
	inline bool isBinary() const { return itemData.isBinary(); }
	//! Is the item data templated. Templated means the type is dynamic.
	inline bool isTemplated() const { return itemData.isTemplated(); }
	//! Is the item data a compound. Compound means the data type is a compound block.
	inline bool isCompound() const { return itemData.isCompound(); }
	//! Is the item data an array. Array means the data on this row repeats.
	inline bool isArray() const { return itemData.isArray(); }
	//! Is the item data a multi-array. Multi-array means the item's children are also arrays.
	inline bool isMultiArray() const { return itemData.isMultiArray(); }
	//! Is the item data conditionless. Conditionless means no expression evaluation is necessary.
	inline bool isConditionless() const { return itemData.isConditionless(); }

	//! Set the name
	inline void setName( const QString & name ) {   itemData.setName( name );   }
	//! Set the type
	inline void setType( const QString & type ) {   itemData.setType( type );   }
	//! Set the template type
	inline void setTemp( const QString & temp ) {   itemData.setTemp( temp );   }
	//! Set the argument attribute
	inline void setArg( const QString & arg )   {   itemData.setArg( arg );     }
	//! Set the first array length
	inline void setArr1( const QString & arr1 ) {   itemData.setArr1( arr1 );   }
	//! Set the second array length
	inline void setArr2( const QString & arr2 ) {   itemData.setArr2( arr2 );   }
	//! Set the condition attribute
	inline void setCond( const QString & cond ) {   itemData.setCond( cond );   }

	//! Set the earliest version attribute
	inline void setVer1( int v1 ) {   itemData.setVer1( v1 );     }
	//! Set the latest version attribute
	inline void setVer2( int v2 ) {   itemData.setVer2( v2 );     }

	//! Set the description text
	inline void setText( const QString & text )    {   itemData.setText( text );    }
	//! Set the version condition attribute
	inline void setVerCond( const QString & cond ) {   itemData.setVerCond( cond ); }

	//! Determine if this item is present in the specified version
	inline bool evalVersion( quint32 v )
	{
		return ( ( ver1() == 0 || ver1() <= v ) && ( ver2() == 0 || v <= ver2() ) );
	}

	//! Get the child items as an array
	template <typename T> QVector<T> getArray() const
	{
		QVector<T> array;
		for ( NifItem * child : childItems ) {
			array.append( child->itemData.value.get<T>() );
		}
		return array;
	}

	//! Set the child items from an array
	template <typename T> void setArray( const QVector<T> & array )
	{
		int x = 0;
		for ( NifItem * child : childItems ) {
			child->itemData.value.set<T>( array.value( x++ ) );
		}
	}

	//! Set the child items from a single value
	template <typename T> void setArray( const T & val )
	{
		for ( NifItem * child : childItems ) {
			child->itemData.value.set<T>( val );
		}
	}

private:
	//! The data held by the item
	NifData itemData;
	//! The parent of this item
	NifItem * parentItem = nullptr;
	//! The child items
	QVector<NifItem *> childItems;

	//! Rows which have links under them at any level
	QVector<ushort> linkAncestorRows;
	//! Rows which are links
	QVector<ushort> linkRows;
	//! If item is array with fixed compounds, the conditions are stored here for reuse
	QVector<bool> arrConds;

	//! Item's row index, -1 is invalid, otherwise 0+
	mutable int rowIdx = -1;
	//! Item's condition status, -1 is invalid, otherwise 0/1
	char conditionStatus = -1;
	//! Item's vercond status, -1 is invalid, otherwise 0/1
	char vercondStatus = -1;
};

#endif
