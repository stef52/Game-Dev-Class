//*****************************************************************************************//
//                                      Shaders                                            //
//*****************************************************************************************//

#ifndef shaderModule
#define shaderModule 

//*****************************************************************************************//
//                                      Shaders                                            //
//*****************************************************************************************//

extern const char *defaultAttributes [];
enum ShaderUniformSetterSwitch {NoUniforms, DefaultShaderUniforms};

class Shader {
	public:
	
		Shader (const std::string &name, ShaderUniformSetterSwitch setter = NoUniforms) {
			this->name = name; privateProgramHandle = 0; uniformSetter = setter; isBroken = isLoaded = isUnloaded = false;
		}
		~Shader () {unload ();};

		static void setup ();
		static void wrapup ();

		static inline void deactivate () {glUseProgramObjectARB (0);} //Deactivate current shader whatever it is...

		void activate (); 

		//To pass information to the shader if it's activated.
		void setUniform1f (const std::string &variable, float value);
		void setUniform3f (const std::string &variable, float value0, float value1, float value2);
		void setUniform4f (const std::string &variable, float value0, float value1, float value2, float value3);
		void setUniform1i (const std::string &variable, long value);
		void setUniformTexture (const std::string &variable, long textureUnit);		
		void setUniformMatrix4fv (const std::string &variable, float *matrix);
		
		void load (const char *attributes [] = defaultAttributes, long attributesSize = 3);
		void unload ();

		inline GLhandleARB handle () {return privateProgramHandle;}

		static void example (); //Go look at this example...
		static Shader* activeShader; //Note: This tracks the last shader to execute setUniforms (not the true last active shader since direct settings via glUseProgram do not record the active shader)...
		static void reset (); //Done on each draw cycle...
		
		std::string name;

	private:
		bool isBroken; bool isLoaded; bool isUnloaded; GLhandleARB privateProgramHandle; ShaderUniformSetterSwitch uniformSetter;

		GLhandleARB privateLoadAndCompileShader (const char *fileName, GLenum type);
		GLhandleARB privateLinkShaders (GLhandleARB *shaderHandles, const long shaderHandlesSize, const char *attributes [], long attributesSize);
		void setUniforms ();
};

declareCollection (Shader);

#endif // shaderModule