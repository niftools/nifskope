#ifndef GLMATH_H
#define GLMATH_H

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
		xyz[0] = model->getFloat( index, "x" );
		xyz[1] = model->getFloat( index, "y" );
		xyz[2] = model->getFloat( index, "z" );
	}
	Vector( const Vector & v )
	{
		operator=( v );
	}
	void operator=( const Vector & v )
	{
		xyz[0] = v.xyz[0];
		xyz[1] = v.xyz[1];
		xyz[2] = v.xyz[2];
	}
	void operator+=( const Vector & v )
	{
		xyz[0] += v[0];
		xyz[1] += v[1];
		xyz[2] += v[2];
	}
	Vector operator*( GLfloat s )
	{
		Vector v;
		v.xyz[ 0 ] = xyz[ 0 ] * s;
		v.xyz[ 1 ] = xyz[ 1 ] * s;
		v.xyz[ 2 ] = xyz[ 2 ] * s;
		return v;
	}
	
	inline GLfloat & operator[]( int i )
	{
		return xyz[i];
	}
	inline const GLfloat & operator[]( int i ) const
	{
		return xyz[i];
	}
	
	inline void glVertex() const
	{
		glVertex3f( xyz[0], xyz[1], xyz[2] );
	}
	inline void glNormal() const
	{
		glNormal3f( xyz[0], xyz[1], xyz[2] );
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
		m[3][0] = model->getFloat( translation, "x" );
		m[3][1] = model->getFloat( translation, "y" );
		m[3][2] = model->getFloat( translation, "z" );
		
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
	void operator=( const Matrix & m2 )
	{
		for ( int r = 0; r < 4; r++ )
			for ( int c = 0; c < 4; c++ )
				m[r][c] = m2.m[r][c];
	}
	Matrix operator*( const Matrix & m2 ) const
	{
		Matrix r;
		for ( int i = 0; i < 4; i++ )
			for ( int j = 0; j < 4; j++ )
				r.m[i][j] = m2.m[i][0]*m[0][j] + m2.m[i][1]*m[1][j] + m2.m[i][2]*m[2][j] + m2.m[i][3]*m[3][j];
		return r;
	}
	Vector operator*( const Vector & v ) const
	{
		return Vector(
			m[0][0]*v[0] + m[1][0]*v[1] + m[2][0]*v[2] + m[3][0],
			m[0][1]*v[0] + m[1][1]*v[1] + m[2][1]*v[2] + m[3][1],
			m[0][2]*v[0] + m[1][2]*v[1] + m[2][2]*v[2] + m[3][2] );
	}
	
	inline void glMultMatrix() const
	{
		glMultMatrixf( m[0] );
	}

protected:
	GLfloat m[4][4];	
};

#endif
