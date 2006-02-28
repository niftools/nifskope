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

#ifndef NIFMODEL_H
#define NIFMODEL_H

#include <QAbstractItemModel>

#include <QHash>
#include <QIODevice>
#include <QStack>
#include <QStringList>

class QAbstractItemDelegate;

#include "niftypes.h"
#include "nifitem.h"

class NifModel : public QAbstractItemModel
{
Q_OBJECT
public:
	NifModel( QObject * parent = 0 );
	~NifModel();
	
	// call this once with the nif description xml file as argument
	static QString parseXmlDescription( const QString & filename );

	
	// clear model data
	void clear();
	
	// load and save to and from file
	bool load( const QString & filename );
	bool save( const QString & filename ) const;
	
	// generic load and save to and from QIODevice
	bool load( QIODevice & device );
	bool save( QIODevice & device ) const;
	
	bool load( QIODevice & device, const QModelIndex & );
	bool save( QIODevice & device, const QModelIndex & ) const;
	
	bool loadAndMapLinks( QIODevice & device, const QModelIndex &, const QMap<qint32,qint32> & map );
	
	// if the model was loaded from a file getFolder returns the directory
	// can be used to resolve external resources
	QString getFolder() const { return folder; }
	
	
	QString getVersion() const { return version2string( version ); }
	
	// returns the model index of the NiHeader
	QModelIndex getHeader() const;
	// this updates the header infos ( num blocks etc. )
	void updateHeader();
	
	QModelIndex getFooter() const;
	void updateFooter();
	

	// insert or append ( row == -1 ) a new NiBlock
	QModelIndex insertNiBlock( const QString & identifier, int row = -1, bool fast = false );
	// remove a block from the list
	void removeNiBlock( int blocknum );
	// returns the block number
	int getBlockNumber( const QModelIndex & ) const;
	// returns the parent block ( optional: check if it is of type name )
	QModelIndex getBlock( const QModelIndex &, const QString & name = QString() ) const;
	// returns the parent block/header
	QModelIndex getBlockOrHeader( const QModelIndex & ) const;
	// get the NiBlock at index x ( optional: check if it is of type name )
	QModelIndex getBlock( int x, const QString & name = QString() ) const;
	// get the number of NiBlocks
	int getBlockCount() const;
	// returns true if the index is a niblock ( optional: check if it is the specified type of block )
	bool isNiBlock( const QModelIndex & index, const QString & name = QString() ) const;
	// returns a list with all known NiXXX ids
	static QStringList allNiBlocks();
	// is name a NiBlock identifier?
	static bool isNiBlock( const QString & name );
	
	
	// return the root blocks
	QList<int> getRootLinks() const;
	// return the list of block links
	QList<int> getChildLinks( int block ) const;
	QList<int> getParentLinks( int block ) const;
	// return the parent block number or none (-1) if there is no parent or if there are multiple parents
	int getParent( int block ) const;

	// is it a child or parent link?
	bool isLink( const QModelIndex & index, bool * ischildLink = 0 ) const;
	// this returns a block number if the index is a valid link
	qint32 getLink( const QModelIndex & index ) const;
	int getLink( const QModelIndex & parent, const QString & name ) const;
	bool setLink( const QModelIndex & parent, const QString & name, qint32 l );
	bool setLink( const QModelIndex & index, qint32 l );


	bool isArray( const QModelIndex & array ) const;
	// this updates an array ( append or remove items )
	bool updateArray( const QModelIndex & array );
	// read an array
	template <typename T> QVector<T> getArray( const QModelIndex & iArray ) const;
	template <typename T> QVector<T> getArray( const QModelIndex & iArray, const QString & name ) const;
	// write an array
	template <typename T> void setArray( const QModelIndex & iArray, const QVector<T> & array );
	template <typename T> void setArray( const QModelIndex & iArray, const QString & name, const QVector<T> & array );
	
	
	// is it a compound type?
	static bool isCompound( const QString & name );
	// is name an ancestor identifier?
	static bool isAncestor( const QString & name );
	// returns true if name inherits ancestor
	static bool inherits( const QString & name, const QString & ancestor );
	// returns true if the block containing index inherits ancestor
	bool inherits( const QModelIndex & index, const QString & ancestor ) const;
	
	
	template <typename T> T get( const QModelIndex & index ) const;
	template <typename T> bool set( const QModelIndex & index, const T & d );	
	
	// find an item named name and return the coresponding value
	template <typename T> T get( const QModelIndex & parent, const QString & name ) const;
	template <typename T> bool set( const QModelIndex & parent, const QString & name, const T & v );
	
	// set item value
	bool setValue( const QModelIndex & index, const NifValue & v );
	// sets a named attribute to value
	bool setValue( const QModelIndex & index, const QString & name, const NifValue & v );
	
	NifValue getValue( const QModelIndex & index ) const;
	NifValue getValue( const QModelIndex & index, const QString & name ) const;
	
	// get item attributes
	QString  itemName( const QModelIndex & index ) const;
	QString  itemType( const QModelIndex & index ) const;
	QString  itemArg( const QModelIndex & index ) const;
	QString  itemArr1( const QModelIndex & index ) const;
	QString  itemArr2( const QModelIndex & index ) const;
	QString  itemCond( const QModelIndex & index ) const;
	quint32  itemVer1( const QModelIndex & index ) const;
	quint32  itemVer2( const QModelIndex & index ) const;
	

	// find a branch by name
	QModelIndex getIndex( const QModelIndex & parent, const QString & name ) const;
	
	// evaluate condition and version
	bool evalCondition( const QModelIndex & idx, bool chkParents = false ) const;
	// evaluate version
	bool evalVersion( const QModelIndex & idx, bool chkParents = false ) const;

	// check wether the current nif file version lies in the range since~until
	bool checkVersion( quint32 since, quint32 until ) const;

	// version conversion
	static QString version2string( quint32 );
	static quint32 version2number( const QString & );
	
	
	// QAbstractModel interface
	
	QModelIndex index( int row, int column, const QModelIndex & parent = QModelIndex() ) const;
	QModelIndex parent( const QModelIndex & index ) const;

	int rowCount( const QModelIndex & parent = QModelIndex() ) const;
	int columnCount( const QModelIndex & parent = QModelIndex() ) const { return 9; }
	
	QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const;
	bool setData( const QModelIndex & index, const QVariant & value, int role = Qt::EditRole );
	
	QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
	
	Qt::ItemFlags flags( const QModelIndex & index ) const
	{
		if ( !index.isValid() ) return Qt::ItemIsEnabled;
		switch( index.column() )
		{
			case TypeCol:
				return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
			case ValueCol:
				if ( itemArr1( index ).isEmpty() )
					return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
				else
					return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
			default:
				return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
		}
	}
	
	void reset();
	
	static QAbstractItemDelegate * createDelegate();
	

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
		Ver2Col  = 8
	};
	
signals:
	void linksChanged();

protected:
	void		insertAncestor( NifItem * parent, const QString & identifier, int row = -1 );
	void		insertType( NifItem * parent, const NifData & data, int row = -1 );
	NifItem *	insertBranch( NifItem * parent, const NifData & data, int row = -1 );

	bool		updateArray( NifItem * array, bool fast );
	int			getArraySize( NifItem * array ) const;

	NifItem *	getHeaderItem() const;
	NifItem *   getFooterItem() const;
	NifItem *	getBlockItem( int x ) const;
	NifItem *	getItem( NifItem * parent, const QString & name ) const;

	template <typename T> T get( NifItem * parent, const QString & name ) const;
	template <typename T> T get( NifItem * item ) const;
	
	template <typename T> bool set( NifItem * parent, const QString & name, const T & d );
	template <typename T> bool set( NifItem * item, const T & d );
	bool setItemValue( NifItem * item, const NifValue & v );
	bool setItemValue( NifItem * parent, const QString & name, const NifValue & v );
	
	
	bool		evalVersion( NifItem * item, bool chkParents = false ) const;
	bool		evalCondition( NifItem * item, bool chkParents = false ) const;

	bool		itemIsLink( NifItem * item, bool * ischildLink = 0 ) const;
	int			getBlockNumber( NifItem * item ) const;
	
	bool		load( NifItem * parent, NifStream & stream, bool fast = true );
	bool		save( NifItem * parent, NifStream & stream ) const;
	
	// root item
	NifItem *	root;

	// nif file version
	quint32		version;
	
	static QList<quint32>		supportedVersions;
	
	QString folder;
	
	//
	static QHash<QString,NifBlock*>		compounds;
	static QHash<QString,NifBlock*>		ancestors;
	static QHash<QString,NifBlock*>		blocks;
	
	QHash< int, QList<int> >	childLinks;
	QHash< int, QList<int> >	parentLinks;
	QList< int >				rootLinks;
	
	void updateLinks( int block = -1 );
	void updateLinks( int block, NifItem * parent );
	void checkLinks( int block, QStack<int> & parents );
	void adjustLinks( NifItem * parent, int block, int delta );
	void mapLinks( NifItem * parent, const QMap<qint32,qint32> & map );
		
	friend class NifXmlHandler;
}; // class NifModel


inline QStringList NifModel::allNiBlocks()
{
	return blocks.keys();
}

inline bool NifModel::isNiBlock( const QString & name )
{
	return blocks.contains( name );
}

inline bool NifModel::isAncestor( const QString & name )
{
	return ancestors.contains( name );
}

inline bool NifModel::isCompound( const QString & name )
{
	return compounds.contains( name );
}

inline QList<int> NifModel::getRootLinks() const
{
	return rootLinks;
}

inline QList<int> NifModel::getChildLinks( int block ) const
{
	return childLinks.value( block );
}

inline QList<int> NifModel::getParentLinks( int block ) const
{
	return parentLinks.value( block );
}

template <typename T> inline T NifModel::get( NifItem * parent, const QString & name ) const
{
	NifItem * item = getItem( parent, name );
	if ( item )
		return item->value().get<T>();
	else
		return T();
}

template <typename T> inline T NifModel::get( const QModelIndex & parent, const QString & name ) const
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

template <typename T> inline bool NifModel::set( NifItem * parent, const QString & name, const T & d )
{
	NifItem * item = getItem( parent, name );
	if ( item )
		return set( item, d );
	else
		return false;
}

template <typename T> inline bool NifModel::set( const QModelIndex & parent, const QString & name, const T & d )
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

template <typename T> inline T NifModel::get( NifItem * item ) const
{
	return item->value().get<T>();
}

template <typename T> inline T NifModel::get( const QModelIndex & index ) const
{
	NifItem * item = static_cast<NifItem*>( index.internalPointer() );
	if ( ! ( index.isValid() && item && index.model() == this ) )
		return T();
	
	return item->value().get<T>();
}

template <typename T> inline bool NifModel::set( NifItem * item, const T & d )
{
	if ( item->value().set( d ) )
	{
		emit dataChanged( createIndex( item->row(), ValueCol, item ), createIndex( item->row(), ValueCol, item ) );
		return true;
	}
	else
		return false;
}

template <typename T> inline bool NifModel::set( const QModelIndex & index, const T & d )
{
	NifItem * item = static_cast<NifItem*>( index.internalPointer() );
	if ( ! ( index.isValid() && item && index.model() == this ) )	return false;
	return set( item, d );
}

template <typename T> inline QVector<T> NifModel::getArray( const QModelIndex & iArray ) const
{
	NifItem * item = static_cast<NifItem*>( iArray.internalPointer() );
	if ( isArray( iArray ) && item && iArray.model() == this )
		return item->getArray<T>();
	return QVector<T>();
}

template <typename T> inline QVector<T> NifModel::getArray( const QModelIndex & iParent, const QString & name ) const
{
	return getArray<T>( getIndex( iParent, name ) );
}

template <typename T> inline void NifModel::setArray( const QModelIndex & iArray, const QVector<T> & array )
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

template <typename T> inline void NifModel::setArray( const QModelIndex & iParent, const QString & name, const QVector<T> & array )
{
	setArray<T>( getIndex( iParent, name ), array );
}

inline bool NifModel::checkVersion( quint32 since, quint32 until ) const
{
	return ( ( since == 0 || since <= version ) && ( until == 0 || version <= until ) );
}

#endif
