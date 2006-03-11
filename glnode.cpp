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

#include "glnode.h"
#include "glcontroller.h"
#include "glscene.h"

class TransformController : public Controller
{
public:
	TransformController( Node * node, const QModelIndex & index )
		: Controller( index ), target( node ), lTrans( 0 ), lRotate( 0 ), lScale( 0 ) {}
	
	void update( float time )
	{
		if ( ! ( flags.controller.active && target ) )
			return;
		
		time = ctrlTime( time );
		
		interpolate( target->local.rotation, iRotate, time, lRotate );
		interpolate( target->local.translation, iTrans, time, lTrans );
		interpolate( target->local.scale, iScale, time, lScale );
	}
	
	void update( const NifModel * nif, const QModelIndex & index )
	{
		Controller::update( nif, index );
		
		if ( ( iBlock.isValid() && iBlock == index ) || ( iInterpolator.isValid() && iInterpolator == index ) || ( iData.isValid() && iData == index ) )
		{
			iInterpolator = nif->getBlock( nif->getLink( iBlock, "Interpolator" ), "NiTransformInterpolator" );
			iData = nif->getBlock( nif->getLink( iInterpolator.isValid() ? iInterpolator : iBlock, "Data" ) );
			
			iTrans = nif->getIndex( iData, "Translations" );
			iRotate = nif->getIndex( iData, "Rotations" );
			iScale = nif->getIndex( iData, "Scales" );
		}
	}
	
protected:
	QPersistentModelIndex iInterpolator;
	QPersistentModelIndex iData;
	
	QPointer<Node> target;
	
	QPersistentModelIndex iTrans, iRotate, iScale;
	
	int lTrans, lRotate, lScale;
};

class VisibilityController : public Controller
{
public:
	VisibilityController( Node * node, const QModelIndex & index )
		: Controller( index ), target( node ), visLast( 0 ) {}
	
	void update( float time )
	{
		if ( ! ( flags.controller.active && target ) )
			return;
		
		time = ctrlTime( time );
		
		bool isVisible;
		if ( interpolate( isVisible, iKeys, time, visLast ) )
		{
			target->flags.node.hidden = ! isVisible;
		}
	}
	
	void update( const NifModel * nif, const QModelIndex & index )
	{
		Controller::update( nif, index );
		
		if ( iBlock.isValid() && iBlock == index )
		{
			iData = nif->getBlock( nif->getLink( iBlock, "Data" ), "NiVisData" );
			if ( ! iData.isValid() )
				iData = nif->getBlock( nif->getLink( iBlock, "Data" ), "NiBoolData" );
			iKeys = nif->getIndex( iData, "Data" );
		}
	}
	
protected:
	QPointer<Node> target;
	
	QPersistentModelIndex iData;
	QPersistentModelIndex iKeys;
	
	int	visLast;
};

/*
 *  Node list
 */

NodeList::NodeList()
{
}

NodeList::NodeList( const NodeList & other )
{
	operator=( other );
}

NodeList::~NodeList()
{
	clear();
}

void NodeList::clear()
{
	foreach( Node * n, nodes )
		del( n );
}

NodeList & NodeList::operator=( const NodeList & other )
{
	clear();
	foreach ( Node * n, other.list() )
		add( n );
	return *this;
}

void NodeList::add( Node * n )
{
	if ( n && ! nodes.contains( n ) )
	{
		++ n->ref;
		nodes.append( n );
	}
}

void NodeList::del( Node * n )
{
	if ( nodes.contains( n ) )
	{
		int cnt = nodes.removeAll( n );
		
		if ( n->ref <= cnt )
		{
			delete n;
		}
		else
			n->ref -= cnt;
	}
}

Node * NodeList::get( const QModelIndex & index ) const
{
	foreach ( Node * n, nodes )
	{
		if ( n->index().isValid() && n->index() == index )
			return n;
	}
	return 0;
}

void NodeList::validate()
{
	QList<Node *> rem;
	foreach ( Node * n, nodes )
	{
		if ( ! n->isValid() )
			rem.append( n );
	}
	foreach ( Node * n, rem )
	{
		del( n );
	}
}

bool compareNodes( const Node * node1, const Node * node2 )
{
	// opaque meshes first (sorted from front to rear)
	// then alpha enabled meshes (sorted from rear to front)
	bool a1 = node1->findProperty<AlphaProperty>();
	bool a2 = node2->findProperty<AlphaProperty>();
	
	if ( a1 == a2 )
		if ( a1 )
			return ( node1->center()[2] < node2->center()[2] );
		else
			return ( node1->center()[2] > node2->center()[2] );
	else
		return a2;
}

void NodeList::sort()
{
	qStableSort( nodes.begin(), nodes.end(), compareNodes );
}


/*
 *	Node
 */


Node::Node( Scene * s, const QModelIndex & index ) : Controllable( s, index ), parent( 0 ), ref( 0 )
{
	nodeId = 0;
	flags.bits = 0;
}

void Node::clear()
{
	Controllable::clear();

	nodeId = 0;
	flags.bits = 0;
	local = Transform();
	
	children.clear();
	properties.clear();
}

void Node::update( const NifModel * nif, const QModelIndex & index )
{
	Controllable::update( nif, index );
	
	if ( ! iBlock.isValid() )
	{
		clear();
		return;
	}
	
	nodeId = nif->getBlockNumber( iBlock );

	if ( iBlock == index )
	{
		flags.bits = nif->get<int>( iBlock, "Flags" );
		local = Transform( nif, iBlock );
	}
	
	if ( iBlock == index || ! index.isValid() )
	{
		PropertyList newProps;
		QModelIndex iProperties = nif->getIndex( nif->getIndex( iBlock, "Properties" ), "Indices" );
		if ( iProperties.isValid() )
		{
			for ( int p = 0; p < nif->rowCount( iProperties ); p++ )
			{
				QModelIndex iProp = nif->getBlock( nif->getLink( iProperties.child( p, 0 ) ) );
				newProps.add( scene->getProperty( nif, iProp ) );
			}
		}
		properties = newProps;
		
		children.clear();
		QModelIndex iChildren = nif->getIndex( nif->getIndex( iBlock, "Children" ), "Indices" );
		QList<qint32> lChildren = nif->getChildLinks( nif->getBlockNumber( iBlock ) );
		if ( iChildren.isValid() )
		{
			for ( int c = 0; c < nif->rowCount( iChildren ); c++ )
			{
				qint32 link = nif->getLink( iChildren.child( c, 0 ) );
				if ( lChildren.contains( link ) )
				{
					QModelIndex iChild = nif->getBlock( link );
					Node * node = scene->getNode( nif, iChild );
					if ( node )
					{
						node->makeParent( this );
					}
				}
			}
		}
	}
}

void Node::makeParent( Node * newParent )
{
	if ( parent )
		parent->children.del( this );
	parent = newParent;
	if ( parent )
		parent->children.add( this );
}

void Node::setController( const NifModel * nif, const QModelIndex & iController )
{
	QString cname = nif->itemName( iController );
	if ( cname == "NiKeyframeController" || cname == "NiTransformController" )
	{
		Controller * ctrl = new TransformController( this, iController );
		ctrl->update( nif, iController );
		controllers.append( ctrl );
	}
	else if ( cname == "NiVisController" )
	{
		Controller * ctrl = new VisibilityController( this, iController );
		ctrl->update( nif, iController );
		controllers.append( ctrl );
	}
}

const Transform & Node::viewTrans() const
{
	if ( scene->viewTrans.contains( nodeId ) )
		return scene->viewTrans[ nodeId ];
	
	Transform t;
	if ( parent )
		t = parent->viewTrans() * local;
	else
		t = scene->view * worldTrans();
	
	scene->viewTrans.insert( nodeId, t );
	return scene->viewTrans[ nodeId ];
}

const Transform & Node::worldTrans() const
{
	if ( scene->worldTrans.contains( nodeId ) )
		return scene->worldTrans[ nodeId ];

	Transform t = local;
	if ( parent )
		t = parent->worldTrans() * t;
	
	scene->worldTrans.insert( nodeId, t );
	return scene->worldTrans[ nodeId ];
}

Transform Node::localTransFrom( int root ) const
{
	Transform trans;
	const Node * node = this;
	while ( node && node->nodeId != root )
	{
		trans = node->local * trans;
		node = node->parent;
	}
	return trans;
}

Vector3 Node::center() const
{
	return worldTrans().translation;
}

Node * Node::findParent( int id ) const
{
	Node * node = parent;
	while ( node && node->nodeId != id )
		node = node->parent;
	return node;
}

Node * Node::findChild( int id ) const
{
	foreach ( Node * child, children.list() )
	{
		if ( child->nodeId == id )
			return child;
		child = child->findChild( id );
		if ( child )
			return child;
	}
	return 0;
}

bool Node::isHidden() const
{
	if ( scene->showHidden )
		return false;
	
	if ( flags.node.hidden || ( parent && parent->isHidden() ) )
		return true;
	
	return ( name.contains( "collidee" ) || name.contains( "shadowcaster" ) || name.contains( "!LoD_cullme" ) || name.contains( "footprint" ) );
}

void Node::transform()
{
	Controllable::transform();

	foreach ( Node * node, children.list() )
		node->transform();
}

void Node::transformShapes()
{
	foreach ( Node * node, children.list() )
		node->transformShapes();
}

void Node::draw( NodeList * draw2nd )
{
	if ( isHidden() )
		return;
	
	glLoadName( nodeId );
	
	glEnable( GL_DEPTH_TEST );
	glDepthMask( GL_TRUE );
	glDepthFunc( GL_ALWAYS );
	glDisable( GL_TEXTURE_2D );
	glDisable( GL_NORMALIZE );
	glDisable( GL_LIGHTING );
	glDisable( GL_COLOR_MATERIAL );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE );
	
	if ( scene->highlight && scene->currentNode == nodeId )
		glColor( Color4( 0.3, 0.4, 1.0, 0.8 ) );
	else
		glColor( Color4( 0.2, 0.2, 0.7, 0.5 ) );
	glPointSize( 8.5 );
	glLineWidth( 2.5 );
	
	Vector3 a = viewTrans().translation;
	Vector3 b = a;
	if ( parent )
		b = parent->viewTrans().translation;
	
	glBegin( GL_POINTS );
	glVertex( a );
	glEnd();
	
	glBegin( GL_LINES );
	glVertex( a );
	glVertex( b );
	glEnd();
	
	foreach ( Node * node, children.list() )
	{
		node->draw( draw2nd );
	}
}

void Node::drawShapes( NodeList * draw2nd )
{
	if ( isHidden() )
		return;
	
	foreach ( Node * node, children.list() )
	{
		node->drawShapes( draw2nd );
	}
}

void Node::setupRenderState()
{
	// setup lighting
	
	scene->setupLights( this );
	
	// setup blending
	
	glProperty( findProperty< AlphaProperty >() );
	
	// setup material
	
	glProperty( findProperty< MaterialProperty >(), findProperty< SpecularProperty >() );

	// setup vertex colors
	
	glProperty( findProperty< VertexColorProperty >() );
	
	// setup texturing
	
	glProperty( findProperty< TexturingProperty >() );
	
	// setup z buffer
	
	glProperty( findProperty< ZBufferProperty >() );
	
	// setup stencil
	
	glProperty( findProperty< StencilProperty >() );
	
	// wireframe ?
	
	glProperty( findProperty< WireframeProperty >() );

	// normalize
	
	glEnable( GL_NORMALIZE );
}


BoundSphere Node::bounds() const
{
	if ( scene->showNodes )
		return BoundSphere( worldTrans().translation, 1 );
	else
		return BoundSphere();
}

BoundSphere::BoundSphere()
{
	radius	= 0;
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
		radius	= 0;
	}
	else
	{
		center	= Vector3();
		foreach ( Vector3 v, verts )
		{
			center += v;
		}
		center /= verts.count();
		
		radius	= 0.1;
		foreach ( Vector3 v, verts )
		{
			float d = ( center - v ).length();
			if ( d > radius )
				radius = d;
		}
	}
}

BoundSphere & BoundSphere::operator=( const BoundSphere & other )
{
	center	= other.center;
	radius	= other.radius;
	return *this;
}

BoundSphere & BoundSphere::operator|=( const BoundSphere & other )
{
	if ( other.radius <= 0 )
		return *this;
	if ( radius <= 0 )
		return operator=( other );
	
	float d = ( center - other.center ).length();
	
	if ( radius > d + other.radius )
		return * this;
	if ( other.radius > d + radius )
		return operator=( other );
	
	if ( other.radius > radius )
		radius = other.radius;
	radius += d / 2;
	center = ( center + other.center ) / 2;
	return *this;
}

BoundSphere BoundSphere::operator|( const BoundSphere & other )
{
	BoundSphere b( *this );
	b |= other;
	return b;
}

BoundSphere operator*( const Transform & t, const BoundSphere & sphere )
{
	BoundSphere bs( sphere );
	bs.center = t * bs.center;
	bs.radius *= fabs( t.scale );
	return bs;
}
