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

#include "valueedit.h"

#include <QFrame>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QTextEdit>

ValueEdit::ValueEdit( QWidget * parent ) : QWidget( parent ), typ( NifValue::tNone ), edit( 0 )
{
	setAutoFillBackground( true );
}

bool ValueEdit::canEdit( NifValue::Type t )
{
	return t == NifValue::tByte || t == NifValue::tWord || t == NifValue::tInt || t == NifValue::tFlags
		|| t == NifValue::tLink || t == NifValue::tUpLink || t == NifValue::tFloat || t == NifValue::tText
		|| t == NifValue::tString || t == NifValue::tFilePath || t == NifValue::tLineString || t == NifValue::tShortString 
		|| t == NifValue::tVector4 || t == NifValue::tVector3 || t == NifValue::tVector2
		|| t == NifValue::tColor3 || t == NifValue::tColor4
		|| t == NifValue::tMatrix || t == NifValue::tQuat || t == NifValue::tQuatXYZW 
		|| t == NifValue::tTriangle || t == NifValue::tShort || t == NifValue::tUInt 
      ;
}

class CenterLabel : public QLabel
{
public:
	CenterLabel( const QString & txt ) : QLabel( txt ) { setAlignment( Qt::AlignCenter ); }
};

class UIntSpinBox : public QSpinBox
{
public:
	UIntSpinBox( QWidget * parent ) : QSpinBox( parent ) { setRange( INT_MIN, INT_MAX ); }

protected:
	QString textFromValue( int i ) const
	{
		return QString::number( (unsigned int) i );
	}
	
	int valueFromText( const QString & text ) const
	{
		return text.toUInt();
	}
};

void ValueEdit::setValue( const NifValue & v )
{
	typ = v.type();
	
	if ( edit )
		delete edit;
	
	switch ( typ )
	{
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
      case NifValue::tShort:
         {	
            QSpinBox * we = new QSpinBox( this );
            we->setFrame(false);
            we->setRange( SHRT_MIN, SHRT_MAX );
            we->setValue( (short)v.toCount() );
            edit = we;
         }	break;
      case NifValue::tInt:
         {	
            QSpinBox * ie = new QSpinBox( this );
            ie->setFrame(false);
            ie->setRange( INT_MIN, INT_MAX );
            ie->setValue( (int)v.toCount() );
            edit = ie;
         }	break;
      case NifValue::tUInt:
		{	
			QSpinBox * ie = new UIntSpinBox( this );
			ie->setFrame(false);
			ie->setValue( v.toCount() );
			edit = ie;
		}	break;
		case NifValue::tLink:
		case NifValue::tUpLink:
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
		case NifValue::tFilePath:
		case NifValue::tLineString:
		case NifValue::tShortString:
		{	
			QLineEdit * le = new QLineEdit( this );
			le->setText( v.toString() );
			edit = le;
		}	break;
		case NifValue::tText:
		{
			QTextEdit * te = new QTextEdit( this );
			te->setPlainText( v.toString() );
			edit = te;
		}	break;
		case NifValue::tColor4:
		{
			ColorEdit * ce = new ColorEdit( this );
			ce->setColor4( v.get<Color4>() );
			edit = ce;
		}	break;
		case NifValue::tColor3:
		{
			ColorEdit * ce = new ColorEdit( this );
			ce->setColor3( v.get<Color3>() );
			edit = ce;
		}	break;
		case NifValue::tVector4:
		{
			VectorEdit * ve = new VectorEdit( this );
			ve->setVector4( v.get<Vector4>() );
			edit = ve;
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
		case NifValue::tQuatXYZW:
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
	
	setFocusProxy( edit );
}

NifValue ValueEdit::getValue() const
{
	NifValue val( typ );
	
	if ( edit ) switch ( typ )
	{
		case NifValue::tByte:
		case NifValue::tWord:
		case NifValue::tShort:
		case NifValue::tFlags:
		case NifValue::tInt:
		case NifValue::tUInt:
			val.setCount( qobject_cast<QSpinBox*>( edit )->value() );
			break;
		case NifValue::tLink:
		case NifValue::tUpLink:
			val.setLink( qobject_cast<QSpinBox*>( edit )->value() );
			break;
		case NifValue::tFloat:
			val.setFloat( qobject_cast<QDoubleSpinBox*>( edit )->value() );
			break;
		case NifValue::tString:
		case NifValue::tFilePath:
		case NifValue::tLineString:
		case NifValue::tShortString:
			val.fromString( qobject_cast<QLineEdit*>( edit )->text() );
			break;
		case NifValue::tText:
			val.fromString( qobject_cast<QTextEdit*>( edit )->toPlainText() );
			break;
		case NifValue::tColor4:
			val.set<Color4>( qobject_cast<ColorEdit*>( edit )->getColor4() );
			break;
		case NifValue::tColor3:
			val.set<Color3>( qobject_cast<ColorEdit*>( edit )->getColor3() );
			break;
		case NifValue::tVector4:
			val.set<Vector4>( qobject_cast<VectorEdit*>( edit )->getVector4() );
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
		case NifValue::tQuatXYZW:
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


ColorEdit::ColorEdit( QWidget * parent ) : QWidget( parent )
{
	QHBoxLayout * lay = new QHBoxLayout;
	lay->setMargin( 0 );
	lay->setSpacing( 0 );
	setLayout( lay );
	
	lay->addWidget( new CenterLabel( "R" ), 1 );
	lay->addWidget( r = new QDoubleSpinBox, 5 );
	r->setDecimals( 3 );
	r->setRange( 0, 1 );
	r->setSingleStep( 0.01 );
	connect( r, SIGNAL( valueChanged( double ) ), this, SLOT( sltChanged() ) );
	lay->addWidget( new CenterLabel( "G" ), 1 );
	lay->addWidget( g = new QDoubleSpinBox, 5 );
	g->setDecimals( 3 );
	g->setRange( 0, 1 );
	g->setSingleStep( 0.01 );
	connect( g, SIGNAL( valueChanged( double ) ), this, SLOT( sltChanged() ) );
	lay->addWidget( new CenterLabel( "B" ), 1 );
	lay->addWidget( b = new QDoubleSpinBox, 5 );
	b->setDecimals( 3 );
	b->setRange( 0, 1 );
	b->setSingleStep( 0.01 );
	connect( b, SIGNAL( valueChanged( double ) ), this, SLOT( sltChanged() ) );
	lay->addWidget( al = new CenterLabel( "A" ), 1 );
	lay->addWidget( a = new QDoubleSpinBox, 5 );
	a->setDecimals( 3 );
	a->setRange( 0, 1 );
	a->setSingleStep( 0.01 );
	connect( a, SIGNAL( valueChanged( double ) ), this, SLOT( sltChanged() ) );
	
	setting = false;
	setFocusProxy( r );
}

void ColorEdit::setColor4( const Color4 & v )
{
	setting = true;
	r->setValue( v[0] );
	g->setValue( v[1] );
	b->setValue( v[2] );
	a->setValue( v[3] ); a->setShown( true ); al->setShown( true );
	setting = false;
}

void ColorEdit::setColor3( const Color3 & v )
{
	setting = true;
	r->setValue( v[0] );
	g->setValue( v[1] );
	b->setValue( v[2] );
	a->setValue( 1.0 ); a->setHidden( true ); al->setHidden( true );
	setting = false;
}

void ColorEdit::sltChanged()
{
	if ( ! setting ) emit sigEdited();
}

Color4 ColorEdit::getColor4() const
{
	return Color4( r->value(), g->value(), b->value(), a->value() );
}

Color3 ColorEdit::getColor3() const
{
	return Color3( r->value(), g->value(), b->value() );
}


VectorEdit::VectorEdit( QWidget * parent ) : QWidget( parent )
{
	QHBoxLayout * lay = new QHBoxLayout( this );
	lay->setMargin( 0 );
	lay->setSpacing( 0 );
	
	CenterLabel * xl, * yl;
	
	lay->addWidget( xl = new CenterLabel( "X" ), 1 );
	lay->addWidget( x = new QDoubleSpinBox, 5 );
	x->setDecimals( 4 );
	x->setRange( - 100000000, + 100000000 );
	connect( x, SIGNAL( valueChanged( double ) ), this, SLOT( sltChanged() ) );
	lay->addWidget( yl = new CenterLabel( "Y" ), 1 );
	lay->addWidget( y = new QDoubleSpinBox, 5 );
	y->setDecimals( 4 );
	y->setRange( - 100000000, + 100000000 );
	connect( y, SIGNAL( valueChanged( double ) ), this, SLOT( sltChanged() ) );
	lay->addWidget( zl = new CenterLabel( "Z" ), 1 );
	lay->addWidget( z = new QDoubleSpinBox, 5 );
	z->setDecimals( 4 );
	z->setRange( - 100000000, + 100000000 );
	connect( z, SIGNAL( valueChanged( double ) ), this, SLOT( sltChanged() ) );
	lay->addWidget( wl = new CenterLabel( "W" ), 1 );
	lay->addWidget( w = new QDoubleSpinBox, 5 );
	w->setDecimals( 4 );
	w->setRange( - 100000000, + 100000000 );
	connect( w, SIGNAL( valueChanged( double ) ), this, SLOT( sltChanged() ) );
	
	/*
	xl->setBuddy( x );
	yl->setBuddy( y );
	zl->setBuddy( z );
	wl->setBuddy( w );
	*/
	
	setting = false;
	setFocusProxy( x );
}

void VectorEdit::setVector4( const Vector4 & v )
{
	setting = true;
	x->setValue( v[0] );
	y->setValue( v[1] );
	z->setValue( v[2] ); z->setShown( true ); zl->setShown( true );
	w->setValue( v[3] ); w->setShown( true ); wl->setShown( true );
	setting = false;
}

void VectorEdit::setVector3( const Vector3 & v )
{
	setting = true;
	x->setValue( v[0] );
	y->setValue( v[1] );
	z->setValue( v[2] ); z->setShown( true ); zl->setShown( true );
	w->setValue( 0.0 ); w->setHidden( true ); wl->setHidden( true );
	setting = false;
}

void VectorEdit::setVector2( const Vector2 & v )
{
	setting = true;
	x->setValue( v[0] );
	y->setValue( v[1] );
	z->setValue( 0.0 ); z->setHidden( true ); zl->setHidden( true );
	w->setValue( 0.0 ); w->setHidden( true ); wl->setHidden( true );
	setting = false;
}

void VectorEdit::sltChanged()
{
	if ( ! setting ) emit sigEdited();
}

Vector4 VectorEdit::getVector4() const
{
	return Vector4( x->value(), y->value(), z->value(), w->value() );
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
	QHBoxLayout * lay = new QHBoxLayout( this );
	lay->setMargin( 0 );
	lay->setSpacing( 0 );

	lay->addWidget( new CenterLabel( "Y" ), 1 );
	lay->addWidget( y = new QDoubleSpinBox, 5 );
	y->setDecimals( 1 );
	y->setRange( - 360, + 360 );
	connect( y, SIGNAL( valueChanged( double ) ), this, SLOT( sltChanged() ) );
	lay->addWidget( new CenterLabel( "P" ), 1 );
	lay->addWidget( p = new QDoubleSpinBox, 5 );
	p->setDecimals( 1 );
	p->setRange( - 360, + 360 );
	connect( p, SIGNAL( valueChanged( double ) ), this, SLOT( sltChanged() ) );
	lay->addWidget( new CenterLabel( "R" ), 1 );
	lay->addWidget( r = new QDoubleSpinBox, 5 );
	r->setDecimals( 1 );
	r->setRange( - 360, + 360 );
	connect( r, SIGNAL( valueChanged( double ) ), this, SLOT( sltChanged() ) );
	
	setting = false;
	setFocusProxy( y );
}

void RotationEdit::setMatrix( const Matrix & m )
{
	setting = true;
	float Y, P, R;
	m.toEuler( Y, P, R );
	y->setValue( Y / PI * 180 );
	p->setValue( P / PI * 180 );
	r->setValue( R / PI * 180 );
	setting = false;
}

void RotationEdit::setQuat( const Quat & q )
{
	setting = true;
	Matrix m; m.fromQuat( q );
	float Y, P, R;
	m.toEuler( Y, P, R );
	y->setValue( Y / PI * 180 );
	p->setValue( P / PI * 180 );
	r->setValue( R / PI * 180 );
	setting = false;
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

void RotationEdit::sltChanged()
{
	if ( ! setting ) emit sigEdited();
}

TriangleEdit::TriangleEdit( QWidget * parent ) : QWidget( parent )
{
	QHBoxLayout * lay = new QHBoxLayout( this );
	lay->setMargin( 0 );
	lay->setSpacing( 0 );
	
	lay->addWidget( v1 = new QSpinBox );
	v1->setRange( 0, + 0xffff );
	lay->addWidget( v2 = new QSpinBox );
	v2->setRange( 0, + 0xffff );
	lay->addWidget( v3 = new QSpinBox );
	v3->setRange( 0, + 0xffff );
	
	setFocusProxy( v1 );
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

