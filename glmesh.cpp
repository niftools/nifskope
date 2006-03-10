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

#include "glmesh.h"
#include "glcontroller.h"
#include "glscene.h"

class MorphController : public Controller
{
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
		if ( ! ( target && iData.isValid() && flags.controller.active && morph.count() > 1 ) )
			return;
		
		time = ctrlTime( time );
		
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
	
	
	void update( const NifModel * nif, const QModelIndex & index )
	{
		Controller::update( nif, index );
		if ( ( iBlock.isValid() && iBlock == index ) || ( iData.isValid() && iData == index ) )
		{
			iData = nif->getBlock( nif->getLink( iBlock, "Data" ), "NiMorphData" );
			
			qDeleteAll( morph );
			morph.clear();
			
			QModelIndex midx = nif->getIndex( iData, "Morphs" );
			if ( midx.isValid() )
			{
				for ( int r = 0; r < nif->rowCount( midx ); r++ )
				{
					QModelIndex iKey = midx.child( r, 0 );
					
					MorphKey * key = new MorphKey;
					key->index = 0;
					key->iFrames = nif->getIndex( iKey, "Frames" );
					key->verts = nif->getArray<Vector3>( nif->getIndex( iKey, "Vectors" ) );
					
					morph.append( key );
				}
			}
		}
	}

protected:
	QPointer<Mesh> target;
	
	QPersistentModelIndex iData;
	
	QVector<MorphKey*>	morph;
};


class TexCoordController : public Controller
{
public:
	TexCoordController( Mesh * mesh, const QModelIndex & index )
		: Controller( index ), target( mesh ), lS( 0 ), lT( 0 ) {}
	
	void update( float time )
	{
		if ( ! ( flags.controller.active && target ) )
			return;
		
		time = ctrlTime( time );
		
		interpolate( target->texOffset[0], iKeys.child( 0, 0 ), time, lS );
		interpolate( target->texOffset[1], iKeys.child( 1, 0 ), time, lT );
	}
	
	void update( const NifModel * nif, const QModelIndex & index )
	{
		Controller::update( nif, index );
		if ( ( iBlock.isValid() && iBlock == index ) || ( iData.isValid() && iData == index ) )
		{
			iData = nif->getBlock( nif->getLink( index, "Data" ), "NiUVData" );
			iKeys = nif->getIndex( iData, "UV Groups" );
		}
	}
	
protected:
	QPointer<Mesh> target;
	
	QPersistentModelIndex iData;
	QPersistentModelIndex iKeys;
	
	int lS, lT;
};


/*
 *  Mesh
 */

Mesh::Tristrip::Tristrip( const NifModel * nif, const QModelIndex & tristrip )
{
	if ( ! tristrip.isValid() ) return;
	
	for ( int s = 0; s < nif->rowCount( tristrip ); s++ )
		vertices.append( nif->get<int>( tristrip.child( s, 0 ) ) );
}


void Mesh::clear()
{
	Node::clear();

	texOffset = Vector2();
	
	iData = iSkin = iSkinData = QModelIndex();
	
	verts.clear();
	norms.clear();
	colors.clear();
	coords.clear();
	triangles.clear();
	tristrips.clear();
	transVerts.clear();
	transNorms.clear();
	transColors.clear();
	transCoords.clear();
}

void Mesh::update( const NifModel * nif, const QModelIndex & index )
{
	Node::update( nif, index );
	
	if ( ! iBlock.isValid() )
		return;
	
	upData |= ( iData == index );
	upSkin |= ( iSkin == index );
	upSkin |= ( iSkinData == index );
	
	if ( iBlock == index )
	{
		foreach ( int link, nif->getChildLinks( id() ) )
		{
			QModelIndex iChild = nif->getBlock( link );
			if ( ! iChild.isValid() ) continue;
			QString name = nif->itemName( iChild );
			
			if ( name == "NiTriShapeData" || name == "NiTriStripsData" )
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
			else if ( name == "NiSkinInstance" )
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
		Controller * ctrl = new TexCoordController( this, iController );
		ctrl->update( nif, iController );
		controllers.append( ctrl );
	}
	else
		Node::setController( nif, iController );
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
		
		verts = nif->getArray<Vector3>( nif->getIndex( iData, "Vertices" ) );
		norms = nif->getArray<Vector3>( nif->getIndex( iData, "Normals" ) );
		colors = nif->getArray<Color4>( nif->getIndex( iData, "Vertex Colors" ) );
		
		QModelIndex uvcoord = nif->getIndex( iData, "UV Sets" );
		if ( ! uvcoord.isValid() ) uvcoord = nif->getIndex( iData, "UV Sets 2" );
		if ( uvcoord.isValid() )
			coords = nif->getArray<Vector2>( uvcoord.child( 0, 0 ) );
		else
			coords.clear();
		
		if ( nif->itemName( iData ) == "NiTriShapeData" )
		{
			triangles = nif->getArray<Triangle>( nif->getIndex( iData, "Triangles" ) );
			tristrips.clear();
		}
		else if ( nif->itemName( iData ) == "NiTriStripsData" )
		{
			QModelIndex points = nif->getIndex( iData, "Points" );
			if ( points.isValid() )
			{
				for ( int r = 0; r < nif->rowCount( points ); r++ )
					tristrips.append( Tristrip( nif, points.child( r, 0 ) ) );
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
	}
	
	if ( upSkin )
	{
		upSkin = false;
		
		iSkinData = nif->getBlock( nif->getLink( iSkin, "Data" ), "NiSkinData" );
		
		skelRoot = nif->getLink( iSkin, "Skeleton Root" );
		skelTrans = Transform( nif, iSkinData );
		
		QVector<int> bones;
		QModelIndex idxBones = nif->getIndex( nif->getIndex( iSkin, "Bones" ), "Bones" );
		if ( idxBones.isValid() )
			for ( int b = 0; b < nif->rowCount( idxBones ); b++ )
				bones.append( nif->getLink( nif->index( b, 0, idxBones ) ) );
		
		idxBones = nif->getIndex( iSkinData, "Bone List" );
		if ( idxBones.isValid() )
		{
			weights.clear();		
			for ( int b = 0; b < nif->rowCount( idxBones ) && b < bones.count(); b++ )
			{
				weights.append( BoneWeights( nif, idxBones.child( b, 0 ), bones[ b ] ) );
			}
		}
	}

	Node::transform();
}

void Mesh::transformShapes()
{
	Node::transformShapes();
	
	AlphaProperty * alphaprop = findProperty<AlphaProperty>();
	transformRigid = ! ( ( alphaprop && alphaprop->sort() ) || weights.count() );
	
	if ( weights.count() )
	{
		transVerts.resize( verts.count() );
		transVerts.fill( Vector3() );
		transNorms.resize( norms.count() );
		transNorms.fill( Vector3() );
		
		Node * root = findParent( skelRoot );
		foreach ( BoneWeights bw, weights )
		{
			Transform trans = viewTrans() * skelTrans;
			if ( root )
			{
				Node * bone = root->findChild( bw.bone );
				if ( bone ) trans = bone->viewTrans() * bw.trans; //trans * bone->localTransFrom( skelRoot ) * bw.trans;
			}	// FIXME
			
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
		
		upBounds = true;
	}
	else if ( ! transformRigid )
	{
		transVerts.resize( verts.count() );
		Transform vtrans = viewTrans();
		for ( int v = 0; v < verts.count(); v++ )
			transVerts[v] = vtrans * verts[v];
		
		transNorms.resize( norms.count() );
		Matrix natrix = viewTrans().rotation;
		for ( int n = 0; n < norms.count(); n++ )
		{
			transNorms[n] = natrix * norms[n];
		}
	}
	else
	{
		transVerts = verts;
		transNorms = norms;
	}

	if ( alphaprop && alphaprop->sort() )
	{
		triOrder.resize( triangles.count() );
		int t = 0;
		foreach ( Triangle tri, triangles )
		{
			QPair< int, float > tp;
			tp.first = t;
			tp.second = transVerts.value( tri.v1() )[2] + transVerts.value( tri.v2() )[2] + transVerts.value( tri.v3() )[2];
			triOrder[t++] = tp;
		}
		qSort( triOrder.begin(), triOrder.end(), compareTriangles );
	}
	else
		triOrder.clear();
	
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
	
	if ( texOffset[0] != 0.0 || texOffset[1] != 0.0 )
	{
		transCoords.resize( coords.count() );
		for ( int c = 0; c < coords.count(); c++ )
			transCoords[c] += texOffset;
	}
	else
		transCoords = coords;
}

BoundSphere Mesh::bounds() const
{
	if ( upBounds )
	{
		upBounds = false;
		bndSphere = worldTrans() * BoundSphere( verts );
	}
	return bndSphere | Node::bounds();
}

void Mesh::drawShapes( NodeList * draw2nd )
{
	if ( isHidden() )
		return;
	
	glLoadName( nodeId );
	
	// setup alpha blending
	
	AlphaProperty * aprop = findProperty< AlphaProperty >();
	if ( aprop && aprop->blend() && scene->blending && draw2nd )
	{
		draw2nd->add( this );
		return;
	}
	glProperty( aprop );
	
	// setup lighting
	
	scene->setupLights( this );
	
	// setup material
	
	glProperty( findProperty< MaterialProperty >(), findProperty< SpecularProperty >() );

	// setup vertex colors
	
	if ( transColors.count() >= transVerts.count() )
		glProperty( findProperty< VertexColorProperty >() );
	else
		glDisable( GL_COLOR_MATERIAL );
	glColor( Color4() );
	
	// setup texturing
	
	glProperty( findProperty< TexturingProperty >() );
	
	// setup z buffer
	
	glProperty( findProperty< ZBufferProperty >() );
	
	// setup stencil
	
	glProperty( findProperty< StencilProperty >() );
	
	// wireframe ?
	
	glProperty( findProperty< WireframeProperty >() );

	// normalize
	
	if ( transNorms.count() >= transVerts.count() )
		glEnable( GL_NORMALIZE );
	else
		glDisable( GL_NORMALIZE );
	
	// rigid mesh? then pass the transformation on to the gl layer
	
	if ( transformRigid )
	{
		glPushMatrix();
		viewTrans().glLoadMatrix();
	}

	// render the triangles

	if ( triangles.count() > 0 )
	{
		glBegin( GL_TRIANGLES );
		for ( int t = 0; t < triangles.count(); t++ )
		{
			const Triangle & tri = ( triOrder.count() ? triangles[ triOrder[ t ].first ] : triangles[ t ] );
			
			if ( transVerts.count() > tri.v1() && transVerts.count() > tri.v2() && transVerts.count() > tri.v3() )
			{
				if ( transNorms.count() > tri.v1() ) glNormal( transNorms[tri.v1()] );
				if ( transCoords.count() > tri.v1() ) glTexCoord( transCoords[tri.v1()] );
				if ( transColors.count() > tri.v1() ) glColor( transColors[tri.v1()] );
				glVertex( transVerts[tri.v1()] );
				if ( transNorms.count() > tri.v2() ) glNormal( transNorms[tri.v2()] );
				if ( transCoords.count() > tri.v2() ) glTexCoord( transCoords[tri.v2()] );
				if ( transColors.count() > tri.v2() ) glColor( transColors[tri.v2()] );
				glVertex( transVerts[tri.v2()] );
				if ( transNorms.count() > tri.v3() ) glNormal( transNorms[tri.v3()] );
				if ( transCoords.count() > tri.v3() ) glTexCoord( transCoords[tri.v3()] );
				if ( transColors.count() > tri.v3() ) glColor( transColors[tri.v3()] );
				glVertex( transVerts[tri.v3()] );
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
			if ( transCoords.count() > v ) glTexCoord( transCoords[v] );
			//if ( transColors.count() > v ) glColor( transColors[v] );
			if ( transVerts.count() > v ) glVertex( transVerts[v] );
		}
		glEnd();
	}
	
	// draw green mesh outline if selected
	
	if ( scene->highlight && scene->currentNode == nodeId )
	{
		glDisable( GL_LIGHTING );
		glDisable( GL_COLOR_MATERIAL );
		glDisable( GL_TEXTURE_2D );
		glDisable( GL_NORMALIZE );
		glEnable( GL_DEPTH_TEST );
		glDepthMask( GL_TRUE );
		glDepthFunc( GL_LEQUAL );
		glLineWidth( 1.0 );
		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		
		glColor( Color4( 0.0, 1.0, 0.0, 0.5 ) );
		
		foreach ( Triangle tri, triangles )
		{
			if ( transVerts.count() > tri.v1() && transVerts.count() > tri.v2() && transVerts.count() > tri.v3() )
			{
				glBegin( GL_LINE_STRIP );
				glVertex( transVerts[tri.v1()] );
				glVertex( transVerts[tri.v2()] );
				glVertex( transVerts[tri.v3()] );
				glVertex( transVerts[tri.v1()] );
				glEnd();
			}
		}
		
		static const GLfloat stripcolor[6][4] = {
			{ 0, 1, 0, .5 }, { 0, 1, 1, .5 },
			{ 0, 0, 1, .5 }, { 1, 0, 1, .5 },
			{ 1, 0, 0, .5 }, { 1, 1, 0, .5 } };
		int c = 0;
		foreach ( Tristrip strip, tristrips )
		{
			glColor4fv( stripcolor[c] );
			if ( ++c >= 6 ) c = 0;
			
			glBegin( GL_LINE_STRIP );
			foreach ( quint16 v, strip.vertices )
			{
				if ( transVerts.count() > v )
					glVertex( transVerts[v] );
			}
			glEnd();
			
		}
		
		BoundSphere bs = bounds();
		if ( transformRigid )
			glPopMatrix();
		bs = scene->view * bs;
		
		Vector3 mn = bs.center;
		Vector3 mx = bs.center;
		for ( int i = 0; i < 3; i++ )
		{
			mn[i] -= bs.radius;
			mx[i] += bs.radius;
		}
		
		glColor3f( 1.0, 0.0, 1.0 );
		glBegin( GL_LINE_STRIP );
		glVertex3f( mn[0], mn[1], mn[2] );
		glVertex3f( mn[0], mx[1], mn[2] );
		glVertex3f( mn[0], mx[1], mx[2] );
		glVertex3f( mn[0], mn[1], mx[2] );
		glVertex3f( mn[0], mn[1], mn[2] );
		glEnd();
		glBegin( GL_LINE_STRIP );
		glVertex3f( mx[0], mn[1], mn[2] );
		glVertex3f( mx[0], mx[1], mn[2] );
		glVertex3f( mx[0], mx[1], mx[2] );
		glVertex3f( mx[0], mn[1], mx[2] );
		glVertex3f( mx[0], mn[1], mn[2] );
		glEnd();
		glBegin( GL_LINES );
		glVertex3f( mn[0], mn[1], mn[2] );
		glVertex3f( mx[0], mn[1], mn[2] );
		glVertex3f( mn[0], mx[1], mn[2] );
		glVertex3f( mx[0], mx[1], mn[2] );
		glVertex3f( mn[0], mx[1], mx[2] );
		glVertex3f( mx[0], mx[1], mx[2] );
		glVertex3f( mn[0], mn[1], mx[2] );
		glVertex3f( mx[0], mn[1], mx[2] );
		glEnd();
		return;
	}
	
	if ( transformRigid )
		glPopMatrix();
}
