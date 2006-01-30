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

#include "nifmodel.h"

class Scene;

class Transform
{
public:
	Transform( const NifModel * nif, const QModelIndex & transform )
	{
		rotation = nif->get<Matrix>( transform, "Rotation" );
		translation = nif->get<Vector3>( transform, "Translation" );
		scale = nif->get<float>( transform, "Scale" );
	}
	
	Transform()
	{
		scale = 1.0;
	}
	
	void writeBack( NifModel * nif, const QModelIndex & transform ) const
	{
		nif->set<Matrix>( transform, "Rotation", rotation );
		nif->set<Vector3>( transform, "Translation", translation );
		nif->set<float>( transform, "Scale", scale );
	}
	
	Transform operator*( const Transform & m ) const
	{
		Transform t;
		t.translation = rotation * m.translation + translation * m.scale;
		t.rotation = rotation * m.rotation;
		t.scale = m.scale * scale;
		return t;
	}
	
	Vector3 operator*( const Vector3 & v ) const
	{
		return rotation * v * scale + translation;
	}
	
	void glMultMatrix() const;
	void glLoadMatrix() const;

	Matrix rotation;
	Vector3 translation;
	GLfloat scale;
};

class Tristrip
{
public:
	Tristrip() {}
	Tristrip( const NifModel * nif, const QModelIndex & );
	
	QVector<int>	vertices;
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
	BoneWeights( const NifModel * nif, const QModelIndex & index, int b );
	
	Transform trans;
	int bone;
	QVector<VertexWeight> weights;
};

class Controller;

class Controllable
{
public:
	Controllable( Scene * Scene, const QModelIndex & index );
	virtual ~Controllable();
	
	virtual void clear();
	virtual bool update( const QModelIndex & index );
	
	virtual void transform();
	
	virtual void timeBounds( float & start, float & stop );

protected:
	Scene * scene;
	
	QPersistentModelIndex iBlock;

	QList<Controller*> controllers;
};

typedef union
{
	quint16 bits;
	
	struct Node
	{
		bool hidden : 1;
	} node;

} NodeFlags;

class Node : public Controllable
{
public:
	Node( Scene * scene, Node * parent, const QModelIndex & block );
	
	virtual void clear();
	virtual bool make();
	virtual bool update( const QModelIndex & block );
	
	virtual const Transform & worldTrans();
	virtual Transform localTransFrom( int parentNode );
	
	virtual void transform();
	virtual void draw( bool selected );
	
	bool isHidden() const;
	
	int id() const { return nodeId; }
	
	void depthBuffer( bool & test, bool & mask );
	
	virtual void boundaries( Vector3 & min, Vector3 & max );
	
protected:
	virtual void setController( const NifModel * nif, const QModelIndex & controller );
	virtual void setProperty( const NifModel * nif, const QModelIndex & property );
	virtual void setSpecial( const NifModel * nif, const QModelIndex & special );

	Node * parent;
	
	QPersistentModelIndex block;
	QList<QPersistentModelIndex> blocks;
	
	int nodeId;
	
	Transform local;

	NodeFlags flags;
	
	bool depthProp;
	bool depthTest;
	bool depthMask;
	
	friend class KeyframeController;
	friend class VisibilityController;
};

class Mesh : public Node
{
public:
	Mesh( Scene * s, Node * parent, const QModelIndex & block );
	
	void clear();
	bool make();
	
	void transform();
	void draw( bool selected );
	
	void boundaries( Vector3 & min, Vector3 & max );
	
protected:	
	void setSpecial( const NifModel * nif, const QModelIndex & special );
	void setProperty( const NifModel * nif, const QModelIndex & property );
	void setController( const NifModel * nif, const QModelIndex & controller );
	
	Vector3 localCenter;
	Vector3 sceneCenter;
	
	QVector<Vector3> verts;
	QVector<Vector3> norms;
	QVector<Color4>  colors;
	QVector<Vector2> uvs;
	
	Color4 ambient, diffuse, specular, emissive;
	GLfloat shininess, alpha;
	
	QPersistentModelIndex iBaseTex;
	GLenum texFilter;
	GLint texWrapS, texWrapT;
	Vector2 texOffset;
	int texSet;
	
	bool alphaBlend, alphaTest;
	GLenum alphaSrc, alphaDst, alphaFunc;
	GLfloat alphaThreshold;
	
	bool specularEnable;
	
	int skelRoot;
	Transform skelTrans;
	QVector<BoneWeights> weights;
	
	QVector<Triangle> triangles;
	QVector<Tristrip> tristrips;
	
	QVector< QPair< int, float > > triOrder;
	
	QVector<Vector3> transVerts;
	QVector<Vector3> transNorms;
	
	friend bool compareMeshes( const Mesh * mesh1, const Mesh * mesh2 );
	
	friend class AlphaController;
	friend class MorphController;
	friend class TexFlipController;
	friend class TexCoordController;
};

class GLTex
{
public:
	GLTex( const QModelIndex & );
	~GLTex();
	
	void release();

	static void initialize( const QGLContext * context );
	
	static QString findFile( const QString & file );
	
	bool exportFile( const QString & file );

	GLuint		id;

	QPersistentModelIndex iSource;
	
	bool		external;
	
	QString		filepath;
	bool		readOnly;
	QDateTime	loaded;

	QPersistentModelIndex iPixelData;

	static QStringList texfolders;

private:
	static QIcon * icon;
};


class Scene
{
public:
	Scene();
	~Scene();

	void clear();
	void make( NifModel * nif );
	void make( NifModel * nif, int blockNumber, QStack<int> & nodestack );
	
	void update( const NifModel * nif, const QModelIndex & index );
	
	void transform( const Transform & trans, float time = 0.0 );
	void draw();
	
	bool bindTexture( const QModelIndex & );

	QList<Mesh*> meshes;
	QHash<int,Node*> nodes;
	
	QHash<int,Transform> worldTrans;
	
	Transform view;
	
	bool animate;
	
	float time;

	bool texturing;
	QList<GLTex*> textures;
	
	bool blending;
	
	bool highlight;
	int currentNode;
	
	bool drawNodes;
	bool drawHidden;
	
	Vector3 boundMin, boundMax, boundCenter, boundRadius;
	float timeMin, timeMax;
};

#endif
