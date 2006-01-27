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
#include "nifproxy.h"

#include <QItemDelegate>

#include "nifdelegate.h"
#include "spellbook.h"

#include <QAction>
#include <QComboBox>
#include <QDebug>
#include <QEvent>
#include <QFrame>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QSpinBox>
#include <QToolButton>

class NifDelegate : public QItemDelegate
{
public:
	NifDelegate() : QItemDelegate() {}
	
	virtual bool editorEvent( QEvent * event, QAbstractItemModel * model, const QStyleOptionViewItem & option, const QModelIndex & index )
	{
		Q_ASSERT( event );
		Q_ASSERT( model );
		
		if ( event->type() == QEvent::MouseButtonRelease && static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton )
		{
			Spell * spell = SpellBook::lookup( model->data( index, Qt::UserRole ).toString() );
			if ( spell && ! spell->icon().isNull() )
			{
				int m = qMin( option.rect.width(), option.rect.height() );
				QRect iconRect( option.rect.x(), option.rect.y(), m, m );
				if ( iconRect.contains( static_cast<QMouseEvent*>(event)->pos() ) )
				{
					NifModel * nif = 0;
					QModelIndex buddy = index;
					
					if ( model->inherits( "NifModel" ) )
					{
						nif = static_cast<NifModel *>( model );
					}
					else if ( model->inherits( "NifProxyModel" ) )
					{
						NifProxyModel * proxy = static_cast<NifProxyModel*>( model );
						nif = static_cast<NifModel *>( proxy->model() );
						buddy = proxy->mapTo( index );
					}
					
					if ( nif && spell->isApplicable( nif, buddy ) )
						spell->cast( nif, buddy );
					
					//return true;
				}
			}
		}
		
		return false;
	}
	
	virtual void paint( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const
	{
		const QAbstractItemModel *model = index.model();
		if ( ! model ) return;
		
		QVariant data = model->data( index, Qt::DisplayRole );
		
		QString text;
		QString deco = model->data(index, Qt::DecorationRole).toString();
		
		if ( data.canConvert<NifValue>() )
			text = data.value<NifValue>().toString();
		else
			text = data.toString();
		
		QString user = model->data( index, Qt::UserRole ).toString();
		QIcon icon;
		
		if ( ! user.isEmpty() )
		{
			Spell * spell = SpellBook::lookup( user );
			if ( spell ) icon = spell->icon();
		}
		
		QStyleOptionViewItem opt = option;
		
		QRect textRect(0, 0, opt.fontMetrics.width(text), opt.fontMetrics.lineSpacing());
		
		QRect decoRect;
		
		if ( ! icon.isNull() )
		{
			int m = qMin( option.rect.width(), option.rect.height() );
			decoRect = QRect( option.rect.x(), option.rect.y(), m, m );
		}
		else if ( ! deco.isEmpty() )
			decoRect = QRect(0, 0, opt.fontMetrics.width(deco), opt.fontMetrics.lineSpacing());
		
		QRect dummy;
		doLayout( opt, &dummy, &decoRect, &textRect, false );
		
		QPalette::ColorGroup cg = option.state & QStyle::State_Enabled ? QPalette::Normal : QPalette::Disabled;
		
		QVariant color = model->data( index, Qt::BackgroundColorRole );
		if ( color.canConvert<QColor>() )
			painter->fillRect( option.rect, color.value<QColor>() );
		else if ( option.state & QStyle::State_Selected )
			painter->fillRect( option.rect, option.palette.brush( cg, QPalette::Highlight ) );
		
		painter->save();
		painter->setPen( opt.palette.color( cg, opt.state & QStyle::State_Selected ? QPalette::HighlightedText : QPalette::Text ) );
		painter->setFont( opt.font );
		
		if ( ! icon.isNull() )
			icon.paint( painter, decoRect );
		else if ( ! deco.isEmpty() )
			qt_format_text( opt.font, decoRect, opt.displayAlignment, deco, 0, 0, 0, 0, painter );
		
		if ( painter->fontMetrics().width( text ) > textRect.width() )
			text = elidedText( opt.fontMetrics, textRect.width(), opt.textElideMode, text );
		qt_format_text( opt.font, textRect, opt.displayAlignment, text, 0, 0, 0, 0, painter );
		
		drawFocus(painter, opt, textRect);
		
		painter->restore();
	}

	QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
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
	
	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
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
	setAutoFillBackground( true );
}

bool ValueEdit::canEdit( NifValue::Type t )
{
	return ( t == NifValue::tBool || t == NifValue::tByte || t == NifValue::tWord || t == NifValue::tInt || t == NifValue::tFlags
		|| t == NifValue::tLink || t == NifValue::tParent || t == NifValue::tFloat || t == NifValue::tString
		|| t == NifValue::tVector3 || t == NifValue::tVector2 || t == NifValue::tMatrix || t == NifValue::tQuat
		|| t == NifValue::tTriangle );
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
		case NifValue::tTriangle:
		{
			TriangleEdit * te = new TriangleEdit( this );
			te->setTriangle( v.get<Triangle>() );
			edit = te;
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
		case NifValue::tTriangle:
			val.set<Triangle>( qobject_cast<TriangleEdit*>( edit )->getTriangle() );
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

TriangleEdit::TriangleEdit( QWidget * parent ) : QWidget( parent )
{
	QHBoxLayout * lay = new QHBoxLayout;
	lay->setMargin( 0 );
	setLayout( lay );
	
	lay->addWidget( v1 = new QSpinBox );
	v1->setRange( 0, + 0xffff );
	lay->addWidget( v2 = new QSpinBox );
	v2->setRange( 0, + 0xffff );
	lay->addWidget( v3 = new QSpinBox );
	v3->setRange( 0, + 0xffff );
}

void TriangleEdit::setTriangle( const Triangle & t )
{
	v1->setValue( t[0] );
	v2->setValue( t[1] );
	v3->setValue( t[2] );
}


Triangle TriangleEdit::getTriangle() const
{
	return Triangle( v1->value(), v2->value(), v3->value() );
}

