#include "../spellbook.h"

#include "../glscene.h"

#include <QBuffer>
#include <QDebug>

#define SKEL_DAT ":/res/spells/skel.dat"

QDataStream & operator<<( QDataStream & ds, const Transform & t )
{
	for ( int x = 0; x < 3; x++ )
	{
		for ( int y = 0; y < 3; y++ )
			ds << t.rotation( x, y );
		ds << t.translation[ x ];
	}
	ds << t.scale;
	return ds;
}

QDataStream & operator>>( QDataStream & ds, Transform & t )
{
	for ( int x = 0; x < 3; x++ )
	{
		for ( int y = 0; y < 3; y++ )
			ds >> t.rotation( x, y );
		ds >> t.translation[ x ];
	}
	ds >> t.scale;
	return ds;
}

QDebug operator<<( QDebug dbg, const Vector3 & v )
{
	return dbg << v[0] << v[1] << v[2];
}

typedef QMap<QString,Transform> TransMap;

class spFixSkeleton : public Spell
{
public:
	QString name() const { return "Fix Bip01"; }
	QString page() const { return "Skeleton"; }
	
	bool isApplicable( NifModel * nif, const QModelIndex & index )
	{
		return ( nif->getVersion() == "4.0.0.2" && nif->itemType( index ) == "NiBlock" && nif->get<QString>( index, "Name" ) == "Bip01" ); //&& QFile::exists( SKEL_DAT ) );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		QFile file( SKEL_DAT );
		if ( file.open( QIODevice::ReadOnly ) )
		{
			QDataStream stream( &file );
			
			TransMap local;
			TransMap world;
			QString name;
			do
			{
				stream >> name;
				if ( !name.isEmpty() )
				{
					Transform t;
					stream >> t;
					local.insert( name, t );
					stream >> t;
					world.insert( name, t );
				}
			}
			while ( ! name.isEmpty() );
			
			TransMap bones;
			doBones( nif, index, Transform(), local, bones );
			
			foreach ( int link, nif->getChildLinks( nif->getBlockNumber( index ) ) )
			{
				QModelIndex iChild = nif->getBlock( link );
				if ( iChild.isValid() )
				{
					if ( nif->itemName( iChild ) == "NiNode" )
					{
						doNodes( nif, iChild, Transform(), world, bones );
					}
					else if ( nif->inherits( iChild, "AShape" ) )
					{
						doShape( nif, iChild, Transform(), world, bones );
					}
				}
			}
			
			nif->reset();
		}
		
		return index;
	}
	
	void doBones( NifModel * nif, const QModelIndex & index, const Transform & tparent, const TransMap & local, TransMap & bones )
	{
		QString name = nif->get<QString>( index, "Name" );
		if ( name.startsWith( "Bip01" ) )
		{
			Transform tlocal( nif, index );
			bones.insert( name, tparent * tlocal );
			
			local.value( name ).writeBack( nif, index );
			
			foreach ( int link, nif->getChildLinks( nif->getBlockNumber( index ) ) )
			{
				QModelIndex iChild = nif->getBlock( link, "NiNode" );
				if ( iChild.isValid() )
					doBones( nif, iChild, tparent * tlocal, local, bones );
			}
		}
	}
	
	bool doNodes( NifModel * nif, const QModelIndex & index, const Transform & tparent, const TransMap & world, const TransMap & bones )
	{
		bool hasSkinnedChildren = false;
		
		QString name = nif->get<QString>( index, "Name" );
		if ( ! name.startsWith( "Bip01" ) )
		{
			Transform tlocal( nif, index );
			
			foreach ( int link, nif->getChildLinks( nif->getBlockNumber( index ) ) )
			{
				QModelIndex iChild = nif->getBlock( link );
				if ( iChild.isValid() )
				{
					if ( nif->itemName( iChild ) == "NiNode" )
					{
						hasSkinnedChildren |= doNodes( nif, iChild, tparent * tlocal, world, bones );
					}
					else if ( nif->inherits( iChild, "AShape" ) )
					{
						hasSkinnedChildren |= doShape( nif, iChild, tparent * tlocal, world, bones );
					}
				}
			}
		}
		
		if ( hasSkinnedChildren )
		{
			Transform().writeBack( nif, index );
		}
		
		return hasSkinnedChildren;
	}
	bool doShape( NifModel * nif, const QModelIndex & index, const Transform & tparent, const TransMap & world, const TransMap & bones )
	{
		QModelIndex iSkinInstance = nif->getBlock( nif->getLink( index, "Skin Instance" ), "NiSkinInstance" );
		if ( ! iSkinInstance.isValid() )
			return false;
		QStringList names;
		QModelIndex iNames = nif->getIndex( iSkinInstance, "Bones" );
		if ( iNames.isValid() )
			iNames = nif->getIndex( iNames, "Bones" );
		if ( iNames.isValid() )
			for ( int n = 0; n < nif->rowCount( iNames ); n++ )
			{
				QModelIndex iBone = nif->getBlock( nif->itemLink( iNames.child( n, 0 ) ), "NiNode" );
				if ( iBone.isValid() )
					names.append( nif->get<QString>( iBone, "Name" ) );
				else
					names.append("");
			}
		QModelIndex iSkinData = nif->getBlock( nif->getLink( iSkinInstance, "Data" ), "NiSkinData" );
		if ( !iSkinData.isValid() )
			return false;
		QModelIndex iBones = nif->getIndex( iSkinData, "Bone List" );
		if ( ! iBones.isValid() )
			return false;
		
		Transform t( nif, iSkinData );
		t = tparent * t;
		t.writeBack( nif, iSkinData );
		
		for ( int b = 0; b < nif->rowCount( iBones ) && b < names.count(); b++ )
		{
			QModelIndex iBone = iBones.child( b, 0 );
			
			t = Transform( nif, iBone );
			
			t.rotation = world.value( names[ b ] ).rotation.inverted() * bones.value( names[ b ] ).rotation * t.rotation;
			t.translation = world.value( names[ b ] ).rotation.inverted() * bones.value( names[ b ] ).rotation * t.translation;
			
			t.writeBack( nif, iBone );
		}
		return true;
	}
};

REGISTER_SPELL( spFixSkeleton )


class spScanSkeleton : public Spell
{
public:
	QString name() const { return "Scan Bip01"; }
	QString page() const { return "Skeleton"; }
	
	bool isApplicable( NifModel * nif, const QModelIndex & index )
	{
		return ( nif->getVersion() == "4.0.0.2" && nif->itemType( index ) == "NiBlock" && nif->get<QString>( index, "Name" ) == "Bip01" );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		QFile file( SKEL_DAT );
		if ( file.open( QIODevice::WriteOnly ) )
		{
			QDataStream stream( &file );
			scan( nif, index, Transform(), stream );
			stream << QString();
		}
		return index;
	}
	
	void scan( NifModel * nif, const QModelIndex & index, const Transform & tparent, QDataStream & stream )
	{
		QString name = nif->get<QString>( index, "Name" );
		if ( name.startsWith( "Bip01" ) )
		{
			Transform local( nif, index );
			stream << name << local << tparent * local;
			qWarning() << name;
			foreach ( int link, nif->getChildLinks( nif->getBlockNumber( index ) ) )
			{
				QModelIndex iChild = nif->getBlock( link, "NiNode" );
				if ( iChild.isValid() )
					scan( nif, iChild, tparent * local, stream );
			}
		}
	}
};

//REGISTER_SPELL( spScanSkeleton )
