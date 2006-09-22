#include "../spellbook.h"

#include <QBuffer>
#include <QDebug>


class spCombiProps : public Spell
{
public:
	QString name() const { return "Combine Properties"; }
	QString page() const { return "Optimize"; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return nif && ! index.isValid();
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & )
	{
		QMap<qint32,QByteArray> props;
		QMap<qint32,qint32> map;
		do
		{
			props.clear();
			map.clear();
			
			for ( qint32 b = 0; b < nif->getBlockCount(); b++ )
			{
				QModelIndex iBlock = nif->getBlock( b );
				if ( nif->isNiBlock( iBlock, "NiMaterialProperty" ) )
					if ( nif->get<QString>( iBlock, "Name" ).contains( "Material" ) )
						nif->set<QString>( iBlock, "Name", "Material" );
				if ( nif->inherits( iBlock, "NiProperty" ) || nif->inherits( iBlock, "NiSourceTexture" ) )
				{
					QBuffer data;
					data.open( QBuffer::WriteOnly );
					data.write( nif->itemName( iBlock ).toAscii() );
					nif->save( data, iBlock );
					props.insert( b, data.buffer() );
				}
			}
			
			foreach ( qint32 x, props.keys() )
			{
				foreach ( qint32 y, props.keys() )
				{
					if ( x < y && ( ! map.contains( y ) ) && props[x].size() == props[y].size() )
					{
						int c = 0;
						while ( c < props[x].size() )
							if ( props[x][c] == props[y][c] )
								c++;
							else
								break;
						if ( c == props[x].size() )
							map.insert( y, x );
					}
				}
			}
			
			if ( ! map.isEmpty() )
			{
				qWarning() << "removing" << map.count() << "properties";
				nif->mapLinks( map );
				QList<qint32> l = map.keys();
				qSort( l.begin(), l.end(), qGreater<qint32>() );
				foreach ( qint32 b, l )
					nif->removeNiBlock( b );
			}
			
		} while ( ! map.isEmpty() );
		
		return QModelIndex();
	}
};

REGISTER_SPELL( spCombiProps )


class spUniqueProps : public Spell
{
public:
	QString name() const { return "Unique Properties"; }
	QString page() const { return "Optimize"; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return nif && ! index.isValid();
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		for ( int b = 0; b < nif->getBlockCount(); b++ )
		{
			QModelIndex iAVObj = nif->getBlock( b, "NiAVObject" );
			if ( iAVObj.isValid() )
			{
				QVector<qint32> props = nif->getLinkArray( iAVObj, "Properties" );
				QMutableVectorIterator<qint32> it( props );
				while ( it.hasNext() )
				{
					qint32 & l = it.next();
					QModelIndex iProp = nif->getBlock( l, "NiProperty" );
					if ( iProp.isValid() && nif->getParent( l ) != b )
					{
						QMap<qint32,qint32> map;
						if ( nif->isNiBlock( iProp, "NiTexturingProperty" ) )
						{
							foreach ( qint32 sl, nif->getChildLinks( nif->getBlockNumber( iProp ) ) )
							{
								QModelIndex iSrc = nif->getBlock( sl, "NiSourceTexture" );
								if ( iSrc.isValid() && ! map.contains( sl ) )
								{
									QModelIndex iSrc2 = nif->insertNiBlock( "NiSourceTexture", nif->getBlockCount() + 1 );
									QBuffer buffer;
									buffer.open( QBuffer::WriteOnly );
									nif->save( buffer, iSrc );
									buffer.close();
									buffer.open( QBuffer::ReadOnly );
									nif->load( buffer, iSrc2 );
									map[ sl ] = nif->getBlockNumber( iSrc2 );
								}
							}
						}
						
						QModelIndex iProp2 = nif->insertNiBlock( nif->itemName( iProp ), nif->getBlockCount() + 1 );
						QBuffer buffer;
						buffer.open( QBuffer::WriteOnly );
						nif->save( buffer, iProp );
						buffer.close();
						buffer.open( QBuffer::ReadOnly );
						nif->loadAndMapLinks( buffer, iProp2, map );
						l = nif->getBlockNumber( iProp2 );
					}
				}
				nif->setLinkArray( iAVObj, "Properties", props );
			}
		}
		return index;
	}
};

REGISTER_SPELL( spUniqueProps )


class spRemoveBogusNodes : public Spell
{
public:
	QString name() const { return "Remove Bogus Nodes"; }
	QString page() const { return "Optimize"; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return nif && ! index.isValid();
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		bool removed;
		int cnt = 0;
		do
		{
			removed = false;
			for ( int b = 0; b < nif->getBlockCount(); b++ )
			{
				QModelIndex iNode = nif->getBlock( b, "NiNode" );
				if ( iNode.isValid() )
				{
					if ( nif->getChildLinks( b ).isEmpty() && nif->getParentLinks( b ).isEmpty() )
					{
						int x = 0;
						for ( int c = 0; c < nif->getBlockCount(); c++ )
						{
							if ( c != b )
							{
								if ( nif->getChildLinks( c ).contains( b ) )
									x++;
								if ( nif->getParentLinks( c ).contains( b ) )
									x = 2;
								if ( x >= 2 )
									break;
							}
						}
						if ( x < 2 )
						{
							removed = true;
							cnt++;
							nif->removeNiBlock( b );
							break;
						}
					}
				}
			}
		} while ( removed );
		
		if ( cnt > 0 )
			qWarning() << "removed" << cnt << "ninodes";
		
		return QModelIndex();
	}
};

REGISTER_SPELL( spRemoveBogusNodes )

