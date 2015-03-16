#version 130

uniform vec4 material_ambient;
uniform vec4 material_diffuse;
uniform vec4 material_specular;
uniform float material_shininess;

struct light {
	vec4 position;
	vec4 diffuse;
	vec4 specular;
	vec4 ambient;
};

uniform light light0;

attribute vec3 vertex;
attribute vec2 textureCoordinate;
attribute vec3 normal;

out vec4 color;
out vec2 texCoord;

out vec4 ambient;
out vec4 diffuse; 
out vec4 specular; 
out float shininess; 

out vec3 pixelNormalCS;
out vec3 pixelToLightCS;
out vec3 toCameraCS;

//NOTE: If you want a vector from A to B, it's B - A.

void main () {
	ambient = max (material_ambient, light0.ambient);
	diffuse = material_diffuse * light0.diffuse;
	specular = material_specular * light0.specular;
	shininess = material_shininess;

	pixelNormalCS = gl_NormalMatrix * normal;
	vec4 pixelPositionCS = gl_ModelViewMatrix * vec4 (vertex, 1);
	vec4 lightPositionCS = gl_ModelViewMatrix * light0.position; 
	toCameraCS = /*vec3 (0,0,0)*/ - pixelPositionCS.xyz;
	//If a light's ".w" is 0, it's a direction rather than a position...
	pixelToLightCS = light0.position.w < 0.01 ? lightPositionCS.xyz /*-0*/: (lightPositionCS - pixelPositionCS).xyz; 
	
	texCoord = textureCoordinate; //Pass through...
	gl_Position = gl_ModelViewProjectionMatrix * vec4 (vertex, 1);	
}
