uniform sampler2D BaseMap;
uniform sampler2D NormalMap;

varying vec4 Color;

vec4 emissive( vec4 e_color );
vec4 ambient( vec4 a_color );
vec4 diffuse( vec4 d_color, vec3 normal );
vec4 specular( vec4 s_color, vec3 normal );

void main( void )
{
	vec4 normal = texture2D( NormalMap, gl_TexCoord[0].st );
	normal.xyz = normal.xyz * 2.0 - 1.0;
	
	vec4 col =
		emissive( gl_FrontMaterial.emission ) +
		ambient( Color ) +
		diffuse( Color, normal.xyz ) +
		specular( normal.a * gl_FrontMaterial.shininess * gl_FrontMaterial.specular, normal.xyz );
	
	col = clamp( col, 0.0, 1.0 );
	
	col *= texture2D( BaseMap, gl_TexCoord[0].st );
	
	gl_FragColor = col;
}
