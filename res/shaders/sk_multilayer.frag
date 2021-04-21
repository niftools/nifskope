#version 120

uniform sampler2D BaseMap;
uniform sampler2D NormalMap;
uniform sampler2D LightMask;
uniform sampler2D BacklightMap;
uniform sampler2D InnerMap;
uniform sampler2D EnvironmentMap;
uniform samplerCube CubeMap;

uniform vec3 specColor;
uniform float specStrength;
uniform float specGlossiness;

uniform vec3 glowColor;
uniform float glowMult;

uniform float alpha;

uniform vec2 uvScale;
uniform vec2 uvOffset;

uniform bool hasEmit;
uniform bool hasSoftlight;
uniform bool hasBacklight;
uniform bool hasRimlight;
uniform bool hasCubeMap;
uniform bool hasEnvMask;

uniform float lightingEffect1;
uniform float lightingEffect2;

uniform vec2 innerScale;
uniform float innerThickness;
uniform float outerRefraction;
uniform float outerReflection;

uniform mat4 worldMatrix;

varying vec3 LightDir;
varying vec3 ViewDir;

varying vec4 A;
varying vec4 C;
varying vec4 D;

varying vec3 N;
varying vec3 t;
varying vec3 b;


vec3 tonemap(vec3 x)
{
	float _A = 0.15;
	float _B = 0.50;
	float _C = 0.10;
	float _D = 0.20;
	float _E = 0.02;
	float _F = 0.30;

	return ((x*(_A*x+_C*_B)+_D*_E)/(x*(_A*x+_B)+_D*_F))-_E/_F;
}

vec3 toGrayscale(vec3 color)
{
	return vec3(dot(vec3(0.3, 0.59, 0.11), color));
}

// Compute inner layer’s texture coordinate and transmission depth
// vTexCoord: Outer layer’s texture coordinate
// vInnerScale: Tiling of inner texture
// vViewTS: View vector in tangent space
// vNormalTS: Normal in tangent space (sampled normal map)
// fLayerThickness: Distance from outer layer to inner layer
vec3 ParallaxOffsetAndDepth( vec2 vTexCoord, vec2 vInnerScale, vec3 vViewTS, vec3 vNormalTS, float fLayerThickness )
{
	// Tangent space reflection vector
	vec3 vReflectionTS = reflect( -vViewTS, vNormalTS );
	// Tangent space transmission vector (reflect about surface plane)
	vec3 vTransTS = vec3( vReflectionTS.xy, -vReflectionTS.z );
	
	// Distance along transmission vector to intersect inner layer
	float fTransDist = fLayerThickness / abs(vTransTS.z);
	
	// Texel size
	// 	Bethesda's version does indeed seem to assume 1024, which is why they
	//	introduced the additional parameter.
	vec2 vTexelSize = vec2( 1.0/(1024.0 * vInnerScale.x), 1.0/(1024.0 * vInnerScale.y) );
	
	// Inner layer’s texture coordinate due to parallax
	vec2 vOffset = vTexelSize * fTransDist * vTransTS.xy;
	vec2 vOffsetTexCoord = vTexCoord + vOffset;
	
	// Return offset texture coordinate in xy and transmission dist in z
	return vec3( vOffsetTexCoord, fTransDist );
}

void main( void )
{
	vec2 offset = gl_TexCoord[0].st * uvScale + uvOffset;

	vec4 baseMap = texture2D( BaseMap, offset );
	vec4 normalMap = texture2D( NormalMap, offset );
	
	vec3 normal = normalize(normalMap.rgb * 2.0 - 1.0);

	// Sample the non-parallax offset alpha channel of the inner map
	//	Used to modulate the innerThickness
	float innerMapAlpha = texture2D( InnerMap, offset ).a;
	
	
	vec3 L = normalize(LightDir);
	vec3 E = normalize(ViewDir);
	vec3 R = reflect(-L, normal);
	vec3 H = normalize( L + E );
	
	float NdotL = max( dot(normal, L), 0.0 );
	float NdotH = max( dot(normal, H), 0.0 );
	float EdotN = max( dot(normal, E), 0.0 );
	float NdotNegL = max( dot(normal, -L), 0.0 );
	
	
	// Mix between the face normal and the normal map based on the refraction scale
	vec3 mixedNormal = mix( vec3(0.0, 0.0, 1.0), normal, clamp( outerRefraction, 0.0, 1.0 ) );
	vec3 parallax = ParallaxOffsetAndDepth( offset, innerScale, E, mixedNormal, innerThickness * innerMapAlpha );
	
	// Sample the inner map at the offset coords
	vec4 innerMap = texture2D( InnerMap, parallax.xy * innerScale );

	vec3 reflected = reflect( -E, normal );
	vec3 reflectedVS = b * reflected.x + t * reflected.y + N * reflected.z;
	vec3 reflectedWS = vec3( worldMatrix * (gl_ModelViewMatrixInverse * vec4( reflectedVS, 0.0 )) );


	vec4 color;
	vec3 albedo;
	vec3 diffuse = A.rgb + (D.rgb * NdotL);
	vec3 inner = innerMap.rgb * C.rgb;
	vec3 outer = baseMap.rgb * C.rgb;


	// Mix inner/outer layer based on fresnel
	float outerMix = max( 1.0 - EdotN, baseMap.a );
	albedo = mix( inner, outer, outerMix );


	// Environment
	if ( hasCubeMap ) {
		vec4 cube = textureCube( CubeMap, reflectedWS );
		cube.rgb *= outerReflection;
		
		if ( hasEnvMask ) {
			vec4 env = texture2D( EnvironmentMap, offset );
			cube.rgb *= env.r;
		} else {
			cube.rgb *= normalMap.a;
		}

		albedo += cube.rgb;
	}

	// Specular
	vec3 spec = clamp( specColor * specStrength * normalMap.a * pow(NdotH, specGlossiness), 0.0, 1.0 );
	spec *= D.rgb;

	// Emissive
	//	Mixed with outer map
	vec3 emissive = vec3(0.0);
	if ( hasEmit ) {
		emissive += glowColor * glowMult;
	}

	// Backlight
	// 	Mixed with inner and outer map
	vec3 backlight = vec3(0.0);
	if ( hasBacklight ) {
		backlight = texture2D( BacklightMap, offset ).rgb;
		backlight *= NdotNegL;
		
		emissive += backlight * D.rgb;
	}

	// TODO: Test rim and soft light mixing with inner/outer layer

	vec4 mask = vec4(0.0);
	if ( hasRimlight || hasSoftlight ) {
		mask = texture2D( LightMask, offset );
	}

	vec3 rim = vec3(0.0);
	if ( hasRimlight ) {
		rim = mask.rgb * pow(vec3((1.0 - EdotN)), vec3(lightingEffect2));
		rim *= smoothstep( -0.2, 1.0, dot(-L, E) );
		
		emissive += rim * D.rgb;
	}

	vec3 soft = vec3(0.0);
	if ( hasSoftlight ) {
		float wrap = (dot(normal, L) + lightingEffect1) / (1.0 + lightingEffect1);

		soft = max( wrap, 0.0 ) * mask.rgb * smoothstep( 1.0, 0.0, NdotL );
		soft *= sqrt( clamp( lightingEffect1, 0.0, 1.0 ) );
		
		emissive += soft * D.rgb;
	}

	color.rgb = albedo * (diffuse + emissive) + spec;
	color.rgb = tonemap( color.rgb ) / tonemap( vec3(1.0) );
	color.a = C.a * baseMap.a;
	
	gl_FragColor = color;
	gl_FragColor.a *= alpha;
}
