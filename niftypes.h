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

class Vector
{
public:
	Vector()
	{
		xyz[ 0 ] = xyz[ 1 ] = xyz[ 2 ] = 0.0;
	}
	Vector( float x, float y, float z )
	{
		xyz[0] = x;
		xyz[1] = y;
		xyz[2] = z;
	}
	Vector( QIODevice & device )
	{
		read( device );
	}
	Vector( const QVariant & v )
	{
		if ( ! v.canConvert<Vector>() )
			qWarning( "Vector::Vector( QVariant )" );
		operator=( v.value<Vector>() );
	}
	void read( QIODevice & device )
	{
		device.read( (char *) xyz, 12 );
	}
	void write( QIODevice & device )
	{
		device.write( (char *) xyz, 12 );
	}
	Vector & operator+=( const Vector & v )
	{
		xyz[0] += v.xyz[0];
		xyz[1] += v.xyz[1];
		xyz[2] += v.xyz[2];
		return *this;
	}
	Vector & operator-=( const Vector & v )
	{
		xyz[0] -= v.xyz[0];
		xyz[1] -= v.xyz[1];
		xyz[2] -= v.xyz[2];
		return *this;
	}
	Vector & operator*=( float s )
	{
		xyz[ 0 ] *= s;
		xyz[ 1 ] *= s;
		xyz[ 2 ] *= s;
		return *this;
	}
	Vector operator+( Vector v )
	{
		Vector w( *this );
		return w += v;
	}
	Vector operator-( Vector v )
	{
		Vector w( *this );
		return w -= v;
	}
	Vector operator*( float s ) const
	{
		Vector v( *this );
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
	
	static Vector min( const Vector & v1, const Vector & v2 )
	{
		return Vector( qMin( v1.xyz[0], v2.xyz[0] ), qMin( v1.xyz[1], v2.xyz[1] ), qMin( v1.xyz[2], v2.xyz[2] ) );
	}
	static Vector max( const Vector & v1, const Vector & v2 )
	{
		return Vector( qMax( v1.xyz[0], v2.xyz[0] ), qMax( v1.xyz[1], v2.xyz[1] ), qMax( v1.xyz[2], v2.xyz[2] ) );
	}
	
	operator QVariant () const
	{
		QVariant v;
		v.setValue( *this );
		return v;
	}
	
	operator QString () const
	{
		return QString( "x %1 y %2 z %3" ).arg( xyz[0] ).arg( xyz[1] ).arg( xyz[2] );
	}
	
protected:
	float xyz[3];
};

class Quat
{
public:
	Quat()
	{
		memcpy( wxyz, identity, 16 );
	}
	Quat( const QVariant & v )
	{
		if ( ! v.canConvert<Quat>() )
			qWarning( "Quat::Quat( QVariant )" );
		operator=( v.value<Quat>() );
	}
	Quat( QIODevice & device )
	{
		read( device );
	}
	void read( QIODevice & device )
	{
		device.read( (char *) wxyz, 16 );
	}
	void write( QIODevice & device )
	{
		device.write( (char *) wxyz, 16 );
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
	static Quat interpolate( const Quat & q1, const Quat & q2, float x )
	{
		float a = acos( dotproduct( q1, q2 ) );
		
		if ( fabs( a ) >= 0.00005 )
		{
			float i = 1.0 / sin( a );
			float y = 1.0 - x;
			float f1 = sin( y * a ) * i;
			float f2 = sin( x * a ) * i;
			return q1*f1 + q2*f2;
		}
		else
			return q1;
	}
	operator QVariant() const
	{
		QVariant v;
		v.setValue( *this );
		return v;
	}
	
	QString toHtml() const
	{
		return QString( "W %1<br>X %2<br>Y %3<br>Z %4" ).arg( wxyz[0], 0, 'f', 4 ).arg( wxyz[1], 0, 'f', 4 ).arg( wxyz[2], 0, 'f', 4 ).arg( wxyz[3], 0, 'f', 4 );
	}
	
protected:
	float	wxyz[4];
	static const float identity[4];
};

class Matrix
{
public:
	Matrix()
	{
		memcpy( m, identity, 36 );
	}
	Matrix( const QVariant & v )
	{
		if ( ! v.canConvert<Matrix>() )
			qWarning( "Matrix::Matrix( QVariant )" );
		operator=( v.value<Matrix>() );
	}
	Matrix( QIODevice & device )
	{
		read( device );
	}
	void read( QIODevice & device )
	{
		device.read( (char *) m, 36 );
	}
	void write( QIODevice & device )
	{
		device.write( (char *) m, 36 );
	}
	Matrix & operator=( const QVariant & v )
	{
		if ( ! v.canConvert<Matrix>() )
			qWarning( "Matrix::Matrix( QVariant )" );
		return operator=( v.value<Matrix>() );
	}
	Matrix operator*( const Matrix & m2 ) const
	{
		Matrix m3;
		for ( int r = 0; r < 3; r++ )
			for ( int c = 0; c < 3; c++ )
				m3.m[r][c] = m[r][0]*m2.m[0][c] + m[r][1]*m2.m[1][c] + m[r][2]*m2.m[2][c];
		return m3;
	}
	Vector operator*( const Vector & v ) const
	{
		return Vector(
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
	
	Matrix & operator=( const Quat & q )
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
		return *this;
	}
	
	Quat toQuat() const
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
	
	static Matrix fromEuler( float x, float y, float z )
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
		
		Matrix m;
		m( 0, 0 ) = cosY * cosZ;
		m( 0, 1 ) = - cosY * sinZ;
		m( 0, 2 ) = sinY;
		m( 1, 0 ) = sinX * sinY * cosZ + sinZ * cosX;
		m( 1, 1 ) = cosX * cosZ - sinX * sinY * sinZ;
		m( 1, 2 ) = - sinX * cosY;
		m( 2, 0 ) = sinX * sinZ - cosX * sinY * cosZ;
		m( 2, 1 ) = cosX * sinY * sinZ + sinX * cosZ;
		m( 2, 2 ) = cosX * cosY;
		return m;
	}
	
	bool toEuler( float & x, float & y, float & z ) const
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
	
	QString toHtml() const
	{
		return QString( "<table>" )
			+ QString( "<tr><td>%1</td><td>%2</td><td>%3</td></tr>" ).arg( m[0][0], 0, 'f', 4 ).arg( m[0][1], 0, 'f', 4 ).arg( m[0][2], 0, 'f', 4 )
			+ QString( "<tr><td>%1</td><td>%2</td><td>%3</td></tr>" ).arg( m[1][0], 0, 'f', 4 ).arg( m[1][1], 0, 'f', 4 ).arg( m[1][2], 0, 'f', 4 )
			+ QString( "<tr><td>%1</td><td>%2</td><td>%3</td></tr>" ).arg( m[2][0], 0, 'f', 4 ).arg( m[2][1], 0, 'f', 4 ).arg( m[2][2], 0, 'f', 4 )
			+ QString( "</table>" );
	}
	
	operator QVariant() const
	{
		QVariant v;
		v.setValue( *this );
		return v;
	}
	
protected:
	float m[3][3];
	static const float identity[9];
};


Q_DECLARE_METATYPE( Vector )
Q_DECLARE_METATYPE( Quat )
Q_DECLARE_METATYPE( Matrix )


#endif
