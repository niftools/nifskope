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

#include "nifeditors.h"

#include "model/nifmodel.h"
#include "ui/widgets/colorwheel.h"
#include "ui/widgets/floatslider.h"
#include "ui/widgets/valueedit.h"

#include <QDoubleSpinBox>
#include <QLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTimer>


NifBlockEditor::NifBlockEditor( NifModel * n, const QModelIndex & i, bool fireAndForget )
	: QWidget(), nif( n ), iBlock( i )
{
	connect( nif, &NifModel::dataChanged, this, &NifBlockEditor::nifDataChanged );
	connect( nif, &NifModel::modelReset, this, &NifBlockEditor::updateData );
	connect( nif, &NifModel::destroyed, this, &NifBlockEditor::nifDestroyed );

	QVBoxLayout * layout = new QVBoxLayout();
	setLayout( layout );
	layouts.push( layout );

	timer = new QTimer( this );
	connect( timer, &QTimer::timeout, this, &NifBlockEditor::updateData );
	timer->setInterval( 0 );
	timer->setSingleShot( true );

	if ( fireAndForget ) {
		setAttribute( Qt::WA_DeleteOnClose );

		QHBoxLayout * btnlayout = new QHBoxLayout();

		QPushButton * btAccept = new QPushButton( tr( "Accept" ) );
		connect( btAccept, &QPushButton::clicked, this, &NifBlockEditor::close );
		btnlayout->addWidget( btAccept );

		QPushButton * btReject = new QPushButton( tr( "Reject" ) );
		connect( btReject, &QPushButton::clicked, this, &NifBlockEditor::reset );
		btnlayout->addWidget( btReject );

		layout->addLayout( btnlayout );
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

	connect( this, &NifBlockEditor::reset, box, &NifEditBox::sltReset );
}

void NifBlockEditor::pushLayout( QBoxLayout * lay, const QString & name )
{
	if ( name.isEmpty() ) {
		if ( layouts.count() == 1 && testAttribute( Qt::WA_DeleteOnClose ) )
			layouts.top()->insertLayout( layouts.top()->count() - 1, lay );
		else
			layouts.top()->addLayout( lay );

		layouts.push( lay );
	} else {
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
	nif = nullptr;
	setDisabled( true );

	if ( testAttribute( Qt::WA_DeleteOnClose ) )
		close();
}

void NifBlockEditor::nifDataChanged( const QModelIndex & begin, const QModelIndex & end )
{
	if ( nif && iBlock.isValid() ) {
		if ( nif->getBlockOrHeader( begin ) == iBlock || nif->getBlockOrHeader( end ) == iBlock ) {
			if ( !timer->isActive() )
				timer->start();
		}
	} else {
		nifDestroyed();
	}
}

void NifBlockEditor::updateData()
{
	if ( nif && iBlock.isValid() ) {
		QString x = nif->itemName( iBlock );
		QModelIndex iName = nif->getIndex( iBlock, "Name" );

		if ( iName.isValid() )
			x += " - " + nif->get<QString>( iName );

		setWindowTitle( x );

		for ( NifEditBox * box : editors ) {
			box->setEnabled( box->getIndex().isValid() );
			box->updateData( nif );
		}
	} else {
		nifDestroyed();
	}

	if ( timer->isActive() )
		timer->stop();
}


NifEditBox::NifEditBox( NifModel * n, const QModelIndex & i )
	: QGroupBox(), nif( n ), index( i )
{
	setTitle( nif->itemName( index ) );

	originalValue = n->getValue( i );
}

QLayout * NifEditBox::getLayout()
{
	QLayout * lay = QGroupBox::layout();

	if ( !lay ) {
		lay = new QHBoxLayout;
		setLayout( lay );
	}

	return lay;
}

void NifEditBox::sltReset()
{
	if ( nif && index.isValid() )
		nif->setValue( index, originalValue );
}

void NifEditBox::sltApplyData()
{
	if ( nif && index.isValid() ) {
		applyData( nif );
	}
}


NifFloatSlider::NifFloatSlider( NifModel * n, const QModelIndex & index, float min, float max )
	: NifEditBox( n, index )
{
	getLayout()->addWidget( slider = new FloatSlider( Qt::Horizontal, true, false ) );
	slider->setRange( min, max );
	connect( slider, &FloatSlider::valueChanged, this, &NifFloatSlider::sltApplyData );
}

void NifFloatSlider::updateData( NifModel * n )
{
	slider->setValue( n->get<float>( index ) );
}

void NifFloatSlider::applyData( NifModel * n )
{
	n->set<float>( index, slider->value() );
}

NifFloatEdit::NifFloatEdit( NifModel * n, const QModelIndex & index, float min, float max )
	: NifEditBox( n, index )
{
	getLayout()->addWidget( spinbox = new QDoubleSpinBox() );
	spinbox->setRange( min, max );
	spinbox->setDecimals( 4 );
	// Cast QDoubleSpinBox slot
	auto dsbValueChanged = static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged);

	connect( spinbox, dsbValueChanged, this, &NifFloatEdit::sltApplyData );
}

void NifFloatEdit::updateData( NifModel * n )
{
	spinbox->setValue( n->get<float>( index ) );
}

void NifFloatEdit::applyData( NifModel * n )
{
	n->set<float>( index, spinbox->value() );
}

NifLineEdit::NifLineEdit( NifModel * n, const QModelIndex & index )
	: NifEditBox( n, index )
{
	getLayout()->addWidget( line = new QLineEdit() );
	connect( line, &QLineEdit::textEdited, this, &NifLineEdit::sltApplyData );
}

void NifLineEdit::updateData( NifModel * n )
{
	line->setText( n->get<QString>( index ) );
}

void NifLineEdit::applyData( NifModel * n )
{
	n->set<QString>( index, line->text() );
}


NifColorEdit::NifColorEdit( NifModel * n, const QModelIndex & index )
	: NifEditBox( n, index )
{
	getLayout()->addWidget( color = new ColorWheel() );
	color->setSizeHint( QSize( 140, 140 ) );
	connect( color, &ColorWheel::sigColor, this, &NifColorEdit::sltApplyData );

	auto typ = n->getValue( index ).type();
	if ( typ == NifValue::tColor4 || typ == NifValue::tByteColor4 ) {
		getLayout()->addWidget( alpha = new AlphaSlider() );
		connect( alpha, &AlphaSlider::valueChanged, this, &NifColorEdit::sltApplyData );
	} else {
		alpha = nullptr;
	}
}

void NifColorEdit::updateData( NifModel * n )
{
	if ( alpha ) {
		color->setColor( n->get<Color4>( index ).toQColor() );
		alpha->setValue( n->get<Color4>( index )[3] );
	} else {
		color->setColor( n->get<Color3>( index ).toQColor() );
	}
}

void NifColorEdit::applyData( NifModel * n )
{
	if ( alpha ) {
		Color4 c4;
		c4.fromQColor( color->getColor() );
		c4[3] = alpha->value();
		n->set<Color4>( index, c4 );
	} else {
		Color3 c3;
		c3.fromQColor( color->getColor() );
		n->set<Color3>( index, c3 );
	}
}

NifVectorEdit::NifVectorEdit( NifModel * n, const QModelIndex & index )
	: NifEditBox( n, index )
{
	getLayout()->addWidget( vector = new VectorEdit() );
	connect( vector, &VectorEdit::sigEdited, this, &NifVectorEdit::sltApplyData );
}

void NifVectorEdit::updateData( NifModel * n )
{
	NifValue val = n->getValue( index );

	if ( val.type() == NifValue::tVector3 )
		vector->setVector3( val.get<Vector3>() );
	else if ( val.type() == NifValue::tVector2 )
		vector->setVector2( val.get<Vector2>() );
}

void NifVectorEdit::applyData( NifModel * n )
{
	NifValue::Type type = n->getValue( index ).type();

	if ( type == NifValue::tVector3 )
		n->set<Vector3>( index, vector->getVector3() );
	else if ( type == NifValue::tVector2 )
		n->set<Vector2>( index, vector->getVector2() );
}

NifRotationEdit::NifRotationEdit( NifModel * n, const QModelIndex & index )
	: NifEditBox( n, index )
{
	getLayout()->addWidget( rotation = new RotationEdit() );
	connect( rotation, &RotationEdit::sigEdited, this, &NifRotationEdit::sltApplyData );
}

void NifRotationEdit::updateData( NifModel * n )
{
	NifValue val = n->getValue( index );

	if ( val.type() == NifValue::tMatrix )
		rotation->setMatrix( val.get<Matrix>() );
	else if ( val.type() == NifValue::tQuat || val.type() == NifValue::tQuatXYZW )
		rotation->setQuat( val.get<Quat>() );
}

void NifRotationEdit::applyData( NifModel * n )
{
	NifValue::Type type = n->getValue( index ).type();

	if ( type == NifValue::tMatrix )
		n->set<Matrix>( index, rotation->getMatrix() );
	else if ( type == NifValue::tQuat || type == NifValue::tQuatXYZW )
		n->set<Quat>( index, rotation->getQuat() );
}

NifMatrix4Edit::NifMatrix4Edit( NifModel * n, const QModelIndex & index )
	: NifEditBox( n, index )
{
	QBoxLayout * vbox = new QVBoxLayout;
	setLayout( vbox );

	QGroupBox * group = new QGroupBox;
	vbox->addWidget( group );
	group->setTitle( tr( "Translation" ) );
	group->setLayout( new QHBoxLayout );
	group->layout()->addWidget( translation = new VectorEdit() );
	connect( translation, &VectorEdit::sigEdited, this, &NifMatrix4Edit::sltApplyData );

	group = new QGroupBox;
	vbox->addWidget( group );
	group->setTitle( tr( "Rotation" ) );
	group->setLayout( new QHBoxLayout );
	group->layout()->addWidget( rotation = new RotationEdit() );
	connect( rotation, &RotationEdit::sigEdited, this, &NifMatrix4Edit::sltApplyData );

	group = new QGroupBox;
	vbox->addWidget( group );
	group->setTitle( tr( "Scale" ) );
	group->setLayout( new QHBoxLayout );
	group->layout()->addWidget( scale = new VectorEdit() );
	connect( scale, &VectorEdit::sigEdited, this, &NifMatrix4Edit::sltApplyData );
}

void NifMatrix4Edit::updateData( NifModel * n )
{
	if ( setting )
		return;

	Matrix4 mtx = n->get<Matrix4>( index );

	Vector3 t, s;
	Matrix r;

	mtx.decompose( t, r, s );

	translation->setVector3( t );
	rotation->setMatrix( r );
	scale->setVector3( s );
}

void NifMatrix4Edit::applyData( NifModel * n )
{
	setting = true;
	Matrix4 mtx;
	mtx.compose( translation->getVector3(), rotation->getMatrix(), scale->getVector3() );
	n->set<Matrix4>( index, mtx );
	setting = false;
}

