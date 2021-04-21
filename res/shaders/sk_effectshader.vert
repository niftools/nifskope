#version 120

varying vec3 LightDir;
varying vec3 ViewDir;

varying vec4 C;

varying vec3 N;
varying vec3 t;
varying vec3 b;
varying vec3 v;

uniform bool isGPUSkinned;
uniform mat4 boneTransforms[100];

void main( void )
{
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	gl_TexCoord[0] = gl_MultiTexCoord0;
	
	if ( !isGPUSkinned ) {
		N = normalize(gl_NormalMatrix * gl_Normal);
		t = normalize(gl_NormalMatrix * gl_MultiTexCoord1.xyz);
		b = normalize(gl_NormalMatrix * gl_MultiTexCoord2.xyz);
		v = vec3(gl_ModelViewMatrix * gl_Vertex);
	} else {
		mat4 bt = boneTransforms[int(gl_MultiTexCoord3[0])] * gl_MultiTexCoord4[0];
		bt += boneTransforms[int(gl_MultiTexCoord3[1])] * gl_MultiTexCoord4[1];
		bt += boneTransforms[int(gl_MultiTexCoord3[2])] * gl_MultiTexCoord4[2];
		bt += boneTransforms[int(gl_MultiTexCoord3[3])] * gl_MultiTexCoord4[3];

		vec4 V = bt * gl_Vertex;
		vec3 normal = vec3(bt * vec4(gl_Normal, 0.0));
		vec3 tan = vec3(bt * vec4(gl_MultiTexCoord1.xyz, 0.0));
		vec3 bit = vec3(bt * vec4(gl_MultiTexCoord2.xyz, 0.0));

		gl_Position = gl_ModelViewProjectionMatrix * V;
		N = normalize(gl_NormalMatrix * normal);
		t = normalize(gl_NormalMatrix * tan);
		b = normalize(gl_NormalMatrix * bit);
		v = vec3(gl_ModelViewMatrix * V);
	}
	
	mat3 tbnMatrix = mat3(b.x, t.x, N.x,
                          b.y, t.y, N.y,
                          b.z, t.z, N.z);
	
	ViewDir = tbnMatrix * -v.xyz;
	LightDir = tbnMatrix * gl_LightSource[0].position.xyz;
	
	C = gl_Color;
}