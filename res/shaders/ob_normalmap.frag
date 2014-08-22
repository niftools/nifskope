uniform sampler2D BaseMap;
uniform sampler2D NormalMap;
uniform sampler2D LightMask;
uniform sampler2D BacklightMap;

uniform vec3 specColor;
uniform float specStrength;
uniform float specGlossiness;

uniform bool hasSoftlight;
uniform bool hasBacklight;
uniform bool hasRimlight;

uniform float lightingEffect1;
uniform float lightingEffect2;

varying vec3 LightDir;
varying vec3 ViewDir;

varying vec4 ColorEA;
varying vec4 ColorD;

void main( void )
{
	vec4 color = ColorEA;
	float spec = 0.0;
	
	vec4 nmap = texture2D( NormalMap, gl_TexCoord[0].st );
	vec3 normal = normalize(nmap.rgb * 2.0 - 1.0);

	vec3 L = normalize(LightDir);
	vec3 E = normalize(ViewDir);
	vec3 R = reflect(-L, normal);
	//vec3 H = normalize( L + E );

	float NdotL = max(dot(normal, L), 0.0);

	color += ColorD * NdotL;
	color.a = ColorD.a;
	vec4 baseMap = texture2D( BaseMap, gl_TexCoord[0].st );
	color *= baseMap;

	if ( NdotL > 0.0 && specStrength > 0.0 ) {
		float RdotE = max( dot( R, E ), 0.0 );

		// TODO: Attenuation?

		if ( RdotE > 0.0 ) {
			spec = nmap.a * gl_LightSource[0].specular.rgb * specStrength * pow(RdotE, 0.8*specGlossiness);
			color.rgb += spec * specColor;
		}
	}

	if ( hasBacklight ) {
		vec3 backlight = texture2D( BacklightMap, gl_TexCoord[0].st ).rgb;
		color.rgb += backlight * (1.0 - NdotL) * 0.66;
	}

	vec4 mask;
	if ( hasRimlight || hasSoftlight ) {
		mask = texture2D( LightMask, gl_TexCoord[0].st );
	}

	float facing = dot(-L, E);

	vec3 rim;
	if ( hasRimlight ) {
		rim = vec3(( 1.0 - NdotL ) * ( 1.0 - max(dot(normal, E), 0.0)));
		//rim = smoothstep( 0.0, 1.0, rim );
		rim = mask.rgb * pow(rim, vec3(lightingEffect2)) * gl_LightSource[0].diffuse.rgb * vec3(0.66);
		rim *= smoothstep( -0.5, 1.0, facing );
		color.rgb += rim;
	}

	float wrap = dot(normal, -L);

	vec3 soft;
	if ( hasSoftlight ) {
		soft = vec3((1.0 - wrap) * (1.0 - NdotL));
		soft = smoothstep( -1.0, 1.0, soft );

		// TODO: Very approximate, kind of arbitrary. There is surely a more correct way.
		soft *= mask.rgb * pow(soft, vec3(4.0/(lightingEffect1*lightingEffect1))); // * gl_LightSource[0].ambient.rgb;
		//soft *= smoothstep( -1.0, 0.0, soft );
		//soft = mix( soft, color.rgb, gl_LightSource[0].ambient.rgb );

		//soft = smoothstep( 0.0, 1.0, soft );
		soft *= gl_LightSource[0].diffuse.rgb * gl_LightSource[0].ambient.rgb + (0.01 * lightingEffect1*lightingEffect1);

		//soft = clamp( soft, 0.0, 0.5 );
		//soft *= smoothstep( -0.5, 1.0, facing );
		//soft = mix( soft, color.rgb, gl_LightSource[0].diffuse.rgb );
		color.rgb += baseMap.rgb * soft;
	}

	//color = min( color, 1.0 );

	gl_FragColor = color;
}
