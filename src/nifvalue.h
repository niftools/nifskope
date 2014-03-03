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

#ifndef NIFVALUE_H
#define NIFVALUE_H

#include <QDataStream>
#include <QIODevice>
#include <QPair>
#include <QVariant>
#include <QHash>
#include <QString>
#include "niftypes.h"

//! \file nifvalue.h NifValue, NifIStream, NifOStream, NifSStream

// if there is demand for it, consider moving these into Options
//! Number of decimals when editing vector types (Vector2, Vector3, Vector4)
#define VECTOR_DECIMALS 4
//! Maximum/minimum range when editing vector types (Vector2, Vector3, Vector4)
#define VECTOR_RANGE 100000000
//! Number of decimals when editing Quat rotations in Euler mode
#define ROTATION_COARSE 2
//! Number of decimals when editing Quat rotations in Axis-Angle mode
#define ROTATION_FINE 5
//! Number of decimals when editing color types (Color3, Color4)
#define COLOR_DECIMALS 3
//! Increment when editing color types (Color3, Color4)
#define COLOR_STEP 0.01

//! A generic class used for storing a value of any type.
/*!
 * The NifValue::Type enum lists all supported types.
 */
class NifValue
{
	Q_DECLARE_TR_FUNCTIONS(NifValue)

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
		tString = 34, //!< not a regular string: an integer for nif versions 20.1.0.3 and up
		tFilePath = 35, //!< not a string: requires special handling for slash/backslash etc.
		tByteMatrix = 36,
		tBlob = 37,
		
		tNone = 0xff
	};
	
	enum EnumType
	{
		eNone,    //!< Not an enum
		eDefault, //!< Standard enum
		eFlags,   //!< bitflag enum
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
	
	//! A struct holding information about a enumeration
	struct EnumOptions {
		EnumType t; //!< The enumeration type
		QHash<quint32, QPair<QString, QString> > o; //!< The enumeration dictionary as a value, a name and a description
	};
	
	//! Register an enum type.
	static bool registerEnumType( const QString & eid, EnumType eTyp );
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
	//! Get an option from an option string.
	/*!
	 * \param eid The name of the enum type.
	 * \param oid The name of the option.
	 * \param ok Is set to true if successful, is set to false if the option string was not found.
	 */
	static quint32 enumOptionValue( const QString & eid, const QString & oid, bool * ok = 0 );
	//! Get list of all options that have been registered for the given enum type.
	static QStringList enumOptions( const QString & eid );
	//! Get type of enum for given enum type
	static EnumType enumType( const QString & eid );
	//! Get list of all options that have been registered for the given enum type.
	static const EnumOptions& enumOptionData( const QString & eid );
	
	//! Constructor - initialize the value to nothing, type tNone.
	NifValue() { typ = tNone; abstract = false; }
	//! Constructor - initialize the value to a default value of the specified type.
	NifValue( Type t );
	//! Copy constructor.
	NifValue( const NifValue & other );
	//! Destructor.
	~NifValue();
	
	//! Clear the data, setting its type to tNone.
	void clear();
	//! Change the type of data stored.
	/*!
	 * Clears existing data, changes its type, and then reinitializes the data to its default.
	 * Note that if Type is the same as originally, then the data is not cleared.
	 */
	void changeType( Type );
	
	//! Get the abstract flag on this value. Does not seem to be reliably initialised yet.
	inline bool isAbstract() { return abstract; }
	
	//! Set the abstract flag on this value.
	inline void setAbstract( bool flag ) { abstract = flag; }
	
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
	//! Check if the type of the data is of a link type (Ref or Ptr in xml).
	bool isLink() const { return typ == tLink || typ == tUpLink; }
	//! Check if the type of the data is a 3x3 matrix type (Matrix33 in xml).
	bool isMatrix() const { return typ == tMatrix; }
	//! Check if the type of the data is a 4x4 matrix type (Matrix44 in xml).
	bool isMatrix4() const { return typ == tMatrix4; }
	//! Check if the type of the data is a quaternion type.
	bool isQuat() const { return typ == tQuat || typ == tQuatXYZW; }
	//! Check if the type of the data is a string type.
	bool isString() const { return (typ >= tSizedString && typ <= tChar8String) || typ == tString; }
	//! Check if the type of the data is a Vector 4.
	bool isVector4() const { return typ == tVector4; }
	//! Check if the type of the data is a Vector 3.
	bool isVector3() const { return typ == tVector3; }
	//! Check if the type of the data is a Vector 2.
	bool isVector2() const { return typ == tVector2; }
	//! Check if the type of the data is a triangle type.
	bool isTriangle() const { return typ == tTriangle; }
	//! Check if the type of the data is a byte array.
	bool isByteArray() const { return typ == tByteArray || typ == tStringPalette || typ == tBlob; }
	//! Check if the type of the data is a File Version.
	bool isFileVersion() const { return typ == tFileVersion; }
	//! Check if the type of the data is a byte matrix.
	bool isByteMatrix() const { return typ == tByteMatrix; }
	
	//! Return the value of the data as a QColor, if applicable.
	QColor toColor() const;
	//! Return the value of the data as a count.
	quint32 toCount() const;
	//! Return the value of the data as a float.
	float toFloat() const;
	//! Return the value of the data as a link, if applicable.
	qint32 toLink() const;
	//! Return the value of the data as a file version, if applicable.
	quint32 toFileVersion() const;
	
	//! Return a string which represents the value of the data.
	QString toString() const;
	//! See the documentation of QVariant for details.
	QVariant toVariant() const;
	
	//! Set this value to a count.
	/**
	 * \return True if applicable, false otherwise
	 */
	bool setCount( quint32 );
	
	//! Set this value to a float.
	/**
	 * \return True if applicable, false otherwise
	 */
	bool setFloat( float );
	
	//! Set this value to a link.
	/**
	 * \return True if applicable, false otherwise
	 */
	bool setLink( int );
	
	//! Set this value to a file version.
	/**
	 * \return True if applicable, false otherwise
	 */
	bool setFileVersion( quint32 );
	
	//! Set this value from a string.
	/**
	 * \return True if applicable, false otherwise
	 */
	bool fromString( const QString & );
	
	//! Set this value from a QVariant.
	/**
	 * \return True if applicable, false otherwise
	 */
	bool fromVariant( const QVariant & );
	
	//! Check whether the data is of type T.
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
	
	//! If the value represents an abstract field. Does not seem to be reliably initialised yet.
	bool abstract;
	
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
	
	//! A dictionary yielding the enumeration dictionary from a string.
	/*!
	 * Enums are stored as mappings from quint32 to pairs of strings, where
	 * the first string in the pair is the enumerant string, and the second
	 * is the enumerant documentation string. For example,
	 * enumMap["AlphaFormat"][1] = QPair<"ALPHA_BINARY", "Texture is either fully transparent or fully opaque.">
	 */
	static QHash<QString, EnumOptions>	enumMap;
	//! A dictionary yielding the documentation string of a type string.
	static QHash<QString, QString>	typeTxt;
	//! A dictionary yielding the underlying type string from an alias string.
	/*!
	 * Enums are stored as an underlying type (not always uint) which is normally not visible.
	 * This dictionary allows that type to be exposed, eg. for NifValue::typeDescription().
	 */
	static QHash<QString, QString> aliasMap;
	
	friend class NifIStream;
	friend class NifOStream;
	friend class NifSStream;
};

// documented above; should this really be inlined?
// GCC only allows type punning via union (http://gcc.gnu.org/onlinedocs/gcc-4.2.1/gcc/Optimize-Options.html#index-fstrict_002daliasing-550)
// This also works on GCC 3.4.5
inline quint32 NifValue::toCount() const { if ( isCount() || isFloat() ) return val.u32; else return 0; }
// documented above
inline float NifValue::toFloat() const { if ( isFloat() ) return val.f32; else return 0.0; }
// documented above
inline qint32 NifValue::toLink() const { if ( isLink() ) return val.i32; else return -1; }
// documented above
inline quint32 NifValue::toFileVersion() const { if ( isFileVersion() ) return val.u32; else return 0; }

// documented above
inline bool NifValue::setCount( quint32 c ) { if ( isCount() ) { val.u32 = c; return true; } else return false; }
// documented above
inline bool NifValue::setFloat( float f ) { if ( isFloat() ) { val.f32 = f; return true; } else return false; }
// documented above
inline bool NifValue::setLink( int l ) { if ( isLink() ) { val.i32 = l; return true; } else return false; }
// documented above
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
template <> inline QByteArray* NifValue::get() const
{
	if ( isByteArray() )
		return static_cast<QByteArray*>( val.data );
	else
		return NULL;
}
template <> inline Quat NifValue::get() const
{
	if ( isQuat() )
		return *static_cast<Quat*>( val.data );
	else
		return Quat();
}
template <> inline ByteMatrix* NifValue::get() const
{
	if ( isByteMatrix() )
		return static_cast<ByteMatrix*>( val.data );
	else
		return NULL;
}

//! Set the data from a boolean. Return true if successful.
template <> inline bool NifValue::set( const bool & b ) { return setCount( b ); }
//! Set the data from an integer. Return true if successful.
template <> inline bool NifValue::set( const int & i ) { return setCount( i ); }
//! Set the data from an unsigned integer. Return true if successful.
template <> inline bool NifValue::set( const quint32 & i ) { return setCount( i ); }
//! Set the data from a short. Return true if successful.
template <> inline bool NifValue::set( const qint16 & i ) { return setCount( i ); }
//! Set the data from an unsigned short. Return true if successful.
template <> inline bool NifValue::set( const quint16 & i ) { return setCount( i ); }
//! Set the data from an unsigned byte. Return true if successful.
template <> inline bool NifValue::set( const quint8 & i ) { return setCount( i ); }
//! Set the data from a float. Return true if successful.
template <> inline bool NifValue::set( const float & f ) { return setFloat( f ); }
//! Set the data from a Matrix. Return true if successful.
template <> inline bool NifValue::set( const Matrix & x ) { return setType( tMatrix, x ); }
//! Set the data from a Matrix4. Return true if successful.
template <> inline bool NifValue::set( const Matrix4 & x ) { return setType( tMatrix4, x ); }
//! Set the data from a Vector4. Return true if successful.
template <> inline bool NifValue::set( const Vector4 & x ) { return setType( tVector4, x ); }
//! Set the data from a Vector3. Return true if successful.
template <> inline bool NifValue::set( const Vector3 & x ) { return setType( tVector3, x ); }
//! Set the data from a Vector2. Return true if successful.
template <> inline bool NifValue::set( const Vector2 & x ) { return setType( tVector2, x ); }
//! Set the data from a Color3. Return true if successful.
template <> inline bool NifValue::set( const Color3 & x ) { return setType( tColor3, x ); }
//! Set the data from a Color4. Return true if successful.
template <> inline bool NifValue::set( const Color4 & x ) { return setType( tColor4, x ); }
//! Set the data from a Triangle. Return true if successful.
template <> inline bool NifValue::set( const Triangle & x ) { return setType( tTriangle, x ); }

// should this really be inlined?
//! Set the data from a string. Return true if successful.
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

// should this really be inlined?
//! Set the data from a byte array. Return true if successful.
template <> inline bool NifValue::set( const QByteArray & x )
{
	if ( isByteArray() )
	{
		*static_cast<QByteArray*>( val.data ) = x;
		return true;
	}
	return false;
}

// should this really be inlined?
//! Set the data from a quaternion. Return true if successful.
template <> inline bool NifValue::set( const Quat & x )
{
	if ( isQuat() )
	{
		*static_cast<Quat*>( val.data ) = x;
		return true;
	}
	return false;
}

//! Check whether the data is a boolean.
template <> inline bool NifValue::ask( bool * ) const { return isCount(); }
//! Check whether the data is an integer.
template <> inline bool NifValue::ask( int * ) const { return isCount(); }
//! Check whether the data is a short.
template <> inline bool NifValue::ask( short * ) const { return isCount(); }
//! Check whether the data is a float.
template <> inline bool NifValue::ask( float * ) const { return isFloat(); }
//! Check whether the data is a Matrix.
template <> inline bool NifValue::ask( Matrix * ) const { return type() == tMatrix; }
//! Check whether the data is a Matrix4.
template <> inline bool NifValue::ask( Matrix4 * ) const { return type() == tMatrix4; }
//! Check whether the data is a quaternion.
template <> inline bool NifValue::ask( Quat * ) const { return isQuat(); }
//! Check whether the data is a Vector4.
template <> inline bool NifValue::ask( Vector4 * ) const { return type() == tVector4; }
//! Check whether the data is a Vector3.
template <> inline bool NifValue::ask( Vector3 * ) const { return type() == tVector3; }
//! Check whether the data is a Vector2.
template <> inline bool NifValue::ask( Vector2 * ) const { return type() == tVector2; }
//! Check whether the data is a Color3.
template <> inline bool NifValue::ask( Color3 * ) const { return type() == tColor3; }
//! Check whether the data is a Color4.
template <> inline bool NifValue::ask( Color4 * ) const { return type() == tColor4; }
//! Check whether the data is a Triangle.
template <> inline bool NifValue::ask( Triangle * ) const { return type() == tTriangle; }
//! Check whether the data is a string.
template <> inline bool NifValue::ask( QString * ) const { return isString(); }
//! Check whether the data is a byte array.
template <> inline bool NifValue::ask( QByteArray * ) const { return isByteArray(); }

class BaseModel;
class NifItem;

//! An input stream that reads a file into a model.
class NifIStream
{
	Q_DECLARE_TR_FUNCTIONS(NifIStream)

public:
	//! Constructor.
	NifIStream( BaseModel * m, QIODevice * d ) : model( m ), device( d )
	{
		init();
	}
	
	//! Reads a NifValue from the underlying device. Returns true if successful.
	bool read( NifValue & );
	
private:
	//! The model that data is being read into.
	BaseModel * model;
	//! The underlying device that data is being read from.
	QIODevice * device;
	//! The data stream that is wrapped around the device (simplifies endian conversion)
	QDataStream * dataStream;
	
	//! Initialises the stream.
	void init();
	
	//! Whether a boolean is 32-bit.
	bool bool32bit;
	//! Whether link adjustment is required.
	bool linkAdjust;
	//! Whether string adjustment is required.
	bool stringAdjust;
	//! Whether the model is big-endian
	bool bigEndian;
	
	//! The maximum length of a string that can be read.
	int maxLength;
};

//! An output stream that writes a model to a file.
class NifOStream
{
	Q_DECLARE_TR_FUNCTIONS(NifOStream)

public:
	//! Constructor.
	NifOStream( const BaseModel * n, QIODevice * d ) : model( n ), device( d ) { init(); }
	
	//! Writes a NifValue to the underlying device. Returns true if successful.
	bool write( const NifValue & );

private:
	//! The model that data is being read from.
	const BaseModel * model;
	//! The underlying device that data is being written to.
	QIODevice * device;
	
	//! Initialises the stream.
	void init();
	
	//! Whether a boolean is 32-bit.
	bool bool32bit;
	//! Whether link adjustment is required.
	bool linkAdjust;
	//! Whether string adjustment is required.
	bool stringAdjust;
	//! Whether the model is big-endian
	bool bigEndian;
};

//! A stream that determines the size of values in a model.
class NifSStream
{
public:
	//! Constructor.
	NifSStream( const BaseModel * n ) : model( n ) { init(); }
	
	//! Determine the size of a given NifValue.
	int size( const NifValue & );
	
private:
	//! The model that values are being sized for.
	const BaseModel * model;
	
	//! Initialises the stream.
	void init();
	
	//! Whether booleans are 32-bit or not.
	bool bool32bit;
	//! Whether string adjustment is required.
	bool stringAdjust;
};


Q_DECLARE_METATYPE( NifValue )

#endif
