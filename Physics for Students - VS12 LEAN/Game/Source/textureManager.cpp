//*****************************************************************************************//
//                                      Includes                                           //
//*****************************************************************************************//

#include <sys/types.h>
#include <sys/stat.h>

#include "includes.all"

//*****************************************************************************************//
//                                      Texture                                            //
//*****************************************************************************************//

TextureManager::TextureManager () {}
TextureManager::~TextureManager () {removeTextures ();}

Texture* TextureManager::textureAtIndex (long textureIndex) {
	return textures [textureIndex];
}

void TextureManager::removeTextures () {
	loopVector (textureIndex, textures)
		Texture *texture = textures [textureIndex];
		texture->unload (); delete texture;
	endloop
	textures.clear ();
}

Texture *TextureManager::addTexture (const char *textureName, bool load) {
	char fullPath [MAX_PATH];

	const long numSuffixes = 2;
	static char *suffixes [] = {".tga", ".bmp"};
	Texture *texture = NULL;
 	
	for (long suffixIndex = 0; suffixIndex < numSuffixes; suffixIndex++) {
		strcpy (fullPath, "../Textures/");
		strcat (fullPath, textureName);
		strcat (fullPath, suffixes [suffixIndex]);
		
		texture = Texture::readTexture (fullPath);
		if (texture != NULL) {textures.push_back (texture); break;}
	}
	if (load && texture != NULL) {texture->load ();}
	return texture;
}

void TextureManager::import (::ifstream &input, World *world) {
	char line [256]; //Working variable...

	//Grab the textures
	SKIP_TO_COLON;
	SKIP_TO_SEMICOLON; long textureSize = atoi (line);
	CLEAR_THE_LINE;

	//Delete Old Textures
	if (!textures.empty ()) {	
		textures.clear ();
	}

	// Reset our colletion of textures
	textures.reserve (textureSize);

	for (long i = 0; i < textureSize; i++) {
		//Grab texture name
		SKIP_TO_ENDLINE;
		addTexture (line);
	}
}