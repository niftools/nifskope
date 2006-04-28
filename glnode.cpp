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
		node->draw( draw2nd );
}


void drawAxes( Vector3 c, float axis )
{
	glPushMatrix();
	glTranslate( c );
	GLfloat arrow = axis / 12.0;
	glBegin( GL_LINES );
	glColor3f( 1.0, 0.0, 0.0 );
	glVertex3f( - axis, 0, 0 );
	glVertex3f( + axis, 0, 0 );
	glVertex3f( + axis, 0, 0 );
	glVertex3f( + axis - arrow, + arrow, 0 );
	glVertex3f( + axis, 0, 0 );
	glVertex3f( + axis - arrow, - arrow, 0 );
	glVertex3f( + axis, 0, 0 );
	glVertex3f( + axis - arrow, 0, + arrow );
	glVertex3f( + axis, 0, 0 );
	glVertex3f( + axis - arrow, 0, - arrow );
	glColor3f( 0.0, 1.0, 0.0 );
	glVertex3f( 0, - axis, 0 );
	glVertex3f( 0, + axis, 0 );
	glVertex3f( 0, + axis, 0 );
	glVertex3f( + arrow, + axis - arrow, 0 );
	glVertex3f( 0, + axis, 0 );
	glVertex3f( - arrow, + axis - arrow, 0 );
	glVertex3f( 0, + axis, 0 );
	glVertex3f( 0, + axis - arrow, + arrow );
	glVertex3f( 0, + axis, 0 );
	glVertex3f( 0, + axis - arrow, - arrow );
	glColor3f( 0.0, 0.0, 1.0 );
	glVertex3f( 0, 0, - axis );
	glVertex3f( 0, 0, + axis );
	glVertex3f( 0, 0, + axis );
	glVertex3f( 0, + arrow, + axis - arrow );
	glVertex3f( 0, 0, + axis );
	glVertex3f( 0, - arrow, + axis - arrow );
	glVertex3f( 0, 0, + axis );
	glVertex3f( + arrow, 0, + axis - arrow );
	glVertex3f( 0, 0, + axis );
	glVertex3f( - arrow, 0, + axis - arrow );
	glEnd();
	glPopMatrix();
}

void drawBox( Vector3 a, Vector3 b )
{
	glBegin( GL_LINE_STRIP );
	glVertex3f( a[0], a[1], a[2] );
	glVertex3f( a[0], b[1], a[2] );
	glVertex3f( a[0], b[1], b[2] );
	glVertex3f( a[0], a[1], b[2] );
	glVertex3f( a[0], a[1], a[2] );
	glEnd();
	glBegin( GL_LINE_STRIP );
	glVertex3f( b[0], a[1], a[2] );
	glVertex3f( b[0], b[1], a[2] );
	glVertex3f( b[0], b[1], b[2] );
	glVertex3f( b[0], a[1], b[2] );
	glVertex3f( b[0], a[1], a[2] );
	glEnd();
	glBegin( GL_LINES );
	glVertex3f( a[0], a[1], a[2] );
	glVertex3f( b[0], a[1], a[2] );
	glVertex3f( a[0], b[1], a[2] );
	glVertex3f( b[0], b[1], a[2] );
	glVertex3f( a[0], b[1], b[2] );
	glVertex3f( b[0], b[1], b[2] );
	glVertex3f( a[0], a[1], b[2] );
	glVertex3f( b[0], a[1], b[2] );
	glEnd();
}

void drawSphere( Vector3 c, float r, int sd = 8 )
{
	glBegin( GL_POINTS );
	glVertex( c );
	glEnd();
	
	for ( int j = -sd; j <= sd; j++ )
	{
		float f = PI * float( j ) / float( sd );
		Vector3 cj = c + Vector3( 0, 0, r * cos( f ) );
		float rj = r * sin( f );
		
		glBegin( GL_LINE_STRIP );
		for ( int i = 0; i <= sd*2; i++ )
			glVertex( Vector3( sin( PI / sd * i ), cos( PI / sd * i ), 0 ) * rj + cj );
		glEnd();
	}
	for ( int j = -sd; j <= sd; j++ )
	{
		float f = PI * float( j ) / float( sd );
		Vector3 cj = c + Vector3( 0, r * cos( f ), 0 );
		float rj = r * sin( f );
		
		glBegin( GL_LINE_STRIP );
		for ( int i = 0; i <= sd*2; i++ )
			glVertex( Vector3( sin( PI / sd * i ), 0, cos( PI / sd * i ) ) * rj + cj );
		glEnd();
	}
	for ( int j = -sd; j <= sd; j++ )
	{
		float f = PI * float( j ) / float( sd );
		Vector3 cj = c + Vector3( r * cos( f ), 0, 0 );
		float rj = r * sin( f );
		
		glBegin( GL_LINE_STRIP );
		for ( int i = 0; i <= sd*2; i++ )
			glVertex( Vector3( 0, sin( PI / sd * i ), cos( PI / sd * i ) ) * rj + cj );
		glEnd();
	}
}

void drawCapsule( Vector3 a, Vector3 b, float r, int sd = 5 )
{
	Vector3 d = b - a;
	if ( d.length() < 0.001 )
	{
		drawSphere( a, r );
		return;
	}
	
	Vector3 n = d;
	n.normalize();
	
	Vector3 x( n[1], n[2], n[0] );
	Vector3 y = Vector3::crossproduct( n, x );
	x = Vector3::crossproduct( n, y );
	
	x *= r;
	y *= r;
	
	glBegin( GL_LINE_STRIP );
	for ( int i = 0; i <= sd*2; i++ )
		glVertex( a + x * sin( PI / sd * i ) + y * cos( PI / sd * i ) );
	glEnd();
	glBegin( GL_LINE_STRIP );
	for ( int i = 0; i <= sd*2; i++ )
		glVertex( a + d/2 + x * sin( PI / sd * i ) + y * cos( PI / sd * i ) );
	glEnd();
	glBegin( GL_LINE_STRIP );
	for ( int i = 0; i <= sd*2; i++ )
		glVertex( b + x * sin( PI / sd * i ) + y * cos( PI / sd * i ) );
	glEnd();
	glBegin( GL_LINES );
	for ( int i = 0; i <= sd*2; i++ )
	{
		glVertex( a + x * sin( PI / sd * i ) + y * cos( PI / sd * i ) );
		glVertex( b + x * sin( PI / sd * i ) + y * cos( PI / sd * i ) );
	}
	glEnd();
	for ( int j = 0; j <= sd; j++ )
	{
		float f = PI * float( j ) / float( sd * 2 );
		Vector3 dj = n * r * cos( f );
		float rj = sin( f );
		
		glBegin( GL_LINE_STRIP );
		for ( int i = 0; i <= sd*2; i++ )
			glVertex( a - dj + x * sin( PI / sd * i ) * rj + y * cos( PI / sd * i ) * rj );
		glEnd();
		glBegin( GL_LINE_STRIP );
		for ( int i = 0; i <= sd*2; i++ )
			glVertex( b + dj + x * sin( PI / sd * i ) * rj + y * cos( PI / sd * i ) * rj );
		glEnd();
	}
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
		nif->get<Matrix4>( iShape, "Transform" ).glMultMatrix();
		drawHvkShape( nif, nif->getBlock( nif->getLink( iShape, "Sub Shape" ) ), stack );
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
	
	viewTrans().glLoadMatrix();
	
	if ( nif->itemName( iBody ) == "bhkRigidBodyT" )
	{
		Transform t;
		t.rotation.fromQuat( nif->get<Quat>( iBody, "Rotation" ) );
		t.translation = nif->get<Vector3>( iBody, "Translation" ) * 7;
		t.glMultMatrix();
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


BoundSphere Node::bounds() const
{
	if ( scene->showNodes )
		return BoundSphere( worldTrans().translation, 0 );
	else
		return BoundSphere();
}

BoundSphere::BoundSphere()
{
	radius	= -1;
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
		radius	= -1;
	}
	else
	{
		center	= Vector3();
		foreach ( Vector3 v, verts )
		{
			center += v;
		}
		center /= verts.count();
		
		radius	= 0;
		foreach ( Vector3 v, verts )
		{
			float d = ( center - v ).squaredLength();
			if ( d > radius )
				radius = d;
		}
		radius = sqrt( radius );
	}
}

BoundSphere & BoundSphere::operator=( const BoundSphere & o )
{
	center	= o.center;
	radius	= o.radius;
	return *this;
}

BoundSphere & BoundSphere::operator|=( const BoundSphere & o )
{
	if ( o.radius < 0 )
		return *this;
	if ( radius < 0 )
		return operator=( o );
	
	float d = ( center - o.center ).length();
	
	if ( radius >= d + o.radius )
		return * this;
	if ( o.radius >= d + radius )
		return operator=( o );
	
	if ( o.radius > radius ) radius = o.radius;
	radius += d / 2;
	center = ( center + o.center ) / 2;
	
	return *this;
}

BoundSphere BoundSphere::operator|( const BoundSphere & other )
{
	BoundSphere b( *this );
	b |= other;
	return b;
}

BoundSphere & BoundSphere::apply( const Transform & t )
{
	center = t * center;
	radius *= fabs( t.scale );
	return *this;
}

BoundSphere & BoundSphere::applyInv( const Transform & t )
{
	center = t.rotation.inverted() * ( center - t.translation ) / t.scale;
	radius /= fabs( t.scale );
	return *this;
}

BoundSphere operator*( const Transform & t, const BoundSphere & sphere )
{
	BoundSphere bs( sphere );
	return bs.apply( t );
}

