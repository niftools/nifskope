#include "../spellbook.h"

#include <QApplication>
#include <QBuffer>
#include <QClipboard>
#include <QCursor>
#include <QMimeData>


class spInsertBlock : public Spell
{
public:
	QString name() const { return "Insert"; }
	QString page() const { return "Block"; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return ( ! index.isValid() || ! index.parent().isValid() );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		QStringList ids = nif->allNiBlocks();
		
		QMap< QChar, QMenu *> map;
		QMenu * rest = new QMenu( "*" );
		foreach ( QString id, ids )
		{
			if ( id.startsWith( "Ni" ) )
			{
				QChar x = id[2];
				if ( ! map.contains( x.toUpper() ) )
					map.insert( x.toUpper(), new QMenu( x ) );
				map[ x ]->addAction( id );
			}
			else
				rest->addAction( id );
		}

		QMenu menu;
		foreach ( QMenu * m, map )
			menu.addMenu( m );
		menu.addMenu( rest );
		
		QAction * act = menu.exec( QCursor::pos() );
		if ( act )
			return nif->insertNiBlock( act->text(), nif->getBlockNumber( index ) + 1 );
		else
			return index;
	}
};

REGISTER_SPELL( spInsertBlock )

void addLink( NifModel * nif, QModelIndex iParent, QString array, int link )
{
	QModelIndex iLinkGroup = nif->getIndex( iParent, array );
	if ( iLinkGroup.isValid() )
	{
		QModelIndex iIndices = nif->getIndex( iLinkGroup, "Indices" );
		if ( iLinkGroup.isValid() )
		{
			int numlinks = nif->get<int>( iLinkGroup, "Num Indices" );
			nif->set<int>( iLinkGroup, "Num Indices", numlinks + 1 );
			nif->updateArray( iIndices );
			nif->setLink( iIndices.child( numlinks, 0 ), link );
		}
	}
}

class spAttachProperty : public Spell
{
public:
	QString name() const { return "Attach Property"; }
	QString page() const { return "Node"; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return nif->itemType( index ) == "NiBlock" && nif->inherits( index, "ANode" );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		QMenu menu;
		QStringList ids = nif->allNiBlocks();
		ids.sort();
		foreach ( QString id, ids )
			if ( nif->inherits( id, "AProperty" ) )
				menu.addAction( id );
		
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

class spAttachNode : public Spell
{
public:
	QString name() const { return "Attach Node"; }
	QString page() const { return "Node"; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return nif->itemType( index ) == "NiBlock" && nif->inherits( index, "ANode" );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		QMenu menu;
		QStringList ids = nif->allNiBlocks();
		ids.sort();
		foreach ( QString id, ids )
			if ( nif->inherits( id, "ANode" ) && ! nif->inherits( id, "ALight" ) )
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

class spAttachLight : public Spell
{
public:
	QString name() const { return "Attach Light"; }
	QString page() const { return "Node"; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return nif->itemType( index ) == "NiBlock" && nif->inherits( index, "AParentNode" );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		QMenu menu;
		QStringList ids = nif->allNiBlocks();
		ids.sort();
		foreach ( QString id, ids )
			if ( nif->inherits( id, "ALight" ) )
				menu.addAction( id );
		
		QAction * act = menu.exec( QCursor::pos() );
		if ( act )
		{
			QPersistentModelIndex iParent = index;
			QModelIndex iLight = nif->insertNiBlock( act->text(), nif->getBlockNumber( index ) + 1 );
			addLink( nif, iParent, "Children", nif->getBlockNumber( iLight ) );
			addLink( nif, iParent, "Effects", nif->getBlockNumber( iLight ) );
			nif->set<float>( iLight, "Dimmer", 1.0 );
			nif->set<float>( iLight, "Constant Attenuation", 1.0 );
			nif->set<float>( iLight, "Cutoff Angle", 45.0 );
			return iLight;
		}
		else
			return index;
	}
};

REGISTER_SPELL( spAttachLight )

class spRemoveBlock : public Spell
{
public:
	QString name() const { return "Remove"; }
	QString page() const { return "Block"; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return ( nif->itemType( index ) == "NiBlock" && nif->getBlockNumber( index ) >= 0 );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		nif->removeNiBlock( nif->getBlockNumber( index ) );
		return QModelIndex();
	}
};

REGISTER_SPELL( spRemoveBlock )

class spCopyBlock : public Spell
{
public:
	QString name() const { return "Copy"; }
	QString page() const { return "Block"; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return ( nif->itemType( index ) == "NiBlock" && nif->isNiBlock( nif->itemName( index ) ) );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		QByteArray data;
		QBuffer buffer( & data );
		if ( buffer.open( QIODevice::WriteOnly ) && nif->save( buffer, index ) )
		{
			QMimeData * mime = new QMimeData;
			mime->setData( QString( "nifskope/niblock/%1/%2" ).arg( nif->getVersion() ).arg( nif->itemName( index ) ), data );
			QApplication::clipboard()->setMimeData( mime );
		}
		return index;
	}
};

REGISTER_SPELL( spCopyBlock )

class spPasteBlock : public Spell
{
public:
	QString name() const { return "Paste"; }
	QString page() const { return "Block"; }
	
	QString blockType( const QString & format, const NifModel * nif )
	{
		QStringList split = format.split( "/" );
		if ( split.value( 0 ) == "nifskope" && split.value( 1 ) == "niblock"
		&& split.value( 2 ) == nif->getVersion() && nif->isNiBlock( split.value( 3 ) ) )
			return split.value( 3 );
		return QString();
	}
	
	bool acceptFormat( const QString & format, const NifModel * nif )
	{
		return ! blockType( format, nif ).isEmpty();
	}
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		const QMimeData * mime = QApplication::clipboard()->mimeData();
		if ( mime )
			foreach ( QString form, mime->formats() )
				if ( acceptFormat( form, nif ) )
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
				if ( acceptFormat( form, nif ) )
				{
					QByteArray data = mime->data( form );
					QBuffer buffer( & data );
					if ( buffer.open( QIODevice::ReadOnly ) )
					{
						QModelIndex block = nif->insertNiBlock( blockType( form, nif ), nif->getBlockNumber( index ) + 1 );
						nif->load( buffer, block );
						return block;
					}
				}
			}
		}
		return QModelIndex();
	}
};

REGISTER_SPELL( spPasteBlock )

class spPasteOverBlock : public Spell
{
public:
	QString name() const { return "Paste Over"; }
	QString page() const { return "Block"; }
	
	QString blockType( const QString & format, const NifModel * nif )
	{
		QStringList split = format.split( "/" );
		if ( split.value( 0 ) == "nifskope" && split.value( 1 ) == "niblock"
		&& split.value( 2 ) == nif->getVersion() && nif->isNiBlock( split.value( 3 ) ) )
			return split.value( 3 );
		return QString();
	}
	
	bool acceptFormat( const QString & format, const NifModel * nif, const QModelIndex & block )
	{
		return nif->itemType( block ) == "NiBlock" && blockType( format, nif ) == nif->itemName( block );
	}
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		const QMimeData * mime = QApplication::clipboard()->mimeData();
		if ( mime )
			foreach ( QString form, mime->formats() )
				if ( acceptFormat( form, nif, index ) )
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
				if ( acceptFormat( form, nif, index ) )
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

#include <QDebug>

class spCopyBranch : public Spell
{
public:
	QString name() const { return "Copy Branch"; }
	QString page() const { return "Block"; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return ( nif->itemType( index ) == "NiBlock" && nif->isNiBlock( nif->itemName( index ) ) );
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
					QModelIndex iParent = nif->getBlock( link );
					if ( iParent.isValid() )
					{
						QString name = nif->get<QString>( iParent, "Name" );
						if ( ! name.isEmpty() )
						{
							parentMap.insert( link, nif->itemName( iParent ) + "|" + name );
							continue;
						}
					}
					qWarning() << "failed to map parent link" << link << "for block" << block << nif->itemName( nif->getBlock( block ) );
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
					qWarning() << "failed to save block" << block << nif->itemName( nif->getBlock( block ) );
					return index;
				}
			}
			QMimeData * mime = new QMimeData;
			mime->setData( QString( "nifskope/nibranch/%1" ).arg( nif->getVersion() ), data );
			QApplication::clipboard()->setMimeData( mime );
		}
		return index;
	}
	
	void populateBlocks( QList<qint32> & blocks, NifModel * nif, qint32 block )
	{
		if ( ! blocks.contains( block ) ) blocks.append( block );
		foreach ( qint32 link, nif->getChildLinks( block ) )
			populateBlocks( blocks, nif, link );
	}
};

REGISTER_SPELL( spCopyBranch )

class spPasteBranch : public Spell
{
public:
	QString name() const { return "Paste Branch"; }
	QString page() const { return "Block"; }
	
	bool acceptFormat( const QString & format, const NifModel * nif )
	{
		QStringList split = format.split( "/" );
		return ( split.value( 0 ) == "nifskope" && split.value( 1 ) == "nibranch" && split.value( 2 ) == nif->getVersion() );
	}
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		const QMimeData * mime = QApplication::clipboard()->mimeData();
		if ( mime )
			foreach ( QString form, mime->formats() )
				if ( acceptFormat( form, nif ) )
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
				if ( acceptFormat( form, nif ) )
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
								qWarning() << "failed to map parent link" << ipm.value();
								return index;
							}
						}
						
						for ( int c = 0; c < count; c++ )
						{
							QString type;
							ds >> type;
							
							QModelIndex block = nif->insertNiBlock( type, -1 );
							if ( ! nif->loadAndMapLinks( buffer, block, blockMap ) )
								return index;
						}
						return QModelIndex();
					}
				}
			}
		}
		return QModelIndex();
	}
	
	qint32 getBlockByName( NifModel * nif, const QString & tn )
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
};

REGISTER_SPELL( spPasteBranch )

