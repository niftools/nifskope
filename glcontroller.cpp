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

#include "glcontroller.h"
#include "glscene.h"

Controller::Controller( const NifModel * nif, const QModelIndex & index )
{
	iBlock = index;
	
	start = nif->get<float>( index, "Start Time" );
	stop = nif->get<float>( index, "Stop Time" );
	phase = nif->get<float>( index, "Phase" );
	frequency = nif->get<float>( index, "Frequency" );
	flags.bits = nif->get<int>( index, "Flags" );
}

bool Controller::update( const QModelIndex & index )
{
	return ( iBlock == index || ! iBlock.isValid() );
}

float Controller::ctrlTime( float time ) const
{
	time = frequency * time + phase;
	
	if ( time >= start && time <= stop )
		return time;
	
	switch ( flags.controller.extrapolation )
	{
		case ControllerFlags::Controller::Cyclic:
			{
				float delta = stop - start;
				if ( delta <= 0 )
					return start;
				
				float x = ( time - start ) / delta;
				float y = ( x - floor( x ) ) * delta;
				
				return start + y;
			}
		case ControllerFlags::Controller::Reverse:
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
		case ControllerFlags::Controller::Constant:
		default:
			if ( time < start )
				return start;
			if ( time > stop )
				return stop;
			return time;
	}
}

bool Controller::timeIndex( float time, const NifModel * nif, const QModelIndex & array, int & i, int & j, float & x )
{
	int count;
	if ( array.isValid() && ( count = nif->rowCount( array ) ) > 0 )
	{
		if ( time <= nif->get<float>( array.child( 0, 0 ), "Time" ) )
		{
			i = j = 0;
			x = 0.0;
			return true;
		}
		if ( time >= nif->get<float>( array.child( count - 1, 0 ), "Time" ) )
		{
			i = j = count - 1;
			x = 0.0;
			return true;
		}
		
		if ( i < 0 || i >= count )
			i = 0;
		
		float tI = nif->get<float>( array.child( i, 0 ), "Time" );
		if ( time > tI )
		{
			j = i + 1;
			float tJ;
			while ( time >= ( tJ = nif->get<float>( array.child( j, 0 ), "Time" ) ) )
			{
				i = j++;
				tI = tJ;
			}
			x = ( time - tI ) / ( tJ - tI );
			return true;
		}
		else if ( time < tI )
		{
			j = i - 1;
			float tJ;
			while ( time <= ( tJ = nif->get<float>( array.child( j, 0 ), "Time" ) ) )
			{
				i = j--;
				tI = tJ;
			}
			x = ( time - tI ) / ( tJ - tI );
			return true;
		}
		else
		{
			j = i;
			x = 0.0;
			return true;
		}
	}
	else
		return false;
}

template <> bool Controller::interpolate( float & value, const QModelIndex & array, float time, int type, int & last )
{
	int next;
	float x;
	const NifModel * nif = static_cast<const NifModel *>( array.model() );
	
	if ( nif && timeIndex( time, nif, array, last, next, x ) )
	{
		float v1 = nif->get<float>( array.child( last, 0 ), "Value" );
		float v2 = nif->get<float>( array.child( next, 0 ), "Value" );
		/*
		if ( type == 2 )
		{
			float t1 = nif->get<float>( array.child( last, 0 ), "Forward" );
			float t2 = nif->get<float>( array.child( next, 0 ), "Backward" );
			
			float x2 = x * x;
			float x3 = x2 * x;
			
			value = ( 2 * x3 - 3 * x2 + 1 ) * v1 + ( - 2 * x3 + 3 * x2 ) * v2 + ( x3 - 2 * x2 + x ) * t1 + ( x3 - x2 ) * t2;
		}
		else
		*/
		value = v1 + ( v2 - v1 ) * x;
		
		return true;
	}
	
	return false;
}

template <> bool Controller::interpolate( Vector3 & value, const QModelIndex & array, float time, int type, int & last )
{
	int next;
	float x;
	const NifModel * nif = static_cast<const NifModel *>( array.model() );
	
	if ( nif && timeIndex( time, nif, array, last, next, x ) )
	{
		Vector3 v1 = nif->get<Vector3>( array.child( last, 0 ), "Pos" );
		Vector3 v2 = nif->get<Vector3>( array.child( next, 0 ), "Pos" );
		/*
		if ( type == 2 )
		{
			Vector3 t1 = nif->get<Vector3>( array.child( last, 0 ), "Forward" );
			Vector3 t2 = nif->get<Vector3>( array.child( next, 0 ), "Backward" );
			
			float x2 = x * x;
			float x3 = x2 * x;
			
			value = v1 * ( 2 * x3 - 3 * x2 + 1 ) + v2 * ( - 2 * x3 + 3 * x2 ) + t1 * ( x3 - 2 * x2 + x ) + t2 * ( x3 - x2 );
		}
		else
		*/
		value = v1 + ( v2 - v1 ) * x;
		
		return true;
	}
	
	return false;
}

template <> bool Controller::interpolate( Quat & value, const QModelIndex & array, float time, int type, int & last )
{
	int next;
	float x;
	const NifModel * nif = static_cast<const NifModel *>( array.model() );
	
	if ( nif && timeIndex( time, nif, array, last, next, x ) )
	{
		Quat v1 = nif->get<Quat>( array.child( last, 0 ), "Quat" );
		Quat v2 = nif->get<Quat>( array.child( next, 0 ), "Quat" );
		
		float a = acos( Quat::dotproduct( v1, v2 ) );
		
		if ( fabs( a ) >= 0.00005 )
		{
			float i = 1.0 / sin( a );
			value = v1 * sin( ( 1.0 - x ) * a ) * i + v2 * sin( x * a ) * i;
		}
		else
			value = v1;
		
		return true;
	}
	
	return false;
}


KeyframeController::KeyframeController( Node * node, const NifModel * nif, const QModelIndex & index )
	: NodeController( node, nif, index )
{
	iData = nif->getBlock( nif->getLink( index, "Data" ), "NiKeyframeData" );
	
	if ( iData.isValid() )
	{
		iTrans = nif->getIndex( iData, "Translations" );
		iRotate = nif->getIndex( iData, "Rotations" );
		iScale = nif->getIndex( iData, "Scales" );
		
		tTrans = nif->get<int>( iData, "Translation Type" );
		tRotate = nif->get<int>( iData, "Rotation Type" );
		tScale = nif->get<int>( iData, "Scale Type" );
	}
	
	lTrans = lRotate = lScale = 0;
}

void KeyframeController::update( float time )
{
	if ( ! flags.controller.active )
		return;
	
	time = ctrlTime( time );

	Quat q;
	if ( interpolate( q, iRotate, time, tRotate, lRotate ) )
		target->local.rotation.fromQuat( q );
	
	interpolate( target->local.translation, iTrans, time, tTrans, lTrans );
	interpolate( target->local.scale, iScale, time, tScale, lScale );
}

bool KeyframeController::update( const QModelIndex & index )
{
	if ( NodeController::update( index ) )
		return true;
	return ( iData == index || ! iData.isValid() );
}

VisibilityController::VisibilityController( Node * node, const NifModel * nif, const QModelIndex & index )
	: NodeController( node, nif, index )
{
	visLast = 0;
	
	iData = nif->getBlock( nif->getLink( index, "Data" ), "NiVisData" );
}

void VisibilityController::update( float time )
{
	if ( ! flags.controller.active )
		return;

	time = ctrlTime( time );
	
	const NifModel * nif = static_cast<const NifModel *>( iData.model() );
	if ( iData.isValid() && nif )
	{
		QModelIndex array;
		int next;
		float x;
		
		array = nif->getIndex( iData, "Keys" );
		if ( timeIndex( time, nif, array, visLast, next, x ) )
			target->flags.node.hidden = ! nif->get<int>( array.child( visLast, 0 ), "Is Visible" );
	}
}

bool VisibilityController::update( const QModelIndex & index )
{
	if ( NodeController::update( index ) )
		return true;
	return ( iData == index || ! iData.isValid() );
}

AlphaController::AlphaController( Mesh * mesh, const NifModel * nif, const QModelIndex & index )
	: MeshController( mesh, nif, index )
{
	iData = nif->getBlock( nif->getLink( index, "Data" ), "NiFloatData" );
	
	if ( iData.isValid() )
	{
		iAlpha = nif->getIndex( iData, "Keys" );
		tAlpha = nif->get<int>( iData, "Key Type" );
	}

	lAlpha = 0;	
}

void AlphaController::update( float time )
{
	if ( ! ( flags.controller.active && target->alphaEnable ) )
		return;
	
	interpolate( target->alpha, iAlpha, ctrlTime( time ), tAlpha, lAlpha );
	
	if ( target->alpha < 0 )
		target->alpha = 0;
	if ( target->alpha > 1 )
		target->alpha = 1;
}

bool AlphaController::update( const QModelIndex & index )
{
	if ( MeshController::update( index ) )
		return true;
	return ( iData == index || ! iData.isValid() );
}

MorphController::MorphController( Mesh * mesh, const NifModel * nif, const QModelIndex & index )
	: MeshController( mesh, nif, index )
{
	int dataLink = nif->getLink( index, "Data" );
	iData = nif->getBlock( dataLink, "NiMorphData" );
	if ( iData.isValid() )
	{
		QModelIndex midx = nif->getIndex( iData, "Morphs" );
		if ( midx.isValid() )
		{
			for ( int r = 0; r < nif->rowCount( midx ); r++ )
			{
				QModelIndex iKey = midx.child( r, 0 );
				
				MorphKey * key = new MorphKey;
				key->index = 0;
				key->keyType = nif->get<int>( iKey, "Key Type" );
				key->iFrames = nif->getIndex( iKey, "Frames" );
				
				QModelIndex verts = nif->getIndex( iKey, "Vectors" );
				if ( verts.isValid() )
				{
					int count = nif->rowCount( verts );
					key->verts.resize( count );
					for ( int v = 0; v < count; v++ )
						key->verts[ v ] = nif->itemData<Vector3>( verts.child( v, 0 ) );
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
	if ( ! ( iData.isValid() && flags.controller.active && morph.count() > 1 ) )
		return;
	
	time = ctrlTime( time );
	
	const NifModel * nif = static_cast<const NifModel *>( iData.model() );	
	
	if ( ! nif || target->verts.count() != morph[0]->verts.count() )
		return;
	
	target->verts = morph[0]->verts;
	
	float x;
	
	for ( int i = 1; i < morph.count(); i++ )
	{
		MorphKey * key = morph[i];
		if ( interpolate( x, key->iFrames, time, key->keyType, key->index ) )
		{
			if ( x != 0 && target->verts.count() == key->verts.count() )
			{
				//if ( x < 0 ) x = 0;
				//if ( x > 1 ) x = 1;
				for ( int v = 0; v < target->verts.count(); v++ )
					target->verts[v] += key->verts[v] * x;
			}
		}
	}
}

bool MorphController::update( const QModelIndex & index )
{
	if ( MeshController::update( index ) )
		return true;
	return ( iData == index || ! iData.isValid() );
}

TexFlipController::TexFlipController( Mesh * mesh, const NifModel * nif, const QModelIndex & index )
	: MeshController( mesh, nif, index )
{
	flipDelta = nif->get<float>( index, "Delta" );
	flipSlot = nif->get<int>( index, "Texture Slot" );
	
	iSources = nif->getIndex( index, "Sources" );
	if ( iSources.isValid() )
		iSources = nif->getIndex( iSources, "Indices" );
}

void TexFlipController::update( float time )
{
	const NifModel * nif = static_cast<const NifModel *>( iSources.model() );
	if ( ! ( flags.controller.active && iSources.isValid() && nif && flipDelta > 0 && flipSlot == 0 ) )
		return;

	int m = nif->rowCount( iSources );
	if ( m == 0 )	return;
	int r = ( (int) ( ctrlTime( time ) / flipDelta ) ) % m;

	target->iBaseTex = nif->getBlock( nif->itemValue( iSources.child( r, 0 ) ).toLink(), "NiSourceTexture" );
}

TexCoordController::TexCoordController( Mesh * mesh, const NifModel * nif, const QModelIndex & index )
	: MeshController( mesh, nif, index )
{
	iData = nif->getBlock( nif->getLink( index, "Data" ), "NiUVData" );

	if ( iData.isValid() )
	{
		QModelIndex iGroups = nif->getIndex( iData, "UV Groups" );
		if ( iGroups.isValid() && nif->rowCount( iGroups ) >= 2 )
		{
			tS = nif->get<int>( iGroups.child( 0, 0 ), "Key Type" );
			tT = nif->get<int>( iGroups.child( 1, 0 ), "Key Type" );
			
			iS = nif->getIndex( iGroups.child( 0, 0 ), "UV Keys" );
			iT = nif->getIndex( iGroups.child( 1, 0 ), "UV Keys" );
		}
	}

	lS = lT = 0;
}

void TexCoordController::update( float time )
{
	if ( ! flags.controller.active )
		return;
	
	time = ctrlTime( time );
	
	interpolate( target->texOffset[0], iS, time, tS, lS );
	interpolate( target->texOffset[1], iT, time, tT, lT );
}

bool TexCoordController::update( const QModelIndex & index )
{
	if ( MeshController::update( index ) )
		return true;
	return ( iData == index || ! iData.isValid() );
}

