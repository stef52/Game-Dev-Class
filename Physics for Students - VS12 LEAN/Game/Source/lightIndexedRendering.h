
//*****************************************************************************************//
//                                Light Indexed Rendering                                  //
//*****************************************************************************************//


//Note: In shaders, you have access to EXPERIMENT but experiment numbers are simply 0, 1, 2, ...

#define EXPERIMENT0 0 //Normal drawing without colored lights. 
#define EXPERIMENT1 1 //Normal drawing with lights as fuzz balls. 
#define EXPERIMENT2 2 //Normal drawing with lights as spheres.
#define EXPERIMENT3 3 //Indexed light drawing with lights as fuzz balls (NO POSITION G-BUFFER).
#define EXPERIMENT4 4 //Indexed light drawing with lights as fuzz balls WITH POSITION G-BUFFER.
#define EXPERIMENT5 5 //Indexed light drawing with POSITION G-BUFFER and lights drawing backs.

#define EXPERIMENT EXPERIMENT3

#if (EXPERIMENT == EXPERIMENT0)
#define drawingChoice NormalOnly
#define drawFuzzBallsInNormalDraw false
#define drawLightSpheresInNormalDraw false
#define SCALE_DOWN_LIGHT_SIZES 0
#define DONT_BUNCH_UP 1
#define CAPTURE_WORLD_POSITION_CS 0
#define DRAW_BACKS_OF_LIGHT_MODELS_INSTEAD_OF_FRONTS 0
#elif (EXPERIMENT == EXPERIMENT1)
#define drawingChoice NormalOnly
#define drawFuzzBallsInNormalDraw true		//changed
#define drawLightSpheresInNormalDraw false
#define SCALE_DOWN_LIGHT_SIZES 0
#define DONT_BUNCH_UP 1
#define CAPTURE_WORLD_POSITION_CS 0
#define DRAW_BACKS_OF_LIGHT_MODELS_INSTEAD_OF_FRONTS 0
#elif (EXPERIMENT == EXPERIMENT2)
#define drawingChoice NormalOnly
#define drawFuzzBallsInNormalDraw false		//changed
#define drawLightSpheresInNormalDraw true	//changed
#define SCALE_DOWN_LIGHT_SIZES 0
#define DONT_BUNCH_UP 1
#define CAPTURE_WORLD_POSITION_CS 0
#define DRAW_BACKS_OF_LIGHT_MODELS_INSTEAD_OF_FRONTS 0
#elif (EXPERIMENT == EXPERIMENT3)
#define drawingChoice UseIndexedLights		//changed
#define drawFuzzBallsInNormalDraw false		
#define drawLightSpheresInNormalDraw false	//changed
#define SCALE_DOWN_LIGHT_SIZES 0
#define DONT_BUNCH_UP 1
#define CAPTURE_WORLD_POSITION_CS 0
#define DRAW_BACKS_OF_LIGHT_MODELS_INSTEAD_OF_FRONTS 0
#elif (EXPERIMENT == EXPERIMENT4)
#define drawingChoice UseIndexedLights		
#define drawFuzzBallsInNormalDraw false		
#define drawLightSpheresInNormalDraw false	
#define SCALE_DOWN_LIGHT_SIZES 0
#define DONT_BUNCH_UP 1
#define CAPTURE_WORLD_POSITION_CS 1 //changed
#define DRAW_BACKS_OF_LIGHT_MODELS_INSTEAD_OF_FRONTS 0
#elif (EXPERIMENT == EXPERIMENT5)
#define drawingChoice UseIndexedLights		
#define drawFuzzBallsInNormalDraw false		
#define drawLightSpheresInNormalDraw false	
#define SCALE_DOWN_LIGHT_SIZES 0
#define DONT_BUNCH_UP 1
#define CAPTURE_WORLD_POSITION_CS 1
#define DRAW_BACKS_OF_LIGHT_MODELS_INSTEAD_OF_FRONTS 1  //changed
#endif

//*****************************************************************************************//
//                   Routines for building frame, depth, and color buffers                 //
//*****************************************************************************************//

void buildRawFrameBuffers(long howMany, GLuint *frameBufferIDs);
void buildRawDepthBuffers(long howMany, GLuint *depthBufferIDs, long width, long height);
//void buildRawTextures(long howMany, GLuint *textureIDs, long kind, long format, long components, long width, long height, long filter = GL_NEAREST);

void buildRawCubeMapFromFile(GLuint &textureID, const char *folder, const char *fileName);

void attachDepthTexture(GLuint depthBufferID);
void attachColorTexture(long whichAttachment, long textureType, GLuint textureID);

void drawAllLightFuzzBallsInNormalWorld();
void drawAllLightSpheresInNormalWorld();

//*****************************************************************************************//
//                        Routines for Light Indexed Rendering                             //
//*****************************************************************************************//

void setupColoredLights();
void wrapupColoredLights();
void tickColoredLights();
void drawColoredLights(World *world);

void drawWater();