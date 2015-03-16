//*****************************************************************************************//
//                                        World                                            //
//*****************************************************************************************//

#ifndef worldModule
#define worldModule 

// Forward declarations
class EnvironmentObject;
class TextureManager;
class Object;

class World {
public:
	EnvironmentObject* environment;
	Terrain *terrain;
	Object** objects; long objectsSize;
	Point startPosition;
	TextureManager textureManager; //Unloads the textures when the world is deleted...

	//WILF PhysX...
	PxRigidStatic *physicsTerrain; 
	PxRigidStatic *physicsWorldMesh;
	PxRigidStatic *physicsPlane;
	ObjectCollection dynamicObjects;

	World ();
	~World ();

	static void setup ();
	static void wrapup ();

	void tick ();
	void draw ();
	void log ();

	static World *read (); //Prompts for a file name to read a ".wrl" file from and reads it into a new world...
	void import (::ifstream &input);
	Object *privateImportObject (::ifstream &input);
	void finalize ();

	void addDynamicObject (Object *object) {dynamicObjects.push_back (object);}
	double approximatePlayerHeightAboveTerrain () {return terrain == NULL ? 0.0 : player->position ().y - terrain-> heightAtPoint (player->position ());}
};

#endif //worldModule