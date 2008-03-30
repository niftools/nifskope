#include "qtwrapper.h"

#include "NvTriStrip.h"

QList< QVector<quint16> > stripify( QVector<Triangle> triangles, bool stitch )
{
	unsigned short * data = (unsigned short *) malloc( triangles.count() * 3 * sizeof( unsigned short ) );
	for ( int t = 0; t < triangles.count(); t++ )
	{
		data[ t * 3 + 0 ] = triangles[t][0];
		data[ t * 3 + 1 ] = triangles[t][1];
		data[ t * 3 + 2 ] = triangles[t][2];
	}
	
	PrimitiveGroup * groups = 0;
	unsigned short numGroups = 0;
	
	SetStitchStrips( stitch );
	//SetCacheSize( 64 );
	GenerateStrips( data, triangles.count()*3, &groups, &numGroups );
	free( data );
	
	QList< QVector<quint16> > strips;

	if ( !groups )
		return strips;
	
	for ( int g = 0; g < numGroups; g++ )
	{
		if ( groups[g].type == PT_STRIP )
		{
			QVector< quint16 > strip( groups[g].numIndices, 0 );
			for ( quint32 s = 0; s < groups[g].numIndices; s++ )
			{
				strip[s] = groups[g].indices[s];
			}
			strips.append( strip );
		}
	}
	
	delete [] groups;
	
	return strips;
}

QVector<Triangle> triangulate( QVector<quint16> strip )
{
	QVector<Triangle> tris;
	quint16 a, b = strip.value( 0 ), c = strip.value( 1 );
	bool flip = false;
	for ( int s = 2; s < strip.count(); s++ )
	{
		a = b;
		b = c;
		c = strip.value( s );
		if ( a != b && b != c && c != a )
		{
			if ( ! flip )
				tris.append( Triangle( a, b, c ) );
			else
				tris.append( Triangle( a, c, b ) );
		}
		flip = ! flip;
	}
	return tris;
}

QVector<Triangle> triangulate( QList< QVector<quint16> > strips )
{
	QVector<Triangle> tris;
	foreach( QVector<quint16> strip, strips )
	{
		tris += triangulate( strip );
	}
	return tris;
}

