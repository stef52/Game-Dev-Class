//*****************************************************************************************//
//                                      Includes                                           //
//*****************************************************************************************//

#include "includes.all"

//*****************************************************************************************//
//                RAW Routines for building frame, depth, and color buffers                //
//*****************************************************************************************//

void buildRawFrameBuffers (long howMany, GLuint *frameBufferIDs) {
	glGenFramebuffers (howMany, frameBufferIDs);
}
void buildRawDepthBuffers (long howMany, GLuint *depthBufferIDs, long width, long height) {
	glGenRenderbuffers (howMany, depthBufferIDs);
	for (long index = 0; index < howMany; index++) {
		GLuint depthBufferID = depthBufferIDs [index];
		glBindRenderbuffer (GL_RENDERBUFFER, depthBufferID); 
		glRenderbufferStorage (GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
	}
	::log ("\nBuild raw depth buffer %dx%d.", width, height);
}

void buildRawShadowMapDepthBuffer(long howMany, GLuint* textureIDs, long format, long width, long height){
	glGenTextures(howMany, textureIDs);
	GLenum kind = GL_TEXTURE_2D;
	for (long index = 0; index < howMany; index++) {
		GLuint textureID = textureIDs[index];
		glBindTexture(kind, textureID);
		glTexImage2D(kind, 0, GL_DEPTH_COMPONENT32F, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glTexParameterf(kind, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(kind, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(kind, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE); //GL_NEAREST or GL_LINEAR
		glTexParameteri(kind, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL); //GL_NEAREST or GL_LINEAR
		::log("\nBuild RawShadowMapDepthBuffer %dx%d.", width, height);
	}
}
	

void buildRawCubeMapFromFile(GLuint &textureID, const char *folder, const char *fileName) {
	//Builds a cube map from 6 textures with prefixes "p_x_", "n_x_", ..y.., ..z..; Currently, only handles RGBA8 textures...
	long width, height;
	if (!Texture::readTextureExtent(asString("%s%s%s", folder, "p_x_", fileName), width, height)) {//Not able to get the extent.
		halt("\nCould not find file \"%s\"...", asString("%s%s%s", folder, "p_x_", fileName)); textureID = 0; return;
	}
	//Load the 6 cube map textures into individual texture object (in memory, not on the graphics card).
	Texture *textures[6]; const char *prefixes[] = { "p_x_", "n_x_", "p_y_", "n_y_", "p_z_", "n_z_" };
	for (long index = 0; index < 6; index++) {
		textures[index] = Texture::readTexture(asString("%s%s%s", folder, prefixes[index], fileName));
	}

	//Build the cube map and upload the texture bytes onto the graphics card.

	//Build the cube map: Like buildRawTextures but without executing glTexImage2D...
	long howMany = 1; GLuint *textureIDs = &textureID;
	long kind = GL_TEXTURE_CUBE_MAP; long format = GL_RGBA8; long components = GL_RGBA; long filter = GL_LINEAR;
	glGenTextures(howMany, textureIDs);
	for (long index = 0; index < howMany; index++) {
		GLuint textureID = textureIDs[index];
		glBindTexture(kind, textureID);
		glTexParameterf(kind, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(kind, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameterf(kind, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(kind, GL_TEXTURE_MAG_FILTER, filter); //GL_NEAREST or GL_LINEAR
		glTexParameteri(kind, GL_TEXTURE_MIN_FILTER, filter); //GL_NEAREST or GL_LINEAR
		::log("\nBuild raw texture %dx%d.", width, height);
	}

	//Upload the 6 sets of texture bytes...
	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);
	for (long kind = GL_TEXTURE_CUBE_MAP_POSITIVE_X; kind <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z; kind++) {
		glTexImage2D(kind, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
			textures[kind - GL_TEXTURE_CUBE_MAP_POSITIVE_X]->bytes);
	}

	//Discard the textures that were temporarily built...
	for (long index = 0; index < 6; index++) {
		delete textures[index];
	}
}

//Variables kind, format, components are intended to provide a few variations....
//	1. Kind must be one of GL_TEXTURE_2D, GL_TEXTURE_RECTANGLE which determines how
//	   the shader accesses texture coordinates; i.e. in the 0.0/1.0 float range or 0/width-1 (or height-1) integer range.
//  2. Format must be something like
//	     GL_RGBA32F or GL_RGBA8
//  3. Components must indicate which of RGBA are used and is normally
//       GL_RGBA

void buildRawTextures (long howMany, GLuint* textureIDs, long kind, long format, long components, long width, long height, long filter) {
	glGenTextures (howMany, textureIDs);
	for (long index = 0; index < howMany; index++) {
		GLuint textureID = textureIDs [index];
		glBindTexture (kind, textureID); 
		glTexImage2D (kind, 0, format, width, height, 0, components, format == GL_RGBA8 ? GL_UNSIGNED_BYTE : GL_FLOAT, NULL); 
		glTexParameterf (kind, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
		glTexParameterf (kind, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
		glTexParameteri (kind, GL_TEXTURE_MAG_FILTER, filter); //GL_NEAREST or GL_LINEAR
		glTexParameteri (kind, GL_TEXTURE_MIN_FILTER, filter); //GL_NEAREST or GL_LINEAR
		::log ("\nBuild raw texture %dx%d.", width, height);
	}
}
	
//After binding a frame buffer, you can then attach depth and color buffers via the following...
void attachDepthTexture (GLuint depthBufferID) {
	glFramebufferRenderbuffer (GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBufferID);
}

void attachShadowMapDepthTexture(GLuint depthBufferID) {
	long mipmap = 0; //Like attachColorTexture below but 		//as a DEPTH attachment...
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthBufferID, mipmap);
}


//Variable "whichAttachment" must be one of GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, ...
//Variable "textureType" must be one of GL_TEXTURE_2D, GL_TEXTURE_RECTANGLE

void attachColorTexture (long whichAttachment, long textureType, GLuint textureID) {
	glFramebufferTexture2D (GL_FRAMEBUFFER, whichAttachment, textureType, textureID, 0);
}

void attachColorTexture(long whichAttachment, long textureType, GLuint textureID, long mipmap) {
	glFramebufferTexture2D(GL_FRAMEBUFFER, whichAttachment, textureType, textureID, mipmap);
}

//Finally, when you use a frame buffer with a number of color buffer attachments, you
//need to indicate which subset of the attachments you are using... If that never 
//changes, you can do it ONCE at attachment time BUT if you need to use the buffer
//twice in the same tick with different attachments, you need to do it just before
//you draw as follows:

//	Suppose, you provided 3 color buffers attached to GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,
//  and GL_COLOR_ATTACHMENT2...

//  One draw might need only attachement 0, so
//	GLenum drawBuffers [] = {GL_COLOR_ATTACHMENT0}; glDrawBuffers (1, drawBuffers); 
//  Another draw might need only attachements 1 and 2, so
//	GLenum drawBuffers [] = {GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2}; glDrawBuffers (2, drawBuffers); 

