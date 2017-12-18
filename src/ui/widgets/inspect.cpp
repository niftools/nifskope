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

#include "inspect.h"

#include "gl/glnode.h"
#include "gl/glscene.h"
#include "model/nifmodel.h"

#include <QApplication>
#include <QBuffer>
#include <QCheckBox>
#include <QClipboard>
#include <QDialog>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMimeData>
#include <QPushButton>
#include <QTextEdit>


//! \file inspect.cpp InspectView and InspectViewInternal

//! Keeps track of elements
class InspectViewInternal
{
public:
	~InspectViewInternal();

	Transform transform;
	QPushButton * btnCopyTransform;

	bool needUpdate;
	float time;
	QLabel * nameLabel;
	QLineEdit * nameText;
	QLabel * typeLabel;
	QLineEdit * typeText;
	QLabel * timeLabel;
	QLineEdit * timeText;

	QCheckBox * localCheck;

	QGroupBox * posGroup;
	QLabel * posXLabel;
	QLineEdit * posXText;
	QLabel * posYLabel;
	QLineEdit * posYText;
	QLabel * posZLabel;
	QLineEdit * posZText;

	QCheckBox * invertCheck;

	QGroupBox * rotEulGroup;

	QGroupBox * rotGroup;
	QLabel * rotWLabel;
	QLineEdit * rotWText;
	QLabel * rotXLabel;
	QLineEdit * rotXText;
	QLabel * rotYLabel;
	QLineEdit * rotYText;
	QLabel * rotZLabel;
	QLineEdit * rotZText;

	QGroupBox * eulGroup;
	QLabel * eulXLabel;
	QLineEdit * eulXText;
	QLabel * eulYLabel;
	QLineEdit * eulYText;
	QLabel * eulZLabel;
	QLineEdit * eulZText;

	QGroupBox * matGroup;
	QTextEdit * matText;

	QLabel * lenLabel;
	QLineEdit * lenText;
	QPushButton * refreshBtn;
};

InspectViewInternal::~InspectViewInternal()
{
	if ( nameLabel != 0 )  delete nameLabel;
	if ( nameText != 0 )   delete nameText;
	if ( typeLabel != 0 )  delete typeLabel;
	if ( typeText != 0 )   delete typeText;
	if ( timeLabel != 0 )  delete timeLabel;
	if ( timeText != 0 )   delete timeText;
	if ( localCheck != 0 ) delete localCheck;
	if ( posGroup != 0 )   delete posGroup;

	/*if(posXLabel != 0) delete posXLabel;
	  if(posXText != 0) delete posXText;
	  if(posYLabel != 0) delete posYLabel;
	  if(posYText != 0) delete posYText;
	  if(posZLabel != 0) delete posZLabel;
	  if(posZText != 0) delete posZText;*/

	if ( invertCheck != 0 ) delete invertCheck;
	if ( rotEulGroup != 0 ) delete rotEulGroup;

	//if(rotGroup != 0) delete rotGroup;
	/*if(rotWLabel != 0) delete rotWLabel;
	  if(rotWText != 0) delete rotWText;
	  if(rotXLabel != 0) delete rotXLabel;
	  if(rotXText != 0) delete rotXText;
	  if(rotYLabel != 0) delete rotYLabel;
	  if(rotYText != 0) delete rotYText;
	  if(rotZLabel != 0) delete rotZLabel;
	  if(rotZText != 0) delete rotZText;*/

	//if(eulGroup != 0) delete eulGroup;
	/*if(eulXLabel != 0) delete eulXLabel;
	  if(eulXText != 0) delete eulXText;
	  if(eulYLabel != 0) delete eulYLabel;
	  if(eulYText != 0) delete eulYText;
	  if(eulZLabel != 0) delete eulZLabel;
	  if(eulZText != 0) delete eulZText;*/

	if ( matGroup != 0 )   delete matGroup;
	//if(matText != 0) delete matText;
	if ( lenLabel != 0 )   delete lenLabel;
	if ( lenText != 0 )    delete lenText;
	if ( refreshBtn != 0 ) delete refreshBtn;
}

InspectView::InspectView( QWidget * parent, Qt::WindowFlags f )
	: QDialog( parent, f )
{
	nif  = nullptr;
	impl = new InspectViewInternal;

	impl->needUpdate = true;
	impl->time = 0.0f;

	impl->nameLabel = new QLabel( this );
	impl->nameLabel->setText( tr( "Name:" ) );
	impl->nameText = new QLineEdit( this );
	impl->nameText->setReadOnly( true );

	impl->typeLabel = new QLabel( this );
	impl->typeLabel->setText( tr( "Type:" ) );
	impl->typeText = new QLineEdit( this );
	impl->typeText->setReadOnly( true );

	impl->timeLabel = new QLabel( this );
	impl->timeLabel->setText( tr( "Time:" ) );
	impl->timeText = new QLineEdit( this );
	impl->timeText->setReadOnly( true );

	impl->localCheck = new QCheckBox( this );
	impl->localCheck->setCheckState( Qt::Unchecked );
	impl->localCheck->setText( tr( "Show Local Transform" ) );

	impl->btnCopyTransform = new QPushButton( this );
	impl->btnCopyTransform->setText( tr( "Copy Transform to Clipboard" ) );
	impl->btnCopyTransform->setFocus();

	impl->posGroup = new QGroupBox( this );
	impl->posGroup->setTitle( tr( "Position:" ) );
	impl->posXLabel = new QLabel( this );
	impl->posXLabel->setText( tr( "X:" ) );
	impl->posXText = new QLineEdit( this );
	impl->posXText->setReadOnly( true );
	impl->posYLabel = new QLabel( this );
	impl->posYLabel->setText( tr( "Y:" ) );
	impl->posYText = new QLineEdit( this );
	impl->posYText->setReadOnly( true );
	impl->posZLabel = new QLabel( this );
	impl->posZLabel->setText( tr( "Z:" ) );
	impl->posZText = new QLineEdit( this );
	impl->posZText->setReadOnly( true );

	QGridLayout * posGrid = new QGridLayout;
	impl->posGroup->setLayout( posGrid );
	posGrid->addWidget( impl->posXLabel,  0, 0 );
	posGrid->addWidget( impl->posXText,   0, 1 );
	posGrid->addWidget( impl->posYLabel,  1, 0 );
	posGrid->addWidget( impl->posYText,   1, 1 );
	posGrid->addWidget( impl->posZLabel,  2, 0 );
	posGrid->addWidget( impl->posZText,   2, 1 );

	impl->invertCheck = new QCheckBox( this );
	impl->invertCheck->setCheckState( Qt::Unchecked );
	impl->invertCheck->setText( tr( "Invert Rotation" ) );

	impl->rotGroup = new QGroupBox( this );
	impl->rotGroup->setTitle( tr( "Quaternion Rotation:" ) );
	impl->rotWLabel = new QLabel( this );
	impl->rotWLabel->setText( tr( "W:" ) );
	impl->rotWText = new QLineEdit( this );
	impl->rotWText->setReadOnly( true );
	impl->rotXLabel = new QLabel( this );
	impl->rotXLabel->setText( tr( "X:" ) );
	impl->rotXText = new QLineEdit( this );
	impl->rotXText->setReadOnly( true );
	impl->rotYLabel = new QLabel( this );
	impl->rotYLabel->setText( tr( "Y:" ) );
	impl->rotYText = new QLineEdit( this );
	impl->rotYText->setReadOnly( true );
	impl->rotZLabel = new QLabel( this );
	impl->rotZLabel->setText( tr( "Z:" ) );
	impl->rotZText = new QLineEdit( this );
	impl->rotZText->setReadOnly( true );

	QGridLayout * rotGrid = new QGridLayout;
	impl->rotGroup->setLayout( rotGrid );
	rotGrid->addWidget( impl->rotWLabel,  0, 0 );
	rotGrid->addWidget( impl->rotWText,   0, 1 );
	rotGrid->addWidget( impl->rotXLabel,  1, 0 );
	rotGrid->addWidget( impl->rotXText,   1, 1 );
	rotGrid->addWidget( impl->rotYLabel,  2, 0 );
	rotGrid->addWidget( impl->rotYText,   2, 1 );
	rotGrid->addWidget( impl->rotZLabel,  3, 0 );
	rotGrid->addWidget( impl->rotZText,   3, 1 );

	impl->eulGroup = new QGroupBox( this );
	impl->eulGroup->setTitle( tr( "Euler Rotation:" ) );
	impl->eulXLabel = new QLabel( this );
	impl->eulXLabel->setText( tr( "X:" ) );
	impl->eulXText = new QLineEdit( this );
	impl->eulXText->setReadOnly( true );
	impl->eulYLabel = new QLabel( this );
	impl->eulYLabel->setText( tr( "Y:" ) );
	impl->eulYText = new QLineEdit( this );
	impl->eulYText->setReadOnly( true );
	impl->eulZLabel = new QLabel( this );
	impl->eulZLabel->setText( tr( "Z:" ) );
	impl->eulZText = new QLineEdit( this );
	impl->eulZText->setReadOnly( true );

	QGridLayout * eulGrid = new QGridLayout;
	impl->eulGroup->setLayout( eulGrid );
	eulGrid->addWidget( impl->eulXLabel,  0, 0 );
	eulGrid->addWidget( impl->eulXText,   0, 1 );
	eulGrid->addWidget( impl->eulYLabel,  1, 0 );
	eulGrid->addWidget( impl->eulYText,   1, 1 );
	eulGrid->addWidget( impl->eulZLabel,  2, 0 );
	eulGrid->addWidget( impl->eulZText,   2, 1 );

	QGridLayout * rotEulGrid = new QGridLayout;
	impl->rotEulGroup = new QGroupBox( this );
	impl->rotEulGroup->setLayout( rotEulGrid );
	rotEulGrid->addWidget( impl->rotGroup, 0, 0 );
	rotEulGrid->addWidget( impl->eulGroup, 0, 1 );

	impl->matGroup = new QGroupBox( this );
	impl->matGroup->setTitle( tr( "Transform Matrix" ) );
	impl->matText = new QTextEdit( this );
	impl->matText->setLineWrapMode( QTextEdit::NoWrap );

	QGridLayout * matGrid = new QGridLayout;
	impl->matGroup->setLayout( matGrid );
	matGrid->addWidget( impl->matText );

	impl->lenLabel = new QLabel( this );
	impl->lenLabel->setText( tr( "Length:" ) );
	impl->lenText = new QLineEdit( this );
	impl->lenText->setReadOnly( true );

	impl->refreshBtn = new QPushButton( this );
	impl->refreshBtn->setText( tr( "Refresh" ) );
	impl->refreshBtn->setFocus();

	QGridLayout * grid = new QGridLayout;
	this->setLayout( grid );
	grid->addWidget( impl->nameLabel,   0, 0 );
	grid->addWidget( impl->nameText,    0, 1 );
	grid->addWidget( impl->typeLabel,   1, 0 );
	grid->addWidget( impl->typeText,    1, 1 );
	grid->addWidget( impl->timeLabel,   2, 0 );
	grid->addWidget( impl->timeText,    2, 1 );
	grid->addWidget( impl->localCheck,  3, 0, 1, 2, Qt::AlignLeft | Qt::AlignAbsolute );
	grid->addWidget( impl->btnCopyTransform,  3, 1, 1, 2, Qt::AlignRight | Qt::AlignAbsolute );
	grid->addWidget( impl->posGroup,    4, 0, 1, 2, Qt::AlignLeft | Qt::AlignAbsolute );
	grid->addWidget( impl->invertCheck, 5, 0, 1, 2, Qt::AlignLeft | Qt::AlignAbsolute );
	//grid->addWidget( impl->rotGroup,    6, 0, 1, 2, Qt::AlignLeft | Qt::AlignAbsolute );
	//grid->addWidget( impl->eulGroup,    7, 0, 1, 2, Qt::AlignLeft | Qt::AlignAbsolute );
	grid->addWidget( impl->rotEulGroup, 6, 0, 1, 2, Qt::AlignLeft | Qt::AlignAbsolute );
	grid->addWidget( impl->matGroup,    7, 0, 1, 2, Qt::AlignLeft | Qt::AlignAbsolute );
	grid->addWidget( impl->lenLabel,    8, 0 );
	grid->addWidget( impl->lenText,     8, 1 );
	grid->addWidget( impl->refreshBtn,  9, 1 );

	connect( impl->localCheck, &QCheckBox::stateChanged, this, &InspectView::update );
	connect( impl->invertCheck, &QCheckBox::stateChanged, this, &InspectView::update );
	connect( impl->refreshBtn, &QPushButton::clicked, this, &InspectView::update );
	connect( impl->btnCopyTransform, &QPushButton::clicked, this, &InspectView::copyTransformToMimedata );
}

InspectView::~InspectView()
{
	delete impl;
}

void InspectView::setNifModel( NifModel * nifModel )
{
	nif = nifModel;
}

void InspectView::setScene( Scene * scene )
{
	this->scene = scene;
}

#define Farg( X ) arg( X, 0, 'f', 6 )

void InspectView::updateSelection( const QModelIndex & select )
{
	impl->needUpdate = true;

	if ( !scene || !nif ) {
		clear();
	} else {
		selection = select;
		Node * node = scene->getNode( nif, selection );

		if ( !node ) {
			clear();
		} else {
			refresh();
		}
	}
}

void InspectView::updateTime( float t, float, float )
{
	impl->needUpdate = true;
	impl->time = t;
}

void InspectView::refresh()
{
	if ( !impl->needUpdate )
		return;

	update();
}

void InspectView::update()
{
	if ( this->isHidden() )
		return;

	impl->needUpdate = false;

	if ( !scene || !nif || !selection.isValid() ) {
		clear();
		return;
	}

	Node * node = scene->getNode( nif, selection );

	if ( !node ) {
		clear();
		return;
	}

	Transform tm = ( impl->localCheck->isChecked() ) ? node->localTrans() : node->worldTrans();
	Matrix mat = tm.rotation;

	impl->transform = tm;

	if ( impl->invertCheck->isChecked() )
		mat = mat.inverted();

	Quat q = mat.toQuat();
	float x, y, z;
	mat.toEuler( x, y, z );

	impl->nameText->setText( nif->get<QString>( selection, "Name" ) );
	impl->typeText->setText( nif->getBlockName( selection ) );
	impl->timeText->setText( QString( "%1" ).Farg( impl->time ) );

	impl->posXText->setText( QString( "%1" ).Farg( tm.translation[0] ) );
	impl->posYText->setText( QString( "%1" ).Farg( tm.translation[1] ) );
	impl->posZText->setText( QString( "%1" ).Farg( tm.translation[2] ) );

	impl->rotWText->setText( QString( "%1" ).Farg( q[0] ) );
	impl->rotXText->setText( QString( "%1" ).Farg( q[1] ) );
	impl->rotYText->setText( QString( "%1" ).Farg( q[2] ) );
	impl->rotZText->setText( QString( "%1" ).Farg( q[3] ) );

	impl->eulXText->setText( QString( "%1" ).Farg( x * 180.0f / PI ) );
	impl->eulYText->setText( QString( "%1" ).Farg( y * 180.0f / PI ) );
	impl->eulZText->setText( QString( "%1" ).Farg( z * 180.0f / PI ) );

	impl->matText->setText(
		QString( "[%1, %2, %3]\n[%4, %5, %6]\n[%7, %8, %9]\n" )
		.Farg( mat( 0, 0 ) ).Farg( mat( 0, 1 ) ).Farg( mat( 0, 2 ) )
		.Farg( mat( 1, 0 ) ).Farg( mat( 1, 1 ) ).Farg( mat( 1, 2 ) )
		.Farg( mat( 2, 0 ) ).Farg( mat( 2, 1 ) ).Farg( mat( 2, 2 ) )
	);

	impl->lenText->setText( QString( "%1" ).Farg( node->localTrans().translation.length() ) );
}

void InspectView::clear()
{
	QString empty;
	selection = QModelIndex();

	impl->nameText->setText( empty );
	impl->typeText->setText( empty );
	impl->timeText->setText( empty );

	impl->posXText->setText( empty );
	impl->posYText->setText( empty );
	impl->posZText->setText( empty );

	impl->rotWText->setText( empty );
	impl->rotXText->setText( empty );
	impl->rotYText->setText( empty );
	impl->rotZText->setText( empty );

	impl->eulXText->setText( empty );
	impl->eulYText->setText( empty );
	impl->eulZText->setText( empty );

	impl->matText->setText( empty );

	impl->lenText->setText( empty );
}

void InspectView::setVisible( bool visible )
{
	impl->needUpdate = visible;
	QDialog::setVisible( visible );
}

void InspectView::copyTransformToMimedata()
{
	QByteArray data;
	QBuffer buffer( &data );
	if ( buffer.open( QIODevice::WriteOnly ) ) {
		QDataStream ds( &buffer );
		ds << impl->transform;

		QMimeData * mime = new QMimeData;
		mime->setData( QString( "nifskope/transform" ), data );
		QApplication::clipboard()->setMimeData( mime );
	}
}
