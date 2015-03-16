
//*****************************************************************************************//
//                                      Includes                                           //
//*****************************************************************************************//

#include "includes.all"

//*****************************************************************************************//
//                                       Object                                            //
//*****************************************************************************************//

EnvironmentObject::EnvironmentObject () : Object (), skyname (NULL) {
}

EnvironmentObject::EnvironmentObject (const char* typeName) : Object (typeName), skyname (NULL) {
}

EnvironmentObject::~EnvironmentObject () {
	if (skyname) {
		delete [] skyname;
	}
}

void EnvironmentObject::tick () {
	//This could make an object rotate or change size or do nothing.
}

void EnvironmentObject::draw () {
	if (!disableShaders) {defaultShader->activate ();}

	//Draw the faces in this object.
	glPushMatrix ();
		Transformation cameraInverse;

		// Grab the current camera transformation
		glGetMatrixf (cameraInverse);

		// Remove the translation of the camera
		glTranslated (-cameraInverse.m41, -cameraInverse.m42, -cameraInverse.m43);

		glPushMatrix ();
			// Move it up
			//glTranslated (0, 25, 0);
			glScalef (2000, 2000, 2000);
			drawSkyBox ();
		glPopMatrix ();
	glPopMatrix ();

	/*
	glPushMatrix ();
		//glColor3f (30, 200, 200); 
		glColor3f (1.0, 0.0, 0.0); 
		glTranslated (5, 5, 5);

		GLUquadric* quadric = gluNewQuadric ();
		//gluQuadricDrawStyle (quadric, GLU_FILL);
		//gluQuadricNormals (quadric, GL_SMOOTH);
		//gluQuadricOrientation (quadric, GLU_OUTSIDE);		
		//glutSolidSphere (3.0, 20, 20);
		//gluDeleteQuadric (quadric);
		glColor4d (1.0, 1.0, 1.0, 1.0); //White...
	glPopMatrix ();
	*/
}

void EnvironmentObject::drawSkyBox () {
	//Want a colour
	glColor3f (1.0f, 1.0f, 1.0f);

	//For the purposes of this trick we disable depth testing and writing...
	glDisable (GL_DEPTH_TEST); //Disable depth testing...
	glDepthMask (GL_FALSE); //Disable depth writing...

	//glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	//glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Top
	// Texture Coordinates		
	skyTextures [UP]->activate ();
	glPushMatrix ();		
		glRotated (90, 1, 0, 0);		
		glTranslated (0, 0 , -0.5);
		drawSide ();
	glPopMatrix ();

	// Front
	skyTextures [FRONT]->activate ();
	glPushMatrix ();
		glRotated (0, 0, 1, 0);
		glTranslated (0, 0, -0.5);
		
		drawSide ();
	glPopMatrix ();	

	// Left
	skyTextures [LEFT]->activate ();
	glPushMatrix ();
		glRotated (90, 0, 1, 0);
		glTranslated (0, 0 , -0.5);		
		drawSide ();
	glPopMatrix ();

	// Right
	skyTextures [RIGHT]->activate ();
	glPushMatrix ();
		glRotated (-90, 0, 1, 0);
		glTranslated (0, 0 , -0.5);
		drawSide ();
	glPopMatrix ();


	// Back
	skyTextures [BACK]->activate ();
	glPushMatrix ();
		glRotated (-180, 0, 1, 0);
		glTranslated (0, 0 , -0.5);
		drawSide ();
	glPopMatrix ();

	//// Bottom
	skyTextures [DOWN]->activate ();
	glPushMatrix ();	
		glRotated (-90, 1, 0, 0);		
		glTranslated (0, 0 , -0.5);
		drawSide ();
	glPopMatrix ();
	glEnable (GL_DEPTH_TEST); //Re-enable depth testing since that is the default...
	glDepthMask (GL_TRUE); //Re-enable depth writing since that is the default...
}

void EnvironmentObject::drawSide () {
	static double maxTexCoord = 1.0 - (0.5 * (1.0 / 256.0));
	static double minTextCoord = 0.0 + (0.5 * (1.0 / 256.0));
	glBegin (GL_QUADS);		
		//Recall: our shaders describe vertices with attributes 0, 1, 2 as "vertex", "textureCoordinate", "normal".
		//Note: the first parameter in the glVertexAttrib* function below is one of the indices 0, 1, or 2 above...

		glVertexAttrib2f (1, maxTexCoord, maxTexCoord); glVertexAttrib3f (0, 0.5, 0.5, 0);
		glVertexAttrib2f (1, minTextCoord, maxTexCoord); glVertexAttrib3f (0, -0.5f, 0.5, 0);
		glVertexAttrib2f (1, minTextCoord, minTextCoord); glVertexAttrib3f (0, -0.5, -0.5, 0);
		glVertexAttrib2f (1, maxTexCoord, minTextCoord); glVertexAttrib3f (0, 0.5, -0.5, 0);
	glEnd ();
}

void EnvironmentObject::log () {
	//The type (it's implicit... it will be different for subclasses)...
	::log ("\nEnvironmentObject");
}

void EnvironmentObject::import (::ifstream &input, World *world) {
	char line [256]; //Working variable...

	//Input the properties
	SKIP_TO_COLON;
	SKIP_TO_SEMICOLON; long propertiesSize = atoi (line); CLEAR_THE_LINE;
	for (long propertiesIndex = 0; propertiesIndex < propertiesSize; propertiesIndex++) {
		SKIP_TO_ENDLINE;
		// The properties of a regular object are not used
		char key [256]; char value [256]; value [0] = '\0';
		sscanf (line, " \"%[^\"]\" => \"%[^\"]\"", key, value);

		// Record the skyboxname
		if (stricmp (key, "skyboxname") == 0) {
			long size = strlen (value) + 1;
			skyname = new char [size]; 
			strncpy (skyname, value, size);			
			char textureName [255];
			char* names [] = { "-up" , "-down", "-back", "-left", "-right", "-front"};
			for (long i = 0; i < 6; i++) {
				strcpy (textureName, value);
				strcat (textureName, names [i]);				
				skyTextures [i] = world->textureManager.addTexture (textureName);
				::log ("\nSky texture %d is \"%s\".", skyTextures [i]->textureName);
			}
		}
	}
}