

//*****************************************************************************************//
//                   Routines for building frame, depth, and color buffers                 //
//*****************************************************************************************//

void buildRawFrameBuffers (long howMany, GLuint *frameBufferIDs);
void buildRawDepthBuffers (long howMany, GLuint *depthBufferIDs, long width, long height);
void buildRawTextures (long howMany, GLuint *textureIDs, long kind, long format, long components, long width, long height, long filter = GL_NEAREST);

void attachDepthTexture (GLuint depthBufferID);
void attachColorTexture (long whichAttachment, long textureType, GLuint textureID);

//buildRawShadowMapDepthBuffer
void buildRawShadowMapDepthBuffer(long howMany, GLuint* textureIDs, long width, long height);
void attachShadowMapDepthTexture(GLuint depthBufferID);
