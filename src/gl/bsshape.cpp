#include "bsshape.h"

#include "gl/glnode.h"
#include "gl/glscene.h"
#include "gl/renderer.h"
#include "io/material.h"
#include "model/nifmodel.h"


BSShape::BSShape( Scene * s, const QModelIndex & b ) : Shape( s, b )
{
	
}

void BSShape::clear()
{
	Node::clear();

	iSkin = iSkinData = iSkinPart = QModelIndex();
	bssp = nullptr;
	bslsp = nullptr;
	bsesp = nullptr;

	verts.clear();
	norms.clear();
	tangents.clear();
	bitangents.clear();
	triangles.clear();
	coords.clear();
	colors.clear();
	bones.clear();
	weights.clear();
}

void BSShape::update( const NifModel * nif, const QModelIndex & index )
{
	Shape::update( nif, index );

	if ( !iBlock.isValid() || !index.isValid() )
		return;

	bool extraData = nif->inherits( index, "NiProperty" ) || nif->inherits( index, "BSShaderTextureSet" );

	if ( iBlock != index && iSkin != index && iSkinData != index && !extraData )
		return;

	// Update shaders from this mesh's shader property
	updateShaderProperties( nif );

	if ( extraData )
		return;

	nifVersion = nif->getUserVersion2();

	isLOD = nif->isNiBlock( iBlock, "BSMeshLODTriShape" );
	if ( isLOD )
		emit nif->lodSliderChanged( true );

	auto vertexFlags = nif->get<BSVertexDesc>( iBlock, "Vertex Desc" );

	hasVertexColors = vertexFlags.HasFlag( VertexAttribute::VA_COLOR );

	bool isDynamic = nif->inherits( iBlock, "BSDynamicTriShape" );

	bool isDataOnSkin = false;
	bool isSkinned = vertexFlags & VertexFlags::VF_SKINNED;
	if ( nifVersion == 130 ) {
		skinInstName = "BSSkin::Instance";
		skinDataName = "BSSkin::BoneData";
	} else {
		skinInstName = "NiSkinInstance";
		skinDataName = "NiSkinData";
	}

	iSkin = nif->getBlock( nif->getLink( nif->getIndex( iBlock, "Skin" ) ), skinInstName );
	if ( !iSkin.isValid() )
		isSkinned = false;

	if ( isSkinned ) {
		iSkinData = nif->getBlock( nif->getLink( iSkin, "Data" ), skinDataName );
		iSkinPart = nif->getBlock( nif->getLink( iSkin, "Skin Partition" ), "NiSkinPartition" );

		if ( nifVersion == 100 )
			isDataOnSkin = true;
	}

	updateData = true;
	updateBounds |= updateData;
	updateSkin = isSkinned;
	transformRigid = !isSkinned;

	int dataSize = 0;
	if ( !isDataOnSkin ) {
		iVertData = nif->getIndex( iBlock, "Vertex Data" );
		iTriData = nif->getIndex( iBlock, "Triangles" );
		iData = iVertData;
		if ( !iVertData.isValid() || !iTriData.isValid() )
			return;

		numVerts = std::min( nif->get<int>( iBlock, "Num Vertices" ), nif->rowCount( iVertData ) );
		numTris = std::min( nif->get<int>( iBlock, "Num Triangles" ), nif->rowCount( iTriData ) );

		dataSize = nif->get<int>( iBlock, "Data Size" );
	} else {
		// For skinned geometry, the vertex data is stored on the NiSkinPartition
		// The triangles are split up among the partitions

		iSkinPart = nif->getBlock( nif->getLink( iSkin, "Skin Partition" ), "NiSkinPartition" );
		if ( !iSkinPart.isValid() )
			return;

		iVertData = nif->getIndex( iSkinPart, "Vertex Data" );
		iTriData = QModelIndex();
		iData = iVertData;

		dataSize = nif->get<int>( iSkinPart, "Data Size" );
		auto vertexSize = nif->get<int>( iSkinPart, "Vertex Size" );
		if ( !iVertData.isValid() || dataSize == 0 || vertexSize == 0 )
			return;

		numVerts = dataSize / vertexSize;
	}


	auto bsphere = nif->getIndex( iBlock, "Bounding Sphere" );
	if ( bsphere.isValid() ) {
		bsphereCenter = nif->get<Vector3>( bsphere, "Center" );
		bsphereRadius = nif->get<float>( bsphere, "Radius" );
	}

	if ( iBlock == index && dataSize > 0 ) {
		verts.clear();
		norms.clear();
		tangents.clear();
		bitangents.clear();
		triangles.clear();
		coords.clear();
		colors.clear();

		// For compatibility with coords list
		TexCoords coordset;

		for ( int i = 0; i < numVerts; i++ ) {
			auto idx = nif->index( i, 0, iVertData );

			if ( !isDynamic )
				verts << nif->get<Vector3>( idx, "Vertex" );

			coordset << nif->get<HalfVector2>( idx, "UV" );

			// Bitangent X
			auto bitX = nif->getValue( nif->getIndex( idx, "Bitangent X" ) ).toFloat();
			// Bitangent Y/Z
			auto bitYi = nif->getValue( nif->getIndex( idx, "Bitangent Y" ) ).toCount();
			auto bitZi = nif->getValue( nif->getIndex( idx, "Bitangent Z" ) ).toCount();
			auto bitY = (double( bitYi ) / 255.0) * 2.0 - 1.0;
			auto bitZ = (double( bitZi ) / 255.0) * 2.0 - 1.0;

			norms += nif->get<ByteVector3>( idx, "Normal" );
			tangents += nif->get<ByteVector3>( idx, "Tangent" );
			bitangents += Vector3( bitX, bitY, bitZ );

			auto vcIdx = nif->getIndex( idx, "Vertex Colors" );
			if ( vcIdx.isValid() ) {
				colors += nif->get<ByteColor4>( vcIdx );
			}
		}

		if ( isDynamic ) {
			auto dynVerts = nif->getArray<Vector4>( iBlock, "Vertices" );
			for ( const auto & v : dynVerts )
				verts << Vector3( v );
		}

		// Add coords as first set of QList
		coords.append( coordset );

		if ( !isDataOnSkin ) {
			triangles = nif->getArray<Triangle>( iTriData );
			triangles = triangles.mid( 0, numTris );
		} else {
			auto partIdx = nif->getIndex( iSkinPart, "Partition" );
			for ( int i = 0; i < nif->rowCount( partIdx ); i++ )
				triangles << nif->getArray<Triangle>( nif->index( i, 0, partIdx ), "Triangles" );
		}
	}

	if ( bssp )
		isVertexAlphaAnimation = bssp->hasSF2( ShaderFlags::SLSF2_Tree_Anim );

	if ( isVertexAlphaAnimation ) {
		for ( int i = 0; i < colors.count(); i++ )
			colors[i].setRGBA( colors[i].red(), colors[i].green(), colors[i].blue(), 1.0 );
	}
}

QModelIndex BSShape::vertexAt( int idx ) const
{
	auto nif = static_cast<const NifModel *>(iBlock.model());
	if ( !nif )
		return QModelIndex();

	// Vertices are on NiSkinPartition in version 100
	auto blk = iBlock;
	if ( nifVersion < 130 && iSkinPart.isValid() ) {
		if ( nif->inherits( blk, "BSDynamicTriShape" ) )
			return nif->getIndex( blk, "Vertices" ).child( idx, 0 );

		blk = iSkinPart;
	}

	return nif->getIndex( nif->getIndex( blk, "Vertex Data" ).child( idx, 0 ), "Vertex" );
}


void BSShape::transform()
{
	if ( isHidden() )
		return;

	const NifModel * nif = static_cast<const NifModel *>(iBlock.model());
	if ( !nif || !iBlock.isValid() ) {
		clear();
		return;
	}	
	
	if ( updateData ) {
		updateData = false;
	}

	if ( updateSkin ) {
		updateSkin = false;
		isSkinned = false;

		bones.clear();
		weights.clear();
		partitions.clear();

		if ( iSkin.isValid() && iSkinData.isValid() ) {
			skeletonRoot = nif->getLink( iSkin, "Skeleton Root" );
			if ( nifVersion < 130 )
				skeletonTrans = Transform( nif, iSkinData );

			bones = nif->getLinkArray( iSkin, "Bones" );
			weights.fill( BoneWeights(), bones.count() );
			for ( int i = 0; i < bones.count(); i++ )
				weights[i].bone = bones[i];

			for ( int i = 0; i < numVerts; i++ ) {
				auto idx = nif->index( i, 0, iVertData );
				auto wts = nif->getArray<float>( idx, "Bone Weights" );
				auto bns = nif->getArray<quint8>( idx, "Bone Indices" );
				if ( wts.count() < 4 || bns.count() < 4 )
					continue;

				for ( int j = 0; j < 4; j++ ) {
					if ( bns[j] >= weights.count() )
						continue;

					if ( wts[j] > 0.0 )
						weights[bns[j]].weights << VertexWeight( i, wts[j] );
				}
			}

			auto b = nif->getIndex( iSkinData, "Bone List" );
			for ( int i = 0; i < weights.count(); i++ )
				weights[i].setTransform( nif, b.child( i, 0 ) );

			isSkinned = weights.count();
		}
	}

	Node::transform();
}

void BSShape::transformShapes()
{
	if ( isHidden() )
		return;

	const NifModel * nif = static_cast<const NifModel *>(iBlock.model());
	if ( !nif || !iBlock.isValid() ) {
		clear();
		return;
	}

	Node::transformShapes();

	transformRigid = true;

	if ( isSkinned && scene->options & Scene::DoSkinning ) {
		transformRigid = false;

		int vcnt = verts.count();

		transVerts.resize( vcnt );
		transVerts.fill( Vector3() );
		transNorms.resize( vcnt );
		transNorms.fill( Vector3() );
		transTangents.resize( vcnt );
		transTangents.fill( Vector3() );
		transBitangents.resize( vcnt );
		transBitangents.fill( Vector3() );


		Node * root = findParent( 0 );
		for ( const BoneWeights & bw : weights ) {
			Node * bone = root ? root->findChild( bw.bone ) : nullptr;
			if ( bone ) {
				Transform t = scene->view * bone->localTrans( 0 ) * bw.trans;
				for ( const VertexWeight & w : bw.weights ) {
					if ( w.vertex >= vcnt )
						continue;

					transVerts[w.vertex] += t * verts[w.vertex] * w.weight;
					transNorms[w.vertex] += t.rotation * norms[w.vertex] * w.weight;
					transTangents[w.vertex] += t.rotation * tangents[w.vertex] * w.weight;
					transBitangents[w.vertex] += t.rotation * bitangents[w.vertex] * w.weight;
				}
			}
		}

		for ( int n = 0; n < vcnt; n++ ) {
			transNorms[n].normalize();
			transTangents[n].normalize();
			transBitangents[n].normalize();
		}

		boundSphere = BoundSphere( transVerts );
		boundSphere.applyInv( viewTrans() );
		updateBounds = false;
	} else {
		transVerts = verts;
		transNorms = norms;
		transTangents = tangents;
		transBitangents = bitangents;
	}

	transColors = colors;
	if ( bslsp ) {
		if ( !(bslsp->getFlags1() & ShaderFlags::SLSF1_Vertex_Alpha) ) {
			for ( int c = 0; c < colors.count(); c++ )
				transColors[c] = Color4( colors[c].red(), colors[c].green(), colors[c].blue(), 1.0f );
		}
	}
}

void BSShape::drawShapes( NodeList * secondPass, bool presort )
{
	if ( isHidden() )
		return;

	glPointSize( 8.5 );

	// TODO: Only run this if BSXFlags has "EditorMarkers present" flag
	if ( !(scene->options & Scene::ShowMarkers) && name.contains( "EditorMarker" ) )
		return;

	auto nif = static_cast<const NifModel *>(iBlock.model());

	if ( Node::SELECTING ) {
		if ( scene->selMode & Scene::SelObject ) {
			int s_nodeId = ID2COLORKEY( nodeId );
			glColor4ubv( (GLubyte *)&s_nodeId );
		} else {
			glColor4f( 0, 0, 0, 1 );
		}
	}

	// Draw translucent meshes in second pass
	AlphaProperty * aprop = findProperty<AlphaProperty>();
	Material * mat = (bssp) ? bssp->mat() : nullptr;

	drawSecond |= aprop && aprop->blend();
	drawSecond |= mat && mat->bDecal;

	if ( secondPass && drawSecond ) {
		secondPass->add( this );
		return;
	}

	if ( transformRigid ) {
		glPushMatrix();
		glMultMatrix( viewTrans() );
	}

	// Render polygon fill slightly behind alpha transparency and wireframe
	if ( !drawSecond ) {
		glEnable( GL_POLYGON_OFFSET_FILL );
		glPolygonOffset( 1.0f, 2.0f );
	}

	glEnableClientState( GL_VERTEX_ARRAY );
	glVertexPointer( 3, GL_FLOAT, 0, transVerts.constData() );

	if ( !Node::SELECTING ) {
		glEnableClientState( GL_NORMAL_ARRAY );
		glNormalPointer( GL_FLOAT, 0, transNorms.constData() );

		bool doVCs = (bssp && (bssp->getFlags2() & ShaderFlags::SLSF2_Vertex_Colors));
		// Always do vertex colors for FO4 if colors present
		if ( nifVersion == 130 && hasVertexColors && colors.count() )
			doVCs = true;

		if ( transColors.count() && (scene->options & Scene::DoVertexColors) && doVCs ) {
			glEnableClientState( GL_COLOR_ARRAY );
			glColorPointer( 4, GL_FLOAT, 0, transColors.constData() );
		} else if ( !hasVertexColors && (bslsp && bslsp->hasVertexColors) ) {
			// Correctly blacken the mesh if SLSF2_Vertex_Colors is still on
			//	yet "Has Vertex Colors" is not.
			glColor( Color3( 0.0f, 0.0f, 0.0f ) );
		} else {
			glColor( Color3( 1.0f, 1.0f, 1.0f ) );
		}
	}


	if ( !Node::SELECTING )
		shader = scene->renderer->setupProgram( this, shader );
	
	if ( isDoubleSided ) {
		glCullFace( GL_FRONT );
		glDrawElements( GL_TRIANGLES, triangles.count() * 3, GL_UNSIGNED_SHORT, triangles.constData() );
		glCullFace( GL_BACK );
	}

	if ( !isLOD ) {
		glDrawElements( GL_TRIANGLES, triangles.count() * 3, GL_UNSIGNED_SHORT, triangles.constData() );
	} else if ( triangles.count() ) {
		auto lod0 = nif->get<uint>( iBlock, "LOD0 Size" );
		auto lod1 = nif->get<uint>( iBlock, "LOD1 Size" );
		auto lod2 = nif->get<uint>( iBlock, "LOD2 Size" );

		auto lod0tris = triangles.mid( 0, lod0 );
		auto lod1tris = triangles.mid( lod0, lod1 );
		auto lod2tris = triangles.mid( lod0 + lod1, lod2 );

		// If Level2, render all
		// If Level1, also render Level0
		switch ( scene->lodLevel ) {
		case Scene::Level2:
			if ( lod2tris.count() )
				glDrawElements( GL_TRIANGLES, lod2tris.count() * 3, GL_UNSIGNED_SHORT, lod2tris.constData() );
		case Scene::Level1:
			if ( lod1tris.count() )
				glDrawElements( GL_TRIANGLES, lod1tris.count() * 3, GL_UNSIGNED_SHORT, lod1tris.constData() );
		case Scene::Level0:
		default:
			if ( lod0tris.count() )
				glDrawElements( GL_TRIANGLES, lod0tris.count() * 3, GL_UNSIGNED_SHORT, lod0tris.constData() );
			break;
		}
	}

	if ( !Node::SELECTING )
		scene->renderer->stopProgram();

	glDisableClientState( GL_VERTEX_ARRAY );
	glDisableClientState( GL_NORMAL_ARRAY );
	glDisableClientState( GL_COLOR_ARRAY );

	glDisable( GL_POLYGON_OFFSET_FILL );


	if ( scene->selMode & Scene::SelVertex ) {
		drawVerts();
	}

	if ( transformRigid )
		glPopMatrix();
}

void BSShape::drawVerts() const
{
	glDisable( GL_LIGHTING );
	glNormalColor();

	glBegin( GL_POINTS );

	for ( int i = 0; i < numVerts; i++ ) {
		if ( Node::SELECTING ) {
			int id = ID2COLORKEY( ( shapeNumber << 16 ) + i );
			glColor4ubv( (GLubyte *)&id );
		}
		glVertex( transVerts.value( i ) );
	}

	auto nif = static_cast<const NifModel *>(iBlock.model());
	if ( !nif )
		return;

	// Vertices are on NiSkinPartition in version 100
	bool selected = iBlock == scene->currentBlock;
	if ( nifVersion < 130 && iSkinPart.isValid() ) {
		selected |= iSkinPart == scene->currentBlock;
		selected |= nif->inherits( iBlock, "BSDynamicTriShape" );
	}


	// Highlight selected vertex
	if ( !Node::SELECTING && selected ) {
		auto idx = scene->currentIndex;
		auto n = idx.data( Qt::DisplayRole ).toString();
		if ( n == "Vertex" || n == "Vertices" ) {
			glHighlightColor();
			glVertex( transVerts.value( idx.parent().row() ) );
		}
	}

	glEnd();
}

void BSShape::drawSelection() const
{
	if ( scene->options & Scene::ShowNodes )
		Node::drawSelection();

	if ( isHidden() || !(scene->selMode & Scene::SelObject) )
		return;

	auto idx = scene->currentIndex;
	auto blk = scene->currentBlock;

	// Is the current block extra data
	bool extraData = false;

	auto nif = static_cast<const NifModel *>(idx.model());
	if ( !nif || !blk.isValid() )
		return;

	// Set current block name and detect if extra data
	auto blockName = nif->getBlockName( blk );
	if ( blockName.startsWith( "BSPackedCombined" ) )
		extraData = true;

	// Don't do anything if this block is not the current block
	//	or if there is not extra data
	if ( blk != iBlock && blk != iSkin && blk != iSkinData && blk != iSkinPart && !extraData )
		return;

	// Name of this index
	auto n = idx.data( NifSkopeDisplayRole ).toString();
	// Name of this index's parent
	auto p = idx.parent().data( NifSkopeDisplayRole ).toString();
	// Parent index
	auto pBlock = nif->getBlock( nif->getParent( blk ) );

	auto push = [this] ( const Transform & t ) {	
		if ( transformRigid ) {
			glPushMatrix();
			glMultMatrix( t );
		}
	};

	auto pop = [this] () {
		if ( transformRigid )
			glPopMatrix();
	};

	push( viewTrans() );

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

	// TODO: User Settings
	GLfloat lineWidth = 1.5;
	GLfloat pointSize = 5.0;

	glLineWidth( lineWidth );
	glPointSize( pointSize );

	glNormalColor();

	glEnable( GL_POLYGON_OFFSET_FILL );
	glPolygonOffset( -1.0f, -2.0f );

	float normalScale = bounds().radius / 20;
	normalScale /= 2.0f;

	if ( normalScale < 0.1f )
		normalScale = 0.1f;

	

	// Draw All Verts lambda
	auto allv = [this]( float size ) {
		glPointSize( size );
		glBegin( GL_POINTS );

		for ( int j = 0; j < transVerts.count(); j++ )
			glVertex( transVerts.value( j ) );

		glEnd();
	};

	if ( n == "Bounding Sphere" && !extraData ) {
		auto sph = BoundSphere( nif, idx );
		if ( sph.radius > 0.0 ) {
			glColor4f( 1, 1, 1, 0.33 );
			drawSphereSimple( sph.center, sph.radius, 72 );
		}
	}
	
	if ( blockName.startsWith( "BSPackedCombined" ) && pBlock == iBlock ) {
		QVector<QModelIndex> idxs;
		if ( n == "Bounding Sphere" ) {
			idxs += idx;
		} else if ( n.startsWith( "BSPackedCombined" ) ) {
			auto data = nif->getIndex( idx, "Object Data" );
			int dataCt = nif->rowCount( data );

			for ( int i = 0; i < dataCt; i++ ) {
				auto d = data.child( i, 0 );

				int numC = nif->get<int>( d, "Num Combined" );
				auto c = nif->getIndex( d, "Combined" );
				int cCt = nif->rowCount( c );

				for ( int j = 0; j < cCt; j++ ) {
					idxs += nif->getIndex( c.child( j, 0 ), "Bounding Sphere" );
				}
			}
		}

		if ( !idxs.count() ) {
			glPopMatrix();
			return;
		}

		Vector3 pTrans = nif->get<Vector3>( pBlock.child( 1, 0 ), "Translation" );
		auto iBSphere = nif->getIndex( pBlock, "Bounding Sphere" );
		Vector3 pbvC = nif->get<Vector3>( iBSphere.child( 0, 2 ) );
		float pbvR = nif->get<float>( iBSphere.child( 1, 2 ) );

		if ( pbvR > 0.0 ) {
			glColor4f( 0, 1, 0, 0.33 );
			drawSphereSimple( pbvC, pbvR, 72 );
		}

		glPopMatrix();

		for ( auto i : idxs ) {
			// Transform compound
			auto iTrans = i.parent().child( 1, 0 );
			Matrix mat = nif->get<Matrix>( iTrans, "Rotation" );
			//auto trans = nif->get<Vector3>( iTrans, "Translation" );
			float scale = nif->get<float>( iTrans, "Scale" );

			Vector3 bvC = nif->get<Vector3>( i, "Center" );
			float bvR = nif->get<float>( i, "Radius" );

			Transform t;
			t.rotation = mat.inverted();
			t.translation = bvC;
			t.scale = scale;

			glPushMatrix();
			glMultMatrix( scene->view * t );

			if ( bvR > 0.0 ) {
				glColor4f( 1, 1, 1, 0.33 );
				drawSphereSimple( Vector3( 0, 0, 0 ), bvR, 72 );
			}

			glPopMatrix();
		}

		glPushMatrix();
		glMultMatrix( viewTrans() );
	}

	if ( n == "Vertex Data" || n == "Vertex" || n == "Vertices" ) {
		allv( 5.0 );

		int s = -1;
		if ( (n == "Vertex Data" && p == "Vertex Data")
			 || (n == "Vertices" && p == "Vertices") ) {
			s = idx.row();
		} else if ( n == "Vertex" ) {
			s = idx.parent().row();
		}

		if ( s >= 0 ) {
			glPointSize( 10 );
			glDepthFunc( GL_ALWAYS );
			glHighlightColor();
			glBegin( GL_POINTS );
			glVertex( transVerts.value( s ) );
			glEnd();
		}
	} 
	
	glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

	// Draw Lines lambda
	auto lines = [this, &normalScale, &allv, &lineWidth]( const QVector<Vector3> & v ) {
		allv( 7.0 );

		int s = scene->currentIndex.parent().row();
		glBegin( GL_LINES );

		for ( int j = 0; j < transVerts.count() && j < v.count(); j++ ) {
			glVertex( transVerts.value( j ) );
			glVertex( transVerts.value( j ) + v.value( j ) * normalScale * 2 );
			glVertex( transVerts.value( j ) );
			glVertex( transVerts.value( j ) - v.value( j ) * normalScale / 2 );
		}

		glEnd();

		if ( s >= 0 ) {
			glDepthFunc( GL_ALWAYS );
			glHighlightColor();
			glLineWidth( 3.0 );
			glBegin( GL_LINES );
			glVertex( transVerts.value( s ) );
			glVertex( transVerts.value( s ) + v.value( s ) * normalScale * 2 );
			glVertex( transVerts.value( s ) );
			glVertex( transVerts.value( s ) - v.value( s ) * normalScale / 2 );
			glEnd();
			glLineWidth( lineWidth );
		}
	};
	
	// Draw Normals
	if ( n.contains( "Normal" ) ) {
		lines( transNorms );
	}

	// Draw Tangents
	if ( n.contains( "Tangent" ) ) {
		lines( transTangents );
	}

	// Draw Triangles
	if ( n == "Triangles" ) {
		int s = scene->currentIndex.row();
		if ( s >= 0 ) {
			glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
			glHighlightColor();

			Triangle tri = triangles.value( s );
			glBegin( GL_TRIANGLES );
			glVertex( transVerts.value( tri.v1() ) );
			glVertex( transVerts.value( tri.v2() ) );
			glVertex( transVerts.value( tri.v3() ) );
			glEnd();
			glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		}
	}

	// Draw Segments/Subsegments
	if ( n == "Segment" || n == "Sub Segment" || n == "Num Primitives" ) {
		auto sidx = idx;
		int s;

		QVector<QColor> cols = { { 255, 0, 0, 128 }, { 0, 255, 0, 128 }, { 0, 0, 255, 128 }, { 255, 255, 0, 128 },
								{ 0, 255, 255, 128 }, { 255, 0, 255, 128 }, { 255, 255, 255, 128 } 
		};

		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

		auto type = idx.sibling( idx.row(), 1 ).data( Qt::DisplayRole ).toString();

		bool isSegmentArray = (n == "Segment" && type == "BSGeometrySegmentData" && nif->isArray( idx ));
		bool isSegmentItem = (n == "Segment" && type == "BSGeometrySegmentData" && !nif->isArray( idx ));
		bool isSubSegArray = (n == "Sub Segment" && nif->isArray( idx ));

		int off = 0;
		int cnt = 0;
		int numRec = 0;

		int o = 0;
		if ( isSegmentItem || isSegmentArray ) {
			o = 3; // Offset 3 rows for < 130 BSGeometrySegmentData
		} else if ( isSubSegArray ) {
			o = -3; // Look 3 rows above for Sub Seg Array info
		}

		int maxTris = triangles.count();

		int loopNum = 1;
		if ( isSegmentArray )
			loopNum = nif->rowCount( idx );

		for ( int l = 0; l < loopNum; l++ ) {

			if ( n != "Num Primitives" && !isSubSegArray && !isSegmentArray ) {
				sidx = idx.child( 1, 0 );
			} else if ( isSegmentArray ) {
				sidx = idx.child( l, 0 ).child( 1, 0 );
			}
			s = sidx.row() + o;

			off = sidx.sibling( s - 1, 2 ).data().toInt() / 3;
			cnt = sidx.sibling( s, 2 ).data().toInt();
			numRec = sidx.sibling( s + 2, 2 ).data().toInt();

			auto recs = sidx.sibling( s + 3, 0 );
			for ( int i = 0; i < numRec; i++ ) {
				auto subrec = recs.child( i, 0 );
				int o = 0;
				if ( subrec.data( Qt::DisplayRole ).toString() != "Sub Segment" )
					o = 3; // Offset 3 rows for < 130 BSGeometrySegmentData

				auto suboff = subrec.child( o, 2 ).data().toInt() / 3;
				auto subcnt = subrec.child( o + 1, 2 ).data().toInt();

				for ( int j = suboff; j < subcnt + suboff; j++ ) {
					if ( j >= maxTris )
						continue;

					glColor( Color4( cols.value( i % 7 ) ) );
					Triangle tri = triangles[j];
					glBegin( GL_TRIANGLES );
					glVertex( transVerts.value( tri.v1() ) );
					glVertex( transVerts.value( tri.v2() ) );
					glVertex( transVerts.value( tri.v3() ) );
					glEnd();
				}
			}

			// Sub-segmentless Segments
			if ( numRec == 0 && cnt > 0 ) {
				glColor( Color4( cols.value( (idx.row() + l) % 7 ) ) );

				for ( int i = off; i < cnt + off; i++ ) {
					if ( i >= maxTris )
						continue;
			
					Triangle tri = triangles[i];
					glBegin( GL_TRIANGLES );
					glVertex( transVerts.value( tri.v1() ) );
					glVertex( transVerts.value( tri.v2() ) );
					glVertex( transVerts.value( tri.v3() ) );
					glEnd();
				}
			}
		}

		pop();
		return;
	}

	// Draw all bones' bounding spheres
	if ( n == "NiSkinData" || n == "BSSkin::BoneData" ) {
		// Get shape block
		if ( nif->getBlock( nif->getParent( nif->getParent( blk ) ) ) == iBlock ) {
			auto iBones = nif->getIndex( blk, "Bone List" );
			int ct = nif->rowCount( iBones );

			for ( int i = 0; i < ct; i++ ) {
				auto b = iBones.child( i, 0 );
				boneSphere( nif, b );
			}
		}
		pop();
		return;
	}

	// Draw bone bounding sphere
	if ( n == "Bone List" ) {
		if ( nif->isArray( idx ) ) {
			for ( int i = 0; i < nif->rowCount( idx ); i++ )
				boneSphere( nif, idx.child( i, 0 ) );
		} else {
			boneSphere( nif, idx );
		}
	}

	// General wireframe
	if ( blk == iBlock && idx != iVertData && p != "Vertex Data" && p != "Vertices" ) {
		glLineWidth( 1.6f );
		glNormalColor();
		for ( const Triangle& tri : triangles ) {
			glBegin( GL_TRIANGLES );
			glVertex( transVerts.value( tri.v1() ) );
			glVertex( transVerts.value( tri.v2() ) );
			glVertex( transVerts.value( tri.v3() ) );
			glEnd();
		}
	}

	glDisable( GL_POLYGON_OFFSET_FILL );

	pop();
}

BoundSphere BSShape::bounds() const
{
	if ( updateBounds ) {
		updateBounds = false;
		if ( verts.count() ) {
			boundSphere = BoundSphere( verts );
		} else {
			boundSphere = BoundSphere( bsphereCenter, bsphereRadius );
		}
	}

	return worldTrans() * boundSphere;
}

bool BSShape::isHidden() const
{
	return Node::isHidden();
}
