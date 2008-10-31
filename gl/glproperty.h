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

#ifndef GLPROPERTY_H
#define GLPROPERTY_H

#include <QtOpenGL>

#include "glcontrolable.h"

class Property : public Controllable
{
protected:
	Property( Scene * scene, const QModelIndex & index ) : Controllable( scene, index ), ref( 0 ) {}
	
	int ref;
	
	friend class PropertyList;
	
public:
	static Property * create( Scene * scene, const NifModel * nif, const QModelIndex & index );

	enum Type
	{
		Alpha, ZBuffer, Material, Texturing, Texture, Specular, Wireframe, VertexColor, Stencil, ShaderLighting
	};
	
	virtual Type type() const = 0;
	virtual QString typeId() const = 0;
	
	template <typename T> static Type _type();
	template <typename T> T * cast()
	{
		if ( type() == _type<T>() )
			return static_cast<T*>( this );
		else
			return 0;
	}
};

#define REGISTER_PROPERTY( CLASSNAME, TYPENAME ) template <> inline Property::Type Property::_type< CLASSNAME >() { return Property::TYPENAME; }

class PropertyList
{
public:
	PropertyList();
	PropertyList( const PropertyList & other );
	~PropertyList();
	
	void add( Property * );
	void del( Property * );
	bool contains( Property * ) const;
	
	Property * get( const QModelIndex & idx ) const;
	
	template <typename T> T * get() const;
	template <typename T> bool contains() const;
	
	void validate();
	
	void clear();
	
	PropertyList & operator=( const PropertyList & other );
	
	QList<Property*> list() const { return properties.values(); }
	
	void merge( const PropertyList & list );
	
protected:
	QMultiHash<Property::Type, Property*> properties;
};

template <typename T> inline T * PropertyList::get() const
{
	Property * p = properties.value( Property::_type<T>() );
	if ( p )
		return p->cast<T>();
	else
		return 0;
}

template <typename T> inline bool PropertyList::contains() const
{
	return properties.contains( Property::_type<T>() );
}


class AlphaProperty : public Property
{
public:
	AlphaProperty( Scene * scene, const QModelIndex & index ) : Property( scene, index ) {}
	
	Type type() const { return Alpha; }
	QString typeId() const { return "NiAlphaProperty"; }
	
	void update( const NifModel * nif, const QModelIndex & block );
	
	bool blend() const { return alphaBlend; }
	bool test() const { return alphaTest; }
	bool sort() const { return alphaSort; }
	
	friend void glProperty( AlphaProperty * );

protected:
	bool alphaBlend, alphaTest, alphaSort;
	GLenum alphaSrc, alphaDst, alphaFunc;
	GLfloat alphaThreshold;	
};

REGISTER_PROPERTY( AlphaProperty, Alpha )


class ZBufferProperty : public Property
{
public:
	ZBufferProperty( Scene * scene, const QModelIndex & index ) : Property( scene, index ) {}
	
	Type type() const { return ZBuffer; }
	QString typeId() const { return "NiZBufferProperty"; }
	
	void update( const NifModel * nif, const QModelIndex & block );
	
	bool test() const { return depthTest; }
	bool mask() const { return depthMask; }
	
	friend void glProperty( ZBufferProperty * );
	
protected:
	bool depthTest;
	bool depthMask;
	GLenum depthFunc;
};

REGISTER_PROPERTY( ZBufferProperty, ZBuffer )


class TexturingProperty : public Property
{
	struct TexDesc
	{
		QPersistentModelIndex iSource;
		GLenum filter;
		GLint wrapS, wrapT;
		int coordset;
		
		bool hasTransform;
		
		Vector2 translation;
		Vector2 tiling;
		float	rotation;
		Vector2 center;
	};
public:
	TexturingProperty( Scene * scene, const QModelIndex & index ) : Property( scene, index ) {}
	
	Type type() const { return Texturing; }
	QString typeId() const { return "NiTexturingProperty"; }
	
	void update( const NifModel * nif, const QModelIndex & block );
	
	friend void glProperty( TexturingProperty * );
	
	bool bind( int id, const QString & fname = QString() );
	
	bool bind( int id, const QList< QVector<Vector2> > & texcoords );
	bool bind( int id, const QList< QVector<Vector2> > & texcoords, int stage );
	
	QString fileName( int id ) const;
	int coordSet( int id ) const;
	
	static int getId( const QString & id );
	
protected:
	TexDesc	textures[8];

	void setController( const NifModel * nif, const QModelIndex & controller );
	
	friend class TexFlipController;
	friend class TexTransController;
};

REGISTER_PROPERTY( TexturingProperty, Texturing )


class TextureProperty : public Property
{
public:
	TextureProperty( Scene * scene, const QModelIndex & index ) : Property( scene, index ) {}
	
	Type type() const { return Texture; }
	QString typeId() const { return "NiTextureProperty"; }
	
	void update( const NifModel * nif, const QModelIndex & block );
	
	friend void glProperty( TextureProperty * );
	
	bool bind();
	bool bind( const QList< QVector<Vector2> > & texcoords );
	
	QString fileName() const;

protected:
	QPersistentModelIndex iImage;
};

REGISTER_PROPERTY( TextureProperty, Texture )


class MaterialProperty : public Property
{
public:
	MaterialProperty( Scene * scene, const QModelIndex & index ) : Property( scene, index ) {}
	
	Type type() const { return Material; }
	QString typeId() const { return "NiMaterialProperty"; }
	
	void update( const NifModel * nif, const QModelIndex & block );
	
	friend void glProperty( class MaterialProperty *, class SpecularProperty * );
	
	GLfloat alphaValue() const { return alpha; }
	
protected:
	Color4 ambient, diffuse, specular, emissive;
	GLfloat shininess, alpha;
   bool overridden;

	void setController( const NifModel * nif, const QModelIndex & controller );
	
	friend class AlphaController;
	friend class MaterialColorController;
};

REGISTER_PROPERTY( MaterialProperty, Material )


class SpecularProperty : public Property
{
public:
	SpecularProperty( Scene * scene, const QModelIndex & index ) : Property( scene, index ) {}
	
	Type type() const { return Specular; }
	QString typeId() const { return "NiSpecularProperty"; }

	void update( const NifModel * nif, const QModelIndex & index );
	
	friend void glProperty( class MaterialProperty *, class SpecularProperty * );
	
protected:
	bool spec;
};

REGISTER_PROPERTY( SpecularProperty, Specular )


class WireframeProperty : public Property
{
public:
	WireframeProperty( Scene * scene, const QModelIndex & index ) : Property( scene, index ) {}
	
	Type type() const { return Wireframe; }
	QString typeId() const { return "NiWireframeProperty"; }
	
	void update( const NifModel * nif, const QModelIndex & index );
	
	friend void glProperty( WireframeProperty * );
	
protected:
	bool wire;
};

REGISTER_PROPERTY( WireframeProperty, Wireframe )


class VertexColorProperty : public Property
{
public:
	VertexColorProperty( Scene * scene, const QModelIndex & index ) : Property( scene, index ) {}
	
	Type type() const { return VertexColor; }
	QString typeId() const { return "NiVertexColorProperty"; }
	
	void update( const NifModel * nif, const QModelIndex & index );
	
	friend void glProperty( VertexColorProperty *, bool vertexcolors );

protected:
	int	lightmode;
	int	vertexmode;
};

REGISTER_PROPERTY( VertexColorProperty, VertexColor )


class StencilProperty : public Property
{
public:
	StencilProperty( Scene * scene, const QModelIndex & index ) : Property( scene, index ) {}
	
	Type type() const { return Stencil; }
	QString typeId() const { return "NiStencilProperty"; }
	
	void update( const NifModel * nif, const QModelIndex & index );
	
	friend void glProperty( StencilProperty * );

protected:
	bool	stencil;
	
	GLenum	func;
	GLint	ref;
	GLuint	mask;
	
	GLenum	failop;
	GLenum	zfailop;
	GLenum	zpassop;
	
	bool	cullEnable;
	GLenum	cullMode;
};

REGISTER_PROPERTY( StencilProperty, Stencil )


class BSShaderLightingProperty : public Property
{
public:
   BSShaderLightingProperty( Scene * scene, const QModelIndex & index ) : Property( scene, index ) {}

   Type type() const { return ShaderLighting; }
   QString typeId() const { return "BSShaderLightingProperty"; }

   void update( const NifModel * nif, const QModelIndex & block );

   friend void glProperty( BSShaderLightingProperty * );

   bool bind( int id, const QString & fname = QString() );
   bool bind( int id, const QList< QVector<Vector2> > & texcoords );
   bool bind( int id, const QList< QVector<Vector2> > & texcoords, int stage );

   QString fileName( int id ) const;
   //int coordSet( int id ) const;

   static int getId( const QString & id );

protected:
   //QVector<QString> textures;
   QPersistentModelIndex iTextureSet;
};

REGISTER_PROPERTY( BSShaderLightingProperty, ShaderLighting )


#endif
