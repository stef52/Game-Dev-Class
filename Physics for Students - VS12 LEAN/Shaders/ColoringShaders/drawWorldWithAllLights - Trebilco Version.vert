#version 130

#define projectSpace clipSpace

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

attribute vec2 textureCoord;
attribute vec3 tangent;
attribute vec3 binormal;
attribute vec3 normal;

uniform vec3 camPos;

out vec2 texCoord;
out vec3 vVec;
out vec4 projectSpace;
out vec3 vVecTangent;

out mat3 tangentToView;

void main(){

  // Calculate the output position and projection space lookup
  projectSpace = gl_ModelViewProjectionMatrix * gl_Vertex;  
  gl_Position = projectSpace;
  projectSpace.xy = (projectSpace.xy + vec2(projectSpace.w)) * 0.5;

  // Pass-through tex-coord
	texCoord = textureCoord;

  // Calculate the transform from tangent space to view space
  mat3 modelToTangent = mat3(tangent, binormal, normal);

  // Calculate the tangent space to view space matrix
  tangentToView = gl_NormalMatrix * modelToTangent;

  // Calculate the view vector in view space
  vVec = (gl_ModelViewMatrix * gl_Vertex).xyz;

  // Calculate the view vector in tangent space
	vec3 viewVec = camPos - gl_Vertex.xyz;
	vVecTangent.x = dot(viewVec, tangent);
	vVecTangent.y = dot(viewVec, binormal);
	vVecTangent.z = dot(viewVec, normal);
}