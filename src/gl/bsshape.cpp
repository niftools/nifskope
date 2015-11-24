#include "bsshape.h"
#include "settings.h"

#include "glscene.h"


BSShape::BSShape( Scene * s, const QModelIndex & b ) : Shape( s, b )
{
	
}

void BSShape::clear()
{
	Node::clear();

	verts.clear();
	norms.clear();
	tangents.clear();
	bitangents.clear();
	triangles.clear();
	coords.clear();
	colors.clear();
	test1.clear();
	test2.clear();
	test3.clear();
}

void BSShape::update( const NifModel * nif, const QModelIndex & index )
{
	Node::update( nif, index );

	if ( !iBlock.isValid() || !index.isValid() )
		return;

	if ( iBlock != index )
		return;

	//updateData |= index == iBlock;
	updateData = true;
	updateBounds |= updateData;

	iVertData = nif->getIndex( iBlock, "Vertex Data" );
	iTriData = nif->getIndex( iBlock, "Triangles" );
	iData = iVertData;

	numVerts = nif->get<int>( iBlock, "Num Vertices" );
	numTris = nif->get<int>( iBlock, "Num Triangles" );

	auto vc = nif->rowCount( iVertData );
	auto tc = nif->rowCount( iTriData );

	Q_ASSERT( (vc == numVerts) && (tc == numTris) );

	// Calling BSShape::clear here is bad
	verts.clear();
	norms.clear();
	tangents.clear();
	bitangents.clear();
	triangles.clear();
	coords.clear();
	colors.clear();
	test1.clear();
	test2.clear();
	test3.clear();

	// For compatibility with coords QList
	QVector<Vector2> coordset;

	for ( int i = 0; i < numVerts; i++ ) {
		auto idx = nif->index( i, 0, iVertData );

		auto v = nif->get<HalfVector3>( idx, "Vertex" );
		verts += v;

		coordset << nif->get<HalfVector2>( idx, "UV" );

		// Bitangent X
		auto bitX = nif->getValue( nif->getIndex( idx, "Bitangent X" ) ).toFloat();
		// Bitangent Y/Z
		auto bitYi = nif->getValue( nif->getIndex( idx, "Bitangent Y" ) ).toCount();
		auto bitZi = nif->getValue( nif->getIndex( idx, "Bitangent Z" ) ).toCount();
		auto bitY = (double( bitYi ) / 255.0) * 2.0 - 1.0;
		auto bitZ = (double( bitZi ) / 255.0) * 2.0 - 1.0;

		auto n = nif->get<ByteVector3>( idx, "Normal" );
		norms += n;

		auto t = nif->get<ByteVector3>( idx, "Tangent" );
		tangents += t;
		bitangents += Vector3( bitX, bitY, bitZ );
	}

	// Add coords as first set of QList
	coords.append( coordset );

	// TODO: Temporary vertex colors for now
	colors.fill( Color4( 1, 1, 1, 1 ), verts.count() );

	triangles = nif->getArray<Triangle>( iTriData );
	Q_ASSERT( numTris == triangles.count() );

	QVector<qint32> props = nif->getLinkArray( iBlock, "Properties" ) + nif->getLinkArray( iBlock, "BS Properties" );


	// TODO: Mostly copied from glmesh, combine them in Shape at some point
	for ( int i = 0; i < props.count(); i++ ) {
		QModelIndex iProp = nif->getBlock( props[i], "BSLightingShaderProperty" );

		if ( iProp.isValid() ) {

			if ( !bslsp )
				bslsp = properties.get<BSLightingShaderProperty>();
			
			if ( !bslsp )
				break;
			
			auto hasSF1 = [this]( ShaderFlags::SF1 flag ) {
				return bslsp->getFlags1() & flag;
			};

			auto hasSF2 = [this]( ShaderFlags::SF2 flag ) {
				return bslsp->getFlags2() & flag;
			};

			auto isST = [this]( ShaderFlags::ShaderType st ) {
				return bslsp->getShaderType() == st;
			};


			auto shaderType = nif->get<unsigned int>( iProp, "Skyrim Shader Type" );

			auto sf1 = nif->get<unsigned int>( iProp, "Shader Flags 1" );
			auto sf2 = nif->get<unsigned int>( iProp, "Shader Flags 2" );

			// Get all textures for shader property
			auto textures = nif->getArray<QString>( bslsp->getTextureSet(), "Textures" );

			bslsp->setShaderType( shaderType );
			bslsp->setFlags1( sf1 );
			bslsp->setFlags2( sf2 );

			bslsp->setAlpha( nif->get<float>( iProp, "Alpha" ) );

			depthTest = hasSF1( ShaderFlags::SLSF1_ZBuffer_Test );
			depthWrite = hasSF2( ShaderFlags::SLSF2_ZBuffer_Write );


			auto uvScale = nif->get<Vector2>( iProp, "UV Scale" );
			auto uvOffset = nif->get<Vector2>( iProp, "UV Offset" );

			bslsp->setUvScale( uvScale[0], uvScale[1] );
			bslsp->setUvOffset( uvOffset[0], uvOffset[1] );

			auto clampMode = nif->get<uint>( iProp, "Texture Clamp Mode" );

			bslsp->setClampMode( clampMode );

			// Specular
			if ( hasSF1( ShaderFlags::SLSF1_Specular ) ) {
				auto spC = nif->get<Color3>( iProp, "Specular Color" );
				auto spG = nif->get<float>( iProp, "Glossiness" );
				auto spS = nif->get<float>( iProp, "Specular Strength" );
				bslsp->setSpecular( spC, spG, spS );
			} else {
				bslsp->setSpecular( Color3( 0, 0, 0 ), 0, 0 );
			}

			bslsp->hasVertexAlpha = hasSF1( ShaderFlags::SLSF1_Vertex_Alpha );
			bslsp->hasVertexColors = hasSF2( ShaderFlags::SLSF2_Vertex_Colors );
			bslsp->hasSpecularMap = hasSF1( ShaderFlags::SLSF1_Specular ) && !textures.value( 7, "" ).isEmpty();
		}
	}
}


void BSShape::transform()
{
	const NifModel * nif = static_cast<const NifModel *>(iBlock.model());
	if ( !nif || !iBlock.isValid() ) {
		clear();
		return;
	}

	if ( updateData ) {
		
	}

	Node::transform();
}

void BSShape::transformShapes()
{
	Node::transformShapes();
}

void BSShape::drawShapes( NodeList * secondPass, bool presort )
{
	if ( Node::SELECTING ) {
		int s_nodeId = ID2COLORKEY( nodeId );
		glColor4ubv( (GLubyte *)&s_nodeId );
	}

	// Draw translucent meshes in second pass
	AlphaProperty * aprop = findProperty<AlphaProperty>();

	drawSecond |= (aprop && aprop->blend());

	if ( secondPass && drawSecond ) {
		secondPass->add( this );
		return;
	}

	glPushMatrix();
	glMultMatrix( viewTrans() );

	glEnable( GL_DEPTH_TEST );
	glDepthFunc( GL_LEQUAL );

	// Render polygon fill slightly behind alpha transparency and wireframe
	glEnable( GL_POLYGON_OFFSET_FILL );
	glPolygonOffset( 1.0f, 2.0f );

	glEnableClientState( GL_VERTEX_ARRAY );
	glVertexPointer( 3, GL_FLOAT, 0, verts.data() );

	if ( !Node::SELECTING ) {
		glEnableClientState( GL_NORMAL_ARRAY );
		glNormalPointer( GL_FLOAT, 0, norms.data() );

		Color4 * c = nullptr;
		if ( (scene->options & Scene::Test1) && test1.count() ) {
			//c = test1.data();
		} else if ( (scene->options & Scene::Test2) && test2.count() ) {
			//c = test2.data();
		} else if ( (scene->options & Scene::Test3) && test3.count() ) {
			//c = test3.data();
		} else if ( colors.count() ) {
			c = colors.data();
		}

		if ( c ) {
			glEnableClientState( GL_COLOR_ARRAY );
			glColorPointer( 4, GL_FLOAT, 0, c );
		}
	}


	if ( !Node::SELECTING )
		shader = scene->renderer->setupProgram( this, shader );

	if ( isDoubleSided )
		glDisable( GL_CULL_FACE );

	glDrawElements( GL_TRIANGLES, triangles.count() * 3, GL_UNSIGNED_SHORT, triangles.data() );


	if ( !Node::SELECTING )
		scene->renderer->stopProgram();

	glDisableClientState( GL_VERTEX_ARRAY );
	glDisableClientState( GL_NORMAL_ARRAY );
	glDisableClientState( GL_COLOR_ARRAY );

	glDisable( GL_POLYGON_OFFSET_FILL );

	glPopMatrix();
}

void BSShape::drawSelection() const
{
	Node::drawSelection();

	if ( isHidden() )
		return;

	if ( scene->currentBlock != iBlock )
		return;

	auto n = scene->currentIndex.data( NifSkopeDisplayRole ).toString();

	glPushMatrix();
	glMultMatrix( viewTrans() );

	glDepthFunc( GL_LEQUAL );

	glDisable( GL_LIGHTING );
	glDisable( GL_COLOR_MATERIAL );
	glDisable( GL_TEXTURE_2D );
	glDisable( GL_NORMALIZE );
	glEnable( GL_DEPTH_TEST );
	glDepthMask( GL_FALSE );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glDisable( GL_ALPHA_TEST );

	glDisable( GL_CULL_FACE );

	glLineWidth( 1.5 );
	glPointSize( 5.0 );

	float normalScale = bounds().radius / 20;
	normalScale /= 2.0f;

	if ( normalScale < 0.1f )
		normalScale = 0.1f;

	bool vp = scene->currentIndex.parent().data( NifSkopeDisplayRole ).toString() == "Vertex Data";

	// Draw All Verts lambda
	auto allv = [this]( float size ) {
		glPointSize( size );
		glBegin( GL_POINTS );

		for ( int j = 0; j < verts.count(); j++ )
			glVertex( verts.value( j ) );

		glEnd();
	};

	if ( n == "Vertex Data" || n == "Vertex" ) {
		allv( 5.0 );

		int s = -1;
		if ( n == "Vertex Data" && vp ) {
			s = scene->currentIndex.row();
		} else if ( n == "Vertex" ) {
			s = scene->currentIndex.parent().row();
		}

		if ( s >= 0 ) {
			glPointSize( 10 );
			glDepthFunc( GL_ALWAYS );
			glColor4f( 1, 0, 0, 1 );
			glBegin( GL_POINTS );
			glVertex( verts.value( s ) );
			glEnd();
		}
	} 
	
	glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

	// Draw Lines lambda
	auto lines = [this, &normalScale, &allv]( const QVector<Vector3> & v ) {
		allv( 7.0 );

		int s = scene->currentIndex.parent().row();
		glBegin( GL_LINES );

		for ( int j = 0; j < verts.count() && j < v.count(); j++ ) {
			glVertex( verts.value( j ) );
			glVertex( verts.value( j ) + v.value( j ) * normalScale * 2 );
			glVertex( verts.value( j ) );
			glVertex( verts.value( j ) - v.value( j ) * normalScale / 2 );
		}

		glEnd();

		if ( s >= 0 ) {
			glDepthFunc( GL_ALWAYS );
			glColor4f( 1, 0, 0, 1 );
			glBegin( GL_LINES );
			glVertex( verts.value( s ) );
			glVertex( verts.value( s ) + v.value( s ) * normalScale * 2 );
			glVertex( verts.value( s ) );
			glVertex( verts.value( s ) - v.value( s ) * normalScale / 2 );
			glEnd();
		}
	};
	
	if ( n.contains( "Normal" ) ) {
		lines( norms );
	}

	if ( n.contains( "Tangent" ) ) {
		lines( tangents );
	}

	if ( (scene->currentIndex != iVertData) && !vp ) {
		glLineWidth( 1.6f );

		for ( const Triangle& tri : triangles ) {
			glBegin( GL_TRIANGLES );
			glVertex( verts.value( tri.v1() ) );
			glVertex( verts.value( tri.v2() ) );
			glVertex( verts.value( tri.v3() ) );
			glEnd();
		}
	}

	glPopMatrix();
}

BoundSphere BSShape::bounds() const
{
	if ( updateBounds ) {
		updateBounds = false;
		boundSphere = BoundSphere( verts );
	}

	return worldTrans() * boundSphere;
}

bool BSShape::isHidden() const
{
	return false;
}
