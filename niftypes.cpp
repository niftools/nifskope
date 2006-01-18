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

#include "niftypes.h"
#include "nifmodel.h"

#include <QHash>

void Matrix::fromQuat( const Quat & q )
{
	float fTx  = ((float)2.0)*q[1];
	float fTy  = ((float)2.0)*q[2];
	float fTz  = ((float)2.0)*q[3];
	float fTwx = fTx*q[0];
	float fTwy = fTy*q[0];
	float fTwz = fTz*q[0];
	float fTxx = fTx*q[1];
	float fTxy = fTy*q[1];
	float fTxz = fTz*q[1];
	float fTyy = fTy*q[2];
	float fTyz = fTz*q[2];
	float fTzz = fTz*q[3];
	
	m[0][0] = (float)1.0-(fTyy+fTzz);
	m[0][1] = fTxy-fTwz;
	m[0][2] = fTxz+fTwy;
	m[1][0] = fTxy+fTwz;
	m[1][1] = (float)1.0-(fTxx+fTzz);
	m[1][2] = fTyz-fTwx;
	m[2][0] = fTxz-fTwy;
	m[2][1] = fTyz+fTwx;
	m[2][2] = (float)1.0-(fTxx+fTyy);
}

Quat Matrix::toQuat() const
{
	Quat q;
	
	float trace = m[0][0] + m[1][1] + m[2][2];
	float root;
	
	if (trace > 0.0)
	{
		root = sqrt( trace + 1.0 );
		q[0] = root / 2.0;
		root = 0.5 / root;
		q[1] = ( m[2][1] - m[1][2] ) * root;
		q[2] = ( m[0][2] - m[2][0] ) * root;
		q[3] = ( m[1][0] - m[0][1] ) * root;
	}
	else
	{
		int i = ( m[1][1] > m[0][0] ? 1 : 0 );
		if ( m[2][2] > m[i][i] )
			i = 2;
		const int next[3] = { 1, 2, 0 };
		int j = next[i];
		int k = next[j];

		root = sqrt( m[i][i] - m[j][j] - m[k][k] + 1.0 );
		q[i+1] = root / 2;
		root = 0.5 / root;
		q[0]   = ( m[k][j] - m[j][k] ) * root;
		q[j+1] = ( m[j][i] + m[i][j] ) * root;
		q[k+1] = ( m[k][i] + m[i][k] ) * root;
	}
	return q;
}

void Matrix::fromEuler( float x, float y, float z )
{
	x = x;
	y = y;
	z = z;
	
	float sinX = sin( x );
	float cosX = cos( x );
	float sinY = sin( y );
	float cosY = cos( y );
	float sinZ = sin( z );
	float cosZ = cos( z );
	
	m[0][0] = cosY * cosZ;
	m[0][1] = - cosY * sinZ;
	m[0][2] = sinY;
	m[1][0] = sinX * sinY * cosZ + sinZ * cosX;
	m[1][1] = cosX * cosZ - sinX * sinY * sinZ;
	m[1][2] = - sinX * cosY;
	m[2][0] = sinX * sinZ - cosX * sinY * cosZ;
	m[2][1] = cosX * sinY * sinZ + sinX * cosZ;
	m[2][2] = cosX * cosY;
}

bool Matrix::toEuler( float & x, float & y, float & z ) const
{
	if ( m[0][2] < 1.0 )
	{
		if ( m[0][2] > - 1.0 )
		{
			x = atan2( - m[1][2], m[2][2] );
			y = asin( m[0][2] );
			z = atan2( - m[0][1], m[0][0] );
			return true;
		}
		else
		{
			x = - atan2( - m[1][0], m[1][1] );
			y = - PI / 2;
			z = 0.0;
			return false;
		}
	}
	else
	{
		x = atan2( m[1][0], m[1][1] );
		y = PI / 2;
		z = 0.0;
		return false;
	}
}

QString Matrix::toHtml() const
{
	return QString( "<table>" )
		+ QString( "<tr><td>%1</td><td>%2</td><td>%3</td></tr>" ).arg( m[0][0], 0, 'f', 4 ).arg( m[0][1], 0, 'f', 4 ).arg( m[0][2], 0, 'f', 4 )
		+ QString( "<tr><td>%1</td><td>%2</td><td>%3</td></tr>" ).arg( m[1][0], 0, 'f', 4 ).arg( m[1][1], 0, 'f', 4 ).arg( m[1][2], 0, 'f', 4 )
		+ QString( "<tr><td>%1</td><td>%2</td><td>%3</td></tr>" ).arg( m[2][0], 0, 'f', 4 ).arg( m[2][1], 0, 'f', 4 ).arg( m[2][2], 0, 'f', 4 )
		+ QString( "</table>" );
}


/*
 *  NifValue
 */

QHash<QString,NifValue::Type>	NifValue::typeMap;

void NifValue::initialize()
{
	typeMap.clear();
	
	typeMap.insert( "bool", NifValue::tBool );
	typeMap.insert( "byte", NifValue::tByte );
	typeMap.insert( "word", NifValue::tWord );
	typeMap.insert( "short", NifValue::tWord );
	typeMap.insert( "int", NifValue::tInt );
	typeMap.insert( "flags", NifValue::tFlags );
	typeMap.insert( "link", NifValue::tLink );
	typeMap.insert( "parent", NifValue::tParent );
	typeMap.insert( "float", NifValue::tFloat );
	typeMap.insert( "string", NifValue::tString );
	typeMap.insert( "color3", NifValue::tColor3 );
	typeMap.insert( "color4", NifValue::tColor4 );
	typeMap.insert( "vector3", NifValue::tVector3 );
	typeMap.insert( "quat", NifValue::tQuat );
	typeMap.insert( "quaternion", NifValue::tQuat );
	typeMap.insert( "matrix33", NifValue::tMatrix );
	typeMap.insert( "vector2", NifValue::tVector2 );
	typeMap.insert( "bytearray", NifValue::tByteArray );
	typeMap.insert( "version", NifValue::tVersion );
}

NifValue::Type NifValue::type( const QString & id )
{
	if ( typeMap.isEmpty() )
		initialize();
	
	if ( typeMap.contains( id ) )
		return typeMap[id];
	
	return tNone;
}

bool NifValue::registerAlias( const QString & alias, const QString & original )
{
	if ( typeMap.isEmpty() )
		initialize();
	
	if ( typeMap.contains( original ) && ! typeMap.contains( alias ) )
	{
		typeMap.insert( alias, typeMap[original] );
		return true;
	}
	
	return false;
}
	
NifValue::NifValue( Type t ) : typ( tNone )
{
	changeType( t );
}

NifValue::NifValue( const NifValue & other ) : typ( tNone )
{
	operator=( other );
}

NifValue::~NifValue()
{
	clear();
}

void NifValue::clear()
{
	switch ( typ )
	{
		case tString:
			delete static_cast<QString*>( val.data );
			break;
		case tColor3:
			delete static_cast<Color3*>( val.data );
			break;
		case tColor4:
			delete static_cast<Color4*>( val.data );
			break;
		case tVector3:
			delete static_cast<Vector3*>( val.data );
			break;
		case tVector2:
			delete static_cast<Vector2*>( val.data );
			break;
		case tMatrix:
			delete static_cast<Matrix*>( val.data );
			break;
		case tQuat:
			delete static_cast<Quat*>( val.data );
			break;
		case tByteArray:
			delete static_cast<QByteArray*>( val.data );
			break;
		default:
			break;
	}
	typ = tNone;
	val.u32 = 0;
}

void NifValue::changeType( Type t )
{
	if ( typ == t )
		return;
	
	if ( typ != tNone )
		clear();
	
	switch ( ( typ = t ) )
	{
		case tLink:
		case tParent:
			val.i32 = -1;
			return;
		case tVector3:
			val.data = new Vector3();
			return;
		case tMatrix:
			val.data = new Matrix();
			return;
		case tQuat:
			val.data = new Quat();
			return;
		case tVector2:
			val.data = new Vector2();
			return;
		case tString:
			val.data = new QString();
			return;
		case tColor3:
			val.data = new Color3();
			return;
		case tColor4:
			val.data = new Color4();
			return;
		case tByteArray:
			val.data = new QByteArray();
			return;
		default:
			val.u32 = 0;
			return;
	}
}

void NifValue::operator=( const NifValue & other )
{
	if ( typ != other.typ )
		changeType( other.typ );

	switch ( typ )
	{
		case tVector3:
			*static_cast<Vector3*>( val.data ) = *static_cast<Vector3*>( other.val.data );
			return;
		case tMatrix:
			*static_cast<Matrix*>( val.data ) = *static_cast<Matrix*>( other.val.data );
			return;
		case tQuat:
			*static_cast<Quat*>( val.data ) = *static_cast<Quat*>( other.val.data );
			return;
		case tVector2:
			*static_cast<Vector2*>( val.data ) = *static_cast<Vector2*>( other.val.data );
			return;
		case tString:
			*static_cast<QString*>( val.data ) = *static_cast<QString*>( other.val.data );
			return;
		case tColor3:
			*static_cast<Color3*>( val.data ) = *static_cast<Color3*>( other.val.data );
			return;
		case tColor4:
			*static_cast<Color4*>( val.data ) = *static_cast<Color4*>( other.val.data );
			return;
		case tByteArray:
			*static_cast<QByteArray*>( val.data ) = *static_cast<QByteArray*>( other.val.data );
			return;
		default:
			val = other.val;
			return;
	}
}

QVariant NifValue::toVariant() const
{
	QVariant v;
	v.setValue( *this );
	return v;
}

bool NifValue::fromVariant( const QVariant & var )
{
	if ( var.canConvert<NifValue>() )
	{
		operator=( var.value<NifValue>() );
		return true;
	}
	return false;
}

bool NifValue::fromString( const QString & s )
{
	bool ok;
	switch ( typ )
	{
		case tBool:
			if ( s == "yes" || s == "true" )
			{
				val.u32 = 1;
				return true;
			}
			else if ( s == "no" || s == "false" )
			{
				val.u32 = 0;
				return true;
			}
		case tByte:
			val.u32 = 0;
			val.u08 = s.toUInt( &ok );
			return ok;
		case tWord:
			val.u32 = 0;
			val.u16 = s.toUInt( &ok );
			return ok;
		case tInt:
			val.u32 = s.toUInt( &ok );
			return ok;
		case tFlags:
			val.u32 = 0;
			val.u16 = s.toUInt( &ok, 2 );
			return ok;
		case tLink:
		case tParent:
			val.i32 = s.toInt( &ok );
			return ok;
		case tFloat:
			val.f32 = s.toDouble( &ok );
			return ok;
		case tString:
			*static_cast<QString*>( val.data ) = s;
			return true;
		case tColor3:
			static_cast<Color3*>( val.data )->fromQColor( QColor( s ) );
			return true;
		case tColor4:
			static_cast<Color4*>( val.data )->fromQColor( QColor( s ) );
			return true;
		case tVersion:
			val.u32 = NifModel::version2number( s );
			return val.u32 != 0;
		case tByteArray:
		case tVector2:
		case tVector3:
		case tQuat:
		case tMatrix:
		case tNone:
			return false;
	}
	return false;
}

QString NifValue::toString() const
{
	switch ( typ )
	{
		case tBool:
			return ( val.u32 ? "yes" : "no" );
		case tByte:
		case tWord:
		case tInt:
			return QString::number( val.u32 );
		case tFlags:
			return QString::number( val.u16, 2 );
		case tLink:
		case tParent:
			return QString::number( val.i32 );
		case tFloat:
			return QString::number( val.f32, 'f', 4 );
		case tString:
			return *static_cast<QString*>( val.data );
		case tColor3:
		{
			Color3 * col = static_cast<Color3*>( val.data );
			return QString( "#%1%2%3" ).arg( (int) ( col->red() * 0xff ), 2, 16, QChar( '0' ) ).arg( (int) ( col->green() * 0xff ), 2, 16, QChar( '0' ) ).arg( (int) ( col->blue() * 0xff ), 2, 16, QChar( '0' ) );
		}
		case tColor4:
		{
			Color4 * col = static_cast<Color4*>( val.data );
			return QString( "#%1%2%3%4" ).arg( (int) ( col->red() * 0xff ), 2, 16, QChar( '0' ) ).arg( (int) ( col->green() * 0xff ), 2, 16, QChar( '0' ) ).arg( (int) ( col->blue() * 0xff ), 2, 16, QChar( '0' ) ).arg( (int) ( col->alpha() * 0xff ), 2, 16, QChar( '0' ) );
		}
		case tVector2:
		{
			Vector2 * v = static_cast<Vector2*>( val.data );
			return QString( "X %1 Y %2" ).arg( (*v)[0], 0, 'f', 3 ).arg( (*v)[1], 0, 'f', 3 );
		}
		case tVector3:
		{
			Vector3 * v = static_cast<Vector3*>( val.data );
			return QString( "X %1 Y %2 Z %3" ).arg( (*v)[0], 0, 'f', 3 ).arg( (*v)[1], 0, 'f', 3 ).arg( (*v)[2], 0, 'f', 3 );
		}
		case tMatrix:
		{
			Matrix * m = static_cast<Matrix*>( val.data );
			float x, y, z;
			if ( m->toEuler( x, y, z ) )
				return QString( "Y %1 P %2 R %3" ).arg( x / PI * 180, 0, 'f', 0 ).arg( y / PI * 180, 0, 'f', 0 ).arg( z / PI * 180, 0, 'f', 0 );
			else
				return QString( "(Y %1 P %2 R %3)" ).arg( x / PI * 180, 0, 'f', 0 ).arg( y / PI * 180, 0, 'f', 0 ).arg( z / PI * 180, 0, 'f', 0 );
		}
		case tQuat:
		{
			Matrix m;
			m.fromQuat( *static_cast<Quat*>( val.data ) );
			float x, y, z;
			if ( m.toEuler( x, y, z ) )
				return QString( "Y %1 P %2 R %3" ).arg( x / PI * 180, 0, 'f', 0 ).arg( y / PI * 180, 0, 'f', 0 ).arg( z / PI * 180, 0, 'f', 0 );
			else
				return QString( "(Y %1 P %2 R %3)" ).arg( x / PI * 180, 0, 'f', 0 ).arg( y / PI * 180, 0, 'f', 0 ).arg( z / PI * 180, 0, 'f', 0 );
		}
		case tByteArray:
			return QString( "%1 bytes" ).arg( static_cast<QByteArray*>( val.data )->count() );
		case tVersion:
			return NifModel::version2string( val.u32 );
		default:
			return QString();
	}
}

QColor NifValue::toColor() const
{
	if ( type() == tColor3 ) 
		return static_cast<Color3*>( val.data )->toQColor();
	else if ( type() == tColor4 ) 
		return static_cast<Color4*>( val.data )->toQColor();
	else
		return QColor();
}


bool NifStream::read( NifValue & val )
{
	switch ( val.type() )
	{
		case NifValue::tBool:
			val.val.u32 = 0;
			if ( version <= 0x04000002 )
				return device->read( (char *) &val.val.u32, 4 ) == 4;
			else
				return device->read( (char *) &val.val.u08, 1 ) == 1;
		case NifValue::tByte:
			val.val.u32 = 0;
			return device->read( (char *) &val.val.u08, 1 ) == 1;
		case NifValue::tWord:
		case NifValue::tFlags:
			val.val.u32 = 0;
			return device->read( (char *) &val.val.u16, 2 ) == 2;
		case NifValue::tInt:
		case NifValue::tVersion:
			return device->read( (char *) &val.val.u32, 4 ) == 4;
		case NifValue::tLink:
		case NifValue::tParent:
			return device->read( (char *) &val.val.i32, 4 ) == 4;
		case NifValue::tFloat:
			return device->read( (char *) &val.val.f32, 4 ) == 4;
		case NifValue::tVector3:
			return device->read( (char *) static_cast<Vector3*>( val.val.data )->xyz, 12 ) == 12;
		case NifValue::tQuat:
			return device->read( (char *) static_cast<Quat*>( val.val.data )->wxyz, 16 ) == 16;
		case NifValue::tMatrix:
			return device->read( (char *) static_cast<Matrix*>( val.val.data )->m, 36 ) == 36;
		case NifValue::tVector2:
			return device->read( (char *) static_cast<Vector2*>( val.val.data )->xy, 8 ) == 8;
		case NifValue::tString:
		{
			int len;
			device->read( (char *) &len, 4 );
			if ( len > 4096 )	qWarning( "maximum string length exceeded" );
			QByteArray string = device->read( len );
			if ( string.count() != len ) return false;
			string.replace( "\r", "\\r" );
			string.replace( "\n", "\\n" );
			*static_cast<QString*>( val.val.data ) = QString( string );
		}	return true;
		case NifValue::tColor3:
			return device->read( (char *) static_cast<Color3*>( val.val.data )->rgb, 12 ) == 12;
		case NifValue::tColor4:
			return device->read( (char *) static_cast<Color4*>( val.val.data )->rgba, 16 ) == 16;
		case NifValue::tByteArray:
		{
			int len;
			device->read( (char *) &len, 4 );
			*static_cast<QByteArray*>( val.val.data ) = device->read( len );
			return static_cast<QByteArray*>( val.val.data )->count() == len;
		}
		case NifValue::tNone:
			return true;
	}
	return false;
}

bool NifStream::write( const NifValue & val )
{
	switch ( val.type() )
	{
		case NifValue::tBool:
			if ( version <= 0x04000002 )
				return device->write( (char *) &val.val.u32, 4 ) == 4;
			else
				return device->write( (char *) &val.val.u08, 1 ) == 1;
		case NifValue::tByte:
			return device->write( (char *) &val.val.u08, 1 ) == 1;
		case NifValue::tWord:
		case NifValue::tFlags:
			return device->write( (char *) &val.val.u16, 2 ) == 2;
		case NifValue::tInt:
		case NifValue::tVersion:
			return device->write( (char *) &val.val.u32, 4 ) == 4;
		case NifValue::tLink:
		case NifValue::tParent:
			return device->write( (char *) &val.val.i32, 4 ) == 4;
		case NifValue::tFloat:
			return device->write( (char *) &val.val.f32, 4 ) == 4;
		case NifValue::tVector3:
			return device->write( (char *) static_cast<Vector3*>( val.val.data )->xyz, 12 ) == 12;
		case NifValue::tQuat:
			return device->write( (char *) static_cast<Quat*>( val.val.data )->wxyz, 16 ) == 16;
		case NifValue::tMatrix:
			return device->write( (char *) static_cast<Matrix*>( val.val.data )->m, 36 ) == 36;
		case NifValue::tVector2:
			return device->write( (char *) static_cast<Vector2*>( val.val.data )->xy, 8 ) == 8;
		case NifValue::tString:
		{
			QByteArray string = static_cast<QString*>( val.val.data )->toAscii();
			string.replace( "\\r", "\r" );
			string.replace( "\\n", "\n" );
			int len = string.length();
			if ( device->write( (char *) &len, 4 ) != 4 )
				return false;
			return device->write( (const char *) string, string.length() ) == string.length();
		}
		case NifValue::tColor3:
			return device->write( (char *) static_cast<Color3*>( val.val.data )->rgb, 12 ) == 12;
		case NifValue::tColor4:
			return device->write( (char *) static_cast<Color4*>( val.val.data )->rgba, 16 ) == 16;
		case NifValue::tByteArray:
		{
			QByteArray * array = static_cast<QByteArray*>( val.val.data );
			int len = array->count();
			if ( device->write( (char *) &len, 4 ) != 4 )
				return false;
			return device->write( *array ) == len;
		}
		case NifValue::tNone:
			return true;
	}
	return false;
}
