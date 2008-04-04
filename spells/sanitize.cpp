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


class spSanitizeHavokBlockOrder : public Spell
{
public:
	QString name() const { return "Reorder Havok Blocks"; }
	QString page() const { return "Sanitize"; }
	bool sanity() const { return true; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		// bhk blocks can occur in 20.0.0.4 and 20.0.0.5 files
		return nif->checkVersion( 0x14000004, 0x14000005 ) && ! index.isValid();
	}
/*	
	struct wrap
	{
		qint32 blocknr;
		QList<qint32> parents;
		QList<qint32> children;
	};
*/	
	QModelIndex cast( NifModel * nif, const QModelIndex & )
	{
/*
		// wrap blocks for sorting
		QVector<wrap> sortwrapper( nif->getBlockCount() );
		for ( qint32 n = 0; n < nif->getBlockCount(); n++ )
		{
			sortwrapper[n].blocknr = n;
			if ( isHvkBlock( nif, nif->getBlock( n ) ) )
			{
				QStack<qint32> stack;
				stack.push( n );
				setup( nif, stack, sortwrapper );
			}
			else if ( nif->inherits( nif->getBlock( n ), "bhkConstraint" ) )
			{
				QStack<qint32> stack;
				stack.push( n );
				setupConstraint( nif, stack, sortwrapper );
			}
		}
		
		// brute force sorting
		QVector<wrap>::iterator i, j;
		i = sortwrapper.begin();
		while ( i != sortwrapper.end() )
		{
			j = i + 1;
			while ( j != sortwrapper.end() )
			{
				if ( compareBlocks( *j, *i ) )
				{
					qSwap( *j, *i );
					break;
				}
				++j;
			}
			if ( j == sortwrapper.end() )
				++i;
		}
		
		// extract the new block order
		QVector<qint32> order( sortwrapper.count() );
		for ( qint32 n = 0; n < sortwrapper.count(); n++ )
			order[ sortwrapper[ n ].blocknr ] = n;

		// DEBUG
		for ( qint32 n = 0; n < sortwrapper.count(); n++ )
			qWarning() << n << sortwrapper[n].blocknr;
		// reorder the blocks
		nif->reorderBlocks( order );
*/
		// new version of order block code

		// list of blocks that still must be added
		// at this moment this is just the identity map
		QList<qint32> oldblocks;
		for (qint32 n = 0; n < nif->getBlockCount(); n++)
			oldblocks.append(n);

		// list of blocks that have been added
		// newblocks[0] is the block number of the block that must be 
		// assigned number 0
		// newblocks[1] is the block number of the block that must be
		// assigned number 1
		// etc.
		QVector<qint32> newblocks;

		// add blocks, as long as there are blocks to be added
		while (!oldblocks.isEmpty()) {
			QList<qint32>::iterator oldblocks_iter = oldblocks.begin();
			while (oldblocks_iter != oldblocks.end()) {
				// flag to signal block adding
				bool addit = true;
				// model index of current block (to check type)
				QModelIndex iBlock(nif->getBlock(*oldblocks_iter));
				// should we add block oldblocknr?
				
				// first check that havok children are added before their parents
				if (nif->inherits(iBlock, "bhkRefObject") || nif->inherits(iBlock, "NiCollisionObject")) {
					// havok block: check that all of its
					// children are in newblocks
					// (note: for speed we really should
					// have a lookup table, oh well...)
					QList<qint32> allchildren = getAllChildLinks(nif, *oldblocks_iter);
					//qWarning() << *oldblocks_iter << "children" << allchildren;
					foreach (qint32 child, allchildren) {
						if (!newblocks.contains(child)) {
							//qWarning() << "not yet adding" << *oldblocks_iter;
							addit = false;
							break;
						}
					}					
				}

				// nitribasedgeom block: check if it has a ninode parent,
				// and if so check that this ninode's collision object
				// tree has already been added into newblocks
				if (nif->inherits(iBlock, "NiTriBasedGeom")) {
					// get the parent block index
					qint32 oldblock_parent = nif->getParent(*oldblocks_iter);
					QModelIndex iParent(nif->getBlock(oldblock_parent));
					// check that the parent is a NiAVObject (that is, one that has a Collision Object)
					if (nif->inherits(iParent, "NiAVObject")) {
						//qWarning() << "NiTriBasedGeom" << *oldblocks_iter << "with parent" << oldblock_parent;
						// it is! get its collision object block number
						QModelIndex iCollision(nif->getIndex(iParent, "Collision Object"));

						if (iCollision.isValid()) {
							// convert index into block number
							quint32 collision_sibling = nif->getLink(iCollision);
							//qWarning() << "checking collision" << collision_sibling;
							// we don't need to check the children anymore!
							// the previous check ensures already that
							// collision_block already has the highest block
							// number in the tree
							// so we only need to check that collision_block
							// is already added
							if (!newblocks.contains(collision_sibling)) {
								//qWarning() << "not yet adding" << *oldblocks_iter;
								addit = false;
							}
						}
					}
				}

				// note, other blocks: no extra check needed
				if (addit) {
					//qWarning() << "adding" << *oldblocks_iter;
					// add the block and remove it from old list
					newblocks.append(*oldblocks_iter);
					oldblocks.erase(oldblocks_iter);
					// start a new iteration
					break;
				}
				oldblocks_iter++;
			}
		}

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

	// get all children, as block numbers
	static QList<qint32> getAllChildLinks(const NifModel * nif, qint32 link)
	{
		// get list of immediate children
		QList<qint32> children(nif->getChildLinks(link));
		// construct list of all children: start with immediate ones
		QList<qint32> allchildren(children);
		foreach (qint32 child, children)
			// now add allchildren of each immediate child
			allchildren += getAllChildLinks(nif, child);
		// return result
		return allchildren;
	}
/*
	static bool isHvkBlock( const NifModel * nif, const QModelIndex & iBlock )
	{
		return nif->inherits( iBlock, "bhkShape" ) || nif->inherits( iBlock, "NiCollisionObject" ) || nif->inherits( iBlock, "NiTriBasedGeomData" );
	}
	
	static void setup( const NifModel * nif, QStack<qint32> & stack, QVector<wrap> & sortwrapper )
	{
		foreach ( qint32 link, nif->getChildLinks( stack.top() ) )
		{
			if ( isHvkBlock( nif, nif->getBlock( link ) ) )
			{
				foreach ( qint32 x, stack )
				{
					if ( ! sortwrapper[ link ].parents.contains( x ) )
						sortwrapper[ link ].parents.append( x );
					if ( ! sortwrapper[ x ].children.contains( link ) )
						sortwrapper[ x ].children.append( link );
				}
				
				stack.push( link );
				setup( nif, stack, sortwrapper );
				stack.pop();
			}
		}
	}
	
	static void setupConstraint( const NifModel * nif, QStack<qint32> & stack, QVector<wrap> & sortwrapper )
	{
		foreach ( qint32 link, nif->getParentLinks( stack.top() ) )
		{
			if ( isHvkBlock( nif, nif->getBlock( link ) ) )
			{
				foreach ( qint32 x, stack )
				{
					if ( ! sortwrapper[ link ].parents.contains( x ) )
						sortwrapper[ link ].parents.append( x );
					if ( ! sortwrapper[ x ].children.contains( link ) )
						sortwrapper[ x ].children.append( link );
				}
			}
		}
	}

	static bool compareBlocks( const wrap & a, const wrap & b )
	{
		return a.parents.contains( b.blocknr ) || b.children.contains( a.blocknr );
	}
*/
};

REGISTER_SPELL( spSanitizeHavokBlockOrder )


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
