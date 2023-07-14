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

#ifndef GLSHAPE_H
#define GLSHAPE_H

#include "gl/glnode.h" // Inherited
#include "gl/gltools.h"

#include <QPersistentModelIndex>
#include <QVector>
#include <QString>

//! @file glshape.h Shape

using TriStrip = QVector<quint16>;
Q_DECLARE_TYPEINFO( TriStrip, Q_MOVABLE_TYPE );
using TexCoords = QVector<Vector2>;
Q_DECLARE_TYPEINFO( TexCoords, Q_MOVABLE_TYPE );

class NifModel;

class Shape : public Node
{
	friend class MorphController;
	friend class UVController;
	friend class Renderer;

public:
	Shape( Scene * s, const QModelIndex & b );

	// IControllable

	void clear() override;
	void transform() override;

	// end IControllable

	virtual void drawVerts() const {};
	virtual QModelIndex vertexAt( int ) const { return QModelIndex(); };

protected:
	int shapeNumber;

	void setController( const NifModel * nif, const QModelIndex & controller ) override;
	void updateImpl( const NifModel * nif, const QModelIndex & index ) override;
	virtual void updateData( const NifModel* nif ) = 0;

	void boneSphere( const NifModel * nif, const QModelIndex & index ) const;

	//! Shape data
	QPersistentModelIndex iData;
	//! Tangent data
	QPersistentModelIndex iTangentData;
	//! Does the data need updating?
	bool needUpdateData = false;

	//! Skin instance
	QPersistentModelIndex iSkin;
	//! Skin data
	QPersistentModelIndex iSkinData;
	//! Skin partition
	QPersistentModelIndex iSkinPart;

	void resetSkinning();

	int numVerts = 0;

	//! Vertices
	QVector<Vector3> verts;
	//! Normals
	QVector<Vector3> norms;
	//! Vertex colors
	QVector<Color4> colors;
	//! Tangents
	QVector<Vector3> tangents;
	//! Bitangents
	QVector<Vector3> bitangents;
	//! UV coordinate sets
	QVector<TexCoords> coords;
	//! Triangles
	QVector<Triangle> triangles;
	//! Strip points
	QVector<TriStrip> tristrips;
	//! Sorted triangles
	QVector<Triangle> sortedTriangles;

	void resetVertexData();

	//! Is the transform rigid or weighted?
	bool transformRigid = true;
	//! Transformed vertices
	QVector<Vector3> transVerts;
	//! Transformed normals
	QVector<Vector3> transNorms;
	//! Transformed colors (alpha blended)
	QVector<Color4> transColors;
	//! Transformed tangents
	QVector<Vector3> transTangents;
	//! Transformed bitangents
	QVector<Vector3> transBitangents;

	//! Toggle for skinning
	bool isSkinned = false;

	int skeletonRoot = 0;
	Transform skeletonTrans;
	QVector<int> bones;
	QVector<BoneWeights> weights;
	QVector<SkinPartition> partitions;

	void resetSkeletonData();

	//! Holds the name of the shader, or "" if no shader
	QString shader = "";

	//! Shader property
	BSShaderLightingProperty * bssp = nullptr;
	//! Skyrim shader property
	BSLightingShaderProperty * bslsp = nullptr;
	//! Skyrim effect shader property
	BSEffectShaderProperty * bsesp = nullptr;

	AlphaProperty * alphaProperty = nullptr;

	//! Is shader set to double sided?
	bool isDoubleSided = false;
	//! Is shader set to animate using vertex alphas?
	bool isVertexAlphaAnimation = false;
	//! Is "Has Vertex Colors" set to Yes
	bool hasVertexColors = false;

	bool depthTest = true;
	bool depthWrite = true;
	bool drawInSecondPass = false;
	bool translucent = false;

	void updateShader();

	mutable BoundSphere boundSphere;
	mutable bool needUpdateBounds = false;

	bool isLOD = false;
};

#endif
