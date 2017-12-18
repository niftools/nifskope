#include "spellbook.h"


// Brief description is deliberately not autolinked to class Spell
/*! \file moppcode.cpp
 * \brief Havok MOPP spells
 *
 * Note that this code only works on the Windows platform due an external
 * dependency on the Havok SDK, with which NifMopp.dll is compiled.
 *
 * Most classes here inherit from the Spell class.
 */

// Need to include headers before testing this
#ifdef Q_OS_WIN32

// This code is only intended to be run with Win32 platform.

extern "C" void * __stdcall SetDllDirectoryA( const char * lpPathName );
extern "C" void * __stdcall LoadLibraryA( const char * lpModuleName );
extern "C" void * __stdcall GetProcAddress ( void * hModule, const char * lpProcName );
extern "C" void __stdcall FreeLibrary( void * lpModule );

//! Interface to the external MOPP library
class HavokMoppCode
{
private:
	typedef int (__stdcall * fnGenerateMoppCode)( int nVerts, Vector3 const * verts, int nTris, Triangle const * tris );
	typedef int (__stdcall * fnGenerateMoppCodeWithSubshapes)( int nShapes, int const * shapes, int nVerts, Vector3 const * verts, int nTris, Triangle const * tris );
	typedef int (__stdcall * fnRetrieveMoppCode)( int nBuffer, char * buffer );
	typedef int (__stdcall * fnRetrieveMoppScale)( float * value );
	typedef int (__stdcall * fnRetrieveMoppOrigin)( Vector3 * value );

	void * hMoppLib;
	fnGenerateMoppCode GenerateMoppCode;
	fnRetrieveMoppCode RetrieveMoppCode;
	fnRetrieveMoppScale RetrieveMoppScale;
	fnRetrieveMoppOrigin RetrieveMoppOrigin;
	fnGenerateMoppCodeWithSubshapes GenerateMoppCodeWithSubshapes;

public:
	HavokMoppCode() : hMoppLib( 0 ), GenerateMoppCode( 0 ), RetrieveMoppCode( 0 ), RetrieveMoppScale( 0 ),
		  RetrieveMoppOrigin( 0 ), GenerateMoppCodeWithSubshapes( 0 )
	{
	}

	~HavokMoppCode()
	{
		if ( hMoppLib )
			FreeLibrary( hMoppLib );
	}

	bool Initialize()
	{
		if ( !hMoppLib ) {
			SetDllDirectoryA( QCoreApplication::applicationDirPath().toLocal8Bit().constData() );
			hMoppLib = LoadLibraryA( "NifMopp.dll" );
			GenerateMoppCode   = (fnGenerateMoppCode)GetProcAddress( hMoppLib, "GenerateMoppCode" );
			RetrieveMoppCode   = (fnRetrieveMoppCode)GetProcAddress( hMoppLib, "RetrieveMoppCode" );
			RetrieveMoppScale  = (fnRetrieveMoppScale)GetProcAddress( hMoppLib, "RetrieveMoppScale" );
			RetrieveMoppOrigin = (fnRetrieveMoppOrigin)GetProcAddress( hMoppLib, "RetrieveMoppOrigin" );
			GenerateMoppCodeWithSubshapes = (fnGenerateMoppCodeWithSubshapes)GetProcAddress( hMoppLib, "GenerateMoppCodeWithSubshapes" );
		}

		return (GenerateMoppCode && RetrieveMoppCode && RetrieveMoppScale && RetrieveMoppOrigin);
	}

	QByteArray CalculateMoppCode( QVector<Vector3> const & verts, QVector<Triangle> const & tris, Vector3 * origin, float * scale )
	{
		QByteArray code;

		if ( Initialize() ) {
			int len = GenerateMoppCode( verts.size(), &verts[0], tris.size(), &tris[0] );

			if ( len > 0 ) {
				code.resize( len );

				if ( 0 != RetrieveMoppCode( len, code.data() ) ) {
					if ( scale )
						RetrieveMoppScale( scale );

					if ( origin )
						RetrieveMoppOrigin( origin );
				} else {
					code.clear();
				}
			}
		}

		return code;
	}

	QByteArray CalculateMoppCode( QVector<int> const & subShapesVerts,
	                              QVector<Vector3> const & verts,
	                              QVector<Triangle> const & tris,
	                              Vector3 * origin, float * scale )
	{
		QByteArray code;

		if ( Initialize() ) {
			int len;

			if ( GenerateMoppCodeWithSubshapes )
				len = GenerateMoppCodeWithSubshapes( subShapesVerts.size(), &subShapesVerts[0], verts.size(), &verts[0], tris.size(), &tris[0] );
			else
				len = GenerateMoppCode( verts.size(), &verts[0], tris.size(), &tris[0] );

			if ( len > 0 ) {
				code.resize( len );

				if ( 0 != RetrieveMoppCode( len, code.data() ) ) {
					if ( scale )
						RetrieveMoppScale( scale );

					if ( origin )
						RetrieveMoppOrigin( origin );
				} else {
					code.clear();
				}
			}
		}

		return code;
	}
}
TheHavokCode;

//! Update Havok MOPP for a given shape
class spMoppCode final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Update MOPP Code" ); }
	QString page() const override final { return Spell::tr( "Havok" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		if ( nif->getUserVersion() != 10 && nif->getUserVersion() != 11 )
			return false;

		if ( TheHavokCode.Initialize() ) {
			//QModelIndex iData = nif->getBlock( nif->getLink( index, "Data" ) );

			if ( nif->isNiBlock( index, "bhkMoppBvTreeShape" ) ) {
				return ( nif->checkVersion( 0x14000004, 0x14000005 )
				         || nif->checkVersion( 0x14020007, 0x14020007 ) );
			}
		}

		return false;
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & iBlock ) override final
	{
		if ( !TheHavokCode.Initialize() ) {
			Message::critical( nullptr, Spell::tr( "Unable to locate NifMopp.dll" ) );
			return iBlock;
		}

		QPersistentModelIndex ibhkMoppBvTreeShape = iBlock;

		QModelIndex ibhkPackedNiTriStripsShape = nif->getBlock( nif->getLink( ibhkMoppBvTreeShape, "Shape" ) );

		if ( !nif->isNiBlock( ibhkPackedNiTriStripsShape, "bhkPackedNiTriStripsShape" ) ) {
			Message::warning( nullptr, Spell::tr( "Only bhkPackedNiTriStripsShape is supported at this time." ) );
			return iBlock;
		}

		QModelIndex ihkPackedNiTriStripsData = nif->getBlock( nif->getLink( ibhkPackedNiTriStripsShape, "Data" ) );

		if ( !nif->isNiBlock( ihkPackedNiTriStripsData, "hkPackedNiTriStripsData" ) )
			return iBlock;

		QVector<int> subshapeVerts;

		if ( nif->checkVersion( 0x14000004, 0x14000005 ) ) {
			int nSubShapes = nif->get<int>( ibhkPackedNiTriStripsShape, "Num Sub Shapes" );
			QModelIndex ihkSubShapes = nif->getIndex( ibhkPackedNiTriStripsShape, "Sub Shapes" );
			subshapeVerts.resize( nSubShapes );

			for ( int t = 0; t < nSubShapes; t++ ) {
				subshapeVerts[t] = nif->get<int>( ihkSubShapes.child( t, 0 ), "Num Vertices" );
			}
		} else if ( nif->checkVersion( 0x14020007, 0x14020007 ) ) {
			int nSubShapes = nif->get<int>( ihkPackedNiTriStripsData, "Num Sub Shapes" );
			QModelIndex ihkSubShapes = nif->getIndex( ihkPackedNiTriStripsData, "Sub Shapes" );
			subshapeVerts.resize( nSubShapes );

			for ( int t = 0; t < nSubShapes; t++ ) {
				subshapeVerts[t] = nif->get<int>( ihkSubShapes.child( t, 0 ), "Num Vertices" );
			}
		}

		QVector<Vector3> verts = nif->getArray<Vector3>( ihkPackedNiTriStripsData, "Vertices" );
		QVector<Triangle> triangles;

		int nTriangles = nif->get<int>( ihkPackedNiTriStripsData, "Num Triangles" );
		QModelIndex iTriangles = nif->getIndex( ihkPackedNiTriStripsData, "Triangles" );
		triangles.resize( nTriangles );

		for ( int t = 0; t < nTriangles; t++ ) {
			triangles[t] = nif->get<Triangle>( iTriangles.child( t, 0 ), "Triangle" );
		}

		if ( verts.isEmpty() || triangles.isEmpty() ) {
			Message::critical( nullptr, Spell::tr( "Insufficient data to calculate MOPP code" ),
				Spell::tr("Vertices: %1, Triangles: %2").arg( !verts.isEmpty() ).arg( !triangles.isEmpty() )
			);
			return iBlock;
		}

		Vector3 origin;
		float scale;
		QByteArray moppcode = TheHavokCode.CalculateMoppCode( subshapeVerts, verts, triangles, &origin, &scale );

		if ( moppcode.size() == 0 ) {
			Message::critical( nullptr, Spell::tr( "Failed to generate MOPP code" ) );
		} else {
			QModelIndex iCodeOrigin = nif->getIndex( ibhkMoppBvTreeShape, "Origin" );
			nif->set<Vector3>( iCodeOrigin, origin );

			QModelIndex iCodeScale = nif->getIndex( ibhkMoppBvTreeShape, "Scale" );
			nif->set<float>( iCodeScale, scale );

			QModelIndex iCodeSize = nif->getIndex( ibhkMoppBvTreeShape, "MOPP Data Size" );
			QModelIndex iCode = nif->getIndex( ibhkMoppBvTreeShape, "MOPP Data" ).child( 0, 0 );

			if ( iCodeSize.isValid() && iCode.isValid() ) {
				nif->set<int>( iCodeSize, moppcode.size() );
				nif->updateArray( iCode );
				nif->set<QByteArray>( iCode, moppcode );
			}
		}

		return iBlock;
	}
};

REGISTER_SPELL( spMoppCode )

//! Update MOPP code on all shapes in this model
class spAllMoppCodes final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Update All MOPP Code" ); }
	QString page() const override final { return Spell::tr( "Batch" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & idx ) override final
	{
		if ( nif && nif->getUserVersion() != 10 && nif->getUserVersion() != 11 )
			return false;

		if ( TheHavokCode.Initialize() ) {
			if ( nif && !idx.isValid() ) {
				return ( nif->checkVersion( 0x14000004, 0x14000005 )
				         || nif->checkVersion( 0x14020007, 0x14020007 ) );
			}
		}

		return false;
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & ) override final
	{
		QList<QPersistentModelIndex> indices;

		spMoppCode TSpacer;

		for ( int n = 0; n < nif->getBlockCount(); n++ ) {
			QModelIndex idx = nif->getBlock( n );

			if ( TSpacer.isApplicable( nif, idx ) )
				indices << idx;
		}

		for ( const QModelIndex& idx : indices ) {
			TSpacer.castIfApplicable( nif, idx );
		}

		return QModelIndex();
	}
};

REGISTER_SPELL( spAllMoppCodes )

#endif // Q_OS_WIN32

