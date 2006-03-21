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

ValueEdit::ValueEdit( QWidget * parent ) : QWidget( parent ), typ( NifValue::tNone ), edit( 0 )
{
	setAutoFillBackground( true );
}

bool ValueEdit::canEdit( NifValue::Type t )
{
	return ( t == NifValue::tByte || t == NifValue::tWord || t == NifValue::tInt || t == NifValue::tFlags
		|| t == NifValue::tLink || t == NifValue::tUpLink || t == NifValue::tFloat || t == NifValue::tString
		|| t == NifValue::tVector3 || t == NifValue::tVector2 || t == NifValue::tMatrix || t == NifValue::tQuat
		|| t == NifValue::tTriangle || t == NifValue::tFilePath || t == NifValue::tHeaderString );
}

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
		case NifValue::tInt:
		{	
			QSpinBox * ie = new QSpinBox( this );
			ie->setFrame(false);
			ie->setRange( 0, INT_MAX );
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
		case NifValue::tHeaderString:
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
		case NifValue::tByte:
		case NifValue::tWord:
		case NifValue::tFlags:
		case NifValue::tInt:
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
		case NifValue::tHeaderString:
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
	connect( x, SIGNAL( valueChanged( double ) ), this, SLOT( sltChanged() ) );
	lay->addWidget( y = new QDoubleSpinBox );
	//y->setFrame(false);
	y->setDecimals( 4 );
	y->setRange( - 100000000, + 100000000 );
	y->setPrefix( "Y " );
	connect( y, SIGNAL( valueChanged( double ) ), this, SLOT( sltChanged() ) );
	lay->addWidget( z = new QDoubleSpinBox );
	//z->setFrame(false);
	z->setDecimals( 4 );
	z->setRange( - 100000000, + 100000000 );
	z->setPrefix( "Z " );
	connect( z, SIGNAL( valueChanged( double ) ), this, SLOT( sltChanged() ) );
	
	setting = false;
}

void VectorEdit::setVector3( const Vector3 & v )
{
	setting = true;
	x->setValue( v[0] );
	y->setValue( v[1] );
	z->setValue( v[2] ); z->setShown( true );
	setting = false;
}

void VectorEdit::setVector2( const Vector2 & v )
{
	setting = true;
	x->setValue( v[0] );
	y->setValue( v[1] );
	z->setValue( 0.0 ); z->setHidden( true );
	setting = false;
}

void VectorEdit::sltChanged()
{
	if ( ! setting ) emit sigEdited();
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
	connect( y, SIGNAL( valueChanged( double ) ), this, SLOT( sltChanged() ) );
	lay->addWidget( p = new QDoubleSpinBox );
	//p->setFrame(false);
	p->setDecimals( 1 );
	p->setRange( - 360, + 360 );
	p->setPrefix( "P " );
	connect( p, SIGNAL( valueChanged( double ) ), this, SLOT( sltChanged() ) );
	lay->addWidget( r = new QDoubleSpinBox );
	//r->setFrame(false);
	r->setDecimals( 1 );
	r->setRange( - 360, + 360 );
	r->setPrefix( "R " );
	connect( r, SIGNAL( valueChanged( double ) ), this, SLOT( sltChanged() ) );
	
	setting = false;
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

