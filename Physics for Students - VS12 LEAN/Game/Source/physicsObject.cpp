
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

void Sphere::draw() {
	//WILF: The OpenGL default shader can't deal with the new way of defining vertex attributes. So it draws only color...
	extern bool disableShaders; if (!disableShaders) defaultShader->activate(); //glUseProgram (0);

	physicsManager->thrownObjectsTexture->activate();
	glPushMatrix();
		glMultMatrixf(transformation());
		glScaled(diameter, diameter, diameter);
		unitSolidSphere->draw();
	glPopMatrix();
}

void Sphere::playerThrowSphere() {
	if (game->world == NULL || physicsManager->scene == NULL) return;
	Vector aheadAndUp = camera->raisedHeading();
	float speed = 10; //meters per second...
	::log("\nThrow sphere heading "); aheadAndUp.log();
	Sphere *sphere = new Sphere(camera->position(), aheadAndUp * speed, 0.5, 1.0); //diameter, mass
	physicsManager->add(sphere->physicsSphere);
	game->world->addDynamicObject(sphere); //The world now owns the sphere and will tick, draw, and delete it.
}

void Building::setUp()
{
	Vector velo = Point(0, 0, 0);
	for (int j = 0; j < 10; j++)
	{
		//wall
		for (int i = 0; i < 10; i++)
		{
			Cube *cube = new Cube(Point(15 + i, -4.3 + j, 5), velo, 1.0, 1.0, 1.0, 0.1); //width, height, depth, mass
			physicsManager->add(cube->physicsCube);
			game->world->addDynamicObject(cube); //The world now owns the cube and will tick, draw, and delete it.
			cube->physicsCube->putToSleep();
		}
		//wall
		for (int i = 0; i < 10; i++)
		{
			Cube *cube = new Cube(Point(15, -4.3 + j, 5 + i), velo, 1.0, 1.0, 1.0, 0.1); //width, height, depth, mass
			physicsManager->add(cube->physicsCube);
			game->world->addDynamicObject(cube); //The world now owns the cube and will tick, draw, and delete it.
			cube->physicsCube->putToSleep();
		}
		//wall
		for (int i = 0; i < 10; i++)
		{
			Cube *cube = new Cube(Point(15 + i, -4.3 + j, 14), velo, 1.0, 1.0, 1.0, 0.1); //width, height, depth, mass
			physicsManager->add(cube->physicsCube);
			game->world->addDynamicObject(cube); //The world now owns the cube and will tick, draw, and delete it.
			cube->physicsCube->putToSleep();
		}
		//wall
		for (int i = 0; i < 10; i++)
		{
			Cube *cube = new Cube(Point(24, -4.3 + j, 5 + i), velo, 1.0, 1.0, 1.0, 0.1); //width, height, depth, mass
			physicsManager->add(cube->physicsCube);
			game->world->addDynamicObject(cube); //The world now owns the cube and will tick, draw, and delete it.
			cube->physicsCube->putToSleep();
		}
		//roof
		for (int i = 0; i < 10; i++)
		{
			Cube *cube = new Cube(Point(15 + i, 5.3 , 5 + j), velo, 1.0, 1.0, 1.0, 0.1); //width, height, depth, mass
			physicsManager->add(cube->physicsCube);
			game->world->addDynamicObject(cube); //The world now owns the cube and will tick, draw, and delete it.
			cube->physicsCube->putToSleep();
		}
	}
}