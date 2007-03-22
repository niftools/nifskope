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

#ifndef NIFMODEL_H
#define NIFMODEL_H

#include "basemodel.h"

#include <QHash>
#include <QStack>
#include <QStringList>
#include <QReadWriteLock>

class NifModel : public BaseModel
{
Q_OBJECT
public:
	NifModel( QObject * parent = 0 );
	
	// call this once on startup to load the XML descriptions
	static bool loadXML();

	// when creating NifModels from outside the main thread protect them with a QReadLocker (see the XML check spell for an example)
	static QReadWriteLock XMLlock;
	
	// clear model data
	void clear();
	
	// generic load and save to and from QIODevice
	bool load( QIODevice & device );
	bool save( QIODevice & device ) const;
	
	bool load( QIODevice & device, const QModelIndex & );
	bool save( QIODevice & device, const QModelIndex & ) const;
	
	bool loadAndMapLinks( QIODevice & device, const QModelIndex &, const QMap<qint32,qint32> & map );
	bool loadHeaderOnly( const QString & fname );
	
	// this returns the the estimated file offset of the model index
	int fileOffset( const QModelIndex & ) const;
	
	// checks if the nif pointed to by filepath contains the  specified block id
	static bool checkForBlock( const QString & filepath, const QString & blockId );
	
	// returns the model index of the NiHeader
	QModelIndex getHeader() const;
	// this updates the header infos ( num blocks etc. )
	void updateHeader();
	
	QModelIndex getFooter() const;
	void updateFooter();

	// insert or append ( row == -1 ) a new NiBlock
	QModelIndex insertNiBlock( const QString & identifier, int row = -1, bool fast = false );
	// remove a block from the list
	void removeNiBlock( int blocknum );
	// move a block in the list
	void moveNiBlock( int src, int dst );
	// return the block name
	QString getBlockName( const QModelIndex & ) const;
	// return the block type
	QString getBlockType( const QModelIndex & ) const;
	// returns the block number
	int getBlockNumber( const QModelIndex & ) const;
	// returns the parent block ( optional: check if it is of type name )
	QModelIndex getBlock( const QModelIndex &, const QString & name = QString() ) const;
	// returns the parent block/header
	QModelIndex getBlockOrHeader( const QModelIndex & ) const;
	// get the NiBlock at index x ( optional: check if it is of type name )
	QModelIndex getBlock( int x, const QString & name = QString() ) const;
	// get the number of NiBlocks
	int getBlockCount() const;
	// returns true if the index is a niblock ( optional: check if it is the specified type of block )
	bool isNiBlock( const QModelIndex & index, const QString & name = QString() ) const;
	// returns a list with all known NiXXX ids
	static QStringList allNiBlocks();
	// is name a NiBlock identifier?
	static bool isNiBlock( const QString & name );
	// reorders the blocks according to a list of new block numbers
	void reorderBlocks( const QVector<qint32> & order );
	// moves all niblocks from this nif to another nif, returns a map which maps old block numbers to new block numbers
	QMap<qint32,qint32> moveAllNiBlocks( NifModel * targetnif );
	
	void insertType( const QModelIndex & parent, const NifData & data, int atRow );
	
	// return the root blocks
	QList<int> getRootLinks() const;
	// return the list of block links
	QList<int> getChildLinks( int block ) const;
	QList<int> getParentLinks( int block ) const;
	// return the parent block number or none (-1) if there is no parent or if there are multiple parents
	int getParent( int block ) const;

	// is it a child or parent link?
	bool isLink( const QModelIndex & index, bool * ischildLink = 0 ) const;
	// this returns a block number if the index is a valid link
	qint32 getLink( const QModelIndex & index ) const;
	int getLink( const QModelIndex & parent, const QString & name ) const;
	QVector<qint32> getLinkArray( const QModelIndex & array ) const;
	QVector<qint32> getLinkArray( const QModelIndex & parent, const QString & name ) const;
	bool setLink( const QModelIndex & index, qint32 l );
	bool setLink( const QModelIndex & parent, const QString & name, qint32 l );
	bool setLinkArray( const QModelIndex & array, const QVector<qint32> & links );
	bool setLinkArray( const QModelIndex & parent, const QString & name, const QVector<qint32> & links );
	
	void mapLinks( const QMap<qint32,qint32> & map );
	
	// is it a compound type?
	static bool isCompound( const QString & name );
	// is name an ancestor identifier?
	static bool isAncestor( const QString & name );
	// returns true if name inherits ancestor
	static bool inherits( const QString & name, const QString & ancestor );
	// returns true if the block containing index inherits ancestor
	bool inherits( const QModelIndex & index, const QString & ancestor ) const;
	
	// is this version supported ?
	static bool isVersionSupported( quint32 );
	
	// version conversion
	static QString version2string( quint32 );
	static quint32 version2number( const QString & );
	
	// check wether the current nif file version lies in the range since~until
	bool checkVersion( quint32 since, quint32 until ) const;

	QString getVersion() const { return version2string( version ); }
	quint32 getVersionNumber() const { return version; }
	
	// QAbstractModel interface
	QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const;
	bool setData( const QModelIndex & index, const QVariant & value, int role = Qt::EditRole );
	void reset();
	
	// removes an item from the model
	bool removeRows( int row, int count, const QModelIndex & parent );
	
	static QAbstractItemDelegate * createDelegate( class SpellBook * );

signals:
	void linksChanged();

protected:
	void		insertAncestor( NifItem * parent, const QString & identifier, int row = -1 );
	void		insertType( NifItem * parent, const NifData & data, int row = -1 );
	NifItem *	insertBranch( NifItem * parent, const NifData & data, int row = -1 );

	bool		updateArrayItem( NifItem * array, bool fast );
	
	NifItem *	getHeaderItem() const;
	NifItem *	getFooterItem() const;
	NifItem *	getBlockItem( int ) const;
	NifItem *	getItem( NifItem * parent, const QString & name ) const;

	bool		load( NifItem * parent, NifIStream & stream, bool fast = true );
	bool		save( NifItem * parent, NifOStream & stream ) const;
	bool		fileOffset( NifItem * parent, NifItem * target, NifSStream & stream, int & ofs ) const;
	
	bool		setItemValue( NifItem * item, const NifValue & v );
	
	bool		itemIsLink( NifItem * item, bool * ischildLink = 0 ) const;
	int			getBlockNumber( NifItem * item ) const;
	
	bool		setHeaderString( const QString & );
	
	QString		ver2str( quint32 v ) const { return version2string( v ); }
	quint32		str2ver( QString s ) const { return version2number( s ); }
	
	bool		evalVersion( NifItem * item, bool chkParents = false ) const;

	// nif file version
	quint32		version;
	
	QHash< int, QList<int> >	childLinks;
	QHash< int, QList<int> >	parentLinks;
	QList< int >				rootLinks;
	
	void updateLinks( int block = -1 );
	void updateLinks( int block, NifItem * parent );
	void checkLinks( int block, QStack<int> & parents );
	void adjustLinks( NifItem * parent, int block, int delta );
	void mapLinks( NifItem * parent, const QMap<qint32,qint32> & map );
	
	// XML structures
	static QList<quint32>		supportedVersions;
	
	static QHash<QString,NifBlock*>		compounds;
	static QHash<QString,NifBlock*>		blocks;
	
	static QString parseXmlDescription( const QString & filename );

	friend class NifXmlHandler;
}; // class NifModel


inline QStringList NifModel::allNiBlocks()
{
	QStringList lst;
	foreach ( NifBlock * blk, blocks )
		if ( ! blk->abstract )
			lst.append( blk->id );
	return lst;
}

inline bool NifModel::isNiBlock( const QString & name )
{
	NifBlock * blk = blocks.value( name );
	return blk && ! blk->abstract;
}

inline bool NifModel::isAncestor( const QString & name )
{
	NifBlock * blk = blocks.value( name );
	return blk && blk->abstract;
}

inline bool NifModel::isCompound( const QString & name )
{
	return compounds.contains( name );
}

inline bool NifModel::isVersionSupported( quint32 v )
{
	return supportedVersions.contains( v );
}

inline QList<int> NifModel::getRootLinks() const
{
	return rootLinks;
}

inline QList<int> NifModel::getChildLinks( int block ) const
{
	return childLinks.value( block );
}

inline QList<int> NifModel::getParentLinks( int block ) const
{
	return parentLinks.value( block );
}

inline bool NifModel::itemIsLink( NifItem * item, bool * isChildLink ) const
{
	if ( isChildLink )
		*isChildLink = ( item->value().type() == NifValue::tLink );
	return item->value().isLink();
}

inline bool NifModel::checkVersion( quint32 since, quint32 until ) const
{
	return ( ( since == 0 || since <= version ) && ( until == 0 || version <= until ) );
}


#endif
