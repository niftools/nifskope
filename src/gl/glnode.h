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

#ifndef GLNODE_H
#define GLNODE_H

#include "gl/icontrollable.h" // Inherited
#include "gl/glproperty.h"

#include <QList>
#include <QPersistentModelIndex>
#include <QPointer>


//! @file glnode.h Node, NodeList

class Node;
class NifModel;

class NodeList final
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

	const QVector<Node *> & list() const { return nodes; }

	void sort();
	void alphaSort();

protected:
	QVector<Node *> nodes;
};

class Node : public IControllable
{
	friend class ControllerManager;
	friend class KeyframeController;
	friend class MultiTargetTransformController;
	friend class TransformController;
	friend class VisibilityController;
	friend class NodeList;
	friend class LODNode;

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

	static int SELECTING;

	int id() const { return nodeId; }

	// IControllable

	void clear() override;
	void update( const NifModel * nif, const QModelIndex & block ) override;
	void transform() override;

	// end IControllable

	virtual void transformShapes();

	virtual void draw();
	virtual void drawShapes( NodeList * secondPass = nullptr, bool presort = false );
	virtual void drawHavok();
	virtual void drawFurn();
	virtual void drawSelection() const;

	virtual float viewDepth() const;
	virtual class BoundSphere bounds() const;
	virtual const Vector3 center() const;
	virtual const Transform & viewTrans() const;
	virtual const Transform & worldTrans() const;
	virtual const Transform & localTrans() const { return local; }
	virtual Transform localTrans( int parentNode ) const;

	virtual bool isHidden() const;
	virtual QString textStats() const;

	bool isVisible() const { return !isHidden(); }
	bool isPresorted() const { return presorted; }
	
	Node * findChild( int id ) const;
	Node * findChild( const QString & str ) const;

	Node * findParent( int id ) const;
	Node * parentNode() const { return parent; }
	void makeParent( Node * parent );

	template <typename T> T * findProperty() const;
	void activeProperties( PropertyList & list ) const;

	Controller * findController( const QString & proptype, const QString & ctrltype, const QString & var1, const QString & var2 );

	Controller * findController( const QString & proptype, const QModelIndex & index );

public slots:
	void updateSettings();

protected:
	void setController( const NifModel * nif, const QModelIndex & controller ) override;
	// Old Options API
	//	TODO: Move away from the GL-like naming
	void glHighlightColor() const;
	void glNormalColor() const;

	QPointer<Node> parent;
	NodeList children;

	PropertyList properties;

	Transform local;

	NodeFlags flags;

	struct Settings
	{
		QColor highlight;
		QColor wireframe;
	} cfg;


	bool presorted = false;

	int nodeId;
	int ref;
};

template <typename T> inline T * Node::findProperty() const
{
	T * prop = properties.get<T>();

	if ( prop )
		return prop;

	if ( parent )
		return parent->findProperty<T>();

	return nullptr;
}

//! A Node with levels of detail
class LODNode : public Node
{
public:
	LODNode( Scene * scene, const QModelIndex & block );

	// IControllable

	void clear() override;
	void update( const NifModel * nif, const QModelIndex & block ) override;
	void transform() override;

	// end IControllable

protected:
	QList<QPair<float, float> > ranges;
	QPersistentModelIndex iData;

	Vector3 center;
};

//! A Node that always faces the camera
class BillboardNode : public Node
{
public:
	BillboardNode( Scene * scene, const QModelIndex & block );

	const Transform & viewTrans() const override;
};


#endif
