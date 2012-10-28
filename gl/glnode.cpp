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

#include "glscene.h"
#include "glmarker.h"
#include "glnode.h"
int Node::SELECTING = 0;
#include "glcontroller.h"
#include "../options.h"

#include "../NvTriStrip/qtwrapper.h"

#include "marker/furniture.h"
#include "marker/constraints.h"

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
		if ( ! ( active && target ) )
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
		if ( ! ( active && target ) )
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
						{
							scene->animGroups.append( name );
							
							QMap<QString,float> tags = scene->animTags[ name ];
							
							QModelIndex iKeys = nif->getBlock( nif->getLink( iSeq, "Text Keys" ), "NiTextKeyExtraData" );
							QModelIndex iTags = nif->getIndex( iKeys, "Text Keys" );
							for ( int r = 0; r < nif->rowCount( iTags ); r++ )
							{
								tags.insert( nif->get<QString>( iTags.child( r, 0 ), "Value" ), nif->get<float>( iTags.child( r, 0 ), "Time" ) );
							}
							
							scene->animTags[ name ] = tags;
						}
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
						
						QString nodename = nif->get<QString>( iCB, "Node Name" );
						if ( nodename.isEmpty() ) {
							QModelIndex idx = nif->getIndex( iCB, "Node Name Offset" );
							nodename = idx.sibling( idx.row(), NifModel::ValueCol ).data( NifSkopeDisplayRole ).toString();
						}
						QString proptype = nif->get<QString>( iCB, "Property Type" );
						if ( proptype.isEmpty() ) {
							QModelIndex idx = nif->getIndex( iCB, "Property Type Offset" );
							proptype = idx.sibling( idx.row(), NifModel::ValueCol ).data( NifSkopeDisplayRole ).toString();
						}
						QString ctrltype = nif->get<QString>( iCB, "Controller Type" );
						if ( ctrltype.isEmpty() ) {
							QModelIndex idx = nif->getIndex( iCB, "Controller Type Offset" );
							ctrltype = idx.sibling( idx.row(), NifModel::ValueCol ).data( NifSkopeDisplayRole ).toString();
						}
						QString var1 = nif->get<QString>( iCB, "Variable 1" );
						if ( var1.isEmpty() ) {
							QModelIndex idx = nif->getIndex( iCB, "Variable 1 Offset" );
							var1 = idx.sibling( idx.row(), NifModel::ValueCol ).data( NifSkopeDisplayRole ).toString();
						}
						QString var2 = nif->get<QString>( iCB, "Variable 2" );
						if ( var2.isEmpty() ) {
							QModelIndex idx = nif->getIndex( iCB, "Variable 2 Offset" );
							var2 = idx.sibling( idx.row(), NifModel::ValueCol ).data( NifSkopeDisplayRole ).toString();
						}
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
		if ( ! ( active && target ) )
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
		if ( ! ( active && target ) )
			return;
		
		time = ctrlTime( time );
		
		bool isVisible;
		if ( interpolate( isVisible, iData, time, visLast ) )
		{
			target->flags.node.hidden = ! isVisible;
		}
	}
	
	bool update( const NifModel * nif, const QModelIndex & index )
	{
		if ( Controller::update( nif, index ) )
		{
			// iData already points to the NiVisData
			// note that nif.xml needs to have "Keys" not "Vis Keys" for interpolate() to work
			//iKeys = nif->getIndex( iData, "Data" );
			return true;
		}
		return false;
	}
	
protected:
	QPointer<Node> target;
	
	//QPersistentModelIndex iKeys;
	
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
			if ( Property * p = scene->getProperty( nif, nif->getBlock( l ) ) )
				newProps.add( p );
		foreach ( qint32 l, nif->getLinkArray( iBlock, "BS Properties" ) )
			if ( Property * p = scene->getProperty( nif, nif->getBlock( l ) ) )
				newProps.add( p );
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
	if ( Options::drawHidden() )
		return false;
	
	if ( flags.node.hidden || ( parent && parent->isHidden() ) )
		return true;
	
	return ! Options::cullExpression().isEmpty() && name.contains( Options::cullExpression() );
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
					t.translation = Vector3( nif->get<Vector4>( iBody, "Translation" ) * 7 );
				}
				scene->bhkBodyTrans.insert( nif->getBlockNumber( iBody ), worldTrans() * t );
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

	//glLoadName( nodeId ); - disabled glRenderMode( GL_SELECT );
	if (Node::SELECTING) {
		int s_nodeId = ID2COLORKEY( nodeId );
		glColor4ubv( (GLubyte *)&s_nodeId );
		glLineWidth( 5 ); // make hitting a line a litlle bit more easy
	}
	else {
		glEnable( GL_DEPTH_TEST );
		glDepthFunc( GL_LEQUAL );
		glDepthMask( GL_TRUE );
		glDisable( GL_TEXTURE_2D );
		glDisable( GL_NORMALIZE );
		glDisable( GL_LIGHTING );
		glDisable( GL_COLOR_MATERIAL );
		glEnable( GL_BLEND );
		glDisable( GL_ALPHA_TEST );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

		glNormalColor();
		glLineWidth( 2.5 );
	}

	glPointSize( 8.5 );
	
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
	if ( scene->currentBlock != iBlock || ! Options::drawNodes() )
		return;
	
	//glLoadName( nodeId ); - disabled glRenderMode( GL_SELECT );
	if (Node::SELECTING) {
		int s_nodeId = ID2COLORKEY( nodeId );
		glColor4ubv( (GLubyte *)&s_nodeId );
		glLineWidth( 5 );
	}
	else {
		glEnable( GL_DEPTH_TEST );
		glDepthFunc( GL_ALWAYS );
		glDepthMask( GL_TRUE );
		glDisable( GL_TEXTURE_2D );
		glDisable( GL_NORMALIZE );
		glDisable( GL_LIGHTING );
		glDisable( GL_COLOR_MATERIAL );
		glEnable( GL_BLEND );
		glDisable( GL_ALPHA_TEST );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	
		glHighlightColor();
		glLineWidth( 2.5 );
	}

	glPointSize( 8.5 );
	
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
}

void DrawVertexSelection( QVector<Vector3> &verts, int i )
{
	glPointSize( 3.5 );
	glDepthFunc( GL_LEQUAL );
	glNormalColor();
	glBegin( GL_POINTS );
	for ( int j = 0; j < verts.count(); j++ )
		glVertex( verts.value( j ) );
	glEnd();

	if ( i >= 0 )
	{
		glDepthFunc( GL_ALWAYS );
		glHighlightColor();
		glBegin( GL_POINTS );
		glVertex( verts.value( i ) );
		glEnd();
	}
}

void DrawTriangleSelection( QVector<Vector3> const &verts, Triangle const &tri )
{
	glLineWidth( 1.5f );
	glDepthFunc( GL_ALWAYS );
	glHighlightColor();
	glBegin( GL_LINE_STRIP );
	glVertex( verts.value( tri.v1() ) );
	glVertex( verts.value( tri.v2() ) );
	glVertex( verts.value( tri.v3() ) );
	glVertex( verts.value( tri.v1() ) );
	glEnd();
}

void DrawTriangleIndex( QVector<Vector3> const &verts, Triangle const &tri, int index)
{
	Vector3 c = ( verts.value( tri.v1() ) + verts.value( tri.v2() ) + verts.value( tri.v3() ) ) /  3.0;
	renderText(c, QString("%1").arg(index));
}

void drawHvkShape( const NifModel * nif, const QModelIndex & iShape, QStack<QModelIndex> & stack, const Scene * scene, const float origin_color3fv[3] )
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
				if ( !Node::SELECTING ) {
					if ( scene->currentBlock == nif->getBlock( nif->getLink( iShapes.child( r, 0 ) ) ) ) {// fix: add selected visual to havok meshes
						glHighlightColor();
						glLineWidth( 2.5 );
					}
					else {
						if ( scene->currentBlock != iShape) {// allow group highlighting
							glLineWidth( 1.0 );
							glColor3fv( origin_color3fv );
						}
					}
				}
				drawHvkShape( nif, nif->getBlock( nif->getLink( iShapes.child( r, 0 ) ) ), stack, scene, origin_color3fv );
			}
		}
	}
	else if ( name == "bhkTransformShape" || name == "bhkConvexTransformShape" )
	{
		glPushMatrix();
		Matrix4 tm = nif->get<Matrix4>( iShape, "Transform" );
		// TODO find a better way to apply tm
		Transform t;
		Vector3 s;
		tm.decompose(t.translation, t.rotation, s);
		t.scale = (s[0] + s[1] + s[2]) / 3.0; // assume uniform
		glMultMatrix( t );
		drawHvkShape( nif, nif->getBlock( nif->getLink( iShape, "Shape" ) ), stack, scene, origin_color3fv );
		glPopMatrix();
	}
	else if ( name == "bhkSphereShape" )
	{
		//glLoadName( nif->getBlockNumber( iShape ) );
		if (Node::SELECTING) {
			int s_nodeId = ID2COLORKEY( nif->getBlockNumber( iShape ) );
			glColor4ubv( (GLubyte *)&s_nodeId );
		}
		drawSphere( Vector3(), nif->get<float>( iShape, "Radius" ) );
	}
	else if ( name == "bhkMultiSphereShape" )
	{
		//glLoadName( nif->getBlockNumber( iShape ) );
		if (Node::SELECTING) {
			int s_nodeId = ID2COLORKEY( nif->getBlockNumber( iShape ) );
			glColor4ubv( (GLubyte *)&s_nodeId );
		}
		QModelIndex iSpheres = nif->getIndex( iShape, "Spheres" );
		for ( int r = 0; r < nif->rowCount( iSpheres ); r++ )
		{
			drawSphere( nif->get<Vector3>( iSpheres.child( r, 0 ), "Center" ), nif->get<float>( iSpheres.child( r, 0 ), "Radius" ) );
		}
	}
	else if ( name == "bhkBoxShape" )
	{
		//glLoadName( nif->getBlockNumber( iShape ) );
		if (Node::SELECTING) {
			int s_nodeId = ID2COLORKEY( nif->getBlockNumber( iShape ) );
			glColor4ubv( (GLubyte *)&s_nodeId );
		}
		Vector3 v = nif->get<Vector3>( iShape, "Dimensions" );
		drawBox( v, - v );
	}
	else if ( name == "bhkCapsuleShape" )
	{
		//glLoadName( nif->getBlockNumber( iShape ) );
		if (Node::SELECTING) {
			int s_nodeId = ID2COLORKEY( nif->getBlockNumber( iShape ) );
			glColor4ubv( (GLubyte *)&s_nodeId );
		}
		drawCapsule( nif->get<Vector3>( iShape, "First Point" ), nif->get<Vector3>( iShape, "Second Point" ), nif->get<float>( iShape, "Radius" ) );
	}
	else if ( name == "bhkNiTriStripsShape" )
	{
		glPushMatrix();
		float s = 1.0f / 7.0f;
		glScalef( s, s, s );
		
		//glLoadName( nif->getBlockNumber( iShape ) );
		if (Node::SELECTING) {
			int s_nodeId = ID2COLORKEY( nif->getBlockNumber( iShape ) );
			glColor4ubv( (GLubyte *)&s_nodeId );
		}
		
		QModelIndex iStrips = nif->getIndex( iShape, "Strips Data" );
		for ( int r = 0; r < nif->rowCount( iStrips ); r++ )
		{
			QModelIndex iStripData = nif->getBlock( nif->getLink( iStrips.child( r, 0 ) ), "NiTriStripsData" );
			if ( iStripData.isValid() )
			{
				QVector<Vector3> verts = nif->getArray<Vector3>( iStripData, "Vertices" );
				
				glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
				glBegin( GL_TRIANGLES );
				
				QModelIndex iPoints = nif->getIndex( iStripData, "Points" );
				for ( int r = 0; r < nif->rowCount( iPoints ); r++ )
				{	// draw the strips like they appear in the tescs
					// (use the unstich strips spell to avoid the spider web effect)
					QVector<quint16> strip = nif->getArray<quint16>( iPoints.child( r, 0 ) );
					if ( strip.count() >= 3 )
					{
						quint16 a = strip[0];
						quint16 b = strip[1];
						
						for ( int x = 2; x < strip.size(); x++ )
						{
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
				glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
			}
		}
		glPopMatrix();
	}
	else if ( name == "bhkConvexVerticesShape" )
	{
		//glLoadName( nif->getBlockNumber( iShape ) );
		if (Node::SELECTING) {
			int s_nodeId = ID2COLORKEY( nif->getBlockNumber( iShape ) );
			glColor4ubv( (GLubyte *)&s_nodeId );
		}
		drawConvexHull( nif->getArray<Vector4>( iShape, "Vertices" ), nif->getArray<Vector4>( iShape, "Normals" ) );
	}
	else if ( name == "bhkMoppBvTreeShape" )
	{
		if ( !Node::SELECTING ) {
			if ( scene->currentBlock == nif->getBlock( nif->getLink( iShape, "Shape" ) ) ) {// fix: add selected visual to havok meshes
				glHighlightColor();
				glLineWidth( 1.5f );// taken from "DrawTriangleSelection"
			}
			else {
				glLineWidth( 1.0 );
				glColor3fv( origin_color3fv );
			}
		}
		drawHvkShape( nif, nif->getBlock( nif->getLink( iShape, "Shape" ) ), stack, scene, origin_color3fv );
	}
	else if ( name == "bhkPackedNiTriStripsShape" )
	{
		//glLoadName( nif->getBlockNumber( iShape ) );
		if (Node::SELECTING) {
			int s_nodeId = ID2COLORKEY( nif->getBlockNumber( iShape ) );
			glColor4ubv( (GLubyte *)&s_nodeId );
		}
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

			// Handle Selection of hkPackedNiTriStripsData
			if (scene->currentBlock == iData )
			{
				int i = -1;
				QString n = scene->currentIndex.data( NifSkopeDisplayRole ).toString();
				QModelIndex iParent = scene->currentIndex.parent();
				if ( iParent.isValid() && iParent != iData )
				{
					n = iParent.data( NifSkopeDisplayRole ).toString();
					i = scene->currentIndex.row();
				}
				if ( n == "Vertices" || n == "Normals" || n == "Vertex Colors" || n == "UV Sets" )
					DrawVertexSelection(verts, i);
				else if ( ( n == "Faces" || n == "Triangles" ) )
				{
					if ( i == -1 )
					{
						glDepthFunc( GL_ALWAYS );
						glHighlightColor();
						for ( int t = 0; t < nif->rowCount( iTris ); t++ )
							DrawTriangleIndex(verts, nif->get<Triangle>( iTris.child( t, 0 ), "Triangle" ), t);
					}
					else if ( nif->isCompound( nif->getBlockType( scene->currentIndex ) ) )
					{
						Triangle tri = nif->get<Triangle>( iTris.child( i, 0 ), "Triangle" );
						DrawTriangleSelection(verts, tri );
						DrawTriangleIndex(verts, tri, i);
					}
					else if ( nif->getBlockName( scene->currentIndex ) == "Normal" )
					{
						Triangle tri = nif->get<Triangle>( scene->currentIndex.parent(), "Triangle" );
						Vector3 triCentre = ( verts.value( tri.v1() ) + verts.value( tri.v2() ) + verts.value( tri.v3() ) ) /  3.0;
						glLineWidth( 1.5f );
						glDepthFunc( GL_ALWAYS );
						glHighlightColor();
						glBegin( GL_LINES );
						glVertex( triCentre );
						glVertex( triCentre + nif->get<Vector3>( scene->currentIndex ) );
						glEnd();
					}
				}
			}
			// Handle Selection of bhkPackedNiTriStripsShape
			else if ( scene->currentBlock == iShape )
			{
				int i = -1;
				QString n = scene->currentIndex.data( NifSkopeDisplayRole ).toString();
				QModelIndex iParent = scene->currentIndex.parent();
				if ( iParent.isValid() && iParent != iShape )
				{
					n = iParent.data( NifSkopeDisplayRole ).toString();
					i = scene->currentIndex.row();
				}
				//qDebug() << n;
				// n == "Sub Shapes" if the array is selected and if an element of the array is selected
				// iParent != iShape only for the elements of the array
				if (( n == "Sub Shapes" ) && ( iParent != iShape )) {
					// get subshape vertex indices
					QModelIndex iSubShapes = iParent;
					QModelIndex iSubShape = scene->currentIndex;
					int start_vertex = 0;
					int end_vertex = 0;
					for (int subshape = 0; subshape < nif->rowCount(iSubShapes); subshape++) {
						QModelIndex iCurrentSubShape = iSubShapes.child(subshape, 0);
						int num_vertices = nif->get<int>( iCurrentSubShape, "Num Vertices" );
						//qDebug() << num_vertices;
						end_vertex += num_vertices;
						if ( iCurrentSubShape == iSubShape ) {
							break;
						} else {
							start_vertex += num_vertices;
						}
					}
					// highlight the triangles of the subshape
					for ( int t = 0; t < nif->rowCount( iTris ); t++ ) {
						Triangle tri = nif->get<Triangle>( iTris.child( t, 0 ), "Triangle" );
						if ((start_vertex <= tri[0]) && (tri[0] < end_vertex)) {
							if ((start_vertex <= tri[1]) && (tri[1] < end_vertex) && (start_vertex <= tri[2]) && (tri[2] < end_vertex)) {
								DrawTriangleSelection(verts, tri );
								DrawTriangleIndex(verts, tri, t);
							} else {
								qWarning() << "triangle with multiple materials?" << t;
							}
						}
					}
				}
			}
		}
	}
	
	stack.pop();
}

void drawHvkConstraint( const NifModel * nif, const QModelIndex & iConstraint, const Scene * scene )
{
	if ( ! ( nif && iConstraint.isValid() && scene && Options::drawConstraints() ) )
		return;
	
	QList<Transform> tBodies;
	QModelIndex iBodies = nif->getIndex( iConstraint, "Entities" );
	if( !iBodies.isValid() ) {
		return;
	}

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
	
	Color3 color_a( 0.8f, 0.6f, 0.0f );
	Color3 color_b( 0.6f, 0.8f, 0.0f );
	//glLoadName( nif->getBlockNumber( iConstraint ) );
	if (Node::SELECTING) {
		int s_nodeId = ID2COLORKEY( nif->getBlockNumber( iConstraint ) );
		glColor4ubv( (GLubyte *)&s_nodeId );
		glLineWidth( 5 ); // make hitting a line a litlle bit more easy
	}
	else {
		if ( scene->currentBlock == nif->getBlock( iConstraint ) ) {// fix: add selected visual to havok meshes
			glHighlightColor();
			color_a.fromQColor( Options::hlColor() );
			color_b.setRGB( Options::hlColor().blueF(),  Options::hlColor().redF(), Options::hlColor().greenF());
		}
	}
	
	glPushMatrix();
	glLoadMatrix( scene->view );
	
	glPushAttrib( GL_ENABLE_BIT );
	glEnable( GL_DEPTH_TEST );
	
	QString name = nif->itemName( iConstraint );
	if ( name == "bhkMalleableConstraint" )
	{
		if ( nif->getIndex( iConstraint, "Ragdoll" ).isValid() )
		{
			name = "bhkRagdollConstraint";
		}
		else if ( nif->getIndex( iConstraint, "Limited Hinge" ).isValid() )
		{
			name = "bhkLimitedHingeConstraint";
		}
	}
	
	if ( name == "bhkLimitedHingeConstraint" )
	{
		QModelIndex iHinge = nif->getIndex( iConstraint, "Limited Hinge" );
		
		const Vector3 pivotA( nif->get<Vector4>( iHinge, "Pivot A" ) );
		const Vector3 pivotB( nif->get<Vector4>( iHinge, "Pivot B" ) );
		
		const Vector3 axleA( nif->get<Vector4>( iHinge, "Axle A" ) );
		const Vector3 axleA1( nif->get<Vector4>( iHinge, "Perp2 Axle In A1" ) );
		const Vector3 axleA2( nif->get<Vector4>( iHinge, "Perp2 Axle In A2" ) );
		
		const Vector3 axleB( nif->get<Vector4>( iHinge, "Axle B" ) );
		const Vector3 axleB2( nif->get<Vector4>( iHinge, "Perp2 Axle In B2" ) );
		
		const float minAngle = nif->get<float>( iHinge, "Min Angle" );
		const float maxAngle = nif->get<float>( iHinge, "Max Angle" );
		
		glPushMatrix();
		glMultMatrix( tBodies.value( 0 ) );
		if (!Node::SELECTING)
			glColor( color_a );
		glBegin( GL_POINTS ); glVertex( pivotA ); glEnd();
		glBegin( GL_LINES ); glVertex( pivotA ); glVertex( pivotA + axleA ); glEnd();
		drawDashLine( pivotA, pivotA + axleA1, 14 );
		drawDashLine( pivotA, pivotA + axleA2, 14 );
		drawCircle( pivotA, axleA, 1.0f );
		drawSolidArc( pivotA, axleA / 5, axleA2, axleA1, minAngle, maxAngle, 1.0f );
		glPopMatrix();
		
		glPushMatrix();
		glMultMatrix( tBodies.value( 1 ) );
		if (!Node::SELECTING)
			glColor( color_b );
		glBegin( GL_POINTS ); glVertex( pivotB ); glEnd();
		glBegin( GL_LINES ); glVertex( pivotB ); glVertex( pivotB + axleB ); glEnd();
		drawDashLine( pivotB + axleB2, pivotB, 14 );
		drawDashLine( pivotB + Vector3::crossproduct( axleB2, axleB ), pivotB, 14 );
		drawCircle( pivotB, axleB, 1.01f );
		drawSolidArc( pivotB, axleB / 7, axleB2, Vector3::crossproduct( axleB2, axleB ), minAngle, maxAngle, 1.01f );
		glPopMatrix();
		
		glMultMatrix( tBodies.value( 0 ) );
		float angle = Vector3::angle( tBodies.value( 0 ).rotation * axleA2, tBodies.value( 1 ).rotation * axleB2 );
		if (!Node::SELECTING)
			glColor( color_a );
		glBegin( GL_LINES );
		glVertex( pivotA );
		glVertex( pivotA + axleA1 * cosf( angle ) + axleA2 * sinf( angle ) );
		glEnd();
	}
	else if ( name == "bhkHingeConstraint" )
	{
		QModelIndex iHinge = nif->getIndex( iConstraint, "Hinge" );
		
		const Vector3 pivotA( nif->get<Vector4>( iHinge, "Pivot A" ) );
		const Vector3 pivotB( nif->get<Vector4>( iHinge, "Pivot B" ) );
		
		const Vector3 axleA1( nif->get<Vector4>( iHinge, "Perp2 Axle In A1" ) );
		const Vector3 axleA2( nif->get<Vector4>( iHinge, "Perp2 Axle In A2" ) );
		const Vector3 axleA( Vector3::crossproduct( axleA1, axleA2 ) );
		
		const Vector3 axleB( nif->get<Vector4>( iHinge, "Axle B" ) );
		
		const Vector3 axleB1( axleB[1], axleB[2], axleB[0] );
		const Vector3 axleB2( Vector3::crossproduct( axleB, axleB1 ) );
		
		/*
		 * This should be correct but is visually strange...
		 *
		Vector3 axleB1temp;
		Vector3 axleB2temp;
		
		if ( nif->checkVersion( 0, 0x14000002 ) )
		{
			Vector3 axleB1temp( axleB[1], axleB[2], axleB[0] );
			Vector3 axleB2temp( Vector3::crossproduct( axleB, axleB1temp ) );
		}
		else if ( nif->checkVersion( 0x14020007, 0 ) )
		{
			Vector3 axleB1temp( nif->get<Vector4>( iHinge, "Perp2 Axle In B1" ) );
			Vector3 axleB2temp( nif->get<Vector4>( iHinge, "Perp2 Axle In B2" ) );
		}
		
		const Vector3 axleB1( axleB1temp );
		const Vector3 axleB2( axleB2temp );
		*/
		
		const float minAngle = - PI;
		const float maxAngle = + PI;
		
		glPushMatrix();
		glMultMatrix( tBodies.value( 0 ) );
		if (!Node::SELECTING)
			glColor( color_a );
		glBegin( GL_POINTS ); glVertex( pivotA ); glEnd();
		drawDashLine( pivotA, pivotA + axleA1 );
		drawDashLine( pivotA, pivotA + axleA2 );
		drawSolidArc( pivotA, axleA / 5, axleA2, axleA1, minAngle, maxAngle, 1.0f, 16 );
		glPopMatrix();
		
		glMultMatrix( tBodies.value( 1 ) );
		if (!Node::SELECTING)
			glColor( color_b );
		glBegin( GL_POINTS ); glVertex( pivotB ); glEnd();
		glBegin( GL_LINES ); glVertex( pivotB ); glVertex( pivotB + axleB ); glEnd();
		drawSolidArc( pivotB, axleB / 7, axleB2, axleB1, minAngle, maxAngle, 1.01f, 16 );
	}
	else if ( name == "bhkStiffSpringConstraint" )
	{
		const Vector3 pivotA = tBodies.value( 0 ) * Vector3( nif->get<Vector4>( iConstraint, "Pivot A" ) );
		const Vector3 pivotB = tBodies.value( 1 ) * Vector3( nif->get<Vector4>( iConstraint, "Pivot B" ) );
		const float length = nif->get<float>( iConstraint, "Length" );
		
		if (!Node::SELECTING)
			glColor( color_b );
		
		drawSpring( pivotA, pivotB, length );
	}
	else if ( name == "bhkRagdollConstraint" )
	{
		QModelIndex iRagdoll = nif->getIndex( iConstraint, "Ragdoll" );
		
		const Vector3 pivotA( nif->get<Vector4>( iRagdoll, "Pivot A" ) );
		const Vector3 pivotB( nif->get<Vector4>( iRagdoll, "Pivot B" ) );
		
		const Vector3 planeA( nif->get<Vector4>( iRagdoll, "Plane A" ) );
		const Vector3 planeB( nif->get<Vector4>( iRagdoll, "Plane B" ) );
		
		const Vector3 twistA( nif->get<Vector4>( iRagdoll, "Twist A" ) );
		const Vector3 twistB( nif->get<Vector4>( iRagdoll, "Twist B" ) );
		
		const float coneAngle( nif->get<float>( iRagdoll, "Cone Max Angle" ) );
		
		const float minPlaneAngle( nif->get<float>( iRagdoll, "Plane Min Angle" ) );
		const float maxPlaneAngle( nif->get<float>( iRagdoll, "Plane Max Angle" ) );
		
		// Unused? GCC complains
		/*
		const float minTwistAngle( nif->get<float>( iRagdoll, "Twist Min Angle" ) );
		const float maxTwistAngle( nif->get<float>( iRagdoll, "Twist Max Angle" ) );
		*/
		
		glPushMatrix();
		glMultMatrix( tBodies.value( 0 ) );
		if (!Node::SELECTING)
			glColor( color_a );
		glPopMatrix();
		
		glPushMatrix();
		glMultMatrix( tBodies.value( 0 ) );
		if (!Node::SELECTING)
			glColor( color_a );
		glBegin( GL_POINTS ); glVertex( pivotA ); glEnd();
		glBegin( GL_LINES ); glVertex( pivotA ); glVertex( pivotA + twistA ); glEnd();
		drawDashLine( pivotA, pivotA + planeA, 14 );
		drawRagdollCone( pivotA, twistA, planeA, coneAngle, minPlaneAngle, maxPlaneAngle );
		glPopMatrix();
		
		glPushMatrix();
		glMultMatrix( tBodies.value( 1 ) );
		if (!Node::SELECTING)
			glColor( color_b );
		glBegin( GL_POINTS ); glVertex( pivotB ); glEnd();
		glBegin( GL_LINES ); glVertex( pivotB ); glVertex( pivotB + twistB ); glEnd();
		drawDashLine( pivotB + planeB, pivotB, 14 );
		drawRagdollCone( pivotB, twistB, planeB, coneAngle, minPlaneAngle, maxPlaneAngle );
		glPopMatrix();
	}
	else if ( name == "bhkPrismaticConstraint" )
	{
		const Vector3 pivotA( nif->get<Vector4>( iConstraint, "Pivot A" ) );
		const Vector3 pivotB( nif->get<Vector4>( iConstraint, "Pivot B" ) );

		const Vector3 planeNormal( nif->get<Vector4>( iConstraint, "Plane" ) );
		const Vector3 slidingAxis( nif->get<Vector4>( iConstraint, "Sliding Axis" ) );

		const float minDistance = nif->get<float>( iConstraint, "Min Distance" );
		const float maxDistance = nif->get<float>( iConstraint, "Max Distance" );

		const Vector3 d1 = pivotA + slidingAxis * minDistance;
		const Vector3 d2 = pivotA + slidingAxis * maxDistance;

		/* draw Pivot A and Plane */
		glPushMatrix();
		glMultMatrix( tBodies.value( 0 ) );
		if (!Node::SELECTING)
			glColor( color_a );
		glBegin( GL_POINTS ); glVertex( pivotA ); glEnd();
		glBegin( GL_LINES ); glVertex( pivotA ); glVertex( pivotA + planeNormal ); glEnd();
		drawDashLine( pivotA, d1, 14 );

		/* draw rail */
		if ( minDistance < maxDistance ) {
			drawRail( d1, d2 );
		}

		/*draw first marker*/
		Transform t;
		float angle = atan2f( slidingAxis[1], slidingAxis[0] );
		if ( slidingAxis[0] < 0.0001f && slidingAxis[1] < 0.0001f ) {
			angle = PI / 2;
		}

		t.translation = d1;
		t.rotation.fromEuler( 0.0f, 0.0f, angle );
		glMultMatrix( t );

		angle = - asinf( slidingAxis[2] / slidingAxis.length() );
		t.translation = Vector3( 0.0f, 0.0f, 0.0f );
		t.rotation.fromEuler( 0.0f, angle, 0.0f );
		glMultMatrix( t );
		
		drawMarker( &BumperMarker01 );

		/*draw second marker*/
		t.translation = Vector3( minDistance < maxDistance ? ( d2 - d1 ).length() : 0.0f, 0.0f, 0.0f );
		t.rotation.fromEuler( 0.0f, 0.0f, PI );
		glMultMatrix( t );

		drawMarker( &BumperMarker01 );
		glPopMatrix();

		/* draw Pivot B */
		glPushMatrix();
		glMultMatrix( tBodies.value( 1 ) );
		if (!Node::SELECTING)
			glColor( color_b );
		glBegin( GL_POINTS ); glVertex( pivotB ); glEnd();
		glPopMatrix();
	}
	
	glPopAttrib();
	glPopMatrix();
}

void Node::drawHavok()
{// TODO: Why are all these here - "drawNodes", "drawFurn", "drawHavok"?
 // Idea: Make them go to their own classes in different cpp files
	foreach ( Node * node, children.list() )
		node->drawHavok();
	
	const NifModel * nif = static_cast<const NifModel *>( iBlock.model() );
	if ( ! ( iBlock.isValid() && nif ) )
		return;

	//Check if there's any old style collision bounding box set
	if ( nif->get<bool>( iBlock, "Has Bounding Box" ) == true )
	{
		QModelIndex iBox = nif->getIndex( iBlock, "Bounding Box" );
		
		Transform bt;
		
		bt.translation = nif->get<Vector3>( iBox, "Translation" );
		bt.rotation = nif->get<Matrix>( iBox, "Rotation" );
		bt.scale = 1.0f;
		
		Vector3 rad = nif->get<Vector3>( iBox, "Radius" );
		
		glPushMatrix();
		glLoadMatrix( scene->view );
		// The Morrowind construction set seems to completely ignore the node transform
		//glMultMatrix( worldTrans() );
		glMultMatrix( bt );
		
		//glLoadName( nodeId );
		if (Node::SELECTING) {
			int s_nodeId = ID2COLORKEY( nodeId );
			glColor4ubv( (GLubyte *)&s_nodeId );
		}
		else {
			glColor( Color3( 1.0f, 0.0f, 0.0f ) );
			glDisable( GL_LIGHTING );
		}
		glLineWidth( 1.0f );
		drawBox( rad, -rad );
		
		glPopMatrix();
	}
	
	// Draw BSBound dimensions
	QModelIndex iExtraDataList = nif->getIndex( iBlock, "Extra Data List" );
	
	if ( iExtraDataList.isValid() )
	{
		for ( int d = 0; d < nif->rowCount( iExtraDataList ); d++ )
		{
			QModelIndex iBound = nif->getBlock( nif->getLink( iExtraDataList.child( d, 0 ) ), "BSBound" );
			if ( ! iBound.isValid() )
				continue;
			
			Vector3 center = nif->get<Vector3>( iBound, "Center" );
			Vector3 dim = nif->get<Vector3>( iBound, "Dimensions" );
			
			glPushMatrix();
			glLoadMatrix( scene->view );
			// Not sure if world transform is taken into account
			glMultMatrix( worldTrans() );
			
			//glLoadName( nif->getBlockNumber( iBound ) );
			if (Node::SELECTING) {
				int s_nodeId = ID2COLORKEY( nif->getBlockNumber( iBound ) );
				glColor4ubv( (GLubyte *)&s_nodeId );
			}
			else {
				glColor( Color3( 1.0f, 0.0f, 0.0f ) );
				glDisable( GL_LIGHTING );
			}
			glLineWidth( 1.0f );
			drawBox( dim + center, -dim + center );

			glPopMatrix();

		}
	}
	
	QModelIndex iObject = nif->getBlock( nif->getLink( iBlock, "Collision Data" ) );
	if ( ! iObject.isValid() )
		iObject = nif->getBlock( nif->getLink( iBlock, "Collision Object" ) );
	if ( ! iObject.isValid() )
		return;
	
	QModelIndex iBody = nif->getBlock( nif->getLink( iObject, "Body" ) );
	
	glPushMatrix();
	glLoadMatrix( scene->view );
	glMultMatrix( scene->bhkBodyTrans.value( nif->getBlockNumber( iBody ) ) );
	

	//qWarning() << "draw obj" << nif->getBlockNumber( iObject ) << nif->itemName( iObject );

	if (!Node::SELECTING) {
		glEnable( GL_DEPTH_TEST );
		glDepthMask( GL_TRUE );
		glDepthFunc( GL_LEQUAL );
		glDisable( GL_TEXTURE_2D );
		glDisable( GL_NORMALIZE );
		glDisable( GL_LIGHTING );
		glDisable( GL_COLOR_MATERIAL );
		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		glDisable( GL_ALPHA_TEST );
	}
	glPointSize( 4.5 );
	glLineWidth( 1.0 );
	
	static const float colors[8][3] = {
		{ 0.0f, 1.0f, 0.0f },
		{ 1.0f, 0.0f, 0.0f },
		{ 1.0f, 0.0f, 1.0f },
		{ 1.0f, 1.0f, 1.0f },
		{ 0.5f, 0.5f, 1.0f },
		{ 1.0f, 0.8f, 0.0f },
		{ 1.0f, 0.8f, 0.4f },
		{ 0.0f, 1.0f, 1.0f }
	};
	
	int color_index = nif->get<int>( iBody, "Layer" ) & 7;
	glColor3fv( colors[ color_index ] );
	if ( !Node::SELECTING )
		if ( scene->currentBlock == nif->getBlock( nif->getLink( iBody, "Shape" ) ) ) {// fix: add selected visual to havok meshes
			glHighlightColor(); // TODO: idea: I do not recommend mimicking the Open GL API
								// It confuses the one who reads the code. And the Open GL API is
								// in constant development.
			glLineWidth( 2.5 );
			//glPointSize( 8.5 );
		}

	QStack<QModelIndex> shapeStack;
	if (Node::SELECTING)
		glLineWidth( 5 );// make selection click a little more easy
	drawHvkShape( nif, nif->getBlock( nif->getLink( iBody, "Shape" ) ), shapeStack, scene, colors[ color_index ] );

	//glLoadName( nif->getBlockNumber( iBody ) );
	if (Node::SELECTING) {
		int s_nodeId = ID2COLORKEY (nif->getBlockNumber( iBody ) );
		glColor4ubv( (GLubyte *)&s_nodeId );
		glDepthFunc( GL_ALWAYS );
		drawAxes( Vector3( nif->get<Vector4>( iBody, "Center" ) ), 0.2f );
		glDepthFunc( GL_LEQUAL );
	}
	else {
		drawAxes( Vector3( nif->get<Vector4>( iBody, "Center" ) ), 0.2f );
	}
	
	glPopMatrix();
	
	foreach ( qint32 l, nif->getLinkArray( iBody, "Constraints" ) )
	{
		QModelIndex iConstraint = nif->getBlock( l );
		if ( nif->inherits( iConstraint, "bhkConstraint" ) )
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
	const GLMarker *mark;
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

	//glLoadName( ( nif->getBlockNumber( iPosition ) & 0xffff ) | ( ( iPosition.row() & 0xffff ) << 16 ) ); - disabled glRenderMode( GL_SELECT );
	if (Node::SELECTING) {// TODO: not tested! need nif files what contain that
		GLint id = ( nif->getBlockNumber( iPosition ) & 0xffff ) | ( ( iPosition.row() & 0xffff ) << 16 );
		int s_nodeId = ID2COLORKEY( id );
		glColor4ubv ( (GLubyte *)&s_nodeId);
	}
	
	glPushMatrix();

	Transform t;
	t.rotation.fromEuler( 0, 0, roll );
	t.translation = offs;
	glMultMatrix( t );
	
	glScale(flip);

	drawMarker( mark );
	
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

	if (!Node::SELECTING) {
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
		glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
	}
	glLineWidth( 1.0 );
	
	glPushMatrix();
	
	glMultMatrix( viewTrans() );
	
	for ( int p = 0; p < nif->rowCount( iExtraDataList ); p++ )
	{// DONE: never seen Furn in nifs, so there may be a need of a fix here later - saw one, fixed a bug
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
	BoundSphere boundsphere;
	// the node itself
	if ( Options::drawNodes() || Options::drawHavok() ) {
		boundsphere |= BoundSphere( worldTrans().translation, 0 );
	}

	const NifModel * nif = static_cast<const NifModel *>( iBlock.model() );
	if ( ! ( iBlock.isValid() && nif ) )
		return boundsphere;

	// old style collision bounding box
	if ( nif->get<bool>( iBlock, "Has Bounding Box" ) == true )
	{
		QModelIndex iBox = nif->getIndex( iBlock, "Bounding Box" );
		Vector3 trans = nif->get<Vector3>( iBox, "Translation" );
		Vector3 rad = nif->get<Vector3>( iBox, "Radius" );
		boundsphere |= BoundSphere(trans, rad.length());
	}
	
	// BSBound collision bounding box
	QModelIndex iExtraDataList = nif->getIndex( iBlock, "Extra Data List" );
	
	if ( iExtraDataList.isValid() )
	{
		for ( int d = 0; d < nif->rowCount( iExtraDataList ); d++ )
		{
			QModelIndex iBound = nif->getBlock( nif->getLink( iExtraDataList.child( d, 0 ) ), "BSBound" );
			if ( ! iBound.isValid() )
				continue;
			
			Vector3 center = nif->get<Vector3>( iBound, "Center" );
			Vector3 dim = nif->get<Vector3>( iBound, "Dimensions" );
			boundsphere |= BoundSphere(center, dim.length());
		}
	}
	return boundsphere;
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
