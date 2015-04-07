#version 140

uniform mat4 gluOrtho; //wilf: orthographic

void main()
{
  gl_Position = gluOrtho * gl_Vertex;
}