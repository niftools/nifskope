
varying vec3 LightDir;
varying vec3 ViewDir;

varying vec4 ColorEA;
varying vec4 ColorD;

varying vec3 v;

void main( void )
{
	gl_Position = ftransform();
	gl_TexCoord[0] = gl_MultiTexCoord0;
	
	v = vec3(gl_ModelViewMatrix * gl_Vertex);

	ViewDir = gl_NormalMatrix * -v.xyz;
	LightDir = gl_NormalMatrix * gl_LightSource[0].position.xyz;
	
	ColorEA = gl_FrontMaterial.emission + gl_FrontMaterial.ambient * gl_LightSource[0].ambient;
	ColorD = gl_FrontMaterial.diffuse * gl_LightSource[0].diffuse;
}
