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


#ifndef ARCHIVEENGINE_H
#define ARCHIVEENGINE_H

#include <QStringList>
#include <QDateTime>

#include <QAtomicInt>

#include <memory>

//! Provides a way to register an FSArchiveEngine with the application.
class FSArchiveHandler
{
public:
	//! Opens a BSA for the specified file
	static std::shared_ptr<FSArchiveHandler> openArchive( const QString & );
	
public:
	//! Constructor
	FSArchiveHandler( class FSArchiveFile * a );
	//! Destructor
	~FSArchiveHandler();
	
	template <typename T> T getArchive() const;
	FSArchiveFile * getArchive() { return archive; }

protected:
	class FSArchiveFile * archive;
};

template <typename T> inline T FSArchiveHandler::getArchive() const
{
	return static_cast<T>(archive);
}


//! A file system archive
class FSArchiveFile
{
public:
	//! Constructor
	FSArchiveFile() : ref( 0 ) {}
	virtual ~FSArchiveFile() {}
	
	virtual bool open() = 0;
	virtual void close() = 0;
	
	virtual QString base() const = 0;
	virtual QString name() const = 0;
	virtual QString path() const = 0;
	
	virtual bool hasFolder( const QString & ) const = 0;
	virtual bool hasFile( const QString & ) const = 0;
	virtual qint64 fileSize( const QString & ) const = 0;
	virtual bool fileContents( const QString &, QByteArray & ) = 0;
	virtual QString getAbsoluteFilePath( const QString & ) const = 0;

	virtual uint ownerId( const QString & ) const = 0;
	virtual QString owner( const QString & ) const = 0;
	virtual QDateTime fileTime( const QString & ) const = 0;

protected:
	//! A reference counter for an implicitly shared class
	QAtomicInt ref;
	
	friend class FSArchiveHandler;
};

#endif
