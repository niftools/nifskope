#version 120

uniform sampler2D BaseMap;
uniform sampler2D GreyscaleMap;

uniform bool doubleSided;

uniform bool hasSourceTexture;
uniform bool hasGreyscaleMap;
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

varying vec3 LightDir;
varying vec3 ViewDir;

varying vec4 C;

varying vec3 N;
varying vec3 v;

vec4 colorLookup( float x, float y ) {
	
	return texture2D( GreyscaleMap, vec2( clamp(x, 0.0, 1.0), clamp(y, 0.0, 1.0)) );
}

void main( void )
{
	vec4 baseMap = texture2D( BaseMap, gl_TexCoord[0].st * uvScale + uvOffset );
	
	vec4 color;

	vec3 normal = N;
	
	// Reconstructed normal
	//normal = normalize(cross(dFdy(v.xyz), dFdx(v.xyz)));
	
	//if ( !doubleSided && !gl_FrontFacing ) { return; }
	
	vec3 E = normalize(ViewDir);

	float tmp2 = falloffDepth; // Unused right now
	
	// Falloff
	float falloff = 1.0;
	if ( useFalloff ) {
		float startO = min(falloffParams.z, 1.0);
		float stopO = max(falloffParams.w, 0.0);
		
		// TODO: When X and Y are both 0.0 or both 1.0 the effect is reversed.
		falloff = smoothstep( falloffParams.y, falloffParams.x, abs(E.b));

		falloff = mix( max(falloffParams.w, 0.0), min(falloffParams.z, 1.0), falloff );
	}
	
	float alphaMult = glowColor.a * glowColor.a;
	
	color.rgb = baseMap.rgb;
	color.a = baseMap.a;
	
	if ( hasWeaponBlood ) {
		color.rgb = vec3( 1.0, 0.0, 0.0 ) * baseMap.r;
		color.a = baseMap.a * baseMap.g;
	}
	
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

	gl_FragColor.rgb = color.rgb * glowMult;
	gl_FragColor.a = color.a;
}
