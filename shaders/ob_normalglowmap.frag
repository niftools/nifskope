uniform sampler2D BaseMap;
uniform sampler2D NormalMap;
uniform sampler2D GlowMap;

varying vec3 LightDir;
varying vec3 HalfVector;
//varying vec3 ViewDir;

varying vec4 ColorEA;
varying vec4 ColorD;

void main( void )
{
	vec4 color = ColorEA;
	
	color += texture2D( GlowMap, gl_TexCoord[0].st );

	vec4 normal = texture2D( NormalMap, gl_TexCoord[0].st );
	normal.rgb = normal.rgb * 2.0 - 1.0;
	
	float NdotL = max( dot( normal.rgb, normalize( LightDir ) ), 0.0 );
	
	if ( NdotL > 0.0 )
	{
		color += ColorD * NdotL;
		float NdotHV = max( dot( normal.rgb, normalize( HalfVector ) ), 0.0 );
		color += normal.a * gl_FrontMaterial.specular * gl_LightSource[0].specular * pow( NdotHV, gl_FrontMaterial.shininess );
	}
	
	color = min( color, 1.0 );
	color *= texture2D( BaseMap, gl_TexCoord[0].st );
	
	gl_FragColor = color;
}
