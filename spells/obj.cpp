#include "../spellbook.h"

#include "../NvTriStrip/qtwrapper.h"

#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QSettings>
#include <QTextStream>

class spObjExport : public Spell
{
public:
	QString name() const { return "Export .OBJ"; }
	QString page() const { return "Mesh"; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return getShapeData( nif, index ).isValid();
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		QModelIndex iData = getShapeData( nif, index );
		
		QSettings settings( "NifTools", "NifSkope" );
		settings.beginGroup( "spells" );
		settings.beginGroup( page() );
		settings.beginGroup( name() );
		
		// target file setup
		
		QString fname = QFileDialog::getSaveFileName( 0, "Choose a .OBJ file for export", settings.value( "File Name" ).toString(), "*.obj" );
		if ( fname.isEmpty() )
			return index;
		if ( ! fname.endsWith( ".obj", Qt::CaseInsensitive ) )
			fname.append( ".obj" );
		
		QFile file( fname );
		if ( ! file.open( QIODevice::WriteOnly ) )
		{
			qWarning() << "could not open " << file.fileName() << " for write access";
			return index;
		}
		
		QTextStream s( &file );
		
		// copy vertices
		
		QVector<Vector3> verts = nif->getArray<Vector3>( iData, "Vertices" );
		foreach ( Vector3 v, verts )
			s << "v " << v[0] << " " << v[1] << " " << v[2] << "\r\n";
		
		// copy texcoords
		
		QModelIndex iUV = nif->getIndex( iData, "UV Sets" );
		if ( ! iUV.isValid() )
			iUV = nif->getIndex( iData, "UV Sets 2" );
		
		QVector<Vector2> texco = nif->getArray<Vector2>( iUV.child( 0, 0 ) );
		foreach( Vector2 t, texco )
			s << "vt " << t[0] << " " << t[1] << "\r\n";
		
		// copy normals
		
		QVector<Vector3> norms = nif->getArray<Vector3>( iData, "Normals" );
		foreach ( Vector3 n, norms )
			s << "vn " << n[0] << " " << n[1] << " " << n[2] << "\r\n";
		
		// get the triangles
		
		QVector<Triangle> tris;
		
		QModelIndex iPoints = nif->getIndex( iData, "Points" );
		if ( iPoints.isValid() )
		{
			for ( int r = 0; r < nif->rowCount( iPoints ); r++ )
			{
				QVector<quint16> p = nif->getArray<quint16>( iPoints.child( r, 0 ) );
				quint16 a = p.value( 0 );
				quint16 b = p.value( 1 );
				bool flip = false;
				for ( int x = 2; x < p.count(); x++ )
				{
					quint16 c = p[x];
					if ( a != b && b != c && c != a )
						if ( flip )
							tris.append( Triangle( a, c, b ) );
						else
							tris.append( Triangle( a, b, c ) );
					a = b;
					b = c;
					flip = ! flip;
				}
			}
		}
		else
		{
			tris = nif->getArray<Triangle>( iData, "Triangles" );
		}
		
		// write the triangles
		
		foreach ( Triangle t, tris )
			s << "f " << t[0]+1 << "/" << t[0]+1 << "/" << t[0]+1
				<< " " << t[1]+1 << "/" << t[1]+1 << "/" << t[1]+1
				<< " " << t[2]+1 << "/" << t[2]+1 << "/" << t[2]+1 << "\r\n";
		
		// done
		
		file.close();
		settings.setValue( "File Name", file.fileName() );
		
		return index;
	}

	static QModelIndex getShapeData( const NifModel * nif, const QModelIndex & index )
	{
		if ( nif->isNiBlock( index, "NiTriShape" ) || nif->isNiBlock( index, "NiTriStrips" ) )
			return nif->getBlock( nif->getLink( index, "Data" ) );
		else if ( nif->isNiBlock( index, "NiTriShapeData" ) || nif->isNiBlock( index, "NiTriStripsData" ) )
			return index;
		return QModelIndex();
	}
	
};

REGISTER_SPELL( spObjExport )

class spObjImport : public Spell
{
public:
	QString name() const { return "Import .OBJ"; }
	QString page() const { return "Mesh"; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return spObjExport::getShapeData( nif, index ).isValid();
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & iBlock )
	{
		// read the file
		
		QSettings settings( "NifTools", "NifSkope" );
		settings.beginGroup( "spells" );
		settings.beginGroup( page() );
		settings.beginGroup( name() );
		
		QString fname = QFileDialog::getOpenFileName( 0, "Choose a .OBJ file to import", settings.value( "File Name" ).toString(), "*.obj" );
		if ( fname.isEmpty() )
			return iBlock;
		
		QFile file( fname );
		if ( ! file.open( QIODevice::ReadOnly ) )
		{
			qWarning() << "could not open " << file.fileName() << " for read access";
			return iBlock;
		}
		
		QTextStream s( & file );
		
		QVector<Vector3> overts;
		QVector<Vector3> onorms;
		QVector<Vector2> otexco;
		QHash<Point,quint16> indices;
		
		QVector<Vector3> verts( indices.count() );
		QVector<Vector3> norms( indices.count() );
		QVector<Vector2> texco( indices.count() );
		
		QVector<Triangle> triangles;
		
		quint16 index = 0;
		
		while ( ! s.atEnd() )
		{
			QString l = s.readLine();
			
			QStringList t = l.split( " ", QString::SkipEmptyParts );
			
			if ( t.value( 0 ) == "v" )
			{
				overts.append( Vector3( t.value( 1 ).toDouble(), t.value( 2 ).toDouble(), t.value( 3 ).toDouble() ) );
			}
			else if ( t.value( 0 ) == "vt" )
			{
				otexco.append( Vector2( t.value( 1 ).toDouble(), t.value( 2 ).toDouble() ) );
			}
			else if ( t.value( 0 ) == "vn" )
			{
				onorms.append( Vector3( t.value( 1 ).toDouble(), t.value( 2 ).toDouble(), t.value( 3 ).toDouble() ) );
			}
			else if ( t.value( 0 ) == "f" )
			{
				if ( triangles.count() == 0xffff )
				{
					qWarning() << "obj contains too many faces";
					return iBlock;
				}
				
				QList<Point> points;
				for ( int i = 1; i < t.count(); i++ )
					points.append( Point( t[i] ) );
				
				if ( points.count() < 3 )
					continue;
				
				//if ( points.count() > 3 )
				//{
				//	qWarning() << "please triangulate your mesh before import";
				//	return iBlock;
				//}
				
				Triangle tri;
				for ( int i = 0; i < 3; i++ )
				{
					if ( indices.contains( points[ i ] ) )
					{
						tri[ i ] = indices[ points[ i ] ];
					}
					else
					{
						if ( index == 0xffff )
						{
							qWarning() << "obj contains too many vertices";
							return iBlock;
						}
						
						verts.append( overts.value( points[ i ].v() ) );
						norms.append( onorms.value( points[ i ].n() ) );
						texco.append( otexco.value( points[ i ].t() ) );
						
						indices.insert( points[ i ], index );
						tri[ i ] = index++;
					}
				}
				
				triangles.append( tri );
			}
		}
		
		file.close();
		
		qWarning() << ".obj verts" << overts.count() << "norms" << onorms.count() << "texco" << otexco.count();
		qWarning() << ".nif verts/norms/texco" << verts.count() << "triangles" << triangles.count();
		
		QModelIndex iData = spObjExport::getShapeData( nif, iBlock );
		nif->set<int>( iData, "Num Vertices", indices.count() );
		nif->set<int>( iData, "Has Vertices", 1 );
		nif->updateArray( iData, "Vertices" );
		nif->setArray<Vector3>( iData, "Vertices", verts );
		nif->set<int>( iData, "Num UV Sets", 1 );
		nif->set<int>( iData, "Num UV Sets 2", 1 );
		nif->set<int>( iData, "Unknown Byte", 0 );
		nif->set<int>( iData, "Has Normals", onorms.count() ? 1 : 0 );
		nif->updateArray( iData, "Normals" );
		nif->setArray<Vector3>( iData, "Normals", norms );
		Vector3 center;
		foreach ( Vector3 v, verts )
			center += v;
		if ( verts.count() > 0 ) center /= verts.count();
		nif->set<Vector3>( iData, "Center", center );
		float radius = 0;
		foreach ( Vector3 v, verts )
		{
			float d = ( center - v ).length();
			if ( d > radius ) radius = d;
		}
		nif->set<float>( iData, "Radius", radius );
		nif->set<int>( iData, "Has Vertex Colors", 0 );
		nif->set<int>( iData, "Has UV Sets", 1 );
		QModelIndex iUV = nif->getIndex( iData, "UV Sets" );
		if ( ! iUV.isValid() )	iUV = nif->getIndex( iData, "UV Sets 2" );
		nif->updateArray( iUV );
		iUV = iUV.child( 0, 0 );
		nif->updateArray( iUV );
		nif->setArray<Vector2>( iUV, texco );
		nif->set<int>( iData, "Unknown Short 2", 0x4000 );
		
		if ( nif->itemName( iData ) == "NiTriStripsData" )
		{
			QList< QVector< quint16 > > strips = strippify( triangles );
			
			qWarning() << strips.count();
			
			nif->set<int>( iData, "Num Strips", strips.count() );
			nif->set<int>( iData, "Has Points", 1 );
			
			QModelIndex iLengths = nif->getIndex( iData, "Strip Lengths" );
			QModelIndex iPoints = nif->getIndex( iData, "Points" );
			
			if ( iLengths.isValid() && iPoints.isValid() )
			{
				nif->updateArray( iLengths );
				nif->updateArray( iPoints );
				int x = 0;
				int z = 0;
				foreach ( QVector<quint16> strip, strips )
				{
					nif->set<int>( iLengths.child( x, 0 ), strip.count() );
					QModelIndex iStrip = iPoints.child( x, 0 );
					nif->updateArray( iStrip );
					nif->setArray<quint16>( iStrip, strip );
					x++;
					z += strip.count() - 2;
				}
				nif->set<int>( iData, "Num Triangles", z );
			}
		}
		else // NiTriShapeData
		{
			nif->set<int>( iData, "Num Triangles", triangles.count() );
			nif->set<int>( iData, "Num Triangle Points", triangles.count() * 3 );
			nif->updateArray( iData, "Triangles" );
			nif->setArray<Triangle>( iData, "Triangles", triangles );
			nif->set<int>( iData, "Num Match Groups", 0 );
			nif->updateArray( iData, "Match Groups" );
		}
		
		settings.setValue( "File Name", fname );
		
		return iBlock;
	}
	
	class Point
	{
	public:
		Point( const QString & s )
		{
			QStringList l = s.split( "/" );
			v() = l.value( 0 ).toInt() - 1;
			t() = l.value( 1 ).toInt() - 1;
			n() = l.value( 2 ).toInt() - 1;
		}
		
		union
		{
			quint64 hash;
			
			struct
			{
				quint16 v, t, n;
			} y;
		} x;
		
		quint16 & v() { return x.y.v; }
		quint16 & t() { return x.y.t; }
		quint16 & n() { return x.y.n; }
		
		bool operator==( const Point & p ) const
		{
			return x.hash == p.x.hash;
		}
	};
};

quint64 qHash( const spObjImport::Point & p )
{
	return p.x.hash;
}

REGISTER_SPELL( spObjImport )
