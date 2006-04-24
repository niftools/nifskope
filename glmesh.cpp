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

#include <GL/glext.h>

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
	
	bool update( const NifModel * nif, const QModelIndex & index )
	{
		if ( Controller::update( nif, index ) )
		{
			qDeleteAll( morph );
			morph.clear();
			
			QModelIndex iInterpolators = nif->getIndex( nif->getIndex( iBlock, "Interpolators" ), "Indices" );
			
			QModelIndex midx = nif->getIndex( iData, "Morphs" );
			if ( midx.isValid() )
			{
				for ( int r = 0; r < nif->rowCount( midx ); r++ )
				{
					QModelIndex iKey = midx.child( r, 0 );
					
					MorphKey * key = new MorphKey;
					key->index = 0;
					key->iFrames = nif->getIndex( iKey, "Frames" );
					if ( ! key->iFrames.isValid() && iInterpolators.isValid() )
					{
						key->iFrames = nif->getIndex( nif->getBlock( nif->getLink( nif->getBlock( nif->getLink( iInterpolators.child( r, 0 ) ), "NiFloatInterpolator" ), "Data" ), "NiFloatData" ), "Data" );
					}
					key->verts = nif->getArray<Vector3>( nif->getIndex( iKey, "Vectors" ) );
					
					morph.append( key );
				}
			}
			return true;
		}
		return false;
	}

protected:
	QPointer<Mesh> target;
	QVector<MorphKey*>	morph;
};


/*
 *  Mesh
 */

void Mesh::clear()
{
	Node::clear();

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
	else
		Node::setController( nif, iController );
}

bool Mesh::isHidden() const
{
	return ( Node::isHidden() || ( ! scene->showHidden && scene->onlyTextured && ! properties.get< TexturingProperty >() ) );
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
			triangles = nif->getArray<Triangle>( nif->getIndex( iData, "Triangles" ) );
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
	if ( isHidden() )
		return;
	
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
		
		bndSphere = BoundSphere( transVerts );
		bndSphere.applyInv( viewTrans() );
		upBounds = false;
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
}

BoundSphere Mesh::bounds() const
{
	if ( upBounds )
	{
		upBounds = false;
		bndSphere = BoundSphere( verts );
	}
	return worldTrans() * bndSphere | Node::bounds();
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
	
	setupRenderState( transColors.count() );
	
	if ( ! transNorms.count() )
		glDisable( GL_NORMALIZE );
	
	// rigid mesh? then pass the transformation on to the gl layer
	
	if ( transformRigid )
	{
		glPushMatrix();
		viewTrans().glLoadMatrix();
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
	
	// setup multitexturing
	
	TexturingProperty * texprop = findProperty< TexturingProperty >();
	if ( texprop )
	{
		int stage = 0;
		
		if ( texprop->bind( 1, coords, stage ) )
		{	// dark
			stage++;
			glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB );
			
			glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS_ARB );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_TEXTURE );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR );
			
			glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_MODULATE );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS_ARB );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, GL_TEXTURE );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND1_ALPHA_ARB, GL_SRC_ALPHA );
			
			glTexEnvf( GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 1.0 );
		}
		
		if ( texprop->bind( 0, coords, stage ) )
		{	// base
			stage++;
			glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB );
			
			glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS_ARB );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_TEXTURE );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR );
			
			glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_MODULATE );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS_ARB );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, GL_TEXTURE );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND1_ALPHA_ARB, GL_SRC_ALPHA );
			
			glTexEnvf( GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 1.0 );
		}
		
		if ( texprop->bind( 2, coords, stage ) )
		{	// detail
			stage++;
			glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB );
			
			glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS_ARB );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_TEXTURE );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR );
			
			glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_MODULATE );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS_ARB );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, GL_TEXTURE );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND1_ALPHA_ARB, GL_SRC_ALPHA );
			
			glTexEnvf( GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 2.0 );
		}
		
		if ( texprop->bind( 6, coords, stage ) )
		{	// decal 0
			stage++;
			glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB );
			
			glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_INTERPOLATE );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PREVIOUS_ARB );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE2_RGB_ARB, GL_TEXTURE );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND2_RGB_ARB, GL_SRC_ALPHA );
			
			glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS_ARB );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA );
			
			glTexEnvf( GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 1.0 );
		}
		
		if ( texprop->bind( 7, coords, stage ) )
		{	// decal 1
			stage++;
			glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB );
			
			glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_INTERPOLATE );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PREVIOUS_ARB );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE2_RGB_ARB, GL_TEXTURE );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND2_RGB_ARB, GL_SRC_ALPHA );
			
			glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS_ARB );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA );
			
			glTexEnvf( GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 1.0 );
		}
		
		if ( texprop->bind( 4, coords, stage ) )
		{	// glow
			stage++;
			glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB );
			
			glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_ADD );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS_ARB );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_TEXTURE );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR );
			
			glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS_ARB );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA );
			
			glTexEnvf( GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 1.0 );
		}
	}
	
	// render the triangles

	if ( triangles.count() )
		glDrawElements( GL_TRIANGLES, triangles.count() * 3, GL_UNSIGNED_SHORT, triangles.data() );
	
	// render the tristrips
	
	for ( int s = 0; s < tristrips.count(); s++ )
		glDrawElements( GL_TRIANGLE_STRIP, tristrips[s].count(), GL_UNSIGNED_SHORT, tristrips[s].data() );
	
	resetTextureUnits();

	glDisableClientState( GL_VERTEX_ARRAY );
	glDisableClientState( GL_NORMAL_ARRAY );
	glDisableClientState( GL_COLOR_ARRAY );
	
	
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
		glDisable( GL_ALPHA_TEST );
		
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
		foreach ( QVector<quint16> strip, tristrips )
		{
			glColor4fv( stripcolor[c] );
			if ( ++c >= 6 ) c = 0;
			
			quint16 a = strip.value( 0 );
			quint16 b = strip.value( 1 );
			
			for ( int v = 2; v < strip.count(); v++ )
			{
				quint16 c = strip[v];
				
				if ( a != b && b != c && c != a )
				{
					glBegin( GL_LINE_STRIP );
					glVertex( transVerts[a] );
					glVertex( transVerts[b] );
					glVertex( transVerts[c] );
					glVertex( transVerts[a] );
					glEnd();
				}
				
				a = b;
				b = c;
			}
			
		}
		/*
		if ( transformRigid )
			glPopMatrix();
		
		BoundSphere bs = bounds();
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
		*/
	}
	
	if ( transformRigid )
		glPopMatrix();
}
