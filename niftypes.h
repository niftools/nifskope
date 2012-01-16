/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005-2012, NIF File Format Library and Tools
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

#ifndef NIFTYPES_H
#define NIFTYPES_H

#include <QColor>
#include <QApplication>
#include <QDebug>
#include <cfloat>
#include <cmath>

#ifndef PI
#ifdef M_PI
#define PI M_PI
#else
//! This is only defined if we can't find M_PI, which should be in stdlib's %math.h
#define PI 3.1416f
#endif
#endif

//! \file niftypes.h Type definitions and functions

//! Format a float with out of range values
QString NumOrMinMax( float val, char f = 'g', int prec = 6 );

class NifModel;
class QModelIndex;
class QDataStream;

/*
 * Perhaps look at introducing a parent class for all of the vector/matrix types to improve abstraction?
 */

//! A vector of 2 floats
class Vector2
{
public:
	//! Default constructor
	Vector2()
	{
		xy[0] = xy[1] = 0.0;
	}
	//! Constructor
	Vector2( float x, float y )
	{
		xy[0] = x; xy[1] = y;
	}
	//! Add-equals operator
	Vector2 & operator+=( const Vector2 & v )
	{
		xy[0] += v.xy[0];
		xy[1] += v.xy[1];
		return *this;
	}
	//! Add operator
	Vector2 operator+( const Vector2 & v ) const
	{
		Vector2 w( *this );
		return ( w += v );
	}
	//! Minus-equals operator
	Vector2 & operator-=( const Vector2 & v )
	{
		xy[0] -= v.xy[0];
		xy[1] -= v.xy[1];
		return *this;
	}
	//! Minus operator
	Vector2 operator-( const Vector2 & v ) const
	{
		Vector2 w( *this );
		return ( w -= v );
	}
	//! Negation operator
	Vector2 operator-() const
	{
		return Vector2() - *this;
	}
	//! Times-equals operator
	Vector2 & operator*=( float s )
	{
		xy[0] *= s;
		xy[1] *= s;
		return *this;
	}
	//! Times operator
	Vector2 operator*( float s ) const
	{
		Vector2 w( *this );
		return ( w *= s );
	}
	//! Divide equals operator
	Vector2 & operator/=( float s )
	{
		xy[0] /= s;
		xy[1] /= s;
		return *this;
	}
	//! Divide operator
	Vector2 operator/( float s ) const
	{
		Vector2 w( *this );
		return ( w /= s );
	}
	
	//! Equality operator
	bool operator==( const Vector2 & v ) const
	{
		return xy[0] == v.xy[0] && xy[1] == v.xy[1];
	}
	
	//! Array operator
	float & operator[]( unsigned int i )
	{
		Q_ASSERT( i < 2 );
		return xy[i];
	}
	//! Const array operator
	const float & operator[]( unsigned int i ) const
	{
		Q_ASSERT( i < 2 );
		return xy[i];
	}
	
	//! Comparison function for lexicographic sorting
	static bool lexLessThan( const Vector2 & v1, const Vector2 & v2 )
	{
		if( v1[0] != v2[0] )
		{
			return v1[0] < v2[0];
		}
		else
		{
			return v1[1] < v2[1];
		}
	}
	
	//! %Data accessor
	const float * data() const
	{
		return xy;
	}
	
	//! Set from string
	void fromString( QString str );

protected:
	float	xy[2];
	
	friend class NifIStream;
	friend class NifOStream;
	
	friend QDataStream & operator>>( QDataStream & ds, Vector2 & v );
};

//! An attempt at providing a QDebug stream operator for Vector2
/**
 * See <a href="http://doc.trolltech.com/latest/debug.html">Qt Debugging</a>
 * for how this is supposed to work.
 */
inline QDebug &operator<<( QDebug dbg, Vector2 v )
{
	dbg.nospace() << "(" << v[0] << ", " << v[1] << ")";
	return dbg.space();
}

//! A stream operator for reading in a Vector2
inline QDataStream & operator>>( QDataStream & ds, Vector2 & v )
{
	ds >> v.xy[0] >> v.xy[1];
	return ds;
}

//! A vector of 3 floats
class Vector3
{
	Q_DECLARE_TR_FUNCTIONS(Vector3)

public:
	//! Default constructor
	Vector3()
	{
		xyz[ 0 ] = xyz[ 1 ] = xyz[ 2 ] = 0.0;
	}
	//! Constructor
	Vector3( float x, float y, float z )
	{
		xyz[0] = x;
		xyz[1] = y;
		xyz[2] = z;
	}
	//! Constructor from a Vector2 and a float
	explicit Vector3( const Vector2 & v2, float z = 0 )
	{
		xyz[0] = v2[0];
		xyz[1] = v2[1];
		xyz[2] = z;
	}
	//! Constructor from a Vector4
	/*!
	 * Construct a Vector3 from a Vector4 by truncating.
	 * Doxygen can't document this due to a circular dependency?
	 */
	explicit Vector3( const class Vector4 & );
	
	//! Add-equals operator
	Vector3 & operator+=( const Vector3 & v )
	{
		xyz[0] += v.xyz[0];
		xyz[1] += v.xyz[1];
		xyz[2] += v.xyz[2];
		return *this;
	}
	//! Minus-equals operator
	Vector3 & operator-=( const Vector3 & v )
	{
		xyz[0] -= v.xyz[0];
		xyz[1] -= v.xyz[1];
		xyz[2] -= v.xyz[2];
		return *this;
	}
	//! Times-equals operator
	Vector3 & operator*=( float s )
	{
		xyz[ 0 ] *= s;
		xyz[ 1 ] *= s;
		xyz[ 2 ] *= s;
		return *this;
	}
	//! Divide-equals operator
	Vector3 & operator/=( float s )
	{
		xyz[ 0 ] /= s;
		xyz[ 1 ] /= s;
		xyz[ 2 ] /= s;
		return *this;
	}
	//! Add operator
	Vector3 operator+( Vector3 v ) const
	{
		Vector3 w( *this );
		return w += v;
	}
	//! Minus operator
	Vector3 operator-( Vector3 v ) const
	{
		Vector3 w( *this );
		return w -= v;
	}
	//! Negation operator
	Vector3 operator-() const
	{
		return Vector3() - *this;
	}
	//! Times operator
	Vector3 operator*( float s ) const
	{
		Vector3 v( *this );
		return v *= s;
	}
	//! Divide operator
	Vector3 operator/( float s ) const
	{
		Vector3 v( *this );
		return v /= s;
	}
	
	//! Equality operator
	bool operator==( const Vector3 & v ) const
	{
		return xyz[0] == v.xyz[0] && xyz[1] == v.xyz[1] && xyz[2] == v.xyz[2];
	}
	
	//! Array operator
	float & operator[]( unsigned int i )
	{
		Q_ASSERT( i < 3 );
		return xyz[i];
	}
	//! Const array operator
	const float & operator[]( unsigned int i ) const
	{
		Q_ASSERT( i < 3 );
		return xyz[i];
	}
	
	//! Find the length of the vector
	float length() const
	{
		return sqrt( xyz[0]*xyz[0] + xyz[1]*xyz[1] + xyz[2]*xyz[2] );
	}
	
	//! Find the length of the vector, squared
	float squaredLength() const
	{
		return xyz[0]*xyz[0] + xyz[1]*xyz[1] + xyz[2]*xyz[2];
	}
	
	//! Normalize the vector
	Vector3 & normalize()
	{
		float m = length();
		if ( m > 0.0 )
			m = 1.0 / m;
		else
			m = 0.0F;
		xyz[0] *= m;
		xyz[1] *= m;
		xyz[2] *= m;
		return *this;
	}
	
	//! Find the dot product of two vectors
	static float dotproduct( const Vector3 & v1, const Vector3 & v2 )
	{
		return v1[0]*v2[0]+v1[1]*v2[1]+v1[2]*v2[2];
	}
	//! Find the cross product of two vectors
	static Vector3 crossproduct( const Vector3 & a, const Vector3 & b )
	{
		return Vector3( a[1]*b[2] - a[2]*b[1], a[2]*b[0] - a[0]*b[2], a[0]*b[1] - a[1]*b[0] );
	}
	
	//! Find the angle between two vectors
	static float angle( const Vector3 & v1, const Vector3 & v2 )
	{
		float dot = dotproduct( v1, v2 );
		if ( dot > 1.0 )
			return 0.0;
		else if ( dot < - 1.0 )
			return (float)PI;
		else if ( dot == 0.0 )
			return (float)(PI/2);
		else
			return acos( dot );
	}
	
	//! Comparison function for lexicographic sorting
	static bool lexLessThan( const Vector3 & v1, const Vector3 & v2 )
	{
		if( v1[0] != v2[0] )
		{
			return v1[0] < v2[0];
		}
		else
		{
			if( v1[1] != v2[1] )
			{
				return v1[1] < v2[1];
			}
			else
			{
				return v1[2] < v2[2];
			}
		}
	}
	
	//! Size a vector to a minimum bound
	void boundMin( const Vector3 & v )
	{
		if ( v[0] < xyz[0] ) xyz[0] = v[0];
		if ( v[1] < xyz[1] ) xyz[1] = v[1];
		if ( v[2] < xyz[2] ) xyz[2] = v[2];
	}
	//! Size a vector to a maximum bound
	void boundMax( const Vector3 & v )
	{
		if ( v[0] > xyz[0] ) xyz[0] = v[0];
		if ( v[1] > xyz[1] ) xyz[1] = v[1];
		if ( v[2] > xyz[2] ) xyz[2] = v[2];
	}
	
	//! %Data accessor
	const float * data() const { return xyz; }
	
	//! Set from string
	void fromString( QString str );
	
	//! Format as HTML
	QString toHtml() const
	{
		return tr( "X %1 Y %2 Z %3\nlength %4" )
			.arg( NumOrMinMax( xyz[0] ) )
			.arg( NumOrMinMax( xyz[1] ) )
			.arg( NumOrMinMax( xyz[2] ) )
			.arg( length() );
	}
	
protected:
	float xyz[3];
	
	friend class NifIStream;
	friend class NifOStream;
	
	friend QDataStream & operator>>( QDataStream & ds, Vector3 & v );
};

//! An attempt at providing a QDebug stream operator for Vector3
/**
 * See <a href="http://doc.trolltech.com/latest/debug.html">Qt Debugging</a>
 * for how this is supposed to work.
 */
inline QDebug &operator<<( QDebug dbg, Vector3 v )
{
	dbg.nospace() << "(" << v[0] << ", " << v[1] << ", " << v[2] << ")";
	return dbg.space();
}

//! A stream operator for reading in a Vector3
inline QDataStream & operator>>( QDataStream & ds, Vector3 & v )
{
	ds >> v.xyz[0] >> v.xyz[1] >> v.xyz[2];
	return ds;
}

//! A vector of 4 floats
class Vector4
{
	Q_DECLARE_TR_FUNCTIONS(Vector4)

public:
	//! Default constructor
	Vector4()
	{
		xyzw[ 0 ] = xyzw[ 1 ] = xyzw[ 2 ] = xyzw[ 3 ] = 0.0;
	}
	//! Constructor
	Vector4( float x, float y, float z, float w )
	{
		xyzw[ 0 ] = x;
		xyzw[ 1 ] = y;
		xyzw[ 2 ] = z;
		xyzw[ 3 ] = w;
	}
	//! Constructor from a Vector3 and a float
	explicit Vector4( const Vector3 & v3, float w = 0.0 )
	{
		xyzw[0] = v3[0];
		xyzw[1] = v3[1];
		xyzw[2] = v3[2];
		xyzw[3] = w;
	}
	//! Add-equals operator
	Vector4 & operator+=( const Vector4 & v )
	{
		xyzw[0] += v.xyzw[0];
		xyzw[1] += v.xyzw[1];
		xyzw[2] += v.xyzw[2];
		xyzw[3] += v.xyzw[3];
		return *this;
	}
	//! Minus-equals operator
	Vector4 & operator-=( const Vector4 & v )
	{
		xyzw[0] -= v.xyzw[0];
		xyzw[1] -= v.xyzw[1];
		xyzw[2] -= v.xyzw[2];
		xyzw[3] -= v.xyzw[3];
		return *this;
	}
	//! Times-equals operator
	Vector4 & operator*=( float s )
	{
		xyzw[ 0 ] *= s;
		xyzw[ 1 ] *= s;
		xyzw[ 2 ] *= s;
		xyzw[ 3 ] *= s;
		return *this;
	}
	//! Divide-equals operator
	Vector4 & operator/=( float s )
	{
		xyzw[ 0 ] /= s;
		xyzw[ 1 ] /= s;
		xyzw[ 2 ] /= s;
		xyzw[ 3 ] /= s;
		return *this;
	}
	//! Add operator
	Vector4 operator+( Vector4 v ) const
	{
		Vector4 w( *this );
		return w += v;
	}
	//! Minus operator
	Vector4 operator-( Vector4 v ) const
	{
		Vector4 w( *this );
		return w -= v;
	}
	//! Negation operator
	Vector4 operator-() const
	{
		return Vector4() - *this;
	}
	//! Times operator
	Vector4 operator*( float s ) const
	{
		Vector4 v( *this );
		return v *= s;
	}
	//! Divide operator
	Vector4 operator/( float s ) const
	{
		Vector4 v( *this );
		return v /= s;
	}
	
	//! Equality operator
	bool operator==( const Vector4 & v ) const
	{
		return xyzw[0] == v.xyzw[0] && xyzw[1] == v.xyzw[1] && xyzw[2] == v.xyzw[2] && xyzw[3] == v.xyzw[3];
	}
	
	//! Array operator
	float & operator[]( unsigned int i )
	{
		Q_ASSERT( i < 4 );
		return xyzw[i];
	}
	//! Const array operator
	const float & operator[]( unsigned int i ) const
	{
		Q_ASSERT( i < 4 );
		return xyzw[i];
	}
	
	//! Find the length of the vector
	float length() const
	{
		return sqrt( xyzw[0]*xyzw[0] + xyzw[1]*xyzw[1] + xyzw[2]*xyzw[2] + xyzw[3]*xyzw[3] );
	}
	
	//! Find the length of the vector, squared
	float squaredLength() const
	{
		return xyzw[0]*xyzw[0] + xyzw[1]*xyzw[1] + xyzw[2]*xyzw[2] + xyzw[3]*xyzw[3];
	}
	
	//! Normalize the vector
	void normalize()
	{
		float m = length();
		if ( m > 0.0 )
			m = 1.0 / m;
		else
			m = 0.0F;
		xyzw[0] *= m;
		xyzw[1] *= m;
		xyzw[2] *= m;
		xyzw[3] *= m;
	}
	
	//! Find the dot product of two vectors
	static float dotproduct( const Vector4 & v1, const Vector4 & v2 )
	{
		return v1[0]*v2[0]+v1[1]*v2[1]+v1[2]*v2[2]+v1[3]*v2[3];
	}
	
	//! Find the angle between two vectors
	static float angle( const Vector4 & v1, const Vector4 & v2 )
	{
		float dot = dotproduct( v1, v2 );
		if ( dot > 1.0 )
			return 0.0;
		else if ( dot < - 1.0 )
			return (float)PI;
		else if ( dot == 0.0 )
			return (float)PI/2;
		else
			return (float)acos( dot );
	}
	
	//! Comparison function for lexicographic sorting
	static bool lexLessThan( const Vector4 & v1, const Vector4 & v2 )
	{
		if( v1[0] != v2[0] )
		{
			return v1[0] < v2[0];
		}
		else
		{
			if( v1[1] != v2[1] )
			{
				return v1[1] < v2[1];
			}
			else
			{
				if( v1[2] != v2[2] )
				{
					return v1[2] < v2[2];
				}
				else
				{
					return v1[3] < v2[3];
				}
			}
		}
	}
	
	//! %Data accessor
	const float * data() const { return xyzw; }
	
	//! Set from string
	void fromString( QString str );
	
	//! Format as HTML
	QString toHtml() const
	{
		return tr( "X %1 Y %2 Z %3 W %4\nlength %5" )
			.arg( NumOrMinMax( xyzw[0] ) )
			.arg( NumOrMinMax( xyzw[1] ) )
			.arg( NumOrMinMax( xyzw[2] ) )
			.arg( NumOrMinMax( xyzw[3] ) )
			.arg( length() );
	}
	
protected:
	float xyzw[4];
	
	friend class NifIStream;
	friend class NifOStream;
	
	friend QDataStream & operator>>( QDataStream & ds, Vector4 & v );
};

//! An attempt at providing a QDebug stream operator for Vector2
/**
 * See <a href="http://doc.trolltech.com/latest/debug.html">Qt Debugging</a>
 * for how this is supposed to work.
 */
inline QDebug &operator<<( QDebug dbg, Vector4 v )
{
	dbg.nospace() << "(" << v[0] << ", " << v[1] << ", " << v[2] << ", " << v[3] << ")";
	return dbg.space();
}

//! A stream operator for reading in a Vector4
inline QDataStream & operator>>( QDataStream & ds, Vector4 & v )
{
	ds >> v.xyzw[0] >> v.xyzw[1] >> v.xyzw[2] >> v.xyzw[3];
	return ds;
}

// This doesn't seem to document properly in Doxygen.
inline Vector3::Vector3( const Vector4 & v4 )
{
	xyz[0] = v4[0];
	xyz[1] = v4[1];
	xyz[2] = v4[2];
}

//! A quaternion
class Quat
{
	Q_DECLARE_TR_FUNCTIONS(Quat)

public:
	//! Default constructor
	Quat()
	{
		memcpy( wxyz, identity, 16 );
	}
	//! Constructor
	Quat( float w, float x, float y, float z )
	{
		wxyz[0] = w;
		wxyz[1] = x;
		wxyz[2] = y;
		wxyz[3] = z;
	}

	void normalize ()
	{
		float mag = (
			(wxyz[0]*wxyz[0]) +
			(wxyz[1]*wxyz[1]) +
			(wxyz[2]*wxyz[2]) +
			(wxyz[3]*wxyz[3]));
		wxyz[0] /= mag;
		wxyz[1] /= mag;
		wxyz[2] /= mag;
		wxyz[3] /= mag;
	}

	void negate ()
	{
		wxyz[0] = -wxyz[0];
		wxyz[1] = -wxyz[1];
		wxyz[2] = -wxyz[2];
		wxyz[3] = -wxyz[3];
	}
	
	//! Array operator
	float & operator[]( unsigned int i )
	{
		Q_ASSERT( i < 4 );
		return wxyz[ i ];
	}
	//! Const array operator
	const float & operator[]( unsigned int i ) const
	{
		Q_ASSERT( i < 4 );
		return wxyz[ i ];
	}
	//! Times-equals operator
	Quat & operator*=( float s )
	{
		for ( int c = 0; c < 4; c++ )
			wxyz[ c ] *= s;
		return *this;
	}
	//! Times operator
	Quat operator*( float s ) const
	{
		Quat q( *this );
		return ( q *= s );
	}
	//! Add-equals operator
	Quat & operator+=( const Quat & q )
	{
		for ( int c = 0; c < 4; c++ )
			wxyz[ c ] += q.wxyz[ c ];
		return *this;
	}
	//! Add operator
	Quat operator+( const Quat & q ) const
	{
		Quat r( *this );
		return ( r += q );
	}
	//! Find the dot product of two quaternions
	static float dotproduct( const Quat & q1, const Quat & q2 )
	{
		return q1[0]*q2[0]+q1[1]*q2[1]+q1[2]*q2[2]+q1[3]*q2[3];
	}
	
	//! Set from string
	void fromString( QString str );
	//! Set from vector and angle
	void fromAxisAngle( Vector3 axis, float angle );
	//! Find vector and angle
	void toAxisAngle( Vector3 & axis, float & angle ) const;
	
	//! Spherical linear interpolatation between two quaternions
	static Quat slerp (float t, const Quat& p, const Quat& q);
	
	//! Format as HTML
	QString toHtml() const
	{
		return tr( "W %1\nX %2\nY %3\nZ %4" )
			.arg( NumOrMinMax( wxyz[0] ) )
			.arg( NumOrMinMax( wxyz[1] ) )
			.arg( NumOrMinMax( wxyz[2] ) )
			.arg( NumOrMinMax( wxyz[3] ) );
	}
	
protected:
	float	wxyz[4];
	static const float identity[4];
	
	friend class NifIStream;
	friend class NifOStream;

	friend QDataStream & operator>>( QDataStream & ds, Quat & q );
};

//! A stream operator for reading in a Quat
inline QDataStream & operator>>( QDataStream & ds, Quat & q )
{
	ds >> q.wxyz[0] >> q.wxyz[1] >> q.wxyz[2] >> q.wxyz[3];
	return ds;
}

//! A 3 by 3 matrix
class Matrix
{
public:
	//! Constructor
	Matrix()
	{
		memcpy( m, identity, 36 );
	}
	//! Times operator for a matrix
	Matrix operator*( const Matrix & m2 ) const
	{
		Matrix m3;
		for ( int r = 0; r < 3; r++ )
			for ( int c = 0; c < 3; c++ )
				m3.m[r][c] = m[r][0]*m2.m[0][c] + m[r][1]*m2.m[1][c] + m[r][2]*m2.m[2][c];
		return m3;
	}
	//! Times operator for a vector
	Vector3 operator*( const Vector3 & v ) const
	{
		return Vector3(
			m[0][0]*v[0] + m[0][1]*v[1] + m[0][2]*v[2],
			m[1][0]*v[0] + m[1][1]*v[1] + m[1][2]*v[2],
			m[2][0]*v[0] + m[2][1]*v[1] + m[2][2]*v[2] );
	}
	//! %Element operator
	float & operator()( unsigned int c, unsigned int d )
	{
		Q_ASSERT( c < 3 && d < 3 );
		return m[c][d];
	}
	//! Const element operator
	float operator()( unsigned int c, unsigned int d ) const
	{
		Q_ASSERT( c < 3 && d < 3 );
		return m[c][d];
	}
	
	//! Find the inverted form
	Matrix inverted() const;
	
	//! Set from quaternion
	void fromQuat( const Quat & q );
	//! Convert to quaternion
	Quat toQuat() const;
	
	//! Convert from Euler angles in a (Z,Y,X) manner.
	void fromEuler( float x, float y, float z );
	//! Convert to Euler angles
	bool toEuler( float & x, float & y, float & z ) const;
	
	//! Find a matrix from Euler angles
	static Matrix euler( float x, float y, float z )
	{
		Matrix m; m.fromEuler( x, y, z );
		return m;
	}
	
	//! Format as HTML
	QString toHtml() const;

protected:
	float m[3][3];
	static const float identity[9];
	
	friend class NifIStream;
	friend class NifOStream;
};

//! A 4 by 4 matrix
class Matrix4
{
public:
	//! Constructor
	Matrix4()
	{
		memcpy( m, identity, 64 );
	}
	//! Times operator for a matrix
	Matrix4 operator*( const Matrix4 & m2 ) const
	{
		Matrix4 m3;
		for ( int r = 0; r < 4; r++ )
			for ( int c = 0; c < 4; c++ )
				m3.m[r][c] = m[r][0]*m2.m[0][c] + m[r][1]*m2.m[1][c] + m[r][2]*m2.m[2][c] + m[r][3]*m2.m[3][c];
		return m3;
	}
	//! Times operator for a vector
	Vector3 operator*( const Vector3 & v ) const
	{
		return Vector3(
			m[0][0]*v[0] + m[0][1]*v[1] + m[0][2]*v[2] + m[0][3],
			m[1][0]*v[0] + m[1][1]*v[1] + m[1][2]*v[2] + m[1][3],
			m[2][0]*v[0] + m[2][1]*v[1] + m[2][2]*v[2] + m[2][3] );
	}
	//! %Element operator
	float & operator()( unsigned int c, unsigned int d )
	{
		Q_ASSERT( c < 4 && d < 4 );
		return m[c][d];
	}
	//! Const element operator
	float operator()( unsigned int c, unsigned int d ) const
	{
		Q_ASSERT( c < 4 && d < 4 );
		return m[c][d];
	}
	
	// Not implemented; use decompose() instead
	/*
	Matrix rotation() const;
	Vector3 translation() const;
	Vector3 scale() const;
	*/
	
	//! Decompose into translation, rotation and scale
	void decompose( Vector3 & trans, Matrix & rot, Vector3 & scale ) const;
	//! Compose from translation, rotation and scale
	void compose( const Vector3 & trans, const Matrix & rot, const Vector3 & scale );
	
	//Matrix44 inverted() const;
	
	//! Format as HTML
	QString toHtml() const;
	
	//! %Data accessor
	const float * data() const { return (float *) m; }

protected:
	float m[4][4];
	static const float identity[16];
	
	friend class NifIStream;
	friend class NifOStream;
};

//! A transformation consisting of a translation, rotation and scale
class Transform
{
public:
	//! Constructor
	/*!
	 * Creates a transform from a NIF index.
	 *
	 * \param nif The model
	 * \param transform The index to create the transform from
	 */
	Transform( const NifModel * nif, const QModelIndex & transform );
	//! Default constructor
	Transform()	{ scale = 1.0; }
	
	//! Tests if a transform can be constructed
	static bool canConstruct( const NifModel * nif, const QModelIndex & parent );
	
	//! Writes a transform back to a NIF index
	void writeBack( NifModel * nif, const QModelIndex & transform ) const;
	
	//! Times operator
	friend Transform operator*( const Transform & t1, const Transform & t2 );
	
	//! Times operator
	Vector3 operator*( const Vector3 & v ) const
	{
		return rotation * v * scale + translation;
	}
	
	//! Returns a matrix holding the transform
	Matrix4 toMatrix4() const;
	
	// Format of rotation matrix? See http://en.wikipedia.org/wiki/Euler_angles
	// fromEuler indicates that it might be "zyx" form
	// Yaw, Pitch, Roll correspond to rotations in X, Y, Z axes respectively
	// Note that by convention, all angles in mathematics are anticlockwise, looking top-down at the plane
	// ie. rotation on X axis means "in the YZ plane where a positive X vector is up"
	// From entim: "The rotations are applied after each other: first yaw, then pitch, then roll"
	// For A,X,Y,Z representation, A is amplitude, each axis is a percentage
	Matrix rotation;
	Vector3 translation;
	float scale;
	
	friend QDataStream & operator<<( QDataStream & ds, const Transform & t );
	friend QDataStream & operator>>( QDataStream & ds, Transform & t );
	
	QString toString() const;
};

//! A triangle
class Triangle
{
public:
	//! Default constructor
	Triangle() { v[0] = v[1] = v[2] = 0; }
	//! Constructor
	Triangle( quint16 a, quint16 b, quint16 c ) { set( a, b, c ); }
	
	//! Array operator
	quint16 & operator[]( unsigned int i )
	{
		Q_ASSERT( i < 3 );
		return v[i];
	}
	//! Const array operator
	const quint16 & operator[]( unsigned int i ) const
	{
		Q_ASSERT( i < 3 );
		return v[i];
	}
	//! Sets the vertices of the triangle
	void set( quint16 a, quint16 b, quint16 c )
	{
		v[0] = a; v[1] = b; v[2] = c;
	}
	//! Gets the first vertex
	inline quint16 v1() const { return v[0]; }
	//! Gets the second vertex
	inline quint16 v2() const { return v[1]; }
	//! Gets the third vertex
	inline quint16 v3() const { return v[2]; }
	
	//! Flips the triangle face
	/*!
	 * Triangles are usually drawn anticlockwise(?); by changing the order of
	 * the vertices the triangle is flipped.
	 */
	void flip() { quint16 x = v[0]; v[0] = v[1]; v[1] = x; }
	
	//! Add operator
	Triangle operator+( quint16 d )
	{
		Triangle t( *this );
		t.v[0] += d;
		t.v[1] += d;
		t.v[2] += d;
		return t;
	}

protected:
	quint16 v[3];
	friend class NifIStream;
	friend class NifOStream;
	
	friend QDataStream & operator>>( QDataStream & ds, Triangle & t );
};

//! An attempt at providing a QDebug stream operator for Triangle
/**
 * See <a href="http://doc.trolltech.com/latest/debug.html">Qt Debugging</a>
 * for how this is supposed to work.
 */
inline QDebug &operator<<( QDebug dbg, Triangle t )
{
	dbg.nospace() << "(" << t[0] << "," << t[1] << "," << t[2] << ")";
	return dbg.space();
}

//! A stream operator for reading in a Triangle
inline QDataStream & operator>>( QDataStream & ds, Triangle & t )
{
	ds >> t.v[0] >> t.v[1] >> t.v[2];
	return ds;
}

//! Clamps a float to have a value between 0 and 1 inclusive
inline float clamp01( float a )
{
	if ( a < 0 )	return 0;
	if ( a > 1 )	return 1;
	return a;
}

//! A 3 value color (RGB)
class Color3
{
public:
	//! Default constructor
	Color3() { rgb[0] = rgb[1] = rgb[2] = 1.0; }
	//! Constructor
	Color3( float r, float g, float b ) { setRGB( r, g, b ); }
	//! Constructor
	explicit Color3( const QColor & c ) { fromQColor( c ); }
	//! Constructor
	explicit Color3( const Vector3 & v ) { fromVector3( v ); }
	//! Constructor
	/*!
	 * Construct a Color3 from a Color4 by truncating.
	 * Doxygen can't document this due to a circular dependency?
	 */
	explicit Color3( const class Color4 & c4 );
	
	//! Array operator
	float & operator[]( unsigned int i )
	{
		Q_ASSERT( i < 3 );
		return rgb[i];
	}
	
	//! Const array operator
	const float & operator[]( unsigned int i ) const
	{
		Q_ASSERT( i < 3 );
		return rgb[i];
	}
	
	//! Times operator
	Color3 operator*( float x ) const
	{
		Color3 c( *this );
		c.rgb[0] *= x;
		c.rgb[1] *= x;
		c.rgb[2] *= x;
		return c;
	}
	
	//! Add-equals operator
	Color3 & operator+=( const Color3 & o )
	{
		for ( int x = 0; x < 3; x++ )
			rgb[x] += o.rgb[x];
		return *this;
	}
	
	//! Minus-equals operator
	Color3 & operator-=( const Color3 & o )
	{
		for ( int x = 0; x < 3; x++ )
			rgb[x] -= o.rgb[x];
		return *this;
	}
	
	//! Add operator
	Color3 operator+( const Color3 & o ) const
	{
		Color3 c( *this );
		return ( c += o );
	}
	
	//! Minus operator
	Color3 operator-( const Color3 & o ) const
	{
		Color3 c( *this );
		return ( c -= o );
	}
	
	//! Get the red component
	float red() const { return rgb[0]; }
	//! Get the green component
	float green() const { return rgb[1]; }
	//! Get the blue component
	float blue() const { return rgb[2]; }
	
	//! Set the red component
	void setRed( float r ) { rgb[0] = r; }
	//! Set the green component
	void setGreen( float g ) { rgb[1] = g; }
	//! Set the blue component
	void setBlue( float b ) { rgb[2] = b; }
	
	//! Set the color components
	void setRGB( float r, float g, float b ) { rgb[0] = r; rgb[1] = g; rgb[2] = b; }
	
	//! Convert to QColor
	QColor toQColor() const
	{
		return QColor::fromRgbF( clamp01( rgb[0] ), clamp01( rgb[1] ), clamp01( rgb[2] ) );
	}
	
	//! Set from QColor
	void fromQColor( const QColor & c )
	{
		rgb[0] = c.redF();
		rgb[1] = c.greenF();
		rgb[2] = c.blueF();
	}
	
	//! Set from vector
	void fromVector3( const Vector3 & v )
	{
		rgb[0] = v[0];
		rgb[1] = v[1];
		rgb[2] = v[2];
	}
	
	//! %Data accessor
	const float * data() const { return rgb; }
	
protected:
	float rgb[3];
	
	friend class NifIStream;
	friend class NifOStream;
};

//! A 4 value color (RGBA)
class Color4
{
public:
	//! Default constructor
	Color4() { rgba[0] = rgba[1] = rgba[2] = rgba[3] = 1.0; }
	//! Constructor
	explicit Color4( const Color3 & c, float alpha = 1.0 ) { rgba[0] = c[0]; rgba[1] = c[1]; rgba[2] = c[2]; rgba[3] = alpha; }
	//! Constructor
	explicit Color4( const QColor & c ) { fromQColor( c ); }
	//! Constructor
	Color4( float r, float g, float b, float a ) { setRGBA( r, g, b, a ); }
	
	//! Array operator
	float & operator[]( unsigned int i )
	{
		Q_ASSERT( i < 4 );
		return rgba[ i ];
	}
	
	//! Const array operator
	const float & operator[]( unsigned int i ) const
	{
		Q_ASSERT( i < 4 );
		return rgba[ i ];
	}
	
	//! Times operator
	Color4 operator*( float x ) const
	{
		Color4 c( *this );
		c.rgba[0] *= x;
		c.rgba[1] *= x;
		c.rgba[2] *= x;
		c.rgba[3] *= x;
		return c;
	}
	
	//! Add-equals operator
	Color4 & operator+=( const Color4 & o )
	{
		for ( int x = 0; x < 4; x++ )
			rgba[x] += o.rgba[x];
		return *this;
	}
	
	//! Minus-equals operator
	Color4 & operator-=( const Color4 & o )
	{
		for ( int x = 0; x < 4; x++ )
			rgba[x] -= o.rgba[x];
		return *this;
	}
	
	//! Add operator
	Color4 operator+( const Color4 & o ) const
	{
		Color4 c( *this );
		return ( c += o );
	}
	
	//! Minus operator
	Color4 operator-( const Color4 & o ) const
	{
		Color4 c( *this );
		return ( c -= o );
	}
	
	//! Equality operator
	bool operator==( const Color4 & c ) const
	{
		return rgba[0] == c.rgba[0] && rgba[1] == c.rgba[1] && rgba[2] == c.rgba[2] && rgba[3] == c.rgba[3];
	}
	
	//! Get the red component
	float red() const { return rgba[0]; }
	//! Get the green component
	float green() const { return rgba[1]; }
	//! Get the blue component
	float blue() const { return rgba[2]; }
	//! Get the alpha component
	float alpha() const { return rgba[3]; }
	
	//! Set the red component
	void setRed( float r ) { rgba[0] = r; }
	//! Set the green component
	void setGreen( float g ) { rgba[1] = g; }
	//! Set the blue component
	void setBlue( float b ) { rgba[2] = b; }
	//! Set the alpha component
	void setAlpha( float a ) { rgba[3] = a; }
	
	//! Set the color components
	void setRGBA( float r, float g, float b, float a ) { rgba[ 0 ] = r; rgba[ 1 ] = g; rgba[ 2 ] = b; rgba[ 3 ] = a; }

	//! Convert to QColor
	QColor toQColor() const
	{
		return QColor::fromRgbF( clamp01( rgba[0] ), clamp01( rgba[1] ), clamp01( rgba[2] ), clamp01( rgba[3] ) );
	}
	
	//! Set from QColor
	void fromQColor( const QColor & c )
	{
		rgba[0] = c.redF();
		rgba[1] = c.greenF();
		rgba[2] = c.blueF();
		rgba[3] = c.alphaF();
	}
	
	//! %Data accessor
	const float * data() const { return rgba; }
	
	//! Alpha blend
	Color4 blend( float alpha ) const
	{
		Color4 c( *this );
		c.setAlpha( c.alpha() * alpha );
		return c;
	}

protected:
	float rgba[4];
	
	friend class NifIStream;
	friend class NifOStream;

	friend QDataStream & operator>>( QDataStream & ds, Color4 & c );
};

// This refuses to document properly in doxygen.
inline Color3::Color3( const Color4 & c4 )
{
	rgb[0] = c4[0];
	rgb[1] = c4[1];
	rgb[2] = c4[2];
}

//! A stream operator for reading in a Color4
inline QDataStream & operator>>( QDataStream & ds, Color4 & c )
{
	ds >> c.rgba[0] >> c.rgba[1] >> c.rgba[2] >> c.rgba[3];
	return ds;
}

//! A fixed length vector of type T.
/*!
 * %Data is allocated into a vector portion and the data section.
 * The vector simply points to appropriate places in the data section.
 * @param   T   Type of Vector
 */
template<typename T>
class FixedMatrix
{
public:
	//! Default Constructor:  Allocates empty vector
	FixedMatrix() : v_( NULL ), len0(0), len1(0)
	{}

	//! Size Constructor
	/*!
	 * Allocate the requested number of elements.
	 */
	FixedMatrix(int length1, int length2)
	{
		int length = length1*length2;
		v_ = (T*)qMalloc(sizeof(T)*length);
		len0 = length1;
		len1 = length2;
	}

	//! Copy Constructor
	FixedMatrix(const FixedMatrix& other)
	{
		int datalen = other.count();
		len0 = other.count(0);
		len1 = other.count(1);
		v_ = (T*)qMalloc(sizeof(T) * datalen);
		qMemCopy( array(), other.array(), datalen );
	}

	//! Default Destructor
	~FixedMatrix()
	{ qFree(v_); }

	//! Copy Assignment
	FixedMatrix& operator=(const FixedMatrix& other)         
	{
		FixedMatrix tmp( other );
		swap( tmp );
		return *this;
	}

	//! Accessor for...?
	T* operator[](int index)
	{
		// assert( index >= 0 && index < len_ )
		return &v_[index*count(0) + 0];
	} 

	//! Accessor for element (i,j) in the matrix
	const T& operator[](int index) const
	{
		// assert( index >= 0 && index < len_ )
		return &v_[index*count(0) + 0];
	} 

	//! Accessor for element (i,j) in the matrix
	T* operator()(int index)
	{
		// assert( index >= 0 && index < len_ )
		return &v_[index*count(0) + 0];
	} 

	//! Accessor for element (i,j) in the matrix
	T& operator()(int index1, int index2)
	{
		// assert( index >= 0 && index < len_ )
		return element(index1, index2);
	} 

	//! Accessor for element (i,j) in the matrix
	operator T*() const
	{
		return array();
	}

	//! Accessor for element (i,j) in the matrix
	T& element(int index1, int index2)
	{
		return v_[ calcindex(index1, index2) ];
	}

	//! Calculates row? of element
	/*!
	 *  @param[in]  index1  i value of element
	 *  @param[in]  index2  j value of element
	 *  @return             position of element?
	 */
	int calcindex(int index1, int index2)
	{
		return index1*count(1) + index2;
	}

	//! Number of items in the vector.
	int count() const 
	{ return len0 * len1; }

	//! Number of items in specified dimension.
	int count(int dimension) const 
	{ return (dimension == 0 ? len0 : (dimension == 1 ? len1 : 0)); }

	//! Start of the array portion of the vector
	T*array() const { return v_; }
	// Department of Rendundancy Department?
	//! Start of the array portion of the vector
	T*data() const { return v_; }

	//! Assign a string to vector at specified index.
	/*!
	 *  @param[in]  index1  Index (i) in array to assign
	 *  @param[in]  index2  Index (j) in array to assign
	 *  @param[in]   value  Value to copy into string
	 */
	void assign(int index1, int index2, T value)
	{
		element(index1, index2) = value;
	}

	//! Swap contents with another FixedMatrix
	/*!
	 *  @param[in,out]  other   Other vector to swap with
	 */
	void swap( FixedMatrix &other )
	{
		qSwap(v_, other.v_);
		qSwap(len0, other.len0);
		qSwap(len1, other.len1);
	}

private:
	T* v_; //!< Vector data
	int len0; //!< Length in first dimension
	int len1; //!< Length in second dimension
};

//! 
typedef FixedMatrix<char> ByteMatrix;

#endif
