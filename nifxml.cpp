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

#include <QtXml>
#include <QColor>

#define err( X ) { errorStr = X; return false; }

QStringList						NifModel::internalTypes;

QHash<QString,NifBasicType*>	NifModel::types;
QHash<QString,NifBlock*>		NifModel::compounds;
QHash<QString,NifBlock*>		NifModel::ancestors;
QHash<QString,NifBlock*>		NifModel::blocks;

class NifXmlHandler : public QXmlDefaultHandler
{
public:
	NifXmlHandler()
	{
		depth = 0;
		elements << "niflotoxml" << "type" << "compound" << "ancestor" << "niblock" << "add" << "inherit";
		typ = 0;
		blk = 0;
	}

	int depth;
	int stack[10];
	QStringList elements;
	QString errorStr;
	
	NifBasicType	* typ;
	NifBlock		* blk;
	
	int current() const
	{
		return stack[depth-1];
	}
	void push( int x )
	{
		stack[depth++] = x;
	}
	int pop()
	{
		return stack[--depth];
	}
	
	QVariant convertToType( const QString & vstring, int type )
	{
		switch ( type )
		{
			case NifModel::it_uint8:
			case NifModel::it_uint16:
			case NifModel::it_uint32:
			case NifModel::it_int8:
			case NifModel::it_int16:
			case NifModel::it_int32:
				return vstring.toInt();
			case NifModel::it_float:
				return vstring.toDouble();
			case NifModel::it_string:
				return vstring;
			case NifModel::it_color3f:
			case NifModel::it_color4f:
				return QColor( vstring );
			default:
				errorStr = "can't convert unknown internal type " + type;
				return QVariant();
		}
	}
	
	bool startElement( const QString &, const QString &, const QString & name, const QXmlAttributes & list )
	{
		if ( depth >= 8 )	err( "error maximum nesting level exceeded" );
		
		int x = elements.indexOf( name );
		if ( x < 0 )	err( "error unknown element '" + name + "'" );
		
		if ( depth == 0 )
		{
			if ( x != 0 )	err( "this is not a niflotoxml file" );
			push( x );
			return true;
		}
		
		switch ( current() )
		{
			case 0:
				if ( ! ( x >= 1 && x <= 4 ) )	err( "expected type, compound, ancestor or niblock  got " + name + " instead" );
				push( x );
				switch ( x )
				{
					case 1:
						{
							int intTyp = NifModel::internalTypes.indexOf( list.value( "type" ) );
							if ( intTyp < 0 )	err( "type declaration must name a valid internal type" );
							if ( ! typ ) typ = new NifBasicType;
							typ->id = list.value( "name" ).toLower();
							typ->internalType = intTyp;
							typ->display = list.value( "display" ).toLower();
							typ->value = convertToType( list.value( "value" ), typ->internalType );
							typ->ver1 = NifModel::version2number( list.value( "ver1" ) );
							typ->ver2 = NifModel::version2number( list.value( "ver2" ) );
						}
						break;
					case 2:
					case 3:
					case 4:
						if ( ! blk ) blk = new NifBlock;
						blk->id = list.value( "name" );
						break;
				}
				break;
			case 1:
				err( "types only contain a description" );
				break;
			case 2:
				if ( x != 5 )	err( "only add tags allowed in compound type declaration" );
			case 3:
			case 4:
				if ( ! ( x == 5 || x == 6 ) )	err( "only add and inherit tags allowed in " + elements.value( x ) + " declaration" );
				if ( x == 5 )
				{
					NifData		data(
						list.value( "name" ).toLower().toAscii(),
						list.value( "type" ).toLower().toAscii(),
						QVariant(),
						list.value( "arg" ).toLower().toAscii(),
						list.value( "arr1" ).toLower().toAscii(),
						list.value( "arr2" ).toLower().toAscii(),
						list.value( "cond" ).toLower().toAscii(),
						NifModel::version2number( list.value( "ver1" ) ),
						NifModel::version2number( list.value( "ver2" ) )
					);
					if ( data.name().isEmpty() || data.type().isEmpty() ) err( "add needs at least name and type attributes" );
					if ( blk )	blk->types.append( data );
				}
				else if ( x == 6 )
				{
					QString n = list.value( "name" );
					if ( n.isEmpty() )	err( "inherit needs name attribute" );
					if ( blk ) blk->ancestors.append( n );
				}
				push( x );
				break;
			default:
				err( "error unhandled tag " + name + " in " + elements.value( current() ) );
				break;
		}
		return true;
	}
	
	bool endElement( const QString &, const QString &, const QString & name )
	{
		if ( depth <= 0 )		err( "mismatching end element tag for element " + name );
		int x = elements.indexOf( name );
		if ( pop() != x )		err( "mismatching end element tag for element " + elements.value( current() ) );
		switch ( x )
		{
			case 1:
				if ( typ )
				{
					if ( ! typ->id.isEmpty() )
					{
						NifModel::types.insertMulti( typ->id, typ );
						typ = 0;
					}
					else
					{
						delete typ;
						typ = 0;
						err( "invalid type declaration: specify at least name and type" );
					}
				}
				break;
			case 2:
			case 3:
			case 4:
				if ( blk )
				{
					if ( ! blk->id.isEmpty() )
					{
						switch ( x )
						{
							case 2: NifModel::compounds.insert( blk->id, blk );	break;
							case 3: NifModel::ancestors.insert( blk->id, blk );	break;
							case 4: NifModel::blocks.insert( blk->id, blk );		break;
						}
						blk = 0;
					}
					else
					{
						delete blk;
						blk = 0;
						err( "invalid " + elements.value( x ) + " declaration: name is empty" );
					}
				}
				break;
		}
		return true;
	}
	
	bool characters( const QString & s )
	{
		//if ( current() == 1 && typ )	typ->text = s.trimmed();
		return true;
	}
	
	bool endDocument()
	{	// make a rough check of the maps
		foreach ( QString key, NifModel::compounds.keys() )
		{
			NifBlock * c = NifModel::compounds.value( key );
			foreach ( NifData data, c->types )
			{
				if ( ! ( NifModel::compounds.contains( data.type() ) || NifModel::types.contains( data.type() ) ) )
					err( "compound type " + key + " referes to unknown type " + data.type() );
				if ( data.type() == key )
					err( "compound type " + key + " contains itself" );
			}
			for ( QList<NifData>::iterator it = c->types.begin(); it != c->types.end(); ++it )
				if ( NifModel::types.contains( it->type() ) )
					if ( ! it->value().isValid() ) it->setValue( NifModel::types.value( it->type() )->value );
		}
		
		foreach ( QString key, NifModel::ancestors.keys() )
		{
			NifBlock * blk = NifModel::ancestors.value( key );
			foreach ( QString a, blk->ancestors )
			{
				if ( ! NifModel::ancestors.contains( a ) )
					err( "ancestor block " + key + " inherits unknown ancestor " + a );
				if ( a == key )
					err( "ancestor block " + key + " inherits itself" );
			}
			foreach ( NifData data, blk->types )
			{
				if ( ! ( NifModel::compounds.contains( data.type() ) || NifModel::types.contains( data.type() ) ) )
					err( "ancestor block " + key + " referes to unknown type " + data.type() );
			}
			for ( QList<NifData>::iterator it = blk->types.begin(); it != blk->types.end(); ++it )
				if ( NifModel::types.contains( it->type() ) )
					if ( ! it->value().isValid() ) it->setValue( NifModel::types.value( it->type() )->value );
		}
		
		foreach ( QString key, NifModel::blocks.keys() )
		{
			NifBlock * blk = NifModel::blocks.value( key );
			foreach ( QString a, blk->ancestors )
			{
				if ( ! NifModel::ancestors.contains( a ) )
					err( "niblock " + key + " inherits unknown ancestor " + a );
			}
			foreach ( NifData data, blk->types )
			{
				if ( ! ( NifModel::compounds.contains( data.type() ) || NifModel::types.contains( data.type() ) ) )
					err( "compound type " + key + " referres to unknown type " + data.type() );
			}
			for ( QList<NifData>::iterator it = blk->types.begin(); it != blk->types.end(); ++it )
				if ( NifModel::types.contains( it->type() ) )
					if ( ! it->value().isValid() ) it->setValue( NifModel::types.value( it->type() )->value );
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

QString NifModel::parseXmlDescription( const QString & filename )
{
	qDeleteAll( types );		types.clear();
	qDeleteAll( compounds );	compounds.clear();
	qDeleteAll( ancestors );	ancestors.clear();
	qDeleteAll( blocks );		blocks.clear();
	
	internalTypes.clear();
	internalTypes
		<< "uint8" << "uint16" << "uint32"
		<< "int8" << "int16" << "int32"
		<< "float" << "string"
		<< "color3f" << "color4f";
	
	QFile f( filename );
	if ( ! f.open( QIODevice::ReadOnly | QIODevice::Text ) )
	{
		f.setFileName( ":/res/NifSkope.xml" );
		if ( ! f.open( QIODevice::ReadOnly | QIODevice::Text ) )
			return QString( "error: couldn't open xml description file: " + filename );
	}
	
	NifXmlHandler handler;
	QXmlSimpleReader reader;
	reader.setContentHandler( &handler );
	reader.setErrorHandler( &handler );
	QXmlInputSource source( &f );
	reader.parse( source );
	return handler.errorString();
}

