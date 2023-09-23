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

#ifndef NIFVALUE_H
#define NIFVALUE_H

#include "data/niftypes.h"

#include <QByteArray>
#include <QHash>
#include <QPair>
#include <QString>
#include <QVariant>


//! @file nifvalue.h NifValue

// if there is demand for it, consider moving these into Options
//! Number of decimals when editing vector types (Vector2, Vector3, Vector4)
#define VECTOR_DECIMALS 6
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

class BaseModel;
class NifItem;

/*! A generic class used for storing a value of any type.
 *
 * The NifValue::Type enum lists all supported types.
 */
class NifValue
{
	Q_DECLARE_TR_FUNCTIONS( NifValue )

	friend class NifIStream;
	friend class NifOStream;
	friend class NifSStream;

public:
	/*! List of all types implemented internally by NifSkope.
	 *
	 * To add a new type, add a new enumerant to Type, and update NifValue::initialize()
	 * to reflect the name of the type as used in the xml.
	 */
	enum Type
	{
		// all count types should come between tBool and tUInt
		tBool = 0,
		tByte,
		tWord,
		tFlags,
		tStringOffset,
		tStringIndex,
		tBlockTypeIndex,
		tInt,
		tShort,
		tULittle32,
		tInt64,
		tUInt64,
		tUInt,
		// all count types should come between tBool and tUInt
		tLink,
		tUpLink,
		tFloat,
		// all string types should come between tSizedString and tChar8String
		tSizedString,
		tText,
		tShortString,
		tHeaderString,
		tLineString,
		tChar8String,
		// all string types should come between tSizedString and tChar8String
		tColor3,
		tColor4,
		tVector3,
		tQuat,
		tQuatXYZW,
		tMatrix,
		tMatrix4,
		tVector2,
		tVector4,
		tTriangle,
		tFileVersion,
		tByteArray,
		tStringPalette,
		tString,   //!< not a regular string: an integer for nif versions 20.1.0.3 and up
		tFilePath, //!< not a string: requires special handling for slash/backslash etc.
		tByteMatrix,
		tBlob,
		tHfloat,
		tHalfVector3,
		tUshortVector3,
		tByteVector3,
		tHalfVector2,
		tByteColor4,
		tBSVertexDesc,
		tNormbyte,
		tNone= 0xff
	};

	enum EnumType
	{
		eNone,    //!< Not an enum
		eDefault, //!< Standard enum
		eFlags,   //!< bitflag enum
	};

	//! Constructor - initialize the value to nothing, type tNone.
	NifValue() {}
	//! Constructor - initialize the value to a default value of the specified type.
	NifValue( Type t );
	//! Copy constructor.
	NifValue( const NifValue & other );
	//! Destructor.
	~NifValue();


	//! Assignment. Performs a deep copy of the data.
	void operator=(const NifValue & other);
	//! Custom comparator for QVariant::operator==()
	bool operator==(const NifValue & other) const;
	//! Necessary for QMetaType::registerComparators(), but unused
	bool operator<(const NifValue &) const;

	//! Clear the data, setting its type to tNone.
	void clear();

	//! Get the type.
	Type type() const { return typ; }

	/*! Change the type of data stored.
	 *
	 * Clears existing data, changes its type, and then reinitializes the data to its default.
	 * Note that if Type is the same as originally, then the data is not cleared.
	 */
	void changeType( Type );

	// *** apparently not used ***
	//template <typename T> static Type typeId();

	/*! Initialize the class data
	 *
	 * Sets typeMap. Clears typeTxt and enumMap (which will be filled later during xml parsing).
	 */
	static void initialize();

	/*! Get the Type corresponding to a string typId, as stored in the typeMap.
	 *
	 * @param typId The type string (as used in the xml).
	 * @return The Type corresponding to the string, or tNone if the type is not found.
	 */
	static Type type( const QString & typId );

	//! Get a html formatted description of the type.
	static QString typeDescription( const QString & typId );
	//! Update the typeTxt map with the type description. Newline characters are replaced by html line break tags.
	static void setTypeDescription( const QString & typId, const QString & txt );

	/*! Register an alias for a type.
	 *
	 * This is done by updating the typeMap and maps the alias string to the type
	 * corresponding to the internal string.
	 */
	static bool registerAlias( const QString & alias, const QString & internal );

	//! A struct holding information about a enumeration
	struct EnumOptions
	{
		EnumType t;                                 //!< The enumeration type
		QMap<quint32, QPair<QString, QString> > o;  //!< The enumeration dictionary as a value, a name and a description
	};

	//! Register an enum type.
	static bool registerEnumType( const QString & eid, EnumType eTyp );

	/*! Register an option for an enum type.
	 *
	 * @param eid	The name of the enum type.
	 * @param oid	The name of the option of that type to add.
	 * @param oval	The value of that option.
	 * @param otxt	The documentation string for the option.
	 * @return		True if successful, false if the option value was already registered.
	 */
	static bool registerEnumOption( const QString & eid, const QString & oid, quint32 oval, const QString & otxt );

	//! Get the name of an option from its value.
	static QString enumOptionName( const QString & eid, quint32 oval );
	//! Get the documentation string of an option from its value.
	static QString enumOptionText( const QString & eid, quint32 oval );

	/*! Get an option from an option string.
	 * 
	 * @param eid	The name of the enum type.
	 * @param oid	The name of the option.
	 * @param ok	Is set to true if successful, is set to false if the option string was not found.
	 */
	static quint32 enumOptionValue( const QString & eid, const QString & oid, bool * ok = 0 );

	//! Get list of all options that have been registered for the given enum type.
	static QStringList enumOptions( const QString & eid );
	//! Get type of enum for given enum type
	static EnumType enumType( const QString & eid );
	//! Get list of all options that have been registered for the given enum type.
	static const EnumOptions & enumOptionData( const QString & eid );


	//! Check if the type is not tNone.
	static bool isValid( Type t ) { return t != tNone; }
	//! Check if a type is of a link type (Ref or Ptr in xml).
	static bool isLink( Type t ) { return t == tLink || t == tUpLink; }

	//! Check if the type of the data is not tNone.
	bool isValid() const { return typ != tNone; }
	//! Check if the type of the data is a color type (Color3 or Color4 in xml).
	bool isColor() const { return typ == tColor3 || typ == tColor4 || typ == tByteColor4; }
	//! Check if the type of the data is a count.
	bool isCount() const { return (typ >= tBool && typ <= tUInt); }
	//! Check if the type of the data is a flag type (Flags in xml).
	bool isFlags() const { return typ == tFlags; }
	//! Check if the type of the data is a float type (Float in xml).
	bool isFloat() const { return (typ == tFloat) || (typ == tHfloat) || (typ == tNormbyte); }
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
	//! Check if the type of the data is a Half Vector3.
	bool isHalfVector3() const { return typ == tHalfVector3; }
	//! Check if the type of the data is a Byte Vector3.
	bool isByteVector3() const { return typ == tByteVector3; }
	//! Check if the type of the data is a HalfVector2.
	bool isHalfVector2() const { return typ == tHalfVector2; }
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
	QColor toColor( const BaseModel * model, const NifItem * item ) const;
	//! Return the value of the data as a count.
	quint64 toCount( const BaseModel * model, const NifItem * item ) const;
	//! Return the value of the data as a float.
	float toFloat( const BaseModel * model, const NifItem * item ) const;
	//! Return the value of the data as a link, if applicable.
	qint32 toLink( const BaseModel * model, const NifItem * item ) const;
	//! Return the value of the data as a file version, if applicable.
	quint32 toFileVersion( const BaseModel * model, const NifItem * item ) const;

	//! Return a string which represents the value of the data.
	QString toString() const;
	//! See the documentation of QVariant for details.
	QVariant toVariant() const;

	/*! Set this value to a count.
	 *
	 * @return True if applicable, false otherwise
	 */
	bool setCount( quint64 c, const BaseModel * model, const NifItem * item );

	/*! Set this value to a float.
	 *
	 * @return True if applicable, false otherwise
	 */
	bool setFloat( float f, const BaseModel * model, const NifItem * item );

	/*! Set this value to a link.
	 *
	 * @return True if applicable, false otherwise
	 */
	bool setLink( qint32 link, const BaseModel * model, const NifItem * item );

	/*! Set this value to a file version.
	 *
	 * @return True if applicable, false otherwise
	 */
	bool setFileVersion( quint32 v, const BaseModel * model, const NifItem * item );

	/*! Set this value from a string.
	 *
	 * @return True if applicable, false otherwise
	 */
	bool setFromString( const QString & s, const BaseModel * model, const NifItem * item );

	/*! Set this value from a QVariant.
	 *
	 * @return True if applicable, false otherwise
	 */
	bool setFromVariant( const QVariant & );

	//! Get the data in the form of something of type T.
	template <typename T> T get( const BaseModel * model, const NifItem * item ) const;
	//! Set the data from an instance of type T. Return true if successful.
	template <typename T> bool set( const T & x, const BaseModel * model, const NifItem * item );

protected:
	//! The type of this data.
	Type typ = tNone;

	//! The structure containing the data.
	union Value
	{
		quint8 u08;
		quint16 u16;
		quint32 u32;
		qint32 i32;
		quint64 u64;
		qint64 i64;
		float f32;
		void * data;
	};

	//! The data value.
	Value val = {0};

	/*! Get the data as an object of type T.
	 *
	 * If the type t is not equal to the actual type of the data, then return T(). Serves
	 * as a helper function for get, intended for internal use only.
	 */
	template <typename T> T getType( Type t, const BaseModel * model, const NifItem * item ) const;

	/*! Set the data from an object of type T.
	 *
	 * If the type t is not equal to the actual type of the data, then return false, else
	 * return true. Helper function for set, intended for internal use only.
	 */
	template <typename T> bool setType( Type t, T v, const BaseModel * model, const NifItem * item );

	//! A dictionary yielding the Type from a type string.
	static QHash<QString, Type> typeMap;

	/*! A dictionary yielding the enumeration dictionary from a string.
	 *
	 * Enums are stored as mappings from quint32 to pairs of strings, where
	 * the first string in the pair is the enumerant string, and the second
	 * is the enumerant documentation string. For example,
	 * enumMap["AlphaFormat"][1] = QPair<"ALPHA_BINARY", "Texture is either fully transparent or fully opaque.">
	 */
	static QHash<QString, EnumOptions>  enumMap;

	//! A dictionary yielding the documentation string of a type string.
	static QHash<QString, QString>  typeTxt;

	/*! A dictionary yielding the underlying type string from an alias string.
	 *
	 * Enums are stored as an underlying type (not always uint) which is normally not visible.
	 * This dictionary allows that type to be exposed, eg. for NifValue::typeDescription().
	 */
	static QHash<QString, QString> aliasMap;

	static void reportModelError( const BaseModel * model, const NifItem * item, const QString & msg );
	static QString getTypeDebugStr( NifValue::Type t );
public:
	void reportConvertToError( const BaseModel * model, const NifItem * item, const QString & toType ) const;
	void reportConvertFromError( const BaseModel * model, const NifItem * item, const QString & fromType ) const;
};

Q_DECLARE_METATYPE( NifValue )



// Inlines

// documented above; should this really be inlined?
// GCC only allows type punning via union (http://gcc.gnu.org/onlinedocs/gcc-4.2.1/gcc/Optimize-Options.html#index-fstrict_002daliasing-550)
// This also works on GCC 3.4.5
inline quint64 NifValue::toCount( const BaseModel * model, const NifItem * item ) const
{
	if ( isCount() || isFloat() )
		return val.u64;

	if ( model )
		reportConvertToError( model, item, "a number");
	return 0;
}

inline float NifValue::toFloat( const BaseModel * model, const NifItem * item ) const
{
	if ( isFloat() )
		return val.f32;

	if ( model )
		reportConvertToError(model, item, "a float");
	return 0.0;
}

inline qint32 NifValue::toLink( const BaseModel * model, const NifItem * item ) const
{
	if ( isLink() )
		return val.i32;

	if ( model )
		reportConvertToError(model, item, "a link");
	return -1;
}

inline quint32 NifValue::toFileVersion( const BaseModel * model, const NifItem * item ) const
{
	if ( isFileVersion() )
		return val.u32;

	if ( model )
		reportConvertToError(model, item, "a file version");
	return 0;
}

inline bool NifValue::setCount( quint64 c, const BaseModel * model, const NifItem * item )
{
	if ( isCount() ) {
		val.u64 = c; return true;
	}

	if ( model )
		reportConvertFromError( model, item, "a number" );
	return false;
}

inline bool NifValue::setFloat( float f, const BaseModel * model, const NifItem * item )
{
	if ( isFloat() ) {
		val.f32 = f; return true;
	}

	if ( model )
		reportConvertFromError( model, item, "a float" );
	return false;
}

inline bool NifValue::setLink( qint32 lnk, const BaseModel * model, const NifItem * item )
{
	if ( isLink() ) {
		val.i32 = lnk; return true;
	}

	if ( model )
		reportConvertFromError( model, item, "a link" );
	return false;
}

inline bool NifValue::setFileVersion( quint32 v, const BaseModel * model, const NifItem * item  )
{
	if ( isFileVersion() ) {
		val.u32 = v; return true;
	}

	if ( model )
		reportConvertFromError( model, item, "a file version" );
	return false;
}


// Templates

template <typename T> inline T NifValue::getType( Type t, const BaseModel * model, const NifItem * item ) const
{
	if ( typ == t )
		return *static_cast<T *>( val.data ); // WARNING: this throws an exception if the type of v is not the original type by which val.data was initialized; the programmer must make sure that T matches t.

	if ( model )
		reportConvertToError( model, item, getTypeDebugStr( t ) );
	return T();
}

template <typename T> inline bool NifValue::setType( Type t, T v, const BaseModel * model, const NifItem * item )
{
	if ( typ == t ) {
		*static_cast<T *>( val.data ) = v; // WARNING: this throws an exception if the type of v is not the original type by which val.data was initialized; the programmer must make sure that T matches t.
		return true;
	}

	if ( model )
		reportConvertFromError( model, item, getTypeDebugStr(t) );
	return false;
}

template <> inline bool NifValue::get( const BaseModel * model, const NifItem * item ) const
{
	return toCount( model, item );
}
template <> inline qint64 NifValue::get( const BaseModel * model, const NifItem * item ) const
{
	return toCount( model, item );
}
template <> inline quint64 NifValue::get( const BaseModel * model, const NifItem * item ) const
{
	return toCount( model, item );
}
template <> inline qint32 NifValue::get( const BaseModel * model, const NifItem * item ) const
{
	return toCount( model, item );
}
template <> inline quint32 NifValue::get( const BaseModel * model, const NifItem * item ) const
{
	return toCount( model, item );
}
template <> inline qint16 NifValue::get( const BaseModel * model, const NifItem * item ) const
{
	return toCount( model, item );
}
template <> inline quint16 NifValue::get( const BaseModel * model, const NifItem * item ) const
{
	return toCount( model, item );
}
template <> inline quint8 NifValue::get( const BaseModel * model, const NifItem * item ) const
{
	return toCount( model, item );
}
template <> inline float NifValue::get( const BaseModel * model, const NifItem * item ) const
{
	return toFloat( model, item );
}
template <> inline QColor NifValue::get( const BaseModel * model, const NifItem * item ) const
{
	return toColor( model, item );
}
template <> inline QVariant NifValue::get( const BaseModel * /* model */, const NifItem * /* item */ ) const
{
	return toVariant();
}
template <> inline Matrix NifValue::get( const BaseModel * model, const NifItem * item ) const
{
	return getType<Matrix>( tMatrix, model, item );
}
template <> inline Matrix4 NifValue::get( const BaseModel * model, const NifItem * item ) const
{
	return getType<Matrix4>( tMatrix4, model, item );
}
template <> inline Vector4 NifValue::get( const BaseModel * model, const NifItem * item ) const
{
	return getType<Vector4>( tVector4, model, item );
}
template <> inline Vector3 NifValue::get( const BaseModel * model, const NifItem * item ) const
{
	if ( typ == tVector3 || typ == tHalfVector3 )
		return *static_cast<Vector3 *>(val.data);

	if ( model )
		reportConvertToError( model, item, "a Vector3" );
	return Vector3();
}
template <> inline HalfVector3 NifValue::get( const BaseModel * model, const NifItem * item ) const
{
	return getType<HalfVector3>( tHalfVector3, model, item );
}
template <> inline UshortVector3 NifValue::get( const BaseModel * model, const NifItem * item ) const
{
	return getType<UshortVector3>( tUshortVector3, model, item );
}
template <> inline ByteVector3 NifValue::get( const BaseModel * model, const NifItem * item ) const
{
	return getType<ByteVector3>( tByteVector3, model, item );
}
template <> inline HalfVector2 NifValue::get( const BaseModel * model, const NifItem * item ) const
{
	return getType<HalfVector2>( tHalfVector2, model, item );
}
template <> inline Vector2 NifValue::get( const BaseModel * model, const NifItem * item ) const
{
	if ( typ == tVector2 || typ == tHalfVector2 )
		return *static_cast<Vector2 *>(val.data);

	if ( model )
		reportConvertToError( model, item, "a Vector2" );
	return Vector2();
}
template <> inline Color3 NifValue::get( const BaseModel * model, const NifItem * item ) const
{
	return getType<Color3>( tColor3, model, item );
}
template <> inline ByteColor4 NifValue::get( const BaseModel * model, const NifItem * item ) const
{
	return getType<ByteColor4>( tByteColor4, model, item );
}
template <> inline Color4 NifValue::get( const BaseModel * model, const NifItem * item ) const
{
	return getType<Color4>( tColor4, model, item );
}
template <> inline Triangle NifValue::get( const BaseModel * model, const NifItem * item ) const
{
	return getType<Triangle>( tTriangle, model, item );
}
template <> inline QString NifValue::get( const BaseModel * model, const NifItem * item ) const
{
	if ( isString() )
		return *static_cast<QString *>( val.data );

	if ( model )
		reportConvertToError( model, item, "a string" );
	return QString();
}
template <> inline QByteArray NifValue::get( const BaseModel * model, const NifItem * item ) const
{
	if ( isByteArray() )
		return *static_cast<QByteArray *>( val.data );

	if ( model )
		reportConvertToError( model, item, "a byte array" );
	return QByteArray();
}
template <> inline QByteArray * NifValue::get( const BaseModel * model, const NifItem * item ) const
{
	if ( isByteArray() )
		return static_cast<QByteArray *>( val.data );

	if ( model )
		reportConvertToError( model, item, "a byte array" );
	return nullptr;
}
template <> inline Quat NifValue::get( const BaseModel * model, const NifItem * item ) const
{
	if ( isQuat() )
		return *static_cast<Quat *>( val.data );

	if ( model )
		reportConvertToError( model, item, "Quat" );
	return Quat();
}
template <> inline ByteMatrix * NifValue::get( const BaseModel * model, const NifItem * item ) const
{
	if ( isByteMatrix() )
		return static_cast<ByteMatrix *>( val.data );

	if ( model )
		reportConvertToError( model, item, "ByteMatrix" );
	return nullptr;
}
template <> inline BSVertexDesc NifValue::get( const BaseModel * model, const NifItem * item ) const
{
	if ( isCount() )
		return BSVertexDesc( val.u64 );

	if ( model )
		reportConvertToError( model, item, "BSVertexDesc" );
	return BSVertexDesc( 0 );
}

//! Set the data from a boolean. Return true if successful.
template <> inline bool NifValue::set( const bool & b, const BaseModel * model, const NifItem * item )
{
	return setCount( b, model, item );
}
//! Set the data from an integer. Return true if successful.
template <> inline bool NifValue::set( const int & i, const BaseModel * model, const NifItem * item )
{
	return setCount( i, model, item);
}
//! Set the data from an unsigned integer. Return true if successful.
template <> inline bool NifValue::set( const quint32 & i, const BaseModel * model, const NifItem * item )
{
	return setCount( i, model, item );
}
//! Set the data from a short. Return true if successful.
template <> inline bool NifValue::set( const qint16 & i, const BaseModel * model, const NifItem * item )
{
	return setCount( i, model, item );
}
//! Set the data from an unsigned short. Return true if successful.
template <> inline bool NifValue::set( const quint16 & i, const BaseModel * model, const NifItem * item )
{
	return setCount( i, model, item );
}
//! Set the data from an unsigned byte. Return true if successful.
template <> inline bool NifValue::set( const quint8 & i, const BaseModel * model, const NifItem * item )
{
	return setCount( i, model, item );
}
//! Set the data from a float. Return true if successful.
template <> inline bool NifValue::set( const float & f, const BaseModel * model, const NifItem * item )
{
	return setFloat( f, model, item );
}
//! Set the data from a Matrix. Return true if successful.
template <> inline bool NifValue::set( const Matrix & x, const BaseModel * model, const NifItem * item )
{
	return setType( tMatrix, x, model, item );
}
//! Set the data from a Matrix4. Return true if successful.
template <> inline bool NifValue::set( const Matrix4 & x, const BaseModel * model, const NifItem * item )
{
	return setType( tMatrix4, x, model, item );
}
//! Set the data from a Vector4. Return true if successful.
template <> inline bool NifValue::set( const Vector4 & x, const BaseModel * model, const NifItem * item )
{
	return setType( tVector4, x, model, item );
}
//! Set the data from a Vector3. Return true if successful.
template <> inline bool NifValue::set( const Vector3 & x, const BaseModel * model, const NifItem * item )
{
	return setType( tVector3, x, model, item );
}
//! Set the data from a HalfVector3. Return true if successful.
template <> inline bool NifValue::set( const HalfVector3 & x, const BaseModel * model, const NifItem * item )
{
	return setType( tHalfVector3, x, model, item );
}
//! Set the data from a UshortVector3. Return true if successful.
template <> inline bool NifValue::set( const UshortVector3 & x, const BaseModel * model, const NifItem * item )
{
	return setType( tUshortVector3, x, model, item );
}
//! Set the data from a ByteVector3. Return true if successful.
template <> inline bool NifValue::set( const ByteVector3 & x, const BaseModel * model, const NifItem * item )
{
	return setType( tByteVector3, x, model, item );
}
//! Set the data from a HalfVector2. Return true if successful.
template <> inline bool NifValue::set( const HalfVector2 & x, const BaseModel * model, const NifItem * item )
{
	return setType( tHalfVector2, x, model, item );
}
//! Set the data from a Vector2. Return true if successful.
template <> inline bool NifValue::set( const Vector2 & x, const BaseModel * model, const NifItem * item )
{
	return setType( tVector2, x, model, item );
}
//! Set the data from a Color3. Return true if successful.
template <> inline bool NifValue::set( const Color3 & x, const BaseModel * model, const NifItem * item )
{
	return setType( tColor3, x, model, item );
}
//! Set the data from a ByteColor4. Return true if successful.
template <> inline bool NifValue::set( const ByteColor4 & x, const BaseModel * model, const NifItem * item )
{
	return setType( tByteColor4, x, model, item );
}
//! Set the data from a Color4. Return true if successful.
template <> inline bool NifValue::set( const Color4 & x, const BaseModel * model, const NifItem * item )
{
	return setType( tColor4, x, model, item );
}
//! Set the data from a Triangle. Return true if successful.
template <> inline bool NifValue::set( const Triangle & x, const BaseModel * model, const NifItem * item )
{
	return setType( tTriangle, x, model, item );
}
//! Set the data from a string. Return true if successful.
template <> inline bool NifValue::set( const QString & x, const BaseModel * model, const NifItem * item )
{
	if ( isString() ) {
		if ( !val.data ) {
			val.data = new QString;
		}

		*static_cast<QString *>( val.data ) = x;
		return true;
	}

	if ( model )
		reportConvertFromError( model, item, "a QString" );
	return false;
}
//! Set the data from a byte array. Return true if successful.
template <> inline bool NifValue::set( const QByteArray & x, const BaseModel * model, const NifItem * item )
{
	if ( isByteArray() ) {
		*static_cast<QByteArray *>( val.data ) = x;
		return true;
	}

	if ( model )
		reportConvertFromError( model, item, "a QByteArray" );
	return false;
}
//! Set the data from a quaternion. Return true if successful.
template <> inline bool NifValue::set( const Quat & x, const BaseModel * model, const NifItem * item )
{
	if ( isQuat() ) {
		*static_cast<Quat *>( val.data ) = x;
		return true;
	}

	if ( model )
		reportConvertFromError( model, item, "a Quat" );
	return false;
}
//! Set the data from a BSVertexDesc. Return true if successful.
template <> inline bool NifValue::set( const BSVertexDesc & x, const BaseModel * model, const NifItem * item )
{
	return setCount(x.Value(), model, item );
}

#endif
