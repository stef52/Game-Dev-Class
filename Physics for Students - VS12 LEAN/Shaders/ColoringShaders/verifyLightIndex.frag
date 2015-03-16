#version 130

#define OVERLAP_LIGHTS 4

uniform sampler2D LightIndexedColorTexture; 

out vec4 finalColor;

//Note: void decodeLights (vec4 floatLightIndices, out float lightIndices [4]) returns the 
//lights ordered such that if there is only one light, it's in lightIndices [0]; if there 
//are 2, it's in [0] and [1], etc.

#include "ColoringShaders/decodeLightIndices.function"

void main () {
	if (mod(gl_FragCoord.x,50.0) < 2.0 || mod(gl_FragCoord.y,50.0) < 2.0) {finalColor = vec4 (0,1,0,1); return;}
	
	vec4 floatLightIndices = texelFetch (LightIndexedColorTexture, ivec2 (gl_FragCoord.xy), 0);
	float lightIndices [4]; decodeLightIndices (floatLightIndices, lightIndices);
	
	int cell = mod(gl_FragCoord.x,50.0) < 25 //Cells 0,1,2,3 starts at bottom left and rotate counter clockwise...
		? (mod(gl_FragCoord.y,50.0) < 25
			? 0
			: 3)
		: (mod(gl_FragCoord.y,50.0) < 25
			? 1
			: 2);
	
	float lightIndex = (16 * floor(gl_FragCoord.y/50.0)) + floor(gl_FragCoord.x/50.0);
	
	//Actually used: ivec2(lightIndex+epsilon)*256,0)
	float cellLightIndex = lightIndices [cell];
	
	bool showRawLightIndex = true; bool showUndecodedLightIndex = false;
	if (showRawLightIndex) {
		finalColor = vec4 (cellLightIndex/256, cellLightIndex/256, cellLightIndex/256, 1.0); 
		return;
	}
	if (showUndecodedLightIndex) {
		finalColor = texelFetch (LightIndexedColorTexture, ivec2 (gl_FragCoord.xy), 0); 
		return;
	}
	
	//Show the light index in gray scale or red if it's not what it's supposed to be...
	finalColor  = abs (lightIndex-cellLightIndex) < 1.5e0
		? vec4 (cellLightIndex/256, cellLightIndex/256, cellLightIndex/256, 1.0)
		: vec4 (1.0, 0.0, 0.0, 1.0);
}
