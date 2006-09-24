#include "../spellbook.h"

#include <QDebug>

class spFlipTexCoords : public Spell
{
public:
	QString name() const { return "Flip UV"; }
	QString page() const { return "Mesh"; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return nif->itemType( index ).toLower() == "texcoord" || nif->inherits( index, "NiTriBasedGeomData" );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		QModelIndex idx = index;
		if ( nif->itemType( index ).toLower() != "texcoord" )
		{
			idx = nif->getIndex( nif->getBlock( index ), "UV Sets" );
		}
		QMenu menu;
		static const char * const flipCmds[3] = { "S = 1.0 - S", "T = 1.0 - T", "S <=> T" };
		for ( int c = 0; c < 3; c++ )
			menu.addAction( flipCmds[c] );
		QAction * act = menu.exec( QCursor::pos() );
		for ( int c = 0; c < 3; c++ )
			if ( act->text() == flipCmds[c] )
				flip( nif, idx, c );
		return index;
	}

	void flip( NifModel * nif, const QModelIndex & index, int f )
	{
		if ( nif->isArray( index ) )
		{
			QModelIndex idx = index.child( 0, 0 );
			if ( idx.isValid() )
			{
				if ( nif->isArray( idx ) )
					flip( nif, idx, f );
				else
				{
					QVector<Vector2> tc = nif->getArray<Vector2>( index );
					for ( int c = 0; c < tc.count(); c++ )
						flip( tc[c], f );
					nif->setArray<Vector2>( index, tc );
				}
			}
		}
		else
		{
			Vector2 v = nif->get<Vector2>( index );
			flip( v, f );
			nif->set<Vector2>( index, v );
		}
	}

	void flip( Vector2 & v, int f )
	{
		switch ( f )
		{
			case 0:
				v[0] = 1.0 - v[0];
				break;
			case 1:
				v[1] = 1.0 - v[1];
				break;
			default:
				{
					float x = v[0];
					v[0] = v[1];
					v[1] = x;
				}	break;
		}
	}
};

REGISTER_SPELL( spFlipTexCoords )


class spPruneRedundantTriangles : public Spell
{
public:
	QString name() const { return "Prune Redundant Triangles"; }
	QString page() const { return "Mesh"; }
	
	static QModelIndex getTriShapeData( const NifModel * nif, const QModelIndex & index )
	{
		QModelIndex iData = nif->getBlock( index );
		if ( nif->isNiBlock( index, "NiTriShape" ) )
			iData = nif->getBlock( nif->getLink( index, "Data" ) );
		if ( nif->isNiBlock( iData, "NiTriShapeData" ) )
			return iData;
		else return QModelIndex();
	}
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return getTriShapeData( nif, index ).isValid();
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		QModelIndex iData = getTriShapeData( nif, index );
		
		QList<Triangle> tris = nif->getArray<Triangle>( iData, "Triangles" ).toList();
		int cnt = 0;
		
		int i = 0;
		while ( i < tris.count() )
		{
			const Triangle & t = tris[i];
			if ( t[0] == t[1] || t[1] == t[2] || t[2] == t[0] )
			{
				tris.removeAt( i );
				cnt++;
			}
			else
				i++;
		}
		
		i = 0;
		while ( i < tris.count() )
		{
			const Triangle & t = tris[i];
			
			int j = i + 1;
			while ( j < tris.count() )
			{
				const Triangle & r = tris[j];
				
				if ( ( t[0] == r[0] && t[1] == r[1] && t[2] == r[2] )
					|| ( t[0] == r[1] && t[1] == r[2] && t[2] == r[0] )
					|| ( t[0] == r[2] && t[1] == r[0] && t[2] == r[1] ) )
				{
					tris.removeAt( j );
					cnt++;
				}
				else
					j++;
			}
			i++;
		}
		
		if ( cnt > 0 )
		{
			qWarning() << "removed" << cnt << "triangles";
			nif->set<int>( iData, "Num Triangles", tris.count() );
			nif->set<int>( iData, "Num Triangle Points", tris.count() * 3 );
			nif->updateArray( iData, "Triangles" );
			nif->setArray<Triangle>( iData, "Triangles", tris.toVector() );
		}
		return index;
	}
};

REGISTER_SPELL( spPruneRedundantTriangles )

