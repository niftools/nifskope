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

#include "message.h"

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
	root = new NifItem( 0 );
	parentWindow = qobject_cast<QWidget *>(p);
	msgMode = TstMessage;
}

BaseModel::~BaseModel()
{
	delete root;
}

QWidget * BaseModel::getWindow()
{
	return parentWindow;
}

void BaseModel::setEmitChanges( bool e )
{
	emitChanges = e;
}

void BaseModel::setMessageMode( MsgMode mode )
{
	msgMode = mode;
}

void BaseModel::testMsg( const QString & m ) const
{
	messages.append( TestMessage() << m );
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

bool BaseModel::isArray( const QModelIndex & index ) const
{
	return !itemArr1( index ).isEmpty();
}

bool BaseModel::isArray( NifItem * item ) const
{
	return item->isArray() || (item->parent() && item->parent()->isMultiArray());
}

int BaseModel::getArraySize( NifItem * array ) const
{
	// shortcut for speed
	if ( !isArray( array ) )
		return 0;

	return evaluateInt( array, array->arr1expr() );
}

bool BaseModel::updateArray( const QModelIndex & array )
{
	NifItem * item = static_cast<NifItem *>( array.internalPointer() );

	if ( !( array.isValid() && item && array.model() == this ) )
		return false;

	return updateArrayItem( item );
}

bool BaseModel::updateArray( const QModelIndex & parent, const QString & name )
{
	return updateArray( getIndex( parent, name ) );
}

/*
 *  item value functions
 */

QString BaseModel::itemName( const QModelIndex & index ) const
{
	NifItem * item = static_cast<NifItem *>( index.internalPointer() );

	if ( !( index.isValid() && item && index.model() == this ) )
		return QString();

	return item->name();
}

QString BaseModel::itemType( const QModelIndex & index ) const
{
	NifItem * item = static_cast<NifItem *>( index.internalPointer() );

	if ( !( index.isValid() && item && index.model() == this ) )
		return QString();

	return item->type();
}

QString BaseModel::itemTmplt( const QModelIndex & index ) const
{
	NifItem * item = static_cast<NifItem *>( index.internalPointer() );

	if ( !( index.isValid() && item && index.model() == this ) )
		return QString();

	return item->temp();
}


NifValue BaseModel::getValue( const QModelIndex & index ) const
{
	NifItem * item = static_cast<NifItem *>( index.internalPointer() );

	if ( !( index.isValid() && item && index.model() == this ) )
		return NifValue();

	return item->value();
}

// where is
// NifValue BaseModel::getValue( const QModelIndex & parent, const QString & name ) const
// ?

QString BaseModel::itemArg( const QModelIndex & index ) const
{
	NifItem * item = static_cast<NifItem *>( index.internalPointer() );

	if ( !( index.isValid() && item && index.model() == this ) )
		return QString();

	return item->arg();
}

QString BaseModel::itemArr1( const QModelIndex & index ) const
{
	NifItem * item = static_cast<NifItem *>( index.internalPointer() );

	if ( !( index.isValid() && item && index.model() == this ) )
		return QString();

	return item->arr1();
}

QString BaseModel::itemArr2( const QModelIndex & index ) const
{
	NifItem * item = static_cast<NifItem *>( index.internalPointer() );

	if ( !( index.isValid() && item && index.model() == this ) )
		return QString();

	return item->arr2();
}

QString BaseModel::itemCond( const QModelIndex & index ) const
{
	NifItem * item = static_cast<NifItem *>( index.internalPointer() );

	if ( !( index.isValid() && item && index.model() == this ) )
		return QString();

	return item->cond();
}

quint32 BaseModel::itemVer1( const QModelIndex & index ) const
{
	NifItem * item = static_cast<NifItem *>( index.internalPointer() );

	if ( !( index.isValid() && item && index.model() == this ) )
		return 0;

	return item->ver1();
}

quint32 BaseModel::itemVer2( const QModelIndex & index ) const
{
	NifItem * item = static_cast<NifItem *>( index.internalPointer() );

	if ( !( index.isValid() && item && index.model() == this ) )
		return 0;

	return item->ver2();
}

QString BaseModel::itemText( const QModelIndex & index ) const
{
	NifItem * item = static_cast<NifItem *>( index.internalPointer() );

	if ( !( index.isValid() && item && index.model() == this ) )
		return QString();

	return item->text();
}


bool BaseModel::setValue( const QModelIndex & index, const NifValue & val )
{
	NifItem * item = static_cast<NifItem *>( index.internalPointer() );

	if ( !( index.isValid() && item && index.model() == this ) )
		return false;

	return setItemValue( item, val );
}

bool BaseModel::setValue( const QModelIndex & parent, const QString & name, const NifValue & val )
{
	NifItem * parentItem = static_cast<NifItem *>( parent.internalPointer() );

	if ( !( parent.isValid() && parentItem && parent.model() == this ) )
		return false;

	NifItem * item = getItem( parentItem, name );

	if ( item )
		return setItemValue( item, val );

	return false;
}


/*
 *  QAbstractModel interface
 */

QModelIndex BaseModel::index( int row, int column, const QModelIndex & parent ) const
{
	NifItem * parentItem;

	if ( !( parent.isValid() && parent.model() == this ) )
		parentItem = root;
	else
		parentItem = static_cast<NifItem *>( parent.internalPointer() );

	NifItem * childItem = ( parentItem ? parentItem->child( row ) : 0 );

	if ( childItem )
		return createIndex( row, column, childItem );

	return QModelIndex();
}

QModelIndex BaseModel::parent( const QModelIndex & child ) const
{
	if ( !( child.isValid() && child.model() == this ) )
		return QModelIndex();

	NifItem * childItem = static_cast<NifItem *>( child.internalPointer() );

	if ( !childItem || childItem == root )
		return QModelIndex();

	NifItem * parentItem = childItem->parent();

	if ( !parentItem || parentItem == root )
		return QModelIndex();

	return createIndex( parentItem->row(), 0, parentItem );
}

int BaseModel::rowCount( const QModelIndex & parent ) const
{
	NifItem * parentItem;

	if ( !( parent.isValid() && parent.model() == this ) )
		parentItem = root;
	else
		parentItem = static_cast<NifItem *>( parent.internalPointer() );

	return ( parentItem ? parentItem->childCount() : 0 );
}

QVariant BaseModel::data( const QModelIndex & index, int role ) const
{
	NifItem * item = static_cast<NifItem *>( index.internalPointer() );

	if ( !( index.isValid() && item && index.model() == this ) )
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
				return item->type();
			case ValueCol:
				return item->value().toString();
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
				return item->type();
			case ValueCol:
				return item->value().toVariant();
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
					switch ( item->value().type() ) {
					case NifValue::tWord:
					case NifValue::tShort:
						{
							quint16 s = item->value().toCount();
							return QString( "dec: %1<br>hex: 0x%2" ).arg( s ).arg( s, 4, 16, QChar( '0' ) );
						}
					case NifValue::tBool:
					case NifValue::tInt:
					case NifValue::tUInt:
					case NifValue::tULittle32:
						{
							quint32 i = item->value().toCount();
							return QString( "dec: %1<br>hex: 0x%2" ).arg( i ).arg( i, 8, 16, QChar( '0' ) );
						}
					case NifValue::tFloat:
					case NifValue::tHfloat:
						{
							float f = item->value().toFloat();
							quint32 i = item->value().toCount();
							return QString( "float: %1<br>data: 0x%2" ).arg( f ).arg( i, 8, 16, QChar( '0' ) );
						}
					case NifValue::tFlags:
						{
							quint16 f = item->value().toCount();
							return QString( "dec: %1<br>hex: 0x%2<br>bin: 0b%3" ).arg( f ).arg( f, 4, 16, QChar( '0' ) ).arg( f, 16, 2, QChar( '0' ) );
						}
					case NifValue::tVector3:
						return item->value().get<Vector3>().toHtml();
					case NifValue::tHalfVector3:
						return item->value().get<HalfVector3>().toHtml();
					case NifValue::tByteVector3:
						return item->value().get<ByteVector3>().toHtml();
					case NifValue::tMatrix:
						return item->value().get<Matrix>().toHtml();
					case NifValue::tQuat:
					case NifValue::tQuatXYZW:
						return item->value().get<Quat>().toHtml();
					case NifValue::tColor3:
						{
							Color3 c = item->value().get<Color3>();
							return QString( "R %1<br>G %2<br>B %3" ).arg( c[0] ).arg( c[1] ).arg( c[2] );
						}
					case NifValue::tByteColor4:
						{
							Color4 c = item->value().get<ByteColor4>();
							return QString( "R %1<br>G %2<br>B %3<br>A %4" ).arg( c[0] ).arg( c[1] ).arg( c[2] ).arg( c[3] );
						}
					case NifValue::tColor4:
						{
							Color4 c = item->value().get<Color4>();
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
			if ( column == ValueCol && item->value().isColor() ) {
				return item->value().toColor();
			}
		}
		return QVariant();
	default:
		return QVariant();
	}
}

bool BaseModel::setData( const QModelIndex & index, const QVariant & value, int role )
{
	NifItem * item = static_cast<NifItem *>( index.internalPointer() );

	if ( !( index.isValid() && role == Qt::EditRole && index.model() == this && item ) )
		return false;

	switch ( index.column() ) {
	case BaseModel::NameCol:
		item->setName( value.toString() );
		break;
	case BaseModel::TypeCol:
		item->setType( value.toString() );
		break;
	case BaseModel::ValueCol:
		item->value().setFromVariant( value );
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
		emit dataChanged( index, index );

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

	auto item = static_cast<NifItem *>(index.internalPointer());

	bool condExpr;
	if ( item )
		condExpr = item->condition();
	else
		condExpr = evalCondition( index, true );

	if ( condExpr )
		flags = (Qt::ItemIsEnabled | Qt::ItemIsSelectable);

	switch ( index.column() ) {
	case TypeCol:
	case NameCol:
		return flags;
	case ValueCol:
		if ( condExpr )
			return flags | Qt::ItemIsEditable;

		return flags;

	default:
		return flags;
	}
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

NifItem * BaseModel::getItem( NifItem * item, const QString & name ) const
{
	if ( !item || item == root )
		return nullptr;

	int slash = name.indexOf( "\\" );
	if ( slash > 0 ) {
		QString left  = name.left( slash );
		QString right = name.right( name.length() - slash - 1 );

		if ( left == ".." )
			return getItem( item->parent(), right );

		return getItem( getItem( item, left ), right );
	}

	for ( int c = 0; c < item->childCount(); c++ ) {
		NifItem * child = item->child( c );

		if ( child->name() == name && evalCondition( child ) )
			return child;
	}

	return nullptr;
}

/*
*  Uses implicit load order
*/
NifItem * BaseModel::getItemX( NifItem * item, const QString & name ) const
{
	if ( !item || !item->parent() )
		return 0;

	NifItem * parent = item->parent();

	for ( int c = item->row() - 1; c >= 0; c-- ) {
		NifItem * child = parent->child( c );

		if ( child && child->name() == name && evalCondition( child ) )
			return child;
	}

	return getItemX( parent, name );
}

NifItem * BaseModel::findItemX( NifItem * item, const QString & name ) const
{
	while ( item ) {
		NifItem * r = getItem( item, name );

		if ( r )
			return r;

		item = item->parent();
	}

	return nullptr;
}

QModelIndex BaseModel::getIndex( const QModelIndex & parent, const QString & name ) const
{
	NifItem * parentItem = static_cast<NifItem *>( parent.internalPointer() );

	if ( !( parent.isValid() && parentItem && parent.model() == this ) )
		return QModelIndex();

	NifItem * item = getItem( parentItem, name );

	if ( item )
		return createIndex( item->row(), 0, item );

	return QModelIndex();
}

/*
 *  conditions and version
 */

int BaseModel::evaluateInt( NifItem * item, const NifExpr & expr ) const
{
	if ( !item || item == root )
		return -1;

	BaseModelEval functor( this, item );
	return expr.evaluateUInt( functor );
}

bool BaseModel::evalCondition( NifItem * item, bool chkParents ) const
{
	if ( !evalVersion( item, chkParents ) ) {
		// Version is global and cond is not so set false and abort
		item->setCondition( false );
		return false;
	}

	if ( item->isConditionValid() )
		return item->condition();

	item->setCondition( item == root );
	if ( item->condition() )
		return true;

	if ( chkParents && item->parent() ) {
		// Set false if parent is false and reject early
		item->setCondition( evalCondition( item->parent(), true ) );
		if ( !item->condition() )
			return false;
	}

	// Early reject for cond
	item->setCondition( item->cond().isEmpty() );
	if ( item->condition() )
		return true;

	// If there is a cond, evaluate it
	BaseModelEval functor( this, item );
	item->setCondition( item->condexpr().evaluateBool( functor ) );

	return item->condition();
}

bool BaseModel::evalVersion( const QModelIndex & index, bool chkParents ) const
{
	NifItem * item = static_cast<NifItem *>(index.internalPointer());

	if ( index.isValid() && index.model() == this && item )
		return evalVersion( item, chkParents );

	return false;
}

bool BaseModel::evalCondition( const QModelIndex & index, bool chkParents ) const
{
	NifItem * item = static_cast<NifItem *>(index.internalPointer());

	if ( index.isValid() && index.model() == this && item )
		return evalCondition( item, chkParents );

	return false;
}


/*
 *  BaseModelEval
 */

BaseModelEval::BaseModelEval( const BaseModel * model, const NifItem * item )
{
	this->model = model;
	this->item  = item;
}

QVariant BaseModelEval::operator()(const QVariant & v) const
{
	if ( v.type() == QVariant::String ) {
		QString left = v.toString();
		const NifItem * i = item;

		// resolve "ARG"
		while ( left == "ARG" ) {
			if ( !i->parent() )
				return false;

			i = i->parent();
			left = i->arg();
		}

		// resolve reference to sibling
		const NifItem * sibling = model->getItem( i->parent(), left );

		if ( sibling ) {
			if ( sibling->value().isCount() || sibling->value().isFloat() ) {
				return QVariant( sibling->value().toCount() );
			} else if ( sibling->value().isFileVersion() ) {
				return QVariant( sibling->value().toFileVersion() );
			// this is tricky to understand
			// we check whether the reference is an array
			// if so, we get the current item's row number (i->row())
			// and get the sibling's child at that row number
			// this is used for instance to describe array sizes of strips
			} else if ( sibling->childCount() > 0 ) {
				const NifItem * i2 = sibling->child( i->row() );

				if ( i2 && i2->value().isCount() )
					return QVariant( i2->value().toCount() );
			} else {
				if ( sibling->value().type() == NifValue::tBSVertexDesc )
					return QVariant( sibling->value().get<BSVertexDesc>().GetFlags() << 4 );

				qDebug() << ("can't convert " + left + " to a count");
			}
		}

		// resolve reference to block type
		// is the condition string a type?
		if ( model->isAncestorOrNiBlock( left ) ) {
			// get the type of the current block
			const NifItem * block = i;

			while ( block->parent() && block->parent()->parent() ) {
				block = block->parent();
			}

			return QVariant( model->inherits( block->name(), left ) );
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
