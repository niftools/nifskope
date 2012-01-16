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

#ifndef GLSHADER_H
#define GLSHADER_H

#include <QtCore/QtCore> // extra include to avoid compile error
#include <QtGui/QtGui>   // dito

#include "GLee.h"
#include <QtOpenGL>

#include "../nifmodel.h"

class Mesh;
class PropertyList;

//! Manages rendering and shaders?
class Renderer
{
public:
	//! Constructor
	Renderer();
	//! Destructor
	~Renderer();
	
	//! Init from context?
	static bool initialize( const QGLContext * );
	//! Whether the shaders are available
	static bool hasShaderSupport();
	
	//! Updates shaders
	void updateShaders();
	//! Releases shaders
	void releaseShaders();
	
	//! Sets up rendering?
	QString setupProgram( Mesh *, const QString & hint = QString() );
	//! Stops rendering?
	void stopProgram();
	
protected:
	class Condition
	{
	public:
		Condition() {}
		virtual ~Condition() {}
		
		virtual bool eval( const NifModel * nif, const QList<QModelIndex> & iBlocks ) const = 0;
	};
	
	class ConditionSingle : public Condition
	{
	public:
		ConditionSingle( const QString & line, bool neg = false );
		
		bool eval( const NifModel * nif, const QList<QModelIndex> & iBlocks ) const;
	
	protected:
		QString left, right;
		enum Type
		{
			NONE, EQ, NE, LE, GE, LT, GT, AND
		};
		Type comp;
		static QHash<Type,QString> compStrs;
		
		bool invert;
		
		QModelIndex getIndex( const NifModel * nif, const QList<QModelIndex> & iBlock, QString name ) const;
		template <typename T> bool compare( T a, T b ) const;
	};
	
	class ConditionGroup : public Condition
	{
	public:
		ConditionGroup( bool o = false ) { _or = o; }
		~ConditionGroup() { qDeleteAll( conditions ); }
		
		bool eval( const NifModel * nif, const QList<QModelIndex> & iBlocks ) const;
		
		void addCondition( Condition * c );
		
		bool isOrGroup() const { return _or; }
		
	protected:
		QList<Condition*> conditions;
		bool _or;
	};
	
	class Shader
	{
	public:
		Shader( const QString & name, GLenum type );
		~Shader();
		
		bool load( const QString & filepath );
		
		QString	name;
		GLuint	id;
		bool	status;
		
	protected:
		GLenum	type;
	};
	
	class Program
	{
	public:
		Program( const QString & name );
		~Program();
		
		bool load( const QString & filepath, Renderer * );
		
		QString	name;
		GLuint	id;
		bool	status;
		
		ConditionGroup conditions;
		QMap<int, QString> texcoords;
	};	

	QMap<QString, Shader *> shaders;
	QMap<QString, Program *> programs;
	
	friend class Program;

	bool setupProgram( Program *, Mesh *, const PropertyList &, const QList<QModelIndex> & iBlocks );
	void setupFixedFunction( Mesh *, const PropertyList & );	
};

template <typename T> inline bool Renderer::ConditionSingle::compare( T a, T b ) const
{
	switch ( comp )
	{
		case EQ:
			return a == b;
		case NE:
			return a != b;
		case LE:
			return a <= b;
		case GE:
			return a >= b;
		case LT:
			return a < b;
		case GT:
			return a > b;
		case AND:
			return a & b;
		default:
			return true;
	}
}

template <> inline bool Renderer::ConditionSingle::compare( float a, float b ) const
{
	switch ( comp )
	{
		case EQ:
			return a == b;
		case NE:
			return a != b;
		case LE:
			return a <= b;
		case GE:
			return a >= b;
		case LT:
			return a < b;
		case GT:
			return a > b;
		default:
			return true;
	}
}

template <> inline bool Renderer::ConditionSingle::compare( QString a, QString b ) const
{
	switch ( comp )
	{
		case EQ:
			return a == b;
		case NE:
			return a != b;
		default:
			return false;
	}
}

#endif
