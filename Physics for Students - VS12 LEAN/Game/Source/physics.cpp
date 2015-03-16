
//*****************************************************************************************//
//                                      Includes                                           //
//*****************************************************************************************//

#include "includes.all"

//*****************************************************************************************//
//                                      Physics                                            //
//*****************************************************************************************//

#define LOGGING_PHYSICS_CREATION 1

class GameErrorCallback : public physx::PxErrorCallback {
public:
	GameErrorCallback () {};
	~GameErrorCallback () {};

	virtual void reportError (physx::PxErrorCode::Enum code, const char* message, const char* file, int line) {
		::log ("\nPhysX error: \"%s\"...", message); 
		static long counter = 0; //Make sure we don't get into a loop of error messages we can't get out of...
		if (++counter < 5) MessageBox (NULL, "Look at log for PHYX error", NULL, MB_OK);
	}
};

PxDefaultAllocator defaultAllocatorCallback;
PxDefaultErrorCallback defaultErrorCallback;
GameErrorCallback gameErrorCallback;

PxPhysics *physicsSystem;
PhysicsManager *physicsManager = NULL;

#define USING_CUDA false

void PhysicsManager::initializePhysicsManager () {
	bool recordMemoryAllocations = true; 
	PxFoundation *physicsFoundation = PxCreateFoundation (PX_PHYSICS_VERSION, defaultAllocatorCallback, gameErrorCallback);
	if (physicsFoundation == NULL) halt("\nFailed to create a physics foundation...");

	physicsSystem = PxCreatePhysics (PX_PHYSICS_VERSION, *physicsFoundation, PxTolerancesScale (), recordMemoryAllocations);
	if (physicsSystem == NULL) halt ("\nFailed to create a physics system...");
	if (!PxInitExtensions (*physicsSystem)) halt ("\nFailed to initialize physics extensions...");
	
	#ifdef PX_WINDOWS
		if (USING_CUDA) {
			PxCudaContextManagerDesc cudaContextManagerDescription;
			PxCudaContextManager *cudaContextManager = PxCreateCudaContextManager (*physicsFoundation, cudaContextManagerDescription, physicsSystem->getProfileZoneManager ());
			if (cudaContextManager != NULL) {
				halt ("\nFailed to connect with CUDA (physics simulation facility)...");
			}
		}
	#endif
	thrownObjectsTexture = Texture::readTexture ("..\\Textures\\throw.bmp"); //dust1.bmp");
	thrownObjectsTexture->load ();
	if (LOGGING_PHYSICS_CREATION) ::log ("\nCreating physics system %x and manager %x.", physicsSystem, this);
}; 

void PhysicsManager::finalizePhysicsManager () {
	thrownObjectsTexture->unload (); delete thrownObjectsTexture;
	if (scene != NULL) scene->release ();
	#ifdef PX_WINDOWS
		if (USING_CUDA) {cudaContextManager->release ();}
	#endif

	if (LOGGING_PHYSICS_CREATION) ::log ("\nDeleting physics system %x and manager %x.", physicsSystem, this);
	physicsSystem->release ();
};

PxRigidStatic *PhysicsManager::physicsTerrain (Terrain *terrain) {
	if (DISABLE_PHYSICS) return NULL;
	if (LOGGING_PHYSICS_CREATION) ::log ("\nCreate physics terrain");

	//Obtain physX scale factors and the position of the terrain from the terrain object itself...
	float physXXScale, physXYScale, physXZScale; Point physXPosition; //Get the terrain to fill this in...
	terrain->physicsAttributes(physXXScale, physXYScale, physXZScale, physXPosition);

	//Create points for the terrain (PhysX calls them samples). 
	PxHeightFieldSample* samplePoints = new PxHeightFieldSample [terrain->heightMapWidth * terrain->heightMapHeight];

	for (long y = 0; y < terrain->heightMapHeight; y++) {
		for (long x = 0; x < terrain->heightMapWidth; x++) {
			PxHeightFieldSample &toPoint = samplePoints [terrain->physicsCoordinateFor (x, y)];
			toPoint.height = (PxI16)terrain->physicsHeightFor(x, y);
			toPoint.clearTessFlag();	
			toPoint.materialIndex0 = 0;	
			toPoint.materialIndex1 = 0;
			//Note: setTessFlag means triangle diagonal going from top left to bottom right (like backslash character), 
			//and clearTessFlag means triangle diagonal from bottom left to top right (like divide character)...
			//Fill in all variables in toPoint...; terrain quad diagonals are like "/" (not "\")...
			//The terrain knows what the appropriate physics height values should be for [x, y].
		}
	}

	//Create the physX terrain which they call a height field (which requires a description to do it)...

	//Fill in the description and obtain a height field....
	PxHeightFieldDesc description;
	description.format = PxHeightFieldFormat::eS16_TM;
	description.nbColumns = terrain->heightMapWidth;
	description.nbRows = terrain->heightMapHeight; 
	description.samples.data = samplePoints;
	description.samples.stride = sizeof (PxHeightFieldSample);

	PxHeightField* heightField = physicsSystem->createHeightField(description);


	//To position it at "physXPosition" in a scene, you need a static actor since it alone contains position information.

	PxRigidStatic* terrainActor = physicsSystem->createRigidStatic(PxTransform(asVec3(physXPosition)));

	//But the actor's shape is responsible for collision detection, so we create a shape containing the height field and an appropriate friction based material...

	PxMaterial* material = physicsSystem->createMaterial(0.5f, 0.5f, 0.1f);
	PxMaterial* materialArray[1] = { material };

	PxShape* terrainShape = terrainActor->createShape(PxHeightFieldGeometry(heightField, PxMeshGeometryFlags(), physXYScale, physXXScale, physXZScale), materialArray, 1);
	
	//Delete the pieces no longer needed...
	material->release();
	delete samplePoints;

	if (LOGGING_PHYSICS_CREATION) ::log (" %x AT ", terrainActor); physXPosition.log (); ::log ("...");
	return terrainActor;
}

PxRigidStatic *PhysicsManager::physicsMesh (World *world) {
	if (DISABLE_PHYSICS) return NULL;
	if (LOGGING_PHYSICS_CREATION) ::log ("\nCreate physics mesh");
	std::vector<PxVec3> vertices; std::vector<PxU32> indices; //Note: use ".push_back" for adding...
	
	//Process only the static geometry (though other specialized objects could be added too eventually)...
	//Note that a face consists of a convex polygon which needs to be triangulated for physX...
	for (long objectIndex = 0; objectIndex < world->objectsSize; objectIndex++) {
		Object &object = *(world->objects [objectIndex]);
		DualTransformation &transformation = object.transformation;
		if (stricmp (object.type, "static geometry") == 0) {
			for (long faceIndex = 0; faceIndex < object.facesSize; faceIndex++) {		
				Face &face = *(object.faces [faceIndex]);
				long nextIndex = vertices.size (); //If the size is 0, the NEXT index will be 0 too...
				//Add the transformed vertices; i.e., "gamePoint.asPoint () * transformation)" without worrying about triangulation...
				
				for (long pointIndex = 0; pointIndex < face.pointsSize; pointIndex++) {
					GamePoint &gamePoint = *(face.points [pointIndex]);
					//Need code to add the vertices...
					vertices.push_back(asVec3(gamePoint.asPoint() * transformation));
				}

				//Add the indices (3 per triangle). We triangulate n points from 0 to n-1 as follows: [0, 1, 2], [0, 2, 3], [0, 3, 4], ..., [0, n-2, n-1]. 
				//But note that the first index is really "nextIndex" rather than "0" because prior faces may have already been processed...
				for (long pointIndex = 2; pointIndex < face.pointsSize; pointIndex++) {
					//Need code to add 3 indices per triangle...
					indices.push_back(nextIndex + 0);
					indices.push_back(nextIndex + pointIndex - 1);
					indices.push_back(nextIndex + pointIndex);
				}
			}
		}
	}
	if (LOGGING_PHYSICS_CREATION) ::log (" with %d vertices and %d indices", vertices.size (), indices.size ());

	//Create the physX mesh which they call a PxTriangleMesh (which requires a description to do it)...
	PxTriangleMeshDesc description;
	description.points.count = vertices.size();
	description.triangles.count = indices.size() / 3; 
	description.points.stride = sizeof(PxVec3); 
	description.triangles.stride = sizeof(PxU32) * 3;
	description.points.data = &vertices[0];  //std::vector<PxVec3>
	description.triangles.data = &indices[0];  //std::vector<PxU32>

		
	//Additionally, meshes need to be cooked... So you need a cooker along with a memory write buffer and a memory read buffer to create the mesh
	PxCooking* cooker = PxCreateCooking(PX_PHYSICS_VERSION, physicsSystem->getFoundation(), PxCookingParams(PxTolerancesScale()));

	PxDefaultMemoryOutputStream buffer;
	bool status = cooker->cookTriangleMesh(description, buffer);

	PxTriangleMesh* triangleMesh = physicsSystem->createTriangleMesh(PxDefaultMemoryInputData(buffer.getData(), buffer.getSize()));
	cooker->release();


	//To position it in a scene, you need a static actor since it alone contains such information; it can be positioned at [0,0,0] or placed
	//via an identity transformation since the static geometry is in world space.
	//But the actor's shape is responsible for collision detection, so we create one containing a resonable friction based material...

	PxMaterial* material = physicsSystem->createMaterial(0.5f, 0.5f, 0.1f);

	PxRigidStatic* triangleMeshActor = physicsSystem->createRigidStatic(PxTransform(asVec3(Point(0.0f, 0.0f, 0.0f))));
	PxShape* triangleMeshShape = triangleMeshActor->createShape(PxTriangleMeshGeometry(triangleMesh), *material);



	//Delete everything that is no longer needed...
	//Note that vertices and indices are on the stack and will disappear as soon as we exit this method...
	material->release();
	triangleMesh->release();


	if (LOGGING_PHYSICS_CREATION) ::log ("; i.e., %x...", triangleMeshActor); 
	return triangleMeshActor;
}

PxRigidStatic *PhysicsManager::physicsPlane (Point &position, Vector &direction) {
	if (DISABLE_PHYSICS) return NULL;
	//Intuitively, we can hit the plane if it is facing you... This convention is opposite of that used by PhysX which we handle here...

	//The PhysX plane's default direction has the BACK of the plane pointing in the positive x... We pre-rotate it clockwise 90 degrees 
	//around y so the back of the plane is facing negative z. Transformation toAxes computed via lookAtForObject makes the negative z point 
	//along "direction". So Rotate (90, up) * toAxes makes the BACK of the plane point in negative z and then negative z (rather than 
	//positive z) points along "direction"...

	Vector up (0.0, 1.0, 0.0); Vector back (0.0, 0.0, 1.0); //Use back for alternate up.
	Transformation toAxes = Transformation::lookAtForObject (position, direction, up, back); //Make back point along "direction"..
	toAxes.preRotateBy (90.0, up); //The pre-rotate by itself makes right (the back of the physX plane) point back.
	//The above combination implements Rotate (90, up) * toAxes; i.e., right points back, then back points along direction...

	PxTransform pose = asTransform (toAxes);
	PxMaterial* material = physicsSystem->createMaterial(0.5f, 0.5f, 0.1f);

	//Create a static actor with an appropriate material and a plane geometry shape..
	PxRigidStatic* planeActor = physicsSystem->createRigidStatic(pose); //You need to create it using the proper code...
	PxShape* planeShape = planeActor->createShape(PxPlaneGeometry(), *material);

	//Delete all but the actor and shape...
	material->release();

	if (LOGGING_PHYSICS_CREATION) ::log ("\nCreate physics plane %x.", planeActor);
	return planeActor;
}

PxRigidDynamic *PhysicsManager::physicsSphere (Point &position, Vector &velocity, float diameter, float mass) {
	if (DISABLE_PHYSICS) return NULL;
	//Create a dynamic actor with an appropriate material and sphere geometry shape..
	PxRigidDynamic* sphereActor = physicsSystem->createRigidDynamic(PxTransform(asVec3(position))); //You need to create it using the proper code...

	PxMaterial* sphereMaterial = physicsSystem->createMaterial(0.5f, 0.5f, 0.1f);
	PxShape* sphereShape = sphereActor->createShape(PxSphereGeometry(diameter/2), *sphereMaterial);

	PxReal sphereDensity = 1.0;
	PxRigidBodyExt::updateMassAndInertia(*sphereActor, sphereDensity);
	sphereActor->setLinearVelocity(asVec3(velocity));

	//Delete all but the actor and shape...
	sphereMaterial->release();


	if (LOGGING_PHYSICS_CREATION) ::log ("\nCreate physics sphere %x.", sphereActor);
	return sphereActor;
}

PxRigidDynamic *PhysicsManager::physicsBox (Point &position, Vector &velocity, float width, float height, float depth, float mass) {
	if (DISABLE_PHYSICS) return NULL;
	//Create a dynamic actor with an appropriate material and box geometry shape..
	PxRigidDynamic* boxActor = physicsSystem->createRigidDynamic(PxTransform(asVec3(position))); //You need to create it using the proper code... 

	PxMaterial* boxMaterial = physicsSystem->createMaterial(0.5f, 0.5f, 0.1f);
	PxShape* boxShape = boxActor->createShape(PxBoxGeometry(width/2, height/2, depth/2), *boxMaterial);

	PxReal boxDensity = 1.0;
	PxRigidBodyExt::updateMassAndInertia(*boxActor, boxDensity);
	boxActor->setLinearVelocity(asVec3(velocity));

	//Delete all but the actor and shape...
	boxMaterial->release();	

	if (LOGGING_PHYSICS_CREATION) ::log ("\nCreate physics box %x.", boxActor);
	return boxActor;
}

PxScene *PhysicsManager::physicsScene () {
	//Describe a standard gravity scene with 1 simulation thread...
	PxSceneDesc sceneDescription (physicsSystem->getTolerancesScale ());
	sceneDescription.gravity = PxVec3 (0.0f, -9.81f, 0.0f);

	if (sceneDescription.cpuDispatcher == NULL) {
		if (USING_CUDA && cudaContextManager != NULL) {
			sceneDescription.gpuDispatcher = cudaContextManager->getGpuDispatcher ();
		} else {
			const long parallelThreads = 1;
			PxDefaultCpuDispatcher *cpuDispatcher = PxDefaultCpuDispatcherCreate (parallelThreads);
			if (cpuDispatcher == NULL) halt ("\nFailed to be get a physics simulation thread dispatcher...");
			sceneDescription.cpuDispatcher = cpuDispatcher;
		}
	}
	if (sceneDescription.filterShader == NULL) {
		sceneDescription.filterShader = PxDefaultSimulationFilterShader;
        //halt ("\nFailed to be get a physics filter shader (whatever that is)...");
	}

	//And create a scene with it...
	PxScene* scene = physicsSystem->createScene (sceneDescription);
	if (scene == NULL) halt ("\nFailed to create a physics scene...");

	if (LOGGING_PHYSICS_CREATION) ::log ("\nCreate physics scene %x.", scene);
	return scene;
}

bool PhysicsManager::sweptSphereHits (PxRigidDynamic *physicsSphere, Point &from, Point &to, double &intersectionT, Point &intersectionPoint) {
	if (DISABLE_PHYSICS) return false;
	//Returns true if there is an intersection along with intersectionT and the intersection point; otherwise, false with intersectionT = 1.0
	//and the intersection point set to "to". Hence the intersection point can be interpreted as where you can get to if you try to go 
	//from "from" to "to" WHETHER YOU HIT SOMETHING OR NOT...

	//We need the sphere geometry of the shape of the physics sphere (an actor)..
	if (physicsSphere->getNbShapes () != 1) halt ("\nSphere sweeping with a sphere with %d shapes instead of 1...", physicsSphere->getNbShapes ());

	PxShape* shapeBuffer [1]; PxU32 shapeBufferSize = 1;
	physicsSphere->getShapes (shapeBuffer, shapeBufferSize);
	PxSphereGeometry sphereGeometry; shapeBuffer [0]->getSphereGeometry (sphereGeometry);

	PxSweepHit hit; //To be filled in by a scene "sweepSingle" query...	Use query flags "PxSceneQueryFlag::eBLOCKING_HIT | PxSceneQueryFlag::eDISTANCE".

	float toDistance;
	PxVec3 UnitDi = asVec3((to - from).normalized(toDistance));

	//true if blocked..
	if (scene->sweepSingle(sphereGeometry, PxTransform(asVec3(from)), UnitDi, toDistance, PxSceneQueryFlag::eMESH_ANY | PxSceneQueryFlag::eDISTANCE, hit))
	{
		float hitDistance = hit.distance;
		intersectionT = hitDistance / toDistance;
		intersectionPoint = from + (to - from) * intersectionT;
		return true;
	}

	//Deal with both the hit and no hit cases here...
	else
	{
		intersectionPoint = to;
		intersectionT = 1.0;
		return false;
	}

} 

bool PhysicsManager::sweptRayHits (Point &from, Point &to, double &intersectionT, Point &intersectionPoint) {
	if (DISABLE_PHYSICS) return NULL;
	//Returns true if there is an intersection along with intersectionT and the intersection point; otherwise, false with intersectionT = 1.0
	//and the intersection point set to "to". Hence the intersection point can be interpreted as where you can get to if you try to go 
	//from "from" to "to" WHETHER YOU HIT SOMETHING OR NOT...

	PxRaycastHit hit; //To be filled in by the query...	To be filled in by a scene "raycastSingle" query...	Use query flags "PxSceneQueryFlag::eBLOCKING_HIT | PxSceneQueryFlag::eDISTANCE".
	
	float toDistance;
	PxVec3 UnitDi = asVec3((to - from).normalized(toDistance));

	if (scene->raycastSingle(asVec3(from), UnitDi, toDistance, PxSceneQueryFlag::eMESH_ANY | PxSceneQueryFlag::eDISTANCE, hit))
	{
		float hitDistance = hit.distance;
		intersectionT = hitDistance / toDistance;
		intersectionPoint = from + (to - from) * intersectionT;
		return true;
	}

	//Deal with both the hit and no hit cases here...
	else
	{
		intersectionPoint = to;
		intersectionT = 1.0;
		return false;
	}
}
