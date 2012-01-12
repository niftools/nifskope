#include "../spellbook.h"

#include <QDebug>
#include <QFileDialog>

#include "../options.h"

// Brief description is deliberately not autolinked to class Spell
/*! \file animation.cpp
 * \brief Animation editing spells
 *
 * All classes here inherit from the Spell class.
 */

//! Attach a .KF to a .NIF
/*!
 * This only works for 10.0.1.0 onwards; prior to then a NiSequenceStreamHelper or NiSequence can be a root block.
 * The layout is different too; for 4.0.0.2 it appears that the chain of NiStringExtraData gives the names of
 * the block which the corresponding NiKeyframeController should attach to.
 * See Node (gl/glnode.cpp) for how controllers are handled
 */
class spAttachKf : public Spell
{
public:
	QString name() const { return Spell::tr("Attach .KF"); }
	QString page() const { return Spell::tr("Animation"); }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return nif && ! index.isValid();
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		if (nif == NULL)
			return index;

		QStringList kfnames = QFileDialog::getOpenFileNames( 0, Spell::tr("Choose .kf file(s)"), nif->getFolder(), "*.kf" );
		foreach( QString kfname, kfnames )
		{
			try
			{
				//QString kfname = QFileDialog::getOpenFileName( 0, Spell::tr("Choose a .kf file"), nif->getFolder(), "*.kf" );
				
				if ( kfname.isEmpty() )
					return index;
				
				NifModel kf;
				
				QFile kffile( kfname );
				if ( ! kffile.open( QFile::ReadOnly ) )
					throw QString( Spell::tr("failed to open .kf %1") ).arg( kfname );
				
				if ( ! kf.load( kffile ) )
					throw QString( Spell::tr("failed to load .kf from file %1") ).arg( kfname );
				
				QPersistentModelIndex iRoot;
				
				foreach ( qint32 l, kf.getRootLinks() )
				{
					QModelIndex iSeq = kf.getBlock( l, "NiControllerSequence" );
					if ( ! iSeq.isValid() )
						throw QString( Spell::tr("this is not a normal .kf file; there should be only NiControllerSequences as root blocks") );
					
					QString rootName = kf.get<QString>( iSeq, "Target Name" );
					if (rootName.isEmpty())
						rootName = kf.get<QString>( iSeq, "Text Keys Name" );// 10.0.1.0
					QModelIndex ir = findRootTarget( nif, rootName );
					
					if ( ! ir.isValid() )
						throw QString( Spell::tr("couldn't find the animation's root node (%1)") ).arg( rootName );
					
					if ( ! iRoot.isValid() )
						iRoot = ir;
					else if ( iRoot != ir )
						throw QString( Spell::tr("the animation root nodes differ; bailing out...") );
				}
				
				QPersistentModelIndex iMultiTransformer = findController( nif, iRoot, "NiMultiTargetTransformController" );
				QPersistentModelIndex iCtrlManager = findController( nif, iRoot, "NiControllerManager" );
				
				QList<qint32> seqLinks = kf.getRootLinks();
				QStringList missingNodes;
				
				foreach ( qint32 lSeq, kf.getRootLinks() )
				{
					QModelIndex iSeq = kf.getBlock( lSeq, "NiControllerSequence" );
					
					QList< QPersistentModelIndex > controlledNodes;
					
					QModelIndex iCtrlBlcks = kf.getIndex( iSeq, "Controlled Blocks" );
					for ( int r = 0; r < kf.rowCount( iCtrlBlcks ); r++ )
					{
						QString nodeName = kf.string( iCtrlBlcks.child( r, 0 ), "Node Name", false );
						if (nodeName.isEmpty())
							nodeName = kf.string( iCtrlBlcks.child( r, 0 ), "Target Name", false );// 10.0.1.0
						if (nodeName.isEmpty()) {
							QModelIndex iNodeName = kf.getIndex( iCtrlBlcks.child( r, 0 ), "Node Name Offset" );
							nodeName = iNodeName.sibling( iNodeName.row(), NifModel::ValueCol ).data( NifSkopeDisplayRole ).toString();
						}
						QModelIndex iCtrlNode = findChildNode( nif, iRoot, nodeName );
						if ( iCtrlNode.isValid() )
						{
							if ( ! controlledNodes.contains( iCtrlNode ) )
								controlledNodes.append( iCtrlNode );
						}
						else
						{
							if ( ! missingNodes.contains( nodeName ) )
								missingNodes << nodeName;
						}
					}

					bool oldHoldUpdates = nif->holdUpdates(true);

					if ( ! iMultiTransformer.isValid() )
						iMultiTransformer = attachController( nif, iRoot, "NiMultiTargetTransformController", true );
					if ( ! iCtrlManager.isValid() )
						iCtrlManager = attachController( nif, iRoot, "NiControllerManager", true );
					
					setLinkArray( nif, iMultiTransformer, "Extra Targets", controlledNodes );
					
					QPersistentModelIndex iObjPalette = nif->getBlock( nif->getLink( iCtrlManager, "Object Palette" ), "NiDefaultAVObjectPalette" );
					if ( ! iObjPalette.isValid() )
					{
						iObjPalette = nif->insertNiBlock( "NiDefaultAVObjectPalette", nif->getBlockNumber( iCtrlManager ) + 1, true );
						nif->setLink( iCtrlManager, "Object Palette", nif->getBlockNumber( iObjPalette ) );
					}
					
					setNameLinkArray( nif, iObjPalette, "Objs", controlledNodes );

					if (!oldHoldUpdates)
						nif->holdUpdates(false);
				}
				
				bool oldHoldUpdates = nif->holdUpdates(true);

				QMap<qint32,qint32> map = kf.moveAllNiBlocks( nif );
				int iMultiTransformerIdx = nif->getBlockNumber( iMultiTransformer );
				foreach ( qint32 lSeq, seqLinks )
				{
					qint32 nSeq = map.value( lSeq );
					int numSeq = nif->get<int>( iCtrlManager, "Num Controller Sequences" );
					nif->set<int>( iCtrlManager, "Num Controller Sequences", numSeq+1 );
					nif->updateArray( iCtrlManager, "Controller Sequences" );
					nif->setLink( nif->getIndex( iCtrlManager, "Controller Sequences" ).child( numSeq, 0 ), nSeq );
					QModelIndex iSeq = nif->getBlock( nSeq, "NiControllerSequence" );
					nif->setLink( iSeq, "Manager", nif->getBlockNumber( iCtrlManager ) );

					QModelIndex iCtrlBlcks = nif->getIndex( iSeq, "Controlled Blocks" );
					for ( int r = 0; r < nif->rowCount( iCtrlBlcks ); r++ )
					{
						QModelIndex iCtrlBlck = iCtrlBlcks.child( r, 0 );
						if ( nif->getLink( iCtrlBlck, "Controller" ) == -1 )
							nif->setLink( iCtrlBlck, "Controller", iMultiTransformerIdx );
					}
				}

				if (!oldHoldUpdates)
					nif->holdUpdates(false);

				if ( ! missingNodes.isEmpty() )
				{
					qWarning() << Spell::tr("The following controlled nodes were not found in the nif:");
					foreach ( QString nn, missingNodes )
						qWarning() << nn;
				}

				//return iRoot;
			}
			catch ( QString e )
			{
				qWarning( e.toAscii() );
			}
		}
		return index;
	}
	
	static QModelIndex findChildNode( const NifModel * nif, const QModelIndex & parent, const QString & name )
	{
		if ( ! nif->inherits( parent, "NiAVObject" ) )
			return QModelIndex();
		
		QString thisName = nif->get<QString>( parent, "Name" );
		if ( thisName == name )
			return parent;
		
		foreach ( qint32 l, nif->getChildLinks( nif->getBlockNumber( parent ) ) )
		{
			QModelIndex child = findChildNode( nif, nif->getBlock( l ), name );
			if ( child.isValid() )
				return child;
		}
		
		return QModelIndex();
	}
	
	static QModelIndex findRootTarget( const NifModel * nif, const QString & name )
	{
		foreach ( qint32 l, nif->getRootLinks() )
		{
			QModelIndex root = findChildNode( nif, nif->getBlock( l ), name );
			if ( root.isValid() )
				return root;
		}
		
		return QModelIndex();
	}
	
	static QModelIndex findController( const NifModel * nif, const QModelIndex & node, const QString & ctrltype )
	{
		foreach ( qint32 l, nif->getChildLinks( nif->getBlockNumber( node ) ) )
		{
			QModelIndex iCtrl = nif->getBlock( l, "NiTimeController" );
			if ( iCtrl.isValid() )
			{
				if ( nif->inherits( iCtrl, ctrltype ) )
					return iCtrl;
				else
				{
					iCtrl = findController( nif, iCtrl, ctrltype );
					if ( iCtrl.isValid() )
						return iCtrl;
				}
			}
		}
		return QModelIndex();
	}
	
	static QModelIndex attachController( NifModel * nif, const QPersistentModelIndex & iNode, const QString & ctrltype, bool fast = false )
	{
		QModelIndex iCtrl = nif->insertNiBlock( ctrltype, nif->getBlockNumber( iNode ) + 1, fast );
		if ( ! iCtrl.isValid() )
			return QModelIndex();
		
		qint32 oldctrl = nif->getLink( iNode, "Controller" );
		nif->setLink( iNode, "Controller", nif->getBlockNumber( iCtrl ) );
		nif->setLink( iCtrl, "Next Controller", oldctrl );
		nif->setLink( iCtrl, "Target", nif->getBlockNumber( iNode ) );
		nif->set<int>( iCtrl, "Flags", 8 );
		
		return iCtrl;
	}
	
	static void setLinkArray( NifModel * nif, const QModelIndex & iParent, const QString & array, const QList< QPersistentModelIndex > & iBlocks )
	{
		QModelIndex iNum = nif->getIndex( iParent, QString( "Num %1" ).arg( array ) );
		QModelIndex iArray = nif->getIndex( iParent, array );
		
		if ( ! iNum.isValid() || ! iArray.isValid() )
			throw QString( Spell::tr("array %1 not found") ).arg( array );
		
		QVector<qint32> links = nif->getLinkArray( iArray );
		
		foreach ( QModelIndex iBlock, iBlocks )
			if ( ! links.contains( nif->getBlockNumber( iBlock ) ) )
				links.append( nif->getBlockNumber( iBlock ) );
		
		nif->set<int>( iNum, links.count() );
		nif->updateArray( iArray );
		nif->setLinkArray( iArray, links );
	}
	
	static void setNameLinkArray( NifModel * nif, const QModelIndex & iParent, const QString & array, const QList< QPersistentModelIndex > & iBlocks )
	{
		QModelIndex iNum = nif->getIndex( iParent, QString( "Num %1" ).arg( array ) );
		QModelIndex iArray = nif->getIndex( iParent, array );
		
		if ( ! iNum.isValid() || ! iArray.isValid() )
			throw QString( Spell::tr("array %1 not found") ).arg( array );
		
		QList< QPersistentModelIndex > blocksToAdd;
		
		foreach ( QPersistentModelIndex idx, iBlocks )
		{
			QString name = nif->get<QString>( idx, "Name" );
			int r;
			for ( r = 0; r < nif->rowCount( iArray ); r++ )
			{
				if ( nif->get<QString>( iArray.child( r, 0 ), "Name" ) == name )
					break;
			}
			if ( r == nif->rowCount( iArray ) )
				blocksToAdd << idx;
		}
		
		int r = nif->get<int>( iNum );
		nif->set<int>( iNum, r + blocksToAdd.count() );
		nif->updateArray( iArray );
		foreach ( QPersistentModelIndex idx, blocksToAdd )
		{
			nif->set<QString>( iArray.child( r, 0 ), "Name", nif->get<QString>( idx, "Name" ) );
			nif->setLink( iArray.child( r, 0 ), "AV Object", nif->getBlockNumber( idx ) );
			r++;
		}
	}
};

REGISTER_SPELL( spAttachKf )

//! Convert quaternions to euler rotations.
/*!
 * There doesn't seem to be much use for this - most official meshes use
 * Quaternions, and a spell going the other way (ZYX to Quat) might be
 * of more use. Or, they can be converted in a modelling program.
 *
 * Also, since Quaternions can't store tangents (or can they?), quadratic
 * keys are out, leaving linear and tension-bias-continuity to be converted.
 */
class spConvertQuatsToEulers : public Spell
{
public:
	QString name() const { return Spell::tr("Convert Quat- to ZYX-Rotations"); }
	QString page() const { return Spell::tr("Animation"); }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		QModelIndex iBlock = nif->getBlock( index, "NiKeyframeData" );
		return iBlock.isValid() && nif->get<int>( iBlock, "Rotation Type" ) != 4;
	}
	
	/*
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		QModelIndex iQuats = nif->getIndex( index, "Quaternion Keys" );
		int rotationType = nif->get<int>( index, "Rotation Type" );
		nif->set<int>( index, "Rotation Type", 4 );
		nif->updateArray( index, "XYZ Rotations" );
		QModelIndex iRots = nif->getIndex( index, "XYZ Rotations" );

		for( int i = 0; i < 3; i++ )
		{
			QModelIndex iRot = iRots.child( i, 0 );
			nif->set<int>( iRot, "Num Keys", nif->get<int>(index, "Num Rotation Keys") );
			nif->set<int>( iRot, "Interpolation", rotationType );
			nif->updateArray( iRot, "Keys" );
		}

		for ( int q = 0; q < nif->rowCount( iQuats ); q++ )
		{
			QModelIndex iQuat = iQuats.child( q, 0 );
			
			float time = nif->get<float>( iQuat, "Time" );
			Quat value = nif->get<Quat>( iQuat, "Value" );

			Matrix tlocal;
			tlocal.fromQuat( value );
			
			float x, y, z;
			tlocal.toEuler( x, y, z );

			QModelIndex xRot = iRots.child( 0, 0 );
			QModelIndex yRot = iRots.child( 1, 0 );
			QModelIndex zRot = iRots.child( 2, 0 );
		
			xRot = nif->getIndex( xRot, "Keys" );

		}

		return index;
	}*/
};

//REGISTER_SPELL( spConvertQuatsToEulers )

