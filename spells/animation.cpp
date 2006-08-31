#include "../spellbook.h"

#include <QDebug>
#include <QFileDialog>

class spAttachKf : public Spell
{
public:
	QString name() const { return "Attach .KF"; }
	QString page() const { return "Animation"; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return nif && ! index.isValid();
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		try
		{
			QString kfname = QFileDialog::getOpenFileName( 0, "Choose a .kf file", nif->getFolder(), "*.kf" );
			
			if ( kfname.isEmpty() )
				return index;
			
			NifModel kf;
			
			QFile kffile( kfname );
			if ( ! kffile.open( QFile::ReadOnly ) )
				throw QString( "failed to open .kf %1" ).arg( kfname );
			
			if ( ! kf.load( kffile ) )
				throw QString( "failed to load .kf from file %1" ).arg( kfname );
			
			if ( kf.getRootLinks().count() != 1 )
				throw QString( "this is not a normal .kf file; there should be only one root block" );
			
			QModelIndex iSeq;
			
			foreach ( qint32 l, kf.getRootLinks() )
			{
				QModelIndex idx = kf.getBlock( l, "NiControllerSequence" );
				if ( idx.isValid() )
				{
					iSeq = idx;
					break;
				}
			}
			
			if ( ! iSeq.isValid() )
				throw QString( "found no NiControllerSequence in the .kf file" );
			
			QString rootName = kf.get<QString>( iSeq, "Target Name" );
			QPersistentModelIndex iRoot = findRootTarget( nif, rootName );
			if ( ! iRoot.isValid() )
				throw QString( "couldn't find the animation's root node (%1)" ).arg( rootName );
			
			QStringList missingNodes;
			QList< QPersistentModelIndex > controlledNodes;
			
			QModelIndex iCtrlBlcks = kf.getIndex( iSeq, "Controlled Blocks" );
			for ( int r = 0; r < kf.rowCount( iCtrlBlcks ); r++ )
			{
				QModelIndex iNodeName = kf.getIndex( iCtrlBlcks.child( r, 0 ), "Node Name Offset" );
				QString nodeName = iNodeName.sibling( iNodeName.row(), NifModel::ValueCol ).data( Qt::DisplayRole ).toString();
				
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
			
			if ( ! missingNodes.isEmpty() )
			{
				qWarning() << "The following nodes were not found in the nif:";
				foreach ( QString nn, missingNodes )
					qWarning() << nn;
			}
			
			if ( controlledNodes.isEmpty() )
			{
				throw QString( "actually none of the nodes were found in the nif" );
			}
			
			QPersistentModelIndex iMultiTransformer = findController( nif, iRoot, "NiMultiTargetTransformController" );
			if ( ! iMultiTransformer.isValid() )
				iMultiTransformer = attachController( nif, iRoot, "NiMultiTargetTransformController" );
			
			setLinkArray( nif, iMultiTransformer, "Extra Targets", controlledNodes );
			
			QPersistentModelIndex iCtrlManager = findController( nif, iRoot, "NiControllerManager" );
			if ( ! iCtrlManager.isValid() )
				iCtrlManager = attachController( nif, iRoot, "NiControllerManager" );
			
			QPersistentModelIndex iObjPalette = nif->getBlock( nif->getLink( iCtrlManager, "Object Palette" ), "NiDefaultAVObjectPalette" );
			if ( ! iObjPalette.isValid() )
			{
				iObjPalette = nif->insertNiBlock( "NiDefaultAVObjectPalette", nif->getBlockNumber( iCtrlManager ) + 1 );
				nif->setLink( iCtrlManager, "Object Palette", nif->getBlockNumber( iObjPalette ) );
			}
			
			setNameLinkArray( nif, iObjPalette, "Objs", controlledNodes );
			
			qint32 lSeq = kf.getBlockNumber( iSeq );
			
			QMap<qint32,qint32> map = kf.moveAllNiBlocks( nif );
			
			qint32 nSeq = map.value( lSeq );
			int numSeq = nif->get<int>( iCtrlManager, "Num Controller Sequences" );
			nif->set<int>( iCtrlManager, "Num Controller Sequences", numSeq+1 );
			nif->updateArray( iCtrlManager, "Controller Sequences" );
			nif->setLink( nif->getIndex( iCtrlManager, "Controller Sequences" ).child( numSeq, 0 ), nSeq );
			
			return iRoot;
		}
		catch ( QString e )
		{
			qWarning( e.toAscii() );
		}
		return index;
	}
	
	static QModelIndex findChildNode( const NifModel * nif, const QModelIndex & parent, const QString & name )
	{
		if ( ! nif->inherits( parent, "NiAVObject" ) )
			return QModelIndex();
		
		if ( nif->get<QString>( parent, "Name" ) == name )
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
	
	static QModelIndex attachController( NifModel * nif, const QPersistentModelIndex & iNode, const QString & ctrltype )
	{
		QModelIndex iCtrl = nif->insertNiBlock( ctrltype, nif->getBlockNumber( iNode ) + 1 );
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
			throw QString( "array %1 not found" ).arg( array );
		
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
			throw QString( "array %1 not found" ).arg( array );
		
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
