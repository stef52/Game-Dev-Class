#version 130

attribute vec3 vertex;
attribute vec2 textureCoordinate;
attribute vec3 normal;

void main () {
	gl_Position = gl_ModelViewProjectionMatrix * vec4 (vertex, 1.0); 
}


