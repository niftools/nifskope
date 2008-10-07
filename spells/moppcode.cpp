#if defined(_WIN32) || defined(WIN32)

// This code is only intended to be run with Win32 platform.

#include "../spellbook.h"

#include <QDebug>


extern "C" void * __stdcall LoadLibraryA( const char * lpModuleName );
extern "C" void * __stdcall GetProcAddress ( void * hModule, const char * lpProcName);
extern "C" void __stdcall FreeLibrary( void * lpModule );

class HavokMoppCode
{
private:
	typedef int (__stdcall * fnGenerateMoppCode)(int nVerts, Vector3 const* verts, int nTris, Triangle const *tris);
   typedef int (__stdcall * fnGenerateMoppCodeWithSubshapes)(int nShapes, int const *shapes, int nVerts, Vector3 const* verts, int nTris, Triangle const *tris);
	typedef int (__stdcall * fnRetrieveMoppCode)(int nBuffer, char *buffer);
	typedef int (__stdcall * fnRetrieveMoppScale)(float *value);
	typedef int (__stdcall * fnRetrieveMoppOrigin)(Vector3 *value);

	void *hMoppLib;
	fnGenerateMoppCode GenerateMoppCode;
	fnRetrieveMoppCode RetrieveMoppCode;
	fnRetrieveMoppScale RetrieveMoppScale;
	fnRetrieveMoppOrigin RetrieveMoppOrigin;
   fnGenerateMoppCodeWithSubshapes GenerateMoppCodeWithSubshapes;

public:
   HavokMoppCode() : hMoppLib(0), GenerateMoppCode(0), RetrieveMoppCode(0)
      , RetrieveMoppScale(0), RetrieveMoppOrigin(0), GenerateMoppCodeWithSubshapes(0)
   {
   }

	~HavokMoppCode() {
		if (hMoppLib) FreeLibrary(hMoppLib);
	}

	bool Initialize()
	{
		if (hMoppLib == NULL)
		{
			hMoppLib = LoadLibraryA( "NifMopp.dll" );
			GenerateMoppCode = (fnGenerateMoppCode)GetProcAddress( hMoppLib, "GenerateMoppCode" );
			RetrieveMoppCode = (fnRetrieveMoppCode)GetProcAddress( hMoppLib, "RetrieveMoppCode" );
			RetrieveMoppScale = (fnRetrieveMoppScale)GetProcAddress( hMoppLib, "RetrieveMoppScale" );
			RetrieveMoppOrigin = (fnRetrieveMoppOrigin)GetProcAddress( hMoppLib, "RetrieveMoppOrigin" );
         GenerateMoppCodeWithSubshapes = (fnGenerateMoppCodeWithSubshapes)GetProcAddress( hMoppLib, "GenerateMoppCodeWithSubshapes" );
		}
		return ( NULL != GenerateMoppCode  && NULL != RetrieveMoppCode 
			&& NULL != RetrieveMoppScale && NULL != RetrieveMoppOrigin
			);
	}

	QByteArray CalculateMoppCode( QVector<Vector3> const & verts, QVector<Triangle> const & tris, Vector3* origin, float* scale)
	{
		QByteArray code;
		if ( Initialize() )
		{
			int len = GenerateMoppCode( verts.size(), &verts[0], tris.size(), &tris[0] );
			if ( len > 0 )
			{
				code.resize( len );
				if ( 0 != RetrieveMoppCode( len , code.data() ) )
				{
					if ( NULL != scale )
						RetrieveMoppScale(scale);
					if ( NULL != origin )
						RetrieveMoppOrigin(origin);
				}
				else
				{
					code.clear();
				}
			}
		}
		return code;
	}

   QByteArray CalculateMoppCode( QVector<int> const & subShapesVerts
                               , QVector<Vector3> const & verts
                               , QVector<Triangle> const & tris
                               , Vector3* origin, float* scale)
   {
      QByteArray code;
      if ( Initialize() )
      {
         int len;
         if (GenerateMoppCodeWithSubshapes != NULL)
            len = GenerateMoppCodeWithSubshapes( subShapesVerts.size(), &subShapesVerts[0], verts.size(), &verts[0], tris.size(), &tris[0] );
         else
            len = GenerateMoppCode( verts.size(), &verts[0], tris.size(), &tris[0] );
         if ( len > 0 )
         {
            code.resize( len );
            if ( 0 != RetrieveMoppCode( len , code.data() ) )
            {
               if ( NULL != scale )
                  RetrieveMoppScale(scale);
               if ( NULL != origin )
                  RetrieveMoppOrigin(origin);
            }
            else
            {
               code.clear();
            }
         }
      }
      return code;
   }
} TheHavokCode;



class spMoppCode : public Spell
{
public:
	QString name() const { return Spell::tr("Update MOPP Code"); }
	QString page() const { return Spell::tr("Code"); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index );
	QModelIndex cast( NifModel * nif, const QModelIndex & iBlock );
};

bool spMoppCode::isApplicable( const NifModel * nif, const QModelIndex & index )
{
	if ( TheHavokCode.Initialize() )
	{
		QModelIndex iData = nif->getBlock( nif->getLink( index, "Data" ) );
		return nif->checkVersion( 0x14000004, 0x14000005 ) && nif->isNiBlock( index, "bhkMoppBvTreeShape" );
	}
	return false;
}

QModelIndex spMoppCode::cast( NifModel * nif, const QModelIndex & iBlock )
{
	if ( !TheHavokCode.Initialize() ) {
		qWarning() << Spell::tr( "unable to locate the NifMopp.dll library" );
		return iBlock;
	}

	QPersistentModelIndex ibhkMoppBvTreeShape = iBlock;
	
	QModelIndex ibhkPackedNiTriStripsShape = nif->getBlock( nif->getLink( ibhkMoppBvTreeShape, "Shape" ) );
	if ( !nif->isNiBlock( ibhkPackedNiTriStripsShape, "bhkPackedNiTriStripsShape" ))
	{
		qWarning() << Spell::tr( "only bhkPackedNiTriStripsShape can be used with bhkMoppBvTreeShape Mopp code at this time" );
		return iBlock;
	}

   QVector<int> subshapeVerts;
   int nSubShapes = nif->get<int>( ibhkPackedNiTriStripsShape, "Num Sub Shapes" );
   QModelIndex ihkSubShapes = nif->getIndex( ibhkPackedNiTriStripsShape, "Sub Shapes" );
   subshapeVerts.resize(nSubShapes);
   for ( int t = 0; t < nSubShapes; t++ ) {
      subshapeVerts[t] = nif->get<int>( ihkSubShapes.child( t, 0 ), "Num Vertices" );
   }

	QModelIndex ihkPackedNiTriStripsData = nif->getBlock( nif->getLink( ibhkPackedNiTriStripsShape, "Data" ) );
	QVector<Vector3> verts = nif->getArray<Vector3>( ihkPackedNiTriStripsData, "Vertices" );
	QVector<Triangle> triangles;
	
	int nTriangles = nif->get<int>( ihkPackedNiTriStripsData, "Num Triangles" );
	QModelIndex iTriangles = nif->getIndex( ihkPackedNiTriStripsData, "Triangles" );
	triangles.resize(nTriangles);
	for ( int t = 0; t < nTriangles; t++ ) {
		triangles[t] = nif->get<Triangle>( iTriangles.child( t, 0 ), "Triangle" );
	}

	if ( verts.isEmpty() || triangles.isEmpty() )
	{
		qWarning() << Spell::tr( "need vertices and faces to calculate mopp code" );
		return iBlock;
	}

	Vector3 origin;
	float scale;
	QByteArray moppcode = TheHavokCode.CalculateMoppCode(subshapeVerts, verts, triangles, &origin, &scale);

	if (moppcode.size() == 0)
	{
		qWarning() << Spell::tr( "failed to generate mopp code" );
	}
	else
	{
		QModelIndex iCodeOrigin = nif->getIndex( ibhkMoppBvTreeShape, "Origin" );
		nif->set<Vector3>( iCodeOrigin, origin );
	    
		QModelIndex iCodeScale = nif->getIndex( ibhkMoppBvTreeShape, "Scale" );
		nif->set<float>( iCodeScale, scale );
	    
		QModelIndex iCodeSize = nif->getIndex( ibhkMoppBvTreeShape, "MOPP Data Size" );
		QModelIndex iCode = nif->getIndex( ibhkMoppBvTreeShape, "MOPP Data" );
		if ( iCodeSize.isValid() && iCode.isValid() )
		{
			nif->set<int>( iCodeSize, moppcode.size() );
			nif->updateArray( iCode );
	        //nif->set<QByteArray>( iCode, moppcode );
			for ( int i=0; i<moppcode.size(); ++i ){
				nif->set<quint8>( iCode.child( i, 0 ), moppcode[i] );
			}
		}
	}
    
    return iBlock;
}

REGISTER_SPELL( spMoppCode )

class spAllMoppCodes : public Spell
{
public:
	QString name() const { return Spell::tr( "Update All MOPP Code" ); }
	QString page() const { return Spell::tr( "Batch" ); }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & idx )
	{
    	if ( TheHavokCode.Initialize() )
    	{
            return nif && nif->checkVersion( 0x14000004, 0x14000005 ) && ! idx.isValid();
        }
        return false;
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & )
	{
		QList< QPersistentModelIndex > indices;
		
		spMoppCode TSpacer;
		
		for ( int n = 0; n < nif->getBlockCount(); n++ )
		{
			QModelIndex idx = nif->getBlock( n );
			if ( TSpacer.isApplicable( nif, idx ) )
				indices << idx;
		}
		
		foreach ( QModelIndex idx, indices )
		{
			TSpacer.castIfApplicable( nif, idx );
		}
		
		return QModelIndex();
	}
};

REGISTER_SPELL( spAllMoppCodes )

#endif // _MSC_VER
