#version 130

uniform sampler2D SourceTexture;
uniform sampler2D GreyscaleMap;
uniform samplerCube CubeMap;
uniform sampler2D NormalMap;
uniform sampler2D SpecularMap;

uniform bool doubleSided;

uniform bool hasSourceTexture;
uniform bool hasGreyscaleMap;
uniform bool hasCubeMap;
uniform bool hasNormalMap;
uniform bool hasEnvMask;

uniform bool greyscaleAlpha;
uniform bool greyscaleColor;

uniform bool useFalloff;
uniform bool vertexColors;
uniform bool vertexAlpha;

uniform bool hasWeaponBlood;

uniform vec4 glowColor;
uniform float glowMult;

uniform vec2 uvScale;
uniform vec2 uvOffset;

uniform vec4 falloffParams;
uniform float falloffDepth;

uniform float lightingInfluence;
uniform float envReflection;

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

vec4 colorLookup( float x, float y ) {
	
	return texture2D( GreyscaleMap, vec2( clamp(x, 0.0, 1.0), clamp(y, 0.0, 1.0)) );
}

void main( void )
{
	vec2 offset = gl_TexCoord[0].st * uvScale + uvOffset;
	
	vec4 baseMap = texture2D( SourceTexture, offset );
	vec4 normalMap = texture2D( NormalMap, offset );
	vec4 specMap = texture2D( SpecularMap, offset );
	
	vec3 normal = N;
	if ( hasNormalMap ) {
		normal = normalize(normalMap.rgb * 2.0 - 1.0);
	}
	if ( !gl_FrontFacing && doubleSided ) {
		normal *= -1.0;	
	}
	
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
	vec3 reflectedVS = t * reflected.x + b * reflected.y + N * reflected.z;
	vec3 reflectedWS = vec3( worldMatrix * (gl_ModelViewMatrixInverse * vec4( reflectedVS, 0.0 )) );
	
	// Falloff
	float falloff = 1.0;
	if ( useFalloff ) {
		float startO = min(falloffParams.z, 1.0);
		float stopO = max(falloffParams.w, 0.0);
		
		// TODO: When X and Y are both 0.0 or both 1.0 the effect is reversed.
		falloff = smoothstep( falloffParams.y, falloffParams.x, abs(V.b));
    
		falloff = mix( max(falloffParams.w, 0.0), min(falloffParams.z, 1.0), falloff );
	}
	
	float alphaMult = glowColor.a * glowColor.a;
	
	vec4 color;
	color.rgb = baseMap.rgb;
	color.a = baseMap.a;
	
	// FO4 Unused?
	//if ( hasWeaponBlood ) {
	//	color.rgb = vec3( 1.0, 0.0, 0.0 ) * baseMap.r;
	//	color.a = baseMap.a * baseMap.g;
	//}
	
	color.rgb *= C.rgb * glowColor.rgb;
	color.a *= C.a * falloff * alphaMult;

	if ( greyscaleColor ) {
		// Only Red emissive channel is used
		float emRGB = glowColor.r;

		vec4 luG = colorLookup( baseMap.g, C.g * falloff * emRGB );

		color.rgb = luG.rgb;
	}
	
	if ( greyscaleAlpha ) {
		vec4 luA = colorLookup( baseMap.a, C.a * falloff * alphaMult );
		
		color.a = luA.a;
	}
	
	vec3 diffuse = A.rgb + (D.rgb * NdotL);
	color.rgb = mix( color.rgb, color.rgb * diffuse, lightingInfluence );
	
	color.rgb *= glowMult;
	
	// Specular
	float g = 1.0;
	float s = 1.0;
	if ( hasEnvMask ) {
		g = specMap.r;
		s = specMap.g;
	}
	
	// Environment
	vec4 cube = textureCube( CubeMap, reflectedWS );
	if ( hasCubeMap ) {
		cube.rgb *= envReflection * sqrt(g) * s;
		
		color.rgb += cube.rgb * falloff;
	}
	
	gl_FragColor.rgb = color.rgb;
	gl_FragColor.a = color.a;
}
