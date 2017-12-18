/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005-2015, NIF File Format Library and Tools
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the NIF File Format Library and Tools project may not be
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

#include "glparticles.h"

#include "gl/controllers.h"
#include "gl/glscene.h"
#include "model/nifmodel.h"

#include <math.h>


/*
 *  Particle
 */

void Particles::clear()
{
	Node::clear();

	verts.clear();
	colors.clear();
	transVerts.clear();
}

void Particles::update( const NifModel * nif, const QModelIndex & index )
{
	Node::update( nif, index );

	if ( !iBlock.isValid() )
		return;

	upData |= ( iData == index );

	if ( iBlock == index ) {
		for ( const auto link : nif->getChildLinks( id() ) ) {
			QModelIndex iChild = nif->getBlock( link );

			if ( !iChild.isValid() )
				continue;

			if ( nif->inherits( iChild, "NiParticlesData" ) ) {
				iData  = iChild;
				upData = true;
			}
		}
	}
}

void Particles::setController( const NifModel * nif, const QModelIndex & index )
{
	if ( nif->itemName( index ) == "NiParticleSystemController" || nif->itemName( index ) == "NiBSPArrayController" ) {
		Controller * ctrl = new ParticleController( this, index );
		ctrl->update( nif, index );
		controllers.append( ctrl );
	} else {
		Node::setController( nif, index );
	}
}

void Particles::transform()
{
	const NifModel * nif = static_cast<const NifModel *>( iBlock.model() );

	if ( !nif || !iBlock.isValid() ) {
		clear();
		return;
	}

	if ( upData ) {
		upData = false;

		verts  = nif->getArray<Vector3>( nif->getIndex( iData, "Vertices" ) );
		colors = nif->getArray<Color4>( nif->getIndex( iData, "Vertex Colors" ) );
		sizes  = nif->getArray<float>( nif->getIndex( iData, "Sizes" ) );

		active = nif->get<int>( iData, "Num Valid" );
		size = nif->get<float>( iData, "Active Radius" );
	}

	Node::transform();
}

void Particles::transformShapes()
{
	Node::transformShapes();

	Transform vtrans = viewTrans();

	transVerts.resize( verts.count() );

	for ( int v = 0; v < verts.count(); v++ )
		transVerts[v] = vtrans * verts[v];
}

BoundSphere Particles::bounds() const
{
	BoundSphere sphere( verts );
	sphere.radius += size;
	return worldTrans() * sphere | Node::bounds();
}

void Particles::drawShapes( NodeList * secondPass, bool presort )
{
	Q_UNUSED( presort );

	if ( isHidden() )
		return;

	AlphaProperty * aprop = findProperty<AlphaProperty>();

	if ( aprop && aprop->blend() && secondPass ) {
		secondPass->add( this );
		return;
	}

	// Disable texturing,  texturing properties will reenable if applicable
	glDisable( GL_TEXTURE_2D );

	// setup blending

	glProperty( findProperty<AlphaProperty>() );

	// setup vertex colors

	glProperty( findProperty<VertexColorProperty>(), ( colors.count() >= transVerts.count() ) );

	// setup material

	glProperty( findProperty<MaterialProperty>(), findProperty<SpecularProperty>() );

	// setup texturing

	glProperty( findProperty<TexturingProperty>() );

	// setup texturing

	glProperty( findProperty<BSShaderLightingProperty>() );

	// setup z buffer

	glProperty( findProperty<ZBufferProperty>() );

	// setup stencil

	glProperty( findProperty<StencilProperty>() );

	// wireframe ?

	glProperty( findProperty<WireframeProperty>() );

	// normalize

	glEnable( GL_NORMALIZE );
	glNormal3f( 0.0, 0.0, -1.0 );

	// render the particles

	/*
	 * Goal: get multitexturing happening
	 *
	 * Mesh uses Renderer::setupProgram which calls
	 * Renderer::setupFixedFunction
	 *
	 * setupFixedFunction calls TexturingProperty::bind( id, coords, stage )
	 * bind calls activateTextureUnit
	 *
	 * Mesh also draws using glDrawElements and glVertexPointer
	 *
	 */

	static const Vector2 tex[4] = {
		Vector2( 1.0, 1.0 ), Vector2( 0.0, 1.0 ), Vector2( 1.0, 0.0 ), Vector2( 0.0, 0.0 )
	};

	int p = 0;
	for ( Vector3 v : transVerts ) {
		if ( p >= active )
			break;

		GLfloat s2 = ( sizes.count() > p ? sizes[ p ] * size : size );
		s2 *= worldTrans().scale;

		glBegin( GL_TRIANGLE_STRIP );

		if ( p < colors.count() )
			glColor( colors[ p ] );

		glTexCoord( tex[0] );
		glVertex( v + Vector3( +s2, +s2, 0 ) );
		glTexCoord( tex[1] );
		glVertex( v + Vector3( -s2, +s2, 0 ) );
		glTexCoord( tex[2] );
		glVertex( v + Vector3( +s2, -s2, 0 ) );
		glTexCoord( tex[3] );
		glVertex( v + Vector3( -s2, -s2, 0 ) );
		glEnd();

		p++;
	}
}
