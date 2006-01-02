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
	
	Color operator*( const Color & m ) const
	{
		Color r = *this;
		for ( int c = 0; c < 4; c++ )
			r.rgba[c] *= m.rgba[c];
		return r;
	}
	
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
	
	Transform trans;
	int bone;
	QVector<VertexWeight> weights;
};

typedef union
{
	quint16 bits;
	
	struct Node
	{
		bool hidden : 1;
	} node;

	struct Controller
	{
		bool unknown : 1;
		enum
		{
			Cyclic = 0, Reverse = 1, Constant = 2
		} extrapolation : 2;
		bool active : 1;
	} controller;
	
} Flags;
	

class Controller
{
public:
	Controller( NifModel * nif, const QModelIndex & index );
	virtual ~Controller() {}
	
	float start;
	float stop;
	float phase;
	float frequency;
	
	Flags flags;
	
	virtual void update( float time ) = 0;
	
protected:
	float ctrlTime( float time ) const;
	
	static void timeIndex( float time, const QVector<float> & times, int & i, int & j, float & x );
};

class Node
{
public:
	Node( Scene * scene, Node * parent );
	virtual ~Node() { qDeleteAll( controllers ); }
	
	virtual void init( NifModel * nif, const QModelIndex & block );
	
	virtual const Transform & worldTrans();
	virtual Transform localTransFrom( int parentNode );
	
	virtual void transform();
	virtual void draw( bool selected );
	
	bool isHidden() const;
	
	int id() const { return nodeId; }
	
	void depthBuffer( bool & test, bool & mask );
	
	virtual void boundaries( Vector & min, Vector & max );
	virtual void timeBounds( float & start, float & stop );
	
protected:
	virtual void setController( NifModel * nif, const QModelIndex & controller );
	virtual void setProperty( NifModel * nif, const QModelIndex & property );
	virtual void setSpecial( NifModel * nif, const QModelIndex & special );

	Scene * scene;
	Node * parent;
	
	int nodeId;
	
	Transform local;
	Transform localOrig;

	Flags flags;
	
	bool depthProp;
	bool depthTest;
	bool depthMask;
	
	QList<Controller*> controllers;
	
	friend class KeyframeController;

private:
	bool		worldDirty;
	Transform	world;
};

class Mesh : public Node
{
public:
	Mesh( Scene * s, Node * parent );
	
	void init( NifModel * nif, const QModelIndex & block );
	
	void transform();
	
	void draw( bool selected );
	
	void boundaries( Vector & min, Vector & max );
	
protected:	
	void setSpecial( NifModel * nif, const QModelIndex & special );
	void setProperty( NifModel * nif, const QModelIndex & property );
	void setController( NifModel * nif, const QModelIndex & controller );
	
	Vector localCenter;
	Vector sceneCenter;
	
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
	
	int skelRoot;
	Transform skelTrans;
	QVector<BoneWeights> weights;
	
	QVector<Triangle> triangles;
	QVector<Tristrip> tristrips;
	
	QVector<Vector> transVerts;
	QVector<Vector> transNorms;
	
	friend bool compareMeshes( const Mesh * mesh1, const Mesh * mesh2 );
	friend class AlphaController;
};

class GLTex
{
public:
	GLTex();
	~GLTex();
	
	void release();

	static void initialize( const QGLContext * context );
	

	GLuint		id;
	QString		filepath;
	bool		readOnly;
	QDateTime	loaded;
};


class Scene
{
public:
	Scene();
	~Scene();

	void clear();
	void make( NifModel * nif );
	void make( NifModel * nif, int blockNumber, QStack<int> & nodestack );
	
	void transform( const Transform & trans, float time = 0.0 );
	void draw();
	
	GLuint bindTexture( const QString & );

	QList<Mesh*> meshes;
	QHash<int,Node*> nodes;
	
	Transform view;
	
	bool animate;
	
	float time;

	bool texturing;
	QStringList texfolders;
	QCache<QString,GLTex> textures;
	
	bool blending;
	
	bool highlight;
	int currentNode;
	
	bool drawNodes;
	bool drawHidden;
	
	Vector boundMin, boundMax, boundCenter, boundRadius;
	float timeMin, timeMax;
};


class NodeController : public Controller
{
public:
	NodeController( Node * node, NifModel * nif, const QModelIndex & index )
		: Controller( nif, index ), target( node ) {}
protected:
	Node * target;
};

class MeshController : public Controller
{
public:
	MeshController( Mesh * mesh, NifModel * nif, const QModelIndex & index )
		: Controller( nif, index ), target( mesh ) {}
protected:
	Mesh * target;
};

class KeyframeController : public NodeController
{
public:
	KeyframeController( Node * node, NifModel * nif, const QModelIndex & index );
	
	void update( float time );
	
protected:
	QVector<float> transTime;
	QVector<Vector> transData;
	int transIndex;
	
	QVector<float> rotTime;
	QVector<Quat> rotData;
	int rotIndex;
	
	QVector<float> scaleTime;
	QVector<float> scaleData;
	int scaleIndex;
};

class AlphaController : public MeshController
{
public:
	AlphaController( Mesh * mesh, NifModel * nif, const QModelIndex & index );
	
	void update( float time );

protected:
	QVector<float> alphaTime;
	QVector<float> alphaData;
	
	int alphaIndex;
};

#endif
