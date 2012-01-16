/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005-2012, NIF File Format Library and Tools
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

#include "nifeditors.h"

#include "../nifmodel.h"

#include "colorwheel.h"
#include "floatslider.h"
#include "valueedit.h"

#include <QDoubleSpinBox>
#include <QLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTimer>

NifBlockEditor::NifBlockEditor( NifModel * n, const QModelIndex & i, bool fireAndForget )
	: QWidget(), nif( n ), iBlock( i )
{
	connect( nif, SIGNAL( dataChanged( const QModelIndex &, const QModelIndex & ) ),
		this, SLOT( nifDataChanged( const QModelIndex &, const QModelIndex & ) ) );
	connect( nif, SIGNAL( modelReset() ), this, SLOT( updateData() ) );
	connect( nif, SIGNAL( destroyed() ), this, SLOT( nifDestroyed() ) );
	
	QVBoxLayout * layout = new QVBoxLayout();
	setLayout( layout );
	layouts.push( layout );

	QModelIndex iName = nif->getIndex( iBlock, "Name" );
	if ( iName.isValid() )
		add( new NifLineEdit( nif, iName ) );

	timer = new QTimer( this );
	connect( timer, SIGNAL( timeout() ), this, SLOT( updateData() ) );
	timer->setInterval( 0 );
	timer->setSingleShot( true );

	if ( fireAndForget )
	{
		setAttribute( Qt::WA_DeleteOnClose );
		
		QPushButton * btAccept = new QPushButton( tr("Accept") );
		connect( btAccept, SIGNAL( clicked() ), this, SLOT( close() ) );
		layout->addWidget( btAccept );
	}
	
	setWindowFlags( Qt::Tool | Qt::WindowStaysOnTopHint );
}

void NifBlockEditor::add( NifEditBox * box )
{
	editors.append( box );
	if ( layouts.count() == 1 && testAttribute( Qt::WA_DeleteOnClose ) )
		layouts.top()->insertWidget( layouts.top()->count() - 1, box );
	else
		layouts.top()->addWidget( box );
}

void NifBlockEditor::pushLayout( QBoxLayout * lay, const QString & name )
{
	if ( name.isEmpty() )
	{
		if ( layouts.count() == 1 && testAttribute( Qt::WA_DeleteOnClose ) )
			layouts.top()->insertLayout( layouts.top()->count() - 1, lay );
		else
			layouts.top()->addLayout( lay );
		layouts.push( lay );
	}
	else
	{
		QGroupBox * group = new QGroupBox;
		group->setTitle( name );
		group->setLayout( lay );
		
		if ( layouts.count() == 1 && testAttribute( Qt::WA_DeleteOnClose ) )
			layouts.top()->insertWidget( layouts.top()->count() - 1, group );
		else
			layouts.top()->addWidget( group );
		
		layouts.push( lay );
	}
}

void NifBlockEditor::popLayout()
{
	if ( layouts.count() > 1 )
		layouts.pop();
}

void NifBlockEditor::showEvent( QShowEvent * )
{
	updateData();
}

void NifBlockEditor::nifDestroyed()
{
	nif = 0;
	setDisabled( true );
	if ( testAttribute( Qt::WA_DeleteOnClose ) )
		close();
}

void NifBlockEditor::nifDataChanged( const QModelIndex & begin, const QModelIndex & end )
{
	if ( nif && iBlock.isValid() )
	{
		if ( nif->getBlockOrHeader( begin ) == iBlock || nif->getBlockOrHeader( end ) == iBlock )
		{
			if ( ! timer->isActive() )
				timer->start();
		}
	}
	else
		nifDestroyed();
}

void NifBlockEditor::updateData()
{
	if ( nif && iBlock.isValid() )
	{
		QString x = nif->itemName( iBlock );
		QModelIndex iName = nif->getIndex( iBlock, "Name" );
		if ( iName.isValid() )
			x += " - " + nif->get<QString>( iName );
		setWindowTitle( x );
		
		foreach ( NifEditBox * box, editors )
		{
			box->setEnabled( box->getIndex().isValid() );
			box->updateData( nif );
		}
	}
	else
		nifDestroyed();

	if ( timer->isActive() )
		timer->stop();
}


NifEditBox::NifEditBox( NifModel * n, const QModelIndex & i )
	: QGroupBox(), nif( n ), index( i )
{
	setTitle( nif->itemName( index ) );
}

QLayout * NifEditBox::getLayout()
{
	QLayout * lay = QGroupBox::layout();
	if ( ! lay )
	{
		lay = new QHBoxLayout;
		setLayout( lay );
	}
	return lay;
}

void NifEditBox::applyData()
{
	if ( nif && index.isValid() )
	{
		applyData( nif );
	}
}


NifFloatSlider::NifFloatSlider( NifModel * nif, const QModelIndex & index, float min, float max )
	: NifEditBox( nif, index )
{
	getLayout()->addWidget( slider = new FloatSlider( Qt::Horizontal, true, false ) );
	slider->setRange( min, max );
	connect( slider, SIGNAL( valueChanged( float ) ), this, SLOT( applyData() ) );
}

void NifFloatSlider::updateData( NifModel * nif )
{
	slider->setValue( nif->get<float>( index ) );
}

void NifFloatSlider::applyData( NifModel * nif )
{
	nif->set<float>( index, slider->value() );
}

NifFloatEdit::NifFloatEdit( NifModel * nif, const QModelIndex & index, float min, float max )
	: NifEditBox( nif, index )
{
	getLayout()->addWidget( spinbox = new QDoubleSpinBox() );
	spinbox->setRange( min, max );
	spinbox->setDecimals( 4 );
	connect( spinbox, SIGNAL( valueChanged( double ) ), this, SLOT( applyData() ) );
}

void NifFloatEdit::updateData( NifModel * nif )
{
	spinbox->setValue( nif->get<float>( index ) );
}

void NifFloatEdit::applyData( NifModel * nif )
{
	nif->set<float>( index, spinbox->value() );
}

NifLineEdit::NifLineEdit( NifModel * nif, const QModelIndex & index )
	: NifEditBox( nif, index )
{
	getLayout()->addWidget( line = new QLineEdit() );
	connect( line, SIGNAL( textEdited( const QString & ) ), this, SLOT( applyData() ) );
}

void NifLineEdit::updateData( NifModel * nif )
{
	line->setText( nif->get<QString>( index ) );
}

void NifLineEdit::applyData( NifModel * nif )
{
	nif->set<QString>( index, line->text() );
}


NifColorEdit::NifColorEdit( NifModel * nif, const QModelIndex & index )
	: NifEditBox( nif, index )
{
	getLayout()->addWidget( color = new ColorWheel() );
	color->setSizeHint( QSize( 140, 140 ) );
	connect( color, SIGNAL( sigColor( const QColor & ) ), this, SLOT( applyData() ) );
	if ( nif->getValue( index ).type() == NifValue::tColor4 )
	{
		getLayout()->addWidget( alpha = new AlphaSlider() );
		connect( alpha, SIGNAL( valueChanged( float ) ), this, SLOT( applyData() ) );
	}
	else
		alpha = 0;
}

void NifColorEdit::updateData( NifModel * nif )
{
	if ( alpha )
	{
		color->setColor( nif->get<Color4>( index ).toQColor() );
		alpha->setValue( nif->get<Color4>( index )[3] );
	}
	else
	{
		color->setColor( nif->get<Color3>( index ).toQColor() );
	}
}

void NifColorEdit::applyData( NifModel * nif )
{
	if ( alpha )
	{
		Color4 c4;
		c4.fromQColor( color->getColor() );
		c4[3] = alpha->value();
		nif->set<Color4>( index, c4 );
	}
	else
	{
		Color3 c3;
		c3.fromQColor( color->getColor() );
		nif->set<Color3>( index, c3 );
	}
}

NifVectorEdit::NifVectorEdit( NifModel * nif, const QModelIndex & index )
	: NifEditBox( nif, index )
{
	getLayout()->addWidget( vector = new VectorEdit() );
	connect( vector, SIGNAL( sigEdited() ), this, SLOT( applyData() ) );
}

void NifVectorEdit::updateData( NifModel * nif )
{
	NifValue val = nif->getValue( index );
	if ( val.type() == NifValue::tVector3 )
		vector->setVector3( val.get<Vector3>() );
	else if ( val.type() == NifValue::tVector2 )
		vector->setVector2( val.get<Vector2>() );
}

void NifVectorEdit::applyData( NifModel * nif )
{
	NifValue::Type type = nif->getValue( index ).type();
	if ( type == NifValue::tVector3 )
		nif->set<Vector3>( index, vector->getVector3() );
	else if ( type == NifValue::tVector2 )
		nif->set<Vector2>( index, vector->getVector2() );
}

NifRotationEdit::NifRotationEdit( NifModel * nif, const QModelIndex & index )
	: NifEditBox( nif, index )
{
	getLayout()->addWidget( rotation = new RotationEdit() );
	connect( rotation, SIGNAL( sigEdited() ), this, SLOT( applyData() ) );
}

void NifRotationEdit::updateData( NifModel * nif )
{
	NifValue val = nif->getValue( index );
	if ( val.type() == NifValue::tMatrix )
		rotation->setMatrix( val.get<Matrix>() );
	else if ( val.type() == NifValue::tQuat || val.type() == NifValue::tQuatXYZW )
		rotation->setQuat( val.get<Quat>() );
}

void NifRotationEdit::applyData( NifModel * nif )
{
	NifValue::Type type = nif->getValue( index ).type();
	if ( type == NifValue::tMatrix )
		nif->set<Matrix>( index, rotation->getMatrix() );
	else if ( type == NifValue::tQuat || type == NifValue::tQuatXYZW )
		nif->set<Quat>( index, rotation->getQuat() );
}

NifMatrix4Edit::NifMatrix4Edit( NifModel * nif, const QModelIndex & index )
	: NifEditBox( nif, index ), setting( false )
{
	QBoxLayout * vbox = new QVBoxLayout;
	setLayout( vbox );
	
	QGroupBox * group = new QGroupBox;
	vbox->addWidget( group );
	group->setTitle( tr("Translation") );
	group->setLayout( new QHBoxLayout );
	group->layout()->addWidget( translation = new VectorEdit() );
	connect( translation, SIGNAL( sigEdited() ), this, SLOT( applyData() ) );
	
	group = new QGroupBox;
	vbox->addWidget( group );
	group->setTitle( tr("Rotation") );
	group->setLayout( new QHBoxLayout );
	group->layout()->addWidget( rotation = new RotationEdit() );
	connect( rotation, SIGNAL( sigEdited() ), this, SLOT( applyData() ) );
	
	group = new QGroupBox;
	vbox->addWidget( group );
	group->setTitle( tr("Scale") );
	group->setLayout( new QHBoxLayout );
	group->layout()->addWidget( scale = new VectorEdit() );
	connect( scale, SIGNAL( sigEdited() ), this, SLOT( applyData() ) );
}

void NifMatrix4Edit::updateData( NifModel * nif )
{
	if ( setting )
		return;
	Matrix4 mtx = nif->get<Matrix4>( index );
	
	Vector3 t, s;
	Matrix r;
	
	mtx.decompose( t, r, s );
	
	translation->setVector3( t );
	rotation->setMatrix( r );
	scale->setVector3( s );
}

void NifMatrix4Edit::applyData( NifModel * nif )
{
	setting = true;
	Matrix4 mtx;
	mtx.compose( translation->getVector3(), rotation->getMatrix(), scale->getVector3() );
	nif->set<Matrix4>( index, mtx );
	setting = false;
}

