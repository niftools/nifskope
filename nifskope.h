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

#define NIFSKOPE_IPC_PORT 12583

class NifModel;
class NifProxyModel;
class KfmModel;
class NifTreeView;

class GLView;
class FileSelector;
 
class QModelIndex;

class QAction;
class QActionGroup;
class QSettings;
class QSlider;
class QSpinBox;
class QTextEdit;

class QUdpSocket;

#include "message.h"

class NifSkope : public QMainWindow
{
Q_OBJECT
public:
	NifSkope();
	~NifSkope();
	
	static NifSkope * createWindow( const QString & fname = QString() );
	
	void save( QSettings & settings ) const;
	void restore( QSettings & settings );
	
public slots:
	void load( const QString & filepath );
	
	void load();
	void save();
	
	void loadXML();
	void reload();
	
	void sltWindow();
	
	void about();
	
protected slots:
	void clearRoot();
	void select( const QModelIndex & );
	
	void setListMode();
	
	void sltSelectFont();
	
	void contextMenu( const QPoint & pos );
	
	void dispatchMessage( const Message & msg );
	
	void overrideViewFont();
	
protected:
	void closeEvent( QCloseEvent * e );
	bool eventFilter( QObject * o, QEvent * e );
	
private:
	void initActions();
	void initDockWidgets();
	void initToolBars();
	void initMenu();
	
	void setViewFont( const QFont & );
	
	NifModel * nif;
	NifProxyModel * proxy;
	KfmModel * kfm;
	
	NifTreeView * list;
	NifTreeView * tree;
	NifTreeView * kfmtree;
	
	GLView * ogl;
	
	bool selecting;
	
	FileSelector * lineLoad;
	FileSelector * lineSave;
	
	QDockWidget * dList;
	QDockWidget * dTree;
	QDockWidget * dKfm;
	
	QToolBar * tool;
	
	QAction * aSanitize;
	QAction * aLoadXML;
	QAction * aReload;
	QAction * aWindow;
	QAction * aQuit;

	QAction * aCondition;
	
	QActionGroup * gListMode;
	QAction * aList;
	QAction * aHierarchy;
	
	QAction * aSelectFont;
	
	QAction * aNifSkope;
	QAction * aAboutQt;
};

class IPCsocket : public QObject
{
	Q_OBJECT
public:
	static bool nifskope( const QString & );

public slots:
	void openNif( const QUrl & );
	
	void execCommand( QString cmd );

protected slots:
	void processDatagram();
	
protected:
	IPCsocket( QUdpSocket * );
	~IPCsocket();
	
	QUdpSocket * socket;
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
