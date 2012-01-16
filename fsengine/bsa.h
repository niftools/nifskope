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

#ifndef BSA_H
#define BSA_H

#include "fsengine.h"

#include <QAbstractFileEngineIterator>
#include <QFSFileEngine>
#include <QHash>
#include <QMutex>

//! \file bsa.h BSA file, BSAIterator

//! A Bethesda Software Archive file
/*!
 * See <a href="http://www.uesp.net/wiki/Main_Page">The UESP</a> for descriptions of the
 * <a href="http://www.uesp.net/wiki/Tes3Mod:BSA_File_Format">Morrowind format</a> and the
 * <a href="http://www.uesp.net/wiki/Tes4Mod:BSA_File_Format">Oblivion format</a>.
 *
 * \sa MWBSAHeader, MWBSAFileSizeOffset, OBBSAHeader, OBBSAFileInfo, OBBSAFolderInfo
 */
class BSA : public FSArchiveFile
{
public:
	//! Constructor; creates a %BSA from the given file path.
	BSA( const QString & filePath );
	//! Destructor; closes the file.
	~BSA();
	
	//! Opens the %BSA file
	bool open();
	//! Closes the %BSA file
	void close();
	
	//! Returns BSA::bsaPath.
	QString path() const { return bsaPath; }
	//! Returns BSA::bsaBase.
	QString base() const { return bsaBase; }
	//! Returns BSA::bsaName.
	QString name() const { return bsaName; }
	
	//! \copybrief FSArchiveFile::stripBasePath()
	/*!
	 * In this case, strips bsaPath from the provided string.
	 * 
	 * @param p the string to strip the base path from
	 * @return true if the path was stripped, false otherwise
	 */
	bool stripBasePath( QString & p ) const;
	
	//! Whether the specified folder exists or not
	bool hasFolder( const QString & ) const;
	//! Returns a list of files or folders as requested
	/*!
	 * Called by FSArchiveEngine::entryList().
	 * \param fn The folder to search in
	 * \param filters Whether to look for files or folders
	 */
	QStringList entryList( const QString & fn, QDir::Filters filters ) const;
	
	//! Whether the specified file exists or not
	bool hasFile( const QString & ) const;
	//! Returns the size of the file per BSAFile::size().
	qint64 fileSize( const QString & ) const;
	//! Returns the contents of the specified file
	/*!
	* \param fn The filename to get the contents for
	* \param content Reference to the byte array that holds the file contents
	* \return True if successful
	*/
	bool fileContents( const QString &, QByteArray & );
	
	//! See QFSFileEngine::ownerId().
	uint ownerId( const QString &, QAbstractFileEngine::FileOwner type ) const;
	//! See QFSFileEngine::owner().
	QString owner( const QString &, QAbstractFileEngine::FileOwner type ) const;
	//! See QFSFileEngine::fileTime().
	QDateTime fileTime( const QString &, QAbstractFileEngine::FileTime type ) const;
	//! See QFileInfo::absoluteFilePath().
	QString absoluteFilePath( const QString & ) const;
	
	//! Whether the given file can be opened as a %BSA or not
	static bool canOpen( const QString & );
	
	//! Returns BSA::status.
	QString statusText() const { return status; }

protected:
	//! A file inside a BSA
	struct BSAFile
	{
		quint32 sizeFlags; //!< The size of the file in the BSA
		quint32 offset; //!< The offset of the file in the BSA
		
		//! The size of the file inside the BSA
		quint32 size() const;
		//! Whether the file is compressed inside the BSA
		bool compressed() const;
	};
	
	//! A folder inside a BSA
	struct BSAFolder
	{
		//! Constructor
		BSAFolder() : parent( 0 ) {}
		//! Destructor
		~BSAFolder() { qDeleteAll( children ); qDeleteAll( files ); }
		
		BSAFolder * parent; //!< The parent item
		QHash<QString, BSAFolder*> children; //!< A map of child folders
		QHash<QString, BSAFile*> files; //!< A map of files inside the folder
	};
	
	//! Recursive function to generate the tree structure of folders inside a %BSA
	BSAFolder * insertFolder( QString name );
	//! Inserts a file into the structure of a %BSA
	BSAFile * insertFile( BSAFolder * folder, QString name, quint32 sizeFlags, quint32 offset );
	
	//! Gets the specified folder, or the root folder if not found
	const BSAFolder * getFolder( QString fn ) const;
	//! Gets the specified file, or null if not found
	const BSAFile * getFile( QString fn ) const;
	
	//! The engine which reads the %BSA from a file
	QFSFileEngine bsa;
	//! Mutual exclusion handler
	QMutex bsaMutex;
	
	//! The absolute name of the file, e.g. "d:/temp/test.bsa"
	QString bsaPath;
	//! The base path of the file, e.g. "d:/temp"
	QString bsaBase;
	//! The name of the file, e.g. "test.bsa"
	QString bsaName;
	
	//! Map of folders inside a %BSA
	QHash<QString, BSAFolder*> folders;
	//! The root folder
	BSAFolder root;
	
	//! Error string for exception handling
	QString status;
	
	//! Whether the %BSA is compressed
	bool compressToggle;
	//! Whether Fallout 3 names are prefixed with an extra string
	bool namePrefix;
	
	//! An iterator that has access to the %BSA
	friend class BSAIterator;
};

//! Iterator for a BSA file.
class BSAIterator : public QAbstractFileEngineIterator
{
public:
	//! Constructor
	BSAIterator( QDir::Filters filters, const QStringList & nameFilters, BSA * archive, QString & base );

	//! Destructor
	~BSAIterator();
	
	//! Whether there are any entries left in the list
	/*!
	 * \return true if next() can be called
	 */
	bool hasNext() const;
	
	//! Name of the entry currently in the list
	QString currentFileName() const;
	
	//! Gets the next entry in the list
	/*!
	 * \return the name of the next entry in the list
	 */
	QString next();

	//! Return the current file path
	QString currentFilePath() const;

	//! Return the path for the iterator
	QString path() const;

protected:
	//! The type of the current entry
	enum CurrentEntry
	{
		File, //!< Current entry is a file
		Folder, //!< Current entry is a folder
		None //!< No current entry has been set
	} current;

private:
	//! The BSA that is being iterated over
	BSA * archive;
	//! The current folder inside the BSA
	BSA::BSAFolder * currentFolder;
	//! The list of files known by the iterator
	QStringList fileList;
	//! The list of folders known by the iterator
	QStringList folderList;
	//! The index of the iterator in the list of files
	qint32 fileIndex;
	//! The index of the iterator in the list of folders
	qint32 folderIndex;
};

#endif
