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

#include "gltools.h"

#include "nifmodel.h"

#include <QtOpenGL>

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
	center = nif->get<Vector3>( index, "Center" );
	radius = nif->get<float>( index, "Radius" );
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


/*
 *  Bound Sphere
 */


BoundSphere::BoundSphere()
{
	radius	= -1;
}

BoundSphere::BoundSphere( const Vector3 & c, float r )
{
	center	= c;
	radius	= r;
}

BoundSphere::BoundSphere( const BoundSphere & other )
{
	operator=( other );
}

BoundSphere::BoundSphere( const QVector<Vector3> & verts )
{
	if ( verts.isEmpty() )
	{
		center	= Vector3();
		radius	= -1;
	}
	else
	{
		center	= Vector3();
		foreach ( Vector3 v, verts )
		{
			center += v;
		}
		center /= verts.count();
		
		radius	= 0;
		foreach ( Vector3 v, verts )
		{
			float d = ( center - v ).squaredLength();
			if ( d > radius )
				radius = d;
		}
		radius = sqrt( radius );
	}
}

BoundSphere & BoundSphere::operator=( const BoundSphere & o )
{
	center	= o.center;
	radius	= o.radius;
	return *this;
}

BoundSphere & BoundSphere::operator|=( const BoundSphere & o )
{
	if ( o.radius < 0 )
		return *this;
	if ( radius < 0 )
		return operator=( o );
	
	float d = ( center - o.center ).length();
	
	if ( radius >= d + o.radius )
		return * this;
	if ( o.radius >= d + radius )
		return operator=( o );
	
	if ( o.radius > radius ) radius = o.radius;
	radius += d / 2;
	center = ( center + o.center ) / 2;
	
	return *this;
}

BoundSphere BoundSphere::operator|( const BoundSphere & other )
{
	BoundSphere b( *this );
	b |= other;
	return b;
}

BoundSphere & BoundSphere::apply( const Transform & t )
{
	center = t * center;
	radius *= fabs( t.scale );
	return *this;
}

BoundSphere & BoundSphere::applyInv( const Transform & t )
{
	center = t.rotation.inverted() * ( center - t.translation ) / t.scale;
	radius /= fabs( t.scale );
	return *this;
}

BoundSphere operator*( const Transform & t, const BoundSphere & sphere )
{
	BoundSphere bs( sphere );
	return bs.apply( t );
}


/*
 * draw primitives
 */

void drawAxes( Vector3 c, float axis )
{
	glPushMatrix();
	glTranslate( c );
	GLfloat arrow = axis / 12.0;
	glBegin( GL_LINES );
	glColor3f( 1.0, 0.0, 0.0 );
	glVertex3f( - axis, 0, 0 );
	glVertex3f( + axis, 0, 0 );
	glVertex3f( + axis, 0, 0 );
	glVertex3f( + axis - arrow, + arrow, 0 );
	glVertex3f( + axis, 0, 0 );
	glVertex3f( + axis - arrow, - arrow, 0 );
	glVertex3f( + axis, 0, 0 );
	glVertex3f( + axis - arrow, 0, + arrow );
	glVertex3f( + axis, 0, 0 );
	glVertex3f( + axis - arrow, 0, - arrow );
	glColor3f( 0.0, 1.0, 0.0 );
	glVertex3f( 0, - axis, 0 );
	glVertex3f( 0, + axis, 0 );
	glVertex3f( 0, + axis, 0 );
	glVertex3f( + arrow, + axis - arrow, 0 );
	glVertex3f( 0, + axis, 0 );
	glVertex3f( - arrow, + axis - arrow, 0 );
	glVertex3f( 0, + axis, 0 );
	glVertex3f( 0, + axis - arrow, + arrow );
	glVertex3f( 0, + axis, 0 );
	glVertex3f( 0, + axis - arrow, - arrow );
	glColor3f( 0.0, 0.0, 1.0 );
	glVertex3f( 0, 0, - axis );
	glVertex3f( 0, 0, + axis );
	glVertex3f( 0, 0, + axis );
	glVertex3f( 0, + arrow, + axis - arrow );
	glVertex3f( 0, 0, + axis );
	glVertex3f( 0, - arrow, + axis - arrow );
	glVertex3f( 0, 0, + axis );
	glVertex3f( + arrow, 0, + axis - arrow );
	glVertex3f( 0, 0, + axis );
	glVertex3f( - arrow, 0, + axis - arrow );
	glEnd();
	glPopMatrix();
}

void drawBox( Vector3 a, Vector3 b )
{
	glBegin( GL_LINE_STRIP );
	glVertex3f( a[0], a[1], a[2] );
	glVertex3f( a[0], b[1], a[2] );
	glVertex3f( a[0], b[1], b[2] );
	glVertex3f( a[0], a[1], b[2] );
	glVertex3f( a[0], a[1], a[2] );
	glEnd();
	glBegin( GL_LINE_STRIP );
	glVertex3f( b[0], a[1], a[2] );
	glVertex3f( b[0], b[1], a[2] );
	glVertex3f( b[0], b[1], b[2] );
	glVertex3f( b[0], a[1], b[2] );
	glVertex3f( b[0], a[1], a[2] );
	glEnd();
	glBegin( GL_LINES );
	glVertex3f( a[0], a[1], a[2] );
	glVertex3f( b[0], a[1], a[2] );
	glVertex3f( a[0], b[1], a[2] );
	glVertex3f( b[0], b[1], a[2] );
	glVertex3f( a[0], b[1], b[2] );
	glVertex3f( b[0], b[1], b[2] );
	glVertex3f( a[0], a[1], b[2] );
	glVertex3f( b[0], a[1], b[2] );
	glEnd();
}

void drawCircle( Vector3 c, Vector3 n, float r, int sd )
{
	Vector3 x = Vector3::crossproduct( n, Vector3( n[1], n[2], n[0] ) );
	Vector3 y = Vector3::crossproduct( n, x );
	drawArc( c, x * r, y * r, - PI, + PI, sd );
}

void drawArc( Vector3 c, Vector3 x, Vector3 y, float an, float ax, int sd )
{
	glBegin( GL_LINE_STRIP );
	for ( int j = 0; j <= sd; j++ )
	{
		float f = ( ax - an ) * float( j ) / float( sd ) + an;
		
		glVertex( c + x * sin( f ) + y * cos( f ) );
	}
	glEnd();
}

void drawSolidArc( Vector3 c, Vector3 n, Vector3 x, Vector3 y, float an, float ax, int sd )
{
	bool cull = glIsEnabled( GL_CULL_FACE );
	glDisable( GL_CULL_FACE );
	glBegin( GL_QUAD_STRIP );
	for ( int j = 0; j <= sd; j++ )
	{
		float f = ( ax - an ) * float( j ) / float( sd ) + an;
		
		glVertex( c + x * sin( f ) + y * cos( f ) + n );
		glVertex( c + x * sin( f ) + y * cos( f ) - n );
	}
	glEnd();
	if ( cull )
		glEnable( GL_CULL_FACE );
}

void drawSphere( Vector3 c, float r, int sd )
{
	for ( int j = -sd; j <= sd; j++ )
	{
		float f = PI * float( j ) / float( sd );
		Vector3 cj = c + Vector3( 0, 0, r * cos( f ) );
		float rj = r * sin( f );
		
		glBegin( GL_LINE_STRIP );
		for ( int i = 0; i <= sd*2; i++ )
			glVertex( Vector3( sin( PI / sd * i ), cos( PI / sd * i ), 0 ) * rj + cj );
		glEnd();
	}
	for ( int j = -sd; j <= sd; j++ )
	{
		float f = PI * float( j ) / float( sd );
		Vector3 cj = c + Vector3( 0, r * cos( f ), 0 );
		float rj = r * sin( f );
		
		glBegin( GL_LINE_STRIP );
		for ( int i = 0; i <= sd*2; i++ )
			glVertex( Vector3( sin( PI / sd * i ), 0, cos( PI / sd * i ) ) * rj + cj );
		glEnd();
	}
	for ( int j = -sd; j <= sd; j++ )
	{
		float f = PI * float( j ) / float( sd );
		Vector3 cj = c + Vector3( r * cos( f ), 0, 0 );
		float rj = r * sin( f );
		
		glBegin( GL_LINE_STRIP );
		for ( int i = 0; i <= sd*2; i++ )
			glVertex( Vector3( 0, sin( PI / sd * i ), cos( PI / sd * i ) ) * rj + cj );
		glEnd();
	}
}

void drawCapsule( Vector3 a, Vector3 b, float r, int sd )
{
	Vector3 d = b - a;
	if ( d.length() < 0.001 )
	{
		drawSphere( a, r );
		return;
	}
	
	Vector3 n = d;
	n.normalize();
	
	Vector3 x( n[1], n[2], n[0] );
	Vector3 y = Vector3::crossproduct( n, x );
	x = Vector3::crossproduct( n, y );
	
	x *= r;
	y *= r;
	
	glBegin( GL_LINE_STRIP );
	for ( int i = 0; i <= sd*2; i++ )
		glVertex( a + d/2 + x * sin( PI / sd * i ) + y * cos( PI / sd * i ) );
	glEnd();
	glBegin( GL_LINES );
	for ( int i = 0; i <= sd*2; i++ )
	{
		glVertex( a + x * sin( PI / sd * i ) + y * cos( PI / sd * i ) );
		glVertex( b + x * sin( PI / sd * i ) + y * cos( PI / sd * i ) );
	}
	glEnd();
	for ( int j = 0; j <= sd; j++ )
	{
		float f = PI * float( j ) / float( sd * 2 );
		Vector3 dj = n * r * cos( f );
		float rj = sin( f );
		
		glBegin( GL_LINE_STRIP );
		for ( int i = 0; i <= sd*2; i++ )
			glVertex( a - dj + x * sin( PI / sd * i ) * rj + y * cos( PI / sd * i ) * rj );
		glEnd();
		glBegin( GL_LINE_STRIP );
		for ( int i = 0; i <= sd*2; i++ )
			glVertex( b + dj + x * sin( PI / sd * i ) * rj + y * cos( PI / sd * i ) * rj );
		glEnd();
	}
}

void drawDashLine( Vector3 a, Vector3 b, int sd )
{
	Vector3 d = ( b - a ) / float( sd );
	glBegin( GL_LINES );
	for ( int c = 0; c <= sd; c++ )
	{
		glVertex( a + d * c );
	}
	glEnd();
}

