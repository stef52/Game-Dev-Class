
//*****************************************************************************************//
//                                      Includes                                           //
//*****************************************************************************************//

#include "includes.all"

//*****************************************************************************************//
//                                     InputManager                                        //
//*****************************************************************************************//

const double InputManager::translationSpeed = 10.0; //meters per second.
const double InputManager::rotationSpeed = 120.0; //degrees per second

InputManager *inputManager = NULL;

void InputManager::setup () {
	inputManager = new InputManager;
}

void InputManager::wrapup () {
	delete inputManager;
} 

void InputManager::tick () {
	//::log ("\nInputManager tick Translate Left %s Right %s Down %s Up %s Ahead %s Back; Rotate Left %s Right %s Down %s Up %s", 
	//	(translateLeft ? "T" : "F"),  (translateRight ? "T" : "F"),
	//	(translateDown ? "T" : "F"), (translateUp ? "T" : "F"),
	//	(translateAhead ? "T" : "F"), (translateBack ? "T" : "F"), 
	//	(rotateLeft ? "T" : "F"),  (rotateRight ? "T" : "F"),
	//	(rotateDown ? "T" : "F"), (rotateUp ? "T" : "F"));

	//Deal with control keys... 
	Point translation (
		(translateLeft ? -translationSpeed : 0.0) + (translateRight ? translationSpeed : 0.0),
		(translateDown ? -translationSpeed : 0.0) + (translateUp ? translationSpeed : 0.0),
		(translateAhead ? -translationSpeed : 0.0) + (translateBack ? translationSpeed : 0.0));

	Point rotation (
		(rotateDown ? -rotationSpeed : 0.0) + (rotateUp ? rotationSpeed : 0.0),
		(rotateRight ? -rotationSpeed : 0.0) + (rotateLeft ? rotationSpeed : 0.0),
		0.0);

	rotateBy (rotation * DT); //degrees = degrees per second * second
	moveBy (translation * DT); //meters = meters per second * meters
}
