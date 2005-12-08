/****************************************************************************
**
** nifscope: a tool for analyzing and editing NetImmerse files (.nif & .kf)
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/


#ifndef NIFMODEL_H
#define NIFMODEL_H

#include <QAbstractItemModel>

#include <QHash>
#include <QIODevice>
#include <QStack>
#include <QStringList>

class QAbstractItemDelegate;

#define str( X ) ( (const char *) X.toAscii() )

struct NifData
{
	QString  name;
	QString  type;
	QVariant value;
	QString  arg;
	QString  arr1;
	QString  arr2;
	QString  cond;
	quint32  ver1;
	quint32  ver2;
	
	NifData( QString n, QString t, QVariant v, QString ar, QString ar1, QString ar2, QString c, quint32 v1, quint32 v2 )
	{
		name  = n;
		type  = t;
		value = v;
		arg   = ar;
		arr1  = ar1;
		arr2  = ar2;
		cond  = c;
		ver1  = v1;
		ver2  = v2;
	}
	
	NifData( QString n, QString t = QString() )
	{
		name = n;
		type = t;
		ver1 = 0;
		ver2 = 0;
	}
	
	NifData()
	{
		ver1 = ver2 = 0;
	}
};

class NifBasicType
{
public:
	QString				id;
	int					internalType;
	QString				display;
	QVariant			value;
	QString				text;
	quint32				ver1;
	quint32				ver2;
};

class NifBlock
{
public:
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
	
	// clear model data
	void clear();
	
	// load and save model data
	bool load( QIODevice & device );
	bool save( QIODevice & device );
	
	// call this once with the nif description xml file as argument
	static QString parseXmlDescription( const QString & filename );
	

	// returns the model index of the NiHeader
	QModelIndex getHeader() const;
	// this updates the header infos ( num blocks etc. )
	void updateHeader();
	
	int getArraySize( const QModelIndex & array ) const;
	// this updates an array ( append or remove items )
	void updateArray( const QModelIndex & array, bool fast = false );
	
	
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
	QList<int> getChildLinks( int block ) const;
	QList<int> getParentLinks( int block ) const;


	// insert abstract ancestor block
	void inherit( const QString & identifier, const QModelIndex & idx, int row = -1 );
	// is name an ancestor identifier?
	static bool isAncestor( const QString & name );
	// returns true if name inherits ancestor
	static bool inherits( const QString & name, const QString & ancestor );
	
	
	// insert or append a basic or compound type
	void insertType( const NifData & data, const QModelIndex & idx, int row = -1 );
	// is it a basic or compound type?
	static bool isCompound( const QString & name );
	static bool isBasicType( const QString & name );
	// this returns true if type is a basic or compound type and has no conditional attributes
	static bool isUnconditional( const QString & type );
		
	// this returns the internal type of a basic type
	int getInternalType( const QString & name ) const;
	// this returns the display hint of a basic type
	QString getDisplayHint( const QString & name ) const;
	// this returns the description of a basic type
	QString getTypeDescription( const QString & name ) const;
	// this returns the default value of a basic type
	QVariant getTypeValue( const QString & name ) const;
	
	
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
	
	// this returns a block number if the index is a valid link
	qint32	itemLink( const QModelIndex & index ) const;
	// is it a child or parent link?
	bool	isLink( const QModelIndex & index, bool * ischildLink = 0 ) const;
	
	// set item attributes
	void setItemValue( const QModelIndex & index, const QVariant & v );

	// find an item named name and return the coresponding value as a QVariant
	QVariant getValue( const QModelIndex & parent, const QString & name ) const;
	// same as getValue but converts the QVariant data to the requested type
	int getInt( const QModelIndex & parent, const QString & nameornumber ) const;
	float getFloat( const QModelIndex & parent, const QString & nameornumber ) const;
	
	// sets a named attribute to value
	bool setValue( const QModelIndex & index, const QString & name, const QVariant & v );
	
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
	
signals:
	void linksChanged();

protected:
	// create a tree branch
	QModelIndex insertBranch( NifData data, const QModelIndex & parent, int row = -1 );
	// create a tree leaf
	void insertLeaf( const NifData & data, const QModelIndex & parent, int row = -1 );
	
	// recursive load and save a branch
	bool load( const QModelIndex & parent, QIODevice & device );
	bool save( const QModelIndex & parent, QIODevice & device );
	
	// find basic type with matching name and version
	NifBasicType * getType( const QString & name ) const;
	
	// root item
	NifItem * root;

	// nif file version
	quint32		version;
	QByteArray	version_string;
	
	// internal types: every basic type drops down to one of these
	enum {
		it_uint8 = 0, it_uint16 = 1, it_uint32 = 2,
		it_int8 = 3, it_int16 = 4, it_int32 = 5,
		it_float = 6, it_string = 7,
		it_color3f = 8, it_color4f = 9
	};
	static QStringList internalTypes;
	
	//
	static QHash<QString,NifBasicType*>	types;
	static QHash<QString,NifBlock*>		compounds;
	static QHash<QString,NifBlock*>		ancestors;
	static QHash<QString,NifBlock*>		blocks;
	
	static QStringList uncondTypes;
	
	QHash< int, QList<int> > childLinks;
	QHash< int, QList<int> > parentLinks;
	QList<int> rootLinks;
	
	void updateRootLinks();
	void checkLinks( int block, QStack<int> & parents );
	void updateLinks( int block = -1 );
	void updateLinks( int block, const QModelIndex & );
	
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

inline bool NifModel::isUnconditional( const QString & name )
{
	return uncondTypes.contains( name );
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
