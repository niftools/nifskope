uniform sampler2D BaseMap;
uniform sampler2D NormalMap;

varying vec3 LightDir;
varying vec3 HalfVector;
varying vec3 ViewDir;

varying vec4 ColorEA;
varying vec4 ColorD;

void main( void )
{
	float offset = 0.015 - texture2D( BaseMap, gl_TexCoord[0].st ).a * 0.03;
	vec2 texco = gl_TexCoord[0].st + normalize( ViewDir ).xy * offset;
	
	vec4 color = ColorEA;

	vec4 normal = texture2D( NormalMap, texco );
	normal.rgb = normal.rgb * 2.0 - 1.0;
	
	float NdotL = max( dot( normal.rgb, normalize( LightDir ) ), 0.0 );
	
	if ( NdotL > 0.0 )
	{
		color += ColorD * NdotL;
		float NdotHV = max( dot( normal.rgb, normalize( HalfVector ) ), 0.0 );
		color += normal.a * gl_FrontMaterial.specular * gl_LightSource[0].specular * pow( NdotHV, gl_FrontMaterial.shininess );
	}
	
	color = min( color, 1.0 );
	color *= texture2D( BaseMap, texco );
	
	gl_FragColor = color;
}
