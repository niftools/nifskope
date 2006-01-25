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

#include "spellbook.h"

#include <QApplication>
#include <QBuffer>
#include <QClipboard>
#include <QGLPixelBuffer>
#include <QMimeData>

#include "glmath.h"
#include "glscene.h"

class spUpdateArray : public Spell
{
public:
	QString name() const { return "Update Array"; }
	
	bool isApplicable( NifModel * nif, const QModelIndex & index )
	{
		return ! nif->itemArr1( index ).isEmpty();
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		nif->updateArray( index );
		return index;
	}
};

REGISTER_SPELL( spUpdateArray )

class spUpdateHeader : public Spell
{
public:
	QString name() const { return "Update Header"; }
	
	bool isApplicable( NifModel * nif, const QModelIndex & index )
	{
		return ( nif->getHeader() == index );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		nif->updateHeader();
		return index;
	}
};

REGISTER_SPELL( spUpdateHeader )

class spFollowLink : public Spell
{
public:
	QString name() const { return "Follow Link"; }
	
	bool isApplicable( NifModel * nif, const QModelIndex & index )
	{
		return nif->itemIsLink( index );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		QModelIndex idx = nif->getBlock( nif->itemLink( index ) );
		if ( idx.isValid() )
			return idx;
		else
			return index;
	}
};

REGISTER_SPELL( spFollowLink )

class spInsertBlock : public Spell
{
public:
	QString name() const { return "Insert"; }
	QString page() const { return "Block"; }
	
	bool isApplicable( NifModel * nif, const QModelIndex & index )
	{
		return ( ! index.isValid() || ! index.parent().isValid() );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		QMenu menu;
		foreach ( QString id, nif->allNiBlocks() )
			menu.addAction( id );
		QAction * act = menu.exec();
		if ( act )
			return nif->insertNiBlock( act->text(), nif->getBlockNumber( index ) + 1 );
		else
			return index;
	}
};

REGISTER_SPELL( spInsertBlock )

class spInsertProperty : public Spell
{
public:
	QString name() const { return "Add Property"; }
	QString page() const { return "Block"; }
	
	bool isApplicable( NifModel * nif, const QModelIndex & index )
	{
		return nif->itemType( index ) == "NiBlock" && nif->inherits( nif->itemName( index ), "ANode" );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		QMenu menu;
		foreach ( QString id, nif->allNiBlocks() )
			if ( nif->inherits( id, "AProperty" ) )
				menu.addAction( id );
		QAction * act = menu.exec();
		if ( act )
		{
			QPersistentModelIndex iParent = index;
			QModelIndex iProperty = nif->insertNiBlock( act->text(), nif->getBlockNumber( index ) + 1 );
			
			QModelIndex iProperties = nif->getIndex( iParent, "Properties" );
			if ( iProperties.isValid() )
			{
				QModelIndex iIndices = nif->getIndex( iProperties, "Indices" );
				if ( iIndices.isValid() )
				{
					int numlinks = nif->get<int>( iProperties, "Num Indices" );
					nif->set<int>( iProperties, "Num Indices", numlinks + 1 );
					nif->updateArray( iIndices );
					nif->setLink( iIndices.child( numlinks, 0 ), nif->getBlockNumber( iProperty ) );
				}
			}
			return iProperty;
		}
		else
			return index;
	}
};

REGISTER_SPELL( spInsertProperty )

class spRemoveBlock : public Spell
{
public:
	QString name() const { return "Remove"; }
	QString page() const { return "Block"; }
	
	bool isApplicable( NifModel * nif, const QModelIndex & index )
	{
		return ( nif->itemType( index ) == "NiBlock" && nif->getBlockNumber( index ) >= 0 );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		nif->removeNiBlock( nif->getBlockNumber( index ) );
		return QModelIndex();
	}
};

REGISTER_SPELL( spRemoveBlock )

class spCopyBlock : public Spell
{
public:
	QString name() const { return "Copy Block"; }
	QString page() const { return "Clipboard"; }
	
	bool isApplicable( NifModel * nif, const QModelIndex & index )
	{
		return ( nif->itemType( index ) == "NiBlock" && nif->isNiBlock( nif->itemName( index ) ) );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		QByteArray data;
		QBuffer buffer( & data );
		if ( buffer.open( QIODevice::WriteOnly ) && nif->save( buffer, index ) )
		{
			QMimeData * mime = new QMimeData;
			mime->setData( QString( "nifskope/niblock/%1/%2" ).arg( nif->getVersion() ).arg( nif->itemName( index ) ), data );
			QApplication::clipboard()->setMimeData( mime );
		}
		return index;
	}
};

REGISTER_SPELL( spCopyBlock )

class spPasteBlock : public Spell
{
public:
	QString name() const { return "Paste Block"; }
	QString page() const { return "Clipboard"; }
	
	QString blockType( const QString & format, NifModel * nif )
	{
		QStringList split = format.split( "/" );
		if ( split.value( 0 ) == "nifskope" && split.value( 1 ) == "niblock"
		&& split.value( 2 ) == nif->getVersion() && nif->isNiBlock( split.value( 3 ) ) )
			return split.value( 3 );
		return QString();
	}
	
	bool acceptFormat( const QString & format, NifModel * nif )
	{
		return ! blockType( format, nif ).isEmpty();
	}
	
	bool isApplicable( NifModel * nif, const QModelIndex & index )
	{
		const QMimeData * mime = QApplication::clipboard()->mimeData();
		if ( mime )
			foreach ( QString form, mime->formats() )
				if ( acceptFormat( form, nif ) )
					return true;
		return false;
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		const QMimeData * mime = QApplication::clipboard()->mimeData();
		if ( mime )
		{
			foreach ( QString form, mime->formats() )
			{
				if ( acceptFormat( form, nif ) )
				{
					QByteArray data = mime->data( form );
					QBuffer buffer( & data );
					if ( buffer.open( QIODevice::ReadOnly ) )
					{
						QModelIndex block = nif->insertNiBlock( blockType( form, nif ), nif->getBlockNumber( index ) + 1 );
						nif->load( buffer, block );
						return block;
					}
				}
			}
		}
		return QModelIndex();
	}
};

REGISTER_SPELL( spPasteBlock )

class spApplyTransformation : public Spell
{
public:
	QString name() const { return "Apply"; }
	QString page() const { return "Transform"; }
	
	bool isApplicable( NifModel * nif, const QModelIndex & index )
	{
		return ( nif->itemType( index ) == "NiBlock" &&
			( ( nif->inherits( nif->itemName( index ), "AParentNode" ) && nif->get<int>( index, "Flags" ) & 8 )
				|| nif->itemName( index ) == "NiTriShape" ) );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		if ( nif->itemType( index ) == "NiBlock" && nif->inherits( nif->itemName( index ), "AParentNode" ) )
		{
			Transform tp( nif, index );
			bool ok = false;
			foreach ( int l, nif->getChildLinks( nif->getBlockNumber( index ) ) )
			{
				QModelIndex iChild = nif->getBlock( l );
				if ( iChild.isValid() && nif->inherits( nif->itemName( iChild ), "ANode" ) )
				{
					Transform tc( nif, iChild );
					tc = tp * tc;
					tc.writeBack( nif, iChild );
					ok = true;
				}
			}
			if ( ok )
			{
				tp = Transform();
				tp.writeBack( nif, index );
			}
		}
		else if ( nif->itemType( index ) == "NiBlock" && nif->itemName( index ) == "NiTriShape" )
		{
			Transform t( nif, index );
			QModelIndex iData = nif->getBlock( nif->getLink( index, "Data" ), "NiTriShapeData" );
			if ( iData.isValid() )
			{
				QModelIndex iVertices = nif->getIndex( iData, "Vertices" );
				if ( iVertices.isValid() )
				{
					for ( int v = 0; v < nif->rowCount( iVertices ); v++ )
						nif->setItemData<Vector3>( iVertices.child( v, 0 ), t * nif->itemData<Vector3>( iVertices.child( v, 0 ) ) );
					
					QModelIndex iNormals = nif->getIndex( iData, "Normals" );
					if ( iNormals.isValid() )
					{
						for ( int n = 0; n < nif->rowCount( iNormals ); n++ )
							nif->setItemData<Vector3>( iNormals.child( n, 0 ), t.rotation * nif->itemData<Vector3>( iNormals.child( n, 0 ) ) );
					}
				}
				QModelIndex iCenter = nif->getIndex( iData, "Center" );
				if ( iCenter.isValid() )
					nif->setItemData<Vector3>( iCenter, t.rotation * nif->itemData<Vector3>( iCenter ) + t.translation );
				QModelIndex iScale = nif->getIndex( iData, "Scale" );
				if ( iScale.isValid() )
					nif->setItemData<float>( iScale, t.scale * nif->itemData<float>( iScale ) );
				t = Transform();
				t.writeBack( nif, index );
			}
		}
		return index;
	}
};

REGISTER_SPELL( spApplyTransformation )

class spClearTransformation : public Spell
{
public:
	QString name() const { return "Clear"; }
	QString page() const { return "Transform"; }
	
	bool isApplicable( NifModel * nif, const QModelIndex & index )
	{
		return ( nif->itemType( index ) == "NiBlock" && nif->inherits( nif->itemName( index ), "ANode" ) );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		Transform tp;
		tp.writeBack( nif, index );
		return index;
	}
};

REGISTER_SPELL( spClearTransformation )

class spPackTexture : public Spell
{
public:
	QString name() const { return "Pack"; }
	QString page() const { return "Texture"; }
	
	bool isApplicable( NifModel * nif, const QModelIndex & index )
	{
		if ( ! ( QGLPixelBuffer::hasOpenGLPbuffers() && nif->itemType( index ) == "NiBlock" && nif->itemName( index ) == "NiSourceTexture" ) )
			return false;
		QModelIndex iTex = nif->getIndex( index, "Texture Source" );
		return ( iTex.isValid() && nif->get<int>( iTex, "Use External" ) == 1 );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		QGLPixelBuffer gl( QSize( 32, 32 ) );
		gl.makeCurrent();
		
		GLTex * tex = new GLTex( index );
		
		if ( tex->id )
		{
			glBindTexture( GL_TEXTURE_2D, tex->id );
			
			GLint w, h;
			
			glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w );
			glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h );
			
			quint32 s = w * h * 4;
			quint32 o = 0;
			
			QByteArray data;
			data.resize( s );
			glPixelStorei( GL_PACK_ALIGNMENT, 1 );
			glPixelStorei( GL_PACK_SWAP_BYTES, GL_FALSE );
			glGetTexImage( GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data() );
			
			QList<QSize> sizes;
			QList<quint32> offsets;
			
			sizes.append( QSize( w, h ) );
			offsets.append( o );
			
			while ( w != 1 || h != 1 )
			{
				o = s;
				w /= 2; if ( w == 0 ) w = 1;
				h /= 2; if ( h == 0 ) h = 1;
				s += w*h*4;
				sizes.append( QSize( w, h ) );
				offsets.append( o );
			}
			
			data.resize( s );
			
			for ( int i = 1; i < sizes.count(); i++ )
			{
				const quint8 * src = (const quint8 * ) data.data() + offsets[ i-1 ];
				quint8 * dst = (quint8 *) data.data() + offsets[ i ];
				
				w = sizes[ i ].width();
				h = sizes[ i ].height();
				
				quint32 xo = ( sizes[ i-1 ].width() > 1 ? 4 : 0 );
				quint32 yo = ( sizes[ i-1 ].height() > 1 ? sizes[ i-1 ].width() * 4 : 0 );
				
				for ( int y = 0; y < h; y++ )
				{
					for ( int x = 0; x < w; x++ )
					{
						for ( int b = 0; b < 4; b++ )
						{
							*dst++ = ( *(src+xo) + *(src+yo) + *(src+xo+yo) + *src++ ) / 4;
						}
						src += xo;
					}
					src += yo;
				}
			}
			
			int blockNum = nif->getBlockNumber( index );
			nif->insertNiBlock( "NiPixelData", blockNum+1 );
			
			QPersistentModelIndex iSourceTexture = nif->getBlock( blockNum, "NiSourceTexture" );
			QModelIndex iPixelData = nif->getBlock( blockNum+1, "NiPixelData" );
			if ( iSourceTexture.isValid() && iPixelData.isValid() )
			{
				nif->set<int>( iPixelData, "Pixel Format", 1 );
				nif->set<int>( iPixelData, "Red Mask", 0x000000ff );
				nif->set<int>( iPixelData, "Green Mask", 0x0000ff00 );
				nif->set<int>( iPixelData, "Blue Mask", 0x00ff0000 );
				nif->set<int>( iPixelData, "Alpha Mask", 0xff000000 );
				nif->set<int>( iPixelData, "Bits Per Pixel", 32 );
				nif->set<int>( iPixelData, "Bytes Per Pixel", 4 );
				nif->set<int>( iPixelData, "Num Mipmaps", sizes.count() );
				QModelIndex iMipMaps = nif->getIndex( iPixelData, "Mipmaps" );
				if ( iMipMaps.isValid() )
				{
					nif->updateArray( iMipMaps );
					for ( int m = 0; m < sizes.count() && m < nif->rowCount( iMipMaps ); m++ )
					{
						nif->set<int>( iMipMaps.child( m, 0 ), "Width", sizes[m].width() );
						nif->set<int>( iMipMaps.child( m, 0 ), "Height", sizes[m].height() );
						nif->set<int>( iMipMaps.child( m, 0 ), "Offset", offsets[m] );
					}
				}
				nif->set<QByteArray>( iPixelData, "Pixel Data", data );
				
				QModelIndex iUnknown = nif->getIndex( iPixelData, "Unknown 8 Bytes" );
				if ( iUnknown.isValid() )
				{
					static const int unknownPixeldataBytes[8] = { 129, 8, 130, 32, 0, 65, 12, 0 };
					for ( int r = 0; r < 8 && r < nif->rowCount( iUnknown ); r++ )
					{
						nif->setItemData<int>( iUnknown.child( r, 0 ), unknownPixeldataBytes[r] );
					}
				}
				
				QModelIndex iTexSrc = nif->getIndex( iSourceTexture, "Texture Source" );
				if ( iTexSrc.isValid() )
				{
					nif->set<int>( iTexSrc, "Use External", 0 );
					nif->set<int>( iTexSrc, "Unknown Byte", 1 );
					nif->setLink( iTexSrc, "Pixel Data", blockNum+1 );
				}
			}
			return iSourceTexture;
		}
		delete tex;
		return QModelIndex();
	}
};

REGISTER_SPELL( spPackTexture )
