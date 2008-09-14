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


#ifndef ARCHIVEENGINE_H
#define ARCHIVEENGINE_H


#include <QAbstractFileEngine>
#include <QAbstractFileEngineHandler>
#if (QT_VERSION >= QT_VERSION_CHECK(4, 4, 0))
#include <QAtomicInt>
#else
// define QAtomicInt as QAtomic in older versions of qt
#include <QSharedData>
#define QAtomicInt QAtomic
#endif


class FSOverlayHandler : public QAbstractFileEngineHandler
{
public:
	QAbstractFileEngine * create( const QString & fileName ) const;
};


class FSArchiveHandler : public QAbstractFileEngineHandler
{
public:
	static FSArchiveHandler * openArchive( const QString & );
	
public:
	FSArchiveHandler( class FSArchiveFile * a );
	~FSArchiveHandler();
	
	QAbstractFileEngine * create( const QString & filename ) const;
	FSArchiveFile * getArchive() { return archive; }

protected:
	class FSArchiveFile * archive;
};


class FSArchiveFile
{
public:
	FSArchiveFile() : ref( 0 ) {}
	virtual ~FSArchiveFile() {}
	
	virtual bool open() = 0;
	virtual void close() = 0;
	
	virtual QString base() const = 0;
	virtual QString name() const = 0;
	virtual QString path() const = 0;
	
	virtual bool stripBasePath( QString & ) const = 0;
	
	virtual bool hasFolder( const QString & ) const = 0;
	virtual QStringList entryList( const QString &, QDir::Filters ) const = 0;
	
	virtual bool hasFile( const QString & ) const = 0;
	virtual qint64 fileSize( const QString & ) const = 0;
	virtual bool fileContents( const QString &, QByteArray & ) = 0;	
	virtual QString absoluteFilePath( const QString & ) const = 0;

	virtual uint ownerId( const QString &, QAbstractFileEngine::FileOwner type ) const = 0;
	virtual QString owner( const QString &, QAbstractFileEngine::FileOwner type ) const = 0;
	virtual QDateTime fileTime( const QString &, QAbstractFileEngine::FileTime type ) const = 0;

protected:
	QAtomicInt ref;
	
	friend class FSArchiveHandler;
	friend class FSArchiveEngine;
};



#endif
