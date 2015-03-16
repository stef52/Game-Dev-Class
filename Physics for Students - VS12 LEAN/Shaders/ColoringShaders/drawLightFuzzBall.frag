#version 130

/////////////////////////////////////////////////////////////////////
// drawLightFuzzBall
// This shader program draws a fuzz ball via a circular texture...
/////////////////////////////////////////////////////////////////////

uniform sampler2D Base;

in vec2 texCoord;
out vec4 finalColor; 

void main () {
	finalColor = texture2D (Base, texCoord) * gl_Color;
}
