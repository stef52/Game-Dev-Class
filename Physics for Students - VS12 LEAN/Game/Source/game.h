//*****************************************************************************************//
//                                        Game                                             //
//*****************************************************************************************//

#ifndef gameModule
#define gameModule 

extern double DT; //Elapsed time since previous tick/draw....

class Game {
public:
	Game (): helpOn (false), flyModeOn (true) {world = NULL;};
	~Game () {};

	World *world;

	static HDC deviceContext; static GLuint fontBase; 

	static void setup ();
	static void wrapup ();

	void tick ();
	void draw ();

	static void setupFont ();
	static void wrapupFont ();

	void drawMessage (long x, long y, const char *message, ...);
	void drawFrameRate ();
	void drawNote (const char *message, ...);
	void drawHelp ();

	void drawTeapots ();
	void displayHelp (bool helpOn) { this->helpOn = helpOn;};

	bool flyModeOn;
private:
	
	bool helpOn;
	void drawText (const char *message, ...);
	void begin2DDrawing (); void end2DDrawing (); 
	void privateDrawString (const char *text);
};


extern Game *game;
extern bool disableShaders;
#define DISABLE_SHADERS disableShaders = true;
#define ENABLE_SHADERS disableShaders = false;

#endif //gameModule