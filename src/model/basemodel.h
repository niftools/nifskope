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

#ifndef BaseModel_H
#define BaseModel_H

#include "data/nifitem.h"
#include "message.h"

#include <QAbstractItemModel> // Inherited
#include <QFileInfo>
#include <QIODevice>
#include <QStack>
#include <QString>
#include <QVariant>
#include <QVector>

#include <climits>

#define NifSkopeDisplayRole (Qt::UserRole + 42)


// Used for Block Name hashing
unsigned DJB1Hash( const char * key, unsigned tableSize = UINT_MAX );
// Used to fill arrays
QString addConditionParentPrefix( const QString & x );

//! @file basemodel.h BaseModel, BaseModelEval

class TestMessage;
class QAbstractItemDelegate;

class NifIStream;
class NifOStream;
class NifSStream;

/*! Base class for NIF and KFM models, which store files in memory.
 *
 * This class serves as an abstract base class for NifModel and KfmModel
 * classes.
 */
class BaseModel : public QAbstractItemModel
{
	Q_OBJECT

	friend class NifIStream;
	friend class NifOStream;
	friend class BaseModelEval;

public:
	BaseModel( QObject * parent = nullptr );
	~BaseModel();

	//! Get parent window
	QWidget * getWindow() const { return parentWindow; }

	//! Clear model data.
	virtual void clear() = 0;
	//! Generic load from QIODevice.
	virtual bool load( QIODevice & device ) = 0;
	//! Generic save to QIODevice.
	virtual bool save( QIODevice & device ) const = 0;
	//! Get version as a string
	virtual QString getVersion() const = 0;
	//! Get version as a number
	virtual quint32 getVersionNumber() const = 0;

	//! Load from file.
	bool loadFromFile( const QString & filename );
	//! Save to file.
	bool saveToFile( const QString & str ) const;

	/*! If the model was loaded from a file then getFolder returns the folder.
	 *
	 * This function is used to resolve external resources.
	 * @return The folder of the last file that was loaded with loadFromFile.
	 */
	QString getFolder() const { return folder; }

	/*! If the model was loaded from a file then getFilename returns the filename.
	 *
	 * This function is used to resolve external resources.
	 * @return The filename (without extension) of the last file that was loaded with loadFromFile.
	 */
	QString getFilename() const { return filename; }

	/*! If the model was loaded from a file then getFileInfo returns a QFileInfo object.
	 *
	 * This function is used to resolve external resources.
	 * @return The file info of the last file that was loaded with loadFromFile.
	 */
	QFileInfo getFileInfo() const { return fileinfo; }

	//! Updates stored file and folder information
	void refreshFileInfo( const QString & );

	/*! Return true if the index pointed to is an array.
	 *
	 * @param iArray The index to check.
	 * @return		true if the index is an array.
	 */
	bool isArray( const QModelIndex & iArray ) const;

	/*! Return true if the item is an array or its parent is a multi-array.
	*
	* @param item	The item to check.
	* @return		true if the index is an array.
	*/
	bool isArrayEx( const NifItem * item ) const;

	//! Get an item as a NifValue.
	NifValue getValue( const QModelIndex & index ) const;

	//! Set an item from a NifValue.
	bool setIndexValue( const QModelIndex & index, const NifValue & v );

	//! Get the item name.
	QString itemName( const QModelIndex & index ) const;
	//! Get the item type string.
	QString itemStrType( const QModelIndex & index ) const;
	//! Get the item argument string.
	QString itemArg( const QModelIndex & index ) const;
	//! Get the item arr1 string.
	QString itemArr1( const QModelIndex & index ) const;
	//! Get the item arr2 string.
	QString itemArr2( const QModelIndex & index ) const;
	//! Get the item condition string.
	QString itemCond( const QModelIndex & index ) const;
	//! Get the item first version.
	quint32 itemVer1( const QModelIndex & index ) const;
	//! Get the item last version.
	quint32 itemVer2( const QModelIndex & index ) const;
	//! Get the item documentation.
	QString itemText( const QModelIndex & index ) const;
	//! Get the item template string.
	QString itemTempl( const QModelIndex & index ) const;


	//! Is name a NiBlock identifier (<niobject abstract="0"> or <niobject abstract="1">)?
	virtual bool isAncestorOrNiBlock( const QString & /*name*/ ) const { return false; }
	//! Returns true if name inherits ancestor.
	virtual bool inherits( const QString & /*name*/, const QString & /*ancestor*/ ) const { return false; }
	// This is here to avoid compiler confusion with QObject::inherits.
	bool inherits ( const char * className ) const
	{
		return QObject::inherits( className );
	}

	enum ModelState
	{
		Default,
		Loading,
		Saving,
		Inserting,
		Removing,
		Processing
	};

	//! Get the model's state
	ModelState getState() const { return state; }
	//! Set the model's state
	void setState( ModelState s ) const { states.push( state ); state = s; }
	//! Restore the model's state to the previous
	void restoreState() const { state = states.pop(); }
	//! Reset the model's state
	void resetState() const { state = Default; states.clear(); }
	//! Were there updates while batch processing (also clears the result)
	bool getProcessingResult();

	//! Get Messages collected
	QList<TestMessage> getMessages() const;

	//! Column names
	enum
	{
		NameCol    = 0,
		TypeCol    = 1,
		ValueCol   = 2,
		ArgCol     = 3,
		Arr1Col    = 4,
		Arr2Col    = 5,
		CondCol    = 6,
		Ver1Col    = 7,
		Ver2Col    = 8,
		VerCondCol = 9,
		NumColumns = 10,
	};

	// QAbstractItemModel

	/*! Creates a model index for the given row and column
	 * @see QAbstractItemModel::createIndex()
	 */
	QModelIndex index( int row, int column, const QModelIndex & parent = QModelIndex() ) const override;

	//! Finds the parent of the specified index
	QModelIndex parent( const QModelIndex & index ) const override;

	//! Finds the number of rows
	int rowCount( const QModelIndex & parent = QModelIndex() ) const override;
	//! Finds the number of columns
	int columnCount( const QModelIndex & parent = QModelIndex() ) const override { Q_UNUSED( parent ); return NumColumns; }

	/*! Finds the data associated with an index
	 *
	 * @param index The index to find data for
	 * @param role  The Qt::ItemDataRole to get data for
	 */
	QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const override;

	/*! Sets data associated with an index
	 *
	 * @param index The index to set data for
	 * @param value The data to set
	 * @param role  The Qt::ItemDataRole to use
	 */
	bool setData( const QModelIndex & index, const QVariant & value, int role = Qt::EditRole ) override;

	//! Get the header data for a section
	QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const override;

	//! Finds the flags for an index
	Qt::ItemFlags flags( const QModelIndex & index ) const override;

	// end QAbstractItemModel

	enum MsgMode
	{
		MSG_USER, MSG_TEST
	};

	void setMessageMode( MsgMode mode );
	MsgMode getMessageMode() const { return msgMode; }

	// TODO(Gavrant): replace with reportError?
	void logMessage( const QString & message, const QString & details, QMessageBox::Icon lvl = QMessageBox::Warning ) const;
	// TODO(Gavrant): replace with reportError?
	void logWarning( const QString & details ) const;

public:
	//! Return string representation ("path") of an item within its model (e.g., "NiTriShape [0]\Vertex Data [3]\Vertex colors").
	// Mostly for messages and debug.
	QString itemRepr( const NifItem * item ) const;

	//! Return string representation ("path") of a model index within its model (e.g., "NiTriShape [0]\Vertex Data [3]\Vertex colors").
	// Mostly for messages and debug.
	static QString indexRepr( const QModelIndex & index );

protected:
	virtual QString topItemRepr( const NifItem * item ) const;

public:
	void reportError( const QString & err ) const;
	void reportError( const NifItem * item, const QString & err ) const;
	void reportError( const NifItem * item, const QString & funcName, const QString & err ) const;
	void reportError( const QModelIndex & index, const QString & err ) const;
	void reportError( const QModelIndex & index, const QString & funcName, const QString & err ) const;

public:
	QModelIndex itemToIndex( const NifItem * item, int column = 0 ) const;

	//! Get the top-level parent (a child of the model's root item) of an item.
	// Return null if the item is the root itself or not a child of the root.
	const NifItem * getTopItem( const NifItem * item ) const;
	//! Get the top-level parent (a child of the model's root item) of an item.
	// Return null if the item is the root itself or not a child of the root.
	NifItem * getTopItem( const NifItem * item );
	//! Get the top-level parent (a child of the model's root item) of a model index.
	// Return null if the item is the root itself or not a child of the root.
	const NifItem * getTopItem( const QModelIndex & index ) const;
	//! Get the top-level parent (a child of the model's root item) of a model index.
	// Return null if the item is the root itself or not a child of the root.
	NifItem * getTopItem( const QModelIndex & index );

	//! Get the model index of the top-level parent (a child of the model's root item) of an item.
	// Return invalid index if the item is the root itself or not a child of the root.
	QModelIndex getTopIndex( const NifItem * item ) const;
	//! Get the model index of the top-level parent (a child of the model's root item) of a model index.
	// Return invalid index if the index is the root itself or not a child of the root.
	QModelIndex getTopIndex( const QModelIndex & index ) const;

	// Evaluating NifItem condition and model version
public:
	//! Evaluate NifItem model version.
	bool evalVersion( const NifItem * item ) const;
protected:
	virtual bool evalVersionImpl( const NifItem * item ) const = 0;

public:
	//! Evaluate NifItem model version and condition.
	bool evalCondition( const NifItem * item ) const;
protected:
	virtual bool evalConditionImpl( const NifItem * item ) const;

	// NifItem getters
protected:
	const NifItem * getItemInternal( const NifItem * parent, const QString & name, bool reportErrors ) const;
	const NifItem * getItemInternal( const NifItem * parent, const QLatin1String & name, bool reportErrors ) const;

public:
	//! Get a child NifItem from its parent and name.
	const NifItem * getItem( const NifItem * parent, const QString & name, bool reportErrors = false ) const;
	//! Get a child NifItem from its parent and name.
	NifItem * getItem( const NifItem * parent, const QString & name, bool reportErrors = false );
	//! Get a child NifItem from its parent and name.
	const NifItem * getItem( const NifItem * parent, const QLatin1String & name, bool reportErrors = false ) const;
	//! Get a child NifItem from its parent and name.
	NifItem * getItem( const NifItem * parent, const QLatin1String & name, bool reportErrors = false );
	//! Get a child NifItem from its parent and name.
	const NifItem * getItem( const NifItem * parent, const char * name, bool reportErrors = false ) const;
	//! Get a child NifItem from its parent and name.
	NifItem * getItem( const NifItem * parent, const char * name, bool reportErrors = false );
	//! Get a child NifItem from its parent and numerical index.
	const NifItem * getItem( const NifItem * parent, int childIndex, bool reportErrors = true ) const;
	//! Get a child NifItem from its parent and numerical index.
	NifItem * getItem( const NifItem * parent, int childIndex, bool reportErrors = true );
	//! Get a NifItem from its model index.
	const NifItem * getItem( const QModelIndex & index, bool reportErrors = true ) const;
	//! Get a NifItem from its model index.
	NifItem * getItem( const QModelIndex & index, bool reportErrors = true );
	//! Get a child NifItem from its parent and name.
	const NifItem * getItem( const QModelIndex & parent, const QString & name, bool reportErrors = false ) const;
	//! Get a child NifItem from its parent and name.
	NifItem * getItem( const QModelIndex & parent, const QString & name, bool reportErrors = false );
	//! Get a child NifItem from its parent and name.
	const NifItem * getItem( const QModelIndex & parent, const QLatin1String & name, bool reportErrors = false ) const;
	//! Get a child NifItem from its parent and name.
	NifItem * getItem( const QModelIndex & parent, const QLatin1String & name, bool reportErrors = false );
	//! Get a child NifItem from its parent and name.
	const NifItem * getItem( const QModelIndex & parent, const char * name, bool reportErrors = false ) const;
	//! Get a child NifItem from its parent and name.
	NifItem * getItem( const QModelIndex & parent, const char * name, bool reportErrors = false );
	//! Get a child NifItem from its parent and numerical index.
	const NifItem * getItem( const QModelIndex & parent, int childIndex, bool reportErrors = true ) const;
	//! Get a child NifItem from its parent and numerical index.
	NifItem * getItem( const QModelIndex & parent, int childIndex, bool reportErrors = true );

protected:
	//! Find a NifItem by name, going up from 'item' in the load order.
	const NifItem * getItemX( const NifItem * item, const QLatin1String & name ) const;
	//! Find a NifItem by name, going up from 'item' in the load order.
	const NifItem * getItemX( const NifItem * item, const char * name ) const;

	//! Find a NifItem by name, first in parent, then in parent's parent, etc.
	const NifItem * findItemX( const NifItem * parent, const QLatin1String & name ) const;
	//! Find a NifItem by name, first in parent, then in parent's parent, etc.
	const NifItem * findItemX( const NifItem * parent, const char * name ) const;

	// Item model index getters
public:
	//! Get the model index of a child item.
	QModelIndex getIndex( const NifItem * itemParent, const QString & itemName, int column = 0 ) const;
	//! Get the model index of a child item.
	QModelIndex getIndex( const NifItem * itemParent, const QLatin1String & itemName, int column = 0 ) const;
	//! Get the model index of a child item.
	QModelIndex getIndex( const NifItem * itemParent, const char * itemName, int column = 0 ) const;
	//! Get the model index of a child item.
	QModelIndex getIndex( const QModelIndex & itemParent, const QString & itemName, int column = 0 ) const;
	//! Get the model index of a child item.
	QModelIndex getIndex( const QModelIndex & itemParent, const QLatin1String & itemName, int column = 0 ) const;
	//! Get the model index of a child item.
	QModelIndex getIndex( const QModelIndex & itemParent, const char * itemName, int column = 0 ) const;

	// Item value getters
public:
	//! Get the value of an item.
	template <typename T> T get( const NifItem * item ) const;
	//! Get the value of a child item.
	template <typename T> T get( const NifItem * itemParent, int itemIndex ) const;
	//! Get the value of a child item.
	template <typename T> T get( const NifItem * itemParent, const QString & itemName ) const;
	//! Get the value of a child item.
	template <typename T> T get( const NifItem * itemParent, const QLatin1String & itemName ) const;
	//! Get the value of a child item.
	template <typename T> T get( const NifItem * itemParent, const char * itemName ) const;
	//! Get the value of a model index.
	template <typename T> T get( const QModelIndex & index ) const;
	//! Get the value of a child item.
	template <typename T> T get( const QModelIndex & itemParent, int itemIndex ) const;
	//! Get the value of a child item.
	template <typename T> T get( const QModelIndex & itemParent, const QString & itemName ) const;
	//! Get the value of a child item.
	template <typename T> T get( const QModelIndex & itemParent, const QLatin1String & itemName ) const;
	//! Get the value of a child item.
	template <typename T> T get( const QModelIndex & itemParent, const char * itemName ) const;

	// Item value setters
public:
	//! Set the value of an item.
	template <typename T> bool set( NifItem * item, const T & val );
	//! Set the value of a child item.
	template <typename T> bool set( const NifItem * itemParent, int itemIndex, const T & val );
	//! Set the value of a child item.
	template <typename T> bool set( const NifItem * itemParent, const QString & itemName, const T & val );
	//! Set the value of a child item.
	template <typename T> bool set( const NifItem * itemParent, const QLatin1String & itemName, const T & val );
	//! Set the value of a child item.
	template <typename T> bool set( const NifItem * itemParent, const char * itemName, const T & val );
	//! Set the value of a model index.
	template <typename T> bool set( const QModelIndex & index, const T & val );
	//! Set the value of a child item.
	template <typename T> bool set( const QModelIndex & itemParent, int itemIndex, const T & val );
	//! Set the value of a child item.
	template <typename T> bool set( const QModelIndex & itemParent, const QString & itemName, const T & val );
	//! Set the value of a child item.
	template <typename T> bool set( const QModelIndex & itemParent, const QLatin1String & itemName, const T & val );
	//! Set the value of a child item.
	template <typename T> bool set( const QModelIndex & itemParent, const char * itemName, const T & val );

	// Array size management
protected:	
	//! Get the size of an array
	int evalArraySize( const NifItem * array ) const;
	virtual bool updateArraySizeImpl( NifItem * array ) = 0;
public:
	//! Update the size of an array from its conditions (append missing or remove excess items).
	bool updateArraySize( NifItem * arrayRootItem );
	//! Update the size of a child array from its conditions (append missing or remove excess items).
	bool updateArraySize( const NifItem * arrayParent, int arrayIndex );
	//! Update the size of a child array from its conditions (append missing or remove excess items).
	bool updateArraySize( const NifItem * arrayParent, const QString & arrayName );
	//! Update the size of a child array from its conditions (append missing or remove excess items).
	bool updateArraySize( const NifItem * arrayParent, const QLatin1String & arrayName );
	//! Update the size of a child array from its conditions (append missing or remove excess items).
	bool updateArraySize( const NifItem * arrayParent, const char * arrayName );
	//! Update the size of a model index array from its conditions (append missing or remove excess items).
	bool updateArraySize( const QModelIndex & iArray );
	//! Update the size of a child array from its conditions (append missing or remove excess items).
	bool updateArraySize( const QModelIndex & arrayParent, int arrayIndex );
	//! Update the size of a child array from its conditions (append missing or remove excess items).
	bool updateArraySize( const QModelIndex & arrayParent, const QString & arrayName );
	//! Update the size of a child array from its conditions (append missing or remove excess items).
	bool updateArraySize( const QModelIndex & arrayParent, const QLatin1String & arrayName );
	//! Update the size of a child array from its conditions (append missing or remove excess items).
	bool updateArraySize( const QModelIndex & arrayParent, const char * arrayName );

	// Array getters
public:
	//! Get an array as a QVector.
	template <typename T> QVector<T> getArray( const NifItem * arrayRootItem ) const;
	//! Get a child array as a QVector.
	template <typename T> QVector<T> getArray( const NifItem * arrayParent, int arrayIndex ) const;
	//! Get a child array as a QVector.
	template <typename T> QVector<T> getArray( const NifItem * arrayParent, const QString & arrayName ) const;
	//! Get a child array as a QVector.
	template <typename T> QVector<T> getArray( const NifItem * arrayParent, const QLatin1String & arrayName ) const;
	//! Get a child array as a QVector.
	template <typename T> QVector<T> getArray( const NifItem * arrayParent, const char * arrayName ) const;
	//! Get a model index array as a QVector.
	template <typename T> QVector<T> getArray( const QModelIndex & iArray ) const;
	//! Get a child array as a QVector.
	template <typename T> QVector<T> getArray( const QModelIndex & arrayParent, int arrayIndex ) const;
	//! Get a child array as a QVector.
	template <typename T> QVector<T> getArray( const QModelIndex & arrayParent, const QString & arrayName ) const;
	//! Get a child array as a QVector.
	template <typename T> QVector<T> getArray( const QModelIndex & arrayParent, const QLatin1String & arrayName ) const;
	//! Get a child array as a QVector.
	template <typename T> QVector<T> getArray( const QModelIndex & arrayParent, const char * arrayName ) const;

	// Array setters
public:
	//! Write a QVector to an array.
	template <typename T> void setArray( NifItem * arrayRootItem, const QVector<T> & array );
	//! Write a QVector to a child array.
	template <typename T> void setArray( const NifItem * arrayParent, int arrayIndex, const QVector<T> & array );
	//! Write a QVector to a child array.
	template <typename T> void setArray( const NifItem * arrayParent, const QString & arrayName, const QVector<T> & array );
	//! Write a QVector to a child array.
	template <typename T> void setArray( const NifItem * arrayParent, const QLatin1String & arrayName, const QVector<T> & array );
	//! Write a QVector to a child array.
	template <typename T> void setArray( const NifItem * arrayParent, const char * arrayName, const QVector<T> & array );
	//! Write a QVector to a model index array.
	template <typename T> void setArray( const QModelIndex & iArray, const QVector<T> & array );
	//! Write a QVector to a child array.
	template <typename T> void setArray( const QModelIndex & arrayParent, int arrayIndex, const QVector<T> & array );
	//! Write a QVector to a child array.
	template <typename T> void setArray( const QModelIndex & arrayParent, const QString & arrayName, const QVector<T> & array );
	//! Write a QVector to a child array.
	template <typename T> void setArray( const QModelIndex & arrayParent, const QLatin1String & arrayName, const QVector<T> & array );
	//! Write a QVector to a child array.
	template <typename T> void setArray( const QModelIndex & arrayParent, const char * arrayName, const QVector<T> & array );

	// Array fillers
public:
	//! Fill an array with a value.
	template <typename T> void fillArray( NifItem * arrayRootItem, const T & val );
	//! Fill a child array with a value.
	template <typename T> void fillArray( const NifItem * arrayParent, int arrayIndex, const T & val );
	//! Fill a child array with a value.
	template <typename T> void fillArray( const NifItem * arrayParent, const QString & arrayName, const T & val );
	//! Fill a child array with a value.
	template <typename T> void fillArray( const NifItem * arrayParent, const QLatin1String & arrayName, const T & val );
	//! Fill a child array with a value.
	template <typename T> void fillArray( const NifItem * arrayParent, const char * arrayName, const T & val );
	//! Fill a model index array with a value.
	template <typename T> void fillArray( const QModelIndex & iArray, const T & val );
	//! Fill a child array with a value.
	template <typename T> void fillArray( const QModelIndex & arrayParent, int arrayIndex, const T & val );
	//! Fill a child array with a value.
	template <typename T> void fillArray( const QModelIndex & arrayParent, const QString & arrayName, const T & val );
	//! Fill a child array with a value.
	template <typename T> void fillArray( const QModelIndex & arrayParent, const QLatin1String & arrayName, const T & val );
	//! Fill a child array with a value.
	template <typename T> void fillArray( const QModelIndex & arrayParent, const char * arrayName, const T & val );

signals:
	//! Messaging signal
	void sigMessage( const TestMessage & msg ) const;
	//! Progress signal
	void sigProgress( int c, int m ) const;

protected:
	//! Set an item value
	bool setItemValue( NifItem * item, const NifValue & v );

	//! Convert a version number to a string
	virtual QString ver2str( quint32 ) const = 0;
	//! Convert a version string to a number
	virtual quint32 str2ver( QString ) const = 0;

	//! Set the header string
	virtual bool setHeaderString( const QString &, uint ver = 0 ) = 0;

	void beginInsertRows( const QModelIndex & parent, int first, int last );
	void endInsertRows();

	void beginRemoveRows( const QModelIndex & parent, int first, int last );
	void endRemoveRows();

	virtual void onItemValueChange( NifItem * item );
	void onArrayValuesChange( NifItem * arrayRootItem );

	//! NifSkope window the model belongs to
	QWidget * parentWindow;

	//! The root item
	NifItem * root;

	//! The filepath of the model
	QString folder;
	//! The filename of the model
	QString filename;
	//! The file info for the model
	QFileInfo fileinfo;

	//! A list of test messages
	mutable QList<TestMessage> messages;
	//! Handle a test message
	void testMsg( const QString & m ) const;

	MsgMode msgMode;

	//! The model's state
	mutable ModelState state = Default;
	mutable QStack<ModelState> states;

	//! Has any data changed while processing
	bool changedWhileProcessing = false;
};


//! Helper class for evaluating condition expressions
class BaseModelEval
{
public:
	//! Constructor
	BaseModelEval( const BaseModel * model, const NifItem * item );

	//! Evaluation function
	QVariant operator()( const QVariant & v ) const;

private:
	const BaseModel * model;
	const NifItem * item;
};


// Inlines

inline QModelIndex BaseModel::itemToIndex( const NifItem * item, int column ) const
{
	return ( item && item != root ) ? createIndex( item->row(), column, const_cast<NifItem *>(item) ) : QModelIndex();
}

inline NifItem * BaseModel::getTopItem( const NifItem * item )
{
	return const_cast<NifItem *>( const_cast<const BaseModel *>(this)->getTopItem( item ) );
}
inline const NifItem * BaseModel::getTopItem( const QModelIndex & index ) const
{
	return getTopItem( getItem(index) );
}
inline NifItem * BaseModel::getTopItem( const QModelIndex & index )
{
	return getTopItem( getItem(index) );
}

inline QModelIndex BaseModel::getTopIndex( const NifItem * item ) const
{
	return itemToIndex( getTopItem(item) );
}
inline QModelIndex BaseModel::getTopIndex( const QModelIndex & index ) const
{
	return itemToIndex( getTopItem(index) );
}

inline bool BaseModel::setIndexValue( const QModelIndex & index, const NifValue & val )
{
	return setItemValue( getItem(index), val );
}

inline bool BaseModel::isArray( const QModelIndex & index ) const
{
	auto item = getItem( index ); 
	return item && item->isArray();
}

inline bool BaseModel::isArrayEx( const NifItem * item ) const
{
	return item && item->isArrayEx();
}


// Item getters

#define _BASEMODEL_NONCONST_GETITEM_2(arg1, arg2) const_cast<NifItem *>( const_cast<const BaseModel *>(this)->getItem( arg1, arg2 ) )
#define _BASEMODEL_NONCONST_GETITEM_3(arg1, arg2, arg3) const_cast<NifItem *>( const_cast<const BaseModel *>(this)->getItem( arg1, arg2, arg3 ) )

inline NifItem * BaseModel::getItem( const NifItem * parent, const QString & name, bool reportErrors )
{
	return _BASEMODEL_NONCONST_GETITEM_3( parent, name, reportErrors );
}
inline NifItem * BaseModel::getItem( const NifItem * parent, const QLatin1String & name, bool reportErrors )
{
	return _BASEMODEL_NONCONST_GETITEM_3( parent, name, reportErrors );
}
inline const NifItem * BaseModel::getItem( const NifItem * parent, const char * name, bool reportErrors ) const
{
	return getItem( parent, QLatin1String(name), reportErrors );
}
inline NifItem * BaseModel::getItem( const NifItem * parent, const char * name, bool reportErrors )
{
	return _BASEMODEL_NONCONST_GETITEM_3( parent, QLatin1String(name), reportErrors );
}
inline NifItem * BaseModel::getItem( const NifItem * parent, int childIndex, bool reportErrors )
{
	return _BASEMODEL_NONCONST_GETITEM_3( parent, childIndex, reportErrors );
}
inline NifItem * BaseModel::getItem( const QModelIndex & index, bool reportErrors )
{
	return _BASEMODEL_NONCONST_GETITEM_2( index, reportErrors );
}
inline const NifItem * BaseModel::getItem( const QModelIndex & parent, const QString & name, bool reportErrors ) const
{
	return getItem( getItem(parent), name, reportErrors );
}
inline NifItem * BaseModel::getItem( const QModelIndex & parent, const QString & name, bool reportErrors )
{
	return _BASEMODEL_NONCONST_GETITEM_3( getItem(parent), name, reportErrors );
}
inline const NifItem * BaseModel::getItem( const QModelIndex & parent, const QLatin1String & name, bool reportErrors ) const
{
	return getItem( getItem(parent), name, reportErrors );
}
inline NifItem * BaseModel::getItem( const QModelIndex & parent, const QLatin1String & name, bool reportErrors )
{
	return _BASEMODEL_NONCONST_GETITEM_3( getItem(parent), name, reportErrors );
}
inline const NifItem * BaseModel::getItem( const QModelIndex & parent, const char * name, bool reportErrors ) const
{
	return getItem( getItem(parent), QLatin1String(name), reportErrors );
}
inline NifItem * BaseModel::getItem( const QModelIndex & parent, const char * name, bool reportErrors )
{
	return _BASEMODEL_NONCONST_GETITEM_3( getItem(parent), QLatin1String(name), reportErrors );
}
inline const NifItem * BaseModel::getItem( const QModelIndex & parent, int childIndex, bool reportErrors ) const
{
	return getItem( getItem(parent), childIndex, reportErrors );
}
inline NifItem * BaseModel::getItem( const QModelIndex & parent, int childIndex, bool reportErrors )
{
	return _BASEMODEL_NONCONST_GETITEM_3( getItem(parent), childIndex, reportErrors );
}

inline const NifItem * BaseModel::getItemX( const NifItem * item, const char * name ) const
{
	return getItemX( item, QLatin1String(name) );
}

inline const NifItem * BaseModel::findItemX( const NifItem * parent, const char * name ) const
{
	return findItemX( parent, QLatin1String(name) );
}


// Item model index getters

inline QModelIndex BaseModel::getIndex( const NifItem * itemParent, const QString & itemName, int column ) const
{
	return itemToIndex( getItem(itemParent, itemName), column );
}
inline QModelIndex BaseModel::getIndex( const NifItem * itemParent, const QLatin1String & itemName, int column ) const
{
	return itemToIndex( getItem(itemParent, itemName), column );
}
inline QModelIndex BaseModel::getIndex( const NifItem * itemParent, const char * itemName, int column ) const
{
	return itemToIndex( getItem(itemParent, QLatin1String(itemName)), column );
}
inline QModelIndex BaseModel::getIndex( const QModelIndex & itemParent, const QString & itemName, int column ) const
{
	return itemToIndex( getItem(itemParent, itemName), column );
}
inline QModelIndex BaseModel::getIndex( const QModelIndex & itemParent, const QLatin1String & itemName, int column ) const
{
	return itemToIndex( getItem(itemParent, itemName), column );
}
inline QModelIndex BaseModel::getIndex( const QModelIndex & itemParent, const char * itemName, int column ) const
{
	return itemToIndex( getItem(itemParent, QLatin1String(itemName)), column );
}


// Item value getters

template <typename T> inline T BaseModel::get( const NifItem * item ) const
{
	return NifItem::get<T>( item );
}
template <typename T> inline T BaseModel::get( const NifItem * itemParent, int itemIndex ) const
{
	return NifItem::get<T>( getItem(itemParent, itemIndex) );
}
template <typename T> inline T BaseModel::get( const NifItem * itemParent, const QString & itemName ) const
{
	return NifItem::get<T>( getItem(itemParent, itemName) );
}
template <typename T> inline T BaseModel::get( const NifItem * itemParent, const QLatin1String & itemName ) const
{
	return NifItem::get<T>( getItem(itemParent, itemName) );
}
template <typename T> inline T BaseModel::get( const NifItem * itemParent, const char * itemName ) const
{
	return NifItem::get<T>( getItem(itemParent, QLatin1String(itemName)) );
}
template <typename T> inline T BaseModel::get( const QModelIndex & index ) const
{
	return NifItem::get<T>( getItem(index) );
}
template <typename T> inline T BaseModel::get( const QModelIndex & itemParent, int itemIndex ) const
{
	return NifItem::get<T>( getItem(itemParent, itemIndex) );
}
template <typename T> inline T BaseModel::get( const QModelIndex & itemParent, const QString & itemName ) const
{
	return NifItem::get<T>( getItem(itemParent, itemName) );
}
template <typename T> inline T BaseModel::get( const QModelIndex & itemParent, const QLatin1String & itemName ) const
{
	return NifItem::get<T>( getItem(itemParent, itemName) );
}
template <typename T> inline T BaseModel::get( const QModelIndex & itemParent, const char * itemName ) const
{
	return NifItem::get<T>( getItem(itemParent, QLatin1String(itemName)) );
}


// Item value setters

template <typename T> inline bool BaseModel::set( NifItem * item, const T & val )
{
	if ( NifItem::set<T>( item, val ) ) {
		onItemValueChange( item );
		return true;
	}

	return false;
}
template <typename T> inline bool BaseModel::set( const NifItem * itemParent, int itemIndex, const T & val )
{
	return set( getItem(itemParent, itemIndex, true), val );
}
template <typename T> inline bool BaseModel::set( const NifItem * itemParent, const QString & itemName, const T & val )
{
	return set( getItem(itemParent, itemName, true), val );
}
template <typename T> inline bool BaseModel::set( const NifItem * itemParent, const QLatin1String & itemName, const T & val )
{
	return set( getItem(itemParent, itemName, true), val );
}
template <typename T> inline bool BaseModel::set( const NifItem * itemParent, const char * itemName, const T & val )
{
	return set( getItem(itemParent, QLatin1String(itemName), true), val );
}
template <typename T> inline bool BaseModel::set( const QModelIndex & index, const T & val )
{
	return set( getItem(index), val );
}
template <typename T> inline bool BaseModel::set( const QModelIndex & itemParent, int itemIndex, const T & val )
{
	return set( getItem(itemParent, itemIndex, true), val );
}
template <typename T> inline bool BaseModel::set( const QModelIndex & itemParent, const QString & itemName, const T & val )
{
	return set( getItem(itemParent, itemName, true), val );
}
template <typename T> inline bool BaseModel::set( const QModelIndex & itemParent, const QLatin1String & itemName, const T & val )
{
	return set( getItem(itemParent, itemName, true), val );
}
template <typename T> inline bool BaseModel::set( const QModelIndex & itemParent, const char * itemName, const T & val )
{
	return set( getItem(itemParent, QLatin1String(itemName), true), val );
}


// Array size management

inline bool BaseModel::updateArraySize( NifItem * arrayRootItem )
{
	return updateArraySizeImpl( arrayRootItem );
}
inline bool BaseModel::updateArraySize( const NifItem * arrayParent, int arrayIndex )
{
	return updateArraySizeImpl( getItem(arrayParent, arrayIndex, true) );
}
inline bool BaseModel::updateArraySize( const NifItem * arrayParent, const QString & arrayName )
{
	return updateArraySizeImpl( getItem(arrayParent, arrayName, true) );
}
inline bool BaseModel::updateArraySize( const NifItem * arrayParent, const QLatin1String & arrayName )
{
	return updateArraySizeImpl( getItem(arrayParent, arrayName, true) );
}
inline bool BaseModel::updateArraySize( const NifItem * arrayParent, const char * arrayName )
{
	return updateArraySizeImpl( getItem(arrayParent, QLatin1String(arrayName), true) );
}
inline bool BaseModel::updateArraySize( const QModelIndex & iArray )
{
	return updateArraySizeImpl( getItem(iArray) );
}
inline bool BaseModel::updateArraySize( const QModelIndex & arrayParent, int arrayIndex )
{
	return updateArraySizeImpl( getItem(arrayParent, arrayIndex, true) );
}
inline bool BaseModel::updateArraySize( const QModelIndex & arrayParent, const QString & arrayName )
{
	return updateArraySizeImpl( getItem(arrayParent, arrayName, true) );
}
inline bool BaseModel::updateArraySize( const QModelIndex & arrayParent, const QLatin1String & arrayName )
{
	return updateArraySizeImpl( getItem(arrayParent, arrayName, true) );
}
inline bool BaseModel::updateArraySize( const QModelIndex & arrayParent, const char * arrayName )
{
	return updateArraySizeImpl( getItem(arrayParent, QLatin1String(arrayName), true) );
}


// Array getters

template <typename T> inline QVector<T> BaseModel::getArray( const NifItem * arrayRootItem ) const
{
	return NifItem::getArray<T>( arrayRootItem );
}
template <typename T> inline QVector<T> BaseModel::getArray( const NifItem * arrayParent, int arrayIndex ) const
{
	return NifItem::getArray<T>( getItem(arrayParent, arrayIndex) );
}
template <typename T> inline QVector<T> BaseModel::getArray( const NifItem * arrayParent, const QString & arrayName ) const
{
	return NifItem::getArray<T>( getItem(arrayParent, arrayName) );
}
template <typename T> inline QVector<T> BaseModel::getArray( const NifItem * arrayParent, const QLatin1String & arrayName ) const
{
	return NifItem::getArray<T>( getItem(arrayParent, arrayName) );
}
template <typename T> inline QVector<T> BaseModel::getArray( const NifItem * arrayParent, const char * arrayName ) const
{
	return NifItem::getArray<T>( getItem(arrayParent, QLatin1String(arrayName)) );
}
template <typename T> inline QVector<T> BaseModel::getArray( const QModelIndex & iArray ) const
{
	return NifItem::getArray<T>( getItem(iArray) );
}
template <typename T> inline QVector<T> BaseModel::getArray( const QModelIndex & arrayParent, int arrayIndex ) const
{
	return NifItem::getArray<T>( getItem(arrayParent, arrayIndex) );
}
template <typename T> inline QVector<T> BaseModel::getArray( const QModelIndex & arrayParent, const QString & arrayName ) const
{
	return NifItem::getArray<T>( getItem(arrayParent, arrayName));
}
template <typename T> inline QVector<T> BaseModel::getArray( const QModelIndex & arrayParent, const QLatin1String & arrayName ) const
{
	return NifItem::getArray<T>( getItem(arrayParent, arrayName) );
}
template <typename T> inline QVector<T> BaseModel::getArray( const QModelIndex & arrayParent, const char * arrayName ) const
{
	return NifItem::getArray<T>( getItem(arrayParent, QLatin1String(arrayName)) );
}


// Array setters

template <typename T> inline void BaseModel::setArray( NifItem * arrayRootItem, const QVector<T> & array )
{
	if ( arrayRootItem ) {
		arrayRootItem->setArray<T>( array );
		onArrayValuesChange( arrayRootItem );
	}
}
template <typename T> inline void BaseModel::setArray( const NifItem * arrayParent, int arrayIndex, const QVector<T> & array )
{
	setArray( getItem(arrayParent, arrayIndex, true), array );
}
template <typename T> inline void BaseModel::setArray( const NifItem * arrayParent, const QString & arrayName, const QVector<T> & array )
{
	setArray( getItem(arrayParent, arrayName, true), array );
}
template <typename T> inline void BaseModel::setArray( const NifItem * arrayParent, const QLatin1String & arrayName, const QVector<T> & array )
{
	setArray( getItem(arrayParent, arrayName, true), array );
}
template <typename T> inline void BaseModel::setArray( const NifItem * arrayParent, const char * arrayName, const QVector<T> & array )
{
	setArray( getItem(arrayParent, QLatin1String(arrayName), true), array );
}
template <typename T> inline void BaseModel::setArray( const QModelIndex & iArray, const QVector<T> & array )
{
	setArray( getItem(iArray), array );
}
template <typename T> inline void BaseModel::setArray( const QModelIndex & arrayParent, int arrayIndex, const QVector<T> & array )
{
	setArray( getItem(arrayParent, arrayIndex, true), array );
}
template <typename T> inline void BaseModel::setArray( const QModelIndex & arrayParent, const QString & arrayName, const QVector<T> & array )
{
	setArray( getItem(arrayParent, arrayName, true), array );
}
template <typename T> inline void BaseModel::setArray( const QModelIndex & arrayParent, const QLatin1String & arrayName, const QVector<T> & array )
{
	setArray( getItem(arrayParent, arrayName, true), array );
}
template <typename T> inline void BaseModel::setArray( const QModelIndex & arrayParent, const char * arrayName, const QVector<T> & array )
{
	setArray( getItem(arrayParent, QLatin1String(arrayName), true), array );
}


// Array fillers

template <typename T> inline void BaseModel::fillArray( NifItem * arrayRootItem, const T & val )
{
	if ( arrayRootItem ) {
		arrayRootItem->fillArray<T>( val );
		onArrayValuesChange( arrayRootItem );
	}
}
template <typename T> inline void BaseModel::fillArray( const NifItem * arrayParent, int arrayIndex, const T & val )
{
	fillArray( getItem(arrayParent, arrayIndex, true), val );
}
template <typename T> inline void BaseModel::fillArray( const NifItem * arrayParent, const QString & arrayName, const T & val )
{
	fillArray( getItem(arrayParent, arrayName, true), val );
}
template <typename T> inline void BaseModel::fillArray( const NifItem * arrayParent, const QLatin1String & arrayName, const T & val )
{
	fillArray( getItem(arrayParent, arrayName, true), val );
}
template <typename T> inline void BaseModel::fillArray( const NifItem * arrayParent, const char * arrayName, const T & val )
{
	fillArray( getItem(arrayParent, QLatin1String(arrayName), true), val );
}
template <typename T> inline void BaseModel::fillArray( const QModelIndex & iArray, const T & val )
{
	fillArray( getItem(iArray), val );
}
template <typename T> inline void BaseModel::fillArray( const QModelIndex & arrayParent, int arrayIndex, const T & val )
{
	fillArray( getItem(arrayParent, arrayIndex, true), val );
}
template <typename T> inline void BaseModel::fillArray( const QModelIndex & arrayParent, const QString & arrayName, const T & val )
{
	fillArray( getItem(arrayParent, arrayName, true), val );
}
template <typename T> inline void BaseModel::fillArray( const QModelIndex & arrayParent, const QLatin1String & arrayName, const T & val )
{
	fillArray( getItem(arrayParent, arrayName, true), val );
}
template <typename T> inline void BaseModel::fillArray( const QModelIndex & arrayParent, const char * arrayName, const T & val )
{
	fillArray( getItem(arrayParent, QLatin1String(arrayName), true), val );
}

#endif
