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

#include "nifmodel.h"

Triangle::Triangle( NifModel * nif, const QModelIndex & triangle )
{
	if ( !triangle.isValid() )		{	v1 = v2 = v3 = 0; return;	}
	v1 = nif->getInt( triangle, "v1" );
	v2 = nif->getInt( triangle, "v2" );
	v3 = nif->getInt( triangle, "v3" );
}

Tristrip::Tristrip( NifModel * nif, const QModelIndex & tristrip )
{
	if ( ! tristrip.isValid() ) return;
	
	for ( int s = 0; s < nif->rowCount( tristrip ); s++ )
		vertices.append( nif->itemValue( tristrip.child( s, 0 ) ).toInt() );
}

Color::Color( const QColor & c )
{
	rgba[ 0 ] = c.redF();
	rgba[ 1 ] = c.greenF();
	rgba[ 2 ] = c.blueF();
	rgba[ 3 ] = c.alphaF();
}

BoneWeights::BoneWeights( NifModel * nif, const QModelIndex & index, int b )
{
	matrix = Matrix( nif, index );
	bone = b;
	
	QModelIndex idxWeights = nif->getIndex( index, "vertex weights" );
	if ( idxWeights.isValid() )
	{
		for ( int c = 0; c < nif->rowCount( idxWeights ); c++ )
		{
			QModelIndex idx = idxWeights.child( c, 0 );
			weights.append( VertexWeight( nif->getInt( idx, "index" ), nif->getFloat( idx, "weight" ) ) );
		}
	}
	else
		qWarning() << nif->getBlockNumber( index ) << "vertex weights not found";
}

/*
 *	Node
 */

Node::Node( Scene * s, Node * p ) : scene( s ), parent( p )
{
	nodeId = 0;
	hidden = false;
	
	depthProp = false;
	depthTest = true;
	depthMask = true;
}

void Node::init( NifModel * nif, const QModelIndex & index )
{
	nodeId = nif->getBlockNumber( index );

	hidden = nif->getInt( index, "flags" ) & 1;

	local = Matrix( nif, index );
	
	if ( parent )
		world = parent->world * local;
	else
		world = local;

	foreach( int link, nif->getChildLinks( nodeId ) )
	{
		QModelIndex block = nif->getBlock( link );
		if ( ! block.isValid() ) continue;
		QString name = nif->itemName( block );
		
		if ( nif->inherits( name, "AProperty" ) )
			setProperty( nif, block );
		else if ( nif->inherits( name, "AController" ) )
			setController( nif, block );
		else
			setSpecial( nif, block );
	}
}

void Node::setController( NifModel * nif, const QModelIndex & )
{
}

void Node::setProperty( NifModel * nif, const QModelIndex & property )
{
	QString propname = nif->itemName( property );
	if ( propname == "NiZBufferProperty" )
	{
		int flags = nif->getInt( property, "flags" );
		depthTest = flags & 1;
		depthMask = flags & 2;
	}
}

void Node::setSpecial( NifModel * nif, const QModelIndex & )
{
}

const Matrix & Node::worldTrans() const
{
	return world;
}

bool Node::isHidden() const
{
	if ( hidden )
		return true;
	if ( parent )
		return parent->isHidden();
	else
		return false;
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

/*
 *  Mesh
 */


Mesh::Mesh( Scene * s, Node * p ) : Node( s, p )
{
	shininess = 33.0;
	alpha = 1.0;
	texSet = 0;
	texFilter = GL_LINEAR;
	texWrapS = texWrapT = GL_REPEAT;
	alphaEnable = false;
	alphaSrc = GL_SRC_ALPHA;
	alphaDst = GL_ONE_MINUS_SRC_ALPHA;
	specularEnable = false;

	useList = true;
	list = 0;
	
}

Mesh::~Mesh()
{
	if ( list )
		glDeleteLists( list, 1 );
}

void Mesh::init( NifModel * nif, const QModelIndex & index )
{
	Node::init( nif, index );
	
	localCenter = Vector( nif, nif->getIndex( index, "center" ) );
	
	if ( ! alphaEnable )
	{
		alpha = 1.0;
		ambient[3] = alpha;
		diffuse[3] = alpha;
		specular[3] = alpha;
		emissive[3] = alpha;
	}
	
	if ( ! specularEnable )
	{
		specular = Color( 0, 0, 0, alpha );
	}
}

void Mesh::setSpecial( NifModel * nif, const QModelIndex & special )
{
	QString name = nif->itemName( special );
	if ( name == "NiTriShapeData" || name == "NiTriStripsData" )
	{
		verts.clear();
		norms.clear();
		colors.clear();
		uvs.clear();
		triangles.clear();
		tristrips.clear();
		
		QModelIndex vertices = nif->getIndex( special, "vertices" );
		if ( vertices.isValid() )
		{
			for ( int r = 0; r < nif->rowCount( vertices ); r++ )
				verts.append( Vector( nif, nif->index( r, 0, vertices ) ) );
		}
		
		QModelIndex normals = nif->getIndex( special, "normals" );
		if ( normals.isValid() )
		{
			for ( int r = 0; r < nif->rowCount( normals ); r++ )
				norms.append( Vector( nif, nif->index( r, 0, normals ) ) );
		}
		QModelIndex vertexcolors = nif->getIndex( special, "vertex colors" );
		if ( vertexcolors.isValid() )
		{
			for ( int r = 0; r < nif->rowCount( vertexcolors ); r++ )
				colors.append( Color( nif->itemValue( vertexcolors.child( r, 0 ) ).value<QColor>() ) );
		}
		QModelIndex uvcoord = nif->getIndex( special, "uv sets" );
		if ( uvcoord.isValid() )
		{
			QModelIndex uvcoordset = nif->index( texSet, 0, uvcoord );
			if ( uvcoordset.isValid() )
			{
				for ( int r = 0; r < nif->rowCount( uvcoordset ); r++ )
				{
					QModelIndex uv = nif->index( r, 0, uvcoordset );
					uvs.append( nif->getFloat( uv, "u" ) );
					uvs.append( nif->getFloat( uv, "v" ) );
				}
			}
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
		weights.clear();
		
		int sdat = nif->getInt( special, "data" );
		QModelIndex skindata = nif->getBlock( sdat, "NiSkinData" );
		if ( ! skindata.isValid() )
		{
			qWarning() << "niskindata not found";
			return;
		}
		
		QVector<int> bones;
		QModelIndex idxBones = nif->getIndex( nif->getIndex( special, "bones" ), "bones" );
		if ( ! idxBones.isValid() )
		{
			qWarning() << "bones array not found";
			return;
		}
		
		for ( int b = 0; b < nif->rowCount( idxBones ); b++ )
			bones.append( nif->itemValue( nif->index( b, 0, idxBones ) ).toInt() );
		
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

void Mesh::setProperty( NifModel * nif, const QModelIndex & property )
{
	QString propname = nif->itemName( property );
	if ( propname == "NiMaterialProperty" )
	{
		ambient = Color( nif->getValue( property, "ambient color" ).value<QColor>() );
		diffuse = Color( nif->getValue( property, "diffuse color" ).value<QColor>() );
		specular = Color( nif->getValue( property, "specular color" ).value<QColor>() );
		emissive = Color( nif->getValue( property, "emissive color" ).value<QColor>() );
		
		shininess = nif->getFloat( property, "glossiness" ) * 1.28; // range 0 ~ 128 (nif 0~100)
		alpha = nif->getFloat( property, "alpha" );
		if ( alpha < 0.0 ) alpha = 0.0;
		if ( alpha > 1.0 ) alpha = 1.0;
		ambient[3] = alpha;
		diffuse[3] = alpha;
		specular[3] = alpha;
		emissive[3] = alpha;
	}
	else if ( propname == "NiTexturingProperty" )
	{
		QModelIndex basetex = nif->getIndex( property, "base texture" );
		if ( ! basetex.isValid() )	return;
		QModelIndex basetexdata = nif->getIndex( basetex, "texture data" );
		if ( ! basetexdata.isValid() )	return;
		
		int src = nif->getInt( basetexdata, "source" );
		QModelIndex sourcetexture = nif->getBlock( src, "NiSourceTexture" );
		if ( ! sourcetexture.isValid() ) return;

		QModelIndex source = nif->getIndex( sourcetexture, "texture source" );
		texFile = nif->getValue( source, "file name" ).toString();

		switch ( nif->getInt( basetexdata, "filter mode" ) )
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
		switch ( nif->getInt( basetexdata, "clamp mode" ) )
		{
			case 0:		texWrapS = GL_CLAMP;	texWrapT = GL_CLAMP;	break;
			case 1:		texWrapS = GL_CLAMP;	texWrapT = GL_REPEAT;	break;
			case 2:		texWrapS = GL_REPEAT;	texWrapT = GL_CLAMP;	break;
			default:	texWrapS = GL_REPEAT;	texWrapT = GL_REPEAT;	break;
		}
		texSet = nif->getInt( basetexdata, "texture set" );
	}
	else if ( propname == "NiAlphaProperty" )
	{
		alphaEnable = true;
		alphaSrc = GL_SRC_ALPHA; // using default blend mode
		alphaDst = GL_ONE_MINUS_SRC_ALPHA;
		useList = false;
	}
	else if ( propname == "NiSpecularProperty" )
	{
		specularEnable = true;
	}
	else
		Node::setProperty( nif, property );
}

bool compareTriangles( const Triangle & tri1, const Triangle & tri2 )
{
	return ( tri1.depth < tri2.depth );
}

void Mesh::transform( const Matrix & trans )
{
	Matrix sceneTrans = trans * world;
	
	sceneCenter = sceneTrans * localCenter;
	
	if ( useList && transVerts.count() )
	{
		if ( weights.count() )
			glmatrix = trans;
		else
			glmatrix = sceneTrans;
		return;
	}
	
	if ( weights.count() )
	{
		transVerts.resize( verts.count() );
		transVerts.fill( Vector() );
		transNorms.resize( norms.count() );
		transNorms.fill( Vector() );
		
		foreach ( BoneWeights bw, weights )
		{
			Node * bone = scene->nodes.value( bw.bone );
			Matrix matrix = ( bone ? bone->worldTrans() * bw.matrix : bw.matrix );
			if ( alphaEnable )
				matrix = trans * matrix;
			
			Matrix natrix = matrix;
			natrix.clearTrans();
			
			foreach ( VertexWeight vw, bw.weights )
			{
				if ( transVerts.count() > vw.vertex )
					transVerts[ vw.vertex ] += matrix * verts[ vw.vertex ] * vw.weight;
				if ( transNorms.count() > vw.vertex )
					transNorms[ vw.vertex ] += natrix * norms[ vw.vertex ] * vw.weight;
			}
		}
		for ( int n = 0; n < transNorms.count(); n++ )
			transNorms[n].normalize();
		
		if ( alphaEnable )
			glmatrix = Matrix();
		else
			glmatrix = trans;
	}
	else
	{
		if ( alphaEnable )
		{
			transVerts.resize( verts.count() );
			for ( int v = 0; v < verts.count(); v++ )
				transVerts[v] = sceneTrans * verts[v];
			
			transNorms.resize( norms.count() );
			Matrix natrix = sceneTrans;
			natrix.clearTrans();
			for ( int n = 0; n < norms.count(); n++ )
			{
				transNorms[n] = natrix * norms[n];
				transNorms[n].normalize();
			}
			glmatrix = Matrix();
		}
		else
		{
			transVerts = verts;
			transNorms = norms;
			glmatrix = sceneTrans;
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

void Mesh::draw( bool selected )
{
	if ( isHidden() )
		return;
	
	glLoadName( nodeId );
	
	// setup material colors
	
	glEnable( GL_COLOR_MATERIAL );
	glMaterialf(GL_FRONT, GL_SHININESS, shininess);
	ambient.glMaterial( GL_FRONT, GL_AMBIENT );
	diffuse.glMaterial( GL_FRONT, GL_DIFFUSE );
	emissive.glMaterial( GL_FRONT, GL_EMISSION );
	specular.glMaterial( GL_FRONT, GL_SPECULAR );
	
	if ( colors.count() )
	{	 // color material is controlled by NiVertexColorProperty using the default for now
		glColorMaterial( GL_FRONT, GL_AMBIENT_AND_DIFFUSE );
	}
	else
	{
		glColorMaterial( GL_FRONT, GL_AMBIENT );
		ambient.glColor();
	}
	
	// setup texturing
	
	if ( ! texFile.isEmpty() && scene->texturing && uvs.count() && scene->bindTexture( texFile ) )
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
		glDepthMask( GL_TRUE );
		glDisable( GL_BLEND );
	}
	
	// normalize
	
	if ( transNorms.count() > 0 )
		glEnable( GL_NORMALIZE );
	else
		glDisable( GL_NORMALIZE );
	
	// load matrix
	
	glmatrix.glLoadMatrix();

	// check display list
	/*
	if ( useList )
	{
		if ( list )
		{
			glCallList( list );
			return;
		}
		list = glGenLists( 1 );
		glNewList( list, GL_COMPILE_AND_EXECUTE );
	}
	*/
	// render the triangles
	
	if ( triangles.count() > 0 )
	{
		glBegin( GL_TRIANGLES );
		foreach ( Triangle tri, triangles )
		{
			if ( transVerts.count() > tri.v1 && transVerts.count() > tri.v2 && transVerts.count() > tri.v3 )
			{
				if ( transNorms.count() > tri.v1 ) transNorms[tri.v1].glNormal();
				if ( uvs.count() > tri.v1*2 ) glTexCoord2f( uvs[tri.v1*2+0], uvs[tri.v1*2+1] );
				if ( colors.count() > tri.v1 ) colors[tri.v1].glColor();
				transVerts[tri.v1].glVertex();
				if ( transNorms.count() > tri.v2 ) transNorms[tri.v2].glNormal();
				if ( uvs.count() > tri.v2*2 ) glTexCoord2f( uvs[tri.v2*2+0], uvs[tri.v2*2+1] );
				if ( colors.count() > tri.v2 ) colors[tri.v2].glColor();
				transVerts[tri.v2].glVertex();
				if ( transNorms.count() > tri.v3 ) transNorms[tri.v3].glNormal();
				if ( uvs.count() > tri.v3*2 ) glTexCoord2f( uvs[tri.v3*2+0], uvs[tri.v3*2+1] );
				if ( colors.count() > tri.v3 ) colors[tri.v3].glColor();
				transVerts[tri.v3].glVertex();
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
			if ( transNorms.count() > v ) transNorms[v].glNormal();
			if ( uvs.count() > v*2 ) glTexCoord2f( uvs[v*2+0], uvs[v*2+1] );
			if ( colors.count() > v ) colors[v].glColor();
			transVerts[v].glVertex();
		}
		glEnd();
	}
	
	//if ( useList )
	//	glEndList();

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
		
		foreach ( Triangle tri, triangles )
		{
			if ( transVerts.count() > tri.v1 && transVerts.count() > tri.v2 && transVerts.count() > tri.v3 )
			{
				glBegin( GL_LINE_STRIP );
				transVerts[tri.v1].glVertex();
				transVerts[tri.v2].glVertex();
				transVerts[tri.v3].glVertex();
				transVerts[tri.v1].glVertex();
				glEnd();
			}
		}
		
		foreach ( Tristrip strip, tristrips )
		{
			glBegin( GL_LINE_STRIP );
			foreach ( int v, strip.vertices )
			{
				transVerts[v].glVertex();
			}
			glEnd();
		}
		
		glPopAttrib();
	}
}

void Mesh::boundaries( Vector & min, Vector & max )
{
	min = Vector( +1000000000, +1000000000, +1000000000 );
	max = Vector( -1000000000, -1000000000, -1000000000 );

	QVector<Vector> boundVerts = verts;
	if ( weights.count() )
	{
		boundVerts.fill( Vector() );
		foreach ( BoneWeights bw, weights )
		{
			Node * bone = scene->nodes.value( bw.bone );
			Matrix matrix = ( bone ? bone->worldTrans() * bw.matrix : bw.matrix );
			
			foreach ( VertexWeight vw, bw.weights )
			{
				if ( boundVerts.count() > vw.vertex )
					boundVerts[ vw.vertex ] += matrix * verts[ vw.vertex ] * vw.weight;
			}
		}
	}
	else
	{
		Matrix matrix = worldTrans();
		for ( int v = 0; v < boundVerts.count(); v++ )
			boundVerts[v] = matrix * boundVerts[v];
	}
	
	foreach ( Vector v, boundVerts )
	{
		min = Vector::min( min, v );
		max = Vector::max( max, v );
	}
}

Scene::Scene( const QGLContext * context )
{
	this->context = context;
	
	texturing = true;
	blending = true;
	highlight = true;
	currentNode = 0;
}

Scene::~Scene()
{
	clear();
	textures.clear();
}

void Scene::clear()
{
	qDeleteAll( nodes ); nodes.clear();
	qDeleteAll( meshes ); meshes.clear();
	nodestack.clear();
	texInitPhase = true;
}

void Scene::make( NifModel * nif )
{
	clear();
	if ( ! nif ) return;
	foreach ( int link, nif->getRootLinks() )
		make( nif, link );
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

void Scene::make( NifModel * nif, int blockNumber )
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
		
		Node * node = new Node( this, parent );
		node->init( nif, idx );
		
		nodes.insert( blockNumber, node );
		
		nodestack.push( blockNumber );
		
		foreach ( int link, nif->getChildLinks( blockNumber ) )
			make( nif, link );
		
		nodestack.pop();
	}
	else if ( nif->itemName( idx ) == "NiTriShape" || nif->itemName( idx ) == "NiTriStrips" )
	{
		Mesh * mesh = new Mesh( this, parent );
		mesh->init( nif, idx );
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

void Scene::draw( const Matrix & matrix )
{	
	foreach ( Mesh * mesh, meshes )
		mesh->transform( matrix );

	qStableSort( meshes.begin(), meshes.end(), compareMeshes );
	
	drawAgain();
	texInitPhase = false;
}

void Scene::drawAgain()
{	
	glPushMatrix();
	glEnable( GL_CULL_FACE );
	
	foreach ( Mesh * mesh, meshes )
		mesh->draw( highlight && mesh->id() == currentNode );

	glPopMatrix();
}

void Scene::boundaries( Vector & min, Vector & max )
{
	if ( meshes.count() )
	{
		min = Vector( +1000000000, +1000000000, +1000000000 );
		max = Vector( -1000000000, -1000000000, -1000000000 );
		Vector mmin, mmax;
		foreach ( Mesh * mesh, meshes )
		{
			mesh->boundaries( mmin, mmax );
			min = Vector::min( min, mmin );
			max = Vector::max( max, mmax );
		}
	}
	else
	{
		min = Vector();
		max = Vector();
	}
}

#endif
