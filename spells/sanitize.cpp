#include "../spellbook.h"

#include <QDebug>

class spSanitizeLinkArrays20000005 : public Spell
{
public:
	QString name() const { return "Adjust Link Arrays"; }
	QString page() const { return "Sanitize"; }
	bool sanity() const { return true; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return nif->checkVersion( 0x14000005, 0x14000005 ) && ! index.isValid();
	}
	
	static bool compareChildLinks( QPair<qint32, bool> a, QPair<qint32, bool> b )
	{
		return a.second != b.second ? a.second : a.first < b.first;
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & )
	{
		for ( int n = 0; n < nif->getBlockCount(); n++ )
		{
			QModelIndex iBlock = nif->getBlock( n );
			
			// reorder children (tribasedgeom first)
			QModelIndex iNumChildren = nif->getIndex( iBlock, "Num Children" );
			QModelIndex iChildren = nif->getIndex( iBlock, "Children" );
			if ( iNumChildren.isValid() && iChildren.isValid() )
			{
				QList< QPair<qint32, bool> > links;
				for ( int r = 0; r < nif->rowCount( iChildren ); r++ )
				{
					qint32 l = nif->getLink( iChildren.child( r, 0 ) );
					if ( l >= 0 )
						links.append( QPair<qint32, bool>( l, nif->inherits( nif->getBlock( l ), "NiTriBasedGeom" ) ) );
				}
				
				qStableSort( links.begin(), links.end(), compareChildLinks );
				
				for ( int r = 0; r < links.count(); r++ )
				{
					if ( links[r].first != nif->getLink( iChildren.child( r, 0 ) ) )
						nif->setLink( iChildren.child( r, 0 ), links[r].first );
					nif->set<int>( iNumChildren, links.count() );
					nif->updateArray( iChildren );
				}
			}
			
			// remove empty property links
			QModelIndex iNumProperties = nif->getIndex( iBlock, "Num Properties" );
			QModelIndex iProperties = nif->getIndex( iBlock, "Properties" );
			if ( iNumProperties.isValid() && iProperties.isValid() )
			{
				QVector<qint32> links;
				for ( int r = 0; r < nif->rowCount( iProperties ); r++ )
				{
					qint32 l = nif->getLink( iProperties.child( r, 0 ) );
					if ( l >= 0 ) links.append( l );
				}
				if ( links.count() < nif->rowCount( iProperties ) )
				{
					nif->set<int>( iNumProperties, links.count() );
					nif->updateArray( iProperties );
					nif->setLinkArray( iProperties, links );
				}
			}

			// remove empty modifier links (NiParticleSystem crashes Oblivion for those)
			QModelIndex iNumModifiers = nif->getIndex( iBlock, "Num Modifiers" );
			QModelIndex iModifiers = nif->getIndex( iBlock, "Modifiers" );
			if ( iNumModifiers.isValid() && iModifiers.isValid() )
			{
				QVector<qint32> links;
				for ( int r = 0; r < nif->rowCount( iModifiers ); r++ )
				{
					qint32 l = nif->getLink( iModifiers.child( r, 0 ) );
					if ( l >= 0 ) links.append( l );
				}
				if ( links.count() < nif->rowCount( iModifiers ) )
				{
					nif->set<int>( iNumModifiers, links.count() );
					nif->updateArray( iModifiers );
					nif->setLinkArray( iModifiers, links );
				}
			}
		}
		return QModelIndex();
	}
};

REGISTER_SPELL( spSanitizeLinkArrays20000005 )


class spAdjustTextureSources : public Spell
{
public:
	QString name() const { return "Adjust Texture Sources"; }
	QString page() const { return "Sanitize"; }
	bool sanity() const { return true; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return nif && ! index.isValid();
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & )
	{
		for ( int i = 0; i < nif->getBlockCount(); i++ )
		{
			QModelIndex iTexSrc = nif->getBlock( i, "NiSourceTexture" );
			if ( iTexSrc.isValid() )
			{
				QModelIndex iFileName = nif->getIndex( iTexSrc, "File Name" );
				if ( iFileName.isValid() ) // adjust file path
					nif->set<QString>( iFileName, nif->get<QString>( iFileName ).replace( "/", "\\" ) );
				
				if ( nif->checkVersion( 0x14000005, 0x14000005 ) )
				{	// adjust format options (oblivion only)
					nif->set<int>( iTexSrc, "Pixel Layout", 6 );
					nif->set<int>( iTexSrc, "Use Mipmaps", 1 );
					nif->set<int>( iTexSrc, "Alpha Format", 3 );
					nif->set<int>( iTexSrc, "Unknown Byte", 1 );
					nif->set<int>( iTexSrc, "Unknown Byte 2", 1 );
				}
			}
		}
		return QModelIndex();
	}
};

REGISTER_SPELL( spAdjustTextureSources )

class spSanitizeBlockOrder : public Spell
{
public:
	QString name() const { return "Reorder Blocks"; }
	QString page() const { return "Sanitize"; }
	bool sanity() const { return true; }
	
	bool isApplicable( const NifModel *, const QModelIndex & index )
	{
		// all files
		return !index.isValid();
	}

	// check whether the block is of a type that comes before the parent or not
	bool childBeforeParent(NifModel * nif, qint32 block) {
		// get index to the block
		QModelIndex iBlock(nif->getBlock(block));
		// check its type
		return (
			nif->inherits(iBlock, "bhkRefObject")
			&& !nif->inherits(iBlock, "bhkConstraint")
		);
	}
	
	// build the nif tree at node block; the block itself and its children are recursively added to
	// the newblocks list
	void addTree(NifModel * nif, qint32 block, QList<qint32> & newblocks) {
		// is the block already added?
		if (newblocks.contains(block))
			return;
		// add all children of block that should be before block
		foreach (qint32 child, nif->getChildLinks(block))
			if (childBeforeParent(nif, child))
				// now add this child and all of its children
				addTree(nif, child, newblocks);
		// add the block
		newblocks.append(block);
		// add all children of block that should be after block
		foreach (qint32 child, nif->getChildLinks(block))
			if (!childBeforeParent(nif, child))
				// now add this child and all of its children
				addTree(nif, child, newblocks);
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & )
	{
		// list of root blocks
		QList<qint32> rootblocks = nif->getRootLinks();

		// list of blocks that have been added
		// newblocks[0] is the block number of the block that must be 
		// assigned number 0
		// newblocks[1] is the block number of the block that must be
		// assigned number 1
		// etc.
		QList<qint32> newblocks;

		// add blocks recursively		
		for (QList<qint32>::iterator rootblock_iter = rootblocks.begin();
			rootblock_iter != rootblocks.end(); rootblock_iter++) {
			addTree(nif, *rootblock_iter, newblocks);
		};

		// check whether all blocks have been added
		if (nif->getBlockCount() != newblocks.size()) {
			qWarning() << "failed to sanitize blocks order, corrupt nif tree?";
			return QModelIndex();
		};

		// invert mapping
		QVector<qint32> order(nif->getBlockCount());

		for (qint32 n = 0; n < newblocks.size(); n++) {
			order[newblocks[n]] = n;
			// DEBUG
			//qWarning() << n << newblocks[n];
		}

		// reorder the blocks
		nif->reorderBlocks(order);

		return QModelIndex();
	}
};

REGISTER_SPELL( spSanitizeBlockOrder )

class spSanityCheckLinks : public Spell
{
public:
	QString name() const { return "Check Links"; }
	QString page() const { return "Sanitize"; }
	bool sanity() const { return true; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return ! index.isValid();
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & )
	{
		for ( int b = 0; b < nif->getBlockCount(); b++ )
		{
			QModelIndex iBlock = nif->getBlock( b );
			QModelIndex idx = check( nif, iBlock );
			if ( idx.isValid() )
				return idx;
		}
		return QModelIndex();
	}
	
	QModelIndex check( NifModel * nif, const QModelIndex & iParent )
	{
		for ( int r = 0; r < nif->rowCount( iParent ); r++ )
		{
			QModelIndex idx = iParent.child( r, 0 );
			bool child;
			if ( nif->isLink( idx, &child ) )
			{
				qint32 l = nif->getLink( idx );
				if ( l < 0 )
				{
					/*
					if ( ! child )
					{
						qWarning() << "unassigned parent link";
						return idx;
					}
					*/
				}
				else if ( l >= nif->getBlockCount() )
				{
					qWarning() << "invalid link";
					return idx;
				}
				else
				{
					QString tmplt = nif->itemTmplt( idx );
					if ( ! tmplt.isEmpty() )
					{
						QModelIndex iBlock = nif->getBlock( l );
						if ( ! nif->inherits( iBlock, tmplt ) )
						{
							qWarning() << "link points to wrong block type";
							return idx;
						}
					}
				}
			}
			if ( nif->rowCount( idx ) > 0 )
			{
				QModelIndex x = check( nif, idx );
				if ( x.isValid() )
					return x;
			}
		}
		return QModelIndex();
	}
};

REGISTER_SPELL( spSanityCheckLinks )
