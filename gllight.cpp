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

#include "gllight.h"
#include "glscene.h"

/*
 *  Light
 */
 
void Light::update( const NifModel * nif, const QModelIndex & index )
{
	Node::update( nif, index );
	
	if ( iBlock.isValid() && iBlock == index )
	{
		dimmer = nif->get<float>( iBlock, "Dimmer" );
		ambient = nif->get<Color3>( iBlock, "Ambient Color" );
		diffuse = nif->get<Color3>( iBlock, "Diffuse Color" );
		specular = nif->get<Color3>( iBlock, "Specular Color" );
		
		directional = nif->itemName( iBlock ) == "NiDirectionalLight";
		spot = nif->itemName( iBlock ) == "NiSpotLight";
		
		if ( spot )
		{
			constant = nif->get<float>( iBlock, "Constant Attenuation" );
			linear = nif->get<float>( iBlock, "Linear Attenuation" );
			quadratic = nif->get<float>( iBlock, "Quadratic Attenuation" );
			exponent = nif->get<float>( iBlock, "Exponent" );
			cutoff = nif->get<float>( iBlock, "Cutoff Angle" );
		}
		else
		{
			constant = 1.0;
			linear = quadratic = exponent = 0.0;
			cutoff = 180.0;
		}
	}
}

static const GLenum light_enum[8] = { GL_LIGHT0, GL_LIGHT1, GL_LIGHT2, GL_LIGHT3, GL_LIGHT4, GL_LIGHT5, GL_LIGHT6, GL_LIGHT7 };

void Light::on( int n )
{
	GLenum e = light_enum[ n ];
	
	Vector3 pos = viewTrans().translation;
	
	if ( directional )
	{
		Vector3 dir = viewTrans().rotation * Vector3( -1, 0, 0 );
		GLfloat pos4[4] = { dir[0], dir[1], dir[2], 0.0 };
		glLightfv( e, GL_POSITION, pos4 );
	}
	else
	{
		GLfloat pos4[4] = { pos[0], pos[1], pos[2], 1.0 };
		glLightfv( e, GL_POSITION, pos4 );
	}
	
	glLightfv( e, GL_AMBIENT, ( ambient * dimmer ).data() );
	glLightfv( e, GL_DIFFUSE, ( diffuse * dimmer ).data() );
	glLightfv( e, GL_SPECULAR, ( specular * dimmer ).data() );
	glLightfv( e, GL_SPOT_DIRECTION, ( viewTrans().rotation * Vector3( 1, 0, 0 ) ).data() );
	glLightf( e, GL_CONSTANT_ATTENUATION, constant );
	glLightf( e, GL_LINEAR_ATTENUATION, linear );
	glLightf( e, GL_QUADRATIC_ATTENUATION, quadratic );
	glLightf( e, GL_SPOT_EXPONENT, exponent );
	glLightf( e, GL_SPOT_CUTOFF, cutoff );
	
	glEnable( e );
}

void Light::off( int n )
{
	GLenum e = light_enum[ n ];
	glDisable( e );
}

void Light::draw( NodeList * draw2nd = 0 )
{
	Node::draw( draw2nd );
	
	if ( isHidden() )
		return;
	
	glLoadName( nodeId );
	
	glEnable( GL_DEPTH_TEST );
	glDepthMask( GL_TRUE );
	glDepthFunc( GL_ALWAYS );
	glDisable( GL_TEXTURE_2D );
	glDisable( GL_NORMALIZE );
	glDisable( GL_LIGHTING );
	glDisable( GL_COLOR_MATERIAL );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE );
	
	if ( scene->highlight && scene->currentNode == nodeId )
		glColor( Color4( 0.8, 0.8, 0.3, 0.8 ) );
	else
		glColor( Color4( 0.6, 0.6, 0.1, 0.5 ) );
	
	glPointSize( 8.5 );
	glLineWidth( 2.5 );
	
	Vector3 a = viewTrans().translation;
	Vector3 b = a + viewTrans().rotation * Vector3( 10, 0, 0 );
	
	glBegin( GL_LINES );
	glVertex( a );
	glVertex( b );
	glEnd();
}
