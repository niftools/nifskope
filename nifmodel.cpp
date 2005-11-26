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


#include "nifmodel.h"

#include <QByteArray>
#include <QColor>
#include <QDebug>

class NifItem
{
public:
	NifItem( NifItem * parent = 0 ) : parentItem( parent )
	{
	}
	NifItem( const NifData & data, NifItem * parent = 0 ) : parentItem( parent ), itemData( data )
	{
	}
	~NifItem()
	{
		qDeleteAll( childItems );
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
	
	void removeChild( int row )
	{
		NifItem * item = child( row );
		if ( item )
		{
			childItems.removeAt( row );
			delete item;
		}
	}
	
	void killChildren()
	{
		qDeleteAll( childItems );
		childItems.clear();
	}
	
	NifItem * parent() const
	{
		return parentItem;
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
	
	int row() const
	{
		if ( parentItem )
			return parentItem->childItems.indexOf( const_cast<NifItem*>(this) );
		return 0;
	}
	
	inline QString  name() const	{	return itemData.name;	}
	inline QString  type() const	{	return itemData.type;	}
	inline QVariant value() const	{	return itemData.value;	}
	inline QString  arg() const	{	return itemData.arg;	}
	inline QString  arr1() const	{	return itemData.arr1;	}
	inline QString  arr2() const	{	return itemData.arr2;	}
	inline QString  cond() const	{	return itemData.cond;	}
	inline quint32  ver1() const	{	return itemData.ver1;	}
	inline quint32  ver2() const	{	return itemData.ver2;	}
	
	inline void setName( const QString & name )	{	itemData.name = name;	}
	inline void setType( const QString & type )	{	itemData.type = type;	}
	inline	void setValue( const QVariant & v )	{	itemData.value = v;		}
	inline void setArg( const QString & arg )		{	itemData.arg = arg;		}
	inline void setArr1( const QString & arr1 )	{	itemData.arr1 = arr1;	}
	inline void setArr2( const QString & arr2 )	{	itemData.arr2 = arr2;	}
	inline void setCond( const QString & cond )	{	itemData.cond = cond;	}
	inline void setVer1( int v1 )					{	itemData.ver1 = v1;		}
	inline void setVer2( int v2 )					{	itemData.ver2 = v2;		}

	void adjustLinks( NifModel * model, int l, int d )
	{
		if ( childItems.count() == 0 )
		{
			if ( model->getDisplayHint( type() ) == "link" && ( ( d != 0 && value().toInt() >= l ) || value().toInt() == l ) )
			{
				if ( d == 0 )
					itemData.value = -1;
				else
					itemData.value = itemData.value.toInt() + d;
			}
		}
		else
		{
			foreach ( NifItem * child, childItems )
			{
				child->adjustLinks( model, l, d );
			}
		}
	}

private:
	NifItem * parentItem;
	QList<NifItem*> childItems;
	NifData itemData;
};


NifModel::NifModel( QObject * parent ) : QAbstractItemModel( parent ), version( 0x04000002 )
{
	root = new NifItem();
}

NifModel::~NifModel()
{
	delete root;
}

void NifModel::clear()
{
	root->killChildren();
	version = 0;
	version_string.clear();
	reset();
}


/*
 *  extra logic
 */

void NifModel::updateHeader()
{
	QModelIndex header = index( 0, 0 );
	if ( ! header.isValid() || itemName( header ) != "NiHeader" )
	{
		qDebug( "header not found" );
		return;
	}
	
	setValue( header, "version", version );
	setValue( header, "num blocks", getBlockCount() );
	
	QModelIndex idxBlockTypes = getIndex( header, "block types" );
	QModelIndex idxBlockTypeIndices = getIndex( header, "block type index" );
	if ( idxBlockTypes.isValid() && idxBlockTypeIndices.isValid() )
	{
		QStringList blocktypes;
		QList<int>	blocktypeindices;
		for ( int r = 0; r < rowCount(); r++ )
		{
			QModelIndex block = index( r, 0 );
			if ( itemType( block ) == "NiBlock" )
			{
				QString name = itemName( block );
				if ( ! blocktypes.contains( name ) )
					blocktypes.append( name );
				blocktypeindices.append( blocktypes.indexOf( name ) );
			}
		}
		setValue( header, "num block types", blocktypes.count() );
		
		updateArray( idxBlockTypes );
		updateArray( idxBlockTypeIndices );
		
		for ( int r = 0; r < rowCount( idxBlockTypes ); r++ )
			setItemValue( idxBlockTypes.child( r, 0 ), blocktypes.value( r ) );
		
		for ( int r = 0; r < rowCount( idxBlockTypeIndices ); r++ )
			setItemValue( idxBlockTypeIndices.child( r, 0 ), blocktypeindices.value( r ) );
	}
}

void NifModel::updateArray( const QModelIndex & array, bool fast )
{
	QModelIndex parent = array.parent();
	if ( ! parent.isValid() )
		return;
		
	QString	dim1 = itemArr1( array );
	if ( dim1.isEmpty() )
		return;

	QString name = itemName( array );

	//qDebug( "update array %s[%s]", str( name ), str( dim1 ) );
	int d1 = 0;
	QModelIndex dim1idx = getIndex( parent, dim1 );
	if ( ! dim1idx.isValid() || rowCount( dim1idx ) == 0 )
		d1 = getInt( parent, dim1 );
	else
	{
		// extract array index from name
		int x = name.indexOf( '[' );
		int y = name.indexOf( ']' );
		if ( x >= 0 && y >= 0 && x < y )
		{
			//qDebug() << "extracted array index is " << name.mid( x+1, y-x-1 );
			int z = name.mid( x+1, y-x-1 ).toInt();
			d1 = itemValue( index( z, 0, dim1idx ) ).toInt();
		}
		else
		{
			qCritical() << "failed to get array size for array " << name;
			return;
		}
	}
	if ( d1 > 1024*1024 )	qWarning( "maximum array size exceeded" );
	
	int rows = rowCount( array );
	if ( d1 > rows )
	{
		QString type = itemType( array );
		QString dim2 = itemArr2( array );
		QString arg  = itemArg( array );
		if ( ! dim2.isEmpty() )	dim2 = QString( "(%1)" ).arg( dim2 );
		if ( ! arg.isEmpty() )		arg  = QString( "(%1)" ).arg( arg );
		
		NifData data( name, type, getTypeValue( type ), arg, dim2, QString(), QString(), 0, 0 );
		if ( ! fast )
			beginInsertRows( array, rows, d1-1 );
		for ( int c = rows; c < d1; c++ )
		{
			data.name = QString( "%1[%2]" ).arg( name ).arg( c );
			insertType( data, array );
		}
		if ( ! fast )
			endInsertRows();
	}
	if ( d1 < rows && d1 >= 0 )
	{
		NifItem * arrayItem = static_cast<NifItem*>( array.internalPointer() );
		if ( ! fast )
			beginRemoveRows( array, d1, rows-1 );
		while ( arrayItem && d1 < arrayItem->childCount() )
			arrayItem->removeChild( d1 );
		if ( ! fast )
			endRemoveRows();
	}
}

/*
 *  block functions
 */

void NifModel::insertNiBlock( const QString & identifier, int at, bool fast )
{
	NifBlock * block = blocks.value( identifier );
	if ( block )
	{
		if ( at < 0 || at > getBlockCount() )
			at = -1;
		if ( at >= 0 )
			root->adjustLinks( this, at, 1 );
		
		if ( at >= 0 ) at++;
		
		if ( ! fast ) beginInsertRows( QModelIndex(), ( at >= 0 ? at : rowCount() ), ( at >= 0 ? at : rowCount() ) );
		QModelIndex idy = insertBranch( NifData( identifier, "NiBlock" ), QModelIndex(), at );
		if ( ! fast ) endInsertRows();
		foreach ( QString a, block->ancestors )
			inherit( a, idy );
		foreach ( NifData data, block->types )
			insertType( data, idy );
	}
	else
	{
		qCritical() << "unknown block " << identifier;
	}
}

void NifModel::removeNiBlock( int blocknum )
{
	root->adjustLinks( this, blocknum, 0 );
	root->adjustLinks( this, blocknum, -1 );
	root->removeChild( blocknum + 1 );
	reset();
}

int NifModel::getBlockNumber( const QModelIndex & idx ) const
{
	if ( ! ( idx.isValid() && idx.model() == this ) )
		return -1;
	
	const NifItem * block = static_cast<NifItem*>( idx.internalPointer() );
	while ( block && block->parent() != root )
		block = block->parent();
	
	if ( block )
		return block->row() - 1;
	else
		return -1;
}

QModelIndex NifModel::getBlock( int x, const QString & name ) const
{
	x += 1; //the first block is the NiHeader
	QModelIndex idx = index( x, 0 );
	if ( ! name.isEmpty() )
		return ( itemName( idx ) == name ? idx : QModelIndex() );
	else
		return idx;
}

int NifModel::getBlockCount() const
{
	return rowCount() - 1;
}


/*
 *  ancestor functions
 */

void NifModel::inherit( const QString & identifier, const QModelIndex & idx, int at )
{
	NifBlock * ancestor = ancestors.value( identifier );
	if ( ancestor )
	{
		QModelIndex idy = insertBranch( NifData( identifier ), idx, at );
		foreach ( QString a, ancestor->ancestors )
			inherit( a, idy );
		foreach ( NifData data, ancestor->types )
			insertType( data, idy );
	}
	else
	{
		qCritical() << "unknown ancestor " << identifier;
	}
}


/*
 *  basic and compound type functions
 */

void NifModel::insertType( const NifData & data, const QModelIndex & idx, int )
{
	if ( ! data.arr1.isEmpty() )
	{	// recursive array construction
		QModelIndex idy = insertBranch( data, idx );
		
		if ( evalCondition( idy ) )
		{
			updateArray( idy, true );
		}
		return;
	}

	NifBlock * compound = compounds.value( data.type );
	if ( compound )
	{
		QModelIndex idy = insertBranch( data, idx );
		QString arg = "(" + data.arg + ")";
		foreach ( NifData d, compound->types )
		{
			if ( d.arr1.contains( "(arg)" ) ) d.arr1 = d.arr1.replace( d.arr1.indexOf( "(arg)" ), 5, arg );
			if ( d.arr2.contains( "(arg)" ) ) d.arr2 = d.arr2.replace( d.arr2.indexOf( "(arg)" ), 5, arg );
			if ( d.cond.contains( "(arg)" ) ) d.cond = d.cond.replace( d.cond.indexOf( "(arg)" ), 5, arg );
			insertType( d, idy );
		}
	}
	else if ( types.contains( data.type ) )
		insertLeaf( data, idx );
	else
		qCritical() << "insertType() : unknown type " << data.type;
}


/*
 *  item value functions
 */

QString NifModel::itemName( const QModelIndex & index ) const
{
	NifItem * item = static_cast<NifItem*>( index.internalPointer() );
	if ( ! ( index.isValid() && item ) )	return QString();
	return item->name();
}

QString NifModel::itemType( const QModelIndex & index ) const
{
	NifItem * item = static_cast<NifItem*>( index.internalPointer() );
	if ( ! ( index.isValid() && item ) )	return QString();
	return item->type();
}

QVariant NifModel::itemValue( const QModelIndex & index ) const
{
	NifItem * item = static_cast<NifItem*>( index.internalPointer() );
	if ( ! ( index.isValid() && item ) )	return QVariant();
	return item->value();
}

QString NifModel::itemArg( const QModelIndex & index ) const
{
	NifItem * item = static_cast<NifItem*>( index.internalPointer() );
	if ( ! ( index.isValid() && item ) )	return QString();
	return item->arg();
}

QString NifModel::itemArr1( const QModelIndex & index ) const
{
	NifItem * item = static_cast<NifItem*>( index.internalPointer() );
	if ( ! ( index.isValid() && item ) )	return QString();
	return item->arr1();
}

QString NifModel::itemArr2( const QModelIndex & index ) const
{
	NifItem * item = static_cast<NifItem*>( index.internalPointer() );
	if ( ! ( index.isValid() && item ) )	return QString();
	return item->arr2();
}

QString NifModel::itemCond( const QModelIndex & index ) const
{
	NifItem * item = static_cast<NifItem*>( index.internalPointer() );
	if ( ! ( index.isValid() && item ) )	return QString();
	return item->cond();
}

quint32 NifModel::itemVer1( const QModelIndex & index ) const
{
	NifItem * item = static_cast<NifItem*>( index.internalPointer() );
	if ( ! ( index.isValid() && item ) )	return 0;
	return item->ver1();
}

quint32 NifModel::itemVer2( const QModelIndex & index ) const
{
	NifItem * item = static_cast<NifItem*>( index.internalPointer() );
	if ( ! ( index.isValid() && item ) )	return 0;
	return item->ver2();
}

void NifModel::setItemValue( const QModelIndex & index, const QVariant & var )
{
	NifItem * item = static_cast<NifItem*>( index.internalPointer() );
	if ( ! ( index.isValid() && item ) )	return;
	item->setValue( var );
}

QVariant NifModel::getValue( const QModelIndex & parent, const QString & name ) const
{
	if ( ! parent.isValid() )
	{
		qWarning( "getValue( %s ) reached top level", str( name ) );
		return QVariant();
	}
	
	//if ( name == "num uv sets" )
	//	qDebug( "getValue( %s, %s )", str( itemName( parent ) ), str( name ) );
	if ( name.startsWith( "(" ) && name.endsWith( ")" ) )
		return getValue( parent.parent(), name.mid( 1, name.length() - 2 ).trimmed() );
	
	for ( int c = 0; c < rowCount( parent ); c++ )
	{
		QModelIndex child = parent.child( c, 0 );
		//if ( name == "num uv sets" )
		//	qDebug( "%i, %s, %i", c, str( itemName( child ) ), itemValue( child ).toInt() );
		if ( itemName( child ) == name && evalCondition( child ) )
			return itemValue( child );
		if ( rowCount( child ) > 0 && itemType( child ).isEmpty() )
		{
			QVariant v = getValue( child, name );
			if ( v.isValid() ) return v;
		}
	}
	
	return QVariant();
}

int NifModel::getInt( const QModelIndex & parent, const QString & nameornumber ) const
{
	//qDebug( "getInt( %s, %s )", str( itemName( parent ) ), str( nameornumber ) );

	if ( nameornumber.isEmpty() )
		return 0;
	
	QModelIndex idx = parent;
	QString n = nameornumber;
	while ( n.startsWith( "(" ) && n.endsWith( ")" ) )
	{
		n = n.mid( 1, n.length() - 2 ).trimmed();
		idx = idx.parent();
	}
	
	bool ok;
	int i = n.toInt( &ok );
	if ( ok )	return i;
	
	QVariant v = getValue( idx, n );
	if ( ! v.canConvert( QVariant::Int ) )
		qWarning( "failed to get int for %s ('%s','%s')", str( nameornumber ), str( v.toString() ), v.typeName() );

	//qDebug( "getInt( %s, %s ) -> %i", str( itemName( parent ) ), str( nameornumber ), v.toInt() );
	return v.toInt();
}

float NifModel::getFloat( const QModelIndex & parent, const QString & nameornumber ) const
{
	if ( nameornumber.isEmpty() )
		return 0;
	
	bool ok;
	double d = nameornumber.toDouble( &ok );
	if ( ok )	return d;
	
	QVariant v = getValue( parent, nameornumber );
	if ( ! v.canConvert( QVariant::Double ) )
		qWarning( "failed to get float value for %s ('%s','%s')", str( nameornumber ), str( v.toString() ), v.typeName() );
	return v.toDouble();
}

bool NifModel::setValue( const QModelIndex & parent, const QString & name, const QVariant & var )
{
	if ( ! parent.isValid() )
	{
		qWarning( "setValue( %s ) reached top level", str( name ) );
		return false;
	}
	
	if ( name.startsWith( "(" ) && name.endsWith( ")" ) )
		return setValue( parent.parent(), name.mid( 1, name.length() - 2 ).trimmed(), var );
	
	for ( int c = 0; c < rowCount( parent ); c++ )
	{
		QModelIndex child = parent.child( c, 0 );
		if ( itemName( child ) == name && evalCondition( child ) )
		{
			setItemValue( child, var );
			return true;
		}
		if ( rowCount( child ) > 0 && itemType( child ).isEmpty() )
		{
			if ( setValue( child, name, var ) ) return true;
		}
	}
	
	return false;
}


/*
 *  version conversion
 */

QString NifModel::version2string( quint32 v )
{
	if ( v == 0 )	return QString();
	QString s = QString::number( ( v >> 24 ) & 0xff, 10 ) + "."
		+ QString::number( ( v >> 16 ) & 0xff, 10 ) + "."
		+ QString::number( ( v >> 8 ) & 0xff, 10 ) + "."
		+ QString::number( v & 0xff, 10 );
	return s;
}	

quint32 NifModel::version2number( const QString & s )
{
	if ( s.isEmpty() )	return 0;
	QStringList l = s.split( "." );
	if ( l.count() != 4 )
	{
		bool ok;
		quint32 i = s.toUInt( &ok );
		if ( ! ok )
			qDebug() << "version2number( " << s << " ) : conversion failed";
		return ( i == 0xffffffff ? 0 : i );
	}
	quint32 v = 0;
	for ( int i = 0; i < 4; i++ )
		v += l[i].toInt( 0, 10 ) << ( (3-i) * 8 );
	return v;
}


/*
 *  QAbstractModel interface
 */

QModelIndex NifModel::index( int row, int column, const QModelIndex & parent ) const
{
	NifItem * parentItem;
	
	if ( ! parent.isValid() )
		parentItem = root;
	else
		parentItem = static_cast<NifItem*>( parent.internalPointer() );
	
	NifItem * childItem = ( parentItem ? parentItem->child( row ) : 0 );
	if ( childItem )
		return createIndex( row, column, childItem );
	else
		return QModelIndex();
}

QModelIndex NifModel::parent( const QModelIndex & child ) const
{
	if ( ! child.isValid() )
		return QModelIndex();
	
	NifItem *childItem = static_cast<NifItem*>( child.internalPointer() );
	NifItem *parentItem = childItem->parent();
	
	if ( parentItem == root || ! parentItem )
		return QModelIndex();
	
	return createIndex( parentItem->row(), 0, parentItem );
}

int NifModel::rowCount( const QModelIndex & parent ) const
{
	NifItem * parentItem;
	
	if ( ! parent.isValid() )
		parentItem = root;
	else
		parentItem = static_cast<NifItem*>( parent.internalPointer() );
	
	return ( parentItem ? parentItem->childCount() : 0 );
}

QVariant NifModel::data( const QModelIndex & index, int role ) const
{
	NifItem * item = static_cast<NifItem*>( index.internalPointer() );
	if ( ! ( index.isValid() && item ) )
		return QVariant();

	int column = index.column();
	
	switch ( role )
	{
		case Qt::DisplayRole:
		{
			switch ( column )
			{
				case NameCol:	return item->name();
				case TypeCol:	return item->type();
				case ValueCol:
				{
					QString displayHint = getDisplayHint( item->type() );
					if ( displayHint == "float" )
						return QString::number( item->value().toDouble(), 'f', 4 );
					else if ( displayHint == "dec" )
						return QString::number( item->value().toInt(), 10 );
					else if ( displayHint == "bool" )
						return ( item->value().toInt() != 0 ? "yes" : "no" );
					else if ( displayHint == "hex" )
						return QString::number( item->value().toInt(), 16 ).prepend( "0x" );
					else if ( displayHint == "bin" )
						return QString::number( item->value().toInt(), 2 ).prepend( "0b" );
					else if ( displayHint == "link" )
						if ( item->value().toInt() == -1 )
							return QString( "none" );						
					return item->value();
				}
				case ArgCol:	return item->arg();
				case Arr1Col:	return item->arr1();
				case Arr2Col:	return item->arr2();
				case CondCol:	return item->cond();
				case Ver1Col:	return version2string( item->ver1() );
				case Ver2Col:	return version2string( item->ver2() );
				default:		return QVariant();
			}
		}
		case Qt::DecorationRole:
		{
			switch ( column )
			{
				case NameCol:
					if ( itemType( index ) == "NiBlock" )
						return QString::number( getBlockNumber( index ) );
				default:
					return QVariant();
			}
		}
		case Qt::EditRole:
		{
			switch ( column )
			{
				case NameCol:	return item->name();
				case TypeCol:	return item->type();
				case ValueCol:	return item->value();
				case ArgCol:	return item->arg();
				case Arr1Col:	return item->arr1();
				case Arr2Col:	return item->arr2();
				case CondCol:	return item->cond();
				case Ver1Col:	return version2string( item->ver1() );
				case Ver2Col:	return version2string( item->ver2() );
				default:		return QVariant();
			}
		}
		case Qt::ToolTipRole:
		{
			QString tip;
			if ( column == TypeCol )
			{
				tip = getTypeDescription( item->type() );
				if ( ! tip.isEmpty() ) return tip;
			}
			return QVariant();
		}
		case Qt::BackgroundColorRole:
		{
			if ( column == ValueCol && ( getDisplayHint( item->type() ) == "color" ) )
				return qvariant_cast<QColor>( item->value() );
			else
				return QVariant();
		}
		default:
			return QVariant();
	}
}

bool NifModel::setData( const QModelIndex & index, const QVariant & value, int role )
{
	NifItem * item = static_cast<NifItem*>( index.internalPointer() );
	
	if ( ! ( index.isValid() && item && role == Qt::EditRole ) )
		return false;
	
	switch ( index.column() )
	{
		case NifModel::NameCol:
			item->setName( value.toString() );
			break;
		case NifModel::TypeCol:
			item->setType( value.toString() );
			break;
		case NifModel::ValueCol:
			item->setValue( value );
			break;
		case NifModel::ArgCol:
			item->setArg( value.toString() );
			break;
		case NifModel::Arr1Col:
			item->setArr1( value.toString() );
			break;
		case NifModel::Arr2Col:
			item->setArr2( value.toString() );
			break;
		case NifModel::CondCol:
			item->setCond( value.toString() );
			break;
		case NifModel::Ver1Col:
			item->setVer1( NifModel::version2number( value.toString() ) );
			break;
		case NifModel::Ver2Col:
			item->setVer2( NifModel::version2number( value.toString() ) );
			break;
		default:
			return false;
	}
	emit dataChanged( index, index );
	return true;
}

QVariant NifModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
	if ( role != Qt::DisplayRole )
		return QVariant();
	switch ( role )
	{
		case Qt::DisplayRole:
			switch ( section )
			{
				case NameCol:		return "Name";
				case TypeCol:		return "Type";
				case ValueCol:		return "Value";
				case ArgCol:		return "Argument";
				case Arr1Col:		return "Array1";
				case Arr2Col:		return "Array2";
				case CondCol:		return "Condition";
				case Ver1Col:		return "since";
				case Ver2Col:		return "until";
				default:			return QVariant();
			}
		default:
			return QVariant();
	}
}


/*
 *  load and save
 */

bool NifModel::load( QDataStream & stream )
{
	// reset model
	clear();

	qDebug( "size %i", (int) stream.device()->size() );
	
	{	// read magic version string
		int c = 0;
		char chr = 0;
		while ( c < 80 && chr != '\n' )
		{
			stream.readRawData( & chr, 1 );
			version_string.append( chr );
		}
	}
	
	// verify magic id
	qDebug() << version_string;
	if ( ! ( version_string.startsWith( "NetImmerse File Format" ) || version_string.startsWith( "Gamebryo" ) ) )
	{
		qCritical( "this is not a NIF" );
		return false;
	}
	
	// read version number
	stream.readRawData( (char *) &version, 4 );
	qDebug( "version %08X", version );
	stream.device()->seek( stream.device()->pos() - 4 );

	// read header
	insertType( NifData( "NiHeader", "header" ), QModelIndex() );
	load( index( 0, 0, QModelIndex() ), stream );
	
	QModelIndex header = index( 0, 0 );
	
	int numblocks = getInt( header, "num blocks" );
	qDebug( "numblocks %i", numblocks );
	
	// read in the NiBlocks
	for ( int c = 0; c < numblocks; c++ )
	{
		QString blktyp;
		
		if ( version > 0x0a000000 )
		{
			if ( version < 0x0a020000 )		stream.device()->read( 4 );
			
			int blktypidx = itemValue( index( c, 0, getIndex( header, "block type index" ) ) ).toInt();
			blktyp = itemValue( index( blktypidx, 0, getIndex( header, "block types" ) ) ).toString();
		}
		else
		{
			int len;
			stream.readRawData( (char *) &len, 4 );
			if ( len < 2 || len > 80 )
			{
				qCritical( "next block starts not with a NiString" );
				return false;
			}
			blktyp = stream.device()->read( len );
		}
		
		if ( isNiBlock( blktyp ) )
		{
			qDebug() << "insert block " << c << " : " << blktyp;
			insertNiBlock( blktyp, -1, true );
			if ( ! load( getBlock( c ), stream ) ) 
			{
				qCritical() << "failed to load block " << c << " " << blktyp;
				return false;
			}
		}
		else
		{
			qCritical() << "encountered unknown block '" << blktyp << "'";
			return false;
		}
	}
	reset(); // notify model views that a significant change to the data structure has occurded
	return true;
}

bool NifModel::save( QDataStream & stream )
{
	// write magic version string
	stream.device()->write( version_string );
	
	for ( int c = 0; c < rowCount( QModelIndex() ); c++ )
	{
		qDebug() << "saving block " << itemName( index( c, 0 ) ) << "(" << c << ")";
		if ( itemType( index( c, 0 ) ) == "NiBlock" )
		{
			if ( version > 0x0a000000 )
			{
				if ( version < 0x0a020000 )	
				{
					int null = 0;
					stream.writeRawData( (char *) &null, 4 );
				}
			}
			else
			{
				QString string = itemName( index( c, 0 ) );
				int len = string.length();
				stream.writeRawData( (char *) &len, 4 );
				stream.writeRawData( (const char *) string.toAscii(), len );
			}
		}
		save( index( c, 0, QModelIndex() ), stream );
	}
	int foot1 = 1;
	stream.writeRawData( (char *) &foot1, 4 );
	int foot2 = 0;
	stream.writeRawData( (char *) &foot2, 4 );
	return true;
}

bool NifModel::load( const QModelIndex & parent, QDataStream & stream )
{
	if ( ! parent.isValid() ) return false;
	
	int numrows = rowCount( parent );
	//qDebug( "loading branch %s (%i)",str( data( parent.sibling( parent.row(), NameCol ) ).toString() ), numrows );
	for ( int row = 0; row < numrows; row++ )
	{
		QModelIndex child = parent.child( row, 0 );
		NifItem * item = static_cast<NifItem*>( child.internalPointer() );
		if ( item && evalCondition( child ) )
		{
			QString name = item->name();
			int		type = getInternalType( item->type() );
			QString	dim1 = item->arr1();
			QString dim2 = item->arr2();
			QString arg  = item->arg();
			
			if ( item->type().isEmpty() )
			{
				load( child, stream );
			}
			else if ( ! dim1.isEmpty() )
			{
				updateArray( child, true );
				load( child, stream );
			}
			else if ( rowCount( child ) > 0 )
			{
				load( child, stream );
			}
			else switch ( type )
			{
				case it_uint8:
				{
					quint8 u8;
					stream.readRawData( (char *) &u8, 1 );
					item->setValue( u8 );
				} break;
				case it_uint16:
				{
					quint16 u16;
					stream.readRawData( (char *) &u16, 2 );
					item->setValue( u16 );
				} break;
				case it_uint32:
				{
					quint32 u32;
					stream.readRawData( (char *) &u32, 4 );
					item->setValue( u32 );
				} break;
				case it_int8:
				{
					qint8 s8;
					stream.readRawData( (char *) &s8, 1 );
					item->setValue( s8 );
				} break;
				case it_int16:
				{
					qint16 s16;
					stream.readRawData( (char *) &s16, 2 );
					item->setValue( s16 );
				} break;
				case it_int32:
				{
					qint32 s32;
					stream.readRawData( (char *) &s32, 4 );
					item->setValue( s32 );
				} break;
				case it_float:
				{
					float f32;
					stream.readRawData( (char *) &f32, 4 );
					item->setValue( f32 );
				} break;
				case it_color3f:
				{
					float r, g, b;
					stream.readRawData( (char *) &r, 4 );
					stream.readRawData( (char *) &g, 4 );
					stream.readRawData( (char *) &b, 4 );
					item->setValue( QColor::fromRgbF( r, g, b ) );
				} break;
				case it_color4f:
				{
					float r, g, b, a;
					stream.readRawData( (char *) &r, 4 );
					stream.readRawData( (char *) &g, 4 );
					stream.readRawData( (char *) &b, 4 );
					stream.readRawData( (char *) &a, 4 );
					item->setValue( QColor::fromRgbF( r, g, b, a ) );
				} break;
				case it_string:
				{
					int len;
					stream.readRawData( (char *) &len, 4 );
					if ( len > 4096 )
						qWarning( "maximum string length exceeded" );
					QByteArray string = stream.device()->read( len );
					item->setValue( QString( string ) );
				} break;
				default:
				{
					qCritical() << "encountered unknown type " << type << " during load of " << itemName( child ) << "(" << itemType( child ) << ")";
					return false;
				}
			}
		}
	}
	return true;
}

void NifModel::save( const QModelIndex & parent, QDataStream & stream )
{
	int numrows = rowCount( parent );
	//qDebug( "save branch %s (%i)",str( data( parent.sibling( parent.row(), NameCol ) ).toString() ), numrows );
	for ( int row = 0; row < numrows; row++ )
	{
		QModelIndex child = index( row, 0, parent );
		if ( evalCondition( child ) )
		{
			int		 type  = getInternalType( itemType( child ) );
			QVariant value = itemValue( child );
			QString  dim1  = itemArr1( child );
			QString  dim2  = itemArr2( child );
			
			if ( ! dim1.isEmpty() || ! dim2.isEmpty() || rowCount( child ) > 0 || itemType( child ).isEmpty() )
			{
				save( child, stream );
			}
			else switch ( type )
			{
				case it_uint8:
				{
					quint8 u8 = (quint8) value.toInt();
					stream.writeRawData( (char *) &u8, 1 );
				} break;
				case it_uint16:
				{
					quint16 u16 = (quint16) value.toInt();
					stream.writeRawData( (char *) &u16, 2 );
				} break;
				case it_uint32:
				{
					quint32 u32 = (quint32) value.toInt();
					stream.writeRawData( (char *) &u32, 4 );
				} break;
				case it_int8:
				{
					qint8 s8 = (qint8) value.toInt();
					stream.writeRawData( (char *) &s8, 4 );
				} break;
				case it_int16:
				{
					qint16 s16 = (qint16) value.toInt();
					stream.writeRawData( (char *) &s16, 4 );
				} break;
				case it_int32:
				{
					qint32 s32 = (qint32) value.toInt();
					stream.writeRawData( (char *) &s32, 4 );
				} break;
				case it_float:
				{
					float f32 = value.toDouble();
					stream.writeRawData( (char *) &f32, 4 );
				} break;
				case it_color3f:
				{
					QColor rgb = value.value<QColor>();
					float r = rgb.redF();					
					float g = rgb.greenF();					
					float b = rgb.blueF();					
					stream.writeRawData( (char *) &r, 4 );
					stream.writeRawData( (char *) &g, 4 );
					stream.writeRawData( (char *) &b, 4 );
				} break;
				case it_color4f:
				{
					QColor rgba = value.value<QColor>();
					float r = rgba.redF();					
					float g = rgba.greenF();					
					float b = rgba.blueF();					
					float a = rgba.alphaF();
					stream.writeRawData( (char *) &r, 4 );
					stream.writeRawData( (char *) &g, 4 );
					stream.writeRawData( (char *) &b, 4 );
					stream.writeRawData( (char *) &a, 4 );
				} break;
				case it_string:
				{
					QString string = value.toString();
					int len = string.length();
					stream.writeRawData( (char *) &len, 4 );
					stream.writeRawData( (const char *) string.toAscii(), len );
				} break;
				default:
				{
					qCritical() << "encountered unknown type during save of " << itemName( child ) << "(" << itemType( child ) << ")";
				}
			}
		}
	}
}

QModelIndex NifModel::getIndex( const QModelIndex & parent, const QString & name ) const
{
	if ( ! parent.isValid() || parent.model() != this )
		return QModelIndex();
	if ( name.startsWith( "(" ) && name.endsWith( ")" ) )
		return getIndex( parent.parent(), name.mid( 1, name.length() - 2 ).trimmed() );
	
	for ( int c = 0; c < rowCount( parent ); c++ )
	{
		QModelIndex child = parent.child( c, 0 );
		if ( itemName( child ) == name && evalCondition( child ) )
			return child;
		if ( rowCount( child ) > 0 && itemType( child ).isEmpty() )
		{
			QModelIndex i = getIndex( child, name );
			if ( i.isValid() ) return i;
		}
	}
	return QModelIndex();
}

bool NifModel::evalCondition( const QModelIndex & idx, bool chkParents ) const
{
	if ( chkParents && idx.parent().isValid() )
	{
		if ( ! evalCondition( idx.parent(), true ) )	
			return false;
	}
	quint32 v1 = itemVer1( idx );
	quint32 v2 = itemVer2( idx );
	
	bool vchk = ( v1 != 0 ? version >= v1 : true ) && ( v2 != 0 ? version <= v2 : true );
	
	//qDebug( "evalVersion( %08X <= %08X <= %08X ) -> %i", v1, version, v2, vchk );

	if ( ! vchk )
		return false;
	
	QString cond = itemCond( idx );
	
	if ( cond.isEmpty() )
		return true;
		
	//qDebug( "evalCondition( '%s' )", str( cond ) );
	
	QString left, right;
	
	static const char * exp[] = { "!=", "==" };
	static const int num_exp = 2;
	
	int c;
	for ( c = 0; c < num_exp; c++ )
	{
		int p = cond.indexOf( exp[c] );
		if ( p > 0 )
		{
			left = cond.left( p ).trimmed();
			right = cond.right( cond.length() - p - 2 ).trimmed();
			break;
		}
	}
	
	if ( c >= num_exp )
	{
		qCritical( "could not eval condition '%s'", str( cond ) );
		return false;
	}
	
	int l = getInt( idx.parent(), left );
	int r = getInt( idx.parent(), right );
	
	if ( c == 0 )
	{
		//qDebug( "evalCondition '%s' (%i) != '%s' (%i) : %i", str( left ), l, str( right ), r, l != r );
		return l != r;
	}
	else
	{
		//qDebug( "evalCondition '%s' (%i) == '%s' (%i) : %i", str( left ), l, str( right ), r, l == r );
		return l == r;
	}
}

NifBasicType * NifModel::getType( const QString & name ) const
{
	int c = types.count( name );
	if ( c == 1 )
		return types.value( name );
	else if ( c > 1 )
	{
		QList<NifBasicType*> tlst = types.values( name );
		foreach ( NifBasicType * typ, tlst )
		{
			if ( ( typ->ver1 == 0 || version >= typ->ver1 ) && ( typ->ver2 == 0 || version <= typ->ver2 ) )
				return typ;
		}
	}
	
	return 0;
}

QModelIndex NifModel::insertBranch( NifData data, const QModelIndex & parent, int at )
{
	NifItem * parentItem = static_cast<NifItem*>( parent.internalPointer() );
	if ( ! parent.isValid() || ! parentItem )
		parentItem = root;
	
	data.value = QVariant();
	
	NifItem * item = parentItem->insertChild( data, at );

	return createIndex( item->row(), 0, item );
}

void NifModel::insertLeaf( const NifData & data, const QModelIndex & parent, int at )
{
	NifItem * parentItem = static_cast<NifItem*>( parent.internalPointer() );
	if ( ! parent.isValid() || ! parentItem )
		parentItem = root;
	
	parentItem->insertChild( data, at );
}

