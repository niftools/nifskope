uniform sampler2D BaseMap;
uniform sampler2D NormalMap;

varying vec3 ViewDir;
varying vec4 Color;

vec4 emissive( vec4 e_color );
vec4 ambient( vec4 a_color );
vec4 diffuse( vec4 d_color, vec3 normal );
vec4 specular( vec4 s_color, vec3 normal );

void main( void )
{
	float offset = 0.015 - texture2D( BaseMap, gl_TexCoord[0].st ).a * 0.03;
	vec2 texco = gl_TexCoord[0].st + ViewDir.xy * offset;
	
	vec4 normal = texture2D( NormalMap, texco );
	normal.xyz = normal.xyz * 2.0 - 1.0;
	
	vec4 col =
		emissive( Color ) +
		ambient( gl_FrontMaterial.ambient ) +
		diffuse( gl_FrontMaterial.diffuse, normal.xyz ) +
		specular( normal.a * gl_FrontMaterial.shininess * gl_FrontMaterial.specular, normal.xyz );
	
	col = clamp( col, 0.0, 1.0 );
	
	col *= texture2D( BaseMap, texco );
	
	gl_FragColor = col;
}
