// lighting terms for one directional light

varying vec3 LightDir;
varying vec3 ViewDir;

vec4 emissive( vec4 e_color )
{
	return e_color;
}

vec4 ambient( vec4 a_color )
{
	return a_color * gl_LightSource[0].ambient;
}

vec4 diffuse( vec4 d_color, vec3 normal )
{
	float NdotL = dot( normal, LightDir );
	NdotL = max( NdotL, 0.0 );
	//NdotL *= sign( NdotL );
	return NdotL * d_color * gl_LightSource[0].diffuse;
}

vec4 specular( vec4 s_color, vec3 normal )
{
	float SdotN = dot( normalize( ViewDir + LightDir ), normal );
	SdotN = max( SdotN, 0.0 );
	//SdotN *= sign( SdotN );
	return SdotN * s_color * gl_LightSource[0].specular;
}
