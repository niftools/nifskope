/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005-2015, NIF File Format Library and Tools
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

#include "model/nifmodel.h"

#include <QMap>
#include <QStack>
#include <QVector>

#include <stack>
#include <map>
#include <algorithm>
#include <functional>


//! \file gltools.cpp GL helper functions

BoneWeights::BoneWeights( const NifModel * nif, const QModelIndex & index, int b, int vcnt )
{
	trans  = Transform( nif, index );
	auto sph = BoundSphere( nif, index );
	center = sph.center;
	radius = sph.radius;
	bone = b;

	QModelIndex idxWeights = nif->getIndex( index, "Vertex Weights" );
	if ( vcnt && idxWeights.isValid() ) {
		for ( int c = 0; c < nif->rowCount( idxWeights ); c++ ) {
			QModelIndex idx = idxWeights.child( c, 0 );
			weights.append( VertexWeight( nif->get<int>( idx, "Index" ), nif->get<float>( idx, "Weight" ) ) );
		}
	}
}

void BoneWeights::setTransform( const NifModel * nif, const QModelIndex & index )
{
	trans = Transform( nif, index );
	auto sph = BoundSphere( nif, index );
	center = sph.center;
	radius = sph.radius;
}


SkinPartition::SkinPartition( const NifModel * nif, const QModelIndex & index )
{
	numWeightsPerVertex = nif->get<int>( index, "Num Weights Per Vertex" );

	vertexMap = nif->getArray<int>( index, "Vertex Map" );

	if ( vertexMap.isEmpty() ) {
		vertexMap.resize( nif->get<int>( index, "Num Vertices" ) );

		for ( int x = 0; x < vertexMap.count(); x++ )
			vertexMap[x] = x;
	}

	boneMap = nif->getArray<int>( index, "Bones" );

	QModelIndex iWeights = nif->getIndex( index, "Vertex Weights" );
	QModelIndex iBoneIndices = nif->getIndex( index, "Bone Indices" );

	weights.resize( vertexMap.count() * numWeightsPerVertex );

	for ( int v = 0; v < vertexMap.count(); v++ ) {
		for ( int w = 0; w < numWeightsPerVertex; w++ ) {
			QModelIndex iw = iWeights.child( v, 0 ).child( w, 0 );
			QModelIndex ib = iBoneIndices.child( v, 0 ).child( w, 0 );

			weights[ v * numWeightsPerVertex + w ].first  = ( ib.isValid() ? nif->get<int>( ib ) : 0 );
			weights[ v * numWeightsPerVertex + w ].second = ( iw.isValid() ? nif->get<float>( iw ) : 0 );
		}
	}

	QModelIndex iStrips = nif->getIndex( index, "Strips" );

	for ( int s = 0; s < nif->rowCount( iStrips ); s++ ) {
		tristrips << nif->getArray<quint16>( iStrips.child( s, 0 ) );
	}

	triangles = nif->getArray<Triangle>( index, "Triangles" );
}

QVector<Triangle> SkinPartition::getRemappedTriangles() const
{
	QVector<Triangle> tris;

	for ( const auto& t : triangles )
		tris << Triangle( vertexMap[t.v1()], vertexMap[t.v2()], vertexMap[t.v3()] );

	return tris;
}

QVector<QVector<quint16>> SkinPartition::getRemappedTristrips() const
{
	QVector<QVector<quint16>> tris;

	for ( const auto& t : tristrips ) {
		QVector<quint16> points;
		for ( const auto& p : t )
			points << vertexMap[p];
		tris << points;
	}

	return tris;
}

/*
 *  Bound Sphere
 */


BoundSphere::BoundSphere()
{
	radius = -1;
}

BoundSphere::BoundSphere( const Vector3 & c, float r )
{
	center = c;
	radius = r;
}

BoundSphere::BoundSphere( const BoundSphere & other )
{
	operator=( other );
}

BoundSphere::BoundSphere( const NifModel * nif, const QModelIndex & index )
{
	auto idx = index;
	auto sph = nif->getIndex( idx, "Bounding Sphere" );
	if ( sph.isValid() )
		idx = sph;

	center = nif->get<Vector3>( idx, "Center" );
	radius = nif->get<float>( idx, "Radius" );
}

BoundSphere::BoundSphere( const QVector<Vector3> & verts )
{
	if ( verts.isEmpty() ) {
		center = Vector3();
		radius = -1;
	} else {
		center = Vector3();
		for ( const Vector3& v : verts ) {
			center += v;
		}
		center /= verts.count();

		radius = 0;
		for ( const Vector3& v : verts ) {
			float d = ( center - v ).squaredLength();

			if ( d > radius )
				radius = d;
		}
		radius = sqrt( radius );
	}
}

BoundSphere & BoundSphere::operator=( const BoundSphere & o )
{
	center = o.center;
	radius = o.radius;
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
		return *this;

	if ( o.radius >= d + radius )
		return operator=( o );

	if ( o.radius > radius )
		radius = o.radius;

	radius += d / 2;
	center  = ( center + o.center ) / 2;

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
	center  = t * center;
	radius *= fabs( t.scale );
	return *this;
}

BoundSphere & BoundSphere::applyInv( const Transform & t )
{
	center  = t.rotation.inverted() * ( center - t.translation ) / t.scale;
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

void drawAxes( const Vector3 & c, float axis, bool color )
{
	glPushMatrix();
	glTranslate( c );
	GLfloat arrow = axis / 36.0;
	glBegin( GL_LINES );
	if ( color )
		glColor3f( 1.0, 0.0, 0.0 );
	glVertex3f( -axis, 0, 0 );
	glVertex3f( +axis, 0, 0 );
	glVertex3f( +axis, 0, 0 );
	glVertex3f( +axis - 3 * arrow, +arrow, +arrow );
	glVertex3f( +axis, 0, 0 );
	glVertex3f( +axis - 3 * arrow, -arrow, +arrow );
	glVertex3f( +axis, 0, 0 );
	glVertex3f( +axis - 3 * arrow, +arrow, -arrow );
	glVertex3f( +axis, 0, 0 );
	glVertex3f( +axis - 3 * arrow, -arrow, -arrow );
	if ( color )
		glColor3f( 0.0, 1.0, 0.0 );
	glVertex3f( 0, -axis, 0 );
	glVertex3f( 0, +axis, 0 );
	glVertex3f( 0, +axis, 0 );
	glVertex3f( +arrow, +axis - 3 * arrow, +arrow );
	glVertex3f( 0, +axis, 0 );
	glVertex3f( -arrow, +axis - 3 * arrow, +arrow );
	glVertex3f( 0, +axis, 0 );
	glVertex3f( +arrow, +axis - 3 * arrow, -arrow );
	glVertex3f( 0, +axis, 0 );
	glVertex3f( -arrow, +axis - 3 * arrow, -arrow );
	if ( color )
		glColor3f( 0.0, 0.0, 1.0 );
	glVertex3f( 0, 0, -axis );
	glVertex3f( 0, 0, +axis );
	glVertex3f( 0, 0, +axis );
	glVertex3f( +arrow, +arrow, +axis - 3 * arrow );
	glVertex3f( 0, 0, +axis );
	glVertex3f( -arrow, +arrow, +axis - 3 * arrow );
	glVertex3f( 0, 0, +axis );
	glVertex3f( +arrow, -arrow, +axis - 3 * arrow );
	glVertex3f( 0, 0, +axis );
	glVertex3f( -arrow, -arrow, +axis - 3 * arrow );
	glEnd();
	glPopMatrix();
}

QVector<int> sortAxes( QVector<float> axesDots )
{
	QVector<float> dotsSorted = axesDots;
	std::stable_sort( dotsSorted.begin(), dotsSorted.end() );

	// Retrieve position of X, Y, Z axes in sorted list
	auto x = axesDots.indexOf( dotsSorted[0] );
	auto y = axesDots.indexOf( dotsSorted[1] );
	auto z = axesDots.indexOf( dotsSorted[2] );

	// When z == 1.0, x and y both == 0
	if ( axesDots[2] == 1.0 ) {
		x = 0;
		y = 1;
	}

	return{ x, y, z };
}

void drawAxesOverlay( const Vector3 & c, float axis, QVector<int> axesOrder )
{
	glPushMatrix();
	glTranslate( c );
	GLfloat arrow = axis / 36.0;

	glDisable( GL_LIGHTING );
	glDepthFunc( GL_ALWAYS );
	glLineWidth( 2.0f );
	glBegin( GL_LINES );

	// Render the X axis
	std::function<void()> xAxis = [axis, arrow]() {
		glColor3f( 1.0, 0.0, 0.0 );
		glVertex3f( 0, 0, 0 );
		glVertex3f( +axis, 0, 0 );
		glVertex3f( +axis, 0, 0 );
		glVertex3f( +axis - 3 * arrow, +arrow, +arrow );
		glVertex3f( +axis, 0, 0 );
		glVertex3f( +axis - 3 * arrow, -arrow, +arrow );
		glVertex3f( +axis, 0, 0 );
		glVertex3f( +axis - 3 * arrow, +arrow, -arrow );
		glVertex3f( +axis, 0, 0 );
		glVertex3f( +axis - 3 * arrow, -arrow, -arrow );
	};

	// Render the Y axis
	std::function<void()> yAxis = [axis, arrow]() {
		glColor3f( 0.0, 1.0, 0.0 );
		glVertex3f( 0, 0, 0 );
		glVertex3f( 0, +axis, 0 );
		glVertex3f( 0, +axis, 0 );
		glVertex3f( +arrow, +axis - 3 * arrow, +arrow );
		glVertex3f( 0, +axis, 0 );
		glVertex3f( -arrow, +axis - 3 * arrow, +arrow );
		glVertex3f( 0, +axis, 0 );
		glVertex3f( +arrow, +axis - 3 * arrow, -arrow );
		glVertex3f( 0, +axis, 0 );
		glVertex3f( -arrow, +axis - 3 * arrow, -arrow );
	};

	// Render the Z axis
	std::function<void()> zAxis = [axis, arrow]() {
		glColor3f( 0.0, 0.0, 1.0 );
		glVertex3f( 0, 0, 0 );
		glVertex3f( 0, 0, +axis );
		glVertex3f( 0, 0, +axis );
		glVertex3f( +arrow, +arrow, +axis - 3 * arrow );
		glVertex3f( 0, 0, +axis );
		glVertex3f( -arrow, +arrow, +axis - 3 * arrow );
		glVertex3f( 0, 0, +axis );
		glVertex3f( +arrow, -arrow, +axis - 3 * arrow );
		glVertex3f( 0, 0, +axis );
		glVertex3f( -arrow, -arrow, +axis - 3 * arrow );
	};

	// List of the lambdas
	QVector<std::function<void()>> axes = { xAxis, yAxis, zAxis };

	// Render the axes in the given order
	//	e.g. {2, 1, 0} = zAxis(); yAxis(); xAxis();
	for ( auto i : axesOrder ) {
		axes[i]();
	}

	glEnd();
	glPopMatrix();
}

void drawBox( const Vector3 & a, const Vector3 & b )
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

void drawGrid( int s /* grid size */, int line /* line spacing */, int sub /* # subdivisions */ )
{
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glLineWidth( 1.0f );
	glColor4f( 1.0f, 1.0f, 1.0f, 0.2f );

	glBegin( GL_LINES );
	for ( int i = -s; i <= s; i += line ) {
		glVertex3f( i, -s, 0.0f );
		glVertex3f( i, s, 0.0f );
		glVertex3f( -s, i, 0.0f );
		glVertex3f( s, i, 0.0f );
	}
	glEnd();

	glColor4f( 1.0f, 1.0f, 1.0f, 0.1f );
	glLineWidth( 0.25f );
	glBegin( GL_LINES );
	for ( int i = -s; i <= s; i += line/sub ) {
		glVertex3f( i, -s, 0.0f );
		glVertex3f( i, s, 0.0f );
		glVertex3f( -s, i, 0.0f );
		glVertex3f( s, i, 0.0f );
	}
	glEnd();
	glDisable( GL_BLEND );
}

void drawCircle( const Vector3 & c, const Vector3 & n, float r, int sd )
{
	Vector3 x = Vector3::crossproduct( n, Vector3( n[1], n[2], n[0] ) );
	Vector3 y = Vector3::crossproduct( n, x );
	drawArc( c, x * r, y * r, -PI, +PI, sd );
}

void drawArc( const Vector3 & c, const Vector3 & x, const Vector3 & y, float an, float ax, int sd )
{
	glBegin( GL_LINE_STRIP );

	for ( int j = 0; j <= sd; j++ ) {
		float f = ( ax - an ) * float(j) / float(sd) + an;

		glVertex( c + x * sin( f ) + y * cos( f ) );
	}

	glEnd();
}

void drawCone( const Vector3 & c, Vector3 n, float a, int sd )
{
	Vector3 x = Vector3::crossproduct( n, Vector3( n[1], n[2], n[0] ) );
	Vector3 y = Vector3::crossproduct( n, x );

	x = x * sin( a );
	y = y * sin( a );
	n = n * cos( a );

	glBegin( GL_TRIANGLE_FAN );
	glVertex( c );

	for ( int i = 0; i <= sd; i++ ) {
		float f = ( 2 * PI * float(i) / float(sd) );

		glVertex( c + n + x * sin( f ) + y * cos( f ) );
	}

	glEnd();

	// double-sided, please

	glBegin( GL_TRIANGLE_FAN );
	glVertex( c );

	for ( int i = 0; i <= sd; i++ ) {
		float f = ( 2 * PI * float(i) / float(sd) );

		glVertex( c + n + x * sin( -f ) + y * cos( -f ) );
	}

	glEnd();
}

void drawRagdollCone( const Vector3 & pivot, const Vector3 & twist, const Vector3 & plane, float coneAngle, float minPlaneAngle, float maxPlaneAngle, int sd )
{
	Vector3 z = twist;
	Vector3 y = plane;
	Vector3 x = Vector3::crossproduct( z, y );

	x = x * sin( coneAngle );

	glBegin( GL_TRIANGLE_FAN );
	glVertex( pivot );

	for ( int i = 0; i <= sd; i++ ) {
		float f = ( 2.0f * PI * float(i) / float(sd) );

		Vector3 xy = x * sin( f ) + y * sin( f <= PI / 2 || f >= 3 * PI / 2 ? maxPlaneAngle : -minPlaneAngle ) * cos( f );

		glVertex( pivot + z * sqrt( 1 - xy.length() * xy.length() ) + xy );
	}

	glEnd();

	// double-sided, please

	glBegin( GL_TRIANGLE_FAN );
	glVertex( pivot );

	for ( int i = 0; i <= sd; i++ ) {
		float f = ( 2.0f * PI * float(i) / float(sd) );

		Vector3 xy = x * sin( -f ) + y * sin( -f <= PI / 2 || -f >= 3 * PI / 2 ? maxPlaneAngle : -minPlaneAngle ) * cos( -f );

		glVertex( pivot + z * sqrt( 1 - xy.length() * xy.length() ) + xy );
	}

	glEnd();
}

void drawSpring( const Vector3 & a, const Vector3 & b, float stiffness, int sd, bool solid )
{
	// draw a spring with stiffness turns
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

	x *= r;
	y *= r;

	glBegin( GL_LINES );
	glVertex( a );
	glVertex( a + x * sinf( 0 ) + y * cosf( 0 ) );
	glEnd();
	glBegin( solid ? GL_QUAD_STRIP : GL_LINE_STRIP );
	int m = int(stiffness * sd);

	for ( int i = 0; i <= m; i++ ) {
		float f = 2 * PI * float(i) / float(sd);

		glVertex( a + h * i / m + x * sinf( f ) + y * cosf( f ) );

		if ( solid )
			glVertex( a + h * i / m + x * 0.8f * sinf( f ) + y * 0.8f * cosf( f ) );
	}

	glEnd();
	glBegin( GL_LINES );
	glVertex( b + x * sinf( 2 * PI * float(m) / float(sd) ) + y * cosf( 2 * PI * float(m) / float(sd) ) );
	glVertex( b );
	glEnd();

	if ( cull )
		glEnable( GL_CULL_FACE );
}

void drawRail( const Vector3 & a, const Vector3 & b )
{
	/* offset between beginning and end points */
	Vector3 off = b - a;

	/* direction vector of "rail track width", in xy-plane */
	Vector3 x = Vector3( -off[1], off[0], 0 );

	if ( x.length() < 0.0001f ) {
		x[0] = 1.0f;
	}

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

	for ( int i = 0; i <= len; i++ ) {
		float rel_off = ( 1.0f * i ) / len;
		glVertex( a + off * rel_off + x * 1.3f );
		glVertex( a + off * rel_off - x * 1.3f );
	}

	glEnd();
}

void drawSolidArc( const Vector3 & c, const Vector3 & n, const Vector3 & x, const Vector3 & y, float an, float ax, float r, int sd )
{
	bool cull = glIsEnabled( GL_CULL_FACE );
	glDisable( GL_CULL_FACE );
	glBegin( GL_QUAD_STRIP );

	for ( int j = 0; j <= sd; j++ ) {
		float f = ( ax - an ) * float(j) / float(sd) + an;

		glVertex( c + x * r * sin( f ) + y * r * cos( f ) + n );
		glVertex( c + x * r * sin( f ) + y * r * cos( f ) - n );
	}

	glEnd();

	if ( cull )
		glEnable( GL_CULL_FACE );
}

void drawSphereSimple( const Vector3 & c, float r, int sd )
{
	drawCircle( c, Vector3( 0, 0, 1 ), r, sd );
	drawCircle( c, Vector3( 0, 1, 0 ), r, sd );
	drawCircle( c, Vector3( 1, 0, 0 ), r, sd );
}

void drawSphere( const Vector3 & c, float r, int sd )
{
	for ( int j = -sd; j <= sd; j++ ) {
		float f = PI * float(j) / float(sd);
		Vector3 cj = c + Vector3( 0, 0, r * cos( f ) );
		float rj = r * sin( f );

		glBegin( GL_LINE_STRIP );

		for ( int i = 0; i <= sd * 2; i++ )
			glVertex( Vector3( sin( PI / sd * i ), cos( PI / sd * i ), 0 ) * rj + cj );

		glEnd();
	}

	for ( int j = -sd; j <= sd; j++ ) {
		float f = PI * float(j) / float(sd);
		Vector3 cj = c + Vector3( 0, r * cos( f ), 0 );
		float rj = r * sin( f );

		glBegin( GL_LINE_STRIP );

		for ( int i = 0; i <= sd * 2; i++ )
			glVertex( Vector3( sin( PI / sd * i ), 0, cos( PI / sd * i ) ) * rj + cj );

		glEnd();
	}

	for ( int j = -sd; j <= sd; j++ ) {
		float f = PI * float(j) / float(sd);
		Vector3 cj = c + Vector3( r * cos( f ), 0, 0 );
		float rj = r * sin( f );

		glBegin( GL_LINE_STRIP );

		for ( int i = 0; i <= sd * 2; i++ )
			glVertex( Vector3( 0, sin( PI / sd * i ), cos( PI / sd * i ) ) * rj + cj );

		glEnd();
	}
}

void drawCapsule( const Vector3 & a, const Vector3 & b, float r, int sd )
{
	Vector3 d = b - a;

	if ( d.length() < 0.001 ) {
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

	for ( int i = 0; i <= sd * 2; i++ )
		glVertex( a + d / 2 + x * sin( PI / sd * i ) + y * cos( PI / sd * i ) );

	glEnd();
	glBegin( GL_LINES );

	for ( int i = 0; i <= sd * 2; i++ ) {
		glVertex( a + x * sin( PI / sd * i ) + y * cos( PI / sd * i ) );
		glVertex( b + x * sin( PI / sd * i ) + y * cos( PI / sd * i ) );
	}

	glEnd();

	for ( int j = 0; j <= sd; j++ ) {
		float f = PI * float(j) / float(sd * 2);
		Vector3 dj = n * r * cos( f );
		float rj = sin( f );

		glBegin( GL_LINE_STRIP );

		for ( int i = 0; i <= sd * 2; i++ )
			glVertex( a - dj + x * sin( PI / sd * i ) * rj + y * cos( PI / sd * i ) * rj );

		glEnd();
		glBegin( GL_LINE_STRIP );

		for ( int i = 0; i <= sd * 2; i++ )
			glVertex( b + dj + x * sin( PI / sd * i ) * rj + y * cos( PI / sd * i ) * rj );

		glEnd();
	}
}

void drawDashLine( const Vector3 & a, const Vector3 & b, int sd )
{
	Vector3 d = ( b - a ) / float(sd);
	glBegin( GL_LINES );

	for ( int c = 0; c <= sd; c++ ) {
		glVertex( a + d * c );
	}

	glEnd();
}

//! Find the dot product of two vectors
static float dotproduct( const Vector3 & v1, const Vector3 & v2 )
{
	return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}
//! Find the cross product of two vectors
static Vector3 crossproduct( const Vector3 & a, const Vector3 & b )
{
	return Vector3( a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0] );
}

//! Generate triangles for convex hull
static QVector<Vector3> generateTris( const NifModel * nif, const QModelIndex & iShape, float scale )
{
	QVector<Vector4> vertices = nif->getArray<Vector4>( iShape, "Vertices" );
	//QVector<Vector4> normals = nif->getArray<Vector4>( iShape, "Normals" );

	if ( vertices.isEmpty() )
		return QVector<Vector3>();

	Vector3 A, B, C, N, V;
	float D;
	int L, prev, eps;
	bool good;

	L = vertices.count();
	QVector<Vector3> P( L );
	QVector<Vector3> tris;

	// Convert Vector4 to Vector3
	for ( int v = 0; v < L; v++ ) {
		P[v] = Vector3( vertices[v] );
	}

	for ( int i = 0; i < L - 2; i++ ) {
		A = P[i];

		for ( int j = i + 1; j < L - 1; j++ ) {
			B = P[j];

			for ( int k = j + 1; k < L; k++ ) {
				C = P[k];

				prev = 0;
				good = true;

				N = crossproduct( (B - A), (C - A) );

				for ( int p = 0; p < L; p++ ) {
					V = P[p];

					if ( (V == A) || (V == B) || (V == C) ) continue;

					D = dotproduct( (V - A), N );

					if ( D == 0 ) continue;

					eps = (D > 0) ? 1 : -1;

					if ( eps + prev == 0 ) {
						good = false;
						continue;
					}

					prev = eps;
				}

				if ( good ) {
					// Append ABC
					tris << (A*scale) << (B*scale) << (C*scale);
				}
			}
		}
	}

	return tris;
}

void drawConvexHull( const NifModel * nif, const QModelIndex & iShape, float scale, bool solid )
{
	static QMap<QModelIndex, QVector<Vector3>> shapes;
	QVector<Vector3> shape;

	shape = shapes[iShape];

	if ( shape.empty() ) {
		shape = generateTris( nif, iShape, scale );
		shapes[iShape] = shape;
	}

	glPolygonMode( GL_FRONT_AND_BACK, solid ? GL_FILL : GL_LINE );
	glDisable( GL_CULL_FACE );
	glBegin( GL_TRIANGLES );

	for ( int i = 0; i < shape.count(); i += 3 ) {
		// DRAW ABC
		glVertex( shape[i] );
		glVertex( shape[i+1] );
		glVertex( shape[i+2] );
	}

	glEnd();
	glPolygonMode( GL_FRONT_AND_BACK, solid ? GL_LINE : GL_FILL );
	glEnable( GL_CULL_FACE );
}

void drawNiTSS( const NifModel * nif, const QModelIndex & iShape, bool solid )
{
	QModelIndex iStrips = nif->getIndex( iShape, "Strips Data" );
	for ( int r = 0; r < nif->rowCount( iStrips ); r++ ) {
		QModelIndex iStripData = nif->getBlock( nif->getLink( iStrips.child( r, 0 ) ), "NiTriStripsData" );
		if ( iStripData.isValid() ) {
			QVector<Vector3> verts = nif->getArray<Vector3>( iStripData, "Vertices" );

			glPolygonMode( GL_FRONT_AND_BACK, solid ? GL_FILL : GL_LINE );
			glDisable( GL_CULL_FACE );
			glBegin( GL_TRIANGLES );

			QModelIndex iPoints = nif->getIndex( iStripData, "Points" );
			for ( int r = 0; r < nif->rowCount( iPoints ); r++ ) {	// draw the strips like they appear in the tescs
				// (use the unstich strips spell to avoid the spider web effect)
				QVector<quint16> strip = nif->getArray<quint16>( iPoints.child( r, 0 ) );
				if ( strip.count() >= 3 ) {
					quint16 a = strip[0];
					quint16 b = strip[1];

					for ( int x = 2; x < strip.size(); x++ ) {
						quint16 c = strip[x];
						glVertex( verts.value( a ) );
						glVertex( verts.value( b ) );
						glVertex( verts.value( c ) );
						a = b;
						b = c;
					}
				}
			}

			glEnd();
			glEnable( GL_CULL_FACE );
			glPolygonMode( GL_FRONT_AND_BACK, solid ? GL_LINE : GL_FILL );
		}
	}
}

void drawCMS( const NifModel * nif, const QModelIndex & iShape, bool solid )
{
	// Scale up for Skyrim
	float havokScale = (nif->checkVersion( 0x14020007, 0x14020007 ) && nif->getUserVersion() >= 12) ? 10.0f : 1.0f;

	//QModelIndex iParent = nif->getBlock( nif->getParent( nif->getBlockNumber( iShape ) ) );
	//Vector4 origin = Vector4( nif->get<Vector3>( iParent, "Origin" ), 0 );

	QModelIndex iData = nif->getBlock( nif->getLink( iShape, "Data" ) );
	if ( iData.isValid() ) {
		QModelIndex iBigVerts = nif->getIndex( iData, "Big Verts" );
		QModelIndex iBigTris = nif->getIndex( iData, "Big Tris" );
		QModelIndex iChunkTrans = nif->getIndex( iData, "Chunk Transforms" );

		QVector<Vector4> verts = nif->getArray<Vector4>( iBigVerts );

		glPolygonMode( GL_FRONT_AND_BACK, solid ? GL_FILL : GL_LINE );
		glDisable( GL_CULL_FACE );

		for ( int r = 0; r < nif->rowCount( iBigTris ); r++ ) {
			quint16 a = nif->get<quint16>( iBigTris.child( r, 0 ), "Triangle 1" );
			quint16 b = nif->get<quint16>( iBigTris.child( r, 0 ), "Triangle 2" );
			quint16 c = nif->get<quint16>( iBigTris.child( r, 0 ), "Triangle 3" );

			glBegin( GL_TRIANGLES );

			glVertex( verts[a] * havokScale );
			glVertex( verts[b] * havokScale );
			glVertex( verts[c] * havokScale );

			glEnd();
		}

		glPolygonMode( GL_FRONT_AND_BACK, solid ? GL_LINE : GL_FILL );
		glEnable( GL_CULL_FACE );

		QModelIndex iChunks = nif->getIndex( iData, "Chunks" );
		for ( int r = 0; r < nif->rowCount( iChunks ); r++ ) {
			Vector4 chunkOrigin = nif->get<Vector4>( iChunks.child( r, 0 ), "Translation" );

			quint32 transformIndex = nif->get<quint32>( iChunks.child( r, 0 ), "Transform Index" );
			QModelIndex chunkTransform = iChunkTrans.child( transformIndex, 0 );
			Vector4 chunkTranslation = nif->get<Vector4>( chunkTransform.child( 0, 0 ) );
			Quat chunkRotation = nif->get<Quat>( chunkTransform.child( 1, 0 ) );

			quint32 numOffsets = nif->get<quint32>( iChunks.child( r, 0 ), "Num Vertices" );
			quint32 numIndices = nif->get<quint32>( iChunks.child( r, 0 ), "Num Indices" );
			quint32 numStrips = nif->get<quint32>( iChunks.child( r, 0 ), "Num Strips" );
			QVector<quint16> offsets = nif->getArray<quint16>( iChunks.child( r, 0 ), "Vertices" );
			QVector<quint16> indices = nif->getArray<quint16>( iChunks.child( r, 0 ), "Indices" );
			QVector<quint16> strips = nif->getArray<quint16>( iChunks.child( r, 0 ), "Strips" );

			QVector<Vector4> vertices( numOffsets / 3 );

			int numStripVerts = 0;
			int offset = 0;

			for ( int v = 0; v < (int)numStrips; v++ ) {
				numStripVerts += strips[v];
			}

			for ( int n = 0; n < ((int)numOffsets / 3); n++ ) {
				vertices[n] = chunkOrigin + chunkTranslation + Vector4( offsets[3 * n], offsets[3 * n + 1], offsets[3 * n + 2], 0 ) / 1000.0f;
				vertices[n] *= havokScale;
			}

			glPolygonMode( GL_FRONT_AND_BACK, solid ? GL_FILL : GL_LINE );
			glDisable( GL_CULL_FACE );

			Transform trans;
			trans.rotation.fromQuat( chunkRotation );

			// Stripped tris
			for ( int s = 0; s < (int)numStrips; s++ ) {

				for ( int idx = 0; idx < strips[s] - 2; idx++ ) {

					glBegin( GL_TRIANGLES );

					glVertex( trans.rotation * Vector3( vertices[indices[offset + idx]] ) );
					glVertex( trans.rotation * Vector3( vertices[indices[offset + idx + 1]] ) );
					glVertex( trans.rotation * Vector3( vertices[indices[offset + idx + 2]] ) );

					glEnd();

				}

				offset += strips[s];

			}

			// Non-stripped tris
			for ( int f = 0; f < (int)(numIndices - offset); f += 3 ) {
				glBegin( GL_TRIANGLES );

				glVertex( trans.rotation * Vector3( vertices[indices[offset + f]] ) );
				glVertex( trans.rotation * Vector3( vertices[indices[offset + f + 1]] ) );
				glVertex( trans.rotation * Vector3( vertices[indices[offset + f + 2]] ) );

				glEnd();

			}

			glPolygonMode( GL_FRONT_AND_BACK, solid ? GL_LINE : GL_FILL );
			glEnable( GL_CULL_FACE );

		}
	}
}

// Renders text using the font initialized in the primary view class
void renderText( const Vector3 & c, const QString & str )
{
	renderText( c[0], c[1], c[2], str );
}
void renderText( double x, double y, double z, const QString & str )
{
	glPushAttrib( GL_ALL_ATTRIB_BITS );

	glDisable( GL_TEXTURE_1D );
	glDisable( GL_TEXTURE_2D );
	glDisable( GL_CULL_FACE );

	glRasterPos3d( x, y, z );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glEnable( GL_BLEND );
	glAlphaFunc( GL_GREATER, 0.0 );
	glEnable( GL_ALPHA_TEST );

	QByteArray cstr( str.toLatin1() );
	glCallLists( cstr.size(), GL_UNSIGNED_BYTE, cstr.constData() );
	glPopAttrib();
}

