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
		QMenu menu;
		QStringList ids = nif->allNiBlocks();
		ids.sort();
		foreach ( QString id, ids )
			menu.addAction( id );
		QAction * act = menu.exec( QCursor::pos() );
		if ( act )
			return nif->insertNiBlock( act->text(), nif->getBlockNumber( index ) + 1 );
		else
			return index;
	}
};

REGISTER_SPELL( spInsertBlock )

class spInsertProperty : public Spell
{
public:
	QString name() const { return "Add Property"; }
	QString page() const { return "Block"; }
	
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
			
			QModelIndex iProperties = nif->getIndex( iParent, "Properties" );
			if ( iProperties.isValid() )
			{
				QModelIndex iIndices = nif->getIndex( iProperties, "Indices" );
				if ( iIndices.isValid() )
				{
					int numlinks = nif->get<int>( iProperties, "Num Indices" );
					nif->set<int>( iProperties, "Num Indices", numlinks + 1 );
					nif->updateArray( iIndices );
					nif->setLink( iIndices.child( numlinks, 0 ), nif->getBlockNumber( iProperty ) );
				}
			}
			
			return iProperty;
		}
		else
			return index;
	}
};

REGISTER_SPELL( spInsertProperty )

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

