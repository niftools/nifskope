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

#ifdef QT_OPENGL_LIB

#include "glscene.h"
#include "glcontroller.h"

#include "nifmodel.h"

inline void glVertex( const Vector3 & v )
{
	glVertex3fv( v.data() );
}

inline void glNormal( const Vector3 & v )
{
	glNormal3fv( v.data() );
}

inline void glTexCoord( const Vector2 & v )
{
	glTexCoord2fv( v.data() );
}

inline void glColor( const Color3 & c )
{
	glColor3fv( c.data() );
}

inline void glColor( const Color4 & c )
{
	glColor4fv( c.data() );
}

inline void glMaterial( GLenum x, GLenum y, const Color4 & c )
{
	glMaterialfv( x, y, c.data() );
}


Triangle::Triangle( const NifModel * nif, const QModelIndex & triangle )
{
	if ( !triangle.isValid() )		{	v1 = v2 = v3 = 0; return;	}
	v1 = nif->get<int>( triangle, "v1" );
	v2 = nif->get<int>( triangle, "v2" );
	v3 = nif->get<int>( triangle, "v3" );
}

Tristrip::Tristrip( const NifModel * nif, const QModelIndex & tristrip )
{
	if ( ! tristrip.isValid() ) return;
	
	for ( int s = 0; s < nif->rowCount( tristrip ); s++ )
		vertices.append( nif->itemData<int>( tristrip.child( s, 0 ) ) );
}

BoneWeights::BoneWeights( const NifModel * nif, const QModelIndex & index, int b )
{
	trans = Transform( nif, index );
	bone = b;
	
	QModelIndex idxWeights = nif->getIndex( index, "vertex weights" );
	if ( idxWeights.isValid() )
	{
		for ( int c = 0; c < nif->rowCount( idxWeights ); c++ )
		{
			QModelIndex idx = idxWeights.child( c, 0 );
			weights.append( VertexWeight( nif->get<int>( idx, "index" ), nif->get<float>( idx, "weight" ) ) );
		}
	}
	else
		qWarning() << nif->getBlockNumber( index ) << "vertex weights not found";
}


/*
 *  Controllable
 */
 
Controllable::Controllable( Scene * s, const QModelIndex & i ) : scene( s ), iBlock( i )
{
}

Controllable::~Controllable()
{
	qDeleteAll( controllers );
}

void Controllable::clear()
{
	qDeleteAll( controllers );
	controllers.clear();
}

bool Controllable::update( const QModelIndex & i )
{
	if ( iBlock == i || ! iBlock.isValid() )
		return true;
	foreach ( Controller * ctrl, controllers )
		if ( ctrl->update( i ) )
			return true;
	return false;
}

void Controllable::transform()
{
	if ( scene->animate )
		foreach ( Controller * controller, controllers )
			controller->update( scene->time );
}

void Controllable::timeBounds( float & tmin, float & tmax )
{
	if ( controllers.isEmpty() )
		return;
	
	float mn = controllers.first()->start;
	float mx = controllers.first()->stop;
	foreach ( Controller * c, controllers )
	{
		mn = qMin( mn, c->start );
		mx = qMax( mx, c->stop );
	}
	tmin = qMin( tmin, mn );
	tmax = qMax( tmax, mx );
}

/*
 *	Node
 */


Node::Node( Scene * s, Node * p, const QModelIndex & index ) : Controllable( s, index ), parent( p )
{
	nodeId = 0;
	flags.bits = 0;
	
	depthProp = false;
	depthTest = true;
	depthMask = true;
}

void Node::clear()
{
	Controllable::clear();
	
	blocks.clear();

	nodeId = 0;
	flags.bits = 0;
	
	depthProp = false;
	depthTest = true;
	depthMask = true;
}

bool Node::make()
{
	clear();
	
	if ( ! iBlock.isValid() )
		return false;
	
	const NifModel * nif = static_cast<const NifModel*>( iBlock.model() );
	
	if ( ! nif )
		return false;
	
	nodeId = nif->getBlockNumber( iBlock );

	flags.bits = nif->getInt( iBlock, "flags" ) & 1;

	local = Transform( nif, iBlock );
	worldDirty = true;
	
	foreach( int link, nif->getChildLinks( nodeId ) )
	{
		QModelIndex iChild = nif->getBlock( link );
		if ( ! iChild.isValid() ) continue;
		QString name = nif->itemName( iChild );
		
		if ( nif->inherits( name, "AProperty" ) )
			setProperty( nif, iChild );
		else if ( nif->inherits( name, "AController" ) )
		{
			do
			{
				setController( nif, iChild );
				iChild = nif->getBlock( nif->getInt( iChild, "next controller" ) );
			}
			while ( iChild.isValid() && nif->inherits( nif->itemName( iChild ), "AController" ) );
		}
		else
			setSpecial( nif, iChild );
	}
	return true;
}

bool Node::update( const QModelIndex & index )
{
	if ( Controllable::update( index ) )
		return true;
	
	foreach ( QPersistentModelIndex idx, blocks )
	{
		if ( idx == index || ! idx.isValid() )
			return true;
	}
	
	return false;
}

void Node::setController( const NifModel * nif, const QModelIndex & index )
{
	if ( nif->itemName( index ) == "NiKeyframeController" )
		controllers.append( new KeyframeController( this, nif, index ) );
	else if ( nif->itemName( index ) == "NiVisController" )
		controllers.append( new VisibilityController( this, nif, index ) );
}

void Node::setProperty( const NifModel * nif, const QModelIndex & property )
{
	QString propname = nif->itemName( property );
	if ( propname == "NiZBufferProperty" )
	{
		blocks.append( QPersistentModelIndex( property ) );
		int flags = nif->getInt( property, "flags" );
		depthTest = flags & 1;
		depthMask = flags & 2;
	}
}

void Node::setSpecial( const NifModel * nif, const QModelIndex & )
{
}

const Transform & Node::worldTrans()
{
	if ( worldDirty )
	{
		if ( parent )
			world = parent->worldTrans() * local;
		else
			world = local;
		worldDirty = false;
	}
	return world;
}

Transform Node::localTransFrom( int root )
{
	Transform trans;
	Node * node = this;
	while ( node && node->nodeId != root )
	{
		trans = node->local * trans;
		node = node->parent;
	}
	if ( node )
		return trans;
	else
		return Transform();
}

bool Node::isHidden() const
{
	if ( flags.node.hidden || ! parent )
		return flags.node.hidden;
	return parent->isHidden();
}

void Node::depthBuffer( bool & test, bool & mask )
{
	if ( depthProp || ! parent )
	{
		test = depthTest;
		mask = depthMask;
		return;
	}
	parent->depthBuffer( test, mask );
}

void Node::transform()
{
	Controllable::transform();
	worldDirty = true;
}

void Node::boundaries( Vector3 & min, Vector3 & max )
{
	min = max = worldTrans() * Vector3( 0.0, 0.0, 0.0 );
}

void Node::draw( bool selected )
{
	if ( isHidden() && ! scene->drawHidden )
		return;
	
	glLoadName( nodeId );
	
	glPushAttrib( GL_LIGHTING_BIT );
	glEnable( GL_DEPTH_TEST );
	glDepthMask( GL_TRUE );
	glDepthFunc( GL_ALWAYS );
	glDisable( GL_TEXTURE_2D );
	glDisable( GL_NORMALIZE );
	glDisable( GL_LIGHTING );
	glDisable( GL_COLOR_MATERIAL );
	if ( selected )
		glColor( Color4( QColor( "steelblue" ).light( 180 ) ) );
	else
		glColor( Color4( QColor( "steelblue" ).dark( 110 ) ) );
	glPointSize( 8.5 );
	glLineWidth( 2.5 );

	Vector3 a = scene->view * worldTrans() * Vector3();
	Vector3 b;
	if ( parent )
		b = scene->view * parent->worldTrans() * b;
	
	glBegin( GL_POINTS );
	glVertex( a );
	glEnd();

	glBegin( GL_LINES );
	glVertex( a );
	glVertex( b );
	glEnd();
	
	glPopAttrib();
}


/*
 *  Mesh
 */


Mesh::Mesh( Scene * s, Node * p, const QModelIndex & i ) : Node( s, p, i )
{
	shininess = 33.0;
	alpha = 1.0;
	texSet = 0;
	texFilter = GL_LINEAR;
	texWrapS = texWrapT = GL_REPEAT;
	texOffset[0] = texOffset[1] = 0.0;
	alphaEnable = false;
	alphaSrc = GL_SRC_ALPHA;
	alphaDst = GL_ONE_MINUS_SRC_ALPHA;
	specularEnable = false;
}

void Mesh::clear()
{
	Node::clear();

	shininess = 33.0;
	alpha = 1.0;
	texSet = 0;
	texFilter = GL_LINEAR;
	texWrapS = texWrapT = GL_REPEAT;
	texOffset[0] = texOffset[1] = 0.0;
	alphaEnable = false;
	alphaSrc = GL_SRC_ALPHA;
	alphaDst = GL_ONE_MINUS_SRC_ALPHA;
	specularEnable = false;
	
	verts.clear();
	norms.clear();
	colors.clear();
	uvs.clear();
	triangles.clear();
	tristrips.clear();
	transVerts.clear();
	transNorms.clear();
}

bool Mesh::make()
{
	bool b = Node::make();
	
	if ( ! specularEnable )
	{
		specular = Color4();
		specular.setAlpha( alpha );
	}
	
	return b;
}

void Mesh::setSpecial( const NifModel * nif, const QModelIndex & special )
{
	QString name = nif->itemName( special );
	if ( name == "NiTriShapeData" || name == "NiTriStripsData" )
	{
		blocks.append( QPersistentModelIndex( special ) );
		
		verts.clear();
		norms.clear();
		colors.clear();
		uvs.clear();
		triangles.clear();
		tristrips.clear();
		
		localCenter = nif->get<Vector3>( special, "center" );
		
		QModelIndex vertices = nif->getIndex( special, "vertices" );
		if ( vertices.isValid() )
			for ( int r = 0; r < nif->rowCount( vertices ); r++ )
				verts.append( nif->itemData<Vector3>( nif->index( r, 0, vertices ) ) );
		
		QModelIndex normals = nif->getIndex( special, "normals" );
		if ( normals.isValid() )
			for ( int r = 0; r < nif->rowCount( normals ); r++ )
				norms.append( nif->itemData<Vector3>( nif->index( r, 0, normals ) ) );
		
		QModelIndex vertexcolors = nif->getIndex( special, "vertex colors" );
		if ( vertexcolors.isValid() )
			for ( int r = 0; r < nif->rowCount( vertexcolors ); r++ )
				colors.append( nif->itemData<Color4>( vertexcolors.child( r, 0 ) ) );
		
		QModelIndex uvcoord = nif->getIndex( special, "uv sets" );
		if ( uvcoord.isValid() )
		{
			QModelIndex uvcoordset = nif->index( texSet, 0, uvcoord );
			if ( uvcoordset.isValid() )
				for ( int r = 0; r < nif->rowCount( uvcoordset ); r++ )
					uvs.append( nif->itemData<Vector2>( uvcoordset.child( r, 0 ) ) );
		}
		
		if ( nif->itemName( special ) == "NiTriShapeData" )
		{
			QModelIndex idxTriangles = nif->getIndex( special, "triangles" );
			if ( idxTriangles.isValid() )
			{
				for ( int r = 0; r < nif->rowCount( idxTriangles ); r++ )
					triangles.append( Triangle( nif, idxTriangles.child( r, 0 ) ) );
			}
			else
				qWarning() << nif->itemName( special ) << "(" << nif->getBlockNumber( special ) << ") triangle array not found";
		}
		else
		{
			QModelIndex points = nif->getIndex( special, "points" );
			if ( points.isValid() )
			{
				for ( int r = 0; r < nif->rowCount( points ); r++ )
					tristrips.append( Tristrip( nif, points.child( r, 0 ) ) );
			}
			else
				qWarning() << nif->itemName( special ) << "(" << nif->getBlockNumber( special ) << ") 'points' array not found";
		}
	}
	else if ( name == "NiSkinInstance" )
	{
		blocks.append( QPersistentModelIndex( special ) );
		
		weights.clear();
		
		int sdat = nif->getLink( special, "data" );
		QModelIndex skindata = nif->getBlock( sdat, "NiSkinData" );
		if ( ! skindata.isValid() )
		{
			qWarning() << "niskindata not found";
			return;
		}
		
		skelRoot = nif->getLink( special, "skeleton root" );
		skelTrans = Transform( nif, skindata );
		
		QVector<int> bones;
		QModelIndex idxBones = nif->getIndex( nif->getIndex( special, "bones" ), "bones" );
		if ( ! idxBones.isValid() )
		{
			qWarning() << "bones array not found";
			return;
		}
		
		for ( int b = 0; b < nif->rowCount( idxBones ); b++ )
			bones.append( nif->itemValue( nif->index( b, 0, idxBones ) ).toLink() );
		
		idxBones = nif->getIndex( skindata, "bone list" );
		if ( ! idxBones.isValid() )
		{
			qWarning() << "bone list not found";
			return;
		}
		
		for ( int b = 0; b < nif->rowCount( idxBones ) && b < bones.count(); b++ )
		{
			weights.append( BoneWeights( nif, idxBones.child( b, 0 ), bones[ b ] ) );
		}
	}
	else
		Node::setSpecial( nif, special );
}

void Mesh::setProperty( const NifModel * nif, const QModelIndex & property )
{
	QString propname = nif->itemName( property );
	if ( propname == "NiMaterialProperty" )
	{
		blocks.append( QPersistentModelIndex( property ) );
		
		alpha = nif->get<float>( property, "alpha" );
		if ( alpha < 0.0 ) alpha = 0.0;
		if ( alpha > 1.0 ) alpha = 1.0;

		ambient = Color4( nif->get<Color3>( property, "ambient color" ) );
		diffuse = Color4( nif->get<Color3>( property, "diffuse color" ) );
		specular = Color4( nif->get<Color3>( property, "specular color" ) );
		emissive = Color4( nif->get<Color3>( property, "emissive color" ) );
		
		shininess = nif->get<float>( property, "glossiness" ) * 1.28; // range 0 ~ 128 (nif 0~100)
		
		foreach( int link, nif->getChildLinks( nif->getBlockNumber( property ) ) )
		{
			QModelIndex block = nif->getBlock( link );
			if ( block.isValid() && nif->inherits( nif->itemName( block ), "AController" ) )
				setController( nif, block );
		}
	}
	else if ( propname == "NiTexturingProperty" )
	{
		blocks.append( QPersistentModelIndex( property ) );
		
		QModelIndex basetex = nif->getIndex( property, "base texture" );
		if ( ! basetex.isValid() )	return;
		QModelIndex basetexdata = nif->getIndex( basetex, "texture data" );
		if ( ! basetexdata.isValid() )	return;
		
		switch ( nif->get<int>( basetexdata, "filter mode" ) )
		{
			case 0:		texFilter = GL_NEAREST;		break;
			case 1:		texFilter = GL_LINEAR;		break;
			default:	texFilter = GL_NEAREST;		break;
			/*
			case 2:		texFilter = GL_NEAREST_MIPMAP_NEAREST;		break;
			case 3:		texFilter = GL_LINEAR_MIPMAP_NEAREST;		break;
			case 4:		texFilter = GL_NEAREST_MIPMAP_LINEAR;		break;
			case 5:		texFilter = GL_LINEAR_MIPMAP_LINEAR;		break;
			*/
		}
		switch ( nif->get<int>( basetexdata, "clamp mode" ) )
		{
			case 0:		texWrapS = GL_CLAMP;	texWrapT = GL_CLAMP;	break;
			case 1:		texWrapS = GL_CLAMP;	texWrapT = GL_REPEAT;	break;
			case 2:		texWrapS = GL_REPEAT;	texWrapT = GL_CLAMP;	break;
			default:	texWrapS = GL_REPEAT;	texWrapT = GL_REPEAT;	break;
		}
		texSet = nif->get<int>( basetexdata, "texture set" );
		
		iBaseTex = nif->getBlock( nif->getLink( basetexdata, "source" ), "NiSourceTexture" );
		
		foreach( int link, nif->getChildLinks( nif->getBlockNumber( property ) ) )
		{
			QModelIndex block = nif->getBlock( link );
			if ( block.isValid() && nif->inherits( nif->itemName( block ), "AController" ) )
				setController( nif, block );
		}
	}
	else if ( propname == "NiAlphaProperty" )
	{
		blocks.append( QPersistentModelIndex( property ) );
		
		alphaEnable = true;
		alphaSrc = GL_SRC_ALPHA; // using default blend mode
		alphaDst = GL_ONE_MINUS_SRC_ALPHA;
	}
	else if ( propname == "NiSpecularProperty" )
	{
		blocks.append( QPersistentModelIndex( property ) );
		
		specularEnable = true;
	}
	else
		Node::setProperty( nif, property );
}

void Mesh::setController( const NifModel * nif, const QModelIndex & controller )
{
	if ( nif->itemName( controller ) == "NiAlphaController" )
		controllers.append( new AlphaController( this, nif, controller ) );
	else if ( nif->itemName( controller ) == "NiGeomMorpherController" )
		controllers.append( new MorphController( this, nif, controller ) );
	else if ( nif->itemName( controller ) == "NiFlipController" )
		controllers.append( new TexFlipController( this, nif, controller ) );
	else if ( nif->itemName( controller ) == "NiUVController" )
		controllers.append( new TexCoordController( this, nif, controller ) );
	else
		Node::setController( nif, controller );
}

bool compareTriangles( const Triangle & tri1, const Triangle & tri2 )
{
	return ( tri1.depth < tri2.depth );
}

void Mesh::transform()
{
	Node::transform();
	
	Transform sceneTrans = scene->view * worldTrans();
	
	sceneCenter = sceneTrans * localCenter;
	
	if ( weights.count() )
	{
		transVerts.resize( verts.count() );
		transVerts.fill( Vector3() );
		transNorms.resize( norms.count() );
		transNorms.fill( Vector3() );
		
		foreach ( BoneWeights bw, weights )
		{
			Node * bone = scene->nodes.value( bw.bone );
			Transform trans = sceneTrans * skelTrans;
			if ( bone )
				trans = trans * bone->localTransFrom( skelRoot );
			trans = trans * bw.trans;
			
			Matrix natrix = trans.rotation;
			foreach ( VertexWeight vw, bw.weights )
			{
				if ( transVerts.count() > vw.vertex )
					transVerts[ vw.vertex ] += trans * verts[ vw.vertex ] * vw.weight;
				if ( transNorms.count() > vw.vertex )
					transNorms[ vw.vertex ] += natrix * norms[ vw.vertex ] * vw.weight;
			}
		}
		for ( int n = 0; n < transNorms.count(); n++ )
			transNorms[n].normalize();
	}
	else
	{
		transVerts.resize( verts.count() );
		for ( int v = 0; v < verts.count(); v++ )
			transVerts[v] = sceneTrans * verts[v];
		
		transNorms.resize( norms.count() );
		Matrix natrix = sceneTrans.rotation;
		for ( int n = 0; n < norms.count(); n++ )
		{
			transNorms[n] = natrix * norms[n];
			transNorms[n].normalize();
		}
	}
	
	if ( alphaEnable )
	{
		for ( int t = 0; t < triangles.count(); t++ )
		{
			Triangle tri = triangles[t];
			triangles[t].depth = transVerts[tri.v1][2] + transVerts[tri.v2][2] + transVerts[tri.v3][2];
		}
		qStableSort( triangles.begin(), triangles.end(), compareTriangles );
	}
}

void Mesh::boundaries( Vector3 & min, Vector3 & max )
{
	if ( transVerts.count() )
	{
		min = max = transVerts[ 0 ];
		
		foreach ( Vector3 v, transVerts )
		{
			for ( int c = 0; c < 3; c++ )
			{
				min[ c ] = qMin( min[ c ], v[ c ] );
				max[ c ] = qMax( max[ c ], v[ c ] );
			}
		}
	}
}

void Mesh::draw( bool selected )
{
	if ( isHidden() && ! scene->drawHidden )
		return;
	
	glLoadName( nodeId );
	
	// setup material colors
	
	if ( colors.count() )
	{
		glEnable( GL_COLOR_MATERIAL );
		glColorMaterial( GL_FRONT, GL_AMBIENT_AND_DIFFUSE );
	}
	else
	{
		glDisable( GL_COLOR_MATERIAL );
	}

	glMaterialf(GL_FRONT, GL_SHININESS, shininess );
	glMaterial( GL_FRONT, GL_AMBIENT, ambient.blend( alpha ) );
	glMaterial( GL_FRONT, GL_DIFFUSE, diffuse.blend( alpha ) );
	glMaterial( GL_FRONT, GL_EMISSION, emissive.blend( alpha ) );
	glMaterial( GL_FRONT, GL_SPECULAR, specular.blend( alpha ) );
	glColor4f( 1.0, 1.0, 1.0, 1.0 );

	// setup texturing
	
	if ( scene->texturing && uvs.count() && scene->bindTexture( iBaseTex ) )
	{
		glEnable( GL_TEXTURE_2D );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texFilter );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, texWrapS );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, texWrapT );
		glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	}
	else
	{
		glDisable( GL_TEXTURE_2D );
	}
	
	// setup z buffer
	
	bool dTest, dMask;
	depthBuffer( dTest, dMask );
	if ( dTest )
	{
		glEnable( GL_DEPTH_TEST );
		glDepthMask( dMask ? GL_TRUE : GL_FALSE );
	}
	else
	{
		glDisable( GL_DEPTH_TEST );
	}
	glDepthFunc( GL_LEQUAL );

	// setup alpha blending

	if ( alphaEnable && scene->blending )
	{
		glEnable( GL_BLEND );
		glBlendFunc( alphaSrc, alphaDst );
	}
	else
	{
		glDisable( GL_BLEND );
	}
	
	// normalize
	
	if ( transNorms.count() > 0 )
		glEnable( GL_NORMALIZE );
	else
		glDisable( GL_NORMALIZE );
	
	// render the triangles
	
	if ( triangles.count() > 0 )
	{
		glBegin( GL_TRIANGLES );
		foreach ( Triangle tri, triangles )
		{
			if ( transVerts.count() > tri.v1 && transVerts.count() > tri.v2 && transVerts.count() > tri.v3 )
			{
				if ( transNorms.count() > tri.v1 ) glNormal( transNorms[tri.v1] );
				if ( uvs.count() > tri.v1 ) glTexCoord( uvs[tri.v1] + texOffset );
				if ( colors.count() > tri.v1 ) glColor( colors[tri.v1].blend( alpha ) );
				glVertex( transVerts[tri.v1] );
				if ( transNorms.count() > tri.v2 ) glNormal( transNorms[tri.v2] );
				if ( uvs.count() > tri.v2 ) glTexCoord( uvs[tri.v2] + texOffset );
				if ( colors.count() > tri.v2 ) glColor( colors[tri.v2].blend( alpha ) );
				glVertex( transVerts[tri.v2] );
				if ( transNorms.count() > tri.v3 ) glNormal( transNorms[tri.v3] );
				if ( uvs.count() > tri.v3 ) glTexCoord( uvs[tri.v3] + texOffset );
				if ( colors.count() > tri.v3 ) glColor( colors[tri.v3].blend( alpha ) );
				glVertex( transVerts[tri.v3] );
			}
		}
		glEnd();
	}
	
	// render the tristrips
	
	foreach ( Tristrip strip, tristrips )
	{
		glBegin( GL_TRIANGLE_STRIP );
		foreach ( int v, strip.vertices )
		{
			if ( transNorms.count() > v ) glNormal( transNorms[v] );
			if ( uvs.count() > v ) glTexCoord( uvs[v] + texOffset );
			if ( colors.count() > v ) glColor( colors[v].blend( alpha ) );
			if ( transVerts.count() > v ) glVertex( transVerts[v] );
		}
		glEnd();
	}
	
	// draw green mesh outline if selected
	
	if ( selected )
	{
		glPushAttrib( GL_LIGHTING_BIT );
		glEnable( GL_DEPTH_TEST );
		glDepthMask( GL_TRUE );
		glDepthFunc( GL_LEQUAL );
		glDisable( GL_TEXTURE_2D );
		glDisable( GL_NORMALIZE );
		glDisable( GL_LIGHTING );
		glDisable( GL_COLOR_MATERIAL );
		glColor4f( 0.0, 1.0, 0.0, 1.0 );
		glLineWidth( 2.0 );
		
		foreach ( Triangle tri, triangles )
		{
			if ( transVerts.count() > tri.v1 && transVerts.count() > tri.v2 && transVerts.count() > tri.v3 )
			{
				glBegin( GL_LINE_STRIP );
				glVertex( transVerts[tri.v1] );
				glVertex( transVerts[tri.v2] );
				glVertex( transVerts[tri.v3] );
				glVertex( transVerts[tri.v1] );
				glEnd();
			}
		}
		
		foreach ( Tristrip strip, tristrips )
		{
			glBegin( GL_LINE_STRIP );
			foreach ( int v, strip.vertices )
			{
				if ( transVerts.count() > v )
					glVertex( transVerts[v] );
			}
			glEnd();
		}
		
		glPopAttrib();
	}
}


/*
 *  Scene
 */


Scene::Scene()
{
	texturing = true;
	blending = true;
	highlight = true;
	drawNodes = true;
	drawHidden = false;
	currentNode = 0;
	animate = true;
	
	time = timeMin = timeMax = 0.0;
}

Scene::~Scene()
{
	clear();
}

void Scene::clear()
{
	qDeleteAll( nodes ); nodes.clear();
	qDeleteAll( meshes ); meshes.clear();
	qDeleteAll( textures ); textures.clear();
	boundMin = boundMax = boundCenter = Vector3( 0.0, 0.0, 0.0 );
	boundRadius = Vector3( 1.0, 1.0, 1.0 );
	timeMin = timeMax = 0.0;
}

void Scene::update( const NifModel * nif, const QModelIndex & index )
{
	QModelIndex block = nif->getBlock( index );
	
	foreach ( Node * node, nodes )
		if ( node->update( block ) )
			node->make();
	foreach ( Mesh * mesh, meshes )
		if ( mesh->update( block ) )
			mesh->make();
	
	QList<GLTex*> rem;
	foreach ( GLTex * tex, textures )
		if ( tex->iSource == block || tex->iPixelData == block )
			rem.append( tex );
	foreach ( GLTex * tex, rem )
	{
		textures.removeAll( tex );
		delete tex;
	}
}

void Scene::make( NifModel * nif )
{
	clear();
	if ( ! nif ) return;
	foreach ( int link, nif->getRootLinks() )
	{
		QStack<int> nodestack;
		make( nif, link, nodestack );
	}

	if ( ! ( nodes.isEmpty() && meshes.isEmpty() ) )
	{
		timeMin = +1000000000; timeMax = -1000000000;
		foreach ( Node * node, nodes )
			node->timeBounds( timeMin, timeMax );
		foreach ( Mesh * mesh, meshes )
			mesh->timeBounds( timeMin, timeMax );
	}
	
	/*
	int v = 0;
	int t = 0;
	int s = 0;
	foreach ( Mesh * mesh, meshes )
	{
		v += mesh->verts.count();
		t += mesh->triangles.count();
		s += mesh->tristrips.count();
	}
	qWarning() << v << t << s;
	*/
}

void Scene::make( NifModel * nif, int blockNumber, QStack<int> & nodestack )
{
	QModelIndex idx = nif->getBlock( blockNumber );

	if ( ! idx.isValid() )
	{
		qWarning() << "block " << blockNumber << " not found";
		return;
	}
	
	Node * parent = ( nodestack.count() ? nodes.value( nodestack.top() ) : 0 );

	if ( nif->inherits( nif->itemName( idx ), "AParentNode" ) )
	{
		if ( nodestack.contains( blockNumber ) )
		{
			qWarning( "infinite recursive node construct detected ( %d -> %d )", nodestack.top(), blockNumber );
			return;
		}
		
		if ( nodes.contains( blockNumber ) )
		{
			qWarning( "node %d is referrenced multiple times ( %d )", blockNumber, nodestack.top() );
			return;
		}
		
		Node * node = new Node( this, parent, idx );
		node->make();
		
		nodes.insert( blockNumber, node );
		
		nodestack.push( blockNumber );
		
		foreach ( int link, nif->getChildLinks( blockNumber ) )
			make( nif, link, nodestack );
		
		nodestack.pop();
	}
	else if ( nif->itemName( idx ) == "NiTriShape" || nif->itemName( idx ) == "NiTriStrips" )
	{
		Mesh * mesh = new Mesh( this, parent, idx );
		mesh->make();
		meshes.append( mesh );
	}
}

bool compareMeshes( const Mesh * mesh1, const Mesh * mesh2 )
{
	// opaque meshes first (sorted from front to rear)
	// then alpha enabled meshes (sorted from rear to front)
	
	if ( mesh1->alphaEnable == mesh2->alphaEnable )
		if ( mesh1->alphaEnable )
			return ( mesh1->sceneCenter[2] < mesh2->sceneCenter[2] );
		else
			return ( mesh1->sceneCenter[2] > mesh2->sceneCenter[2] );
	else
		return mesh2->alphaEnable;
}

void Scene::transform( const Transform & trans, float time )
{
	view = trans;
	this->time = time;
	
	foreach ( Node * node, nodes )
		node->transform();
	foreach ( Mesh * mesh, meshes )
		mesh->transform();
	
	qStableSort( meshes.begin(), meshes.end(), compareMeshes );

	if ( ! ( nodes.isEmpty() && meshes.isEmpty() ) )
	{
		boundMin = Vector3( +1000000000, +1000000000, +1000000000 );
		boundMax = Vector3( -1000000000, -1000000000, -1000000000 );
		foreach ( Node * node, nodes )
		{
			Vector3 min, max;
			node->boundaries( min, max );
			boundMin = Vector3::min( boundMin, min );
			boundMax = Vector3::max( boundMax, max );
		}
		foreach ( Mesh * mesh, meshes )
		{
			Vector3 min, max;
			mesh->boundaries( min, max );
			boundMin = Vector3::min( boundMin, min );
			boundMax = Vector3::max( boundMax, max );
		}
		for ( int c = 0; c < 3; c++ )
		{
			boundRadius[c] = ( boundMax[c] - boundMin[c] ) / 2;
			boundCenter[c] = boundMin[c] + boundRadius[c];
		}
	}
}

void Scene::draw()
{	
	glEnable( GL_CULL_FACE );
	
	foreach ( Mesh * mesh, meshes )
		mesh->draw( highlight && mesh->id() == currentNode );

	if ( drawNodes )
		foreach ( Node * node, nodes )
			node->draw( highlight && node->id() == currentNode );
}

#endif
