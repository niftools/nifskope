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

#ifndef GLCONTROLLER_H
#define GLCONTROLLER_H

#include "nifmodel.h"

class Node;
class Mesh;

typedef union
{
	quint16 bits;
	
	struct Controller
	{
		bool unknown : 1;
		enum
		{
			Cyclic = 0, Reverse = 1, Constant = 2
		} extrapolation : 2;
		bool active : 1;
	} controller;
	
} ControllerFlags;

class Controller
{
public:
	Controller( const NifModel * nif, const QModelIndex & index );
	virtual ~Controller() {}
	
	float start;
	float stop;
	float phase;
	float frequency;
	
	ControllerFlags flags;
	
	virtual void update( float time ) = 0;
	
	virtual bool update( const QModelIndex & index );
	
protected:
	float ctrlTime( float time ) const;
	
	static void timeIndex( float time, const QVector<float> & times, int & i, int & j, float & x );
	static bool timeIndex( float time, const NifModel * nif, const QModelIndex & array, int & i, int & j, float & x );
	
	template <typename T> static bool interpolate( T & value, const QModelIndex & array, float time, int keyType, int & lastIndex );
	
	QPersistentModelIndex iBlock;
};

class NodeController : public Controller
{
public:
	NodeController( Node * node, const NifModel * nif, const QModelIndex & index )
		: Controller( nif, index ), target( node ) {}
protected:
	Node * target;
};

class MeshController : public Controller
{
public:
	MeshController( Mesh * mesh, const NifModel * nif, const QModelIndex & index )
		: Controller( nif, index ), target( mesh ) {}
protected:
	Mesh * target;
};

class KeyframeController : public NodeController
{
public:
	KeyframeController( Node * node, const NifModel * nif, const QModelIndex & index );
	
	void update( float time );
	
	bool update( const QModelIndex & index );
	
protected:
	QPersistentModelIndex iData;
	
	QPersistentModelIndex iTrans, iRotate, iScale;
	
	int lTrans, lRotate, lScale, tTrans, tRotate, tScale;
};

class VisibilityController : public NodeController
{
public:
	VisibilityController( Node * node, const NifModel * nif, const QModelIndex & index );
	
	void update( float time );
	
	bool update( const QModelIndex & index );
	
protected:
	QPersistentModelIndex iData;
	
	int	visLast;
};

class AlphaController : public MeshController
{
public:
	AlphaController( Mesh * mesh, const NifModel * nif, const QModelIndex & index );
	
	void update( float time );
	
	bool update( const QModelIndex & index );

protected:
	QPersistentModelIndex iData;
	QPersistentModelIndex iAlpha;
	
	int lAlpha, tAlpha;
};

class MorphController : public MeshController
{
	struct MorphKey
	{
		QPersistentModelIndex iFrames;
		int keyType;
		QVector<Vector3> verts;
		int index;
	};
	
public:
	MorphController( Mesh * mesh, const NifModel * nif, const QModelIndex & index );
	~MorphController();
	
	void update( float time );
	
	bool update( const QModelIndex & data );
	
protected:
	QPersistentModelIndex iData;
	
	QVector<MorphKey*>	morph;
};

class TexFlipController : public MeshController
{
public:
	TexFlipController( Mesh * mesh, const NifModel * nif, const QModelIndex & index );
	
	void update( float time );

protected:
	float	flipDelta;
	int		flipSlot;
	
	QPersistentModelIndex iSources;
};

class TexCoordController : public MeshController
{
public:
	TexCoordController( Mesh * mesh, const NifModel * nif, const QModelIndex & index );
	
	void update( float time );
	
	bool update( const QModelIndex & index );
	
protected:
	QPersistentModelIndex iData;
	QPersistentModelIndex iS, iT;
	
	int lS, lT, tS, tT;
};

#endif


