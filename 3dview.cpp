/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005-2010, NIF File Format Library and Tools
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

#include "3dview.h"
// 1.add Qt ...
//#include <QtGui>

NifSkopeQt3D *NifSkopeQt3D::create()
{
	return new NifSkopeQt3D();
}
NifSkopeQt3D::NifSkopeQt3D(QWidget *): QWidget (NULL)
{
	setAttribute(Qt::WA_PaintOnScreen, true);
	setAttribute(Qt::WA_NoSystemBackground, true);
	setAttribute(Qt::WA_NoBackground, true);

	// TODO: their place is not here. It is a 3d renderer window. It has nothing
	// to do with toolbars, foobars or anything like that. But leave them here for
	// now to finish the extraction og GLView
	tAnim = new QToolBar( tr("Animation") );
	tAnim->setObjectName( "AnimTool" );
	tAnim->setAllowedAreas( Qt::TopToolBarArea | Qt::BottomToolBarArea );
	tAnim->setIconSize( QSize( 16, 16 ) );
	animGroups = new QComboBox();
	animGroups->setMinimumWidth( 100 );
	tAnim->addWidget( animGroups );
	sldTime = new FloatSlider( Qt::Horizontal, true, true );
	sldTime->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Maximum );
	tAnim->addWidget( sldTime );
}
void NifSkopeQt3D::paintEvent(QPaintEvent *)// override or hide - no idea
{
}
QPaintEngine *NifSkopeQt3D::paintEngine() const// override
{
	return 0;
}
// fields by "GLView"
QModelIndex NifSkopeQt3D::indexAt(const QPoint &, int /*cycle*/)
{
	return QModelIndex();// TODO: implement me
}
void NifSkopeQt3D::center()
{
	// TODO: implement me
}
void NifSkopeQt3D::save( QSettings & )
{
	// TODO: implement me
}
void NifSkopeQt3D::restore( const QSettings & )
{
	// TODO: implement me
}
Scene * NifSkopeQt3D::getScene()
{
	return NULL;
}
QMenu * NifSkopeQt3D::createMenu() const
{
	return new QMenu((QWidget *)this);// TODO: implement me
}
QList<QToolBar*> NifSkopeQt3D::toolbars() const
{
	return QList<QToolBar*>() << tAnim;// TODO: implement me
}
  // public slots:
void NifSkopeQt3D::setNif( NifModel * )
{
	// TODO: implement me
}
void NifSkopeQt3D::setCurrentIndex( const QModelIndex & )
{
	// TODO: implement me
}
