
//*****************************************************************************************//
//                                      Includes                                           //
//*****************************************************************************************//

#include "includes.all"

//*****************************************************************************************//
//                                   Physics Object                                        //
//*****************************************************************************************//

void Cube::draw () {
	//WILF: The OpenGL default shader can't deal with the new way of defining vertex attributes. So it draws only color...
	extern bool disableShaders; if (!disableShaders) defaultShader->activate (); //glUseProgram (0);

	physicsManager->thrownObjectsTexture->activate ();
	glPushMatrix ();
		glMultMatrixf (transformation ());
		glScaled (width, depth, height);
		unitSolidCube->draw ();
	glPopMatrix ();
}

void Cube::playerThrowCube () {
	if (game->world == NULL || physicsManager->scene == NULL) return;
	Vector aheadAndUp = camera->raisedHeading ();
	float speed = 10; //meters per second...
	::log ("\nThrow cube heading "); aheadAndUp.log (); 
	Cube *cube = new Cube (camera->position (), aheadAndUp * speed, 0.5, 0.5, 0.5, 1.0); //width, height, depth, mass
	physicsManager->add (cube->physicsCube);
	game->world->addDynamicObject (cube); //The world now owns the cube and will tick, draw, and delete it.
}