#version 130

/////////////////////////////////////////////////////////////////////
// drawZPrepass
// This shader program draws nothing in the color buffer but sets up the z-buffer...
/////////////////////////////////////////////////////////////////////

attribute vec3 vertex;
attribute vec2 textureCoordinate;
attribute vec3 normal;

void main(){
	gl_Position = gl_ModelViewProjectionMatrix * vec4 (vertex, 1.0);
}
