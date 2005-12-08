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

Mesh::Mesh( Scene * s ) : scene( s )
{
	init();
}

Mesh::~Mesh()
{
	if ( list )
		glDeleteLists( list, 1 );
}

Mesh::Mesh( NifModel * nif, const QModelIndex & index, Scene * s, int parent ) : scene( s )
{
	init();
	
	parentNode = parent;
	meshId = nif->getBlockNumber( index );
	
	localTrans = Matrix( nif, index );
	localCenter = Vector( nif, nif->getIndex( index, "center" ) );
	
	QModelIndex properties = nif->getIndex( index, "properties" );
	if ( properties.isValid() )
	{
		QModelIndex proplinks = nif->getIndex( properties, "indices" );
		if ( proplinks.isValid() )
		{
			for ( int p = 0; p < nif->rowCount( proplinks ); p++ )
			{
				int prop = nif->itemValue( proplinks.child( p, 0 ) ).toInt();
				QModelIndex property = nif->getBlock( prop );
				if ( property.isValid() )
					setProperty( nif, property );
			}
		}
	}
	
	int data = nif->getInt( index, "data" );
	QModelIndex tridata = nif->getBlock( data );
	if ( tridata.isValid() )
	{
		setData( nif, tridata );
	}
	else
	{
		qWarning() << nif->itemName( index ) << "(" << nif->getBlockNumber( index ) << ") data block (" << data << ") not found";
		return;
	}

	int skin = nif->getInt( index, "skin instance" );
	QModelIndex skininst = nif->getBlock( skin, "NiSkinInstance" );
	if ( skininst.isValid() )
		setSkinInstance( nif, skininst );	
}

void Mesh::init()
{
	parentNode = 0;
	meshId = 0;
	shininess = 33.0;
	alpha = 1.0;
	texSet = 0;
	texFilter = GL_LINEAR;
	alphaEnable = false;
	alphaSrc = GL_SRC_ALPHA;
	alphaDst = GL_ONE_MINUS_SRC_ALPHA;
	specularEnable = false;

	useList = true;
	list = 0;
}

void Mesh::setData( NifModel * nif, const QModelIndex & tridata )
{
	verts.clear();
	norms.clear();
	colors.clear();
	uvs.clear();
	triangles.clear();
	tristrips.clear();
	
	if ( nif->itemName( tridata ) != "NiTriShapeData" && nif->itemName( tridata ) != "NiTriStripsData" )
	{
		qWarning() << "data block (" << nif->getBlockNumber( tridata ) << ") type mismatch";
		return;
	}
	
	QModelIndex vertices = nif->getIndex( tridata, "vertices" );
	if ( vertices.isValid() )
	{
		for ( int r = 0; r < nif->rowCount( vertices ); r++ )
			verts.append( Vector( nif, nif->index( r, 0, vertices ) ) );
	}
	
	QModelIndex normals = nif->getIndex( tridata, "normals" );
	if ( normals.isValid() )
	{
		for ( int r = 0; r < nif->rowCount( normals ); r++ )
			norms.append( Vector( nif, nif->index( r, 0, normals ) ) );
	}
	QModelIndex vertexcolors = nif->getIndex( tridata, "vertex colors" );
	if ( vertexcolors.isValid() )
	{
		for ( int r = 0; r < nif->rowCount( vertexcolors ); r++ )
			colors.append( Color( nif->itemValue( vertexcolors.child( r, 0 ) ).value<QColor>() ) );
	}
	QModelIndex uvcoord = nif->getIndex( tridata, "uv sets" );
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

	if ( nif->itemName( tridata ) == "NiTriShapeData" )
	{
		QModelIndex idxTriangles = nif->getIndex( tridata, "triangles" );
		if ( idxTriangles.isValid() )
		{
			for ( int r = 0; r < nif->rowCount( idxTriangles ); r++ )
				triangles.append( Triangle( nif, idxTriangles.child( r, 0 ) ) );
		}
		else
			qWarning() << nif->itemName( tridata ) << "(" << nif->getBlockNumber( tridata ) << ") triangle array not found";
	}
	else
	{
		QModelIndex points = nif->getIndex( tridata, "points" );
		if ( points.isValid() )
		{
			for ( int r = 0; r < nif->rowCount( points ); r++ )
				tristrips.append( Tristrip( nif, points.child( r, 0 ) ) );
		}
		else
			qWarning() << nif->itemName( tridata ) << "(" << nif->getBlockNumber( tridata ) << ") 'points' array not found";
	}
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

void Mesh::setSkinInstance( NifModel * nif, const QModelIndex & skininst )
{
	weights.clear();
	
	int sdat = nif->getInt( skininst, "data" );
	QModelIndex skindata = nif->getBlock( sdat, "NiSkinData" );
	if ( ! skindata.isValid() )
	{
		qWarning() << "niskindata not found";
		return;
	}

	QVector<int> bones;
	QModelIndex idxBones = nif->getIndex( nif->getIndex( skininst, "bones" ), "bones" );
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

bool compareTriangles( const Triangle & tri1, const Triangle & tri2 )
{
	return ( tri1.depth < tri2.depth );
}

void Mesh::transform( const Matrix & trans )
{
	Node * node = scene->nodes.value( parentNode );
	if ( node )
		sceneTrans = trans * node->worldTrans * localTrans;
	else
		sceneTrans = trans * localTrans;
	
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
		if ( alphaEnable )
		{
			transNorms.resize( norms.count() );
			transNorms.fill( Vector() );
		}
		foreach ( BoneWeights bw, weights )
		{
			Node * bone = scene->nodes.value( bw.bone );
			Matrix matrix = ( bone ? bone->worldTrans * bw.matrix : bw.matrix );
			if ( alphaEnable )
				matrix = trans * matrix;
			
			Matrix natrix = matrix;
			natrix.clearTrans();
			
			foreach ( VertexWeight vw, bw.weights )
			{
				if ( transVerts.count() > vw.vertex )
					transVerts[ vw.vertex ] += matrix * verts[ vw.vertex ] * vw.weight;
				if ( alphaEnable && transNorms.count() > vw.vertex )
					transNorms[ vw.vertex ] += natrix * norms[ vw.vertex ] * vw.weight;
			}
		}
		if ( alphaEnable )
		{
			glmatrix = Matrix();
			for ( int n = 0; n < transNorms.count(); n++ )
				transNorms[n].normalize();
		}
		else
		{
			glmatrix = trans;
			transNorms = norms;
		}
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

void Mesh::draw()
{
	glLoadName( meshId );
	
	// setup material colors
	
	glEnable( GL_COLOR_MATERIAL );

	ambient.glMaterial( GL_FRONT, GL_AMBIENT );
	diffuse.glMaterial( GL_FRONT, GL_DIFFUSE );
	//emissive.glMaterial( GL_FRONT, GL_EMISIVE );
	
	if ( specularEnable )
		specular.glMaterial( GL_FRONT, GL_SPECULAR );
	else
		Color().glMaterial( GL_FRONT, GL_SPECULAR );
	glMaterialf(GL_FRONT, GL_SHININESS, shininess);
	
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
		glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	}
	else
	{
		glDisable( GL_TEXTURE_2D );
	}

	// setup alpha blending

	if ( alphaEnable && scene->blending )
	{
		glDepthMask( GL_FALSE );
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
	
	if ( useList )
	{
		if ( list )
		{
			glCallList( list );
			glPopMatrix();
			return;
		}
		list = glGenLists( 1 );
		glNewList( list, GL_COMPILE_AND_EXECUTE );
	}
	
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
	
	if ( useList )
		glEndList();
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
			Matrix matrix = ( bone ? bone->worldTrans * bw.matrix : bw.matrix );
			
			foreach ( VertexWeight vw, bw.weights )
			{
				if ( boundVerts.count() > vw.vertex )
					boundVerts[ vw.vertex ] += matrix * verts[ vw.vertex ] * vw.weight;
			}
		}
	}
	else
	{
		Node * node = scene->nodes.value( parentNode );
		Matrix matrix;
		if ( node )
			matrix = node->worldTrans;
		matrix = matrix * localTrans;
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
	matrixstack.clear();
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

	if ( ! ( idx.isValid() && nif->inherits( nif->itemName( idx ), "AParentNode" ) ) )
		return;
	
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
	
	if ( nif->getInt( idx, "flags" ) & 1 )
		return;

	Node * node = new Node;
	node->localTrans = Matrix( nif, idx );
	
	nodestack.push( blockNumber );
	matrixstack.push( node->localTrans );
	
	foreach ( Matrix m, matrixstack )
		node->worldTrans = node->worldTrans * m;
	
	nodes.insert( blockNumber, node );
	
	//qDebug() << "compile " << nif->itemName( idx ) << " (" << blockNumber << ") " << nif->itemValue( nif->getIndex( idx, "name" ) ).toString();
	
	QModelIndex children = nif->getIndex( idx, "children" );
	QModelIndex childlinks = nif->getIndex( children, "indices" );
	if ( ! ( children.isValid() && childlinks.isValid() ) )
	{
		qWarning() << "compileNode( " << blockNumber << " ) : children link list not found";
		return;
	}

	for ( int c = 0; c < nif->rowCount( childlinks ); c++ )
	{
		int r = nif->itemValue( childlinks.child( c, 0 ) ).toInt();
		QModelIndex child = nif->getBlock( r );
		if ( ! child.isValid() )
		{
			qWarning() << "block " << r << " not found";
			continue;
		}
		if ( nif->inherits( nif->itemName( child ), "AParentNode" ) )
		{
			make( nif, r );
		}
		else if ( nif->itemName( child ) == "NiTriShape" || nif->itemName( child ) == "NiTriStrips" )
		{
			if ( nif->getInt( child, "flags" ) & 1 )	continue;
			meshes.append( new Mesh( nif, child, this, nodestack.top() ) );
		}
	}

	matrixstack.pop();
	nodestack.pop();

	return;
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
	glPushMatrix();
	glEnable( GL_CULL_FACE );
	glEnable( GL_DEPTH_TEST ); // depth test is actually controlled by NiZBufferProperty using the default for now
	
	foreach ( Mesh * mesh, meshes )
		mesh->transform( matrix );

	qStableSort( meshes.begin(), meshes.end(), compareMeshes );
	
	foreach ( Mesh * mesh, meshes )
		mesh->draw();

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
