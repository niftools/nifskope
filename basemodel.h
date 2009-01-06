/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005-2009, NIF File Format Library and Tools
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

#ifndef BaseModel_H
#define BaseModel_H

#include <QAbstractItemModel>

#include <QIODevice>

class QAbstractItemDelegate;

#include "niftypes.h"
#include "nifitem.h"

#include "message.h"

//! Base class for nif and kfm models, which store files in memory.
/*!
 * This class serves as an abstract base class for NifModel and KfmModel
 * classes.
 */
class BaseModel : public QAbstractItemModel
{
Q_OBJECT
public:
	BaseModel( QObject * parent = 0 );
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
	bool updateArray( const QModelIndex & parent, const QString & name );
	//! Get an model index array as a QVector.
	/*!
	 * \param array The index of the array to get.
	 * \return The array as QVector.
	 */
	template <typename T> QVector<T> getArray( const QModelIndex & iArray ) const;
	template <typename T> QVector<T> getArray( const QModelIndex & iArray, const QString & name ) const;
	//! Write a QVector to a model index array.
	template <typename T> void setArray( const QModelIndex & iArray, const QVector<T> & array );
	template <typename T> void setArray( const QModelIndex & iArray, const QString & name, const QVector<T> & array );
	
	//! Get an item.
	template <typename T> T get( const QModelIndex & index ) const;
	template <typename T> T get( const QModelIndex & parent, const QString & name ) const;
	//! Set an item.
	template <typename T> bool set( const QModelIndex & index, const T & d );	
	template <typename T> bool set( const QModelIndex & parent, const QString & name, const T & v );

	//! Get an item as a NifValue.
	NifValue getValue( const QModelIndex & index ) const;
	NifValue getValue( const QModelIndex & parent, const QString & name ) const;
	
	//! Set an item from a NifValue.
	bool setValue( const QModelIndex & index, const NifValue & v );
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

	// find a branch by name
	QModelIndex getIndex( const QModelIndex & parent, const QString & name ) const;
	
	// evaluate condition and version
	bool evalCondition( const QModelIndex & idx, bool chkParents = false ) const;
	// evaluate version
	bool evalVersion( const QModelIndex & idx, bool chkParents = false ) const;
	
	virtual QString getVersion() const = 0;
	virtual quint32 getVersionNumber() const = 0;

	

   // column numbers
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
	
	QModelIndex index( int row, int column, const QModelIndex & parent = QModelIndex() ) const;
	QModelIndex parent( const QModelIndex & index ) const;

	int rowCount( const QModelIndex & parent = QModelIndex() ) const;
	int columnCount( const QModelIndex & parent = QModelIndex() ) const { Q_UNUSED(parent); return NumColumns; }
	
	QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const;
	bool setData( const QModelIndex & index, const QVariant & value, int role = Qt::EditRole );
	
	QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
	
	Qt::ItemFlags flags( const QModelIndex & index ) const;
	
	
	enum MsgMode
	{
		EmitMessages, CollectMessages
	};
	
	void setMessageMode( MsgMode m ) { msgMode = m; }
	QList<Message> getMessages() const { QList<Message> lst = messages; messages.clear(); return lst; }
	
signals:
	void sigMessage( const Message & msg ) const;
	void sigProgress( int c, int m ) const;
	
protected:
	virtual bool		updateArrayItem( NifItem * array, bool fast ) = 0;
	int			getArraySize( NifItem * array ) const;
	int			evaluateString( NifItem * array, const QString & text ) const;

	virtual NifItem *	getItem( NifItem * parent, const QString & name ) const;
	NifItem *	getItemX( NifItem * item, const QString & name ) const; // find upwards

	template <typename T> T get( NifItem * parent, const QString & name ) const;
	template <typename T> T get( NifItem * item ) const;
	
	template <typename T> bool set( NifItem * parent, const QString & name, const T & d );
	template <typename T> bool set( NifItem * item, const T & d );
	virtual bool setItemValue( NifItem * item, const NifValue & v ) = 0;
	bool setItemValue( NifItem * parent, const QString & name, const NifValue & v );
	
	virtual bool		evalVersion( NifItem * item, bool chkParents = false ) const = 0;
	bool		evalCondition( NifItem * item, bool chkParents = false ) const;
	bool		evalConditionHelper( NifItem * item, const QString & cond ) const;

	virtual QString ver2str( quint32 ) const = 0;
	virtual quint32 str2ver( QString ) const = 0;

	virtual bool setHeaderString( const QString & ) = 0;
	
	// root item
	NifItem *	root;

	QString folder;
	
	MsgMode msgMode;
	mutable QList<Message> messages;
	
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
