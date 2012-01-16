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

#include "gltools.h"

#include "../nifmodel.h"

#include <QtOpenGL>

//! \file gltools.cpp GL helper functions

BoneWeights::BoneWeights( const NifModel * nif, const QModelIndex & index, int b, int vcnt )
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
	else // create artificial ones, TODO: should they weight nothing* instead?
		for ( int c = 0; c < vcnt; c++ )
			weights.append( VertexWeight( c, 1.0f ) );
}


SkinPartition::SkinPartition( const NifModel * nif, const QModelIndex & index )
{
	numWeightsPerVertex = nif->get<int>( index, "Num Weights Per Vertex" );
	
	vertexMap = nif->getArray<int>( index, "Vertex Map" );
	
	if ( vertexMap.isEmpty() )
	{
		vertexMap.resize( nif->get<int>( index, "Num Vertices" ) );
		for ( int x = 0; x < vertexMap.count(); x++ )
			vertexMap[x] = x;
	}
	
	boneMap = nif->getArray<int>( index, "Bones" );
	
	QModelIndex iWeights = nif->getIndex( index, "Vertex Weights" );
	QModelIndex iBoneIndices = nif->getIndex( index, "Bone Indices" );
	
	weights.resize( vertexMap.count() * numWeightsPerVertex );
	
	for ( int v = 0; v < vertexMap.count(); v++ )
	{
		for ( int w = 0; w < numWeightsPerVertex; w++ )
		{
			QModelIndex iw = iWeights.child( v, 0 ).child( w, 0 );
			QModelIndex ib = iBoneIndices.child( v, 0 ).child( w, 0 );
			
			weights[ v * numWeightsPerVertex + w ].first = ( ib.isValid() ? nif->get<int>( ib ) : 0 );
			weights[ v * numWeightsPerVertex + w ].second = ( iw.isValid() ? nif->get<float>( iw ) : 0 );
		}
	}
	
	QModelIndex iStrips = nif->getIndex( index, "Strips" );
	for ( int s = 0; s < nif->rowCount( iStrips ); s++ )
	{
		tristrips << nif->getArray<quint16>( iStrips.child( s, 0 ) );
	}
	
	triangles = nif->getArray<Triangle>( index, "Triangles" );
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
	GLfloat arrow = axis / 36.0;
	glBegin( GL_LINES );
	glColor3f( 1.0, 0.0, 0.0 );
	glVertex3f( - axis, 0, 0 );
	glVertex3f( + axis, 0, 0 );
	glVertex3f( + axis, 0, 0 );
	glVertex3f( + axis - 3 * arrow, + arrow, + arrow );
	glVertex3f( + axis, 0, 0 );
	glVertex3f( + axis - 3 * arrow, - arrow, + arrow );
	glVertex3f( + axis, 0, 0 );
	glVertex3f( + axis - 3 * arrow, + arrow, - arrow );
	glVertex3f( + axis, 0, 0 );
	glVertex3f( + axis - 3 * arrow, - arrow, - arrow );
	glColor3f( 0.0, 1.0, 0.0 );
	glVertex3f( 0, - axis, 0 );
	glVertex3f( 0, + axis, 0 );
	glVertex3f( 0, + axis, 0 );
	glVertex3f( + arrow, + axis - 3 * arrow, + arrow );
	glVertex3f( 0, + axis, 0 );
	glVertex3f( - arrow, + axis - 3 * arrow, + arrow );
	glVertex3f( 0, + axis, 0 );
	glVertex3f( + arrow, + axis - 3 * arrow, - arrow );
	glVertex3f( 0, + axis, 0 );
	glVertex3f( - arrow, + axis - 3 * arrow, - arrow );
	glColor3f( 0.0, 0.0, 1.0 );
	glVertex3f( 0, 0, - axis );
	glVertex3f( 0, 0, + axis );
	glVertex3f( 0, 0, + axis );
	glVertex3f( + arrow, + arrow, + axis - 3 * arrow );
	glVertex3f( 0, 0, + axis );
	glVertex3f( - arrow, + arrow, + axis - 3 * arrow );
	glVertex3f( 0, 0, + axis );
	glVertex3f( + arrow, - arrow, + axis - 3 * arrow );
	glVertex3f( 0, 0, + axis );
	glVertex3f( - arrow, - arrow, + axis - 3 * arrow );
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

void drawCone( Vector3 c, Vector3 n, float a, int sd )
{
	Vector3 x = Vector3::crossproduct( n, Vector3( n[1], n[2], n[0] ) );
	Vector3 y = Vector3::crossproduct( n, x );
	
	x = x * sin( a );
	y = y * sin( a );
	n = n * cos( a );
	
	glBegin( GL_TRIANGLE_FAN );
	glVertex( c );
	for ( int i = 0; i <= sd; i++ )
	{
		float f = ( 2 * PI * float( i ) / float( sd ) );
		
		glVertex( c + n + x * sin( f ) + y * cos( f ) );
	}
	glEnd();
	
	// double-sided, please
	
	glBegin( GL_TRIANGLE_FAN );
	glVertex( c );
	for ( int i = 0; i <= sd; i++ )
	{
		float f = ( 2 * PI * float( i ) / float( sd ) );
		
		glVertex( c + n + x * sin( -f ) + y * cos( -f ) );
	}
	glEnd();
}

void drawRagdollCone( Vector3 pivot, Vector3 twist, Vector3 plane, float coneAngle, float minPlaneAngle, float maxPlaneAngle, int sd )
{
	Vector3 z = twist;
	Vector3 y = plane;
	Vector3 x = Vector3::crossproduct( z, y );
	
	x = x * sin( coneAngle );
	y = y;
	z = z;
	
	glBegin( GL_TRIANGLE_FAN );
	glVertex( pivot );
	for ( int i = 0; i <= sd; i++ )
	{
		float f = ( 2.0f * PI * float( i ) / float( sd ) );
		
		Vector3 xy = x * sin( f ) + y * sin( f <= PI / 2 || f >= 3 * PI / 2 ? maxPlaneAngle : -minPlaneAngle ) * cos( f );
		
		glVertex( pivot + z * sqrt( 1 - xy.length() * xy.length() ) + xy );
	}
	glEnd();
	
	// double-sided, please
	
	glBegin( GL_TRIANGLE_FAN );
	glVertex( pivot );
	for ( int i = 0; i <= sd; i++ )
	{
		float f = ( 2.0f * PI * float( i ) / float( sd ) );
		
		Vector3 xy = x * sin( -f ) + y * sin( -f <= PI / 2 || -f >= 3 * PI / 2 ? maxPlaneAngle : -minPlaneAngle ) * cos( -f );
		
		glVertex( pivot + z * sqrt( 1 - xy.length() * xy.length() ) + xy );
	}
	glEnd();
}

void drawSpring( Vector3 a, Vector3 b, float stiffness, int sd, bool solid )
{	// draw a spring with stiffness turns
	bool cull = glIsEnabled( GL_CULL_FACE );
	glDisable( GL_CULL_FACE );

	Vector3 h = b - a;
	
	float r = h.length() / 5;
	
	Vector3 n = h;
	n.normalize();
	
	Vector3 x = Vector3::crossproduct( n, Vector3( n[1], n[2], n[0] ) );
	Vector3 y = Vector3::crossproduct( n, x );
	
	x.normalize();
	y.normalize();
	
	x*=r;
	y*=r;
	
	glBegin( GL_LINES );
	glVertex( a );
	glVertex( a + x * sinf( 0 ) + y * cosf( 0 ) );
	glEnd();
	glBegin( solid ? GL_QUAD_STRIP : GL_LINE_STRIP );
	int m = int( stiffness * sd );
	for ( int i = 0; i <= m; i++ )
	{
		float f = 2 * PI * float( i ) / float( sd );
		
		glVertex( a + h * i / m + x * sinf( f ) + y * cosf( f ) );
		if ( solid )
			glVertex( a + h * i / m + x * 0.8f * sinf( f ) + y * 0.8f * cosf( f ) );
	}
	glEnd();
	glBegin( GL_LINES );
	glVertex( b + x * sinf( 2 * PI * float( m ) / float( sd ) ) + y * cosf( 2 * PI * float( m ) / float( sd ) ) );
	glVertex( b );
	glEnd();
	if ( cull )
		glEnable( GL_CULL_FACE );
}

void drawRail( const Vector3 &a, const Vector3 &b )
{
	/* offset between beginning and end points */
	Vector3 off = b - a;

	/* direction vector of "rail track width", in xy-plane */
	Vector3 x = Vector3( - off[1], off[0], 0 );
	if( x.length() < 0.0001f ) {
		x[0] = 1.0f; }
	x.normalize();

	glBegin( GL_POINTS );
		glVertex( a );
		glVertex( b );
	glEnd();

	/* draw the rail */
	glBegin( GL_LINES );
		glVertex( a + x );
		glVertex( b + x );
		glVertex( a - x );
		glVertex( b - x );
	glEnd();

	int len = int( off.length() );

	/* draw the logs */
	glBegin( GL_LINES );
	for ( int i = 0; i <= len; i++ )
	{
		float rel_off = ( 1.0f * i ) / len;
		glVertex( a + off * rel_off + x * 1.3f );
		glVertex( a + off * rel_off - x * 1.3f );
	}
	glEnd();
}

void drawSolidArc( Vector3 c, Vector3 n, Vector3 x, Vector3 y, float an, float ax, float r, int sd )
{
	bool cull = glIsEnabled( GL_CULL_FACE );
	glDisable( GL_CULL_FACE );
	glBegin( GL_QUAD_STRIP );
	for ( int j = 0; j <= sd; j++ )
	{
		float f = ( ax - an ) * float( j ) / float( sd ) + an;
		
		glVertex( c + x * r * sin( f ) + y * r * cos( f ) + n );
		glVertex( c + x * r * sin( f ) + y * r * cos( f ) - n );
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

void drawConvexHull( QVector<Vector4> vertices, QVector<Vector4> normals )
{
	glBegin( GL_LINES );
	for ( int i = 1; i < vertices.count(); i++ )
	{
		for ( int j = 0; j < i; j++ )
		{
			glVertex( vertices[i] );
			glVertex( vertices[j] );
		}
	}
	/*
	Vector3 c;
	foreach ( Vector4 v, vertices )
		c += Vector3( v );
	if ( vertices.count() )
		c /= vertices.count();
	foreach ( Vector4 n, normals )
	{
		glVertex( c + Vector3( n ) * ( n[3] ) );
		glVertex( c + Vector3( n ) * ( - 1 + n[3] ) );
	}
	*/
	glEnd();
}

// Renders text using the font initialized in the primary view class
void renderText(const Vector3& c, const QString & str)
{
	renderText(c[0], c[1], c[2], str);
}
void renderText(double x, double y, double z, const QString & str)
{
	glPushAttrib(GL_ALL_ATTRIB_BITS);

	glDisable(GL_TEXTURE_1D);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_CULL_FACE);

	glRasterPos3d(x, y, z);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glAlphaFunc(GL_GREATER, 0.0);
	glEnable(GL_ALPHA_TEST);

	QByteArray cstr(str.toLatin1());
	glCallLists(cstr.size(), GL_UNSIGNED_BYTE, cstr.constData());
	glPopAttrib();
}

