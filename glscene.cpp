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

#ifdef QT_OPENGL_LIB

#include "glscene.h"

#include "nifmodel.h"

void glVertex( const Vector & v )
{
	glVertex3f( v[0], v[1], v[2] );
}
void glNormal( const Vector & v )
{
	glNormal3f( v[0], v[1], v[2] );
}
void glTranslate( const Vector & v )
{
	glTranslatef( v[0], v[1], v[2] );
}
	

Triangle::Triangle( NifModel * nif, const QModelIndex & triangle )
{
	if ( !triangle.isValid() )		{	v1 = v2 = v3 = 0; return;	}
	v1 = nif->getInt( triangle, "v1" );
	v2 = nif->getInt( triangle, "v2" );
	v3 = nif->getInt( triangle, "v3" );
}

Tristrip::Tristrip( NifModel * nif, const QModelIndex & tristrip )
{
	if ( ! tristrip.isValid() ) return;
	
	for ( int s = 0; s < nif->rowCount( tristrip ); s++ )
		vertices.append( nif->itemValue( tristrip.child( s, 0 ) ).toInt() );
}

Color::Color( const QColor & c )
{
	rgba[ 0 ] = c.redF();
	rgba[ 1 ] = c.greenF();
	rgba[ 2 ] = c.blueF();
	rgba[ 3 ] = c.alphaF();
}

BoneWeights::BoneWeights( NifModel * nif, const QModelIndex & index, int b )
{
	trans = Transform( nif, index );
	bone = b;
	
	QModelIndex idxWeights = nif->getIndex( index, "vertex weights" );
	if ( idxWeights.isValid() )
	{
		for ( int c = 0; c < nif->rowCount( idxWeights ); c++ )
		{
			QModelIndex idx = idxWeights.child( c, 0 );
			weights.append( VertexWeight( nif->getInt( idx, "index" ), nif->getFloat( idx, "weight" ) ) );
		}
	}
	else
		qWarning() << nif->getBlockNumber( index ) << "vertex weights not found";
}


/*
 *  Controller
 */


Controller::Controller( NifModel * nif, const QModelIndex & index )
{
	start = nif->getFloat( index, "start time" );
	stop = nif->getFloat( index, "stop time" );
	phase = nif->getFloat( index, "phase" );
	frequency = nif->getFloat( index, "frequency" );
	flags.bits = nif->getInt( index, "flags" );
}

float Controller::ctrlTime( float time ) const
{
	time = frequency * time + phase;
	
	if ( time >= start && time <= stop )
		return time;
	
	switch ( flags.controller.extrapolation )
	{
		case Flags::Controller::Cyclic:
			{
				float delta = stop - start;
				if ( delta <= 0 )
					return start;
				
				float x = ( time - start ) / delta;
				float y = ( x - floor( x ) ) * delta;
				
				return start + y;
			}
		case Flags::Controller::Reverse:
			{
				float delta = stop - start;
				if ( delta <= 0 )
					return start;
				
				float x = ( time - start ) / ( delta * 2 );
				float y = ( x - floor( x ) ) * delta;
				if ( y * 2 < delta )
					return start + y;
				else
					return stop - y;
			}
		case Flags::Controller::Constant:
		default:
			if ( time < start )
				return start;
			if ( time > stop )
				return stop;
			return time;
	}
}

void Controller::timeIndex( float time, const QVector<float> & times, int & i, int & j, float & x )
{
	if ( time <= times.first() )
	{
		i = j = 0;
		x = 0.0;
		return;
	}
	if ( time >= times.last() )
	{
		i = j = times.count() -1;
		x = 0.0;
		return;
	}
	
	if ( i < 0 || i >= times.count() )
		i = 0;
	
	if ( time > times[ i ] )
	{
		j = i + 1;
		while ( time >= times[ j ] )
			i = j++;
        x = ( time - times[ i ] ) / ( times[ j ] - times[ i ] );
	}
	else if ( time < times[ i ] )
	{
		j = i - 1;
		while ( time <= times[ j ] )
			i = j--;
		x = ( time - times[ i ] ) / ( times[ j ] - times[ i ] );
	}
	else
	{
		j = i;
		x = 0.0;
	}
}

KeyframeController::KeyframeController( Node * node, NifModel * nif, const QModelIndex & index )
	: NodeController( node, nif, index )
{
	transIndex = rotIndex = 0;
	
	int dataLink = nif->getInt( index, "data" );
	QModelIndex data = nif->getBlock( dataLink, "NiKeyframeData" );
	if ( data.isValid() )
	{
		QModelIndex trans = nif->getIndex( data, "translations" );
		int count = nif->rowCount( trans );
		if ( trans.isValid() && count > 0 )
		{
			transTime.resize( count );
			transData.resize( count );
			for ( int r = 0; r < count; r++ )
			{
				transTime[ r ] = nif->getFloat( trans.child( r, 0 ), "time" );
				transData[ r ] = nif->itemValue( nif->getIndex( trans.child( r, 0 ), "pos" ) );
			}
		}
		QModelIndex rot = nif->getIndex( data, "rotations" );
		count = nif->rowCount( rot );
		if ( rot.isValid() && count > 0 )
		{
			rotTime.resize( count );
			rotData.resize( count );
			for ( int r = 0; r < count; r++ )
			{
				rotTime[ r ] = nif->getFloat( rot.child( r, 0 ), "time" );
				rotData[ r ] = nif->itemValue( nif->getIndex( rot.child( r, 0 ), "quat" ) );
			}
		}
		QModelIndex scale = nif->getIndex( data, "scales" );
		count = nif->rowCount( scale );
		if ( scale.isValid() && count > 0 )
		{
			scaleTime.resize( count );
			scaleData.resize( count );
			for ( int r = 0; r < count; r++ )
			{
				scaleTime[ r ] = nif->getFloat( scale.child( r, 0 ), "time" );
				scaleData[ r ] = nif->getFloat( scale.child( r, 0 ), "value" );
			}
		}
	}
}

void KeyframeController::update( float time )
{
	if ( ! flags.controller.active )
		return;

	time = ctrlTime( time );

	int next;
	float x;
	
	if ( transTime.count() )
	{
		timeIndex( time, transTime, transIndex, next, x );
		target->local.translation = transData[ transIndex ] * ( 1.0 - x ) + transData[ next ] * x;
	}
	if ( rotTime.count() )
	{
		timeIndex( time, rotTime, rotIndex, next, x );
		target->local.rotation = Quat::interpolate( rotData[ rotIndex ], rotData[ next ], x );
	}
	if ( scaleTime.count() )
	{
		timeIndex( time, scaleTime, scaleIndex, next, x );
		target->local.scale = scaleData[ scaleIndex ] * ( 1.0 - x ) + scaleData[ next ] * x;
	}
}

VisibilityController::VisibilityController( Node * node, NifModel * nif, const QModelIndex & index )
	: NodeController( node, nif, index )
{
	visIndex = 0;
	
	int dataLink = nif->getInt( index, "data" );
	QModelIndex iData = nif->getBlock( dataLink, "NiVisData" );
	if ( iData.isValid() )
	{
		QModelIndex iKeys = nif->getIndex( iData, "keys" );
		if ( iKeys.isValid() )
		{
			int count = nif->rowCount( iKeys );
			visTime.resize( count );
			visData.resize( count );
			for ( int r = 0; r < count; r++ )
			{
				visTime[ r ] = nif->getFloat( iKeys.child( r, 0 ), "time" );
				visData[ r ] = nif->getInt( iKeys.child( r, 0 ), "is visible" );
			}
		}
	}
	else
		qWarning() << "NiVisData not found";
}

void VisibilityController::update( float time )
{
	if ( ! flags.controller.active )
		return;

	time = ctrlTime( time );
	
	int next;
	float x;
	
	if ( visTime.count() )
	{
		timeIndex( time, visTime, visIndex, next, x );
		target->flags.node.hidden = ! visData[ visIndex ];
	}
}

AlphaController::AlphaController( Mesh * mesh, NifModel * nif, const QModelIndex & index )
	: MeshController( mesh, nif, index )
{
	alphaIndex = 0;
	
	int dataLink = nif->getInt( index, "data" );
	QModelIndex data = nif->getBlock( dataLink, "NiFloatData" );
	if ( data.isValid() )
	{
		QModelIndex floats = nif->getIndex( data, "keys" );
		int count = nif->rowCount( floats );
		if ( floats.isValid() && count > 0 )
		{
			alphaTime.resize( count );
			alphaData.resize( count );
			for ( int r = 0; r < count; r++ )
			{
				alphaTime[ r ] = nif->getFloat( floats.child( r, 0 ), "time" );
				alphaData[ r ] = nif->getFloat( floats.child( r, 0 ), "value" );
			}
		}
	}
}

void AlphaController::update( float time )
{
	if ( ! ( flags.controller.active && target->alphaEnable ) )
		return;
	
	time = ctrlTime( time );
	
	int next;
	float x;
	
	if ( alphaTime.count() )
	{
		timeIndex( time, alphaTime, alphaIndex, next, x );
		target->alpha = alphaData[ alphaIndex ] * ( 1.0 - x ) + alphaData[ next ] * x;
	}
}

MorphController::MorphController( Mesh * mesh, NifModel * nif, const QModelIndex & index )
	: MeshController( mesh, nif, index )
{
	int dataLink = nif->getInt( index, "data" );
	QModelIndex data = nif->getBlock( dataLink, "NiMorphData" );
	if ( data.isValid() )
	{
		QModelIndex midx = nif->getIndex( data, "morphs" );
		if ( midx.isValid() )
		{
			for ( int r = 0; r < nif->rowCount( midx ); r++ )
			{
				QModelIndex iKey = midx.child( r, 0 );
				
				MorphKey * key = new MorphKey;
				key->index = 0;
				
				QModelIndex value = nif->getIndex( iKey, "frames" );
				
				if ( value.isValid() )
				{
					int count = nif->rowCount( value );
					key->times.resize( count );
					key->value.resize( count );
					for ( int v = 0; v < count; v++ )
					{
						key->times[ v ] = nif->getFloat( value.child( v, 0 ), "time" );
						key->value[ v ] = nif->getFloat( value.child( v, 0 ), "value" );
					}
				}
				
				QModelIndex verts = nif->getIndex( iKey, "vectors" );
				if ( verts.isValid() )
				{
					int count = nif->rowCount( verts );
					key->verts.resize( count );
					for ( int v = 0; v < count; v++ )
						key->verts[ v ] = nif->itemValue( verts.child( v, 0 ) );
				}
				
				morph.append( key );
			}
		}
	}
}

MorphController::~MorphController()
{
	qDeleteAll( morph );
}

void MorphController::update( float time )
{
	if ( ! ( flags.controller.active && morph.count() > 1 ) )
		return;
	
	time = ctrlTime( time );
	
	int next;
	float x;
	
	if ( target->verts.count() != morph[0]->verts.count() )
		return;
	
	target->verts = morph[0]->verts;
	
	for ( int i = 1; i < morph.count(); i++ )
	{
		MorphKey * key = morph[i];
		timeIndex( time, key->times, key->index, next, x );
		float value = key->value[ key->index ] * ( 1.0 - x ) + key->value[ next ] * x;
		if ( value != 0 && target->verts.count() == key->verts.count() )
		{
			for ( int v = 0; v < target->verts.count(); v++ )
				target->verts[v] += key->verts[v] * value;
		}
	}
}

TexFlipController::TexFlipController( Mesh * mesh, NifModel * nif, const QModelIndex & index )
	: MeshController( mesh, nif, index )
{
	flipIndex = 0;
	flipSlot = nif->getInt( index, "texture slot" );
	
	float delta = nif->getFloat( index, "delta" );
	
	QModelIndex iSources = nif->getIndex( index, "sources" );
	if ( iSources.isValid() )
	{
		QModelIndex iIndices = nif->getIndex( iSources, "indices" );
		if ( iIndices.isValid() )
		{
			int count = nif->rowCount( iIndices );
			flipTime.resize( count );
			flipData.resize( count );
			for ( int r = 0; r < count; r++ )
			{
				flipTime[r] = r * delta;
				QModelIndex iSource = nif->getBlock( nif->itemValue( iIndices.child( r, 0 ) ).toInt(), "NiSourceTexture" );
				if ( iSource.isValid() )
				{
					QModelIndex iTexSource = nif->getIndex( iSource, "texture source" );
					if ( iTexSource.isValid() )
					{
						flipData[r] = nif->getValue( iTexSource, "file name" ).toString();
					}
				}
			}
		}
	}
}

void TexFlipController::update( float time )
{
	if ( ! flags.controller.active )
		return;
	
	int next;
	float x;
	
	if ( flipTime.count() > 0 && flipSlot == 0 )
	{
		timeIndex( time, flipTime, flipIndex, next, x );
		target->texFile = flipData[flipIndex];
	}
}

TexCoordController::TexCoordController( Mesh * mesh, NifModel * nif, const QModelIndex & index )
	: MeshController( mesh, nif, index )
{
	QModelIndex iData = nif->getBlock( nif->getInt( index, "data" ), "NiUVData" );
	
	QModelIndex iGroups = nif->getIndex( iData, "uv groups" );
	if ( iGroups.isValid() && nif->rowCount( iGroups ) >= 2 )
	{
		for ( int r = 0; r < 2; r++ )
		{
			QModelIndex iKeys = nif->getIndex( iGroups.child( r, 0 ), "uv keys" );
			if ( iKeys.isValid() )
			{
				int count = nif->rowCount( iKeys );
				coord[r].times.resize( count );
				coord[r].value.resize( count );
				coord[r].index = 0;
				for ( int c = 0; c < count; c++ )
				{
					coord[r].times[c] = nif->getFloat( iKeys.child( c, 0 ), "time" );
					coord[r].value[c] = nif->getFloat( iKeys.child( c, 0 ), "value" );
				}
			}
			else
				qWarning() << "UV keys not found";
		}
	}
	else
		qWarning() << "UV Groups not found";
}

void TexCoordController::update( float time )
{
	if ( ! flags.controller.active )
		return;
	
	int next;
	float x;
	
	if ( coord[0].times.count() > 0 )
	{
		timeIndex( time, coord[0].times, coord[0].index, next, x );
		target->texOffsetS = coord[0].value[coord[0].index] * ( 1.0 - x ) + coord[0].value[next] * x;
	}
	if ( coord[1].times.count() > 0 )
	{
		timeIndex( time, coord[1].times, coord[1].index, next, x );
		target->texOffsetT = coord[1].value[coord[1].index] * ( 1.0 - x ) + coord[1].value[next] * x;
	}
}

/*
 *	Node
 */


Node::Node( Scene * s, Node * p ) : scene( s ), parent( p )
{
	nodeId = 0;
	flags.bits = 0;
	
	depthProp = false;
	depthTest = true;
	depthMask = true;
}

void Node::init( NifModel * nif, const QModelIndex & index )
{
	nodeId = nif->getBlockNumber( index );

	flags.bits = nif->getInt( index, "flags" ) & 1;

	local = Transform( nif, index );
	worldDirty = true;
	
	foreach( int link, nif->getChildLinks( nodeId ) )
	{
		QModelIndex block = nif->getBlock( link );
		if ( ! block.isValid() ) continue;
		QString name = nif->itemName( block );
		
		if ( nif->inherits( name, "AProperty" ) )
			setProperty( nif, block );
		else if ( nif->inherits( name, "AController" ) )
		{
			do
			{
				setController( nif, block );
				block = nif->getBlock( nif->getInt( block, "next controller" ) );
			}
			while ( block.isValid() && nif->inherits( nif->itemName( block ), "AController" ) );
		}
		else
			setSpecial( nif, block );
	}
}

void Node::setController( NifModel * nif, const QModelIndex & index )
{
	if ( nif->itemName( index ) == "NiKeyframeController" )
		controllers.append( new KeyframeController( this, nif, index ) );
	else if ( nif->itemName( index ) == "NiVisController" )
		controllers.append( new VisibilityController( this, nif, index ) );
}

void Node::setProperty( NifModel * nif, const QModelIndex & property )
{
	QString propname = nif->itemName( property );
	if ( propname == "NiZBufferProperty" )
	{
		int flags = nif->getInt( property, "flags" );
		depthTest = flags & 1;
		depthMask = flags & 2;
	}
}

void Node::setSpecial( NifModel * nif, const QModelIndex & )
{
}

const Transform & Node::worldTrans()
{
	if ( worldDirty )
	{
		if ( parent )
			world = parent->worldTrans() * local;
		else
			world = local;
		worldDirty = false;
	}
	return world;
}

Transform Node::localTransFrom( int root )
{
	Transform trans;
	Node * node = this;
	while ( node && node->nodeId != root )
	{
		trans = node->local * trans;
		node = node->parent;
	}
	if ( node )
		return trans;
	else
		return Transform();
}

bool Node::isHidden() const
{
	if ( flags.node.hidden || ! parent )
		return flags.node.hidden;
	return parent->isHidden();
}

void Node::depthBuffer( bool & test, bool & mask )
{
	if ( depthProp || ! parent )
	{
		test = depthTest;
		mask = depthMask;
		return;
	}
	parent->depthBuffer( test, mask );
}

void Node::transform()
{
	worldDirty = true;
	
	if ( scene->animate )
		foreach ( Controller * controller, controllers )
			controller->update( scene->time );
}

void Node::boundaries( Vector & min, Vector & max )
{
	min = max = worldTrans() * Vector( 0.0, 0.0, 0.0 );
}

void Node::timeBounds( float & tmin, float & tmax )
{
	if ( controllers.isEmpty() )
		return;
	
	float mn = controllers.first()->start;
	float mx = controllers.first()->stop;
	foreach ( Controller * c, controllers )
	{
		mn = qMin( mn, c->start );
		mx = qMax( mx, c->stop );
	}
	tmin = qMin( tmin, mn );
	tmax = qMax( tmax, mx );
}

void Node::draw( bool selected )
{
	if ( isHidden() )
		return;
	
	glLoadName( nodeId );
	
	glPushAttrib( GL_LIGHTING_BIT );
	glEnable( GL_DEPTH_TEST );
	glDepthMask( GL_TRUE );
	glDepthFunc( GL_ALWAYS );
	glDisable( GL_TEXTURE_2D );
	glDisable( GL_NORMALIZE );
	glDisable( GL_LIGHTING );
	glDisable( GL_COLOR_MATERIAL );
	if ( selected )
		Color( QColor( "steelblue" ).light( 180 ) ).glColor();
	else
		Color( QColor( "steelblue" ).dark( 110 ) ).glColor();
	glPointSize( 8.5 );
	glLineWidth( 2.5 );

	Vector a = scene->view * worldTrans() * Vector();
	Vector b;
	if ( parent )
		b = scene->view * parent->worldTrans() * b;
	
	glBegin( GL_POINTS );
	glVertex( a );
	glEnd();

	glBegin( GL_LINES );
	glVertex( a );
	glVertex( b );
	glEnd();
	
	glPopAttrib();
}


/*
 *  Mesh
 */


Mesh::Mesh( Scene * s, Node * p ) : Node( s, p )
{
	shininess = 33.0;
	alpha = 1.0;
	texSet = 0;
	texFilter = GL_LINEAR;
	texWrapS = texWrapT = GL_REPEAT;
	texOffsetS = texOffsetT = 0.0;
	alphaEnable = false;
	alphaSrc = GL_SRC_ALPHA;
	alphaDst = GL_ONE_MINUS_SRC_ALPHA;
	specularEnable = false;
}

void Mesh::init( NifModel * nif, const QModelIndex & index )
{
	Node::init( nif, index );
	
	if ( ! alphaEnable )
	{
		alpha = 1.0;
	}
	
	if ( ! specularEnable )
	{
		specular = Color( 0, 0, 0, alpha );
	}
}

void Mesh::setSpecial( NifModel * nif, const QModelIndex & special )
{
	QString name = nif->itemName( special );
	if ( name == "NiTriShapeData" || name == "NiTriStripsData" )
	{
		verts.clear();
		norms.clear();
		colors.clear();
		uvs.clear();
		triangles.clear();
		tristrips.clear();
		
		localCenter = nif->itemValue( nif->getIndex( special, "center" ) );
		
		QModelIndex vertices = nif->getIndex( special, "vertices" );
		if ( vertices.isValid() )
		{
			for ( int r = 0; r < nif->rowCount( vertices ); r++ )
				verts.append( nif->itemValue( nif->index( r, 0, vertices ) ) );
		}
		
		QModelIndex normals = nif->getIndex( special, "normals" );
		if ( normals.isValid() )
		{
			for ( int r = 0; r < nif->rowCount( normals ); r++ )
				norms.append( nif->itemValue( nif->index( r, 0, normals ) ) );
		}
		QModelIndex vertexcolors = nif->getIndex( special, "vertex colors" );
		if ( vertexcolors.isValid() )
		{
			for ( int r = 0; r < nif->rowCount( vertexcolors ); r++ )
				colors.append( Color( nif->itemValue( vertexcolors.child( r, 0 ) ).value<QColor>() ) );
		}
		QModelIndex uvcoord = nif->getIndex( special, "uv sets" );
		if ( uvcoord.isValid() )
		{
			QModelIndex uvcoordset = nif->index( texSet, 0, uvcoord );
			if ( uvcoordset.isValid() )
			{
				for ( int r = 0; r < nif->rowCount( uvcoordset ); r++ )
				{
					QModelIndex uv = nif->index( r, 0, uvcoordset );
					uvs.append( nif->getFloat( uv, "u" ) );
					uvs.append( nif->getFloat( uv, "v" ) );
				}
			}
		}
		
		if ( nif->itemName( special ) == "NiTriShapeData" )
		{
			QModelIndex idxTriangles = nif->getIndex( special, "triangles" );
			if ( idxTriangles.isValid() )
			{
				for ( int r = 0; r < nif->rowCount( idxTriangles ); r++ )
					triangles.append( Triangle( nif, idxTriangles.child( r, 0 ) ) );
			}
			else
				qWarning() << nif->itemName( special ) << "(" << nif->getBlockNumber( special ) << ") triangle array not found";
		}
		else
		{
			QModelIndex points = nif->getIndex( special, "points" );
			if ( points.isValid() )
			{
				for ( int r = 0; r < nif->rowCount( points ); r++ )
					tristrips.append( Tristrip( nif, points.child( r, 0 ) ) );
			}
			else
				qWarning() << nif->itemName( special ) << "(" << nif->getBlockNumber( special ) << ") 'points' array not found";
		}
	}
	else if ( name == "NiSkinInstance" )
	{
		weights.clear();
		
		int sdat = nif->getInt( special, "data" );
		QModelIndex skindata = nif->getBlock( sdat, "NiSkinData" );
		if ( ! skindata.isValid() )
		{
			qWarning() << "niskindata not found";
			return;
		}
		
		skelRoot = nif->getInt( special, "skeleton root" );
		skelTrans = Transform( nif, skindata );
		
		QVector<int> bones;
		QModelIndex idxBones = nif->getIndex( nif->getIndex( special, "bones" ), "bones" );
		if ( ! idxBones.isValid() )
		{
			qWarning() << "bones array not found";
			return;
		}
		
		for ( int b = 0; b < nif->rowCount( idxBones ); b++ )
			bones.append( nif->itemValue( nif->index( b, 0, idxBones ) ).toInt() );
		
		idxBones = nif->getIndex( skindata, "bone list" );
		if ( ! idxBones.isValid() )
		{
			qWarning() << "bone list not found";
			return;
		}
		
		for ( int b = 0; b < nif->rowCount( idxBones ) && b < bones.count(); b++ )
		{
			weights.append( BoneWeights( nif, idxBones.child( b, 0 ), bones[ b ] ) );
		}
	}
	else
		Node::setSpecial( nif, special );
}

void Mesh::setProperty( NifModel * nif, const QModelIndex & property )
{
	QString propname = nif->itemName( property );
	if ( propname == "NiMaterialProperty" )
	{
		ambient = Color( nif->getValue( property, "ambient color" ).value<QColor>() );
		diffuse = Color( nif->getValue( property, "diffuse color" ).value<QColor>() );
		specular = Color( nif->getValue( property, "specular color" ).value<QColor>() );
		emissive = Color( nif->getValue( property, "emissive color" ).value<QColor>() );
		
		shininess = nif->getFloat( property, "glossiness" ) * 1.28; // range 0 ~ 128 (nif 0~100)
		alpha = nif->getFloat( property, "alpha" );
		if ( alpha < 0.0 ) alpha = 0.0;
		if ( alpha > 1.0 ) alpha = 1.0;
		
		foreach( int link, nif->getChildLinks( nif->getBlockNumber( property ) ) )
		{
			QModelIndex block = nif->getBlock( link );
			if ( block.isValid() && nif->inherits( nif->itemName( block ), "AController" ) )
				setController( nif, block );
		}
	}
	else if ( propname == "NiTexturingProperty" )
	{
		QModelIndex basetex = nif->getIndex( property, "base texture" );
		if ( ! basetex.isValid() )	return;
		QModelIndex basetexdata = nif->getIndex( basetex, "texture data" );
		if ( ! basetexdata.isValid() )	return;
		
		switch ( nif->getInt( basetexdata, "filter mode" ) )
		{
			case 0:		texFilter = GL_NEAREST;		break;
			case 1:		texFilter = GL_LINEAR;		break;
			default:	texFilter = GL_NEAREST;		break;
			/*
			case 2:		texFilter = GL_NEAREST_MIPMAP_NEAREST;		break;
			case 3:		texFilter = GL_LINEAR_MIPMAP_NEAREST;		break;
			case 4:		texFilter = GL_NEAREST_MIPMAP_LINEAR;		break;
			case 5:		texFilter = GL_LINEAR_MIPMAP_LINEAR;		break;
			*/
		}
		switch ( nif->getInt( basetexdata, "clamp mode" ) )
		{
			case 0:		texWrapS = GL_CLAMP;	texWrapT = GL_CLAMP;	break;
			case 1:		texWrapS = GL_CLAMP;	texWrapT = GL_REPEAT;	break;
			case 2:		texWrapS = GL_REPEAT;	texWrapT = GL_CLAMP;	break;
			default:	texWrapS = GL_REPEAT;	texWrapT = GL_REPEAT;	break;
		}
		texSet = nif->getInt( basetexdata, "texture set" );
		
		QModelIndex iSource = nif->getBlock( nif->getInt( basetexdata, "source" ), "NiSourceTexture" );
		if ( iSource.isValid() )
		{
			QModelIndex iTexSource = nif->getIndex( iSource, "texture source" );
			if ( iTexSource.isValid() )
				texFile = nif->getValue( iTexSource, "file name" ).toString();
		}
		
		foreach( int link, nif->getChildLinks( nif->getBlockNumber( property ) ) )
		{
			QModelIndex block = nif->getBlock( link );
			if ( block.isValid() && nif->inherits( nif->itemName( block ), "AController" ) )
				setController( nif, block );
		}
	}
	else if ( propname == "NiAlphaProperty" )
	{
		alphaEnable = true;
		alphaSrc = GL_SRC_ALPHA; // using default blend mode
		alphaDst = GL_ONE_MINUS_SRC_ALPHA;
	}
	else if ( propname == "NiSpecularProperty" )
	{
		specularEnable = true;
	}
	else
		Node::setProperty( nif, property );
}

void Mesh::setController( NifModel * nif, const QModelIndex & controller )
{
	if ( nif->itemName( controller ) == "NiAlphaController" )
		controllers.append( new AlphaController( this, nif, controller ) );
	else if ( nif->itemName( controller ) == "NiGeomMorpherController" )
		controllers.append( new MorphController( this, nif, controller ) );
	else if ( nif->itemName( controller ) == "NiFlipController" )
		controllers.append( new TexFlipController( this, nif, controller ) );
	else if ( nif->itemName( controller ) == "NiUVController" )
		controllers.append( new TexCoordController( this, nif, controller ) );
	else
		Node::setController( nif, controller );
}

bool compareTriangles( const Triangle & tri1, const Triangle & tri2 )
{
	return ( tri1.depth < tri2.depth );
}

void Mesh::transform()
{
	Node::transform();
	
	Transform sceneTrans = scene->view * worldTrans();
	
	sceneCenter = sceneTrans * localCenter;
	
	if ( weights.count() )
	{
		transVerts.resize( verts.count() );
		transVerts.fill( Vector() );
		transNorms.resize( norms.count() );
		transNorms.fill( Vector() );
		
		foreach ( BoneWeights bw, weights )
		{
			Node * bone = scene->nodes.value( bw.bone );
			Transform trans = sceneTrans * skelTrans;
			if ( bone )
				trans = trans * bone->localTransFrom( skelRoot );
			trans = trans * bw.trans;
			
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
	}
	else
	{
		transVerts.resize( verts.count() );
		for ( int v = 0; v < verts.count(); v++ )
			transVerts[v] = sceneTrans * verts[v];
		
		transNorms.resize( norms.count() );
		Matrix natrix = sceneTrans.rotation;
		for ( int n = 0; n < norms.count(); n++ )
		{
			transNorms[n] = natrix * norms[n];
			transNorms[n].normalize();
		}
	}
	
	if ( alphaEnable )
	{
		for ( int t = 0; t < triangles.count(); t++ )
		{
			Triangle tri = triangles[t];
			triangles[t].depth = transVerts[tri.v1][2] + transVerts[tri.v2][2] + transVerts[tri.v3][2];
		}
		qStableSort( triangles.begin(), triangles.end(), compareTriangles );
	}
}

void Mesh::boundaries( Vector & min, Vector & max )
{
	if ( transVerts.count() )
	{
		min = max = transVerts[ 0 ];
		
		foreach ( Vector v, transVerts )
		{
			for ( int c = 0; c < 3; c++ )
			{
				min[ c ] = qMin( min[ c ], v[ c ] );
				max[ c ] = qMax( max[ c ], v[ c ] );
			}
		}
	}
}

void Mesh::draw( bool selected )
{
	if ( isHidden() && ! scene->drawHidden )
		return;
	
	glLoadName( nodeId );
	
	// setup material colors
	
	if ( colors.count() )
	{
		glEnable( GL_COLOR_MATERIAL );
		glColorMaterial( GL_FRONT, GL_AMBIENT_AND_DIFFUSE );
	}
	else
	{
		glDisable( GL_COLOR_MATERIAL );
	}

	Color blend( 1.0, 1.0, 1.0, alpha );
	
	glMaterialf(GL_FRONT, GL_SHININESS, shininess);
	( ambient * blend ).glMaterial( GL_FRONT, GL_AMBIENT );
	( diffuse * blend ).glMaterial( GL_FRONT, GL_DIFFUSE );
	( emissive * blend ).glMaterial( GL_FRONT, GL_EMISSION );
	( specular * blend ).glMaterial( GL_FRONT, GL_SPECULAR );
	glColor4f( 1.0, 1.0, 1.0, 1.0 );

	// setup texturing
	
	if ( ! texFile.isEmpty() && scene->texturing && uvs.count() && scene->bindTexture( texFile ) )
	{
		glEnable( GL_TEXTURE_2D );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texFilter );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, texWrapS );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, texWrapT );
		glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	}
	else
	{
		glDisable( GL_TEXTURE_2D );
	}
	
	// setup z buffer
	
	bool dTest, dMask;
	depthBuffer( dTest, dMask );
	if ( dTest )
	{
		glEnable( GL_DEPTH_TEST );
		glDepthMask( dMask ? GL_TRUE : GL_FALSE );
	}
	else
	{
		glDisable( GL_DEPTH_TEST );
	}
	glDepthFunc( GL_LEQUAL );

	// setup alpha blending

	if ( alphaEnable && scene->blending )
	{
		glEnable( GL_BLEND );
		glBlendFunc( alphaSrc, alphaDst );
	}
	else
	{
		glDisable( GL_BLEND );
	}
	
	// normalize
	
	if ( transNorms.count() > 0 )
		glEnable( GL_NORMALIZE );
	else
		glDisable( GL_NORMALIZE );
	
	// render the triangles
	
	if ( triangles.count() > 0 )
	{
		glBegin( GL_TRIANGLES );
		foreach ( Triangle tri, triangles )
		{
			if ( transVerts.count() > tri.v1 && transVerts.count() > tri.v2 && transVerts.count() > tri.v3 )
			{
				if ( transNorms.count() > tri.v1 ) glNormal( transNorms[tri.v1] );
				if ( uvs.count() > tri.v1*2 ) glTexCoord2f( uvs[tri.v1*2+0] + texOffsetS, uvs[tri.v1*2+1] + texOffsetT );
				if ( colors.count() > tri.v1 ) ( colors[tri.v1] * blend ).glColor();
				glVertex( transVerts[tri.v1] );
				if ( transNorms.count() > tri.v2 ) glNormal( transNorms[tri.v2] );
				if ( uvs.count() > tri.v2*2 ) glTexCoord2f( uvs[tri.v2*2+0] + texOffsetS, uvs[tri.v2*2+1] + texOffsetT );
				if ( colors.count() > tri.v2 ) ( colors[tri.v2] * blend ).glColor();
				glVertex( transVerts[tri.v2] );
				if ( transNorms.count() > tri.v3 ) glNormal( transNorms[tri.v3] );
				if ( uvs.count() > tri.v3*2 ) glTexCoord2f( uvs[tri.v3*2+0] + texOffsetS, uvs[tri.v3*2+1] + texOffsetT );
				if ( colors.count() > tri.v3 ) ( colors[tri.v3] * blend ).glColor();
				glVertex( transVerts[tri.v3] );
			}
		}
		glEnd();
	}
	
	// render the tristrips
	
	foreach ( Tristrip strip, tristrips )
	{
		glBegin( GL_TRIANGLE_STRIP );
		foreach ( int v, strip.vertices )
		{
			if ( transNorms.count() > v ) glNormal( transNorms[v] );
			if ( uvs.count() > v*2 ) glTexCoord2f( uvs[v*2+0] + texOffsetS, uvs[v*2+1] + texOffsetT );
			if ( colors.count() > v ) ( colors[v] * blend ).glColor();
			if ( transVerts.count() > v ) glVertex( transVerts[v] );
		}
		glEnd();
	}
	
	// draw green mesh outline if selected
	
	if ( selected )
	{
		glPushAttrib( GL_LIGHTING_BIT );
		glEnable( GL_DEPTH_TEST );
		glDepthMask( GL_TRUE );
		glDepthFunc( GL_LEQUAL );
		glDisable( GL_TEXTURE_2D );
		glDisable( GL_NORMALIZE );
		glDisable( GL_LIGHTING );
		glDisable( GL_COLOR_MATERIAL );
		glColor4f( 0.0, 1.0, 0.0, 1.0 );
		glLineWidth( 2.0 );
		
		foreach ( Triangle tri, triangles )
		{
			if ( transVerts.count() > tri.v1 && transVerts.count() > tri.v2 && transVerts.count() > tri.v3 )
			{
				glBegin( GL_LINE_STRIP );
				glVertex( transVerts[tri.v1] );
				glVertex( transVerts[tri.v2] );
				glVertex( transVerts[tri.v3] );
				glVertex( transVerts[tri.v1] );
				glEnd();
			}
		}
		
		foreach ( Tristrip strip, tristrips )
		{
			glBegin( GL_LINE_STRIP );
			foreach ( int v, strip.vertices )
			{
				if ( transVerts.count() > v )
					glVertex( transVerts[v] );
			}
			glEnd();
		}
		
		glPopAttrib();
	}
}


/*
 *  Scene
 */


Scene::Scene()
{
	texturing = true;
	blending = true;
	highlight = true;
	drawNodes = true;
	drawHidden = false;
	currentNode = 0;
	animate = true;
	
	time = timeMin = timeMax = 0.0;
}

Scene::~Scene()
{
	clear();
	textures.clear();
}

void Scene::clear()
{
	qDeleteAll( nodes ); nodes.clear();
	qDeleteAll( meshes ); meshes.clear();
	textures.clear();
	boundMin = boundMax = boundCenter = Vector( 0.0, 0.0, 0.0 );
	boundRadius = Vector( 1.0, 1.0, 1.0 );
	timeMin = timeMax = 0.0;
}

void Scene::make( NifModel * nif )
{
	clear();
	if ( ! nif ) return;
	foreach ( int link, nif->getRootLinks() )
	{
		QStack<int> nodestack;
		make( nif, link, nodestack );
	}

	if ( ! ( nodes.isEmpty() && meshes.isEmpty() ) )
	{
		timeMin = +1000000000; timeMax = -1000000000;
		foreach ( Node * node, nodes )
			node->timeBounds( timeMin, timeMax );
		foreach ( Mesh * mesh, meshes )
			mesh->timeBounds( timeMin, timeMax );
	}
	
	/*
	int v = 0;
	int t = 0;
	int s = 0;
	foreach ( Mesh * mesh, meshes )
	{
		v += mesh->verts.count();
		t += mesh->triangles.count();
		s += mesh->tristrips.count();
	}
	qWarning() << v << t << s;
	*/
}

void Scene::make( NifModel * nif, int blockNumber, QStack<int> & nodestack )
{
	QModelIndex idx = nif->getBlock( blockNumber );

	if ( ! idx.isValid() )
	{
		qWarning() << "block " << blockNumber << " not found";
		return;
	}
	
	Node * parent = ( nodestack.count() ? nodes.value( nodestack.top() ) : 0 );

	if ( nif->inherits( nif->itemName( idx ), "AParentNode" ) )
	{
		if ( nodestack.contains( blockNumber ) )
		{
			qWarning( "infinite recursive node construct detected ( %d -> %d )", nodestack.top(), blockNumber );
			return;
		}
		
		if ( nodes.contains( blockNumber ) )
		{
			qWarning( "node %d is referrenced multiple times ( %d )", blockNumber, nodestack.top() );
			return;
		}
		
		Node * node = new Node( this, parent );
		node->init( nif, idx );
		
		nodes.insert( blockNumber, node );
		
		nodestack.push( blockNumber );
		
		foreach ( int link, nif->getChildLinks( blockNumber ) )
			make( nif, link, nodestack );
		
		nodestack.pop();
	}
	else if ( nif->itemName( idx ) == "NiTriShape" || nif->itemName( idx ) == "NiTriStrips" )
	{
		Mesh * mesh = new Mesh( this, parent );
		mesh->init( nif, idx );
		meshes.append( mesh );
	}
}

bool compareMeshes( const Mesh * mesh1, const Mesh * mesh2 )
{
	// opaque meshes first (sorted from front to rear)
	// then alpha enabled meshes (sorted from rear to front)
	
	if ( mesh1->alphaEnable == mesh2->alphaEnable )
		if ( mesh1->alphaEnable )
			return ( mesh1->sceneCenter[2] < mesh2->sceneCenter[2] );
		else
			return ( mesh1->sceneCenter[2] > mesh2->sceneCenter[2] );
	else
		return mesh2->alphaEnable;
}

void Scene::transform( const Transform & trans, float time )
{
	view = trans;
	this->time = time;
	
	foreach ( Node * node, nodes )
		node->transform();
	foreach ( Mesh * mesh, meshes )
		mesh->transform();
	
	qStableSort( meshes.begin(), meshes.end(), compareMeshes );

	if ( ! ( nodes.isEmpty() && meshes.isEmpty() ) )
	{
		boundMin = Vector( +1000000000, +1000000000, +1000000000 );
		boundMax = Vector( -1000000000, -1000000000, -1000000000 );
		foreach ( Node * node, nodes )
		{
			Vector min, max;
			node->boundaries( min, max );
			boundMin = Vector::min( boundMin, min );
			boundMax = Vector::max( boundMax, max );
		}
		foreach ( Mesh * mesh, meshes )
		{
			Vector min, max;
			mesh->boundaries( min, max );
			boundMin = Vector::min( boundMin, min );
			boundMax = Vector::max( boundMax, max );
		}
		for ( int c = 0; c < 3; c++ )
		{
			boundRadius[c] = ( boundMax[c] - boundMin[c] ) / 2;
			boundCenter[c] = boundMin[c] + boundRadius[c];
		}
	}
}

void Scene::draw()
{	
	glEnable( GL_CULL_FACE );
	
	foreach ( Mesh * mesh, meshes )
		mesh->draw( highlight && mesh->id() == currentNode );

	if ( drawNodes )
		foreach ( Node * node, nodes )
			node->draw( highlight && node->id() == currentNode );
}

#endif
