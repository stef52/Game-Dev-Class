
//*****************************************************************************************//
//                                      Includes                                           //
//*****************************************************************************************//

#include "includes.all"

//*****************************************************************************************//
//                                       Object                                            //
//*****************************************************************************************//

Object::Object () : type (NULL), faces (NULL), facesSize (0) {
}

Object::Object (const char* typeName) : faces (NULL), facesSize (0) {
	if (strlen (typeName) >= 1) {
		type = new char [strlen (typeName) + 1];
	} else {
		type = new char [1];		
	}
	strcpy (type, typeName);
}

Object::~Object () {
	// Not certain why this is causing a heap violation
	if (type != NULL) {
		delete [] type;
		type = NULL;
	}

	for (long i = 0; i < facesSize; i++) {		
		delete faces [i];
	}
	delete [] faces;
}

void Object::tick () {
	//This could make an object rotate or change size or do nothing.
}

void Object::draw () {
	if (!disableShaders) shader->activate ();
	glPushMatrix ();
		glMultMatrixf (transformation.normal ());
		//Draw the faces in this object.
		for (long faceIndex = 0; faceIndex < facesSize; faceIndex++) {
			Face &face = *(faces [faceIndex]);
			face.draw ();
		}
	glPopMatrix ();
}

void Object::log () {
	//The type (it's implicit... it will be different for subclasses)...
	::log ("\nObject");

	//The faces...	
	for (long faceIndex = 0; faceIndex < facesSize; faceIndex++) {
		Face &face = *(faces [faceIndex]);
		::log ("\n\tFace %d:", faceIndex);
	  face.log ();
	}
}

void Object::import (::ifstream &input, World *world) {
	char line [256]; //Working variable...

	//Input the transformation.
	SKIP_TO_COLON; CLEAR_THE_LINE;

	//The standard transformation matrix elements...
	Transformation &normal = transformation.normal ();
	SKIP_TO_COMMA; normal.m11 = atof (line);
	SKIP_TO_COMMA; normal.m12 = atof (line);
	SKIP_TO_COMMA; normal.m13 = atof (line);
	SKIP_TO_COMMA; normal.m14 = atof (line);
	
	SKIP_TO_COMMA; normal.m21 = atof (line);
	SKIP_TO_COMMA; normal.m22 = atof (line);
	SKIP_TO_COMMA; normal.m23 = atof (line);
	SKIP_TO_COMMA; normal.m24 = atof (line);
	
	SKIP_TO_COMMA; normal.m31 = atof (line);
	SKIP_TO_COMMA; normal.m32 = atof (line);
	SKIP_TO_COMMA; normal.m33 = atof (line);
	SKIP_TO_COMMA; normal.m34 = atof (line);
	
	SKIP_TO_COMMA; normal.m41 = atof (line);
	SKIP_TO_COMMA; normal.m42 = atof (line);
	SKIP_TO_COMMA; normal.m43 = atof (line);
	SKIP_TO_SEMICOLON; normal.m44 = atof (line);
	CLEAR_THE_LINE;

	//The inverse transformation matrix elements...
	Transformation &inverse = transformation.inverse;
	SKIP_TO_COMMA; inverse.m11 = atof (line);
	SKIP_TO_COMMA; inverse.m12 = atof (line);
	SKIP_TO_COMMA; inverse.m13 = atof (line);
	SKIP_TO_COMMA; inverse.m14 = atof (line);
	
	SKIP_TO_COMMA; inverse.m21 = atof (line);
	SKIP_TO_COMMA; inverse.m22 = atof (line);
	SKIP_TO_COMMA; inverse.m23 = atof (line);
	SKIP_TO_COMMA; inverse.m24 = atof (line);
	
	SKIP_TO_COMMA; inverse.m31 = atof (line);
	SKIP_TO_COMMA; inverse.m32 = atof (line);
	SKIP_TO_COMMA; inverse.m33 = atof (line);
	SKIP_TO_COMMA; inverse.m34 = atof (line);
	
	SKIP_TO_COMMA; inverse.m41 = atof (line);
	SKIP_TO_COMMA; inverse.m42 = atof (line);
	SKIP_TO_COMMA; inverse.m43 = atof (line);
	SKIP_TO_SEMICOLON; inverse.m44 = atof (line);
	CLEAR_THE_LINE;

	//Input the properties
	SKIP_TO_COLON;
	SKIP_TO_SEMICOLON; long propertiesSize = atoi (line); CLEAR_THE_LINE;
	long shaderIndex = -1;
	for (long propertiesIndex = 0; propertiesIndex < propertiesSize; propertiesIndex++) {
		SKIP_TO_ENDLINE;
		char key [256]; char value [256]; value [0] = '\0';
		sscanf (line, " \"%[^\"]\" => \"%[^\"]\"", key, value);

		// Only a shader is applied
		if (stricmp (key, "shader") == 0) {
			sscanf (value, "%d", &shaderIndex);
		}
	}
	shader = shaderManager->importShaderFor (shaderIndex);

	//Input the faces.
	SKIP_TO_COLON;
	SKIP_TO_SEMICOLON; facesSize = atoi (line);
	faces = new Face* [facesSize];
	for (long faceIndex = 0; faceIndex < facesSize; faceIndex++) {
		Face *face = new Face;
		face->import (input, world);
		faces [faceIndex] = face;		
	}
}