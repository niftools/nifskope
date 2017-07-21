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

#ifndef GLCONTROLLER_H
#define GLCONTROLLER_H

#include "model/nifmodel.h"

#include <QObject> // Inherited
#include <QPersistentModelIndex>
#include <QString>


//! @file glcontroller.h Controller, Interpolator, TransformInterpolator, BSplineTransformInterpolator

class Transform;

//! Something which can be attached to anything Controllable
class Controller
{
	friend class Interpolator;

public:
	Controller( const QModelIndex & index );
	virtual ~Controller() {}

	float start = 0;
	float stop = 0;
	float phase = 0;
	float frequency = 0;

	//! Extrapolation type
	enum Extrapolation
	{
		Cyclic = 0, Reverse = 1, Constant = 2
	} extrapolation = Cyclic;

	bool active = false;

	//! Find the model index of the controller
	QModelIndex index() const { return iBlock; }

	//! Set the interpolator
	virtual void setInterpolator( const QModelIndex & iInterpolator );

	//! Set sequence name for animation groups
	virtual void setSequence( const QString & seqname );

	//! Find the type of the controller
	virtual QString typeId() const;

	//! Update for model and index
	virtual bool update( const NifModel * nif, const QModelIndex & index );

	//! Update for specified time
	virtual void updateTime( float time ) = 0;

	//! Determine the controller time based on the specified time
	float ctrlTime( float time ) const;

	/*! Interpolate given the index of an array
	 *
	 * @param[out] value		The value being interpolated
	 * @param[in]  array		The array index
	 * @param[in]  time			The scene time
	 * @param[out] lastIndex	The last index
	 */
	template <typename T> static bool interpolate( T & value, const QModelIndex & array, float time, int & lastIndex );

	/*! Interpolate given an index and the array name
	 *
	 * @param[out] value		The value being interpolated
	 * @param[in]  data			The index which houses the array
	 * @param[in]  arrayid		The name of the array
	 * @param[in]  time			The scene time
	 * @param[out] lastIndex	The last index
	 */
	template <typename T> static bool interpolate( T & value, const QModelIndex & data, const QString & arrayid, float time, int & lastindex );
	
	/*! Returns the fraction of the way between two keyframes based on the scene time
	 *
	 * @param[in]  inTime		The scene time
	 * @param[in]  nif			The NIF
	 * @param[in]  keysArray	The Keys array in the interpolator
	 * @param[out] prevFrame	The previous row in the Keys array
	 * @param[out] nextFrame	The next row in the Keys array
	 * @param[out] fraction		The current distance between the prev and next frame, as a fraction
	 */
	static bool timeIndex( float inTime, const NifModel * nif, const QModelIndex & keysArray, int & prevFrame, int & nextFrame, float & fraction );

protected:

	QPersistentModelIndex iBlock;
	QPersistentModelIndex iInterpolator;
	QPersistentModelIndex iData;
};

template <typename T> bool Controller::interpolate( T & value, const QModelIndex & data, const QString & arrayid, float time, int & lastindex )
{
	const NifModel * nif = static_cast<const NifModel *>( data.model() );

	if ( nif && data.isValid() ) {
		QModelIndex array = nif->getIndex( data, arrayid );
		return interpolate( value, array, time, lastindex );
	}

	return false;
}

class Interpolator : public QObject
{
public:
	Interpolator( Controller * owner );

	virtual bool update( const NifModel * nif, const QModelIndex & index );

protected:
	QPersistentModelIndex GetControllerData();
	Controller * parent;
};

class TransformInterpolator : public Interpolator
{
public:
	TransformInterpolator( Controller * owner );

	bool update( const NifModel * nif, const QModelIndex & index ) override;
	virtual bool updateTransform( Transform & tm, float time );

protected:
	QPersistentModelIndex iTranslations, iRotations, iScales;
	int lTrans, lRotate, lScale;
};

class BSplineTransformInterpolator : public TransformInterpolator
{
public:
	BSplineTransformInterpolator( Controller * owner );

	bool update( const NifModel * nif, const QModelIndex & index ) override;
	bool updateTransform( Transform & tm, float time ) override;

protected:
	float start = 0, stop = 0;
	QPersistentModelIndex iControl, iSpline, iBasis;
	QPersistentModelIndex lTrans, lRotate, lScale;
	uint lTransOff = USHRT_MAX, lRotateOff = USHRT_MAX, lScaleOff = USHRT_MAX;
	float lTransMult = 0, lRotateMult = 0, lScaleMult = 0;
	float lTransBias = 0, lRotateBias = 0, lScaleBias = 0;
	uint nCtrl = 0;
	int degree = 3;
};


#endif


