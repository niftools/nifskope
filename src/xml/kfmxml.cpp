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
#include "model/kfmmodel.h"

#include <QtXml> // QXmlDefaultHandler Inherited
#include <QCoreApplication>
#include <QMessageBox>

#define err( X ) { errorStr = X; return false; }


QReadWriteLock KfmModel::XMLlock;
QList<quint32>                  KfmModel::supportedVersions;
QHash<QString, NifBlockPtr>        KfmModel::compounds;

class KfmXmlHandler final : public QXmlDefaultHandler
{
	Q_DECLARE_TR_FUNCTIONS( KfmXmlHandler )

public:
	KfmXmlHandler()
	{
	}

	int depth = 0;
	int stack[10] = {0};
	QStringList elements = { "niftoolsxml", "version", "compound", "add" };
	QString errorStr;

	NifBlockPtr blk = nullptr;

	int current() const
	{
		return stack[depth - 1];
	}
	void push( int x )
	{
		stack[depth++] = x;
	}
	int pop()
	{
		return stack[--depth];
	}

	bool startElement( const QString &, const QString &, const QString & name, const QXmlAttributes & list ) override final
	{
		if ( depth >= 8 )
			err( tr( "error maximum nesting level exceeded" ) );

		int x = elements.indexOf( name );

		if ( x < 0 )
			err( tr( "error unknown element" ) + " '" + name + "'" );

		if ( depth == 0 ) {
			if ( x != 0 )
				err( tr( "this is not a niftoolsxml file" ) );

			push( x );
			return true;
		}

		int v;

		switch ( current() ) {
		case 0:

			if ( !( x == 1 || x == 2 ) )
				err( tr( "expected compound or version got %1 instead" ).arg( name ) );

			push( x );

			switch ( x ) {
			case 1:
				v = KfmModel::version2number( list.value( "num" ).trimmed() );

				if ( v != 0 && !list.value( "num" ).isEmpty() )
					KfmModel::supportedVersions.append( v );
				else
					err( tr( "invalid version string" ) );

				break;
			case 2:

				if ( x == 2 && NifValue::isValid( NifValue::type( list.value( "name" ) ) ) )
					err( tr( "compound %1 is already registered as internal type" ).arg( list.value( "name" ) ) );

				if ( !blk )
					blk = NifBlockPtr( new NifBlock );

				blk->id = list.value( "name" );
				break;
			}

			break;
		case 1:
			err( tr( "version tag must not contain any sub tags" ) );
			break;
		case 2:

			if ( x == 3 ) {
				QString type = list.value( "type" );
				QString tmpl = list.value( "template" );
				QString arr1 = list.value( "arr1" );
				QString arr2 = list.value( "arr2" );
				QString cond = list.value( "cond" );
				QString ver1 = list.value( "ver1" );
				QString ver2 = list.value( "ver2" );
				QString abs = list.value( "abstract" );

				NifData data(
				    list.value( "name" ),
					type,
					tmpl,
				    NifValue( NifValue::type( type ) ),
				    list.value( "arg" ),
					arr1,
					arr2,
					cond,
				    KfmModel::version2number( ver1 ),
				    KfmModel::version2number( ver2 )
				);

				bool isTemplated = (type == "TEMPLATE" || tmpl == "TEMPLATE");
				bool isCompound = KfmModel::compounds.contains( type );
				bool isArray = !arr1.isEmpty();
				bool isMultiArray = !arr2.isEmpty();

				data.setAbstract( abs == "1" );
				data.setTemplated( isTemplated );
				data.setIsCompound( isCompound );
				data.setIsArray( isArray );
				data.setIsMultiArray( isMultiArray );

				if ( data.name().isEmpty() || data.type().isEmpty() )
					err( tr( "add needs at least name and type attributes" ) );

				if ( blk )
					blk->types.append( data );
			} else {
				err( tr( "only add tags allowed in compound type declaration" ) );
			}

			push( x );
			break;
		default:
			err( tr( "error unhandled tag %1 in %2" ).arg( name, elements.value( current() ) ) );
			break;
		}

		return true;
	}

	bool endElement( const QString &, const QString &, const QString & name ) override final
	{
		if ( depth <= 0 )
			err( tr( "mismatching end element tag for element " ) + name );

		int x = elements.indexOf( name );

		if ( pop() != x )
			err( tr( "mismatching end element tag for element " ) + elements.value( current() ) );

		switch ( x ) {
		case 2:

			if ( blk ) {
				if ( !blk->id.isEmpty() ) {
					switch ( x ) {
					case 2:
						KfmModel::compounds.insert( blk->id, blk ); break;
					}

					blk = nullptr;
				} else {
					blk = nullptr;
					err( tr( "invalid %1 declaration: name is empty" ).arg( elements.value( x ) ) );
				}
			}

			break;
		}

		return true;
	}

	bool checkType( const NifData & data )
	{
		return KfmModel::compounds.contains( data.type() ) || NifValue::type( data.type() ) != NifValue::tNone || data.type() == "TEMPLATE";
	}

	bool checkTemp( const NifData & data )
	{
		return data.temp().isEmpty() || NifValue::type( data.temp() ) != NifValue::tNone || data.temp() == "TEMPLATE";
	}

	bool endDocument() override final
	{
		// make a rough check of the maps
		for ( const QString& key : KfmModel::compounds.keys() ) {
			NifBlockPtr c = KfmModel::compounds.value( key );
			for ( const NifData& data : c->types ) {
				if ( !checkType( data ) )
					err( tr( "compound type %1 referes to unknown type %2" ).arg( key, data.type() ) );

				if ( !checkTemp( data ) )
					err( tr( "compound type %1 refers to unknown template type %2" ).arg( key, data.temp() ) );

				if ( data.type() == key )
					err( tr( "compound type %1 contains itself" ).arg( key ) );
			}
		}
		return true;
	}

	QString errorString() const override final
	{
		return errorStr;
	}
	bool fatalError( const QXmlParseException & exception ) override final
	{
		if ( errorStr.isEmpty() )
			errorStr = tr( "Syntax error" );

		errorStr.prepend( tr( "%1 XML parse error (line %2): " ).arg( "KFM" ).arg( exception.lineNumber() ) );
		return false;
	}
};

bool KfmModel::loadXML()
{
	QDir dir( QCoreApplication::applicationDirPath() );
	QString fname;
	QStringList xmlList( QStringList()
	                     << "kfm.xml"
#ifdef Q_OS_LINUX
	                     << "/usr/share/nifskope/kfm.xml"
#endif
	);
	for ( const QString& str : xmlList ) {
		if ( dir.exists( str ) ) {
			fname = dir.filePath( str );
			break;
		}
	}
	QString result = KfmModel::parseXmlDescription( fname );

	if ( !result.isEmpty() ) {
		Message::append( tr( "<b>Error loading XML</b><br/>You will need to reinstall the XML and restart the application." ), result, QMessageBox::Critical );
		return false;
	}

	return true;
}

QString KfmModel::parseXmlDescription( const QString & filename )
{
	QWriteLocker lck( &XMLlock );

	compounds.clear();
	supportedVersions.clear();

	QFile f( filename );

	if ( !f.exists() )
		return tr( "kfm.xml could not be found. Please install it and restart the application." );

	if ( !f.open( QIODevice::ReadOnly | QIODevice::Text ) )
		return tr( "Couldn't open KFM XML description file: %1" ).arg( filename );

	KfmXmlHandler handler;
	QXmlSimpleReader reader;
	reader.setContentHandler( &handler );
	reader.setErrorHandler( &handler );
	QXmlInputSource source( &f );
	reader.parse( source );

	if ( !handler.errorString().isEmpty() ) {
		compounds.clear();
		supportedVersions.clear();
	}

	return handler.errorString();
}

