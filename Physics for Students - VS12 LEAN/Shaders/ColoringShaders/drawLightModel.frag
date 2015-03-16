#version 130
/////////////////////////////////////////////////////////////////////
// drawLightModel
// This shader program positions a light volume in the scene and 
// outputs the light's colour
/////////////////////////////////////////////////////////////////////

#if (0 && CAPTURE_WORLD_POSITION_CS)
	//You need to deal with this case... here... Once you do, remove "0 &" in the #if...

#else

	uniform vec4 lightColor;

	out vec4 finalColor;  

	void main(){											
		finalColor = lightColor;
	}
#endif //CAPTURE_WORLD_POSITION_CS
