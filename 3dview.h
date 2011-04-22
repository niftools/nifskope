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

#ifndef __3DVIEW_H__
#define __3DVIEW_H__

// 1.add Qt ...
#include <QtGui>

// fields by "GLView"
#include "nifmodel.h"
#include "widgets/floatslider.h"
class Scene;

class NifSkopeQt3D: public QWidget
{
public:
	static NifSkopeQt3D * create();
	NifSkopeQt3D( QWidget *parent = NULL );
	void paintEvent( QPaintEvent * );
	QPaintEngine *paintEngine() const;
	// fields by "GLView"
	QModelIndex indexAt( const QPoint & p, int cycle = 0 );
	void center();
	QToolBar * tAnim;
	QComboBox * animGroups;
	FloatSlider * sldTime;
	void save( QSettings & );
	void restore( const QSettings & );
	Scene * getScene();
	QMenu * createMenu() const;
	QList<QToolBar*> toolbars() const;
	  // public slots:
	void setNif( NifModel * );
	void setCurrentIndex( const QModelIndex & );
};

#endif /* __3DVIEW_H__ */