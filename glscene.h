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

#ifndef GLSCENE_H
#define GLSCENE_H

#include <QtOpenGL>

#include "glmath.h"

#include "nifmodel.h"

class Scene;

class Triangle
{
public:
	Triangle()
	{ v1 = v2 = v3 = 0; depth = 0.0; }
	Triangle( NifModel * nif, const QModelIndex & );
	
	int v1, v2, v3;
	GLfloat depth;
};

class Tristrip
{
public:
	Tristrip() {}
	Tristrip( NifModel * nif, const QModelIndex & );
	
	QVector<int>	vertices;
};

class Color
{
public:
	Color()
	{ rgba[ 0 ] = rgba[ 1 ] = rgba[ 2 ] = rgba[ 3 ] = 0.0; }
	
	Color( GLfloat r, GLfloat g, GLfloat b, GLfloat a )
	{ rgba[ 0 ] = r; rgba[ 1 ] = g; rgba[ 2 ] = b; rgba[ 3 ] = a; }
	
	Color( const QColor & );
	
	inline void glColor() const
	{
		glColor4fv( rgba );
	}
	
	inline void glMaterial( GLenum x, GLenum y )
	{
		glMaterialfv( x, y, rgba );
	}
	
	GLfloat & operator[]( int i )
	{
		return rgba[ i ];
	}
	
	GLfloat rgba[4];
};

class VertexWeight
{
public:
	VertexWeight()
	{ vertex = 0; weight = 0.0; }
	VertexWeight( int v, GLfloat w )
	{ vertex = v; weight = w; }
	
	int vertex;
	GLfloat weight;
};

class BoneWeights
{
public:
	BoneWeights() { bone = 0; }
	BoneWeights( NifModel * nif, const QModelIndex & index, int b );
	
	Matrix matrix;
	int bone;
	QVector<VertexWeight> weights;
};

class Mesh
{
public:
	Mesh( Scene * s );
	Mesh( NifModel * nif, const QModelIndex & index, Scene * s, int parent );
	~Mesh();
	
	void init();
	
	void setData( NifModel * nif, const QModelIndex & tridata );
	void setProperty( NifModel * nif, const QModelIndex & property );
	void setSkinInstance( NifModel * nif, const QModelIndex & skininst );
	
	void draw();
	
	void transform( const Matrix & trans );

	void boundaries( Vector & min, Vector & max );

	Matrix localTrans;
	Matrix sceneTrans;
	Matrix glmatrix;
	
	Vector localCenter;
	Vector sceneCenter;
	
	Scene * scene;
	
	int parentNode;
	int meshId;

	bool useList;
	GLuint list;
	
	QVector<Vector> verts;
	QVector<Vector> norms;
	QVector<Color>  colors;
	QVector<GLfloat> uvs;
	
	Color ambient, diffuse, specular, emissive;
	GLfloat shininess, alpha;
	
	QString texFile;
	GLenum texFilter;
	GLint texWrapS, texWrapT;
	int texSet;
	
	bool alphaEnable;
	GLenum alphaSrc;
	GLenum alphaDst;
	
	bool specularEnable;
	
	QVector<BoneWeights> weights;
	
	QVector<Triangle> triangles;
	QVector<Tristrip> tristrips;

	QVector<Vector> transVerts;
	QVector<Vector> transNorms;
};

class Node
{
public:
	Node() {}
	
	Matrix localTrans;
	Matrix worldTrans;
};

class GLTex
{
public:
	static GLTex * create( const QString & filepath, const QGLContext * context );
	
	~GLTex();

	GLuint		id;
	QString		filepath;
	bool		readOnly;
	QDateTime	loaded;
	
protected:
	GLTex();
};


class Scene
{
public:
	Scene( const QGLContext * context );
	~Scene();

	void clear();
	void make( NifModel * nif );
	void make( NifModel * nif, int blockNumber );
	
	void draw( const Matrix & matrix );
	void drawAgain();
	
	void boundaries( Vector & min, Vector & max );
	
	GLuint bindTexture( const QString & );

	QList<Mesh*> meshes;
	QHash<int,Node*> nodes;
	
	QStack<int> nodestack;
	QStack<Matrix> matrixstack;

	bool texturing;
	QString texfolder;
	QCache<QString,GLTex> textures;
	
	bool blending;
	
	const QGLContext * context;
};

#endif
