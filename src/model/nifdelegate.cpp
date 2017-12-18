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

#include "spellbook.h"
#include "model/kfmmodel.h"
#include "model/nifmodel.h"
#include "model/nifproxymodel.h"
#include "model/undocommands.h"
#include "ui/widgets/valueedit.h"
#include "ui/widgets/nifcheckboxlist.h"

#include <QItemDelegate> // Inherited
#include <QComboBox>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QListView>


//! @file nifdelegate.cpp NifDelegate

extern void qt_format_text( const QFont & font, const QRectF & _r,
                            int tf, const QString & str, QRectF * brect,
                            int tabstops, int * tabarray, int tabarraylen,
                            QPainter * painter );

class NifDelegate final : public QItemDelegate
{
	SpellBookPtr book;

public:
	NifDelegate( QObject * p, SpellBookPtr sb = 0 ) : QItemDelegate( p ), book( sb )
	{
	}

	virtual bool editorEvent( QEvent * event, QAbstractItemModel * model, const QStyleOptionViewItem & option, const QModelIndex & index ) override final
	{
		Q_ASSERT( event );
		Q_ASSERT( model );

		// Ignore indices which are not editable
		//	since this QItemDelegate bypasses the flags.
		if ( !(model->flags( index ) & Qt::ItemIsEditable) )
			return false;

		switch ( event->type() ) {
		case QEvent::MouseButtonPress:
		case QEvent::MouseButtonRelease:

			if ( static_cast<QMouseEvent *>(event)->button() == Qt::LeftButton
			     && decoRect( option ).contains( static_cast<QMouseEvent *>(event)->pos() ) )
			{
				// Spell Icons in Value column
				SpellPtr spell = SpellBook::lookup( model->data( index, Qt::UserRole ).toString() );
				if ( spell && !spell->icon().isNull() ) {
					// Spell Icon click
					if ( event->type() == QEvent::MouseButtonRelease ) {
						NifModel * nif = 0;
						QModelIndex buddy = index;

						if ( model->inherits( "NifModel" ) ) {
							nif = static_cast<NifModel *>( model );
						} else if ( model->inherits( "NifProxyModel" ) ) {
							NifProxyModel * proxy = static_cast<NifProxyModel *>( model );
							nif = static_cast<NifModel *>( proxy->model() );
							buddy = proxy->mapTo( index );
						}

						// Cast Spell for icon which was clicked
						if ( nif && spell->isApplicable( nif, buddy ) ) {
							if ( book )
								book->cast( nif, buddy, spell );
							else
								spell->cast( nif, buddy );
						}
					}

					return true;
				}
			}

			break;
		case QEvent::MouseButtonDblClick:

			if ( static_cast<QMouseEvent *>(event)->button() == Qt::LeftButton ) {
				QVariant v = model->data( index, Qt::EditRole );

				if ( v.canConvert<NifValue>() ) {
					NifValue nv = v.value<NifValue>();

					// Yes/No toggle for bool types
					if ( nv.type() == NifValue::tBool ) {
						nv.set<int>( !nv.get<int>() );
						model->setData( index, nv.toVariant(), Qt::EditRole );
						return true;
					}
				}
			}

			break;
		default:
			break;
		}

		return false;
	}

	virtual void paint( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const override final
	{
		int namerole = (index.isValid() && index.column() == 0) ? Qt::DisplayRole : NifSkopeDisplayRole;

		QString text = index.data( namerole ).toString();
		QString deco = index.data( Qt::DecorationRole ).toString();
		QString user = index.data( Qt::UserRole ).toString();
		QIcon icon;

		if ( !user.isEmpty() ) {
			// Find the icon for this Spell if one exists
			SpellPtr spell = SpellBook::lookup( user );
			if ( spell )
				icon = spell->icon();
		}

		QStyleOptionViewItem opt = option;

		QRect tRect = opt.rect;
		QRect dRect;

		if ( !icon.isNull() || !deco.isEmpty() ) {
			dRect = decoRect( opt );
			tRect = textRect( opt );
		}

		opt.state |= QStyle::State_Active;
		QPalette::ColorGroup cg = ( opt.state & QStyle::State_Enabled ) ? QPalette::Normal : QPalette::Disabled;

		// Color the field background if the value type is a color
		//	Otherwise normal behavior
		QVariant color = index.data( Qt::BackgroundColorRole );
		if ( color.canConvert<QColor>() )
			painter->fillRect( option.rect, color.value<QColor>() );
		else if ( option.state & QStyle::State_Selected )
			painter->fillRect( option.rect, option.palette.brush( cg, QPalette::Highlight ) );

		painter->save();
		painter->setPen( opt.palette.color( cg, opt.state & QStyle::State_Selected ? QPalette::HighlightedText : QPalette::Text ) );
		painter->setFont( opt.font );

		if ( !icon.isNull() )
			icon.paint( painter, dRect );
		else if ( !deco.isEmpty() )
			painter->drawText( dRect, opt.decorationAlignment, deco );

		if ( !text.isEmpty() ) {
			drawDisplay( painter, opt, tRect, text );
			drawFocus( painter, opt, tRect );
		}

		painter->restore();
	}

	QSize sizeHint( const QStyleOptionViewItem & option, const QModelIndex & index ) const override final
	{
		QString text = index.data( NifSkopeDisplayRole ).toString();
		auto height = option.fontMetrics.lineSpacing() * (text.count( QLatin1Char( '\n' ) ) + 1);
		// Increase height by 25%
		height *= 1.25;

		return {option.fontMetrics.width( text ), height};
	}

	QWidget * createEditor( QWidget * parent, const QStyleOptionViewItem &, const QModelIndex & index ) const override final
	{
		if ( !index.isValid() )
			return nullptr;

		QVariant v  = index.data( Qt::EditRole );
		QWidget * w = 0;

		if ( v.canConvert<NifValue>() ) {
			NifValue nv = v.value<NifValue>();

			if ( nv.isCount() && index.column() == NifModel::ValueCol ) {
				NifValue::EnumType type = NifValue::enumType( index.sibling( index.row(), NifModel::TypeCol ).data( NifSkopeDisplayRole ).toString() );

				if ( type == NifValue::eFlags ) {
					w = new NifCheckBoxList( parent );
				} else if ( type == NifValue::eDefault ) {
					QComboBox * c = new QComboBox( parent );
					w = c;
					c->setEditable( true );
				}
			}

			if ( !w && ValueEdit::canEdit( nv.type() ) )
				w = new ValueEdit( parent );
		} else if ( v.type() == QVariant::String ) {
			QLineEdit * le = new QLineEdit( parent );
			le->setFrame( false );
			w = le;
		}

		if ( w )
			w->installEventFilter( const_cast<NifDelegate *>( this ) );

		return w;
	}

	void setEditorData( QWidget * editor, const QModelIndex & index ) const override final
	{
		ValueEdit * vedit = qobject_cast<ValueEdit *>( editor );
		QComboBox * cedit = qobject_cast<QComboBox *>( editor );
		QLineEdit * ledit = qobject_cast<QLineEdit *>( editor );
		QVariant v = index.data( Qt::EditRole );

		if ( vedit && v.canConvert<NifValue>() ) {
			vedit->setValue( v.value<NifValue>() );
		} else if ( cedit && v.canConvert<NifValue>() && v.value<NifValue>().isCount() ) {
			cedit->clear();
			QString t = index.sibling( index.row(), NifModel::TypeCol ).data( NifSkopeDisplayRole ).toString();
			const NifValue::EnumOptions & eo = NifValue::enumOptionData( t );
			quint32 value = v.value<NifValue>().toCount();
			QMapIterator<quint32, QPair<QString, QString> > it( eo.o );

			while ( it.hasNext() ) {
				it.next();
				bool ok = false;

				if ( eo.t == NifValue::eFlags ) {
					cedit->addItem( it.value().first, ok = ( value & ( 1 << it.key() ) ) );
				} else {
					cedit->addItem( it.value().first, ok = ( value == it.key() ) );
				}
			}

			cedit->setEditText( NifValue::enumOptionName( t, value ) );

			if ( eo.t == NifValue::eFlags ) {
				// Using setCurrentIndex on bitflag enums causes the QComboBox
				// to break when only one option is selected. It returns something
				// other than -1 and in doing so checked option's UI is inactive.
				// See: https://github.com/niftools/nifskope/issues/51
				cedit->setCurrentIndex( -1 );
			} else {
				cedit->setCurrentIndex( cedit->findText( NifValue::enumOptionName( t, value ) ) );
			}
				

		} else if ( ledit ) {
			ledit->setText( v.toString() );
		}
	}

	void setModelData( QWidget * editor, QAbstractItemModel * model, const QModelIndex & index ) const override final
	{
		Q_ASSERT( model );
		ValueEdit * vedit = qobject_cast<ValueEdit *>( editor );
		QComboBox * cedit = qobject_cast<QComboBox *>( editor );
		QLineEdit * ledit = qobject_cast<QLineEdit *>( editor );
		QVariant v;

		if ( vedit ) {
			v.setValue( vedit->getValue() );

			// Value is unchanged, do not push to Undo Stack or call setData()
			if ( v == index.data( Qt::EditRole ) )
				return;

			if ( model->inherits( "NifModel" ) ) {
				auto valueType = model->sibling( index.row(), 0, index ).data().toString();

				ChangeValueCommand::createTransaction();
				auto nif = static_cast<NifModel *>(model);
				nif->undoStack->push( new ChangeValueCommand( index, v, vedit->getValue().toString(), valueType, nif ) );
			}

			model->setData( index, v, Qt::EditRole );
		} else if ( cedit ) {
			QString t  = index.sibling( index.row(), NifModel::TypeCol ).data( NifSkopeDisplayRole ).toString();
			QVariant v = index.data( Qt::EditRole );
			bool ok;
			quint32 x = NifValue::enumOptionValue( t, cedit->currentText(), &ok );

			if ( !ok )
				x = cedit->currentText().toUInt();

			if ( v.canConvert<NifValue>() ) {
				NifValue nv = v.value<NifValue>();
				nv.setCount( x );
				v.setValue( nv );

				// Value is unchanged, do not push to Undo Stack or call setData()
				if ( v == index.data( Qt::EditRole ) )
					return;

				if ( model->inherits( "NifModel" ) ) {
					auto valueType = model->sibling( index.row(), 0, index ).data().toString();

					auto nif = static_cast<NifModel *>(model);
					nif->undoStack->push( new ToggleCheckBoxListCommand( index, v, valueType, nif ) );
				}

				model->setData( index, v, Qt::EditRole );
			}
		} else if ( ledit ) {
			v.setValue( ledit->text() );
			model->setData( index, v, Qt::EditRole );
		}
	}

	QRect decoRect( const QStyleOptionViewItem & opt ) const
	{
		// allways upper left
		return QRect( opt.rect.topLeft(), opt.decorationSize );
	}

	QRect textRect( const QStyleOptionViewItem & opt ) const
	{
		return QRect( QPoint( opt.rect.x() + opt.decorationSize.width(), opt.rect.y() ),
		              QSize( opt.rect.width() - opt.decorationSize.width(), opt.rect.height() )
		);
	}
};

QAbstractItemDelegate * NifModel::createDelegate( QObject * parent, SpellBookPtr book )
{
	return new NifDelegate( parent, book );
}

QAbstractItemDelegate * KfmModel::createDelegate( QObject * p )
{
	return new NifDelegate( p );
}
