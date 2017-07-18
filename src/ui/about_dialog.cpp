#include "about_dialog.h"

AboutDialog::AboutDialog( QWidget * parent )
	: QDialog( parent )
{
	ui.setupUi( this );

	setAttribute( Qt::WA_DeleteOnClose );

#ifdef NIFSKOPE_REVISION
	this->setWindowTitle( tr( "About NifSkope %1 (revision %2)" ).arg( NIFSKOPE_VERSION, NIFSKOPE_REVISION ) );
#else
	this->setWindowTitle( tr( "About NifSkope %1" ).arg( NIFSKOPE_VERSION ) );
#endif
	QString text = tr( R"rhtml(
	<p>NifSkope is a tool for opening and editing the NetImmerse file format (NIF).</p>

	<p>NifSkope is free software available under a BSD license.
	The source is available via <a href='https://github.com/niftools/nifskope'>GitHub</a></p>

	<p>For more information visit <a href='https://forum.niftools.org'>our forum</a>.<br>
	To receive support for NifSkope please use the
	<a href='https://forum.niftools.org/24-nifskope/'>NifSkope Help subforum</a>.</p>

	<p>The most recent stable version of NifSkope can be downloaded from the <a href='https://github.com/niftools/nifskope/releases'>
	official GitHub release page</a>.</p>
	
	<p>A detailed changelog and the latest developmental builds of NifSkope 
	<a href='https://github.com/jonwd7/nifskope/releases'>can be found here</a>.</p>
	
	<p>For the decompression of BSA (Version 105) files, NifSkope uses <a href='https://github.com/lz4/lz4'>LZ4</a>:<br>
	LZ4 Library<br>
	Copyright (c) 2011-2015, Yann Collet<br>
	All rights reserved.</p>
	
	<p>For the generation of mopp code on Windows builds, NifSkope uses <a href='http://www.havok.com'>Havok(R)</a>:<br>
	Copyright (c) 1999-2008 Havok.com Inc. (and its Licensors).<br>
	All Rights Reserved.</p>
	
	<p>For the generation of convex hulls, NifSkope uses <a href='http://www.qhull.org'>Qhull</a>:<br>
	Copyright (c) 1993-2015  C.B. Barber and The Geometry Center.<br>
	Qhull is free software and may be obtained via http from www.qhull.org or <a href='https://github.com/qhull/qhull'>GitHub</a>.
	See Qhull_COPYING.txt for details.</p>
	
	)rhtml" );

	ui.text->setText( text );
}
