/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005-2009, NIF File Format Library and Tools
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
	Q_DECLARE_TR_FUNCTIONS(NifXmlHandler)

public:
	enum Tag
	{
		tagNone = 0,
		tagFile,
		tagVersion,
		tagCompound,
		tagBlock,
		tagAdd,
		tagBasic,
		tagEnum,
		tagOption,
		tagBitFlag
	};
	
	NifXmlHandler()
	{
		depth = 0;
		tags.insert( "niftoolsxml", tagFile );
		tags.insert( "version", tagVersion );
		tags.insert( "compound", tagCompound );
		tags.insert( "niobject", tagBlock );
		tags.insert( "add", tagAdd );
		tags.insert( "basic", tagBasic );
		tags.insert( "enum", tagEnum );
		tags.insert( "option", tagOption );
		tags.insert( "bitflags", tagBitFlag );
		blk = 0;
	}

	int depth;
	Tag stack[10];
	QHash<QString, Tag> tags;
	QString errorStr;
	
	QString typId;
	QString typTxt;
	
	QString optId;
	QString optVal;
	QString optTxt;
	
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
		if ( depth >= 8 )	err( tr("error maximum nesting level exceeded") );
		
		Tag x = tags.value( tagid );
		if ( x == tagNone )	err( tr("error unknown element '%1'").arg(tagid) );
		
		if ( depth == 0 )
		{
			if ( x != tagFile )	err( tr("this is not a niftoolsxml file") );
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
									err( tr("failed to register alias %1 for type %2").arg(alias).arg(type) );
							typId = alias;
							typTxt = QString();
						}
						else
						{
							if ( x == tagCompound && NifValue::isValid( NifValue::type( list.value( "name" ) ) ) )
								err( tr("compound %1 is already registered as internal type").arg( list.value( "name" ) ) );
							
							QString id = list.value( "name" );
							if ( id.isEmpty() )
								err( tr("compound and niblocks must have a name") );
							
							if ( NifModel::compounds.contains( id ) || NifModel::blocks.contains( id ) )
								err( tr("multiple declarations of %1").arg(id) );
							
							if ( ! blk ) blk = new NifBlock;
							blk->id = list.value( "name" );
							blk->abstract = ( list.value( "abstract" ) == "1" );
							
							if ( x == tagBlock )
							{
								blk->ancestor = list.value( "inherit" );
								if ( ! blk->ancestor.isEmpty() )
								{
									if ( ! NifModel::blocks.contains( blk->ancestor ) )
										err( tr("forward declaration of block id %1").arg(blk->ancestor) );
								}
							}
						};
					}	break;
					case tagBasic:
					{
						QString alias = list.value( "name" );
						QString type = list.value( "nifskopetype" );
						if ( alias.isEmpty() || type.isEmpty() )
							err( tr("basic definition must have a name and a nifskopetype") );
						if ( alias != type )
							if ( ! NifValue::registerAlias( alias, type ) )
								err( tr("failed to register alias %1 for type %2" ).arg(alias).arg(type) );
						typId = alias;
						typTxt = QString();
					}	break;
					case tagEnum:
					case tagBitFlag:
					{
						typId = list.value( "name" );
						typTxt = QString();
						QString storage = list.value( "storage" );
						if ( typId.isEmpty() || storage.isEmpty() )
							err( tr("enum definition must have a name and a known storage type") );
						if ( ! NifValue::registerAlias( typId, storage ) )
							err( tr("failed to register alias %1 for enum type %2").arg(storage).arg(typId) );
						NifValue::EnumType flags = (x == tagBitFlag) ? NifValue::eFlags : NifValue::eDefault;
						NifValue::registerEnumType(typId, flags);
					}	break;
					case tagVersion:
					{
						int v = NifModel::version2number( list.value( "num" ).trimmed() );
						if ( v != 0 && ! list.value( "num" ).isEmpty() )
							NifModel::supportedVersions.append( v );
						else
							err( tr("invalid version tag") );
					}	break;
					default:
						err( tr("expected basic, enum, compound, niobject or version got %1 instead").arg(tagid) );
				}	break;
			case tagVersion:
				//err( tr("version tag must not contain any sub tags") );
				break;
			case tagCompound:
				if ( x != tagAdd )	err( tr("only add tags allowed in compound type declaration") );
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
						QString nstype = list.value( "nifskopetype" );
						if (!nstype.isEmpty() && nstype != type) {
							if ( NifValue::type( nstype ) == NifValue::tNone )
								err( "failed to locate alias " + nstype );
							type = nstype;
						}
						
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

                  QString vercond = list.value( "vercond" );

						QString userver = list.value( "userver" );
						if ( ! userver.isEmpty() )
						{
							if ( ! vercond.isEmpty() )
							   vercond += " && ";
							vercond += QString("(User Version == %1)").arg(userver);
						}
						QString userver2 = list.value( "userver2" );
						if ( ! userver2.isEmpty() )
						{
							if ( ! vercond.isEmpty() )
							   vercond += " && ";
							vercond += QString("(User Version 2 == %1)").arg(userver2);
						}
                  if ( !vercond.isEmpty() )
                  {
                     data.setVerCond( vercond );
                  }

						if ( data.name().isEmpty() || data.type().isEmpty() ) 
							err( tr("add needs at least name and type attributes") );
					}	break;
					default:
						err( tr("only add tags allowed in block declaration") );
				}	break;
			case tagEnum:
			case tagBitFlag:
				push( x );
				switch ( x )
				{
					case tagOption:
						optId = list.value( "name" );
						optVal = list.value( "value" );
						optTxt = QString();
						
						if ( optId.isEmpty() || optVal.isEmpty() )
							err( tr("option defintion must have a name and a value") );
						bool ok;
						optVal.toInt( &ok, 0 );
						if ( ! ok )
							err( tr("option value error (only integers please)") );
						break;
					default:
						err( tr("only option tags allowed in enum declaration") );
				}	break;
			default:
				err( tr("error unhandled tag %1").arg(tagid) );
				break;
		}
		return true;
	}
	
	bool endElement( const QString &, const QString &, const QString & tagid )
	{
		if ( depth <= 0 )		err( tr("mismatching end element tag for element %1").arg(tagid) );
		Tag x = tags.value( tagid );
		if ( pop() != x )		err( tr("mismatching end element tag for element %1").arg(tagid) );
		switch ( x )
		{
			case tagCompound:
				if ( blk && ! blk->id.isEmpty() && ! blk->text.isEmpty() )
					NifValue::setTypeDescription( blk->id, blk->text );
				else if ( !typId.isEmpty() && ! typTxt.isEmpty() )
					NifValue::setTypeDescription( typId, typTxt );
			case tagBlock:
				if ( blk )
				{
					if ( blk->id.isEmpty() )
					{
						delete blk;
						blk = 0;
						err( tr("invalid %1 declaration: name is empty").arg(tagid) );
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
			case tagOption:
				if ( ! NifValue::registerEnumOption( typId, optId, optVal.toInt(), optTxt ) )
					err( tr("failed to register enum option") );
				break;
			case tagBasic:
			case tagEnum:
			case tagBitFlag:
				NifValue::setTypeDescription( typId, typTxt );
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
				break;
			case tagCompound:
			case tagBlock:
				if ( blk )
					blk->text += s.trimmed();
				else
					typTxt += s.trimmed();
				break;
			case tagAdd:
				data.setText( data.text() + s.trimmed() );
				break;
			case tagBasic:
			case tagEnum:
			case tagBitFlag:
				typTxt += s.trimmed();
				break;
			case tagOption:
				optTxt += s.trimmed();
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
					err( tr("compound type %1 refers to unknown type %2").arg(key).arg(data.type()) );
				if ( ! checkTemp( data ) )
					err( tr("compound type %1 refers to unknown template type %2").arg(key).arg(data.temp()) );
				if ( data.type() == key )
					err( tr("compound type %1 contains itself").arg(key) );
			}
		}
		
		foreach ( QString key, NifModel::blocks.keys() )
		{
			NifBlock * blk = NifModel::blocks.value( key );
			if ( ! blk->ancestor.isEmpty() && ! NifModel::blocks.contains( blk->ancestor ) )
				err( tr("niobject %1 inherits unknown ancestor %2").arg(key).arg(blk->ancestor) );
			if ( blk->ancestor == key )
				err( tr("niobject %1 inherits itself").arg(key) );
			foreach ( NifData data, blk->types )
			{
				if ( ! checkType( data ) )
					err( tr("niobject %1 refers to unknown type ").arg(key).arg(data.type()) );
				if ( ! checkTemp( data ) )
					err( tr("niobject %1 refers to unknown template type ").arg(key).arg(data.temp()) );
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
		errorStr.prepend( tr( "XML parse error (line %1):<br>" ).arg( exception.lineNumber() ) );
		return false;
	}
};

bool NifModel::loadXML()
{
	QDir dir( QApplication::applicationDirPath() );
	QString fname;
	// Try local copy first
	if ( dir.exists ( "nif.xml" ) )
		fname = dir.filePath( "nif.xml" );
	else if ( dir.exists( "../docsys/nif.xml" ) )
		fname = dir.filePath( "../docsys/nif.xml" );
	// relative from nifskope/release
	else if ( dir.exists( "../../docsys/nif.xml" ) )
		fname = dir.filePath( "../../docsys/nif.xml" );
	else if ( dir.exists( "/usr/share/nifskope/nif.xml" ) )
		fname = dir.filePath( "/usr/share/nifskope/nif.xml" );
	// last resort
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
		return tr( "error: couldn't open xml description file: ") + filename;
	
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

