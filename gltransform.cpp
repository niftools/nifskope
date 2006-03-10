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

#include "gltransform.h"

#include "nifmodel.h"

#include <QtOpenGL>

/*
 *  Transform
 */

Transform operator*( const Transform & t1, const Transform & t2 )
{
	Transform t;
	t.rotation = t1.rotation * t2.rotation;
	t.translation = t1.translation + t1.rotation * t2.translation * t1.scale;
	t.scale = t1.scale * t2.scale;
	return t;
}

bool Transform::canConstruct( const NifModel * nif, const QModelIndex & parent )
{
	return nif && parent.isValid() && nif->getIndex( parent, "Rotation" ).isValid()
		&& nif->getIndex( parent, "Translation" ).isValid()
		&& nif->getIndex( parent, "Scale" ).isValid();
}
 
Transform::Transform( const NifModel * nif, const QModelIndex & transform )
{
	rotation = nif->get<Matrix>( transform, "Rotation" );
	translation = nif->get<Vector3>( transform, "Translation" );
	scale = nif->get<float>( transform, "Scale" );
}

void Transform::writeBack( NifModel * nif, const QModelIndex & transform ) const
{
	nif->set<Matrix>( transform, "Rotation", rotation );
	nif->set<Vector3>( transform, "Translation", translation );
	nif->set<float>( transform, "Scale", scale );
}

void makeGLmatrix( const Transform & t, GLfloat f[] )
{
	for ( int c = 0; c < 3; c++ )
	{
		for ( int d = 0; d < 3; d++ )
			f[ c*4 + d ] = t.rotation( d, c ) * t.scale;
		f[ 3*4 + c ] = t.translation[ c ];
	}
	f[  3 ] = 0.0;
	f[  7 ] = 0.0;
	f[ 11 ] = 0.0;
	f[ 15 ] = 1.0;
}

void Transform::glMultMatrix() const
{
	GLfloat f[16];
	makeGLmatrix( *this, f );
	glMultMatrixf( f );
}

void Transform::glLoadMatrix() const
{
	GLfloat f[16];
	makeGLmatrix( *this, f );
	glLoadMatrixf( f );
}

QDataStream & operator<<( QDataStream & ds, const Transform & t )
{
	for ( int x = 0; x < 3; x++ )
	{
		for ( int y = 0; y < 3; y++ )
			ds << t.rotation( x, y );
		ds << t.translation[ x ];
	}
	ds << t.scale;
	return ds;
}

QDataStream & operator>>( QDataStream & ds, Transform & t )
{
	for ( int x = 0; x < 3; x++ )
	{
		for ( int y = 0; y < 3; y++ )
			ds >> t.rotation( x, y );
		ds >> t.translation[ x ];
	}
	ds >> t.scale;
	return ds;
}


BoneWeights::BoneWeights( const NifModel * nif, const QModelIndex & index, int b )
{
	trans = Transform( nif, index );
	bone = b;
	
	QModelIndex idxWeights = nif->getIndex( index, "Vertex Weights" );
	if ( idxWeights.isValid() )
	{
		for ( int c = 0; c < nif->rowCount( idxWeights ); c++ )
		{
			QModelIndex idx = idxWeights.child( c, 0 );
			weights.append( VertexWeight( nif->get<int>( idx, "Index" ), nif->get<float>( idx, "Weight" ) ) );
		}
	}
	else
		qWarning() << nif->getBlockNumber( index ) << "vertex weights not found";
}

