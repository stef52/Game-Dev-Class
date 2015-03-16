#version 130

/////////////////////////////////////////////////////////////////////
// drawWorldWithAllLights
// This shader program takes a screen sized light index texture and
// draws lit geometry with it. 
/////////////////////////////////////////////////////////////////////

#define OVERLAP_LIGHTS 4

uniform sampler2D Base;
uniform sampler2D LightIndexedColorTexture; 
uniform sampler2D LightPositionTexture; 
uniform sampler2D LightColorTexture;

in vec4 clipSpace; 
in vec2 texCoord;
in vec3 pixelPositionCS, pixelNormalCS, pixelToCameraCS;

out vec4 finalColor; 

vec3 lightColor; //Referenced by Kd () in library...
#define USING_LIGHT_INDEXED_DRAWING
#include "ColoringShaders/WilfLightIndexDrawingLibrary.all"

//Note: void decodeLights (vec4 floatLightIndices, out float lightIndices [4]) returns the 
//lights ordered such that if there is only one light, it's in lightIndices [0]; if there 
//are 2, it's in [0] and [1], etc.

#include "ColoringShaders/decodeLightIndices.function"

vec3 attenuatedLightColor (vec3 pixelPositionCS, float lightIndex, out vec3 pixelToLightCS) {
		#if (EXPERIMENT <= 2)
			//Pretend it's black... with no attenuation...
			return vec3 (0.0, 0.0, 0.0);
		#else
			/////
			//Fetch the light position from the LightPositionTexture and use it to compute a pixel to light vector (not normalized)...
			//Recall that the rgba texture contains xyzs where s is the scale factor to scale the light model's unit sphere.
			//vec4 lightPosition = texture2D(LightPositionTexture, vec2(lightIndex, 0),0); //texelFetch(LightPositionTexture, ivec2(lightIndex, 0), 0);
			vec4 lightPosition = texelFetch (LightPositionTexture, ivec2 (lightIndex * 256, 0), 0);
			vec3 pixelToLightvec = vec3(pixelPositionCS- lightPosition.xyz);
		
			//Extract the light radius from the scale in light position's.a; A unit sphere scaled by s has radius s * 0.5.
			float lightRadius = lightPosition.a*0.5;

			//Fetch the light color from the LightColorTexture texture...
			vec4 lightColor = texture2D(LightColorTexture, vec2(lightIndex, lightIndex));

			//Build an interpolation t which is the "distance to the light" / "light radius"
			//and use it to construction an attenuation factor that interpolates from 1 when t=0 to 0 when t=1 and clamps to 0 for t>1.
			//So we go from full brightness at the position of the light to zero at the radius and beyond...
			
			float squareDistanceToLight = dot(pixelToLightvec, pixelToLightvec);

			float t = sqrt(squareDistanceToLight/(lightRadius*lightRadius));
			
			//But, we want a value that goes from 1 to 0 (clamp when < 0).
			float attenuation = clamp01 (interpolate (1.0, 0, t)); 

			//Return the attenuated color...
			return lightColor.xyz * attenuation;
		
	#endif //EXPERIMENT
}

//Note: The only case we actually made work is USE_EMISSIVE...
#define USE_DIRECTION_SENSITIVE_DIFFUSE 0
#define USE_DIRECTION_INSENSITIVE_DIFFUSE 1
#define USE_EMISSIVE 2
#define AMBIENT_TYPE USE_EMISSIVE

void main () {
	//When a vector is interpolated, it's length changes so renormalize direction vectors used for lighting...
	vec3 pixelNormal = normalize (pixelNormalCS); //When a vector is interpolated, it's length changes...
	vec3 pixelToCamera = normalize (pixelToCameraCS); //When a vector is interpolated, it's length changes...

	//Some useful constants...
	float integerOne = 1.0 / 256.0;
//vec4 floatLightIndices = texelFetch (LightIndexedColorTexture, ivec2 (gl_FragCoord.xy), 0);
//finalColor = floatLightIndices; return;
	
	//Obtain the texture color and the 4 light indices...
	vec4 baseColor = texture2D (Base, texCoord);
	vec4 floatLightIndices = texelFetch (LightIndexedColorTexture, ivec2 (gl_FragCoord.xy), 0);
	float lightIndices [4]; decodeLightIndices (floatLightIndices, lightIndices);
	
	vec3 color = vec3 (0.0, 0.0, 0.0);
	for (int i = 0; i < OVERLAP_LIGHTS; i++) {
		//Obtain the light index and convert from range 0:255 to texture coordinates range 0:1...
		float lightIndex = clamp01 ((lightIndices [i] + 0.5) * integerOne);
		if (lightIndex < integerOne) continue; //Skip black light...
	
		vec3 pixelToLightCS; lightColor = attenuatedLightColor (pixelPositionCS, lightIndex, pixelToLightCS);
	    
	    //Recall: Kd () references lightColor...
	    #if (AMBIENT_TYPE == USE_DIRECTION_SENSITIVE_DIFFUSE)
			vec3 pixelToLight = normalize (pixelToLightCS); 
			color += ambientFreeCombinedColor (baseColor, pixelNormal, pixelToLight, pixelToCamera, 1.0).rgb;
	    #elif (AMBIENT_TYPE == USE_DIRECTION_INSENSITIVE_DIFFUSE)
			vec3 pixelToLight = normalize (pixelToLightCS); 
			color += ambientFreeAndDirectionInsensitiveDiffuseCombinedColor (baseColor, pixelNormal, pixelToLight, pixelToCamera, 1.0).rgb;	
		#elif (AMBIENT_TYPE == USE_EMISSIVE)
			color += lightColor;
		#endif //AMBIENT_TYPE
	}
	
	vec3 darkenedBaseColor = (baseColor * ambientColor ()).rgb;
	finalColor = vec4 (darkenedBaseColor + color, 1.0);
}