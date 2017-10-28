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

#include "glcontroller.h"

#include "gl/glscene.h"
#include "model/nifmodel.h"


//! @file glcontroller.cpp Controllable management, Interpolation management

/*
 *  IControllable
 */

IControllable::IControllable( Scene * s, const QModelIndex & i ) : scene( s ), iBlock( i )
{
}

IControllable::~IControllable()
{
	qDeleteAll( controllers );
}

QString IControllable::getName() const
{
	return name;
}

void IControllable::setController( const NifModel * nif, const QModelIndex & iController )
{
	Q_UNUSED( nif ); Q_UNUSED( iController );
}

void IControllable::clear()
{
	name = QString();

	qDeleteAll( controllers );
	controllers.clear();
}

Controller * IControllable::findController( const QString & ctrltype, const QString & var1, const QString & var2 )
{
	Q_UNUSED( var2 ); Q_UNUSED( var1 );
	Controller * ctrl = nullptr;

	for ( Controller * c : controllers ) {
		if ( c->typeId() == ctrltype ) {
			if ( !ctrl ) {
				ctrl = c;
			} else {
				ctrl = nullptr;
				// TODO: eval var1 + var2 offset to determine which controller is targeted
				break;
			}
		}
	}

	return ctrl;
}

Controller * IControllable::findController( const QModelIndex & index )
{
	for ( Controller * c : controllers ) {
		if ( c->index() == index )
			return c;
	}

	return nullptr;
}


void IControllable::update( const NifModel * nif, const QModelIndex & i )
{
	if ( !iBlock.isValid() ) {
		clear();
		return;
	}

	bool x = false;

	for ( Controller * ctrl : controllers ) {
		ctrl->update( nif, i );

		if ( ctrl->index() == i )
			x = true;
	}

	if ( iBlock == i || x ) {
		name = nif->get<QString>( iBlock, "Name" );
		// sync the list of attached controllers
		QList<Controller *> rem( controllers );
		QModelIndex iCtrl = nif->getBlock( nif->getLink( iBlock, "Controller" ) );

		while ( iCtrl.isValid() && nif->inherits( iCtrl, "NiTimeController" ) ) {
			bool add = true;

			for ( Controller * ctrl : controllers ) {
				if ( ctrl->index() == iCtrl ) {
					add = false;
					rem.removeAll( ctrl );
				}
			}

			if ( add )
				setController( nif, iCtrl );

			iCtrl = nif->getBlock( nif->getLink( iCtrl, "Next Controller" ) );
		}

		for ( Controller * ctrl : rem ) {
			controllers.removeAll( ctrl );
			delete ctrl;
		}
	}
}

void IControllable::transform()
{
	if ( scene->animate ) {
		for ( Controller * controller : controllers ) {
			controller->updateTime( scene->time );
		}
	}
}

void IControllable::timeBounds( float & tmin, float & tmax )
{
	if ( controllers.isEmpty() )
		return;

	float mn = controllers.first()->start;
	float mx = controllers.first()->stop;

	for ( Controller * c : controllers ) {
		mn = qMin( mn, c->start );
		mx = qMax( mx, c->stop );
	}
	tmin = qMin( tmin, mn );
	tmax = qMax( tmax, mx );
}

void IControllable::setSequence( const QString & seqname )
{
	for ( Controller * ctrl : controllers ) {
		ctrl->setSequence( seqname );
	}
}


/*
 *  Controller
 */

Controller::Controller( const QModelIndex & index ) : iBlock( index )
{
}

QString Controller::typeId() const
{
	if ( iBlock.isValid() )
		return iBlock.data( NifSkopeDisplayRole ).toString();

	return QString();
}

void Controller::setSequence( const QString & seqname )
{
	Q_UNUSED( seqname );
}

void Controller::setInterpolator( const QModelIndex & index )
{
	iInterpolator = index;

	const NifModel * nif = static_cast<const NifModel *>( index.model() );

	if ( nif )
		iData = nif->getBlock( nif->getLink( iInterpolator, "Data" ) );
}

bool Controller::update( const NifModel * nif, const QModelIndex & index )
{
	if ( iBlock.isValid() && iBlock == index ) {
		start = nif->get<float>( index, "Start Time" );
		stop  = nif->get<float>( index, "Stop Time" );
		phase = nif->get<float>( index, "Phase" );
		frequency = nif->get<float>( index, "Frequency" );

		int flags = nif->get<int>( index, "Flags" );
		active = flags & 0x08;
		extrapolation = (Extrapolation)( ( flags & 0x06 ) >> 1 );

		// TODO: Bit 4 (16) - Plays entire animation backwards.
		// TODO: Bit 5 (32) - Generally only set when sequences are present.
		// TODO: Bit 6 (64) - Always seems to be set on Skyrim NIFs, unknown function.

		QModelIndex idx = nif->getBlock( nif->getLink( iBlock, "Interpolator" ) );

		if ( idx.isValid() ) {
			setInterpolator( idx );
		} else {
			idx = nif->getBlock( nif->getLink( iBlock, "Data" ) );

			if ( idx.isValid() )
				iData = idx;
		}
	}

	if ( iInterpolator.isValid() && ( iInterpolator == index ) )
		iData = nif->getBlock( nif->getLink( iInterpolator, "Data" ) );

	return ( index.isValid() && ( ( index == iBlock ) || ( index == iInterpolator ) || ( index == iData ) ) );
}

float Controller::ctrlTime( float time ) const
{
	time = frequency * time + phase;

	if ( time >= start && time <= stop )
		return time;

	switch ( extrapolation ) {
	case Cyclic:
		{
			float delta = stop - start;

			if ( delta <= 0 )
				return start;

			float x = ( time - start ) / delta;
			float y = ( x - floor( x ) ) * delta;

			return start + y;
		}
	case Reverse:
		{
			float delta = stop - start;

			if ( delta <= 0 )
				return start;

			float x = ( time - start ) / delta;
			float y = ( x - floor( x ) ) * delta;

			if ( ( ( (int)fabs( floor( x ) ) ) & 1 ) == 0 )
				return start + y;

			return stop - y;
		}
	case Constant:
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

	if ( array.isValid() && ( count = nif->rowCount( array ) ) > 0 ) {
		if ( time <= nif->get<float>( array.child( 0, 0 ), "Time" ) ) {
			i = j = 0;
			x = 0.0;

			return true;
		}

		if ( time >= nif->get<float>( array.child( count - 1, 0 ), "Time" ) ) {
			i = j = count - 1;
			x = 0.0;

			return true;
		}

		if ( i < 0 || i >= count )
			i = 0;

		float tI = nif->get<float>( array.child( i, 0 ), "Time" );

		if ( time > tI ) {
			j = i + 1;
			float tJ;

			while ( time >= ( tJ = nif->get<float>( array.child( j, 0 ), "Time" ) ) ) {
				i  = j++;
				tI = tJ;
			}

			x = ( time - tI ) / ( tJ - tI );

			return true;
		} else if ( time < tI ) {
			j = i - 1;
			float tJ;

			while ( time <= ( tJ = nif->get<float>( array.child( j, 0 ), "Time" ) ) ) {
				i  = j--;
				tI = tJ;
			}

			x = ( time - tI ) / ( tJ - tI );

			// Quadratic Bug Fix

			// Invert x
			//	Previously, this branch was causing x to decrement from 1.0.
			//	(This works fine for linear interpolation apparently)
			x = 1.0 - x;
			
			// Swap I and J
			//	With x inverted, we must swap I and J or the animation will reverse.
			auto tmpI = i;
			i = j;
			j = tmpI;

			// End Bug Fix

			return true;
		}

		j = i;
		x = 0.0;

		return true;
	}

	return false;
}

template <typename T> bool interpolate( T & value, const QModelIndex & array, float time, int & last )
{
	const NifModel * nif = static_cast<const NifModel *>( array.model() );

	if ( nif && array.isValid() ) {
		QModelIndex frames = nif->getIndex( array, "Keys" );
		int next;
		float x;

		if ( Controller::timeIndex( time, nif, frames, last, next, x ) ) {
			T v1 = nif->get<T>( frames.child( last, 0 ), "Value" );
			T v2 = nif->get<T>( frames.child( next, 0 ), "Value" );

			switch ( nif->get<int>( array, "Interpolation" ) ) {
			
			case 2:
			{
				// Quadratic
				/*
					In general, for keyframe values v1 = 0, v2 = 1 it appears that
					setting v1's corresponding "Backward" value to 1 and v2's
					corresponding "Forward" to 1 results in a linear interpolation.
				*/

				// Tangent 1
				float t1 = nif->get<float>( frames.child( last, 0 ), "Backward" );
				// Tangent 2
				float t2 = nif->get<float>( frames.child( next, 0 ), "Forward" );

				float x2 = x * x;
				float x3 = x2 * x;

				// Cubic Hermite spline
				//	x(t) = (2t^3 - 3t^2 + 1)P1  + (-2t^3 + 3t^2)P2 + (t^3 - 2t^2 + t)T1 + (t^3 - t^2)T2

				value = v1 * (2.0f * x3 - 3.0f * x2 + 1.0f) + v2 * (-2.0f * x3 + 3.0f * x2) + t1 * (x3 - 2.0f * x2 + x) + t2 * (x3 - x2);

			}	return true;
			
			case 5:
				// Constant
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

	if ( nif && array.isValid() ) {
		QModelIndex frames = nif->getIndex( array, "Keys" );

		if ( timeIndex( time, nif, frames, last, next, x ) ) {
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

	if ( nif && array.isValid() ) {
		switch ( nif->get<int>( array, "Rotation Type" ) ) {
		case 4:
			{
				QModelIndex subkeys = nif->getIndex( array, "XYZ Rotations" );

				if ( subkeys.isValid() ) {
					float r[3] = {};

					for ( int s = 0; s < 3 && s < nif->rowCount( subkeys ); s++ ) {
						r[s] = 0;
						interpolate( r[s], subkeys.child( s, 0 ), time, last );
					}

					value = Matrix::euler( 0, 0, r[2] ) * Matrix::euler( 0, r[1], 0 ) * Matrix::euler( r[0], 0, 0 );

					return true;
				}
			}
			break;
		default:
			{
				QModelIndex frames = nif->getIndex( array, "Quaternion Keys" );

				if ( timeIndex( time, nif, frames, last, next, x ) ) {
					Quat v1 = nif->get<Quat>( frames.child( last, 0 ), "Value" );
					Quat v2 = nif->get<Quat>( frames.child( next, 0 ), "Value" );

					if ( Quat::dotproduct( v1, v2 ) < 0 )
						v1.negate(); // don't take the long path

					Quat v3 = Quat::slerp( x, v1, v2 );
					/*
					Quat v4;
					float a = acos( Quat::dotproduct( v1, v2 ) );
					if ( fabs( a ) >= 0.00005 )
					{
					    float i = 1.0 / sin( a );
					    v4 = v1 * sin( ( 1.0 - x ) * a ) * i + v2 * sin( x * a ) * i;
					}
					*/
					value.fromQuat( v3 );

					return true;
				}
			}
			break;
		}
	}

	return false;
}

/*********************************************************************
Simple b-spline curve algorithm

Copyright 1994 by Keith Vertanen (vertankd@cda.mrs.umn.edu)

Released to the public domain (your mileage may vary)

Found at: Programmers Heaven (www.programmersheaven.com/zone3/cat415/6660.htm)
Modified by: Theo
- reformat and convert doubles to floats
- removed point structure in favor of arbitrary sized float array
**********************************************************************/

/*! Used to enable static arrays to be members of vectors */
template <typename T>
struct qarray
{
	qarray( const QModelIndex & array, uint off = 0 )
		: array_( array ), off_( off )
	{
		nif_ = static_cast<const NifModel *>( array_.model() );
	}
	qarray( const qarray & other, uint off = 0 )
		: nif_( other.nif_ ), array_( other.array_ ), off_( other.off_ + off )
	{
	}

	T operator[]( uint index ) const
	{
		return nif_->get<T>( array_.child( index + off_, 0 ) );
	}
	const NifModel * nif_;
	const QModelIndex & array_;
	uint off_;
};

template <typename T>
struct SplineTraits
{
	// Zero data
	static T & Init( T & v )
	{
		v = T();
		return v;
	}

	// Number of control points used
	static int CountOf()
	{
		return ( sizeof(T) / sizeof(float) );
	}

	// Compute point from short array and mult/bias
	static T & Compute( T & v, qarray<short> & c, float mult )
	{
		float * vf = (float *)&v; // assume default data is a vector of floats. specialize if necessary.

		for ( int i = 0; i < CountOf(); ++i )
			vf[i] = vf[i] + ( float(c[i]) / float(SHRT_MAX) ) * mult;

		return v;
	}
	static T & Adjust( T & v, float mult, float bias )
	{
		float * vf = (float *)&v;  // assume default data is a vector of floats. specialize if necessary.

		for ( int i = 0; i < CountOf(); ++i )
			vf[i] = vf[i] * mult + bias;

		return v;
	}
};

template <> struct SplineTraits<Quat>
{
	static Quat & Init( Quat & v )
	{
		v = Quat(); v[0] = 0.0f; return v;
	}
	static int CountOf() { return 4; }
	static Quat & Compute( Quat & v, qarray<short> & c, float mult )
	{
		for ( int i = 0; i < CountOf(); ++i )
			v[i] = v[i] + ( float(c[i]) / float(SHRT_MAX) ) * mult;

		return v;
	}
	static Quat & Adjust( Quat & v, float mult, float bias )
	{
		for ( int i = 0; i < CountOf(); ++i )
			v[i] = v[i] * mult + bias;

		return v;
	}
};

// calculate the blending value
static float blend( int k, int t, int * u, float v )
{
	float value;

	if ( t == 1 ) {
		// base case for the recursion
		value = ( ( u[k] <= v ) && ( v < u[k + 1] ) ) ? 1.0f : 0.0f;
	} else {
		if ( ( u[k + t - 1] == u[k] ) && ( u[k + t] == u[k + 1] ) ) // check for divide by zero
			value = 0;
		else if ( u[k + t - 1] == u[k] )                            // if a term's denominator is zero,use just the other
			value = ( u[k + t] - v) / ( u[k + t] - u[k + 1] ) * blend( k + 1, t - 1, u, v );
		else if ( u[k + t] == u[k + 1] )
			value = (v - u[k]) / (u[k + t - 1] - u[k]) * blend( k, t - 1, u, v );
		else
			value = ( v - u[k] ) / ( u[k + t - 1] - u[k] ) * blend( k, t - 1, u, v )
			        + ( u[k + t] - v ) / ( u[k + t] - u[k + 1] ) * blend( k + 1, t - 1, u, v );
	}

	return value;
}

// figure out the knots
static void compute_intervals( int * u, int n, int t )
{
	for ( int j = 0; j <= n + t; j++ ) {
		if ( j < t )
			u[j] = 0;
		else if ( ( t <= j ) && ( j <= n ) )
			u[j] = j - t + 1;
		else if ( j > n )
			u[j] = n - t + 2;  // if n-t=-2 then we're screwed, everything goes to 0
	}
}

template <typename T>
static void compute_point( int * u, int n, int t, float v, qarray<short> & control, T & output, float mult, float bias )
{
	// initialize the variables that will hold our output
	int l = SplineTraits<T>::CountOf();
	SplineTraits<T>::Init( output );

	for ( int k = 0; k <= n; k++ ) {
		qarray<short> qa( control, k * l );
		SplineTraits<T>::Compute( output, qa, blend( k, t, u, v ) );
	}

	SplineTraits<T>::Adjust( output, mult, bias );
}

template <typename T>
bool bsplineinterpolate( T & value, int degree, float interval, uint nctrl, const QModelIndex & array, uint off, float mult, float bias )
{
	if ( off == USHRT_MAX )
		return false;

	qarray<short> subArray( array, off );
	int t = degree + 1;
	int n = nctrl - 1;
	int l = SplineTraits<T>::CountOf();

	if ( interval >= float(nctrl - degree) ) {
		SplineTraits<T>::Init( value );
		qarray<short> sa( subArray, n * l );
		SplineTraits<T>::Compute( value, sa, 1.0f );
		SplineTraits<T>::Adjust( value, mult, bias );
	} else {
		int * u = new int[ n + t + 1 ];
		compute_intervals( u, n, t );
		compute_point( u, n, t, interval, subArray, value, mult, bias );
		delete [] u;
	}

	return true;
}

Interpolator::Interpolator( Controller * owner ) : parent( owner )
{
}

bool Interpolator::update( const NifModel * nif, const QModelIndex & index )
{
	Q_UNUSED( nif ); Q_UNUSED( index );
	return true;
}
QPersistentModelIndex Interpolator::GetControllerData()
{
	return parent->iData;
}

TransformInterpolator::TransformInterpolator( Controller * owner )
	: Interpolator( owner ), lTrans( 0 ), lRotate( 0 ), lScale( 0 )
{
}

bool TransformInterpolator::update( const NifModel * nif, const QModelIndex & index )
{
	if ( Interpolator::update( nif, index ) ) {
		QModelIndex iData = nif->getBlock( nif->getLink( index, "Data" ), "NiTransformData" );
		iTranslations = nif->getIndex( iData, "Translations" );
		iRotations = nif->getIndex( iData, "Rotations" );

		if ( !iRotations.isValid() )
			iRotations = iData;

		iScales = nif->getIndex( iData, "Scales" );

		return true;
	}

	return false;
}

bool TransformInterpolator::updateTransform( Transform & tm, float time )
{
	Controller::interpolate( tm.rotation, iRotations, time, lRotate );
	Controller::interpolate( tm.translation, iTranslations, time, lTrans );
	Controller::interpolate( tm.scale, iScales, time, lScale );

	return true;
}


BSplineTransformInterpolator::BSplineTransformInterpolator( Controller * owner ) : TransformInterpolator( owner )
{
}

bool BSplineTransformInterpolator::update( const NifModel * nif, const QModelIndex & index )
{
	if ( Interpolator::update( nif, index ) ) {
		start = nif->get<float>( index, "Start Time" );
		stop  = nif->get<float>( index, "Stop Time" );

		iSpline = nif->getBlock( nif->getLink( index, "Spline Data" ) );
		iBasis  = nif->getBlock( nif->getLink( index, "Basis Data" ) );

		if ( iSpline.isValid() )
			iControl = nif->getIndex( iSpline, "Compact Control Points" );

		if ( iBasis.isValid() )
			nCtrl = nif->get<uint>( iBasis, "Num Control Points" );

		auto trans = nif->getIndex( index, "Transform" );

		lTrans  = nif->getIndex( trans, "Translation" );
		lRotate = nif->getIndex( trans, "Rotation" );
		lScale  = nif->getIndex( trans, "Scale" );

		lTransOff   = nif->get<uint>( index, "Translation Handle" );
		lRotateOff  = nif->get<uint>( index, "Rotation Handle" );
		lScaleOff   = nif->get<uint>( index, "Scale Handle" );
		lTransMult  = nif->get<float>( index, "Translation Half Range" );
		lRotateMult = nif->get<float>( index, "Rotation Half Range" );
		lScaleMult  = nif->get<float>( index, "Scale Half Range" );
		lTransBias  = nif->get<float>( index, "Translation Offset" );
		lRotateBias = nif->get<float>( index, "Rotation Offset" );
		lScaleBias  = nif->get<float>( index, "Scale Offset" );

		return true;
	}

	return false;
}

bool BSplineTransformInterpolator::updateTransform( Transform & transform, float time )
{
	float interval = ( ( time - start ) / ( stop - start ) ) * float(nCtrl - degree);
	Quat q = transform.rotation.toQuat();

	if ( ::bsplineinterpolate<Quat>( q, degree, interval, nCtrl, iControl, lRotateOff, lRotateMult, lRotateBias ) )
		transform.rotation.fromQuat( q );

	::bsplineinterpolate<Vector3>( transform.translation, degree, interval, nCtrl, iControl, lTransOff, lTransMult, lTransBias );
	::bsplineinterpolate<float>( transform.scale, degree, interval, nCtrl, iControl, lScaleOff, lScaleMult, lScaleBias );

	return true;
}
