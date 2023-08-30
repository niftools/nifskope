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

#ifndef NIFMODEL_H
#define NIFMODEL_H

#include "basemodel.h" // Inherited

#include <QHash>
#include <QReadWriteLock>
#include <QStack>
#include <QStringList>

#include <memory>

class SpellBook;
class QUndoStack;

using NifBlockPtr = std::shared_ptr<NifBlock>;
using SpellBookPtr = std::shared_ptr<SpellBook>;

//! @file nifmodel.h NifModel, NifModelEval


//! Primary string for read failure
const char * const readFail = QT_TR_NOOP( "The NIF file could not be read. See Details for more information." );

//! Secondary string for read failure
const char * const readFailFinal = QT_TR_NOOP( "Failed to load %1" );


//! The main data model for the NIF file.
class NifModel final : public BaseModel
{
	Q_OBJECT

	friend class NifXmlHandler;
	friend class NifModelEval;
	friend class NifOStream;
	friend class ArrayUpdateCommand;

public:
	NifModel( QObject * parent = 0 );

	static const NifModel * fromIndex( const QModelIndex & index );
	static const NifModel * fromValidIndex( const QModelIndex & index );

	//! Find and parse the XML file
	static bool loadXML();

	//! When creating NifModels from outside the main thread protect them with a QReadLocker
	static QReadWriteLock XMLlock;

	// QAbstractItemModel

	QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const override final;
	bool setData( const QModelIndex & index, const QVariant & value, int role = Qt::EditRole ) override final;
	bool removeRows( int row, int count, const QModelIndex & parent ) override final;

	QModelIndex buddy( const QModelIndex & index ) const override;

	// end QAbstractItemModel

	// BaseModel
	
	void clear() override final;
	bool load( QIODevice & device ) override final;
	bool save( QIODevice & device ) const override final;

	QString getVersion() const override final { return version2string( version ); }
	quint32 getVersionNumber() const override final { return version; }

	// end BaseModel

	//! Load from QIODevice and index
	bool loadIndex( QIODevice & device, const QModelIndex & );
	//! Save to QIODevice and index
	bool saveIndex( QIODevice & device, const QModelIndex & ) const;
	//! Resets the model to its original state in any attached views.
	void reset();

	//! Invalidate only the conditions of the items dependent on this item
	void invalidateDependentConditions( NifItem * item );
	//! Reset all cached conditions of the header
	void invalidateHeaderConditions();

	//! Loads a model and maps links
	bool loadAndMapLinks( QIODevice & device, const QModelIndex &, const QMap<qint32, qint32> & map );
	//! Loads the header from a filename
	bool loadHeaderOnly( const QString & fname );

	//! Returns the the estimated file offset of the model index
	int fileOffset( const QModelIndex & ) const;

	//! Returns the estimated file size of the item
	int blockSize( const NifItem * item ) const;
	//! Returns the estimated file size of the stream
	int blockSize( const NifItem * item, NifSStream & stream ) const;

	/*! Checks if the specified file contains the specified block ID in its header and is of the specified version
	 *
	 * Note that it will not open the full file to look for block types, only the header
	 *
	 * @param filepath	The NIF to check
	 * @param blockId	The block to check for
	 * @param version	The version to check for
	 */
	bool earlyRejection( const QString & filepath, const QString & blockId, quint32 version );

	const NifItem * getHeaderItem() const;
	NifItem * getHeaderItem();
	//! Returns the model index of the NiHeader
	// TODO(Gavrant): try replace it with getHeaderItem
	QModelIndex getHeaderIndex() const;

	//! Updates the header infos ( num blocks etc. )
	void updateHeader();
	//! Extracts the 0x01 separated args from NiDataStream. NiDataStream is the only known block to use RTTI args.
	QString extractRTTIArgs( const QString & RTTIName, NiMesh::DataStreamMetadata & metadata ) const;
	//! Creates the 0x01 separated args for NiDataStream. NiDataStream is the only known block to use RTTI args.
	QString createRTTIName( const QModelIndex & iBlock ) const;
	//! Creates the 0x01 separated args for NiDataStream. NiDataStream is the only known block to use RTTI args.
	QString createRTTIName( const NifItem * block ) const;

	const NifItem * getFooterItem() const;
	NifItem * getFooterItem();

	//! Updates the footer info (num root links etc. )
	void updateFooter();

	//! Set delayed updating of model links
	bool holdUpdates( bool value );

	QList<int> getRootLinks() const;
	QList<int> getChildLinks( int block ) const;
	QList<int> getParentLinks( int block ) const;

	/*! Get parent
	 * @return	Parent block number or -1 if there are zero or multiple parents.
	 */
	int getParent( int block ) const;

	/*! Get parent
	 * @return	Parent block number or -1 if there are zero or multiple parents.
	 */
	int getParent( const QModelIndex & index ) const;

	//! Is an item's value a child or parent link?
	bool isLink( const NifItem * item ) const;
	//! Is a model index' value a child or parent link?
	bool isLink( const QModelIndex & index ) const;

	void mapLinks( const QMap<qint32, qint32> & map );

	//! Is name a compound type?
	static bool isCompound( const QString & name );
	//! Is compound of fixed size/condition? (Array optimization)
	static bool isFixedCompound( const QString & name );
	//! Is name an ancestor identifier (<niobject abstract="1">)?
	static bool isAncestor( const QString & name );
	//! Is name a NiBlock identifier (<niobject abstract="0"> or <niobject abstract="1">)?
	bool isAncestorOrNiBlock( const QString & name ) const override final;

	//! Is this version supported?
	static bool isVersionSupported( quint32 );

	static QString version2string( quint32 );
	static quint32 version2number( const QString & );

	//! Check whether the current NIF file version lies in the range [since, until]
	bool checkVersion( quint32 since, quint32 until = 0 ) const;

	quint32 getUserVersion() const { return get<int>( getHeaderItem(), "User Version" ); }
	quint32 getBSVersion() const { return bsVersion; }

	//! Create and return delegate for SpellBook
	static QAbstractItemDelegate * createDelegate( QObject * parent, SpellBookPtr book );

	//! Undo Stack for changes to NifModel
	QUndoStack * undoStack = nullptr;

	// Basic block functions
protected:
	constexpr int firstBlockRow() const;
	int lastBlockRow() const;

public:
	//! Get the number of NiBlocks
	int getBlockCount() const;

	//! Get the numerical index (or link) of the block an item belongs to.
	// Return -1 if the item is the root or header or footer or null.
	int getBlockNumber( const NifItem * item ) const;
	//! Get the numerical index (or link) of the block an item belongs to.
	// Return -1 if the item is the root or header or footer or null.
	int getBlockNumber( const QModelIndex & index ) const;

	//! Insert or append ( row == -1 ) a new NiBlock
	QModelIndex insertNiBlock( const QString & identifier, int row = -1 );
	//! Remove a block from the list
	void removeNiBlock( int blocknum );
	//! Move a block in the list
	void moveNiBlock( int src, int dst );

	//! Returns a list with all known NiXXX ids (<niobject abstract="0">)
	static QStringList allNiBlocks();
	//! Reorders the blocks according to a list of new block numbers
	void reorderBlocks( const QVector<qint32> & order );
	//! Moves all niblocks from this nif to another nif, returns a map which maps old block numbers to new block numbers
	QMap<qint32, qint32> moveAllNiBlocks( NifModel * targetnif, bool update = true );
	//! Convert a block from one type to another
	void convertNiBlock( const QString & identifier, const QModelIndex & index );

	// Block item getters
private:
	const NifItem * _getBlockItem( const NifItem * block, const QString & ancestor ) const;
	const NifItem * _getBlockItem( const NifItem * block, const QLatin1String & ancestor ) const;
	const NifItem * _getBlockItem( const NifItem * block, const std::initializer_list<const char *> & ancestors ) const;
	const NifItem * _getBlockItem( const NifItem * block, const QStringList & ancestors ) const;

public:
	//! Get a block NifItem by its number.
	const NifItem * getBlockItem( qint32 link ) const;
	//! Get a block NifItem by its number, with a check that it inherits ancestor.
	const NifItem * getBlockItem( qint32 link, const QString & ancestor ) const;
	//! Get a block NifItem by its number, with a check that it inherits ancestor.
	const NifItem * getBlockItem( qint32 link, const QLatin1String & ancestor ) const;
	//! Get a block NifItem by its number, with a check that it inherits ancestor.
	const NifItem * getBlockItem( qint32 link, const char * ancestor ) const;
	//! Get a block NifItem by its number, with a check that it inherits any of ancestors.
	const NifItem * getBlockItem( qint32 link, const std::initializer_list<const char *> & ancestors ) const;
	//! Get a block NifItem by its number, with a check that it inherits any of ancestors.
	const NifItem * getBlockItem( qint32 link, const QStringList & ancestors ) const;

	//! Get the block NifItem an item belongs to.
	const NifItem * getBlockItem( const NifItem * item ) const;
	//! Get the block NifItem an item belongs to, with a check that it inherits ancestor.
	const NifItem * getBlockItem( const NifItem * item, const QString & ancestor ) const;
	//! Get the block NifItem an item belongs to, with a check that it inherits ancestor.
	const NifItem * getBlockItem( const NifItem * item, const QLatin1String & ancestor ) const;
	//! Get the block NifItem an item belongs to, with a check that it inherits ancestor.
	const NifItem * getBlockItem( const NifItem * item, const char * ancestor ) const;
	//! Get the block NifItem an item belongs to, with a check that it inherits any of ancestors.
	const NifItem * getBlockItem( const NifItem * item, const std::initializer_list<const char *> & ancestors ) const;
	//! Get the block NifItem an item belongs to, with a check that it inherits any of ancestors.
	const NifItem * getBlockItem( const NifItem * item, const QStringList & ancestors ) const;

	//! Get the block NifItem a model index belongs to.
	const NifItem * getBlockItem( const QModelIndex & index ) const;
	//! Get the block NifItem a model index belongs to, with a check that it inherits ancestor.
	const NifItem * getBlockItem( const QModelIndex & index, const QString & ancestor ) const;
	//! Get the block NifItem a model index belongs to, with a check that it inherits ancestor.
	const NifItem * getBlockItem( const QModelIndex & index, const QLatin1String & ancestor ) const;
	//! Get the block NifItem a model index belongs to, with a check that it inherits ancestor.
	const NifItem * getBlockItem( const QModelIndex & index, const char * ancestor ) const;
	//! Get the block NifItem a model index belongs to, with a check that it inherits any of ancestors.
	const NifItem * getBlockItem( const QModelIndex & index, const std::initializer_list<const char *> & ancestors ) const;
	//! Get the block NifItem a model index belongs to, with a check that it inherits any of ancestors.
	const NifItem * getBlockItem( const QModelIndex & index, const QStringList & ancestors ) const;

	//! Get a block NifItem by its number.
	NifItem * getBlockItem( qint32 link );
	//! Get a block NifItem by its number, with a check that it inherits ancestor.
	NifItem * getBlockItem( qint32 link, const QString & ancestor );
	//! Get a block NifItem by its number, with a check that it inherits ancestor.
	NifItem * getBlockItem( qint32 link, const QLatin1String & ancestor );
	//! Get a block NifItem by its number, with a check that it inherits ancestor.
	NifItem * getBlockItem( qint32 link, const char * ancestor );
	//! Get a block NifItem by its number, with a check that it inherits any of ancestors.
	NifItem * getBlockItem( qint32 link, const std::initializer_list<const char *> & ancestors );
	//! Get a block NifItem by its number, with a check that it inherits any of ancestors.
	NifItem * getBlockItem( qint32 link, const QStringList & ancestors );

	//! Get the block NifItem an item belongs to.
	NifItem * getBlockItem( const NifItem * item );
	//! Get the block NifItem an item belongs to, with a check that it inherits ancestor.
	NifItem * getBlockItem( const NifItem * item, const QString & ancestor );
	//! Get the block NifItem an item belongs to, with a check that it inherits ancestor.
	NifItem * getBlockItem( const NifItem * item, const QLatin1String & ancestor );
	//! Get the block NifItem an item belongs to, with a check that it inherits ancestor.
	NifItem * getBlockItem( const NifItem * item, const char * ancestor );
	//! Get the block NifItem an item belongs to, with a check that it inherits any of ancestors.
	NifItem * getBlockItem( const NifItem * item, const std::initializer_list<const char *> & ancestors );
	//! Get the block NifItem an item belongs to, with a check that it inherits any of ancestors.
	NifItem * getBlockItem( const NifItem * item, const QStringList & ancestors );

	//! Get the block NifItem a model index belongs to.
	NifItem * getBlockItem( const QModelIndex & index );
	//! Get the block NifItem a model index belongs to, with a check that it inherits ancestor.
	NifItem * getBlockItem( const QModelIndex & index, const QString & ancestor );
	//! Get the block NifItem a model index belongs to, with a check that it inherits ancestor.
	NifItem * getBlockItem( const QModelIndex & index, const QLatin1String & ancestor );
	//! Get the block NifItem a model index belongs to, with a check that it inherits ancestor.
	NifItem * getBlockItem( const QModelIndex & index, const char * ancestor );
	//! Get the block NifItem a model index belongs to, with a check that it inherits any of ancestors.
	NifItem * getBlockItem( const QModelIndex & index, const std::initializer_list<const char *> & ancestors );
	//! Get the block NifItem a model index belongs to, with a check that it inherits any of ancestors.
	NifItem * getBlockItem( const QModelIndex & index, const QStringList & ancestors );

	// Block index getters
public:
	//! Get a block model index by its number.
	QModelIndex getBlockIndex( qint32 link ) const;
	//! Get a block model index by its number, with a check that it inherits ancestor.
	QModelIndex getBlockIndex( qint32 link, const QString & ancestor ) const;
	//! Get a block model index by its number, with a check that it inherits ancestor.
	QModelIndex getBlockIndex( qint32 link, const QLatin1String & ancestor ) const;
	//! Get a block model index by its number, with a check that it inherits ancestor.
	QModelIndex getBlockIndex( qint32 link, const char * ancestor ) const;
	//! Get a block model index by its number, with a check that it inherits any of ancestors.
	QModelIndex getBlockIndex( qint32 link, const std::initializer_list<const char *> & ancestors ) const;
	//! Get a block model index by its number, with a check that it inherits any of ancestors.
	QModelIndex getBlockIndex( qint32 link, const QStringList & ancestors ) const;

	//! Get the block model index an item belongs to.
	QModelIndex getBlockIndex( const NifItem * item ) const;
	//! Get the block model index an item belongs to, with a check that it inherits ancestor.
	QModelIndex getBlockIndex( const NifItem * item, const QString & ancestor ) const;
	//! Get the block model index an item belongs to, with a check that it inherits ancestor.
	QModelIndex getBlockIndex( const NifItem * item, const QLatin1String & ancestor ) const;
	//! Get the block model index an item belongs to, with a check that it inherits ancestor.
	QModelIndex getBlockIndex( const NifItem * item, const char * ancestor ) const;
	//! Get the block model index an item belongs to, with a check that it inherits any of ancestors.
	QModelIndex getBlockIndex( const NifItem * item, const std::initializer_list<const char *> & ancestors ) const;
	//! Get the block model index an item belongs to, with a check that it inherits any of ancestors.
	QModelIndex getBlockIndex( const NifItem * item, const QStringList & ancestors ) const;

	//! Get the block model index a model index belongs to.
	QModelIndex getBlockIndex( const QModelIndex & index ) const;
	//! Get the block model index a model index belongs to, with a check that it inherits ancestor.
	QModelIndex getBlockIndex( const QModelIndex & index, const QString & ancestor ) const;
	//! Get the block model index a model index belongs to, with a check that it inherits ancestor.
	QModelIndex getBlockIndex( const QModelIndex & index, const QLatin1String & ancestor ) const;
	//! Get the block model index a model index belongs to, with a check that it inherits ancestor.
	QModelIndex getBlockIndex( const QModelIndex & index, const char * ancestor ) const;
	//! Get the block model index a model index belongs to, with a check that it inherits any of ancestors.
	QModelIndex getBlockIndex( const QModelIndex & index, const std::initializer_list<const char *> & ancestors ) const;
	//! Get the block model index a model index belongs to, with a check that it inherits any of ancestors.
	QModelIndex getBlockIndex( const QModelIndex & index, const QStringList & ancestors ) const;

	// isNiBlock
public:
	//! Determine if a value is a NiBlock identifier (<niobject abstract="0">).
	static bool isNiBlock( const QString & name );

	//! Check if a given item is a NiBlock.
	bool isNiBlock( const NifItem * item ) const;
	//! Check if a given item is a NiBlock of testType.
	bool isNiBlock( const NifItem * item, const QString & testType ) const;
	//! Check if a given item is a NiBlock of testType.
	bool isNiBlock( const NifItem * item, const QLatin1String & testType ) const;
	//! Check if a given item is a NiBlock of testType.
	bool isNiBlock( const NifItem * item, const char * testType ) const;
	//! Check if a given item is a NiBlock of one of testTypes.
	bool isNiBlock( const NifItem * item, const std::initializer_list<const char *> & testTypes ) const;
	//! Check if a given item is a NiBlock of one of testTypes.
	bool isNiBlock( const NifItem * item, const QStringList & testTypes ) const;
	//! Check if a given model index is a NiBlock.
	bool isNiBlock( const QModelIndex & index ) const;
	//! Check if a given model index is a NiBlock of testType.
	bool isNiBlock( const QModelIndex & index, const QString & testType ) const;
	//! Check if a given model index is a NiBlock of testType.
	bool isNiBlock( const QModelIndex & index, const QLatin1String & testType ) const;
	//! Check if a given model index is a NiBlock of testType.
	bool isNiBlock( const QModelIndex & index, const char * testType ) const;
	//! Check if a given model index is a NiBlock of one of testTypes.
	bool isNiBlock( const QModelIndex & index, const std::initializer_list<const char *> & testTypes ) const;
	//! Check if a given model index is a NiBlock of one of testTypes.
	bool isNiBlock( const QModelIndex & index, const QStringList & testTypes ) const;

	// Block inheritance
public:
	//! Returns true if blockName inherits ancestor.
	bool inherits( const QString & blockName, const QString & ancestor ) const override final;
	//! Returns true if blockName inherits ancestor.
	bool inherits( const QString & blockName, const QLatin1String & ancestor ) const;
	//! Returns true if blockName inherits ancestor.
	bool inherits( const QString & blockName, const char * ancestor ) const;
	//! Returns true if blockName inherits any of ancestors.
	bool inherits( const QString & blockName, const std::initializer_list<const char *> & ancestors ) const;
	//! Returns true if blockName inherits any of ancestors.
	bool inherits( const QString & blockName, const QStringList & ancestors ) const;

	//! Returns true if the block containing an item inherits ancestor.
	bool blockInherits( const NifItem * item, const QString & ancestor ) const;
	//! Returns true if the block containing an item inherits ancestor.
	bool blockInherits( const NifItem * item, const QLatin1String & ancestor ) const;
	//! Returns true if the block containing an item inherits ancestor.
	bool blockInherits( const NifItem * item, const char * ancestor ) const;
	//! Returns true if the block containing an item inherits any of ancestors.
	bool blockInherits( const NifItem * item, const std::initializer_list<const char *> & ancestors ) const;
	//! Returns true if the block containing an item inherits any of ancestors.
	bool blockInherits( const NifItem * item, const QStringList & ancestors ) const;

	//! Returns true if the block containing a model index inherits ancestor.
	bool blockInherits( const QModelIndex & index, const QString & ancestor ) const;
	//! Returns true if the block containing a model index inherits ancestor.
	bool blockInherits( const QModelIndex & index, const QLatin1String & ancestor ) const;
	//! Returns true if the block containing a model index inherits ancestor.
	bool blockInherits( const QModelIndex & index, const char * ancestor ) const;
	//! Returns true if the block containing a model index inherits any of ancestors.
	bool blockInherits( const QModelIndex & index, const std::initializer_list<const char *> & ancestors ) const;
	//! Returns true if the block containing a model index inherits any of ancestors.
	bool blockInherits( const QModelIndex & index, const QStringList & ancestors ) const;

	// Item value getters
public:
	//! Get the value of an item.
	template <typename T> T get( const NifItem * item ) const;
	//! Get the value of a child item.
	template <typename T> T get( const NifItem * itemParent, int itemIndex ) const;
	//! Get the value of a child item.
	template <typename T> T get( const NifItem * itemParent, const QString & itemName ) const;
	//! Get the value of a child item.
	template <typename T> T get( const NifItem * itemParent, const QLatin1String & itemName ) const;
	//! Get the value of a child item.
	template <typename T> T get( const NifItem * itemParent, const char * itemName ) const;
	//! Get the value of a model index.
	template <typename T> T get( const QModelIndex & index ) const;
	//! Get the value of a child item.
	template <typename T> T get( const QModelIndex & itemParent, int itemIndex ) const;
	//! Get the value of a child item.
	template <typename T> T get( const QModelIndex & itemParent, const QString & itemName ) const;
	//! Get the value of a child item.
	template <typename T> T get( const QModelIndex & itemParent, const QLatin1String & itemName ) const;
	//! Get the value of a child item.
	template <typename T> T get( const QModelIndex & itemParent, const char * itemName ) const;

	// Item value setters
public:
	//! Set the value of an item.
	template <typename T> bool set( NifItem * item, const T & val );
	//! Set the value of a child item.
	template <typename T> bool set( const NifItem * itemParent, int itemIndex, const T & val );
	//! Set the value of a child item.
	template <typename T> bool set( const NifItem * itemParent, const QString & itemName, const T & val );
	//! Set the value of a child item.
	template <typename T> bool set( const NifItem * itemParent, const QLatin1String & itemName, const T & val );
	//! Set the value of a child item.
	template <typename T> bool set( const NifItem * itemParent, const char * itemName, const T & val );
	//! Set the value of a model index.
	template <typename T> bool set( const QModelIndex & index, const T & val );
	//! Set the value of a child item.
	template <typename T> bool set( const QModelIndex & itemParent, int itemIndex, const T & val );
	//! Set the value of a child item.
	template <typename T> bool set( const QModelIndex & itemParent, const QString & itemName, const T & val );
	//! Set the value of a child item.
	template <typename T> bool set( const QModelIndex & itemParent, const QLatin1String & itemName, const T & val );
	//! Set the value of a child item.
	template <typename T> bool set( const QModelIndex & itemParent, const char * itemName, const T & val );

	// String resolving ("get ex")
public:
	//! Get the string value of an item, expanding string indices or subitems if necessary.
	QString resolveString( const NifItem * item ) const;
	//! Get the string value of a child item, expanding string indices or subitems if necessary.
	QString resolveString( const NifItem * itemParent, int itemIndex ) const;
	//! Get the string value of a child item, expanding string indices or subitems if necessary.
	QString resolveString( const NifItem * itemParent, const QString & itemName ) const;
	//! Get the string value of a child item, expanding string indices or subitems if necessary.
	QString resolveString( const NifItem * itemParent, const QLatin1String & itemName ) const;
	//! Get the string value of a child item, expanding string indices or subitems if necessary.
	QString resolveString( const NifItem * itemParent, const char * itemName ) const;
	//! Get the string value of a model index, expanding string indices or subitems if necessary.
	QString resolveString( const QModelIndex & index ) const;
	//! Get the string value of a child item, expanding string indices or subitems if necessary.
	QString resolveString( const QModelIndex & itemParent, int itemIndex ) const;
	//! Get the string value of a child item, expanding string indices or subitems if necessary.
	QString resolveString( const QModelIndex & itemParent, const QString & itemName ) const;
	//! Get the string value of a child item, expanding string indices or subitems if necessary.
	QString resolveString( const QModelIndex & itemParent, const QLatin1String & itemName ) const;
	//! Get the string value of a child item, expanding string indices or subitems if necessary.
	QString resolveString( const QModelIndex & itemParent, const char * itemName ) const;

	// String assigning ("set ex")
public:
	//! Set the string value of an item, updating string indices or subitems if necessary.
	bool assignString( NifItem * item, const QString & string, bool replace = false );
	//! Set the string value of a child item, updating string indices or subitems if necessary.
	bool assignString( const NifItem * itemParent, int itemIndex, const QString & string, bool replace = false );
	//! Set the string value of a child item, updating string indices or subitems if necessary.
	bool assignString( const NifItem * itemParent, const QString & itemName, const QString & string, bool replace = false );
	//! Set the string value of a child item, updating string indices or subitems if necessary.
	bool assignString( const NifItem * itemParent, const QLatin1String & itemName, const QString & string, bool replace = false );
	//! Set the string value of a child item, updating string indices or subitems if necessary.
	bool assignString( const NifItem * itemParent, const char * itemName, const QString & string, bool replace = false );
	//! Set the string value of a model index, updating string indices or subitems if necessary.
	bool assignString( const QModelIndex & index, const QString & string, bool replace = false );
	//! Set the string value of a child item, updating string indices or subitems if necessary.
	bool assignString( const QModelIndex & itemParent, int itemIndex, const QString & string, bool replace = false );
	//! Set the string value of a child item, updating string indices or subitems if necessary.
	bool assignString( const QModelIndex & itemParent, const QString & itemName, const QString & string, bool replace = false );
	//! Set the string value of a child item, updating string indices or subitems if necessary.
	bool assignString( const QModelIndex & itemParent, const QLatin1String & itemName, const QString & string, bool replace = false );
	//! Set the string value of a child item, updating string indices or subitems if necessary.
	bool assignString( const QModelIndex & itemParent, const char * itemName, const QString & string, bool replace = false );

	// Link getters
public:
	//! Return the link value (block number) of an item if it's a valid link, otherwise -1.
	qint32 getLink( const NifItem * item ) const;
	//! Return the link value (block number) of a child item if it's a valid link, otherwise -1.
	qint32 getLink( const NifItem * itemParent, int itemIndex ) const;
	//! Return the link value (block number) of a child item if it's a valid link, otherwise -1.
	qint32 getLink( const NifItem * itemParent, const QString & itemName ) const;
	//! Return the link value (block number) of a child item if it's a valid link, otherwise -1.
	qint32 getLink( const NifItem * itemParent, const QLatin1String & itemName ) const;
	//! Return the link value (block number) of a child item if it's a valid link, otherwise -1.
	qint32 getLink( const NifItem * itemParent, const char * itemName ) const;
	//! Return the link value (block number) of a model index if it's a valid link, otherwise -1.
	qint32 getLink( const QModelIndex & index ) const;
	//! Return the link value (block number) of a child item if it's a valid link, otherwise -1.
	qint32 getLink( const QModelIndex & itemParent, int itemIndex ) const;
	//! Return the link value (block number) of a child item if it's a valid link, otherwise -1.
	qint32 getLink( const QModelIndex & itemParent, const QString & itemName ) const;
	//! Return the link value (block number) of a child item if it's a valid link, otherwise -1.
	qint32 getLink( const QModelIndex & itemParent, const QLatin1String & itemName ) const;
	//! Return the link value (block number) of a child item if it's a valid link, otherwise -1.
	qint32 getLink( const QModelIndex & itemParent, const char * itemName ) const;

	// Link setters
public:
	//! Set the link value (block number) of an item if it's a valid link.
	bool setLink( NifItem * item, qint32 link );
	//! Set the link value (block number) of a child item if it's a valid link.
	bool setLink( const NifItem * itemParent, int itemIndex, qint32 link );
	//! Set the link value (block number) of a child item if it's a valid link.
	bool setLink( const NifItem * itemParent, const QString & itemName, qint32 link );
	//! Set the link value (block number) of a child item if it's a valid link.
	bool setLink( const NifItem * itemParent, const QLatin1String & itemName, qint32 link );
	//! Set the link value (block number) of a child item if it's a valid link.
	bool setLink( const NifItem * itemParent, const char * itemName, qint32 link );
	//! Set the link value (block number) of a model index if it's a valid link.
	bool setLink( const QModelIndex & index, qint32 link );
	//! Set the link value (block number) of a child item if it's a valid link.
	bool setLink( const QModelIndex & itemParent, int itemIndex, qint32 link );
	//! Set the link value (block number) of a child item if it's a valid link.
	bool setLink( const QModelIndex & itemParent, const QString & itemName, qint32 link );
	//! Set the link value (block number) of a child item if it's a valid link.
	bool setLink( const QModelIndex & itemParent, const QLatin1String & itemName, qint32 link );
	//! Set the link value (block number) of a child item if it's a valid link.
	bool setLink( const QModelIndex & itemParent, const char * itemName, qint32 link );

	// Link array getters
public:
	//! Return a QVector of link values (block numbers) of an item if it's a valid link array.
	QVector<qint32> getLinkArray( const NifItem * arrayRootItem ) const;
	//! Return a QVector of link values (block numbers) of a child item if it's a valid link array.
	QVector<qint32> getLinkArray( const NifItem * arrayParent, int arrayIndex ) const;
	//! Return a QVector of link values (block numbers) of a child item if it's a valid link array.
	QVector<qint32> getLinkArray( const NifItem * arrayParent, const QString & arrayName ) const;
	//! Return a QVector of link values (block numbers) of a child item if it's a valid link array.
	QVector<qint32> getLinkArray( const NifItem * arrayParent, const QLatin1String & arrayName ) const;
	//! Return a QVector of link values (block numbers) of a child item if it's a valid link array.
	QVector<qint32> getLinkArray( const NifItem * arrayParent, const char * arrayName ) const;
	//! Return a QVector of link values (block numbers) of a model index if it's a valid link array.
	QVector<qint32> getLinkArray( const QModelIndex & iArray ) const;
	//! Return a QVector of link values (block numbers) of a child item if it's a valid link array.
	QVector<qint32> getLinkArray( const QModelIndex & arrayParent, int arrayIndex ) const;
	//! Return a QVector of link values (block numbers) of a child item if it's a valid link array.
	QVector<qint32> getLinkArray( const QModelIndex & arrayParent, const QString & arrayName ) const;
	//! Return a QVector of link values (block numbers) of a child item if it's a valid link array.
	QVector<qint32> getLinkArray( const QModelIndex & arrayParent, const QLatin1String & arrayName ) const;
	//! Return a QVector of link values (block numbers) of a child item if it's a valid link array.
	QVector<qint32> getLinkArray( const QModelIndex & arrayParent, const char * arrayName ) const;

	// Link array setters
public:
	//! Write a QVector of link values (block numbers) to an item if it's a valid link array.
	// The size of QVector must match the current size of the array.
	bool setLinkArray( NifItem * arrayRootItem, const QVector<qint32> & links );
	//! Write a QVector of link values (block numbers) to a child item if it's a valid link array.
	// The size of QVector must match the current size of the array.
	bool setLinkArray( const NifItem * arrayParent, int arrayIndex, const QVector<qint32> & links );
	//! Write a QVector of link values (block numbers) to a child item if it's a valid link array.
	// The size of QVector must match the current size of the array.
	bool setLinkArray( const NifItem * arrayParent, const QString & arrayName, const QVector<qint32> & links );
	//! Write a QVector of link values (block numbers) to a child item if it's a valid link array.
	// The size of QVector must match the current size of the array.
	bool setLinkArray( const NifItem * arrayParent, const QLatin1String & arrayName, const QVector<qint32> & links );
	//! Write a QVector of link values (block numbers) to a child item if it's a valid link array.
	// The size of QVector must match the current size of the array.
	bool setLinkArray( const NifItem * arrayParent, const char * arrayName, const QVector<qint32> & links );
	//! Write a QVector of link values (block numbers) to a model index if it's a valid link array.
	// The size of QVector must match the current size of the array.
	bool setLinkArray( const QModelIndex & iArray, const QVector<qint32> & links );
	//! Write a QVector of link values (block numbers) to a child item if it's a valid link array.
	// The size of QVector must match the current size of the array.
	bool setLinkArray( const QModelIndex & arrayParent, int arrayIndex, const QVector<qint32> & links );
	//! Write a QVector of link values (block numbers) to a child item if it's a valid link array.
	// The size of QVector must match the current size of the array.
	bool setLinkArray( const QModelIndex & arrayParent, const QString & arrayName, const QVector<qint32> & links );
	//! Write a QVector of link values (block numbers) to a child item if it's a valid link array.
	// The size of QVector must match the current size of the array.
	bool setLinkArray( const QModelIndex & arrayParent, const QLatin1String & arrayName, const QVector<qint32> & links );
	//! Write a QVector of link values (block numbers) to a child item if it's a valid link array.
	// The size of QVector must match the current size of the array.
	bool setLinkArray( const QModelIndex & arrayParent, const char * arrayName, const QVector<qint32> & links );

public slots:
	void updateSettings();

signals:
	void linksChanged();
	void lodSliderChanged( bool ) const;
	void beginUpdateHeader();

protected:
	// BaseModel

	bool updateArraySizeImpl( NifItem * array ) override final;
	bool updateByteArraySize( NifItem * array );
	bool updateChildArraySizes( NifItem * parent );

	QString ver2str( quint32 v ) const override final { return version2string( v ); }
	quint32 str2ver( QString s ) const override final { return version2number( s ); }

	//! Get condition value cache NifItem for an item.
	// If the item has no cache NifItem, returns the item itself.
	const NifItem * getConditionCacheItem( const NifItem * item ) const;

	bool evalVersionImpl( const NifItem * item ) const override final;

	bool evalConditionImpl( const NifItem * item ) const override final;

	bool setHeaderString( const QString &, uint ver = 0 ) override final;

	// end BaseModel

	bool loadItem( NifItem * parent, NifIStream & stream );
	bool loadHeader( NifItem * parent, NifIStream & stream );
	bool saveItem( const NifItem * parent, NifOStream & stream ) const;
	bool fileOffset( const NifItem * parent, const NifItem * target, NifSStream & stream, int & ofs ) const;

protected:
	void insertAncestor( NifItem * parent, const QString & identifier, int row = -1 );
	void insertType( NifItem * parent, const NifData & data, int row = -1 );
	NifItem * insertBranch( NifItem * parent, const NifData & data, int row = -1 );

	void updateLinks( int block = -1 );
	void updateLinks( int block, NifItem * parent );
	void checkLinks( int block, QStack<int> & parents );
	void adjustLinks( NifItem * parent, int block, int delta );
	void mapLinks( NifItem * parent, const QMap<qint32, qint32> & map );

	static void updateStrings( NifModel * src, NifModel * tgt, NifItem * item );

	//! NIF file version
	quint32 version;

	QHash<int, QList<int> > childLinks;
	QHash<int, QList<int> > parentLinks;
	QList<int> rootLinks;

	bool lockUpdates;

	enum UpdateType
	{
		utNone   = 0,
		utHeader = 0x1,
		utLinks  = 0x2,
		utFooter = 0x4,
		utAll = 0x7
	};
	UpdateType needUpdates;

	void updateModel( UpdateType value = utAll );

	quint32 bsVersion;
	void cacheBSVersion( const NifItem * headerItem );

	QString topItemRepr( const NifItem * item ) const override final;
	void onItemValueChange( NifItem * item ) override final;

	void invalidateItemConditions( NifItem * item );

	//! Parse the XML file using a NifXmlHandler
	static QString parseXmlDescription( const QString & filename );

	// XML structures
	static QList<quint32> supportedVersions;
	static QHash<QString, NifBlockPtr> compounds;
	static QHash<QString, NifBlockPtr> fixedCompounds;
	static QHash<QString, NifBlockPtr> blocks;
	static QMap<quint32, NifBlockPtr> blockHashes;

	static QHash<QString, QString> arrayPseudonyms;
	static void setupArrayPseudonyms();

private:
	struct Settings
	{
		QString startupVersion;
		int userVersion;
		int userVersion2;
	} cfg;
};


//! Helper class for evaluating condition expressions
class NifModelEval
{
public:
	NifModelEval( const NifModel * model, const NifItem * item );

	QVariant operator()( const QVariant & v ) const;
private:
	const NifModel * model;
	const NifItem * item;
};


// Inlines

inline const NifModel * NifModel::fromIndex( const QModelIndex & index )
{
	return static_cast<const NifModel *>( index.model() ); // qobject_cast
}

inline const NifModel * NifModel::fromValidIndex( const QModelIndex & index )
{
	return index.isValid() ? NifModel::fromIndex( index ) : nullptr;
}

inline QString NifModel::createRTTIName( const QModelIndex & iBlock ) const
{
	return createRTTIName( getItem(iBlock) );
}

inline NifItem * NifModel::getHeaderItem()
{
	return const_cast<NifItem *>( const_cast<const NifModel *>(this)->getHeaderItem() );
}

inline QModelIndex NifModel::getHeaderIndex() const
{
	return itemToIndex( getHeaderItem() );
}

inline NifItem * NifModel::getFooterItem()
{
	return const_cast<NifItem *>( const_cast<const NifModel *>(this)->getFooterItem() );
}

inline QStringList NifModel::allNiBlocks()
{
	QStringList lst;
	for ( NifBlockPtr blk : blocks ) {
		if ( !blk->abstract )
			lst.append( blk->id );
	}
	return lst;
}

inline bool NifModel::isAncestorOrNiBlock( const QString & name ) const
{
	return blocks.contains( name );
}

inline bool NifModel::isNiBlock( const QString & name )
{
	NifBlockPtr blk = blocks.value( name );
	return blk && !blk->abstract;
}

inline bool NifModel::isAncestor( const QString & name )
{
	NifBlockPtr blk = blocks.value( name );
	return blk && blk->abstract;
}

inline bool NifModel::isCompound( const QString & name )
{
	return compounds.contains( name );
}

inline bool NifModel::isFixedCompound( const QString & name )
{
	return fixedCompounds.contains( name );
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

inline bool NifModel::isLink( const NifItem * item ) const
{
	return item ? item->valueIsLink() : false;
}

inline bool NifModel::isLink( const QModelIndex & index ) const
{
	return isLink( getItem(index) );
}

inline bool NifModel::checkVersion( quint32 since, quint32 until ) const
{
	return (( since == 0 || since <= version ) && ( until == 0 || version <= until ));
}

constexpr inline int NifModel::firstBlockRow() const
{
	return 1; // The fist root's child is always the header
}

inline int NifModel::lastBlockRow() const
{	
	return root->childCount() - 2; // The last root's child is always the footer.
}

inline int NifModel::getBlockCount() const
{
	return std::max( lastBlockRow() - firstBlockRow() + 1, 0 );
}

inline int NifModel::getBlockNumber( const QModelIndex & index ) const
{
	return getBlockNumber( getItem(index) );
}


// Block item getters

inline const NifItem * NifModel::getBlockItem( qint32 link, const QString & ancestor ) const
{
	return _getBlockItem( getBlockItem(link), ancestor );
}
inline const NifItem * NifModel::getBlockItem( qint32 link, const QLatin1String & ancestor ) const
{
	return _getBlockItem( getBlockItem(link), ancestor );
}
inline const NifItem * NifModel::getBlockItem( qint32 link, const char * ancestor ) const
{
	return _getBlockItem( getBlockItem(link), QLatin1Literal(ancestor) );
}
inline const NifItem * NifModel::getBlockItem( qint32 link, const std::initializer_list<const char *> & ancestors ) const
{
	return _getBlockItem( getBlockItem(link), ancestors );
}
inline const NifItem * NifModel::getBlockItem( qint32 link, const QStringList & ancestors ) const
{
	return _getBlockItem( getBlockItem(link), ancestors );
}

inline const NifItem * NifModel::getBlockItem( const NifItem * item, const QString & ancestor ) const
{
	return _getBlockItem( getBlockItem(item), ancestor );
}
inline const NifItem * NifModel::getBlockItem( const NifItem * item, const QLatin1String & ancestor ) const
{
	return _getBlockItem( getBlockItem(item), ancestor );
}
inline const NifItem * NifModel::getBlockItem( const NifItem * item, const char * ancestor ) const
{
	return _getBlockItem( getBlockItem(item), QLatin1String(ancestor) );
}
inline const NifItem * NifModel::getBlockItem( const NifItem * item, const std::initializer_list<const char *> & ancestors ) const
{
	return _getBlockItem( getBlockItem(item), ancestors );
}
inline const NifItem * NifModel::getBlockItem( const NifItem * item, const QStringList & ancestors ) const
{
	return _getBlockItem( getBlockItem(item), ancestors );
}

inline const NifItem * NifModel::getBlockItem( const QModelIndex & index ) const
{
	return getBlockItem( getItem(index) );
}
inline const NifItem * NifModel::getBlockItem( const QModelIndex & index, const QString & ancestor ) const
{
	return getBlockItem( getItem(index), ancestor );
}
inline const NifItem * NifModel::getBlockItem( const QModelIndex & index, const QLatin1String & ancestor ) const
{
	return getBlockItem( getItem(index), ancestor );
}
inline const NifItem * NifModel::getBlockItem( const QModelIndex & index, const char * ancestor ) const
{
	return getBlockItem( getItem(index), QLatin1String(ancestor) );
}
inline const NifItem * NifModel::getBlockItem( const QModelIndex & index, const std::initializer_list<const char *> & ancestors ) const
{
	return getBlockItem( getItem(index), ancestors );
}
inline const NifItem * NifModel::getBlockItem( const QModelIndex & index, const QStringList & ancestors ) const
{
	return getBlockItem( getItem(index), ancestors );
}

#define _NIFMODEL_NONCONST_GETBLOCKITEM_1(arg) const_cast<NifItem *>( const_cast<const NifModel *>(this)->getBlockItem( arg ) )
#define _NIFMODEL_NONCONST_GETBLOCKITEM_2(arg1, arg2) const_cast<NifItem *>( const_cast<const NifModel *>(this)->getBlockItem( arg1, arg2 ) )

inline NifItem * NifModel::getBlockItem( qint32 link )
{
	return _NIFMODEL_NONCONST_GETBLOCKITEM_1( link );
}
inline NifItem * NifModel::getBlockItem( qint32 link, const QString & ancestor )
{
	return _NIFMODEL_NONCONST_GETBLOCKITEM_2( link, ancestor );
}
inline NifItem * NifModel::getBlockItem( qint32 link, const QLatin1String & ancestor )
{
	return _NIFMODEL_NONCONST_GETBLOCKITEM_2( link, ancestor );
}
inline NifItem * NifModel::getBlockItem( qint32 link, const char * ancestor )
{
	return _NIFMODEL_NONCONST_GETBLOCKITEM_2( link, QLatin1String(ancestor) );
}
inline NifItem * NifModel::getBlockItem( qint32 link, const std::initializer_list<const char *> & ancestors )
{
	return _NIFMODEL_NONCONST_GETBLOCKITEM_2( link, ancestors );
}
inline NifItem * NifModel::getBlockItem( qint32 link, const QStringList & ancestors )
{
	return _NIFMODEL_NONCONST_GETBLOCKITEM_2( link, ancestors );
}

inline NifItem * NifModel::getBlockItem( const NifItem * item )
{
	return _NIFMODEL_NONCONST_GETBLOCKITEM_1( item );
}
inline NifItem * NifModel::getBlockItem( const NifItem * item, const QString & ancestor )
{
	return _NIFMODEL_NONCONST_GETBLOCKITEM_2( item, ancestor );
}
inline NifItem * NifModel::getBlockItem( const NifItem * item, const QLatin1String & ancestor )
{
	return _NIFMODEL_NONCONST_GETBLOCKITEM_2( item, ancestor );
}
inline NifItem * NifModel::getBlockItem( const NifItem * item, const char * ancestor )
{
	return _NIFMODEL_NONCONST_GETBLOCKITEM_2( item, QLatin1String(ancestor) );
}
inline NifItem * NifModel::getBlockItem( const NifItem * item, const std::initializer_list<const char *> & ancestors )
{
	return _NIFMODEL_NONCONST_GETBLOCKITEM_2( item, ancestors );
}
inline NifItem * NifModel::getBlockItem( const NifItem * item, const QStringList & ancestors )
{
	return _NIFMODEL_NONCONST_GETBLOCKITEM_2( item, ancestors );
}

inline NifItem * NifModel::getBlockItem( const QModelIndex & index )
{
	return _NIFMODEL_NONCONST_GETBLOCKITEM_1( index );
}
inline NifItem * NifModel::getBlockItem( const QModelIndex & index, const QString & ancestor )
{
	return _NIFMODEL_NONCONST_GETBLOCKITEM_2( index, ancestor );
}
inline NifItem * NifModel::getBlockItem( const QModelIndex & index, const QLatin1String & ancestor )
{
	return _NIFMODEL_NONCONST_GETBLOCKITEM_2( index, ancestor );
}
inline NifItem * NifModel::getBlockItem( const QModelIndex & index, const char * ancestor )
{
	return _NIFMODEL_NONCONST_GETBLOCKITEM_2( index, QLatin1String(ancestor) );
}
inline NifItem * NifModel::getBlockItem( const QModelIndex & index, const std::initializer_list<const char *> & ancestors )
{
	return _NIFMODEL_NONCONST_GETBLOCKITEM_2( index, ancestors );
}
inline NifItem * NifModel::getBlockItem( const QModelIndex & index, const QStringList & ancestors )
{
	return _NIFMODEL_NONCONST_GETBLOCKITEM_2( index, ancestors );
}


// Block index getters

#define _NIFMODEL_GETBLOCKINDEX_1(arg) itemToIndex( getBlockItem( arg ) )
#define _NIFMODEL_GETBLOCKINDEX_2(arg1, arg2) itemToIndex( getBlockItem( arg1, arg2 ) )

inline QModelIndex NifModel::getBlockIndex( qint32 link ) const
{
	return _NIFMODEL_GETBLOCKINDEX_1( link );
}
inline QModelIndex NifModel::getBlockIndex( qint32 link, const QString & ancestor ) const
{
	return _NIFMODEL_GETBLOCKINDEX_2( link, ancestor );
}
inline QModelIndex NifModel::getBlockIndex( qint32 link, const QLatin1String & ancestor ) const
{
	return _NIFMODEL_GETBLOCKINDEX_2( link, ancestor );
}
inline QModelIndex NifModel::getBlockIndex( qint32 link, const char * ancestor ) const
{
	return _NIFMODEL_GETBLOCKINDEX_2( link, QLatin1String(ancestor) );
}
inline QModelIndex NifModel::getBlockIndex( qint32 link, const std::initializer_list<const char *> & ancestors ) const
{
	return _NIFMODEL_GETBLOCKINDEX_2( link, ancestors );
}
inline QModelIndex NifModel::getBlockIndex( qint32 link, const QStringList & ancestors ) const
{
	return _NIFMODEL_GETBLOCKINDEX_2( link, ancestors );
}

inline QModelIndex NifModel::getBlockIndex( const NifItem * item ) const
{
	return _NIFMODEL_GETBLOCKINDEX_1( item );
}
inline QModelIndex NifModel::getBlockIndex( const NifItem * item, const QString & ancestor ) const
{
	return _NIFMODEL_GETBLOCKINDEX_2( item, ancestor );
}
inline QModelIndex NifModel::getBlockIndex( const NifItem * item, const QLatin1String & ancestor ) const
{
	return _NIFMODEL_GETBLOCKINDEX_2( item, ancestor );
}
inline QModelIndex NifModel::getBlockIndex( const NifItem * item, const char * ancestor ) const
{
	return _NIFMODEL_GETBLOCKINDEX_2( item, QLatin1String(ancestor) );
}
inline QModelIndex NifModel::getBlockIndex( const NifItem * item, const std::initializer_list<const char *> & ancestors ) const
{
	return _NIFMODEL_GETBLOCKINDEX_2( item, ancestors );
}
inline QModelIndex NifModel::getBlockIndex( const NifItem * item, const QStringList & ancestors ) const
{
	return _NIFMODEL_GETBLOCKINDEX_2( item, ancestors );
}

inline QModelIndex NifModel::getBlockIndex( const QModelIndex & index ) const
{
	return _NIFMODEL_GETBLOCKINDEX_1( index );
}
inline QModelIndex NifModel::getBlockIndex( const QModelIndex & index, const QString & ancestor ) const
{
	return _NIFMODEL_GETBLOCKINDEX_2( index, ancestor );
}
inline QModelIndex NifModel::getBlockIndex( const QModelIndex & index, const QLatin1String & ancestor ) const
{
	return _NIFMODEL_GETBLOCKINDEX_2( index, ancestor );
}
inline QModelIndex NifModel::getBlockIndex( const QModelIndex & index, const char * ancestor ) const
{
	return _NIFMODEL_GETBLOCKINDEX_2( index, QLatin1String(ancestor) );
}
inline QModelIndex NifModel::getBlockIndex( const QModelIndex & index, const std::initializer_list<const char *> & ancestors ) const
{
	return _NIFMODEL_GETBLOCKINDEX_2( index, ancestors );
}
inline QModelIndex NifModel::getBlockIndex( const QModelIndex & index, const QStringList & ancestors ) const
{
	return _NIFMODEL_GETBLOCKINDEX_2( index, ancestors );
}


// isNiBlock

inline bool NifModel::isNiBlock( const NifItem * item, const QString & testType ) const
{
	return isNiBlock(item) && item->hasName(testType);
}
inline bool NifModel::isNiBlock( const NifItem * item, const QLatin1String & testType ) const
{
	return isNiBlock(item) && item->hasName(testType);
}
inline bool NifModel::isNiBlock( const NifItem * item, const char * testType ) const
{
	return isNiBlock( item, QLatin1String(testType) );
}
inline bool NifModel::isNiBlock( const QModelIndex & index ) const
{
	return isNiBlock( getItem(index) );
}
inline bool NifModel::isNiBlock( const QModelIndex & index, const QString & testType ) const
{
	return isNiBlock( getItem(index), testType );
}
inline bool NifModel::isNiBlock( const QModelIndex & index, const QLatin1String & testType ) const
{
	return isNiBlock( getItem(index), testType );
}
inline bool NifModel::isNiBlock( const QModelIndex & index, const char * testType ) const
{
	return isNiBlock( getItem(index), QLatin1String(testType) );
}
inline bool NifModel::isNiBlock( const QModelIndex & index, const std::initializer_list<const char *> & testTypes ) const
{
	return isNiBlock( getItem(index), testTypes );
}
inline bool NifModel::isNiBlock( const QModelIndex & index, const QStringList & testTypes ) const
{
	return isNiBlock( getItem(index), testTypes );
}


// Block inheritance

inline bool NifModel::inherits( const QString & blockName, const char * ancestor ) const
{
	return inherits( blockName, QLatin1String(ancestor) );
}
inline bool NifModel::blockInherits( const NifItem * item, const char * ancestor ) const
{
	return blockInherits( item, QLatin1String(ancestor) );
}
inline bool NifModel::blockInherits( const QModelIndex & index, const QString & ancestor ) const
{
	return blockInherits( getItem(index), ancestor );
}
inline bool NifModel::blockInherits( const QModelIndex & index, const QLatin1String & ancestor ) const
{
	return blockInherits( getItem(index), ancestor );
}
inline bool NifModel::blockInherits( const QModelIndex & index, const char * ancestor ) const
{
	return blockInherits( getItem(index), QLatin1String(ancestor) );
}
inline bool NifModel::blockInherits( const QModelIndex & index, const std::initializer_list<const char *> & ancestors ) const
{
	return blockInherits( getItem(index), ancestors );
}
inline bool NifModel::blockInherits( const QModelIndex & index, const QStringList & ancestors ) const
{
	return blockInherits( getItem(index), ancestors );
}


// Item value getters

template <typename T> inline T NifModel::get( const NifItem * item ) const
{
	return BaseModel::get<T>( item );
}
template <> inline QString NifModel::get( const NifItem * item ) const
{
	return resolveString( item );
}
template <typename T> inline T NifModel::get( const NifItem * itemParent, int itemIndex ) const
{
	return get<T>( getItem(itemParent, itemIndex) );
}
template <typename T> inline T NifModel::get( const NifItem * itemParent, const QString & itemName ) const
{
	return get<T>( getItem(itemParent, itemName) );
}
template <typename T> inline T NifModel::get( const NifItem * itemParent, const QLatin1String & itemName ) const
{
	return get<T>( getItem(itemParent, itemName) );
}
template <typename T> inline T NifModel::get( const NifItem * itemParent, const char * itemName ) const
{
	return get<T>( getItem(itemParent, QLatin1String(itemName)) );
}
template <typename T> inline T NifModel::get( const QModelIndex & index ) const
{
	return get<T>( getItem(index) );
}
template <typename T> inline T NifModel::get( const QModelIndex & itemParent, int itemIndex ) const
{
	return get<T>( getItem(itemParent, itemIndex) );
}
template <typename T> inline T NifModel::get( const QModelIndex & itemParent, const QString & itemName ) const
{
	return get<T>( getItem(itemParent, itemName) );
}
template <typename T> inline T NifModel::get( const QModelIndex & itemParent, const QLatin1String & itemName ) const
{
	return get<T>( getItem(itemParent, itemName) );
}
template <typename T> inline T NifModel::get( const QModelIndex & itemParent, const char * itemName ) const
{
	return get<T>( getItem(itemParent, QLatin1String(itemName)) );
}


// Item value setters

template <typename T> inline bool NifModel::set( NifItem * item, const T & val )
{
	return BaseModel::set<T>( item, val );
}
template <> inline bool NifModel::set( NifItem * item, const QString & val )
{
	return assignString( item, val );
}
template <typename T> inline bool NifModel::set( const NifItem * itemParent, int itemIndex, const T & val )
{
	return set<T>( getItem(itemParent, itemIndex, true), val );
}
template <typename T> inline bool NifModel::set( const NifItem * itemParent, const QString & itemName, const T & val )
{
	return set<T>( getItem(itemParent, itemName, true), val );
}
template <typename T> inline bool NifModel::set( const NifItem * itemParent, const QLatin1String & itemName, const T & val )
{
	return set<T>( getItem(itemParent, itemName, true), val );
}
template <typename T> inline bool NifModel::set( const NifItem * itemParent, const char * itemName, const T & val )
{
	return set<T>( getItem(itemParent, QLatin1String(itemName), true), val );
}
template <typename T> inline bool NifModel::set( const QModelIndex & index, const T & val )
{
	return set<T>( getItem(index), val );
}
template <typename T> inline bool NifModel::set( const QModelIndex & itemParent, int itemIndex, const T & val )
{
	return set<T>( getItem(itemParent, itemIndex, true), val );
}
template <typename T> inline bool NifModel::set( const QModelIndex & itemParent, const QString & itemName, const T & val )
{
	return set<T>( getItem(itemParent, itemName, true), val );
}
template <typename T> inline bool NifModel::set( const QModelIndex & itemParent, const QLatin1String & itemName, const T & val )
{
	return set<T>( getItem(itemParent, itemName, true), val );
}
template <typename T> inline bool NifModel::set( const QModelIndex & itemParent, const char * itemName, const T & val )
{
	return set<T>( getItem(itemParent, QLatin1String(itemName), true), val );
}


// String resolving ("get ex")

inline QString NifModel::resolveString( const NifItem * itemParent, int itemIndex ) const
{
	return resolveString( getItem(itemParent, itemIndex) );
}
inline QString NifModel::resolveString( const NifItem * itemParent, const QString & itemName ) const
{
	return resolveString( getItem(itemParent, itemName) );
}
inline QString NifModel::resolveString( const NifItem * itemParent, const QLatin1String & itemName ) const
{
	return resolveString( getItem(itemParent, itemName) );
}
inline QString NifModel::resolveString( const NifItem * itemParent, const char * itemName ) const
{
	return resolveString( getItem(itemParent, QLatin1String(itemName)) );
}
inline QString NifModel::resolveString( const QModelIndex & index ) const
{
	return resolveString( getItem(index) );
}
inline QString NifModel::resolveString( const QModelIndex & itemParent, int itemIndex ) const
{
	return resolveString( getItem(itemParent, itemIndex) );
}
inline QString NifModel::resolveString( const QModelIndex & itemParent, const QString & itemName ) const
{
	return resolveString( getItem(itemParent, itemName) );
}
inline QString NifModel::resolveString( const QModelIndex & itemParent, const QLatin1String & itemName ) const
{
	return resolveString( getItem(itemParent, itemName) );
}
inline QString NifModel::resolveString( const QModelIndex & itemParent, const char * itemName ) const
{
	return resolveString( getItem(itemParent, QLatin1String(itemName)) );
}


// String assigning ("set ex")

inline bool NifModel::assignString( const NifItem * itemParent, int itemIndex, const QString & string, bool replace )
{
	return assignString( getItem(itemParent, itemIndex, true), string, replace );
}
inline bool NifModel::assignString( const NifItem * itemParent, const QString & itemName, const QString & string, bool replace )
{
	return assignString( getItem(itemParent, itemName, true), string, replace );
}
inline bool NifModel::assignString( const NifItem * itemParent, const QLatin1String & itemName, const QString & string, bool replace )
{
	return assignString( getItem(itemParent, itemName, true), string, replace );
}
inline bool NifModel::assignString( const NifItem * itemParent, const char * itemName, const QString & string, bool replace )
{
	return assignString( getItem(itemParent, QLatin1String(itemName), true), string, replace );
}
inline bool NifModel::assignString( const QModelIndex & index, const QString & string, bool replace )
{
	return assignString( getItem(index), string, replace );
}
inline bool NifModel::assignString( const QModelIndex & itemParent, int itemIndex, const QString & string, bool replace )
{
	return assignString( getItem(itemParent, itemIndex, true), string, replace );
}
inline bool NifModel::assignString( const QModelIndex & itemParent, const QString & itemName, const QString & string, bool replace )
{
	return assignString( getItem(itemParent, itemName, true), string, replace );
}
inline bool NifModel::assignString( const QModelIndex & itemParent, const QLatin1String & itemName, const QString & string, bool replace )
{
	return assignString( getItem(itemParent, itemName, true), string, replace );
}
inline bool NifModel::assignString( const QModelIndex & itemParent, const char * itemName, const QString & string, bool replace )
{
	return assignString( getItem(itemParent, QLatin1String(itemName), true), string, replace );
}


// Link getters

inline qint32 NifModel::getLink( const NifItem * item ) const
{
	return NifItem::valueToLink( item );
}
inline qint32 NifModel::getLink( const NifItem * itemParent, int itemIndex ) const
{
	return getLink( getItem(itemParent, itemIndex) );
}
inline qint32 NifModel::getLink( const NifItem * itemParent, const QString & itemName ) const
{
	return getLink( getItem(itemParent, itemName) );
}
inline qint32 NifModel::getLink( const NifItem * itemParent, const QLatin1String & itemName ) const
{
	return getLink( getItem(itemParent, itemName) );
}
inline qint32 NifModel::getLink( const NifItem * itemParent, const char * itemName ) const
{
	return getLink( getItem(itemParent, QLatin1String(itemName)) );
}
inline qint32 NifModel::getLink( const QModelIndex & index ) const
{
	return getLink( getItem(index) );
}
inline qint32 NifModel::getLink( const QModelIndex & itemParent, int itemIndex ) const
{
	return getLink( getItem(itemParent, itemIndex) );
}
inline qint32 NifModel::getLink( const QModelIndex & itemParent, const QString & itemName ) const
{
	return getLink( getItem(itemParent, itemName) );
}
inline qint32 NifModel::getLink( const QModelIndex & itemParent, const QLatin1String & itemName ) const
{
	return getLink( getItem(itemParent, itemName) );
}
inline qint32 NifModel::getLink( const QModelIndex & itemParent, const char * itemName ) const
{
	return getLink( getItem(itemParent, QLatin1String(itemName)) );
}


// Link setters

inline bool NifModel::setLink( const NifItem * itemParent, int itemIndex, qint32 link )
{
	return setLink( getItem(itemParent, itemIndex), link );
}
inline bool NifModel::setLink( const NifItem * itemParent, const QString & itemName, qint32 link )
{
	return setLink( getItem(itemParent, itemName), link );
}
inline bool NifModel::setLink( const NifItem * itemParent, const QLatin1String & itemName, qint32 link )
{
	return setLink( getItem(itemParent, itemName), link );
}
inline bool NifModel::setLink( const NifItem * itemParent, const char * itemName, qint32 link )
{
	return setLink( getItem(itemParent, QLatin1String(itemName)), link );
}
inline bool NifModel::setLink( const QModelIndex & index, qint32 link )
{
	return setLink( getItem(index), link );
}
inline bool NifModel::setLink( const QModelIndex & itemParent, int itemIndex, qint32 link )
{
	return setLink( getItem(itemParent, itemIndex), link );
}
inline bool NifModel::setLink( const QModelIndex & itemParent, const QString & itemName, qint32 link )
{
	return setLink( getItem(itemParent, itemName), link );
}
inline bool NifModel::setLink( const QModelIndex & itemParent, const QLatin1String & itemName, qint32 link )
{
	return setLink( getItem(itemParent, itemName), link );
}
inline bool NifModel::setLink( const QModelIndex & itemParent, const char * itemName, qint32 link )
{
	return setLink( getItem(itemParent, QLatin1String(itemName)), link );
}


// Link array getters

inline QVector<qint32> NifModel::getLinkArray( const NifItem * arrayParent, int arrayIndex ) const
{
	return getLinkArray( getItem(arrayParent, arrayIndex) );
}
inline QVector<qint32> NifModel::getLinkArray( const NifItem * arrayParent, const QString & arrayName ) const
{
	return getLinkArray( getItem(arrayParent, arrayName) );
}
inline QVector<qint32> NifModel::getLinkArray( const NifItem * arrayParent, const QLatin1String & arrayName ) const
{
	return getLinkArray( getItem(arrayParent, arrayName) );
}
inline QVector<qint32> NifModel::getLinkArray( const NifItem * arrayParent, const char * arrayName ) const
{
	return getLinkArray( getItem(arrayParent, QLatin1String(arrayName)) );
}
inline QVector<qint32> NifModel::getLinkArray( const QModelIndex & iArray ) const
{
	return getLinkArray( getItem(iArray) );
}
inline QVector<qint32> NifModel::getLinkArray( const QModelIndex & arrayParent, int arrayIndex ) const
{
	return getLinkArray( getItem(arrayParent, arrayIndex) );
}
inline QVector<qint32> NifModel::getLinkArray( const QModelIndex & arrayParent, const QString & arrayName ) const
{
	return getLinkArray( getItem(arrayParent, arrayName) );
}
inline QVector<qint32> NifModel::getLinkArray( const QModelIndex & arrayParent, const QLatin1String & arrayName ) const
{
	return getLinkArray( getItem(arrayParent, arrayName) );
}
inline QVector<qint32> NifModel::getLinkArray( const QModelIndex & arrayParent, const char * arrayName ) const
{
	return getLinkArray( getItem(arrayParent, QLatin1String(arrayName)) );
}


// Link array setters

inline bool NifModel::setLinkArray( const NifItem * arrayParent, int arrayIndex, const QVector<qint32> & links )
{
	return setLinkArray( getItem(arrayParent, arrayIndex), links );
}
inline bool NifModel::setLinkArray( const NifItem * arrayParent, const QString & arrayName, const QVector<qint32> & links )
{
	return setLinkArray( getItem(arrayParent, arrayName), links );
}
inline bool NifModel::setLinkArray( const NifItem * arrayParent, const QLatin1String & arrayName, const QVector<qint32> & links )
{
	return setLinkArray( getItem(arrayParent, arrayName), links );
}
inline bool NifModel::setLinkArray( const NifItem * arrayParent, const char * arrayName, const QVector<qint32> & links )
{
	return setLinkArray( getItem(arrayParent, QLatin1String(arrayName)), links );
}
inline bool NifModel::setLinkArray( const QModelIndex & iArray, const QVector<qint32> & links )
{
	return setLinkArray( getItem(iArray), links );
}
inline bool NifModel::setLinkArray( const QModelIndex & arrayParent, int arrayIndex, const QVector<qint32> & links )
{
	return setLinkArray( getItem(arrayParent, arrayIndex), links );
}
inline bool NifModel::setLinkArray( const QModelIndex & arrayParent, const QString & arrayName, const QVector<qint32> & links )
{
	return setLinkArray( getItem(arrayParent, arrayName), links );
}
inline bool NifModel::setLinkArray( const QModelIndex & arrayParent, const QLatin1String & arrayName, const QVector<qint32> & links )
{
	return setLinkArray( getItem(arrayParent, arrayName), links );
}
inline bool NifModel::setLinkArray( const QModelIndex & arrayParent, const char * arrayName, const QVector<qint32> & links )
{
	return setLinkArray( getItem(arrayParent, QLatin1String(arrayName)), links );
}

#endif
