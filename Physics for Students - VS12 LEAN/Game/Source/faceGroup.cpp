//*****************************************************************************************//
//                                      Includes                                           //
//*****************************************************************************************//

#include "includes.all"

//*****************************************************************************************//
//                                      FaceGroup                                          //
//*****************************************************************************************//


//*****************************************************************************************//
//                                   Custom Geometry                                       //
//*****************************************************************************************//

FaceGroup *unitSolidCube = NULL;
FaceGroup *unitSolidSphere = NULL;
FaceGroup *unitSolidFace = NULL;

void setupCustomFaceGroups () {
	const bool logging = false;
	if (logging) log ("\nSetup Global FaceGroup Primitives");
		
	//For reuse by each custom object...
	GamePoint *vertices; unsigned long *indices;

	//SOLID CUBE SETUP

	//Cube vertices in unitSolidCube.
	//               4---5
	//              /|  /|
	// 7(behind)-> 0---1 6
	//             |/  |/
	//             3---2

	const long solidCubeVerticesSize = 8;
	GamePoint solidCubeVertices [solidCubeVerticesSize] = {
		//Note that normals are not correctly set up (since vertices are shared, they would have to be smooth)...
		//Texture coordinates are set up to handle a vertical style gradient; i.e., x is 0 everywhere but y goes from 0 to 1 (bottom to top)...
		{-0.5, 0.5, 0.5, 0.0, 1.0, 0.0, 0.0, 0.0}, //-0.707, +0.707, +0.707
		{0.5, 0.5, 0.5, 0.0, 1.0, 0.0, 0.0, 0.0},
		{0.5, -0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.0},
		{-0.5, -0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.0},
		{-0.5, 0.5, -0.5, 0.0, 1.0, 0.0, 0.0, 0.0},
		{0.5, 0.5, -0.5, 0.0, 1.0, 0.0, 0.0, 0.0},
		{0.5, -0.5, -0.5, 0.0, 0.0, 0.0, 0.0, 0.0},
		{-0.5, -0.5, -0.5, 0.0, 0.0, 0.0, 0.0, 0.0}
	};

	const long solidCubeIndicesSize = 20; const GLenum solidCubePrimitive = GL_TRIANGLE_STRIP;
	unsigned long solidCubeIndices [solidCubeIndicesSize] = {0, 3, 1, 2, 5, 6, 4, 7, 0, 3, 3, 7, 2, 6, 6, 5, 5, 4, 1, 0}; //GL_TRIANGLE_STRIP

	vertices = new GamePoint [solidCubeVerticesSize]; CopyMemory (vertices, &solidCubeVertices, solidCubeVerticesSize * sizeof (GamePoint));
	indices = new unsigned long [solidCubeIndicesSize]; CopyMemory (indices, &solidCubeIndices, solidCubeIndicesSize * sizeof (long));
	unitSolidCube = new FaceGroup (NULL, vertices, solidCubeVerticesSize, indices, solidCubeIndicesSize);

	//SOLID SPHERE SETUP

	{
		long verticalCuts = 3*2; //not counting bottom; 0 degrees at top, 180 at bottom...
		long horizontalCuts = verticalCuts * 2; //twice as many for front and back; not counting wrap around
		const long vertexBufferSize = verticalCuts * horizontalCuts * 4;
		const long indexBufferSize = verticalCuts * horizontalCuts * (4 + 2) - 2;
		
		float diameter = 1.0;
		float radius = 0.5; float verticalCutsInverse = 1.0 / verticalCuts;
		const float pi = 3.14159;

		float verticalAngleIncrement = pi * verticalCutsInverse;
		float horizontalAngleIncrement = verticalAngleIncrement;

		vertices = new GamePoint [vertexBufferSize]; indices = new unsigned long [indexBufferSize];
		long vertexCounter = 0; long indexCounter = 0;

		for (long verticalSlice = 0; verticalSlice < verticalCuts; verticalSlice++) {
			for (long horizontalSlice = 0; horizontalSlice < horizontalCuts; horizontalSlice++) {
				float horizontalAmplitudeTop = sin (verticalSlice * verticalAngleIncrement);
				//Point3D topRight:
					vertices [vertexCounter].x = cos (horizontalSlice * horizontalAngleIncrement) * horizontalAmplitudeTop * radius;
					vertices [vertexCounter].y = cos (verticalSlice * verticalAngleIncrement) * radius;
					vertices [vertexCounter].z = sin (horizontalSlice * horizontalAngleIncrement) * horizontalAmplitudeTop * radius;
					vertices [vertexCounter].tx = (float) horizontalSlice / (horizontalCuts + 1);
					vertices [vertexCounter].ty = (float) verticalSlice / (verticalCuts + 1);
					vertices [vertexCounter].nx = vertices [vertexCounter].x * 2.0;
					vertices [vertexCounter].ny = vertices [vertexCounter].y * 2.0;
					vertices [vertexCounter].nz = vertices [vertexCounter].z * 2.0;

					indices [indexCounter++] = vertexCounter;
					vertexCounter++;

				//Point3D topLeft:
					vertices [vertexCounter].x = cos ((horizontalSlice + 1) * horizontalAngleIncrement) * horizontalAmplitudeTop * radius;
					vertices [vertexCounter].y = cos (verticalSlice * verticalAngleIncrement) * radius;
					vertices [vertexCounter].z = sin ((horizontalSlice + 1) * horizontalAngleIncrement) * horizontalAmplitudeTop * radius;
					vertices [vertexCounter].tx = (float) (horizontalSlice + 1) / (horizontalCuts + 1);
					vertices [vertexCounter].ty = (float) verticalSlice / (verticalCuts + 1);
					vertices [vertexCounter].nx = vertices [vertexCounter].x * 2.0;
					vertices [vertexCounter].ny = vertices [vertexCounter].y * 2.0;
					vertices [vertexCounter].nz = vertices [vertexCounter].z * 2.0;

					indices [indexCounter++] = vertexCounter;
					vertexCounter++;
				
				float horizontalAmplitudeBottom = sin ((verticalSlice + 1) * verticalAngleIncrement);
				//Point3D bottomRight:
					vertices [vertexCounter].x = cos (horizontalSlice * horizontalAngleIncrement) * horizontalAmplitudeBottom * radius;
					vertices [vertexCounter].y = cos ((verticalSlice + 1) * verticalAngleIncrement) * radius;
					vertices [vertexCounter].z = sin (horizontalSlice * horizontalAngleIncrement) * horizontalAmplitudeBottom * radius;
					vertices [vertexCounter].tx = (float) horizontalSlice / (horizontalCuts + 1);
					vertices [vertexCounter].ty = (float) (verticalSlice + 1) / (verticalCuts + 1);
					vertices [vertexCounter].nx = vertices [vertexCounter].x * 2.0;
					vertices [vertexCounter].ny = vertices [vertexCounter].y * 2.0;
					vertices [vertexCounter].nz = vertices [vertexCounter].z * 2.0;

					indices [indexCounter++] = vertexCounter;
					vertexCounter++;

				//Point3D bottomLeft:
					vertices [vertexCounter].x = cos ((horizontalSlice + 1) * horizontalAngleIncrement) * horizontalAmplitudeBottom * radius;
					vertices [vertexCounter].y = cos ((verticalSlice + 1) * verticalAngleIncrement) * radius;
					vertices [vertexCounter].z = sin ((horizontalSlice + 1) * horizontalAngleIncrement) * horizontalAmplitudeBottom * radius;
					vertices [vertexCounter].tx = (float) (horizontalSlice + 1) / (horizontalCuts + 1);
					vertices [vertexCounter].ty = (float) (verticalSlice + 1) / (verticalCuts + 1);
					vertices [vertexCounter].nx = vertices [vertexCounter].x * 2.0;
					vertices [vertexCounter].ny = vertices [vertexCounter].y * 2.0;
					vertices [vertexCounter].nz = vertices [vertexCounter].z * 2.0;

					indices [indexCounter++] = vertexCounter;
					vertexCounter++;
				
				//one vertex bridge per quad
					if (verticalSlice < verticalCuts - 1 || horizontalSlice < horizontalCuts - 1) {
						indices [indexCounter++] = vertexCounter - 1;
						indices [indexCounter++] = vertexCounter;
					}
			}
		}
		if (indexCounter != indexBufferSize || vertexCounter != vertexBufferSize) halt ("\nSolid sphere size mismatch");
		unitSolidSphere = new FaceGroup (NULL, vertices, vertexBufferSize, indices, indexBufferSize);
	}

	//SOLID FACE SETUP

	//Solid face vertices in unitSolidFace.
	//               0---3				
	//               | / |
	//               1---2

	const long solidFaceVerticesSize = 4;
	GamePoint solidFaceVertices [solidFaceVerticesSize] = {
		{-0.5, 0.5, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0}, //top-left
		{-0.5, -0.5, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0}, //bottom-left
		{0.5, -0.5, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0}, //bottom-right
		{0.5, 0.5, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0} //top-right
	};

	const long solidFaceIndicesSize = 4; 
	long solidFaceIndices [solidFaceIndicesSize] = {0, 1, 3, 2}; //GL_TRIANGLE_STRIP

	vertices = new GamePoint [solidFaceVerticesSize]; CopyMemory (vertices, &solidFaceVertices, solidFaceVerticesSize * sizeof (GamePoint));
	indices = new unsigned long [solidCubeIndicesSize]; CopyMemory (indices, &solidFaceIndices, solidFaceIndicesSize * sizeof (long));
	unitSolidFace = new FaceGroup (NULL, vertices, solidFaceVerticesSize, indices, solidFaceIndicesSize);
	
	unitSolidSphere->load ();
	unitSolidCube->load ();
	unitSolidFace->load ();
}

void wrapupCustomFaceGroups () {
	const bool logging = false;
	if (logging) log ("\nWrapup Global Primitives");
	unitSolidSphere->unload (); delete unitSolidSphere;
	unitSolidCube->unload (); delete unitSolidCube;
	unitSolidFace->unload (); delete unitSolidFace;
}
