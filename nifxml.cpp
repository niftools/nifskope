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

#include "nifmodel.h"
#include "niftypes.h"

#include <QtXml>
#include <QApplication>
#include <QMessageBox>

#define err( X ) { errorStr = X; return false; }

QReadWriteLock					NifModel::XMLlock;

QList<quint32>					NifModel::supportedVersions;

QHash<QString,NifBlock*>		NifModel::compounds;
QHash<QString,NifBlock*>		NifModel::blocks;

class NifXmlHandler : public QXmlDefaultHandler
{
public:
	enum Tag
	{
		tagNone = 0,
		tagFile,
		tagVersion,
		tagCompound,
		tagBlock,
		tagAdd,
		tagInherit,
		tagBasic
	};
	
	NifXmlHandler()
	{
		depth = 0;
		tags.insert( "niflotoxml", tagFile );
		tags.insert( "version", tagVersion );
		tags.insert( "compound", tagCompound );
		tags.insert( "niblock", tagBlock );
		tags.insert( "add", tagAdd );
		tags.insert( "inherit", tagInherit );
		tags.insert( "basic", tagBasic );
		blk = 0;
	}

	int depth;
	Tag stack[10];
	QHash<QString, Tag> tags;
	QString errorStr;
	
	NifBlock		* blk;
	NifData data;
	
	Tag current() const
	{
		return stack[depth-1];
	}
	void push( Tag x )
	{
		stack[depth++] = x;
	}
	Tag pop()
	{
		return stack[--depth];
	}
	
	bool startElement( const QString &, const QString &, const QString & tagid, const QXmlAttributes & list )
	{
		if ( depth >= 8 )	err( "error maximum nesting level exceeded" );
		
		Tag x = tags.value( tagid );
		if ( x == tagNone )	err( "error unknown element '" + tagid + "'" );
		
		if ( depth == 0 )
		{
			if ( x != tagFile )	err( "this is not a niflotoxml file" );
			push( x );
			return true;
		}
		
		switch ( current() )
		{
			case tagFile:
				push( x );
				switch ( x )
				{
					case tagCompound:
					case tagBlock:
					{
						if ( ! list.value("nifskopetype").isEmpty() )
						{
							QString alias = list.value( "name" );
							QString type = list.value( "nifskopetype" );
							if ( alias != type )
								if ( ! NifValue::registerAlias( alias, type ) )
									err( "failed to register alias " + alias + " for type " + type );
						}
						else
						{
							if ( x == tagCompound && NifValue::isValid( NifValue::type( list.value( "name" ) ) ) )
								err( "compound " + list.value( "name" ) + " is already registered as internal type" );
							
							QString id = list.value( "name" );
							if ( id.isEmpty() )
								err( "compound and niblocks must have a name" );
							
							if ( NifModel::compounds.contains( id ) || NifModel::blocks.contains( id ) )
								err( "multiple declarations of " + id );
							
							if ( ! blk ) blk = new NifBlock;
							blk->id = list.value( "name" );
							blk->abstract = ( list.value( "abstract" ) == "1" );
							
							if ( x == tagBlock )
							{
								blk->ancestor = list.value( "inherit" );
								if ( ! blk->ancestor.isEmpty() )
								{
									if ( ! NifModel::blocks.contains( blk->ancestor ) )
										err( "forward declaration of block id " + blk->ancestor );
								}
							}
						};
					}	break;
					case tagBasic:
					{
						QString alias = list.value( "name" );
						QString type = list.value( "nifskopetype" );
						if ( alias.isEmpty() || type.isEmpty() )
							err( "basic definition must have a name and a nifskopetype" );
						if ( alias != type )
							if ( ! NifValue::registerAlias( alias, type ) )
								err( "failed to register alias " + alias + " for type " + type );
					}	break;
					case tagVersion:
						break;
					default:
						err( "expected compound, ancestor, niblock, basic or version got " + tagid + " instead" );
				}	break;
			case tagVersion:
				//err( "version tag must not contain any sub tags" );
				break;
			case tagCompound:
				if ( x != tagAdd )	err( "only add tags allowed in compound type declaration" );
			case tagBlock:
				push( x );
				switch ( x )
				{
					case tagAdd:
					{
						// ns type optimizers come here
						// we really shouldn't be doing this
						// but it will work for now until we find a better solution
						QString type = list.value( "type" );
						
						if ( type == "KeyArray" ) type = "ns keyarray";
						else if ( type == "VectorKeyArray" ) type = "ns keyvecarray";
						else if ( type == "TypedVectorKeyArray" ) type = "ns keyvecarraytyp";
						else if ( type == "RotationKeyArray" ) type = "ns keyrotarray";
						
						// now allocate
						data = NifData(
							list.value( "name" ),
							type,
							list.value( "template" ),
							NifValue( NifValue::type( type ) ),
							list.value( "arg" ),
							list.value( "arr1" ),
							list.value( "arr2" ),
							list.value( "cond" ),
							NifModel::version2number( list.value( "ver1" ) ),
							NifModel::version2number( list.value( "ver2" ) )
						);
						QString defval = list.value( "default" );
						if ( ! defval.isEmpty() )
							data.value.fromString( defval );
						QString userver = list.value( "userver" );
						if ( ! userver.isEmpty() )
						{
							QString cond = data.cond();
							if ( ! cond.isEmpty() )
								cond += " && ";
							cond += "HEADER/User Version == " + userver;
							data.setCond( cond );
						}
						if ( data.name().isEmpty() || data.type().isEmpty() ) err( "add needs at least name and type attributes" );
					}	break;
					case tagInherit:
					{
						QString n = list.value( "name" );
						if ( n.isEmpty() )	err( "inherit needs name attribute" );
						if ( ! NifModel::blocks.contains( n ) )
							err( "forward declaration of block id " + n );
						if ( blk )
						{
							if ( blk->ancestor.isEmpty() )
								blk->ancestor = n;
							else
								err( "allowed is only one inherit tag per block" );
						}
					}	break;
					default:
						err( "only add, inherit, and interface tags allowed in " + tagid + " declaration" );
				}	break;
			default:
				err( "error unhandled tag " + tagid );
				break;
		}
		return true;
	}
	
	bool endElement( const QString &, const QString &, const QString & tagid )
	{
		if ( depth <= 0 )		err( "mismatching end element tag for element " + tagid );
		Tag x = tags.value( tagid );
		if ( pop() != x )		err( "mismatching end element tag for element " + tagid );
		switch ( x )
		{
			case tagCompound:
			case tagBlock:
				if ( blk )
				{
					if ( blk->id.isEmpty() )
					{
						delete blk;
						blk = 0;
						err( "invalid " + tagid + " declaration: name is empty" );
					}
					switch ( x )
					{
						case tagCompound:
							NifModel::compounds.insert( blk->id, blk );
							break;
						case tagBlock:
							NifModel::blocks.insert( blk->id, blk );
							break;
						default:
							break;
					}
					blk = 0;
				}
				break;
			case tagAdd:
				if ( blk )	blk->types.append( data );
				break;
			default:
				break;
		}
		return true;
	}
	
	bool characters( const QString & s )
	{
		switch ( current() )
		{
			case tagVersion:
			{
				int v = NifModel::version2number( s.trimmed() );
				if ( v != 0 )
					NifModel::supportedVersions.append( v );
				else
					err( "invalid version string " + s );
			}	break;
			case tagCompound:
			case tagBlock:
				if ( blk )
					blk->text += s.trimmed();
			case tagAdd:
				data.setText( data.text() + s.trimmed() );
				break;
			default:
				break;
		}
		return true;
	}
	
	bool checkType( const NifData & data )
	{
		return NifModel::compounds.contains( data.type() ) || NifValue::type( data.type() ) != NifValue::tNone || data.type() == "TEMPLATE";
	}
	
	bool checkTemp( const NifData & data )
	{
		return data.temp().isEmpty() || NifValue::type( data.temp() ) != NifValue::tNone || data.temp() == "TEMPLATE" || NifModel::blocks.contains( data.temp() );
	}
	
	bool endDocument()
	{	// make a rough check of the maps
		foreach ( QString key, NifModel::compounds.keys() )
		{
			NifBlock * c = NifModel::compounds.value( key );
			foreach ( NifData data, c->types )
			{
				if ( ! checkType( data ) )
					err( "compound type " + key + " referes to unknown type " + data.type() );
				if ( ! checkTemp( data ) )
					err( "compound type " + key + " referes to unknown template type " + data.temp() );
				if ( data.type() == key )
					err( "compound type " + key + " contains itself" );
			}
		}
		
		foreach ( QString key, NifModel::blocks.keys() )
		{
			NifBlock * blk = NifModel::blocks.value( key );
			if ( ! blk->ancestor.isEmpty() && ! NifModel::blocks.contains( blk->ancestor ) )
				err( "niblock " + key + " inherits unknown ancestor " + blk->ancestor );
			if ( blk->ancestor == key )
				err( "niblock " + key + " inherits itself" );
			foreach ( NifData data, blk->types )
			{
				if ( ! checkType( data ) )
					err( "niblock " + key + " referres to unknown type " + data.type() );
				if ( ! checkTemp( data ) )
					err( "niblock " + key + " referes to unknown template type " + data.temp() );
			}
		}
		
		return true;
	}
	
	QString errorString() const
	{
		return errorStr;
	}
	bool fatalError( const QXmlParseException & exception )
	{
		if ( errorStr.isEmpty() ) errorStr = "Syntax error";
		errorStr.prepend( QString( "XML parse error (line %1):<br>" ).arg( exception.lineNumber() ) );
		return false;
	}
};

bool NifModel::loadXML()
{
	QDir dir( QApplication::applicationDirPath() );
	QString fname;
	if ( dir.exists( "../docsys/nif.xml" ) )
		fname = dir.filePath( "../docsys/nif.xml" );
	else
		fname = dir.filePath( "nif.xml" );
	QString result = NifModel::parseXmlDescription( fname );
	if ( ! result.isEmpty() )
	{
		QMessageBox::critical( 0, "NifSkope", result );
		return false;
	}
	return true;
}

QString NifModel::parseXmlDescription( const QString & filename )
{
	QWriteLocker lck( &XMLlock );
	
	qDeleteAll( compounds );	compounds.clear();
	qDeleteAll( blocks );		blocks.clear();
	
	supportedVersions.clear();
	
	NifValue::initialize();
	
	QFile f( filename );
	if ( ! f.open( QIODevice::ReadOnly | QIODevice::Text ) )
		return QString( "error: couldn't open xml description file: " + filename );
	
	NifXmlHandler handler;
	QXmlSimpleReader reader;
	reader.setContentHandler( &handler );
	reader.setErrorHandler( &handler );
	QXmlInputSource source( &f );
	reader.parse( source );
	
	if ( ! handler.errorString().isEmpty() )
	{
		qDeleteAll( compounds );	compounds.clear();
		qDeleteAll( blocks );		blocks.clear();
		
		supportedVersions.clear();
	}
	
	return handler.errorString();
}

