#version 120

uniform sampler2D BaseMap;
uniform sampler2D NormalMap;
uniform sampler2D LightMask;
uniform sampler2D BacklightMap;

uniform vec3 specColor;
uniform float specStrength;
uniform float specGlossiness;

uniform vec3 glowColor;
uniform float glowMult;

uniform vec2 uvScale;
uniform vec2 uvOffset;

uniform bool hasEmit;
uniform bool hasSoftlight;
uniform bool hasBacklight;
uniform bool hasRimlight;

uniform float lightingEffect1;
uniform float lightingEffect2;

varying vec3 LightDir;
varying vec3 ViewDir;

varying vec4 A;
varying vec4 C;
varying vec4 D;


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

void main( void )
{
	vec2 offset = gl_TexCoord[0].st * uvScale + uvOffset;

	vec4 baseMap = texture2D( BaseMap, offset );
	vec4 normalMap = texture2D( NormalMap, offset );
	
	vec3 normal = normalize(normalMap.rgb * 2.0 - 1.0);
	
	
	vec3 L = normalize(LightDir);
	vec3 E = normalize(ViewDir);
	vec3 R = reflect(-L, normal);
	
	float NdotL = max( dot(normal, L), 0.0 );
	float EdotN = max( dot(normal, E), 0.0 );
	float wrap = max( dot(normal, -L), 0.0 );
	float facing = max( dot(-L, E), 0.0 );


	vec4 color;
	vec3 albedo = baseMap.rgb * C.rgb;
	vec3 diffuse = A.rgb + (D.rgb * NdotL);


	// Emissive
	vec3 emissive;
	if ( hasEmit ) {
		emissive += albedo * glowColor * glowMult;
	}

	// Specular
	vec3 spec;
	if ( NdotL > 0.0 && specStrength > 0.0 ) {
		float RdotE = max( dot(R, E), 0.0 );
		if ( RdotE > 0.0 ) {
			spec = vec3(normalMap.a * gl_LightSource[0].specular.r * specStrength * pow(RdotE, 0.8*specGlossiness));
			spec *= specColor;
		}
	}

	vec3 backlight;
	if ( hasBacklight ) {
		backlight = texture2D( BacklightMap, offset ).rgb;
		emissive += albedo * backlight * wrap * D.rgb;
	}

	vec4 mask;
	if ( hasRimlight || hasSoftlight ) {
		mask = texture2D( LightMask, offset );
	}

	vec3 rim;
	if ( hasRimlight ) {
		rim = vec3((1.0 - NdotL) * (1.0 - EdotN));
		rim = mask.rgb * pow(rim, vec3(lightingEffect2)) * D.rgb * vec3(0.66);
		rim *= smoothstep( -0.5, 1.0, facing );
		
		emissive += rim;
	}
	
	vec3 soft;
	if ( hasSoftlight ) {
		soft = vec3((1.0 - wrap) * (1.0 - NdotL));
		soft = smoothstep( -1.0, 1.0, soft );

		// TODO: Very approximate, kind of arbitrary. There is surely a more correct way.
		soft *= mask.rgb * pow(soft, vec3(4.0/(lightingEffect1*lightingEffect1)));
		soft *= D.rgb * A.rgb + (0.01 * lightingEffect1*lightingEffect1);

		emissive += albedo * soft;
	}

	color.rgb = (albedo * diffuse) + spec + emissive;
	color.rgb = tonemap( color.rgb ) / tonemap( vec3(1.0) );
	color.a = C.a * baseMap.a;

	gl_FragColor = color;
}
