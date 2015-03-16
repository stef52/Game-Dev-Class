#version 130

/////////////////////////////////////////////////////////////////////
// drawWorldWithAllLights
// This shader program takes a screen sized light index texture and
// draws lit geometry with it. 
/////////////////////////////////////////////////////////////////////

attribute vec3 vertex;
attribute vec2 textureCoordinate;
attribute vec3 normal;

out vec2 texCoord;
out vec4 clipSpace; 
out vec3 pixelPositionCS, pixelNormalCS, pixelToCameraCS;

//NOTE: The combinedColor routine in the library requires pixelNormal, pixelToLight, pixelToCamera
//in which all 3 are normalized and in camera space... Two of them are computed here (pixelToLight is computed
//in the pixel shader once the light position is known). Since the vertices are interpolated, they need to
//be re-normalized in the pixel shader so they don't need to be normalized here...

void main () {

	//Calculate the output position in clip space.
	gl_Position = clipSpace = gl_ModelViewProjectionMatrix * vec4 (vertex, 1.0); 

	//Pass-through the texture coordinates for the base texture.
	texCoord = textureCoordinate;
	
	pixelNormalCS = gl_NormalMatrix * normal; //Equivalent to gl_ModelViewMatrix for normals...
	pixelPositionCS = (gl_ModelViewMatrix * vec4 (vertex, 1)).xyz;
	pixelToCameraCS = normalize (/*vec3 (0,0,0)*/ - pixelPositionCS.xyz); //Normalize to make it more accurate in the pixel shader...
	//Light position is calculated in the pixel shader because there are four of them per pixel...
}
