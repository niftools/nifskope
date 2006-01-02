#ifndef GLMATH_H
#define GLMATH_H

#include <QtOpenGL>

#include "nifmodel.h"
#include "niftypes.h"


class Transform
{
public:
	Transform( NifModel * nif, const QModelIndex & transform )
	{
		rotation = nif->itemValue( nif->getIndex( transform, "rotation" ) );
		translation = nif->itemValue( nif->getIndex( transform, "translation" ) );
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
				f[ c*4 + d ] = rotation( d, c );
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
				f[ c*4 + d ] = rotation( d, c );
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
