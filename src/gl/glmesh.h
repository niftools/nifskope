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

#ifndef GLMESH_H
#define GLMESH_H

#include "glnode.h" // Inherited
#include "gltools.h"

#include <QPersistentModelIndex>
#include <QVector>
#include <QString>


//! @file glmesh.h Mesh

//! A mesh
class Mesh : public Node
{
	friend class MorphController;
	friend class UVController;
	friend class Renderer;

public:
	Mesh( Scene * s, const QModelIndex & b );
	~Mesh() { clear(); }

	// IControllable

	void clear() override;
	void update( const NifModel * nif, const QModelIndex & ) override;
	void transform() override;

	// end IControllable

	// Node

	void transformShapes() override;

	void drawShapes( NodeList * secondPass = nullptr, bool presort = false ) override;
	void drawSelection() const override;

	BoundSphere bounds() const override;

	bool isHidden() const override;
	QString textStats() const override;

	// end Node

protected:
	//! Sets the Controller
	void setController( const NifModel * nif, const QModelIndex & controller ) override;

	//! Shape data
	QPersistentModelIndex iData;
	//! Skin instance
	QPersistentModelIndex iSkin;
	//! Skin data
	QPersistentModelIndex iSkinData;
	//! Skin partition
	QPersistentModelIndex iSkinPart;
	//! Tangent data
	QPersistentModelIndex iTangentData;
	//! Does the data need updating?
	bool updateData;
	//! Does the skin data need updating?
	bool updateSkin;

	//! Holds the name of the shader, or "fixed function pipeline" if no shader
	QString shader;
	//! Skyrim shader property
	BSLightingShaderProperty * bslsp;
	//! Skyrim effect shader property
	BSEffectShaderProperty * bsesp;
	//! Is shader set to double sided?
	bool isDoubleSided;
	//! Is shader set to animate using vertex alphas?
	bool isVertexAlphaAnimation;
	//! Is "Has Vertex Colors" set to Yes
	bool hasVertexColors;

	bool depthTest = true;
	bool depthWrite = true;
	bool drawSecond = false;
	bool translucent = false;

	//! Vertices
	QVector<Vector3> verts;
	//! Normals
	QVector<Vector3> norms;
	//! Vertex colors
	QVector<Color4>  colors;
	//! Tangents
	QVector<Vector3> tangents;
	//! Bitangents
	QVector<Vector3> bitangents;
	//! UV coordinate sets
	QList<QVector<Vector2>> coords;
	//! Triangles
	QVector<Triangle> triangles;
	//! Strip points
	QList<QVector<quint16>> tristrips;
	//! Sorted triangles
	QVector<Triangle> sortedTriangles;
	//! Triangle indices
	QVector<quint16> indices;

	//! Is the transform rigid or weighted?
	bool transformRigid;
	//! Transformed vertices
	QVector<Vector3> transVerts;
	//! Transformed normals
	QVector<Vector3> transNorms;
	//! Transformed colors (alpha blended)
	QVector<Color4> transColors;
	//! Transformed colors (alpha removed)
	QVector<Color4> transColorsNoAlpha;
	//! Transformed tangents
	QVector<Vector3> transTangents;
	//! Transformed bitangents
	QVector<Vector3> transBitangents;

	int skeletonRoot;
	Transform skeletonTrans;
	QVector<int> bones;
	QVector<BoneWeights> weights;
	QVector<SkinPartition> partitions;

	mutable BoundSphere boundSphere;
	mutable bool updateBounds;

	static bool isBSLODPresent;
};


#endif
