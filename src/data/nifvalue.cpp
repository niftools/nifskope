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

#include "nifvalue.h"

#include "model/nifmodel.h"

#include <QRegularExpression>
#include <QSettings>


//! @file nifvalue.cpp NifValue

QHash<QString, NifValue::Type>        NifValue::typeMap;
QHash<QString, QString>               NifValue::typeTxt;
QHash<QString, NifValue::EnumOptions> NifValue::enumMap;
QHash<QString, QString>               NifValue::aliasMap;


static int OPT_PER_LINE = -1;

/*
 *  NifValue
 */

NifValue::NifValue( Type t )
{
	changeType( t );
}

NifValue::NifValue( const NifValue & other )
{
	operator=(other);
}

NifValue::~NifValue()
{
	clear();
}

void NifValue::initialize()
{
	typeMap.clear();
	typeTxt.clear();

	typeMap.insert( "bool",   NifValue::tBool );
	typeMap.insert( "byte",   NifValue::tByte );
	typeMap.insert( "char",   NifValue::tByte );
	typeMap.insert( "word",   NifValue::tWord );
	typeMap.insert( "short",  NifValue::tShort );
	typeMap.insert( "int",    NifValue::tInt );
	typeMap.insert( "Flags",  NifValue::tFlags );
	typeMap.insert( "ushort", NifValue::tWord );
	typeMap.insert( "uint",   NifValue::tUInt );
	typeMap.insert( "ulittle32", NifValue::tULittle32 );
	typeMap.insert( "Ref",    NifValue::tLink );
	typeMap.insert( "Ptr",    NifValue::tUpLink );
	typeMap.insert( "float",  NifValue::tFloat );
	typeMap.insert( "SizedString", NifValue::tSizedString );
	typeMap.insert( "Text",        NifValue::tText );
	typeMap.insert( "ShortString", NifValue::tShortString );
	typeMap.insert( "Color3",      NifValue::tColor3 );
	typeMap.insert( "Color4",      NifValue::tColor4 );
	typeMap.insert( "Vector4",     NifValue::tVector4 );
	typeMap.insert( "Vector3",     NifValue::tVector3 );
	typeMap.insert( "TBC",         NifValue::tVector3 );
	typeMap.insert( "Quaternion",  NifValue::tQuat );
	typeMap.insert( "QuaternionWXYZ", NifValue::tQuat );
	typeMap.insert( "QuaternionXYZW", NifValue::tQuatXYZW );
	typeMap.insert( "hkQuaternion",   NifValue::tQuatXYZW );
	typeMap.insert( "Matrix33",       NifValue::tMatrix );
	typeMap.insert( "Matrix44",       NifValue::tMatrix4 );
	typeMap.insert( "Vector2",        NifValue::tVector2 );
	typeMap.insert( "TexCoord",       NifValue::tVector2 );
	typeMap.insert( "Triangle",       NifValue::tTriangle );
	typeMap.insert( "ByteArray",      NifValue::tByteArray );
	typeMap.insert( "ByteMatrix",     NifValue::tByteMatrix );
	typeMap.insert( "FileVersion",    NifValue::tFileVersion );
	typeMap.insert( "HeaderString",   NifValue::tHeaderString );
	typeMap.insert( "LineString",     NifValue::tLineString );
	typeMap.insert( "StringPalette",  NifValue::tStringPalette );
	typeMap.insert( "StringOffset",   NifValue::tStringOffset );
	typeMap.insert( "StringIndex",    NifValue::tStringIndex );
	typeMap.insert( "BlockTypeIndex", NifValue::tBlockTypeIndex );
	typeMap.insert( "char8string",    NifValue::tChar8String );
	typeMap.insert( "string",   NifValue::tString );
	typeMap.insert( "FilePath", NifValue::tFilePath );
	typeMap.insert( "blob",     NifValue::tBlob );
	typeMap.insert( "hfloat",   NifValue::tHfloat );
	typeMap.insert( "HalfVector3", NifValue::tHalfVector3 );
	typeMap.insert( "ByteVector3", NifValue::tByteVector3 );
	typeMap.insert( "HalfVector2", NifValue::tHalfVector2 );
	typeMap.insert( "HalfTexCoord", NifValue::tHalfVector2 );
	typeMap.insert( "ByteColor4", NifValue::tByteColor4 );
	typeMap.insert( "BSVertexDesc", NifValue::tBSVertexDesc );

	enumMap.clear();
}

NifValue::Type NifValue::type( const QString & id )
{
	if ( typeMap.isEmpty() )
		initialize();

	return typeMap.value( id, tNone );
}

void NifValue::setTypeDescription( const QString & typId, const QString & txt )
{
	typeTxt[typId] = QString( txt ).replace( "<", "&lt;" ).replace( "\n", "<br/>" );
}

QString NifValue::typeDescription( const QString & typId )
{
	if ( !enumMap.contains( typId ) )
		return QString( "<p><b>%1</b></p><p>%2</p>" ).arg( typId, typeTxt.value( typId ) );

	// Cache the generated HTML description
	static QHash<QString, QString> txtCache;

	if ( txtCache.contains( typId ) )
		return txtCache[typId];
	
	QString txt = QString( "<p><b>%1 (%2)</b><p>%3</p>" ).arg( typId, aliasMap.value( typId ), typeTxt.value( typId ) );

	txt += "<table><tr><td><table>";
	QMapIterator<quint32, QPair<QString, QString> > it( enumMap[ typId ].o );
	int cnt = 0;

	while ( it.hasNext() ) {
		if ( cnt++ > 31 ) {
			cnt  = 0;
			txt += "</table></td><td><table>";
		}

		it.next();
		txt += QString( "<tr><td><p style='white-space:pre'>%2 %1</p></td><td><p style='white-space:pre'>%3</p></td></tr>" )
		       .arg( it.value().first ).arg( it.key() ).arg( it.value().second );
	}

	txt += "</table></td></tr></table>";

	txtCache.insert( typId, txt );

	return txt;
}

bool NifValue::registerAlias( const QString & alias, const QString & original )
{
	if ( typeMap.isEmpty() )
		initialize();

	if ( typeMap.contains( original ) && !typeMap.contains( alias ) ) {
		typeMap.insert( alias, typeMap[original] );
		aliasMap.insert( alias, original );
		return true;
	}

	return false;
}

bool NifValue::registerEnumOption( const QString & eid, const QString & oid, quint32 oval, const QString & otxt )
{
	QMap<quint32, QPair<QString, QString> > & e = enumMap[eid].o;

	if ( e.contains( oval ) )
		return false;

	e[oval] = QPair<QString, QString>( oid, otxt );
	return true;
}

QStringList NifValue::enumOptions( const QString & eid )
{
	QStringList opts;

	if ( enumMap.contains( eid ) ) {
		QMapIterator<quint32, QPair<QString, QString> > it( enumMap[ eid ].o );

		while ( it.hasNext() ) {
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
	return (enumMap.contains( eid )) ? enumMap[eid].t : EnumType::eNone;
}

QString NifValue::enumOptionName( const QString & eid, quint32 val )
{
	if ( enumMap.contains( eid ) ) {
		NifValue::EnumOptions & eo = enumMap[eid];

		if ( eo.t == NifValue::eFlags ) {
			QString text;
			quint32 val2 = 0;

			if ( OPT_PER_LINE == -1 ) {
				QSettings settings;
				OPT_PER_LINE = settings.value( "Settings/UI/Options Per Line", 3 ).toInt();
			}

			int opt = 0;
			auto it = eo.o.constBegin();
			while ( it != eo.o.constEnd() ) {
				if ( val & ( 1 << it.key() ) ) {
					val2 |= ( 1 << it.key() ); 

					if ( !text.isEmpty() )
						text += " | ";

					if ( it != eo.o.constEnd() && opt != 0 && opt % OPT_PER_LINE == 0 )
						text += "\n";

					text += it.value().first;

					opt++;
				}

				it++;
			}

			// Append any leftover value not covered by enums
			val2 = (val & ~val2);
			if ( val2 ) {
				if ( !text.isEmpty() )
					text += " | ";

				text += QString::number( val2, 16 );
			}

			return text;
		} else if ( eo.t == NifValue::eDefault ) {
			if ( eo.o.contains( val ) )
				return eo.o.value( val ).first;
		}

		return QString::number( val );
	}

	return QString();
}

QString NifValue::enumOptionText( const QString & eid, quint32 val )
{
	return enumMap.value( eid ).o.value( val ).second;
}

quint32 NifValue::enumOptionValue( const QString & eid, const QString & oid, bool * ok )
{
	if ( enumMap.contains( eid ) ) {
		EnumOptions & eo = enumMap[ eid ];
		QMapIterator<quint32, QPair<QString, QString> > it( eo.o );

		if ( eo.t == NifValue::eFlags ) {
			if ( ok )
				*ok = true;

			quint32 value = 0;
			QStringList list = oid.split( QRegularExpression( "\\s*\\|\\s*" ), QString::SkipEmptyParts );
			QStringListIterator lit( list );

			while ( lit.hasNext() ) {
				QString str = lit.next();
				bool found  = false;
				it.toFront();

				while ( it.hasNext() ) {
					it.next();

					if ( it.value().first == str ) {
						value |= ( 1 << it.key() );
						found  = true;
						break;
					}
				}

				if ( !found )
					value |= str.toULong( &found, 0 );

				if ( ok )
					*ok &= found;
			}

			return value;
		} else if ( eo.t == NifValue::eDefault ) {
			while ( it.hasNext() ) {
				it.next();

				if ( it.value().first == oid ) {
					if ( ok )
						*ok = true;

					return it.key();
				}
			}
		}
	}

	if ( ok )
		*ok = false;

	return 0;
}

const NifValue::EnumOptions & NifValue::enumOptionData( const QString & eid )
{
	return enumMap[eid];
}

void NifValue::clear()
{
	switch ( typ ) {
	case tVector4:
		delete static_cast<Vector4 *>( val.data );
		break;
	case tVector3:
	case tHalfVector3:
	case tByteVector3:
		delete static_cast<Vector3 *>( val.data );
		break;
	case tVector2:
	case tHalfVector2:
		delete static_cast<Vector2 *>( val.data );
		break;
	case tMatrix:
		delete static_cast<Matrix *>( val.data );
		break;
	case tMatrix4:
		delete static_cast<Matrix4 *>( val.data );
		break;
	case tQuat:
	case tQuatXYZW:
		delete static_cast<Quat *>( val.data );
		break;
	case tByteMatrix:
		delete static_cast<ByteMatrix *>( val.data );
		break;
	case tByteArray:
	case tStringPalette:
		delete static_cast<QByteArray *>( val.data );
		break;
	case tTriangle:
		delete static_cast<Triangle *>( val.data );
		break;
	case tString:
	case tSizedString:
	case tText:
	case tShortString:
	case tHeaderString:
	case tLineString:
	case tChar8String:
		delete static_cast<QString *>( val.data );
		break;
	case tColor3:
		delete static_cast<Color3 *>( val.data );
		break;
	case tColor4:
	case tByteColor4:
		delete static_cast<Color4 *>( val.data );
		break;
	case tBSVertexDesc:
		delete static_cast<BSVertexDesc *>(val.data);
		break;
	case tBlob:
		delete static_cast<QByteArray *>( val.data );
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

	switch ( ( typ = t ) ) {
	case tLink:
	case tUpLink:
		val.i32 = -1;
		return;
	case tVector3:
	case tHalfVector3:
	case tByteVector3:
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
	case tHalfVector2:
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
	case tByteColor4:
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
	case tBSVertexDesc:
		val.data = new BSVertexDesc();
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

	switch ( typ ) {
	case tVector3:
	case tHalfVector3:
	case tByteVector3:
		*static_cast<Vector3 *>( val.data ) = *static_cast<Vector3 *>( other.val.data );
		return;
	case tVector4:
		*static_cast<Vector4 *>( val.data ) = *static_cast<Vector4 *>( other.val.data );
		return;
	case tMatrix:
		*static_cast<Matrix *>( val.data ) = *static_cast<Matrix *>( other.val.data );
		return;
	case tMatrix4:
		*static_cast<Matrix4 *>( val.data ) = *static_cast<Matrix4 *>( other.val.data );
		return;
	case tQuat:
	case tQuatXYZW:
		*static_cast<Quat *>( val.data ) = *static_cast<Quat *>( other.val.data );
		return;
	case tVector2:
	case tHalfVector2:
		*static_cast<Vector2 *>( val.data ) = *static_cast<Vector2 *>( other.val.data );
		return;
	case tString:
	case tSizedString:
	case tText:
	case tShortString:
	case tHeaderString:
	case tLineString:
	case tChar8String:
		*static_cast<QString *>( val.data ) = *static_cast<QString *>( other.val.data );
		return;
	case tColor3:
		*static_cast<Color3 *>( val.data ) = *static_cast<Color3 *>( other.val.data );
		return;
	case tColor4:
	case tByteColor4:
		*static_cast<Color4 *>( val.data ) = *static_cast<Color4 *>( other.val.data );
		return;
	case tByteArray:
	case tStringPalette:
		*static_cast<QByteArray *>( val.data ) = *static_cast<QByteArray *>( other.val.data );
		return;
	case tByteMatrix:
		*static_cast<ByteMatrix *>( val.data ) = *static_cast<ByteMatrix *>( other.val.data );
		return;
	case tTriangle:
		*static_cast<Triangle *>( val.data ) = *static_cast<Triangle *>( other.val.data );
		return;
	case tBlob:
		*static_cast<QByteArray *>( val.data ) = *static_cast<QByteArray *>( other.val.data );
		return;
	case tBSVertexDesc:
		*static_cast<BSVertexDesc *>(val.data) = *static_cast<BSVertexDesc *>(other.val.data);
		return;
	default:
		val = other.val;
		return;
	}
}

bool NifValue::operator==( const NifValue & other ) const
{
	switch ( typ ) {
	case tByte:
		return val.u08 == other.val.u08;

	case tWord:
	case tFlags:
	case tStringOffset:
	case tBlockTypeIndex:
	case tShort:
		return val.u16 == other.val.u16;

	case tBool:
	case tUInt:
	case tULittle32:
	case tStringIndex:
	case tFileVersion:
		return val.u32 == other.val.u32;

	case tInt:
	case tLink:
	case tUpLink:
		return val.i32 == other.val.i32;

	case tFloat:
	case tHfloat:
		return val.f32 == other.val.f32;
	case tString:
	case tSizedString:
	case tText:
	case tShortString:
	case tHeaderString:
	case tLineString:
	case tChar8String:
	case tFilePath:
	{
		QString * s1 = static_cast<QString *>(val.data);
		QString * s2 = static_cast<QString *>(other.val.data);

		if ( !s1 || !s2 )
			return false;

		return *s1 == *s2;
	}

	case tColor3:
	{
		Color3 * c1 = static_cast<Color3 *>(val.data);
		Color3 * c2 = static_cast<Color3 *>(other.val.data);

		if ( !c1 || !c2 )
			return false;

		return *c1 == *c2;
	}

	case tColor4:
	case tByteColor4:
	{
		Color4 * c1 = static_cast<Color4 *>(val.data);
		Color4 * c2 = static_cast<Color4 *>(other.val.data);

		if ( !c1 || !c2 )
			return false;

		return *c1 == *c2;
	}

	case tVector2:
	case tHalfVector2:
	{
		Vector2 * vec1 = static_cast<Vector2 *>(val.data);
		Vector2 * vec2 = static_cast<Vector2 *>(other.val.data);

		if ( !vec1 || !vec2 )
			return false;

		return *vec1 == *vec2;
	}

	case tVector3:
	case tHalfVector3:
	case tByteVector3:
	{
		Vector3 * vec1 = static_cast<Vector3 *>(val.data);
		Vector3 * vec2 = static_cast<Vector3 *>(other.val.data);

		if ( !vec1 || !vec2 )
			return false;

		return *vec1 == *vec2;
	}

	case tVector4:
	{
		Vector4 * vec1 = static_cast<Vector4 *>(val.data);
		Vector4 * vec2 = static_cast<Vector4 *>(other.val.data);

		if ( !vec1 || !vec2 )
			return false;

		return *vec1 == *vec2;
	}

	case tQuat:
	case tQuatXYZW:
	{
		Quat * quat1 = static_cast<Quat *>(val.data);
		Quat * quat2 = static_cast<Quat *>(other.val.data);

		if ( !quat1 || !quat2 )
			return false;

		return *quat1 == *quat2;
	}

	case tTriangle:
	{
		Triangle * tri1 = static_cast<Triangle *>(val.data);
		Triangle * tri2 = static_cast<Triangle *>(other.val.data);

		if ( !tri1 || !tri2 )
			return false;

		return *tri1 == *tri2;
	}

	case tByteArray:
	case tByteMatrix:
	case tStringPalette:
	case tBlob:
	{
		QByteArray * a1 = static_cast<QByteArray *>(val.data);
		QByteArray * a2 = static_cast<QByteArray *>(other.val.data);

		if ( a1->isNull() || a2->isNull() )
			return false;

		return *a1 == *a2;
	}

	case tMatrix:
	{
		Matrix * m1 = static_cast<Matrix *>(val.data);
		Matrix * m2 = static_cast<Matrix *>(other.val.data);

		if ( !m1 || !m2 )
			return false;

		return *m1 == *m2;
	}
	case tMatrix4:
	{
		Matrix4 * m1 = static_cast<Matrix4 *>(val.data);
		Matrix4 * m2 = static_cast<Matrix4 *>(other.val.data);

		if ( !m1 || !m2 )
			return false;

		return *m1 == *m2;
	}
	case tBSVertexDesc:
	{
		auto d1 = static_cast<BSVertexDesc *>(val.data);
		auto d2 = static_cast<BSVertexDesc *>(other.val.data);

		if ( !d1 || !d2 )
			return false;

		return *d1 == *d2;
	}
	case tNone:
	default:
		return false;
	}

	return false;
}

bool NifValue::operator<( const NifValue & other ) const
{
	Q_UNUSED( other );
	return false;
}


QVariant NifValue::toVariant() const
{
	QVariant v;
	v.setValue( *this );
	return v;
}

bool NifValue::setFromVariant( const QVariant & var )
{
	if ( var.canConvert<NifValue>() ) {
		operator=( var.value<NifValue>() );
		return true;
	} else if ( var.type() == QVariant::String ) {
		return set<QString>( var.toString() );
	}

	return false;
}

bool NifValue::setFromString( const QString & s )
{
	bool ok;

	switch ( typ ) {
	case tBool:

		if ( s == "yes" || s == "true" ) {
			val.u32 = 1;
			return true;
		} else if ( s == "no" || s == "false" ) {
			val.u32 = 0;
			return true;
		} else if ( s == "undefined" ) {
			val.u32 = 2;
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
	case tULittle32:
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
	case tHfloat:
		val.f32 = s.toDouble( &ok );
		return ok;
	case tString:
	case tSizedString:
	case tText:
	case tShortString:
	case tHeaderString:
	case tLineString:
	case tChar8String:
		*static_cast<QString *>( val.data ) = s;
		return true;
	case tColor3:
		static_cast<Color3 *>( val.data )->fromQColor( QColor( s ) );
		return true;
	case tColor4:
	case tByteColor4:
		static_cast<Color4 *>( val.data )->fromQColor( QColor( s ) );
		return true;
	case tFileVersion:
		val.u32 = NifModel::version2number( s );
		return val.u32 != 0;
	case tVector2:
		static_cast<Vector2 *>( val.data )->fromString( s );
		return true;
	case tVector3:
		static_cast<Vector3 *>( val.data )->fromString( s );
		return true;
	case tVector4:
		static_cast<Vector4 *>( val.data )->fromString( s );
		return true;
	case tQuat:
	case tQuatXYZW:
		static_cast<Quat *>( val.data )->fromString( s );
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
	switch ( typ ) {
	case tBool:
		return ( (val.u32 == 2) ? "undefined" : (val.u32 ? "yes" : "no") );
	case tByte:
	case tWord:
	case tFlags:
	case tStringOffset:
	case tBlockTypeIndex:
	case tUInt:
	case tULittle32:
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
		return NumOrMinMax( val.f32, 'f', 6 );
	case tHfloat:
		return QString::number( val.f32, 'f', 4 );
	case tString:
	case tSizedString:
	case tText:
	case tShortString:
	case tHeaderString:
	case tLineString:
	case tChar8String:
		return *static_cast<QString *>( val.data );
	case tColor3:
		{
			Color3 * col = static_cast<Color3 *>( val.data );
			float r = col->red(), g = col->green(), b = col->blue();

			// HDR Colors
			if ( r > 1.0 || g > 1.0 || b > 1.0 )
				return QString( "R %1 G %2 B %3" ).arg( r, 0, 'f', 3 ).arg( g, 0, 'f', 3 ).arg( b, 0, 'f', 3 );

			return QString( "#%1%2%3" )
			       .arg( (int)( r * 0xff ), 2, 16, QChar( '0' ) )
			       .arg( (int)( g * 0xff ), 2, 16, QChar( '0' ) )
			       .arg( (int)( b * 0xff ), 2, 16, QChar( '0' ) );
		}
	case tColor4:
	case tByteColor4:
		{
			Color4 * col = static_cast<Color4 *>( val.data );
			float r = col->red(), g = col->green(), b = col->blue(), a = col->alpha();

			// HDR Colors
			if ( r > 1.0 || g > 1.0 || b > 1.0 || a > 1.0 )
				return QString( "R %1 G %2 B %3 A %4" ).arg( r, 0, 'f', 3 )
						.arg( g, 0, 'f', 3 )
						.arg( b, 0, 'f', 3 )
						.arg( a, 0, 'f', 3 );

			return QString( "#%1%2%3%4" )
			       .arg( (int)( r * 0xff ), 2, 16, QChar( '0' ) )
			       .arg( (int)( g * 0xff ), 2, 16, QChar( '0' ) )
			       .arg( (int)( b * 0xff ), 2, 16, QChar( '0' ) )
			       .arg( (int)( a * 0xff ), 2, 16, QChar( '0' ) );
		}
	case tVector2:
	case tHalfVector2:
		{
			Vector2 * v = static_cast<Vector2 *>( val.data );

			return QString( "X %1 Y %2" )
			       .arg( NumOrMinMax( (*v)[0], 'f', VECTOR_DECIMALS ) )
			       .arg( NumOrMinMax( (*v)[1], 'f', VECTOR_DECIMALS ) );
		}
	case tVector3:
	case tHalfVector3:
	case tByteVector3:
		{
			Vector3 * v = static_cast<Vector3 *>( val.data );

			return QString( "X %1 Y %2 Z %3" )
			       .arg( NumOrMinMax( (*v)[0], 'f', VECTOR_DECIMALS ) )
			       .arg( NumOrMinMax( (*v)[1], 'f', VECTOR_DECIMALS ) )
			       .arg( NumOrMinMax( (*v)[2], 'f', VECTOR_DECIMALS ) );
		}
	case tVector4:
		{
			Vector4 * v = static_cast<Vector4 *>( val.data );

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

			if ( typ == tMatrix )
				m = *( static_cast<Matrix *>( val.data ) );
			else
				m.fromQuat( *( static_cast<Quat *>( val.data ) ) );

			float x, y, z;
			QString pre, suf;

			if ( !m.toEuler( x, y, z ) ) {
				pre = "(";
				suf = ")";
			}

			return ( pre + QString( "Y %1 P %2 R %3" ) + suf )
			       .arg( NumOrMinMax( x / PI * 180, 'f', ROTATION_COARSE ) )
			       .arg( NumOrMinMax( y / PI * 180, 'f', ROTATION_COARSE ) )
			       .arg( NumOrMinMax( z / PI * 180, 'f', ROTATION_COARSE ) );
		}
	case tMatrix4:
		{
			Matrix4 * m = static_cast<Matrix4 *>( val.data );
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
			       .arg( xr,   0, 'f', 3 )
			       .arg( yr,   0, 'f', 3 )
			       .arg( zr,   0, 'f', 3 )
			       .arg( s[0], 0, 'f', 3 )
			       .arg( s[1], 0, 'f', 3 )
			       .arg( s[2], 0, 'f', 3 );
		}
	case tByteArray:
		return QString( "%1 bytes" )
		       .arg( static_cast<QByteArray *>( val.data )->count() );
	case tStringPalette:
		{
			QByteArray * array = static_cast<QByteArray *>( val.data );
			QString s;

			while ( s.length() < array->count() ) {
				s += &array->data()[s.length()];
				s += QChar( '|' );
			}

			return s;
		}
	case tByteMatrix:
		{
			ByteMatrix * array = static_cast<ByteMatrix *>( val.data );
			return QString( "%1 bytes  [%2 x %3]" )
			       .arg( array->count() )
			       .arg( array->count( 0 ) )
				   .arg( array->count( 1 ) );
		}
	case tFileVersion:
		return NifModel::version2string( val.u32 );
	case tTriangle:
		{
			Triangle * tri = static_cast<Triangle *>( val.data );
			return QString( "%1 %2 %3" )
			       .arg( tri->v1() )
			       .arg( tri->v2() )
			       .arg( tri->v3() );
		}
	case tFilePath:
		{
			return *static_cast<QString *>( val.data );
		}
	case tBSVertexDesc:
		return static_cast<BSVertexDesc *>(val.data)->toString();
	case tBlob:
		{
			QByteArray * array = static_cast<QByteArray *>( val.data );
			return QString( "%1 bytes" )
				   .arg( array->size() );
		}
	default:
		return QString();
	}
}

QColor NifValue::toColor() const
{
	if ( type() == tColor3 )
		return static_cast<Color3 *>( val.data )->toQColor();
	else if ( type() == tColor4 || type() == tByteColor4 )
		return static_cast<Color4 *>( val.data )->toQColor();

	return QColor();
}


