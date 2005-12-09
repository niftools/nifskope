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

#include <QLineEdit>
#include <QMenu>
#include <QWidget>


class NifModel;
class NifProxyModel;
class NifTreeView;

class GLView;
class Popup;
 
class QModelIndex;

class QAbstractButton;
class QButtonGroup;
class QCheckBox;
class QGroupBox;
class QSplitter;
class QTextEdit;

class NifSkope : public QWidget
{
Q_OBJECT
public:
	NifSkope();
	~NifSkope();
	
public slots:
	void load( const QString & filepath );
	
	void load();
	void save();
	
	void loadBrowse();
	void saveBrowse();
	
	void saveOptions();
	
	void about();
	
	void addMessage( const QString & );
	
protected slots:
	void clearRoot();
	void select( const QModelIndex & );
	
	void setListMode( QAbstractButton * );
	
	void contextMenu( const QPoint & pos );
	
	void toggleMessages();
	void delayedToggleMessages();
	
private:
	NifModel * model;
	NifProxyModel * proxy;
	
	NifTreeView * list;
	NifTreeView * tree;
	GLView * ogl;
	
	Popup * popOpts;

	QLineEdit * lineLoad;
	QLineEdit * lineSave;
	
	QButtonGroup * listMode;
	
	QCheckBox * conditionZero;
	QCheckBox * autoSettings;
	
	QGroupBox * msgroup;
	QTextEdit * msgview;
	
	QSplitter * split;
};


#endif
