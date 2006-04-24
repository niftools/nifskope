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

#ifndef GLNODE_H
#define GLNODE_H

#include "glcontrolable.h"
#include "glproperty.h"
#include "gltransform.h"

class Node;

class NodeList
{
public:
	NodeList();
	NodeList( const NodeList & other );
	~NodeList();
	
	void add( Node * );
	void del( Node * );
	
	Node * get( const QModelIndex & idx ) const;
	
	void validate();
	
	void clear();
	
	NodeList & operator=( const NodeList & other );
	
	const QList<Node*> & list() const { return nodes; }
	
	void sort();
	
protected:
	QList<Node*> nodes;
};

class Node : public Controllable
{
	typedef union
	{
		quint16 bits;
		
		struct Node
		{
			bool hidden : 1;
		} node;
	
	} NodeFlags;
	
public:
	Node( Scene * scene, const QModelIndex & block );
	
	virtual void clear();
	virtual void update( const NifModel * nif, const QModelIndex & block );
	
	virtual void transform();
	virtual void transformShapes();
	
	virtual void draw( NodeList * draw2nd = 0 );
	virtual void drawShapes( NodeList * draw2nd = 0 );
	virtual void drawHavok();
	
	virtual const Transform & viewTrans() const;
	virtual const Transform & worldTrans() const;
	virtual const Transform & localTrans() const { return local; }
	virtual Transform localTransFrom( int parentNode ) const;
	virtual Vector3 center() const;
	
	virtual bool isHidden() const;
	bool isVisible() const { return ! isHidden(); }
	
	int id() const { return nodeId; }
	
	Node	* findParent( int id ) const;
	Node	* findChild( int id ) const;
	Node	* parentNode() const { return parent; }
	void	makeParent( Node * parent );
	
	virtual class BoundSphere bounds() const;
	//Vector3 center() const { return bounds().center(); }
	//float radius() const { return bounds().radius; }
	
	template <typename T> T * findProperty() const;
	
	void setupRenderState( bool vertexcolors );
	
protected:
	virtual void setController( const NifModel * nif, const QModelIndex & controller );

	QPointer<Node> parent;
	
	int ref;

	NodeList children;
	PropertyList properties;
	
	int nodeId;
	
	Transform local;

	NodeFlags flags;
	
	friend class TransformController;
	friend class VisibilityController;
	friend class NodeList;
	friend class LODNode;
};

template <typename T> inline T * Node::findProperty() const
{
	T * prop = properties.get<T>();
	if ( prop ) return prop;
	if ( parent ) return parent->findProperty<T>();
	return 0;
}

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

#endif


