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

#ifndef NIFSKOPE_H
#define NIFSKOPE_H

#include <QMainWindow>
#include <QProgressDialog>


class NifModel;
class NifProxyModel;
class NifTreeView;

class GLView;
class Popup;
 
class QModelIndex;

class QAction;
class QActionGroup;
class QLineEdit;
class QSettings;
class QSlider;
class QSpinBox;
class QTextEdit;

#include "message.h"

class NifSkope : public QMainWindow
{
Q_OBJECT
public:
	NifSkope();
	~NifSkope();
	
	static NifSkope * createWindow();
	
	void save( QSettings & settings ) const;
	void restore( QSettings & settings );
	
public slots:
	void load( const QString & filepath );
	
	void load();
	void save();
	
	void loadBrowse();
	void saveBrowse();
	
	void loadXML();
	void reload();
	
	void sltWindow();
	
	void about();
	
protected slots:
	void clearRoot();
	void select( const QModelIndex & );
	
	void setListMode();
	
	void contextMenu( const QPoint & pos );
	
	void setFrame( int, int, int );
	void setMaxDistance( int );
	
	void dispatchMessage( const Message & msg );
	
private:
	void initActions();
	void initDockWidgets();
	void initToolBars();
	void initMenu();
	
	NifModel * model;
	NifProxyModel * proxy;
	
	NifTreeView * list;
	NifTreeView * tree;
	GLView * ogl;
	
	QLineEdit * lineLoad;
	QLineEdit * lineSave;
	
	QDockWidget * dList;
	QDockWidget * dTree;
	
	QToolBar * tool;
	
	QAction * aLoad;
	QAction * aSave;
	QAction * aLoadXML;
	QAction * aReload;
	QAction * aWindow;
	QAction * aQuit;

	QAction * aCondition;
	
	QActionGroup * gListMode;
	QAction * aList;
	QAction * aHierarchy;
	
	QAction * aNifSkope;
	QAction * aAboutQt;
	
	QAction * aToolSkel;

	QToolBar * tAnim;
	QSlider * sldTime;
	
	QToolBar * tLOD;
	QSlider * sldDistance;
	QSpinBox * spnMaxDistance;
};

class ProgDlg : public QProgressDialog
{
	Q_OBJECT
public:
	ProgDlg() {}

public slots:
	void sltProgress( int, int );
};

#endif
