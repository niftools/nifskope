#include "../spellbook.h"

class spUpdateArray : public Spell
{
public:
	QString name() const { return "Update"; }
	QString page() const { return "Array"; }
	
	bool isApplicable( NifModel * nif, const QModelIndex & index )
	{
		return ! nif->itemArr1( index ).isEmpty();
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
	
	bool isApplicable( NifModel * nif, const QModelIndex & index )
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

class spFollowLink : public Spell
{
public:
	QString name() const { return "Follow Link"; }
	
	bool isApplicable( NifModel * nif, const QModelIndex & index )
	{
		return nif->itemIsLink( index ) && nif->itemLink( index ) >= 0;
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		QModelIndex idx = nif->getBlock( nif->itemLink( index ) );
		if ( idx.isValid() )
			return idx;
		else
			return index;
	}
};

REGISTER_SPELL( spFollowLink )
