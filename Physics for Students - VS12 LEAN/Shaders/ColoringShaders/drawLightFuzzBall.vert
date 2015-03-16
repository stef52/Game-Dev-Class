#version 130

/////////////////////////////////////////////////////////////////////
// drawLightFuzzBall
// This shader program draws a fuzz ball via a circular texture...
/////////////////////////////////////////////////////////////////////

out vec2 texCoord;
	
attribute vec3 vertex;
attribute vec2 textureCoordinate;
attribute vec3 normal;

void main(){
	gl_Position = gl_ModelViewProjectionMatrix * vec4 (vertex, 1.0);  
	texCoord = textureCoordinate; //Pass through
	gl_FrontColor = gl_Color; //Pass through (not passing back color)
}