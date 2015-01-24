
varying vec3 LightDir;
varying vec3 ViewDir;

varying vec4 ColorEA;
varying vec4 ColorD;

varying vec3 N;
varying vec3 t;
varying vec3 b;
varying vec3 v;

varying vec4 A;
varying vec4 D;

void main( void )
{
	gl_Position = ftransform();
	gl_TexCoord[0] = gl_MultiTexCoord0;
	
	N = normalize(gl_NormalMatrix * gl_Normal);
	t = normalize(gl_NormalMatrix * gl_MultiTexCoord1.xyz);
	b = normalize(gl_NormalMatrix * gl_MultiTexCoord2.xyz);
	
	// NOTE: b<->t 
	mat3 tbnMatrix = mat3(b.x, t.x, N.x,
                          b.y, t.y, N.y,
                          b.z, t.z, N.z);
						  
	v = vec3(gl_ModelViewMatrix * gl_Vertex);
	
	ViewDir = tbnMatrix * -v.xyz;
	LightDir = tbnMatrix * gl_LightSource[0].position.xyz;

	A = gl_LightSource[0].ambient;
	D = gl_LightSource[0].diffuse;

	ColorEA = gl_FrontMaterial.emission + gl_Color * A;
	ColorD = gl_Color * D;
}
