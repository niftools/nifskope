/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005, NIF File Format Library and Tools
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the NIF File Format Library and Tools projectmay not be
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

#include "refrbrowser.h"

#include <QDir>

#include "nifmodel.h"

ReferenceBrowser::ReferenceBrowser( QWidget * parent )
	: QTextBrowser( parent )
{
	nif = NULL;

	docFolderPresent = QDir( "./doc" ).exists();

	if( docFolderPresent ) {
		setSearchPaths( QStringList() << "./doc" );
		setStyleSheet( "docsys.css" );
		setSource( QUrl( "index.html" ) );
	}
	else {
		setText( tr("Please install the reference " \
			"documentation into the 'doc' folder.") );
	}
}

void ReferenceBrowser::setNifModel( NifModel * nifModel )
{
	nif = nifModel;
}

// TODO: Read documentation from ZIP files

void ReferenceBrowser::browse( const QModelIndex & index )
{
	if( !nif || !docFolderPresent ) {
		return;
	}

	QString blockType = nif->getBlockType( index );
	if( blockType == "NiBlock" ) {
		blockType = nif->getBlockName( index );
	}

	QFile RefrFile( QString( "./doc/%1.html" ).arg( blockType ) );
	if( RefrFile.exists() ) {
		setSource( QUrl( QString( "%1.html" ).arg( blockType ) ) );
	}
	else {
		setText( tr("The reference file for '%1' " \
			"could not be found.").arg( blockType ) );
	}
}
