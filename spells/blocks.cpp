#include "blocks.h"
#include "../config.h"

#include <QApplication>
#include <QBuffer>
#include <QClipboard>
#include <QCursor>
#include <QInputDialog>
#include <QMessageBox>
#include <QMimeData>
#include <QSettings>

// Brief description is deliberately not autolinked to class Spell
/*! \file blocks.cpp
 * \brief Block manipulation spells
 *
 * All classes here inherit from the Spell class.
 *
 * spRemoveBranch is declared in \link spells/blocks.h \endlink so that it is accessible to spCombiTris.
 */

//! Add a link to the specified block to a link array
/*!
 * @param nif The model
 * @param iParent The block containing the link array
 * @param array The name of the link array
 * @param link A reference to the block to insert into the link array
 */
static void addLink( NifModel * nif, QModelIndex iParent, QString array, int link )
{
	QModelIndex iSize = nif->getIndex( iParent, QString( "Num %1" ).arg( array ) );
	QModelIndex iArray = nif->getIndex( iParent, array );
	if ( iSize.isValid() && iArray.isValid() )
	{
		int numlinks = nif->get<int>( iSize );
		nif->set<int>( iSize, numlinks + 1 );
		nif->updateArray( iArray );
		nif->setLink( iArray.child( numlinks, 0 ), link );
	}
}

//! Remove a link to a block from the specified link array
/*!
 * @param nif The model
 * @param iParent The block containing the link array
 * @param array The name of the link array
 * @param link A reference to the block to remove from the link array
 */
static void delLink( NifModel * nif, QModelIndex iParent, QString array, int link )
{
	QModelIndex iSize = nif->getIndex( iParent, QString( "Num %1" ).arg( array ) );
	QModelIndex iArray = nif->getIndex( iParent, array );
	QList<qint32> links = nif->getLinkArray( iArray ).toList();
	if ( iSize.isValid() && iArray.isValid() && links.contains( link ) )
	{
		links.removeAll( link );
		nif->set<int>( iSize, links.count() );
		nif->updateArray( iArray );
		nif->setLinkArray( iArray, links.toVector() );
	}
}

// documented in blocks.h
void blockLink( NifModel * nif, const QModelIndex & index, const QModelIndex & iBlock )
{
	if ( nif->isLink( index ) && nif->inherits( iBlock, nif->itemTmplt( index ) ) )
	{
		nif->setLink( index, nif->getBlockNumber( iBlock ) );
	}
	if ( nif->inherits( index, "NiNode" ) && nif->inherits( iBlock, "NiAVObject" ) )
	{
		addLink( nif, index, "Children", nif->getBlockNumber( iBlock ) );
		if ( nif->inherits( iBlock, "NiDynamicEffect" ) )
		{
			addLink( nif, index, "Effects", nif->getBlockNumber( iBlock ) );
		}
	}
	else if ( nif->inherits( index, "NiAVObject" ) && nif->inherits( iBlock, "NiProperty" ) )
	{
		addLink( nif, index, "Properties", nif->getBlockNumber( iBlock ) );
	}
	/*
	*	Temporary workaround for non-NiProperty properties
	*/
	else if ( nif->getBlockName( iBlock ) == "BSLightingShaderProperty" )
	{
		addLink( nif, index, "Properties", nif->getBlockNumber( iBlock ) );
	}
	else if ( nif->inherits( iBlock, "BSShaderProperty") )
	{
		addLink( nif, index, "Properties", nif->getBlockNumber( iBlock ) );
	}

	else if ( nif->inherits( index, "NiAVObject" ) && nif->inherits( iBlock, "NiExtraData" ) )
	{
		addLink( nif, index, "Extra Data List", nif->getBlockNumber( iBlock ) );
	}
	else if ( nif->inherits( index, "NiObjectNET") && nif->inherits( iBlock, "NiTimeController" ) )
	{
		if ( nif->getLink( index, "Controller" ) > 0 )
		{
			blockLink( nif, nif->getBlock( nif->getLink( index, "Controller" ) ), iBlock );
		}
		else
		{
			nif->setLink( index, "Controller", nif->getBlockNumber( iBlock ) );
			nif->setLink( iBlock, "Target", nif->getBlockNumber( index ) );
		}
	}
	else if ( nif->inherits( index, "NiTimeController" ) && nif->inherits( iBlock, "NiTimeController" ) )
	{
		if ( nif->getLink( index, "Next Controller" ) > 0)
		{
			blockLink( nif, nif->getBlock( nif->getLink( index, "Next Controller" ) ), iBlock );
		}
		else
		{
			nif->setLink( index, "Next Controller", nif->getBlockNumber( iBlock ) );
			nif->setLink( iBlock, "Target", nif->getLink( index, "Target" ) );
		}
	}
}

//! Helper function for branch paste
static qint32 getBlockByName( NifModel * nif, const QString & tn )
{
	QStringList ls = tn.split( "|" );
	QString type = ls.value( 0 );
	QString name = ls.value( 1 );
	if ( type.isEmpty() || name.isEmpty() )
		return -1;
	for ( int b = 0; b < nif->getBlockCount(); b++ )
	{
		QModelIndex iBlock = nif->getBlock( b );
		if ( nif->itemName( iBlock ) == type && nif->get<QString>( iBlock, "Name" ) == name )
			return b;
	}
	return -1;
}

//! Helper function for branch copy
static void populateBlocks( QList<qint32> & blocks, NifModel * nif, qint32 block )
{
	if ( ! blocks.contains( block ) ) blocks.append( block );
	foreach ( qint32 link, nif->getChildLinks( block ) )
		populateBlocks( blocks, nif, link );
}

//! Remove the children from the specified block
static void removeChildren( NifModel * nif, const QPersistentModelIndex & iBlock )
{
	QList<QPersistentModelIndex> iChildren;
	foreach ( quint32 link, nif->getChildLinks( nif->getBlockNumber( iBlock ) ) )
		iChildren.append( nif->getBlock( link ) );

	foreach ( QPersistentModelIndex iChild, iChildren )
		if ( iChild.isValid() && nif->getBlockNumber( iBlock ) == nif->getParent( nif->getBlockNumber( iChild ) ) )
			removeChildren( nif, iChild );

	foreach ( QPersistentModelIndex iChild, iChildren )
		if ( iChild.isValid() && nif->getBlockNumber( iBlock ) == nif->getParent( nif->getBlockNumber( iChild ) ) )
			nif->removeNiBlock( nif->getBlockNumber( iChild ) );
}

//! Insert an unattached block
class spInsertBlock : public Spell
{
public:
	QString name() const { return Spell::tr("Insert"); }
	QString page() const { return Spell::tr("Block"); }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		Q_UNUSED(nif);
		return ( ! index.isValid() || ! index.parent().isValid() );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		QStringList ids = nif->allNiBlocks();
		ids.sort();
		
		QMap< QString, QMenu *> map;
		foreach ( QString id, ids )
		{
			QString x( "Other" );
			
			if ( id.startsWith( "Ni" ) )
				x = QString("Ni&") + id.mid( 2, 1 ) + "...";
			if ( id.startsWith( "bhk" ) || id.startsWith( "hk" ) )
				x = "Havok";
			if ( id.startsWith( "BS" ) || id == "AvoidNode" || id == "RootCollisionNode" )
				x = "Bethesda";
			if ( id.startsWith( "Fx" ) )
				x = "Firaxis";
			
			if ( ! map.contains( x ) )
				map[ x ] = new QMenu( x );
			map[ x ]->addAction( id );
		}
		
		QMenu menu;
		foreach ( QMenu * m, map )
			menu.addMenu( m );
		
		QAction * act = menu.exec( QCursor::pos() );
		if ( act ) {
			// insert block
			QModelIndex newindex = nif->insertNiBlock( act->text(), nif->getBlockNumber( index ) + 1 );
			// Set values that can't be handled by defaults in nif.xml
			if ( act->text() == "BSXFlags" ) {
				nif->set<QString>( nif->getIndex( newindex, "Name" ), "BSX" );
			}
			else if ( act->text() == "BSBound" ) {
				nif->set<QString>( nif->getIndex( newindex, "Name" ), "BBX" );
			}
			// return index to new block
			return newindex;
		} else {
			return index;
		}
	}
};

REGISTER_SPELL( spInsertBlock )

//! Attach a Property to a block
class spAttachProperty : public Spell
{
public:
	QString name() const { return Spell::tr("Attach Property"); }
	QString page() const { return Spell::tr("Node"); }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return nif->itemType( index ) == "NiBlock" && nif->inherits( index, "NiAVObject" );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		QMenu menu;
		QStringList ids = nif->allNiBlocks();
		ids.sort();
		foreach ( QString id, ids )
			if ( nif->inherits( id, "NiProperty" ) )
				menu.addAction( id );
		if ( menu.actions().isEmpty() )
			return index;
		QAction * act = menu.exec( QCursor::pos() );
		if ( act )
		{
			QPersistentModelIndex iParent = index;
			QModelIndex iProperty = nif->insertNiBlock( act->text(), nif->getBlockNumber( index ) + 1 );
			
			addLink( nif, iParent, "Properties", nif->getBlockNumber( iProperty ) );
			return iProperty;
		}
		else
			return index;
	}
};

REGISTER_SPELL( spAttachProperty )

//! Attach a Node to a block
class spAttachNode : public Spell
{
public:
	QString name() const { return Spell::tr("Attach Node"); }
	QString page() const { return Spell::tr("Node"); }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return nif->isNiBlock( index ) && nif->inherits( index, "NiNode" );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		QMenu menu;
		QStringList ids = nif->allNiBlocks();
		ids.sort();
		foreach ( QString id, ids )
			if ( nif->inherits( id, "NiAVObject" ) && ! nif->inherits( id, "NiDynamicEffect" ) )
				menu.addAction( id );
		
		QAction * act = menu.exec( QCursor::pos() );
		if ( act )
		{
			QPersistentModelIndex iParent = index;
			QModelIndex iNode = nif->insertNiBlock( act->text(), nif->getBlockNumber( index ) + 1 );
			addLink( nif, iParent, "Children", nif->getBlockNumber( iNode ) );
			return iNode;
		}
		else
			return index;
	}
};

REGISTER_SPELL( spAttachNode )

//! Attach a dynamic effect (4/5 are lights) to a block
class spAttachLight : public Spell
{
public:
	QString name() const { return Spell::tr("Attach Effect"); }
	QString page() const { return Spell::tr("Node"); }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return nif->isNiBlock( index ) && nif->inherits( index, "NiNode" );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		QMenu menu;
		QStringList ids = nif->allNiBlocks();
		ids.sort();
		foreach ( QString id, ids )
			if ( nif->inherits( id, "NiDynamicEffect" ) )
				menu.addAction( id );
		
		QAction * act = menu.exec( QCursor::pos() );
		if ( act )
		{
			QPersistentModelIndex iParent = index;
			QModelIndex iLight = nif->insertNiBlock( act->text(), nif->getBlockNumber( index ) + 1 );
			addLink( nif, iParent, "Children", nif->getBlockNumber( iLight ) );
			addLink( nif, iParent, "Effects", nif->getBlockNumber( iLight ) );

			if ( nif->checkVersion(0, 0x04000002) ) {
				nif->set<int>( iLight, "Num Affected Node List Pointers", 1 );
				nif->updateArray( iLight, "Affected Node List Pointers" );
			}

			if ( act->text() == "NiTextureEffect" ) {
				nif->set<int>( iLight, "Flags", 4 );
				QModelIndex iSrcTex = nif->insertNiBlock( "NiSourceTexture", nif->getBlockNumber( iLight ) + 1 );
				nif->setLink( iLight, "Source Texture", nif->getBlockNumber( iSrcTex ) );
			}
			
			return iLight;
		}
		else
			return index;
	}
};

REGISTER_SPELL( spAttachLight )

//! Attach extra data to a block
class spAttachExtraData : public Spell
{
public:
	QString name() const { return Spell::tr("Attach Extra Data"); }
	QString page() const { return Spell::tr("Node"); }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return nif->isNiBlock( index ) && nif->inherits( index, "NiObjectNET" ) && nif->checkVersion( 0x0a000100, 0 );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		QMenu menu;
		QStringList ids = nif->allNiBlocks();
		ids.sort();
		foreach ( QString id, ids )
			if ( nif->inherits( id, "NiExtraData" ) )
				menu.addAction( id );
		QAction * act = menu.exec( QCursor::pos() );
		if ( act )
		{
			QPersistentModelIndex iParent = index;
			QModelIndex iExtra = nif->insertNiBlock( act->text(), nif->getBlockNumber( index ) + 1 );
			
			// fixup
			if ( act->text() == "BSXFlags" ) {
				nif->set<QString>( nif->getIndex( iExtra, "Name" ), "BSX" );
			}
			else if ( act->text() == "BSBound" ) {
				nif->set<QString>( nif->getIndex( iExtra, "Name" ), "BBX" );
			}
			
			addLink( nif, iParent, "Extra Data List", nif->getBlockNumber( iExtra ) );
			return iExtra;
		}
		else
			return index;
	}
};

REGISTER_SPELL( spAttachExtraData )

//! Remove a block
class spRemoveBlock : public Spell
{
public:
	QString name() const { return Spell::tr("Remove"); }
	QString page() const { return Spell::tr("Block"); }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return nif->isNiBlock( index ) && nif->getBlockNumber( index ) >= 0;
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		nif->removeNiBlock( nif->getBlockNumber( index ) );
		return QModelIndex();
	}
};

REGISTER_SPELL( spRemoveBlock )

//! Copy a block to the clipboard
class spCopyBlock : public Spell
{
public:
	QString name() const { return Spell::tr("Copy"); }
	QString page() const { return Spell::tr("Block"); }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return nif->isNiBlock( index );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		QByteArray data;
		QBuffer buffer( & data );
		if ( buffer.open( QIODevice::WriteOnly ) && nif->save( buffer, index ) )
		{
			QMimeData * mime = new QMimeData;
			mime->setData( QString( "nifskope/niblock/%1/%2" ).arg( nif->itemName( index ) ).arg( nif->getVersion() ), data );
			QApplication::clipboard()->setMimeData( mime );
		}
		return index;
	}
};

REGISTER_SPELL( spCopyBlock )

//! Paste a block from the clipboard
class spPasteBlock : public Spell
{
public:
	QString name() const { return Spell::tr("Paste"); }
	QString page() const { return Spell::tr("Block"); }
	
	QString acceptFormat( const QString & format, const NifModel * nif )
	{
		Q_UNUSED(nif);
		QStringList split = format.split( "/" );
		if ( split.value( 0 ) == "nifskope" && split.value( 1 ) == "niblock" && nif->isNiBlock( split.value( 2 ) ) )
			return split.value( 3 );
		return QString();
	}
	
	QString blockType( const QString & format )
	{
		return format.split( "/" ).value( 2 );
	}
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		Q_UNUSED(index);
		const QMimeData * mime = QApplication::clipboard()->mimeData();
		if ( mime )
			foreach ( QString form, mime->formats() )
				if ( ! acceptFormat( form, nif ).isEmpty() )
					return true;
		return false;
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		const QMimeData * mime = QApplication::clipboard()->mimeData();
		if ( mime )
		{
			foreach ( QString form, mime->formats() )
			{
				QString version = acceptFormat( form, nif );
				if ( ! version.isEmpty() && ( version == nif->getVersion() || QMessageBox::question( 0, Spell::tr("Paste Block"), Spell::tr("Nif versions differ!<br><br>Current File Version: %1<br>Clipboard Data Version: %2<br><br>The results will be unpredictable..." ).arg( nif->getVersion() ).arg( version ), Spell::tr("Continue"), Spell::tr("Cancel") ) == 0 ))
				{
					QByteArray data = mime->data( form );
					QBuffer buffer( & data );
					if ( buffer.open( QIODevice::ReadOnly ) )
					{
						QModelIndex block = nif->insertNiBlock( blockType( form ), nif->getBlockCount() );
						nif->load( buffer, block );
						blockLink( nif, index, block );
						return block;
					}
				}
			}
		}
		return QModelIndex();
	}
};

REGISTER_SPELL( spPasteBlock )

//! Paste a block from the clipboard over another
class spPasteOverBlock : public Spell
{
public:
	QString name() const { return Spell::tr("Paste Over"); }
	QString page() const { return Spell::tr("Block"); }
	
	QString acceptFormat( const QString & format, const NifModel * nif, const QModelIndex & block )
	{
		QStringList split = format.split( "/" );
		if ( split.value( 0 ) == "nifskope" && split.value( 1 ) == "niblock" && nif->isNiBlock( block, split.value( 2 ) ) )
			return split.value( 3 );
		return QString();
	}
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		const QMimeData * mime = QApplication::clipboard()->mimeData();
		if ( mime )
			foreach ( QString form, mime->formats() )
				if ( ! acceptFormat( form, nif, index ).isEmpty() )
					return true;
		return false;
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		const QMimeData * mime = QApplication::clipboard()->mimeData();
		if ( mime )
		{
			foreach ( QString form, mime->formats() )
			{
				QString version = acceptFormat( form, nif, index );
				if ( ! version.isEmpty() && ( version == nif->getVersion() || QMessageBox::question( 0, Spell::tr("Paste Over"), Spell::tr("Nif versions differ!<br><br>Current File Version: %1<br>Clipboard Data Version: %2<br><br>The results will be unpredictable...").arg( nif->getVersion() ).arg( version ), Spell::tr("Continue"), Spell::tr("Cancel") ) == 0 ))
				{
					QByteArray data = mime->data( form );
					QBuffer buffer( & data );
					if ( buffer.open( QIODevice::ReadOnly ) )
					{
						nif->load( buffer, index );
						return index;
					}
				}
			}
		}
		return QModelIndex();
	}
};

REGISTER_SPELL( spPasteOverBlock )

//! Copy a branch (a block and its descendents) to the clipboard
class spCopyBranch : public Spell
{
public:
	QString name() const { return Spell::tr("Copy Branch"); }
	QString page() const { return Spell::tr("Block"); }
	QKeySequence hotkey() const { return QKeySequence( QKeySequence::Copy ); }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return nif->isNiBlock( index );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		QList<qint32> blocks;
		populateBlocks( blocks, nif, nif->getBlockNumber( index ) );
		
		QMap<qint32,qint32> blockMap;
		for ( int b = 0; b < blocks.count(); b++ )
			blockMap.insert( blocks[b], b );
		
		QMap<qint32,QString> parentMap;
		foreach ( qint32 block, blocks )
		{
			foreach ( qint32 link, nif->getParentLinks( block ) )
			{
				if ( ! blocks.contains( link ) && ! parentMap.contains( link ) )
				{
					QString failMessage = Spell::tr("parent link invalid");
					QModelIndex iParent = nif->getBlock( link );
					if ( iParent.isValid() )
					{
						failMessage = Spell::tr("parent unnamed");
						QString name = nif->get<QString>( iParent, "Name" );
						if ( ! name.isEmpty() )
						{
							parentMap.insert( link, nif->itemName( iParent ) + "|" + name );
							continue;
						}
					}
					QMessageBox::information( 0, Spell::tr("Copy Branch"), Spell::tr("failed to map parent link %1 %2 for block %3 %4; %5.")
						.arg( link )
						.arg( nif->itemName( nif->getBlock( link ) ) )
						.arg( block )
						.arg( nif->itemName( nif->getBlock( block ) ) )
						.arg( failMessage )
						);
					return index;
				}
			}
		}
		
		QByteArray data;
		QBuffer buffer( & data );
		if ( buffer.open( QIODevice::WriteOnly ) )
		{
			QDataStream ds( & buffer );
			ds << blocks.count();
			ds << blockMap;
			ds << parentMap;
			foreach ( qint32 block, blocks )
			{
				ds << nif->itemName( nif->getBlock( block ) );
				if ( ! nif->save( buffer, nif->getBlock( block ) ) )
				{
					QMessageBox::information( 0, Spell::tr("Copy Branch"), Spell::tr("failed to save block %1 %2.")
						.arg( block )
						.arg( nif->itemName( nif->getBlock( block ) ) )
						);
					return index;
				}
			}
			QMimeData * mime = new QMimeData;
			mime->setData( QString( "nifskope/nibranch/%1" ).arg( nif->getVersion() ), data );
			QApplication::clipboard()->setMimeData( mime );
		}
		return index;
	}
};

REGISTER_SPELL( spCopyBranch )

//! Paste a branch from the clipboard
class spPasteBranch : public Spell
{
public:
	QString name() const { return Spell::tr("Paste Branch"); }
	QString page() const { return Spell::tr("Block"); }
	// Doesn't work unless the menu entry is unique
	QKeySequence hotkey() const { return QKeySequence( QKeySequence::Paste ); }
	
	QString acceptFormat( const QString & format, const NifModel * nif )
	{
		Q_UNUSED(nif);
		QStringList split = format.split( "/" );
		if ( split.value( 0 ) == "nifskope" && split.value( 1 ) == "nibranch" )
			return split.value( 2 );
		else
			return QString();
	}
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		if ( index.isValid() && ! nif->isNiBlock( index ) && ! nif->isLink( index ) )
			return false;
		const QMimeData * mime = QApplication::clipboard()->mimeData();
		if ( index.isValid() && mime )
			foreach ( QString form, mime->formats() )
				if ( nif->isVersionSupported( nif->version2number( acceptFormat( form, nif ) ) ) )
					return true;
		return false;
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		const QMimeData * mime = QApplication::clipboard()->mimeData();
		if ( mime )
		{
			foreach ( QString form, mime->formats() )
			{
				QString v = acceptFormat( form, nif );
				if ( ! v.isEmpty() && ( v == nif->getVersion() || QMessageBox::question( 0, Spell::tr("Paste Branch"), Spell::tr("Nif versions differ!<br><br>Current File Version: %1<br>Clipboard Data Version: %2<br><br>The results will be unpredictable..." ).arg( nif->getVersion() ).arg( v ), Spell::tr("Continue"), Spell::tr("Cancel") ) == 0 ) )
				{
					QByteArray data = mime->data( form );
					QBuffer buffer( & data );
					if ( buffer.open( QIODevice::ReadOnly ) )
					{
						QDataStream ds( & buffer );
						
						int count;
						ds >> count;
						
						QMap<qint32,qint32> blockMap;
						ds >> blockMap;
						QMutableMapIterator<qint32,qint32> ibm( blockMap );
						while ( ibm.hasNext() )
						{
							ibm.next();
							ibm.value() += nif->getBlockCount();
						}
						
						QMap<qint32,QString> parentMap;
						ds >> parentMap;
						
						QMapIterator<qint32, QString> ipm( parentMap );
						while ( ipm.hasNext() )
						{
							ipm.next();
							qint32 block = getBlockByName( nif, ipm.value() );
							if ( block >= 0 )
							{
								blockMap.insert( ipm.key(), block );
							}
							else
							{
								QMessageBox::information( 0, Spell::tr("Paste Branch"), Spell::tr("failed to map parent link %1")
									.arg( ipm.value() )
									);
								return index;
							}
						}
						
						QModelIndex iRoot;
						
						for ( int c = 0; c < count; c++ )
						{
							QString type;
							ds >> type;
							
							QModelIndex block = nif->insertNiBlock( type, -1 );
							if ( ! nif->loadAndMapLinks( buffer, block, blockMap ) )
								return index;
							if ( c == 0 )
								iRoot = block;
						}
						
						blockLink( nif, index, iRoot );
						
						return iRoot;
					}
				}
			}
		}
		return QModelIndex();
	}
};

REGISTER_SPELL( spPasteBranch )

//! Paste branch without parenting; see spPasteBranch
/*!
 * This was originally a dodgy hack involving duplicating the contents of
 * spPasteBranch and neglecting to link the blocks; now it calls
 * spPasteBranch with a bogus index.
 */
class spPasteBranch2 : public Spell
{
public:
	QString name() const { return Spell::tr("Paste At End"); }
	QString page() const { return Spell::tr("Block"); }
	// hotkey() won't work here, probably because the context menu is not available

	QString acceptFormat( const QString & format, const NifModel * nif )
	{
		Q_UNUSED(nif);
		QStringList split = format.split( "/" );
		if ( split.value( 0 ) == "nifskope" && split.value( 1 ) == "nibranch" )
			return split.value( 2 );
		else
			return QString();
	}
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		//if ( index.isValid() && ! nif->isNiBlock( index ) && ! nif->isLink( index ) )
		//	return false;
		const QMimeData * mime = QApplication::clipboard()->mimeData();
		if ( mime && ! index.isValid() )
			foreach ( QString form, mime->formats() )
				if ( nif->isVersionSupported( nif->version2number( acceptFormat( form, nif ) ) ) )
					return true;
		return false;
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		Q_UNUSED(index);
		spPasteBranch paster;
		paster.cast( nif, QModelIndex() );
		return QModelIndex();
	}
};

REGISTER_SPELL( spPasteBranch2 )

// definitions for spRemoveBranch moved to blocks.h
bool spRemoveBranch::isApplicable( const NifModel * nif, const QModelIndex & iBlock )
{
	int ix = nif->getBlockNumber( iBlock );
	return ( nif->isNiBlock( iBlock ) && ix >= 0 && ( nif->getRootLinks().contains( ix ) || nif->getParent( ix ) >= 0 ) );
}

QModelIndex spRemoveBranch::cast( NifModel * nif, const QModelIndex & index )
{
	QPersistentModelIndex iBlock = index;
	removeChildren( nif, iBlock );
	nif->removeNiBlock( nif->getBlockNumber( iBlock ) );
	return QModelIndex();
}

REGISTER_SPELL( spRemoveBranch )

//! Convert descendents to siblings?
class spFlattenBranch : public Spell
{
public:
	QString name() const { return Spell::tr("Flatten Branch"); }
	QString page() const { return Spell::tr("Block"); }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		QModelIndex iParent = nif->getBlock( nif->getParent( nif->getBlockNumber( index ) ), "NiNode" );
		return nif->inherits( index, "NiNode" ) && iParent.isValid();
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & iNode )
	{
		QModelIndex iParent = nif->getBlock( nif->getParent( nif->getBlockNumber( iNode ) ), "NiNode" );
		doNode( nif, iNode, iParent, Transform() );
		return iNode;
	}
	
	void doNode( NifModel * nif, const QModelIndex & iNode, const QModelIndex & iParent, Transform tp )
	{
		if ( ! nif->inherits( iNode, "NiNode" ) )
			return;
		
		Transform t = tp * Transform( nif, iNode );
		
		QList<qint32> links;
		
		foreach ( qint32 l, nif->getLinkArray( iNode, "Children" ) )
		{
			QModelIndex iChild = nif->getBlock( l );
			if ( nif->getParent( nif->getBlockNumber( iChild ) ) == nif->getBlockNumber( iNode ) )
			{
				Transform tc = t * Transform( nif, iChild );
				tc.writeBack( nif, iChild );
				addLink( nif, iParent, "Children", l );
				delLink( nif, iNode, "Children", l );
				links.append( l );
			}
		}
		
		foreach ( qint32 l, links )
		{
			doNode( nif, nif->getBlock( l, "NiNode" ), iParent, tp );
		}
	}
};

REGISTER_SPELL( spFlattenBranch )

//! Move a block up in the NIF
class spMoveBlockUp : public Spell
{
public:
	QString name() const { return Spell::tr("Move Up"); }
	QString page() const { return Spell::tr("Block"); }
	QKeySequence hotkey() const { return QKeySequence( Qt::CTRL + Qt::Key_Up ); }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return nif->isNiBlock( index ) && nif->getBlockNumber( index ) > 0;
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & iBlock )
	{
		int ix = nif->getBlockNumber( iBlock );
		nif->moveNiBlock( ix, ix - 1 );
		return nif->getBlock( ix - 1 );
	}
};

REGISTER_SPELL( spMoveBlockUp )

//! Move a block down in the NIF
class spMoveBlockDown : public Spell
{
public:
	QString name() const { return Spell::tr("Move Down"); }
	QString page() const { return Spell::tr("Block"); }
	QKeySequence hotkey() const { return QKeySequence( Qt::CTRL + Qt::Key_Down ); }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return nif->isNiBlock( index ) && nif->getBlockNumber( index ) < nif->getBlockCount() - 1;
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & iBlock )
	{
		int ix = nif->getBlockNumber( iBlock );
		nif->moveNiBlock( ix, ix + 1 );
		return nif->getBlock( ix + 1 );
	}
};

REGISTER_SPELL( spMoveBlockDown )

//! Remove blocks by regex
class spRemoveBlocksById : public Spell
{
public:
	QString name() const { return Spell::tr("Remove By Id"); }
	QString page() const { return Spell::tr("Block"); }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		Q_UNUSED(nif);
		return ! index.isValid();
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & )
	{
		NIFSKOPE_QSETTINGS(settings);
		settings.beginGroup( "spells" );
		settings.beginGroup( page() );
		settings.beginGroup( name() );
		
		bool ok = true;
		QString match = QInputDialog::getText( 0, Spell::tr("Remove Blocks by Id"), Spell::tr("Enter a regular expression:"), QLineEdit::Normal,
			settings.value( "match expression", "^BS|^NiBS|^bhk|^hk" ).toString(), & ok );
		
		if ( ! ok )
			return QModelIndex();
		
		settings.setValue( "match expression", match );
		
		QRegExp exp( match );
		
		int n = 0;
		while ( n < nif->getBlockCount() )
		{
			QModelIndex iBlock = nif->getBlock( n );
			if ( nif->itemName( iBlock ).indexOf( exp ) >= 0 )
				nif->removeNiBlock( n );
			else
				n++;
		}
		
		return QModelIndex();
	}
};

REGISTER_SPELL( spRemoveBlocksById )

//! Remove all blocks except a given branch
class spCropToBranch : public Spell
{
public:
	QString name() const { return Spell::tr("Crop To Branch"); }
	QString page() const { return Spell::tr("Block"); }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return nif->isNiBlock( index );
	}
	
	// construct list of block numbers of all blocks that are in the link's branch (including link itself)
	QList<quint32> getBranch(NifModel * nif, quint32 link)
	{
		QList<quint32> branch;
		// add the link itself
		branch << link;
		// add all its children, grandchildren, ...
		foreach (quint32 child, nif->getChildLinks(link))
			// check that child is not in branch to avoid infinite recursion
			if (!branch.contains(child))
				// it's not in there yet so add the child and grandchildren etc...
				branch << getBranch(nif, child);
		// done, so return result
		return branch;
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index)
	{
		// construct list of block numbers of all blocks in this branch of index
		QList<quint32> branch = getBranch(nif, nif->getBlockNumber(index));
		//qWarning() << branch; // DEBUG
		// remove non-branch blocks
		int n = 0; // tracks the current block number in the new system (after some blocks have been removed already)
		int m = 0; // tracks the block number in the old system i.e.  as they are numbered in the branch list
		while ( n < nif->getBlockCount() )
		{
			if (!branch.contains(m))
				nif->removeNiBlock(n);
			else
				n++;
			m++;
		}
		// done
		return QModelIndex();
	}
};

REGISTER_SPELL( spCropToBranch )

//! Convert block types
class spConvertBlock : public Spell
{
public:
	QString name() const { return Spell::tr("Convert"); }
	QString page() const { return Spell::tr("Block"); }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		Q_UNUSED(nif);
		return index.isValid();
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		QStringList ids = nif->allNiBlocks();
		ids.sort();
		
		QString btype = nif->getBlockName( index );
		
		QMap< QString, QMenu *> map;
		foreach ( QString id, ids )
		{
			QString x( "Other" );
			
			// Exclude siblings not in inheritance chain
			if ( btype == id || (!nif->inherits( btype, id ) && !nif->inherits(id, btype) ) )
				continue;         
			
			if ( id.startsWith( "Ni" ) )
				x = QString("Ni&") + id.mid( 2, 1 ) + "...";
			if ( id.startsWith( "bhk" ) || id.startsWith( "hk" ) )
				x = "Havok";
			if ( id.startsWith( "BS" ) || id == "AvoidNode" || id == "RootCollisionNode" )
				x = "Bethesda";
			if ( id.startsWith( "Fx" ) )
				x = "Firaxis";
			
			if ( ! map.contains( x ) )
				map[ x ] = new QMenu( x );
			map[ x ]->addAction( id );
		}
		
		QMenu menu;
		foreach ( QMenu * m, map )
			menu.addMenu( m );
		
		QAction * act = menu.exec( QCursor::pos() );
		if ( act ) {
			nif->convertNiBlock( act->text(), index );
		}
		return index;
	}
};

REGISTER_SPELL( spConvertBlock )

//! Duplicate a block in place
class spDuplicateBlock : public Spell
{
public:
	QString name() const { return Spell::tr("Duplicate"); }
	QString page() const { return Spell::tr("Block"); }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return nif->isNiBlock( index );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		// from spCopyBlock
		QByteArray data;
		QBuffer buffer( & data );
		// Opening in ReadWrite doesn't work - race condition?
		if ( buffer.open( QIODevice::WriteOnly ) && nif->save( buffer, index ) )
		{
			// from spPasteBlock
			if ( buffer.open( QIODevice::ReadOnly ) )
			{
				QModelIndex block = nif->insertNiBlock( nif->getBlockName( index ), nif->getBlockCount() );
				nif->load( buffer, block );
				blockLink( nif, nif->getBlock( nif->getParent( nif->getBlockNumber( index ) ) ), block );
				return block;
			}
		}
		
		return QModelIndex();
	}
};

REGISTER_SPELL( spDuplicateBlock )

//! Duplicate a branch in place
class spDuplicateBranch : public Spell
{
public:
	QString name() const { return Spell::tr("Duplicate Branch"); }
	QString page() const { return Spell::tr("Block"); }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return nif->isNiBlock( index );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		// from spCopyBranch
		QList<qint32> blocks;
		populateBlocks( blocks, nif, nif->getBlockNumber( index ) );
		
		QMap<qint32,qint32> blockMap;
		for ( int b = 0; b < blocks.count(); b++ )
			blockMap.insert( blocks[b], b );
		
		QMap<qint32,QString> parentMap;
		foreach ( qint32 block, blocks )
		{
			foreach ( qint32 link, nif->getParentLinks( block ) )
			{
				if ( ! blocks.contains( link ) && ! parentMap.contains( link ) )
				{
					QString failMessage = Spell::tr("parent link invalid");
					QModelIndex iParent = nif->getBlock( link );
					if ( iParent.isValid() )
					{
						failMessage = Spell::tr("parent unnamed");
						QString name = nif->get<QString>( iParent, "Name" );
						if ( ! name.isEmpty() )
						{
							parentMap.insert( link, nif->itemName( iParent ) + "|" + name );
							continue;
						}
					}
					QMessageBox::information( 0, Spell::tr("Duplicate Branch"), Spell::tr("failed to map parent link %1 %2 for block %3 %4; %5.")
						.arg( link )
						.arg( nif->itemName( nif->getBlock( link ) ) )
						.arg( block )
						.arg( nif->itemName( nif->getBlock( block ) ) )
						.arg( failMessage )
						);
					return index;
				}
			}
		}
		
		QByteArray data;
		QBuffer buffer( & data );
		if ( buffer.open( QIODevice::WriteOnly ) )
		{
			QDataStream ds( & buffer );
			ds << blocks.count();
			ds << blockMap;
			ds << parentMap;
			foreach ( qint32 block, blocks )
			{
				ds << nif->itemName( nif->getBlock( block ) );
				if ( ! nif->save( buffer, nif->getBlock( block ) ) )
				{
					QMessageBox::information( 0, Spell::tr("Duplicate Branch"), Spell::tr("failed to save block %1 %2.")
						.arg( block )
						.arg( nif->itemName( nif->getBlock( block ) ) )
						);
					return index;
				}
			}
		}
		
		// from spPasteBranch
		if ( buffer.open( QIODevice::ReadOnly ) )
		{
			QDataStream ds( & buffer );
			
			int count;
			ds >> count;
			
			QMap<qint32,qint32> blockMap;
			ds >> blockMap;
			QMutableMapIterator<qint32,qint32> ibm( blockMap );
			while ( ibm.hasNext() )
			{
				ibm.next();
				ibm.value() += nif->getBlockCount();
			}
			
			QMap<qint32,QString> parentMap;
			ds >> parentMap;
			
			QMapIterator<qint32, QString> ipm( parentMap );
			while ( ipm.hasNext() )
			{
				ipm.next();
				qint32 block = getBlockByName( nif, ipm.value() );
				if ( block >= 0 )
				{
					blockMap.insert( ipm.key(), block );
				}
				else
				{
					QMessageBox::information( 0, Spell::tr("Duplicate Branch"), Spell::tr("failed to map parent link %1")
						.arg( ipm.value() )
						);
					return index;
				}
			}
			
			QModelIndex iRoot;
			
			for ( int c = 0; c < count; c++ )
			{
				QString type;
				ds >> type;
				
				QModelIndex block = nif->insertNiBlock( type, -1 );
				if ( ! nif->loadAndMapLinks( buffer, block, blockMap ) )
					return index;
				if ( c == 0 )
					iRoot = block;
			}
			
			blockLink( nif, nif->getBlock( nif->getParent( nif->getBlockNumber( index ) ) ), iRoot );
			
			return iRoot;
		}
		return QModelIndex();
	}
};

REGISTER_SPELL( spDuplicateBranch )

//! Sort blocks by name
class spSortBlockNames : public Spell
{
public:
	QString name() const { return Spell::tr("Sort By Name"); }
	QString page() const { return Spell::tr("Block"); }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return ( ! index.isValid() || ! index.parent().isValid() );
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		for ( int n = 0; n < nif->getBlockCount(); n++ )
		{
			QModelIndex iBlock = nif->getBlock( n );
			if ( index.isValid() )
			{
				iBlock = index;
				n = nif->getBlockCount();
			}
			
			QModelIndex iNumChildren = nif->getIndex( iBlock, "Num Children" );
			QModelIndex iChildren = nif->getIndex( iBlock, "Children" );
			// NiNode children are NIAVObjects and have a Name
			if ( iNumChildren.isValid() && iChildren.isValid() )
			{
				QList< QPair<QString, qint32> > links;
				for ( int r = 0; r < nif->rowCount( iChildren ); r++ )
				{
					qint32 l = nif->getLink( iChildren.child( r, 0 ) );
					if ( l >= 0 )
						links.append( QPair<QString, qint32>( nif->get<QString>( nif->getBlock( l ), "Name" ), l ) );
				}
				
				qStableSort( links.begin(), links.end() );
				
				for ( int r = 0; r < links.count(); r++ )
				{
					if ( links[r].second != nif->getLink( iChildren.child( r, 0 ) ) )
						nif->setLink( iChildren.child( r, 0 ), links[r].second );
					nif->set<int>( iNumChildren, links.count() );
					nif->updateArray( iChildren );
				}
			}

		}
		if ( index.isValid() )
		{
			return index;
		}
		else
		{
			return QModelIndex();
		}
	}
};

REGISTER_SPELL( spSortBlockNames )

//! Attach a Node as a parent of the current block
class spAttachParentNode : public Spell
{
public:
	QString name() const { return Spell::tr("Attach Parent Node"); }
	QString page() const { return Spell::tr("Node"); }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return nif->isNiBlock( index );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		// find our current block number
		int thisBlockNumber = nif->getBlockNumber( index );
		// find our parent; most functions won't break if it doesn't exist,
		// so we don't care if it doesn't exist
		QModelIndex iParent = nif->getBlock( nif->getParent( thisBlockNumber ) );
		
		// find our index into the parent children array
		QVector<int> parentChildLinks = nif->getLinkArray( iParent, "Children" );
		int thisBlockIndex = parentChildLinks.indexOf( thisBlockNumber );
		
		// attach a new node
		// basically spAttachNode limited to NiNode and without the auto-attachment
		QMenu menu;
		QStringList ids = nif->allNiBlocks();
		ids.sort();
		foreach ( QString id, ids )
			if ( nif->inherits( id, "NiNode" ) )
				menu.addAction( id );
		
		QModelIndex attachedNode;
		
		QAction * act = menu.exec( QCursor::pos() );
		if ( act )
		{
			attachedNode = nif->insertNiBlock( act->text(), thisBlockNumber );
		}
		else
		{
			return index;
		}
		
		// the attached node pushes this block down one row
		int attachedNodeNumber = thisBlockNumber++;
		
		// replace this block with the attached node
		nif->setLink( nif->getIndex( iParent, "Children" ).child( thisBlockIndex, 0 ), attachedNodeNumber );
		
		// attach ourselves to the attached node
		addLink( nif, attachedNode, "Children", thisBlockNumber );
		
		return attachedNode;
	}
};

REGISTER_SPELL( spAttachParentNode )

