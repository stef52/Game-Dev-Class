

//*****************************************************************************************//
//                   Routines for building frame, depth, and color buffers                 //
//*****************************************************************************************//

void buildRawFrameBuffers(long howMany, GLuint *frameBufferIDs);
void buildRawDepthBuffers(long howMany, GLuint *depthBufferIDs, long width, long height);
void buildRawTextures(long howMany, GLuint *textureIDs, long kind, long format, long components, long width, long height, long filter = GL_NEAREST);
void buildRawCubeMapFromFile(GLuint &textureID, const char *folder, const char *fileName);

void attachDepthTexture(GLuint depthBufferID);
void attachColorTexture(long whichAttachment, long textureType, GLuint textureID, long mipmap);
void attachColorTexture(long whichAttachment, long textureType, GLuint textureID);
void buildRawShadowMapDepthBuffer(long howMany, GLuint* textureIDs, long format, long width, long height);
void attachShadowMapDepthTexture(GLuint depthBufferID);