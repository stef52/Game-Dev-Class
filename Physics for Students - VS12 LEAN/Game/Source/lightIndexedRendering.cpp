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
#define LEVEL_COUNT 1
#define DISABLE_TEMPORAL_BLENDING 1
//-----------------------------------------------------------------------------------------//

//String globalLineInsertion contains macros that are inserted into the second line of all shaders; i.e., after #version...
//Here, we define macros EXPERIMENT and CAPTURE_WORLD_POSITION_CS...

extern char globalLineInsertionData [256]; //Initialized below... but defined in shaders.cpp

//float time = 1.0f;

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
Shader *waterShader = NULL;

Shader *buildMRTTexturesShader; 
Shader *downsampleShader [LEVEL_COUNT]; 
Shader *upsampleShader [LEVEL_COUNT]; 
Shader *sharpenShader [LEVEL_COUNT];

GLuint buildMRTTexturesProgram;
GLuint downsampleProgram[LEVEL_COUNT]; // [0] = finest resolution
GLuint aoProgram[LEVEL_COUNT]; // [LEVEL_COUNT - 1] = coarsest resolution
GLuint aoSharpenProgram[LEVEL_COUNT];


#if (CAPTURE_WORLD_POSITION_CS)
Texture *worldPositionTexture = NULL;
Shader *drawPositionAndZPrepassShader = NULL;
#endif //CAPTURE_WORLD_POSITION_CS

Texture *allLightColors = NULL, *allLightPositions = NULL, *fuzzBall = NULL;
Texture *flowMap = NULL, *chasmFlowMap = NULL, *chasmColorMap = NULL, *chasmNormalMap = NULL;

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

/*void drawWater() {
	waterShader->activate();

	time += DT;

	waterShader->setUniformMatrix4fv("osg_ViewMatrixInverse", &camera->cameraMatrix.m11);

	glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, chasmNormalMap->textureHandle);
	glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, chasmColorMap->textureHandle);
	glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, chasmFlowMap->textureHandle);

	waterShader->setUniformTexture("normalMap", 0);
	waterShader->setUniformTexture("colorMap", 1);
	waterShader->setUniformTexture("flowMap", 2);
	waterShader->setUniformTexture("cubeMap", 3);

	glPushMatrix();
	glTranslated(-70.0, -5.0, 0.0);
	glRotatef(270.0f, 1.0f, 0.0f, 0.0f);
	glScalef(200.0f, 200.0f, 200.0f);
	Transformation modelView; glGetMatrixf(GL_MODELVIEW_MATRIX, modelView);
	Transformation projection; glGetMatrixf(GL_PROJECTION_MATRIX, projection);
	Transformation modelViewProjection = modelView * projection;

	unitSolidFace->draw();
	glPopMatrix();
}
*/

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
	//drawWater();
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


	//drawWater();

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
	waterShader = new Shader("hpcv-water-tile");
	#if (CAPTURE_WORLD_POSITION_CS)
	drawPositionAndZPrepassShader = new Shader("ColoringShaders\\drawPositionAndZPrepass");
	#endif //CAPTURE_WORLD_POSITION_CS
	
	drawLightModelShader->load ();
	drawWorldWithAllLightsShader->load ();
	drawLightFuzzBallShader->load ();
	drawZPrepassShader->load ();
	verificationShader->load ();
	waterShader->load();
	#if (CAPTURE_WORLD_POSITION_CS)
	drawPositionAndZPrepassShader->load();
	#endif //CAPTURE_WORLD_POSITION_CS

	allLightColors = new Texture (256, 1, RGBAType);
	allLightPositions = new Texture (256, 1, FloatRGBAType); //FORMAT: GL_RGBA32F
	fuzzBall = Texture::readTexture ("..\\textures\\fuzzBall.tga"); if (fuzzBall != NULL) fuzzBall->load (false, true);

	flowMap = Texture::readTexture("..\\textres\\Flow\\flow.tga"); if (flowMap != NULL) flowMap->load(false, true);
	chasmFlowMap = Texture::readTexture("..\\textures\\Flow\\flow.tga"); if (chasmFlowMap != NULL) chasmFlowMap->load(false, true);
	chasmColorMap = Texture::readTexture("..\\textures\\Flow\\flowBase.tga"); if (chasmColorMap != NULL) chasmColorMap->load(false, true);
	chasmNormalMap = Texture::readTexture("..\\textures\\Flow\\normal.bmp"); if (chasmNormalMap != NULL) chasmNormalMap->load(false, false);


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
	waterShader->unload(); delete waterShader;
	#if (CAPTURE_WORLD_POSITION_CS)
	drawPositionAndZPrepassShader->unload(); delete drawPositionAndZPrepassShader;
	#endif //CAPTURE_WORLD_POSITION_CS

	delete allLightColors; delete allLightPositions; delete fuzzBall;

	glDeleteFramebuffers (1, &lightIndexedFrameBufferID);  
	glDeleteRenderbuffers (1, &lightIndexedDepthBufferID);
	glDeleteTextures (1, &lightIndexedColorBufferID);
}







/**********************************************************************************
*                                                                                *
* Multi-Resolution Screen-Space Ambient Occlusion                                *
* Author: Thai-Duong Hoang and Kok-Lim Low                                       *
*                                                                                *
* You are welcome to use and modify this code for any purpose, but please note   *
* that it comes without any guarantees.                                          *
*                                                                                *
* Please keep in mind that this project should only be considered a prototype to *
* show that an idea works. As such, do not expect high code quality. The         *
* rendering "engine" is also very basic and limited in features, but I hope you  *
* can adapt the technique to your own engine without much effort.                *
*                                                                                *
**********************************************************************************/
/*
#include <cassert>
#include <cmath>
#include <stdio.h>
#include <windows.h> //For va_start, ShowCursor.

#include <ctime>
#include <fstream>
#include <iostream>*/

/*
#include "glew.h"
#include "frame3d.h"
#include "frameGrab.h"
#include "frameRate.h"
#include "glm.h"
#include "math3d.h"

#include "shaders.h"
#include "rawBuilding.h"*/

//WILF VERSION EXPERIMENTS...
#define DISABLE_TEMPORAL_BLENDING 1
#define DISCARD_UNUSED_UNIFORMS 1

#define ORIGINAL_VERSION 0
#define WILF_VERSION 1

//#define WILF_version ORIGINAL_VERSION
#define WILF_version WILF_VERSION
//#define LEVEL_COUNT 5 // number of mip-map levels (NOTE: ORIGINAL_VERSION needs to be 3 or more)...
#define LEVEL_COUNT 1 // number of mip-map levels (NOTE: ORIGINAL_VERSION needs to be 3 or more)...
#define USE_EXTRA_SHARPENING 1 //An experiment to see if it matters (it does)

#define MOVEMENT_SPEED 30.0f // camera's movement speed
#define PI 3.14159265

const float scale = 1.0; // used to scale the scene model
const float rMax = 7.0; // maximum screen-space sampling radius
const float zNear = 0.1f;
const float zFar = 1000.0f;
const float fov = 65.238f;

bool oddFrame = true; // used to ping-pong buffers in the temporal filtering step
bool key_state[256] = { false }; // used to smoothly control camera's movement

// Euler angles to describe camera's orientation
float phi = 0.0f;
float theta = 0.0f;
float psi = 0.0f;

int minResolution = RESOLUTION; // the coarsest resolution

GLuint modelList; // scene models' display list
//GLMmodel *model; // glm object containing scene models

std::string meshFile; // scene models' file name

float gluOrtho[LEVEL_COUNT][16]; // orthogonal projection matrix

#if (DISABLE_TEMPORAL_BLENDING) 
#else
const float dMax = 2.5f; // AO radius of influence (wilf: maximum camera-space sampling radius)
float iMVMat[16]; // inverse model-view matrix
float mVMat[16]; // model-view matrix
#endif //DISABLE_TEMPORAL_BLENDING
float projMat[16]; // perspective projection matrix

// timers, used to control camera's movement and animations
int previousTime;
int currentTime;
int elapsedTime;

//Buffers and textures ([0] = highest resolution)
GLuint frameBuffers[LEVEL_COUNT];
GLuint depthBuffers[LEVEL_COUNT];
GLuint posTex[LEVEL_COUNT];
GLuint normTex[LEVEL_COUNT];
GLuint aoTex[LEVEL_COUNT];
GLuint aoTexSharpen[LEVEL_COUNT];

#if (DISABLE_TEMPORAL_BLENDING)
#else
float *randRot[LEVEL_COUNT];
GLuint randRotTex[LEVEL_COUNT];
GLuint lastFrameAOTex;
GLuint lastFramePosTex;
#endif //DISABLE_TEMPORAL_BLENDING

// render targets
GLenum bufs01[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
#if (DISABLE_TEMPORAL_BLENDING)
#else
GLenum bufs51[2] = { GL_COLOR_ATTACHMENT5, GL_COLOR_ATTACHMENT1 };
#endif //DISABLE_TEMPORAL_BLENDING


#define setupStaticBuffer() \
	static char buffer [500]; va_list parameters; \
	va_start (parameters, message); \
	vsprintf (buffer, message, parameters); \
	va_end (parameters) 



#define doFirstTime(code) {static bool firstTime = true; if (firstTime) {firstTime = false; {code;}}}

//Sharpens the AO texture.
void Sharpen(int size, int index) {
	sharpenShader[index]->activate();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_RECTANGLE, aoTex[index]);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_RECTANGLE, normTex[index]);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_RECTANGLE, posTex[index]);

	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffers[index]);
	glDrawBuffer(GL_COLOR_ATTACHMENT3);

	glPushAttrib(GL_VIEWPORT_BIT);
	glViewport(0, 0, size, size);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBegin(GL_QUADS);
	glVertex2d(0, 0); //WILF: NOW COUNTER-CLOCKWISE SO ALWAYS WORKS.
	glVertex2d(size, 0);
	glVertex2d(size, size);
	glVertex2d(0, size);
	glEnd();
	glPopAttrib();
}

//Creates orthographic projection matrices.
void BuildOrthographicMatrices() {
	int size = RESOLUTION;

	for (int i = 0; i < LEVEL_COUNT; ++i) {
		glPushMatrix();

		glLoadIdentity();
		gluOrtho2D(0, size, 0, size);

		glGetFloatv(GL_MODELVIEW_MATRIX, gluOrtho[i]);

		glPopMatrix();

		size /= 2;
	}

	glPushMatrix();
	glLoadIdentity();
	gluPerspective(fov, (GLfloat)RESOLUTION / (GLfloat)RESOLUTION, zNear, zFar);
	glGetFloatv(GL_MODELVIEW_MATRIX, projMat);
	glPopMatrix();

	glPushMatrix();
	glLoadIdentity();
	//camera.ApplyCameraTransform();
#if (DISABLE_TEMPORAL_BLENDING) 
#else
	glGetFloatv(GL_MODELVIEW_MATRIX, mVMat);
	M3DInvertMatrix44f(mVMat, iMVMat);
#endif //DISABLE_TEMPORAL_BLENDING
	glPopMatrix();
}

///Downsample...
void Downsample(int size, int index) {
	downsampleShader[index]->activate();
#define setUnit(unit,texture) glActiveTexture (GL_TEXTURE##unit); glBindTexture(GL_TEXTURE_RECTANGLE, texture);

	glActiveTexture(GL_TEXTURE0);
#if (DISABLE_TEMPORAL_BLENDING)
	glBindTexture(GL_TEXTURE_RECTANGLE, posTex[index - 1]);
#else 
	glBindTexture(GL_TEXTURE_RECTANGLE, index == 1 && !oddFrame ? lastFramePosTex : posTex[index - 1]);
#endif //DISABLE_TEMPORAL_BLENDING

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_RECTANGLE, normTex[index - 1]);

	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffers[index]);
	glDrawBuffers(2, bufs01);

	glPushAttrib(GL_VIEWPORT_BIT);
	glViewport(0, 0, size, size);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//doFirstTime (logMatrices ("DRAW QUAD FOR DOWNSAMPLE"););
	glBegin(GL_QUADS);
	glVertex2d(0, 0); //WILF: NOW COUNTER-CLOCKWISE SO ALWAYS WORKS.
	glVertex2d(size, 0);
	glVertex2d(size, size);
	glVertex2d(0, size);
	glEnd();
	glPopAttrib();
#undef setUnit
}

//Draws the model.
/*
void DrawModel() {
	//doFirstTime (logMatrices ("DRAW WORLD FOR AO"););
	glPushMatrix();
	glScalef(scale, scale, scale);
	glCallList(modelList);
	glPopMatrix();
}*/

/* Initializes some OpenGL states */
void GLInit() {
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_RECTANGLE);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
}

enum ModelType { UseSponza, UseSibenik, UseConference };
ModelType modelType = /*UseSponza; */UseSibenik; //UseSponza; //WILF

//Converts from matrix representation of camera's orientation to Euler angles.
void Matrix2Euler(float *m, float *psi, float *theta, float *phi) {
	if (m[8] != -1.0f && m[8] != 1.0f) {
		*theta = -asin(m[8]);
		*psi = atan2(m[9] / cos(*theta), m[10] / cos(*theta));
		*phi = atan2(m[4] / cos(*theta), m[0] / cos(*theta));
	}
	else {
		*phi = 0.0f;
		if (m[8] == -1.0f) {
			*theta = PI / 2.0f;
			*psi = *phi + atan2(m[1], m[2]);
		}
		else {
			*theta = -PI / 2.0f;
			*psi = -(*phi) + atan2(-m[1], -m[2]);
		}
	}
}

//Renders the scene for the first time.
void BuildMRTTextures() {
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffers[0]);
#if (DISABLE_TEMPORAL_BLENDING)
	glDrawBuffers(2, bufs01);
#else
	if (oddFrame)
		glDrawBuffers(2, bufs01);
	else
		glDrawBuffers(2, bufs51);
#endif //DISABLE_TEMPORAL_BLENDING

	buildMRTTexturesShader->activate();

	glPushAttrib(GL_VIEWPORT_BIT);
	glViewport(0, 0, RESOLUTION, RESOLUTION);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//DrawModel();
	
	glPopAttrib();
}

//Renders AO at different resolutions.
void Upsample(int size, int index) {
	upsampleShader[index]->activate();
#define setUnit(unit,texture) glActiveTexture (GL_TEXTURE##unit); glBindTexture(GL_TEXTURE_RECTANGLE, texture);

	/*
	setUnit (0, posTex [index]);
	#if (DISABLE_TEMPORAL_BLENDING)
	#else
	setUnit (0, index == 0 && !oddFrame ? lastFramePosTex : posTex [index]);
	#endif //DISABLE_TEMPORAL_BLENDING

	setUnit (1, normTex [index]);

	if (index < LEVEL_COUNT - 1 || LEVEL_COUNT == 1) {
	setUnit (2, USE_EXTRA_SHARPENING ? aoTexSharpen [index + 1] : aoTex [index + 1]);
	setUnit (3, posTex [index + 1]);
	setUnit (4, normTex [index + 1]);
	}
	*/

#if (DISABLE_TEMPORAL_BLENDING)
	setUnit(1, normTex[index]);
	setUnit(0, posTex[index]);
	if (index < LEVEL_COUNT - 1 || LEVEL_COUNT == 1) {
		setUnit(2, USE_EXTRA_SHARPENING ? aoTexSharpen[index + 1] : aoTex[index + 1]);
		setUnit(3, posTex[index + 1]);
		setUnit(4, normTex[index + 1]);
	}
#else
	setUnit(0, index == 0 && !oddFrame ? lastFramePosTex : posTex[index]);
	setUnit(1, normTex[index]);
	if (index < LEVEL_COUNT - 1 || LEVEL_COUNT == 1) {
		setUnit(2, USE_EXTRA_SHARPENING ? aoTexSharpen[index + 1] : aoTex[index + 1]);
		setUnit(3, posTex[index + 1]);
		setUnit(4, normTex[index + 1]);
	}
	if (size == RESOLUTION) {
		if (oddFrame) {
			setUnit(5, lastFrameAOTex);
			setUnit(6, lastFramePosTex);
		}
		else {
			setUnit(5, aoTex[0]);
			setUnit(6, posTex[0]);
		}

#if (DISCARD_UNUSED_UNIFORMS)
#else
		setUnit(7, randRotTex[index]);
#endif //DISCARD_UNUSED_UNIFORMS

		glUniformMatrix4fv(glGetUniformLocation(aoProgram[0], "OLDmVMat"), 1, false, mVMat); //WILF: store old mv
		glGetFloatv(GL_MODELVIEW_MATRIX, mVMat); //wilf: get new mv (will be old next time)
		glUniformMatrix4fv(glGetUniformLocation(aoProgram[0], "mVMat"), 1, false, mVMat); //WILF: store new mv
		M3DInvertMatrix44f(mVMat, iMVMat); //WILF: new 
		glUniformMatrix4fv(glGetUniformLocation(aoProgram[0], "iMVMat"), 1, false, iMVMat); //wilf: store new inverse mv
	}
#endif //DISABLE_TEMPORAL_BLENDING

	if (size < RESOLUTION) {
		glBindFramebuffer(GL_FRAMEBUFFER, frameBuffers[index]);
		glDrawBuffer(GL_COLOR_ATTACHMENT2);
	}
	else {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	glPushAttrib(GL_VIEWPORT_BIT);
	glViewport(0, 0, size, size);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//doFirstTime (logMatrices ("DRAW QUAD FOR UPSAMPLE"););
	glBegin(GL_QUADS);
	glVertex2d(0, 0); //WILF: NOW COUNTER-CLOCKWISE SO ALWAYS WORKS.
	glVertex2d(size, 0);
	glVertex2d(size, size);
	glVertex2d(0, size);
	glEnd();
	glPopAttrib();

	if (size < RESOLUTION && USE_EXTRA_SHARPENING && index < LEVEL_COUNT - 1)
		Sharpen(size, index);
}

//Resizes the display window.
void Reshape(int w, int h) {
	// avoid dividing by 0
	if (h == 0) h = 1;

	glViewport(0, 0, w, h);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(fov, (GLfloat)w / (GLfloat)h, zNear, zFar);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

//Setups the shader programs used in the AO blur steps.
void SetupAOSharpenPrograms() {

	for (int i = LEVEL_COUNT - 1; i >= 0; --i) {
		//Compile many times just so that each one can have a different orthographics matrix...
		Shader *shader = sharpenShader[i] = new Shader("../Shaders/AOshaders/sharpen");
		shader->load();
		aoSharpenProgram[i] = shader->handle();

		shader->activate();
		//Shader output color is called "A0"; 
		shader->setUniformTexture("aoTex", 0);
		shader->setUniformTexture("normTex", 1);
		shader->setUniformTexture("posTex", 2);
		shader->setUniformMatrix4fv("gluOrtho", gluOrtho[i]);
	}
}

void OLDDraw() {

	static int counter = 0; // this is used to control the printing of fps

	previousTime = currentTime;

	glPushMatrix();
	//camera.ApplyCameraTransform();
	BuildMRTTextures();
	for (int index = 1, size = RESOLUTION / 2; index < LEVEL_COUNT; index++, size /= 2) {
		Downsample(size, index);
	}
	for (int index = LEVEL_COUNT - 1, size = minResolution; index >= 0; index--, size *= 2) {
		Upsample(size, index);
	}
	glPopMatrix();

#if (DISABLE_TEMPORAL_BLENDING)
#else
	if (oddFrame)
		glBindTexture(GL_TEXTURE_RECTANGLE, aoTex[0]);
	else
		glBindTexture(GL_TEXTURE_RECTANGLE, lastFrameAOTex);
	oddFrame = !oddFrame;
	glCopyTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, 0, 0, RESOLUTION, RESOLUTION);
#endif //DISABLE_TEMPORAL_BLENDING

	glutSwapBuffers();
}

/* Setups the shader programs used in the AO computation steps */
void SetupUpsamplingShaders() {
	GLfloat poissonDisk[] = {
		-0.6116678f, 0.04548655f, -0.26605980f, -0.6445347f,
		-0.4798763f, 0.78557830f, -0.19723210f, -0.1348270f,
		-0.7351842f, -0.58396650f, -0.35353550f, 0.3798947f,
		0.1423388f, 0.39469180f, -0.01819171f, 0.8008046f,
		0.3313283f, -0.04656135f, 0.58593510f, 0.4467109f,
		0.8577477f, 0.11188750f, 0.03690137f, -0.9906120f,
		0.4768903f, -0.84335800f, 0.13749180f, -0.4746810f,
		0.7814927f, -0.48938420f, 0.38269190f, 0.8695006f };

#define FRAG 

	float size = (float)RESOLUTION;
	const bool original = WILF_version == ORIGINAL_VERSION; //WILF ADDITION
	const char *FIRST = original ? "../Shaders/AOshaders/oldUpsampleFirst" FRAG : "../Shaders/AOshaders/upsample" FRAG;
	const char *MIDDLE = original ? "../Shaders/AOshaders/oldUpsampleMiddle" FRAG : "../Shaders/AOshaders/upsample" FRAG;
	const char *LAST = original ? "../Shaders/AOshaders/oldUpsampleLast" FRAG : "../Shaders/AOshaders/upsample" FRAG;

	for (int i = 0; i < LEVEL_COUNT; ++i, size /= 2.0f) {
		const char *SHADER_NAME = i == 0 ? LAST : (i == LEVEL_COUNT - 1 ? FIRST : MIDDLE); //WILF MODIFICATION

		Shader *shader = upsampleShader[i] = new Shader(SHADER_NAME);
		shader->load();
		aoProgram[i] = shader->handle();

		shader->activate();
		shader->setUniformTexture("posTex", 0);
		shader->setUniformTexture("normTex", 1);

		if (i < LEVEL_COUNT - 1 || LEVEL_COUNT == 1) {
			shader->setUniformTexture("loResAOTex", 2);
			shader->setUniformTexture("loResPosTex", 3);
			shader->setUniformTexture("loResNormTex", 4);
		}
#if (DISABLE_TEMPORAL_BLENDING)
#else
		if (i == 0) {
			shader->setUniformTexture("lastFrameAOTex", 5);
			shader->setUniformTexture("lastFramePosTex", 6);
			shader->setUniformMatrix4fv("projMat", projMat);
			shader->setUniformMatrix4fv("iMVMat", iMVMat);
			shader->setUniformMatrix4fv("mVMat", mVMat);
		}
#endif //DISABLE_TEMPORAL_BLENDING

		shader->setUniformMatrix4fv("gluOrtho", gluOrtho[i]);
		shader->setUniform1f("rMax", rMax);
		//ShaderSetting setting = computeShaderSetting(i); //WILF ADDITION
	//	shader->setUniform1f("usePoisson", setting.usePoisson); //WILF ADDITION
	//	shader->setUniform1f("useUpsampling", setting.useUpsampling); //WILF ADDITION
	//	shader->setUniform1f("useTemporalSmoothing", setting.useTemporalSmoothing); //WILF ADDITION
		shader->setUniform1f("resolution", size); //Referenced by temporal blending...//wilf fix
#if (DISCARD_UNUSED_UNIFORMS)
#else
#define WILF_FUDGE 0.0*
		shader->setUniform1f("dMax", WILF_FUDGE dMax);
		float r = size * dMax / (2.0f * abs(tan(fov * PI / 180.0f / 2.0f)));
		shader->setUniform1f("r", WILF_FUDGE r);
		float angleInRadians = fov * PI / 180.0f; float fovFactor = 0.5 / tan(angleInRadians * 0.5); //WILF ADDITION 
		shader->setUniform1f("fovFactor", WILF_FUDGE r); //WILF ADDITION
		shader->setUniform1f("randRotTex", 7);
		shader->setUniformfv("poissonDisk", 32, poissonDisk);
#endif //DISCARD_UNUSED_UNIFORMS
	}
}

//Setups the shader programs used in the downsampling step.
void SetupDownsamplingShaders() {
	for (int i = 1; i < LEVEL_COUNT; ++i) {
		Shader *shader = downsampleShader[i] = new Shader("../Shaders/AOshaders/downsample");
		const char *defaultMRT[] = { "Pos", "Norm" };
		shader->load();//defaultAttributes, 3, defaultMRT, 2);
		downsampleProgram[i] = shader->handle();

		//Shader output colors are called "Pos" and "Norm". 
		shader->setUniformTexture("hiResPosTex", 0);
		shader->setUniformTexture("hiResNormTex", 1);
		shader->setUniformMatrix4fv("gluOrtho", gluOrtho[i]);
	}
}

void SetupFBOs() {
	buildRawFrameBuffers(LEVEL_COUNT, frameBuffers);
	for (int i = 0, size = RESOLUTION; i < LEVEL_COUNT; i++, size /= 2) {
		buildRawDepthBuffers(1, &depthBuffers[i], size, size);
		buildRawTextures(1, &posTex[i], GL_TEXTURE_RECTANGLE, GL_RGBA32F, GL_RGBA, size, size, GL_NEAREST);
		buildRawTextures(1, &normTex[i], GL_TEXTURE_RECTANGLE, GL_RGBA32F, GL_RGBA, size, size, GL_NEAREST);
		buildRawTextures(1, &aoTex[i], GL_TEXTURE_RECTANGLE, GL_RGBA32F, GL_RGBA, size, size, GL_NEAREST);
		buildRawTextures(1, &aoTexSharpen[i], GL_TEXTURE_RECTANGLE, GL_RGBA32F, GL_RGBA, size, size, GL_NEAREST);

		glBindFramebuffer(GL_FRAMEBUFFER, frameBuffers[i]);
		attachDepthTexture(depthBuffers[i]);
		attachColorTexture(GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE, posTex[i]); //, i);
		attachColorTexture(GL_COLOR_ATTACHMENT1, GL_TEXTURE_RECTANGLE, normTex[i]); //, i);
		attachColorTexture(GL_COLOR_ATTACHMENT2, GL_TEXTURE_RECTANGLE, aoTex[i]); //, i);
		attachColorTexture(GL_COLOR_ATTACHMENT3, GL_TEXTURE_RECTANGLE, aoTexSharpen[i]); //, i);
		//checkActiveFrameBufferStatus();
	}
}

//Setup the shader program used in the buildMRTTextures step.
void SetupMRTBuildingShaders() {
	Shader *shader = buildMRTTexturesShader = new Shader("../Shaders/AOshaders/buildMRTTextures");//..\\textures\\Flow\\normal.bmp
	const char *defaultMRT[] = { "Pos", "Norm" };
	shader->load();// defaultAttributes, 3, defaultMRT, 2);
	buildMRTTexturesProgram = shader->handle();
}

#if (DISCARD_UNUSED_UNIFORMS)
#else
//Creates a random rotation texture in a 3x3 interleaved pattern.
void SetupRandRotationTexture() {
	float pattern[LEVEL_COUNT][3][3][2];

	srand(927);

	glGenTextures(LEVEL_COUNT, randRotTex);

	for (int index = 0; index < LEVEL_COUNT; ++index) {
		for (int i = 0; i < 3; ++i) {
			for (int j = 0; j < 3; ++j) {
				float alpha = (float)rand() / RAND_MAX * 2.0 * PI;
				pattern[index][i][j][0] = sin(alpha);
				pattern[index][i][j][1] = cos(alpha);
			}
		}
	}

	int size = RESOLUTION;

	for (int i = 0; i < LEVEL_COUNT; ++i) {
		randRot[i] = new float[size * size * 2];
		size /= 2;
	}

	size = RESOLUTION;

	for (int index = 0; index < LEVEL_COUNT; ++index) {
		for (int i = 0; i < size; ++i) {
			for (int j = 0; j < size; ++j) {
				randRot[index][i * size * 2 + j * 2 + 0] = pattern[index][i % 3][j % 3][0];
				randRot[index][i * size * 2 + j * 2 + 1] = pattern[index][i % 3][j % 3][1];
				if (i * size * 2 + j * 2 + 1 >= size * size * 2) halt("\nRandom rotation texture index out of bounds...");
			}
		}

		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, randRotTex[index]);
		glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RG32F, size, size, 0, GL_RG, GL_FLOAT, randRot[index]);
		glTexParameterf(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		size /= 2;
	}
}
#endif //DISCARD_UNUSED_UNIFORMS


//Global setup/wrapup...
void SetupAO() {
	Shader::setup();
	//OLDSetupFBOs(); 
	SetupFBOs();
#if (DISCARD_UNUSED_UNIFORMS)
#else
	SetupRandRotationTexture();
#endif //DISCARD_UNUSED_UNIFORMS
	SetupMRTBuildingShaders();
	SetupDownsamplingShaders();
	SetupUpsamplingShaders();
	SetupAOSharpenPrograms();
}

void WrapupAO() {
	log("\nExiting...\n");
	delete buildMRTTexturesShader;
	for (long index = 0; index < LEVEL_COUNT; index++) {
		delete downsampleShader[index];
		delete upsampleShader[index];
		delete sharpenShader[index];
	}

	glDeleteFramebuffers(LEVEL_COUNT, frameBuffers);
	glDeleteRenderbuffers(LEVEL_COUNT, depthBuffers);

#if (DISABLE_TEMPORAL_BLENDING)
	//Eliminate last frames
#else
	glDeleteTextures(1, &lastFrameAOTex);
	glDeleteTextures(1, &lastFramePosTex);
#endif //DISABLE_TEMPORAL_BLENDING
	glDeleteTextures(LEVEL_COUNT, posTex);
	glDeleteTextures(LEVEL_COUNT, normTex);
	glDeleteTextures(LEVEL_COUNT, aoTex);
	glDeleteTextures(LEVEL_COUNT, aoTexSharpen);

	glDeleteLists(modelList, 1);

#if (DISCARD_UNUSED_UNIFORMS) 
#else
	for (int i = 0; i < LEVEL_COUNT; ++i) { delete[] randRot[i]; }
#endif //DISCARD_UNUSED_UNIFORMS
}
/*
//Main...
int main(int argc, char* argv[]) {
	clearLog(); //wilf
	meshFile = std::string("./scn/");
	if (modelType == UseSponza) { meshFile.append("sponza.obj"); }
	else //WILF
		if (modelType == UseSibenik) { meshFile.append("sibenik.obj"); }
		else //WILF
			if (modelType == UseConference) { meshFile.append("conference.obj"); } //WILF

	minResolution = RESOLUTION; //WILF: Clearer to reinitialize here to set it up...
	for (int i = 1; i < LEVEL_COUNT; ++i)
		minResolution /= 2;

	glutInit(&argc, argv);
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowSize(RESOLUTION, RESOLUTION);
	glutCreateWindow("SSAO");

	GLInit();
	glewInit();

	glutReshapeFunc(Reshape);
	glutDisplayFunc(WILF_version == ORIGINAL_VERSION ? OLDDraw : Draw);
	glutKeyboardFunc(KeyDown);
	glutKeyboardUpFunc(KeyUp);
	glutSpecialFunc(Special);
	glutSpecialUpFunc(SpecialUp);
	glutIdleFunc(Tick);

	LoadModel();
	InitCamera();
	BuildOrthographicMatrices();

	Setup();
	currentTime = glutGet(GLUT_ELAPSED_TIME);

	glutMainLoop();

	Wrapup();
}*/