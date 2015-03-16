#version 130

uniform sampler2D texture;

in vec4 color;
in vec2 texCoord;

in vec4 ambient;
in vec4 diffuse; 
in vec4 specular; 
in float shininess; 

in vec3 pixelNormalCS;
in vec3 pixelToLightCS;
in vec3 toCameraCS;

out vec4 finalColor; 

#define USING_DEFAULT_SHADER
#include "WilfLightingLibrary.all"

void main (void) {
	vec4 textureColor = texture2D (texture, texCoord);
	vec3 pixelNormal = normalize (pixelNormalCS); //When a vector is interpolated, it's length changes...
	vec3 pixelToLight = normalize (pixelToLightCS); //When a vector is interpolated, it's length changes...
	vec3 pixelToCamera = normalize (toCameraCS); //When a vector is interpolated, it's length changes...
	finalColor = combinedColor (textureColor, pixelNormal, pixelToLight, pixelToCamera);
}
