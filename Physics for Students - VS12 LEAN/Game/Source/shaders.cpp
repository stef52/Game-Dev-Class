//*****************************************************************************************//
//                                      Includes                                           //
//*****************************************************************************************//

#include "includes.all"


//*****************************************************************************************//
//                                       Shaders                                           //
//*****************************************************************************************//

const char *defaultAttributes [] = {"vertex", "textureCoordinate", "normal"};
Shader *Shader::activeShader = NULL;

char *privateReadFile (const char *fileName) {
	//Returns the complete file as a string, otherwise NULL...

	//Attempt to open the file.
	if (fileName == NULL) return NULL; //In case caller flubbed...
	FILE *file = fopen (fileName, "r");
	if (file == NULL) {
		::halt ("\nFailed to find the file called \"%s\"...", fileName); 
		char shaderFileName [255] = "";
		_fullpath ((char *) &shaderFileName, fileName, sizeof (shaderFileName));
		::log ("\nFull path was \"%s\"...", shaderFileName);
		return NULL;
	}

	//Find out how long it is.
	fseek (file, 0, SEEK_END);
	long fileSize = ftell (file);
	fseek (file, 0, SEEK_SET);

	//Read it in to newly allocated space...
	char *memoryAddress = new char [fileSize+1]; //+1 for end of string '\0'.
	long bytesRead = fread (memoryAddress, 1, fileSize, file);
	memoryAddress [bytesRead] = '\0';
	fclose (file);
	return memoryAddress;
}

void logExpandedShaderWithLineNumbers (const char *name, char *text) {
	::log ("\n\nEXPANDED VERSION OF SHADER \"%s\"...", name);

	long counter = 0;
	//Loop as long as an end of line can be found...
	char *all = text, *line = "";
	for ( ; (line = strstr (all, "\n")) != NULL; all = line+1) {
		::log ("\n%4d: ", ++counter);
		for (char *next = all; next < line; next++) {::log ("%c", *next);}
	}
	//Deal with the last line (if any) that does not end with a "\n"...
	if (*all == '\0') return; //It did end with a "\n" which is why the character after it is the end of string character...
	::log ("\n%4d: %s\n", ++counter, all); //Add add the missing end of line...
}

//Wilf's global line insertion routine...
char *globalLineInsertion; char globalLineInsertionData [256];

//String globalLineInsertion contains macros that are inserted into the second line of all shaders; i.e., after #version...
//For example, it could define something like the following but is actually SLIGHTLY more complex...

//	char *globalLineInsertion = "#define EXPERIMENT EXPERIMENT0";

//The following is an example of how it could be set up... though it's not in use yet (and so is just "")...

/*
void buildGlobalLineInsertion () {
	strcpy (globalLineInsertion, 
		asString ("#define EXPERIMENT %d\n#define CAPTURE_WORLD_POSITION_CS %d", 
			EXPERIMENT, CAPTURE_WORLD_POSITION_CS));
	::log ("\nEXPERIMENT IS %d.", EXPERIMENT);
}
*/

char *globalPostVersionInsertion (char *text) {
	if (*globalLineInsertion == '\0') return text; //It's an empty string...
	//The first line should be of the following form. If not, complain...
	//	#version 130
	char *start = strstr (text, "#version");
	if (start != text) {
		halt ("\nShader in use that doesn't have '#version 130' or equivalent on first line...");
		return text;
	}
	
	//Look for the first end of line...
	char *version = text;
	char *afterVersion = strstr (text, "\n");

	//Terminate the first line by the end of string character and move afterVersion to the second line...
	*afterVersion = '\0'; afterVersion++; //So we can easily pick up the first line (which is now without "\n").

	const long firstLineLength = strlen (version) + 1; //+1 for missing "\n".
	const long insertionStringLength = strlen (globalLineInsertion) + 1; //+1 for an extra "\n" that we'll throw in...
	const long restOfTextLength = strlen (afterVersion);

	char *answer = new char [firstLineLength + insertionStringLength + restOfTextLength + 1];

	*answer = '\0'; //Start empty...
	strcat (answer, version); strcat (answer, "\n"); 
	strcat (answer, globalLineInsertion); strcat (answer, "\n"); 
	strcat (answer, afterVersion);
	delete [] text; return answer;
}

char *nextInclude (char *text) {
	const char *include = "#include \"";
	char *start = strstr (text, include);
	while (true) {
		if (start == NULL) return start;
		bool commentedOut = *(start-1) == '/' && *(start-2) == '/';
		if (!commentedOut) return start;
		
		//If commented out, skip past closing \" and search for new #include...
		char *name = start + strlen (include);
		char *end = strstr (name, "\"");
		char *after = end+1;
		start = strstr (after, include);
	}
}

//Wilf's #include preprocessor that can handle multiple includes...
char *includePreprocessor (char *text) {
	//Looks for one include statement of the form "#include "name" which is NOT preceded by "\\". 
	//If it does NOT exist, returns the text unchanged. Otherwise, loads the file "../shaders/name" 
	//and replaces the include by the contents of the file and then REPEATS the above analysis...

	while (true) {

		const char *include = "#include \"";
		char *start = nextInclude (text);
		if (start == NULL) return text; //There is no include file...

		char *name = start + strlen (include); //There was, so pick out the name...
		char *end = strstr (name, "\"");
		char *after = end+1;

		//Terminate the name by the end of string character and build the above file name...
		*end = '\0'; //So we can pick up everything up to the closing double quote.
		char fileName [256] = "../shaders/";
		strcat (fileName, name);

		//Read the text in and create the final concatenated text...
		char *includeText = privateReadFile (fileName);
		*start = '\0'; //So we can determine how long the string is before the include...

		const long prefixLength = strlen (text) + 1;
		const long includeLength = strlen (includeText) + 1;
		const long suffixLength = strlen (after);

		char *answer = new char [prefixLength + includeLength + suffixLength + 1];

		*answer = '\0'; //Start empty...
		strcat (answer, text); strcat (answer, "\n"); 
		strcat (answer, includeText); strcat (answer, "\n"); 
		strcat (answer, after);
		delete [] text; text = answer; //Then repeat...
	}
}

GLhandleARB Shader::privateLoadAndCompileShader (const char *fileName, GLenum type) {
	//Returns the shader handle for the compiled shader in the given file if everything works out; 0, otherwise...
	if (stricmp (name.c_str (), "drawWorldWithAllLights") == 0) {
		const bool test = true;
	}

	//Attempt to open the file.
	char *memoryAddress = privateReadFile (fileName); if (memoryAddress == NULL) return NULL; //Log already done...
	memoryAddress = globalPostVersionInsertion (memoryAddress);
	memoryAddress = includePreprocessor (memoryAddress); 

	//Obtain a handle and associate the text of the file with this handle...
	GLhandleARB shaderHandle = glCreateShaderObjectARB (type);
	glShaderSourceARB (shaderHandle, 1, (const GLcharARB **) &memoryAddress, 0);
	
	//Proceed to compile it.
	glCompileShaderARB (shaderHandle);

	//Find out if compilation worked and return the handle if it did...
	int status; glGetObjectParameterivARB (shaderHandle, GL_OBJECT_COMPILE_STATUS_ARB, &status);
	if (status != 0) {delete [] memoryAddress; return shaderHandle;} //Everything OK...

	//It didn't, so log error information...
	prompt ("SHADER \"%s\" did not compile (look at log)", name.c_str ());
	::log ("\nFailed to compile %s shader \"%s\"...", type == GL_VERTEX_SHADER_ARB ? "VERTEX" : "PIXEL", name.c_str ());
	int length = 0; glGetObjectParameterivARB (shaderHandle, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
	const long MAXIMUM_LOG_STRING = 1024; char logString [MAXIMUM_LOG_STRING];
	GLsizei messageLength = minimum (length, MAXIMUM_LOG_STRING);
	if (messageLength > 0) {
		glGetInfoLogARB (shaderHandle, messageLength, 0, logString);
		::log ("\n\tREASON: %s", logString);
	}

	//and discard the handle that was obtained...
	glDeleteObjectARB (shaderHandle); 
	logExpandedShaderWithLineNumbers (fileName, memoryAddress); //WILF LOG SHADER
	delete [] memoryAddress;
	return 0;
}

GLhandleARB Shader::privateLinkShaders (GLhandleARB *shaderHandles, const long shaderHandlesSize, const char *attributes [], long attributesSize) {
	//Links the shaders that were successfully loaded and compiled...

	//Start with a handle for the shader program...
	GLhandleARB shaderProgramHandle = glCreateProgramObjectARB ();

	//Specify the shader vertex attributes. By default, attributes 0, 1, 2 are named "vertex", "textureCoordinate", "normal".
	for (long index = 0; index < attributesSize; index++) {
		glBindAttribLocation (shaderProgramHandle, index, attributes [index]);
	}

	//Attach the shaders to the shader program... and link them to the shader program.
	for (long index = 0; index < shaderHandlesSize; index++) {
		glAttachObjectARB (shaderProgramHandle, shaderHandles [index]);
	}

	glLinkProgramARB (shaderProgramHandle);

	//Find out if compilation worked and return the porogram handle if it did...
	int status; glGetObjectParameterivARB (shaderProgramHandle, GL_OBJECT_LINK_STATUS_ARB, &status);
	if (status != 0) return shaderProgramHandle; //Everything OK...

	//It didn't, so log error information...
	prompt ("SHADER \"%s\" did not link (look at log)", name.c_str ());
	::log ("\nFailed to link shader \"%s\"...", name.c_str ());
	int length = 0; 
	glGetObjectParameterivARB (shaderProgramHandle, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);

	const long MAXIMUM_LOG_STRING = 1024; char logString [MAXIMUM_LOG_STRING];
	GLsizei messageLength = minimum (length, MAXIMUM_LOG_STRING);
	if (messageLength > 0) {
		glGetInfoLogARB (shaderProgramHandle, messageLength, 0, logString);
		::log ("\n\tREASON: %s", logString);
	}

	//and detach what was previously attached and discard the program handle that was obtained...
	for (long index = 0; index < shaderHandlesSize; index++) {
		glDetachObjectARB (shaderProgramHandle, shaderHandles [index]);
	}
	glDeleteObjectARB (shaderProgramHandle); //Should also detach the attached handles...
	return 0;
}

void Shader::load (const char *attributes [], long attributesSize) {
	//Private vertex and fragment shader handles are created by loading and compiling the respective shaders
	//and then a shader program handle is created and stored by linking those two private ones. Only the
	//program handle is remembered... If anything goes wrong, isBroken is set to false...
	
	if (isBroken) return; //Once broken, always broken...

	//Build path relative names for the corresponding vertex and fragment shaders.
	const char *name = this->name.c_str ();

	const std::string pathRelativeDirectory = "..\\Shaders\\"; 
	const std::string vert = ".vert"; 
	const std::string frag = ".frag";
	const std::string fullVertexShaderName = pathRelativeDirectory + this->name + vert;
	const std::string fullFragmentShaderName = pathRelativeDirectory + this->name + frag;

	const char *vertexShaderName = fullVertexShaderName.c_str ();
	const char *fragmentShaderName = fullFragmentShaderName.c_str ();

	//Compile and link the shaders leaving it to the called routines to log error messages. 
	GLhandleARB shaderHandles [2]; GLhandleARB handle;

	//Load and compile the vertex shader...
	handle = privateLoadAndCompileShader (vertexShaderName, GL_VERTEX_SHADER_ARB);
	if (handle == 0) {isBroken = true; return;}
	shaderHandles [0] = handle;

	//Load and compile the fragment shader...
	handle = privateLoadAndCompileShader (fragmentShaderName, GL_FRAGMENT_SHADER_ARB);
	if (handle == 0) {isBroken = true; return;}
	shaderHandles [1] = handle;

	//Link the shaders into one shader program.
	handle = privateLinkShaders (shaderHandles, 2, attributes, attributesSize);
	if (handle == 0) {isBroken = true; return;}

	//Record the program handle for future use...
	privateProgramHandle = handle;

	::log ("\nCreate shader program %d for \"%s\" and \"%s\".", handle, vertexShaderName, fragmentShaderName);
	isLoaded = true;
}

void Shader::unload () {
	if (isBroken || isUnloaded) return;

	//Detach and delete each shader from the program shader and also delete the program shader...
	GLsizei size; GLuint shaders [2];
	glGetAttachedShaders (privateProgramHandle, 2, &size, shaders);

	for (long index = 0; index < size; index++) {
		glDetachShader (privateProgramHandle, shaders [index]);
		glDeleteShader (shaders [index]);
	}

	glDeleteProgram (privateProgramHandle);
	isUnloaded = true;
}

void Shader::activate () {
//	if (disableShaders) return;

	if (activeShader == this) return; //Already active...
	if (isBroken) {activeShader = NULL; glUseProgramObjectARB (0); return;} //deactivate (); return;}
	if (!isLoaded) halt ("\nYou forgot to load shader \"%s\"...", name.c_str ());
	glUseProgramObjectARB (privateProgramHandle);
	setUniforms (); //Which also records the active shader...
}

void Shader::setUniform1f (const std::string &variable, float value) {
	if (isBroken) {return;}
	glUniform1f (glGetUniformLocation (privateProgramHandle, variable.c_str ()), value);
}
void Shader::setUniform3f (const std::string &variable, float value0, float value1, float value2) {
	if (isBroken) {return;}
	glUniform3f (glGetUniformLocation (privateProgramHandle, variable.c_str ()), value0, value1, value2);
}
void Shader::setUniform4f (const std::string &variable, float value0, float value1, float value2, float value3) {
	if (isBroken) {return;}
	glUniform4f (glGetUniformLocation (privateProgramHandle, variable.c_str ()), value0, value1, value2, value3);
}

void Shader::setUniform1i (const std::string &variable, long value) {
	if (isBroken) {return;}
	glUniform1i (glGetUniformLocation (privateProgramHandle, variable.c_str ()), value);
}

void Shader::setUniformTexture (const std::string &variable, long textureUnit) {
	if (isBroken) {return;}
	glUniform1iARB (glGetUniformLocationARB (privateProgramHandle, variable.c_str ()), textureUnit);
}

void Shader::setUniformMatrix4fv (const std::string &variable, float *matrix) {
	if (isBroken) {return;}
	const long matrixCount = 1; const bool shouldTranspose = GL_FALSE;
	glUniformMatrix4fvARB (glGetUniformLocationARB (privateProgramHandle, variable.c_str ()), matrixCount, shouldTranspose, matrix);
}

void Shader::setUniforms () {
	if (activeShader == this) return; //Already set...
	activeShader = this;

	switch (uniformSetter) {
		case NoUniforms:
			break;
		
		case DefaultShaderUniforms: {

			//Materials
		//	setUniform4f ("material_ambient", 0.2f, 0.2f, 0.2f, 1.0f);
			setUniform4f ("material_ambient", 0.4f, 0.4f, 0.4f, 1.0f); //WILF: Excessive amount so we can see since there are no interior lights...
			setUniform4f ("material_diffuse", 1.0f, 1.0f, 1.0f, 1.0f);
			setUniform4f ("material_specular", 1.0f, 1.0f, 1.0f, 1.0f);
			setUniform1f ("material_shininess", 32.0f);

			//One light revolving around like the sun (not using yAngle or zAngle)...
			float angle = asRadians (shaderManager->xAngle);
			setUniform4f ("light0.position", cosf (angle), 0.707, sinf (angle), 0.0);
			setUniform4f ("light0.diffuse", 1.0f, 1.0f, 1.0f, 1.0f);
			setUniform4f ("light0.specular", 0.9f, 0.9f, 0.9f, 1.0f);
			setUniform4f ("light0.ambient", 0.2f, 0.2f, 0.2f, 1.0f);

			//Obsolete information (kept to show how it might be passed)
			//setUniformMatrix4fv ("camera_matrix", (float *) &camera->cameraMatrix);
			//setUniformMatrix4fv ("inverse_camera_matrix", (float *) &camera->inverseCameraMatrix); 

			//Transformation modelView; glGetMatrixf (GL_MODELVIEW, modelView); 
			//setUniformMatrix4fv ("modelView", (float *) &modelView);

			//static float runningTime = 0.0; runningTime += DT;
			//setUniform1f ("time", runningTime); 
			break;}
	}
}

void Shader::reset () {
	activeShader = NULL;
}

void Shader::example () {
	//This example is conceptual; i.e., you can't actually run it...
	return; //Just in case you try...
	
	//Shader *shader = ...
	//shader->activate ();

	//Associate texture sampler "reflection" with the texture activated in texture unit 0.
	//i.e., the one you activated after executing   	glActiveTexture (GL_TEXTURE0);      //See below.
	//shader->setUniformTexture ("reflection", 0); 
	//shader->setUniformTexture ("refraction", 1); //Same but set up via glActiveTexture (GL_TEXTURE1); 
	//shader->setUniformTexture ("noise", 2);
	//
	////Assign 1 (true) into bool (or int) variable "adjustHeight". Similar for float variable "heightScale" 
	//shader->setUniform1i ("adjustHeight", 1);
	//shader->setUniform1f ("heightScale", 2.0);
	//
	//float transformation [16];
	//glPushMatrix ();
	//	glLoadIdentity ();
	//	glTranslatef (0.0,0.68,-10.0);
	//	glScalef (0.08,0.08,0.08);
	//	glGetFloatv (GL_MODELVIEW_MATRIX, transformation);
	//	shader->setUniformMatrix4fv ("matrix", transformation); //Assign entire transformation...
	//glPopMatrix ();

	//glActiveTexture (GL_TEXTURE0); //reflectionTexture->activate (); //Commented out since texture
	//glActiveTexture (GL_TEXTURE1); //refractionTexture->activate (); //variables not defined...
	//glActiveTexture (GL_TEXTURE2); //noiseTexture->activate ();

	//shader->deactivate ();
}

void Shader::setup () {
	extern char globalLineInsertionData [256]; extern char *globalLineInsertion; //Initialized below...
	strcpy (globalLineInsertionData, ""); //Empty string initially.
	globalLineInsertion = &globalLineInsertionData [0];
}
void Shader::wrapup () {}