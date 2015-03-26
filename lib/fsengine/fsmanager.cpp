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


#include "fsmanager.h"
#include "fsengine.h"
#include "bsa.h"

#include <QCheckBox>
#include <QFileDialog>
#include <QLayout>
#include <QListView>
#include <QPushButton>
#include <QSettings>
#include <QStringListModel>


//! Global BSA file manager
static FSManager *theFSManager = NULL;
// see fsmanager.h
FSManager* FSManager::get() 
{
	if (theFSManager == NULL)
		theFSManager = new FSManager();
	return theFSManager;
}

// see fsmanager.h
QList <FSArchiveFile *> FSManager::archiveList()
{
	QList<FSArchiveFile *> archives;
	for ( FSArchiveHandler* an : get()->archives.values() ) {
		archives.append( an->getArchive() );
	}
	return archives;
}

// see fsmanager.h
FSManager::FSManager( QObject * parent )
	: QObject( parent ), automatic( false )
{
	initialize();
}

// see fsmanager.h
FSManager::~FSManager()
{
	qDeleteAll( archives );
}

void FSManager::initialize()
{
	QSettings cfg;
	QStringList list = cfg.value( "Settings/Resources/Archives", QStringList() ).toStringList();

	for ( const QString an : list ) {
		if ( FSArchiveHandler * a = FSArchiveHandler::openArchive( an ) )
			archives.insert( an, a );
	}
}

// see fsmanager.h
QStringList FSManager::regPathBSAList( QString regKey, QString dataDir )
{
	QStringList list;
	QSettings reg( regKey, QSettings::NativeFormat );
	QString dataPath = reg.value( "Installed Path" ).toString();
	if ( ! dataPath.isEmpty() )
	{
		if ( ! dataPath.endsWith( '/' ) && ! dataPath.endsWith( '\\' ) )
			dataPath += "/";
		dataPath += dataDir;
		QDir fs( dataPath );
		for ( const QString& fn : fs.entryList( { "*.bsa" }, QDir::Files ) )
		{
			list << QDir::fromNativeSeparators(dataPath + QDir::separator() + fn);
		}
	}
	return list;
}

QStringList FSManager::autodetectArchives()
{
	QStringList list;
	
	list << regPathBSAList( "HKEY_LOCAL_MACHINE\\SOFTWARE\\Bethesda Softworks\\Morrowind", "Data Files" );
	list << regPathBSAList( "HKEY_LOCAL_MACHINE\\SOFTWARE\\Bethesda Softworks\\Oblivion", "Data" );
	list << regPathBSAList( "HKEY_LOCAL_MACHINE\\SOFTWARE\\Bethesda Softworks\\Fallout3", "Data" );
	list << regPathBSAList( "HKEY_LOCAL_MACHINE\\SOFTWARE\\Bethesda Softworks\\FalloutNV", "Data" );
	list << regPathBSAList( "HKEY_LOCAL_MACHINE\\SOFTWARE\\Bethesda Softworks\\Skyrim", "Data" );

	return list;
}
