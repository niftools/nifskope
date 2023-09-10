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

#include "nifitem.h"
#include "model/basemodel.h"

void NifItem::registerChild( NifItem * item, int at )
{
	int nOldChildren = childItems.count();
	if ( at < 0 || at >= nOldChildren ) {
		at = nOldChildren;
		childItems.append( item );
		item->rowIdx = at;
		updateLinkCache( at, false );
	} else {
		childItems.insert( at, item );
		item->rowIdx = at;
		updateChildRows( at + 1 );
		updateLinkCache( at, true );
	}
}

NifItem * NifItem::unregisterChild( int at )
{
	if ( at >= 0 && at < childItems.count() ) {
		NifItem * item = childItems.at( at );
		childItems.remove( at );
		updateChildRows( at );
		updateLinkCache( at, true );
		return item;
	}

	return nullptr;
}

void NifItem::registerInParentLinkCache()
{
	NifItem * c = this;
	NifItem * p = parentItem;
	while( p ) {
		bool bOldHasChildLinks = p->hasChildLinks(); 
		p->linkAncestorRows.append( c->row() );
		if ( bOldHasChildLinks )
			break; // Do NOT register p in its parent (again) if c is NOT a first registered child link for p
		c = p;
		p = c->parentItem;
	}
}

void NifItem::unregisterInParentLinkCache()
{
	NifItem * c = this;
	NifItem * p = parentItem;
	while( p ) {
		int iRemove = p->linkAncestorRows.indexOf( c->row() );
		if ( iRemove < 0 ) 
			break; // c is not even registered in p...
		p->linkAncestorRows.remove( iRemove );
		if ( p->hasChildLinks() ) 
			break; // Do NOT unregister p in its parent if p still has other registered child links
		c = p;
		p = c->parentItem;
	}
}

static void cleanupChildIndexVector( QVector<ushort> v, int iStartChild )
{
	for ( int i = v.count() - 1; i >= 0; i-- ) {
		if ( v.at(i) >= iStartChild )
			v.remove( i );
	}
}

void NifItem::updateLinkCache( int iStartChild, bool bDoCleanup )
{
	bool bOldHasChildLinks = hasChildLinks();

	// Clear outdated links
	if ( bDoCleanup ) {
		cleanupChildIndexVector( linkRows, iStartChild );
		cleanupChildIndexVector( linkAncestorRows, iStartChild );
	}

	// Add new links
	for ( int i = iStartChild; i < childItems.count(); i++ ) {
		const NifItem * c = childItems.at( i );
		if ( c->isLink() )
			linkRows.append( i );
		if ( c->hasChildLinks() )
			linkAncestorRows.append( i );
	}

	// Update parent link caches if needed
	if ( hasChildLinks() ) {
		if ( !bOldHasChildLinks )
			registerInParentLinkCache();
	} else { // not hasChildLinks
		if ( bOldHasChildLinks )
			unregisterInParentLinkCache();
	}
}

void NifItem::onParentItemChange()
{
	parentModel     = parentItem->parentModel;
	vercondStatus   = -1;
	conditionStatus = -1;

	for ( NifItem * c : childItems )
		c->onParentItemChange();
}

QString NifItem::repr() const
{
	return parentModel->itemRepr( this );
}

void NifItem::reportError( const QString & msg ) const
{
	parentModel->reportError( this, msg );
}

void NifItem::reportError( const QString & funcName, const QString & msg ) const
{
	parentModel->reportError( this, funcName, msg );
}
