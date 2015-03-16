//*****************************************************************************************//
//                                      Includes                                           //
//*****************************************************************************************//

#include "includes.all"

//*****************************************************************************************//
//                                Light Indexed Rendering                                  //
//*****************************************************************************************//

//Facilities NOT CONTROLLED by EXPERIMENT...
#define RESOLUTION 800 // the final output resolution

enum ROTATION_OPTION {ROTATE_NORMALLY, ROTATE_SLOWLY}; 
ROTATION_OPTION rotationOption = ROTATE_SLOWLY;

const bool SHOULD_SORT_LIGHTS = true;

//-----------------------------------------------------------------------------------------//

#define LIGHT_MODEL unitSolidSphere

//-----------------------------------------------------------------------------------------//

//String globalLineInsertion contains macros that are inserted into the second line of all shaders; i.e., after #version...
//Here, we define macros EXPERIMENT and CAPTURE_WORLD_POSITION_CS...

extern char globalLineInsertionData [256]; //Initialized below... but defined in shaders.cpp

void buildGlobalLineInsertion () {
	#define SAMPLE_JUNK 3
	char *globalLineInsertion = &globalLineInsertionData [0];
	strcpy (globalLineInsertion, asString ("#define EXPERIMENT %d\n#define CAPTURE_WORLD_POSITION_CS %d", 
		EXPERIMENT, CAPTURE_WORLD_POSITION_CS));
}
//-----------------------------------------------------------------------------------------//

//*****************************************************************************************//
//                        Handles for frame, color, and depth buffers                      //
//*****************************************************************************************//

GLuint lightIndexedFrameBufferID = 0;
GLuint lightIndexedColorBufferID = 0;
GLuint lightIndexedDepthBufferID = 0;
#if (CAPTURE_WORLD_POSITION_CS)
	GLuint worldPositionBufferID = 0;
#endif //CAPTURE_WORLD_POSITION_CS

//*****************************************************************************************//
//         Shader variables, light/position textures for 256 lights, and light data.       //
//*****************************************************************************************//

Shader *drawLightModelShader = NULL;
Shader *drawWorldWithAllLightsShader = NULL;
Shader *drawLightFuzzBallShader = NULL;
Shader *drawZPrepassShader = NULL;
Shader *verificationShader = NULL;
#if (CAPTURE_WORLD_POSITION_CS)
Texture *worldPositionTexture = NULL;
Shader *drawPositionAndZPrepassShader = NULL;
#endif //CAPTURE_WORLD_POSITION_CS

Texture *allLightColors = NULL, *allLightPositions = NULL, *fuzzBall = NULL;

struct LightData {
	bool enabled; float r, g, b; float x, y, z, scale; Point cameraSpacePosition;
};

Point lightRotationCenter = Point (-2.47, -3.66, 13.92); 

#define MAXIMUM_LIGHTS 256
LightData lightData [MAXIMUM_LIGHTS] = {
	#include "lightData.h"
};

void scaledDownLightSizes () {
	//From approximately 4 meters down to 1...
	for (long light1 = 0; light1 < MAXIMUM_LIGHTS; light1++) {
		LightData &data1 = lightData [light1]; 
		data1.scale *= 0.25; //Note that largest 4 meter light scales down to 1 meter...
	}
}

void dontBunchUpLights () {
	const bool logging = false;
	//Scale down and move lights away from center if more than 4 are bunched up...

	//Note that artifacts occurs even when lights are all further apart than radius r... 
	//So introduce an extra scale factor...
	float extraScale = sqrt (2.0);

	float maximumRadius = 0.0;
	for (long light1 = 0; light1 < MAXIMUM_LIGHTS; light1++) {
		LightData &data1 = lightData [light1]; 
		if (!data1.enabled) continue;
		maximumRadius = maximum (maximumRadius, data1.scale * 0.5); //A unit sphere has a radius of 0.5..
	}

	float justBigger = maximumRadius * 1.05 * extraScale; justBigger *= justBigger; //Need squared values below...
	float bigger = maximumRadius * 1.1 * extraScale; bigger *= bigger; //Need squared values below...

	if (logging) ::log ("\nMOVE AWAY PASS... max radius %3.2f, move until %3.2f away.", maximumRadius, sqrt (bigger));
	for (long light1 = 0; light1 < MAXIMUM_LIGHTS; light1++) {
		LightData &data1 = lightData [light1]; Point light1Position = Point (data1.x, data1.y, data1.z);
		if (!data1.enabled) continue;

		//Make a pass to see how many lights are affected by this light (use distance .9)...
		long counter = 0;
		//for (long light2 = 0; light2 < light1; light2++) {
		for (long light2 = 0; light2 < MAXIMUM_LIGHTS; light2++) {
			if (light2 == light1) continue;
			LightData &data2 = lightData [light2]; 
			if (!data2.enabled) continue;
			Point light2Position = Point (data2.x, data2.y, data2.z);
			if (light1Position.squaredDistanceTo (light2Position) < justBigger) counter++;
		}

		//Make sure this light is not too close to all previous lights...
		float totalMovement = 0.0;
		if (counter > 3)
			while (counter > 1) {
				//It's too close... Move until it's all alone... 
				light1Position += (light1Position - lightRotationCenter).normalized () * 0.2; //Move out .2 meters..
				totalMovement += 0.2;
				data1.x = light1Position.x; data1.y = light1Position.y; data1.z = light1Position.z; //Modify permanently...
				
				//Count again... but use distance 1.1 to ensure it's far enough from all previous lights...
				counter = 0;
				//for (long light2 = 0; light2 < light1; light2++) {
				for (long light2 = 0; light2 < MAXIMUM_LIGHTS; light2++) {
					if (light2 == light1) continue;
					LightData &data2 = lightData [light2]; 
					if (!data2.enabled) continue;
					Point light2Position = Point (data2.x, data2.y, data2.z);
					if (light1Position.squaredDistanceTo (light2Position) < bigger) counter++;
				}
			}

		if (logging) 
			if (totalMovement > 0.0) 
				::log ("\nMove light %d by %3.2f meters.", light1, totalMovement);
			//else 
			//	::log ("\nLIGHT %d NOT MOVED.", light1);
	}

	//Don't trust the routine above actually working... So let's log the results.
	if (logging) ::log ("\n\nBUNCH UP RESULTS... How many other lights are closer than %3.2f meters.", maximumRadius);
	long maximumCounter = 0; long maximumProblemLights = 0;
	for (long light1 = 0; light1 < MAXIMUM_LIGHTS; light1++) {
		LightData &data1 = lightData [light1]; 
		if (!data1.enabled) continue;
		Point light1Position = Point (data1.x, data1.y, data1.z);

		//Make a pass to see how many lights are affected by this light (use distance 1.0)...
		long counter = 0;
		for (long light2 = 0; light2 < MAXIMUM_LIGHTS; light2++) {
			if (light2 == light1) continue;
			LightData &data2 = lightData [light2]; 
			if (!data2.enabled) continue;
			Point light2Position = Point (data2.x, data2.y, data2.z);
			if (light1Position.squaredDistanceTo (light2Position) < (maximumRadius*1.0)*(maximumRadius*1.0)) counter++;
		}
		maximumCounter = maximum (maximumCounter, counter);
		if (counter > 4) {maximumProblemLights++; data1.enabled = false;} //Count but also disable...
		if (logging && counter > 4) ::log ("\nLight %d close to %d lights.", light1, counter, counter > 4 ? " DISABLED SINCE TOO MANY" : "");
	}
	if (logging) ::log ("\n%d lights are a PROBLEM and SOME LIGHT HAS %d neighbours closer than %3.2f meters.", 
		maximumProblemLights, maximumCounter, maximumRadius);
}

void prepareLightData () {
	//One shot adjustment of light data; e.g., to change the size or move them away from each other...
	if (SCALE_DOWN_LIGHT_SIZES) scaledDownLightSizes ();
	if (DONT_BUNCH_UP) dontBunchUpLights ();
}

void logLights () {
	//Useful if you want to programmatically change the values in some way...
	//After you log, grab the logged values and place them back in the lightData.h file...
	//Not currently in use.... 

	::log ("\n\nLight data (%d entries)", MAXIMUM_LIGHTS); 

	float minX, maxX, minY, maxY, minZ, maxZ, minScale, maxScale;
	for (long index = 0; index < MAXIMUM_LIGHTS; index++) {
		LightData &data = lightData [index];
		if (index == 0) {
			//Ignore first entry which is now all zeros...
		} else {
			if (index == 1) {
				minX = maxX = data.x; minY = maxY = data.y; minZ = maxZ = data.z; minScale = maxScale = data.scale;
			} else {
				minX = minimum (minX, data.x); maxX = maximum (maxX, data.x); 
				minY = minimum (minY, data.y); maxY = maximum (maxY, data.y); 
				minZ = minimum (minZ, data.z); maxZ = maximum (maxZ, data.z); 
				minScale = minimum (minScale, data.scale); maxScale = maximum (maxScale, data.scale); 
			}
		}
		::log ("\n   {%3.2f,%3.2f,%3.2f,  %3.5f,%3.5f,%3.5f,  %3.2f},", 
			data.r, data.g, data.b, data.x, data.y, data.z, data.scale);
	}
	::log ("\n\nmin/max x, y, z, scale %3.5f,%3.5f,  %3.5f,%3.5f,  %3.5f,%3.5f,  %3.5f,%3.5f\n", minX, maxX, minY, maxY, minZ, maxZ, minScale, maxScale);
}

void updateLightCameraSpacePosition () {
	camera->beginCamera ();
		//Having set up the camera, we can now get the model view matrix from the stack...
		Transformation modelViewMatrix; glGetMatrixf (modelViewMatrix);

		for (long index = 0; index < MAXIMUM_LIGHTS; index++) {
			LightData &data = lightData [index];
			data.cameraSpacePosition = Point (data.x, data.y, data.z) * modelViewMatrix;
		}
	camera->endCamera ();
}

int isInBackToFrontOrderInCameraSpace (const void *a, const void *b) {
	//Return that it's in order if camera z of a is more negative than that of b; i.e., far is most negative.
	return ((LightData *) a)->cameraSpacePosition.z <= ((LightData *) b)->cameraSpacePosition.z;
}

void sortLights () {
	//Note that we sort everything except the first (light index 0) entry...
	qsort (&lightData [1], 255, sizeof (LightData), isInBackToFrontOrderInCameraSpace);
}

void uploadLights () {
	//Upload the color "as is" and upload the position in camera space (the radius/scale is unchanged)...

	Pixel *color = (Pixel *) allLightColors->bytes;
	for (long index = 0; index < MAXIMUM_LIGHTS; index++) {
		LightData &data = lightData [index];
		color->r = floor (data.r * 255.0);
		color->g = floor (data.g * 255.0);
		color->b = floor (data.b * 255.0);
		color->a = 255; //Opaque but it will be ignored by the shader in favor of partial transparency...
		color++;
	}
	allLightColors->load (false, true, GL_NEAREST);
	
	camera->beginCamera ();
		//Having set up the camera, we can now get the model view matrix from the stack...
		Transformation modelViewMatrix; glGetMatrixf (modelViewMatrix);

		FloatPixel *position = (FloatPixel *) allLightPositions->bytes;
		for (long index = 0; index < MAXIMUM_LIGHTS; index++) {
			LightData &data = lightData [index];
			Point cameraSpacePosition = Point (data.x, data.y, data.z) * modelViewMatrix;
			position->r = cameraSpacePosition.x;
			position->g = cameraSpacePosition.y;
			position->b = cameraSpacePosition.z;
			position->a = data.scale; 
			position++;
		}
		allLightPositions->load (false, true, GL_NEAREST);
	camera->endCamera ();

	//::log ("\nBOTH LIGHT TEXTURES SUCCESSFULLY UPLOADED");
}

void rotateLights () {
	//Rotate the lights slowly around the y-axis at lightRotationCenter which happens to be 
	//Point (-2.47, -3.66, 13.92) by physically modifying the points...
	float rotationRate = rotationOption == ROTATE_NORMALLY ? 45.0 : 10.0; //Degrees per second...
	Transformation rotation; rotation.rotateBy (rotationRate * DT, Vector (0.0, 1.0, 0.0));

	for (long index = 0; index < MAXIMUM_LIGHTS; index++) {
		LightData &data = lightData [index];
		//Move the center, rotate there, and move back...
		Point point = lightRotationCenter + (Point (data.x, data.y, data.z) - lightRotationCenter) * rotation;
		data.x = point.x; data.y = point.y; data.z = point.z;
	}
}

void drawDepthOnlyPrepass (World *world) {
	//Used once when drawing into frame buffer "lightIndexedFrameBufferID" and once when drawing into standard back buffer...

	glEnable (GL_DEPTH_TEST); glDepthFunc (GL_LEQUAL); glDepthMask (GL_TRUE); //Depth test <=, depth write. 
	drawZPrepassShader->activate (); 
	DISABLE_SHADERS; 
		world->draw ();
	ENABLE_SHADERS;
}


void drawPositionAndDepthOnlyPrepass (World *world) {//WILF REMOVE
	//Used once by drawLightsInLightIndexedFrameBuffer...

	//Like the above routine but uses drawPositionAndZPrepassShader instead...
	#if (CAPTURE_WORLD_POSITION_CS)
	glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LEQUAL); glDepthMask(GL_TRUE); //Depth test <=, depth write. 
	drawPositionAndZPrepassShader->activate();
	DISABLE_SHADERS;
	world->draw();
	ENABLE_SHADERS;
	#endif //CAPTURE_WORLD_POSITION_CS
}

#include "../../shaders/ColoringShaders/encodeLightIndex.function"


void drawLightModels() {
	//Draw a colored cube or sphere with it's color as a light index...

	glEnable (GL_DEPTH_TEST); glDepthMask (GL_FALSE); //Depth test, no depth write. 
#if (DRAW_BACKS_OF_LIGHT_MODELS_INSTEAD_OF_FRONTS)
	glCullFace (GL_FRONT); glDepthFunc (GL_GEQUAL); //Draw backs, depth test >=, no depth write. 
	//glCullFace (GL_BACK); 
#else
	glDepthFunc(GL_LEQUAL); //Draw fronts (already the default), depth test <=, no depth write. 
#endif //DRAW_BACKS_OF_LIGHT_MODELS_INSTEAD_OF_FRONTS

	glEnable(GL_BLEND); glBlendFunc(GL_ONE, GL_CONSTANT_COLOR);
	glBlendColor(0.251f, 0.251f, 0.251f, 0.251f); //Slightly more than one quarter...
	//	glBlendColor (0.25001f, 0.25001f, 0.25001f, 0.25001f); //Slightly more than one quarter...

	drawLightModelShader->activate();

	for (long lightIndex = 1; lightIndex < MAXIMUM_LIGHTS; lightIndex++) {
		LightData &data = lightData[lightIndex];
		if (!data.enabled) continue;

		//Convert the light index into 4 2-bit values each shifted into the leftmost 2 bit positions...
		unsigned char encodedColor[4]; encodeLightIndex(lightIndex, encodedColor);

		//Convert from an integer 0/255 to a float color in range 0/1...
		float converter = 1.0f / 255.0f; //To convert from a byte to a float color...; i.e., divide by 255.
		float color[4] = { encodedColor[0] * converter, encodedColor[1] * converter,
			encodedColor[2] * converter, encodedColor[3] * converter };

		float scale = data.scale;

		drawLightModelShader->setUniform3f("lightPosition", data.x, data.y, data.z);
		drawLightModelShader->setUniform1f("lightScale", scale);
		drawLightModelShader->setUniform1f("lightRadius", scale * 0.5); //A unit sphere has radius 0.5.
		drawLightModelShader->setUniform4f("lightColor", color[0], color[1], color[2], color[3]);
		LIGHT_MODEL->draw();
	}

#if (DRAW_BACKS_OF_LIGHT_MODELS_INSTEAD_OF_FRONTS)
	glCullFace(GL_BACK); //glDepthFunc (GL_LEQUAL); //Back to the default...
#else
	//Nothing more to do OR undo...
#endif //DRAW_BACKS_OF_LIGHT_MODELS_INSTEAD_OF_FRONTS
}

void drawLitWorld (World *world) {
	glEnable (GL_DEPTH_TEST); glDepthFunc (GL_LEQUAL); glDepthMask (GL_FALSE); //Depth test <=, no depth write. 
	glDisable (GL_BLEND); glBlendFunc (GL_ONE, GL_ZERO); //But the world draw will change blends anyway...

	drawWorldWithAllLightsShader->activate ();

	glActiveTexture (GL_TEXTURE1); glBindTexture (GL_TEXTURE_2D, lightIndexedColorBufferID);
	glActiveTexture (GL_TEXTURE2); glBindTexture (GL_TEXTURE_2D, allLightColors->textureHandle);    
	glActiveTexture (GL_TEXTURE3); glBindTexture (GL_TEXTURE_2D, allLightPositions->textureHandle);    
	glActiveTexture (GL_TEXTURE0); //If a face draws by activating a texture, it will be bound to "Base".

	drawWorldWithAllLightsShader->setUniformTexture ("Base", 0); 
	drawWorldWithAllLightsShader->setUniformTexture ("LightIndexedColorTexture", 1); 
	drawWorldWithAllLightsShader->setUniformTexture ("LightColorTexture", 2); 
	drawWorldWithAllLightsShader->setUniformTexture ("LightPositionTexture", 3); 

	DISABLE_SHADERS;
		doFirstTime (::log ("\nDRAW LIT WORLD with shaders %s.", disableShaders ? "DISABLED" : "ENABLED");)
		world->draw ();
	ENABLE_SHADERS;
}

void drawAllLightFuzzBalls () {
	//Draw each light as a sprite without z-writing...

	glEnable (GL_DEPTH_TEST); glDepthFunc (GL_LEQUAL); glDepthMask (GL_FALSE); //Depth test <=, no depth write. 
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glEnable (GL_BLEND);

	drawLightFuzzBallShader->activate (); 

	glActiveTexture (GL_TEXTURE0); glBindTexture (GL_TEXTURE_2D, fuzzBall->textureHandle);  
	drawLightFuzzBallShader->setUniformTexture ("Base", 0); 

	//The idea is to bring the camera back to the origin and also, bring the world point back along with it...
	//Then draw with the camera at the origin...
	Transformation cameraInverse; glGetMatrixf (cameraInverse); //That's what normally in the pipeline...

	//Skip drawing the first light which is all black but draw all the others as sprites......
	for (long lightIndex = 1; lightIndex < MAXIMUM_LIGHTS; lightIndex++) {
		LightData &data = lightData [lightIndex];
		if (!data.enabled) continue;
		glColor4f (data.r, data.g, data.b, 1.0);

		Point P = Point (data.x, data.y, data.z);
		Point Q = P * cameraInverse; //Bring the point back to the origin (WITH THE CAMERA)...

		float scale = data.scale * 0.1; //The fuzz ball is much smaller than the light itself; so scale by extra 0.1.

		glPushIdentity (); //Ignore the existing model-view matrix. Draw at the origin, hence the Identity...
			glTranslated (Q.x, Q.y, Q.z);
			glScaled (scale, scale, 1.0);
			unitSolidFace->draw (fuzzBall);
		glPopMatrix();
	}
}

void drawAllLightFuzzBallsInNormalWorld () {
	drawAllLightFuzzBalls (); 
	glDepthMask (GL_TRUE); //Restore to our default of depth writing...
}

void drawAllLightSpheresInNormalWorld () {
	//Draw each light model with depth testing and writing; i.e., normal draw...

	glEnable (GL_DEPTH_TEST); glDepthFunc (GL_LEQUAL); glDepthMask (GL_TRUE); //Depth test <=, depth write. 

	drawLightModelShader->activate (); 

	//Skip drawing the first light which is all black but draw all the others as sprites......
	for (long lightIndex = 1; lightIndex < MAXIMUM_LIGHTS; lightIndex++) {
		LightData &data = lightData [lightIndex];
		if (!data.enabled) continue;

		drawLightModelShader->setUniform3f ("lightPosition", data.x, data.y, data.z);
		drawLightModelShader->setUniform1f ("lightScale", data.scale); 
		drawLightModelShader->setUniform1f ("lightRadius", data.scale * 0.5); //A unit sphere has radius 0.5.
		drawLightModelShader->setUniform4f ("lightColor", data.r, data.g, data.b, 1.0);
		LIGHT_MODEL->draw ();
	}
}

void setupForDrawingOnScreen () {	

	//The following should not affect subsequent draws, should it?
	glDisable (GL_DEPTH_TEST); glDepthFunc (GL_LEQUAL); glDepthMask (GL_FALSE); //No depth test <=, no depth write. 
	glMatrixMode (GL_PROJECTION);
		glPushMatrix ();
			glLoadIdentity ();
			glOrtho (-RESOLUTION/2, RESOLUTION/2, -RESOLUTION/2, RESOLUTION/2, -20, 20);
				glMatrixMode (GL_MODELVIEW);
					glPushMatrix ();
						glLoadIdentity ();
						glColor4f (1,1,1,1);
						glTranslated (0,0,-1);
}


void wrapupForDrawingOnScreen () {	
					glPopMatrix ();		
			glMatrixMode (GL_PROJECTION);
		glPopMatrix ();
	glMatrixMode (GL_MODELVIEW);
}

void verifyThatLightIndicesEncodingAndDecodingWorks () {
	setupForDrawingOnScreen ();

	GLuint worldPositionBufferID;

	//Encoding pass...
	glBindFramebuffer (GL_FRAMEBUFFER, lightIndexedFrameBufferID);
	GLenum drawBuffers [1] = {GL_COLOR_ATTACHMENT0}; glDrawBuffers (1, drawBuffers);
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glEnable (GL_BLEND); glBlendFunc (GL_ONE, GL_CONSTANT_COLOR);
	glBlendColor (0.25001f, 0.25001f, 0.25001f, 0.25001f); //Slightly more than one quarter...
	#if (CAPTURE_WORLD_POSITION_CS)
		//You also need to activate the position texture and set uniforms for use in drawing light models...
	//Already done for you but you need to uncomment the following. At the moment, worldPositionBufferID doesn't exist...
		glActiveTexture (GL_TEXTURE0); glBindTexture (GL_TEXTURE_2D, worldPositionBufferID);  
		drawLightModelShader->activate ();
		drawLightModelShader->setUniformTexture ("Position", 0);
	#endif //CAPTURE_WORLD_POSITION_CS 

	drawLightModelShader->activate ();
	for (long x = 0; x < 16; x++) {
		for (long y = 0; y < 16; y++) {
			float xBase = -RESOLUTION/2 + (x * 50.0); float yBase = -RESOLUTION/2 + (y * 50.0);
			long lightIndex = y*16 + x;
			
			unsigned char encodedColor [4]; encodeLightIndex (lightIndex, encodedColor);
			float converter = 1.0f / 255.0f; //To convert from a byte to a float color...; i.e., divide by 255.
			float color [4] = {encodedColor [0] * converter, encodedColor [1] * converter, 
			encodedColor [2] * converter, encodedColor [3] * converter};

			drawLightModelShader->setUniform3f ("lightPosition", 0.0, 0.0, 0.0); //Zero is used so that our quad is not translated
			drawLightModelShader->setUniform1f ("lightScale", 1.0); //We don't want the quad to be scaled
			drawLightModelShader->setUniform1f ("lightRadius", 1.0e10); //A huge value is used so that the pixels are never discarded
			drawLightModelShader->setUniform4f ("lightColor", color [0], color [1], color [2], color [3]);
				
			for (long cell = 0; cell < 4; cell++) {
				glBegin (GL_QUADS);
					//Draw quad in right top area... [0,0] is the middle of the screen...
					glVertexAttrib2f (1, 0,0);	glVertexAttrib3f (0, xBase + 0,  yBase+0,  0);   
					glVertexAttrib2f (1, 1,0);	glVertexAttrib3f (0, xBase + 50, yBase+0,  0);   
					glVertexAttrib2f (1, 1,1);	glVertexAttrib3f (0, xBase + 50, yBase+50, 0);   
					glVertexAttrib2f (1, 0,1);	glVertexAttrib3f (0, xBase + 0,  yBase+50, 0);      
				glEnd ();
			}
		}
	}
	glDisable (GL_BLEND);

	//Verification pass...
	glBindFramebuffer (GL_FRAMEBUFFER, 0);
	verificationShader->activate ();
	//glActiveTexture (GL_TEXTURE0); glBindTexture (GL_TEXTURE_2D, lightIndexedColorBufferID);
	glActiveTexture (GL_TEXTURE1); glBindTexture (GL_TEXTURE_2D, lightIndexedColorBufferID);
	verificationShader->setUniformTexture ("LightIndexedColorTexture", 1);

	glBegin (GL_QUADS);
		//Draw quad in the middle of the screen...
		glVertexAttrib2f (1, 0,0);	glVertexAttrib3f (0, -RESOLUTION/2,  -RESOLUTION/2,  0);   
		glVertexAttrib2f (1, 1,0);	glVertexAttrib3f (0, RESOLUTION/2, -RESOLUTION/2,  0);   
		glVertexAttrib2f (1, 1,1);	glVertexAttrib3f (0, RESOLUTION/2, RESOLUTION/2, 0);   
		glVertexAttrib2f (1, 0,1);	glVertexAttrib3f (0, -RESOLUTION/2,  RESOLUTION/2, 0);      
	glEnd ();

	wrapupForDrawingOnScreen ();
}

//-----------------------------------------------------------------------------------------//

void tickColoredLights () {
	rotateLights (); 
	updateLightCameraSpacePosition ();
	if (SHOULD_SORT_LIGHTS) sortLights ();
	uploadLights ();
}

void drawColoredLights(World *world) {
	if (drawingChoice == NormalOnly) return;

	//A number of passes are needed: 2 in the lightIndexedFrameBuffer and 3 in the standard frame buffer...
	//Of these, 3 are world draws (2 z-prepasses and 1 normal draw with lit faces).

	//Switch to the lightIndexedFrameBuffer to perform 2 passes.
	//Pass1: Perform a z-prepass to set up the z-buffer with the game objects...
	//Pass2: Perform a pass drawing one light model per light to set up the color buffer with light indices...

	//Switch to the standard frame buffer to perform 3 passes.
	//Pass3: Perform a z-prepass to set up the z-buffer with the game objects... AGAIN
	//Pass4: Perform a pass with the game objects to draw lit objects using the light indices texture...
	//Pass5: Perform a pass drawing fuzz balls for the lights so you can additionally see them (not just their effects).

	//Prepare for the 2 passes in the lightIndexedFrameBuffer...
	//Note that we shouldn't perform the clear until the attachments have been performed...
	glBindFramebuffer(GL_FRAMEBUFFER, lightIndexedFrameBufferID);

	//Perform pass1 (depth only prepass)... Using default z-testing, z-writing, and don't care blending...
#if (CAPTURE_WORLD_POSITION_CS)
	//Not yet done...
	glDrawBuffers(0, NULL);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	drawPositionAndDepthOnlyPrepass(world);
#else
	glDrawBuffers(0, NULL);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); //Clear all buffers (faster to clear stencil too even if you don't use it)...
	drawDepthOnlyPrepass(world); //Note: disable color buffer drawing here but leave enabled for drawing light models...
#endif //CAPTURE_WORLD_POSITION_CS

	//Perform pass2 (draw light models)...
#if (CAPTURE_WORLD_POSITION_CS)
	//You also need to activate the position texture and set uniforms for use in drawing light models...
	//Not yet done...

	//drawLightModels();
#endif //CAPTURE_WORLD_POSITION_CS  
	GLenum buffers[1] = { GL_COLOR_ATTACHMENT0 }; glDrawBuffers(1, buffers);
	GLfloat oldClearColor[4]; glGetFloatv(GL_COLOR_CLEAR_VALUE, oldClearColor); //save
	glClearColor(0.0, 0.0, 0.0, 0.0); //Make sure color buffer lightIndexedColorBufferID is reset to all zeros.
	glClear(GL_COLOR_BUFFER_BIT);
	glClearColor(oldClearColor[0], oldClearColor[1], oldClearColor[2], oldClearColor[3]); //restore
	drawLightModels();

	//Prepare for 3 passes in the default frame buffer (it's still clear because of game's tick/draw/clear loop)...
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); //Clear all buffers...

	//Perform pass3 (depth only prepass)... Using default z-testing, z-writing, and don't care blending...
	//Wilf: If we could only attach the lightIndexedFrameBuffer's depth buffer to the main frame buffer, 
	//or if we could just copy the depth texture into the normal depth buffer, we wouldn't need to do this again...
	drawDepthOnlyPrepass(world);

	//Perform pass4. Draw lit world...
	drawLitWorld(world);

	//Perform Pass5. Drawing fuzz balls for the lights so you can additionally see them (not just their effects).
	drawAllLightFuzzBalls();

	glDepthMask(GL_TRUE); //Restore to our default of depth writing...

	//verifyThatLightIndicesEncodingAndDecodingWorks ();
}

void setupColoredLights () {
	buildGlobalLineInsertion (); //This only needs to be done once...
	drawLightModelShader = new Shader ("ColoringShaders\\drawLightModel"); 
	drawWorldWithAllLightsShader = new Shader ("ColoringShaders\\drawWorldWithAllLights"); 
	drawLightFuzzBallShader = new Shader ("ColoringShaders\\drawLightFuzzBall"); 
	drawZPrepassShader = new Shader ("ColoringShaders\\drawZPrepass");
	verificationShader = new Shader ("ColoringShaders\\verifyLightIndex");
	#if (CAPTURE_WORLD_POSITION_CS)
	drawPositionAndZPrepassShader = new Shader("ColoringShaders\\drawPositionAndZPrepass");
	#endif //CAPTURE_WORLD_POSITION_CS
	
	drawLightModelShader->load ();
	drawWorldWithAllLightsShader->load ();
	drawLightFuzzBallShader->load ();
	drawZPrepassShader->load ();
	verificationShader->load ();
	#if (CAPTURE_WORLD_POSITION_CS)
	drawPositionAndZPrepassShader->load();
	#endif //CAPTURE_WORLD_POSITION_CS

	allLightColors = new Texture (256, 1, RGBAType);
	allLightPositions = new Texture (256, 1, FloatRGBAType); //FORMAT: GL_RGBA32F
	fuzzBall = Texture::readTexture ("..\\textures\\fuzzBall.tga"); if (fuzzBall != NULL) fuzzBall->load (false, true);

	buildRawFrameBuffers (1, &lightIndexedFrameBufferID);
	buildRawTextures (1, &lightIndexedColorBufferID, GL_TEXTURE_2D, GL_RGBA8, GL_RGBA, RESOLUTION, RESOLUTION, GL_LINEAR); 
	//buildRawTextures (1, &lightIndexedColorBufferID, GL_TEXTURE_2D, GL_RGBA32F, GL_RGBA, RESOLUTION, RESOLUTION, GL_LINEAR); 
	buildRawDepthBuffers (1, &lightIndexedDepthBufferID, RESOLUTION, RESOLUTION);
	#if (CAPTURE_WORLD_POSITION_CS)
		//Note that we will draw into the position buffer as if it were a pixel...
		//Not yet done...
	buildRawTextures(1, &worldPositionBufferID, GL_TEXTURE_2D, GL_RGBA32F, GL_RGBA, RESOLUTION, RESOLUTION, GL_LINEAR);
	//buildRawDepthBuffers(1, &worldPositionBufferID, RESOLUTION, RESOLUTION);
	#endif //CAPTURE_WORLD_POSITION_CS

	glBindFramebuffer (GL_FRAMEBUFFER, lightIndexedFrameBufferID);
	attachDepthTexture (lightIndexedDepthBufferID);
	attachColorTexture (GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, lightIndexedColorBufferID);  
	#if (CAPTURE_WORLD_POSITION_CS)
		//Not yet done... 
	attachDepthTexture(worldPositionBufferID);
	#endif //CAPTURE_WORLD_POSITION_CS
	GLenum drawBuffers [1] = {GL_COLOR_ATTACHMENT0}; glDrawBuffers (1, drawBuffers); //Normal scenario only...

	GLenum FBOstatus = glCheckFramebufferStatus (GL_FRAMEBUFFER);
	if (FBOstatus != GL_FRAMEBUFFER_COMPLETE)
		::halt ("\nGL_FRAMEBUFFER_COMPLETE test failed, CANNOT use frame buffers.");

	glBindFramebuffer (GL_FRAMEBUFFER, 0); //Back to the standard frame buffer...

	prepareLightData ();
	logLights ();
}

void wrapupColoredLights () {
	drawLightModelShader->unload (); delete drawLightModelShader;
	drawWorldWithAllLightsShader->unload (); delete drawWorldWithAllLightsShader;
	drawLightFuzzBallShader->unload (); delete drawLightFuzzBallShader;
	drawZPrepassShader->unload (); delete drawZPrepassShader;
	verificationShader->unload (); delete verificationShader;
	#if (CAPTURE_WORLD_POSITION_CS)
	drawPositionAndZPrepassShader->unload(); delete drawPositionAndZPrepassShader;
	#endif //CAPTURE_WORLD_POSITION_CS

	delete allLightColors; delete allLightPositions; delete fuzzBall;

	glDeleteFramebuffers (1, &lightIndexedFrameBufferID);  
	glDeleteRenderbuffers (1, &lightIndexedDepthBufferID);
	glDeleteTextures (1, &lightIndexedColorBufferID);
}