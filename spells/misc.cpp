#include "../spellbook.h"

class spUpdateArray : public Spell
{
public:
	QString name() const { return "Update"; }
	QString page() const { return "Array"; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return nif->isArray( index );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		nif->updateArray( index );
		return index;
	}
};

REGISTER_SPELL( spUpdateArray )

class spUpdateHeader : public Spell
{
public:
	QString name() const { return "Update"; }
	QString page() const { return "Header"; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return ( nif->getHeader() == nif->getBlockOrHeader( index ) );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		nif->updateHeader();
		return index;
	}
};

REGISTER_SPELL( spUpdateHeader )

class spUpdateFooter : public Spell
{
public:
	QString name() const { return "Update"; }
	QString page() const { return "Footer"; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return ( nif->getFooter() == nif->getBlockOrHeader( index ) );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		nif->updateFooter();
		return index;
	}
};

REGISTER_SPELL( spUpdateFooter )

class spFollowLink : public Spell
{
public:
	QString name() const { return "Follow Link"; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return nif->isLink( index ) && nif->getLink( index ) >= 0;
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		QModelIndex idx = nif->getBlock( nif->getLink( index ) );
		if ( idx.isValid() )
			return idx;
		else
			return index;
	}
};

REGISTER_SPELL( spFollowLink )

class spAddLink : public Spell
{
public:
	QString name() const { return "Add Empty Link"; }
	QString page() const { return "Array"; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return nif->itemType( index ) == "linkgroup";
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & iLinkGroup )
	{
		QModelIndex iIndices = nif->getIndex( iLinkGroup, "Indices" );
		if ( iIndices.isValid() )
		{
			int numlinks = nif->get<int>( iLinkGroup, "Num Indices" );
			nif->set<int>( iLinkGroup, "Num Indices", numlinks + 1 );
			nif->updateArray( iIndices );
			return iIndices.child( numlinks, 0 );
		}
		return iLinkGroup;
	}
};

REGISTER_SPELL( spAddLink )

class spRemoveBogusLinks : public Spell
{
public:
	QString name() const { return "Remove Bogus Links"; }
	QString page() const { return "Array"; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return nif->itemType( index ) == "linkgroup";
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & iLinkGroup )
	{
		QModelIndex iIndices = nif->getIndex( iLinkGroup, "Indices" );
		if ( iIndices.isValid() )
		{
			QList<qint32> links;
			bool removed = false;
			for ( int r = 0; r < nif->rowCount( iIndices ); r++ )
			{
				qint32 l = nif->getLink( iIndices.child( r, 0 ) );
				if ( l >= 0 )
					links += l;
				else
					removed = true;
			}
			if ( removed )
			{
				for ( int r = 0; r < links.count(); r++ )
				{
					nif->setLink( iIndices.child( r, 0 ), links[r] );
				}
				nif->set<int>( iLinkGroup, "Num Indices", links.count() );
				nif->updateArray( iIndices );
			}
		}
		return iLinkGroup;
	}
};

REGISTER_SPELL( spRemoveBogusLinks )
