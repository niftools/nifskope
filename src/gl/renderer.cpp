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

QModelIndex Renderer::ConditionSingle::getIndex( const NifModel * nif, const QVector<QModelIndex> & iBlocks, QString blkid ) const
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

bool Renderer::ConditionSingle::eval( const NifModel * nif, const QVector<QModelIndex> & iBlocks ) const
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
	else if ( val.type() == NifValue::tBSVertexDesc )
		return compare( (uint)val.get<BSVertexDesc>().GetFlags(), right.toUInt( nullptr, 0 ) ) ^ invert;

	return false;
}

bool Renderer::ConditionGroup::eval( const NifModel * nif, const QVector<QModelIndex> & iBlocks ) const
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

				int id = -1;
				if ( idStr == "tangents" )
					id = CT_TANGENT;
				else if ( idStr == "bitangents" )
					id = CT_BITANGENT;
				else if ( idStr == "indices" )
					id = CT_BONE;
				else if ( idStr == "weights" )
					id = CT_WEIGHT;
				else if ( idStr == "base" )
					id = TexturingProperty::getId( idStr );

				if ( id < 0 )
					throw QString( "texcoord tag refers to unknown texture id '%1'" ).arg( idStr );

				if ( texcoords.contains( unit ) )
					throw QString( "texture unit %1 is assigned twiced" ).arg( unit );

				texcoords.insert( unit, CoordType(id) );
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

void Renderer::Program::setUniformLocations()
{
	for ( int i = 0; i < NUM_UNIFORM_TYPES; i++ )
		uniformLocations[i] = f->glGetUniformLocation( id, uniforms[i].c_str() );
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
		program->setUniformLocations();
		programs.insert( name, program );
	}
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

	if ( !shader_ready || hint.isNull()
		 || (mesh->scene->options & Scene::DisableShaders)
		 || (mesh->scene->visMode & Scene::VisSilhouette)
		 || (mesh->nifVersion == 0) ) {
		setupFixedFunction( mesh, props );
		return {};
	}

	QVector<QModelIndex> iBlocks;
	iBlocks << mesh->index();
	iBlocks << mesh->iData;
	for ( Property * p : props.list() ) {
		iBlocks.append( p->index() );
	}

	if ( !hint.isEmpty() ) {
		Program * program = programs.value( hint );
		if ( program && program->status && setupProgram( program, mesh, props, iBlocks, false ) )
			return program->name;
	}

	for ( Program * program : programs ) {
		if ( program->status && setupProgram( program, mesh, props, iBlocks ) )
			return program->name;
	}

	stopProgram();
	setupFixedFunction( mesh, props );
	return {};
}

void Renderer::stopProgram()
{
	if ( shader_ready ) {
		fn->glUseProgram( 0 );
	}

	resetTextureUnits();
}

void Renderer::Program::uni1f( UniformType var, float x )
{
	f->glUniform1f( uniformLocations[var], x );
}

void Renderer::Program::uni2f( UniformType var, float x, float y )
{
	f->glUniform2f( uniformLocations[var], x, y );
}

void Renderer::Program::uni3f( UniformType var, float x, float y, float z )
{
	f->glUniform3f( uniformLocations[var], x, y, z );
}

void Renderer::Program::uni4f( UniformType var, float x, float y, float z, float w )
{
	f->glUniform4f( uniformLocations[var], x, y, z, w );
}

void Renderer::Program::uni1i( UniformType var, int val )
{
	f->glUniform1i( uniformLocations[var], val );
}

void Renderer::Program::uni3m( UniformType var, const Matrix & val )
{
	if ( uniformLocations[var] >= 0 )
		f->glUniformMatrix3fv( uniformLocations[var], 1, 0, val.data() );
}

void Renderer::Program::uni4m( UniformType var, const Matrix4 & val )
{
	if ( uniformLocations[var] >= 0 )
		f->glUniformMatrix4fv( uniformLocations[var], 1, 0, val.data() );
}

bool Renderer::Program::uniSampler( BSShaderLightingProperty * bsprop, UniformType var,
									int textureSlot, int & texunit, const QString & alternate,
									uint clamp, const QString & forced )
{
	GLint uniSamp = uniformLocations[var];
	if ( uniSamp >= 0 ) {

		QString fname = (forced.isEmpty()) ? bsprop->fileName( textureSlot ) : forced;
		if ( fname.isEmpty() )
			fname = alternate;

		if ( !fname.isEmpty() && (!activateTextureUnit( texunit ) 
								   || !(bsprop->bind( textureSlot, fname, TexClampMode(clamp) ) 
										|| bsprop->bind( textureSlot, alternate, TexClampMode(3) ))) )
			return uniSamplerBlank( var, texunit );

		f->glUniform1i( uniSamp, texunit++ );

		return true;
	}

	return true;
}

bool Renderer::Program::uniSamplerBlank( UniformType var, int & texunit )
{
	GLint uniSamp = uniformLocations[var];
	if ( uniSamp >= 0 ) {
		if ( !activateTextureUnit( texunit ) )
			return false;

		glBindTexture( GL_TEXTURE_2D, 0 );
		f->glUniform1i( uniSamp, texunit++ );

		return true;
	}

	return true;
}

static QString white = "shaders/white.dds";
static QString black = "shaders/black.dds";
static QString gray = "shaders/gray.dds";
static QString magenta = "shaders/magenta.dds";
static QString default_n = "shaders/default_n.dds";
static QString cube = "shaders/cubemap.dds";

bool Renderer::setupProgram( Program * prog, Shape * mesh, const PropertyList & props,
							 const QVector<QModelIndex> & iBlocks, bool eval )
{
	const NifModel * nif = qobject_cast<const NifModel *>( mesh->index().model() );

	if ( !mesh->index().isValid() || !nif )
		return false;

	if ( eval && !prog->conditions.eval( nif, iBlocks ) )
		return false;

	fn->glUseProgram( prog->id );

	auto opts = mesh->scene->options;
	auto vis = mesh->scene->visMode;

	Material * mat = nullptr;
	if ( mesh->bslsp && mesh->bslsp->mat() )
		mat = mesh->bslsp->mat();
	else if ( mesh->bsesp && mesh->bsesp->mat() )
		mat = mesh->bsesp->mat();


	// texturing

	TexturingProperty * texprop = props.get<TexturingProperty>();
	BSShaderLightingProperty * bsprop = props.get<BSShaderLightingProperty>();
	// TODO: BSLSP has been split off from BSShaderLightingProperty so it needs
	//	to be accessible from here

	TexClampMode clamp = TexClampMode::WRAP_S_WRAP_T;
	if ( mesh->bslsp )
		clamp = mesh->bslsp->getClampMode();

	int texunit = 0;
	if ( bsprop ) {
		QString forced;
		if ( (opts & Scene::DoLighting) && (vis & Scene::VisNormalsOnly) )
			forced = white;

		QString alt = white;
		if ( opts & Scene::DoErrorColor )
			alt = magenta;

		bool result = prog->uniSampler( bsprop, SAMP_BASE, 0, texunit, alt, clamp, forced );
	} else {
		GLint uniBaseMap = prog->uniformLocations[SAMP_BASE];
		if ( uniBaseMap >= 0 && (texprop || (bsprop && mesh->bslsp)) ) {
			if ( !activateTextureUnit( texunit ) || (texprop && !texprop->bind( 0 )) )
				prog->uniSamplerBlank( SAMP_BASE, texunit );
			else
				fn->glUniform1i( uniBaseMap, texunit++ );
		}
	}

	if ( bsprop && mesh->bslsp ) {
		QString forced;
		if ( !(opts & Scene::DoLighting) )
			forced = default_n;
		prog->uniSampler( bsprop, SAMP_NORMAL, 1, texunit, default_n, clamp, forced );
	} else if ( !bsprop ) {
		GLint uniNormalMap = prog->uniformLocations[SAMP_NORMAL];
		if ( uniNormalMap >= 0 && texprop ) {
			bool result = true;
			if ( texprop ) {
				QString fname = texprop->fileName( 0 );
				if ( !fname.isEmpty() ) {
					int pos = fname.lastIndexOf( "_" );
					if ( pos >= 0 )
						fname = fname.left( pos ) + "_n.dds";
					else if ( (pos = fname.lastIndexOf( "." )) >= 0 )
						fname = fname.insert( pos, "_n" );
				}

				if ( !fname.isEmpty() && (!activateTextureUnit( texunit ) || !texprop->bind( 0, fname )) )
					result = false;
			}

			if ( !result )
				prog->uniSamplerBlank( SAMP_NORMAL, texunit );
			else
				fn->glUniform1i( uniNormalMap, texunit++ );
		}
	}

	if ( bsprop && mesh->bslsp ) {
		prog->uniSampler( bsprop, SAMP_GLOW, 2, texunit, black, clamp );
	} else if ( !bsprop ) {
		GLint uniGlowMap = prog->uniformLocations[SAMP_GLOW];
		if ( uniGlowMap >= 0 && texprop ) {
			bool result = true;
			if ( texprop ) {
				QString fname = texprop->fileName( 0 );
				if ( !fname.isEmpty() ) {
					int pos = fname.lastIndexOf( "_" );
					if ( pos >= 0 )
						fname = fname.left( pos ) + "_g.dds";
					else if ( (pos = fname.lastIndexOf( "." )) >= 0 )
						fname = fname.insert( pos, "_g" );
				}

				if ( !fname.isEmpty() && (!activateTextureUnit( texunit ) || !texprop->bind( 0, fname )) )
					result = false;
			}

			if ( !result )
				prog->uniSamplerBlank( SAMP_GLOW, texunit );
			else
				fn->glUniform1i( uniGlowMap, texunit++ );
		}
	}


	// BSLightingShaderProperty
	if ( mesh->bslsp ) {
		prog->uni1f( LIGHT_EFF1, mesh->bslsp->getLightingEffect1() );
		prog->uni1f( LIGHT_EFF2, mesh->bslsp->getLightingEffect2() );

		prog->uni1f( ALPHA, mesh->bslsp->getAlpha() );

		auto uvS = mesh->bslsp->getUvScale();
		prog->uni2f( UV_SCALE, uvS.x, uvS.y );

		auto uvO = mesh->bslsp->getUvOffset();
		prog->uni2f( UV_OFFSET, uvO.x, uvO.y );

		prog->uni4m( MAT_VIEW, mesh->viewTrans().toMatrix4() );
		prog->uni4m( MAT_WORLD, mesh->worldTrans().toMatrix4() );

		prog->uni1i( G2P_COLOR, mesh->bslsp->greyscaleColor );
		prog->uniSampler( bsprop, SAMP_GRAYSCALE, 3, texunit, "", TexClampMode::MIRRORED_S_MIRRORED_T );

		prog->uni1i( HAS_TINT_COLOR, mesh->bslsp->hasTintColor );
		if ( mesh->bslsp->hasTintColor ) {
			auto tC = mesh->bslsp->getTintColor();
			prog->uni3f( TINT_COLOR, tC.red(), tC.green(), tC.blue() );
		}

		prog->uni1i( HAS_MAP_DETAIL, mesh->bslsp->hasDetailMask );
		prog->uniSampler( bsprop, SAMP_DETAIL, 3, texunit, "shaders/blankdetailmap.dds", clamp );

		prog->uni1i( HAS_MAP_TINT, mesh->bslsp->hasTintMask );
		prog->uniSampler( bsprop, SAMP_TINT, 6, texunit, gray, clamp );

		// Rim & Soft params

		prog->uni1i( HAS_SOFT, mesh->bslsp->hasSoftlight );
		prog->uni1i( HAS_RIM, mesh->bslsp->hasRimlight );

		prog->uniSampler( bsprop, SAMP_LIGHT, 2, texunit, default_n, clamp );

		// Backlight params

		prog->uni1i( HAS_MAP_BACK, mesh->bslsp->hasBacklight );

		prog->uniSampler( bsprop, SAMP_BACKLIGHT, 7, texunit, default_n, clamp );

		// Glow params

		if ( (opts & Scene::DoGlow) && (opts & Scene::DoLighting) && mesh->bslsp->hasEmittance )
			prog->uni1f( GLOW_MULT, mesh->bslsp->getEmissiveMult() );
		else
			prog->uni1f( GLOW_MULT, 0 );
		
		prog->uni1i( HAS_EMIT, mesh->bslsp->hasEmittance );
		prog->uni1i( HAS_MAP_GLOW, mesh->bslsp->hasGlowMap );
		auto emC = mesh->bslsp->getEmissiveColor();
		prog->uni3f( GLOW_COLOR, emC.red(), emC.green(), emC.blue() );

		// Specular params
		float s = ((opts & Scene::DoSpecular) && (opts & Scene::DoLighting)) ? mesh->bslsp->getSpecularStrength() : 0.0;
		prog->uni1f( SPEC_SCALE, s );

		// Assure specular power does not break the shaders
		auto gloss = mesh->bslsp->getSpecularGloss();
		prog->uni1f( SPEC_GLOSS, gloss );
		
		auto spec = mesh->bslsp->getSpecularColor();
		prog->uni3f( SPEC_COLOR, spec.red(), spec.green(), spec.blue() );

		prog->uni1i( HAS_MAP_SPEC, mesh->bslsp->hasSpecularMap );

		if ( mesh->bslsp->hasSpecularMap && (mesh->nifVersion == 130 || !mesh->bslsp->hasBacklight) )
			prog->uniSampler( bsprop, SAMP_SPECULAR, 7, texunit, white, clamp );
		else
			prog->uniSampler( bsprop, SAMP_SPECULAR, 7, texunit, black, clamp );

		if ( mesh->nifVersion == 130 ) {
			prog->uni1i( DOUBLE_SIDE, mesh->bslsp->getIsDoubleSided() );
			prog->uni1f( G2P_SCALE, mesh->bslsp->paletteScale );
			prog->uni1f( SS_ROLLOFF, mesh->bslsp->getLightingEffect1() );
			prog->uni1f( POW_FRESNEL, mesh->bslsp->fresnelPower );
			prog->uni1f( POW_RIM, mesh->bslsp->rimPower );
			prog->uni1f( POW_BACK, mesh->bslsp->backlightPower );
		}

		// Multi-Layer

		prog->uniSampler( bsprop, SAMP_INNER, 6, texunit, default_n, clamp );
		if ( mesh->bslsp->hasMultiLayerParallax ) {

			auto inS = mesh->bslsp->getInnerTextureScale();
			prog->uni2f( INNER_SCALE, inS.x, inS.y );

			prog->uni1f( INNER_THICK, mesh->bslsp->getInnerThickness() );

			prog->uni1f( OUTER_REFR, mesh->bslsp->getOuterRefractionStrength() );
			prog->uni1f( OUTER_REFL, mesh->bslsp->getOuterReflectionStrength() );
		}

		// Environment Mapping

		prog->uni1i( HAS_MAP_CUBE, mesh->bslsp->hasEnvironmentMap );
		prog->uni1i( HAS_MASK_ENV, mesh->bslsp->useEnvironmentMask );
		float refl = 0.0;
		if ( mesh->bslsp->hasEnvironmentMap
			 && (opts & Scene::DoCubeMapping) && (opts & Scene::DoLighting) )
			refl = mesh->bslsp->getEnvironmentReflection();

		prog->uni1f( ENV_REFLECTION, refl );

		// Always bind cube regardless of shader settings
		GLint uniCubeMap = prog->uniformLocations[SAMP_CUBE];
		if ( uniCubeMap >= 0 ) {
			QString fname = bsprop->fileName( 4 );
			if ( fname.isEmpty() )
				fname = cube;

			if ( !activateTextureUnit( texunit ) || !bsprop->bindCube( 4, fname ) )
				if ( !activateTextureUnit( texunit ) || !bsprop->bindCube( 4, cube ) )
					return false;

			fn->glUniform1i( uniCubeMap, texunit++ );
		}
		// Always bind mask regardless of shader settings
		prog->uniSampler( bsprop, SAMP_ENV_MASK, 5, texunit, white, clamp );

		// Parallax
		prog->uni1i( HAS_MAP_HEIGHT, mesh->bslsp->hasHeightMap );
		prog->uniSampler( bsprop, SAMP_HEIGHT, 3, texunit, gray, clamp );
	}


	// BSEffectShaderProperty
	if ( mesh->bsesp ) {

		prog->uni4m( MAT_WORLD, mesh->worldTrans().toMatrix4() );

		clamp = mesh->bsesp->getClampMode();

		prog->uniSampler( bsprop, SAMP_BASE, 0, texunit, white, clamp );

		prog->uni1i( DOUBLE_SIDE, mesh->bsesp->getIsDoubleSided() );

		auto uvS = mesh->bsesp->getUvScale();
		prog->uni2f( UV_SCALE, uvS.x, uvS.y );

		auto uvO = mesh->bsesp->getUvOffset();
		prog->uni2f( UV_OFFSET, uvO.x, uvO.y );

		prog->uni1i( HAS_MAP_BASE, mesh->bsesp->hasSourceTexture );
		prog->uni1i( HAS_MAP_G2P, mesh->bsesp->hasGreyscaleMap );

		prog->uni1i( G2P_ALPHA, mesh->bsesp->greyscaleAlpha );
		prog->uni1i( G2P_COLOR, mesh->bsesp->greyscaleColor );


		prog->uni1i( USE_FALLOFF, mesh->bsesp->useFalloff );
		prog->uni1i( HAS_RGBFALL, mesh->bsesp->hasRGBFalloff );
		prog->uni1i( HAS_WEAP_BLOOD, mesh->bsesp->hasWeaponBlood );

		// Glow params

		auto emC = mesh->bsesp->getEmissiveColor();
		prog->uni4f( GLOW_COLOR, emC.red(), emC.green(), emC.blue(), emC.alpha() );
		prog->uni1f( GLOW_MULT, mesh->bsesp->getEmissiveMult() );

		// Falloff params

		prog->uni4f( FALL_PARAMS,
			mesh->bsesp->falloff.startAngle, mesh->bsesp->falloff.stopAngle,
			mesh->bsesp->falloff.startOpacity, mesh->bsesp->falloff.stopOpacity
		);

		prog->uni1f( FALL_DEPTH, mesh->bsesp->falloff.softDepth );

		// BSEffectShader textures
		prog->uniSampler( bsprop, SAMP_GRAYSCALE, 1, texunit, "", TexClampMode::MIRRORED_S_MIRRORED_T );

		if ( mesh->nifVersion == 130 ) {

			prog->uni1f( LIGHT_INF, mesh->bsesp->getLightingInfluence() );

			prog->uni1i( HAS_MAP_NORMAL, mesh->bsesp->hasNormalMap && (opts & Scene::DoLighting) );

			prog->uniSampler( bsprop, SAMP_NORMAL, 3, texunit, default_n, clamp );

			prog->uni1i( HAS_MAP_CUBE, mesh->bsesp->hasEnvMap );
			prog->uni1i( HAS_MASK_ENV, mesh->bsesp->hasEnvMask );
			float refl = 0.0;
			if ( mesh->bsesp->hasEnvMap && (opts & Scene::DoCubeMapping) && (opts & Scene::DoLighting) )
				refl = mesh->bsesp->getEnvironmentReflection();

			prog->uni1f( ENV_REFLECTION, refl );

			GLint uniCubeMap = prog->uniformLocations[SAMP_CUBE];
			if ( uniCubeMap >= 0 ) {
				QString fname = bsprop->fileName( 2 );
				if ( fname.isEmpty() )
					fname = cube;

				if ( !activateTextureUnit( texunit ) || !bsprop->bindCube( 2, fname ) )
					if ( !activateTextureUnit( texunit ) || !bsprop->bindCube( 2, cube ) )
						return false;


				fn->glUniform1i( uniCubeMap, texunit++ );
			}
			prog->uniSampler( bsprop, SAMP_SPECULAR, 4, texunit, white, clamp );
		}
	}

	// Defaults for uniforms in older meshes
	if ( !mesh->bsesp && !mesh->bslsp ) {
		prog->uni2f( UV_SCALE, 1.0, 1.0 );
		prog->uni2f( UV_OFFSET, 0.0, 0.0 );
	}

	QMapIterator<int, Program::CoordType> itx( prog->texcoords );

	while ( itx.hasNext() ) {
		itx.next();

		if ( !activateTextureUnit( itx.key() ) )
			return false;

		auto it = itx.value();
		if ( it == Program::CT_TANGENT ) {
			if ( mesh->transTangents.count() ) {
				glEnableClientState( GL_TEXTURE_COORD_ARRAY );
				glTexCoordPointer( 3, GL_FLOAT, 0, mesh->transTangents.constData() );
			} else if ( mesh->tangents.count() ) {
				glEnableClientState( GL_TEXTURE_COORD_ARRAY );
				glTexCoordPointer( 3, GL_FLOAT, 0, mesh->tangents.constData() );
			} else {
				return false;
			}

		} else if ( it == Program::CT_BITANGENT ) {
			if ( mesh->transBitangents.count() ) {
				glEnableClientState( GL_TEXTURE_COORD_ARRAY );
				glTexCoordPointer( 3, GL_FLOAT, 0, mesh->transBitangents.constData() );
			} else if ( mesh->bitangents.count() ) {
				glEnableClientState( GL_TEXTURE_COORD_ARRAY );
				glTexCoordPointer( 3, GL_FLOAT, 0, mesh->bitangents.constData() );
			} else {
				return false;
			}
		} else if ( texprop ) {
			int txid = it;
			if ( txid < 0 )
				return false;

			int set = texprop->coordSet( txid );

			if ( set < 0 || !(set < mesh->coords.count()) || !mesh->coords[set].count() )
				return false;

			glEnableClientState( GL_TEXTURE_COORD_ARRAY );
			glTexCoordPointer( 2, GL_FLOAT, 0, mesh->coords[set].constData() );
		} else if ( bsprop ) {
			int txid = it;
			if ( txid < 0 )
				return false;

			int set = 0;

			if ( set < 0 || !(set < mesh->coords.count()) || !mesh->coords[set].count() )
				return false;

			glEnableClientState( GL_TEXTURE_COORD_ARRAY );
			glTexCoordPointer( 2, GL_FLOAT, 0, mesh->coords[set].constData() );
		}
	}

	// setup lighting

	//glEnable( GL_LIGHTING );

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

	glDisable( GL_COLOR_MATERIAL );

	if ( mesh->nifVersion < 83 ) {
		// setup vertex colors

		//glProperty( props.get< VertexColorProperty >(), glIsEnabled( GL_COLOR_ARRAY ) );
		
		// setup material

		glProperty( props.get<MaterialProperty>(), props.get<SpecularProperty>() );

		// setup z buffer

		glProperty( props.get<ZBufferProperty>() );

		// setup stencil

		glProperty( props.get<StencilProperty>() );

		// wireframe ?

		glProperty( props.get<WireframeProperty>() );
	} else {
		glEnable( GL_DEPTH_TEST );
		glDepthMask( GL_TRUE );
		glDepthFunc( GL_LEQUAL );
		glEnable( GL_CULL_FACE );
		glCullFace( GL_BACK );
		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	}

	if ( !mesh->depthTest ) {
		glDisable( GL_DEPTH_TEST );
	}

	if ( !mesh->depthWrite || mesh->translucent ) {
		glDepthMask( GL_FALSE );
	}

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

	if ( !(mesh->scene->options & Scene::DoTexturing) )
		return;

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

