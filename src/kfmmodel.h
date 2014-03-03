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

#ifndef KFMMODEL_H
#define KFMMODEL_H

#include "basemodel.h"

#include <QHash>
#include <QStringList>
#include <QReadWriteLock>

class KfmModel : public BaseModel
{
Q_OBJECT
public:
	KfmModel( QObject * parent = 0 );
	
	// call this once on startup to load the XML descriptions
	static bool loadXML();

	// when creating kfmmodels from outside the main thread better protect them with a QReadLocker
	static QReadWriteLock XMLlock;
	
	// clear model data
	void clear();
	
	// generic load and save to and from QIODevice
	bool load( QIODevice & device );
	bool save( QIODevice & device ) const;
	
	// is it a compound type?
	static bool isCompound( const QString & name );
	
	QModelIndex getKFMroot() const;
	
	// is this version supported ?
	static bool isVersionSupported( quint32 );
	
	// version conversion
	static QString version2string( quint32 );
	static quint32 version2number( const QString & );
	
	// check wether the current nif file version lies in the range since~until
	bool checkVersion( quint32 since, quint32 until ) const;

	QString getVersion() const { return version2string( version ); }
	quint32 getVersionNumber() const { return version; }
	
	static QAbstractItemDelegate * createDelegate();

protected:
	void		insertType( NifItem * parent, const NifData & data, int row = -1 );
	NifItem *	insertBranch( NifItem * parent, const NifData & data, int row = -1 );

	bool		updateArrayItem( NifItem * array, bool fast );
	
	bool		load( NifItem * parent, NifIStream & stream, bool fast = true );
	bool		save( NifItem * parent, NifOStream & stream ) const;
	
	bool		setItemValue( NifItem * item, const NifValue & v );
	
	bool		setHeaderString( const QString & );
	
	bool		evalVersion( NifItem * item, bool chkParents = false ) const;

	QString		ver2str( quint32 v ) const { return version2string( v ); }
	quint32		str2ver( QString s ) const { return version2number( s ); }
	
	// kfm file version
	quint32		version;
	
	NifItem	* kfmroot;
	
	// XML structures
	static QList<quint32>		supportedVersions;
	
	static QHash<QString,NifBlock*>		compounds;
	
	static QString parseXmlDescription( const QString & filename );

	friend class KfmXmlHandler;
}; // class NifModel


inline bool KfmModel::isCompound( const QString & name )
{
	return compounds.contains( name );
}

inline bool KfmModel::isVersionSupported( quint32 v )
{
	return supportedVersions.contains( v );
}

#endif
