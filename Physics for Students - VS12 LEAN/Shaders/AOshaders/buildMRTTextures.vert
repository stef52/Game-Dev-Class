#version 130

//varying out vec4 pos; //wilf
out vec4 pos;

attribute vec3 vertex;
attribute vec2 textureCoordinate;
attribute vec3 normal;

void main() {
  pos = gl_ModelViewMatrix * vec4 (vertex,1);
  //gl_Position = ftransform();
  gl_Position = gl_ModelViewProjectionMatrix * vec4 (vertex,1);
}