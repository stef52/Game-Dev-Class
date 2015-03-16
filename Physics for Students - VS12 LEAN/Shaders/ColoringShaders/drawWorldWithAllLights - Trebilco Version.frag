#version 130

#define projectSpace clipSpace
#define BitPlane LightIndexedColorTexture
#define LightPosTex LightPositionTexture
#define LightColorTex LightColorTexture

/////////////////////////////////////////////////////////////////////
// drawWorldWithAllLights
// This shader program takes a screen sized light index texture and
// lights rendered geometry using it. 
//
// The light indexed texture can be layed out in one of three ways:
//   1) 3-4 Lights bit-packed
//   2) 2 Lights max blend equation packed
//   3) 1 Light - no packing
//
// The packing technique used is via a OVERLAP_LIGHTS define.
//
// See http://lightindexed-deferredrender.googlecode.com/files/LightIndexedDeferredLighting1.1.pdf 
// for full details
/////////////////////////////////////////////////////////////////////

#define OVERLAP_LIGHTS 4
#define clamp01(x) clamp(x, 0, 1)

#ifndef __GLSL_CG_DATA_TYPES
#define hfloat float
#define hvec2 vec2
#define hvec3 vec3
#define hvec4 vec4
#else
#define hfloat half
#define hvec2 half2
#define hvec3 half3
#define hvec4 half4
#endif

uniform sampler2D Base;
uniform sampler2D Bump;

uniform bool hasParallax;
uniform vec2 plxCoeffs;

uniform sampler2D BitPlane;

// TODO: Combine into one texture? use 1D textures?
uniform sampler1D LightPosTex;
uniform sampler1D LightColorTex;

in vec2 texCoord;
in vec4 projectSpace;
in mat3 tangentToView;
in vec3 vVec;
in vec3 vVecTangent;

out vec4 finalColor; //Or special built-in variable gl_FragColor 

void main(){

  // Calculate the texture lookup offsets
	vec2 plxTexCoord = texCoord;
	if (hasParallax){
		float height = texture2D(Bump, texCoord).w;
		float offset = height * plxCoeffs.x + plxCoeffs.y;
		plxTexCoord += offset * normalize(vVecTangent).xy;
	}

  // Lookup the base textures
	hvec3 base = texture2D(Base, plxTexCoord).rgb;
	hvec3 bump = texture2D(Bump, plxTexCoord).xyz * 2.0 - 1.0;
	bump = normalize(bump);

  // Set initial ambient lighting
  hvec3 lighting = base * 0.2;

  // Get the bump normal in view space
  hvec3 bumpView = normalize(tangentToView * bump);

  // Get reflection view vector
  hvec3 reflVec = reflect(normalize(vVec), bumpView);

  // Look up the bit planes texture
  hvec4 packedLight = texture2DProj(BitPlane, projectSpace);

#if OVERLAP_LIGHTS >= 3

  // Unpack the bit-packed texture (Use Geforce 8 extensions?)

  // Set depending on the texture size
  hvec4 unpackConst = vec4(4.0, 16.0, 64.0, 256.0) / 256.0;

  // Expand out to the 0..255 range (ceil to avoid precision errors)
  hvec4 floorValues = ceil(packedLight * 254.5);

#if OVERLAP_LIGHTS == 3
  // Ignore the first value when using 3 lights 
  // TODO: could avoid this multiply and floor if lights were processed in the reverse order
  floorValues = floor(floorValues * 0.25);
#endif //OVERLAP_LIGHTS == 3

  // Unpack each lighting channel
  for(int i=0; i< OVERLAP_LIGHTS; i++)
  {
    packedLight = floorValues * 0.25;
    floorValues = floor(packedLight);
    hvec4 fracParts = packedLight - floorValues;

    hfloat lightIndex = dot(fracParts, unpackConst);

#elif OVERLAP_LIGHTS == 2

  // Light indexes packaed as (lightIndex1, 1 - LightIndex2)
  packedLight.g =  1.0 - packedLight.g;

  // If the second light index is the same as the first one, ignore it
  // TODO add a range check? Use alpha channel blending somehow to detect more than one render?
  if(abs(packedLight.g - packedLight.r) < 0.001) 
  {
    packedLight.g = 0.0;
  }

  // Expand out to the 0..255 range and scale back to fix precision errors
  packedLight = ceil(packedLight * 254.5) / 256.0;

  for(int i=0; i< 2; i++)
  {
    hfloat lightIndex = packedLight[i];

#else

  // Expand out to the 0..255 range and scale back to fix precision errors
  packedLight = ceil(packedLight * 254.5) / 256.0;

  {
    // No unpack- direct index lookup
    hfloat lightIndex = packedLight.r;

#endif

    // Possibly add a half texel offset to account for possible precision issues?
    //lightIndex += 0.5/256.0;

    // Lookup the Light position (with inverse radius in alpha)
    vec4 lightViewPos = texture1D(LightPosTex, lightIndex); 

    // Lookup the light color
    hvec3 lightColor = texture1D(LightColorTex, lightIndex).rgb;

    // Get the vector from the light center to the surface
    vec3 lightVec = lightViewPos.xyz - vVec;
    
    // Scale based on the light radius
    vec3 lVec = lightVec * lightViewPos.a;

    // Calculate attenuation
	  float atten = clamp01(1.0 - dot(lVec, lVec));
    // TODO: Add a checkd for back facing polygons - like humus demo? - dot product with tangent space normal in view space?
    //atten *= float(lVec.z > 0.0);

    // Calculate the light normal
    lightVec = normalize(lightVec);
    
    // TODO: store the light values and combine later? (do one "pow"?)
		hfloat diffuse = clamp01(dot(lightVec, bumpView));
		hfloat specular = pow(clamp01(dot(lightVec, reflVec)), 16.0);

		lighting += atten * lightColor * (diffuse * base + 0.6 * specular);
  }

  //gl_FragColor.rgb = lighting; gl_FragColor.a = 1.0;
  finalColor.rgb = lighting; finalColor.a = 1.0;
}