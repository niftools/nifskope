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

#include <QItemDelegate>

#include <QPainter>

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>

class NifDelegate : public QItemDelegate
{
public:
	virtual void paint( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const
	{	// this was riped from the QItemDelegate example
		Q_ASSERT(index.isValid());
		const QAbstractItemModel *model = index.model();
		Q_ASSERT(model);
		
		QStyleOptionViewItem opt = option;
		
		QString text = model->data(index, Qt::DisplayRole).toString();
		QRect textRect(0, 0, opt.fontMetrics.width(text), opt.fontMetrics.lineSpacing());
		
		// decoration is a string
		QString deco = model->data(index, Qt::DecorationRole).toString();
		QRect decoRect(0, 0, opt.fontMetrics.width(deco), opt.fontMetrics.lineSpacing());
		
		QRect checkRect;
		/*
		bool isChecked = true;
		if ( index.column() == NifModel::CondCol )
		{
			const NifModel * nif = qobject_cast<const NifModel*>( index.model() );
			isChecked = nif->evalCondition( index );
			checkRect = check(opt, opt.rect, isChecked );
		}
		*/
		
		doLayout(opt, &checkRect, &decoRect, &textRect, false);
		
		// draw the background color
		if (option.showDecorationSelected && (option.state & QStyle::State_Selected)) {
			QPalette::ColorGroup cg = option.state & QStyle::State_Enabled
									  ? QPalette::Normal : QPalette::Disabled;
			painter->fillRect(option.rect, option.palette.brush(cg, QPalette::Highlight));
		} else {
			QVariant value = model->data(index, Qt::BackgroundColorRole);
			if (value.isValid() && qvariant_cast<QColor>(value).isValid())
				painter->fillRect(option.rect, qvariant_cast<QColor>(value));
		}
		
		// draw the item
		//if ( index.column() == NifModel::CondCol )
		//	drawCheck(painter, opt, checkRect, isChecked ? Qt::Checked : Qt::Unchecked );
		drawDeco(painter, opt, decoRect, deco);
		drawDisplay(painter, opt, textRect, text);
		drawFocus(painter, opt, textRect);
	}

	QSize sizeHint(const QStyleOptionViewItem &option,
								  const QModelIndex &index) const
	{
		Q_ASSERT(index.isValid());
		const QAbstractItemModel *model = index.model();
		Q_ASSERT(model);
		
		QString deco = model->data( index, Qt::DecorationRole ).toString();
		QString text = model->data( index, Qt::DisplayRole ).toString();
		
		QRect decoRect(0, 0, option.fontMetrics.width(deco), option.fontMetrics.lineSpacing());
		QRect textRect(0, 0, option.fontMetrics.width(text), option.fontMetrics.lineSpacing());
		QRect checkRect;
		doLayout(option, &checkRect, &decoRect, &textRect, true);
		
		return (decoRect|textRect).size();
	}

	void drawDeco(QPainter *painter, const QStyleOptionViewItem &option,
									const QRect &rect, const QString &text) const
	{
		QPen pen = painter->pen();
		painter->setPen(option.palette.color(option.state & QStyle::State_Enabled ? QPalette::Normal : QPalette::Disabled, ( option.showDecorationSelected && (option.state & QStyle::State_Selected) ? QPalette::HighlightedText : QPalette::Text)) );
		QFont font = painter->font();
		painter->setFont(option.font);
		painter->drawText(rect, option.displayAlignment, text);
		painter->setFont(font);
		painter->setPen(pen);	
	}
	
	QWidget * createEditor(QWidget *parent,
										 const QStyleOptionViewItem &,
										 const QModelIndex &index) const
	{
		if (!index.isValid())
			return 0;
		QVariant::Type type = index.model()->data(index, Qt::EditRole).type();
		
		QWidget * w = 0;
		
		switch (type)
		{
			case QVariant::Bool:
			{
				QComboBox *cb = new QComboBox(parent);
				cb->setFrame(false);
				cb->addItem("False");
				cb->addItem("True");
				w = cb;
			} break;
			case QVariant::UInt:
			{
				QSpinBox *sb = new QSpinBox(parent);
				sb->setFrame(false);
				sb->setMaximum(INT_MAX);
				w = sb;
			} break;
			case QVariant::Int:
			{
				QSpinBox *sb = new QSpinBox(parent);
				sb->setFrame(false);
				sb->setMinimum(INT_MIN);
				sb->setMaximum(INT_MAX);
				w = sb;
			} break;
			case QVariant::Double:
			{
				QDoubleSpinBox *sb = new QDoubleSpinBox(parent);
				sb->setFrame(false);
				sb->setDecimals( 4 );
				sb->setRange( - 100000000, + 100000000 );
				w = sb;
			} break;
			case QVariant::StringList:
			{
				QComboBox *cb = new QComboBox(parent);
				cb->setFrame(false);
				cb->setEditable( true );
				w = cb;
			} break;
			case QVariant::Pixmap:
				w = new QLabel(parent);
				break;
			case QVariant::String:
			default:
			{
				// the default editor is a lineedit
				QLineEdit *le = new QLineEdit(parent);
				le->setFrame(false);
				w = le;
			}
		}
		if ( w ) w->installEventFilter(const_cast<NifDelegate *>(this));
		return w;
	}
	
	void setEditorData(QWidget *editor, const QModelIndex &index) const
	{
		QVariant	v = index.model()->data( index, Qt::EditRole );
		QByteArray	n = valuePropertyName( v.type() );
		if ( !n.isEmpty() )
			editor->setProperty(n, v);
	}
	
	void setModelData(QWidget *editor, QAbstractItemModel *model,
						const QModelIndex &index) const
	{
		Q_ASSERT(model);
		QByteArray n = valuePropertyName( model->data(index, Qt::EditRole).type() );
		if ( !n.isEmpty() )
			model->setData( index, editor->property( n ), Qt::EditRole );
	}
	
	QByteArray valuePropertyName( QVariant::Type type ) const
	{
		switch (type)
		{
			case QVariant::Bool:
				return "currentItem";
			case QVariant::UInt:
			case QVariant::Int:
			case QVariant::Double:
				return "value";
			case QVariant::Date:
				return "date";
			case QVariant::Time:
				return "time";
			case QVariant::DateTime:
				return "dateTime";
			case QVariant::StringList:
				return "contents";
			case QVariant::String:
			default:
				// the default editor is a lineedit
				return "text";
		}
	}
};

QAbstractItemDelegate * NifModel::createDelegate()
{
	return new NifDelegate;
}
