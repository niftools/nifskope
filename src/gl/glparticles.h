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

#ifndef GLPARTICLES_H
#define GLPARTICLES_H

#include "glnode.h" // Inherited
#include "glcontroller.h" // Inherited

#include <QPersistentModelIndex>
#include <QVector>


class Particles : public Node
{
public:
	Particles( Scene * s, const QModelIndex & b ) : Node( s, b ) {}

	void clear() override;
	void update( const NifModel * nif, const QModelIndex & ) override;
	void transform() override;

	void transformShapes() override;

	void drawShapes( NodeList * draw2nd = nullptr ) override;

	BoundSphere bounds() const override;

protected:
	void setController( const NifModel * nif, const QModelIndex & controller ) override;

	QPersistentModelIndex iData;
	bool upData;

	QVector<Vector3> verts;
	QVector<Color4> colors;
	QVector<float> sizes;
	QVector<Vector3> transVerts;

	int active;
	float size;

	friend class ParticleController;
};


class ParticleController final : public Controller
{
	struct Particle
	{
		Vector3 position;
		Vector3 velocity;
		Vector3 unknown;
		float lifetime;
		float lifespan;
		float lasttime;
		short y;
		short vertex;

		Particle() : lifetime( 0 ), lifespan( 0 )
		{
		}
	};
	QVector<Particle> list;
	struct Gravity
	{
		float force;
		int type;
		Vector3 position;
		Vector3 direction;
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
	ParticleController( Particles * particles, const QModelIndex & index );

	bool update( const NifModel * nif, const QModelIndex & index ) override final;

	void update( float time ) override final;

	void startParticle( Particle & p );

	void moveParticle( Particle & p, float deltaTime );

	void sizeParticle( Particle & p, float & size );

	void colorParticle( Particle & p, Color4 & color );
};

#endif
