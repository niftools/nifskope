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

#ifndef NIFDELEGATE_H
#define NIFDELEGATE_H

#include "niftypes.h"

#include <QWidget>

class QSpinBox;
class QDoubleSpinBox;

class ValueEdit : public QWidget
{
	Q_OBJECT
public:
	ValueEdit( QWidget * parent = 0 );
	
	Q_PROPERTY( NifValue value READ getValue WRITE setValue );
	
	NifValue getValue() const;
	
	static bool canEdit( NifValue::Type t );

public slots:
	void setValue( const NifValue & v );
	
protected:
	void resizeEditor();
	void resizeEvent( QResizeEvent * e );

private:
	NifValue::Type typ;
	
	QWidget * edit;
};

class VectorEdit : public QWidget
{
	Q_OBJECT
public:
	VectorEdit( QWidget * parent = 0 );
	
	Q_PROPERTY( Vector3 vector3 READ getVector3 WRITE setVector3 STORED false );
	Q_PROPERTY( Vector2 vector2 READ getVector2 WRITE setVector2 STORED false );
	
	Vector3 getVector3() const;
	Vector2 getVector2() const;

public slots:
	void setVector3( const Vector3 & );
	void setVector2( const Vector2 & );

private:
	QDoubleSpinBox * x;
	QDoubleSpinBox * y;
	QDoubleSpinBox * z;
};

class RotationEdit : public QWidget
{
	Q_OBJECT
public:
	RotationEdit( QWidget * parent = 0 );
	
	Q_PROPERTY( Matrix matrix READ getMatrix WRITE setMatrix STORED false );
	Q_PROPERTY( Quat quat READ getQuat WRITE setQuat STORED false );
	
	Matrix getMatrix() const;
	Quat getQuat() const;
	
public slots:
	void setMatrix( const Matrix & );
	void setQuat( const Quat & );

private:
	QDoubleSpinBox * y;
	QDoubleSpinBox * p;
	QDoubleSpinBox * r;
};

class TriangleEdit : public QWidget
{
	Q_OBJECT
public:
	TriangleEdit( QWidget * parent = 0 );
	
	Q_PROPERTY( Triangle triangle READ getTriangle WRITE setTriangle STORED false );
	
	Triangle getTriangle() const;
	
public slots:
	void setTriangle( const Triangle & );

private:
	QSpinBox * v1;
	QSpinBox * v2;
	QSpinBox * v3;
};

#endif
