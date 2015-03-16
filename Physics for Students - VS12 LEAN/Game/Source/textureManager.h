//*****************************************************************************************//
//                                     Texture Manager                                     //
//*****************************************************************************************//

#ifndef textureManagerModule
#define textureManagerModule 

class TextureManager {
private:
	TextureCollection textures;
public:

	TextureManager ();
	~TextureManager ();

	static void setup ();
	static void wrapup ();

	Texture* textureAtIndex (long textureIndex);
	void removeTextures ();
	Texture *addTexture (const char* textureName, bool load = true);

	void log ();
public:
	void import (::ifstream &input, World *world);
};

#endif //textureManagerModule