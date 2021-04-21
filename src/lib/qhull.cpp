/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005-2015, NIF File Format Library and Tools
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the NIF File Format Library and Tools project may not be
   used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

***** END LICENCE BLOCK *****/

#include "qhull.h"

#include "data/niftypes.h"

#include <QDebug>
#include <QByteArray>

#ifdef _MSC_VER
#pragma warning(push)
// Disable compiler warning
#pragma warning(disable: 4005)
// Disable Code Analysis warnings
#pragma warning(disable: 6001 6011 6031 6305 6387)
#pragma warning(disable: 28182 28183)
//	ALL_CODE_ANALYSIS_WARNINGS seems
//	to no longer work as of msvc2013
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wclobbered"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wunused-but-set-parameter"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
extern "C"
{
#include <libqhull/qhull_a.h>

#include <libqhull/libqhull.c>
#include <libqhull/mem.c>
#include <libqhull/qset.c>
#include <libqhull/geom.c>
#include <libqhull/merge.c>
#include <libqhull/poly.c>
#include <libqhull/io.c>
#include <libqhull/stat.c>
#include <libqhull/global.c>
#include <libqhull/user.c>
#include <libqhull/poly2.c>
#include <libqhull/geom2.c>
#include <libqhull/userprintf.c>
#include <libqhull/userprintf_rbox.c>
#include <libqhull/usermem.c>
#include <libqhull/random.c>
#include <libqhull/rboxlib.c>
}
#ifdef _MSC_VER
#pragma warning(pop)
#else
#pragma GCC diagnostic pop
#endif

#include <algorithm>
#include <functional>
#include <math.h>


//! \file qhull.cpp Computes a convex hull

// TODO: investigate the C++ interfaces to Qhull; the Qt interface requires GCC 4.3

//! An interface to <a href="http://www.qhull.org">Qhull</a> for generating Havok-compatible convex shapes
QVector<Triangle> compute_convex_hull( const QVector<Vector3> & verts, QVector<Vector4> & hullVerts, QVector<Vector4> & hullNorms, float roundError )
{
	QVector<Triangle> tris;

	/* dimension of points */
	int dim = 3;
	/* number of points */
	int numpoints = 0;
	/* array of coordinates for each point */
	coordT * points = 0;
	/* True if qhull should free points in qh_freeqhull() or reallocation */
	boolT ismalloc = 0;
	/* option flags for qhull, see qh-quick.htm
	 * i: print vertices incident to each facet
	 * Qt: produce triangulated output
	 * En: max roundoff
	 */
	QString tempFlags = QString( "qhull i Qt E%1" ).arg( roundError );
	QByteArray flagsChar = tempFlags.toLatin1();
	char * flags = flagsChar.data();
	/* output from qh_produce_output()
	 * use NULL to skip qh_produce_output()
	 */
	FILE * outfile = stdout;
	/* error messages from qhull code */
	FILE * errfile = stderr;
	/* 0 if no error from qhull */
	int exitcode;
	/* set by FORALLfacets */
	facetT * facet;
	/* memory remaining after qh_memfreeshort */
	int curlong, totlong;

	/* vertexT is a struct containing coordinates etc. */
	vertexT * vertex, ** vertexp;
	setT * vertices;

	numpoints = verts.size();
	points = new coordT[3 * numpoints];

	// Shut up Code Analysis C6011
	if ( !points )
		return QVector<Triangle>();

	for ( int i = 0; i < numpoints; ++i ) {
		points[i * 3 + 0] = verts[i][0];
		points[i * 3 + 1] = verts[i][1];
		points[i * 3 + 2] = verts[i][2];
	}

	/* initialize dim, numpoints, points[], ismalloc here */
	exitcode = qh_new_qhull( dim, numpoints, points, ismalloc,
		flags, outfile, errfile );

	if ( !exitcode ) {
		/* if no error */
		/* 'qh facet_list' contains the convex hull */
		FORALLfacets {
			/* from poly2.c */
			vertices = qh_facet3vertex( facet );
			Vector4 hullNorm( facet->normal[0], facet->normal[1], facet->normal[2], facet->offset );
			hullNorms.append( hullNorm );

			if ( qh_setsize( vertices ) == 3 ) {
				Triangle tri;
				int i = 0;
				FOREACHvertex_( vertices ) {
					tri[i++] = qh_pointid( vertex->point );
					/* find the hull vertices */
					Vector4 hullVert( vertex->point[0], vertex->point[1], vertex->point[2], 0 );
					hullVerts.append( hullVert );
				}
				tris.push_back( tri );
			}

			qh_settempfree( &vertices );
		}
	}

	qh_freeqhull( !qh_ALL );
	qh_memfreeshort( &curlong, &totlong );

	if ( curlong || totlong )
		fprintf( errfile, "qhull internal warning (main): did not free %d bytes of long memory (%d pieces)\n",
			totlong, curlong );

	delete[] points;
	return tris;
};
