
varying vec3 LightDir;
varying vec3 ViewDir;

varying vec4 ColorEA;
varying vec4 ColorD;

varying vec3 v;

varying vec4 A;
varying vec4 D;

void main( void )
{
	gl_Position = ftransform();
	gl_TexCoord[0] = gl_MultiTexCoord0;
	
	v = vec3(gl_ModelViewMatrix * gl_Vertex);

	ViewDir = -v.xyz;
	LightDir = gl_LightSource[0].position.xyz;
	
	A = gl_LightSource[0].ambient;
	D = gl_LightSource[0].diffuse;

	ColorEA = gl_FrontMaterial.emission + gl_Color * A;
	ColorD = gl_Color * D;
}
