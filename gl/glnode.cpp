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

#include "NvTriStrip/qtwrapper.h"

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
	
	return ! scene->expCull.isEmpty() && name.contains( scene->expCull );
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
		glColor( Color3( scene->hlcolor ) );
	else
		glColor( Color4( scene->hlcolor ).blend( 0.3 ) );
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
		node->draw( draw2nd );
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
			iShapes = nif->getIndex( iShapes, "Indices" );
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
	else if ( name == "bhkBoxShape" )
	{
		glLoadName( nif->getBlockNumber( iShape ) );
		Vector3 v = nif->get<Vector3>( iShape, "Unknown Vector" );
		drawBox( v, - v );
	}
	else if ( name == "bhkCapsuleShape" )
	{
		glLoadName( nif->getBlockNumber( iShape ) );
		drawCapsule( nif->get<Vector3>( iShape, "Unknown Vector 1" ), nif->get<Vector3>( iShape, "Unknown Vector 2" ), nif->get<float>( iShape, "Radius" ) );
	}
	else if ( name == "bhkNiTriStripsShape" )
	{
		float s = 1 / 7.0;
		glScalef( s, s, s );
		
		glLoadName( nif->getBlockNumber( iShape ) );
		
		QModelIndex iStrips = nif->getIndex( nif->getIndex( iShape, "Strips" ), "Indices" );
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
				
				glBegin( GL_TRIANGLES );
				foreach ( Triangle t, triangulate( strips ) )
				{
					glVertex( verts.value( t[0] ) );
					glVertex( verts.value( t[1] ) );
					glVertex( verts.value( t[2] ) );
				}
				glEnd();
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

void Node::drawHavok()
{
	foreach ( Node * node, children.list() )
		node->drawHavok();
	
	const NifModel * nif = static_cast<const NifModel *>( iBlock.model() );
	if ( ! ( iBlock.isValid() && nif ) )
		return;
	
	QModelIndex iObject = nif->getBlock( nif->getLink( iBlock, "Collision Data" ) );
	if ( !iObject.isValid() )
		return;

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
	
	QModelIndex iBody = nif->getBlock( nif->getLink( iObject, "Body" ) );
	glColor3fv( colors[ nif->get<int>( iBody, "Flags" ) & 7 ] );

	glPushMatrix();
	
	glLoadMatrix( viewTrans() );
	
	if ( nif->itemName( iBody ) == "bhkRigidBodyT" )
	{
		Transform t;
		t.rotation.fromQuat( nif->get<Quat>( iBody, "Rotation" ) );
		t.translation = nif->get<Vector3>( iBody, "Translation" ) * 7;
		glMultMatrix( t );
	}
	
	float s = 7;
	glScalef( s, s, s );
	
	glDisable( GL_CULL_FACE );
	glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
	
	QStack<QModelIndex> shapeStack;
	drawHvkShape( nif, nif->getBlock( nif->getLink( iBody, "Shape" ) ), shapeStack );
	
	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	glEnable( GL_CULL_FACE );
	
	glLoadName( nif->getBlockNumber( iBody ) );
	drawAxes( nif->get<Vector3>( iBody, "Center" ), 0.2 );
	
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

void Node::setupRenderState( bool vertexcolors )
{
	// setup lighting
	
	scene->setupLights( this );
	
	// setup blending
	
	glProperty( findProperty< AlphaProperty >() );
	
	// setup vertex colors
	
	glProperty( findProperty< VertexColorProperty >(), vertexcolors );
	
	// setup material
	
	glProperty( findProperty< MaterialProperty >(), findProperty< SpecularProperty >() );

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


QString trans2string( Transform t )
{
	float xr, yr, zr;
	t.rotation.toEuler( xr, yr, zr );
	return	QString( "translation  X %1, Y %2, Z %3\n" ).arg( t.translation[0] ).arg( t.translation[1] ).arg( t.translation[2] )
		+	QString( "rotation     Y %1, P %2, R %3  " ).arg( xr * 180 / PI ).arg( yr * 180 / PI ).arg( zr * 180 / PI )
		+	QString( "( (%1, %2, %3), " ).arg( t.rotation( 0, 0 ) ).arg( t.rotation( 0, 1 ) ).arg( t.rotation( 0, 2 ) )
		+	QString( "(%1, %2, %3), " ).arg( t.rotation( 1, 0 ) ).arg( t.rotation( 1, 1 ) ).arg( t.rotation( 1, 2 ) )
		+	QString( "(%1, %2, %3) )\n" ).arg( t.rotation( 2, 0 ) ).arg( t.rotation( 2, 1 ) ).arg( t.rotation( 2, 2 ) )
		+	QString( "scale        %1\n" ).arg( t.scale );
}

QString Node::textStats()
{
	return QString( "%1\n\nglobal\n%2\nlocal\n%3\n" ).arg( name ).arg( trans2string( worldTrans() ) ).arg( trans2string( localTrans() ) );
}


BoundSphere Node::bounds() const
{
	if ( scene->showNodes )
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
		iData = QModelIndex();
		
		QModelIndex iLOD = nif->getIndex( iBlock, "LOD Info" );
		if ( iLOD.isValid() )
		{
			QModelIndex iLevels;
			switch ( nif->get<int>( iLOD, "LOD Type" ) )
			{
				case 0:
					center = nif->get<Vector3>( iLOD, "LOD Center" );
					iLevels = nif->getIndex( iLOD, "LOD Levels" );
					break;
				case 1:
					iData = nif->getBlock( nif->getLink( iLOD, "Range Data" ), "NiRangeLODData" );
					if ( iData.isValid() )
					{
						center = nif->get<Vector3>( iLOD, "LOD Center" );
						iLevels = nif->getIndex( iData, "LOD Levels" );
					}
					break;
			}
			if ( iLevels.isValid() )
				for ( int r = 0; r < nif->rowCount( iLevels ); r++ )
					ranges.append( qMakePair<float,float>( nif->get<float>( iLevels.child( r, 0 ), "Near" ), nif->get<float>( iLevels.child( r, 0 ), "Far" ) ) );
		}
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
