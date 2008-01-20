/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005-2008, NIF File Format Library and Tools
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


#ifndef FSMANAGER_H
#define FSMANAGER_H


#include <QDialog>
#include <QObject>
#include <QMap>


class FSOverlayHandler;
class FSArchiveHandler;


class FSManager : public QObject
{
	Q_OBJECT
public:
	FSManager( QObject * parent );
	~FSManager();

public slots:
	void selectArchives();
	
protected:
	FSOverlayHandler * overlay;
	QMap<QString, FSArchiveHandler *> archives;
	bool automatic;
	
	static QStringList autodetectArchives();
	
	friend class FSSelector;
};


class FSSelector : public QDialog
{
	Q_OBJECT
public:
	FSSelector( FSManager * m );
	~FSSelector();
	
protected slots:
	void sltAuto( bool );
	void sltAdd();
	void sltDel();
	
protected:
	FSManager * manager;
	
	class QStringListModel * model;
	class QListView * view;
	
	class QCheckBox * chkAuto;
	class QPushButton * btAdd;
	class QPushButton * btDel;
};


#endif

