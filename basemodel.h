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

#ifndef BaseModel_H
#define BaseModel_H

#include <QAbstractItemModel>

#include <QIODevice>

class QAbstractItemDelegate;

#include "niftypes.h"
#include "nifitem.h"

#include "message.h"

//! \file basemodel.h BaseModel

//! Base class for nif and kfm models, which store files in memory.
/*!
 * This class serves as an abstract base class for NifModel and KfmModel
 * classes.
 */
class BaseModel : public QAbstractItemModel
{
Q_OBJECT
public:
	//! Constructor
	BaseModel( QObject * parent = 0 );
	//! Destructor
	~BaseModel();
	
	//! Clear model data.
	virtual void clear() = 0;
	
	//! Load from file.
	bool loadFromFile( const QString & filename );
	//! Save to file.
	bool saveToFile( const QString & filename ) const;
	
	//! Generic load from QIODevice.
	virtual bool load( QIODevice & device ) = 0;
	//! Generic save to QIODevice.
	virtual bool save( QIODevice & device ) const = 0;
	
	//! If the model was loaded from a file then getFolder returns the folder.
	/*!
	 * This function is used to resolve external resources.
	 * \return The folder of the last file that was loaded with loadFromFile.
	 */
	QString getFolder() const { return folder; }
	
	//! Return true if the index pointed to is an array.
	/*!
	 * \param array The index to check.
	 * \return true if the index is an array.
	 */
	bool isArray( const QModelIndex & iArray ) const;
	//! Update the size of an array (append or remove items).
	/*!
	 * \param array The index of the array whose size to update.
	 * \return true if the update succeeded, false otherwise.
	 */
	bool updateArray( const QModelIndex & iArray );
	//! Update the size of an array by name.
	bool updateArray( const QModelIndex & parent, const QString & name );
	//! Get an model index array as a QVector.
	/*!
	 * \param array The index of the array to get.
	 * \return The array as QVector.
	 */
	template <typename T> QVector<T> getArray( const QModelIndex & iArray ) const;
	//! Get an model index array as a QVector by name.
	template <typename T> QVector<T> getArray( const QModelIndex & iArray, const QString & name ) const;
	//! Write a QVector to a model index array.
	template <typename T> void setArray( const QModelIndex & iArray, const QVector<T> & array );
	//! Write a QVector to a model index array by name.
	template <typename T> void setArray( const QModelIndex & iArray, const QString & name, const QVector<T> & array );
	
	//! Get an item.
	template <typename T> T get( const QModelIndex & index ) const;
	//! Get an item by name.
	template <typename T> T get( const QModelIndex & parent, const QString & name ) const;
	//! Set an item.
	template <typename T> bool set( const QModelIndex & index, const T & d );
	//! Set an item by name.
	template <typename T> bool set( const QModelIndex & parent, const QString & name, const T & v );
	
	//! Get an item as a NifValue.
	NifValue getValue( const QModelIndex & index ) const;
	/* Not implemented? */
	// Get an item as a NifValue by name.
	//NifValue getValue( const QModelIndex & parent, const QString & name ) const;
	
	//! Set an item from a NifValue.
	bool setValue( const QModelIndex & index, const NifValue & v );
	//! Set an item from a NifValue by name.
	bool setValue( const QModelIndex & parent, const QString & name, const NifValue & v );
	
	// get item attributes
	//! Get the item name.
	QString  itemName( const QModelIndex & index ) const;
	//! Get the item type string.
	QString  itemType( const QModelIndex & index ) const;
	//! Get the item argument string.
	QString  itemArg( const QModelIndex & index ) const;
	//! Get the item arr1 string.
	QString  itemArr1( const QModelIndex & index ) const;
	//! Get the item arr2 string.
	QString  itemArr2( const QModelIndex & index ) const;
	//! Get the item condition string.
	QString  itemCond( const QModelIndex & index ) const;
	//! Get the item first version.
	quint32  itemVer1( const QModelIndex & index ) const;
	//! Get the item last version.
	quint32  itemVer2( const QModelIndex & index ) const;
	//! Get the item documentation.
	QString  itemText( const QModelIndex & index ) const;
	//! Get the item template string.
	QString  itemTmplt( const QModelIndex & index ) const;
	
	//! Find a branch by name.
	QModelIndex getIndex( const QModelIndex & parent, const QString & name ) const;
	
	//! Evaluate condition and version.
	bool evalCondition( const QModelIndex & idx, bool chkParents = false ) const;
	//! Evaluate version.
	bool evalVersion( const QModelIndex & idx, bool chkParents = false ) const;
	
	//! Get version as a string
	virtual QString getVersion() const = 0;
	//! Get version as a number
	virtual quint32 getVersionNumber() const = 0;
	
	//! Column names
	enum {
		NameCol  = 0,
		TypeCol  = 1,
		ValueCol = 2,
		ArgCol   = 3,
		Arr1Col  = 4,
		Arr2Col  = 5,
		CondCol  = 6,
		Ver1Col  = 7,
		Ver2Col  = 8,
		VerCondCol  = 9,
		NumColumns = 10,
	};
	
	// QAbstractModel interface
	//! Creates a model index for the given row and column
	/*!
	 * \see QAbstractItemModel::createIndex()
	 */
	QModelIndex index( int row, int column, const QModelIndex & parent = QModelIndex() ) const;
	//! Finds the parent of the specified index
	QModelIndex parent( const QModelIndex & index ) const;
	
	//! Finds the number of rows
	int rowCount( const QModelIndex & parent = QModelIndex() ) const;
	//! Finds the number of columns
	int columnCount( const QModelIndex & parent = QModelIndex() ) const { Q_UNUSED(parent); return NumColumns; }
	
	//! Finds the data associated with an index
	/*!
	 * \param index The index to find data for
	 * \param role The Qt::ItemDataRole to get data for
	 */
	QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const;
	//! Sets data associated with an index
	/*!
	 * \param index The index to set data for
	 * \param value The data to set
	 * \param role The Qt::ItemDataRole to use
	 */
	bool setData( const QModelIndex & index, const QVariant & value, int role = Qt::EditRole );
	
	//! Get the header data for a section
	QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
	
	//! Finds the flags for an index
	Qt::ItemFlags flags( const QModelIndex & index ) const;
	
	//! Message mode
	enum MsgMode
	{
		EmitMessages, CollectMessages
	};
	
	//! Set the Message mode
	void setMessageMode( MsgMode m ) { msgMode = m; }
	//! Get Messages collected
	QList<Message> getMessages() const { QList<Message> lst = messages; messages.clear(); return lst; }
	
signals:
	//! Messaging signal
	void sigMessage( const Message & msg ) const;
	//! Progress signal
	void sigProgress( int c, int m ) const;
	
protected:
	//! Update an array item
	virtual bool		updateArrayItem( NifItem * array, bool fast ) = 0;
	//! Get the size of an array
	int			getArraySize( NifItem * array ) const;
	//! Evaluate a string for an array
	int			evaluateString( NifItem * array, const QString & text ) const;
	
	//! Get an item
	virtual NifItem *	getItem( NifItem * parent, const QString & name ) const;
	//! Get an item by name
	NifItem *	getItemX( NifItem * item, const QString & name ) const; // find upwards
	
	//! Get an item by name
	template <typename T> T get( NifItem * parent, const QString & name ) const;
	//! Get an item
	template <typename T> T get( NifItem * item ) const;
	
	//! Set an item by name
	template <typename T> bool set( NifItem * parent, const QString & name, const T & d );
	//! Set an item
	template <typename T> bool set( NifItem * item, const T & d );
	//! Set an item value
	virtual bool setItemValue( NifItem * item, const NifValue & v ) = 0;
	//! Set an item value by name
	bool setItemValue( NifItem * parent, const QString & name, const NifValue & v );
	
	//! Evaluate version
	virtual bool		evalVersion( NifItem * item, bool chkParents = false ) const = 0;
	//! Evaluate conditions
	bool		evalCondition( NifItem * item, bool chkParents = false ) const;
	//! Evaluate conditions
	bool		evalConditionHelper( NifItem * item, const QString & cond ) const;
	
	//! Convert a version number to a string
	virtual QString ver2str( quint32 ) const = 0;
	//! Convert a version string to a number
	virtual quint32 str2ver( QString ) const = 0;
	
	//! Set the header string
	virtual bool setHeaderString( const QString & ) = 0;
	
	//! The root item
	NifItem *	root;
	
	//! The filepath of the model
	QString folder;
	
	//! The messaging mode
	MsgMode msgMode;
	//! A list of messages
	mutable QList<Message> messages;
	
	//! Handle a message
	void msg( const Message & m ) const;
	
	friend class NifIStream;
	friend class NifOStream;
	friend class BaseModelEval;
}; // class BaseModel


template <typename T> inline T BaseModel::get( NifItem * parent, const QString & name ) const
{
	NifItem * item = getItem( parent, name );
	if ( item )
		return item->value().get<T>();
	else
		return T();
}

template <typename T> inline T BaseModel::get( const QModelIndex & parent, const QString & name ) const
{
	NifItem * parentItem = static_cast<NifItem*>( parent.internalPointer() );
	if ( ! ( parent.isValid() && parentItem && parent.model() == this ) )
		return T();
	
	NifItem * item = getItem( parentItem, name );
	if ( item )
		return item->value().get<T>();
	else
		return T();
}

template <typename T> inline bool BaseModel::set( NifItem * parent, const QString & name, const T & d )
{
	NifItem * item = getItem( parent, name );
	if ( item )
		return set( item, d );
	else
		return false;
}

template <typename T> inline bool BaseModel::set( const QModelIndex & parent, const QString & name, const T & d )
{
	NifItem * parentItem = static_cast<NifItem*>( parent.internalPointer() );
	if ( ! ( parent.isValid() && parentItem && parent.model() == this ) )
		return false;
	
	NifItem * item = getItem( parentItem, name );
	if ( item )
		return set( item, d );
	else
		return false;
}

template <typename T> inline T BaseModel::get( NifItem * item ) const
{
	return item->value().get<T>();
}

template <typename T> inline T BaseModel::get( const QModelIndex & index ) const
{
	NifItem * item = static_cast<NifItem*>( index.internalPointer() );
	if ( ! ( index.isValid() && item && index.model() == this ) )
		return T();
	
	return item->value().get<T>();
}

template <typename T> inline bool BaseModel::set( NifItem * item, const T & d )
{
	if ( item->value().set( d ) )
	{
		emit dataChanged( createIndex( item->row(), ValueCol, item ), createIndex( item->row(), ValueCol, item ) );
		return true;
	}
	else
		return false;
}

template <typename T> inline bool BaseModel::set( const QModelIndex & index, const T & d )
{
	NifItem * item = static_cast<NifItem*>( index.internalPointer() );
	if ( ! ( index.isValid() && item && index.model() == this ) )	return false;
	return set( item, d );
}

template <typename T> inline QVector<T> BaseModel::getArray( const QModelIndex & iArray ) const
{
	NifItem * item = static_cast<NifItem*>( iArray.internalPointer() );
	if ( isArray( iArray ) && item && iArray.model() == this )
		return item->getArray<T>();
	return QVector<T>();
}

template <typename T> inline QVector<T> BaseModel::getArray( const QModelIndex & iParent, const QString & name ) const
{
	return getArray<T>( getIndex( iParent, name ) );
}

template <typename T> inline void BaseModel::setArray( const QModelIndex & iArray, const QVector<T> & array )
{
	NifItem * item = static_cast<NifItem*>( iArray.internalPointer() );
	if ( isArray( iArray ) && item && iArray.model() == this )
	{
		item->setArray<T>( array );
		int x = item->childCount() - 1;
		if ( x >= 0 )
			emit dataChanged( createIndex( 0, ValueCol, item->child( 0 ) ), createIndex( x, ValueCol, item->child( x ) ) );
	}
}

template <typename T> inline void BaseModel::setArray( const QModelIndex & iParent, const QString & name, const QVector<T> & array )
{
	setArray<T>( getIndex( iParent, name ), array );
}

#endif
