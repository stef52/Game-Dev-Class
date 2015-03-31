#version 130
//
// Vertex shader 

out vec3  Normal;
out vec3  EyeDir;

attribute vec3 vertex;
attribute vec2 textureCoordinate;
attribute vec3 normal;

out vec2 texNormal0Coord;
out vec2 texColorCoord;
out vec2 texFlowCoord;

uniform float osg_FrameTime;
uniform mat4 osg_ViewMatrixInverse;
out float myTime;

void main(void)
{
	
    gl_Position = gl_ModelViewProjectionMatrix * vec4 (vertex, 1);
    Normal         = normalize(gl_NormalMatrix * normal);
  
    vec4 pos       = gl_ModelViewMatrix * vec4(vertex, 1);
    EyeDir         = vec3(osg_ViewMatrixInverse*vec4(pos.xyz,0));;

    texNormal0Coord   = textureCoordinate;
    texColorCoord = textureCoordinate;
    texFlowCoord = textureCoordinate;

    myTime = 0.01 * osg_FrameTime;
}