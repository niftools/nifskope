/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005-2012, NIF File Format Library and Tools
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

#include "glscene.h"
#include "glcontroller.h"
#include "glparticles.h"

#include "math.h"

float random( float r )
{
	return r * rand() / RAND_MAX;
}

Vector3 random( Vector3 v )
{
	v[0] *= random( 1.0 );
	v[1] *= random( 1.0 );
	v[2] *= random( 1.0 );
	return v;
}

class ParticleController : public Controller
{
	struct Particle
	{
		Vector3 position;
		Vector3 velocity;
		Vector3 unknown;
		float	lifetime;
		float	lifespan;
		float	lasttime;
		short	y;
		short	vertex;
		
		Particle() : lifetime( 0 ), lifespan( 0 ) {}
	};
	QVector<Particle> list;
	struct Gravity
	{
		float	force;
		int type;
		Vector3	position;
		Vector3	direction;
	};
	QVector<Gravity> grav;
	
	QPointer<Particles> target;
	
	float emitStart, emitStop, emitRate, emitLast, emitAccu, emitMax;
	QPointer<Node> emitNode;
	Vector3 emitRadius;
	
	float spd, spdRnd;
	float ttl, ttlRnd;
	
	float inc, incRnd;
	float dec, decRnd;
	
	float size;
	float grow;
	float fade;
	
	float localtime;
	
	QList<QPersistentModelIndex> iExtras;
	QPersistentModelIndex iColorKeys;
	
public:
	ParticleController( Particles * particles, const QModelIndex & index )
		: Controller( index ), target( particles ) {}
	
	bool update( const NifModel * nif, const QModelIndex & index )
	{
		if ( ! target )
			return false;
		
		if ( Controller::update( nif, index ) || ( index.isValid() && iExtras.contains( index ) ) )
		{
			emitNode = target->scene->getNode( nif, nif->getBlock( nif->getLink( iBlock, "Emitter" ) ) );
			emitStart = nif->get<float>( iBlock, "Emit Start Time" );
			emitStop = nif->get<float>( iBlock, "Emit Stop Time" );
			emitRate = nif->get<float>( iBlock, "Emit Rate" );
			emitRadius = nif->get<Vector3>( iBlock, "Start Random" );
			emitAccu = 0;
			emitLast = emitStart;
			
			spd = nif->get<float>( iBlock, "Speed" );
			spdRnd = nif->get<float>( iBlock, "Speed Random" );
			
			ttl = nif->get<float>( iBlock, "Lifetime" );
			ttlRnd = nif->get<float>( iBlock, "Lifetime Random" );
			
			inc = nif->get<float>( iBlock, "Vertical Direction" );
			incRnd = nif->get<float>( iBlock, "Vertical Angle" );
			
			dec = nif->get<float>( iBlock, "Horizontal Direction" );
			decRnd = nif->get<float>( iBlock, "Horizontal Angle" );
			
			size = nif->get<float>( iBlock, "Size" );
			grow = 0.0;
			fade = 0.0;
			
			list.clear();
			
			QModelIndex iParticles = nif->getIndex( iBlock, "Particles" );
			if ( iParticles.isValid() )
			{
				emitMax = nif->get<int>( iBlock, "Num Particles" );
				int active = nif->get<int>( iBlock, "Num Valid" );
				//iParticles = nif->getIndex( iParticles, "Particles" );
				//if ( iParticles.isValid() )
				//{
					for ( int p = 0; p < active && p < nif->rowCount( iParticles ); p++ )
					{
						Particle particle;
						particle.velocity = nif->get<Vector3>( iParticles.child( p, 0 ), "Velocity" );
						particle.lifetime = nif->get<float>( iParticles.child( p, 0 ), "Lifetime" );
						particle.lifespan = nif->get<float>( iParticles.child( p, 0 ), "Lifespan" );
						particle.lasttime = nif->get<float>( iParticles.child( p, 0 ), "Timestamp" );
						particle.vertex = nif->get<int>( iParticles.child( p, 0 ), "Vertex ID" );
						// Display saved particle start on initial load
						list.append( particle );
					}
				//}
			}
			
			if ( ( nif->get<int>( iBlock, "Emit Flags" ) & 1 ) == 0 )
			{
				emitRate = emitMax / ( ttl + ttlRnd / 2 );
			}
			
			iExtras.clear();
			grav.clear();
			iColorKeys = QModelIndex();
			QModelIndex iExtra = nif->getBlock( nif->getLink( iBlock, "Particle Extra" ) );
			while ( iExtra.isValid() )
			{
				iExtras.append( iExtra );
				
				QString name = nif->itemName( iExtra );
				if ( name == "NiParticleGrowFade" )
				{
					grow = nif->get<float>( iExtra, "Grow" );
					fade = nif->get<float>( iExtra, "Fade" );
				}
				else if ( name == "NiParticleColorModifier" )
				{
					iColorKeys = nif->getIndex( nif->getBlock( nif->getLink( iExtra, "Color Data" ), "NiColorData" ), "Data" );
				}
				else if ( name == "NiGravity" )
				{
					Gravity g;
					g.force = nif->get<float>( iExtra, "Force" );
					g.type = nif->get<int>( iExtra, "Type" );
					g.position = nif->get<Vector3>( iExtra, "Position" );
					g.direction = nif->get<Vector3>( iExtra, "Direction" );
					grav.append( g );
				}
				
				iExtra = nif->getBlock( nif->getLink( iExtra, "Next Modifier" ) );
			}
			return true;
		}
		return false;
	}
	
	void update( float time )
	{
		if ( ! ( target && active ) )
			return;
		
		localtime = ctrlTime( time );
		
		int n = 0;
		while ( n < list.count() )
		{
			Particle & p = list[n];
			
			float deltaTime = ( localtime > p.lasttime ? localtime - p.lasttime : 0 ); //( stop - start ) - p.lasttime + localtime ); 
			
			p.lifetime += deltaTime;
			if ( p.lifetime < p.lifespan && p.vertex < target->verts.count() )
			{
				p.position = target->verts[ p.vertex ];
				for ( int i = 0; i < 4; i++ )
					moveParticle( p, deltaTime/4 );
				p.lasttime = localtime;
				n++;
			}
			else
				list.remove( n );
		}
		
		if ( emitNode && emitNode->isVisible() && localtime >= emitStart && localtime <= emitStop )
		{
			float emitDelta = ( localtime > emitLast ? localtime - emitLast : 0 );
			emitLast = localtime;
			
			emitAccu += emitDelta * emitRate;
			
			int num = int( emitAccu );
			if ( num > 0 )
			{
				emitAccu -= num;
				while ( num-- > 0 && list.count() < target->verts.count() )
				{
					Particle p;
					startParticle( p );
					list.append( p );
				}
			}
		}
		
		n = 0;
		while ( n < list.count() )
		{
			Particle & p = list[n];
			p.vertex = n;
			target->verts[ n ] = p.position;
			if ( n < target->sizes.count() )
				sizeParticle( p, target->sizes[n] );
			if ( n < target->colors.count() )
				colorParticle( p, target->colors[n] );
			n++;
		}
		
		target->active = list.count();
		target->size = size;
	}
	
	void startParticle( Particle & p )
	{
		p.position = random( emitRadius * 2 ) - emitRadius;
		p.position += target->worldTrans().rotation.inverted() * ( emitNode->worldTrans().translation - target->worldTrans().translation );
		
		float i = inc + random( incRnd );
		float d = dec + random( decRnd );
		
		p.velocity = Vector3( rand() & 1 ? sin( i ) : - sin( i ), 0, cos( i ) );
		
		Matrix m; m.fromEuler( 0, 0, rand() & 1 ? d : -d );
		p.velocity = m * p.velocity;
		
		p.velocity = p.velocity * ( spd + random( spdRnd ) );
		p.velocity = target->worldTrans().rotation.inverted() * emitNode->worldTrans().rotation * p.velocity;
		
		p.lifetime = 0;
		p.lifespan = ttl + random( ttlRnd );
		p.lasttime = localtime;
	}
	
	void moveParticle( Particle & p, float deltaTime )
	{
		foreach ( Gravity g, grav )
		{
			switch ( g.type )
			{
				case 0:
					p.velocity += g.direction * ( g.force * deltaTime );
					break;
				case 1:
				{
					Vector3 dir = ( g.position - p.position );
					dir.normalize();
					p.velocity += dir * ( g.force * deltaTime );
				}	break;
			}
		}
		p.position += p.velocity * deltaTime;
	}
	
	void sizeParticle( Particle & p, float & size )
	{
		size = 1.0;
		
		if ( grow > 0 && p.lifetime < grow )
			size *= p.lifetime / grow;
		if ( fade > 0 && p.lifespan - p.lifetime < fade )
			size *= ( p.lifespan - p.lifetime ) / fade;
	}
	
	void colorParticle( Particle & p, Color4 & color )
	{
		if ( iColorKeys.isValid() )
		{
			int i = 0;
			interpolate( color, iColorKeys, p.lifetime / p.lifespan, i );
		}
	}
};

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
	
	if ( ! iBlock.isValid() )
		return;
	
	upData |= ( iData == index );

	if ( iBlock == index )
	{
		foreach ( int link, nif->getChildLinks( id() ) )
		{
			QModelIndex iChild = nif->getBlock( link );
			if ( ! iChild.isValid() ) continue;
			QString name = nif->itemName( iChild );
			
			//if ( name == "NiParticlesData" || name == "NiRotatingParticlesData" || name == "NiAutoNormalParticlesData" )
			if ( nif->inherits( iChild, "NiParticlesData" ) )
			{
				iData = iChild;
				upData = true;
			}
		}
	}
}

void Particles::setController( const NifModel * nif, const QModelIndex & index )
{
	if ( nif->itemName( index ) == "NiParticleSystemController" || nif->itemName( index ) == "NiBSPArrayController" )
	{
		Controller * ctrl = new ParticleController( this, index );
		ctrl->update( nif, index );
		controllers.append( ctrl );
	}
	else
		Node::setController( nif, index );
}

void Particles::transform()
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
		colors = nif->getArray<Color4>( nif->getIndex( iData, "Vertex Colors" ) );
		sizes = nif->getArray<float>( nif->getIndex( iData, "Sizes" ) );
		
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

void Particles::drawShapes( NodeList * draw2nd )
{
	if ( isHidden() )
		return;
	
	AlphaProperty * aprop = findProperty< AlphaProperty >();
	if ( aprop && aprop->blend() && draw2nd )
	{
		draw2nd->add( this );
		return;
	}

	//glLoadName( nodeId );
	// TODO: I don't know what calls this method, because it was not called so far,
	// so I just disabled the GL_SELECT helper
	
	// Disable texturing,  texturing properties will reenable if applicable
	glDisable( GL_TEXTURE_2D );

	// setup blending
	
	glProperty( findProperty< AlphaProperty >() );
	
	// setup vertex colors
	
	glProperty( findProperty< VertexColorProperty >(), ( colors.count() >= transVerts.count() ) );
	
	// setup material
	
	glProperty( findProperty< MaterialProperty >(), findProperty< SpecularProperty >() );

	// setup texturing
	
	glProperty( findProperty< TexturingProperty >() );

	// setup texturing

	glProperty( findProperty< BSShaderLightingProperty >() );

	// setup z buffer
	
	glProperty( findProperty< ZBufferProperty >() );
	
	// setup stencil
	
	glProperty( findProperty< StencilProperty >() );
	
	// wireframe ?
	
	glProperty( findProperty< WireframeProperty >() );

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

	static const Vector2 tex[4] = { Vector2( 1.0, 1.0 ), Vector2( 0.0, 1.0 ), Vector2( 1.0, 0.0 ), Vector2( 0.0, 0.0 ) };
	
	int p = 0;
	foreach ( Vector3 v, transVerts )
	{
		if ( p >= active )
			break;
		
		GLfloat s2 = ( sizes.count() > p ? sizes[ p ] * size : size );
		s2 *= worldTrans().scale;
		
		glBegin( GL_TRIANGLE_STRIP );
		if ( p < colors.count() )
			glColor( colors[ p ] );
		glTexCoord( tex[0] );
		glVertex( v + Vector3( + s2, + s2, 0 ) );
		glTexCoord( tex[1] );
		glVertex( v + Vector3( - s2, + s2, 0 ) );
		glTexCoord( tex[2] );
		glVertex( v + Vector3( + s2, - s2, 0 ) );
		glTexCoord( tex[3] );
		glVertex( v + Vector3( - s2, - s2, 0 ) );
		glEnd();
		
		p++;
	}
}
