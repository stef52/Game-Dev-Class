//*****************************************************************************************//
//                                      Includes                                           //
//*****************************************************************************************//

#include "includes.all"

//*****************************************************************************************//
//                                       Terrain                                           //
//*****************************************************************************************//
#define USE_QUADS 0
#define USE_FACE_GROUPS 1 
#define USE_FACE_GROUPS_AND_FRUSTUM_CULLING 2 
#define USE_QUADS_AND_FRUSTUM_CULLING 3

#define TERRAIN_IMPLEMENTATION USE_FACE_GROUPS_AND_FRUSTUM_CULLING

void Terrain::tick () {
}

void Terrain::draw () { 
	//Note we have several different approaches here..
	#if (TERRAIN_IMPLEMENTATION == USE_QUADS)
		//Do this before drawing a polygon.
		colorTexture->activate ();
		
		for (long y = 0; y < heightMapHeight - 1; y++) {
			for (long x = 0; x < heightMapWidth - 1; x++) {
				GamePoint p4 = points [x + y * heightMapWidth];
				GamePoint p3 = points [x + (y+1) * heightMapWidth];
				GamePoint p2 = points [x+1 + (y+1) * heightMapWidth];
				GamePoint p1 = points [x+1 + y * heightMapWidth];
				glColor3d (1.0, 1.0, 1.0);
				glPushMatrix ();
					glBegin (GL_QUADS);
						//Recall: our shaders describe vertices with attributes 0, 1, 2 as "vertex", "textureCoordinate", "normal".
						//Note: the first parameter in the glVertexAttrib* function below is one of the indices 0, 1, or 2 above...
						/*    
						glTexCoord2d (p1.tx, p1.ty); glVertex3d (p1.x, p1.y, p1.z);
						glTexCoord2d (p2.tx, p2.ty); glVertex3d (p2.x, p2.y, p2.z);
						glTexCoord2d (p3.tx, p3.ty); glVertex3d (p3.x, p3.y, p3.z);
						glTexCoord2d (p4.tx, p4.ty); glVertex3d (p4.x, p4.y, p4.z);
						*/
						glVertexAttrib2f (1, p1.tx, p1.ty);		 glVertexAttrib3f (0, p1.x, p1.y, p1.z);
						glVertexAttrib2f (1, p2.tx, p2.ty);		 glVertexAttrib3f (0, p2.x, p2.y, p2.z);
						glVertexAttrib2f (1, p3.tx, p3.ty);		 glVertexAttrib3f (p3.x, p3.y, p3.z);
						glVertexAttrib2f (1, p4.tx, p4.ty);		 glVertexAttrib3f (p4.x, p4.y, p4.z);  
					glEnd ();
				glPopMatrix ();
			}
		}
		
		// Loop over the structure drawing the appropriate quads
	#elif (TERRAIN_IMPLEMENTATION == USE_QUADS_AND_FRUSTUM_CULLING)	
		//Do this before drawing a polygon.
		colorTexture->activate ();	

		for (long yDiv = 0; yDiv < divisions; yDiv++) {		 
			long yStart = yDiv * (heightMapHeight / divisions);
			long yFinish = (heightMapHeight / divisions) + (yDiv *  heightMapHeight / divisions);

			for (long xDiv = 0; xDiv < divisions; xDiv++) {
				long xStart = xDiv * (heightMapWidth / divisions);
				long xFinish = (heightMapWidth / divisions) + (xDiv *  heightMapWidth / divisions);

				BoundingBox& activeBox = boundingBoxes [(yDiv * divisions) + xDiv];
				// Add the corner point for more speed less accuracy (missing much height)
				activeBox.add (points [xStart + yStart * heightMapWidth]);
				activeBox.add (points [xStart + yFinish * heightMapWidth]);
				activeBox.add (points [xFinish + yFinish * heightMapWidth]);
				activeBox.add (points [xFinish + yStart * heightMapWidth]);

				// Shouldn't be doing this here but ticking the camera is too early
				camera->frustum->tick ();
				if (!camera->frustum->isOutside (activeBox)) {
					for (long y = yStart; y < yFinish; y++) {
						for (long x = xStart; x < xFinish; x++) {
							GamePoint p4 = points [x + y * heightMapWidth];
							GamePoint p3 = points [x + (y+1) * heightMapWidth];
							GamePoint p2 = points [x+1 + (y+1) * heightMapWidth];
							GamePoint p1 = points [x+1 + y * heightMapWidth];
							glColor3d (1.0, 1.0, 1.0);
							glPushMatrix ();
								glBegin (GL_QUADS);{
									//Recall: our shaders describe vertices with attributes 0, 1, 2 as "vertex", "textureCoordinate", "normal".
									//Note: the first parameter in the glVertexAttrib* function below is one of the indices 0, 1, or 2 above...
									/*    
									glTexCoord2d (p1.tx, p1.ty); glVertex3d (p1.x, p1.y, p1.z);
									glTexCoord2d (p2.tx, p2.ty); glVertex3d (p2.x, p2.y, p2.z);
									glTexCoord2d (p3.tx, p3.ty); glVertex3d (p3.x, p3.y, p3.z);
									glTexCoord2d (p4.tx, p4.ty); glVertex3d (p4.x, p4.y, p4.z);
									*/
									glVertexAttrib2f (1, p1.tx, p1.ty);		 glVertexAttrib3f (0, p1.x, p1.y, p1.z);
									glVertexAttrib2f (1, p2.tx, p2.ty);		 glVertexAttrib3f (0, p2.x, p2.y, p2.z);
									glVertexAttrib2f (1, p3.tx, p3.ty);		 glVertexAttrib3f (p3.x, p3.y, p3.z);
									glVertexAttrib2f (1, p4.tx, p4.ty);		 glVertexAttrib3f (p4.x, p4.y, p4.z);  
								}glEnd ();
							glPopMatrix ();
						}
					}
				}
			}
		}

		// Loop over the structure drawing the appropriate quads
	#elif (TERRAIN_IMPLEMENTATION == USE_FACE_GROUPS)
		faceGroup->draw ();	
	#elif (TERRAIN_IMPLEMENTATION == USE_FACE_GROUPS_AND_FRUSTUM_CULLING)

		// Shouldn't be doing this here but ticking the camera is too early
		camera->frustum->tick ();

		if (!disableShaders) shader->activate ();
		//doManyTimes (4, {
		doManyTimes (0, {
			::log ("\nTerrain shader \"%s\".", defaultShader->name.c_str ());
			::log ("\nActive texture unit %d", activeTextureUnit ());
			::log ("\nActive texture handle %d", activeTextureHandle ());
			::log ("\nActive shader handle %d", activeShaderProgramHandle ());
			::log ("\nTexture handle %d", faceGroups [0]->texture->textureHandle);
			::log ("\nTexture \"%s\".", faceGroups [0]->texture->textureName);
		});

		for (long i = 0; i < divisions * divisions; i++) {
			BoundingBox& activeBox = boundingBoxes [i];
			if (!camera->frustum->isOutside (activeBox)) {
				faceGroups [i]->draw ();
				//doManyTimes (4, {
				doManyTimes (0, {
					::log ("\nTerrain shader \"%s\"; i = %d.", defaultShader->name.c_str (), i);
					::log ("\nActive texture unit %d", activeTextureUnit ());
					::log ("\nActive texture handle %d", activeTextureHandle ());
					::log ("\nTexture handle %d", faceGroups [i]->texture->textureHandle);
					::log ("\nTexture \"%s\".", faceGroups [i]->texture->textureName);
				});
			}
		}
	#endif // TERRAIN_IMPLEMENTATION QUADS
}

Terrain::Terrain  (char* typeName)  : Object (typeName), faceGroup (NULL), faceGroups (NULL), points (NULL), indices (NULL), boundingBoxes (NULL), divisions (8) {
	// actual initialization is in initialize designed after we get the data form the import
	boundingBoxes = new BoundingBox [divisions * divisions];
}

Terrain::~Terrain () {
	/*for (long i = 0; i < pointsSize; i++) {
		delete points [i];
	}*/
	if (faceGroup != NULL) {
		faceGroup->unload (); // unload from the card	
		delete faceGroup; // free memory
		faceGroup = NULL;
	}

	if (faceGroups != NULL) {
		for (long i = 0; i < divisions * divisions; i++) {
			faceGroups [i]->unload ();			
			delete faceGroups [i];
			faceGroups [i] = NULL;
		}
		delete [] faceGroups;
		faceGroups = NULL;
	}

	if (pointGroups != NULL) {
		for (long i = 0; i < divisions * divisions; i++) {
			pointGroups [i] = NULL; //Note: Deleted by the faceGroup.
		}
		delete [] pointGroups;
		pointGroups = NULL;
	}

	if (indexGroups != NULL) {
		for (long i = 0; i < divisions * divisions; i++) {
			indexGroups [i] = NULL; //Note: Deleted by the faceGroup.
		}
		delete [] indexGroups;
		indexGroups = NULL;
	}

#if (TERRAIN_IMPLEMENTATION == USE_QUADS || TERRAIN_IMPLEMENTATION == USE_QUADS_AND_FRUSTUM_CULLING)
	if (indices != NULL) delete [] indices;
	delete [] points; // free memory
#endif
	delete [] boundingBoxes;
}

// Constructor-time
void Terrain::initialize () {
	
	// Read in but don't load into memory
	heightMapWidth = heightTexture->width + 1;
	heightMapHeight = heightTexture->height + 1;

	// Scale factor is normally for 1024 for we adjust
	yScale *=  (1024.0 / heightTexture->height);
	xScale *=  (1024.0 / heightTexture->width);	

	// Allocate the space for heightmap
	heightMap = new float [heightMapWidth * heightMapHeight];
	
	for (long y = 0; y < heightTexture->height; y++) {
		for (long x = 0; x < heightTexture->width; x++) {
			float &height = heightAt (x, y);  // Refer to float
			height = heightFromTextureAt (x, y); // Change it			
			{
				// Get the extra column and row
				if (x == heightTexture->width - 1 || y == heightTexture->height) {					
					if (x == heightTexture->width - 1 && y == heightTexture->height) {
						float& height = heightAt (x+1, y+1);  // Refer to float
						height = heightFromTextureAt (x, y); // Change it						
					}
					else if (y == heightTexture->height) {
						float& height = heightAt (x, y+1);  // Refer to float
						height = heightFromTextureAt (x, y); // Change it						
					}
					else{ // just x
						float& height = heightAt (x+1, y);  // Refer to float
						height = heightFromTextureAt (x, y); // Change it						
					}
				}
			}
		}
	}

	// Add the one extra rows that make the height map larger than the texture
	for (long x = 0; x < heightMapWidth; x++) {
		long y = heightMapHeight - 1;
		float& height = heightAt (x, y);  // Refer to float
		height = heightFromTextureAt (x, y - 1); // Change it
	}

#if (TERRAIN_IMPLEMENTATION == USE_QUADS || TERRAIN_IMPLEMENTATION == USE_QUADS_AND_FRUSTUM_CULLING)
	// Generate the Gamepoints
	points = new GamePoint [heightMapWidth * heightMapHeight];

	for (long y = 0; y < heightMapHeight; y++) {
		for (long x = 0; x < heightMapWidth; x++) {
			GamePoint &gamePoint = gamePointForXY (x, y);
			//points [x + y * heightMapWidth] = new GamePoint ();
			points [x + y * heightMapWidth] = gamePoint;
		}
	}
	
#elif (TERRAIN_IMPLEMENTATION == USE_FACE_GROUPS)
	//Reserved for future debugging... Immediately undefined below...
	#define CHECK_VERTEX_BOUNDS(index) {long i = (index); long b = heightMapWidth * heightMapHeight; \
		if (i < 0 || i >= b) halt ("\nTerrain vertex index %d out of bounds %d.", i, b);}
	#define CHECK_INDEX_BOUNDS(index) {long i = (index); long b = width * height; \
		if (i < 0 || i >= b) halt ("\nTerrain index set index %d out of bounds %d.", i, b);}
	//WILF FOUND BUG SO ABOVE NO LONGER NEEDED... TERRAIN DESTRUCTOR WAS DELETING FACE GROUPS VERTICES AND INDICES
	#undef CHECK_VERTEX_BOUNDS
	#undef CHECK_INDEX_BOUNDS
	#define CHECK_VERTEX_BOUNDS(index) 
	#define CHECK_INDEX_BOUNDS(index)

	// Generate the Gamepoints
	points = new GamePoint [heightMapWidth * heightMapHeight];

	for (long y = 0; y < heightMapHeight; y++) {
		for (long x = 0; x < heightMapWidth; x++) {
			GamePoint &gamePoint = gamePointForXY (x, y);
			//points [x + y * heightMapWidth] = new GamePoint ();
			points [x + y * heightMapWidth] = gamePoint;
			CHECK_VERTEX_BOUNDS (x + y * heightMapWidth);
		}
	}

	//Create Indices Extra point for every line to link the next strip with
	// heightMapWidth + 1 for an extra point for each line
	long width = (heightMapWidth + 1);
	// (((heightMapHeight-2)*2)+2) every line of vertices is stored twice excpept for the first and last
	long height = (((heightMapHeight-2)*2)+2);

	indices = new unsigned long [ width * height ]; // two unused points at the end
	for (long y = 0; y < heightMapHeight - 2; y++) {
		for (long x = 0; x < heightMapWidth + 1; x++) {
			// Add the points that will link the strips together
			if (x == width - 1) {
				// These points are here to connect to the next strip
				// these also us a CCW order
				indices [x * 2 + y * 2 * width] = (x - 1) + y * heightMapWidth; // One of the last two points of this strip
				indices [x * 2 + y * 2 * width + 1] = 0 + (y + 1) * heightMapWidth; // One of the first points of the next strip
				CHECK_INDEX_BOUNDS (x * 2 + y * 2 * width + 1);
			} else {
				// Our vertices have a 0,0 in the bottom left so this
				// order is CCW
				indices [x * 2 + y * 2 * width] = x + (y + 1) * heightMapWidth;
				indices [x * 2 + y * 2 * width + 1] = x + y * heightMapWidth;
				CHECK_INDEX_BOUNDS (x * 2 + y * 2 * width + 1);
			}
		}
	}

	// Store our new gamepoints for the heightmap and put them on the card in a facebuffer
	faceGroup = new FaceGroup (colorTexture, points, heightMapHeight * heightMapWidth, indices, width * height);
	//faceGroup->setup ();
	faceGroup->load ();
#elif (TERRAIN_IMPLEMENTATION == USE_FACE_GROUPS_AND_FRUSTUM_CULLING)
	//Reserved for future debugging... Immediately undefined below...
	#define CHECK_VERTEX_BOUNDS(index) {long i = (index); long b = xDivSize * yDivSize; \
		if (i < 0 || i >= b) halt ("\nTerrain vertex index %d out of bounds %d.", i, b);}
	#define CHECK_INDEX_BOUNDS(index) {long i = (index); long b = width * height; \
		if (i < 0 || i >= b) halt ("\nTerrain index set index %d out of bounds %d.", i, b);}
	#define CHECK_GROUP_BOUNDS(index) {long i = (index); long b = divisions * divisions; \
		if (i < 0 || i >= b) halt ("\nTerrain GROUP index %d out of bounds %d.", i, b);}
	//WILF FOUND BUG SO ABOVE NO LONGER NEEDED... TERRAIN DESTRUCTOR WAS DELETING FACE GROUPS VERTICES AND INDICES
	#undef CHECK_VERTEX_BOUNDS
	#undef CHECK_INDEX_BOUNDS
	#undef CHECK_GROUP_BOUNDS
	#define CHECK_VERTEX_BOUNDS(index)
	#define CHECK_INDEX_BOUNDS(index)
	#define CHECK_GROUP_BOUNDS(index)

	// Generate the Gamepoints
	faceGroups = new FaceGroup* [divisions * divisions];
	pointGroups = new GamePoint* [divisions * divisions];	
	indexGroups = new unsigned long* [divisions * divisions];

	long yDivSize = (heightMapHeight / divisions) + 1; // extra one so no points are missing
	long xDivSize = (heightMapWidth / divisions) + 1; // extra one so no points are missing
	long divMapWidth = (heightMapWidth / divisions);
	long divMapHeight = (heightMapHeight / divisions);

	for (long yDiv = 0; yDiv < divisions; yDiv++) {
		long yStart = yDiv * divMapHeight;
		long yFinish = divMapHeight + yStart;

		for (long xDiv = 0; xDiv < divisions; xDiv++) {
			long xStart = xDiv * divMapWidth;
			long xFinish = divMapWidth + xStart;
			
			// The divisions need to be one row higher and one column longer to
			pointGroups [xDiv + yDiv * divisions] = new GamePoint [ xDivSize * yDivSize ];
			CHECK_GROUP_BOUNDS (xDiv + yDiv * divisions);
			GamePoint* currentPoints = pointGroups [xDiv + yDiv * divisions];

			// Create the Bounding Box for the divison here			
			BoundingBox& activeBox = boundingBoxes [xDiv + yDiv * divisions];

			for (long y = yStart; y <= yFinish; y++) {
				for (long x = xStart; x <= xFinish; x++) {
					currentPoints [(x - xStart) + (y - yStart) * xDivSize] = gamePointForXY (x, y);
					// Add the point to the bounding box
					activeBox.add (gamePointForXY (x, y));
					CHECK_VERTEX_BOUNDS ((x - xStart) + (y - yStart) * xDivSize);
				}
			}
		}		
	}

	// Create Indices Extra point for every line to link the next strip with
	// heightMapWidth + 1 for an extra point for each line
	long width = (xDivSize + 1);
	// (((heightMapHeight-2)*2)+2) every line of vertices is stored twice except for the first and last
	long height = (yDivSize * 2 - 2);

	for (long yDiv = 0; yDiv < divisions; yDiv++) {
		for (long xDiv = 0; xDiv < divisions; xDiv++) {
			indexGroups [xDiv + yDiv * divisions] = new unsigned long [ width * height ];
			CHECK_GROUP_BOUNDS (xDiv + yDiv * divisions);
			unsigned long* currentIndex = indexGroups [xDiv + yDiv * divisions];

			for (long y = 0; y < yDivSize - 1; y++) {
				for (long x = 0; x < xDivSize + 1; x++) {
					if (x == width - 1) {
						// These points are here to connect to the next strip
						// these also us a CCW order
						currentIndex [x * 2 + y * 2 * width] = (x - 1) + y * xDivSize; // One of the last two points of this strip
						currentIndex [x * 2 + y * 2 * width + 1] = 0 + (y + 2) * xDivSize; // One of the first points of the next strip
						CHECK_INDEX_BOUNDS (x * 2 + y * 2 * width + 1);
					} else {
						// Our vertices have a 0,0 in the bottom left so this
						// order is CCW
						currentIndex [x * 2 + y * 2 * width] = x + (y + 1) * xDivSize;
						currentIndex [x * 2 + y * 2 * width + 1] = x + y * xDivSize;
						CHECK_INDEX_BOUNDS (x * 2 + y * 2 * width + 1);
					}
				}
			}
		}
	}

	for (long y = 0; y < divisions; y++) {
		for (long x = 0; x < divisions; x++) {
			faceGroups [x + y * divisions] = new FaceGroup (colorTexture, pointGroups [x + y * divisions], xDivSize * yDivSize, indexGroups [x + y * divisions], width * height);
			//faceGroups [x + y * divisions]->setup ();
			faceGroups [x + y * divisions]->load ();	
			CHECK_GROUP_BOUNDS (x + y * divisions);
		}
	}
#endif

}

// Constructor-time
float Terrain::heightFromTextureAt (long x, long y) {
	// Convert color in range 0 to 255 to range 0 .. 1
	long c = heightTexture->colorAt (x,y);
	float t = heightTexture->colorAt (x,y) / 255.0f;

	// Map t to range seaFloor..mountainTop	
	float height = seaFloor +  (mountainTop - seaFloor) * t;

	return height;
}

float &Terrain::heightAt (long x, long y) {
	return heightMap [x + y * heightMapWidth];
}

// Used for face buidling
Point Terrain::pointForXY (long x, long y) {
	return *offset + Point (x * xScale, heightAt (x,y), -y * yScale);
}

GamePoint Terrain::gamePointForXY (long x, long y) {
	Point point = pointForXY(x,y);	
	Point point2;
	Point point3;

	// Normal Calculation the following logic 
	if (x % 2 == 0) {
		if (y % 2 == 0 && y != heightMapHeight - 1) {
			point2 = pointForXY (x + 1, y + 1);
			point3 = pointForXY (x, y + 1);
		} else {
			point2 = pointForXY (x - 1, y - 1);
			point3 = pointForXY (x, y - 1);
		}
	} else {
		if (y % 2 == 0 && y != heightMapHeight - 1) {
			point2 = pointForXY (x + 1, y);
			point3 = pointForXY (x + 1, y + 1);
		} else {
			point2 = pointForXY (x - 1, y - 1);
			point3 = pointForXY (x, y - 1);
		}
	}
	Point normal = point.pointCross (point2,point3);
	// Destructively normalize itself
	normal.normalize ();
	GamePoint gamePoint = {
		point.x, point.y, point.z,
		(double) x / (double) heightMapWidth, (double) y / (double) heightMapHeight,
		normal.x, normal.y, normal.z};
		//0.0, 1.0, 0.0};
	
	return gamePoint;
}

// Collision Detection
void Terrain::XYForPoint (Point &point, long &x, long &y) {
	//Remember: [x, y] for a pixel corresponds to vertex coordinates [x, z]...
	x = (long) ((point.x - offset->x) / xScale); //Truncate; i.e., towards the pixel to the left of point.x...
	//y = (point.z - offset->y) / -yScale; //Truncate; i.e., toward the pixel below point.z... //WILF: "offset->y" WAS A BUG...
	y = (point.z - offset->z) / -yScale; //Truncate; i.e., toward the pixel below point.z...
}

void Terrain::log () {
	//The type (it's implicit... it will be different for subclasses)...
	::log ("\nEnvironmentObject");
}

void Terrain::import (::ifstream &input, World *world) {
	char line [256]; //Working variable...

	//Input the properties
	SKIP_TO_COLON;
	SKIP_TO_SEMICOLON; long propertiesSize = atoi (line); CLEAR_THE_LINE;
	for (long propertiesIndex = 0; propertiesIndex < propertiesSize; propertiesIndex++) {
		SKIP_TO_ENDLINE;
		// The properties of a regular object are not used
		char key [256]; char value [256]; value [0] = '\0';
		sscanf (line, " \"%[^\"]\" => \"%[^\"]\"", key, value);

		// Load the appropriate textures
		if (stricmp (key, "name") == 0) {
			long size = strlen (value) + 1;
			char* skyname = new char [size]; 
			strncpy (skyname, value, size);			
			char textureName [255];

			strcpy (textureName, value);
			strcat (textureName, "ColorMap");				
			colorTexture = world->textureManager.addTexture (textureName);

			strcpy (textureName, value);
			strcat (textureName, "HeightMap");
			heightTexture = world->textureManager.addTexture (textureName, false);	
		}
		if (stricmp (key, "scaleFor1024Texture") == 0) {
			sscanf (value, "%lf", &xScale);
			sscanf (value, "%lf", &yScale);
		} else if (stricmp (key, "offset") == 0) {
			double offsetX, offsetY, offsetZ;
			sscanf (value, "%lf %lf %lf", &offsetX, &offsetY, &offsetZ);
			offset = new Vector (offsetX, offsetY, offsetZ);			
		} else if (stricmp (key, "seaFloor") == 0) {
			sscanf (value, "%f", &seaFloor);
		} else if (stricmp (key, "mountainTop") == 0) {
			sscanf (value, "%f", &mountainTop);
		} else if (stricmp (key, "shader") == 0) {
			long shaderIndex = -1;
			sscanf (value, "%d", &shaderIndex);
			::log ("\nImport terrain shader %d.", shaderIndex);
			shader = shaderManager->importShaderFor (shaderIndex);
		}
	}

	//Now that we have all the values we can initialize the terrain...
	initialize ();
}