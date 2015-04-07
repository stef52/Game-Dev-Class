#version 120
#extension GL_ARB_texture_rectangle: enable
#extension GL_EXT_gpu_shader4: enable

#include "wilfAmbientOcclusionLibrary.all"

//To do the following, we would have to expunge all routines using the unused uniforms...
#define DISCARD_UNUSED_UNIFORMS 0

uniform sampler2DRect normTex;
uniform sampler2DRect posTex;
uniform sampler2DRect loResAOTex;
uniform sampler2DRect loResNormTex;
uniform sampler2DRect loResPosTex;

uniform sampler2DRect lastFrameAOTex;
uniform sampler2DRect lastFramePosTex;
uniform float dMax; //radius of influence in camera-space (meters). //COMMENT IS WILF ADDITION  //wilf fix
uniform float rMax; //radius of influence in pixel-space (pixels). //COMMENT IS WILF ADDITION
uniform float r; //ELIMINATED BY WILF IN FAVOR OF fovFactor...
uniform float resolution;
uniform float fovFactor; //WILF ADDITION...

#if (DISCARD_UNUSED_UNIFORMS)
#else
	uniform mat4 iMVMat;
	uniform mat4 OLDmVMat; //WILF ADDITION
	uniform mat4 mVMat;
	uniform mat4 projMat;
	uniform float poissonDisk[32];
#endif //DISCARD_UNUSED_UNIFORMS

uniform bool usePoisson; uniform bool useUpsampling; uniform bool useTemporalSmoothing;

#define USE_ADJUSTABLE_R_MAX 0
float RMAX; 

out vec4 AO;

//Local variables accessed by functions highResolutionOcclusion, lowResolutionOcclusion, temporallyBlendedOcclusion...
vec2 uv; vec4 basePosition; vec3 baseNormal; 

vec3 highResolutionOcclusion () {
	/*
	//Experiment 0: Use hack version with NO DOWNSAMPLING... .cpp changes. Set LEVEL_COUNT = 1.
	return HACKambientOcclusion (uv, basePosition, baseNormal, posTex, normTex); 
	*/
	
	/*
	//Experiment 1: Use UNFILTERED  version with NO DOWNSAMPLING. .cpp changes. Set LEVEL_COUNT = 1.
	//Use one sample point to the right + up from base uv...
	float occlusion = ambientOcclusion (uv+vec2(1,1), basePosition, baseNormal, posTex, normTex, RMAX); //dMax); //wilf fix
	return vec3 (occlusion, occlusion, 1);
	*/

	return usePoisson
		? poissonFilterAmbientOcclusion (uv, posTex, normTex, basePosition, baseNormal, RMAX) //wilf fix
		: gridFilterAmbientOcclusion (uv, posTex, normTex, basePosition, baseNormal, RMAX); //wilf fix
}

vec3 lowResolutionOcclusion () {
	return (useUpsampling)
		//? HACKedgePreserveUpsample (uv, basePosition, baseNormal, loResNormTex, loResPosTex, loResAOTex)
		? edgePreserveUpsample (uv, basePosition, baseNormal, loResNormTex, loResPosTex, loResAOTex)
		: vec3 (0, 0, 0); //None...
}

float temporallyBlendedOcclusion (float occlusion) {
return occlusion; //DISABLE_TEMPORAL_BLENDING
	const float confidence = 0.4;
	return useTemporalSmoothing   
		? temporalBlend (confidence, resolution, occlusion, basePosition, 
			lastFramePosTex, lastFrameAOTex, projMat, iMVMat, OLDmVMat)
		: occlusion;
}

void main () {
	//Initialize the variable containing the texture coordinates...
	uv = gl_FragCoord.xy;
	basePosition = texture2DRect (posTex, uv);
	baseNormal =  texture2DRect (normTex, uv).xyz;
	#if (USE_ADJUSTABLE_R_MAX)
		const float A = 7.0; const float B = 2.0;
		RMAX = clamp (interpolate (A, B, abs (basePosition.z) / 300.0), B, A);
	#else
		RMAX = rMax;
	#endif //USE_ADJUSTABLE_R_MAX

	/*
	//Experiment 0: Use hack version with NO DOWNSAMPLING... .cpp changes. Set LEVEL_COUNT = 1.
	vec3 occlusion = HACKambientOcclusion (uv, basePosition, baseNormal, posTex, normTex); 
	AO = vec4 (1-occlusion.x); return; //Draw ambient lighting rather than ambient occlusion.
	*/

	/*
	//Experiment 1: Use real version with NO DOWNSAMPLING. .cpp changes. Set LEVEL_COUNT = 1.
	float occlusion = ambientOcclusion (uv, basePosition, baseNormal, posTex, normTex, RMAX); dMax); //wilf fix
	AO = vec4 (1-occlusion); return;
	*/

	//Note: All three sampling/filtering routines return {occlusion/sampleCount, occlusion, sampleCount}.
	vec3 sample1 = highResolutionOcclusion (); //2 filter routines
	vec3 sample2 = lowResolutionOcclusion (); //1 filter routine

	float maximum = max (sample1.x, sample2.x);  
	float sum = sample1.y + sample2.y;
	float count = sample1.z + sample2.z;

	if (usePoisson) {
		//This is true only for the final highest resolution upsampling pass
		//but we return brightness; i.e., 1 - occlusion, rather than ambient occlusion.
		float average = sum / count;
		//WILF: Ask students which is best...
		//float brightness = (1.0 - maximum) *(1.0 - maximum) * (1.0 - maximum) * (1.0 - average); 
		//float brightness = (1.0 - maximum) *(1.0 - maximum) * (1.0 - maximum) * (1.0 - maximum); 
		float brightness = (1.0 - maximum) *(1.0 - maximum) * (1.0 - maximum);
		//float brightness = 1.0 - average; 
		//float brightness = (1.0 - average) * (1.0 - average); 
		//float brightness = (1.0 - maximum) *(1.0 - maximum);
		//float brightness = (1.0 - maximum) * (1.0 - average);
		//float brightness = (1.0 - maximum);
		brightness = temporallyBlendedOcclusion (brightness); //Returns it's input since disabled...
		AO = vec4 (brightness, brightness, brightness, 1.0); 
	} else {
		AO = vec4 (maximum, sum, count, 0.0); 
	}
}
