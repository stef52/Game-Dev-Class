//*****************************************************************************************//
//                                       InputManager                                      //
//*****************************************************************************************//

//Passes the buck to the camera who then passes the buck to the player... Alternatively,
//it could pass different things to the camera and player...

#ifndef inputManagerModule
#define inputManagerModule 

class InputManager {
public:
	InputManager () {
		translateLeft = translateRight = translateAhead = translateBack = translateUp = translateDown = false;
		rotateLeft = rotateRight = rotateUp = rotateDown  = false;
	};
	~InputManager () {};

	static const double translationSpeed; //meters per second.
	static const double rotationSpeed; //degrees per second

	bool translateLeft, translateRight, translateAhead, translateBack, translateUp, translateDown;
	bool rotateLeft, rotateRight, rotateUp, rotateDown;	

	static void setup ();
	static void wrapup ();

	void tick (); //But there is no draw... If you did, what would it look like?
	
	//Movement functions that pass the buck to the camera...
	void moveBy (Point &translation) {camera->moveBy (translation);}
	void rotateBy (Point &rotation) {camera->rotateBy (rotation);}
};

extern InputManager *inputManager;

#endif //inputManagerModule