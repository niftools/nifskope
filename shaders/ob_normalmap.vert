
varying vec3 LightDir;
varying vec3 ViewDir;
varying vec4 Color;

vec3 normal;
vec3 tangent;
vec3 binormal;

vec3 tspace( vec3 v )
{
	return vec3( dot( v, binormal ), dot( v, tangent ), dot( v, normal ) );
}

void main( void )
{
	gl_Position = ftransform();
	gl_TexCoord[0] = gl_MultiTexCoord0;
	Color = gl_Color;
	
	normal = gl_NormalMatrix * gl_Normal;
	tangent = gl_NormalMatrix * gl_MultiTexCoord1.xyz;
	binormal = cross( normal, tangent );
	
	// Put the Light Direction into tangent space
	LightDir = tspace( gl_LightSource[0].position.xyz ); // light 0 is directional
	
    // Put the View Direction into tangent space too
	ViewDir = tspace( normalize( ( gl_ModelViewMatrix * gl_Vertex ).xyz ) );
}
