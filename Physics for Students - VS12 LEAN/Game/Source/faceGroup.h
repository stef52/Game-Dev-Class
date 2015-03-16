//*****************************************************************************************//
//                                        FaceGroup                                        //
//*****************************************************************************************//

#ifndef faceGroupModule
#define faceGroupModule 

class FaceGroup {		
private:
	GLenum drawingHint; //GL_STATIC_DRAW or GL_DYNAMIC_DRAW
	GLenum triangleType; //GL_TRIANGLES or GL_TRIANGLE_STRIP;
	GamePoint *vertices; long verticesByteSize; long verticesCount;
	unsigned long *indices; long indicesByteSize; long indicesCount;
	GLuint verticesBuffer; GLuint indicesBuffer;
public:
	Texture *texture;

	FaceGroup (Texture *texture, GamePoint *vertices, long verticesCount, unsigned long *indices = NULL, long indicesCount = 0) {
		this->texture = texture;
		this->vertices = vertices; this-> verticesCount = verticesCount; 
		verticesByteSize = verticesCount * sizeof (GamePoint);
		this->indices = indices; this-> indicesCount = indicesCount; 
		indicesByteSize = indicesCount * sizeof (unsigned long);
		verticesBuffer = 0; indicesBuffer = 0;
		hint (GL_STATIC_DRAW); type (GL_TRIANGLE_STRIP);
	}
	~FaceGroup () {delete [] vertices; delete [] indices;}

	//GL_STATIC_DRAW or GL_DYNAMIC_DRAW 
	GLenum hint () {return drawingHint;}
	void hint (GLenum newHint) {drawingHint = newHint;}
	
	//GL_TRIANGLES or GL_TRIANGLE_STRIP;
	GLenum type () {return triangleType;}
	void type (GLenum newType) {triangleType = newType;}

private:	
	void setupBufferHandles () {	
		glGenBuffers (1, &verticesBuffer);
		if (indices != NULL) glGenBuffers (1, &indicesBuffer);		
	}
	void wrapupBufferHandles () {	
		glDeleteBuffers (1, &verticesBuffer);
		if (indices != NULL) glDeleteBuffers (1, &indicesBuffer);
	}
	void allocateBufferSpaceOnCard () {//Private (used by load)…
		//Needs to be done ONCE... Not using vertices/indices.
		void *initialData = NULL;
		GLenum type1 = GL_ARRAY_BUFFER;
		glBindBuffer (type1, verticesBuffer);
		glBufferData (type1, verticesByteSize, initialData, hint ()); 

		if (indices == NULL) return;	

		GLenum type2 = GL_ELEMENT_ARRAY_BUFFER;
		glBindBuffer (type2, indicesBuffer);
		glBufferData (type2, indicesByteSize, initialData, hint ());
	}
public:		
	static void setup () {
		void setupCustomFaceGroups (); setupCustomFaceGroups ();
	}
	static void wrapup () {
		void wrapupCustomFaceGroups (); wrapupCustomFaceGroups ();
	}

	void load () {
		setupBufferHandles ();
		allocateBufferSpaceOnCard ();
		update ();
	}
	void unload () {
		wrapupBufferHandles ();
	} 	
	void update () {			
		GLenum type1 = GL_ARRAY_BUFFER;
		glBindBuffer (type1, verticesBuffer);
		glBufferSubData (type1, 0, verticesByteSize, vertices); 

		if (indices == NULL) return;
		
		GLenum type2 = GL_ELEMENT_ARRAY_BUFFER;
		glBindBuffer (type2, indicesBuffer);
		glBufferSubData (type2, 0, indicesByteSize, indices);	
	}
private:
	void activateVertices () {
		if (glBindBuffer == 0) ::halt ("\nYou forgot to say FaceGroup::setup ()...");
		if (verticesBuffer == 0) ::halt ("\nYou forgot to load your face group onto the card...");

		glBindBuffer (GL_ARRAY_BUFFER, verticesBuffer);
		long stride = sizeof (GamePoint);
		long floatSize = sizeof (float);

		//WARNING: This assumes your game points have x, y, z, tx, ty, nx, ny nz in that order.
		//Recall: our shaders describe vertices with attributes 0, 1, 2 as "vertex", "textureCoordinate", "normal".
		glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, stride, (void *) 0);
		glVertexAttribPointer (1, 2, GL_FLOAT, GL_FALSE, stride, (void *) (floatSize * 3));
		glVertexAttribPointer (2, 3, GL_FLOAT, GL_FALSE, stride, (void *) (floatSize * 5));
		
		glEnableVertexAttribArray (0);
		glEnableVertexAttribArray (1);
		glEnableVertexAttribArray (2);
	}
	void activateIndices () {		
		//Invoked if indices != NULL…
		glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, indicesBuffer);
	}
	void deactivate () {
		//Recall: our shaders describe vertices with attributes 0, 1, 2 as "vertex", "textureCoordinate", "normal".
		glDisableVertexAttribArray (0);
		glDisableVertexAttribArray (1);
		glDisableVertexAttribArray (2);
		glBindBuffer (GL_ARRAY_BUFFER, 0);
		glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, 0);
	}

public:
	
	void draw (Texture *texture) {
		if (texture == NULL) glDisable (GL_TEXTURE_2D); else texture->activate ();
		activateVertices ();
		if (indices == NULL) {
			glDrawArrays (type (), 0, verticesCount);
		} else {
			activateIndices (); //This implies indices are on the card...
			//To use indices on the card, supply an offset to start from. To use indices on CPU, supply address to start from.
			const bool usingIndicesOnCard = true; const long offsetOfIndices = 0;
			void *startOfIndices = usingIndicesOnCard ? (void *) offsetOfIndices : indices; 
			glDrawElements (type (), indicesCount, GL_UNSIGNED_INT, startOfIndices);
		}
		deactivate ();
	}

	void draw () {draw (texture);}
};

//**************************************************************************************************//
//                                         Custom Geometry                                          //
//**************************************************************************************************//

extern FaceGroup *unitSolidCube; //Has zeros for texture coordinates and normals...
extern FaceGroup *unitSolidSphere; //Has zeros for texture coordinates and normals...
extern FaceGroup *unitSolidFace; //Has proper texture coordinates and normals...

#endif //faceGroupModule