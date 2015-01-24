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

uniform vec2 uvScale;
uniform vec2 uvOffset;

uniform float alpha;

uniform bool hasEmit;
uniform bool hasSoftlight;
uniform bool hasBacklight;
uniform bool hasRimlight;

uniform float lightingEffect1;
uniform float lightingEffect2;

uniform vec2 innerScale;
uniform float innerThickness;
uniform float outerRefraction;
uniform float outerReflection;

varying vec3 LightDir;
varying vec3 ViewDir;

varying vec4 ColorEA;
varying vec4 ColorD;

varying vec4 A;
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
	
	float NdotL = max( dot(normal, L), 0.0 );
	float EdotN = max( dot(normal, E), 0.0 );
	
	float wrap = max( dot(normal, -L), 0.0 );
	float facing = max( dot(-L, E), 0.0 );
	
	
	// Mix between the face normal and the normal map based on the refraction scale
	vec3 mixedNormal = mix( vec3(0.0, 0.0, 1.0), normal, clamp( outerRefraction, 0.0, 1.0 ) );
	vec3 parallax = ParallaxOffsetAndDepth( offset, innerScale, E, mixedNormal, innerThickness * innerMapAlpha );
	
	// Sample the inner map at the offset coords
	vec4 innerMap = texture2D( InnerMap, parallax.xy * innerScale );

	vec3 reflected = reflect( -E, normal );
	vec3 reflectedVS = b * reflected.x + t * reflected.y + N * reflected.z;
	
	vec4 cube = textureCube( CubeMap, vec3( gl_ModelViewMatrixInverse * vec4( reflectedVS, 0.0 ) ) );
	vec4 env = texture2D( EnvironmentMap, offset );
	
	vec4 color;
	vec3 inner = innerMap.rgb;
	vec3 outer = baseMap.rgb;
	vec3 innerOuter;
	
	vec3 diffuse = ColorEA.rgb + (ColorD.rgb * NdotL);
	outer *= diffuse;
	inner *= diffuse;

	color.a = ColorD.a;
	
	
	// Backlight
	// 	Mixed with inner and outer map
	vec3 backlight;
	if ( hasBacklight ) {
		backlight = texture2D( BacklightMap, offset ).rgb;
		backlight *= wrap * D.rgb;
		
		inner += innerMap.rgb * backlight * (1.0 - baseMap.a);
		outer += baseMap.rgb * backlight * baseMap.a;
	}
	
	// Emissive
	//	Mixed with outer map
	if ( hasEmit ) {
		outer += tonemap( baseMap.rgb * glowColor ) / tonemap( 1.0f / vec3(glowMult + 0.001f) );
	}
	
	// Mix inner/outer layer based on fresnel
	float outerMix = max( 1.0 - EdotN, baseMap.a );
	innerOuter = mix( inner, outer, outerMix );
	
	color.rgb += innerOuter;
	
	// Environment
	color.rgb += cube.rgb * env.r * outerReflection * diffuse;

	// Specular
	float spec = 0.0;
	if ( NdotL > 0.0 && specStrength > 0.0 ) {
		float RdotE = max( dot(R, E), 0.0 );
		if ( RdotE > 0.0 ) {
			spec = normalMap.a * gl_LightSource[0].specular.r * specStrength * pow(RdotE, 0.8*specGlossiness);
			color.rgb += spec * specColor;
		}
	}
	
	// TODO: Test rim and soft light mixing with inner/outer layer

	vec4 mask;
	if ( hasRimlight || hasSoftlight ) {
		mask = texture2D( LightMask, offset );
	}

	vec3 rim;
	if ( hasRimlight ) {
		rim = vec3(( 1.0 - NdotL ) * ( 1.0 - EdotN ));
		rim = mask.rgb * pow(rim, vec3(lightingEffect2)) * D.rgb * vec3(0.66);
		rim *= smoothstep( -0.5, 1.0, facing );
		
		color.rgb += rim;
	}

	vec3 soft;
	if ( hasSoftlight ) {
		soft = vec3((1.0 - wrap) * (1.0 - NdotL));
		soft = smoothstep( -1.0, 1.0, soft );

		// TODO: Very approximate, kind of arbitrary. There is surely a more correct way.
		soft *= mask.rgb * pow(soft, vec3(4.0/(lightingEffect1*lightingEffect1)));
		soft *= D.rgb * A.rgb + (0.01 * lightingEffect1*lightingEffect1);

		color.rgb += baseMap.rgb * soft;
	}

	color.rgb = tonemap( color.rgb ) / tonemap( vec3(1.0) );
	
	gl_FragColor = color;
	gl_FragColor.a *= alpha;
}
