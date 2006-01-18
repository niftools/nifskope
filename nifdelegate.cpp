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
			&& model->data( index, Qt::EditRole ).value<NifValue>().isColor() )
		{
			NifValue val = model->data( index, Qt::EditRole ).value<NifValue>();
			
			int m = qMin( option.rect.width(), option.rect.height() );
			QRect iconRect( option.rect.x(), option.rect.y(), m, m );
			if ( ! iconRect.contains( static_cast<QMouseEvent*>(event)->pos() ) )
				return true;
			
			if ( val.type() == NifValue::tColor3 )
				val.set<Color3>( Color3( ColorWheel::choose( val.toColor(), 0 ) ) );
			else if ( val.type() == NifValue::tColor4 )
				val.set<Color4>( Color4( ColorWheel::choose( val.toColor(), 0 ) ) );
			
			return model->setData( index, val.toVariant(), Qt::EditRole );
		}
		
		return QItemDelegate::editorEvent( event, model, option, index );
	}
	
	virtual void paint( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const
	{
		const QAbstractItemModel *model = index.model();
		if ( ! model ) return;
		
		QVariant data = model->data( index, Qt::DisplayRole );
		QString text;
		
		if ( data.canConvert<NifValue>() )
		{
			NifValue val = data.value<NifValue>();
			
			if ( val.isColor() )
			{
				painter->fillRect( option.rect, val.toColor() );
				if ( model->flags( index ) & Qt::ItemIsEditable )
				{
					int m = qMin( option.rect.width(), option.rect.height() );
					icon.paint( painter, QRect( option.rect.x(), option.rect.y(), m, m ), Qt::AlignCenter );
				}
				drawFocus( painter, option, option.rect );
				return;
			}
			
			text = val.toString();
		}
		else
			text = data.toString();
		
		QStyleOptionViewItem opt = option;
		
		QRect textRect(0, 0, opt.fontMetrics.width(text), opt.fontMetrics.lineSpacing());
		
		// decoration is a string
		QString deco = model->data(index, Qt::DecorationRole).toString();
		QRect decoRect(0, 0, opt.fontMetrics.width(deco), opt.fontMetrics.lineSpacing());
		
		QRect dummy;
		doLayout(opt, &dummy, &decoRect, &textRect, false);
		
		QPalette::ColorGroup cg = option.state & QStyle::State_Enabled ? QPalette::Normal : QPalette::Disabled;
		
		if ( option.state & QStyle::State_Selected )
			painter->fillRect( option.rect, option.palette.brush( cg, QPalette::Highlight ) );
		
		painter->save();
		painter->setPen( opt.palette.color( cg, opt.state & QStyle::State_Selected ? QPalette::HighlightedText : QPalette::Text ) );
		painter->setFont( opt.font );
		
		qt_format_text( opt.font, decoRect, opt.displayAlignment, deco, 0, 0, 0, 0, painter );
		
		if ( painter->fontMetrics().width( text ) > textRect.width() )
			text = elidedText( opt.fontMetrics, textRect.width(), opt.textElideMode, text );
		qt_format_text( opt.font, textRect, opt.displayAlignment, text, 0, 0, 0, 0, painter );
		
		drawFocus(painter, opt, textRect);
		
		painter->restore();
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
		
		return ( decoRect | textRect ).size();
	}

	QWidget * createEditor( QWidget * parent, const QStyleOptionViewItem &, const QModelIndex & index ) const
	{
		if ( ! index.isValid() )
			return 0;
		
		QVariant v = index.model()->data(index, Qt::EditRole);
		QWidget * w = 0;
		
		if ( v.canConvert<NifValue>() )
		{
			NifValue nv = v.value<NifValue>();
			if ( ValueEdit::canEdit( nv.type() ) )
				w = new ValueEdit( parent );
		}
		else if ( v.type() == QVariant::String )
		{
			QLineEdit *le = new QLineEdit(parent);
			le->setFrame(false);
			w = le;
		}
		if ( w ) w->installEventFilter( const_cast<NifDelegate *>( this ) );
		return w;
	}
	
	void setEditorData(QWidget *editor, const QModelIndex &index) const
	{
		ValueEdit * vedit = qobject_cast<ValueEdit*>( editor );
		QLineEdit * ledit = qobject_cast<QLineEdit*>( editor );
		QVariant	v = index.model()->data( index, Qt::EditRole );
		
		if ( v.canConvert<NifValue>() && vedit )
		{
			vedit->setValue( v.value<NifValue>() );
		}
		else if ( ledit )
		{
			ledit->setText( v.toString() );
		}
	}
	
	void setModelData(QWidget *editor, QAbstractItemModel *model,
						const QModelIndex &index) const
	{
		Q_ASSERT(model);
		ValueEdit * vedit = qobject_cast<ValueEdit*>( editor );
		QLineEdit * ledit = qobject_cast<QLineEdit*>( editor );
		QVariant v;
		if ( vedit )
		{
			v.setValue( vedit->getValue() );
			model->setData( index, v, Qt::EditRole );
		}
		else if ( ledit )
		{
			v.setValue( ledit->text() );
			model->setData( index, v, Qt::EditRole );
		}
	}
};

QAbstractItemDelegate * NifModel::createDelegate()
{
	return new NifDelegate;
}

ValueEdit::ValueEdit( QWidget * parent ) : QWidget( parent ), typ( NifValue::tNone ), edit( 0 )
{
}

bool ValueEdit::canEdit( NifValue::Type t )
{
	return ( t == NifValue::tBool || t == NifValue::tByte || t == NifValue::tWord || t == NifValue::tInt || t == NifValue::tFlags
		|| t == NifValue::tLink || t == NifValue::tParent || t == NifValue::tFloat || t == NifValue::tString
		|| t == NifValue::tVector3 || t == NifValue::tVector2 || t == NifValue::tMatrix || t == NifValue::tQuat );
}

void ValueEdit::setValue( const NifValue & v )
{
	typ = v.type();
	
	if ( edit )
		delete edit;
	
	switch ( typ )
	{
		case NifValue::tBool:
		{
			QComboBox * cb = new QComboBox( this );
			cb->setFrame(false);
			cb->addItem("no");
			cb->addItem("yes");
			cb->setCurrentIndex( v.toCount() ? 1 : 0 );
			edit = cb;
		}	break;
		case NifValue::tByte:
		{
			QSpinBox * be = new QSpinBox( this );
			be->setFrame(false);
			be->setRange( 0, 0xff );
			be->setValue( v.toCount() );
			edit = be;
		}	break;
		case NifValue::tWord:
		case NifValue::tFlags:
		{	
			QSpinBox * we = new QSpinBox( this );
			we->setFrame(false);
			we->setRange( 0, 0xffff );
			we->setValue( v.toCount() );
			edit = we;
		}	break;
		case NifValue::tInt:
		{	
			QSpinBox * ie = new QSpinBox( this );
			ie->setFrame(false);
			ie->setRange( 0, INT_MAX );
			ie->setValue( v.toCount() );
			edit = ie;
		}	break;
		case NifValue::tLink:
		case NifValue::tParent:
		{	
			QSpinBox * le = new QSpinBox( this );
			le->setFrame(false);
			le->setRange( -1, 0xffff );
			le->setValue( v.toLink() );
			edit = le;
		}	break;
		case NifValue::tFloat:
		{	
			QDoubleSpinBox * fe = new QDoubleSpinBox( this );
			fe->setFrame(false);
			fe->setRange( -1e10, +1e10 );
			fe->setDecimals( 4 );
			fe->setValue( v.toFloat() );
			edit = fe;
		}	break;
		case NifValue::tString:
		{	
			QLineEdit * le = new QLineEdit( this );
			le->setText( v.toString() );
			edit = le;
		}	break;
		case NifValue::tVector3:
		{
			VectorEdit * ve = new VectorEdit( this );
			ve->setVector3( v.get<Vector3>() );
			edit = ve;
		}	break;
		case NifValue::tVector2:
		{	
			VectorEdit * ve = new VectorEdit( this );
			ve->setVector2( v.get<Vector2>() );
			edit = ve;
		}	break;
		case NifValue::tMatrix:
		{	
			RotationEdit * re = new RotationEdit( this );
			re->setMatrix( v.get<Matrix>() );
			edit = re;
		}	break;
		case NifValue::tQuat:
		{
			RotationEdit * re = new RotationEdit( this );
			re->setQuat( v.get<Quat>() );
			edit = re;
		}	break;
		default:
			edit = 0;
			break;
	}
	
	resizeEditor();
}

NifValue ValueEdit::getValue() const
{
	NifValue val( typ );
	
	switch ( typ )
	{
		case NifValue::tBool:
			val.setCount( qobject_cast<QComboBox*>( edit )->currentIndex() );
			break;
		case NifValue::tByte:
		case NifValue::tWord:
		case NifValue::tFlags:
		case NifValue::tInt:
			val.setCount( qobject_cast<QSpinBox*>( edit )->value() );
			break;
		case NifValue::tLink:
		case NifValue::tParent:
			val.setLink( qobject_cast<QSpinBox*>( edit )->value() );
			break;
		case NifValue::tFloat:
			val.setFloat( qobject_cast<QDoubleSpinBox*>( edit )->value() );
			break;
		case NifValue::tString:
			val.fromString( qobject_cast<QLineEdit*>( edit )->text() );
			break;
		case NifValue::tVector3:
			val.set<Vector3>( qobject_cast<VectorEdit*>( edit )->getVector3() );
			break;
		case NifValue::tVector2:
			val.set<Vector2>( qobject_cast<VectorEdit*>( edit )->getVector2() );
			break;
		case NifValue::tMatrix:
			val.set<Matrix>( qobject_cast<RotationEdit*>( edit )->getMatrix() );
			break;
		case NifValue::tQuat:
			val.set<Quat>( qobject_cast<RotationEdit*>( edit )->getQuat() );
			break;
		default:
			break;
	}
	
	return val;
}

void ValueEdit::resizeEditor()
{
	if ( edit )
	{
		edit->move( QPoint( 0, 0 ) );
		edit->resize( size() );
	}
}

void ValueEdit::resizeEvent( QResizeEvent * )
{
	resizeEditor();
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

void VectorEdit::setVector3( const Vector3 & v )
{
	x->setValue( v[0] );
	y->setValue( v[1] );
	z->setValue( v[2] ); z->setShown( true );
}

void VectorEdit::setVector2( const Vector2 & v )
{
	x->setValue( v[0] );
	y->setValue( v[1] );
	z->setValue( 0.0 ); z->setHidden( true );
}

Vector3 VectorEdit::getVector3() const
{
	return Vector3( x->value(), y->value(), z->value() );
}

Vector2 VectorEdit::getVector2() const
{
	return Vector2( x->value(), y->value() );
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
	Matrix m; m.fromQuat( q );
	float Y, P, R;
	m.toEuler( Y, P, R );
	y->setValue( Y / PI * 180 );
	p->setValue( P / PI * 180 );
	r->setValue( R / PI * 180 );
}

Matrix RotationEdit::getMatrix() const
{
	Matrix m; m.fromEuler( y->value() / 180 * PI, p->value() / 180 * PI, r->value() / 180 * PI );
	return m;
}

Quat RotationEdit::getQuat() const
{
	Matrix m; m.fromEuler( y->value() / 180 * PI, p->value() / 180 * PI, r->value() / 180 * PI );
	return m.toQuat();
}
