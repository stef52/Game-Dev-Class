#version 130

/////////////////////////////////////////////////////////////////////
// drawLightModel
// This shader program positions a light volume in the scene and 
// outputs the light's colour
/////////////////////////////////////////////////////////////////////

uniform vec3 lightPosition;
uniform float lightScale;

attribute vec3 vertex;
attribute vec2 textureCoordinate;
attribute vec3 normal;

//Note that only the vertex attribute is used since the color is predetermined 
//by uniform "lightColor" in the vertex shader.

#if (CAPTURE_WORLD_POSITION_CS)
	//NOT YET DONE
#endif //CAPTURE_WORLD_POSITION_CS
	
void main (){

	//Position the light sphere in the scene...
	//Scale the vertex by lightScale and translate by lightPosition.
	
	//Note: This is exactly how the original shader does it...
	vec3 worldPosition = vertex * lightScale + lightPosition;
	vec4 clipPosition = gl_ModelViewProjectionMatrix * vec4 (worldPosition, 1.0);

	// Check if sphere will be clipped, adjust
	if (-clipPosition.z > clipPosition.w) { 
		clipPosition.w = 1;
		clipPosition.z = -1;
	}

	#if (CAPTURE_WORLD_POSITION_CS)
		//NOT YET DONE...
	#endif //CAPTURE_WORLD_POSITION_CS

	gl_Position = clipPosition;
}