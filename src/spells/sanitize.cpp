#include "spellbook.h"
#include "sanitize.h"
#include "spells/misc.h"

#include <QInputDialog>

#include <algorithm> // std::stable_sort


// Brief description is deliberately not autolinked to class Spell
/*! \file sanitize.cpp
 * \brief Sanity spells
 *
 * These spells are called by SpellBook::sanitize.
 *
 * All classes here inherit from the Spell class.
 */

//! Reorders blocks to put shapes before nodes (for Oblivion / FO3)
class spReorderLinks final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Reorder Link Arrays" ); }
	QString page() const override final { return Spell::tr( "Sanitize" ); }
	bool sanity() const override final { return true; }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		return ( !index.isValid() && ( nif->getVersionNumber() >= 0x14000004 ) );
	}

	//! Comparator for link sort.
	/**
	 * If booleans of the pair are not equal, sort based on the first boolean.
	 * For spReorderLinks this will determine a sort of geometry before nodes
	 * in the children links array.
	 */
	static bool compareChildLinks( const QPair<qint32, bool> & a, const QPair<qint32, bool> & b )
	{
		return a.second != b.second ? a.second : a.first < b.first;
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & ) override final
	{
		for ( int n = 0; n < nif->getBlockCount(); n++ ) {
			QModelIndex iBlock = nif->getBlock( n );

			QModelIndex iNumChildren = nif->getIndex( iBlock, "Num Children" );
			QModelIndex iChildren = nif->getIndex( iBlock, "Children" );

			if ( iNumChildren.isValid() && iChildren.isValid() ) {
				QList<QPair<qint32, bool> > links;

				for ( int r = 0; r < nif->rowCount( iChildren ); r++ ) {
					qint32 l = nif->getLink( iChildren.child( r, 0 ) );

					if ( l >= 0 ) {
						links.append( QPair<qint32, bool>( l, nif->inherits( nif->getBlock( l ), "NiTriBasedGeom" ) ) );
					}
				}

				std::stable_sort( links.begin(), links.end(), compareChildLinks );

				for ( int r = 0; r < links.count(); r++ ) {
					if ( links[r].first != nif->getLink( iChildren.child( r, 0 ) ) ) {
						nif->setLink( iChildren.child( r, 0 ), links[r].first );
					}
				}

				// update child count & array even if there are no rows (i.e. prune empty children)
				nif->set<int>( iNumChildren, links.count() );
				nif->updateArray( iChildren );
			}
		}

		return QModelIndex();
	}
};

REGISTER_SPELL( spReorderLinks )

//! Removes empty members from link arrays
class spSanitizeLinkArrays final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Collapse Link Arrays" ); }
	QString page() const override final { return Spell::tr( "Sanitize" ); }
	bool sanity() const override final { return true; }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		Q_UNUSED( nif );
		return !index.isValid();
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & ) override final
	{
		for ( int n = 0; n < nif->getBlockCount(); n++ ) {
			QModelIndex iBlock = nif->getBlock( n );

			spCollapseArray arrayCollapser;

			// remove empty children links
			QModelIndex iNumChildren = nif->getIndex( iBlock, "Num Children" );
			QModelIndex iChildren = nif->getIndex( iBlock, "Children" );
			arrayCollapser.numCollapser( nif, iNumChildren, iChildren );

			// remove empty property links
			QModelIndex iNumProperties = nif->getIndex( iBlock, "Num Properties" );
			QModelIndex iProperties = nif->getIndex( iBlock, "Properties" );
			arrayCollapser.numCollapser( nif, iNumProperties, iProperties );

			// remove empty extra data links
			QModelIndex iNumExtraData = nif->getIndex( iBlock, "Num Extra Data List" );
			QModelIndex iExtraData = nif->getIndex( iBlock, "Extra Data List" );
			arrayCollapser.numCollapser( nif, iNumExtraData, iExtraData );

			// remove empty modifier links (NiParticleSystem crashes Oblivion for those)
			QModelIndex iNumModifiers = nif->getIndex( iBlock, "Num Modifiers" );
			QModelIndex iModifiers = nif->getIndex( iBlock, "Modifiers" );
			arrayCollapser.numCollapser( nif, iNumModifiers, iModifiers );
		}

		return QModelIndex();
	}
};

REGISTER_SPELL( spSanitizeLinkArrays )

//! Fixes texture path names and options
class spAdjustTextureSources final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Adjust Texture Sources" ); }
	QString page() const override final { return Spell::tr( "Sanitize" ); }
	bool sanity() const override final { return true; }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		return nif && !index.isValid();
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & ) override final
	{
		for ( int i = 0; i < nif->getBlockCount(); i++ ) {
			QModelIndex iTexSrc = nif->getBlock( i, "NiSourceTexture" );

			if ( iTexSrc.isValid() ) {
				QModelIndex iFileName = nif->getIndex( iTexSrc, "File Name" );

				if ( iFileName.isValid() ) // adjust file path
					nif->set<QString>( iFileName, nif->get<QString>( iFileName ).replace( "/", "\\" ) );

				if ( nif->checkVersion( 0x14000005, 0x14000005 ) ) {
					// adjust format options (oblivion only)
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

//! Reorders blocks

bool spSanitizeBlockOrder::isApplicable( const NifModel *, const QModelIndex & index )
{
	// all files
	return !index.isValid();
}

// check whether the block is of a type that comes before the parent or not
bool spSanitizeBlockOrder::childBeforeParent( NifModel * nif, qint32 block )
{
	// get index to the block
	QModelIndex iBlock( nif->getBlock( block ) );
	// check its type
	return (
		nif->inherits( iBlock, "bhkRefObject" )
		&& !nif->inherits( iBlock, "bhkConstraint" )
	);
}

// build the nif tree at node block; the block itself and its children are recursively added to
// the newblocks list
void spSanitizeBlockOrder::addTree( NifModel * nif, qint32 block, QList<qint32> & newblocks )
{
	// is the block already added?
	if ( newblocks.contains( block ) )
		return;

	// special case: add bhkConstraint entities before bhkConstraint
	// (these are actually links, not refs)
	QModelIndex iBlock( nif->getBlock( block ) );

	if ( nif->inherits( iBlock, "bhkConstraint" ) ) {
		for ( const auto entity : nif->getLinkArray( iBlock, "Entities" ) ) {
			addTree( nif, entity, newblocks );
		}
	}


	// add all children of block that should be before block
	for ( const auto child : nif->getChildLinks( block ) ) {
		if ( childBeforeParent( nif, child ) )
			addTree( nif, child, newblocks ); // now add this child and all of its children
	}

	// add the block
	newblocks.append( block );
	// add all children of block that should be after block
	for ( const auto child : nif->getChildLinks( block ) ) {
		if ( !childBeforeParent( nif, child ) )
			addTree( nif, child, newblocks ); // now add this child and all of its children
	}
}

QModelIndex spSanitizeBlockOrder::cast( NifModel * nif, const QModelIndex & )
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
	for ( const auto rootblock : rootblocks )
	{
		addTree( nif, rootblock, newblocks );
	}

	// check whether all blocks have been added
	if ( nif->getBlockCount() != newblocks.size() ) {
		qCCritical( nsSpell ) << Spell::tr( "failed to sanitize blocks order, corrupt nif tree?" );
		return QModelIndex();
	}

	// invert mapping
	QVector<qint32> order( nif->getBlockCount() );

	for ( qint32 n = 0; n < newblocks.size(); n++ ) {
		order[newblocks[n]] = n;
		//qDebug() << n << newblocks[n];
	}

	// reorder the blocks
	nif->reorderBlocks( order );

	return QModelIndex();
}

REGISTER_SPELL( spSanitizeBlockOrder )

//! Checks that links are correct
class spSanityCheckLinks final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Check Links" ); }
	QString page() const override final { return Spell::tr( "Sanitize" ); }
	bool sanity() const override final { return true; }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		Q_UNUSED( nif );
		return !index.isValid();
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & ) override final
	{
		for ( int b = 0; b < nif->getBlockCount(); b++ ) {
			QModelIndex iBlock = nif->getBlock( b );
			QModelIndex idx = check( nif, iBlock );

			if ( idx.isValid() )
				return idx;
		}

		return QModelIndex();
	}

	QModelIndex check( NifModel * nif, const QModelIndex & iParent )
	{
		for ( int r = 0; r < nif->rowCount( iParent ); r++ ) {
			QModelIndex idx = iParent.child( r, 0 );
			bool child;

			if ( nif->isLink( idx, &child ) ) {
				qint32 l = nif->getLink( idx );

				if ( l < 0 ) {
					/*
					if ( ! child )
					{
					    qDebug() << "unassigned parent link";
					    return idx;
					}
					*/
				} else if ( l >= nif->getBlockCount() ) {
					qCCritical( nsSpell ) << Spell::tr( "Invalid link '%1'." ).arg( QString::number(l) );
					return idx;
				} else {
					QString tmplt = nif->itemTmplt( idx );

					if ( !tmplt.isEmpty() ) {
						QModelIndex iBlock = nif->getBlock( l );

						if ( !nif->inherits( iBlock, tmplt ) ) {
							qCCritical( nsSpell ) << Spell::tr( "Link '%1' points to wrong block type." ).arg( QString::number(l) );
							return idx;
						}
					}
				}
			}

			if ( nif->rowCount( idx ) > 0 ) {
				QModelIndex x = check( nif, idx );

				if ( x.isValid() )
					return x;
			}
		}

		return QModelIndex();
	}
};

REGISTER_SPELL( spSanityCheckLinks )

//! Fixes invalid block names
class spFixInvalidNames final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Fix Invalid Block Names" ); }
	QString page() const override final { return Spell::tr( "Sanitize" ); }
	bool sanity() const override final { return true; }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		return nif && nif->getIndex( nif->getHeader(), "Num Strings" ).isValid() && !index.isValid();
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & ) override final
	{
		QVector<QString> stringsToAdd;
		QVector<QString> shapeNames;
		QMap<QModelIndex, QString> modifiedBlocks;

		auto iHeader = nif->getHeader();
		auto numStrings = nif->get<int>( iHeader, "Num Strings" );
		auto strings = nif->getArray<QString>( iHeader, "Strings" );

		// Provides a string index for the desired string
		auto rename = [&strings, &stringsToAdd, numStrings] ( int & newIdx, const QString & str ) {
			newIdx = strings.indexOf( str );
			if ( newIdx < 0 ) {
				bool inNew = stringsToAdd.contains( str );
				newIdx = (inNew) ? stringsToAdd.indexOf( str ) : stringsToAdd.count();
				newIdx += numStrings;

				if ( !inNew )
					stringsToAdd << str;
			}
		};

		// Provides a block name using a base string and the block number,
		//	incrementing the number if necessary to avoid duplicate names
		auto autoRename = [&strings, &stringsToAdd, numStrings] ( int & newIdx, const QString & parentName, int blockNum ) {
			QString newName;
			int j = 0;
			do {
				newName = QString( "%1:%2" ).arg( parentName ).arg( blockNum + j );
				j++;
			} while ( strings.contains( newName ) || stringsToAdd.contains( newName ) );

			newIdx = numStrings + stringsToAdd.count();
			stringsToAdd << newName;
		};

		for ( int i = 0; i < nif->getBlockCount(); i++ ) {
			QModelIndex iBlock = nif->getBlock( i );
			if ( !(nif->inherits( iBlock, "NiObjectNET" ) || nif->inherits( iBlock, "NiExtraData" )) )
				continue;

			auto nameIdx = nif->get<int>( iBlock, "Name" );
			auto nameString = nif->get<QString>( iBlock, "Name" );

			// Ignore Editor Markers and AddOnNodes
			if ( nameString.contains( "EditorMarker" ) || nif->inherits( iBlock, "BSValueNode" ) )
				continue;

			QModelIndex iBlockParent = nif->getBlock( nif->getParent( i ) );
			auto parentNameString = nif->get<QString>( iBlockParent, "Name" );
			if ( !iBlockParent.isValid() ) {
				parentNameString = nif->getFilename();
			}

			int newIdx = -1;

			bool isOutOfBounds = nameIdx >= numStrings;
			bool isProp = nif->isNiBlock( iBlock, 
										{ "BSLightingShaderProperty", "BSEffectShaderProperty", "NiAlphaProperty" } );
			bool isNiAV = nif->inherits( iBlock, "NiAVObject" );

			// Fix 'BSX' strings
			if ( nif->isNiBlock( iBlock, "BSXFlags" ) ) {
				if ( nameString == "BSX" )
					continue;
				rename( newIdx, "BSX" );
			} else if ( nameString == "BSX" ) {
				if ( isProp )
					rename( newIdx,"" );
				else
					autoRename( newIdx, parentNameString, i );
			}

			// Fix duplicate shape names
			if ( isNiAV && !isOutOfBounds ) {
				if ( shapeNames.contains( nameString ) )
					autoRename( newIdx, parentNameString, i );

				shapeNames << nameString;
			}

			// Fix "Wet Material" field
			if ( isProp && nif->getIndex( iBlock, "Wet Material" ).isValid() ) {
				auto wetIdx = nif->get<int>( iBlock, "Wet Material" );
				auto wetString = nif->get<QString>( iBlock, "Wet Material" );

				int newWetIdx = -1;

				bool invalidString = !wetString.isEmpty() && !wetString.endsWith( ".bgsm", Qt::CaseInsensitive );
				if ( wetIdx >= numStrings || invalidString ) {
					rename( newWetIdx, "" );
				}

				if ( newWetIdx > -1 ) {
					nif->set<int>( iBlock, "Wet Material", newWetIdx );
					modifiedBlocks.insert( nif->getIndex( iBlock, "Wet Material" ), "Wet Material" );
				}
			}

			// Fix "Name" field
			if ( isOutOfBounds ) {
				// Fix out of bounds string indices
				// Rename scene objects, or blank out property names
				if ( !isProp )
					autoRename( newIdx, parentNameString, i );
				else
					rename( newIdx, "" );
			} else if ( isProp ) {
				if ( nameString.isEmpty() )
					continue;

				// Fix invalid property names
				if ( nif->isNiBlock( iBlock, "NiAlphaProperty" ) ) {
					rename( newIdx, "" );
				} else {
					auto ci = Qt::CaseInsensitive;
					if ( !(nameString.endsWith( ".bgsm", ci ) || nameString.endsWith( ".bgem", ci )) ) {
						rename( newIdx, "" );
					}
				}
			}

			if ( newIdx > -1 ) {
				nif->set<int>( iBlock, "Name", newIdx );
				modifiedBlocks.insert( nif->getIndex( iBlock, "Name" ), "Name" );
			}
		}

		if ( modifiedBlocks.count() < 1 )
			return QModelIndex();

		// Append new strings to header strings
		strings << stringsToAdd;

		// Update header
		nif->set<int>( iHeader, "Num Strings", strings.count() );
		nif->updateArray( iHeader, "Strings" );
		nif->setArray<QString>( iHeader, "Strings", strings );
		
		nif->updateHeader();

		for ( const auto & b : modifiedBlocks.toStdMap() ) {
			auto blockName = b.first.parent().data( Qt::DisplayRole ).toString();

			Message::append( Spell::tr( "One or more blocks have had their Name sanitized." ), 
							 QString( "%1 (%2) = '%3'" ).arg( blockName ).arg( b.second ).arg( nif->get<QString>( b.first ) )
			);
		}

		return QModelIndex();
	}
};

REGISTER_SPELL( spFixInvalidNames )

//! Fills blank "Controller Type" refs in NiControllerSequence
class spFillBlankControllerTypes final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Fill Blank NiControllerSequence Types" ); }
	QString page() const override final { return Spell::tr( "Sanitize" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		return nif && nif->getIndex( nif->getHeader(), "Num Strings" ).isValid() && !index.isValid();
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & ) override final
	{
		QVector<QString> modifiedNames;

		auto iHeader = nif->getHeader();
		auto numStrings = nif->get<int>( iHeader, "Num Strings" );
		auto strings = nif->getArray<QString>( iHeader, "Strings" );

		bool ok = true;
		QString str = QInputDialog::getText( 0, Spell::tr( "Fill Blank NiControllerSequence Types" ),
											   Spell::tr( "Choose the default Controller Type" ), 
											   QLineEdit::Normal, "NiTransformController", &ok );

		if ( !ok )
			return QModelIndex();

		auto stringIdx = strings.indexOf( str );
		if ( stringIdx == -1 ) {
			// Append new strings to header strings
			strings << str;
			stringIdx = numStrings;
		}

		for ( int i = 0; i < nif->getBlockCount(); i++ ) {
			QModelIndex iBlock = nif->getBlock( i );
			if ( !(nif->inherits( iBlock, "NiControllerSequence" )) )
				continue;

			auto controlledBlocks = nif->getIndex( iBlock, "Controlled Blocks" );
			auto numBlocks = nif->rowCount( controlledBlocks );

			for ( int i = 0; i < numBlocks; i++ ) {
				auto ctrlrType =  nif->getIndex( controlledBlocks.child( i, 0 ), "Controller Type" );
				auto nodeName = nif->getIndex( controlledBlocks.child( i, 0 ), "Node Name" );

				auto ctrlrTypeIdx = nif->get<int>( ctrlrType );
				if ( ctrlrTypeIdx == -1 ) {
					nif->set<int>( ctrlrType, stringIdx );
					modifiedNames << nif->get<QString>( nodeName );
				}
			}
		}

		// Update header
		nif->set<int>( iHeader, "Num Strings", strings.count() );
		nif->updateArray( iHeader, "Strings" );
		nif->setArray<QString>( iHeader, "Strings", strings );

		nif->updateHeader();

		for ( const QString& s : modifiedNames ) {
			Message::append( Spell::tr( "One or more NiControllerSequence rows have been sanitized" ),
							 QString( "%1" ).arg( s ) );
		}

		return QModelIndex();
	}
};

REGISTER_SPELL( spFillBlankControllerTypes )
