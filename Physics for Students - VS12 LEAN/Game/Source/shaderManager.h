//*****************************************************************************************//
//                                     Shader Manager                                      //
//*****************************************************************************************//

#ifndef shaderManagerModule
#define shaderManagerModule 

class ShaderManager;
extern ShaderManager *shaderManager;

class Shader;
extern Shader *defaultShader;

#define ROTATE_LIGHT 0 //To keep lighting constant
//#define ROTATE_LIGHT 1 //To make lighting change fast...

//The following rates are in degrees per second...
#define X_RATE (ROTATE_LIGHT ? 36.0f : 0.0f)
#define Y_RATE (ROTATE_LIGHT ? 36.0f : 0.0f)
#define Z_RATE (ROTATE_LIGHT ? 36.0f : 0.0f)
#define X_ANGLE (ROTATE_LIGHT ? 0.0f : 225.0f)
#define Y_ANGLE (ROTATE_LIGHT ? 0.0f : 0.0f)
#define Z_ANGLE (ROTATE_LIGHT ? 0.0f : 0.0f)

class ShaderManager {
private:
	ShaderCollection shaders;	

	public:
		//Part of the lighting model should be moved to a light object...
		float xRotateRate, yRotateRate, zRotateRate;
		float xAngle, yAngle, zAngle;

	public:
		static void ShaderManager::setup ();
		static void ShaderManager::wrapup () {delete shaderManager; shaderManager = NULL;}

		Shader *addShader (const char *name, ShaderUniformSetterSwitch setter = NoUniforms);
		Shader *importShaderFor (long shaderIndex);
		void tick ();
		void import (::ifstream &input, World *world);
	
	protected:
		void removeShaders ();		

		//Set rotation rates in degrees per second...
		ShaderManager () : xRotateRate (X_RATE), yRotateRate (Y_RATE), zRotateRate (Z_RATE), xAngle (X_ANGLE), yAngle (Y_ANGLE), zAngle (Z_ANGLE) {}
		~ShaderManager ();
};

#undef X_RATE 
#undef Y_RATE 
#undef Z_RATE 
#undef X_ANGLE 
#undef Y_ANGLE 
#undef Z_ANGLE

#endif //shaderManagerModule 
