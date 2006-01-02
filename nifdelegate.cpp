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

#include "nifmodel.h"
#include "niftypes.h"

#include <QItemDelegate>

#include "nifdelegate.h"
#include "popup.h"

#include <QAction>
#include <QComboBox>
#include <QEvent>
#include <QFrame>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QSpinBox>
#include <QToolButton>

/* XPM */
static const char * const hsv42_xpm[] = {
"32 32 43 1",
" 	c None",
".	c #3200FF","+	c #0024FF","@	c #7600FF","#	c #0045FF","$	c #A000FF",
"%	c #FF0704","&	c #015AFF","*	c #FF0031","=	c #FF0058","-	c #FF0074",
";	c #FF0086",">	c #006DFF",",	c #D700FF","'	c #FF00A3",")	c #FF00C5",
"!	c #FC00DE","~	c #FC00FA","{	c #0086FF","]	c #FF3700","^	c #00A7FF",
"/	c #FF6000","(	c #00C3FF","_	c #00FF32",":	c #00DDFF","<	c #FF8D00",
"[	c #00FF55","}	c #23FF00","|	c #00FF72","1	c #00FF88","2	c #00FFA0",
"3	c #00FFBB","4	c #00FFD7","5	c #00F9F8","6	c #FFA900","7	c #60FF00",
"8	c #83FF00","9	c #FFC200","0	c #A3FF00","a	c #FFD800","b	c #C0FF00",
"c	c #FBF100","d	c #E6FF00",
"            7778800b            ",
"         }}}}778800bbdd         ",
"       }}}}}}777800bbddcc       ",
"      }}}}}7}778800bdddcca      ",
"     __}}}}}}77880bbddccca9     ",
"    [___}}}}}}7780bbddcaa996    ",
"   [[[___}}}}77880bddccaa9666   ",
"  ||[[[___}}}}7780bddca9966<<   ",
"  |1|[[[__}}}}7880bdcca966<<</  ",
" 211|||[[__}}}}78bdcca966<</<// ",
" 22211||[[[}}}778bdca96<<<///// ",
" 3222121||[__}}70bda96<<</////] ",
"3333322211|[__}78dc96<////]]]]]]",
"444433332221|_}70d9<<///]]]]]]]%",
"4444444433322|[}ba<//]]]]]]%%%%%",
"555555454544442[c/]%%%%%%%%%%%%%",
"5555555555:5::({!=*%%%%%%%%%%%%%",
":5:5:::::(((^{&.$);-==******%*%%",
":::::((((^^{>&+.@~)';-===*******",
":(((((^^^{{>#+..@,~)';;-====*=* ",
" (^^^^^{{>>#+..@@,~!)'';;--==== ",
" ^^^^{{{>&##+...$$,~!)'';;---== ",
" ^^{{{>>&##....@@$,,~!))'';;--  ",
"  {{>>>&#++....@$$,,~~!)'''';;  ",
"  {>>&&##++....@@$,,~~!!))'''   ",
"   >&&##++.....@@$$,,~~!)))''   ",
"    &#+++......@@$$,,~~~!)))    ",
"     #+++.....@@@$$$,,~~!!!     ",
"      ++.......@$$$,,,~~~!      ",
"        ......@@@$$$$,,~        ",
"         ......@@$$,,,          ",
"            .@@@@$$             "};

class NifDelegate : public QItemDelegate
{
	QIcon icon;
public:
	NifDelegate() : QItemDelegate(), icon( hsv42_xpm )	{}
	
	virtual bool editorEvent( QEvent * event, QAbstractItemModel * model, const QStyleOptionViewItem & option, const QModelIndex & index )
	{
		Q_ASSERT( event );
		Q_ASSERT( model );
		
		if ( event->type() == QEvent::MouseButtonRelease && model->flags( index ) & Qt::ItemIsEditable
			&& model->data( index, Qt::EditRole ).type() == QVariant::Color )
		{
			int m = qMin( option.rect.width(), option.rect.height() );
			QRect iconRect( option.rect.x(), option.rect.y(), m, m );
			if ( iconRect.contains( static_cast<QMouseEvent*>(event)->pos() ) )
				return model->setData( index, ColorWheel::choose( model->data( index, Qt::EditRole ).value<QColor>(), 0 ), Qt::EditRole );
			else
				return true;
		}
		
		return QItemDelegate::editorEvent( event, model, option, index );
	}
	
	virtual void paint( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const
	{
		const QAbstractItemModel *model = index.model();
		if ( ! model ) return;
		
		QVariant data = model->data( index, Qt::DisplayRole );
		
		if ( data.type() == QVariant::Color )
		{
			painter->fillRect( option.rect, data.value<QColor>() );
			if ( model->flags( index ) & Qt::ItemIsEditable )
			{
				int m = qMin( option.rect.width(), option.rect.height() );
				icon.paint( painter, QRect( option.rect.x(), option.rect.y(), m, m ), Qt::AlignCenter );
			}
			drawFocus( painter, option, option.rect );
		}
		else
		{
			QStyleOptionViewItem opt = option;
			
			QString text = data.toString();
			QRect textRect(0, 0, opt.fontMetrics.width(text), opt.fontMetrics.lineSpacing());
			
			// decoration is a string
			QString deco = model->data(index, Qt::DecorationRole).toString();
			QRect decoRect(0, 0, opt.fontMetrics.width(deco), opt.fontMetrics.lineSpacing());
			
			QRect dummy;
			doLayout(opt, &dummy, &decoRect, &textRect, false);
			
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
			drawDeco(painter, opt, decoRect, deco);
			drawDisplay(painter, opt, textRect, text);
			drawFocus(painter, opt, textRect);
		}
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
		QVariant v = index.model()->data(index, Qt::EditRole);
		
		QWidget * w = 0;
		if ( v.canConvert<Vector>() )
			w = new VectorEdit( parent );
		else if ( v.canConvert<Quat>() || v.canConvert<Matrix>() )
			w = new RotationEdit( parent );
		else
		{
			switch ( v.type() )
			{
				case QVariant::Bool:
				{
					QComboBox *cb = new QComboBox(parent);
					cb->setFrame(false);
					cb->addItem("False");
					cb->addItem("True");
					w = cb;
				}	break;
				case QVariant::UInt:
				{
					QSpinBox *sb = new QSpinBox(parent);
					sb->setFrame(false);
					sb->setMaximum(INT_MAX);
					w = sb;
				}	break;
				case QVariant::Int:
				{
					QSpinBox *sb = new QSpinBox(parent);
					sb->setFrame(false);
					sb->setMinimum(INT_MIN);
					sb->setMaximum(INT_MAX);
					w = sb;
				}	break;
				case QVariant::Double:
				{
					QDoubleSpinBox *sb = new QDoubleSpinBox(parent);
					sb->setFrame(false);
					sb->setDecimals( 4 );
					sb->setRange( - 100000000, + 100000000 );
					w = sb;
				}	break;
				case QVariant::String:
				{
					QLineEdit *le = new QLineEdit(parent);
					le->setFrame(false);
					w = le;
				}	break;
				case QVariant::Color:
				default:
					w = 0;
					break;
			}
		}
		if ( w ) w->installEventFilter(const_cast<NifDelegate *>(this));
		return w;
	}
	
	void setEditorData(QWidget *editor, const QModelIndex &index) const
	{
		QVariant	v = index.model()->data( index, Qt::EditRole );
		QByteArray	n = valuePropertyName( v );
		if ( !n.isEmpty() )
			editor->setProperty(n, v);
	}
	
	void setModelData(QWidget *editor, QAbstractItemModel *model,
						const QModelIndex &index) const
	{
		Q_ASSERT(model);
		QByteArray n = valuePropertyName( model->data(index, Qt::EditRole) );
		if ( !n.isEmpty() )
			model->setData( index, editor->property( n ), Qt::EditRole );
	}
	
	QByteArray valuePropertyName( const QVariant & v ) const
	{
		if ( v.canConvert<Vector>() )
			return "vector";
		else if ( v.canConvert<Quat>() )
			return "quat";
		else if ( v.canConvert<Matrix>() )
			return "matrix";
		else
		{
			switch ( v.type() )
			{
				case QVariant::Bool:
					return "currentItem";
				case QVariant::UInt:
				case QVariant::Int:
				case QVariant::Double:
					return "value";
				case QVariant::String:
				default:
					return "text";
			}
		}
	}
};

QAbstractItemDelegate * NifModel::createDelegate()
{
	return new NifDelegate;
}


VectorEdit::VectorEdit( QWidget * parent ) : QWidget( parent )
{
	QHBoxLayout * lay = new QHBoxLayout;
	lay->setMargin( 0 );
	setLayout( lay );
	
	lay->addWidget( x = new QDoubleSpinBox );
	//x->setFrame(false);
	x->setDecimals( 4 );
	x->setRange( - 100000000, + 100000000 );
	x->setPrefix( "X " );
	lay->addWidget( y = new QDoubleSpinBox );
	//y->setFrame(false);
	y->setDecimals( 4 );
	y->setRange( - 100000000, + 100000000 );
	y->setPrefix( "Y " );
	lay->addWidget( z = new QDoubleSpinBox );
	//z->setFrame(false);
	z->setDecimals( 4 );
	z->setRange( - 100000000, + 100000000 );
	z->setPrefix( "Z " );
}

void VectorEdit::setVector( const Vector & v )
{
	x->setValue( v[0] );
	y->setValue( v[1] );
	z->setValue( v[2] );
}

Vector VectorEdit::getVector() const
{
	return Vector( x->value(), y->value(), z->value() );
}

RotationEdit::RotationEdit( QWidget * parent ) : QWidget( parent )
{
	QHBoxLayout * lay = new QHBoxLayout;
	lay->setMargin( 0 );
	setLayout( lay );
	
	lay->addWidget( y = new QDoubleSpinBox );
	//y->setFrame(false);
	y->setDecimals( 1 );
	y->setRange( - 360, + 360 );
	y->setPrefix( "Y " );
	lay->addWidget( p = new QDoubleSpinBox );
	//p->setFrame(false);
	p->setDecimals( 1 );
	p->setRange( - 360, + 360 );
	p->setPrefix( "P " );
	lay->addWidget( r = new QDoubleSpinBox );
	//r->setFrame(false);
	r->setDecimals( 1 );
	r->setRange( - 360, + 360 );
	r->setPrefix( "R " );
}

void RotationEdit::setMatrix( const Matrix & m )
{
	float Y, P, R;
	m.toEuler( Y, P, R );
	y->setValue( Y / PI * 180 );
	p->setValue( P / PI * 180 );
	r->setValue( R / PI * 180 );
}

void RotationEdit::setQuat( const Quat & q )
{
	Matrix m; m = q;
	float Y, P, R;
	m.toEuler( Y, P, R );
	y->setValue( Y / PI * 180 );
	p->setValue( P / PI * 180 );
	r->setValue( R / PI * 180 );
}

Matrix RotationEdit::getMatrix() const
{
	return Matrix::fromEuler( y->value() / 180 * PI, p->value() / 180 * PI, r->value() / 180 * PI );
}

Quat RotationEdit::getQuat() const
{
	return Matrix::fromEuler( y->value() / 180 * PI, p->value() / 180 * PI, r->value() / 180 * PI ).toQuat();
}
