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

#ifndef GLTOOLS_H
#define GLTOOLS_H

#include <QGLContext>

#include "../niftypes.h"

//! \file gltools.h BoundSphere, VertexWeight, BoneWeights, SkinPartition

//! A bounding sphere for an object, typically a Mesh
class BoundSphere
{
public:
	BoundSphere();
	BoundSphere( const BoundSphere & );
	BoundSphere( const Vector3 & center, float radius );
	BoundSphere( const QVector<Vector3> & vertices );
	
	Vector3	center;
	float	radius;
	
	BoundSphere & operator=( const BoundSphere & );
	BoundSphere & operator|=( const BoundSphere & );
	
	BoundSphere operator|( const BoundSphere & o );
	
	BoundSphere & apply( const Transform & t );
	BoundSphere & applyInv( const Transform & t );
	
	friend BoundSphere operator*( const Transform & t, const BoundSphere & s );
};

//! A vertex, weight pair
class VertexWeight
{
public:
	VertexWeight()
	{ vertex = 0; weight = 0.0; }
	VertexWeight( int v, float w )
	{ vertex = v; weight = w; }
	
	int vertex;
	float weight;
};

//! A set of vertices weighted to a bone
class BoneWeights
{
public:
	BoneWeights() { bone = 0; }
	BoneWeights( const NifModel * nif, const QModelIndex & index, int b, int vcnt = 0 );
	
	Transform trans;
	Vector3 center; float radius;
	Vector3 tcenter;
	int bone;
	QVector<VertexWeight> weights;
};

//! A skin partition
class SkinPartition
{
public:
	SkinPartition() { numWeightsPerVertex = 0; }
	SkinPartition( const NifModel * nif, const QModelIndex & index );
	
	QVector<int> boneMap;
	QVector<int> vertexMap;
	
	int numWeightsPerVertex;
	QVector< QPair< int, float > > weights;
	
	QVector< Triangle > triangles;
	QList< QVector< quint16 > > tristrips;
};

void drawAxes( Vector3 c, float axis );
void drawBox( Vector3 a, Vector3 b );
void drawCircle( Vector3 c, Vector3 n, float r, int sd = 16 );
void drawArc( Vector3 c, Vector3 x, Vector3 y, float an, float ax, int sd = 8 );
void drawSolidArc( Vector3 c, Vector3 n, Vector3 x, Vector3 y, float an, float ax, float r, int sd = 8 );
void drawCone( Vector3 c, Vector3 n, float a, int sd = 16 );
void drawRagdollCone( Vector3 pivot, Vector3 twist, Vector3 plane, float coneAngle, float minPlaneAngle, float maxPlaneAngle, int sd = 16 );
void drawSphere( Vector3 c, float r, int sd = 8 );
void drawCapsule( Vector3 a, Vector3 b, float r, int sd = 5 );
void drawDashLine( Vector3 a, Vector3 b, int sd = 15 );
void drawConvexHull( QVector<Vector4> vertices, QVector<Vector4> normals );
void drawSpring( Vector3 a, Vector3 b, float stiffness, int sd = 16, bool solid = false );
void drawRail( const Vector3 &a, const Vector3 &b );

inline void glTranslate( const Vector3 & v )
{
	glTranslatef( v[0], v[1], v[2] );
}

inline void glScale( const Vector3 & v )
{
	glScalef( v[0], v[1], v[2] );
}

inline void glVertex( const Vector2 & v )
{
	glVertex2fv( v.data() );
}

inline void glVertex( const Vector3 & v )
{
	glVertex3fv( v.data() );
}

inline void glVertex( const Vector4 & v )
{
	glVertex3fv( v.data() );
}

inline void glNormal( const Vector3 & v )
{
	glNormal3fv( v.data() );
}

inline void glTexCoord( const Vector2 & v )
{
	glTexCoord2fv( v.data() );
}

inline void glColor( const Color3 & c )
{
	glColor3fv( c.data() );
}

inline void glColor( const Color4 & c )
{
	glColor4fv( c.data() );
}

inline void glMaterial( GLenum x, GLenum y, const Color4 & c )
{
	glMaterialfv( x, y, c.data() );
}

inline void glLoadMatrix( const Matrix4 & m )
{
	glLoadMatrixf( m.data() );
}

inline void glMultMatrix( const Matrix4 & m )
{
	glMultMatrixf( m.data() );
}

inline void glLoadMatrix( const Transform & t )
{
	glLoadMatrix( t.toMatrix4() );
}

inline void glMultMatrix( const Transform & t )
{
	glMultMatrix( t.toMatrix4() );
}


inline GLuint glClosestMatch( GLuint * buffer, GLint hits )
{	// a little helper function, returns the closest matching hit from the name buffer
	GLuint	choose = buffer[ 3 ];
	GLuint	depth = buffer[ 1 ];
	for ( int loop = 1; loop < hits; loop++ )
	{
		if ( buffer[ loop * 4 + 1 ] < depth )
		{
			choose = buffer[ loop * 4 + 3 ];
			depth = buffer[ loop * 4 + 1 ];
		}       
	}
	return choose;
}

void renderText(double x, double y, double z, const QString & str);
void renderText(const Vector3& c, const QString & str);

#define ID2COLORKEY(id) (id + 1)
#define COLORKEY2ID(id) (id - 1)

#endif
