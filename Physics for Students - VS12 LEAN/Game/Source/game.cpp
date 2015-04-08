
//*****************************************************************************************//
//                                      Includes                                           //
//*****************************************************************************************//

#include "includes.all"

//*****************************************************************************************//
//                                        Game                                             //
//*****************************************************************************************//

#define RESOLUTION 800
Game *game = NULL;
double DT; 

Choice choice = AO;

Shader *ShaderForWater = NULL;
Texture *waterText = NULL,
*waterColor = NULL,
*waterFlow = NULL,
*lavaFlow = NULL;
GLuint	waterID = 0,
waterColorID = 0,
waterFlowID = 0,
waterCubeMapID = 0,
lavaCubeMapID = 0,
lavaFlowID = 0;
float ticks = 0;
const float scale = 1.0;

Shader *ShaderForAO = NULL;
Shader *buildMRTTexturesShader = NULL;

HDC Game::deviceContext; GLuint Game::fontBase;
const bool useLights = false;
//Choice choice = UseWater;

void setupOpenGL () {
	glEnable (GL_CULL_FACE); glEnable (GL_DEPTH_TEST); glEnable (GL_TEXTURE_2D);
	glLineWidth (3.0);

	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	gluPerspective (40.0, 1.0, 1.0, 100.0); //See resizeWindow for parameter explanation.
	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity ();

	GLfloat lightColor [] = {1.0f, 1.0f, 1.0f, 1.0f}; //white
	glLightModeli (GL_LIGHT_MODEL_LOCAL_VIEWER, 1);
	glLightfv (GL_LIGHT0, GL_DIFFUSE, lightColor);
	glLightf (GL_LIGHT0, GL_CONSTANT_ATTENUATION, 0.1f);
	glLightf (GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.05f);
	glEnable (GL_LIGHT0); glEnable (GL_LIGHTING);
	glDisable (GL_LIGHTING); //We can deal with our own lighting.
	glEnable (GL_COLOR_MATERIAL); //Track color.

	glClearColor (0.0, 0.0, 0.0, 1.0); //black
	//glClearColor (0.0, 0.0, 0.0, 0.0); //transparent black
	//glClearColor (1.0, 0.0, 1.0, 1.0); //purple
	glClearDepth (1.0); //Far
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glDrawBuffer (GL_BACK); glReadBuffer (GL_BACK);
	glEnable (GL_DEPTH_TEST); glDepthFunc (GL_LEQUAL); glDepthMask (GL_TRUE); 
	glShadeModel (GL_SMOOTH);	

	glFrontFace (GL_CCW); glCullFace (GL_BACK); glEnable (GL_CULL_FACE);

	glHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST); 
	glDisable (GL_ALPHA_TEST);
	glPolygonOffset (0.0, -3.0);

	//Setup materials.
	GLfloat	frontMaterialDiffuse [4] = {0.2f, 0.2f, 0.2f, 1.0f};
	glMaterialfv (GL_FRONT, GL_DIFFUSE, frontMaterialDiffuse);
	GLfloat	frontMaterialAmbient [4] = {0.8f, 0.8f, 0.8f, 1.0f};
	glMaterialfv (GL_FRONT, GL_AMBIENT, frontMaterialAmbient);
	GLfloat frontMaterialSpecular [4] = {0.1f, 0.1f, 0.1f, 1.0f};
	glMaterialfv (GL_FRONT, GL_SPECULAR, frontMaterialSpecular);
	GLfloat	frontMaterialShininess [] = {70.0f};
	glMaterialfv (GL_FRONT, GL_SHININESS, frontMaterialShininess);
	GLfloat	frontMaterialEmission [4] = {0.0f, 0.0f, 0.0f, 1.0f};
	glMaterialfv (GL_FRONT, GL_EMISSION, frontMaterialEmission);

	GLfloat	backMaterialDiffuse [4] = {0.0f, 0.0f, 0.0f, 1.0f};
	glMaterialfv (GL_BACK, GL_DIFFUSE, backMaterialDiffuse);
	GLfloat	backMaterialAmbient [4] = {0.0f, 0.0f, 0.0f, 1.0f};
	glMaterialfv (GL_BACK, GL_AMBIENT, backMaterialAmbient);
	GLfloat	backMaterialSpecular [4] = {0.0f, 0.0f, 0.0f, 1.0f};
	glMaterialfv (GL_BACK, GL_SPECULAR, backMaterialSpecular);
	GLfloat	backMaterialShininess [] = {0.0f};
	glMaterialfv (GL_BACK, GL_SHININESS, backMaterialShininess);
	GLfloat	backMaterialEmission [4] = {0.0f, 0.0f, 0.0f, 1.0f};
	glMaterialfv (GL_BACK, GL_EMISSION, backMaterialEmission);

	glColorMaterial (GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

	//Use a global default texture environment mode.
	glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	if (useLights) glEnable (GL_LIGHTING); else glDisable (GL_LIGHTING);
}

GLuint shadowMapFrameBufferID, shadowMapDepthBufferID;

void Game::setup () {
	setupOpenGL (); setupOpenGLExtensions ();
	glewInit ();
	setupFont ();
	physicsManager = new PhysicsManager (); 
	game = new Game ();
	Player::setup ();
	Camera::setup ();
	InputManager::setup ();
	World::setup ();
	FaceGroup::setup ();
	Shader::setup ();
	//setupColoredLights();
	//setupShadows();

	if (choice == UseShadows)
	{
		setupShadows();
	}
	if (choice == UseIndexedLights || choice == UseShadowsAndIndexedLights)
	{
		setupColoredLights();
	}

	setupWater();

	BuildOrthographicMatrices();
	setupAO();
}

void Game::wrapup () {
	Shader::wrapup();
	FaceGroup::wrapup();
	//if (game == NULL) return; //Wrapping up a second time (shouldn't happen).
	if (game->world != NULL) { delete game->world; game->world = NULL; }
	wrapupFont();
	delete game; game = NULL;
	Player::wrapup();
	Camera::wrapup();
	InputManager::wrapup();
	World::wrapup();
	wrapupColoredLights();
	wrapupShadowedLights();
	delete physicsManager;
	::log ("\nEnding game...\n\n");
	wrapupWater();
	wrapupAO();
}

void Game::wrapupWater()
{
	glDeleteTextures(1, &waterID);
	glDeleteTextures(1, &waterColorID);
	glDeleteTextures(1, &waterFlowID);
	glDeleteTextures(1, &waterCubeMapID);
	glDeleteTextures(1, &lavaCubeMapID);

	ShaderForWater->unload();

	delete ShaderForWater;

}


void Game::tick () {
	inputManager->tick ();
	camera->tick ();
	player->tick ();
	if (physicsManager->scene != NULL) {
		physicsManager->scene->simulate(DT);
		physicsManager->scene->fetchResults(true);
	}
	if (world != NULL) world->tick ();

	if (drawingChoice == UseIndexedLights)
	{
		tickColoredLights();
	}
	tickWater();
	TickAO();
}

void Game::setupWater()
{
	waterText = Texture::readTexture("..\\Textures\\flow\\normal.bmp");
	waterText->load();
	waterID = waterText->textureHandle;

	waterColor = Texture::readTexture("..\\Textures\\flow\\clearback.tga");
	waterColor->load();
	waterColorID = waterColor->textureHandle;

	waterFlow = Texture::readTexture("..\\Textures\\flow\\flow.tga");
	waterFlow->load();
	waterFlowID = waterFlow->textureHandle;

	lavaFlow = Texture::readTexture("..\\Textures\\flow\\lava.tga");
	lavaFlow->load();
	lavaFlowID = lavaFlow->textureHandle;

	buildRawCubeMapFromFile(waterCubeMapID, "..\\Textures\\Flow\\", "sky.tga");

	buildRawCubeMapFromFile(lavaCubeMapID, "..\\Textures\\Flow\\", "lava.tga");

	ShaderForWater = new Shader("hpcv-water-tile");
	ShaderForWater->load();
}

void Game::tickWater()
{
	ticks += DT;
	//set shader uniforms
	ShaderForWater->activate();
	ShaderForWater->setUniform1f("osg_FrameTime", ticks);
	ShaderForWater->setUniformMatrix4fv("osg_ViewMatrixInverse", (float*)(&(camera->cameraMatrix)));

	//setup shader to accept textures
	glActiveTexture(GL_TEXTURE16);
	glBindTexture(GL_TEXTURE_2D, waterID);
	ShaderForWater->setUniformTexture("normalMap", 16);

	glActiveTexture(GL_TEXTURE17);
	glBindTexture(GL_TEXTURE_2D, waterColorID);
	ShaderForWater->setUniformTexture("colorMap", 17);

	glActiveTexture(GL_TEXTURE18);
	glBindTexture(GL_TEXTURE_2D, waterFlowID);
	ShaderForWater->setUniformTexture("flowMap", 18);

	glActiveTexture(GL_TEXTURE19);
	glBindTexture(GL_TEXTURE_CUBE_MAP, waterCubeMapID);
	ShaderForWater->setUniformTexture("cubeMap", 19);

	//ShaderForWater->setUniformTexture("cubeMap", 20);

	glActiveTexture(GL_TEXTURE0);
}

//Glut's idle function.
void Game::TickAO() {
	glutPostRedisplay();

	ticks += DT;
	//set shader uniforms
	buildMRTTexturesShader->activate();
	buildMRTTexturesShader->setUniform1f("osg_FrameTime", ticks);
	buildMRTTexturesShader->setUniformMatrix4fv("osg_ViewMatrixInverse", (float*)(&(camera->cameraMatrix)));

	/*currentTime = glutGet(GLUT_ELAPSED_TIME);
	elapsedTime = currentTime - previousTime;

	MoveCamera();*/
}

void Game::drawTeapots () {
	//Draw 4 teapots around a circle... It's too hard to find just one if the mouse rotates too fast.
	Point locations [] = {Point (2.0, 0.0, 0.0), Point (-2.0, 0.0, 0.0), Point (0.0, 0.0, 2.0),
		Point (0.0, 0.0, -2.0)};
	glColor4d (1.0, 1.0, 0.0, 1.0); //Yellow...

	glDisable (GL_TEXTURE_2D);
	for (long index = 0; index < 4; index++) {
		glPushMatrix ();
			Point &location = locations [index];
			glTranslated (location.x, location.y, location.z); 
			glutSolidTeapot (0.5);
		glPopMatrix ();
	}
	glColor4d (1.0, 1.0, 1.0, 1.0); //White...
}

bool disableShaders = false;
void Game::draw() {
	//If there is no world, draw a teapot; otherwise, draw the world...
	//Neither the input manager nor the camera draws itself...

	camera->beginCamera();

	if (world == NULL)
	{
		drawTeapots();
	}
	else
	{
		switch (drawingChoice)
		{
		case NormalOnly:
			world->draw();

			if (drawFuzzBallsInNormalDraw)
			{
				drawAllLightFuzzBallsInNormalWorld();
			}

			if (drawLightSpheresInNormalDraw)
			{
				drawAllLightSpheresInNormalWorld();
			}
			if (choice == UseWater)
			{
				world->draw();
				tickWater();

				if (!disableShaders){
					ShaderForWater->activate();
				}
				Transformation trans = Transformation::lookAtForObject(Point(-70, -6, 0), Vector(0, -1, 0), Vector(0, 0, -1), Vector(1, 0, 0));
				glPushMatrix();
				glMultMatrixf(trans);
				glScaled(200, 200, 1);
				unitSolidFace->draw();
				glPopMatrix();

				glActiveTexture(GL_TEXTURE19);
				glBindTexture(GL_TEXTURE_CUBE_MAP, lavaCubeMapID);
				trans = Transformation::lookAtForObject(Point(-32.5, 33, -52.6), Vector(0, -1, 0), Vector(0, 0, -1), Vector(1, 0, 0));
				glPushMatrix();
				glMultMatrixf(trans);
				glScaled(9, 9, 1);
				unitSolidFace->draw();
				glPopMatrix();
			}
			if (choice == AO){
				
				TickAO();
				if (!disableShaders){
					buildMRTTexturesShader->activate();
				}
				

				glPushAttrib (GL_VIEWPORT_BIT);  
				glViewport (0, 0, RESOLUTION, RESOLUTION);
				glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				world->draw();

				glPushMatrix();
				glScalef(scale, scale, scale);
				//glCallList(modelList);
				glPopMatrix();

				glPopAttrib();  
				
			}
			break;
		case UseShadows:
			world->draw();
			break;
		case UseIndexedLights:
			drawColoredLights(world);
			break;
		case UseShadowsAndIndexedLights:
			world->draw();
			break;
		}


	}

	player->draw();
	camera->endCamera();
	drawFrameRate();
	drawHelp();
}

void Game::setupFont () {
	deviceContext = GetDC (NULL);
	HFONT font; //Windows font ID...
	fontBase = glGenLists (96); //Storage for 96 characters
	font = CreateFont (	
		-24,							//Height of font
		0,								//Width of font
		0,								//Angle of escapement
		0,								//Orientation angle
		FW_BOLD,						//Font weight
		FALSE,							//Italic
		FALSE,							//Underline
		FALSE,							//Strikeout
		ANSI_CHARSET,					//Character set identifier
		OUT_TT_PRECIS,					//Output precision
		CLIP_DEFAULT_PRECIS,			//Clipping precision
		ANTIALIASED_QUALITY,			//Output quality
		FF_DONTCARE | DEFAULT_PITCH,	//Family and pitch
		"Arial");						//Font name

	SelectObject (deviceContext, font);			//Selects The Font We Want
	wglUseFontBitmaps (deviceContext, 32, 96, fontBase); //Builds 96 characters starting at character 32
}

void Game::wrapupFont () {
	ReleaseDC (NULL, deviceContext);
	glDeleteLists (fontBase, 96); //Delete all 96 characters
}

void Game::privateDrawString (const char *text) {
	glPushAttrib (GL_LIST_BIT);	//Pushes the display list bits
	glListBase (fontBase - 32); //Sets the base character to 32
	glCallLists (strlen (text), GL_UNSIGNED_BYTE, text); //Draws the display list text
	glPopAttrib (); //Pops the display list bits
}

extern long screenWidth, screenHeight;

void Game::begin2DDrawing () {
	glMatrixMode (GL_PROJECTION);
	glPushMatrix ();
		glLoadIdentity ();
		glOrtho (0.0, (GLfloat) screenWidth, 0.0, (GLfloat) screenHeight, -100.0f, 100.0f);
		glMatrixMode (GL_MODELVIEW);
		glPushMatrix ();
			glLoadIdentity ();
}

void Game::end2DDrawing () {
			glMatrixMode (GL_PROJECTION);
		glPopMatrix ();
		glMatrixMode (GL_MODELVIEW);
	glPopMatrix ();
}

void Game::drawMessage (long x, long y, const char *message, ...) {
	char text [1000]; va_list parameters;									
	if (message == NULL) return;

	//Extract variable length parameters and copy into text as in printf.
	va_start (parameters, message);					
	    vsprintf (text, message, parameters);		
	va_end (parameters);

	//Determine the end of the string and convert each occurrence of '\n' to '\0'.
	char *end = text + strlen (text);
	for (char *next = text; *next != '\0'; next++) {
		if (*next == '\n') *next = '\0';
	}

	//Draw the multi-line message...
	glUseProgram (0); //Use no shader...
	begin2DDrawing ();
		glDepthFunc (GL_ALWAYS);
		glDisable (GL_TEXTURE_2D);
		glDisable (GL_LIGHTING);
		glColor4f (1.0f, 1.0f, 1.0f, 1.0f);
		long yOffset = y;
		for (char *line = text; line < end; line += strlen (line) + 1) {
			glRasterPos2i (x, yOffset); yOffset -= 32;
			privateDrawString (line);
		}
		glDepthFunc (GL_LEQUAL);
		if (useLights) glEnable (GL_LIGHTING);
	end2DDrawing ();
}

void Game::drawFrameRate () {
	//Draw the frame rate avoiding extreme fluctuations (since all you see is flickering).
	double frameRate = 1.0 / DT; //Frames/sec = 1/(seconds per frame).
	static double stableRate = frameRate; //This initializes only the first time...
	static double oldFrameRate = frameRate;
	//If it changed by more than 2 per cent of the stable value, use the new value; otherwise use the stable one...
	if (absolute (frameRate - stableRate) > 4.0) stableRate  = frameRate; 

	Point position = player != NULL ? player->position () : Zero; 
	char *positionString = player != NULL ? asString (" at [%3.2f,%3.2f,%3.2f]", position.x, position.y, position.z) : "";
	//drawMessage (1, screenHeight-20, "angle %3.1f FPS: %3.1f%s %s", shaderManager->xAngle, stableRate, positionString, 
	//	game == NULL || game->world == NULL ? "" : (game->flyModeOn ? "FLYING" : "COLLISION DETECTION"));
	drawMessage (1, screenHeight-20, "FPS: %3.1f%s %s", stableRate, positionString, 
		game == NULL || game->world == NULL ? "" : (game->flyModeOn ? "FLYING" : "COLLISION DETECTION"));
}



void Game::drawNote (const char *message, ...) {
	char text [1000]; va_list parameters;									
	if (message == NULL) return;

	//Extract variable length parameters and copy into text as in printf.
	va_start (parameters, message);					
	    vsprintf (text, message, parameters);		
	va_end (parameters);

	drawMessage (1, screenHeight-52, "%s", text);
}

void Game::drawHelp () {
	if (helpOn) {
		drawMessage (1, screenHeight-50, "%s", "W - move forward");
		drawMessage (1, screenHeight-80, "%s", "S - move back");
		drawMessage (1, screenHeight-110, "%s", "A - move left");
		drawMessage (1, screenHeight-140, "%s", "D - move right");
		drawMessage (1, screenHeight-170, "%s", "Q - move up");
		drawMessage (1, screenHeight-200, "%s", "E - move down");
		drawMessage (1, screenHeight-230, "%s", "f - toggle flymode");
		drawMessage (1, screenHeight-260, "%s", "t - toggle wireframe");
		drawMessage (1, screenHeight-290, "%s", "1 - throw cube");
		drawMessage (1, screenHeight-320, "%s", "? - toggle help");
	}
}


Shader *shadowDrawWhatLightSees = NULL;

void Game::setupShadows() {
	shadowDrawWhatLightSees = new Shader("shadowDrawWhatLightSees");
	shadowDrawWhatLightSees->load();

	buildRawFrameBuffers(1, &shadowMapFrameBufferID);
	buildRawShadowMapDepthBuffer(1, &shadowMapDepthBufferID, screenWidth, screenHeight);
}
/*
Shader *shadowDrawWhatLightSees = NULL;
Shader *shadowDrawWorldWithOneSpotlight = NULL;
void setupShadows(){
	shadowDrawWhatLightSees = new Shader("shadowDrawWhatLightSees");
	shadowDrawWorldWithOneSpotlight = new Shader("shadowDrawWorldWithOneSpotLight");
	shadowDrawWhatLightSees->load();
	shadowDrawWorldWithOneSpotlight->load();

	buildRawFrameBuffers(1, &shadowMapFrameBufferID);
	buildRawShadowMapDepthBuffer(1, &shadowMapDepthBufferID, screenWidth, screenHeight);
	//glBindFramebuffer(GL_FRAMEBUFFER,shadowMapFrameBufferID); //something is wrong with these 3 lines, it makes the program draw nothing.
	//attachShadowMapDepthTexture(shadowMapDepthBufferID);

	//glDrawBuffers(0,NULL);
}*/

void Game::tickShadowedLights()
{

}

void Game::wrapupShadowedLights()
{

}



#define LEVEL_COUNT 1
#define ORIGINAL_VERSION 0
#define WILF_VERSION 1
#define DISABLE_TEMPORAL_BLENDING 1
#define DISCARD_UNUSED_UNIFORMS 1

//#define WILF_version ORIGINAL_VERSION
#define WILF_version WILF_VERSION
#define PI 3.14159265

//Shaders...
Shader *downsampleShader[LEVEL_COUNT];
Shader *upsampleShader[LEVEL_COUNT];
Shader *sharpenShader[LEVEL_COUNT];

// shader programs
GLuint buildMRTTexturesProgram;
GLuint downsampleProgram[LEVEL_COUNT];
GLuint aoProgram[LEVEL_COUNT]; 
GLuint aoSharpenProgram[LEVEL_COUNT];


float gluOrtho[LEVEL_COUNT][16]; // orthogonal projection matrix

#if (DISABLE_TEMPORAL_BLENDING) 
#else
const float dMax = 2.5f; // AO radius of influence (wilf: maximum camera-space sampling radius)
float iMVMat[16]; // inverse model-view matrix
float mVMat[16]; // model-view matrix
#endif //DISABLE_TEMPORAL_BLENDING
float projMat[16]; // perspective projection matrix


const float rMax = 7.0; // maximum screen-space sampling radius
const float fov = 65.238f;
const float zNear = 0.1f;
const float zFar = 1000.0f;

void SetupMRTBuildingShaders() {
	Shader *shader = buildMRTTexturesShader = new Shader("../Shaders/AOshaders/buildMRTTextures");
	const char *defaultMRT[] = { "Pos", "Norm" };
	shader->load();//defaultAttributes, 3, defaultMRT, 2);
	buildMRTTexturesProgram = shader->handle();
}

//Setups the shader programs used in the downsampling step.
void SetupDownsamplingShaders() {
	for (int i = 1; i < LEVEL_COUNT; ++i) {
		Shader *shader = downsampleShader[i] = new Shader("../Shaders/AOshaders/downsample");
		const char *defaultMRT[] = { "Pos", "Norm" };
		shader->load();// defaultAttributes, 3, defaultMRT, 2);
		downsampleProgram[i] = shader->handle();

		//Shader output colors are called "Pos" and "Norm". 
		shader->setUniformTexture("hiResPosTex", 0);
		shader->setUniformTexture("hiResNormTex", 1);
		shader->setUniformMatrix4fv("gluOrtho", gluOrtho[i]);
	}
}

struct ShaderSetting {
	bool usePoisson; bool useUpsampling; bool useTemporalSmoothing;
};

ShaderSetting computeShaderSetting(long levelCount) {
	ShaderSetting setting;
	setting.usePoisson = levelCount == 0;
	setting.useUpsampling = levelCount < (LEVEL_COUNT - 1);
	setting.useTemporalSmoothing = true;
	return setting;
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
		ShaderSetting setting = computeShaderSetting(i); //WILF ADDITION
		shader->setUniform1f("usePoisson", setting.usePoisson); //WILF ADDITION
		shader->setUniform1f("useUpsampling", setting.useUpsampling); //WILF ADDITION
		shader->setUniform1f("useTemporalSmoothing", setting.useTemporalSmoothing); //WILF ADDITION
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

//Buffers and textures ([0] = highest resolution)
GLuint frameBuffers[LEVEL_COUNT];
GLuint depthBuffers[LEVEL_COUNT];
GLuint posTex[LEVEL_COUNT];
GLuint normTex[LEVEL_COUNT];
GLuint aoTex[LEVEL_COUNT];
GLuint aoTexSharpen[LEVEL_COUNT];


//Check the FBO status.
void checkActiveFrameBufferStatus() {
	GLenum FBOstatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (FBOstatus == GL_FRAMEBUFFER_COMPLETE) return;
	::halt("\nGL_FRAMEBUFFER_COMPLETE test failed, CANNOT use FBO.");
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
		checkActiveFrameBufferStatus();
	}
}

void Game::setupAO(){
	//SetupFBOs();
	SetupMRTBuildingShaders();
	SetupDownsamplingShaders();
	SetupUpsamplingShaders();
	SetupAOSharpenPrograms();
}

void Game::wrapupAO(){
	log("\nExiting...\n");
	delete buildMRTTexturesShader;
	for (long index = 0; index < LEVEL_COUNT; index++) {
		delete downsampleShader[index];
		delete upsampleShader[index];
		delete sharpenShader[index];
	}

	#if (DISABLE_TEMPORAL_BLENDING)
		//Eliminate last frames
	#else
		glDeleteTextures(1, &lastFrameAOTex);
		glDeleteTextures(1, &lastFramePosTex);
	#endif //DISABLE_TEMPORAL_BLENDING

	#if (DISCARD_UNUSED_UNIFORMS) 
	#else
		for (int i = 0; i < LEVEL_COUNT; ++i) { delete[] randRot[i]; }
	#endif //DISCARD_UNUSED_UNIFORMS
}


//Creates orthographic projection matrices.
void Game::BuildOrthographicMatrices() {
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