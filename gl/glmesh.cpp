/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005-2009, NIF File Format Library and Tools
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

#include "glscene.h"
#include "glmesh.h"
#include "glcontroller.h"
#include "gltools.h"
#include "../options.h"

#include <GL/glext.h>

#include <QDebug>

//! \file glmesh.cpp Mesh, MorphController, UVController

//! A Controller of Mesh geometry
class MorphController : public Controller
{
	//! A representation of Mesh geometry morphs
	struct MorphKey
	{
		QPersistentModelIndex iFrames;
		QVector<Vector3> verts;
		int index;
	};
	
public:
	MorphController( Mesh * mesh, const QModelIndex & index )
		: Controller( index ), target( mesh ) {}
	
	~MorphController()
	{
		qDeleteAll( morph );
	}
	
	void update( float time )
	{
		if ( ! ( target && iData.isValid() && active && morph.count() > 1 ) )
			return;
		
		time = ctrlTime( time );
		
		if ( target->verts.count() != morph[0]->verts.count() )
			return;
		
		target->verts = morph[0]->verts;
		
		float x;
		
		for ( int i = 1; i < morph.count(); i++ )
		{
			MorphKey * key = morph[i];
			if ( interpolate( x, key->iFrames, time, key->index ) )
			{
				if ( x < 0 ) x = 0;
				if ( x > 1 ) x = 1;
				
				if ( x != 0 && target->verts.count() == key->verts.count() )
				{
					for ( int v = 0; v < target->verts.count(); v++ )
						target->verts[v] += key->verts[v] * x;
				}
			}
		}
		
		target->upBounds = true;
	}
	
	bool update( const NifModel * nif, const QModelIndex & index )
	{
		if ( Controller::update( nif, index ) )
		{
			qDeleteAll( morph );
			morph.clear();
			
			QModelIndex iInterpolators = nif->getIndex( iBlock, "Interpolators" );
			
			QModelIndex midx = nif->getIndex( iData, "Morphs" );
			for ( int r = 0; r < nif->rowCount( midx ); r++ )
			{
				QModelIndex iKey = midx.child( r, 0 );
				
				MorphKey * key = new MorphKey;
				key->index = 0;
				if ( iInterpolators.isValid() )
					key->iFrames = nif->getIndex( nif->getBlock( nif->getLink( nif->getBlock( nif->getLink( iInterpolators.child( r, 0 ) ), "NiFloatInterpolator" ), "Data" ), "NiFloatData" ), "Data" );
				else
					key->iFrames = iKey;
				key->verts = nif->getArray<Vector3>( nif->getIndex( iKey, "Vectors" ) );
				
				morph.append( key );
			}
			return true;
		}
		return false;
	}

protected:
	QPointer<Mesh> target;
	QVector<MorphKey*>	morph;
};

//! A Controller of UV data in a Mesh
class UVController : public Controller
{
public:
	UVController( Mesh * mesh, const QModelIndex & index )
		: Controller( index ), target( mesh ) {}

	~UVController() {}

	void update( float time )
	{
		const NifModel * nif = static_cast<const NifModel *>( iData.model() );
		QModelIndex uvGroups = nif->getIndex( iData, "UV Groups" );

		// U trans, V trans, U scale, V scale
		// see NiUVData compound in nif.xml
		float val[4] = { 0.0, 0.0, 1.0, 1.0 };
		if ( uvGroups.isValid() )
		{
			for ( int i = 0; i < 4 && i < nif->rowCount( uvGroups ); i++ )
			{
				interpolate( val[i], uvGroups.child( i, 0 ), ctrlTime( time ), luv );
			}
			// adjust coords; verified in SceneImmerse
			for ( int i = 0; i < target->coords[0].size(); i++ )
			{
				// operating on pointers makes this too complicated, so we don't
				Vector2 current = target->coords[0][i];
				// scaling/tiling applied before translation
				// Note that scaling is relative to center!
				current += Vector2( -0.5, -0.5 );
				current = Vector2( current[0] * val[2], current[1] * val[3] );
				current += Vector2( -val[0], val[1] );
				current += Vector2( 0.5, 0.5 );
				target->coords[0][i] = current;
			}
		}
		target->upData = true;
	}

	bool update( const NifModel * nif, const QModelIndex & index )
	{
		if ( Controller::update( nif, index ) )
		{
			// do stuff here
			return true;
		}
		return false;
	}

protected:
	QPointer<Mesh> target;

	int luv;
};


/*
 *  Mesh
 */

void Mesh::clear()
{
	Node::clear();

	iData = iSkin = iSkinData = iSkinPart = iTangentData = QModelIndex();
	
	verts.clear();
	norms.clear();
	colors.clear();
	coords.clear();
	tangents.clear();
	binormals.clear();
	triangles.clear();
	tristrips.clear();
	weights.clear();
	partitions.clear();
	sortedTriangles.clear();
	transVerts.clear();
	transNorms.clear();
	transColors.clear();
	transTangents.clear();
	transBinormals.clear();
}

void Mesh::update( const NifModel * nif, const QModelIndex & index )
{
	Node::update( nif, index );
	
	if ( ! iBlock.isValid() || ! index.isValid() )
		return;
	
	upData |= ( iData == index ) || ( iTangentData == index );
	upSkin |= ( iSkin == index );
	upSkin |= ( iSkinData == index );
	upSkin |= ( iSkinPart == index );
	
	if ( iBlock == index )
	{
		foreach ( int link, nif->getChildLinks( id() ) )
		{
			QModelIndex iChild = nif->getBlock( link );
			if ( ! iChild.isValid() ) continue;
			QString name = nif->itemName( iChild );
			if ( nif->inherits(iChild, "NiTriShapeData") || nif->inherits(iChild, "NiTriStripsData" ) )
			{
				if ( ! iData.isValid() )
				{
					iData = iChild;
					upData = true;
				}
				else if ( iData != iChild )
				{
					qWarning() << "shape block" << id() << "has multiple data blocks";
				}
			}
			else if ( nif->inherits(iChild, "NiSkinInstance" ) )
			{
				if ( ! iSkin.isValid() )
				{
					iSkin = iChild;
					upSkin = true;
				}
				else if ( iSkin != iChild )
					qWarning() << "shape block" << id() << "has multiple skin instances";
			}
		}
	}
	
	upBounds |= upData;
}

void Mesh::setController( const NifModel * nif, const QModelIndex & iController )
{
	if ( nif->itemName( iController ) == "NiGeomMorpherController" )
	{
		Controller * ctrl = new MorphController( this, iController );
		ctrl->update( nif, iController );
		controllers.append( ctrl );
	}
	else if ( nif->itemName( iController ) == "NiUVController" )
	{
		Controller * ctrl = new UVController( this, iController );
		ctrl->update( nif, iController );
		controllers.append( ctrl );
	}
	else
		Node::setController( nif, iController );
}

bool Mesh::isHidden() const
{
	return ( Node::isHidden() 
          || ( ! Options::drawHidden() && Options::onlyTextured() 
            && ! properties.get< TexturingProperty >() 
            && ! properties.get< BSShaderLightingProperty >()
             ) 
          );
}

bool compareTriangles( const QPair< int, float > & tri1, const QPair< int, float > & tri2 )
{
	return ( tri1.second < tri2.second );
}

void Mesh::transform()
{
	const NifModel * nif = static_cast<const NifModel *>( iBlock.model() );
	if ( ! nif || ! iBlock.isValid() )
	{
		clear();
		return;
	}
	
	if ( upData )
	{
		upData = false;
		
		verts = nif->getArray<Vector3>( iData, "Vertices" );
		norms = nif->getArray<Vector3>( iData, "Normals" );
		colors = nif->getArray<Color4>( iData, "Vertex Colors" );
		tangents = nif->getArray<Vector3>( iData, "Tangents" );
		binormals = nif->getArray<Vector3>( iData, "Binormals" );
		
		if ( norms.count() < verts.count() ) norms.clear();
		if ( colors.count() < verts.count() ) colors.clear();
		
		coords.clear();
		QModelIndex uvcoord = nif->getIndex( iData, "UV Sets" );
		if ( ! uvcoord.isValid() ) uvcoord = nif->getIndex( iData, "UV Sets 2" );
		if ( uvcoord.isValid() )
		{
			for ( int r = 0; r < nif->rowCount( uvcoord ); r++ )
			{
				QVector<Vector2> tc = nif->getArray<Vector2>( uvcoord.child( r, 0 ) );
				if ( tc.count() < verts.count() ) tc.clear();
				coords.append( tc );
			}
		}
		
		if ( nif->itemName( iData ) == "NiTriShapeData" )
		{
			triangles = nif->getArray<Triangle>( iData, "Triangles" );
			tristrips.clear();
		}
		else if ( nif->itemName( iData ) == "NiTriStripsData" )
		{
			tristrips.clear();
			QModelIndex points = nif->getIndex( iData, "Points" );
			if ( points.isValid() )
			{
				for ( int r = 0; r < nif->rowCount( points ); r++ )
					tristrips.append( nif->getArray<quint16>( points.child( r, 0 ) ) );
			}
			else
				qWarning() << nif->itemName( iData ) << "(" << nif->getBlockNumber( iData ) << ") 'points' array not found";
			triangles.clear();
		}
		else
		{
			triangles.clear();
			tristrips.clear();
		}
		
		QModelIndex iExtraData = nif->getIndex( iBlock, "Extra Data List" );
		if ( iExtraData.isValid() )
		{
			for ( int e = 0; e < nif->rowCount( iExtraData ); e++ )
			{
				QModelIndex iExtra = nif->getBlock( nif->getLink( iExtraData.child( e, 0 ) ), "NiBinaryExtraData" );
				if ( nif->get<QString>( iExtra, "Name" ) == "Tangent space (binormal & tangent vectors)" )
				{
					iTangentData = iExtra;
					QByteArray data = nif->get<QByteArray>( iExtra, "Binary Data" );
					if ( data.count() == verts.count() * 4 * 3 * 2 )
					{
						tangents.resize( verts.count() );
						binormals.resize( verts.count() );
						Vector3 * t = (Vector3 *) data.data();
						for ( int c = 0; c < verts.count(); c++ )
							tangents[c] = *t++;
						for ( int c = 0; c < verts.count(); c++ )
							binormals[c] = *t++;
					}
				}
			}
		}	
	}
	
	if ( upSkin )
	{
		upSkin = false;
		weights.clear();		
		partitions.clear();
		
		iSkinData = nif->getBlock( nif->getLink( iSkin, "Data" ), "NiSkinData" );
		
		skelRoot = nif->getLink( iSkin, "Skeleton Root" );
		skelTrans = Transform( nif, iSkinData );
		
		bones = nif->getLinkArray( iSkin, "Bones" );
		
		QModelIndex idxBones = nif->getIndex( iSkinData, "Bone List" );
		if ( idxBones.isValid() )
		{
			for ( int b = 0; b < nif->rowCount( idxBones ) && b < bones.count(); b++ )
			{
				weights.append( BoneWeights( nif, idxBones.child( b, 0 ), bones[ b ] ) );
			}
		}
		
		iSkinPart = nif->getBlock( nif->getLink( iSkin, "Skin Partition" ), "NiSkinPartition" );
		if ( ! iSkinPart.isValid() )
			// nif versions < 10.2.0.0 have skin partition linked in the skin data block
			iSkinPart = nif->getBlock( nif->getLink( iSkinData, "Skin Partition" ), "NiSkinPartition" );
		if ( iSkinPart.isValid() )
		{
			QModelIndex idx = nif->getIndex( iSkinPart, "Skin Partition Blocks" );
			for ( int i = 0; i < nif->rowCount( idx ) && idx.isValid(); i++ )
			{
				partitions.append( SkinPartition( nif, idx.child( i, 0 ) ) );
			}
		}
	}
	
	Node::transform();
}

void Mesh::transformShapes()
{
	if ( isHidden() || !Options::drawMeshes() )
		return;
	
	Node::transformShapes();
	
	transformRigid = true;
	
	if ( weights.count() )
	{
		transformRigid = false;
		
		transVerts.resize( verts.count() );
		transVerts.fill( Vector3() );
		transNorms.resize( norms.count() );
		transNorms.fill( Vector3() );
		transTangents.resize( tangents.count() );
		transTangents.fill( Vector3() );
		transBinormals.resize( binormals.count() );
		transBinormals.fill( Vector3() );
		
		Node * root = findParent( skelRoot );
		
		if ( partitions.count() )
		{
			foreach ( SkinPartition part, partitions )
			{
				QVector<Transform> boneTrans( part.boneMap.count() );
				for ( int t = 0; t < boneTrans.count(); t++ )
				{
					Node * bone = root ? root->findChild( bones.value( part.boneMap[t] ) ) : 0;
					boneTrans[ t ] = viewTrans() * skelTrans;
					if ( bone ) boneTrans[ t ] = boneTrans[ t ] * bone->localTransFrom( skelRoot ) * weights.value( part.boneMap[t] ).trans;
					//if ( bone ) boneTrans[ t ] = bone->viewTrans() * weights.value( part.boneMap[t] ).trans;
				}
				
				for ( int v = 0; v < part.vertexMap.count(); v++ )
				{
					int vindex = part.vertexMap[ v ];
					if ( transVerts[vindex] == Vector3() )
					{
						for ( int w = 0; w < part.numWeightsPerVertex; w++ )
						{
							QPair< int, float > weight = part.weights[ v * part.numWeightsPerVertex + w ];
							Transform trans = boneTrans.value( weight.first );
							
							if ( verts.count() > vindex )
								transVerts[vindex] += trans * verts[ vindex ] * weight.second;
							if ( norms.count() > vindex )
								transNorms[vindex] += trans.rotation * norms[ vindex ] * weight.second;
							if ( tangents.count() > vindex )
								transTangents[vindex] += trans.rotation * tangents[ vindex ] * weight.second;
							if ( binormals.count() > vindex )
								transBinormals[vindex] += trans.rotation * binormals[ vindex ] * weight.second;
						}
					}
				}
			}
		}
		else
		{
			int x = 0;
			foreach ( BoneWeights bw, weights )
			{
				Transform trans = viewTrans() * skelTrans;
				Node * bone = root ? root->findChild( bw.bone ) : 0;
				if ( bone ) trans = trans * bone->localTransFrom( skelRoot ) * bw.trans;
				if ( bone ) weights[x++].tcenter = bone->viewTrans() * bw.center; else x++;
				
				Matrix natrix = trans.rotation;
				foreach ( VertexWeight vw, bw.weights )
				{
					if ( transVerts.count() > vw.vertex )
						transVerts[ vw.vertex ] += trans * verts[ vw.vertex ] * vw.weight;
					if ( transNorms.count() > vw.vertex )
						transNorms[ vw.vertex ] += natrix * norms[ vw.vertex ] * vw.weight;
					if ( transTangents.count() > vw.vertex )
						transTangents[ vw.vertex ] += natrix * tangents[ vw.vertex ] * vw.weight;
					if ( transBinormals.count() > vw.vertex )
						transBinormals[ vw.vertex ] += natrix * binormals[ vw.vertex ] * vw.weight;
				}
			}
		}
		
		for ( int n = 0; n < transNorms.count(); n++ )
			transNorms[n].normalize();
		for ( int t = 0; t < transTangents.count(); t++ )
			transTangents[t].normalize();
		for ( int t = 0; t < transBinormals.count(); t++ )
			transBinormals[t].normalize();
		
		bndSphere = BoundSphere( transVerts );
		bndSphere.applyInv( viewTrans() );
		upBounds = false;
	}
	else
	{
		transVerts = verts;
		transNorms = norms;
		transTangents = tangents;
		transBinormals = binormals;
	}
	
	/*
	//Commented this out because this appears from my tests to be an
	//incorrect understanding of the flag we previously called "sort."
	//Tests have shown that none of the games or official scene viewers
	//ever sort triangles, regarless of the path.  The triangles are always
	//drawn in the order they exist in the triangle array.
	
	AlphaProperty * alphaprop = findProperty<AlphaProperty>();
	
	if ( alphaprop && alphaprop->sort() )
	{
	
		

		QVector< QPair< int, float > > triOrder( triangles.count() );
		if ( transformRigid )
		{
			Transform vt = viewTrans();
			Vector3 ref = vt.rotation.inverted() * ( Vector3() - vt.translation ) / vt.scale;
			int t = 0;
			foreach ( Triangle tri, triangles )
			{
				triOrder[t] = QPair<int, float>( t, 0 - ( ref - verts.value( tri.v1() ) ).squaredLength() - ( ref - verts.value( tri.v2() ) ).squaredLength() - ( ref - verts.value( tri.v1() ) ).squaredLength() );
				t++;
			}
		}
		else
		{
			int t = 0;
			foreach ( Triangle tri, triangles )
			{
				QPair< int, float > tp;
				tp.first = t;
				tp.second = transVerts.value( tri.v1() )[2] + transVerts.value( tri.v2() )[2] + transVerts.value( tri.v3() )[2];
				triOrder[t++] = tp;
			}
		}
		qSort( triOrder.begin(), triOrder.end(), compareTriangles );
		sortedTriangles.resize( triangles.count() );
		for ( int t = 0; t < triangles.count(); t++ )
			sortedTriangles[t] = triangles[ triOrder[t].first ];
	}
	else
	{
	*/
		sortedTriangles = triangles;
	//}
	
	MaterialProperty * matprop = findProperty<MaterialProperty>();
	if ( matprop && matprop->alphaValue() != 1.0 )
	{
		float a = matprop->alphaValue();
		transColors.resize( colors.count() );
		for ( int c = 0; c < colors.count(); c++ )
			transColors[c] = colors[c].blend( a );
	}
	else
		transColors = colors;
}

BoundSphere Mesh::bounds() const
{
	if ( upBounds )
	{
		upBounds = false;
		bndSphere = BoundSphere( verts );
	}
	return worldTrans() * bndSphere;
}

void Mesh::drawShapes( NodeList * draw2nd )
{
	if ( isHidden() || !Options::drawMeshes() )
		return;
	
	glLoadName( nodeId );
	
	// draw transparent meshes during second run
	
	AlphaProperty * aprop = findProperty< AlphaProperty >();
	if ( aprop && aprop->blend() && draw2nd )
	{
		draw2nd->add( this );
		return;
	}
	
	// rigid mesh? then pass the transformation on to the gl layer
	
	if ( transformRigid )
	{
		glPushMatrix();
		glMultMatrix( viewTrans() );
	}
	
	// setup array pointers

	glEnableClientState( GL_VERTEX_ARRAY );
	glVertexPointer( 3, GL_FLOAT, 0, transVerts.data() );
	
	if ( transNorms.count() )
	{
		glEnableClientState( GL_NORMAL_ARRAY );
		glNormalPointer( GL_FLOAT, 0, transNorms.data() );
	}
	
	if ( transColors.count() )
	{
		glEnableClientState( GL_COLOR_ARRAY );
		glColorPointer( 4, GL_FLOAT, 0, transColors.data() );
	}
	else
		glColor( Color3( 1.0f, 0.2f, 1.0f ) );
	
	
	shader = scene->renderer.setupProgram( this, shader );
	
	
	// render the triangles

	if ( sortedTriangles.count() )
		glDrawElements( GL_TRIANGLES, sortedTriangles.count() * 3, GL_UNSIGNED_SHORT, sortedTriangles.data() );
	
	// render the tristrips
	
	for ( int s = 0; s < tristrips.count(); s++ )
		glDrawElements( GL_TRIANGLE_STRIP, tristrips[s].count(), GL_UNSIGNED_SHORT, tristrips[s].data() );
	
	scene->renderer.stopProgram();

	glDisableClientState( GL_VERTEX_ARRAY );
	glDisableClientState( GL_NORMAL_ARRAY );
	glDisableClientState( GL_COLOR_ARRAY );
	
	if ( transformRigid )
		glPopMatrix();
}

void Mesh::drawSelection() const
{
	Node::drawSelection();
	
	if ( isHidden() || !Options::drawMeshes() )
		return;
	
	if ( scene->currentBlock != iBlock && scene->currentBlock != iData && scene->currentBlock != iSkinPart
			&& ( ! iTangentData.isValid() || scene->currentBlock != iTangentData ) )
		return;
	
	if ( transformRigid )
	{
		glPushMatrix();
		glMultMatrix( viewTrans() );
	}
	
	glDisable( GL_LIGHTING );
	glDisable( GL_COLOR_MATERIAL );
	glDisable( GL_TEXTURE_2D );
	glDisable( GL_NORMALIZE );
	glEnable( GL_DEPTH_TEST );
	glDepthMask( GL_FALSE );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glDisable( GL_ALPHA_TEST );
	
	glLineWidth( 1.0 );
	glPointSize( 3.5 );
	
	QString n;
	int i = -1;
	
	if ( scene->currentBlock == iBlock || scene->currentIndex == iData )
	{
		n = "Faces";
	}
	else if ( scene->currentBlock == iData || scene->currentBlock == iSkinPart )
	{
		n = scene->currentIndex.data( Qt::DisplayRole ).toString();
		
		QModelIndex iParent = scene->currentIndex.parent();
		if ( iParent.isValid() && iParent != iData )
		{
			n = iParent.data( Qt::DisplayRole ).toString();
			i = scene->currentIndex.row();
		}
	}
	else if ( scene->currentBlock == iTangentData )
	{
		n = "TSpace";
	}
	
	if ( n == "Vertices" || n == "Normals" || n == "Vertex Colors" 
	  || n == "UV Sets" || n == "Tangents" || n == "Binormals" )
	{
		glDepthFunc( GL_LEQUAL );
		glNormalColor();
		glBegin( GL_POINTS );
		for ( int j = 0; j < transVerts.count(); j++ )
			glVertex( transVerts.value( j ) );
		glEnd();
		if ( i >= 0 )
		{
			glDepthFunc( GL_ALWAYS );
			glHighlightColor();
			glBegin( GL_POINTS );
			glVertex( transVerts.value( i ) );
			glEnd();
		}
	}
	if ( n == "Points" )
	{
		glDepthFunc( GL_LEQUAL );
		glNormalColor();
		glBegin( GL_POINTS);
		const NifModel * nif = static_cast<const NifModel *>( iData.model() );
		QModelIndex points = nif->getIndex( iData, "Points" );
		if ( points.isValid() )
		{
			for ( int j = 0; j < nif->rowCount( points ); j++ )
			{
				QModelIndex iPoints = points.child( j, 0 );
				for ( int k = 0; k < nif->rowCount( iPoints ); k++ )
				{
					glVertex( transVerts.value( nif->get<quint16>( iPoints.child( k, 0 ) ) ) );
				}

			}
		}
		glEnd();
		if ( i >= 0 )
		{
			glDepthFunc( GL_ALWAYS );
			glHighlightColor();
			glBegin( GL_POINTS );
			QModelIndex iPoints = points.child( i, 0 );
			if ( nif->isArray( scene->currentIndex ) )
			{
				for ( int j = 0; j < nif->rowCount( iPoints ); j++ )
				{
					glVertex( transVerts.value( nif->get<quint16>( iPoints.child( j, 0 ) ) ) );
				}
			}
			else
			{
				iPoints = scene->currentIndex.parent();
				glVertex( transVerts.value( nif->get<quint16>( iPoints.child( i, 0 ) ) ) );
			}
			glEnd();
		}
	}
	if ( n == "Normals" || n == "TSpace" )
	{
		glDepthFunc( GL_LEQUAL );
		glNormalColor();
		
		float normalScale = bounds().radius / 20;
		if ( normalScale < 0.1f ) normalScale = 0.1f;
		
		glBegin( GL_LINES );
		
		for ( int j = 0; j < transVerts.count() && j < transNorms.count(); j++ )
		{
			glVertex( transVerts.value( j ) );
			glVertex( transVerts.value( j ) + transNorms.value( j ) * normalScale );
		}
		
		if ( n == "TSpace" )
		{
			for ( int j = 0; j < transVerts.count() && j < transTangents.count() && j < transBinormals.count(); j++ )
			{
				glVertex( transVerts.value( j ) );
				glVertex( transVerts.value( j ) + transTangents.value( j ) * normalScale );
				glVertex( transVerts.value( j ) );
				glVertex( transVerts.value( j ) + transBinormals.value( j ) * normalScale );
			}
		}
		
		glEnd();
		
		if ( i >= 0 )
		{
			glDepthFunc( GL_ALWAYS );
			glHighlightColor();
			glBegin( GL_LINES );
			glVertex( transVerts.value( i ) );
			glVertex( transVerts.value( i ) + transNorms.value( i ) * normalScale );
			glEnd();
		}
	}
	if ( n == "Tangents" )
	{
		glDepthFunc( GL_LEQUAL );
		glNormalColor();

		float normalScale = bounds().radius / 20;
		normalScale /= 2.0f;
		if ( normalScale < 0.1f ) normalScale = 0.1f;

		glBegin( GL_LINES );
		for ( int j = 0; j < transVerts.count() && j < transTangents.count(); j++ )
		{
			glVertex( transVerts.value( j ) );
			glVertex( transVerts.value( j ) + transTangents.value( j ) * normalScale * 2 );
			glVertex( transVerts.value( j ) );
			glVertex( transVerts.value( j ) - transTangents.value( j ) * normalScale / 2 );
		}
		glEnd();

		if ( i >= 0 )
		{
			glDepthFunc( GL_ALWAYS );
			glHighlightColor();
			glBegin( GL_LINES );
			glVertex( transVerts.value( i ) );
			glVertex( transVerts.value( i ) + transTangents.value( i ) * normalScale * 2);
			glVertex( transVerts.value( i ) );
			glVertex( transVerts.value( i ) - transTangents.value( i ) * normalScale / 2 );
			glEnd();
		}
	}
	if ( n == "Binormals" )
	{
		glDepthFunc( GL_LEQUAL );
		glNormalColor();

		float normalScale = bounds().radius / 20;
		normalScale /= 2.0f;
		if ( normalScale < 0.1f ) normalScale = 0.1f;

		glBegin( GL_LINES );
		for ( int j = 0; j < transVerts.count() && j < transBinormals.count(); j++ )
		{
			glVertex( transVerts.value( j ) );
			glVertex( transVerts.value( j ) + transBinormals.value( j ) * normalScale * 2 );
			glVertex( transVerts.value( j ) );
			glVertex( transVerts.value( j ) - transBinormals.value( j ) * normalScale / 2 );
		}
		glEnd();

		if ( i >= 0 )
		{
			glDepthFunc( GL_ALWAYS );
			glHighlightColor();
			glBegin( GL_LINES );
			glVertex( transVerts.value( i ) );
			glVertex( transVerts.value( i ) + transBinormals.value( i ) * normalScale * 2);
			glVertex( transVerts.value( i ) );
			glVertex( transVerts.value( i ) - transBinormals.value( i ) * normalScale / 2 );
			glEnd();
		}
	}
	if ( n == "Faces" || n == "Triangles" )
	{
		glDepthFunc( GL_LEQUAL );
		glLineWidth( 1.5f );
		glNormalColor();
		foreach ( Triangle tri, triangles )
		{
			glBegin( GL_LINE_STRIP );
			glVertex( transVerts.value( tri.v1() ) );
			glVertex( transVerts.value( tri.v2() ) );
			glVertex( transVerts.value( tri.v3() ) );
			glVertex( transVerts.value( tri.v1() ) );
			glEnd();
		}
		if ( i >= 0 )
		{
			glDepthFunc( GL_ALWAYS );
			glHighlightColor();
			Triangle tri = triangles.value( i );
			glBegin( GL_LINE_STRIP );
			glVertex( transVerts.value( tri.v1() ) );
			glVertex( transVerts.value( tri.v2() ) );
			glVertex( transVerts.value( tri.v3() ) );
			glVertex( transVerts.value( tri.v1() ) );
			glEnd();
		}
	}
	if ( n == "Faces" || n == "Strips" || n == "Strip Lengths" )
	{
		glDepthFunc( GL_LEQUAL );
		glLineWidth( 1.5f );
		glNormalColor();
		foreach ( QVector<quint16> strip, tristrips )
		{
			quint16 a = strip.value( 0 );
			quint16 b = strip.value( 1 );
			
			for ( int v = 2; v < strip.count(); v++ )
			{
				quint16 c = strip[v];
				
				if ( a != b && b != c && c != a )
				{
					glBegin( GL_LINE_STRIP );
					glVertex( transVerts.value( a ) );
					glVertex( transVerts.value( b ) );
					glVertex( transVerts.value( c ) );
					glVertex( transVerts.value( a ) );
					glEnd();
				}
				
				a = b;
				b = c;
			}
		}
		if ( i >= 0 && !tristrips.isEmpty() )
		{
			QVector<quint16> strip = tristrips[i];
			
			quint16 a = strip.value( 0 );
			quint16 b = strip.value( 1 );
			
			for ( int v = 2; v < strip.count(); v++ )
			{
				quint16 c = strip[v];
				
				if ( a != b && b != c && c != a )
				{
					glDepthFunc( GL_ALWAYS );
					glHighlightColor();
					glBegin( GL_LINE_STRIP );
					glVertex( transVerts.value( a ) );
					glVertex( transVerts.value( b ) );
					glVertex( transVerts.value( c ) );
					glVertex( transVerts.value( a ) );
					glEnd();
				}
				
				a = b;
				b = c;
			}
		}
	}
	if ( n == "Skin Partition Blocks" )
	{
		glDepthFunc( GL_LEQUAL );
		for ( int c = 0; c < partitions.count(); c++ )
		{
			if ( c == i )
				glHighlightColor();
			else
				glNormalColor();
				
			QVector<int> vmap = partitions[c].vertexMap;
			
			foreach ( Triangle tri, partitions[c].triangles )
			{
				glBegin( GL_LINE_STRIP );
				glVertex( transVerts.value( vmap.value( tri.v1() ) ) );
				glVertex( transVerts.value( vmap.value( tri.v2() ) ) );
				glVertex( transVerts.value( vmap.value( tri.v3() ) ) );
				glVertex( transVerts.value( vmap.value( tri.v1() ) ) );
				glEnd();
			}
			foreach ( QVector<quint16> strip, partitions[c].tristrips )
			{
				quint16 a = vmap.value( strip.value( 0 ) );
				quint16 b = vmap.value( strip.value( 1 ) );
				
				for ( int v = 2; v < strip.count(); v++ )
				{
					quint16 c = vmap.value( strip[v] );
					
					if ( a != b && b != c && c != a )
					{
						glBegin( GL_LINE_STRIP );
						glVertex( transVerts.value( a ) );
						glVertex( transVerts.value( b ) );
						glVertex( transVerts.value( c ) );
						glVertex( transVerts.value( a ) );
						glEnd();
					}
					
					a = b;
					b = c;
				}
			}
		}
	}
	
	if ( transformRigid )
		glPopMatrix();
}

QString Mesh::textStats() const
{
	return Node::textStats() + QString( "\nshader: %1\n" ).arg( shader );
}
