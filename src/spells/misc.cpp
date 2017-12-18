#include "misc.h"
#include "model/undocommands.h"

#include <QFileDialog>

// Brief description is deliberately not autolinked to class Spell
/*! \file misc.cpp
 * \brief Miscellaneous helper spells
 *
 * All classes here inherit from the Spell class.
 */

//! Update an array if eg. the size has changed
class spUpdateArray final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Update" ); }
	QString page() const override final { return Spell::tr( "Array" ); }
	QIcon icon() const override final { return QIcon( ":/img/update" ); }
	bool instant() const override final { return true; }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		if ( nif->isArray( index ) ) {
			//Check if array is of fixed size
			NifItem * item = static_cast<NifItem *>( index.internalPointer() );
			bool static1 = true;
			bool static2 = true;

			if ( item->arr1().isEmpty() == false ) {
				item->arr1().toInt( &static1 );
			}

			if ( item->arr2().isEmpty() == false ) {
				item->arr2().toInt( &static2 );
			}

			//Leave this commented out until a way for static arrays to be initialized to the right size is created.
			//if ( static1 && static2 )
			//{
			//	//Neither arr1 or arr2 is a variable name
			//	return false;
			//}

			//One of arr1 or arr2 is a variable name so the array is dynamic
			return true;
		}

		return false;
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		nif->undoStack->push( new ArrayUpdateCommand( index, nif ) );
		return index;
	}
};

REGISTER_SPELL( spUpdateArray )

//! Updates the header of the NifModel
class spUpdateHeader final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Update" ); }
	QString page() const override final { return Spell::tr( "Header" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		return ( nif->getHeader() == nif->getBlockOrHeader( index ) );
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		nif->updateHeader();
		return index;
	}
};

REGISTER_SPELL( spUpdateHeader )

//! Updates the footer of the NifModel
class spUpdateFooter final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Update" ); }
	QString page() const override final { return Spell::tr( "Footer" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		return ( nif->getFooter() == nif->getBlockOrHeader( index ) );
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		nif->updateFooter();
		return index;
	}
};

REGISTER_SPELL( spUpdateFooter )

//! Follows a link
class spFollowLink final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Follow Link" ); }
	bool instant() const override final { return true; }
	QIcon icon() const override final { return QIcon( ":/img/link" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		return nif->isLink( index ) && nif->getLink( index ) >= 0;
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		QModelIndex idx = nif->getBlock( nif->getLink( index ) );

		if ( idx.isValid() )
			return idx;

		return index;
	}
};

REGISTER_SPELL( spFollowLink )

//! Estimates the file offset of an item in a model
class spFileOffset final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "File Offset" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		return nif && index.isValid();
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		int ofs = nif->fileOffset( index );
		Message::info( nif->getWindow(),
			Spell::tr( "Estimated file offset is %1 (0x%2)" ).arg( ofs ).arg( ofs, 0, 16 ),
			Spell::tr( "Block: %1\nOffset: %2 (0x%3)" ).arg( index.data( Qt::DisplayRole ).toString() ).arg( ofs ).arg( ofs, 0, 16 )
		);
		return index;
	}
};

REGISTER_SPELL( spFileOffset )

//! Exports the binary data of a binary row to a file
class spExportBinary final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Export Binary" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		NifItem * item = static_cast<NifItem *>(index.internalPointer());

		return item && (item->value().isByteArray() || (item->isBinary() && item->isArray())) && index.isValid();
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		NifItem * item = static_cast<NifItem *>(index.internalPointer());

		QByteArray data;

		NifItem * dataItem = item;
		if ( item->isArray() && item->isBinary() ) {
			dataItem = item->child( 0 );
		}

		if ( dataItem && dataItem->value().isByteArray() ) {
			auto bytes = dataItem->value().get<QByteArray *>();
			data.append( *bytes );
		}

		// Get parent block name and number
		int blockNum = nif->getBlockNumber( index );
		QString suffix = QString( "%1_%2" ).arg( nif->getBlockName( nif->getBlock( blockNum ) ) ).arg( blockNum );
		QString filestring = QString( "%1-%2" ).arg( nif->getFilename() ).arg( suffix );

		QString filename = QFileDialog::getSaveFileName( qApp->activeWindow(), tr( "Export Binary File" ),
														 filestring, "*.*" );
		QFile file( filename );
		if ( file.open( QIODevice::WriteOnly ) ) {
			file.write( data );
			file.close();
		}

		return index;
	}
};

REGISTER_SPELL( spExportBinary )

//! Imports the binary data of a file to a binary row
class spImportBinary final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Import Binary" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		NifItem * item = static_cast<NifItem *>(index.internalPointer());

		return item && (item->value().isByteArray() || (item->isBinary() && item->isArray())) && index.isValid();
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		NifItem * item = static_cast<NifItem *>(index.internalPointer());
		NifItem * parent = item->parent();
		auto iParent = index.parent();

		auto idx = index;
		if ( item->isArray() && item->isBinary() ) {
			parent = item;
			iParent = index;
			idx = index.child( 0, 0 );
		}

		QString filename = QFileDialog::getOpenFileName( qApp->activeWindow(), tr( "Import Binary File" ), "", "*.*" );
		QFile file( filename );
		if ( file.open( QIODevice::ReadOnly ) ) {
			QByteArray data = file.readAll();

			if ( parent->isArray() && parent->isBinary() ) {
				// NOTE: This will only work on byte arrays where the array length is not an expression
				nif->set<int>( iParent.parent(), parent->arr1(), data.count() );
				nif->updateArray( iParent );
			}
			
			nif->set<QByteArray>( idx, data );
			
			file.close();
		}

		return index;
	}
};

REGISTER_SPELL( spImportBinary )

// definitions for spCollapseArray moved to misc.h
bool spCollapseArray::isApplicable( const NifModel * nif, const QModelIndex & index )
{
	if ( nif->isArray( index ) && index.isValid()
	     && ( nif->getBlockType( index ) == "Ref" || nif->getBlockType( index ) == "Ptr" ) )
	{
		// copy from spUpdateArray when that changes
		return true;
	}

	return false;
}

QModelIndex spCollapseArray::cast( NifModel * nif, const QModelIndex & index )
{
	nif->updateArray( index );
	// There's probably an easier way of doing this hiding in NifModel somewhere
	NifItem * item = static_cast<NifItem *>( index.internalPointer() );
	QModelIndex size  = nif->getIndex( nif->getBlock( index.parent() ), item->arr1() );
	QModelIndex array = static_cast<QModelIndex>( index );
	return numCollapser( nif, size, array );
}

QModelIndex spCollapseArray::numCollapser( NifModel * nif, QModelIndex & iNumElem, QModelIndex & iArray )
{
	if ( iNumElem.isValid() && iArray.isValid() ) {
		QVector<qint32> links;

		for ( int r = 0; r < nif->rowCount( iArray ); r++ ) {
			qint32 l = nif->getLink( iArray.child( r, 0 ) );

			if ( l >= 0 )
				links.append( l );
		}

		if ( links.count() < nif->rowCount( iArray ) ) {
			nif->set<int>( iNumElem, links.count() );
			nif->updateArray( iArray );
			nif->setLinkArray( iArray, links );
		}
	}

	return iArray;
}

REGISTER_SPELL( spCollapseArray )

