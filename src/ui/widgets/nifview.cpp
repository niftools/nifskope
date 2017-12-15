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

#include "nifview.h"

#include "spellbook.h"
#include "model/basemodel.h"
#include "model/nifproxymodel.h"
#include "model/undocommands.h"

#include <QApplication>
#include <QMimeData>
#include <QClipboard>
#include <QKeyEvent>

#include <vector>

NifTreeView::NifTreeView( QWidget * parent, Qt::WindowFlags flags ) : QTreeView()
{
	Q_UNUSED( flags );

	setParent( parent );

	connect( this, &NifTreeView::expanded, this, &NifTreeView::scrollExpand );
}

NifTreeView::~NifTreeView()
{
}

void NifTreeView::setModel( QAbstractItemModel * model )
{
	if ( nif )
		disconnect( nif, &BaseModel::dataChanged, this, &NifTreeView::updateConditions );

	nif = qobject_cast<BaseModel *>( model );

	QTreeView::setModel( model );

	if ( nif && doRowHiding ) {
		connect( nif, &BaseModel::dataChanged, this, &NifTreeView::updateConditions );
	}
}

void NifTreeView::setRootIndex( const QModelIndex & index )
{
	QModelIndex root = index;

	if ( root.isValid() && root.column() != 0 )
		root = root.sibling( root.row(), 0 );

	QTreeView::setRootIndex( root );
}

void NifTreeView::clearRootIndex()
{
	setRootIndex( QModelIndex() );
}

void NifTreeView::setRowHiding( bool show )
{
	if ( doRowHiding != show )
		return;

	doRowHiding = !show;

	if ( nif && doRowHiding ) {
		connect( nif, &BaseModel::dataChanged, this, &NifTreeView::updateConditions );
	} else if ( nif ) {
		disconnect( nif, &BaseModel::dataChanged, this, &NifTreeView::updateConditions );
	}

	// refresh
	updateConditionRecurse( rootIndex() );
	doItemsLayout();
}


bool NifTreeView::isRowHidden( int r, const QModelIndex & index ) const
{
	NifItem * item = static_cast<NifItem *>(index.internalPointer());
	if ( !item || !doRowHiding )
		return false;

	return !item->condition();
}

void NifTreeView::setAllExpanded( const QModelIndex & index, bool e )
{
	if ( !model() )
		return;

	for ( int r = 0; r < model()->rowCount( index ); r++ ) {
		QModelIndex child = model()->index( r, 0, index );

		if ( model()->hasChildren( child ) ) {
			setExpanded( child, e );
			setAllExpanded( child, e );
		}
	}
}

QStyleOptionViewItem NifTreeView::viewOptions() const
{
	QStyleOptionViewItem opt = QTreeView::viewOptions();
	opt.showDecorationSelected = true;
	return opt;
}

void NifTreeView::copy()
{
	QModelIndex idx = selectionModel()->selectedIndexes().first();
	auto item = static_cast<NifItem *>(idx.internalPointer());
	if ( !item )
		return;

	if ( !item->isArray() && !item->isCompound() ) {
		valueClipboard->setValue( item->value() );
	} else {
		std::vector<NifValue> v;
		v.reserve( item->childCount() );
		for ( const auto i : item->children() )
			v.push_back( i->value() );

		valueClipboard->setValues( v );
	}
}

void NifTreeView::pasteTo( QModelIndex iDest, const NifValue & srcValue )
{	
	// Only run once per row for the correct column
	if ( iDest.column() != NifModel::ValueCol )
		return;

	NifItem * item = static_cast<NifItem *>(iDest.internalPointer());

	auto valueType = model()->sibling( iDest.row(), 0, iDest ).data().toString();

	NifValue destValue = item->value();
	if ( destValue.type() != srcValue.type() )
		return;

	switch ( destValue.type() ) {
	case NifValue::tByte:
		nif->set<quint8>( iDest, srcValue.get<quint8>() );
		break;
	case NifValue::tWord:
	case NifValue::tShort:
	case NifValue::tFlags:
	case NifValue::tBlockTypeIndex:
		nif->set<quint16>( iDest, srcValue.get<quint16>() );
		break;
	case NifValue::tStringOffset:
	case NifValue::tInt:
	case NifValue::tUInt:
	case NifValue::tULittle32:
	case NifValue::tStringIndex:
	case NifValue::tUpLink:
	case NifValue::tLink:
		nif->set<quint32>( iDest, srcValue.get<quint32>() );
		break;
	case NifValue::tVector2:
	case NifValue::tHalfVector2:
		nif->set<Vector2>( iDest, srcValue.get<Vector2>() );
		break;
	case NifValue::tVector3:
	case NifValue::tByteVector3:
	case NifValue::tHalfVector3:
		nif->set<Vector3>( iDest, srcValue.get<Vector3>() );
		break;
	case NifValue::tVector4:
		nif->set<Vector4>( iDest, srcValue.get<Vector4>() );
		break;
	case NifValue::tFloat:
	case NifValue::tHfloat:
		nif->set<float>( iDest, srcValue.get<float>() );
		break;
	case NifValue::tColor3:
		nif->set<Color3>( iDest, srcValue.get<Color3>() );
		break;
	case NifValue::tColor4:
	case NifValue::tByteColor4:
		nif->set<Color4>( iDest, srcValue.get<Color4>() );
		break;
	case NifValue::tQuat:
	case NifValue::tQuatXYZW:
		nif->set<Quat>( iDest, srcValue.get<Quat>() );
		break;
	case NifValue::tMatrix:
		nif->set<Matrix>( iDest, srcValue.get<Matrix>() );
		break;
	case NifValue::tMatrix4:
		nif->set<Matrix4>( iDest, srcValue.get<Matrix4>() );
		break;
	case NifValue::tString:
	case NifValue::tSizedString:
	case NifValue::tText:
	case NifValue::tShortString:
	case NifValue::tHeaderString:
	case NifValue::tLineString:
	case NifValue::tChar8String:
		nif->set<QString>( iDest, srcValue.get<QString>() );
		break;
	default:
		// Return and do not push to Undo Stack
		return;
	}

	auto n = static_cast<NifModel*>(nif);
	if ( n )
		n->undoStack->push( new ChangeValueCommand( iDest, destValue, srcValue, valueType, n ) );
}

void NifTreeView::paste()
{
	ChangeValueCommand::createTransaction();
	for ( const auto i : valueIndexList( selectionModel()->selectedIndexes() ) )
		pasteTo( i, valueClipboard->getValue() );
}

void NifTreeView::pasteArray()
{
	QModelIndexList selected = selectionModel()->selectedIndexes();
	QModelIndexList values = valueIndexList( selected );

	Q_ASSERT( selected.size() == 10 );
	Q_ASSERT( values.size() == 1 );

	auto root = values.at( 0 );
	auto cnt = nif->rowCount( root );

	ChangeValueCommand::createTransaction();
	nif->setState( BaseModel::Processing );
	for ( int i = 0; i < cnt && i < valueClipboard->getValues().size(); i++ ) {
		auto iDest = root.child( i, NifModel::ValueCol );
		auto srcValue = valueClipboard->getValues().at( iDest.row() );

		pasteTo( iDest, srcValue );
	}
	nif->restoreState();

	if ( cnt > 0 )
		emit nif->dataChanged( root.child( 0, NifModel::ValueCol ), root.child( cnt - 1, NifModel::ValueCol ) );
}

void NifTreeView::drawBranches( QPainter * painter, const QRect & rect, const QModelIndex & index ) const
{
	if ( rootIsDecorated() )
		QTreeView::drawBranches( painter, rect, index );
}

void NifTreeView::updateConditions( const QModelIndex & topLeft, const QModelIndex & bottomRight )
{
	if ( nif->getState() != BaseModel::Default )
		return;

	Q_UNUSED( bottomRight );
	updateConditionRecurse( topLeft.parent() );
	doItemsLayout();
}

void NifTreeView::updateConditionRecurse( const QModelIndex & index )
{
	if ( nif->getState() != BaseModel::Default )
		return;

	NifItem * item = static_cast<NifItem *>(index.internalPointer());
	if ( !item )
		return;

	// Skip flat array items
	if ( item->parent() && item->parent()->isArray() && !item->childCount() )
		return;

	for ( int r = 0; r < model()->rowCount( index ); r++ ) {
		QModelIndex child = model()->index( r, 0, index );
		updateConditionRecurse( child );
	}

	setRowHidden( index.row(), index.parent(), doRowHiding && !item->condition() );
}

auto splitMime = []( QString format ) {
	QStringList split = format.split( "/" );
	if ( split.value( 0 ) == "nifskope"
		 && (split.value( 1 ) == "niblock" || split.value( 1 ) == "nibranch") )
		return !split.value( 2 ).isEmpty();

	return false;
};

QModelIndexList NifTreeView::valueIndexList( const QModelIndexList & rows ) const
{
	QModelIndexList values;
	for ( int i = NifModel::ValueCol; i < rows.count(); i += NifModel::NumColumns )
		values << rows[i];

	return values;
}

void NifTreeView::keyPressEvent( QKeyEvent * e )
{
	NifModel * nif = nullptr;
	NifProxyModel * proxy = nullptr;

	nif = static_cast<NifModel *>(model());
	if ( model()->inherits( "NifModel" ) && nif ) {
		// Determine if a block or branch has been copied
		bool hasBlockCopied = false;
		if ( e->matches( QKeySequence::Copy ) || e->matches( QKeySequence::Paste ) ) {
			auto mime = QApplication::clipboard()->mimeData();
			if ( mime ) {
				for ( const QString& form : mime->formats() ) {
					if ( splitMime( form ) ) {
						hasBlockCopied = true;
						break;
					}
				}
			}
		}

		QModelIndexList selectedRows = selectionModel()->selectedIndexes();
		QModelIndexList valueColumns = valueIndexList( selectedRows );
		if ( !(selectedRows.size() && valueColumns.size()) )
			return;

		auto firstRow = selectedRows.at( 0 );
		auto firstValue = valueColumns.at( 0 );
		auto firstRowType = nif->getValue( firstRow ).type();

		if ( e->matches( QKeySequence::Copy ) ) {
			copy();
			// Clear the clipboard in case it holds a block to prevent conflicting behavior
			QApplication::clipboard()->clear();
			return;
		} else if ( e->matches( QKeySequence::Paste )
					&& (valueClipboard->getValue().isValid() || valueClipboard->getValues().size() > 0)
					&& !hasBlockCopied ) {
			// Do row paste if there is no block/branch copied and the NifValue is valid
			if ( valueColumns.size() == 1 && nif->rowCount( firstRow ) > 0 ) {
				pasteArray();
			} else if ( valueClipboard->getValue().isValid() ) {
				paste();
			}
			return;
		} else if ( valueColumns.size() == 1
					&& firstRow.parent().isValid() && nif->isArray( firstRow.parent() )
					&& (firstRowType == NifValue::tUpLink || firstRowType == NifValue::tLink) ) {
			// Link Array Sorting
			auto parent = firstRow.parent();
			auto row = firstRow.row();
			enum {
				MOVE_UP = -1,
				MOVE_NONE = 0,
				MOVE_DOWN = 1
			} moveDir = MOVE_NONE;

			if ( e->key() == Qt::Key_Down && e->modifiers() == Qt::CTRL && row < nif->rowCount( parent ) - 1 )
				moveDir = MOVE_DOWN;
			else if ( e->key() == Qt::Key_Up && e->modifiers() == Qt::CTRL && row > 0 )
				moveDir = MOVE_UP;

			if ( moveDir ) {
				// Swap the rows
				row = row + moveDir;
				QModelIndex newValue = firstRow.sibling( row, NifModel::ValueCol );
				QVariant v = nif->data( firstValue, Qt::EditRole );
				nif->setData( firstValue, nif->data( newValue, Qt::EditRole ) );
				nif->setData( newValue, v );

				// Change the selected row
				selectionModel()->select( parent.child( row, 0 ), QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows );

				// Add row swap to undo
				ChangeValueCommand::createTransaction();
				nif->undoStack->push( new ChangeValueCommand( firstValue, nif->getValue( newValue ), nif->getValue( firstValue ), "Link", nif ) );
				nif->undoStack->push( new ChangeValueCommand( newValue, nif->getValue( firstValue ), nif->getValue( newValue ), "Link", nif ) );
			}
		}
	}

	SpellPtr spell = SpellBook::lookup( QKeySequence( e->modifiers() + e->key() ) );

	if ( spell ) {
		QPersistentModelIndex oldidx;

		// Clear this on any spell cast to prevent it overriding other paste behavior like block -> link row
		// TODO: Value clipboard does not get cleared when using the context menu. 
		valueClipboard->clear();

		if ( model()->inherits( "NifModel" ) ) {
			nif = static_cast<NifModel *>( model() );
			oldidx = currentIndex();
		} else if ( model()->inherits( "NifProxyModel" ) ) {
			proxy = static_cast<NifProxyModel *>( model() );
			nif = static_cast<NifModel *>( proxy->model() );
			oldidx = proxy->mapTo( currentIndex() );
		}

		if ( nif && spell->isApplicable( nif, oldidx ) ) {
			selectionModel()->setCurrentIndex( QModelIndex(), QItemSelectionModel::Clear | QItemSelectionModel::Rows );

			bool noSignals = spell->batch();
			if ( noSignals )
				nif->setState( BaseModel::Processing );
			// Cast the spell and return index
			QModelIndex newidx = spell->cast( nif, oldidx );
			if ( noSignals )
				nif->resetState();

			// Refresh the header
			nif->invalidateConditions( nif->getHeader(), true );
			nif->updateHeader();

			if ( noSignals && nif->getProcessingResult() ) {
				emit nif->dataChanged( newidx, newidx );
			}

			if ( proxy )
				newidx = proxy->mapFrom( newidx, oldidx );

			// grab selection from the selection model as it tends to be more accurate
			if ( !newidx.isValid() ) {
				if ( oldidx.isValid() )
					newidx = ( proxy ) ? proxy->mapFrom( oldidx, oldidx ) : ( (QModelIndex)oldidx );
				else
					newidx = selectionModel()->currentIndex();
			}

			if ( newidx.isValid() ) {
				selectionModel()->setCurrentIndex( newidx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows );
				scrollTo( newidx, EnsureVisible );
				emit clicked( newidx );
			}

			return;
		}
	}

	QTreeView::keyPressEvent( e );
}

void NifTreeView::currentChanged( const QModelIndex & current, const QModelIndex & last )
{
	QTreeView::currentChanged( current, last );

	if ( nif && doRowHiding ) {
		updateConditionRecurse( current );
	}

	autoExpanded = false;
	auto mdl = static_cast<NifModel *>( nif );
	if ( mdl && mdl->isNiBlock( current ) ) {
		auto cnt = mdl->rowCount( current );
		const int ARRAY_LIMIT = 100;
		if ( mdl->inherits( current, "NiTransformInterpolator" ) 
			 || mdl->inherits( current, "NiBSplineTransformInterpolator" ) ) {
			// Auto-Expand NiQuatTransform
			autoExpand( current.child( 0, 0 ) );
		} else if ( mdl->inherits( current, "NiNode" ) ) {
			// Auto-Expand Children array
			auto iChildren = mdl->getIndex( current, "Children" );
			if ( mdl->rowCount( iChildren ) < ARRAY_LIMIT )
				autoExpand( iChildren );
		} else if ( mdl->inherits( current, "NiSkinPartition" ) ) {
			// Auto-Expand skin partitions array
			autoExpand( current.child( 1, 0 ) );
		} else if ( mdl->getValue( current.child( cnt - 1, 0 ) ).type() == NifValue::tNone
					&& mdl->rowCount( current.child( cnt - 1, 0 ) ) < ARRAY_LIMIT ) {
			// Auto-Expand final arrays/compounds
			autoExpand( current.child( cnt - 1, 0 ) );
		}
	}

	emit sigCurrentIndexChanged( currentIndex() );
}

void NifTreeView::autoExpand( const QModelIndex & index )
{
	autoExpanded = true;
	expand( index );
}

void NifTreeView::scrollExpand( const QModelIndex & index )
{
	// this is a compromise between scrolling to the top, and scrolling the last child to the bottom
	if ( !autoExpanded )
		scrollTo( index, PositionAtCenter );
}
