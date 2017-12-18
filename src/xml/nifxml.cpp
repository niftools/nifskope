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

#include "message.h"
#include "data/niftypes.h"
#include "model/nifmodel.h"

#include <QtXml> // QXmlDefaultHandler Inherited
#include <QCoreApplication>
#include <QMessageBox>


//! \file nifxml.cpp NifXmlHandler, NifModel XML

//! Set NifXmlHandler::errorStr and return
#define err( X ) { errorStr = X; return false; }

QReadWriteLock             NifModel::XMLlock;
QList<quint32>             NifModel::supportedVersions;
QHash<QString, NifBlockPtr> NifModel::compounds;
QHash<QString, NifBlockPtr> NifModel::fixedCompounds;
QHash<QString, NifBlockPtr> NifModel::blocks;
QMap<quint32, NifBlockPtr> NifModel::blockHashes;

//! Parses nif.xml
class NifXmlHandler final : public QXmlDefaultHandler
{
//	Q_DECLARE_TR_FUNCTIONS(NifXmlHandler)

public:
	//! XML block types
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

	//! i18n wrapper for various strings
	/*!
	 * Note that we don't use QObject::tr() because that doesn't provide
	 * context. We also don't use the QCoreApplication Q_DECLARE_TR_FUNCTIONS()
	 * macro because that won't document properly.
	 */
	static inline QString tr( const char * key, const char * comment = 0 ) { return QCoreApplication::translate( "NifXmlHandler", key, comment ); }

	//! Constructor
	NifXmlHandler()
	{
		tags.insert( "niftoolsxml", tagFile );
		tags.insert( "version", tagVersion );
		tags.insert( "compound", tagCompound );
		tags.insert( "niobject", tagBlock );
		tags.insert( "add", tagAdd );
		tags.insert( "basic", tagBasic );
		tags.insert( "enum", tagEnum );
		tags.insert( "option", tagOption );
		tags.insert( "bitflags", tagBitFlag );
	}

	//! Current position on stack
	int depth = 0;
	//! Tag stack
	Tag stack[10] = {};
	//! Hashmap of tags
	QHash<QString, Tag> tags;
	//! Error string
	QString errorStr;

	//! Current type ID
	QString typId;
	//! Current type description
	QString typTxt;

	//! Current enumeration ID
	QString optId;
	//! Current enumeration value
	QString optVal;
	//! Current enumeration text
	QString optTxt;

	//! Block
	NifBlockPtr blk = nullptr;
	//! Data
	NifData data;

	//! The current tag
	Tag current() const
	{
		return stack[depth - 1];
	}
	//! Add a tag to the stack
	void push( Tag x )
	{
		stack[depth++] = x;
	}
	//! Get a tag from the stack
	Tag pop()
	{
		return stack[--depth];
	}

	//! Reimplemented from QXmlContentHandler
	/*!
	 * \param Namespace (unused)
	 * \param localName (unused)
	 * \param tagid Qualified name
	 * \param list Attributes
	 */
	bool startElement( const QString &, const QString &, const QString & tagid, const QXmlAttributes & list ) override final
	{
		if ( depth >= 8 )
			err( tr( "error maximum nesting level exceeded" ) );

		Tag x = tags.value( tagid );

		if ( x == tagNone )
			err( tr( "error unknown element '%1'" ).arg( tagid ) );

		if ( depth == 0 ) {
			if ( x != tagFile )
				err( tr( "this is not a niftoolsxml file" ) );

			push( x );
			return true;
		}

		switch ( current() ) {
		case tagFile:
			push( x );

			switch ( x ) {
			case tagCompound:
			case tagBlock:
				{
					QString name = list.value( "name" );

					if ( NifValue::type( name ) != NifValue::tNone ) {
						// Internal Type
						typId  = name;
						typTxt = QString();
					} else {
						QString id = name;

						if ( x == tagCompound && NifValue::isValid( NifValue::type( id ) ) )
							err( tr( "compound %1 is already registered as internal type" ).arg( list.value( "name" ) ) );

						if ( id.isEmpty() )
							err( tr( "compound and niblocks must have a name" ) );

						if ( NifModel::compounds.contains( id ) || NifModel::blocks.contains( id ) )
							err( tr( "multiple declarations of %1" ).arg( id ) );

						if ( !blk )
							blk = NifBlockPtr( new NifBlock );

						blk->id = id;
						blk->abstract = ( list.value( "abstract" ) == "1" );

						if ( x == tagBlock ) {
							blk->ancestor = list.value( "inherit" );

							if ( !blk->ancestor.isEmpty() ) {
								if ( !NifModel::blocks.contains( blk->ancestor ) )
									err( tr( "forward declaration of block id %1" ).arg( blk->ancestor ) );
							}
						}

						QString externalCond = list.value( "externalcond" );
						if ( externalCond == "1" || blk->id.startsWith( "BSVertexData" ) ) {
							NifModel::fixedCompounds.insert( blk->id, blk );
						}
					}
				}
				break;
			case tagBasic:
				{
					QString name = list.value( "name" );

					if ( NifValue::type( name ) == NifValue::tNone )
						err( tr( "basic definition %1 must have an internal NifSkope type" ).arg( name ) );

					typId = name;
					typTxt = QString();
				}
				break;
			case tagEnum:
			case tagBitFlag:
				{
					typId  = list.value( "name" );
					typTxt = QString();
					QString storage = list.value( "storage" );

					if ( typId.isEmpty() || storage.isEmpty() )
						err( tr( "enum definition must have a name and a known storage type" ) );

					if ( !NifValue::registerAlias( typId, storage ) )
						err( tr( "failed to register alias %1 for enum type %2" ).arg( storage, typId ) );

					NifValue::EnumType flags = (x == tagBitFlag) ? NifValue::eFlags : NifValue::eDefault;
					NifValue::registerEnumType( typId, flags );
				}
				break;
			case tagVersion:
				{
					int v = NifModel::version2number( list.value( "num" ).trimmed() );

					if ( v != 0 && !list.value( "num" ).isEmpty() )
						NifModel::supportedVersions.append( v );
					else
						err( tr( "invalid version tag" ) );
				}
				break;
			default:
				err( tr( "expected basic, enum, compound, niobject or version got %1 instead" ).arg( tagid ) );
			}

			break;
		case tagVersion:
			//err( tr("version tag must not contain any sub tags") );
			break;
		case tagCompound:

			if ( x != tagAdd )
				err( tr( "only add tags allowed in compound type declaration" ) );

		case tagBlock:
			push( x );

			switch ( x ) {
			case tagAdd:
				{
					QString type = list.value( "type" );
					QString tmpl = list.value( "template" );
					QString arg  = list.value( "arg" );
					QString arr1 = list.value( "arr1" );
					QString arr2 = list.value( "arr2" );
					QString cond = list.value( "cond" );
					QString ver1 = list.value( "ver1" );
					QString ver2 = list.value( "ver2" );
					QString abs = list.value( "abstract" );
					QString bin = list.value( "binary" );
					QString vercond = list.value( "vercond" );
					QString userver = list.value( "userver" );
					QString userver2 = list.value( "userver2" );

					bool isTemplated = (type == "TEMPLATE" || tmpl == "TEMPLATE");
					bool isCompound = NifModel::compounds.contains( type );
					bool isArray = !arr1.isEmpty();
					bool isMultiArray = !arr2.isEmpty();

					// Override some compounds as mixins (compounds without nesting)
					//	This flattens the hierarchy as if the mixin's <add> rows belong to the mixin's parent
					//	This only takes place on a per-row basis and reverts to nesting when in an array.
					bool isMixin = false;
					if ( isCompound ) {
						static const QVector<QString> mixinTypes {
							"HavokFilter",
							"HavokMaterial",
							"RagdollDescriptor",
							"LimitedHingeDescriptor",
							"HingeDescriptor",
							"BallAndSocketDescriptor",
							"PrismaticDescriptor",
							"MalleableDescriptor",
							"ConstraintData"
						};

						isMixin = mixinTypes.contains( type );
						// The <add> must not use any attributes other than name/type
						isMixin = isMixin && !isTemplated && !isArray;
						isMixin = isMixin && cond.isEmpty() && ver1.isEmpty() && ver2.isEmpty()
							&& vercond.isEmpty() && userver.isEmpty() && userver2.isEmpty();

						isCompound = !isMixin;
					}

					// Special casing for BSVertexDesc as an ARG
					// since we have internalized the type
					if ( arg == "Vertex Desc\\Vertex Attributes" )
						arg = "Vertex Desc";

					// now allocate
					data = NifData(
						list.value( "name" ),
						type,
						tmpl,
						NifValue( NifValue::type( type ) ),
						arg,
						arr1,
						arr2,
						cond,
						NifModel::version2number( ver1 ),
						NifModel::version2number( ver2 )
					);

					// Set data flags
					data.setAbstract( abs == "1" );
					data.setBinary( bin == "1" );
					data.setTemplated( isTemplated );
					data.setIsCompound( isCompound );
					data.setIsArray( isArray );
					data.setIsMultiArray( isMultiArray );
					data.setIsMixin( isMixin );

					QString defval = list.value( "default" );

					if ( !defval.isEmpty() ) {
						bool ok;
						quint32 enumVal = NifValue::enumOptionValue( type, defval, &ok );

						if ( ok ) {
							data.value.setCount( enumVal );
						} else {
							data.value.setFromString( defval );
						}
					}

					if ( !userver.isEmpty() ) {
						if ( !vercond.isEmpty() )
							vercond += " && ";

						vercond += QString( "(User Version == %1)" ).arg( userver );
					}

					if ( !userver2.isEmpty() ) {
						if ( !vercond.isEmpty() )
							vercond += " && ";

						vercond += QString( "(User Version 2 == %1)" ).arg( userver2 );
					}

					if ( !vercond.isEmpty() ) {
						data.setVerCond( vercond );
					}

					// Set conditionless flag on data
					if ( cond.isEmpty() && vercond.isEmpty() && ver1.isEmpty() && ver2.isEmpty() )
						data.setIsConditionless( true );

					if ( data.name().isEmpty() || data.type().isEmpty() )
						err( tr( "add needs at least name and type attributes" ) );
				}
				break;
			default:
				err( tr( "only add tags allowed in block declaration" ) );
			}

			break;
		case tagEnum:
		case tagBitFlag:
			push( x );

			switch ( x ) {
			case tagOption:
				optId  = list.value( "name" );
				optVal = list.value( "value" );
				optTxt = QString();

				if ( optId.isEmpty() || optVal.isEmpty() )
					err( tr( "option defintion must have a name and a value" ) );

				bool ok;
				optVal.toUInt( &ok, 0 );

				if ( !ok )
					err( tr( "option value error (only integers please)" ) );

				break;
			default:
				err( tr( "only option tags allowed in enum declaration" ) );
			}

			break;
		default:
			err( tr( "error unhandled tag %1" ).arg( tagid ) );
			break;
		}

		return true;
	}

	//! Reimplemented from QXmlContentHandler
	/*!
	 * \param Namespace (unused)
	 * \param localName (unused)
	 * \param tagid Qualified name
	 */
	bool endElement( const QString &, const QString &, const QString & tagid ) override final
	{
		if ( depth <= 0 )
			err( tr( "mismatching end element tag for element %1" ).arg( tagid ) );

		Tag x = tags.value( tagid );

		if ( pop() != x )
			err( tr( "mismatching end element tag for element %1" ).arg( tagid ) );

		switch ( x ) {
		case tagCompound:
			if ( blk && !blk->id.isEmpty() && !blk->text.isEmpty() )
				NifValue::setTypeDescription( blk->id, blk->text );
			else if ( !typId.isEmpty() && !typTxt.isEmpty() )
				NifValue::setTypeDescription( typId, typTxt );

		case tagBlock:
			if ( blk ) {
				if ( blk->id.isEmpty() ) {
					blk = nullptr;
					err( tr( "invalid %1 declaration: name is empty" ).arg( tagid ) );
				}

				switch ( x ) {
				case tagCompound:
					NifModel::compounds.insert( blk->id, blk );
					break;
				case tagBlock:
					NifModel::blocks.insert( blk->id, blk );
					NifModel::blockHashes.insert( DJB1Hash(blk->id.toStdString().c_str()), blk );
					break;
				default:
					break;
				}

				blk = 0;
			}

			break;
		case tagAdd:
			if ( blk )
				blk->types.append( data );

			break;
		case tagOption:
			{
				bool ok;
				quint32 optValInt = optVal.toUInt( &ok, 0 );

				if ( !ok || !NifValue::registerEnumOption( typId, optId, optValInt, optTxt ) )
					err( tr( "failed to register enum option" ) );
			}
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

	//! Reimplemented from QXmlContentHandler
	/*!
	 * \param s The character data
	 */
	bool characters( const QString & s ) override final
	{
		switch ( current() ) {
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

	//! Checks that the type of the data is valid
	bool checkType( const NifData & d )
	{
		return ( NifModel::compounds.contains( d.type() )
		        || NifValue::type( d.type() ) != NifValue::tNone
		        || d.type() == "TEMPLATE"
		);
	}

	//! Checks that a template type is valid
	bool checkTemp( const NifData & d )
	{
		return ( d.temp().isEmpty()
		        || NifValue::type( d.temp() ) != NifValue::tNone
		        || d.temp() == "TEMPLATE"
		        || NifModel::blocks.contains( d.temp() )
		        || NifModel::compounds.contains( d.temp() )
		);
	}

	//! Reimplemented from QXmlContentHandler
	bool endDocument() override final
	{
		// make a rough check of the maps
		for ( const QString& key : NifModel::compounds.keys() ) {
			NifBlockPtr c = NifModel::compounds.value( key );
			for ( NifData data :c->types ) {
				if ( !checkType( data ) )
					err( tr( "compound type %1 refers to unknown type %2" ).arg( key, data.type() ) );

				if ( !checkTemp( data ) )
					err( tr( "compound type %1 refers to unknown template type %2" ).arg( key, data.temp() ) );

				if ( data.type() == key )
					err( tr( "compound type %1 contains itself" ).arg( key ) );
			}
		}

		for ( const QString& key : NifModel::blocks.keys() ) {
			NifBlockPtr b = NifModel::blocks.value( key );

			if ( !b->ancestor.isEmpty() && !NifModel::blocks.contains( b->ancestor ) )
				err( tr( "niobject %1 inherits unknown ancestor %2" ).arg( key, b->ancestor ) );

			if ( b->ancestor == key )
				err( tr( "niobject %1 inherits itself" ).arg( key ) );

			for ( const NifData& data : b->types ) {
				if ( !checkType( data ) )
					err( tr( "niobject %1 refers to unknown type %2" ).arg( key, data.type() ) );

				if ( !checkTemp( data ) )
					err( tr( "niobject %1 refers to unknown template type %2" ).arg( key, data.temp() ) );
			}
		}

		return true;
	}

	//! Reimplemented from QXmlContentHandler
	QString errorString() const override final
	{
		return errorStr;
	}
	//! Exception handler
	bool fatalError( const QXmlParseException & exception ) override final
	{
		if ( errorStr.isEmpty() )
			errorStr = "Syntax error";

		errorStr.prepend( tr( "%1 XML parse error (line %2): " ).arg( "NIF" ).arg( exception.lineNumber() ) );
		return false;
	}
};

// documented in nifmodel.h
bool NifModel::loadXML()
{
	QDir        dir( QCoreApplication::applicationDirPath() );
	QString     fname;
	QStringList xmlList( QStringList()
	                     << "nif.xml"
#ifdef Q_OS_LINUX
	                     << "/usr/share/nifskope/nif.xml"
#endif
	);
	for ( const QString& str : xmlList ) {
		if ( dir.exists( str ) ) {
			fname = dir.filePath( str );
			break;
		}
	}
	QString result = NifModel::parseXmlDescription( fname );

	if ( !result.isEmpty() ) {
		Message::append( tr( "<b>Error loading XML</b><br/>You will need to reinstall the XML and restart the application." ), result, QMessageBox::Critical );
		return false;
	}

	return true;
}

// documented in nifmodel.h
QString NifModel::parseXmlDescription( const QString & filename )
{
	QWriteLocker lck( &XMLlock );

	compounds.clear();
	blocks.clear();

	supportedVersions.clear();

	NifValue::initialize();

	QFile f( filename );

	if ( !f.exists() )
		return tr( "nif.xml could not be found. Please install it and restart the application." );

	if ( !f.open( QIODevice::ReadOnly | QIODevice::Text ) )
		return tr( "Couldn't open NIF XML description file: %1" ).arg( filename );

	NifXmlHandler handler;
	QXmlSimpleReader reader;
	reader.setContentHandler( &handler );
	reader.setErrorHandler( &handler );
	QXmlInputSource source( &f );
	reader.parse( source );

	if ( !handler.errorString().isEmpty() ) {
		compounds.clear();
		blocks.clear();
		supportedVersions.clear();
	}

	return handler.errorString();
}

