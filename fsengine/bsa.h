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


#ifndef BSA_H
#define BSA_H


#include "fsengine.h"


#include <QFSFileEngine>
#include <QHash>
#include <QMutex>


class BSA : public FSArchiveFile
{
public:
	BSA( const QString & filePath );
	~BSA();
	
	bool open();
	void close();
	
	QString path() const { return bsaPath; }
	QString base() const { return bsaBase; }
	QString name() const { return bsaName; }
	
	bool stripBasePath( QString & ) const;
	
	bool hasFolder( const QString & ) const;
	QStringList entryList( const QString &, QDir::Filters ) const;
	
	bool hasFile( const QString & ) const;
	qint64 fileSize( const QString & ) const;
	bool fileContents( const QString &, QByteArray & );
	
	uint ownerId( const QString &, QAbstractFileEngine::FileOwner type ) const;
	QString owner( const QString &, QAbstractFileEngine::FileOwner type ) const;
	QDateTime fileTime( const QString &, QAbstractFileEngine::FileTime type ) const;

	static bool canOpen( const QString & );
	
	QString statusText() const { return status; }

protected:
	struct BSAFile
	{
		quint32 sizeFlags;
		quint32 offset;
		
		quint32 size() const;
		bool compressed() const;
	};
	
	struct BSAFolder
	{
		BSAFolder() : parent( 0 ) {}
		~BSAFolder() { qDeleteAll( children ); qDeleteAll( files ); }
		
		BSAFolder * parent;
		QHash<QString, BSAFolder*> children;
		QHash<QString, BSAFile*> files;
	};
	
	BSAFolder * insertFolder( QString name );
	BSAFile * insertFile( BSAFolder * folder, QString name, quint32 sizeFlags, quint32 offset );
	
	const BSAFolder * getFolder( QString fn ) const;
	const BSAFile * getFile( QString fn ) const;
	
	QFSFileEngine bsa;
	QMutex bsaMutex;
	
	QString bsaPath;
	QString bsaBase;
	QString bsaName;
	
	QHash<QString, BSAFolder*> folders;
	BSAFolder root;
	
	QString status;
	
	bool compressToggle;
};

#endif
