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
	Vector & operator*=( GLfloat s )
	{
		xyz[ 0 ] *= s;
		xyz[ 1 ] *= s;
		xyz[ 2 ] *= s;
		return *this;
	}
	Vector operator*( GLfloat s )
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

class Matrix
{
public:
	Matrix()
	{
		for ( int r = 0; r < 4; r++ )
			for ( int c = 0; c < 4; c++ )
				m[r][c] = ( r == c ? 1.0 : 0.0 );
	}
	Matrix( NifModel * model, const QModelIndex & index )
	{
		for ( int r = 0; r < 4; r++ )
			for ( int c = 0; c < 4; c++ )
				m[r][c] = ( r == c ? 1.0 : 0.0 );
		
		if ( !index.isValid() ) return;
		
		QModelIndex rotation = model->getIndex( index, "rotation" );
		if ( rotation.isValid() && model->rowCount( rotation ) == 9 )
		{
			for ( int r = 0; r < 9; r++ )
				m[r%3][r/3] = model->itemValue( model->index( r, 0, rotation ) ).toDouble();
		}
		
		QModelIndex translation = model->getIndex( index, "translation" );
		if ( translation.isValid() )
		{
			m[3][0] = model->getFloat( translation, "x" );
			m[3][1] = model->getFloat( translation, "y" );
			m[3][2] = model->getFloat( translation, "z" );
		}
		
		GLfloat scale = model->getFloat( index, "scale" );
		if ( scale != 1.0 )
		{
			m[0][0] *= scale;
			m[1][1] *= scale;
			m[2][2] *= scale;
		}
	}
	Matrix( const Matrix & m2 )
	{
		operator=( m2 );
	}
	Matrix & operator=( const Matrix & m2 )
	{
		for ( int r = 0; r < 4; r++ )
			for ( int c = 0; c < 4; c++ )
				m[r][c] = m2.m[r][c];
		return *this;
	}
	Matrix operator*( const Matrix & m2 ) const
	{
		Matrix m3;
		for ( int r = 0; r < 4; r++ )
			for ( int c = 0; c < 4; c++ )
				m3.m[c][r] = m[0][r]*m2.m[c][0] + m[1][r]*m2.m[c][1] + m[2][r]*m2.m[c][2] + m[3][r]*m2.m[c][3];
		return m3;
	}
	Vector operator*( const Vector & v ) const
	{
		return Vector(
			m[0][0]*v[0] + m[1][0]*v[1] + m[2][0]*v[2] + m[3][0],
			m[0][1]*v[0] + m[1][1]*v[1] + m[2][1]*v[2] + m[3][1],
			m[0][2]*v[0] + m[1][2]*v[1] + m[2][2]*v[2] + m[3][2] );
	}
	Matrix & operator+=( const Vector & v )
	{
		m[3][0] += v[0];
		m[3][1] += v[1];
		m[3][2] += v[2];
		return *this;
	}
	Matrix operator+( const Vector & v ) const
	{
		Matrix m( *this );
		return ( m += v );
	}
	
	inline void glMultMatrix() const
	{
		glMultMatrixf( m[0] );
	}
	
	inline void glLoadMatrix() const
	{
		glLoadMatrixf( m[0] );
	}
	
	void clearTrans()
	{
		m[3][0] = 0.0;
		m[3][1] = 0.0;
		m[3][2] = 0.0;
	}
	
	static Matrix trans( GLfloat x, GLfloat y, GLfloat z )
	{
		Matrix r;
		r.m[3][0] = x;
		r.m[3][1] = y;
		r.m[3][2] = z;
		return r;
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
	GLfloat m[4][4];	
};

#endif
