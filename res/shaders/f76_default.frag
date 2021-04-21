#version 130
#extension GL_ARB_shader_texture_lod : require

uniform sampler2D BaseMap;
uniform sampler2D NormalMap;
uniform sampler2D GlowMap;
uniform sampler2D ReflMap;
uniform sampler2D LightingMap;
uniform sampler2D GreyscaleMap;
uniform samplerCube CubeMap;

uniform vec4 solidColor;
uniform vec3 specColor;
uniform float specStrength;
uniform float specGlossiness; // "Smoothness" in FO4; 0-1
uniform float fresnelPower;

uniform float paletteScale;

uniform vec3 glowColor;
uniform float glowMult;

uniform float alpha;

uniform vec3 tintColor;

uniform vec2 uvScale;
uniform vec2 uvOffset;

uniform bool hasEmit;
uniform bool hasGlowMap;
uniform bool hasSoftlight;
uniform bool hasTintColor;
uniform bool hasCubeMap;
uniform bool hasSpecularMap;
uniform bool greyscaleColor;
uniform bool doubleSided;

uniform float subsurfaceRolloff;
uniform float rimPower;
uniform float backlightPower;

uniform float envReflection;

uniform bool isWireframe;
uniform bool isSkinned;
uniform mat4 worldMatrix;

in vec3 LightDir;
in vec3 ViewDir;

in vec4 A;
in vec4 C;
in vec4 D;

in vec3 N;
in vec3 t;
in vec3 b;

in mat4 reflMatrix;

#ifndef M_PI
	#define M_PI 3.1415926535897932384626433832795
#endif

#define FLT_EPSILON 1.192092896e-07F // smallest such that 1.0 + FLT_EPSILON != 1.0

float G1V(float NdotV, float k)
{
	return 1.0 / (NdotV * (1.0 - k) + k);
}

float LightingFuncGGX_REF0(float NdotL, float NdotH, float NdotV, float LdotH, float roughness, float F0)
{
	float alpha = roughness * roughness;
	float F, D, vis;
	// D
	float alphaSqr = alpha * alpha;
	float denom = NdotH * NdotH * (alphaSqr - 1.0) + 1.0;
	D = alphaSqr / (M_PI * denom * denom);
	// F
	float LdotH5 = pow(1.0 - LdotH, 5);
	F = F0 + (1.0 - F0) * LdotH5;

	// V
	float k = alpha/2.0f;
	vis = G1V(NdotL, k) * G1V(NdotV, k);

	float specular = NdotL * D * F * vis;
	return specular;
}

vec3 LightingFuncGGX_REF(float NdotL, float NdotH, float NdotV, float LdotH, float roughness, vec3 F0, float specOcc)
{
    float alpha = roughness * roughness;
    // D (GGX normal distribution)
    float alphaSqr = alpha * alpha;
    float denom = NdotH * NdotH * (alphaSqr - 1.0) + 1.0;
    float D = alphaSqr / (denom * denom);
    // no pi because BRDF -> lighting
    // F (Fresnel term)
    float F_a = 1.0;
    float F_b = pow(1.0 - LdotH, 5.0);
    vec3 F = mix(vec3(F_b), vec3(F_a), F0) * specOcc;
    // G (remapped hotness, see Unreal Shading)
    float k = (alpha + 2 * roughness + 1) / 8.0;
    float G = NdotL / (mix(NdotL, 1, k) * mix(NdotV, 1, k));

    return D * F * G / 4.0;
}

float OrenNayar(vec3 L, vec3 V, vec3 N, float roughness, float NdotL)
{
	//float NdotL = dot(N, L);
	float NdotV = dot(N, V);
	float LdotV = dot(L, V);
	
	float rough2 = roughness * roughness;
	
	float A = 1.0 - 0.5 * (rough2 / (rough2 + 0.57));
	float B = 0.45 * (rough2 / (rough2 + 0.09));
	
	float a = min( NdotV, NdotL );
	float b = max( NdotV, NdotL );
	b = (sign(b) == 0.0) ? FLT_EPSILON : sign(b) * max( 0.01, abs(b) ); // For fudging the smoothness of C
	float C = sqrt( (1.0 - a * a) * (1.0 - b * b) ) / b;
	
	float gamma = LdotV - NdotL * NdotV;
	float L1 = A + B * max( gamma, FLT_EPSILON ) * C;
	
	return L1 * max( NdotL, FLT_EPSILON );
}

float OrenNayarFull(vec3 L, vec3 V, vec3 N, float roughness, float NdotL)
{
	//float NdotL = dot(N, L);
	float NdotV = dot(N, V);
	float LdotV = dot(L, V);
	
	float angleVN = acos(max(NdotV, FLT_EPSILON));
	float angleLN = acos(max(NdotL, FLT_EPSILON));
	
	float alpha = max(angleVN, angleLN);
	float beta = min(angleVN, angleLN);
	float gamma = LdotV - NdotL * NdotV;
	
	float roughnessSquared = roughness * roughness;
	float roughnessSquared9 = (roughnessSquared / (roughnessSquared + 0.09));
	
	// C1, C2, and C3
	float C1 = 1.0 - 0.5 * (roughnessSquared / (roughnessSquared + 0.33));
	float C2 = 0.45 * roughnessSquared9;
	
	if( gamma >= 0.0 ) {
		C2 *= sin(alpha);
	} else {
		C2 *= (sin(alpha) - pow((2.0 * beta) / M_PI, 3.0));
	}
	
	float powValue = (4.0 * alpha * beta) / (M_PI * M_PI);
	float C3 = 0.125 * roughnessSquared9 * powValue * powValue;
	
	// Avoid asymptote at pi/2
	float asym = M_PI / 2.0;
	float lim1 = asym + 0.01;
	float lim2 = asym - 0.01;

	float ab2 = (alpha + beta) / 2.0;

	if ( beta >= asym && beta < lim1 )
		beta = lim1;
	else if ( beta < asym && beta >= lim2 )
		beta = lim2;

	if ( ab2 >= asym && ab2 < lim1 )
		ab2 = lim1;
	else if ( ab2 < asym && ab2 >= lim2 )
		ab2 = lim2;
	
	// Reflection
	float A = gamma * C2 * tan(beta);
	float B = (1.0 - abs(gamma)) * C3 * tan(ab2);
	
	float L1 = max(FLT_EPSILON, NdotL) * (C1 + A + B);
	
	// Interreflection
	float twoBetaPi = 2.0 * beta / M_PI;
	float L2 = 0.17 * max(FLT_EPSILON, NdotL) * (roughnessSquared / (roughnessSquared + 0.13)) * (1.0 - gamma * twoBetaPi * twoBetaPi);
	
	return L1 + L2;
}

// Schlick's Fresnel approximation
float fresnelSchlick(float VdotH, float F0)
{
	float base = 1.0 - VdotH;
	float exp = pow(base, fresnelPower);
	return clamp(exp + F0 * (1.0 - exp), 0.0, 1.0);
}

vec3 fresnelSchlickRoughness(float NdotV, vec3 F0, float roughness)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - NdotV, 5.0);
}

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

vec4 colorLookup(float x, float y) {
	
	return texture2D(GreyscaleMap, vec2(clamp(x, 0.0, 1.0), clamp(y, 0.0, 1.0)));
}

void main(void)
{
	if ( isWireframe ) {
		gl_FragColor = solidColor;
		return;
	}
	vec2 offset = gl_TexCoord[0].st * uvScale + uvOffset;

	vec4 baseMap = texture2D(BaseMap, offset);
	vec4 normalMap = texture2D(NormalMap, offset);
	vec4 lightingMap = texture2D(LightingMap, offset);
	vec4 reflMap = texture2D(ReflMap, offset);
	vec4 glowMap = texture2D(GlowMap, offset);

	vec3 normal = normalMap.rgb;
	// Calculate missing blue channel
	normal.b = sqrt(1.0 - dot(normal.rg, normal.rg));
	if ( !gl_FrontFacing && doubleSided ) {
		normal *= -1.0;	
	}
	// For _msn (Test with FSF1_Face)
	//normal.z = sqrt(1.0 - dot(normal.xy, normal.xy));
	
	vec3 L = normalize(LightDir);
	vec3 V = normalize(ViewDir);
	vec3 R = reflect(-L, normal);
	vec3 H = normalize(L + V);
	
	float NdotL = dot(normal, L);
	float NdotL0 = max(NdotL, FLT_EPSILON);
	float NdotH = max(dot(normal, H), FLT_EPSILON);
	float NdotV = max(dot(normal, V), FLT_EPSILON);
	float VdotH = max(dot(V, H), FLT_EPSILON);
	float LdotH = max(dot(L, H), FLT_EPSILON);
	float NdotNegL = max(dot(normal, -L), FLT_EPSILON);

	vec3 reflected = reflect(V, normal);
	vec3 reflectedVS = b * reflected.x + t * reflected.y + N * reflected.z;
	vec3 reflectedWS = vec3(reflMatrix * (gl_ModelViewMatrixInverse * vec4(reflectedVS, 0.0)));

	vec4 color;
	vec3 albedo = baseMap.rgb * C.rgb;
	vec3 diffuse = A.rgb + D.rgb * NdotL0;
	if ( greyscaleColor ) {
		vec4 luG = colorLookup(baseMap.g, paletteScale - (1 - C.r));

		albedo = luG.rgb;
	}
	
	// Emissive
	vec3 emissive = vec3(0.0);
	if ( hasEmit ) {
		emissive += glowColor * glowMult;
		if ( hasGlowMap ) {
			emissive *= glowMap.rgb;
		}
	} else if ( hasGlowMap ) {
		emissive += glowMap.rgb * glowMult;
	}

	emissive *= lightingMap.a;
	//float f0 = 1.0;
	float metallicity = max(reflMap.r, max(reflMap.g, reflMap.b));
	//f0 = mix(0.02, 0.9, metallicity);
	
	vec3 f0 = (reflMap.g == 0 && reflMap.b == 0) ? vec3(reflMap.r) : reflMap.rgb;
	f0 = max(vec3(0.01), f0);

	// Specular
	float g = 1.0;
	float smoothness = clamp(specGlossiness, 0.0, 1.0);
	float specOcc = 1.0;
	vec3 spec = vec3(0.0);
	if ( hasSpecularMap ) {
		g = lightingMap.r;
		specOcc = lightingMap.g;
		smoothness = g * smoothness;
		spec = vec3(LightingFuncGGX_REF(NdotL0, NdotH, NdotV, LdotH, 1.0 - smoothness, f0, specOcc)) * NdotL0 * D.rgb;
		spec *= 1.0 - metallicity;
	}

	// Diffuse
	float diff = OrenNayarFull(L, V, normal, 1.0 - smoothness, NdotL);
	diffuse = vec3(diff);
	diffuse *= 1.0 - metallicity;

	// Environment
	vec4 cube = textureLod(CubeMap, reflectedWS, 8.0 - smoothness * 8.0);
	vec3 refl = vec3(0.0);
	if ( hasCubeMap ) {
		cube.rgb *= envReflection * specStrength;

		// diffuse term alt: (vec3(1.0 - diff) * spec) + (vec3(1.0 - diff) * albedo)
		refl += cube.rgb * fresnelSchlickRoughness(NdotV, f0, 1.0 - smoothness) * D.rgb * specOcc;
	}

	//vec3 soft = vec3(0.0);
	//float wrap = NdotL;
	//if ( hasSoftlight || subsurfaceRolloff > 0.0 ) {
	//	wrap = (wrap + subsurfaceRolloff) / (1.0 + subsurfaceRolloff);
	//	soft = albedo * max(0.0, wrap) * smoothstep(1.0, 0.0, sqrt(diff));
	//
	//	diffuse += soft;
	//}
	
	//if ( hasTintColor ) {
	//	albedo *= tintColor;
	//}
	
	// Diffuse
	color.rgb = diffuse * albedo * D.rgb;
	// Ambient
	color.rgb += A.rgb * albedo;
	// Specular
	color.rgb += spec;
	color.rgb += refl;

	// Emissive
	color.rgb += emissive;
	
	color.rgb = tonemap(color.rgb) / tonemap(vec3(1.0));
	color.a = C.a * baseMap.a;

	gl_FragColor = color;
	gl_FragColor.a *= alpha;
}
