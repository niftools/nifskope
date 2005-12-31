#ifndef GLMATH_H
#define GLMATH_H

#include <QtOpenGL>

#include "nifmodel.h"

#include "math.h"

class Vector
{
public:
	Vector()
	{
		xyz[ 0 ] = xyz[ 1 ] = xyz[ 2 ] = 0.0;
	}
	Vector( GLfloat x, GLfloat y, GLfloat z )
	{
		xyz[0] = x;
		xyz[1] = y;
		xyz[2] = z;
	}
	Vector( NifModel * model, const QModelIndex & index )
	{
		if ( index.isValid() )
		{
			xyz[0] = model->getFloat( index, "x" );
			xyz[1] = model->getFloat( index, "y" );
			xyz[2] = model->getFloat( index, "z" );
		}
		else
			xyz[ 0 ] = xyz[ 1 ] = xyz[ 2 ] = 0.0;
	}
	Vector( const Vector & v )
	{
		operator=( v );
	}
	Vector & operator=( const Vector & v )
	{
		xyz[0] = v.xyz[0];
		xyz[1] = v.xyz[1];
		xyz[2] = v.xyz[2];
		return *this;
	}
	Vector & operator+=( const Vector & v )
	{
		xyz[0] += v[0];
		xyz[1] += v[1];
		xyz[2] += v[2];
		return *this;
	}
	Vector & operator-=( const Vector & v )
	{
		xyz[0] -= v[0];
		xyz[1] -= v[1];
		xyz[2] -= v[2];
		return *this;
	}
	Vector & operator*=( GLfloat s )
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
	Vector operator*( GLfloat s ) const
	{
		Vector v( *this );
		return v *= s;
	}
	
	GLfloat & operator[]( unsigned int i )
	{
		if ( i >= 3 ) qDebug( "vector index out of bounds" );
		return xyz[i];
	}
	const GLfloat & operator[]( unsigned int i ) const
	{
		if ( i >= 3 ) qDebug( "vector index out of bounds" );
		return xyz[i];
	}
	
	void glVertex() const
	{
		glVertex3f( xyz[0], xyz[1], xyz[2] );
	}
	void glNormal() const
	{
		glNormal3f( xyz[0], xyz[1], xyz[2] );
	}
	void glTranslate() const
	{
		glTranslatef( xyz[0], xyz[1], xyz[2] );
	}
	
	GLfloat length() const
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
	
protected:
	GLfloat xyz[3];
};

class Quat
{
public:
	Quat()
	{ wxyz[ 0 ] = 1.0; wxyz[ 1 ] = 0.0; wxyz[ 2 ] = 0.0; wxyz[ 3 ] = 0.0;	}
	Quat( NifModel * nif, const QModelIndex & index )
	{
		if ( ! index.isValid() )
		{
			wxyz[ 0 ] = 1.0; wxyz[ 1 ] = 0.0; wxyz[ 2 ] = 0.0; wxyz[ 3 ] = 0.0;
			return;
		}
		wxyz[0] = nif->getFloat( index, "w" );
		wxyz[1] = nif->getFloat( index, "x" );
		wxyz[2] = nif->getFloat( index, "y" );
		wxyz[3] = nif->getFloat( index, "z" );
	}
	
	float & operator[]( int i )
	{
		return wxyz[ i ];
	}
	
	const float & operator[]( int i ) const
	{
		return wxyz[ i ];
	}
	
	Quat & operator*=( GLfloat s )
	{
		for ( int c = 0; c < 4; c++ )
			wxyz[ c ] *= s;
		return *this;
	}
	Quat operator*( GLfloat s ) const
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
	
	static GLfloat dotproduct( const Quat & q1, const Quat & q2 )
	{
		return q1[0]*q2[0]+q1[1]*q2[1]+q1[2]*q2[2]+q1[3]*q2[3];
	}
	
	static Quat interpolate( const Quat & q1, const Quat & q2, GLfloat x )
	{
		GLfloat a = acos( dotproduct( q1, q2 ) );
		
		if ( fabs( a ) >= 0.00005 )
		{
			GLfloat i = 1.0 / sin( a );
			GLfloat y = 1.0 - x;
			GLfloat f1 = sin( y * a ) * i;
			GLfloat f2 = sin( x * a ) * i;
			return q1*f1 + q2*f2;
		}
		else
			return q1;
	}
	
protected:
	GLfloat	wxyz[4];
};

class Matrix
{
public:
	Matrix()
	{
		for ( int r = 0; r < 3; r++ )
			for ( int c = 0; c < 3; c++ )
				m[r][c] = ( r == c ? 1.0 : 0.0 );
	}
	Matrix( NifModel * model, const QModelIndex & matrix )
	{
		for ( int r = 0; r < 3; r++ )
			for ( int c = 0; c < 3; c++ )
				m[r][c] = ( r == c ? 1.0 : 0.0 );
		
		if ( matrix.isValid() && model->rowCount( matrix ) == 9 )
		{
			for ( int r = 0; r < 9; r++ )
				m[r%3][r/3] = model->itemValue( model->index( r, 0, matrix ) ).toDouble();
		}
	}
	Matrix( const Matrix & m2 )
	{
		operator=( m2 );
	}
	Matrix & operator=( const Matrix & m2 )
	{
		for ( int r = 0; r < 3; r++ )
			for ( int c = 0; c < 3; c++ )
				m[r][c] = m2.m[r][c];
		return *this;
	}
	Matrix operator*( const Matrix & m2 ) const
	{
		Matrix m3;
		for ( int r = 0; r < 3; r++ )
			for ( int c = 0; c < 3; c++ )
				m3.m[c][r] = m[0][r]*m2.m[c][0] + m[1][r]*m2.m[c][1] + m[2][r]*m2.m[c][2];
		return m3;
	}
	Vector operator*( const Vector & v ) const
	{
		return Vector(
			m[0][0]*v[0] + m[1][0]*v[1] + m[2][0]*v[2],
			m[0][1]*v[0] + m[1][1]*v[1] + m[2][1]*v[2],
			m[0][2]*v[0] + m[1][2]*v[1] + m[2][2]*v[2] );
	}
	GLfloat & operator()( int c, int d )
	{
		if ( c >= 0 && c < 3 && d >= 0 && d < 3 )
			return m[c][d];
		return m[0][0];
	}
	GLfloat operator()( int c, int d ) const
	{
		if ( c >= 0 && c < 3 && d >= 0 && d < 3 )
			return m[c][d];
		return 0;
	}
	
	Matrix & operator=( const Quat & q )
	{
		GLfloat fTx  = ((GLfloat)2.0)*q[1];
		GLfloat fTy  = ((GLfloat)2.0)*q[2];
		GLfloat fTz  = ((GLfloat)2.0)*q[3];
		GLfloat fTwx = fTx*q[0];
		GLfloat fTwy = fTy*q[0];
		GLfloat fTwz = fTz*q[0];
		GLfloat fTxx = fTx*q[1];
		GLfloat fTxy = fTy*q[1];
		GLfloat fTxz = fTz*q[1];
		GLfloat fTyy = fTy*q[2];
		GLfloat fTyz = fTz*q[2];
		GLfloat fTzz = fTz*q[3];
		
		m[0][0] = (GLfloat)1.0-(fTyy+fTzz);
		m[1][0] = fTxy-fTwz;
		m[2][0] = fTxz+fTwy;
		m[0][1] = fTxy+fTwz;
		m[1][1] = (GLfloat)1.0-(fTxx+fTzz);
		m[2][1] = fTyz-fTwx;
		m[0][2] = fTxz-fTwy;
		m[1][2] = fTyz+fTwx;
		m[2][2] = (GLfloat)1.0-(fTxx+fTyy);
		return *this;
	}
	
	static Matrix rotX( GLfloat x )
	{
		GLfloat sinX = sin( x );
		GLfloat cosX = cos( x );
		Matrix m;
		m.m[1][1] =  cosX;	m.m[2][1] = -sinX;
		m.m[1][2] =  sinX;	m.m[2][2] =  cosX;
		return m;
	}
	static Matrix rotY( GLfloat y )
	{
		GLfloat sinY = sin( y );
		GLfloat cosY = cos( y );
		Matrix m;
		m.m[0][0] =  cosY;	m.m[2][0] =  sinY;
		m.m[0][2] = -sinY;	m.m[2][2] =  cosY;
		return m;
	}
	static Matrix rotZ( GLfloat z )
	{
		GLfloat sinZ = sin( z );
		GLfloat cosZ = cos( z );
		Matrix m;
		m.m[0][0] =  cosZ;	m.m[1][0] = -sinZ;
		m.m[0][1] =  sinZ;	m.m[1][1] =  cosZ;
		return m;
	}
protected:
	GLfloat m[3][3];	
};

class Transform
{
public:
	Transform( NifModel * nif, const QModelIndex & transform )
	{
		rotation = Matrix( nif, nif->getIndex( transform, "rotation" ) );
		translation = Vector( nif, nif->getIndex( transform, "translation" ) );
		scale = nif->getFloat( transform, "scale" );
	}
	
	Transform()
	{
		scale = 1.0;
	}
	
	Transform operator*( const Transform & m ) const
	{
		Transform t;
		t.translation = rotation * m.translation + translation * m.scale;
		t.rotation = rotation * m.rotation;
		t.scale = m.scale * scale;
		return t;
	}
	
	Vector operator*( const Vector & v ) const
	{
		return rotation * v * scale + translation;
	}

	inline void glMultMatrix() const
	{
		GLfloat f[16];
		for ( int c = 0; c < 3; c++ )
		{
			for ( int d = 0; d < 3; d++ )
				f[ c*4 + d ] = rotation( c, d );
			f[ 3*4 + c ] = translation[ c ];
		}
		f[  0 ] *= scale;
		f[  5 ] *= scale;
		f[ 10 ] *= scale;
		
		f[  3 ] = 0.0;
		f[  7 ] = 0.0;
		f[ 11 ] = 0.0;
		f[ 15 ] = 1.0;
		
		glMultMatrixf( f );
	}
	
	inline void glLoadMatrix() const
	{
		GLfloat f[16];
		for ( int c = 0; c < 3; c++ )
		{
			for ( int d = 0; d < 3; d++ )
				f[ c*4 + d ] = rotation( c, d );
			f[ 3*4 + c ] = translation[ c ];
		}
		f[  0 ] *= scale;
		f[  5 ] *= scale;
		f[ 10 ] *= scale;
		
		f[  3 ] = 0.0;
		f[  7 ] = 0.0;
		f[ 11 ] = 0.0;
		f[ 15 ] = 1.0;
		
		glLoadMatrixf( f );
	}

	Matrix rotation;
	Vector translation;
	GLfloat scale;
};

#endif
