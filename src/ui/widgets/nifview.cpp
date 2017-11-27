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
	if ( item )
		valueClipboard->setValue( item->value() );
}

void NifTreeView::pasteTo( QModelIndex idx )
{
	NifItem * item = static_cast<NifItem *>(idx.internalPointer());
	// Only run once per row for the correct column
	if ( idx.column() != NifModel::ValueCol )
		return;

	auto valueType = model()->sibling( idx.row(), 0, idx ).data().toString();

	auto copyVal = valueClipboard->getValue();

	NifValue dest = item->value();
	if ( dest.type() != copyVal.type() )
		return;

	switch ( item->value().type() ) {
	case NifValue::tByte:
		nif->set<quint8>( idx, copyVal.get<quint8>() );
		break;
	case NifValue::tWord:
	case NifValue::tShort:
	case NifValue::tFlags:
	case NifValue::tBlockTypeIndex:
		nif->set<quint16>( idx, copyVal.get<quint16>() );
		break;
	case NifValue::tStringOffset:
	case NifValue::tInt:
	case NifValue::tUInt:
	case NifValue::tULittle32:
	case NifValue::tStringIndex:
	case NifValue::tUpLink:
	case NifValue::tLink:
		nif->set<quint32>( idx, copyVal.get<quint32>() );
		break;
	case NifValue::tVector2:
	case NifValue::tHalfVector2:
		nif->set<Vector2>( idx, copyVal.get<Vector2>() );
		break;
	case NifValue::tVector3:
	case NifValue::tByteVector3:
	case NifValue::tHalfVector3:
		nif->set<Vector3>( idx, copyVal.get<Vector3>() );
		break;
	case NifValue::tVector4:
		nif->set<Vector4>( idx, copyVal.get<Vector4>() );
		break;
	case NifValue::tFloat:
	case NifValue::tHfloat:
		nif->set<float>( idx, copyVal.get<float>() );
		break;
	case NifValue::tColor3:
		nif->set<Color3>( idx, copyVal.get<Color3>() );
		break;
	case NifValue::tColor4:
	case NifValue::tByteColor4:
		nif->set<Color4>( idx, copyVal.get<Color4>() );
		break;
	case NifValue::tQuat:
	case NifValue::tQuatXYZW:
		nif->set<Quat>( idx, copyVal.get<Quat>() );
		break;
	case NifValue::tMatrix:
		nif->set<Matrix>( idx, copyVal.get<Matrix>() );
		break;
	case NifValue::tMatrix4:
		nif->set<Matrix4>( idx, copyVal.get<Matrix4>() );
		break;
	case NifValue::tString:
	case NifValue::tSizedString:
	case NifValue::tText:
	case NifValue::tShortString:
	case NifValue::tHeaderString:
	case NifValue::tLineString:
	case NifValue::tChar8String:
		nif->set<QString>( idx, copyVal.get<QString>() );
		break;
	default:
		// Return and do not push to Undo Stack
		return;
	}

	auto n = static_cast<NifModel*>(nif);
	if ( n )
		n->undoStack->push( new ChangeValueCommand( idx, dest, valueClipboard->getValue(), valueType, n ) );
}

void NifTreeView::paste()
{
	ChangeValueCommand::createTransaction();
	QModelIndexList idx = selectionModel()->selectedIndexes();
	for ( const auto i : idx ) {
		pasteTo( i );
	}
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

void NifTreeView::keyPressEvent( QKeyEvent * e )
{
	auto details = model()->inherits( "NifModel" );
	if ( details ) {
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

		if ( e->matches( QKeySequence::Copy ) ) {
			copy();
			// Clear the clipboard in case it holds a block to prevent conflicting behavior
			QApplication::clipboard()->clear();
			return;
		} else if ( e->matches( QKeySequence::Paste ) && valueClipboard->getValue().isValid() && !hasBlockCopied ) {
			// Do row paste if there is no block/branch copied and the NifValue is valid
			paste();
			return;
		}
	}

	SpellPtr spell = SpellBook::lookup( QKeySequence( e->modifiers() + e->key() ) );

	if ( spell ) {
		NifModel * nif = nullptr;
		NifProxyModel * proxy = nullptr;

		QPersistentModelIndex oldidx;

		// Clear this on any spell cast to prevent it overriding other paste behavior like block -> link row
		// TODO: Value clipboard does not get cleared when using the context menu. 
		valueClipboard->getValue().clear();

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

	auto mdl = static_cast<NifModel *>( nif );
	if ( mdl ) {
		// Auto-Expand Textures
		if ( mdl->inherits( current, "BSShaderTextureSet" ) ) {
			expand( current.child( 1, 0 ) );
		}
	}

	emit sigCurrentIndexChanged( currentIndex() );
}

void NifTreeView::scrollExpand( const QModelIndex & index )
{
	// this is a compromise between scrolling to the top, and scrolling the last child to the bottom
	scrollTo( index, PositionAtCenter );
}
