/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005-2008, NIF File Format Library and Tools
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


//! A generic class used for storing a value of any type.
/*!
 * The NifValue::Type enum lists all supported types.
 */
class NifValue
{
public:
	//! List of all types implemented internally by NifSkope.
	/*!
	 * To add a new type, add a new enumerant to Type, and update NifValue::initialize()
	 * to reflect the name of the type as used in the xml.
	 */
	enum Type
	{
		// all count types should come between tBool and tUInt
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
		//
		tLink = 10,
		tUpLink = 11,
		tFloat = 12,
		// all string types should come between tSizedString and tChar8String
		tSizedString = 13,
		tText = 15,
		tShortString = 16,
		tHeaderString = 18,
		tLineString = 19,
		tChar8String = 20,
		//
		tColor3 = 21,
		tColor4 = 22,
		tVector3 = 23,
		tQuat = 24,
		tQuatXYZW = 25,
		tMatrix = 26,
		tMatrix4 = 27,
		tVector2 = 28,
		tVector4 = 29,
		tTriangle = 30,
		tFileVersion = 31,
		tByteArray = 32,
		tStringPalette = 33,
		tString = 34, // not a regular string: an integer for nif versions 20.1.0.3 and up
		tFilePath = 35, // not a string: requires special handling for slash/backslash etc.
		tByteMatrix = 36,

		tNone = 0xff
	};
	
	// *** apparently not used ***
	//template <typename T> static Type typeId();
	
	//! Initialize the class data
	/*!
	 * Sets typeMap. Clears typeTxt and enumMap (which will be filled later during xml parsing).
	 */
	static void initialize();
	
	//! Get the Type corresponding to a string typId, as stored in the typeMap.
	/*!
	 * \param typId The type string (as used in the xml).
	 * \return The Type corresponding to the string, or tNone if the type is not found.
	 */
	static Type type( const QString & typId );
	//! Get a html formatted description of the type.
	static QString typeDescription( const QString & typId );
	//! Update the typeTxt map with the type description. Newline characters are replaced by html line break tags.
	static void setTypeDescription( const QString & typId, const QString & txt );
	//! Register an alias for a type.
	/*!
	 * This is done by updating the typeMap and maps the alias string to the type
	 * corresponding to the internal string.
	 */
	static bool registerAlias( const QString & alias, const QString & internal );

	//! Register an option for an enum type.
	/*!
	 * \param eid The name of the enum type.
	 * \param oid The name of the option of that type to add.
	 * \param oval The value of that option.
	 * \param otxt The documentation string for the option.
	 * \return true if successful, false if the option value was already registered.
	 */
	static bool registerEnumOption( const QString & eid, const QString & oid, quint32 oval, const QString & otxt );
	//! Get the name of an option from its value.
	static QString enumOptionName( const QString & eid, quint32 oval );
	//! Get the documentation string of an option from its value.
	static QString enumOptionText( const QString & eid, quint32 oval );
	//! Get the an option from an option string.
	/*!
	 * \param eid The name of the enum type.
	 * \param oid The name of the option.
	 * \param ok Is set to true if succesfull, is set to false if the option string was not found.
	 */
	static quint32 enumOptionValue( const QString & eid, const QString & oid, bool * ok = 0 );
	//! Get list of all options that have been registered for the given enum type.
	static QStringList enumOptions( const QString & eid );
	
	//! Initialize the value to nothing, type tNone.
	NifValue() { typ = tNone; }
	//! Initialize the value to a default value of the specified type.
	NifValue( Type t );
	//! Copy constructor.
	NifValue( const NifValue & other );
	
	~NifValue();
	
	//! Clear the data, setting its type to tNone.
	void clear();
	//! Change the type of data stored.
	/*!
	 * Clears existing data, changes its type, and then reinitializes the data to its default.
	 * Note that if Type is the same as originally, then the data is not cleared.
	 */
	void changeType( Type );
	
	//! Assignment. Performs a deep copy of the data.
	void operator=( const NifValue & other );
	
	//! Get the type.
	Type type() const { return typ; }
	
	//! Check if the type is not tNone.
	static bool isValid( Type t ) { return t != tNone; }
	//! Check if a type is of a link type (Ref or Ptr in xml).
	static bool isLink( Type t ) { return t == tLink || t == tUpLink; }
	
	//! Check if the type of the data is not tNone.
	bool isValid() const { return typ != tNone; }
	//! Check if the type of the data is a color type (Color3 or Color4 in xml).
	bool isColor() const { return typ == tColor3 || typ == tColor4; }
	//! Check if the type of the data is a count.
	bool isCount() const { return (typ >= tBool && typ <= tUInt); }
	//! Check if the type of the data is a flag type (Flags in xml).
	bool isFlags() const { return typ == tFlags; }
	//! Check if the type of the data is a float type (Float in xml).
	bool isFloat() const { return typ == tFloat; }
	//! Check if the type of the data  is of a link type (Ref or Ptr in xml).
	bool isLink() const { return typ == tLink || typ == tUpLink; }
	bool isMatrix() const { return typ == tMatrix; }
	bool isMatrix4() const { return typ == tMatrix4; }
	bool isQuat() const { return typ == tQuat || typ == tQuatXYZW; }
	bool isString() const { return (typ >= tSizedString && typ <= tChar8String) || typ == tString; }
	bool isVector4() const { return typ == tVector4; }
	bool isVector3() const { return typ == tVector3; }
	bool isVector2() const { return typ == tVector2; }
	bool isTriangle() const { return typ == tTriangle; }
	bool isByteArray() const { return typ == tByteArray || typ == tStringPalette ; }
	bool isFileVersion() const { return typ == tFileVersion; }
	bool isByteMatrix() const { return typ == tByteMatrix; }
	
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

	//! Check whether the data can be converted to something of type T.
	template <typename T> bool ask( T * t = 0 ) const;
	//! Get the data in the form of something of type T.
	template <typename T> T get() const;
	//! Set the data from an instance of type T. Return true if successful.
	template <typename T> bool set( const T & x );

protected:
	//! The type of this data.
	Type typ;
	
	//! The structure containing the data.
	union Value
	{
		quint8	u08;
		quint16	u16;
		quint32	u32;
		qint32	i32;
		float	f32;
		void *	data;
	};
	
	//! The data value.
	Value val;
	
	//! Get the data as an object of type T.
	/*!
	 * If the type t is not equal to the actual type of the data, then return T(). Serves
	 * as a helper function for get, intended for internal use only.
	 */
	template <typename T> T getType( Type t ) const;
	//! Set the data from an object of type T.
	/*!
	 * If the type t is not equal to the actual type of the data, then return false, else
	 * return true. Helper function for set, intended for internal use only.
	 */
	template <typename T> bool setType( Type t, T v );
	
	//! A dictionary yielding the Type from a type string.
	static QHash<QString, Type>	typeMap;
	//! A dictionary yielding the enumaration dictionary from a string.
	/*!
	 * Enums are stored as mappings from quint32 to pairs of strings, where
	 * the first string in the pair is the enumerant string, and the second
	 * is the enumerant documentation string. For example,
	 * enumMap["AlphaFormat"][1] = QPair<"ALPHA_BINARY", "Texture is either fully transparent or fully opaque.">
	 */
	static QHash<QString, QHash<quint32, QPair<QString, QString> > >	enumMap;
	//! A dictionary yielding the documentation string of a type string.
	static QHash<QString, QString>	typeTxt;
	
	friend class NifIStream;
	friend class NifOStream;
	friend class NifSStream;
};

inline quint32 NifValue::toCount() const {
#ifdef WIN32
	if ( isCount() )
		return val.u32;
	else if( isFloat() )
		return *(quint32*)&val.f32;
#else
	if ( isCount() || isFloat() )
		return val.u32; // GCC only allows type punning via union (http://gcc.gnu.org/onlinedocs/gcc-4.2.1/gcc/Optimize-Options.html#index-fstrict_002daliasing-550)
#endif
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
		return *static_cast<T*>( val.data ); // WARNING: this throws an exception if the type of v is not the original type by which val.data was initialized; the programmer must make sure that T matches t.
	else
		return T();
}

template <typename T> inline bool NifValue::setType( Type t, T v )
{
	if ( typ == t )
	{
		*static_cast<T*>( val.data ) = v; // WARNING: this throws an exception if the type of v is not the original type by which val.data was initialized; the programmer must make sure that T matches t.
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
		if ( val.data == NULL )
		{
			val.data = new QString;
		}
	
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
	bool stringAdjust;
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
	bool stringAdjust;
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
	bool stringAdjust;
};


Q_DECLARE_METATYPE( NifValue )

#endif
