/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005-2012, NIF File Format Library and Tools
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

#include "nifvalue.h"
#include "nifmodel.h"
#include "config.h"
//#include "options.h"

#include <QHash>
#include <QSettings>

//! \file nifvalue.cpp NifValue, NifIStream, NifOStream, NifSStream

/*
 *  NifValue
 */

QHash<QString,NifValue::Type>	NifValue::typeMap;
QHash<QString,QString>		NifValue::typeTxt;
QHash<QString,NifValue::EnumOptions>	NifValue::enumMap;
QHash<QString,QString>		NifValue::aliasMap;

void NifValue::initialize()
{
	typeMap.clear();
	typeTxt.clear();
	
	typeMap.insert( "bool", NifValue::tBool );
	typeMap.insert( "byte", NifValue::tByte );
	typeMap.insert( "word", NifValue::tWord );
	typeMap.insert( "short", NifValue::tShort );
	typeMap.insert( "int", NifValue::tInt );
	typeMap.insert( "flags", NifValue::tFlags );
	typeMap.insert( "ushort", NifValue::tWord );
	typeMap.insert( "uint", NifValue::tUInt );
	typeMap.insert( "link", NifValue::tLink );
	typeMap.insert( "uplink", NifValue::tUpLink );
	typeMap.insert( "float", NifValue::tFloat );
	typeMap.insert( "sizedstring", NifValue::tSizedString );
	typeMap.insert( "text", NifValue::tText );
	typeMap.insert( "shortstring", NifValue::tShortString );
	typeMap.insert( "color3", NifValue::tColor3 );
	typeMap.insert( "color4", NifValue::tColor4 );
	typeMap.insert( "vector4", NifValue::tVector4 );
	typeMap.insert( "vector3", NifValue::tVector3 );
	typeMap.insert( "quat", NifValue::tQuat );
	typeMap.insert( "quaternion", NifValue::tQuat );
	typeMap.insert( "quaternion_wxyz", NifValue::tQuat );
	typeMap.insert( "quaternion_xyzw", NifValue::tQuatXYZW );
	typeMap.insert( "matrix33", NifValue::tMatrix );
	typeMap.insert( "matrix44", NifValue::tMatrix4 );
	typeMap.insert( "vector2", NifValue::tVector2 );
	typeMap.insert( "triangle", NifValue::tTriangle );
	typeMap.insert( "bytearray", NifValue::tByteArray );
	typeMap.insert( "bytematrix", NifValue::tByteMatrix);
	typeMap.insert( "fileversion", NifValue::tFileVersion );
	typeMap.insert( "headerstring", NifValue::tHeaderString );
	typeMap.insert( "linestring", NifValue::tLineString );
	typeMap.insert( "stringpalette", NifValue::tStringPalette );
	typeMap.insert( "stringoffset", NifValue::tStringOffset );
	typeMap.insert( "stringindex", NifValue::tStringIndex );
	typeMap.insert( "blocktypeindex", NifValue::tBlockTypeIndex );
	typeMap.insert( "char8string", NifValue::tChar8String );
	typeMap.insert( "string", NifValue::tString );
	typeMap.insert( "filepath", NifValue::tFilePath);
	typeMap.insert( "blob", NifValue::tBlob);
	
	enumMap.clear();
}

NifValue::Type NifValue::type( const QString & id )
{
	if ( typeMap.isEmpty() )
		initialize();
	
	if ( typeMap.contains( id ) )
		return typeMap[id];
	
	return tNone;
}

void NifValue::setTypeDescription( const QString & typId, const QString & txt )
{
	typeTxt[typId] = QString( txt ).replace( "<", "&lt;" ).replace( "\n", "<br/>" );
}

QString NifValue::typeDescription( const QString & typId )
{
	QString txt;
	if ( enumMap.contains( typId ) ) {
		txt = QString( "<p><b>%1 (%2)</b><p>%3</p>" ).arg( typId ).arg( aliasMap.value( typId ) ).arg( typeTxt.value( typId ) );
	} else {
		txt = QString( "<p><b>%1</b></p><p>%2</p>" ).arg( typId ).arg( typeTxt.value( typId ) );
	}
	
	if ( enumMap.contains( typId ) )
	{
		txt += "<p><table><tr><td><table>";
		QHashIterator< quint32, QPair< QString, QString > > it( enumMap[ typId ].o );
		int cnt = 0;
		while ( it.hasNext() )
		{
			if ( cnt++ > 30 )
			{
				cnt = 0;
				txt += "</table></td><td><table>";
			}
			it.next();
			txt += QString( "<tr><td>%2</td><td>%1</td><td>%3</td></tr>" ).arg( it.value().first ).arg( it.key() ).arg( it.value().second );
		}
		txt += "</table></td></tr></table></p>";
	}
	
	return txt;
}

bool NifValue::registerAlias( const QString & alias, const QString & original )
{
	if ( typeMap.isEmpty() )
		initialize();
	
	if ( typeMap.contains( original ) && ! typeMap.contains( alias ) )
	{
		typeMap.insert( alias, typeMap[original] );
		aliasMap.insert( alias, original );
		return true;
	}
	
	return false;
}

bool NifValue::registerEnumOption( const QString & eid, const QString & oid, quint32 oval, const QString & otxt )
{
	QHash< quint32, QPair< QString, QString > > & e = enumMap[eid].o;
	
	if ( e.contains( oval ) )
		return false;
	
	e[oval] = QPair<QString,QString>( oid, otxt );
	return true;
}

QStringList NifValue::enumOptions( const QString & eid )
{
	QStringList opts;
	if ( enumMap.contains( eid ) )
	{
		QHashIterator< quint32, QPair< QString, QString > > it( enumMap[ eid ].o );
		while ( it.hasNext() )
		{
			it.next();
			opts << it.value().first;
		}
	}
	return opts;
}

bool NifValue::registerEnumType( const QString & eid, EnumType eTyp )
{
	if ( enumMap.contains( eid ) )
		return false;
	enumMap[eid].t = eTyp;
	return true;
}

NifValue::EnumType NifValue::enumType( const QString & eid )
{
	return enumMap.contains(eid) ? enumMap.value( eid ).t : NifValue::eNone;
}

QString NifValue::enumOptionName( const QString & eid, quint32 val )
{
	if ( enumMap.contains( eid ) )
	{
		NifValue::EnumOptions& eo = enumMap[eid];
		if (eo.t == NifValue::eFlags) {
			QString text;
			quint32 val2 = 0;
			QHashIterator< quint32, QPair< QString, QString > > it( eo.o );
			while ( it.hasNext() ) {
				it.next();
				if (val & (1 << it.key())) { 
					val2 |= (1 << it.key());
					if (!text.isEmpty()) text += " | ";
					text += it.value().first;
				}
			}
			val2 = (val & ~val2);
			if (val2)
			{
				if (!text.isEmpty()) text += " | ";
				text += QString::number(val2, 16);
			}
			return text;
		}
		else if (eo.t == NifValue::eDefault)
		{
			if ( eo.o.contains(val) )
				return eo.o.value(val).first;
		}
		return QString::number(val);
	}
	return QString();
}

QString NifValue::enumOptionText( const QString & eid, quint32 val )
{
	return enumMap.value( eid ).o.value( val ).second;
}

quint32 NifValue::enumOptionValue( const QString & eid, const QString & oid, bool * ok )
{
	if ( enumMap.contains( eid ) )
	{
		EnumOptions& eo = enumMap[ eid ];
		QHashIterator< quint32, QPair< QString, QString > > it( eo.o );
		if (eo.t == NifValue::eFlags)
		{
			if ( ok ) *ok = true;
			quint32 value = 0;
			QStringList list = oid.split(QRegExp("\\s*\\|\\s*"), QString::SkipEmptyParts);
			QStringListIterator lit(list);
			while ( lit.hasNext() ) {
				QString str = lit.next();
				bool found = false;
				it.toFront();
				while ( it.hasNext() ) {
					it.next();
					if ( it.value().first == str ) {
						value |= (1 << it.key());
						found = true;
						break;
					}
				}
				if (!found)
					value |= str.toULong(&found, 0);
				if ( ok ) *ok &= found;
			}
			return value;
		}
		else if (eo.t == NifValue::eDefault)
		{
			while ( it.hasNext() )
			{
				it.next();
				if ( it.value().first == oid )
				{
					if ( ok ) *ok = true;
					return it.key();
				}
			}
		}
	}
	if ( ok ) *ok = false;
	return 0;
}

const NifValue::EnumOptions& NifValue::enumOptionData( const QString & eid )
{
	return enumMap[eid];
}

NifValue::NifValue( Type t ) : typ( tNone ), abstract( false )
{
	changeType( t );
}

NifValue::NifValue( const NifValue & other ) : typ( tNone )
{
	operator=( other );
}

NifValue::~NifValue()
{
	clear();
}

void NifValue::clear()
{
	switch ( typ )
	{
		case tVector4:
			delete static_cast<Vector3*>( val.data );
			break;
		case tVector3:
			delete static_cast<Vector3*>( val.data );
			break;
		case tVector2:
			delete static_cast<Vector2*>( val.data );
			break;
		case tMatrix:
			delete static_cast<Matrix*>( val.data );
			break;
		case tMatrix4:
			delete static_cast<Matrix4*>( val.data );
			break;
		case tQuat:
		case tQuatXYZW:
			delete static_cast<Quat*>( val.data );
			break;
		case tByteMatrix:
			delete static_cast<ByteMatrix*>( val.data );
			break;
		case tByteArray:
		case tStringPalette:
			delete static_cast<QByteArray*>( val.data );
			break;
		case tTriangle:
			delete static_cast<Triangle*>( val.data );
			break;
		case tString:
		case tSizedString:
		case tText:
		case tShortString:
		case tHeaderString:
		case tLineString:
		case tChar8String:
			delete static_cast<QString*>( val.data );
			break;
		case tColor3:
			delete static_cast<Color3*>( val.data );
			break;
		case tColor4:
			delete static_cast<Color4*>( val.data );
			break;
		case tBlob:
			delete static_cast<QByteArray*>( val.data );
			break;
		default:
			break;
	}
	typ = tNone;
	val.u32 = 0;
}

void NifValue::changeType( Type t )
{
	if ( typ == t )
		return;
	
	if ( typ != tNone )
		clear();
	
	switch ( ( typ = t ) )
	{
		case tLink:
		case tUpLink:
			val.i32 = -1;
			return;
		case tVector3:
			val.data = new Vector3();
			break;
		case tVector4:
			val.data = new Vector4();
			return;
		case tMatrix:
			val.data = new Matrix();
			return;
		case tMatrix4:
			val.data = new Matrix4();
			return;
		case tQuat:
		case tQuatXYZW:
			val.data = new Quat();
			return;
		case tVector2:
			val.data = new Vector2();
			return;
		case tTriangle:
			val.data = new Triangle();
			return;
		case tString:
		case tSizedString:
		case tText:
		case tShortString:
		case tHeaderString:
		case tLineString:
		case tChar8String:
			val.data = new QString();
			return;
		case tColor3:
			val.data = new Color3();
			return;
		case tColor4:
			val.data = new Color4();
			return;
		case tByteArray:
		case tStringPalette:
			val.data = new QByteArray();
			return;
		case tByteMatrix:
			val.data = new ByteMatrix();
			return;
		case tStringOffset:
		case tStringIndex:
			val.u32 = 0xffffffff;
			return;
		case tBlob:
			val.data = new QByteArray();
			return;
		default:
			val.u32 = 0;
			return;
	}
}

void NifValue::operator=( const NifValue & other )
{
	if ( typ != other.typ )
		changeType( other.typ );

	switch ( typ )
	{
		case tVector3:
			*static_cast<Vector3*>( val.data ) = *static_cast<Vector3*>( other.val.data );
			return;
		case tVector4:
			*static_cast<Vector4*>( val.data ) = *static_cast<Vector4*>( other.val.data );
			return;
		case tMatrix:
			*static_cast<Matrix*>( val.data ) = *static_cast<Matrix*>( other.val.data );
			return;
		case tMatrix4:
			*static_cast<Matrix4*>( val.data ) = *static_cast<Matrix4*>( other.val.data );
			return;
		case tQuat:
		case tQuatXYZW:
			*static_cast<Quat*>( val.data ) = *static_cast<Quat*>( other.val.data );
			return;
		case tVector2:
			*static_cast<Vector2*>( val.data ) = *static_cast<Vector2*>( other.val.data );
			return;
		case tString:
		case tSizedString:
		case tText:
		case tShortString:
		case tHeaderString:
		case tLineString:
		case tChar8String:
			*static_cast<QString*>( val.data ) = *static_cast<QString*>( other.val.data );
			return;
		case tColor3:
			*static_cast<Color3*>( val.data ) = *static_cast<Color3*>( other.val.data );
			return;
		case tColor4:
			*static_cast<Color4*>( val.data ) = *static_cast<Color4*>( other.val.data );
			return;
		case tByteArray:
		case tStringPalette:
			*static_cast<QByteArray*>( val.data ) = *static_cast<QByteArray*>( other.val.data );
			return;
		case tByteMatrix:
			*static_cast<ByteMatrix*>( val.data ) = *static_cast<ByteMatrix*>( other.val.data );
			return;
		case tTriangle:
			*static_cast<Triangle*>( val.data ) = *static_cast<Triangle*>( other.val.data );
			return;
		case tBlob:
			*static_cast<QByteArray*>( val.data ) = *static_cast<QByteArray*>( other.val.data );
			return;
		default:
			val = other.val;
			return;
	}
}

QVariant NifValue::toVariant() const
{
	QVariant v;
	v.setValue( *this );
	return v;
}

bool NifValue::fromVariant( const QVariant & var )
{
	if ( var.canConvert<NifValue>() )
	{
		operator=( var.value<NifValue>() );
		return true;
	}
	else if ( var.type() == QVariant::String )
	{
		return set<QString>( var.toString() );
	}
	return false;
}

bool NifValue::fromString( const QString & s )
{
	bool ok;
	switch ( typ )
	{
		case tBool:
			if ( s == "yes" || s == "true" )
			{
				val.u32 = 1;
				return true;
			}
			else if ( s == "no" || s == "false" )
			{
				val.u32 = 0;
				return true;
			}
		case tByte:
			val.u32 = 0;
			val.u08 = s.toUInt( &ok, 0 );
			return ok;
		case tWord:
		case tFlags:
		case tStringOffset:
		case tBlockTypeIndex:
		case tShort:
			val.u32 = 0;
			val.u16 = s.toShort( &ok, 0 );
			return ok;
		case tInt:
			val.i32 = s.toInt( &ok, 0 );
			return ok;
		case tUInt:
			val.u32 = s.toUInt( &ok, 0 );
			return ok;
		case tStringIndex:
			val.u32 = s.toUInt( &ok );
			return ok;
		case tLink:
		case tUpLink:
			val.i32 = s.toInt( &ok );
			return ok;
		case tFloat:
			val.f32 = s.toDouble( &ok );
			return ok;
		case tString:
		case tSizedString:
		case tText:
		case tShortString:
		case tHeaderString:
		case tLineString:
		case tChar8String:
			*static_cast<QString*>( val.data ) = s;
			return true;
		case tColor3:
			static_cast<Color3*>( val.data )->fromQColor( QColor( s ) );
			return true;
		case tColor4:
			static_cast<Color4*>( val.data )->fromQColor( QColor( s ) );
			return true;
		case tFileVersion:
			val.u32 = NifModel::version2number( s );
			return val.u32 != 0;
		case tVector2:
			static_cast<Vector2*>( val.data )->fromString( s );
			return true;
		case tVector3:
			static_cast<Vector3*>( val.data )->fromString( s );
			return true;
		case tVector4:
			static_cast<Vector4*>( val.data )->fromString( s );
			return true;
		case tQuat:
		case tQuatXYZW:
			static_cast<Quat*>( val.data )->fromString( s );
			return true;
		case tByteArray:
		case tByteMatrix:
		case tStringPalette:
		case tMatrix:
		case tMatrix4:
		case tTriangle:
		case tBlob:
		case tNone:
			return false;
		default:
			return false;
	}
	return false;
}

QString NifValue::toString() const
{
	switch ( typ )
	{
		case tBool:
			return ( val.u32 ? "yes" : "no" );
		case tByte:
		case tWord:
		case tFlags:
		case tStringOffset:
		case tBlockTypeIndex:
		case tUInt:
			return QString::number( val.u32 );
		case tStringIndex:
			return QString::number( val.u32 );
		case tShort:
			return QString::number( (short)val.u16 );
		case tInt:
			return QString::number( (int)val.u32 );
		case tLink:
		case tUpLink:
			return QString::number( val.i32 );
		case tFloat:
			return NumOrMinMax( val.f32, 'f', 4 );
		case tString:
		case tSizedString:
		case tText:
		case tShortString:
		case tHeaderString:
		case tLineString:
		case tChar8String:
			return *static_cast<QString*>( val.data );
		case tColor3:
		{
			Color3 * col = static_cast<Color3*>( val.data );
			return QString( "#%1%2%3" )
				.arg( (int) ( col->red() * 0xff ), 2, 16, QChar( '0' ) )
				.arg( (int) ( col->green() * 0xff ), 2, 16, QChar( '0' ) )
				.arg( (int) ( col->blue() * 0xff ), 2, 16, QChar( '0' ) );
		}
		case tColor4:
		{
			Color4 * col = static_cast<Color4*>( val.data );
			return QString( "#%1%2%3%4" )
				.arg( (int) ( col->red() * 0xff ), 2, 16, QChar( '0' ) )
				.arg( (int) ( col->green() * 0xff ), 2, 16, QChar( '0' ) )
				.arg( (int) ( col->blue() * 0xff ), 2, 16, QChar( '0' ) )
				.arg( (int) ( col->alpha() * 0xff ), 2, 16, QChar( '0' ) );
		}
		case tVector2:
		{
			Vector2 * v = static_cast<Vector2*>( val.data );
			
			return QString( "X %1 Y %2" )
				.arg( NumOrMinMax( (*v)[0], 'f', VECTOR_DECIMALS ) )
				.arg( NumOrMinMax( (*v)[1], 'f', VECTOR_DECIMALS ) );
		}
		case tVector3:
		{
			Vector3 * v = static_cast<Vector3*>( val.data );
			
			return QString( "X %1 Y %2 Z %3" )
				.arg( NumOrMinMax( (*v)[0], 'f', VECTOR_DECIMALS ) )
				.arg( NumOrMinMax( (*v)[1], 'f', VECTOR_DECIMALS ) )
				.arg( NumOrMinMax( (*v)[2], 'f', VECTOR_DECIMALS ) );
		}
		case tVector4:
		{
			Vector4 * v = static_cast<Vector4*>( val.data );
			
			return QString( "X %1 Y %2 Z %3 W %4" )
				.arg( NumOrMinMax( (*v)[0], 'f', VECTOR_DECIMALS ) )
				.arg( NumOrMinMax( (*v)[1], 'f', VECTOR_DECIMALS ) )
				.arg( NumOrMinMax( (*v)[2], 'f', VECTOR_DECIMALS ) )
				.arg( NumOrMinMax( (*v)[3], 'f', VECTOR_DECIMALS ) );
		}
		case tMatrix:
		case tQuat:
		case tQuatXYZW:
		{
			Matrix m;
			if( typ == tMatrix )
				m = *(static_cast<Matrix*>( val.data ));
			else
				m.fromQuat( *(static_cast<Quat*>( val.data )) );
			float x, y, z;
			QString pre, suf;
			if( !m.toEuler( x, y, z ) ) {
				pre = "(";
				suf = ")";
			}
			
			return ( pre + QString( "Y %1 P %2 R %3" ) + suf )
				.arg( NumOrMinMax( x / PI * 180, 'f', ROTATION_COARSE) )
				.arg( NumOrMinMax( y / PI * 180, 'f', ROTATION_COARSE ) )
				.arg( NumOrMinMax( z / PI * 180, 'f', ROTATION_COARSE ) );
		}
		case tMatrix4:
		{
			Matrix4 * m = static_cast<Matrix4*>( val.data );
			Matrix r; Vector3 t, s;
			m->decompose( t, r, s );
			float xr, yr, zr;
			r.toEuler( xr, yr, zr );
			xr *= 180 / PI;
			yr *= 180 / PI;
			zr *= 180 / PI;
			return QString( "Trans( X %1 Y %2 Z %3 ) Rot( Y %4 P %5 R %6 ) Scale( X %7 Y %8 Z %9 )" )
				.arg( t[0], 0, 'f', 3 )
				.arg( t[1], 0, 'f', 3 )
				.arg( t[2], 0, 'f', 3 )
				.arg( xr, 0, 'f', 3 )
				.arg( yr, 0, 'f', 3 )
				.arg( zr, 0, 'f', 3 )
				.arg( s[0], 0, 'f', 3 )
				.arg( s[1], 0, 'f', 3 )
				.arg( s[2], 0, 'f', 3 );
		}
		case tByteArray:
			return QString( "%1 bytes" )
				.arg( static_cast<QByteArray*>( val.data )->count() );
		case tStringPalette:
		{
			QByteArray * array = static_cast<QByteArray*>( val.data );
			QString s;
			while ( s.length() < array->count() )
			{
				s += & array->data()[s.length()];
				s += QChar( '|' );
			}
			return s;
		}
		case tByteMatrix:
		{
			ByteMatrix * array = static_cast<ByteMatrix*>( val.data );
			return QString( "%1 bytes  [%2 x %3]" )
				.arg( array->count() )
				.arg( array->count(0) )
				.arg( array->count(1) )
				;
		}
		case tFileVersion:
			return NifModel::version2string( val.u32 );
		case tTriangle:
		{
			Triangle * tri = static_cast<Triangle*>( val.data );
			return QString( "%1 %2 %3" )
				.arg( tri->v1() )
				.arg( tri->v2() )
				.arg( tri->v3() );
		}
		case tFilePath:
		{
			return *static_cast<QString*>( val.data );
		}
		case tBlob:
		{
			QByteArray * array = static_cast<QByteArray*>( val.data );
			return QString( "%1 bytes" )
				.arg( array->size() )
				;
		}
		default:
			return QString();
	}
}

QColor NifValue::toColor() const
{
	if ( type() == tColor3 ) 
		return static_cast<Color3*>( val.data )->toQColor();
	else if ( type() == tColor4 ) 
		return static_cast<Color4*>( val.data )->toQColor();
	else
		return QColor();
}

void NifOStream::init()
{
	bool32bit =  ( model->inherits( "NifModel" ) && model->getVersionNumber() <= 0x04000002 );
	linkAdjust = ( model->inherits( "NifModel" ) && model->getVersionNumber() < 0x0303000D );
	stringAdjust = ( model->inherits( "NifModel" ) && model->getVersionNumber() >= 0x14010003 );
}

bool NifIStream::read( NifValue & val )
{
	switch ( val.type() )
	{
		case NifValue::tBool:
		{
			val.val.u32 = 0;
			if ( bool32bit )
				*dataStream >> val.val.u32;
			else
				*dataStream >> val.val.u08;
			return ( dataStream->status() == QDataStream::Ok );
		}
		case NifValue::tByte:
		{
			val.val.u32 = 0;
			*dataStream >> val.val.u08;
			return ( dataStream->status() == QDataStream::Ok );
		}
		case NifValue::tWord:
		case NifValue::tShort:
		case NifValue::tFlags:
		case NifValue::tBlockTypeIndex:
		{
			val.val.u32 = 0;
			*dataStream >> val.val.u16;
			return ( dataStream->status() == QDataStream::Ok );
		}
		case NifValue::tStringOffset:
		case NifValue::tInt:
		case NifValue::tUInt:
		{
			*dataStream >> val.val.u32;
			return ( dataStream->status() == QDataStream::Ok );
		}
		case NifValue::tStringIndex:
		{
			*dataStream >> val.val.u32;
			return ( dataStream->status() == QDataStream::Ok );
		}
		case NifValue::tLink:
		case NifValue::tUpLink:
		{
			*dataStream >> val.val.i32;
			if ( linkAdjust )
				val.val.i32--;
			return ( dataStream->status() == QDataStream::Ok );
		}
		case NifValue::tFloat:
		{
			*dataStream >> val.val.f32;
			return ( dataStream->status() == QDataStream::Ok );
		}
		case NifValue::tVector3:
		{
			Vector3 * v = static_cast<Vector3*>( val.val.data );
			*dataStream >> *v;
			return ( dataStream->status() == QDataStream::Ok );
		}
		case NifValue::tVector4:
		{
			Vector4 * v = static_cast<Vector4*>( val.val.data );
			*dataStream >> *v;
			return ( dataStream->status() == QDataStream::Ok );
		}
		case NifValue::tTriangle:
		{
			Triangle * t = static_cast<Triangle*>( val.val.data );
			*dataStream >> *t;
			return ( dataStream->status() == QDataStream::Ok );
		}
		case NifValue::tQuat:
		{
			Quat * q = static_cast<Quat*>( val.val.data );
			*dataStream >> *q;
			return ( dataStream->status() == QDataStream::Ok );
		}
		case NifValue::tQuatXYZW:
		{
			Quat * q = static_cast<Quat*>( val.val.data );
			return device->read( (char *) &q->wxyz[1], 12 ) == 12 && device->read( (char *) q->wxyz, 4 ) == 4;
		}
		case NifValue::tMatrix:
			return device->read( (char *) static_cast<Matrix*>( val.val.data )->m, 36 ) == 36;
		case NifValue::tMatrix4:
			return device->read( (char *) static_cast<Matrix4*>( val.val.data )->m, 64 ) == 64;
		case NifValue::tVector2:
		{
			Vector2 * v = static_cast<Vector2*>( val.val.data );
			*dataStream >> *v;
			return ( dataStream->status() == QDataStream::Ok );
		}
		case NifValue::tColor3:
			return device->read( (char *) static_cast<Color3*>( val.val.data )->rgb, 12 ) == 12;
		case NifValue::tColor4:
		{
			Color4 * c = static_cast<Color4*>( val.val.data );
			*dataStream >> *c;
			return ( dataStream->status() == QDataStream::Ok );
		}
		case NifValue::tSizedString:
		{
			int len;
			//device->read( (char *) &len, 4 );
			*dataStream >> len;
			if ( len > maxLength || len < 0 ) { *static_cast<QString*>( val.val.data ) = tr("<string too long (0x%1)>").arg(len, 0, 16); return false; }
			QByteArray string = device->read( len );
			if ( string.size() != len ) return false;
			//string.replace( "\r", "\\r" );
			//string.replace( "\n", "\\n" );
			*static_cast<QString*>( val.val.data ) = QString( string );
		}	return true;
		case NifValue::tShortString:
		{
			unsigned char len;
			device->read( (char *) &len, 1 );
			QByteArray string = device->read( len );
			if ( string.size() != len ) return false;
			//string.replace( "\r", "\\r" );
			//string.replace( "\n", "\\n" );
			*static_cast<QString*>( val.val.data ) = QString( string );
		}	return true;
		case NifValue::tText:
		{
			int len;
			device->read( (char *) &len, 4 );
			if ( len > maxLength || len < 0 ) { *static_cast<QString*>( val.val.data ) = tr("<string too long>"); return false; }
			QByteArray string = device->read( len );
			if ( string.size() != len ) return false;
			*static_cast<QString*>( val.val.data ) = QString( string );
		}	return true;
		case NifValue::tByteArray:
		{
			int len;
			device->read( (char *) &len, 4 );
			if ( len < 0 )	return false;
			*static_cast<QByteArray*>( val.val.data ) = device->read( len );
			return static_cast<QByteArray*>( val.val.data )->count() == len;
		}
		case NifValue::tStringPalette:
		{
			int len;
			device->read( (char *) &len, 4 );
			if ( len > 0xffff || len < 0 )	return false;
			*static_cast<QByteArray*>( val.val.data ) = device->read( len );
			device->read( (char *) &len, 4 );
			return true;
		}
		case NifValue::tByteMatrix:
		{
			int len1, len2;
			device->read( (char *) &len1, 4 );
			device->read( (char *) &len2, 4 );
			if ( len1 < 0 || len2 < 0)	return false;
			int len = len1 * len2;
			ByteMatrix tmp(len1, len2);
			qint64 rlen =  device->read( tmp.data(), len );
			tmp.swap( *static_cast<ByteMatrix*>( val.val.data ) );
			return (rlen == len);
		}
		case NifValue::tHeaderString:
		{
			QByteArray string;
			int c = 0;
			char chr = 0;
			while ( c++ < 80 && device->getChar( &chr ) && chr != '\n' )
				string.append( chr );
			if ( c >= 80 ) return false;
			*static_cast<QString*>( val.val.data ) = QString( string );
			bool x = model->setHeaderString( QString( string ) );
			init();
			return x;
		}
		case NifValue::tLineString:
		{
			QByteArray string;
			int c = 0;
			char chr = 0;
			while ( c++ < 255 && device->getChar( &chr ) && chr != '\n' )
				string.append( chr );
			if ( c >= 255 ) return false;
			*static_cast<QString*>( val.val.data ) = QString( string );
			return true;
		}
		case NifValue::tChar8String:
		{
			QByteArray string;
			int c = 0;
			char chr = 0;
			while ( c++ < 8 && device->getChar( &chr ) )
				string.append( chr );
			if ( c > 9 ) return false;
			*static_cast<QString*>( val.val.data ) = QString( string );
			return true;
		}
		case NifValue::tFileVersion:
		{
			if ( device->read( (char *) &val.val.u32, 4 ) != 4 )	return false;
			//bool x = model->setVersion( val.val.u32 );
			//init();
			if( model->inherits( "NifModel" ) && model->getVersionNumber() >= 0x14000004 )
			{
				bool littleEndian;
				device->peek( (char *) &littleEndian, 1 );
				bigEndian = !littleEndian;
				if( bigEndian ) { dataStream->setByteOrder( QDataStream::BigEndian ); }
			}
			// hack for neosteam
			if ( val.val.u32 == 0x08F35232 )
			{
				val.val.u32 = 0x0a010000;
			}
			return true;
		}
		case NifValue::tString:
		{
			if (stringAdjust)
			{
				val.changeType(NifValue::tStringIndex);
				return device->read( (char *) &val.val.i32, 4 ) == 4;
			}
			else
			{
				val.changeType(NifValue::tSizedString);

				int len;
				device->read( (char *) &len, 4 );
				if ( len > maxLength || len < 0 ) { *static_cast<QString*>( val.val.data ) = tr("<string too long>"); return false; }
				QByteArray string = device->read( len );
				if ( string.size() != len ) return false;
				//string.replace( "\r", "\\r" );
				//string.replace( "\n", "\\n" );
				*static_cast<QString*>( val.val.data ) = QString( string );
				return true;
			}
		} 
		case NifValue::tFilePath:
		{
			if (stringAdjust)
			{
				val.changeType(NifValue::tStringIndex);
				return device->read( (char *) &val.val.i32, 4 ) == 4;
			}
			else
			{
				val.changeType(NifValue::tSizedString);

				int len;
				device->read( (char *) &len, 4 );
				if ( len > maxLength || len < 0 ) { *static_cast<QString*>( val.val.data ) = tr("<string too long>"); return false; }
				QByteArray string = device->read( len );
				if ( string.size() != len ) return false;
				*static_cast<QString*>( val.val.data ) = QString( string );
				return true;
			}
		}

		case NifValue::tBlob:
		{
			if ( val.val.data ) {
				QByteArray* array = static_cast<QByteArray*>( val.val.data );
				return device->read( array->data(), array->size() ) == array->size();
			}
			return false;
		}
		case NifValue::tNone:
			return true;
	}
	return false;
}

void NifIStream::init()
{
	bool32bit =  ( model->inherits( "NifModel" ) && model->getVersionNumber() <= 0x04000002 );
	linkAdjust = ( model->inherits( "NifModel" ) && model->getVersionNumber() < 0x0303000D );
	stringAdjust = ( model->inherits( "NifModel" ) && model->getVersionNumber() >= 0x14010003 );
	bigEndian = false; // set when tFileVersion is read
	dataStream = new QDataStream( device );
	dataStream->setByteOrder( QDataStream::LittleEndian );
#if QT_VERSION >= 0x040600
	dataStream->setFloatingPointPrecision( QDataStream::SinglePrecision );
#endif
	NIFSKOPE_QSETTINGS(cfg);
	maxLength = cfg.value("maximum string length", 0x8000).toInt();
	//maxLength = Options::maxStringLength();
}

bool NifOStream::write( const NifValue & val )
{
	switch ( val.type() )
	{
		case NifValue::tBool:
			if ( bool32bit )
				return device->write( (char *) &val.val.u32, 4 ) == 4;
			else
				return device->write( (char *) &val.val.u08, 1 ) == 1;
		case NifValue::tByte:
			return device->write( (char *) &val.val.u08, 1 ) == 1;
		case NifValue::tWord:
		case NifValue::tShort:
		case NifValue::tFlags:
		case NifValue::tBlockTypeIndex:
			return device->write( (char *) &val.val.u16, 2 ) == 2;
		case NifValue::tStringOffset:
		case NifValue::tInt:
		case NifValue::tUInt:
		case NifValue::tStringIndex:
			return device->write( (char *) &val.val.u32, 4 ) == 4;
		case NifValue::tFileVersion:
		{
			if( NifModel* mdl = static_cast<NifModel*>(const_cast<BaseModel*>(model)) )
			{
				QString headerString = mdl->getItem( mdl->getHeaderItem(), "Header String" )->value().toString();
				quint32 version;
				// hack for neosteam
				if( headerString.startsWith( "NS" ) )
				{
					version = 0x08F35232;
				}
				else
				{
					version = val.val.u32;
				}
				return device->write( (char *) &version, 4 ) == 4;
			}
			else
			{
				return device->write( (char *) &val.val.u32, 4 ) == 4;
			}
		}
		case NifValue::tLink:
		case NifValue::tUpLink:
			if ( ! linkAdjust )
			{
				return device->write( (char *) &val.val.i32, 4 ) == 4;
			}
			else
			{
				qint32 l = val.val.i32 + 1;
				return device->write( (char *) &l, 4 ) == 4;
			}
		case NifValue::tFloat:
			return device->write( (char *) &val.val.f32, 4 ) == 4;
		case NifValue::tVector3:
			return device->write( (char *) static_cast<Vector3*>( val.val.data )->xyz, 12 ) == 12;
		case NifValue::tVector4:
			return device->write( (char *) static_cast<Vector4*>( val.val.data )->xyzw, 16 ) == 16;
		case NifValue::tTriangle:
			return device->write( (char *) static_cast<Triangle*>( val.val.data )->v, 6 ) == 6;
		case NifValue::tQuat:
			return device->write( (char *) static_cast<Quat*>( val.val.data )->wxyz, 16 ) == 16;
		case NifValue::tQuatXYZW:
		{
			Quat * q = static_cast<Quat*>( val.val.data );
			return device->write( (char *) &q->wxyz[1], 12 ) == 12 && device->write( (char *) q->wxyz, 4 ) == 4;
		}
		case NifValue::tMatrix:
			return device->write( (char *) static_cast<Matrix*>( val.val.data )->m, 36 ) == 36;
		case NifValue::tMatrix4:
			return device->write( (char *) static_cast<Matrix4*>( val.val.data )->m, 64 ) == 64;
		case NifValue::tVector2:
			return device->write( (char *) static_cast<Vector2*>( val.val.data )->xy, 8 ) == 8;
		case NifValue::tColor3:
			return device->write( (char *) static_cast<Color3*>( val.val.data )->rgb, 12 ) == 12;
		case NifValue::tColor4:
			return device->write( (char *) static_cast<Color4*>( val.val.data )->rgba, 16 ) == 16;
		case NifValue::tSizedString:
		{
			QByteArray string = static_cast<QString*>( val.val.data )->toAscii();
			//string.replace( "\\r", "\r" );
			//string.replace( "\\n", "\n" );
			int len = string.size();
			if ( device->write( (char *) &len, 4 ) != 4 )
				return false;
			return device->write( (const char *) string, string.size() ) == string.size();
		}
		case NifValue::tShortString:
		{
			QByteArray string = static_cast<QString*>( val.val.data )->toAscii();
			string.replace( "\\r", "\r" );
			string.replace( "\\n", "\n" );
			if ( string.size() > 254 )	string.resize( 254 );
			unsigned char len = string.size() + 1;
			if ( device->write( (char *) &len, 1 ) != 1 )
				return false;
			return device->write( (const char *) string, len ) == len;
		}
		case NifValue::tText:
		{
			QByteArray string = static_cast<QString*>( val.val.data )->toAscii();
			int len = string.size();
			if ( device->write( (char *) &len, 4 ) != 4 )
				return false;
			return device->write( (const char *) string, string.size() ) == string.size();
		}
		case NifValue::tHeaderString:
		case NifValue::tLineString:
		{
			QByteArray string = static_cast<QString*>( val.val.data )->toAscii();
			if ( device->write( (const char *) string, string.length() ) != string.length() )
				return false;
			return ( device->write( "\n", 1 ) == 1 );
		}
		case NifValue::tChar8String:
		{
			QByteArray string = static_cast<QString*>( val.val.data )->toAscii();
			quint32 n = std::min<quint32>(8, string.length());
			if ( device->write( (const char *) string, n ) != n )
				return false;
			for ( quint32 i = n; i < 8; ++i)
				if ( device->write( "\0", 1 ) != 1 ) return false;
			return true;
		}
		case NifValue::tByteArray:
		{
			QByteArray * array = static_cast<QByteArray*>( val.val.data );
			int len = array->count();
			if ( device->write( (char *) &len, 4 ) != 4 )
				return false;
			return device->write( *array ) == len;
		}
		case NifValue::tStringPalette:
		{
			QByteArray * array = static_cast<QByteArray*>( val.val.data );
			int len = array->count();
			if ( device->write( (char *) &len, 4 ) != 4 )
				return false;
			if ( device->write( *array ) != len )
				return false;
			return device->write( (char *) &len, 4 ) == 4;
		}
		case NifValue::tByteMatrix:
		{
			ByteMatrix * array = static_cast<ByteMatrix*>( val.val.data );
			int len = array->count(0);
			if ( device->write( (char *) &len, 4 ) != 4 )
				return false;
			len = array->count(1);
			if ( device->write( (char *) &len, 4 ) != 4 )
				return false;
			len = array->count();
			return device->write(array->data(), len) == len;
		}
		case NifValue::tString:
		case NifValue::tFilePath:
		{
			if (stringAdjust)
			{
				if( val.val.u32 < 0x00010000 ) {
					return device->write( (char *) &val.val.u32, 4 ) == 4;
				} else {
					int value = 0;
					return device->write( (char *) &value, 4 ) == 4;
				}
			}
			else
			{
				QByteArray string;
				if( val.val.data != 0 )
				{
					string = static_cast<QString*>( val.val.data )->toAscii();
				}
				//string.replace( "\\r", "\r" );
				//string.replace( "\\n", "\n" );
				int len = string.size();
				if ( device->write( (char *) &len, 4 ) != 4 )
					return false;
				return device->write( (const char *) string, string.size() ) == string.size();
			}
		} 
		case NifValue::tBlob:
			if ( val.val.data ) {
				QByteArray* array = static_cast<QByteArray*>( val.val.data );
				return device->write( array->data(), array->size() ) == array->size();
			}
			return true;

		case NifValue::tNone:
			return true;
	}
	return false;
}

void NifSStream::init()
{
	bool32bit =  ( model->inherits( "NifModel" ) && model->getVersionNumber() <= 0x04000002 );
	stringAdjust = ( model->inherits( "NifModel" ) && model->getVersionNumber() >= 0x14010003 );
}

int NifSStream::size( const NifValue & val )
{
	switch ( val.type() )
	{
		case NifValue::tBool:
			if ( bool32bit )
				return 4;
			else
				return 1;
		case NifValue::tByte:
			return 1;
		case NifValue::tWord:
		case NifValue::tShort:
		case NifValue::tFlags:
		case NifValue::tBlockTypeIndex:
			return 2;
		case NifValue::tStringOffset:
		case NifValue::tInt:
		case NifValue::tUInt:
		case NifValue::tStringIndex:
		case NifValue::tFileVersion:
		case NifValue::tLink:
		case NifValue::tUpLink:
		case NifValue::tFloat:
			return 4;
		case NifValue::tVector3:
			return 12;
		case NifValue::tVector4:
			return 16;
		case NifValue::tTriangle:
			return 6;
		case NifValue::tQuat:
		case NifValue::tQuatXYZW:
			return 16;
		case NifValue::tMatrix:
			return 36;
		case NifValue::tMatrix4:
			return 64;
		case NifValue::tVector2:
			return 8;
		case NifValue::tColor3:
			return 12;
		case NifValue::tColor4:
			return 16;
		case NifValue::tSizedString:
		{
			QByteArray string = static_cast<QString*>( val.val.data )->toAscii();
			//string.replace( "\\r", "\r" );
			//string.replace( "\\n", "\n" );
			return 4 + string.size();
		}
		case NifValue::tShortString:
		{
			QByteArray string = static_cast<QString*>( val.val.data )->toAscii();
			//string.replace( "\\r", "\r" );
			//string.replace( "\\n", "\n" );
			if ( string.size() > 254 )	string.resize( 254 );
			return 1 + string.size() + 1;
		}
		case NifValue::tText:
		{
			QByteArray string = static_cast<QString*>( val.val.data )->toAscii();
			return 4 + string.size();
		}
		case NifValue::tHeaderString:
		case NifValue::tLineString:
		{
			QByteArray string = static_cast<QString*>( val.val.data )->toAscii();
			return string.length() + 1;
		}
		case NifValue::tChar8String:
		{
			return 8;
		}
		case NifValue::tByteArray:
		{
			QByteArray * array = static_cast<QByteArray*>( val.val.data );
			return 4 + array->count();
		}
		case NifValue::tStringPalette:
		{
			QByteArray * array = static_cast<QByteArray*>( val.val.data );
			return 4 + array->count() + 4;
		}
		case NifValue::tByteMatrix:
		{
			ByteMatrix * array = static_cast<ByteMatrix*>( val.val.data );
			return 4 + 4 + array->count();
		}
		case NifValue::tString:
		case NifValue::tFilePath:
		{
			if (stringAdjust)
			{
				return 4;
			}
			else
			{
				QByteArray string = static_cast<QString*>( val.val.data )->toAscii();
				//string.replace( "\\r", "\r" );
				//string.replace( "\\n", "\n" );
				return 4 + string.size();
			}
		}

		case NifValue::tBlob:
			if ( val.val.data ) {
				QByteArray* array = static_cast<QByteArray*>( val.val.data );
				return array->size();
			}
			return 0;

		case NifValue::tNone:
			return 0;
	}
	return 0;
}

