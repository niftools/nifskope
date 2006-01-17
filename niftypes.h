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

#ifndef NIFTYPES_H
#define NIFTYPES_H

#include <QColor>
#include <QIODevice>
#include <QVariant>

#include "math.h"

#ifndef PI
#ifdef M_PI
#define PI M_PI
#else
#define PI 3.1416
#endif
#endif

class Vector2
{
public:
	Vector2()
	{
		xy[0] = xy[1] = 0.0;
	}
	Vector2( float x, float y )
	{
		xy[0] = x; xy[1] = y;
	}
	Vector2 & operator+=( const Vector2 & v )
	{
		xy[0] += v.xy[0];
		xy[1] += v.xy[1];
		return *this;
	}
	Vector2 operator+( const Vector2 & v )
	{
		Vector2 w( *this );
		return ( w += v );
	}
	Vector2 & operator-=( const Vector2 & v )
	{
		xy[0] -= v.xy[0];
		xy[1] -= v.xy[1];
		return *this;
	}
	Vector2 operator-( const Vector2 & v )
	{
		Vector2 w( *this );
		return ( w -= v );
	}
	Vector2 & operator*=( float s )
	{
		xy[0] *= s;
		xy[1] *= s;
		return *this;
	}
	Vector2 operator*( float s )
	{
		Vector2 w( *this );
		return ( w *= s );
	}
	
	float & operator[]( unsigned int i )
	{
		Q_ASSERT( i < 2 );
		return xy[i];
	}
	const float & operator[]( unsigned int i ) const
	{
		Q_ASSERT( i < 2 );
		return xy[i];
	}
	
	const float * data() const
	{
		return xy;
	}
	
protected:
	float	xy[2];
	
	friend class NifStream;
};

class Vector3
{
public:
	Vector3()
	{
		xyz[ 0 ] = xyz[ 1 ] = xyz[ 2 ] = 0.0;
	}
	Vector3( float x, float y, float z )
	{
		xyz[0] = x;
		xyz[1] = y;
		xyz[2] = z;
	}
	Vector3 & operator+=( const Vector3 & v )
	{
		xyz[0] += v.xyz[0];
		xyz[1] += v.xyz[1];
		xyz[2] += v.xyz[2];
		return *this;
	}
	Vector3 & operator-=( const Vector3 & v )
	{
		xyz[0] -= v.xyz[0];
		xyz[1] -= v.xyz[1];
		xyz[2] -= v.xyz[2];
		return *this;
	}
	Vector3 & operator*=( float s )
	{
		xyz[ 0 ] *= s;
		xyz[ 1 ] *= s;
		xyz[ 2 ] *= s;
		return *this;
	}
	Vector3 operator+( Vector3 v )
	{
		Vector3 w( *this );
		return w += v;
	}
	Vector3 operator-( Vector3 v )
	{
		Vector3 w( *this );
		return w -= v;
	}
	Vector3 operator*( float s ) const
	{
		Vector3 v( *this );
		return v *= s;
	}
	
	float & operator[]( unsigned int i )
	{
		Q_ASSERT( i < 3 );
		return xyz[i];
	}
	const float & operator[]( unsigned int i ) const
	{
		Q_ASSERT( i < 3 );
		return xyz[i];
	}
	
	float length() const
	{
		return sqrt( xyz[0]*xyz[0] + xyz[1]*xyz[1] + xyz[2]*xyz[2] );
	}
	
	void normalize()
	{
		float m = length();
		if ( m > 0.0F )
			m = 1.0F / m;
		else
			m = 0.0F;
		xyz[0] *= m;
		xyz[1] *= m;
		xyz[2] *= m;
	}
	
	static Vector3 min( const Vector3 & v1, const Vector3 & v2 )
	{
		return Vector3( qMin( v1.xyz[0], v2.xyz[0] ), qMin( v1.xyz[1], v2.xyz[1] ), qMin( v1.xyz[2], v2.xyz[2] ) );
	}
	static Vector3 max( const Vector3 & v1, const Vector3 & v2 )
	{
		return Vector3( qMax( v1.xyz[0], v2.xyz[0] ), qMax( v1.xyz[1], v2.xyz[1] ), qMax( v1.xyz[2], v2.xyz[2] ) );
	}
	
	const float * data() const { return xyz; }
	
protected:
	float xyz[3];
	
	friend class NifStream;
};

class Quat
{
public:
	Quat()
	{
		memcpy( wxyz, identity, 16 );
	}
	float & operator[]( unsigned int i )
	{
		Q_ASSERT( i < 4 );
		return wxyz[ i ];
	}
	const float & operator[]( unsigned int i ) const
	{
		Q_ASSERT( i < 4 );
		return wxyz[ i ];
	}
	Quat & operator*=( float s )
	{
		for ( int c = 0; c < 4; c++ )
			wxyz[ c ] *= s;
		return *this;
	}
	Quat operator*( float s ) const
	{
		Quat q( *this );
		return ( q *= s );
	}
	Quat & operator+=( const Quat & q )
	{
		for ( int c = 0; c < 4; c++ )
			wxyz[ c ] += q.wxyz[ c ];
		return *this;
	}
	Quat operator+( const Quat & q ) const
	{
		Quat r( *this );
		return ( r += q );
	}
	static float dotproduct( const Quat & q1, const Quat & q2 )
	{
		return q1[0]*q2[0]+q1[1]*q2[1]+q1[2]*q2[2]+q1[3]*q2[3];
	}

	QString toHtml() const
	{
		return QString( "W %1<br>X %2<br>Y %3<br>Z %4" ).arg( wxyz[0], 0, 'f', 4 ).arg( wxyz[1], 0, 'f', 4 ).arg( wxyz[2], 0, 'f', 4 ).arg( wxyz[3], 0, 'f', 4 );
	}
	
protected:
	float	wxyz[4];
	static const float identity[4];
	
	friend class NifStream;
};

class Matrix
{
public:
	Matrix()
	{
		memcpy( m, identity, 36 );
	}
	Matrix operator*( const Matrix & m2 ) const
	{
		Matrix m3;
		for ( int r = 0; r < 3; r++ )
			for ( int c = 0; c < 3; c++ )
				m3.m[r][c] = m[r][0]*m2.m[0][c] + m[r][1]*m2.m[1][c] + m[r][2]*m2.m[2][c];
		return m3;
	}
	Vector3 operator*( const Vector3 & v ) const
	{
		return Vector3(
			m[0][0]*v[0] + m[0][1]*v[1] + m[0][2]*v[2],
			m[1][0]*v[0] + m[1][1]*v[1] + m[1][2]*v[2],
			m[2][0]*v[0] + m[2][1]*v[1] + m[2][2]*v[2] );
	}
	float & operator()( unsigned int c, unsigned int d )
	{
		Q_ASSERT( c < 3 && d < 3 );
		return m[c][d];
	}
	float operator()( unsigned int c, unsigned int d ) const
	{
		Q_ASSERT( c < 3 && d < 3 );
		return m[c][d];
	}
	
	void fromQuat( const Quat & q );
	Quat toQuat() const;
	
	void fromEuler( float x, float y, float z );
	bool toEuler( float & x, float & y, float & z ) const;
	
	QString toHtml() const;

protected:
	float m[3][3];
	static const float identity[9];
	
	friend class NifStream;
};

class Color3 : public QColor
{
public:
	Color3() { rgb[0] = rgb[1] = rgb[2] = 0; }
	explicit Color3( const QColor & c ) { fromQColor( c ); }
	
	float & operator[]( unsigned int i )
	{
		Q_ASSERT( i < 3 );
		return rgb[i];
	}
	
	const float & operator[]( unsigned int i ) const
	{
		Q_ASSERT( i < 3 );
		return rgb[i];
	}
	
	float red() const { return rgb[0]; }
	float green() const { return rgb[1]; }
	float blue() const { return rgb[2]; }
	
	void setRed( float r ) { rgb[0] = r; }
	void setGreen( float g ) { rgb[1] = g; }
	void setBlue( float b ) { rgb[2] = b; }
	
	void setRGB( float r, float g, float b ) { rgb[0] = r; rgb[1] = g; rgb[2] = b; }
	
	QColor toQColor() const
	{
		return QColor::fromRgbF( rgb[0], rgb[1], rgb[2] );
	}
	
	void fromQColor( const QColor & c )
	{
		rgb[0] = c.redF();
		rgb[1] = c.greenF();
		rgb[2] = c.blueF();
	}
	
	const float * data() const { return rgb; }
	
protected:
	float rgb[3];
	
	friend class NifStream;
};

class Color4 : public QColor
{
public:
	Color4() { rgba[0] = rgba[1] = rgba[2] = rgba[3]; }
	Color4( const Color3 & c, float alpha = 1.0 ) { rgba[0] = c[0]; rgba[1] = c[1]; rgba[2] = c[2]; rgba[3] = alpha; }
	explicit Color4( const QColor & c ) { fromQColor( c ); }
	
	float & operator[]( unsigned int i )
	{
		Q_ASSERT( i < 4 );
		return rgba[ i ];
	}
	
	const float & operator[]( unsigned int i ) const
	{
		Q_ASSERT( i < 4 );
		return rgba[ i ];
	}
	
	float red() const { return rgba[0]; }
	float green() const { return rgba[1]; }
	float blue() const { return rgba[2]; }
	float alpha() const { return rgba[3]; }
	
	void setRed( float r ) { rgba[0] = r; }
	void setGreen( float g ) { rgba[1] = g; }
	void setBlue( float b ) { rgba[2] = b; }
	void setAlpha( float a ) { rgba[3] = a; }
	
	void setRGBA( float r, float g, float b, float a ) { rgba[ 0 ] = r; rgba[ 1 ] = g; rgba[ 2 ] = b; rgba[ 3 ] = a; }

	QColor toQColor() const
	{
		return QColor::fromRgbF( rgba[0], rgba[1], rgba[2], rgba[3] );
	}
	
	void fromQColor( const QColor & c )
	{
		rgba[0] = c.redF();
		rgba[1] = c.greenF();
		rgba[2] = c.blueF();
		rgba[3] = c.alphaF();
	}
	
	const float * data() const { return rgba; }
	
	Color4 blend( float alpha ) const
	{
		Color4 c( *this );
		c.setAlpha( c.alpha() * alpha );
		return c;
	}

protected:
	float rgba[4];
	
	friend class NifStream;
};

class NifValue
{
public:
	enum Type
	{
		tBool = 0,
		tByte = 1,
		tWord = 2,
		tInt = 3,
		tLink = 4,
		tParent = 5,
		tFloat = 6,
		tString = 7,
		tColor3 = 8,
		tColor4 = 9,
		tVector3 = 10,
		tQuat = 11,
		tMatrix = 12,
		tVector2 = 13,
		tByteArray = 14,
		
		tNone = 0xff
	};
	
	static void initialize();
	static Type type( const QString & typId );
	static bool registerAlias( const QString & alias, const QString & internal );
	
	NifValue() { typ = tNone; }
	NifValue( Type t );
	NifValue( const NifValue & other );
	
	~NifValue();
	
	void clear();
	void changeType( Type );
	
	void operator=( const NifValue & other );
	//bool operator==( const NifValue & other );
	//bool operator!=( const NifValue & other ) { return ! ( *this == other ); }
	
	Type type() const { return typ; }
	
	static bool isValid( Type t ) { return t != tNone; }
	static bool isLink( Type t ) { return t == tLink || t == tParent; }
	
	bool isValid() const { return typ != tNone; }
	bool isColor() const { return typ == tColor3 || typ == tColor4; }
	bool isCount() const { return typ >= tBool && typ <= tInt; }
	bool isFloat() const { return typ == tFloat; }
	bool isLink() const { return typ == tLink || typ == tParent; }
	bool isMatrix() const { return typ == tMatrix; }
	bool isQuat() const { return typ == tQuat; }
	bool isString() const { return typ == tString; }
	bool isVector3() const { return typ == tVector3; }
	bool isVector2() const { return typ == tVector2; }
	bool isByteArray() const { return typ == tByteArray; }
	
	QColor toColor() const;
	quint32 toCount() const;
	float toFloat() const;
	qint32 toLink() const;
	QString toString() const;
	QVariant toVariant() const;
	
	operator QVariant () const	{ return toVariant(); }
	
	bool setCount( int );
	bool setFloat( float );
	bool setLink( int );
	
	bool fromString( const QString & );
	bool fromVariant( const QVariant & );
	
	template <typename T> T get() const;
	template <typename T> bool set( const T & x );

protected:
	Type typ;
	
	union Value
	{
		quint8	u08;
		quint16 u16;
		quint32 u32;
		qint32	i32;
		float	f32;
		void *	data;
	} val;
	
	template <typename T> T getType( Type t ) const;
	template <typename T> bool setType( Type t, T v );
	
	static QHash<QString,Type>	typeMap;
	
	friend class NifStream;
};

inline quint32 NifValue::toCount() const { if ( isCount() ) return val.u32; return 0; }
inline float NifValue::toFloat() const { if ( isFloat() ) return val.f32; else return 0.0; }
inline qint32 NifValue::toLink() const { if ( isLink() ) return val.i32; else return -1; }

inline bool NifValue::setCount( int c ) { if ( isCount() ) { val.u32 = c; return true; } else return false; }
inline bool NifValue::setFloat( float f ) { if ( isFloat() ) { val.f32 = f; return true; } else return false; }
inline bool NifValue::setLink( int l ) { if ( isLink() ) { val.i32 = l; return true; } else return false; }

template <typename T> inline T NifValue::getType( Type t ) const
{
	if ( typ == t )
		return *static_cast<T*>( val.data );
	else
		return T();
}

template <typename T> inline bool NifValue::setType( Type t, T v )
{
	if ( typ == t )
	{
		*static_cast<T*>( val.data ) = v;
		return true;
	}
	return false;
}

template <> inline bool NifValue::get() const { return toCount(); }
template <> inline int NifValue::get() const { return toCount(); }
template <> inline float NifValue::get() const { return toFloat(); }
template <> inline QColor NifValue::get() const { return toColor(); }
template <> inline QVariant NifValue::get() const { return toVariant(); }

template <> inline Matrix NifValue::get() const { return getType<Matrix>( tMatrix ); }
template <> inline Quat NifValue::get() const { return getType<Quat>( tQuat ); }
template <> inline Vector3 NifValue::get() const { return getType<Vector3>( tVector3 ); }
template <> inline Vector2 NifValue::get() const { return getType<Vector2>( tVector2 ); }
template <> inline Color3 NifValue::get() const { return getType<Color3>( tColor3 ); }
template <> inline Color4 NifValue::get() const { return getType<Color4>( tColor4 ); }
template <> inline QString NifValue::get() const { return getType<QString>( tString ); }
template <> inline QByteArray NifValue::get() const { return getType<QByteArray>( tByteArray ); }


template <> inline bool NifValue::set( const Matrix & x ) { return setType( tMatrix, x ); }
template <> inline bool NifValue::set( const Quat & x ) { return setType( tQuat, x ); }
template <> inline bool NifValue::set( const Vector3 & x ) { return setType( tVector3, x ); }
template <> inline bool NifValue::set( const Vector2 & x ) { return setType( tVector2, x ); }
template <> inline bool NifValue::set( const Color3 & x ) { return setType( tColor3, x ); }
template <> inline bool NifValue::set( const Color4 & x ) { return setType( tColor4, x ); }
template <> inline bool NifValue::set( const QString & x ) { return setType( tString, x ); }
template <> inline bool NifValue::set( const QByteArray & x ) { return setType( tByteArray, x ); }

class NifStream
{
public:
	NifStream( quint32 v, QIODevice * d ) : version( v ), device( d ) {}
	
	bool read( NifValue & );
	bool write( const NifValue & );

private:
	quint32 version;
	QIODevice * device;
};


Q_DECLARE_METATYPE( NifValue );

#endif
