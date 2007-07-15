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

#ifndef NIFVALUE_H
#define NIFVALUE_H


#include <QIODevice>
#include <QPair>
#include <QVariant>


#include "niftypes.h"


class NifValue
{
public:
	enum Type
	{
		tBool = 0,
		tByte = 1,
		tWord = 2,
		tFlags = 3,
		tStringOffset = 4,
		tStringIndex = 5,
		tBlockTypeIndex = 6,
		tInt = 7,
		tShort = 8,
		tUInt = 9,
		tLink = 10,
		tUpLink = 11,
		tFloat = 12,
		tSizedString = 13,
		tText = 14,
		tShortString = 15,
		tFilePath = 16,
		tHeaderString = 17,
		tLineString = 18,
		tColor3 = 20,
		tColor4 = 21,
		tVector3 = 22,
		tQuat = 23,
		tQuatXYZW = 24,
		tMatrix = 25,
		tMatrix4 = 26,
		tVector2 = 27,
		tVector4 = 28,
		tTriangle = 29,
		tFileVersion = 30,
		tByteArray = 31,
		tStringPalette = 32,

		tNone = 0xff
	};
	
	template <typename T> static Type typeId();
	
	static void initialize();
	
	static Type type( const QString & typId );
	static QString typeDescription( const QString & typId );
	static void setTypeDescription( const QString & typId, const QString & txt );
	
	static bool registerAlias( const QString & alias, const QString & internal );
	
	static bool registerEnumOption( const QString & eid, const QString & oid, quint32 oval, const QString & otxt );
	static QString enumOptionName( const QString & eid, quint32 oval );
	static QString enumOptionText( const QString & eid, quint32 oval );
	static quint32 enumOptionValue( const QString & eid, const QString & oid, bool * ok = 0 );
	static QStringList enumOptions( const QString & eid );
	
	NifValue() { typ = tNone; }
	NifValue( Type t );
	NifValue( const NifValue & other );
	
	~NifValue();
	
	void clear();
	void changeType( Type );
	
	void operator=( const NifValue & other );
	
	Type type() const { return typ; }
	
	static bool isValid( Type t ) { return t != tNone; }
	static bool isLink( Type t ) { return t == tLink || t == tUpLink; }
	
	bool isValid() const { return typ != tNone; }
	bool isColor() const { return typ == tColor3 || typ == tColor4; }
	bool isCount() const { return (typ >= tBool && typ <= tUInt); }
	bool isFlags() const { return typ == tFlags; }
	bool isFloat() const { return typ == tFloat; }
	bool isLink() const { return typ == tLink || typ == tUpLink; }
	bool isMatrix() const { return typ == tMatrix; }
	bool isMatrix4() const { return typ == tMatrix4; }
	bool isQuat() const { return typ == tQuat || typ == tQuatXYZW; }
	bool isString() const { return typ >= tSizedString && typ <= tLineString; }
	bool isVector4() const { return typ == tVector4; }
	bool isVector3() const { return typ == tVector3; }
	bool isVector2() const { return typ == tVector2; }
	bool isTriangle() const { return typ == tTriangle; }
	bool isByteArray() const { return typ == tByteArray || typ == tStringPalette; }
	bool isFileVersion() const { return typ == tFileVersion; }
	
	QColor toColor() const;
	quint32 toCount() const;
	float toFloat() const;
	qint32 toLink() const;
	quint32 toFileVersion() const;

	QString toString() const;
	QVariant toVariant() const;
	
	bool setCount( quint32 );
	bool setFloat( float );
	bool setLink( int );
	bool setFileVersion( quint32 );
	
	bool fromString( const QString & );
	bool fromVariant( const QVariant & );
	
	template <typename T> bool ask( T * t = 0 ) const;
	template <typename T> T get() const;
	template <typename T> bool set( const T & x );

protected:
	Type typ;
	
	union Value
	{
		quint8	u08;
		quint16 u16;
		quint32 u32;
		qint32	i32;
		float	f32;
		void *	data;
	} val;
	
	template <typename T> T getType( Type t ) const;
	template <typename T> bool setType( Type t, T v );
	
	static QHash<QString,Type>	typeMap;
	static QHash<QString, QHash<quint32, QPair<QString, QString> > > enumMap;
	static QHash<QString, QString> typeTxt;
	
	friend class NifIStream;
	friend class NifOStream;
	friend class NifSStream;
};

inline quint32 NifValue::toCount() const {
	if ( isCount() )
		return val.u32;
	else if( isFloat() )
		return *(quint32*)&val.f32;
	return 0;
}
inline float NifValue::toFloat() const { if ( isFloat() ) return val.f32; else return 0.0; }
inline qint32 NifValue::toLink() const { if ( isLink() ) return val.i32; else return -1; }
inline quint32 NifValue::toFileVersion() const { if ( isFileVersion() ) return val.u32; else return 0; }

inline bool NifValue::setCount( quint32 c ) { if ( isCount() ) { val.u32 = c; return true; } else return false; }
inline bool NifValue::setFloat( float f ) { if ( isFloat() ) { val.f32 = f; return true; } else return false; }
inline bool NifValue::setLink( int l ) { if ( isLink() ) { val.i32 = l; return true; } else return false; }
inline bool NifValue::setFileVersion( quint32 v ) { if ( isFileVersion() ) { val.u32 = v; return true; } else return false; }

template <typename T> inline T NifValue::getType( Type t ) const
{
	if ( typ == t )
		return *static_cast<T*>( val.data );
	else
		return T();
}

template <typename T> inline bool NifValue::setType( Type t, T v )
{
	if ( typ == t )
	{
		*static_cast<T*>( val.data ) = v;
		return true;
	}
	return false;
}

template <> inline bool NifValue::get() const { return toCount(); }
template <> inline qint32 NifValue::get() const { return toCount(); }
template <> inline quint32 NifValue::get() const { return toCount(); }
template <> inline qint16 NifValue::get() const { return toCount(); }
template <> inline quint16 NifValue::get() const { return toCount(); }
template <> inline quint8 NifValue::get() const { return toCount(); }
template <> inline float NifValue::get() const { return toFloat(); }
template <> inline QColor NifValue::get() const { return toColor(); }
template <> inline QVariant NifValue::get() const { return toVariant(); }

template <> inline Matrix NifValue::get() const { return getType<Matrix>( tMatrix ); }
template <> inline Matrix4 NifValue::get() const { return getType<Matrix4>( tMatrix4 ); }
template <> inline Vector4 NifValue::get() const { return getType<Vector4>( tVector4 ); }
template <> inline Vector3 NifValue::get() const { return getType<Vector3>( tVector3 ); }
template <> inline Vector2 NifValue::get() const { return getType<Vector2>( tVector2 ); }
template <> inline Color3 NifValue::get() const { return getType<Color3>( tColor3 ); }
template <> inline Color4 NifValue::get() const { return getType<Color4>( tColor4 ); }
template <> inline Triangle NifValue::get() const { return getType<Triangle>( tTriangle ); }
template <> inline QString NifValue::get() const
{
	if ( isString() )
		return *static_cast<QString*>( val.data );
	else
		return QString();
}
template <> inline QByteArray NifValue::get() const
{
	if ( isByteArray() )
		return *static_cast<QByteArray*>( val.data );
	else
		return QByteArray();
}
template <> inline Quat NifValue::get() const
{
	if ( isQuat() )
		return *static_cast<Quat*>( val.data );
	else
		return Quat();
}

template <> inline bool NifValue::set( const bool & b ) { return setCount( b ); }
template <> inline bool NifValue::set( const int & i ) { return setCount( i ); }
template <> inline bool NifValue::set( const quint32 & i ) { return setCount( i ); }
template <> inline bool NifValue::set( const qint16 & i ) { return setCount( i ); }
template <> inline bool NifValue::set( const quint16 & i ) { return setCount( i ); }
template <> inline bool NifValue::set( const quint8 & i ) { return setCount( i ); }
template <> inline bool NifValue::set( const float & f ) { return setFloat( f ); }
template <> inline bool NifValue::set( const Matrix & x ) { return setType( tMatrix, x ); }
template <> inline bool NifValue::set( const Matrix4 & x ) { return setType( tMatrix4, x ); }
template <> inline bool NifValue::set( const Vector4 & x ) { return setType( tVector4, x ); }
template <> inline bool NifValue::set( const Vector3 & x ) { return setType( tVector3, x ); }
template <> inline bool NifValue::set( const Vector2 & x ) { return setType( tVector2, x ); }
template <> inline bool NifValue::set( const Color3 & x ) { return setType( tColor3, x ); }
template <> inline bool NifValue::set( const Color4 & x ) { return setType( tColor4, x ); }
template <> inline bool NifValue::set( const Triangle & x ) { return setType( tTriangle, x ); }
template <> inline bool NifValue::set( const QString & x )
{
	if ( isString() )
	{
		*static_cast<QString*>( val.data ) = x;
		return true;
	}
	return false;
}
template <> inline bool NifValue::set( const QByteArray & x )
{
	if ( isByteArray() )
	{
		*static_cast<QByteArray*>( val.data ) = x;
		return true;
	}
	return false;
}
template <> inline bool NifValue::set( const Quat & x )
{
	if ( isQuat() )
	{
		*static_cast<Quat*>( val.data ) = x;
		return true;
	}
	return false;
}

template <> inline bool NifValue::ask( bool * ) const { return isCount(); }
template <> inline bool NifValue::ask( int * ) const { return isCount(); }
template <> inline bool NifValue::ask( short * ) const { return isCount(); }
template <> inline bool NifValue::ask( float * ) const { return isFloat(); }
template <> inline bool NifValue::ask( Matrix * ) const { return type() == tMatrix; }
template <> inline bool NifValue::ask( Matrix4 * ) const { return type() == tMatrix4; }
template <> inline bool NifValue::ask( Quat * ) const { return isQuat(); }
template <> inline bool NifValue::ask( Vector4 * ) const { return type() == tVector4; }
template <> inline bool NifValue::ask( Vector3 * ) const { return type() == tVector3; }
template <> inline bool NifValue::ask( Vector2 * ) const { return type() == tVector2; }
template <> inline bool NifValue::ask( Color3 * ) const { return type() == tColor3; }
template <> inline bool NifValue::ask( Color4 * ) const { return type() == tColor4; }
template <> inline bool NifValue::ask( Triangle * ) const { return type() == tTriangle; }
template <> inline bool NifValue::ask( QString * ) const { return isString(); }
template <> inline bool NifValue::ask( QByteArray * ) const { return isByteArray(); }

class BaseModel;
class NifItem;

class NifIStream
{
public:
	NifIStream( BaseModel * m, QIODevice * d ) : model( m ), device( d ) { init(); }
	
	bool read( NifValue & );

private:
	BaseModel * model;
	QIODevice * device;
	
	void init();
	
	bool bool32bit;
	bool linkAdjust;
};

class NifOStream
{
public:
	NifOStream( const BaseModel * n, QIODevice * d ) : model( n ), device( d ) { init(); }
	
	bool write( const NifValue & );

private:
	const BaseModel * model;
	QIODevice * device;
	
	void init();
	
	bool bool32bit;
	bool linkAdjust;
};

class NifSStream
{
public:
	NifSStream( const BaseModel * n ) : model( n ) { init(); }
	
	int size( const NifValue & );
	
private:
	const BaseModel * model;
	
	void init();
	
	bool bool32bit;
};


Q_DECLARE_METATYPE( NifValue )

#endif
