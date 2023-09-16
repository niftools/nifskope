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

#include "basemodel.h"

#include "xml/xmlconfig.h"

#include <QByteArray>
#include <QColor>
#include <QBuffer>
#include <QFile>
#include <QFileInfo>
#include <QTime>


//! @file basemodel.cpp Abstract base class for NIF data models

/*
 *  BaseModel
 */

BaseModel::BaseModel( QObject * p ) : QAbstractItemModel( p )
{
	root = new NifItem( this, nullptr );
	root->setIsConditionless( true );
	parentWindow = qobject_cast<QWidget *>(p);
	msgMode = MSG_TEST;
}

BaseModel::~BaseModel()
{
	delete root;
}

void BaseModel::setMessageMode( MsgMode mode )
{
	msgMode = mode;
}

void BaseModel::logMessage( const QString & message, const QString & details, QMessageBox::Icon lvl ) const
{
	if ( msgMode == MSG_USER ) {
		Message::append( nullptr, message, details, lvl );
	} else {
		testMsg( details );
	}
}

void BaseModel::logWarning( const QString & details ) const
{
	logMessage(tr("Warnings were generated while reading the file."), details);
}

void BaseModel::testMsg( const QString & m ) const
{
	messages.append( TestMessage() << m );
}

inline NifItem * indexToItem( const QModelIndex & index )
{
	return static_cast<NifItem *>( index.internalPointer() );
}

void BaseModel::beginInsertRows( const QModelIndex & parent, int first, int last )
{
	setState( Inserting );
	QAbstractItemModel::beginInsertRows( parent, first, last );
}

void BaseModel::endInsertRows()
{
	QAbstractItemModel::endInsertRows();
	restoreState();
}

void BaseModel::beginRemoveRows( const QModelIndex & parent, int first, int last )
{
	setState( Removing );
	QAbstractItemModel::beginRemoveRows( parent, first, last );
}

void BaseModel::endRemoveRows()
{
	QAbstractItemModel::endRemoveRows();
	restoreState();
}

bool BaseModel::getProcessingResult()
{
	bool result = changedWhileProcessing;

	changedWhileProcessing = false;

	return result;
}

QList<TestMessage> BaseModel::getMessages() const
{
	QList<TestMessage> lst = messages;
	messages.clear();
	return lst;
}

/*
 *  array functions
 */

int BaseModel::evalArraySize( const NifItem * array ) const
{
	// shortcut for speed
	if ( !isArrayEx(array) ) {
		if ( array )
			reportError( array, __func__, "The input item is not an array." );
		return 0;
	}
	
	if ( array == root ) {
		reportError( array, __func__, "The input item is the root." );
		return 0;
	}

	BaseModelEval functor(this, array);
	return array->arr1expr().evaluateUInt(functor);
}

/*
 *  item value functions
 */

QString BaseModel::itemName( const QModelIndex & index ) const
{
	const NifItem * item = getItem( index );
	if ( item )
		return item->name();
	return QString();
}

QString BaseModel::itemStrType( const QModelIndex & index ) const
{
	const NifItem * item = getItem( index );
	if ( item )
		return item->strType();
	return QString();
}

QString BaseModel::itemTempl( const QModelIndex & index ) const
{
	const NifItem * item = getItem( index );
	if ( item )
		return item->templ();
	return QString();
}

NifValue BaseModel::getValue( const QModelIndex & index ) const
{
	const NifItem * item = getItem( index );
	if ( item )
		return item->value();
	return NifValue();
}

QString BaseModel::itemArg( const QModelIndex & index ) const
{
	const NifItem * item = getItem( index );
	if ( item )
		return item->arg();
	return QString();
}

QString BaseModel::itemArr1( const QModelIndex & index ) const
{
	const NifItem * item = getItem( index );
	if ( item )
		return item->arr1();
	return QString();
}

QString BaseModel::itemArr2( const QModelIndex & index ) const
{
	const NifItem * item = getItem( index );
	if ( item )
		return item->arr2();
	return QString();
}

QString BaseModel::itemCond( const QModelIndex & index ) const
{
	const NifItem * item = getItem( index );
	if ( item )
		return item->cond();
	return QString();
}

quint32 BaseModel::itemVer1( const QModelIndex & index ) const
{
	const NifItem * item = getItem( index );
	return item ? item->ver1() : 0;
}

quint32 BaseModel::itemVer2( const QModelIndex & index ) const
{
	const NifItem * item = getItem( index );
	return item ? item->ver2() : 0;
}

QString BaseModel::itemText( const QModelIndex & index ) const
{
	const NifItem * item = getItem( index );
	if ( item )
		return item->text();
	return QString();
}

bool BaseModel::setItemValue( NifItem * item, const NifValue & val )
{
	if ( !item )
		return false;

	item->value() = val;
	onItemValueChange( item );
	return true;
}


/*
 *  QAbstractModel interface
 */

QModelIndex BaseModel::index( int row, int column, const QModelIndex & parent ) const
{
	const NifItem * parentItem = parent.isValid() ? getItem(parent) : root;
	return itemToIndex( parentItem ? parentItem->child(row) : nullptr, column );
}

QModelIndex BaseModel::parent( const QModelIndex & child ) const
{
	const NifItem * childItem = getItem( child );
	if ( !childItem || childItem == root )
		return QModelIndex();

	const NifItem * parentItem = childItem->parent();
	if ( !parentItem || parentItem == root ) // Yes, we ignore parentItem == root because otherwise "Block Details" shows the whole hierarchy
		return QModelIndex();

	return itemToIndex( parentItem );
}

int BaseModel::rowCount( const QModelIndex & parent ) const
{
	if ( !parent.isValid() )
		return root->childCount();
	const NifItem * parentItem = getItem(parent);
	return parentItem ? parentItem->childCount() : 0;
}

QVariant BaseModel::data( const QModelIndex & index, int role ) const
{
	const NifItem * item = getItem( index );
	if ( !item )
		return QVariant();

	int column = index.column();

	if ( role == NifSkopeDisplayRole )
		role = Qt::DisplayRole;

	switch ( role ) {
	case Qt::DisplayRole:
		{
			switch ( column ) {
			case NameCol:
				return item->name();
			case TypeCol:
				return item->strType();
			case ValueCol:
				return item->getValueAsString();
			case ArgCol:
				return item->arg();
			case Arr1Col:
				return item->arr1();
			case Arr2Col:
				return item->arr2();
			case CondCol:
				return item->cond();
			case Ver1Col:
				return ver2str( item->ver1() );
			case Ver2Col:
				return ver2str( item->ver2() );
			case VerCondCol:
				return item->vercond();
			default:
				return QVariant();
			}
		}
	case Qt::EditRole:
		{
			switch ( column ) {
			case NameCol:
				return item->name();
			case TypeCol:
				return item->strType();
			case ValueCol:
				return item->getValueAsVariant();
			case ArgCol:
				return item->arg();
			case Arr1Col:
				return item->arr1();
			case Arr2Col:
				return item->arr2();
			case CondCol:
				return item->cond();
			case Ver1Col:
				return ver2str( item->ver1() );
			case Ver2Col:
				return ver2str( item->ver2() );
			case VerCondCol:
				return item->vercond();
			default:
				return QVariant();
			}
		}
	case Qt::ToolTipRole:
		{
			switch ( column ) {
			case ValueCol:
				{
					switch ( item->valueType() ) {
					case NifValue::tWord:
					case NifValue::tShort:
						{
							quint16 s = item->getCountValue();
							return QString( "dec: %1<br>hex: 0x%2" ).arg( s ).arg( s, 4, 16, QChar( '0' ) );
						}
					case NifValue::tBool:
					case NifValue::tInt:
					case NifValue::tUInt:
					case NifValue::tULittle32:
						{
							quint32 i = item->getCountValue();
							return QString( "dec: %1<br>hex: 0x%2" ).arg( i ).arg( i, 8, 16, QChar( '0' ) );
						}
					case NifValue::tFloat:
					case NifValue::tHfloat:
					case NifValue::tNormbyte:
						{
							float f = item->getFloatValue();
							quint32 i = item->getCountValue();
							return QString( "float: %1<br>data: 0x%2" ).arg( f ).arg( i, 8, 16, QChar( '0' ) );
						}
					case NifValue::tFlags:
						{
							quint16 f = item->getCountValue();
							return QString( "dec: %1<br>hex: 0x%2<br>bin: 0b%3" ).arg( f ).arg( f, 4, 16, QChar( '0' ) ).arg( f, 16, 2, QChar( '0' ) );
						}
					case NifValue::tVector3:
						return item->get<Vector3>().toHtml();
					case NifValue::tUshortVector3:
						return item->get<UshortVector3>().toHtml();
					case NifValue::tHalfVector3:
						return item->get<HalfVector3>().toHtml();
					case NifValue::tByteVector3:
						return item->get<ByteVector3>().toHtml();
					case NifValue::tMatrix:
						return item->get<Matrix>().toHtml();
					case NifValue::tQuat:
					case NifValue::tQuatXYZW:
						return item->get<Quat>().toHtml();
					case NifValue::tColor3:
						{
							Color3 c = item->get<Color3>();
							return QString( "R %1<br>G %2<br>B %3" ).arg( c[0] ).arg( c[1] ).arg( c[2] );
						}
					case NifValue::tByteColor4:
						{
							Color4 c = item->get<ByteColor4>();
							return QString( "R %1<br>G %2<br>B %3<br>A %4" ).arg( c[0] ).arg( c[1] ).arg( c[2] ).arg( c[3] );
						}
					case NifValue::tColor4:
						{
							Color4 c = item->get<Color4>();
							return QString( "R %1<br>G %2<br>B %3<br>A %4" ).arg( c[0] ).arg( c[1] ).arg( c[2] ).arg( c[3] );
						}
					default:
						break;
					}
				}
				break;
			default:
				break;
			}
		}
		return QVariant();
	case Qt::BackgroundColorRole:
		{
			if ( column == ValueCol && item->isColor() ) {
				return item->getColorValue();
			}
		}
		return QVariant();
	default:
		return QVariant();
	}
}

bool BaseModel::setData( const QModelIndex & index, const QVariant & value, int role )
{
	if ( role != Qt::EditRole )
		return false;

	NifItem * item = getItem( index );
	if ( !item )
		return false;

	switch ( index.column() ) {
	case BaseModel::NameCol:
		item->setName( value.toString() );
		break;
	case BaseModel::TypeCol:
		item->setStrType( value.toString() );
		break;
	case BaseModel::ValueCol:
		item->setValueFromVariant( value );
		break;
	case BaseModel::ArgCol:
		item->setArg( value.toString() );
		break;
	case BaseModel::Arr1Col:
		item->setArr1( value.toString() );
		break;
	case BaseModel::Arr2Col:
		item->setArr2( value.toString() );
		break;
	case BaseModel::CondCol:
		item->setCond( value.toString() );
		break;
	case BaseModel::Ver1Col:
		item->setVer1( str2ver( value.toString() ) );
		break;
	case BaseModel::Ver2Col:
		item->setVer2( str2ver( value.toString() ) );
		break;
	case BaseModel::VerCondCol:
		item->setVerCond( value.toString() );
		break;
	default:
		return false;
	}

	if ( state == Default )
		onItemValueChange( item );

	return true;
}

QVariant BaseModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
	Q_UNUSED( orientation );

	if ( role != Qt::DisplayRole )
		return QVariant();

	switch ( role ) {
	case Qt::DisplayRole:

		switch ( section ) {
		case NameCol:
			return tr( "Name" );
		case TypeCol:
			return tr( "Type" );
		case ValueCol:
			return tr( "Value" );
		case ArgCol:
			return tr( "Argument" );
		case Arr1Col:
			return tr( "Array1" );
		case Arr2Col:
			return tr( "Array2" );
		case CondCol:
			return tr( "Condition" );
		case Ver1Col:
			return tr( "since" );
		case Ver2Col:
			return tr( "until" );
		case VerCondCol:
			return tr( "Version Condition" );
		default:
			return QVariant();
		}

	default:
		return QVariant();
	}
}

Qt::ItemFlags BaseModel::flags( const QModelIndex & index ) const
{
	if ( !index.isValid() )
		return Qt::ItemIsEnabled;

	Qt::ItemFlags flags;

	const NifItem * item = indexToItem( index );
	if ( evalCondition(item) ){
		flags = (Qt::ItemIsEnabled | Qt::ItemIsSelectable);
		if ( index.column() == ValueCol )
			flags |= Qt::ItemIsEditable;
	}
	
	return flags;
}


/*
 *  load and save
 */

bool BaseModel::loadFromFile( const QString & file )
{
	QFile f( file );
	QFileInfo finfo( f );

	setState( Loading );

	if ( f.exists() && finfo.isFile() && f.open( QIODevice::ReadOnly ) && load( f ) ) {
		fileinfo = finfo;
		filename = finfo.baseName();
		folder = finfo.absolutePath();
		resetState();
		return true;
	}

	resetState();
	return false;
}

bool BaseModel::saveToFile( const QString & str ) const
{
	QFile f( str );
	QBuffer buf;
	bool success = false;
	if ( buf.open( QIODevice::WriteOnly ) && save( buf ) )
		success = f.open( QIODevice::WriteOnly ) && f.write( buf.data() ) > 0;

	return success;
}

void BaseModel::refreshFileInfo( const QString & f )
{
	fileinfo = QFileInfo( f );
	filename = fileinfo.baseName();
	folder = fileinfo.absolutePath();
}

/*
 *  searching
 */

const NifItem * BaseModel::getItemInternal( const NifItem * parent, const QString & name, bool reportErrors ) const
{
	for ( auto item : parent->childIter() )
		if ( item->hasName(name) && evalCondition(item) )
			return item;

	if ( reportErrors )
		reportError( parent, tr( "Could not find \"%1\" subitem." ).arg( name ) );
	return nullptr;
}

const NifItem * BaseModel::getItemInternal( const NifItem * parent, const QLatin1String & name, bool reportErrors ) const
{
	for ( auto item : parent->childIter() )
		if ( item->hasName(name) && evalCondition(item) )
			return item;

	if ( reportErrors )
		reportError( parent, tr( "Could not find \"%1\" subitem." ).arg( QString(name) ) );
	return nullptr;
}

const QString SLASH_QSTRING("\\");
const QString DOTS_QSTRING("..");
const QLatin1String SLASH_LATIN("\\");
const QLatin1String DOTS_LATIN("..");

const NifItem * BaseModel::getItem( const NifItem * parent, const QString & name, bool reportErrors ) const
{
	if ( !parent )
		return nullptr;

	int slashPos = name.indexOf( SLASH_QSTRING );
	if ( slashPos > 0 ) {
		QString left  = name.left( slashPos );
		QString right = name.right( name.length() - slashPos - SLASH_QSTRING.length() );

		const NifItem * pp = ( left == DOTS_QSTRING ) ? parent->parent() : getItemInternal( parent, left, reportErrors );
		return getItem( pp, right, reportErrors );	
	}

	return getItemInternal( parent, name, reportErrors );	
}

const NifItem * BaseModel::getItem( const NifItem * parent, const QLatin1String & name, bool reportErrors ) const
{
	if ( !parent )
		return nullptr;

	int slashPos = name.indexOf( SLASH_LATIN );
	if ( slashPos > 0 ) {
		QLatin1String left  = name.left( slashPos );
		QLatin1String right = name.right( name.size() - slashPos - SLASH_LATIN.size() );

		const NifItem * pp = ( left == DOTS_LATIN ) ? parent->parent() : getItemInternal( parent, left, reportErrors );
		return getItem( pp, right, reportErrors );
	}

	return getItemInternal( parent, name, reportErrors );
}

const NifItem * BaseModel::getItem( const NifItem * parent, int childIndex, bool reportErrors ) const
{
	if ( !parent )
		return nullptr;

	const NifItem * item = parent->child( childIndex );
	if ( item ) {
		if ( evalCondition(item) )
			return item;

		if ( reportErrors ) {
			QString repr;
			if ( parent->isArray() )
				repr = QString::number( childIndex );
			else
				repr = QString( "%1 (\"%2\")" ).arg( childIndex ).arg( item->name() );
			reportError( parent, tr( "Subitem with index %1 has failed condition check." ).arg( repr ) );
		}
	} else {
		if ( reportErrors && childIndex >= 0 )
			reportError( parent, tr( "Invalid child index %1." ).arg( childIndex ) );
	}

	return nullptr;
}

const NifItem * BaseModel::getItem( const QModelIndex & index, bool reportErrors ) const
{
	if ( !index.isValid() )
		return nullptr;

	if ( index.model() != this ) {
		if ( reportErrors )
			reportError( tr( "BaseModel::getItem got a model index from another model (%1)." ).arg( indexRepr(index) ) );
		return nullptr;
	}

	const NifItem * item = indexToItem( index );
	if ( !item && reportErrors )
		reportError( "BaseModel::getItem got a valid model index with null internalPointer." ); // WTF
	return item;
}


/*
*  Uses implicit load order
*/
const NifItem * BaseModel::getItemX( const NifItem * item, const QLatin1String & name ) const
{
	while ( item ) {
		const NifItem * parent = item->parent();
		if ( !parent )
			return nullptr;

		for ( int c = item->row() - 1; c >= 0; c-- ) {
			const NifItem * child = parent->child( c );
			if ( child && child->hasName(name) && evalCondition(child) )
				return child;
		}

		item = parent;
	}

	return nullptr;
}

const NifItem * BaseModel::findItemX( const NifItem * parent, const QLatin1String & name ) const
{
	while ( parent ) {
		const NifItem * c = getItem( parent, name, false );
		if ( c )
			return c;
		parent = parent->parent();
	}

	return nullptr;
}

/*
 *  conditions and version
 */

bool BaseModel::evalVersion( const NifItem * item ) const
{
	if ( !item )
		return false;

	if ( !item->isVersionConditionCached() ) {
		if ( item->parent() && !evalVersion( item->parent() ) )
			item->setVersionCondition( false );
		else if ( item->isConditionless() )
			item->setVersionCondition( true );
		else
			item->setVersionCondition( evalVersionImpl( item ) );
	}

	return item->versionCondition();
}

bool BaseModel::evalCondition( const NifItem * item ) const
{
	if ( !item )
		return false;

	if ( !evalVersion(item) )
		return false;

	if ( !item->isConditionCached() ) {
		if ( item->parent() && !evalCondition( item->parent() ) )
			item->setCondition( false );
		else if ( item->isConditionless() )
			item->setCondition( true );
		else
			item->setCondition( evalConditionImpl( item ) );
	}

	return item->condition();
}

bool BaseModel::evalConditionImpl( const NifItem * item ) const
{
	// If there is a cond, evaluate it
	if ( !item->cond().isEmpty() ) {
		BaseModelEval functor( this, item );
		if ( !item->condexpr().evaluateBool(functor) )
			return false;
	}

	return true;
}

QString BaseModel::itemRepr( const NifItem * item ) const
{
	if ( !item )
		return QString("[NULL]");
	if ( item->model() != this )
		return item->model()->itemRepr( item );
	if ( item == root )
		return QString("[ROOT]");

	QString result;
	while( true ) {
		const NifItem * parent = item->parent();
		if ( !parent ) {
			result = "???" + result; // WTF...
			break;
		} else if ( parent == root ) {
			result = topItemRepr( item ) + result;
			break;
		} else {
			QString subres;
			if ( parent->isArrayEx() )
				subres = QString(" [%1]").arg( item->row() );
			else
				subres = "\\" + item->name();
			result = subres + result;
			item = parent;
		}
	}

	return result;
}

QString BaseModel::indexRepr( const QModelIndex & index )
{
	if ( index.isValid() ) {
		auto model = qobject_cast<const BaseModel *>( index.model() );
		if ( !model )
			return QString("[INVALID INDEX (BAD MODEL)]");
		auto item = indexToItem( index );
		if ( !item )
			return QString("[INVALID INDEX (BAD ITEM POINTER)]");
		return model->itemRepr( item );
	}

	return QString("[NULL]");
}

QString BaseModel::topItemRepr( const NifItem * item ) const
{
	return QString("%2 [%1]").arg( item->row() ).arg( item->name() );
}

void BaseModel::reportError( const QString & err ) const
{
	if ( msgMode == MSG_USER )
		Message::append(getWindow(), "Parsing warnings:", err);
	else
		testMsg(err);
}

void BaseModel::reportError( const NifItem * item, const QString & err ) const
{
	reportError( itemRepr( item ) + ": " + err );
}

void BaseModel::reportError( const NifItem * item, const QString & funcName, const QString & err ) const
{
	reportError( QString("%1, %2: %3").arg( itemRepr( item ), funcName, err ) );
}

void BaseModel::reportError( const QModelIndex & index, const QString & err ) const
{
	reportError( indexRepr( index ) + ": " + err );
}

void BaseModel::reportError( const QModelIndex & index, const QString & funcName, const QString & err ) const
{
	reportError( QString("%1, %2: %3").arg( indexRepr( index ), funcName, err ) );
}

void BaseModel::onItemValueChange( NifItem * item )
{
	if ( state != Processing ) {
		QModelIndex idx = itemToIndex( item, ValueCol );
		emit dataChanged( idx, idx );
	} else {
		changedWhileProcessing = true;
	}
}

void BaseModel::onArrayValuesChange( NifItem * arrayRootItem )
{
	int x = arrayRootItem->childCount() - 1;
	if ( x >= 0 ) {
		emit dataChanged(
			createIndex( 0, ValueCol, arrayRootItem->children().at(0) ), 
			createIndex( x, ValueCol, arrayRootItem->children().at(x) )
		);
	}
}

const NifItem * BaseModel::getTopItem( const NifItem * item ) const
{
	while( item ) {
		auto p = item->parent();
		if ( p == root )
			break;
		item = p;
	}

	return item;
}


/*
 *  BaseModelEval
 */

BaseModelEval::BaseModelEval( const BaseModel * model, const NifItem * item )
{
	this->model = model;
	this->item  = item;
}

QVariant BaseModelEval::operator()( const QVariant & v ) const
{
	if ( v.type() == QVariant::String ) {

		// Resolve "ARG"
		QString left = v.toString();
		const NifItem * exprItem = item;
		bool isArgExpr = false;
		while ( left == XMLARG ) {
			exprItem = exprItem->parent();
			if ( !exprItem )
				return false;
			left = exprItem->arg();
			isArgExpr = !exprItem->argexpr().noop();
		}

		// ARG is an expression
		if ( isArgExpr )
			return exprItem->argexpr().evaluateUInt64( BaseModelEval( model, exprItem) );

		bool numeric;
		int val = left.toInt( &numeric, 10 );
		if ( numeric )
			return QVariant( val );

		// resolve reference to sibling
		const NifItem * sibling = model->getItem( exprItem->parent(), left );
		if ( sibling ) {
			if ( sibling->isCount() || sibling->isFloat() ) {
				return QVariant( sibling->getCountValue() );
			} else if ( sibling->isFileVersion() ) {
				return QVariant( sibling->getFileVersionValue() );
			// this is tricky to understand
			// we check whether the reference is an array
			// if so, we get the current item's row number (exprItem->row())
			// and get the sibling's child at that row number
			// this is used for instance to describe array sizes of strips
			} else if ( sibling->childCount() > 0 ) {
				const NifItem * i2 = sibling->child( exprItem->row() );

				if ( i2 && i2->isCount() )
					return QVariant( i2->getCountValue() );
			} else if ( sibling->valueType() == NifValue::tBSVertexDesc ) {
				return QVariant( sibling->get<BSVertexDesc>().GetFlags() << 4 );
			} else {
				model->reportError( item, QString( "BaseModelEval could not convert %1 to a count." ).arg( sibling->repr() ) );
			}
		}

		// resolve reference to block type
		// is the condition string a type?
		if ( model->isAncestorOrNiBlock( left ) ) {
			// get the type of the current block
			auto itemBlock = model->getTopItem( exprItem );
			if ( itemBlock )
				return QVariant( model->inherits( itemBlock->name(), left ) );
		}

		return QVariant( 0 );
	}

	return v;
}

unsigned DJB1Hash( const char * key, unsigned tableSize )
{
	unsigned hash = 0;
	while ( *key ) {
		hash *= 33;
		hash += *key;
		key++;
	}
	return hash % tableSize;
}

QString addConditionParentPrefix( const QString & x )
{
	for ( int c = 0; c < x.length(); c++ ) {
		if ( !x[c].isNumber() )
			return QString( "..\\" ) + x;
	}

	return x;
}
