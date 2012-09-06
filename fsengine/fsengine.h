/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005-2012, NIF File Format Library and Tools
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

#include <QAbstractFileEngine>
#include <QAbstractFileEngineHandler>
#include <QFSFileEngine>
#if (QT_VERSION >= QT_VERSION_CHECK(4, 4, 0))
#include <QAtomicInt>
#else
// define QAtomicInt as QAtomic in older versions of qt
#include <QSharedData>
#define QAtomicInt QAtomic
#endif

// only use the overlay engine before Qt 4.6
#if (QT_VERSION < QT_VERSION_CHECK(4, 6, 0))
#define OVERLAYS_ENABLED
#endif

#ifdef OVERLAYS_ENABLED
//! Registers FSOverlayEngine with the application.
class FSOverlayHandler : public QAbstractFileEngineHandler
{
public:
	//! Registers the file engine handler with the application.
	/*!
	 * \param filename The file to create an overlay engine for
	 * \return FSOverlayEngine
	 */
	QAbstractFileEngine * create( const QString & fileName ) const;
};
#endif // OVERLAYS_ENABLED

//! Provides a way to register an FSArchiveEngine with the application.
class FSArchiveHandler : public QAbstractFileEngineHandler
{
public:
	//! Opens a BSA for the specified file
	static FSArchiveHandler * openArchive( const QString & );
	
public:
	//! Constructor
	FSArchiveHandler( class FSArchiveFile * a );
	//! Destructor
	~FSArchiveHandler();
	
	//! Creates a file engine for the specified filename.
	/*!
	 * \param filename The file to create a file engine for
	 * \return A FSArchiveEngine if the filename is a BSA, 0 otherwise
	 * \sa QAbstractFileEngineHandler
	 */
	QAbstractFileEngine * create( const QString & filename ) const;
	FSArchiveFile * getArchive() { return archive; }

protected:
	class FSArchiveFile * archive;
};

//! Overlay engine which hooks FSArchiveEngine into normal directory traversal.
/*!
 * Not sure why this is needed; the documentation for QFSFileEngine says "by
 * subclassing this class, you can alter its behavior slightly, without
 * having to write a complete QAbstractFileEngine subclass"; but that is what
 * FSArchiveEngine is. Perhaps there is a performance gain by having the overlay?
 */
class FSOverlayEngine : public QFSFileEngine
{
public:
	//! Constructor.
	/*!
	 * \param filename The path to construct a file engine for
	 */
	FSOverlayEngine( const QString & filename );
	
	//! Gets a list of files in the @FSOverlayEngine's directory; see QAbstractFileEngine::entryList().
	/*!
	 * note: as of Qt 4.6 (undocumented as yet), this is not called by QDir,
	 * and an iterator must be provided instead.
	 */
	QStringList entryList( QDir::Filters filters, const QStringList & nameFilters ) const;
	
	//! Gets the applicable flags for the current file; see QAbstractFileEngine::fileFlags().
	FileFlags fileFlags( FileFlags type ) const;

};

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
	
	//! Strips the archive file path from a path possibly inside the file
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
	//! A reference counter for an implicitly shared class
	QAtomicInt ref;
	
	friend class FSArchiveHandler;
	friend class FSArchiveEngine;
};

#endif
