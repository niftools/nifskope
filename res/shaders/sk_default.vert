#version 130

out vec3 LightDir;
out vec3 ViewDir;

out mat3 tbnMatrix;
out vec3 v;

out vec4 A;
out vec4 C;
out vec4 D;

uniform bool isGPUSkinned;
uniform mat4 boneTransforms[100];

void main( void )
{
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	gl_TexCoord[0] = gl_MultiTexCoord0;
	
	vec3 n;
	vec3 t;
	vec3 b;
	if ( !isGPUSkinned ) {
		n = gl_NormalMatrix * gl_Normal;
		t = gl_NormalMatrix * gl_MultiTexCoord1.xyz;
		b = gl_NormalMatrix * gl_MultiTexCoord2.xyz;
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
		n = gl_NormalMatrix * normal;
		t = gl_NormalMatrix * tan;
		b = gl_NormalMatrix * bit;
		v = vec3(gl_ModelViewMatrix * V);
	}

	tbnMatrix = mat3(b.x, b.y, b.z,
                     t.x, t.y, t.z,
                     n.x, n.y, n.z);
	
	ViewDir = -v.xyz;
	LightDir = gl_LightSource[0].position.xyz;

	A = gl_LightSource[0].ambient;
	C = gl_Color;
	D = gl_LightSource[0].diffuse;
}
