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

/*
 *  Controllable
 */
 
Controllable::Controllable( Scene * s, const QModelIndex & i ) : scene( s ), iBlock( i )
{
}

Controllable::~Controllable()
{
	qDeleteAll( controllers );
}

void Controllable::clear()
{
	name = QString();
	
	qDeleteAll( controllers );
	controllers.clear();
}

void Controllable::update( const NifModel * nif, const QModelIndex & i )
{
	if ( ! iBlock.isValid() )
	{
		clear();
		return;
	}
	
	bool x = false;
	
	foreach ( Controller * ctrl, controllers )
	{
		ctrl->update( nif, i );
		if ( ctrl->index() == i )
			x = true;
	}

	if ( iBlock == i || x )
	{	
		name = nif->get<QString>( iBlock, "Name" );
		// sync the list of attached controllers
		QList<Controller*> rem( controllers );
		QModelIndex iCtrl = nif->getBlock( nif->getLink( iBlock, "Controller" ) );
		while ( iCtrl.isValid() && nif->inherits( iCtrl, "AController" ) )
		{
			bool add = true;
			foreach ( Controller * ctrl, controllers )
			{
				if ( ctrl->index() == iCtrl )
				{
					add = false;
					rem.removeAll( ctrl );
				}
			}
			if ( add )
			{
				setController( nif, iCtrl );
			}
			iCtrl = nif->getBlock( nif->getLink( iCtrl, "Next Controller" ) );
		}
		foreach ( Controller * ctrl, rem )
		{
			controllers.removeAll( ctrl );
			delete ctrl;
		}
	}
}

void Controllable::transform()
{
	if ( scene->animate )
		foreach ( Controller * controller, controllers )
			controller->update( scene->time );
}

void Controllable::timeBounds( float & tmin, float & tmax )
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


/*
 *  Controller
 */

Controller::Controller( const QModelIndex & index ) : iBlock( index )
{
}

bool Controller::update( const NifModel * nif, const QModelIndex & index )
{
	if ( iBlock.isValid() && iBlock == index )
	{
		start = nif->get<float>( index, "Start Time" );
		stop = nif->get<float>( index, "Stop Time" );
		phase = nif->get<float>( index, "Phase" );
		frequency = nif->get<float>( index, "Frequency" );
		flags.bits = nif->get<int>( index, "Flags" );
		
		iInterpolator = nif->getBlock( nif->getLink( iBlock, "Interpolator" ) );
		if ( iInterpolator.isValid() )
			iData = nif->getBlock( nif->getLink( iInterpolator, "Data" ) );
		else
			iData = nif->getBlock( nif->getLink( iBlock, "Data" ) );
	}
	
	if ( iInterpolator.isValid() && iInterpolator == index )
	{
		iData = nif->getBlock( nif->getLink( iInterpolator, "Data" ) );
	}
	
	return ( index.isValid() && index == iBlock || index == iInterpolator || index == iData );
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
				
				float x = ( time - start ) / delta;
				float y = ( x - floor( x ) ) * delta;
				if ( ( (int) fabs( floor( x ) ) ) & 1 == 0 )
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

template <typename T> bool interpolate( T & value, const QModelIndex & array, float time, int & last )
{
	const NifModel * nif = static_cast<const NifModel *>( array.model() );
	if ( nif && array.isValid() )
	{
		QModelIndex frames = nif->getIndex( array, "Keys" );
		int next;
		float x;
		if ( Controller::timeIndex( time, nif, frames, last, next, x ) )
		{
			T v1 = nif->get<T>( frames.child( last, 0 ), "Value" );
			T v2 = nif->get<T>( frames.child( next, 0 ), "Value" );
			
			switch ( nif->get<int>( array, "Key Type" ) )
			{
				/*
				case 2:
				{
					float t1 = nif->get<float>( frames.child( last, 0 ), "Forward" );
					float t2 = nif->get<float>( frames.child( next, 0 ), "Backward" );
					
					float x2 = x * x;
					float x3 = x2 * x;
					
					//x(t) = (2t^3 - 3t^2 + 1)*P1  + (-2t^3 + 3t^2)*P4 + (t^3 - 2t^2 + t)*R1 + (t^3 - t^2)*R4
					value = ( 2 * x3 - 3 * x2 + 1 ) * v1 + ( - 2 * x3 + 3 * x2 ) * v2 + ( x3 - 2 * x2 + x ) * t1 + ( x3 - x2 ) * t2;
				}	return true;
				*/
				case 5:
					if ( x < 0.5 )
						value = v1;
					else
						value = v2;
					return true;
				default:
					value = v1 + ( v2 - v1 ) * x;
					return true;
			}
		}
	}
	
	return false;
}

template <> bool Controller::interpolate( float & value, const QModelIndex & array, float time, int & last )
{
	return ::interpolate( value, array, time, last );
}

template <> bool Controller::interpolate( Vector3 & value, const QModelIndex & array, float time, int & last )
{
	return ::interpolate( value, array, time, last );
}

template <> bool Controller::interpolate( Color4 & value, const QModelIndex & array, float time, int & last )
{
	return ::interpolate( value, array, time, last );
}

template <> bool Controller::interpolate( Color3 & value, const QModelIndex & array, float time, int & last )
{
	return ::interpolate( value, array, time, last );
}

template <> bool Controller::interpolate( bool & value, const QModelIndex & array, float time, int & last )
{
	int next;
	float x;
	const NifModel * nif = static_cast<const NifModel *>( array.model() );
	if ( nif && array.isValid() )
	{
		QModelIndex frames = nif->getIndex( array, "Keys" );
		if ( timeIndex( time, nif, frames, last, next, x ) )
		{
			value = nif->get<int>( frames.child( last, 0 ), "Value" );
			return true;
		}
	}
	return false;
}

template <> bool Controller::interpolate( Matrix & value, const QModelIndex & array, float time, int & last )
{
	int next;
	float x;
	const NifModel * nif = static_cast<const NifModel *>( array.model() );
	if ( nif && array.isValid() )
	{
		switch ( nif->get<int>( array, "Key Type" ) )
		{
			case 4:
			{
				QModelIndex subkeys = nif->getIndex( nif->getIndex( array, "Keys Sub" ).child( 0, 0 ), "Sub Keys" );
				if ( subkeys.isValid() )
				{
					float r[3];
					for ( int s = 0; s < 3 && s < nif->rowCount( subkeys ); s++ )
					{
						r[s] = 0;
						interpolate( r[s], subkeys.child( s, 0 ), time, last );
					}
					value = Matrix::euler( 0, 0, r[2] ) * Matrix::euler( 0, r[1], 0 ) * Matrix::euler( r[0], 0, 0 );
					return true;
				}
			}	break;
			default:
			{
				QModelIndex frames = nif->getIndex( array, "Keys" );
				if ( timeIndex( time, nif, frames, last, next, x ) )
				{
					Quat v1 = nif->get<Quat>( frames.child( last, 0 ), "Value" );
					Quat v2 = nif->get<Quat>( frames.child( next, 0 ), "Value" );
					
					float a = acos( Quat::dotproduct( v1, v2 ) );
					
					if ( fabs( a ) >= 0.00005 )
					{
						float i = 1.0 / sin( a );
						v1 = v1 * sin( ( 1.0 - x ) * a ) * i + v2 * sin( x * a ) * i;
					}
					value.fromQuat( v1 );
					return true;
				}
			}	break;
		}
	}
	return false;
}
