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

#ifndef NIFEDITORS_H
#define NIFEDITORS_H

#include "data/nifvalue.h"

#include <QGroupBox> // Inherited
#include <QWidget>   // Inherited
#include <QBoxLayout>
#include <QPersistentModelIndex>
#include <QPointer>
#include <QStack>


class NifModel;

class NifEditBox : public QGroupBox
{
	Q_OBJECT

public:
	NifEditBox( NifModel * nif, const QModelIndex & index );

	virtual void updateData( NifModel * nif ) = 0;
	virtual void applyData( NifModel * nif )  = 0;

	QModelIndex getIndex() const { return index; }

public slots:
	void sltReset();

protected slots:
	void sltApplyData();

protected:
	QLayout * getLayout();

	QPointer<NifModel> nif;
	QPersistentModelIndex index;

	NifValue originalValue;
};

class NifBlockEditor final : public QWidget
{
	Q_OBJECT

public:
	NifBlockEditor( NifModel * nif, const QModelIndex & block, bool fireAndForget = true );

	void add( NifEditBox * );

	void pushLayout( QBoxLayout *, const QString & name = QString() );
	void popLayout();

	NifModel * getNif() { return nif; }

signals:
	void reset();

protected slots:
	void nifDataChanged( const QModelIndex &, const QModelIndex & );
	void nifDestroyed();
	void updateData();

protected:
	void showEvent( QShowEvent * ) override final;

	NifModel * nif;
	QPersistentModelIndex iBlock;

	QList<NifEditBox *> editors;
	QStack<QBoxLayout *> layouts;

	class QTimer * timer;
};

class NifFloatSlider final : public NifEditBox
{
	Q_OBJECT

public:
	NifFloatSlider( NifModel * nif, const QModelIndex & index, float min, float max );

	void updateData( NifModel * ) override final;
	void applyData( NifModel * ) override final;

protected:
	class FloatSlider * slider;
};

class NifFloatEdit final : public NifEditBox
{
	Q_OBJECT

public:
	NifFloatEdit( NifModel * nif, const QModelIndex & index, float min = -10e8, float max = +10e8 );

	void updateData( NifModel * ) override final;
	void applyData( NifModel * ) override final;

protected:
	class QDoubleSpinBox * spinbox;
};

class NifLineEdit final : public NifEditBox
{
	Q_OBJECT

public:
	NifLineEdit( NifModel * nif, const QModelIndex & index );

	void updateData( NifModel * ) override final;
	void applyData( NifModel * ) override final;

protected:
	class QLineEdit * line;
};

class NifColorEdit final : public NifEditBox
{
	Q_OBJECT

public:
	NifColorEdit( NifModel * nif, const QModelIndex & index );

	void updateData( NifModel * ) override final;
	void applyData( NifModel * ) override final;

protected:
	class ColorWheel * color;
	class AlphaSlider * alpha;
};

class NifVectorEdit final : public NifEditBox
{
	Q_OBJECT

public:
	NifVectorEdit( NifModel * nif, const QModelIndex & index );

	void updateData( NifModel * ) override final;
	void applyData( NifModel * ) override final;

protected:
	class VectorEdit * vector;
};

class NifRotationEdit final : public NifEditBox
{
	Q_OBJECT

public:
	NifRotationEdit( NifModel * nif, const QModelIndex & index );

	void updateData( NifModel * ) override final;
	void applyData( NifModel * ) override final;

protected:
	class RotationEdit * rotation;
};

class NifMatrix4Edit final : public NifEditBox
{
	Q_OBJECT

public:
	NifMatrix4Edit( NifModel * nif, const QModelIndex & index );

	void updateData( NifModel * ) override final;
	void applyData( NifModel * ) override final;

protected:
	class VectorEdit * translation;
	class RotationEdit * rotation;
	class VectorEdit * scale;

	bool setting = false;
};

#endif
