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

#include "floatedit.h"

#include "data/nifvalue.h"

#include <QAction>
#include <QContextMenuEvent>
#include <QMenu>

#include <cfloat>


QValidator::State FloatValidator::validate ( QString & input, int & pos ) const
{
	if ( !( bottom() > -FLT_MAX ) && tr( "<float_min>" ).startsWith( input ) ) {
		if ( input == tr( "<float_min>" ) )
			return QValidator::Acceptable;

		return QValidator::Intermediate;
	}

	if ( !( top() < FLT_MAX ) && tr( "<float_max>" ).startsWith( input ) ) {
		if ( input == tr( "<float_max>" ) )
			return QValidator::Acceptable;

		return QValidator::Intermediate;
	}

	return QDoubleValidator::validate( input, pos );
}

bool FloatValidator::canMin() const
{
	return !( bottom() > -FLT_MAX );
}

bool FloatValidator::canMax() const
{
	return !( top() < FLT_MAX );
}

FloatEdit::FloatEdit( QWidget * parent )
	: QLineEdit( parent )
{
	val = 0.0f;

	setValidator( validator = new FloatValidator( this ) );
	validator->setDecimals( 6 );

	connect( this, &FloatEdit::editingFinished, this, &FloatEdit::edited );

	/*
	    context menu
	*/
	actMin = new QAction( tr( "<float_min>" ), this );
	connect( actMin, &QAction::triggered, this, &FloatEdit::setMin );
	addAction( actMin );

	actMax = new QAction( tr( "<float_max>" ), this );
	connect( actMax, &QAction::triggered, this, &FloatEdit::setMax );
	addAction( actMax );
}

void FloatEdit::contextMenuEvent( QContextMenuEvent * event )
{
	QMenu * mnuContext = new QMenu( this );
	mnuContext->setMouseTracking( true );

	QMenu * mnuStd = createStandardContextMenu();

	if ( validator->canMin() )
		mnuContext->addAction( actMin );

	if ( validator->canMax() )
		mnuContext->addAction( actMax );

	if ( validator->canMin() || validator->canMax() )
		mnuContext->addSeparator();

	mnuContext->addActions( mnuStd->actions() );

	mnuContext->exec( event->globalPos() );

	delete mnuStd;
	delete mnuContext;
}

void FloatEdit::edited()
{
	QString str = text();

	if ( str == tr( "<float_min>" ) )
		val = -FLT_MAX;
	else if ( str == tr( "<float_max>" ) )
		val = FLT_MAX;
	else
		val = str.toFloat();

	emit sigEdited( val );
	emit sigEdited( str );
}

void FloatEdit::setValue( float f )
{
	val = f;
	setText( NumOrMinMax( val, 'f', validator->decimals() ) );
	emit valueChanged( val );
	emit valueChanged( text() );
}

void FloatEdit::set( float f, float, float )
{
	setValue( f );
}

void FloatEdit::setMin()
{
	if ( validator->canMin() )
		setValue( -FLT_MAX );
}

void FloatEdit::setMax()
{
	if ( validator->canMax() )
		setValue( FLT_MAX );
}

float FloatEdit::value() const
{
	return val;
}

void FloatEdit::setRange( float minimum, float maximum )
{
	validator->setRange( minimum, maximum );
}

bool FloatEdit::isMin() const
{
	return ( text() == tr( "<float_min>" ) );
}

bool FloatEdit::isMax() const
{
	return ( text() == tr( "<float_max>" ) );
}
