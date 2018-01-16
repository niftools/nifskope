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

#ifndef GLSCENE_H
#define GLSCENE_H

#include "glnode.h"
#include "glproperty.h"
#include "gltools.h"

#include <QFlags>
#include <QObject>
#include <QHash>
#include <QMap>
#include <QPersistentModelIndex>
#include <QStack>
#include <QStringList>


//! @file glscene.h Scene

class NifModel;
class Renderer;
class TexCache;
class Shape;
class QAction;
class QOpenGLContext;
class QOpenGLFunctions;

class Scene final : public QObject
{
	Q_OBJECT
public:
	Scene( TexCache * texcache, QOpenGLContext * context, QOpenGLFunctions * functions, QObject * parent = nullptr );
	~Scene();

	void updateShaders();

	void clear( bool flushTextures = true );
	void make( NifModel * nif, bool flushTextures = false );
	void make( NifModel * nif, int blockNumber, QStack<int> & nodestack );

	void update( const NifModel * nif, const QModelIndex & index );

	void transform( const Transform & trans, float time = 0.0 );

	void draw();
	void drawShapes();
	void drawNodes();
	void drawHavok();
	void drawFurn();
	void drawSelection() const;

	void setSequence( const QString & seqname );

	QString textStats();

	int bindTexture( const QString & fname );
	int bindTexture( const QModelIndex & index );

	Node * getNode( const NifModel * nif, const QModelIndex & iNode );
	Property * getProperty( const NifModel * nif, const QModelIndex & iProperty );

	enum SceneOption
	{
		None = 0x0,
		ShowAxes = 0x1,
		ShowGrid = 0x2,
		ShowNodes = 0x4,
		ShowCollision = 0x8,
		ShowConstraints = 0x10,
		ShowMarkers = 0x20,
		DoDoubleSided = 0x40,
		DoVertexColors = 0x80,
		DoSpecular = 0x100,
		DoGlow = 0x200,
		DoTexturing = 0x400,
		DoBlending = 0x800,
		DoMultisampling = 0x1000,
		DoLighting = 0x2000,
		DoCubeMapping = 0x4000,
		DisableShaders = 0x8000,
		ShowHidden = 0x10000,
		DoSkinning = 0x20000,
		DoErrorColor = 0x40000
	};
	Q_DECLARE_FLAGS( SceneOptions, SceneOption );

	SceneOptions options;

	enum VisModes
	{
		VisNone = 0x0,
		VisLightPos = 0x1,
		VisNormalsOnly = 0x2,
		VisSilhouette = 0x4
	};

	Q_DECLARE_FLAGS( VisMode, VisModes );

	VisMode visMode;

	enum SelModes
	{
		SelNone = 0,
		SelObject = 1,
		SelVertex = 2
	};

	Q_DECLARE_FLAGS( SelMode, SelModes );

	SelMode selMode;

	enum LodLevel
	{
		Level0 = 0,
		Level1 = 1,
		Level2 = 2
	};

	LodLevel lodLevel;

	
	Renderer * renderer;

	NodeList nodes;
	PropertyList properties;

	NodeList roots;

	mutable QHash<int, Transform> worldTrans;
	mutable QHash<int, Transform> viewTrans;
	mutable QHash<int, Transform> bhkBodyTrans;

	Transform view;

	bool animate;

	float time;

	QString animGroup;
	QStringList animGroups;
	QMap<QString, QMap<QString, float> > animTags;

	TexCache * textures;

	QPersistentModelIndex currentBlock;
	QPersistentModelIndex currentIndex;

	QVector<Shape *> shapes;

	BoundSphere bounds() const;

	float timeMin() const;
	float timeMax() const;
signals:
	void sceneUpdated();

public slots:
	void updateSceneOptions( bool checked );
	void updateSceneOptionsGroup( QAction * );
	void updateSelectMode( QAction * );
	void updateLodLevel( int );

protected:
	mutable bool sceneBoundsValid, timeBoundsValid;
	mutable BoundSphere bndSphere;
	mutable float tMin = 0, tMax = 0;

	void updateTimeBounds() const;
};

Q_DECLARE_OPERATORS_FOR_FLAGS( Scene::SceneOptions )

Q_DECLARE_OPERATORS_FOR_FLAGS( Scene::VisMode )

#endif
