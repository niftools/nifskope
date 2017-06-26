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

#include "renderer.h"

#include "message.h"
#include "nifskope.h"
#include "gl/glmesh.h"
#include "gl/glproperty.h"
#include "gl/glscene.h"
#include "gl/gltex.h"
#include "io/material.h"
#include "model/nifmodel.h"
#include "ui/settingsdialog.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QSettings>
#include <QTextStream>


//! @file renderer.cpp Renderer and child classes implementation

bool shader_initialized = false;
bool shader_ready = true;

bool Renderer::initialize()
{
	if ( !shader_initialized ) {

		// check for OpenGL 2.0
		// (we don't use the extension API but the 2.0 API for shaders)
		if ( cfg.useShaders && fn->hasOpenGLFeature( QOpenGLFunctions::Shaders ) ) {
			shader_ready = true;
			shader_initialized = true;
		} else {
			shader_ready = false;
		}

		//qDebug() << "shader support" << shader_ready;
	}

	return shader_ready;
}

bool Renderer::hasShaderSupport()
{
	return shader_ready;
}

const QHash<Renderer::ConditionSingle::Type, QString> Renderer::ConditionSingle::compStrs{
	{ EQ,  " == " },
	{ NE,  " != " },
	{ LE,  " <= " },
	{ GE,  " >= " },
	{ LT,  " < " },
	{ GT,  " > " },
	{ AND, " & " },
	{ NAND, " !& " }
};

Renderer::ConditionSingle::ConditionSingle( const QString & line, bool neg ) : invert( neg )
{
	QHashIterator<Type, QString> i( compStrs );
	int pos = -1;

	while ( i.hasNext() ) {
		i.next();
		pos = line.indexOf( i.value() );

		if ( pos > 0 )
			break;
	}

	if ( pos > 0 ) {
		left  = line.left( pos ).trimmed();
		right = line.right( line.length() - pos - i.value().length() ).trimmed();

		if ( right.startsWith( "\"" ) && right.endsWith( "\"" ) )
			right = right.mid( 1, right.length() - 2 );

		comp = i.key();
	} else {
		left = line;
		comp = NONE;
	}
}

QModelIndex Renderer::ConditionSingle::getIndex( const NifModel * nif, const QList<QModelIndex> & iBlocks, QString blkid ) const
{
	QString childid;

	if ( blkid.startsWith( "HEADER/" ) )
		return nif->getIndex( nif->getHeader(), blkid.remove( "HEADER/" ) );

	int pos = blkid.indexOf( "/" );

	if ( pos > 0 ) {
		childid = blkid.right( blkid.length() - pos - 1 );
		blkid = blkid.left( pos );
	}

	for ( QModelIndex iBlock : iBlocks ) {
		if ( nif->inherits( iBlock, blkid ) ) {
			if ( childid.isEmpty() )
				return iBlock;

			return nif->getIndex( iBlock, childid );
		}
	}
	return QModelIndex();
}

bool Renderer::ConditionSingle::eval( const NifModel * nif, const QList<QModelIndex> & iBlocks ) const
{
	QModelIndex iLeft = getIndex( nif, iBlocks, left );

	if ( !iLeft.isValid() )
		return invert;

	if ( comp == NONE )
		return !invert;

	NifValue val = nif->getValue( iLeft );

	if ( val.isString() )
		return compare( val.toString(), right ) ^ invert;
	else if ( val.isCount() )
		return compare( val.toCount(), right.toUInt( nullptr, 0 ) ) ^ invert;
	else if ( val.isFloat() )
		return compare( val.toFloat(), (float)right.toDouble() ) ^ invert;
	else if ( val.isFileVersion() )
		return compare( val.toFileVersion(), right.toUInt( nullptr, 0 ) ) ^ invert;

	return false;
}

bool Renderer::ConditionGroup::eval( const NifModel * nif, const QList<QModelIndex> & iBlocks ) const
{
	if ( conditions.isEmpty() )
		return true;

	if ( isOrGroup() ) {
		for ( Condition * cond : conditions ) {
			if ( cond->eval( nif, iBlocks ) )
				return true;
		}
		return false;
	} else {
		for ( Condition * cond : conditions ) {
			if ( !cond->eval( nif, iBlocks ) )
				return false;
		}
		return true;
	}
}

void Renderer::ConditionGroup::addCondition( Condition * c )
{
	conditions.append( c );
}

Renderer::Shader::Shader( const QString & n, GLenum t, QOpenGLFunctions * fn )
	: f( fn ), name( n ), id( 0 ), status( false ), type( t )
{
	id = f->glCreateShader( type );
}

Renderer::Shader::~Shader()
{
	if ( id )
		f->glDeleteShader( id );
}

bool Renderer::Shader::load( const QString & filepath )
{
	try
	{
		QFile file( filepath );

		if ( !file.open( QIODevice::ReadOnly ) )
			throw QString( "couldn't open %1 for read access" ).arg( filepath );

		QByteArray data = file.readAll();

		const char * src = data.constData();

		f->glShaderSource( id, 1, &src, 0 );
		f->glCompileShader( id );

		GLint result;
		f->glGetShaderiv( id, GL_COMPILE_STATUS, &result );

		if ( result != GL_TRUE ) {
			GLint logLen;
			f->glGetShaderiv( id, GL_INFO_LOG_LENGTH, &logLen );
			char * log = new char[ logLen ];
			f->glGetShaderInfoLog( id, logLen, 0, log );
			QString errlog( log );
			delete[] log;
			throw errlog;
		}
	}
	catch ( QString & err )
	{
		status = false;
		Message::append( QObject::tr( "There were errors during shader compilation" ), QString( "%1:\r\n\r\n%2" ).arg( name ).arg( err ) );
		return false;
	}
	status = true;
	return true;
}


Renderer::Program::Program( const QString & n, QOpenGLFunctions * fn )
	: f( fn ), name( n ), id( 0 )
{
	id = f->glCreateProgram();
}

Renderer::Program::~Program()
{
	if ( id )
		f->glDeleteShader( id );
}

bool Renderer::Program::load( const QString & filepath, Renderer * renderer )
{
	try
	{
		QFile file( filepath );

		if ( !file.open( QIODevice::ReadOnly ) )
			throw QString( "couldn't open %1 for read access" ).arg( filepath );

		QTextStream stream( &file );

		QStack<ConditionGroup *> chkgrps;
		chkgrps.push( &conditions );

		while ( !stream.atEnd() ) {
			QString line = stream.readLine().trimmed();

			if ( line.startsWith( "shaders" ) ) {
				QStringList list = line.simplified().split( " " );

				for ( int i = 1; i < list.count(); i++ ) {
					Shader * shader = renderer->shaders.value( list[ i ] );

					if ( shader ) {
						if ( shader->status )
							f->glAttachShader( id, shader->id );
						else
							throw QString( "depends on shader %1 which was not compiled successful" ).arg( list[ i ] );
					} else {
						throw QString( "shader %1 not found" ).arg( list[ i ] );
					}
				}
			} else if ( line.startsWith( "checkgroup" ) ) {
				QStringList list = line.simplified().split( " " );

				if ( list.value( 1 ) == "begin" ) {
					ConditionGroup * group = new ConditionGroup( list.value( 2 ) == "or" );
					chkgrps.top()->addCondition( group );
					chkgrps.push( group );
				} else if ( list.value( 1 ) == "end" ) {
					if ( chkgrps.count() > 1 )
						chkgrps.pop();
					else
						throw QString( "mismatching checkgroup end tag" );
				} else {
					throw QString( "expected begin or end after checkgroup" );
				}
			} else if ( line.startsWith( "check" ) ) {
				line = line.remove( 0, 5 ).trimmed();

				bool invert = false;

				if ( line.startsWith( "not " ) ) {
					invert = true;
					line = line.remove( 0, 4 ).trimmed();
				}

				chkgrps.top()->addCondition( new ConditionSingle( line, invert ) );
			} else if ( line.startsWith( "texcoords" ) ) {
				line = line.remove( 0, 9 ).simplified();
				QStringList list = line.split( " " );
				bool ok;
				int unit = list.value( 0 ).toInt( &ok );
				QString idStr = list.value( 1 ).toLower();

				if ( !ok || idStr.isEmpty() )
					throw QString( "malformed texcoord tag" );

				if ( idStr != "tangents" && idStr != "bitangents" && TexturingProperty::getId( idStr ) < 0 )
					throw QString( "texcoord tag referres to unknown texture id '%1'" ).arg( idStr );

				if ( texcoords.contains( unit ) )
					throw QString( "texture unit %1 is assigned twiced" ).arg( unit );

				texcoords.insert( unit, idStr );
			}
		}

		f->glLinkProgram( id );

		GLint result;

		f->glGetProgramiv( id, GL_LINK_STATUS, &result );

		if ( result != GL_TRUE ) {
			GLint logLen = 0;
			f->glGetProgramiv( id, GL_INFO_LOG_LENGTH, &logLen );

			if ( logLen != 0 ) {
				char * log = new char[ logLen ];
				f->glGetProgramInfoLog( id, logLen, 0, log );
				QString errlog( log );
				delete[] log;
				id = 0;
				throw errlog;
			}
		}
	}
	catch ( QString & x )
	{
		status = false;
		Message::append( QObject::tr( "There were errors during shader compilation" ), QString( "%1:\r\n\r\n%2" ).arg( name ).arg( x ) );
		return false;
	}
	status = true;
	return true;
}

Renderer::Renderer( QOpenGLContext * c, QOpenGLFunctions * f )
	: cx( c ), fn( f )
{
	updateSettings();

	connect( NifSkope::getOptions(), &SettingsDialog::saveSettings, this, &Renderer::updateSettings );
}

Renderer::~Renderer()
{
	releaseShaders();
}


void Renderer::updateSettings()
{
	QSettings settings;

	cfg.useShaders = settings.value( "Settings/Render/General/Use Shaders", true ).toBool();

	bool prevStatus = shader_ready;

	shader_ready = cfg.useShaders && fn->hasOpenGLFeature( QOpenGLFunctions::Shaders );
	if ( !shader_initialized && shader_ready && !prevStatus ) {
		updateShaders();
		shader_initialized = true;
	}
}

void Renderer::updateShaders()
{
	if ( !shader_ready )
		return;

	releaseShaders();

	QDir dir( QCoreApplication::applicationDirPath() );

	if ( dir.exists( "shaders" ) )
		dir.cd( "shaders" );

#ifdef Q_OS_LINUX
	else if ( dir.exists( "/usr/share/nifskope/shaders" ) )
		dir.cd( "/usr/share/nifskope/shaders" );
#endif

// linux does not want to load the shaders so disable them for now
#ifdef WIN32
	dir.setNameFilters( { "*.vert" } );
	for ( const QString& name : dir.entryList() ) {
		Shader * shader = new Shader( name, GL_VERTEX_SHADER, fn );
		shader->load( dir.filePath( name ) );
		shaders.insert( name, shader );
	}

	dir.setNameFilters( { "*.frag" } );
	for ( const QString& name : dir.entryList() ) {
		Shader * shader = new Shader( name, GL_FRAGMENT_SHADER, fn );
		shader->load( dir.filePath( name ) );
		shaders.insert( name, shader );
	}

	dir.setNameFilters( { "*.prog" } );
	for ( const QString& name : dir.entryList() ) {
		Program * program = new Program( name, fn );
		program->load( dir.filePath( name ), this );
		programs.insert( name, program );
	}
#endif
}

void Renderer::releaseShaders()
{
	if ( !shader_ready )
		return;

	qDeleteAll( programs );
	programs.clear();
	qDeleteAll( shaders );
	shaders.clear();
}

QString Renderer::setupProgram( Shape * mesh, const QString & hint )
{
	PropertyList props;
	mesh->activeProperties( props );

	if ( !shader_ready || (mesh->scene->options & Scene::DisableShaders) || (mesh->scene->visMode & Scene::VisSilhouette) ) {
		setupFixedFunction( mesh, props );
		return QString( "fixed function pipeline" );
	}

	QList<QModelIndex> iBlocks;
	iBlocks << mesh->index();
	iBlocks << mesh->iData;
	for ( Property * p : props.list() ) {
		iBlocks.append( p->index() );
	}

	if ( !hint.isEmpty() ) {
		Program * program = programs.value( hint );

		if ( program && program->status && setupProgram( program, mesh, props, iBlocks ) )
			return hint;
	}

	for ( Program * program : programs ) {
		if ( program->status && setupProgram( program, mesh, props, iBlocks ) )
			return program->name;
	}

	stopProgram();
	setupFixedFunction( mesh, props );
	return QString( "fixed function pipeline" );
}

void Renderer::stopProgram()
{
	if ( shader_ready ) {
		fn->glUseProgram( 0 );
	}

	resetTextureUnits();
}

bool Renderer::setupProgram( Program * prog, Shape * mesh, const PropertyList & props, const QList<QModelIndex> & iBlocks )
{
	const NifModel * nif = qobject_cast<const NifModel *>( mesh->index().model() );

	if ( !mesh->index().isValid() || !nif )
		return false;

	if ( !prog->conditions.eval( nif, iBlocks ) )
		return false;

	fn->glUseProgram( prog->id );

	auto opts = mesh->scene->options;
	auto vis = mesh->scene->visMode;

	Material * mat = nullptr;
	if ( mesh->bslsp && mesh->bslsp->mat() )
		mat = mesh->bslsp->mat();
	else if ( mesh->bsesp && mesh->bsesp->mat() )
		mat = mesh->bsesp->mat();

	QString diff;

	if ( (opts & Scene::DoLighting) && (vis & Scene::VisNormalsOnly) )
		diff = "shaders/white.dds";

	// texturing

	TexturingProperty * texprop = props.get<TexturingProperty>();
	BSShaderLightingProperty * bsprop = props.get<BSShaderLightingProperty>();
	// TODO: BSLSP has been split off from BSShaderLightingProperty so it needs
	//	to be accessible from here

	TexClampMode clamp = TexClampMode::WRAP_S_WRAP_T;

	if ( mesh->bslsp ) {
		clamp = mesh->bslsp->getClampMode();
	}


	int texunit = 0;

	//GLint baseWidth, baseHeight;

	GLint uniBaseMap = fn->glGetUniformLocation( prog->id, "BaseMap" );

	if ( uniBaseMap >= 0 ) {
		if ( !texprop && !bsprop )
			return false;

		if ( !activateTextureUnit( texunit ) )
			return false;

		if ( (texprop && !texprop->bind( 0 )) || (bsprop && !bsprop->bind( 0, diff, clamp )) )
			return false;

		//glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, (GLint *)&baseWidth );
		//glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, (GLint *)&baseHeight );

		fn->glUniform1i( uniBaseMap, texunit++ );
	}

	GLint uniNormalMap = fn->glGetUniformLocation( prog->id, "NormalMap" );

	if ( uniNormalMap >= 0 ) {
		if ( texprop ) {
			QString fname = texprop->fileName( 0 );

			if ( fname.isEmpty() )
				return false;

			int pos = fname.indexOf( "_" );

			if ( pos >= 0 )
				fname = fname.left( pos ) + "_n.dds";
			else if ( (pos = fname.lastIndexOf( "." )) >= 0 )
				fname = fname.insert( pos, "_n" );

			if ( !activateTextureUnit( texunit ) || !texprop->bind( 0, fname ) )
				return false;
		} else if ( bsprop ) {
			QString fname = bsprop->fileName( 1 );

			if ( !(opts & Scene::DoLighting) )
				fname = "shaders/default_n.dds";

			if ( !fname.isEmpty() && (!activateTextureUnit( texunit ) || !bsprop->bind( 1, fname, clamp )) )
				return false;
		}

		fn->glUniform1i( uniNormalMap, texunit++ );
	}

	GLint uniGlowMap = fn->glGetUniformLocation( prog->id, "GlowMap" );

	if ( uniGlowMap >= 0 ) {
		if ( texprop ) {
			QString fname = texprop->fileName( 0 );

			if ( fname.isEmpty() )
				return false;

			int pos = fname.indexOf( "_" );

			if ( pos >= 0 )
				fname = fname.left( pos ) + "_g.dds";
			else if ( (pos = fname.lastIndexOf( "." )) >= 0 )
				fname = fname.insert( pos, "_g" );

			if ( !activateTextureUnit( texunit ) || !texprop->bind( 0, fname ) )
				return false;
		} else if ( bsprop ) {
			QString fname = bsprop->fileName( 2 );

			if ( !fname.isEmpty() && (!activateTextureUnit( texunit ) || !bsprop->bind( 2, fname, clamp )) )
				return false;
		}

		fn->glUniform1i( uniGlowMap, texunit++ );
	}


	// Sets a float
	auto uni1f = [this, prog, mesh]( const char * var, float x ) {
		GLint uni = fn->glGetUniformLocation( prog->id, var );
		if ( uni >= 0 )
			fn->glUniform1f( uni, x );
	};

	// Sets a vec2 (two floats)
	auto uni2f = [this, prog, mesh]( const char * var, float x, float y ) {
		GLint uni = fn->glGetUniformLocation( prog->id, var );
		if ( uni >= 0 )
			fn->glUniform2f( uni, x, y );
	};

	// Sets a vec3 (three floats)
	auto uni3f = [this, prog, mesh]( const char * var, float x, float y, float z ) {
		GLint uni = fn->glGetUniformLocation( prog->id, var );
		if ( uni >= 0 )
			fn->glUniform3f( uni, x, y, z );
	};

	// Sets a vec4 (four floats)
	auto uni4f = [this, prog, mesh]( const char * var, float x, float y, float z, float w ) {
		GLint uni = fn->glGetUniformLocation( prog->id, var );
		if ( uni >= 0 )
			fn->glUniform4f( uni, x, y, z, w );
	};

	// Sets an integer or boolean
	auto uni1i = [this, prog, mesh]( const char * var, int val ) {
		GLint uni = fn->glGetUniformLocation( prog->id, var );
		if ( uni >= 0 )
			fn->glUniform1i( uni, val );
	};

	// Sets a mat3 (3x3 matrix)
	auto uni3m = [this, prog, mesh]( const char * var, Matrix val ) {
		GLint uni = fn->glGetUniformLocation( prog->id, var );
		if ( uni >= 0 ) {
			fn->glUniformMatrix3fv( uni, 1, 0, val.data() );
		}
	};

	// Sets a mat4 (4x4 matrix)
	auto uni4m = [this, prog, mesh]( const char * var, Matrix4 val ) {
		GLint uni = fn->glGetUniformLocation( prog->id, var );
		if ( uni >= 0 ) {
			fn->glUniformMatrix4fv( uni, 1, 0, val.data() );
		}
	};

	// Sets a sampler2D (texture sampler)
	auto uniSampler = [this, prog, bsprop, &texunit]( const char * var, int textureSlot, QString alternate, TexClampMode clamp ) {
		GLint uniSamp = fn->glGetUniformLocation( prog->id, var );
		if ( uniSamp >= 0 ) {

			QString fname = bsprop->fileName( textureSlot );
			if ( fname.isEmpty() )
				fname = alternate;

			if ( !fname.isEmpty() && (!activateTextureUnit( texunit ) || !bsprop->bind( textureSlot, fname, clamp )) )
				return false;

			fn->glUniform1i( uniSamp, texunit++ );

			return true;
		}

		return true;
	};

	QString white = "shaders/white.dds";
	QString black = "shaders/black.dds";
	QString default_n = "shaders/default_n.dds";
	

	// BSLightingShaderProperty
	if ( mesh->bslsp ) {
		uni1f( "lightingEffect1", mesh->bslsp->getLightingEffect1() );
		uni1f( "lightingEffect2", mesh->bslsp->getLightingEffect2() );

		uni1f( "alpha", mesh->bslsp->getAlpha() );

		auto uvS = mesh->bslsp->getUvScale();
		uni2f( "uvScale", uvS.x, uvS.y );

		auto uvO = mesh->bslsp->getUvOffset();
		uni2f( "uvOffset", uvO.x, uvO.y );

		uni4m( "viewMatrix", mesh->viewTrans().toMatrix4() );
		uni4m( "viewMatrixInverse", mesh->viewTrans().toMatrix4().inverted() );

		uni4m( "localMatrix", mesh->localTrans().toMatrix4() );
		uni4m( "localMatrixInverse", mesh->localTrans().toMatrix4().inverted() );

		uni4m( "worldMatrix", mesh->worldTrans().toMatrix4() );
		uni4m( "worldMatrixInverse", mesh->worldTrans().toMatrix4().inverted() );

		uni1i( "greyscaleColor", mesh->bslsp->greyscaleColor );
		if ( mesh->bslsp->greyscaleColor ) {
			if ( !uniSampler( "GreyscaleMap", 3, "", TexClampMode::MIRRORED_S_MIRRORED_T ) )
				return false;
		}

		uni1i( "hasTintColor", mesh->bslsp->hasTintColor );
		if ( mesh->bslsp->hasTintColor ) {
			auto tC = mesh->bslsp->getTintColor();
			uni3f( "tintColor", tC.red(), tC.green(), tC.blue() );
		}

		uni1i( "hasDetailMask", mesh->bslsp->hasDetailMask );
		if ( mesh->bslsp->hasDetailMask ) {
			if ( !uniSampler( "DetailMask", 3, "shaders/blankdetailmap.dds", clamp ) )
				return false;
		}

		uni1i( "hasTintMask", mesh->bslsp->hasTintMask );
		if ( mesh->bslsp->hasTintMask ) {
			if ( !uniSampler( "TintMask", 6, "shaders/gray.dds", clamp ) )
				return false;
		}

		// Rim & Soft params

		uni1i( "hasSoftlight", mesh->bslsp->hasSoftlight );
		uni1i( "hasRimlight", mesh->bslsp->hasRimlight );

		if ( nif->getUserVersion2() < 130 && (mesh->bslsp->hasSoftlight || mesh->bslsp->hasRimlight) ) {

			if ( !uniSampler( "LightMask", 2, default_n, clamp ) )
				return false;
		}

		// Backlight params

		uni1i( "hasBacklight", mesh->bslsp->hasBacklight );

		if ( nif->getUserVersion2() < 130 && mesh->bslsp->hasBacklight ) {

			if ( !uniSampler( "BacklightMap", 7, default_n, clamp ) )
				return false;
		}

		// Glow params

		if ( (opts & Scene::DoGlow) && (opts & Scene::DoLighting) && mesh->bslsp->hasEmittance )
			uni1f( "glowMult", mesh->bslsp->getEmissiveMult() );
		else
			uni1f( "glowMult", 0 );
		
		uni1i( "hasEmit", mesh->bslsp->hasEmittance );
		uni1i( "hasGlowMap", mesh->bslsp->hasGlowMap );
		auto emC = mesh->bslsp->getEmissiveColor();
		uni3f( "glowColor", emC.red(), emC.green(), emC.blue() );

		// Specular params

		if ( (opts & Scene::DoSpecular) && (opts & Scene::DoLighting) )
			uni1f( "specStrength", mesh->bslsp->getSpecularStrength() );
		else
			uni1f( "specStrength", 0 );

		// Assure specular power does not break the shaders
		auto gloss = mesh->bslsp->getSpecularGloss();
		uni1f( "specGlossiness", gloss );
		
		auto spec = mesh->bslsp->getSpecularColor();
		uni3f( "specColor", spec.red(), spec.green(), spec.blue() );

		uni1i( "hasSpecularMap", mesh->bslsp->hasSpecularMap );

		if ( mesh->bslsp->hasSpecularMap && (nif->getUserVersion2() == 130 || !mesh->bslsp->hasBacklight) ) {
			if ( !uniSampler( "SpecularMap", 7, white, clamp ) )
				return false;
		}

		if ( nif->getUserVersion2() == 130 ) {
			uni1i( "doubleSided", mesh->isDoubleSided );
			uni1f( "paletteScale", mesh->bslsp->paletteScale );
			uni1f( "subsurfaceRolloff", mesh->bslsp->getLightingEffect1() );
			uni1f( "fresnelPower", mesh->bslsp->fresnelPower );
			uni1f( "rimPower", mesh->bslsp->rimPower );
			uni1f( "backlightPower", mesh->bslsp->backlightPower );
		}

		// Multi-Layer

		if ( mesh->bslsp->hasMultiLayerParallax ) {

			auto inS = mesh->bslsp->getInnerTextureScale();
			uni2f( "innerScale", inS.x, inS.y );

			uni1f( "innerThickness", mesh->bslsp->getInnerThickness() );

			uni1f( "outerRefraction", mesh->bslsp->getOuterRefractionStrength() );
			uni1f( "outerReflection", mesh->bslsp->getOuterReflectionStrength() );

			if ( !uniSampler( "InnerMap", 6, default_n, clamp ) )
				return false;
		}

		// Environment Mapping

		uni1i( "hasCubeMap", mesh->bslsp->hasCubeMap );
		uni1i( "hasEnvMask", mesh->bslsp->useEnvironmentMask );

		if ( mesh->bslsp->hasCubeMap && mesh->bslsp->hasEnvironmentMap && (opts & Scene::DoCubeMapping) && (opts & Scene::DoLighting) ) {

			uni1f( "envReflection", mesh->bslsp->getEnvironmentReflection() );

			// Do not test useEnvironmentMask here, always pass white.dds as fallback
			if ( !uniSampler( "EnvironmentMap", 5, white, clamp ) )
				return false;

			GLint uniCubeMap = fn->glGetUniformLocation( prog->id, "CubeMap" );
			if ( uniCubeMap >= 0 ) {

				QString fname = bsprop->fileName( 4 );

				if ( !fname.isEmpty() && (!activateTextureUnit( texunit ) || !bsprop->bindCube( 4, fname )) )
					return false;

				fn->glUniform1i( uniCubeMap, texunit++ );
			}
		} else {
			// In the case that the cube texture has already been bound,
			//	but SLSF1_Environment_Mapping is not set, assure that it 
			//	removes reflections.
			uni1f( "envReflection", 0 );
		}

		// Parallax

		if ( mesh->bslsp->hasHeightMap ) {
			if ( !uniSampler( "HeightMap", 3, "shaders/gray.dds", clamp ) )
				return false;
		}
	}


	// BSEffectShaderProperty
	if ( mesh->bsesp ) {

		uni4m( "worldMatrix", mesh->worldTrans().toMatrix4() );

		clamp = mesh->bsesp->getClampMode();

		if ( !uniSampler( "SourceTexture", 0, white, clamp ) )
			return false;

		uni1i( "doubleSided", mesh->bsesp->getIsDoubleSided() );

		auto uvS = mesh->bsesp->getUvScale();
		uni2f( "uvScale", uvS.x, uvS.y );

		auto uvO = mesh->bsesp->getUvOffset();
		uni2f( "uvOffset", uvO.x, uvO.y );

		uni1i( "hasSourceTexture", mesh->bsesp->hasSourceTexture );
		uni1i( "hasGreyscaleMap", mesh->bsesp->hasGreyscaleMap );

		uni1i( "greyscaleAlpha", mesh->bsesp->greyscaleAlpha );
		uni1i( "greyscaleColor", mesh->bsesp->greyscaleColor );


		uni1i( "useFalloff", mesh->bsesp->useFalloff );
		uni1i( "vertexAlpha", mesh->bsesp->vertexAlpha );
		uni1i( "vertexColors", mesh->bsesp->vertexColors );

		uni1i( "hasWeaponBlood", mesh->bsesp->hasWeaponBlood );

		// Glow params

		auto emC = mesh->bsesp->getEmissiveColor();
		uni4f( "glowColor", emC.red(), emC.green(), emC.blue(), emC.alpha() );
		uni1f( "glowMult", mesh->bsesp->getEmissiveMult() );

		// Falloff params

		uni4f( "falloffParams",
			mesh->bsesp->falloff.startAngle, mesh->bsesp->falloff.stopAngle,
			mesh->bsesp->falloff.startOpacity, mesh->bsesp->falloff.stopOpacity
		);

		uni1f( "falloffDepth", mesh->bsesp->falloff.softDepth );

		// BSEffectShader textures
		if ( mesh->bsesp->hasGreyscaleMap ) {
			if ( !uniSampler( "GreyscaleMap", 1, "", TexClampMode::MIRRORED_S_MIRRORED_T ) )
				return false;
		}

		if ( nif->getUserVersion2() == 130 ) {

			uni1f( "lightingInfluence", mesh->bsesp->getLightingInfluence() );

			uni1i( "hasNormalMap", mesh->bsesp->hasNormalMap && (opts & Scene::DoLighting) );

			uniSampler( "NormalMap", 3, default_n, clamp );

			uni1i( "hasCubeMap", mesh->bsesp->hasEnvMap );
			uni1i( "hasEnvMask", mesh->bsesp->hasEnvMask );

			if ( mesh->bsesp->hasEnvMap && (opts & Scene::DoCubeMapping) && (opts & Scene::DoLighting) ) {
				uni1f( "envReflection", mesh->bsesp->getEnvironmentReflection() );

				if ( mesh->bsesp->hasEnvMask && !uniSampler( "SpecularMap", 4, white, clamp ) )
					return false;

				GLint uniCubeMap = fn->glGetUniformLocation( prog->id, "CubeMap" );
				if ( uniCubeMap >= 0 ) {

					QString fname = bsprop->fileName( 2 );

					if ( !fname.isEmpty() && (!activateTextureUnit( texunit ) || !bsprop->bindCube( 2, fname )) )
						return false;

					fn->glUniform1i( uniCubeMap, texunit++ );
				}
			} else {
				uni1f( "envReflection", 0 );
			}
		
		}
	}

	// Defaults for uniforms in older meshes
	if ( !mesh->bsesp && !mesh->bslsp ) {
		uni2f( "uvScale", 1.0, 1.0 );
		uni2f( "uvOffset", 0.0, 0.0 );
	}

	QMapIterator<int, QString> itx( prog->texcoords );

	while ( itx.hasNext() ) {
		itx.next();

		if ( !activateTextureUnit( itx.key() ) )
			return false;

		if ( itx.value() == "tangents" ) {
			if ( mesh->transTangents.count() ) {
				glEnableClientState( GL_TEXTURE_COORD_ARRAY );
				glTexCoordPointer( 3, GL_FLOAT, 0, mesh->transTangents.data() );
			} else if ( mesh->tangents.count() ) {
				glEnableClientState( GL_TEXTURE_COORD_ARRAY );
				glTexCoordPointer( 3, GL_FLOAT, 0, mesh->tangents.data() );
			} else {
				return false;
			}

		} else if ( itx.value() == "bitangents" ) {
			if ( mesh->transBitangents.count() ) {
				glEnableClientState( GL_TEXTURE_COORD_ARRAY );
				glTexCoordPointer( 3, GL_FLOAT, 0, mesh->transBitangents.data() );
			} else if ( mesh->bitangents.count() ) {
				glEnableClientState( GL_TEXTURE_COORD_ARRAY );
				glTexCoordPointer( 3, GL_FLOAT, 0, mesh->bitangents.data() );
			} else {
				return false;
			}
		} else if ( texprop ) {
			int txid = TexturingProperty::getId( itx.value() );
			if ( txid < 0 )
				return false;

			int set = texprop->coordSet( txid );

			if ( set < 0 || !(set < mesh->coords.count()) || !mesh->coords[set].count() )
				return false;

			glEnableClientState( GL_TEXTURE_COORD_ARRAY );
			glTexCoordPointer( 2, GL_FLOAT, 0, mesh->coords[set].data() );
		} else if ( bsprop ) {
			int txid = BSShaderLightingProperty::getId( itx.value() );
			if ( txid < 0 )
				return false;

			int set = 0;

			if ( set < 0 || !(set < mesh->coords.count()) || !mesh->coords[set].count() )
				return false;

			glEnableClientState( GL_TEXTURE_COORD_ARRAY );
			glTexCoordPointer( 2, GL_FLOAT, 0, mesh->coords[set].data() );
		}
	}

	// setup lighting

	glEnable( GL_LIGHTING );

	// setup blending

	glProperty( props.get<AlphaProperty>() );

	if ( mat && (mesh->scene->options & Scene::DoBlending) ) {
		static const GLenum blendMap[11] = {
			GL_ONE, GL_ZERO, GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR,
			GL_DST_COLOR, GL_ONE_MINUS_DST_COLOR, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
			GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA, GL_SRC_ALPHA_SATURATE
		};

		if ( mat && mat->bAlphaBlend ) {
			glDisable( GL_POLYGON_OFFSET_FILL );
			glEnable( GL_BLEND );
			glBlendFunc( blendMap[mat->iAlphaSrc], blendMap[mat->iAlphaDst] );
		} else {
			glDisable( GL_BLEND );
		}

		if ( mat && mat->bAlphaTest ) {
			glDisable( GL_POLYGON_OFFSET_FILL );
			glEnable( GL_ALPHA_TEST );
			glAlphaFunc( GL_GREATER, float( mat->iAlphaTestRef ) / 255.0 );
		} else {
			glDisable( GL_ALPHA_TEST );
		}

		if ( mat && mat->bDecal ) {
			glEnable( GL_POLYGON_OFFSET_FILL );
			glPolygonOffset( -1.0f, -1.0f );
		}
	}

	// BSESP/BSLSP do not always need an NiAlphaProperty, and appear to override it at times
	if ( !mat && mesh->translucent ) {
		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		// If mesh is alpha tested, override threshold
		glAlphaFunc( GL_GREATER, 0.1f );
	}

	// setup vertex colors

	//glProperty( props.get< VertexColorProperty >(), glIsEnabled( GL_COLOR_ARRAY ) );
	glDisable( GL_COLOR_MATERIAL );

	// setup material

	glProperty( props.get<MaterialProperty>(), props.get<SpecularProperty>() );

	// setup z buffer

	glProperty( props.get<ZBufferProperty>() );

	if ( !mesh->depthTest ) {
		glDisable( GL_DEPTH_TEST );
	}

	if ( !mesh->depthWrite || mesh->translucent ) {
		glDepthMask( GL_FALSE );
	}

	// setup stencil

	glProperty( props.get<StencilProperty>() );

	// wireframe ?

	glProperty( props.get<WireframeProperty>() );

	return true;
}

void Renderer::setupFixedFunction( Shape * mesh, const PropertyList & props )
{
	// setup lighting

	glEnable( GL_LIGHTING );

	// Disable specular because it washes out vertex colors
	//	at perpendicular viewing angles
	float color[4] = { 0, 0, 0, 0 };
	glMaterialfv( GL_FRONT_AND_BACK, GL_SPECULAR, color );
	glLightfv( GL_LIGHT0, GL_SPECULAR, color );

	// setup blending

	glProperty( props.get<AlphaProperty>() );

	// setup vertex colors

	glProperty( props.get<VertexColorProperty>(), glIsEnabled( GL_COLOR_ARRAY ) );

	// setup material

	glProperty( props.get<MaterialProperty>(), props.get<SpecularProperty>() );

	// setup texturing

	//glProperty( props.get< TexturingProperty >() );

	// setup z buffer

	glProperty( props.get<ZBufferProperty>() );

	if ( !mesh->depthTest ) {
		glDisable( GL_DEPTH_TEST );
	}

	if ( !mesh->depthWrite ) {
		glDepthMask( GL_FALSE );
	}

	// setup stencil

	glProperty( props.get<StencilProperty>() );

	// wireframe ?

	glProperty( props.get<WireframeProperty>() );

	// normalize

	if ( glIsEnabled( GL_NORMAL_ARRAY ) )
		glEnable( GL_NORMALIZE );
	else
		glDisable( GL_NORMALIZE );

	// setup texturing

	if ( TexturingProperty * texprop = props.get<TexturingProperty>() ) {
		// standard multi texturing property
		int stage = 0;

		if ( texprop->bind( 1, mesh->coords, stage ) ) {
			// dark
			stage++;
			glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE );

			glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PREVIOUS );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_TEXTURE );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR );

			glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PREVIOUS );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE1_ALPHA, GL_TEXTURE );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA );

			glTexEnvf( GL_TEXTURE_ENV, GL_RGB_SCALE, 1.0 );
		}

		if ( texprop->bind( 0, mesh->coords, stage ) ) {
			// base
			stage++;
			glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE );

			glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PREVIOUS );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_TEXTURE );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR );

			glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PREVIOUS );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE1_ALPHA, GL_TEXTURE );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA );

			glTexEnvf( GL_TEXTURE_ENV, GL_RGB_SCALE, 1.0 );
		}

		if ( texprop->bind( 2, mesh->coords, stage ) ) {
			// detail
			stage++;
			glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE );

			glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PREVIOUS );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_TEXTURE );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR );

			glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PREVIOUS );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE1_ALPHA, GL_TEXTURE );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA );

			glTexEnvf( GL_TEXTURE_ENV, GL_RGB_SCALE, 2.0 );
		}

		if ( texprop->bind( 6, mesh->coords, stage ) ) {
			// decal 0
			stage++;
			glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE );

			glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_INTERPOLATE );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PREVIOUS );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE2_RGB, GL_TEXTURE );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND2_RGB, GL_SRC_ALPHA );

			glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PREVIOUS );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA );

			glTexEnvf( GL_TEXTURE_ENV, GL_RGB_SCALE, 1.0 );
		}

		if ( texprop->bind( 7, mesh->coords, stage ) ) {
			// decal 1
			stage++;
			glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE );

			glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_INTERPOLATE );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PREVIOUS );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE2_RGB, GL_TEXTURE );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND2_RGB, GL_SRC_ALPHA );

			glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PREVIOUS );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA );

			glTexEnvf( GL_TEXTURE_ENV, GL_RGB_SCALE, 1.0 );
		}

		if ( texprop->bind( 8, mesh->coords, stage ) ) {
			// decal 2
			stage++;
			glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE );

			glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_INTERPOLATE );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PREVIOUS );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE2_RGB, GL_TEXTURE );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND2_RGB, GL_SRC_ALPHA );

			glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PREVIOUS );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA );

			glTexEnvf( GL_TEXTURE_ENV, GL_RGB_SCALE, 1.0 );
		}

		if ( texprop->bind( 9, mesh->coords, stage ) ) {
			// decal 3
			stage++;
			glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE );

			glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_INTERPOLATE );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PREVIOUS );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE2_RGB, GL_TEXTURE );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND2_RGB, GL_SRC_ALPHA );

			glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PREVIOUS );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA );

			glTexEnvf( GL_TEXTURE_ENV, GL_RGB_SCALE, 1.0 );
		}

		if ( texprop->bind( 4, mesh->coords, stage ) ) {
			// glow
			stage++;
			glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE );

			glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_ADD );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PREVIOUS );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_TEXTURE );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR );

			glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PREVIOUS );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA );

			glTexEnvf( GL_TEXTURE_ENV, GL_RGB_SCALE, 1.0 );
		}
	} else if ( TextureProperty * texprop = props.get<TextureProperty>() ) {
		// old single texture property
		texprop->bind( mesh->coords );
	} else if ( BSShaderLightingProperty * texprop = props.get<BSShaderLightingProperty>() ) {
		// standard multi texturing property
		int stage = 0;

		if ( texprop->bind( 0, mesh->coords ) ) {
			//, mesh->coords, stage ) )
			// base
			stage++;
			glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE );

			glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PREVIOUS );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_TEXTURE );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR );

			glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PREVIOUS );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE1_ALPHA, GL_TEXTURE );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA );

			glTexEnvf( GL_TEXTURE_ENV, GL_RGB_SCALE, 1.0 );
		}
	} else {
		glDisable( GL_TEXTURE_2D );
	}
}

