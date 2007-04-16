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
		return nif->checkVersion( 0x14000005, 0x14000005 ) && ! index.isValid();
	}
	
	struct wrap
	{
		qint32 blocknr;
		QList<qint32> parents;
		QList<qint32> children;
	};
	
	QModelIndex cast( NifModel * nif, const QModelIndex & )
	{
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
			else if ( nif->inherits( nif->getBlock( n ), "AbhkConstraint" ) )
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
		
		// reorder the blocks
		nif->reorderBlocks( order );
		
		return QModelIndex();
	}

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
