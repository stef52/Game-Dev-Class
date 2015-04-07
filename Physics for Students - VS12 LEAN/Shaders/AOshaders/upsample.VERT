#version 120

//WILF: A copy of ortho.vert

uniform mat4 gluOrtho; //wilf: orthographic

void main()
{
  gl_Position = gluOrtho * gl_Vertex;
}