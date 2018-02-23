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

#include "glscene.h"

#include "gl/renderer.h"
#include "gl/gltex.h"
#include "gl/glcontroller.h"
#include "gl/glmesh.h"
#include "gl/bsshape.h"
#include "gl/glparticles.h"
#include "gl/gltex.h"
#include "model/nifmodel.h"

#include <QAction>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QSettings>


//! \file glscene.cpp %Scene management

Scene::Scene( TexCache * texcache, QOpenGLContext * context, QOpenGLFunctions * functions, QObject * parent ) :
	QObject( parent )
{
	renderer = new Renderer( context, functions );

	currentBlock = currentIndex = QModelIndex();
	animate = true;

	time = 0.0;
	sceneBoundsValid = timeBoundsValid = false;

	textures = texcache;

	options = ( DoLighting | DoTexturing | DoMultisampling | DoBlending | DoVertexColors | DoSpecular | DoGlow | DoCubeMapping );

	lodLevel = Level2;

	visMode = VisNone;

	selMode = SelObject;

	// Startup Defaults

	QSettings settings;
	settings.beginGroup( "Settings/Render/General/Startup Defaults" );

	if ( settings.value( "Show Axes", true ).toBool() )
		options |= ShowAxes;
	if ( settings.value( "Show Grid", true ).toBool() )
		options |= ShowGrid;
	if ( settings.value( "Show Collision" ).toBool() )
		options |= ShowCollision;
	if ( settings.value( "Show Constraints" ).toBool() )
		options |= ShowConstraints;
	if ( settings.value( "Show Markers" ).toBool() )
		options |= ShowMarkers;
	if ( settings.value( "Show Nodes" ).toBool() )
		options |= ShowNodes;
	if ( settings.value( "Show Hidden" ).toBool() )
		options |= ShowHidden;
	if ( settings.value( "Do Skinning", true ).toBool() )
		options |= DoSkinning;
	if ( settings.value( "Do Error Color", true ).toBool() )
		options |= DoErrorColor;

	settings.endGroup();
}

Scene::~Scene()
{
	delete renderer;
}

void Scene::updateShaders()
{
	renderer->updateShaders();
}

void Scene::clear( bool flushTextures )
{
	Q_UNUSED( flushTextures );
	nodes.clear();
	properties.clear();
	roots.clear();
	shapes.clear();

	animGroups.clear();
	animTags.clear();

	//if ( flushTextures )
	textures->flush();

	sceneBoundsValid = timeBoundsValid = false;
}

void Scene::update( const NifModel * nif, const QModelIndex & index )
{
	if ( !nif )
		return;

	if ( index.isValid() ) {
		QModelIndex block = nif->getBlock( index );

		if ( !block.isValid() )
			return;

		for ( Property * prop : properties.list() ) {
			prop->update( nif, block );
		}

		for ( Node * node : nodes.list() ) {
			node->update( nif, block );
		}
	} else {
		properties.validate();
		nodes.validate();

		for ( Node * n : nodes.list() ) {
			n->update( nif, QModelIndex() );
		}

		for ( Property * p : properties.list() ) {
			p->update( nif, QModelIndex() );
		}

		roots.clear();
		for ( const auto link : nif->getRootLinks() ) {
			QModelIndex iBlock = nif->getBlock( link );

			if ( iBlock.isValid() ) {
				Node * node = getNode( nif, iBlock );

				if ( node ) {
					node->makeParent( 0 );
					roots.add( node );
				}
			}
		}
	}

	timeBoundsValid = false;
}

void Scene::updateSceneOptions( bool checked )
{
	Q_UNUSED( checked );

	QAction * action = qobject_cast<QAction *>(sender());
	if ( action ) {
		options ^= SceneOptions( action->data().toInt() );
		emit sceneUpdated();
	}
}

void Scene::updateSceneOptionsGroup( QAction * action )
{
	if ( !action )
		return;

	options ^= SceneOptions( action->data().toInt() );
	emit sceneUpdated();
}

void Scene::updateSelectMode( QAction * action )
{
	if ( !action )
		return;

	selMode = SelMode( action->data().toInt() );
	emit sceneUpdated();
}

void Scene::updateLodLevel( int level )
{
	lodLevel = LodLevel( level );
}

void Scene::make( NifModel * nif, bool flushTextures )
{
	clear( flushTextures );

	if ( !nif )
		return;

	update( nif, QModelIndex() );

	if ( !animGroups.contains( animGroup ) ) {
		if ( animGroups.isEmpty() )
			animGroup = QString();
		else
			animGroup = animGroups.first();
	}

	setSequence( animGroup );
}

Node * Scene::getNode( const NifModel * nif, const QModelIndex & iNode )
{
	if ( !( nif && iNode.isValid() ) )
		return 0;

	Node * node = nodes.get( iNode );

	if ( node )
		return node;

	if ( nif->inherits( iNode, "NiNode" ) ) {
		if ( nif->itemName( iNode ) == "NiLODNode" )
			node = new LODNode( this, iNode );
		else if ( nif->itemName( iNode ) == "NiBillboardNode" )
			node = new BillboardNode( this, iNode );
		else
			node = new Node( this, iNode );
	} else if ( nif->itemName( iNode ) == "NiTriShape"
				|| nif->itemName( iNode ) == "NiTriStrips"
				|| nif->inherits( iNode, "NiTriBasedGeom" ) )
	{
		node = new Mesh( this, iNode );
		shapes += static_cast<Shape *>(node);
	} else if ( nif->checkVersion( 0x14050000, 0 )
				&& nif->itemName( iNode ) == "NiMesh" )
	{
		node = new Mesh( this, iNode );
	}
	//else if ( nif->inherits( iNode, "AParticleNode" ) || nif->inherits( iNode, "AParticleSystem" ) )
	else if ( nif->inherits( iNode, "NiParticles" ) ) {
		// ... where did AParticleSystem go?
		node = new Particles( this, iNode );
	} else if ( nif->inherits( iNode, "BSTriShape" ) ) {
		node = new BSShape( this, iNode );
		shapes += static_cast<Shape *>(node);
	} else if ( nif->inherits( iNode, "NiAVObject" ) ) {
		if ( nif->itemName( iNode ) == "BSTreeNode" )
			node = new Node( this, iNode );
	}

	if ( node ) {
		nodes.add( node );
		node->update( nif, iNode );
	}

	return node;
}

Property * Scene::getProperty( const NifModel * nif, const QModelIndex & iProperty )
{
	Property * prop = properties.get( iProperty );

	if ( prop )
		return prop;

	prop = Property::create( this, nif, iProperty );

	if ( prop )
		properties.add( prop );

	return prop;
}

void Scene::setSequence( const QString & seqname )
{
	animGroup = seqname;

	for ( Node * node : nodes.list() ) {
		node->setSequence( seqname );
	}
	for ( Property * prop : properties.list() ) {
		prop->setSequence( seqname );
	}

	timeBoundsValid = false;
}

void Scene::transform( const Transform & trans, float time )
{
	view = trans;
	this->time = time;

	worldTrans.clear();
	viewTrans.clear();
	bhkBodyTrans.clear();

	for ( Property * prop : properties.list() ) {
		prop->transform();
	}
	for ( Node * node : roots.list() ) {
		node->transform();
	}
	for ( Node * node : roots.list() ) {
		node->transformShapes();
	}

	sceneBoundsValid = false;

	// TODO: purge unused textures
}

void Scene::draw()
{
	drawShapes();

	if ( options & ShowNodes )
		drawNodes();
	if ( options & ShowCollision )
		drawHavok();
	if ( options & ShowMarkers )
		drawFurn();

	drawSelection();
}

void Scene::drawShapes()
{
	if ( options & DoBlending ) {
		NodeList secondPass;

		for ( Node * node : roots.list() ) {
			node->drawShapes( &secondPass );
		}

		if ( secondPass.list().count() > 0 )
			drawSelection(); // for transparency pass

		secondPass.alphaSort();

		for ( Node * node : secondPass.list() ) {
			node->drawShapes();
		}
	} else {
		for ( Node * node : roots.list() ) {
			node->drawShapes();
		}
	}
}

void Scene::drawNodes()
{
	for ( Node * node : roots.list() ) {
		node->draw();
	}
}

void Scene::drawHavok()
{
	for ( Node * node : roots.list() ) {
		node->drawHavok();
	}
}

void Scene::drawFurn()
{
	for ( Node * node : roots.list() ) {
		node->drawFurn();
	}
}

void Scene::drawSelection() const
{
	if ( Node::SELECTING )
		return; // do not render the selection when selecting

	for ( Node * node : nodes.list() ) {
		node->drawSelection();
	}
}

BoundSphere Scene::bounds() const
{
	if ( !sceneBoundsValid ) {
		bndSphere = BoundSphere();
		for ( Node * node : nodes.list() ) {
			if ( node->isVisible() )
				bndSphere |= node->bounds();
		}
		sceneBoundsValid = true;
	}

	return bndSphere;
}

void Scene::updateTimeBounds() const
{
	if ( !nodes.list().isEmpty() ) {
		tMin = +1000000000; tMax = -1000000000;
		for ( Node * node : nodes.list() ) {
			node->timeBounds( tMin, tMax );
		}
		for ( Property * prop : properties.list() ) {
			prop->timeBounds( tMin, tMax );
		}
	} else {
		tMin = tMax = 0;
	}

	timeBoundsValid = true;
}

float Scene::timeMin() const
{
	if ( animTags.contains( animGroup ) ) {
		if ( animTags[ animGroup ].contains( "start" ) )
			return animTags[ animGroup ][ "start" ];
	}

	if ( !timeBoundsValid )
		updateTimeBounds();

	return ( tMin > tMax ? 0 : tMin );
}

float Scene::timeMax() const
{
	if ( animTags.contains( animGroup ) ) {
		if ( animTags[ animGroup ].contains( "end" ) )
			return animTags[ animGroup ][ "end" ];
	}

	if ( !timeBoundsValid )
		updateTimeBounds();

	return ( tMin > tMax ? 0 : tMax );
}

QString Scene::textStats()
{
	for ( Node * node : nodes.list() ) {
		if ( node->index() == currentBlock ) {
			return node->textStats();
		}
	}
	return QString();
}

int Scene::bindTexture( const QString & fname )
{
	if ( !(options & DoTexturing) || fname.isEmpty() )
		return 0;

	return textures->bind( fname );
}

int Scene::bindTexture( const QModelIndex & iSource )
{
	if ( !(options & DoTexturing) || !iSource.isValid() )
		return 0;

	return textures->bind( iSource );
}

