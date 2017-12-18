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


#ifndef FSMANAGER_H
#define FSMANAGER_H


#include <QDialog>
#include <QObject>
#include <QMap>

#include <memory>

class FSArchiveHandler;
class FSArchiveFile;

//! The file system manager class.
class FSManager : public QObject
{
	Q_OBJECT
public:
	//! Gets the global file system manager
	static FSManager * get();
	
	//! Deletes the manager
	static void del();

	//! Gets the list of globally registered BSA files
	static QList<FSArchiveFile *> archiveList();

	//! Filters a list of BSAs from a provided list
	static QStringList filterArchives( const QStringList & list, const QString & folder = "" );

protected:
	//! Constructor
	FSManager( QObject * parent = nullptr );
	//! Destructor
	~FSManager();
	
protected:
	QMap<QString, std::shared_ptr<FSArchiveHandler> > archives;
	bool automatic;
	
	//! Builds a list of global BSAs on Windows platforms
	static QStringList autodetectArchives( const QString & folder = "" );
	//! Helper function to build a list of BSAs
	static QStringList regPathBSAList( QString regKey, QString dataDir );

	void initialize();
	
	friend class NifSkope;
	friend class SettingsResources;
};

#endif

