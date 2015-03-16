//*****************************************************************************************//
//                                      Includes                                           //
//*****************************************************************************************//

#include "includes.all"

//*****************************************************************************************//
//                                    ShaderManager                                        //
//*****************************************************************************************//

ShaderManager *shaderManager = NULL;
Shader *defaultShader = NULL;

void ShaderManager::setup () { 	
	shaderManager = new ShaderManager ();
}		

ShaderManager::~ShaderManager () {
	removeShaders ();
}

void ShaderManager::removeShaders () {
	loopVector (shaderIndex, shaders)
		Shader *shader = shaders [shaderIndex];
		shader->unload (); delete shader;
	endloop
	shaders.clear ();
}

Shader *ShaderManager::addShader (const char *name, ShaderUniformSetterSwitch setter) {
	const bool logging = true;
	Shader* shader = new Shader (name, setter); 
	shader->load ();
	shaders.push_back (shader);
	if (logging) ::log ("\nShaderManager::addShader (\"%s\") at index %d.", name, shaders.size () - 1); 
	return shader;
}

void ShaderManager::tick () {
	xAngle += xRotateRate * (float) DT;
	yAngle += yRotateRate * (float) DT;
	zAngle += zRotateRate * (float) DT;

	xAngle = fmodf (xAngle, 360.0f); 
	yAngle = fmodf (yAngle, 360.0f); 
	zAngle = fmodf (zAngle, 360.0f); 
}

Shader *ShaderManager::importShaderFor (long shaderIndex) {
	if (shaderIndex == -1) return defaultShader;
	if (shaderIndex < 0 || shaderIndex >= shaders.size ()) halt ("\nShader index %d out of bounds; valid bounds 0..%d.", shaderIndex, shaders.size ());
	return shaders [shaderIndex];
}

void ShaderManager::import (::ifstream &input, World *world) {
	char line [256]; //Working variable...

	//Grab the shaders...
	SKIP_TO_COLON;
	SKIP_TO_SEMICOLON; long shadersSize = atoi (line);
	CLEAR_THE_LINE;

	//Delete old shaders...
	if (!shaders.empty ()) {removeShaders ();}

	//Add the new shaders...
	for (long i = 0; i < shadersSize; i++) {
		//Grab shader name...
		SKIP_TO_ENDLINE;
		addShader (line); //There is currently no data for initializing the uniform setter (so it defaults to NoUniforms).
	}

	defaultShader = addShader ("defaultLighting", DefaultShaderUniforms);
	defaultShader->activate ();
	defaultShader->setUniformTexture ("texture", 0); //All world objects activate the texture to draw with on unit 0.
}
