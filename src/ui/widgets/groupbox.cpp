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

#include "groupbox.h"


GroupBox::GroupBox( Qt::Orientation o ) : QGroupBox()
{
	lay.push( new QBoxLayout( o2d( o ), this ) );
}

GroupBox::GroupBox( const QString & title, Qt::Orientation o ) : QGroupBox( title )
{
	lay.push( new QBoxLayout( o2d( o ), this ) );
}

GroupBox::~GroupBox()
{
}

void GroupBox::addWidget( QWidget * widget, int stretch, Qt::Alignment alignment )
{
	lay.top()->addWidget( widget, stretch, alignment );
}

QWidget * GroupBox::pushLayout( const QString & name, Qt::Orientation o, int stretch, Qt::Alignment alignment )
{
	QGroupBox * grp = new QGroupBox( name );
	lay.top()->addWidget( grp, stretch, alignment );
	QBoxLayout * l = new QBoxLayout( o2d( o ) );
	grp->setLayout( l );
	lay.push( l );
	return grp;
}

void GroupBox::pushLayout( Qt::Orientation o, int stretch )
{
	QBoxLayout * l = new QBoxLayout( o2d( o ) );
	lay.top()->addLayout( l, stretch );
	lay.push( l );
}

void GroupBox::popLayout()
{
	if ( lay.count() > 1 )
		lay.pop();
}

QBoxLayout::Direction GroupBox::o2d( Qt::Orientation o )
{
	switch ( o ) {
	case Qt::Vertical:
		return QBoxLayout::TopToBottom;
	case Qt::Horizontal:
	default:
		return QBoxLayout::LeftToRight;
	}
}

