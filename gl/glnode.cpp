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
#include "options.h"

#include "NvTriStrip/qtwrapper.h"

#include "FurnitureMarkers.h"

#ifndef M_PI
#define M_PI 3.1415926535897932385
#endif

class TransformController : public Controller
{
public:
	TransformController( Node * node, const QModelIndex & index )
		: Controller( index ), target( node ){}
	
	void update( float time )
	{
		if ( ! ( flags.controller.active && target ) )
			return;
		
		time = ctrlTime( time );
		
		if ( interpolator )
		{
			interpolator->updateTransform(target->local, time);
		}
	}
	
	void setInterpolator( const QModelIndex & iBlock )
	{
		const NifModel * nif = static_cast<const NifModel *>( iBlock.model() );
		if ( nif && iBlock.isValid() )
		{
			if ( interpolator )
			{
				delete interpolator;
				interpolator = 0;
			}
			
			if ( nif->isNiBlock( iBlock, "NiBSplineCompTransformInterpolator" ) )
			{
				iInterpolator = iBlock;
				interpolator = new BSplineTransformInterpolator(this);
			}
			else if ( nif->isNiBlock( iBlock, "NiTransformInterpolator" ) )
			{
				iInterpolator = iBlock;
				interpolator = new TransformInterpolator(this);
			}
			
			if ( interpolator )
			{
				interpolator->update( nif, iInterpolator );
			}
		}
	}
	
protected:
	QPointer<Node> target;
	QPointer<TransformInterpolator> interpolator;
};

class MultiTargetTransformController : public Controller
{
	typedef QPair< QPointer<Node>, QPointer<TransformInterpolator> > TransformTarget;
	
public:
	MultiTargetTransformController( Node * node, const QModelIndex & index )
		: Controller( index ), target( node ) {}
	
	void update( float time )
	{
		if ( ! ( flags.controller.active && target ) )
			return;
		
		time = ctrlTime( time );
		
		foreach ( TransformTarget tt, extraTargets )
		{
			if ( tt.first && tt.second )
			{
				tt.second->updateTransform( tt.first->local, time );
			}
		}
	}
	
	bool update( const NifModel * nif, const QModelIndex & index )
	{
		if ( Controller::update( nif, index ) )
		{
			if ( target )
			{
				Scene * scene = target->scene;
				extraTargets.clear();
				
				QVector<qint32> lTargets = nif->getLinkArray( index, "Extra Targets" );
				foreach ( qint32 l, lTargets )
				{
					Node * node = scene->getNode( nif, nif->getBlock( l ) );
					if ( node )
					{
						extraTargets.append( TransformTarget( node, 0 ) );
					}
				}
				
			}
			return true;
		}
		
		foreach ( TransformTarget tt, extraTargets )
		{
			// TODO: update the interpolators
		}
		
		return false;
	}
	
	bool setInterpolator( Node * node, const QModelIndex & iInterpolator )
	{
		const NifModel * nif = static_cast<const NifModel *>( iInterpolator.model() );
		if ( ! nif || ! iInterpolator.isValid() )
			return false;
		QMutableListIterator<TransformTarget> it( extraTargets );
		while ( it.hasNext() )
		{
			it.next();
			if ( it.value().first == node )
			{
				if ( it.value().second )
				{
					delete it.value().second;
					it.value().second = 0;
				}
				
				if ( nif->isNiBlock( iInterpolator, "NiBSplineCompTransformInterpolator" ) )
				{
					it.value().second = new BSplineTransformInterpolator( this );
				}
				else if ( nif->isNiBlock( iInterpolator, "NiTransformInterpolator" ) )
				{
					it.value().second = new TransformInterpolator( this );
				}
				
				if ( it.value().second )
				{
					it.value().second->update( nif, iInterpolator );
				}
				return true;
			}
		}
		return false;
	}

protected:
	QPointer<Node> target;
	QList< TransformTarget > extraTargets;
};

class ControllerManager : public Controller
{
public:
	ControllerManager( Node * node, const QModelIndex & index )
		: Controller( index ), target( node ) {}
	
	void update( float ) {}
	
	bool update( const NifModel * nif, const QModelIndex & index )
	{
		if ( Controller::update( nif, index ) )
		{
			if ( target )
			{
				Scene * scene = target->scene;
				QVector<qint32> lSequences = nif->getLinkArray( index, "Controller Sequences" );
				foreach ( qint32 l, lSequences )
				{
					QModelIndex iSeq = nif->getBlock( l, "NiControllerSequence" );
					if ( iSeq.isValid() )
					{
						QString name = nif->get<QString>( iSeq, "Name" );
						if ( ! scene->animGroups.contains( name ) )
							scene->animGroups.append( name );
					}
				}
			}
			return true;
		}
		return false;
	}
	
	void setSequence( const QString & seqname )
	{
		const NifModel * nif = static_cast<const NifModel *>( iBlock.model() );
		if ( target && iBlock.isValid() && nif )
		{
			MultiTargetTransformController * multiTargetTransformer = 0;
			foreach ( Controller * c, target->controllers )
			{
				if ( c->typeId() == "NiMultiTargetTransformController" )
				{
					multiTargetTransformer = static_cast<MultiTargetTransformController*>( c );
					break;
				}
			}
			
			QVector<qint32> lSequences = nif->getLinkArray( iBlock, "Controller Sequences" );
			foreach ( qint32 l, lSequences )
			{
				QModelIndex iSeq = nif->getBlock( l, "NiControllerSequence" );
				if ( iSeq.isValid() && nif->get<QString>( iSeq, "Name" ) == seqname )
				{
					start = nif->get<float>( iSeq, "Start Time" );
					stop = nif->get<float>( iSeq, "Stop Time" );
					phase = nif->get<float>( iSeq, "Phase" );
					frequency = nif->get<float>( iSeq, "Frequency" );
					
					QModelIndex iCtrlBlcks = nif->getIndex( iSeq, "Controlled Blocks" );
					for ( int r = 0; r < nif->rowCount( iCtrlBlcks ); r++ )
					{
						QModelIndex iCB = iCtrlBlcks.child( r, 0 );
						
						QModelIndex iInterpolator = nif->getBlock( nif->getLink( iCB, "Interpolator" ), "NiInterpolator" );
						
						QModelIndex idx = nif->getIndex( iCB, "Node Name Offset" );
						QString nodename = idx.sibling( idx.row(), NifModel::ValueCol ).data( Qt::DisplayRole ).toString();
						idx = nif->getIndex( iCB, "Property Type Offset" );
						QString proptype = idx.sibling( idx.row(), NifModel::ValueCol ).data( Qt::DisplayRole ).toString();
						idx = nif->getIndex( iCB, "Controller Type Offset" );
						QString ctrltype = idx.sibling( idx.row(), NifModel::ValueCol ).data( Qt::DisplayRole ).toString();
						idx = nif->getIndex( iCB, "Variable Offset 1" );
						QString var1 = idx.sibling( idx.row(), NifModel::ValueCol ).data( Qt::DisplayRole ).toString();
						idx = nif->getIndex( iCB, "Variable Offset 2" );
						QString var2 = idx.sibling( idx.row(), NifModel::ValueCol ).data( Qt::DisplayRole ).toString();
						
						Node * node = target->findChild( nodename );
						if ( ! node )
							continue;
						
						if ( ctrltype == "NiTransformController" && multiTargetTransformer )
						{
							if ( multiTargetTransformer->setInterpolator( node, iInterpolator ) )
							{
								multiTargetTransformer->start = start;
								multiTargetTransformer->stop = stop;
								multiTargetTransformer->phase = phase;
								multiTargetTransformer->frequency = frequency;
								continue;
							}
						}
						
						Controller * ctrl = node->findController( proptype, ctrltype, var1, var2 );
						if ( ctrl )
						{
							ctrl->start = start;
							ctrl->stop = stop;
							ctrl->phase = phase;
							ctrl->frequency = frequency;
							
							ctrl->setInterpolator( iInterpolator );
						}
					}
				}
			}
		}
	}

protected:
	QPointer<Node> target;
};

class KeyframeController : public Controller
{
public:
	KeyframeController( Node * node, const QModelIndex & index )
		: Controller( index ), target( node ), lTrans( 0 ), lRotate( 0 ), lScale( 0 ) {}
	
	void update( float time )
	{
		if ( ! ( flags.controller.active && target ) )
			return;
		
		time = ctrlTime( time );
		
		interpolate( target->local.rotation, iRotations, time, lRotate );
		interpolate( target->local.translation, iTranslations, time, lTrans );
		interpolate( target->local.scale, iScales, time, lScale );
	}
	
	bool update( const NifModel * nif, const QModelIndex & index )
	{
		if ( Controller::update( nif, index ) )
		{
			iTranslations = nif->getIndex( iData, "Translations" );
			iRotations = nif->getIndex( iData, "Rotations" );
			if ( ! iRotations.isValid() )
				iRotations = iData;
			iScales = nif->getIndex( iData, "Scales" );
			return true;
		}
		return false;
	}
	
protected:
	QPointer<Node> target;
	
	QPersistentModelIndex iTranslations, iRotations, iScales;
	
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
	
	bool update( const NifModel * nif, const QModelIndex & index )
	{
		if ( Controller::update( nif, index ) )
		{
			iKeys = nif->getIndex( iData, "Data" );
			return true;
		}
		return false;
	}
	
protected:
	QPointer<Node> target;
	
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

Controller * Node::findController( const QString & proptype, const QString & ctrltype, const QString & var1, const QString & var2 )
{
	if ( proptype != "<empty>" && ! proptype.isEmpty() )
	{
		foreach ( Property * prp, properties.list() )
		{
			if ( prp->typeId() == proptype )
			{
				return prp->findController( ctrltype, var1, var2 );
			}
		}
		return 0;
	}
	
	return Controllable::findController( ctrltype, var1, var2 );
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
		foreach ( qint32 l, nif->getLinkArray( iBlock, "Properties" ) )
			newProps.add( scene->getProperty( nif, nif->getBlock( l ) ) );
		properties = newProps;
		
		children.clear();
		QModelIndex iChildren = nif->getIndex( iBlock, "Children" );
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
	if ( cname == "NiTransformController" )
	{
		Controller * ctrl = new TransformController( this, iController );
		ctrl->update( nif, iController );
		controllers.append( ctrl );
	}
	else if ( cname == "NiMultiTargetTransformController" )
	{
		Controller * ctrl = new MultiTargetTransformController( this, iController );
		ctrl->update( nif, iController );
		controllers.append( ctrl );
	}
	else if ( cname == "NiControllerManager" )
	{
		Controller * ctrl = new ControllerManager( this, iController );
		ctrl->update( nif, iController );
		controllers.append( ctrl );
	}
	else if ( cname == "NiKeyframeController" )
	{
		Controller * ctrl = new KeyframeController( this, iController );
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

void Node::activeProperties( PropertyList & list ) const
{
	list.merge( properties );
	if ( parent )
		parent->activeProperties( list );
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

Node * Node::findChild( const QString & name ) const
{
	if ( this->name == name )
		return const_cast<Node*>( this );
	
	foreach ( Node * child, children.list() )
	{
		Node * n = child->findChild( name );
		if ( n )
			return n;
	}
	return 0;
}

bool Node::isHidden() const
{
	if ( GLOptions::drawHidden() )
		return false;
	
	if ( flags.node.hidden || ( parent && parent->isHidden() ) )
		return true;
	
	return ! GLOptions::cullExpression().isEmpty() && name.contains( GLOptions::cullExpression() );
}

void Node::transform()
{
	Controllable::transform();

	// if there's a rigid body attached, then calculate and cache the body's transform
	// (need this later in the drawing stage for the constraints)
	const NifModel * nif = static_cast<const NifModel *>( iBlock.model() );
	if ( iBlock.isValid() && nif )
	{
		QModelIndex iObject = nif->getBlock( nif->getLink( iBlock, "Collision Data" ) );
		if ( ! iObject.isValid() )
			iObject = nif->getBlock( nif->getLink( iBlock, "Collision Object" ) );
		if ( iObject.isValid() )
		{
			QModelIndex iBody = nif->getBlock( nif->getLink( iObject, "Body" ) );
			if ( iBody.isValid() )
			{
				Transform t;
				t.scale = 7;
				if ( nif->isNiBlock( iBody, "bhkRigidBodyT" ) )
				{
					t.rotation.fromQuat( nif->get<Quat>( iBody, "Rotation" ) );
					t.translation = nif->get<Vector3>( iBody, "Translation" ) * 7;
				}
				scene->bhkBodyTrans.insert( nif->getBlockNumber( iBody ), viewTrans() * t );
			}
		}
	}
	
	foreach ( Node * node, children.list() )
		node->transform();
}

void Node::transformShapes()
{
	foreach ( Node * node, children.list() )
		node->transformShapes();
}

void Node::draw()
{
	if ( isHidden() )
		return;
	
	glLoadName( nodeId );
	
	glEnable( GL_DEPTH_TEST );
	glDepthMask( GL_FALSE );
	glDisable( GL_TEXTURE_2D );
	glDisable( GL_NORMALIZE );
	glDisable( GL_LIGHTING );
	glDisable( GL_COLOR_MATERIAL );
	glDisable( GL_BLEND );
	glDisable( GL_ALPHA_TEST );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE );
	
	if ( scene->currentBlock == iBlock )
	{
		glDepthFunc( GL_ALWAYS );
		glHighlightColor();
	}
	else
	{
		glDepthFunc( GL_LEQUAL );
		glNormalColor();
	}
	
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
		node->draw();
}

void Node::drawSelection() const
{
}

void drawHvkShape( const NifModel * nif, const QModelIndex & iShape, QStack<QModelIndex> & stack )
{
	if ( ! nif || ! iShape.isValid() || stack.contains( iShape ) )
		return;
	
	stack.push( iShape );
	
	//qWarning() << "draw shape" << nif->getBlockNumber( iShape ) << nif->itemName( iShape );
	
	QString name = nif->itemName( iShape );
	if ( name == "bhkListShape" )
	{
		QModelIndex iShapes = nif->getIndex( iShape, "Sub Shapes" );
		if ( iShapes.isValid() )
		{
			for ( int r = 0; r < nif->rowCount( iShapes ); r++ )
			{
				drawHvkShape( nif, nif->getBlock( nif->getLink( iShapes.child( r, 0 ) ) ), stack );
			}
		}
	}
	else if ( name == "bhkTransformShape" || name == "bhkConvexTransformShape" )
	{
		glPushMatrix();
		glMultMatrix( nif->get<Matrix4>( iShape, "Transform" ) );
		drawHvkShape( nif, nif->getBlock( nif->getLink( iShape, "Shape" ) ), stack );
		glPopMatrix();
	}
	else if ( name == "bhkSphereShape" )
	{
		glLoadName( nif->getBlockNumber( iShape ) );
		drawSphere( Vector3(), nif->get<float>( iShape, "Radius" ) );
	}
	else if ( name == "bhkMultiSphereShape" )
	{
		glLoadName( nif->getBlockNumber( iShape ) );
		QModelIndex iSpheres = nif->getIndex( iShape, "Spheres" );
		for ( int r = 0; r < nif->rowCount( iSpheres ); r++ )
		{
			drawSphere( nif->get<Vector3>( iSpheres.child( r, 0 ), "Center" ), nif->get<float>( iSpheres.child( r, 0 ), "Radius" ) );
		}
	}
	else if ( name == "bhkBoxShape" )
	{
		glLoadName( nif->getBlockNumber( iShape ) );
		Vector3 v = nif->get<Vector3>( iShape, "Dimensions" );
		drawBox( v, - v );
	}
	else if ( name == "bhkCapsuleShape" )
	{
		glLoadName( nif->getBlockNumber( iShape ) );
		drawCapsule( nif->get<Vector3>( iShape, "First Point" ), nif->get<Vector3>( iShape, "Second Point" ), nif->get<float>( iShape, "Radius" ) );
	}
	else if ( name == "bhkNiTriStripsShape" )
	{
		float s = 1 / 7.0;
		glScalef( s, s, s );
		
		glLoadName( nif->getBlockNumber( iShape ) );
		
		QModelIndex iStrips = nif->getIndex( iShape, "Strips Data" );
		for ( int r = 0; r < nif->rowCount( iStrips ); r++ )
		{
			QModelIndex iStripData = nif->getBlock( nif->getLink( iStrips.child( r, 0 ) ), "NiTriStripsData" );
			if ( iStripData.isValid() )
			{
				QVector<Vector3> verts = nif->getArray<Vector3>( iStripData, "Vertices" );
				
				QList< QVector<quint16> > strips;
				QModelIndex iPoints = nif->getIndex( iStripData, "Points" );
				for ( int r = 0; r < nif->rowCount( iPoints ); r++ )
					strips += nif->getArray<quint16>( iPoints.child( r, 0 ) );
				
				glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
				glBegin( GL_TRIANGLES );
				foreach ( Triangle t, triangulate( strips ) )
				{
					glVertex( verts.value( t[0] ) );
					glVertex( verts.value( t[1] ) );
					glVertex( verts.value( t[2] ) );
				}
				glEnd();
				glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
			}
		}
	}
	else if ( name == "bhkConvexVerticesShape" )
	{
		glLoadName( nif->getBlockNumber( iShape ) );
		QVector< Vector4 > verts = nif->getArray<Vector4>( iShape, "Unknown Vectors 1" );
		glBegin( GL_LINES );
		for ( int i = 1; i < verts.count(); i++ )
		{
			for ( int j = 0; j < i; j++ )
			{
				glVertex( verts[i] );
				glVertex( verts[j] );
			}
		}
		glEnd();
		/* face normals ?
		verts = nif->getArray<Vector4>( iShape, "Unknown Vectors 2" );
		glBegin( GL_POINTS );
		foreach ( Vector4 v, verts )
		{
			Vector3 v3( v[0], v[1], v[2] );
			glVertex( v3 * v[3] );
		}
		glEnd();
		*/
	}
	else if ( name == "bhkMoppBvTreeShape" )
	{
		drawHvkShape( nif, nif->getBlock( nif->getLink( iShape, "Shape" ) ), stack );
	}
	else if ( name == "bhkPackedNiTriStripsShape" )
	{
		glLoadName( nif->getBlockNumber( iShape ) );
		
		QModelIndex iData = nif->getBlock( nif->getLink( iShape, "Data" ) );
		if ( iData.isValid() )
		{
			QVector<Vector3> verts = nif->getArray<Vector3>( iData, "Vertices" );
			QModelIndex iTris = nif->getIndex( iData, "Triangles" );
			for ( int t = 0; t < nif->rowCount( iTris ); t++ )
			{
				Triangle tri = nif->get<Triangle>( iTris.child( t, 0 ), "Triangle" );
				if ( tri[0] != tri[1] || tri[1] != tri[2] || tri[2] != tri[0] )
				{
					glBegin( GL_LINE_STRIP );
					glVertex( verts.value( tri[0] ) );
					glVertex( verts.value( tri[1] ) );
					glVertex( verts.value( tri[2] ) );
					glVertex( verts.value( tri[0] ) );
					glEnd();
				}
			}
		}
	}
	
	stack.pop();
}

void drawHvkConstraint( const NifModel * nif, const QModelIndex & iConstraint, const Scene * scene )
{
	if ( ! ( nif && iConstraint.isValid() && scene ) )
		return;
	
	QList<Transform> tBodies;
	QModelIndex iBodies = nif->getIndex( iConstraint, "Bodies" );
	for ( int r = 0; r < nif->rowCount( iBodies ); r++ )
	{
		qint32 l = nif->getLink( iBodies.child( r, 0 ) );
		if ( ! scene->bhkBodyTrans.contains( l ) )
			return;
		else
			tBodies.append( scene->bhkBodyTrans.value( l ) );
	}
	
	if ( tBodies.count() != 2 )
		return;
	
	glLoadName( nif->getBlockNumber( iConstraint ) );
	glPushMatrix();
	
	QString name = nif->itemName( iConstraint );
	if ( name == "bhkLimitedHingeConstraint" )
	{
		glDisable( GL_DEPTH_TEST );
		
		//glDepthFunc( GL_LEQUAL );
		
		QModelIndex iHinge = nif->getIndex( iConstraint, "Limited Hinge" );
		
		const Vector3 pivotA = Vector3( nif->get<Vector4>( iHinge, "Pivot A" ) );
		const Vector3 pivotB = Vector3( nif->get<Vector4>( iHinge, "Pivot B" ) );
		
		const Vector3 axleA( nif->get<Vector4>( iHinge, "Axle A" ) );
		const Vector3 axleA1( nif->get<Vector4>( iHinge, "Perp2AxleInA1" ) );
		const Vector3 axleA2( nif->get<Vector4>( iHinge, "Perp2AxleInA2" ) );
		
		const Vector3 axleB( nif->get<Vector4>( iHinge, "Axle B" ) );
		const Vector3 axleB2( nif->get<Vector4>( iHinge, "Perp2AxleInB2" ) );
		
		const float minAngle = nif->get<float>( iHinge, "Min Angle" );
		const float maxAngle = nif->get<float>( iHinge, "Max Angle" );
		
		glLoadMatrix( tBodies.value( 0 ) );
		glColor( Color3( 0.8, 0.6, 0.0 ) );
		
		glBegin( GL_POINTS );
		glVertex( pivotA );
		glEnd();
		
		glBegin( GL_LINES );
		glVertex( pivotA );
		glVertex( pivotA + axleA );
		glVertex( pivotA );
		glVertex( pivotA + axleA1 );
		//glVertex( pivotA );
		//glVertex( pivotA + axleA2 );
		glEnd();
		drawDashLine( pivotA, pivotA + axleA2 );
		
		drawCircle( pivotA, axleA, 1.0 );
		drawSolidArc( pivotA, axleA / 5, axleA2, axleA1, minAngle, maxAngle );
		
		glLoadMatrix( tBodies.value( 1 ) );
		glColor( Color3( 0.6, 0.8, 0.0 ) );
		
		glBegin( GL_POINTS );
		glVertex( pivotB );
		glEnd();
		
		glBegin( GL_LINES );
		glVertex( pivotB );
		glVertex( pivotB + axleB );
		glVertex( pivotB );
		glVertex( pivotB + axleB2 );
		//glVertex( pivotB );
		//glVertex( pivotB + Vector3::crossproduct( Vector3( nif->get<Vector4>( iHinge, "Axle B" ) ), Vector3( nif->get<Vector4>( iHinge, "Unknown Vector" ) ) ) );
		glEnd();
		
		
		drawCircle( pivotB, axleB, 1.0 );
		drawSolidArc( pivotB, axleB / 6, axleB2, Vector3::crossproduct( axleB2, axleB ), minAngle, maxAngle );
	}
	else if ( name == "bhkRagdollConstraint" )
	{
	}
	
	glPopMatrix();
}

void Node::drawHavok()
{
	foreach ( Node * node, children.list() )
		node->drawHavok();
	
	const NifModel * nif = static_cast<const NifModel *>( iBlock.model() );
	if ( ! ( iBlock.isValid() && nif ) )
		return;
	
	QModelIndex iObject = nif->getBlock( nif->getLink( iBlock, "Collision Data" ) );
	if ( ! iObject.isValid() )
		iObject = nif->getBlock( nif->getLink( iBlock, "Collision Object" ) );				
	if ( ! iObject.isValid() )
		return;
	
	QModelIndex iBody = nif->getBlock( nif->getLink( iObject, "Body" ) );
	
	glPushMatrix();
	glLoadMatrix( scene->bhkBodyTrans.value( nif->getBlockNumber( iBody ) ) );
	

	//qWarning() << "draw obj" << nif->getBlockNumber( iObject ) << nif->itemName( iObject );
	
	glEnable( GL_DEPTH_TEST );
	glDepthMask( GL_FALSE );
	glDepthFunc( GL_LEQUAL );
	glDisable( GL_TEXTURE_2D );
	glDisable( GL_NORMALIZE );
	glDisable( GL_LIGHTING );
	glDisable( GL_COLOR_MATERIAL );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glDisable( GL_ALPHA_TEST );
	glPointSize( 4.5 );
	glLineWidth( 1.0 );
	
	static const float colors[8][3] = { { 0.0, 1.0, 0.0 }, { 1.0, 0.0, 0.0 }, { 1.0, 0.0, 1.0 }, { 1.0, 1.0, 1.0 }, { 0.5, 0.5, 1.0 }, { 1.0, 0.8, 0.0 }, { 1.0, 0.8, 0.4 }, { 0.0, 1.0, 1.0 } };
	
	glColor3fv( colors[ nif->get<int>( iBody, "Layer" ) & 7 ] );

	QStack<QModelIndex> shapeStack;
	drawHvkShape( nif, nif->getBlock( nif->getLink( iBody, "Shape" ) ), shapeStack );
	
	glLoadName( nif->getBlockNumber( iBody ) );
	drawAxes( nif->get<Vector3>( iBody, "Center" ), 0.2 );
	
	glPopMatrix();
	
	foreach ( qint32 l, nif->getLinkArray( iBody, "Constraints" ) )
	{
		QModelIndex iConstraint = nif->getBlock( l );
		if ( nif->inherits( iConstraint, "AbhkConstraint" ) )
			drawHvkConstraint( nif, iConstraint, scene );
	}
}

void drawFurnitureMarker( const NifModel *nif, const QModelIndex &iPosition )
{
	QString name = nif->itemName( iPosition );
	Vector3 offs = nif->get<Vector3>( iPosition, "Offset" );
	quint16 orient = nif->get<quint16>( iPosition, "Orientation" );
	quint8 ref1 = nif->get<quint8>( iPosition, "Position Ref 1" );
	quint8 ref2 = nif->get<quint8>( iPosition, "Position Ref 2" );

	if ( ref1 != ref2 )
	{
		qDebug() << "Position Ref 1 and 2 are not equal!";
		return;
	}

	Vector3 flip(1, 1, 1);
	const FurnitureMarker *mark;
	switch ( ref1 )
	{
		case 1:
			mark = &FurnitureMarker01;
			break;

		case 2:
			flip[0] = -1;
			mark = &FurnitureMarker01;
			break;
			
		case 3:
			mark = &FurnitureMarker03;
			break;

		case 4:
			mark = &FurnitureMarker04;
			break;

		case 11:
			mark = &FurnitureMarker11;
			break;

		case 12:
			flip[0] = -1;
			mark = &FurnitureMarker11;
			break;
			
		case 13:
			mark = &FurnitureMarker13;
			break;

		case 14:
			mark = &FurnitureMarker14;
			break;

		default:
			qDebug() << "Unknown furniture marker " << ref1 << "!";
			return;
	}
	
	float roll = float( orient ) / 6284.0 * 2.0 * (-M_PI);

	glLoadName( ( nif->getBlockNumber( iPosition ) & 0xffff ) | ( ( iPosition.row() & 0xffff ) << 16 ) );
	
	glPushMatrix();

	Transform t;
	t.rotation.fromEuler( 0, 0, roll );
	t.translation = offs;
	glMultMatrix( t );
	
	glScale(flip);

	glEnableClientState( GL_VERTEX_ARRAY );
	glVertexPointer( 3, GL_FLOAT, 0, mark->verts );

	glDrawElements( GL_TRIANGLES, mark->nf * 3, GL_UNSIGNED_SHORT, mark->faces );

	glDisableClientState( GL_VERTEX_ARRAY );
	
	glPopMatrix();
}

void Node::drawFurn()
{
	foreach ( Node * node, children.list() )
		node->drawFurn();

	const NifModel * nif = static_cast<const NifModel *>( iBlock.model() );
	if ( ! ( iBlock.isValid() && nif ) )
		return;

	QModelIndex iExtraDataList = nif->getIndex( iBlock, "Extra Data List" );

	if ( !iExtraDataList.isValid() )
		return;

	glEnable( GL_DEPTH_TEST );
	glDepthMask( GL_FALSE );
	glDepthFunc( GL_LEQUAL );
	glDisable( GL_TEXTURE_2D );
	glDisable( GL_NORMALIZE );
	glDisable( GL_LIGHTING );
	glDisable( GL_COLOR_MATERIAL );
	glDisable( GL_CULL_FACE );
	glDisable( GL_BLEND );
	glDisable( GL_ALPHA_TEST );
	glColor4f( 1, 1, 1, 1 );
	
	glLineWidth( 1.0 );
	glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
	
	glPushMatrix();
	
	glMultMatrix( viewTrans() );
	
	for ( int p = 0; p < nif->rowCount( iExtraDataList ); p++ )
	{
		QModelIndex iFurnMark = nif->getBlock( nif->getLink( iExtraDataList.child( p, 0 ) ), "BSFurnitureMarker" );
		if ( ! iFurnMark.isValid() )
			continue;
	
		QModelIndex iPositions = nif->getIndex( iFurnMark, "Positions" );		
		if ( !iPositions.isValid() )
			break;
			
		for ( int j = 0; j < nif->rowCount( iPositions ); j++ )
		{
			QModelIndex iPosition = iPositions.child( j, 0 );
			
			if ( scene->currentIndex == iPosition )
				glHighlightColor();
			else
				glNormalColor();
			
			drawFurnitureMarker( nif, iPosition );		
		}
	}

	glPopMatrix();
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

#define Farg( X ) arg( X, 0, 'f', 5 )

QString trans2string( Transform t )
{
	float xr, yr, zr;
	t.rotation.toEuler( xr, yr, zr );
	return	QString( "translation  X %1, Y %2, Z %3\n" ).Farg( t.translation[0] ).Farg( t.translation[1] ).Farg( t.translation[2] )
		+	QString( "rotation     Y %1, P %2, R %3  " ).Farg( xr * 180 / PI ).Farg( yr * 180 / PI ).Farg( zr * 180 / PI )
		+	QString( "( (%1, %2, %3), " ).Farg( t.rotation( 0, 0 ) ).Farg( t.rotation( 0, 1 ) ).Farg( t.rotation( 0, 2 ) )
		+	QString( "(%1, %2, %3), " ).Farg( t.rotation( 1, 0 ) ).Farg( t.rotation( 1, 1 ) ).Farg( t.rotation( 1, 2 ) )
		+	QString( "(%1, %2, %3) )\n" ).Farg( t.rotation( 2, 0 ) ).Farg( t.rotation( 2, 1 ) ).Farg( t.rotation( 2, 2 ) )
		+	QString( "scale        %1\n" ).Farg( t.scale );
}

QString Node::textStats() const
{
	return QString( "%1\n\nglobal\n%2\nlocal\n%3\n" ).arg( name ).arg( trans2string( worldTrans() ) ).arg( trans2string( localTrans() ) );
}

BoundSphere Node::bounds() const
{
	if ( GLOptions::drawNodes() )
		return BoundSphere( worldTrans().translation, 0 );
	else
		return BoundSphere();
}


LODNode::LODNode( Scene * scene, const QModelIndex & iBlock )
	: Node( scene, iBlock )
{
}

void LODNode::clear()
{
	Node::clear();
	ranges.clear();
}

void LODNode::update( const NifModel * nif, const QModelIndex & index )
{
	Node::update( nif, index );
	
	if ( ( iBlock.isValid() && index == iBlock ) || ( iData.isValid() && index == iData ) )
	{
		ranges.clear();
		iData = nif->getBlock( nif->getLink( iBlock, "LOD Level Data" ), "NiRangeLODData" );
		QModelIndex iLevels;
		if ( iData.isValid() )
		{
			center = nif->get<Vector3>( iData, "LOD Center" );
			iLevels = nif->getIndex( iData, "LOD Levels" );
		}
		else
		{
			center = nif->get<Vector3>( iBlock, "LOD Center" );
			iLevels = nif->getIndex( iBlock, "LOD Levels" );
		}
		if ( iLevels.isValid() )
			for ( int r = 0; r < nif->rowCount( iLevels ); r++ )
				ranges.append( qMakePair<float,float>( nif->get<float>( iLevels.child( r, 0 ), "Near Extent" ), nif->get<float>( iLevels.child( r, 0 ), "Far Extent" ) ) );
	}
}

void LODNode::transform()
{
	Node::transform();
	
	if ( children.list().isEmpty() )
		return;
		
	if ( ranges.isEmpty() )
	{
		foreach ( Node * child, children.list() )
			child->flags.node.hidden = true;
		children.list().first()->flags.node.hidden = false;
		return;
	}
	
	float distance = ( viewTrans() * center ).length();

	int c = 0;
	foreach ( Node * child, children.list() )
	{
		if ( c < ranges.count() )
			child->flags.node.hidden = ! ( ranges[c].first <= distance && distance < ranges[c].second );
		else
			child->flags.node.hidden = true;
		c++;
	}
}


BillboardNode::BillboardNode( Scene * scene, const QModelIndex & iBlock )
	: Node( scene, iBlock )
{
}

const Transform & BillboardNode::viewTrans() const
{
	if ( scene->viewTrans.contains( nodeId ) )
		return scene->viewTrans[ nodeId ];
	
	Transform t;
	if ( parent )
		t = parent->viewTrans() * local;
	else
		t = scene->view * worldTrans();
	
	t.rotation = Matrix();
	
	scene->viewTrans.insert( nodeId, t );
	return scene->viewTrans[ nodeId ];
}
