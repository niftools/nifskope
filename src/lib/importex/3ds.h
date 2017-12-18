#ifndef IMPORT_3DS_H
#define IMPORT_3DS_H

#include <QFile>
#include <QList>
#include <QMap>
#include <QMapIterator>
#include <QString>

// Chunk Type definitions
#define NULL_CHUNK          0x0000
#define M3D_VERSION         0x0002
#define M3D_KFVERSION       0x0003
#define COLOR_F             0x0010
#define COLOR_24            0x0011
#define LIN_COLOR_24        0x0012
#define LIN_COLOR_F         0x0013
#define INT_PERCENTAGE      0x0030
#define FLOAT_PERCENTAGE    0x0031
#define MASTER_SCALE        0x0100

#define DEFAULT_VIEW        0x3000
#define VIEW_TOP            0x3010
#define VIEW_BOTTOM         0x3020
#define VIEW_LEFT           0x3030
#define VIEW_RIGHT          0x3040
#define VIEW_FRONT          0x3050
#define VIEW_BACK           0x3060
#define VIEW_USER           0x3070
#define VIEW_CAMERA         0x3080
#define MDATA               0x3D3D
#define MESH_VERSION        0x3D3E

#define NAMED_OBJECT        0x4000
#define N_TRI_OBJECT        0x4100
#define POINT_ARRAY         0x4110
#define POINT_FLAG_ARRAY    0x4111
#define FACE_ARRAY          0x4120
#define MSH_MAT_GROUP       0x4130
#define TEX_VERTS           0x4140
#define SMOOTH_GROUP        0x4150
#define MESH_MATRIX         0x4160
#define MESH_COLOR          0x4165
#define M3DMAGIC            0x4D4D

#define MAT_NAME            0xA000
#define MAT_AMBIENT         0xA010
#define MAT_DIFFUSE         0xA020
#define MAT_SPECULAR        0xA030
#define MAT_SHININESS       0xA040
#define MAT_SHIN2PCT        0xA041
#define MAT_SHIN3PCT        0xA042
#define MAT_TRANSPARENCY    0xA050
#define MAT_XPFALL          0xA052
#define MAT_REFBLUR         0xA053
#define MAT_SELF_ILLUM      0xA080
#define MAT_TWOSIDE         0xA081
#define MAT_SELF_ILPCT      0xA084
#define MAT_WIRESIZE        0xA087
#define MAT_XPFALLIN        0xA08A
#define MAT_SHADING         0xA100
#define MAT_TEXMAP          0xA200
#define MAT_SPECMAP         0xA204
#define MAT_OPACMAP         0xA210
#define MAT_REFLMAP         0xA220
#define MAT_BUMPMAP         0xA230
#define MAT_MAPNAME         0xA300
#define MATERIAL            0xAFFF

#define KFDATA              0xB000
#define OBJECT_NODE_TAG     0xB002
#define KFSEG               0xB008
#define KFCURTIME           0xB009
#define KFHDR               0xB00A
#define NODE_HDR            0xB010
#define PIVOT               0xB013
#define POS_TRACK_TAG       0xB020
#define ROT_TRACK_TAG       0xB021
#define SCL_TRACK_TAG       0xB022
#define FOV_TRACK_TAG       0xB023
#define ROLL_TRACK_TAG      0xB024
#define COL_TRACK_TAG       0xB025
#define MORPH_TRACK_TAG     0xB026
#define HOT_TRACK_TAG       0xB027
#define FALL_TRACK_TAG      0xB028
#define HIDE_TRACK_TAG      0xB029
#define NODE_ID             0xB030

#define CHUNKHEADERSIZE     6
#define FILE_DUMMY          0xFFFF

#define FACE_FLAG_ONESIDE   0x0400

#ifdef _MSC_VER  /* Microsoft Visual C++ */
#pragma warning( disable : 4189 )  /* local variable is initialized but not referenced */
#endif


class Chunk
{
public:

	// general chunk properties

	typedef unsigned short ChunkType;
	typedef unsigned int ChunkPos;
	typedef unsigned long ChunkLength;
	typedef bool ChunkDataFlag;
	typedef unsigned int ChunkDataPos;
	typedef unsigned int ChunkDataLength;
	typedef unsigned short ChunkDataCount;

	struct ChunkHeader
	{
		ChunkType t;
		ChunkLength l;
	};

	// data structures for various chunks

	typedef float ChunkTypeFloat;

	struct ChunkTypeFloat2
	{
		ChunkTypeFloat x, y;
	};

	struct ChunkTypeFloat3
	{
		ChunkTypeFloat x, y, z;
	};

	struct ChunkTypeChar3
	{
		unsigned char x, y, z;
	};

	typedef long ChunkTypeLong;

	typedef short ChunkTypeShort;

	struct ChunkTypeShort3
	{
		ChunkTypeShort x, y, z;
	};

	struct ChunkTypeFaceArray
	{
		ChunkTypeShort vertex1, vertex2, vertex3;
		ChunkTypeShort flags;
	};

	struct ChunkTypeMeshMatrix
	{
		ChunkTypeShort matrix[4][3];
	};

	struct ChunkTypeKfPos
	{
		ChunkTypeShort framenum;
		ChunkTypeLong unknown;
		ChunkTypeFloat pos_x, pos_y, pos_z;
	};

	struct ChunkTypeKfRot
	{
		ChunkTypeShort framenum;
		ChunkTypeFloat rotation_rad;
		ChunkTypeFloat axis_x, axis_y, axis_z;
		ChunkTypeLong unknown;
	};

	struct ChunkTypeKfScl
	{
		ChunkTypeShort framenum;
		ChunkTypeLong unknown;
		ChunkTypeFloat scale_x, scale_y, scale_z;
	};

	// class members

	Chunk( QFile * _f, ChunkHeader _h, ChunkPos _p )
		: f( _f ), h( _h ), p( _p ), df( false ), dp( 0 ), dl( 0 ), dc( 0 )
	{
		if ( h.t == FILE_DUMMY ) {
			this->addchildren();
		} else {
			subproc();
		}
	}

	~Chunk()
	{
		qDeleteAll( c );
	}

	static Chunk * LoadFile( QFile * file )
	{
		file->seek( 0 );

		ChunkHeader hdr;
		hdr.t = FILE_DUMMY;
		hdr.l = file->size();

		Chunk * cnk = new Chunk( file, hdr, ( -CHUNKHEADERSIZE ) );

		return cnk;
	}

	ChunkType getType()
	{
		return h.t;
	}

	QList<Chunk *> getChildren( ChunkType ct = NULL_CHUNK )
	{
		if ( ct == NULL_CHUNK ) {
			return c.values();
		}

		return c.values( ct );
	}

	Chunk * getChild( ChunkType ct )
	{
		return c[ct];
	}

	void reset()
	{
		dp = 0;
	}

	template <class T>
	T read()
	{
		T r = T();

		if ( !df || dp > ( h.l - CHUNKHEADERSIZE ) ) {
			return r;
		}

		f->seek( p + CHUNKHEADERSIZE + dp );
		dp += f->read( (char *)&r, sizeof( r ) );

		return r;
	}

	QString readString()
	{
		QString s;
		char n;

		f->seek( p + CHUNKHEADERSIZE + dp );

		while ( true ) {
			dp += f->read( &n, sizeof( char ) );

			if ( n == 0 ) {
				break;
			}

			s.append( n );
		}

		return s;
	}

private:
	QFile * f;
	ChunkHeader h;
	ChunkPos p;
	ChunkDataFlag df;
	ChunkDataPos dp;
	ChunkDataLength dl;
	ChunkDataCount dc;

	QMap<ChunkType, Chunk *> c;

	void subproc()
	{
		switch ( h.t & 0xf000 ) {
		case 0x0000:
			switch ( h.t ) {
			case M3D_VERSION:
			case M3D_KFVERSION:
				adddata( sizeof( ChunkTypeShort ) );
				break;
			case COLOR_F:
			case LIN_COLOR_F:
				adddata( sizeof( ChunkTypeFloat3 ) );
				break;
			case COLOR_24:
			case LIN_COLOR_24:
				adddata();
				break;
			case INT_PERCENTAGE:
				adddata( sizeof( ChunkTypeShort ) );
				break;
			case FLOAT_PERCENTAGE:
				adddata( sizeof( ChunkTypeFloat ) );
				break;
			case MASTER_SCALE:
				adddata( sizeof( ChunkTypeFloat ) );
				break;
			}
			break;

		case 0x3000:
			switch ( h.t ) {
			case DEFAULT_VIEW:
				addchildren();
				break;
			case VIEW_TOP:
			case VIEW_BOTTOM:
			case VIEW_LEFT:
			case VIEW_RIGHT:
			case VIEW_FRONT:
			case VIEW_BACK:
			case VIEW_USER:
				adddata();
				break;
			case VIEW_CAMERA:
				addname();
				adddata();
				break;
			case MDATA:
				addchildren();
				break;
			}
			break;

		case 0x4000:
			switch ( h.t ) {
			case NAMED_OBJECT:
				addname();
				adddata();
				addchildren();
				break;
			case N_TRI_OBJECT:
				addchildren();
				break;
			case POINT_ARRAY:
				addcount( sizeof( ChunkTypeFloat3 ) );
				adddata();
				break;
			case POINT_FLAG_ARRAY:
				addcount( sizeof( ChunkTypeShort ) );
				adddata();
				break;
			case FACE_ARRAY:
				addcount( sizeof( ChunkTypeFaceArray ) );
				adddata();
				addchildren();
				break;
			case MSH_MAT_GROUP:
				addname();
				addcount( sizeof( ChunkTypeShort ) );
				adddata();
				break;
			case TEX_VERTS:
				addcount( sizeof( ChunkTypeFloat2 ) );
				adddata();
				break;
			case MESH_MATRIX:
				adddata( sizeof( ChunkTypeMeshMatrix ) );
				break;
			case M3DMAGIC:
				addchildren();
				break;
			}
			break;

		case 0xa000:
			switch ( h.t ) {
			case MAT_NAME:
				addname();
				adddata();
				break;
			case MAT_AMBIENT:
			case MAT_DIFFUSE:
			case MAT_SPECULAR:
				addchildren();
				break;
			case MAT_SHININESS:
			case MAT_SHIN2PCT:
			case MAT_SHIN3PCT:
			case MAT_TRANSPARENCY:
			case MAT_XPFALL:
			case MAT_REFBLUR:
			case MAT_SELF_ILPCT:
				addchildren();
				break;
			case MAT_WIRESIZE:
				adddata( sizeof( ChunkTypeFloat ) );
				break;
			case MAT_SHADING:
				adddata( sizeof( ChunkTypeShort) );
				break;
			case MAT_TEXMAP:
			case MAT_SPECMAP:
			case MAT_OPACMAP:
			case MAT_REFLMAP:
			case MAT_BUMPMAP:
				addchildren();
				break;
			case MAT_MAPNAME:
				addname();
				adddata();
				break;
			case MATERIAL:
				addchildren();
				break;
			}
			break;

		case 0xb000:
			switch ( h.t ) {
			case KFDATA:
				addchildren();
				break;
			case KFHDR:
				adddata( sizeof( ChunkTypeShort ) );
				addname();
				adddata( sizeof( ChunkTypeShort ) );
				adddata( sizeof( ChunkTypeShort ) );
				break;
			case KFSEG:
				adddata( sizeof( ChunkTypeLong ) );
				adddata( sizeof( ChunkTypeLong ) );
				break;
			case KFCURTIME:
				adddata( sizeof( ChunkTypeLong ) );
				break;
			case OBJECT_NODE_TAG:
				addchildren();
				break;
			case NODE_ID:
				adddata( sizeof( ChunkTypeShort ) );
				break;
			case NODE_HDR:
				addname();
				adddata( sizeof( ChunkTypeShort ) );
				adddata( sizeof( ChunkTypeShort ) );
				adddata( sizeof( ChunkTypeShort ) );
				break;
			case PIVOT:
				adddata( sizeof( ChunkTypeFloat ) );
				adddata( sizeof( ChunkTypeFloat ) );
				adddata( sizeof( ChunkTypeFloat ) );
				break;
			case POS_TRACK_TAG:
				adddata( sizeof( ChunkTypeShort ) );
				adddata( sizeof( ChunkTypeShort[4] ) );
				addcount( sizeof( ChunkTypeKfPos ) );
				adddata( sizeof( ChunkTypeShort ) );
				break;
			case ROT_TRACK_TAG:
				adddata( sizeof( ChunkTypeShort ) );
				adddata( sizeof( ChunkTypeShort[4] ) );
				addcount( sizeof( ChunkTypeKfRot ) );
				adddata( sizeof( ChunkTypeShort ) );
				break;
			case SCL_TRACK_TAG:
				adddata( sizeof( ChunkTypeShort ) );
				adddata( sizeof( ChunkTypeShort[4] ) );
				addcount( sizeof( ChunkTypeKfScl ) );
				adddata( sizeof( ChunkTypeShort ) );
				break;
			}
			break;
		}
	}

	void addchildren()
	{
		f->seek( p + CHUNKHEADERSIZE + dl );

		QMap<ChunkType, Chunk *> temp;

		while ( f->pos() < ( p + h.l ) ) {
			ChunkPos q = f->pos();

			ChunkHeader k;
			f->read( (char *)( &k.t ), sizeof( k.t ) );
			f->read( (char *)( &k.l ), sizeof( k.l ) );

			Chunk * z = new Chunk( f, k, q );

			temp.insertMulti( k.t, z );

			f->seek( q + k.l );
		}

		QMapIterator<ChunkType, Chunk *> tempIter( temp );

		while ( tempIter.hasNext() ) {
			tempIter.next();
			c.insertMulti( tempIter.key(), tempIter.value() );
		}

		f->seek( p + h.l );
	}

	void addname()
	{
		f->seek( p + CHUNKHEADERSIZE + dl );

		char n = 0x01;
		int nl = 0;

		while ( n != 0 ) {
			if ( !f->getChar( &n ) ) {
				break;
			}

			nl++;
		}

		dl += nl;
	}

	void addcount( ChunkDataLength _dl )
	{
		f->seek( p + CHUNKHEADERSIZE + dl );

		int n = sizeof( dc );

		f->read( (char *)( &dc ), sizeof( dc ) );

		dl += ( sizeof( ChunkDataCount ) + ( dc * _dl ) );
	}


	void adddata( ChunkDataLength _dl = 0 )
	{
		dl += _dl;

		if ( dl == 0 ) {
			dl = ( h.l - CHUNKHEADERSIZE );
		}

		df = true;

		f->seek( p + CHUNKHEADERSIZE + dl );
	}
};

#endif
