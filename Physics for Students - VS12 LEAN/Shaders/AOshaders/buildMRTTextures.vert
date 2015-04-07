#version 130

//varying out vec4 pos; //wilf
out vec4 pos;

void main() {
  pos = gl_ModelViewMatrix * gl_Vertex;
  //gl_Position = ftransform();
  gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}