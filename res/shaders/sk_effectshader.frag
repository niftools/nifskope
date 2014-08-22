#version 120

highp uniform sampler2D BaseMap;
highp uniform sampler2D GreyscaleMap;

uniform bool doubleSided;

uniform bool hasSourceTexture;
uniform bool hasGreyscaleMap;
uniform bool greyscaleAlpha;
uniform bool greyscaleColor;

uniform bool useFalloff;
uniform bool vertexColors;
uniform bool vertexAlpha;

uniform vec4 glowColor;
uniform float glowMult;

uniform vec2 uvScale;
uniform vec2 uvOffset;

uniform vec4 falloffParams;
uniform float falloffDepth;

varying vec3 LightDir;
varying vec3 ViewDir;

//varying vec4 ColorEA;
varying vec4 ColorD;

varying vec3 N;
varying vec3 v;

vec4 colorLookup( float x, float y ) {
	
	return texture2D( GreyscaleMap, vec2( clamp(x, 0.01, 0.99), clamp(y, 0.01, 0.99)) );
}

void main( void )
{
	highp vec4 color;
	vec4 emissiveColor = glowColor;
	float emissiveMult = glowMult;
	
	float alphaMult = emissiveColor.a * emissiveColor.a;
	
	highp vec3 normal = N;
	//vec3 normal = normalize(cross(dFdy(v.xyz), dFdx(v.xyz)));
	
	//if ( !doubleSided && !gl_FrontFacing ) { return; }
	
	//vec3 L = normalize(LightDir);
	vec3 E = normalize(ViewDir);
	//vec3 R = reflect(-E, normal);
	//vec3 H = normalize( L + E );
	//float RdotE = abs(smoothstep(-1.0, 1.0, dot(R, E)));
	
	
	float tmp2 = falloffDepth; // Unused right now
	
	// Falloff
	highp float falloff = 1.0;
	if ( useFalloff ) {
		//falloff = smoothstep( clamp(falloffParams.y * 2.0, 0.0, 2.0), clamp(falloffParams.x * 2.0, 0.0, 2.0), vec3(abs(E.b)));
		
		falloff = falloffParams.x;
		
		//if ( abs(E.b) < 0.5 ) 
		//	color.a = 0.0;
	
		float startO = min(falloffParams.z, 0.99);
		float stopO = max(falloffParams.w, 0.01);
		

		if ( falloffParams.x < falloffParams.y ) {
			falloff = smoothstep( falloffParams.y, falloffParams.x, abs(E.b));
		} else if ( falloffParams.x > falloffParams.y ) {
			falloff = smoothstep( falloffParams.y, falloffParams.x, abs(E.b));
		}

		//falloff = mix( falloffParams.y, falloffParams.x, abs(E.b));

		falloff = mix( max(falloffParams.w, 0.01), min(falloffParams.z, 0.99), falloff );
	}
	
	highp vec4 baseMap;
	//if ( hasSourceTexture ) {
		baseMap = texture2D( BaseMap, gl_TexCoord[0].st * uvScale + uvOffset );

		// This is better for some 
		color.rgb = clamp(baseMap.rgb, 0.00, 1.0) * ColorD.rgb * emissiveColor.rgb;
		color.a = ColorD.a * baseMap.a * alphaMult;
		color.a *= falloff;
		color = clamp( color, 0.00, 1.0 );
		
		// This is better for others
		color.rgb = clamp(baseMap.rgb, 0.00, 1.0) * ColorD.rgb * emissiveColor.rgb;
		color.a = min( ColorD.a, baseMap.a );
		//color *= emissiveColor;
		color.a *= falloff * alphaMult;
		color = clamp( color, 0.01, 1.0 );

	//}
	
	highp vec4 red, green, blue, alpha;
	
	

	
	if ( greyscaleColor ) {
	
		if ( greyscaleAlpha ) {
		
			// "alphaMult" does not seem to be needed in x component
			// Removing "alphaMult" from y component gives proper but too bright falloff on alterposprojectile's glass cubes
			
			alpha = colorLookup( min( ColorD.a, baseMap.a ), max( ColorD.a, baseMap.a ) * alphaMult * falloff );

			color.a = alpha.a;
		}
		
		vec4 luA = colorLookup( color.a + baseMap.a, emissiveColor.a + baseMap.a);
		
		// Rest unused? Only R seems to affect anything
		float emRGB = emissiveColor.r; //( emissiveColor.r + emissiveColor.g + emissiveColor.b ) * 0.33333;
		
		
		// Unknown if y- lookup needs "* falloff"
		//highp vec4 luR = colorLookup( baseMap.r, ColorD.r * emissiveColor.r );
		highp vec4 luG = colorLookup( baseMap.g, ColorD.g * falloff * emRGB  );
		//highp vec4 luB = colorLookup( baseMap.b, ColorD.b * emissiveColor.b );
		
		
		color.rgb = luG.rgb;
		
		float ugh = 0.0;
		if ( vertexAlpha && gl_FrontFacing )
			ugh = luA.a;

		color.a *= max( ugh, max( luG.a, ColorD.a ));
	}

	gl_FragColor.rgb = color.rgb * emissiveMult;
	gl_FragColor.a = color.a;

}
