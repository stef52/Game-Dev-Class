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

void buildRawShadowMapDepthBuffer(long howMany, GLuint* textureIDs, long width, long height){
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
