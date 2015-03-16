
//95.4002 Wilf LaLonde: Student game engine.

//*****************************************************************************************//
//                                      Includes                                           //
//*****************************************************************************************//

#include "includes.all"

//*****************************************************************************************//
//                                        Main                                             //
//*****************************************************************************************//

const bool enableCursorCentering = true;

enum MenuItem {DifficultyEasy, DifficultyChallenging, DifficultyImpossible,
	DisplayModeWireframe, DisplayModeTextured, RunWorld, Quit};

MenuItem difficulty; MenuItem displayMode;
bool wireframe = false, fullscreen = false; long screenWidth, screenHeight;
bool help = false;
char filename [256];

void resizeWindow (int width, int height) {
	//Setup a new viewport.
	glViewport (0, 0, width, height);
	screenWidth = width; screenHeight = height;
	::log ("\nViewport size %dx%d.", width, height);

	//Setup a new perspective matrix.
	GLdouble verticalFieldOfViewInDegrees = 40;
	GLdouble aspectRatio = height == 0 ? 1.0 : (GLdouble) width / (GLdouble) height;
	GLdouble nearDistance = 1.0;
	GLdouble farDistance = 2000.0;

	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	gluPerspective (verticalFieldOfViewInDegrees, aspectRatio, nearDistance, farDistance);

	//Get back to default mode.
	glMatrixMode (GL_MODELVIEW);
}

inline void computeDT () {
	//Compute elapsed time needed for controlling frame rate independent effects.
	//If running slower than 5 frames per second, pretend it's 5 frames/sec.
	//Note: 30 frames per second means 1/30 seconds per frame = 0.03333... seconds per frame.
	static double lastTimeInSeconds = timeNow () - 0.033; //Pretend we are running 30 frames per second on the first tick.
	double timeInSeconds = timeNow ();
	DT = timeInSeconds - lastTimeInSeconds;
	if (DT > 0.2) DT = 0.2; //5 frames/sec means 1 frame in 1/5 (= 0.2) seconds.
	lastTimeInSeconds = timeInSeconds;
}

void displayWindow () {
	if (game == NULL) return;
//glClearColor (0.0, 0.0, 0.0, 1.0); //black
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		//WILF PhysX... Looks like a good place to run the simulation... If you don't have a physics scene, don't to it... Also, DT is available here....
		CAUSE_A_COMPILER_ERROR;
		game->draw ();
	glutSwapBuffers ();
}

void idle () {
	if (game == NULL) return;
	computeDT ();
	game->tick ();
	//::log ("\nSet mouse to %d@%d", screenWidth >> 1, screenHeight >> 1); 
	if (enableCursorCentering) SetCursorPos (screenWidth >> 1, screenHeight >> 1); //Re-center the mouse...
	glutPostRedisplay ();
}

void visibilityChanged (int visible) {
	glutIdleFunc (visible == GLUT_VISIBLE ? idle : NULL);
	//::log ("\nVISIBLE IS %s.", visible == GLUT_VISIBLE ? "true" : "false");
}

inline bool keyIsDown (long key) {return (GetAsyncKeyState (key) & 0x8000) != 0;}
#define VK_ALT VK_MENU

void specialKeyPressed (int character, int x, int y) {
	switch (character) {
		case GLUT_KEY_F1:
			break;
		case GLUT_KEY_F2:
			break;
		case GLUT_KEY_UP: {
			//A partial example...
			if (keyIsDown (VK_CONTROL)) {
			} else if (keyIsDown (VK_ALT)) {
			} else {
			}
			break;}
		case GLUT_KEY_DOWN: {
			break;}
		case GLUT_KEY_RIGHT:
			break;
		case GLUT_KEY_LEFT:
			break;
		case GLUT_KEY_PAGE_UP:
			break;
		case GLUT_KEY_PAGE_DOWN:
			break;
		default: 
			//The log allows you to figure out what key something actually is!!!
			::log ("\nPressed special key consisting of character '%c' decimal %d hex %x.", character, character, character);
			break;
	}
	glutPostRedisplay ();
}

void specialKeyReleased (int character, int x, int y) {
	switch (character) {
		case GLUT_KEY_F1:
			break;
		default: 
			//The log allows you to figure out what key something actually is!!!
			::log ("\nReleased special key consisting of character '%c' decimal %d hex %x.", character, character, character);
			break;
	}
	glutPostRedisplay ();
}
    
#define escapeCharacter 27
#define enterCharacter 13

void normalKeyPressed (unsigned char character, int x, int y) {
	//Handle the key and then force a redisplay.
	switch (character) {
		case escapeCharacter:
			Game::wrapup (); 
			exit (0);
		case ' ': case enterCharacter:
			//Open door perhaps?
			break;
		case '1': Cube::playerThrowCube (); break;
		case '0':
//		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9': {
			//Run some tests perhaps?
			long code = character - '0';
			break;}

		//Translate w/s for forward/back, a/d for left/right, e/q for up/down.
		//Rotate z/c for left/right, r/v for up/down.
		case 'w':
		case 'W':
			inputManager->translateAhead = true;
			break;
		case 's':
		case 'S':
			inputManager->translateBack = true;
			break;

		case 'a':
		case 'A':
			inputManager->translateLeft = true;
			break;
		case 'd':
		case 'D':
			inputManager->translateRight = true;
			break;

		case 'e':
		case 'E':
			inputManager->translateUp = true;
			break;
		case 'q':
		case 'Q':
			inputManager->translateDown = true;
			break;

		case 'f':
		case 'F':
			game->flyModeOn = game->flyModeOn == true ? false : true;
			break;

		case 'z':
		case 'Z':
			inputManager->rotateLeft = true;
			break;

		case 'c':
		case 'C':
			inputManager->rotateRight = true;
			break;

		case 'r':
		case 'R':
			inputManager->rotateUp = true;
			break;
		case 'v':
		case 'V':
			inputManager->rotateDown = true;
			break;

		case 'u':
		case 'U':
			break;

		case 't':
		case 'T':
			wireframe = !wireframe;
			glPolygonMode (GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
			break;

		case '?':
			// Start displaying help
			help = help == true ? false : true;
			game->displayHelp (help);			
			break;

		default:
			//The log allows you to figure out what key something actually is!!!
			::log ("\nPressed normal key consisting of character '%c' decimal %d hex %x.", character, character, character);
			break;
	}

	glutPostRedisplay ();
}

void normalKeyReleased (unsigned char character, int x, int y) {
	//Handle the key and then force a redisplay.
	switch (character) {
		case escapeCharacter:
			Game::wrapup (); 
			exit (0);

		//Translate w/s for forward/back, a/d for left/right, e/q for up/down.
		//Rotate z/c for left/right, r/v for up/down.
		case 'w':
		case 'W':
			inputManager->translateAhead = false;
			break;
		case 's':
		case 'S':
			inputManager->translateBack = false;
			break;

		case 'a':
		case 'A':
			inputManager->translateLeft = false;
			break;
		case 'd':
		case 'D':
			inputManager->translateRight = false;
			break;

		case 'e':
		case 'E':
			inputManager->translateUp = false;
			break;
		case 'q':
		case 'Q':
			inputManager->translateDown = false;
			break;

		case 'z':
		case 'Z':
			inputManager->rotateLeft = false;
			break;

		case 'c':
		case 'C':
			inputManager->rotateRight = false;
			break;

		case 'r':
		case 'R':
			inputManager->rotateUp = false;
			break;
		case 'v':
		case 'V':
			inputManager->rotateDown = false;
			break;
		default: 
			//The log allows you to figure out what key something actually is!!!
			::log ("\nReleased normal key consisting of character '%c' decimal %d hex %x.", character, character, character);
			break;
	}

	glutPostRedisplay ();
}

void mousePressed (int button, int state, int x, int y) {
	if (button == GLUT_LEFT_BUTTON) {
		if (state == GLUT_DOWN) {
			//Start something at mouse coordinates x and y.
		} else {//state == GLUT_UP
			//End something at mouse coordinates x and y.
		}
	} else if (button == GLUT_MIDDLE_BUTTON) {
		if (state == GLUT_DOWN) {
			//Start something at mouse coordinates x and y.
		} else {//state == GLUT_UP
			//End something at mouse coordinates x and y.
		}
	} else {//button == GLUT_RIGHT_BUTTON
		if (state == GLUT_DOWN) {
			//Start something at mouse coordinates x and y.
		} else {//state == GLUT_UP
			//End something at mouse coordinates x and y.
		}
	}
	::log ("\nPressed %s mouse button at %d@%d.", button == GLUT_LEFT_BUTTON ? "LEFT" : 
		(button == GLUT_MIDDLE_BUTTON ? "MIDDLE" : "RIGHT"), x, y);
}

void mouseMoved (int x, int y) {
	if (game == NULL) return;
	//Note: Because we are re-centering the mouse on each tick (see function "idle ()"), here
	//we are determining how far the mouse was moved from the center point and we use this
	//to determine both (1) x-rotation amounts (using the vertical displacement) which we store
	//in rotation.x and (2) y-rotation amounts (using the horizontal displacement) which we store
	//in rotation.y. 
	
	//Keep in mind that in windows, x increases going right and y increases going down... 
	
	//So to rotate so as to look up by moving the mouse up, we get a negative change in y but a 
	//rotation to look up requires that the y-axis rotate toward the z-axis (a positive rotation). 
	//So we store the negative of this amount in rotation.x.

	//Similarly, to rotate so as to look right by moving the mouse right, we get a positive change
	//in x. But rotating the z-axis toward the x-axis is a positive rotation and this causes
	//more of the left to be seen, so we actually want to rotate by the negative of this amount
	//which we store in rotation.y.
	
	//::log ("\nMoved mouse to %d@%d.", x, y);
	POINT screenPOINT; GetCursorPos (&screenPOINT); 
	Point point (screenPOINT.x, screenPOINT.y, 0.0);
	Point center (screenWidth >> 1, screenHeight >> 1, 0.0);

	double sensitivity = 0.1;
	Point difference = (point - center) * sensitivity;
	Point rotation (-difference.y, -difference.x, 0.0);

	inputManager->rotateBy (rotation * (InputManager::rotationSpeed * DT)); //degrees = degrees per second * second
}

#define matches(a,b) strlen (b) >= strlen (a) && memcmp (a,b,strlen (a)) == 0

void parseParameters (int parametersSize, char **parameters) {
	bool optionFound = false;
	for (long i = 1; i < parametersSize; i++) {//Skip program name.
		::log ("\n%d: Consider \"%s\".", i, parameters [i]);
		if (matches ("-w", parameters [i])) {
			::log ("\nMatched -w");
			wireframe = true; optionFound = true;
		} else if (matches ("-f", parameters [i])) {
			::log ("\nMatched \"-f\", extracting from \"%s\".", parameters [i]);
			::log ("\nAssigning into filename [0]");
			filename [0] = 20;
			::log (" WORKED...");
			strcpy (filename, parameters [i]+2);
			::log (" COPY WORKED TOO...");
			::log ("\nRead filename \"%s\".", filename);
			optionFound = true;
		} else 
			::log ("\nFailed to match.");
	}
	if (!optionFound) {
		printf ("\nusage: builder [-options] -fFilename");
		printf ("\n  -w         :: wireframe");
		printf ("\n  -f         :: file name; e.g., -fc:\\test\\data");
		exit (1);
	}
}

void genericMenuHandler (int item) {
	switch (item) {
		case RunWorld: 
			if (game->world != NULL) {delete game->world; game->world = NULL;}
			game->world = World::read (); 
			break;
		case Quit:
			Game::wrapup (); exit (0);
			break;
		default:
			::log ("\nUnknown generic menu selection %d (%x).", item, item);
	}
}

void difficultyMenuHandler (int item) {
	switch (item) {
		case DifficultyEasy:
		case DifficultyChallenging:
		case DifficultyImpossible:
			difficulty = (MenuItem) item; 
			break;
		default:
			::log ("\nUnknown difficulty %d (%x).", item, item);
	}
}

void displayModeMenuHandler (int item) {
	switch (item) {
		case DisplayModeWireframe:
			wireframe = true;
			glPolygonMode (GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
			break;
		case DisplayModeTextured:
			wireframe = false;
			glPolygonMode (GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
			break;
		default:
			::log ("\nUnknown display mode %d (%x).", item, item);
	}
    glutPostRedisplay ();
}

void createMenus () {
  int difficultyMenu = glutCreateMenu (difficultyMenuHandler);
  glutAddMenuEntry ("Easy", DifficultyEasy);
  glutAddMenuEntry ("Hard", DifficultyChallenging);
  glutAddMenuEntry ("Impossible", DifficultyImpossible);

  int displayModeMenu = glutCreateMenu (displayModeMenuHandler);
  glutAddMenuEntry ("Wireframe", DisplayModeWireframe);
  glutAddMenuEntry ("Textured", DisplayModeTextured);

  int genericMenu = glutCreateMenu (genericMenuHandler);
  glutAddSubMenu ("Difficulty...", difficultyMenu);
  glutAddSubMenu ("Mode...", displayModeMenu);
  glutAddMenuEntry ("Run World", RunWorld);
  glutAddMenuEntry ("Quit", Quit);

  glutAttachMenu (GLUT_RIGHT_BUTTON);
}

int main (int parametersSize, char **parameters) {
	//Welcome...
	clearLog (); ::log ("\n\nStarting game...");

	//Process the command line (if any)...
	//parseParameters (parametersSize, parameters); //Better to prompt from a menu...

	//Setup general facilities.
	glutInitDisplayMode (GLUT_RGBA | GLUT_ALPHA | GLUT_DEPTH | GLUT_DOUBLE | GLUT_STENCIL | GLUT_MULTISAMPLE);
	glutInitWindowSize (800, 800);
	glutInit (&parametersSize, parameters);
	if (fullscreen) /*glutEnterGameMode ()*/; else {glutCreateWindow ("game engine"); createMenus ();}
    glutIgnoreKeyRepeat (GLUT_KEY_REPEAT_ON);
    glutSetCursor (GLUT_CURSOR_NONE);

	//Specify function handlers.
	glutDisplayFunc (displayWindow); glutReshapeFunc (resizeWindow); glutKeyboardFunc (normalKeyPressed);
	glutSpecialFunc (specialKeyPressed); glutMouseFunc (mousePressed); glutMotionFunc (mouseMoved);
	glutVisibilityFunc (visibilityChanged);
    glutPassiveMotionFunc (mouseMoved); 
    glutSpecialUpFunc (specialKeyReleased);
    glutKeyboardUpFunc (normalKeyReleased);
    
	//Setup our own facilities.
	Game::setup ();
	const bool logAllExtensions = false; logExtensions (logAllExtensions);
	::log ("\nRunning OpenGL version %s, shader version %s.", glGetString (GL_VERSION), glGetString (GL_SHADING_LANGUAGE_VERSION));
	int units1; glGetIntegerv (GL_MAX_TEXTURE_UNITS_ARB, &units1);
	int units2; glGetIntegerv (GL_MAX_TEXTURE_IMAGE_UNITS, &units2);
	int uniforms; glGetIntegerv (GL_MAX_VERTEX_UNIFORM_COMPONENTS, &uniforms);
	::log ("\nwith %d texture units; %d for shaders; %d uniforms...", units1, units2, uniforms);

	//Rumble
	glutMainLoop ();

	//Cleanup
	Game::wrapup ();
	return 0;
}
