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

#ifndef GLTRANSFORM_H
#define GLTRANSFORM_H

#include "niftypes.h"

#include <QVector>

class NifModel;
class QModelIndex;
class QDataStream;

class Transform
{
public:
	Transform( const NifModel * nif, const QModelIndex & transform );
	Transform()	{ scale = 1.0; }
	
	static bool canConstruct( const NifModel * nif, const QModelIndex & parent );
	
	void writeBack( NifModel * nif, const QModelIndex & transform ) const;
	
	friend Transform operator*( const Transform & t1, const Transform & t2 );
	
	Vector3 operator*( const Vector3 & v ) const
	{
		return rotation * v * scale + translation;
	}
	
	void glMultMatrix() const;
	void glLoadMatrix() const;

	Matrix rotation;
	Vector3 translation;
	float scale;

	friend QDataStream & operator<<( QDataStream & ds, const Transform & t );
	friend QDataStream & operator>>( QDataStream & ds, Transform & t );
};

class VertexWeight
{
public:
	VertexWeight()
	{ vertex = 0; weight = 0.0; }
	VertexWeight( int v, float w )
	{ vertex = v; weight = w; }
	
	int vertex;
	float weight;
};

class BoneWeights
{
public:
	BoneWeights() { bone = 0; }
	BoneWeights( const NifModel * nif, const QModelIndex & index, int b );
	
	Transform trans;
	int bone;
	QVector<VertexWeight> weights;
};

#endif
