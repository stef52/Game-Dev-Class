#version 130

//WILF: A copy of ortho.vert

uniform mat4 gluOrtho; //wilf: orthographic

attribute vec3 vertex;
attribute vec2 textureCoordinate;
attribute vec3 normal;

void main()
{
  gl_Position = gluOrtho * vec4 (vertex, 1);
}