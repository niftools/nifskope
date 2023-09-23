#version 130

uniform sampler2D BaseMap;
uniform sampler2D GreyscaleMap;
uniform samplerCube CubeMap;
uniform sampler2D NormalMap;
uniform sampler2D ReflMap;
uniform sampler2D LightingMap;

uniform bool isWireframe;
uniform vec4 solidColor;

uniform bool doubleSided;

uniform bool hasSourceTexture;
uniform bool hasGreyscaleMap;
uniform bool hasCubeMap;
uniform bool hasNormalMap;
uniform bool hasEnvMask;

uniform bool greyscaleAlpha;
uniform bool greyscaleColor;

uniform bool useFalloff;
uniform bool hasRGBFalloff;

uniform bool hasWeaponBlood;

uniform vec4 glowColor;
uniform float glowMult;

uniform vec2 uvScale;
uniform vec2 uvOffset;

uniform vec4 falloffParams;
uniform float falloffDepth;

uniform float lightingInfluence;
uniform float envReflection;

uniform float fLumEmittance;

uniform mat4 worldMatrix;

in vec3 LightDir;
in vec3 ViewDir;

in vec4 A;
in vec4 C;
in vec4 D;

in vec3 N;
in vec3 t;
in vec3 b;
in vec3 v;

in mat4 reflMatrix;

vec4 colorLookup( float x, float y ) {
	
	return texture2D( GreyscaleMap, vec2( clamp(x, 0.0, 1.0), clamp(y, 0.0, 1.0)) );
}

vec3 fresnelSchlickRoughness(float NdotV, vec3 F0, float roughness)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - NdotV, 5.0);
}

void main( void )
{
	if ( isWireframe ) {
		gl_FragColor = solidColor;
		return;
	}
	vec2 offset = gl_TexCoord[0].st * uvScale + uvOffset;
	
	vec4 baseMap = texture2D( BaseMap, offset );
	vec4 normalMap = texture2D( NormalMap, offset );
	vec4 reflMap = texture2D(ReflMap, offset);
	vec4 lightingMap = texture2D(LightingMap, offset);
	
	vec3 normal = normalMap.rgb;
	// Calculate missing blue channel
	normal.b = sqrt(1.0 - dot(normal.rg, normal.rg));
	if ( !gl_FrontFacing && doubleSided ) {
		normal *= -1.0;	
	}

	vec3 f0 = (reflMap.g == 0 && reflMap.b == 0) ? vec3(reflMap.r) : reflMap.rgb;
	vec3 L = normalize(LightDir);
	vec3 V = normalize(ViewDir);
	vec3 R = reflect(-L, normal);
	vec3 H = normalize( L + V );
	
	float NdotL = max( dot(normal, L), 0.000001 );
	float NdotH = max( dot(normal, H), 0.000001 );
	float NdotV = max( dot(normal, V), 0.000001 );
	float LdotH = max( dot(L, H), 0.000001 );
	float NdotNegL = max( dot(normal, -L), 0.000001 );

	vec3 reflected = reflect( V, normal );
	vec3 reflectedVS = b * reflected.x + t * reflected.y + N * reflected.z;
	vec3 reflectedWS = vec3(reflMatrix * (gl_ModelViewMatrixInverse * vec4( reflectedVS, 0.0 )));
	
	if ( greyscaleAlpha )
		baseMap.a = 1.0;
	
	vec4 baseColor = glowColor;
	if ( !greyscaleColor )
		baseColor.rgb *= glowMult;
	
	// Falloff
	float falloff = 1.0;
	if ( useFalloff || hasRGBFalloff ) {
		falloff = smoothstep( falloffParams.x, falloffParams.y, abs(dot(normal, V)) );
		falloff = mix( max(falloffParams.z, 0.0), min(falloffParams.w, 1.0), falloff );
		
		if ( useFalloff )
			baseMap.a *= falloff;
		
		if ( hasRGBFalloff )
			baseMap.rgb *= falloff;
	}
	
	float alphaMult = baseColor.a * baseColor.a;
	
	vec4 color;
	color.rgb = baseMap.rgb * C.rgb * baseColor.rgb;
	color.a = alphaMult * C.a * baseMap.a;
	
	if ( greyscaleColor ) {
		vec4 luG = colorLookup( texture2D( BaseMap, offset ).g, baseColor.r * C.r * falloff );

		color.rgb = luG.rgb;
	}
	
	if ( greyscaleAlpha ) {
		vec4 luA = colorLookup( texture2D( BaseMap, offset ).a, color.a );
		
		color.a = luA.a;
	}
	
	vec3 diffuse = A.rgb + (D.rgb * NdotL);
	color.rgb = mix( color.rgb, color.rgb * D.rgb, lightingInfluence );
	
	// Specular
	float g = 1.0;
	float s = 1.0;
	//if ( hasEnvMask ) {
	//	g = specMap.r;
	//	s = specMap.g;
	//}
	
	// Environment
	vec4 cube = textureCube( CubeMap, reflectedWS );
	if ( hasCubeMap ) {
		cube.rgb *= envReflection * s;
		cube.rgb = mix( cube.rgb, cube.rgb * D.rgb, lightingInfluence );
		
		color.rgb += cube.rgb * falloff * fresnelSchlickRoughness(NdotV, f0, 1.0 - lightingMap.r);
	}
	
	gl_FragColor.rgb = color.rgb;
	gl_FragColor.a = color.a;
}
