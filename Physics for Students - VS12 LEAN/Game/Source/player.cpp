
//*****************************************************************************************//
//                                      Includes                                           //
//*****************************************************************************************//

#include "includes.all"

//*****************************************************************************************//
//                                       Player                                            //
//*****************************************************************************************//

Player *player = NULL;

void Player::setup () {
	player = new Player;
}

void Player::wrapup () {
	delete player;
} 

Player::Player () {
	//WILF PhysX...
	playerSphere = physicsManager->physicsSphere (Point (0.0, 0.0, 0.0), Vector (0.0, 0.0, 0.0), 0.5, 1.0); //diameter in meters, mass in kilograms
}

Player::~Player () {
	//WILF PhysX...
	if (playerSphere != NULL) playerSphere->release ();
}

//#define EXTRA 0.25

Point Player::asFarAsPossibleGo (Point &from, Point &to, double EXTRA) const {
	//Note that the physics manager allows you to move further inside if you previously moved to it's hit point...
	//Apparently, a hit point must be inside by an epsilon and movement if already inside is allowed...

	//To prevent the above from happening, we will try to move further than we want by amount EXTRA and then
	//come back by EXTRA whether or not there is a hit... That way, we will always be OUTSIDE at least by EXTRA...

	float directionLength; Vector direction = (to - from).normalized (directionLength);
	if (directionLength < epsilon ()) return from; //But don't move at all if its an epsilon amount...

	Vector extra = direction * EXTRA;

	//Note that intersectionPoint is either where we hit having failed to go all the way OR the TO point that we provided if there is no hit...
	double intersectionT; Point intersectionPoint;
	bool hit = physicsManager->sweptSphereHits (playerSphere, from, to + extra, intersectionT, intersectionPoint);

	//Note that we commented out the ray versions since it's not as good... It would allow us to go into a keyhole or crack in the wall...
	//bool hit = physicsManager->sweptRayHits (from, to + extra, intersectionT, intersectionPoint);
	return intersectionPoint - extra;
}

void Player::tick () {
	//Deal with the delta accumulated since the last tick...
	const bool usingPhysics = true; //WILF PhysX... (In case, you need to disable your collision detection)...

	if (game->world == NULL) return;
	if (game->flyModeOn || !usingPhysics) {//Allow controls to do everything including up and down...
		playerMatrix.multiply (delta, playerMatrix); delta.setToIdentity (); return;
	} 

	//Strategy: Move straight to goal, then straight down for gravity... 
	//Since the sphere radius is 0.25, downExtraDelta needs to be at least that to be able to climb stairs...
	const double toGoalExtraDelta = 0.25; const double downExtraDelta = 0.3; //See EXTRA in asFarAsPossibleGo for an explanation...

	DualTransformation goal; goal.multiply (delta, playerMatrix);
	Point from = playerMatrix.position (); Point to = goal.position ();

	//Move straight to goal...
	Point step1 = asFarAsPossibleGo (from, to, toGoalExtraDelta);

	//Move straight down for gravity...
	const double fallingSpeed = -5.0; //meters per second...
	Point gravityStep = Point (0, fallingSpeed * DT, 0); //Real gravity is not game-like...
	Point step2 = asFarAsPossibleGo (step1, step1 + gravityStep, downExtraDelta);

	goal.translateTo (step2);

	//Record the result and prepare for the next tick...
	playerMatrix = goal; delta.setToIdentity ();
}

void Player::draw () {
	//In first person mode, the player is invisible but not in third person mode.
	//You might also consider drawing a cursor in the center of the screen.
}

void Player::beginCamera () {
	glPushMatrix ();
		glMultMatrixf (playerMatrix.inverse);
}

void Player::endCamera () {
	glPopMatrix (); //Match indentation in beginCamera.
}

void Player::moveBy (Point &translation) {
	const bool logging = false;
	if (logging) {
		if (absolute (translation.x) > epsilon () || absolute (translation.y) > epsilon () || absolute (translation.z) > epsilon ())
			::log ("\nPlayer told to translate by [%3.4f,%3.4f,%3.4f]", translation.x, translation.y, translation.z); //translation.log ();
		//::log ("\nTransformation before:"); playerMatrix.normal ().log (1);
	}
	//playerMatrix.translateBy (translation);
	delta.translateBy (translation);
	//if (logging) {::log ("\nTransformation after:"); playerMatrix.normal ().log (1);}
}

void Player::rotateBy (Point &rotation) {
	const bool logging = false;
	if (logging) {
		if (absolute (rotation.x) > epsilon () || absolute (rotation.y) > epsilon () || absolute (rotation.z) > epsilon ())
		::log ("\nPlayer told to rotate by [%3.4f,%3.4f,%3.4f]", rotation.x, rotation.y, rotation.z); //rotation.log ();
	}
	this->rotation = rotation;
	//playerMatrix.rotateBy (rotation); 
	delta.rotateBy (rotation);
}

void Player::reset (Point &worldPosition) {
	playerMatrix.setToIdentity ();
	playerMatrix.preTranslateBy (worldPosition); 
	delta.setToIdentity ();
	::log ("\nPlayer reset to "); worldPosition.log ();
}
