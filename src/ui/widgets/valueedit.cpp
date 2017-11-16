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

#include "valueedit.h"

#include "ui/widgets/floatedit.h"

#include <QLabel>   // Inherited
#include <QSpinBox> // Inherited
#include <QAction>
#include <QFrame>
#include <QLayout>
#include <QLineEdit>
#include <QResizeEvent>
#include <QScrollBar>
#include <QTextCursor>
#include <QTextEdit>
#include <QToolButton>


//! \file valueedit.cpp Value edit widget implementations

ValueEdit::ValueEdit( QWidget * parent ) : QWidget( parent ), typ( NifValue::tNone ), edit( 0 )
{
	setAutoFillBackground( true );
}

bool ValueEdit::canEdit( NifValue::Type t )
{
	return t == NifValue::tByte || t == NifValue::tWord || t == NifValue::tInt || t == NifValue::tFlags
		|| t == NifValue::tLink || t == NifValue::tUpLink || t == NifValue::tFloat || t == NifValue::tText
		|| t == NifValue::tSizedString || t == NifValue::tLineString || t == NifValue::tChar8String
		|| t == NifValue::tShortString || t == NifValue::tStringIndex || t == NifValue::tString
		|| t == NifValue::tVector4 || t == NifValue::tVector3 || t == NifValue::tVector2
		|| t == NifValue::tColor3 || t == NifValue::tColor4 || t == NifValue::tByteColor4
		|| t == NifValue::tMatrix || t == NifValue::tQuat || t == NifValue::tQuatXYZW
		|| t == NifValue::tTriangle || t == NifValue::tShort || t == NifValue::tUInt || t == NifValue::tULittle32
		|| t == NifValue::tHfloat || t == NifValue::tHalfVector3 || t == NifValue::tByteVector3
		|| t == NifValue::tHalfVector2;
}

class CenterLabel final : public QLabel
{
public:
	CenterLabel( const QString & txt ) : QLabel( txt ) { setAlignment( Qt::AlignCenter ); }
	CenterLabel() : QLabel() { setAlignment( Qt::AlignCenter ); }
};


UnsignedValidator::UnsignedValidator( QObject * parent )
	: QValidator( parent )
{
}

QValidator::State UnsignedValidator::validate( QString & input, int & pos ) const
{
	if ( input.trimmed().isEmpty() || input.trimmed() == QLatin1String( "0x" ) )
		return Intermediate;

	bool ok;
	uint val = input.toUInt( &ok, 0 );

	if ( !ok || val > max )
		return Invalid;
	else if ( val < min )
		return Intermediate;

	return Acceptable;
}

void UnsignedValidator::setRange( uint minimum, uint maximum )
{
	min = minimum;
	max = maximum;
}


UIntSpinBox::UIntSpinBox( QWidget * parent ) : QSpinBox( parent )
{
	validator = new UnsignedValidator( this );
}

QValidator::State UIntSpinBox::validate( QString & text, int & pos ) const
{
	return validator->validate( text, pos );
}

void UIntSpinBox::setValue( uint value )
{
	QSpinBox::setValue( toInt( value ) );
}

QString UIntSpinBox::textFromValue( int value ) const
{
	return QString::number( toUInt( value ) );
}

int UIntSpinBox::valueFromText( const QString & text ) const
{
	bool ok;
	QString txt = text;
	uint newVal = txt.toUInt( &ok, 0 );

	if ( !ok && !(prefix().isEmpty() && suffix().isEmpty()) ) {
		newVal = cleanText().toUInt( &ok, 0 );
	}

	return toInt( newVal );
}


void ValueEdit::setValue( const NifValue & v )
{
	typ = v.type();

	if ( edit ) {
		delete edit;
		edit = nullptr;
		resize( this->baseSize() );
	}

	switch ( typ ) {
	case NifValue::tByte:
		{
			QSpinBox * be = new QSpinBox( this );
			be->setFrame( false );
			be->setRange( 0, 0xff );
			be->setValue( v.toCount() );
			edit = be;
		}
		break;
	case NifValue::tWord:
	case NifValue::tFlags:
		{
			QSpinBox * we = new QSpinBox( this );
			we->setFrame( false );
			we->setRange( 0, 0xffff );
			we->setValue( v.toCount() );
			edit = we;
		}
		break;
	case NifValue::tShort:
		{
			QSpinBox * we = new QSpinBox( this );
			we->setFrame( false );
			we->setRange( SHRT_MIN, SHRT_MAX );
			we->setValue( (short)v.toCount() );
			edit = we;
		}
		break;
	case NifValue::tInt:
		{
			QSpinBox * ie = new QSpinBox( this );
			ie->setFrame( false );
			ie->setRange( INT_MIN, INT_MAX );
			ie->setValue( (int)v.toCount() );
			edit = ie;
		}
		break;
	case NifValue::tStringIndex:
		{
			QSpinBox * ie = new QSpinBox( this );
			ie->setFrame( false );
			ie->setRange( -1, INT_MAX );
			ie->setValue( (int)v.toCount() );
			edit = ie;
		}
		break;
	case NifValue::tUInt:
	case NifValue::tULittle32:
		{
			UIntSpinBox * ie = new UIntSpinBox( this );
			ie->setFrame( false );
			ie->setRange( INT_MIN, INT_MAX );
			ie->setValue( v.toCount() );
			edit = ie;
		}
		break;
	case NifValue::tLink:
	case NifValue::tUpLink:
		{
			QLineEdit * le = new QLineEdit( this );
			int tmp = v.toLink();

			if ( tmp > 0 ) {
				le->setText( QString::number( tmp ) );
			}

			edit = le;
		}
		break;
	case NifValue::tFloat:
	case NifValue::tHfloat:
		{
			FloatEdit * fe = new FloatEdit( this );
			/*
			fe->setFrame(false);
			fe->setRange( -1e10, +1e10 );
			fe->setDecimals( 4 );
			*/
			fe->setValue( v.toFloat() );
			edit = fe;
		}
		break;
	case NifValue::tText:
	case NifValue::tSizedString:
		{
			TextEdit * te = new TextEdit( v.toString(), this );
			te->resize( size() );
			connect( te, &TextEdit::sigResized, this, &ValueEdit::childResized );
			edit = te;
		}
		break;
	case NifValue::tLineString:
	case NifValue::tShortString:
	case NifValue::tChar8String:
		{
			QLineEdit * le = new QLineEdit( this );
			le->setText( v.toString() );
			edit = le;
		}
		break;
	//case NifValue::tText:
	//{
	//	TextEdit * te = new TextEdit( v.toString(), this );
	//	te->setMinimumSize( width(), height() );
	//	te->setBaseSize( width(), height() * 5);
	//	edit = te;
	//}	break;
	case NifValue::tByteColor4:
		{
			ColorEdit * ce = new ColorEdit( this );
			ce->setColor4( v.get<ByteColor4>() );
			edit = ce;
		}
		break;
	case NifValue::tColor4:
		{
			ColorEdit * ce = new ColorEdit( this );
			ce->setColor4( v.get<Color4>() );
			edit = ce;
		}
		break;
	case NifValue::tColor3:
		{
			ColorEdit * ce = new ColorEdit( this );
			ce->setColor3( v.get<Color3>() );
			edit = ce;
		}
		break;
	case NifValue::tVector4:
		{
			VectorEdit * ve = new VectorEdit( this );
			ve->setVector4( v.get<Vector4>() );
			edit = ve;
		}
		break;
	case NifValue::tByteVector3:
		{
			VectorEdit * ve = new VectorEdit( this );
			ve->setVector3( v.get<ByteVector3>() );
			edit = ve;
		}
		break;
	case NifValue::tHalfVector3:
		{
			VectorEdit * ve = new VectorEdit( this );
			ve->setVector3( v.get<HalfVector3>() );
			edit = ve;
		}
		break;
	case NifValue::tVector3:
		{
			VectorEdit * ve = new VectorEdit( this );
			ve->setVector3( v.get<Vector3>() );
			edit = ve;
		}
		break;
	case NifValue::tHalfVector2:
		{
			VectorEdit * ve = new VectorEdit( this );
			ve->setVector2( v.get<HalfVector2>() );
			edit = ve;
		}
		break;
	case NifValue::tVector2:
		{
			VectorEdit * ve = new VectorEdit( this );
			ve->setVector2( v.get<Vector2>() );
			edit = ve;
		}
		break;
	case NifValue::tMatrix:
		{
			RotationEdit * re = new RotationEdit( this );
			re->setMatrix( v.get<Matrix>() );
			edit = re;
		}
		break;
	case NifValue::tQuat:
	case NifValue::tQuatXYZW:
		{
			RotationEdit * re = new RotationEdit( this );
			re->setQuat( v.get<Quat>() );
			edit = re;
		}
		break;
	case NifValue::tTriangle:
		{
			TriangleEdit * te = new TriangleEdit( this );
			te->setTriangle( v.get<Triangle>() );
			edit = te;
		}
		break;
	case NifValue::tString:
		{
			if ( /*???*/ false ) {
				QSpinBox * ie = new UIntSpinBox( this );
				ie->setFrame( false );
				ie->setValue( v.toCount() );
				edit = ie;
			} else {
				QLineEdit * le = new QLineEdit( this );
				le->setText( v.toString() );
				edit = le;
			}
		}
		break;
	case NifValue::tFilePath:
		{
			if ( /*???*/ false ) {
				QSpinBox * ie = new UIntSpinBox( this );
				ie->setFrame( false );
				ie->setValue( v.toCount() );
				edit = ie;
			} else {
				QLineEdit * le = new QLineEdit( this );
				le->setText( v.toString() );
				edit = le;
			}
		}
		break;
	default:
		edit = nullptr;
		break;
	}

	resizeEditor();

	setFocusProxy( edit );
}

NifValue ValueEdit::getValue() const
{
	NifValue val( typ );

	if ( edit )
		switch ( typ ) {
		case NifValue::tByte:
		case NifValue::tWord:
		case NifValue::tShort:
		case NifValue::tFlags:
		case NifValue::tInt:
		case NifValue::tUInt:
		case NifValue::tULittle32:
		case NifValue::tStringIndex:
			val.setCount( qobject_cast<QSpinBox *>( edit )->value() );
			break;
		case NifValue::tLink:
		case NifValue::tUpLink:
			{
				QString str = qobject_cast<QLineEdit *>( edit )->text();
				bool ok = false;
				int tmp = str.toInt( &ok );

				if ( ok == false || tmp < 0 ) {
					val.setLink( -1 );
				} else {
					val.setLink( tmp );
				}
			}
			break;
		case NifValue::tFloat:
		case NifValue::tHfloat:
			val.setFloat( qobject_cast<FloatEdit *>( edit )->value() );
			break;
		case NifValue::tLineString:
		case NifValue::tShortString:
		case NifValue::tChar8String:
			val.setFromString( qobject_cast<QLineEdit *>( edit )->text() );
			break;
		case NifValue::tSizedString:
		case NifValue::tText:
			val.setFromString( qobject_cast<QTextEdit *>( edit )->toPlainText() );
			break;
		case NifValue::tByteColor4:
			{
				auto col = qobject_cast<ColorEdit *>(edit)->getColor4();
				val.set<ByteColor4>( *static_cast<ByteColor4 *>(&col) );
				break;
			}
		case NifValue::tColor4:
			val.set<Color4>( qobject_cast<ColorEdit *>( edit )->getColor4() );
			break;
		case NifValue::tColor3:
			val.set<Color3>( qobject_cast<ColorEdit *>( edit )->getColor3() );
			break;
		case NifValue::tVector4:
			val.set<Vector4>( qobject_cast<VectorEdit *>( edit )->getVector4() );
			break;
		case NifValue::tByteVector3:
			{
				auto vec = qobject_cast<VectorEdit *>(edit)->getVector3();
				val.set<ByteVector3>( *static_cast<ByteVector3 *>(&vec) );
				break;
			}
		case NifValue::tHalfVector3:
			{
				auto vec = qobject_cast<VectorEdit *>(edit)->getVector3();
				val.set<HalfVector3>( *static_cast<HalfVector3 *>(&vec) );
				break;
			}
		case NifValue::tVector3:
			val.set<Vector3>( qobject_cast<VectorEdit *>( edit )->getVector3() );
			break;
		case NifValue::tHalfVector2:
			{
				auto vec = qobject_cast<VectorEdit *>(edit)->getVector2();
				val.set<HalfVector2>( *static_cast<HalfVector2 *>(&vec) );
				break;
			}
		case NifValue::tVector2:
			val.set<Vector2>( qobject_cast<VectorEdit *>( edit )->getVector2() );
			break;
		case NifValue::tMatrix:
			val.set<Matrix>( qobject_cast<RotationEdit *>( edit )->getMatrix() );
			break;
		case NifValue::tQuat:
		case NifValue::tQuatXYZW:
			val.set<Quat>( qobject_cast<RotationEdit *>( edit )->getQuat() );
			break;
		case NifValue::tTriangle:
			val.set<Triangle>( qobject_cast<TriangleEdit *>( edit )->getTriangle() );
			break;
		default:
			break;
		}



	return val;
}

void ValueEdit::resizeEditor()
{
	if ( edit ) {
		QSize sz = size();

		switch ( typ ) {
		case NifValue::tSizedString:
		case NifValue::tText:
			{
				TextEdit * te = (TextEdit *)edit;
				te->move( QPoint( 0, 0 ) );
				resize( size() );
				te->CalcSize();
			}
			break;

		default:
			{
				edit->move( QPoint( 0, 0 ) );
				edit->resize( sz );
				resize( sz );
			}
			break;
		}
	}
}

void ValueEdit::childResized( QResizeEvent * e )
{
	switch ( typ ) {
	case NifValue::tSizedString:
	case NifValue::tText:
		{
			//edit->move( QPoint( 1, 0 ) );
			resize( QSize( width(), e->size().height() ) );
		}
		break;
	default:
		break;
	}
}

void ValueEdit::resizeEvent( QResizeEvent * )
{
	resizeEditor();
}

ColorEdit::ColorEdit( QWidget * parent ) : ValueEdit( parent )
{
	QHBoxLayout * lay = new QHBoxLayout;
	lay->setMargin( 0 );
	lay->setSpacing( 0 );
	setLayout( lay );

	// Cast QDoubleSpinBox slot
	auto dsbValueChanged = static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged);

	lay->addWidget( new CenterLabel( "R" ), 1 );
	lay->addWidget( r = new QDoubleSpinBox, 5 );
	r->setDecimals( COLOR_DECIMALS );
	r->setRange( 0, FLT_MAX );
	r->setSingleStep( COLOR_STEP );
	connect( r, dsbValueChanged, this, &ColorEdit::sltChanged );
	lay->addWidget( new CenterLabel( "G" ), 1 );
	lay->addWidget( g = new QDoubleSpinBox, 5 );
	g->setDecimals( COLOR_DECIMALS );
	g->setRange( 0, FLT_MAX );
	g->setSingleStep( COLOR_STEP );
	connect( g, dsbValueChanged, this, &ColorEdit::sltChanged );
	lay->addWidget( new CenterLabel( "B" ), 1 );
	lay->addWidget( b = new QDoubleSpinBox, 5 );
	b->setDecimals( COLOR_DECIMALS );
	b->setRange( 0, FLT_MAX );
	b->setSingleStep( COLOR_STEP );
	connect( b, dsbValueChanged, this, &ColorEdit::sltChanged );
	lay->addWidget( al = new CenterLabel( "A" ), 1 );
	lay->addWidget( a = new QDoubleSpinBox, 5 );
	a->setDecimals( COLOR_DECIMALS );
	a->setRange( 0, FLT_MAX );
	a->setSingleStep( COLOR_STEP );
	connect( a, dsbValueChanged, this, &ColorEdit::sltChanged );

	setting = false;
	setFocusProxy( r );
}

void ColorEdit::setColor4( const Color4 & v )
{
	setting = true;
	r->setValue( v[0] );
	g->setValue( v[1] );
	b->setValue( v[2] );
	a->setValue( v[3] ); a->setVisible( true ); al->setVisible( true );
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
	if ( !setting )
		emit sigEdited();
}

Color4 ColorEdit::getColor4() const
{
	return Color4( r->value(), g->value(), b->value(), a->value() );
}

Color3 ColorEdit::getColor3() const
{
	return Color3( r->value(), g->value(), b->value() );
}

VectorEdit::VectorEdit( QWidget * parent ) : ValueEdit( parent )
{
	QHBoxLayout * lay = new QHBoxLayout( this );
	lay->setMargin( 0 );
	lay->setSpacing( 0 );

	CenterLabel * xl, * yl;

	// Cast QDoubleSpinBox slot
	auto dsbValueChanged = static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged);

	lay->addWidget( xl = new CenterLabel( "X" ), 1 );
	lay->addWidget( x = new QDoubleSpinBox, 5 );
	x->setDecimals( VECTOR_DECIMALS );
	x->setRange( -VECTOR_RANGE, +VECTOR_RANGE );
	connect( x, dsbValueChanged, this, &VectorEdit::sltChanged );
	lay->addWidget( yl = new CenterLabel( "Y" ), 1 );
	lay->addWidget( y = new QDoubleSpinBox, 5 );
	y->setDecimals( VECTOR_DECIMALS );
	y->setRange( -VECTOR_RANGE, +VECTOR_RANGE );
	connect( y, dsbValueChanged, this, &VectorEdit::sltChanged );
	lay->addWidget( zl = new CenterLabel( "Z" ), 1 );
	lay->addWidget( z = new QDoubleSpinBox, 5 );
	z->setDecimals( VECTOR_DECIMALS );
	z->setRange( -VECTOR_RANGE, +VECTOR_RANGE );
	connect( z, dsbValueChanged, this, &VectorEdit::sltChanged );
	lay->addWidget( wl = new CenterLabel( "W" ), 1 );
	lay->addWidget( w = new QDoubleSpinBox, 5 );
	w->setDecimals( VECTOR_DECIMALS );
	w->setRange( -VECTOR_RANGE, +VECTOR_RANGE );
	connect( w, dsbValueChanged, this, &VectorEdit::sltChanged );

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
	z->setValue( v[2] ); z->setVisible( true ); zl->setVisible( true );
	w->setValue( v[3] ); w->setVisible( true ); wl->setVisible( true );
	setting = false;
}

void VectorEdit::setVector3( const Vector3 & v )
{
	setting = true;
	x->setValue( v[0] );
	y->setValue( v[1] );
	z->setValue( v[2] ); z->setVisible( true ); zl->setVisible( true );
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
	if ( !setting )
		emit sigEdited();
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


RotationEdit::RotationEdit( QWidget * parent ) : ValueEdit( parent ), mode( mAuto ), setting( false )
{
	actMode = new QAction( this );
	connect( actMode, &QAction::triggered, this, &RotationEdit::switchMode );
	QToolButton * btMode = new QToolButton( this );
	btMode->setDefaultAction( actMode );

	QHBoxLayout * lay = new QHBoxLayout( this );
	lay->setMargin( 0 );
	lay->setSpacing( 0 );

	lay->addWidget( btMode, 2 );

	// Cast QDoubleSpinBox slot
	auto dsbValueChanged = static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged);

	for ( int x = 0; x < 4; x++ ) {
		lay->addWidget( l[x] = new CenterLabel, 1 );
		lay->addWidget( v[x] = new QDoubleSpinBox, 5 );
		connect( v[x], dsbValueChanged, this, &RotationEdit::sltChanged );
	}

	setFocusProxy( v[0] );

	setupMode();
}

void RotationEdit::switchMode()
{
	Matrix m = getMatrix();

	if ( mode == mAxis )
		mode = mEuler;
	else
		mode = mAxis;

	setupMode();

	setMatrix( m );
}

void RotationEdit::setupMode()
{
	switch ( mode ) {
	case mAuto:
	case mEuler:
		{
			actMode->setText( "Euler" );
			QStringList labs{ "Y", "P", "R" };

			for ( int x = 0; x < 4; x++ ) {
				l[x]->setText( labs.value( x ) );
				v[x]->setDecimals( ROTATION_COARSE );
				v[x]->setRange( -360, +360 );
				v[x]->setSingleStep( 1 );
				l[x]->setHidden( x == 3 );
				v[x]->setHidden( x == 3 );
			}
		}
		break;
	case mAxis:
		{
			actMode->setText( "Axis" );
			QStringList labs{ "A", "X", "Y", "Z" };

			for ( int x = 0; x < 4; x++ ) {
				l[x]->setText( labs.value( x ) );

				if ( x == 0 ) {
					v[x]->setDecimals( ROTATION_COARSE );
					v[x]->setRange( -360, +360 );
					v[x]->setSingleStep( 1 );
				} else {
					v[x]->setDecimals( ROTATION_FINE );
					v[x]->setRange( -1.0, +1.0 );
					v[x]->setSingleStep( 0.1 );
				}

				l[x]->setHidden( false );
				v[x]->setHidden( false );
			}
		}
		break;
	}
}

void RotationEdit::setMatrix( const Matrix & m )
{
	setting = true;

	if ( mode == mAuto ) {
		mode = mEuler;
		setupMode();
	}

	switch ( mode ) {
	case mAuto:
	case mEuler:
		{
			float Y, P, R;
			m.toEuler( Y, P, R );
			v[0]->setValue( Y / PI * 180 );
			v[1]->setValue( P / PI * 180 );
			v[2]->setValue( R / PI * 180 );
		}
		break;
	case mAxis:
		{
			Vector3 axis; float angle;
			m.toQuat().toAxisAngle( axis, angle );
			v[0]->setValue( angle / PI * 180 );

			for ( int x = 0; x < 3; x++ )
				v[x + 1]->setValue( axis[x] );
		}
		break;
	}

	setting = false;
}

void RotationEdit::setQuat( const Quat & q )
{
	setting = true;

	if ( mode == mAuto ) {
		mode = mEuler;
		setupMode();
	}

	Matrix m; m.fromQuat( q );
	setMatrix( m );

	setting = false;
}

Matrix RotationEdit::getMatrix() const
{
	switch ( mode ) {
	case mAuto:
	case mEuler:
		{
			Matrix m; m.fromEuler( v[0]->value() / 180 * PI, v[1]->value() / 180 * PI, v[2]->value() / 180 * PI );
			return m;
		};
	case mAxis:
		{
			Quat q;
			q.fromAxisAngle( Vector3( v[1]->value(), v[2]->value(), v[3]->value() ), v[0]->value() / 180 * PI );
			Matrix m;
			m.fromQuat( q );
			return m;
		};
	}

	return Matrix();
}

Quat RotationEdit::getQuat() const
{
	switch ( mode ) {
	case mAuto:
	case mEuler:
		{
			Matrix m; m.fromEuler( v[0]->value() / 180 * PI, v[1]->value() / 180 * PI, v[2]->value() / 180 * PI );
			return m.toQuat();
		}
	case mAxis:
		{
			Quat q;
			q.fromAxisAngle( Vector3( v[1]->value(), v[2]->value(), v[3]->value() ), v[0]->value() / 180 * PI );
			return q;
		}
	}

	return Quat();
}

void RotationEdit::sltChanged()
{
	if ( !setting )
		emit sigEdited();
}


TriangleEdit::TriangleEdit( QWidget * parent ) : ValueEdit( parent )
{
	QHBoxLayout * lay = new QHBoxLayout( this );
	lay->setMargin( 0 );
	lay->setSpacing( 0 );

	lay->addWidget( v1 = new QSpinBox );
	v1->setRange( 0, +0xffff );
	lay->addWidget( v2 = new QSpinBox );
	v2->setRange( 0, +0xffff );
	lay->addWidget( v3 = new QSpinBox );
	v3->setRange( 0, +0xffff );

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


TextEdit::TextEdit( const QString & str, QWidget * parent ) : QTextEdit( parent )
{
	setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	//setFrameShape(QFrame::NoFrame);
	setFrameShadow( QFrame::Plain );
	setLineWrapMode( QTextEdit::NoWrap );
	setAcceptRichText( false );
	setAutoFormatting( QTextEdit::AutoNone );
	setTabChangesFocus( true );
	setPlainText( str );
	CalcSize();
	connect( this, &TextEdit::textChanged, this,  &TextEdit::sltTextChanged );

	QTextCursor cursor = this->textCursor();
	cursor.select( QTextCursor::LineUnderCursor );
	setTextCursor( cursor );
}

void TextEdit::resizeEvent( QResizeEvent * e )
{
	QTextEdit::resizeEvent( e );
	emit sigResized( e );
}

void TextEdit::keyPressEvent( QKeyEvent * e )
{
	if ( e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return || e->key() == Qt::Key_F4 ) {
		if ( e->modifiers() == Qt::AltModifier || e->modifiers() == Qt::ShiftModifier ) {
			// Eat the Alt and Shift Modifiers and 'send' an enter instead
			e->accept();

			QKeyEvent newEvent(
			    e->type(),
			    e->key(),
			    Qt::NoModifier,
			    e->text(),
			    e->isAutoRepeat(),
			    e->count()
			);
			QTextEdit::keyPressEvent( &newEvent );
			return;
		}

		if ( e->modifiers() == Qt::NoModifier ) {
			e->ignore();
			return;
		}
	}

	QTextEdit::keyPressEvent( e );
}

void TextEdit::sltTextChanged()
{
	CalcSize();
}

void TextEdit::CalcSize()
{
	QSize sz = this->size();
	QString text = this->toPlainText();
	int lines = text.count( QLatin1Char( '\n' ) ) + 1;

	if ( lines > 5 )
		lines = 5;

	int ht = fontMetrics().lineSpacing() * lines + 10; // 10 extra lines because single lines seem to need a bit more than lineSpacing()

	if ( lines == 1 ) {
		setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
		sz.setHeight( ht );
		resize( sz );
	} else if ( ht > sz.height() ) {
		setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded );
		sz.setHeight( ht );
		resize( sz );
	}
}
