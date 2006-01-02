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
#include <QSharedData>
#include <QStringList>

class QAbstractItemDelegate;

#define str( X ) ( (const char *) X.toAscii() )

class NifSharedData : public QSharedData
{
	friend class NifData;
	
	NifSharedData( const QString & n, const QString & t, const QString & a, const QString & a1, const QString & a2, const QString & c, quint32 v1, quint32 v2 )
		: QSharedData(), name( n ), type( t ), arg( a ), arr1( a1 ), arr2( a2 ), cond( c ), ver1( v1 ), ver2( v2 ) {}

	NifSharedData( const QString & n, const QString & t )
		: QSharedData(), name( n ), type( t ), ver1( 0 ), ver2( 0 ) {}
	
	NifSharedData()
		: QSharedData(), ver1( 0 ), ver2( 0 ) {}
	
	QString  name;
	QString  type;
	QString  arg;
	QString  arr1;
	QString  arr2;
	QString  cond;
	quint32  ver1;
	quint32  ver2;
};

class NifData
{
public:
	NifData( const QString & name, const QString & type, const QVariant & value, const QString & arg, const QString & arr1, const QString & arr2, const QString & cond, quint32 ver1, quint32 ver2 )
		: d( new NifSharedData( name, type, arg, arr1, arr2, cond, ver1, ver2 ) ), val( value ) {}
	
	NifData( const QString & name, const QString & type = QString() )
		: d( new NifSharedData( name, type ) ) {}
	
	NifData()
		: d( new NifSharedData() ) {}
	
	inline const QVariant & value() const			{ return val; }
	inline void setValue( const QVariant & v )	{ val = v; }

	inline const QString & name() const	{ return d->name; }
	inline const QString & type() const	{ return d->type; }
	inline const QString & arg() const	{ return d->arg; }
	inline const QString & arr1() const	{ return d->arr1; }
	inline const QString & arr2() const	{ return d->arr2; }
	inline const QString & cond() const	{ return d->cond; }
	inline quint32 ver1() const			{ return d->ver1; }
	inline quint32 ver2() const			{ return d->ver2; }
	
	void setName( const QString & name )	{ d->name = name; }
	void setType( const QString & type )	{ d->type = type; }
	void setArg( const QString & arg )		{ d->arg = arg; }
	void setArr1( const QString & arr1 )	{ d->arr1 = arr1; }
	void setArr2( const QString & arr2 )	{ d->arr2 = arr2; }
	void setCond( const QString & cond )	{ d->cond = cond; }
	void setVer1( quint32 ver1 )			{ d->ver1 = ver1; }
	void setVer2( quint32 ver2 )			{ d->ver2 = ver2; }

protected:
	QSharedDataPointer<NifSharedData> d;
	
	QVariant val;
};

struct NifBasicType
{
	QString				id;
	int					internalType;
	QString				display;
	QVariant			value;
	QString				text;
	quint32				ver1;
	quint32				ver2;
};

struct NifBlock
{
	QString				id;
	QList<QString>		ancestors;
	QList<NifData>		types;
};


class NifItem;

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
	
	// load and save model data
	bool load( QIODevice & device );
	bool save( QIODevice & device );
	
	bool load( QIODevice & device, const QModelIndex & );
	bool save( QIODevice & device, const QModelIndex & );
	

	// returns the model index of the NiHeader
	QModelIndex getHeader() const;
	// this updates the header infos ( num blocks etc. )
	void updateHeader();
	

	// insert or append ( row == -1 ) a new NiBlock
	void insertNiBlock( const QString & identifier, int row = -1, bool fast = false );
	// remove a block from the list
	void removeNiBlock( int blocknum );
	// returns the block number
	int getBlockNumber( const QModelIndex & ) const;
	// get the NiBlock at index x ( optional: check if it is of type name )
	QModelIndex getBlock( int x, const QString & name = QString() ) const;
	// get the number of NiBlocks
	int getBlockCount() const;
	
	// returns a list with all known NiXXX ids
	static QStringList allNiBlocks();
	// is name a NiBlock identifier?
	static bool isNiBlock( const QString & name );
	
	
	// return the root blocks
	QList<int> getRootLinks() const;
	// return the list of block links
	QList<int> getChildLinks( int block ) const;
	QList<int> getParentLinks( int block ) const;


	// this updates an array ( append or remove items )
	void updateArray( const QModelIndex & array, bool fast = false );
	
	
	// is name an ancestor identifier?
	static bool isAncestor( const QString & name );
	// returns true if name inherits ancestor
	static bool inherits( const QString & name, const QString & ancestor );
	
	
	// is it a basic or compound type?
	static bool isCompound( const QString & name );
	static bool isBasicType( const QString & name );
	
	
	// set item value
	void setItemValue( const QModelIndex & index, const QVariant & v );
	// sets a named attribute to value
	bool setValue( const QModelIndex & index, const QString & name, const QVariant & v );
	
	// get item attributes
	QString  itemName( const QModelIndex & index ) const;
	QString  itemType( const QModelIndex & index ) const;
	QVariant itemValue( const QModelIndex & index ) const;
	QString  itemArg( const QModelIndex & index ) const;
	QString  itemArr1( const QModelIndex & index ) const;
	QString  itemArr2( const QModelIndex & index ) const;
	QString  itemCond( const QModelIndex & index ) const;
	quint32  itemVer1( const QModelIndex & index ) const;
	quint32  itemVer2( const QModelIndex & index ) const;
	
	// find an item named name and return the coresponding value as a QVariant
	QVariant getValue( const QModelIndex & parent, const QString & name ) const;
	// same as getValue but converts the QVariant data to the requested type
	int getInt( const QModelIndex & parent, const QString & nameornumber ) const;
	float getFloat( const QModelIndex & parent, const QString & nameornumber ) const;
	
	// is it a child or parent link?
	bool	itemIsLink( const QModelIndex & index, bool * ischildLink = 0 ) const;
	// this returns a block number if the index is a valid link
	qint32	itemLink( const QModelIndex & index ) const;
	

	// find a branch by name
	QModelIndex getIndex( const QModelIndex & parent, const QString & name ) const;
	
	// evaluate condition and version
	bool evalCondition( const QModelIndex & idx, bool chkParents = false ) const;


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

	void		updateArray( NifItem * array, bool fast );
	int			getArraySize( NifItem * array ) const;

	NifItem *	getHeaderItem() const;
	NifItem *	getBlockItem( int x ) const;
	NifItem *	getItem( NifItem * parent, const QString & name ) const;

	QVariant	getValue( NifItem * parent, const QString & name ) const;
	int			getInt( NifItem * parent, const QString & nameornumber ) const;
	float		getFloat( NifItem * parent, const QString & nameornumber ) const;
	
	bool		evalCondition( NifItem * item, bool chkParents = false ) const;

	bool		itemIsLink( NifItem * item, bool * ischildLink = 0 ) const;
	qint32		itemLink( NifItem * item ) const;
	int			getBlockNumber( NifItem * item ) const;
	
	bool		load( NifItem * parent, QIODevice & device, bool fast = true );
	bool		save( NifItem * parent, QIODevice & device );
	
	// find basic type with matching name and version
	NifBasicType * getType( const QString & name ) const;
	// this returns the internal type of a basic type
	int getInternalType( const QString & name ) const;
	// this returns the display hint of a basic type
	QString getDisplayHint( const QString & name ) const;
	// this returns the description of a basic type
	QString getTypeDescription( const QString & name ) const;
	// this returns the default value of a basic type
	QVariant getTypeValue( const QString & name ) const;
	
	
	// root item
	NifItem *	root;

	// nif file version
	quint32		version;
	QByteArray	version_string;
	
	// internal types: every basic type drops down to one of these
	enum {
		it_uint8 = 0, it_uint16 = 1, it_uint32 = 2,
		it_int8 = 3, it_int16 = 4, it_int32 = 5,
		it_float = 6, it_string = 7,
		it_color3f = 8, it_color4f = 9,
		it_vector = 10, it_quat = 11, it_matrix = 12
	};
	static QStringList internalTypes;
	
	static QList<quint32>					supportedVersions;
	
	//
	static QHash<QString,NifBasicType*>	types;
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

inline bool NifModel::isBasicType( const QString & name )
{
	return types.contains( name );
}

inline int NifModel::getInternalType( const QString & name ) const
{
	NifBasicType * type = getType( name );
	if ( type )		return type->internalType;
	else			return -1;
}

inline QString NifModel::getDisplayHint( const QString & name ) const
{
	NifBasicType * type = getType( name );
	if ( type )		return type->display;
	else			return QString();
}

inline QString NifModel::getTypeDescription( const QString & name ) const
{
	NifBasicType * type = getType( name );
	if ( type )		return type->text;
	else			return QString();
}

inline QVariant NifModel::getTypeValue( const QString & name ) const
{
	NifBasicType * type = getType( name );
	if ( type )		return type->value;
	else			return QVariant();
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

#endif
