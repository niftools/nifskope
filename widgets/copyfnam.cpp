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

#include "copyfnam.h"

#include <QMouseEvent>
#include <QPainter>

CopyFilename::CopyFilename( QWidget * parent )
	: QWidget( parent )
{
	this->leftArrow.load( ":/img/ArrowLeft" );
	this->rightArrow.load( ":/img/ArrowRight" );

	this->setMinimumWidth( qMax<int>( leftArrow.width(), this->rightArrow.width() ) + 4 );
	this->setMinimumHeight( this->leftArrow.width() + this->rightArrow.width() + 4 );
}

void CopyFilename::paintEvent( QPaintEvent * e )
{
   Q_UNUSED(e);
	int w = this->width();
	int h = this->height() / 2;

	// right arrow position, top
	this->rightPos = this->rightArrow.rect();
	this->rightPos.moveTo(
		( w - this->rightArrow.width() ) / 2,
		( h - this->rightArrow.height() ) / 2
	);

	// left arrow position, bottom
	this->leftPos = this->leftArrow.rect();
	this->leftPos.moveTo(
		( w - this->leftArrow.width() ) / 2,
		( h - this->leftArrow.height() ) / 2 + h
	);

	// now paint the widget
	QPainter p( this );

	// draw arrows
	p.drawImage( this->rightPos, this->rightArrow, this->rightArrow.rect() );
	p.drawImage( this->leftPos, this->leftArrow, this->leftArrow.rect() );
}

void CopyFilename::mousePressEvent( QMouseEvent * e )
{
	if( e->y() < ( this->height() / 2 ) ) {
		emit( this->rightTriggered() );
	}
	else {
		emit( this->leftTriggered() );
	}
}
