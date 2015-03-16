//*****************************************************************************************//
//                                     Utilities                                           //
//*****************************************************************************************//

#ifndef utilitiesModule
#define utilitiesModule 

//Macro "declareCollection" below declares a type of vector, e.g., declareCollection (Face);
//creates a vector of Face pointers.

//There are two ways to use such a collection

//	1. Typical way of using a collection:
//
//			FaceCollection faces; 
//			Face *face1 = new Face; ...; faces.push_back (face1); 
//			Face *face2 = new Face; ...; faces.push_back (face2);
//				...
//			loopVector (index, faces)
//				... faces [index] .... //provides an element of type Face *.
//			endloop
//			clear (faces); //Each face is deleted and the collection cleared...

//	2. More unusual way (using a pointer to a collection):
//
//			FaceCollection *faces = new FaceCollection; 
//			Face *face1 = new Face; ...; faces->push_back (face1); 
//			Face *face2 = new Face; ...; faces->push_back (face2);
//				...
//			loopVector (index, *faces)
//				... (*faces) [index] .... //provides an element of type Face *.
//			endloop
//			deleteFaceCollection (faces); //deletes the entries and also faces itself...

#define declareCollection(type) \
	typedef vector <type *> type##Collection; \
	typedef vector <type *>::iterator type##CollectionIterator; \
		\
	inline void delete##type##CollectionEntries (type##Collection &collection) { \
		for (long index = 0; index < collection.size (); index++) { \
			delete collection [index]; \
		} \
	} \
	inline void delete##type##Collection (type##Collection *collection) { \
		delete##type##CollectionEntries (*collection); \
		delete collection; \
	} \
	inline void clear (type##Collection &collection) { \
		delete##type##CollectionEntries (collection); \
		collection.clear (); \
	} 

#define loopVector(variable,vector) {for (long variable = 0; variable < (vector).size (); variable++) {
#define endloop }}



//Create a special kind of StringCollection for dealing with string values...
//It eliminates the need to allocate or delete the values since they are 
//handled automatically by the collection.

typedef vector <string> StringCollection; 
typedef vector <string>::iterator StringCollectionIterator; 
	
inline void deleteStringCollection (StringCollection *collection) { 
	delete collection; 
}

//Example use:
//	StringCollection test;
//	test.push_back ("chair");
//	test.push_back ("brick");
//	test.push_back ("house");
//	loopVector (index, test)
//		if (stricmp (test [index], "brick") == 0) ::log ("\nBrick is at index %d.", index);
//		::log ("\nString %d is \"%s\"...", index, test [index]);
//	endloop





//Macro "declareDictionary" below declares a type of map, e.g., declareDictionary (Face)
//creates a map that associates strings with Face pointers.

//There are two ways to use such a collection

//	1. Typical way of using a dictionary:
//
//			FaceDictionary faces; 
//			Face *face1 = new Face; ...; faces ["name1"] = face1; //Storing
//			Face *face2 = new Face; ...; faces ["name2"] = face2; 
//				...
//			Face *face = faces ["name1"]; //Retrieving: get NULL if not found (also creates key "name1")...
//			if (key_exists (faces, "name1") ... //Safer (no new keys created).

//			faces ["name1"] = face2; //Changing... but will cause memory leak unless "delete faces ["name1"]" done first.
//			loopDictionary (faceKey, faceValue, faces, Face)
//				... faceValue .... //faceValue is locally declared by the loop macro to be type Face * and initialized...
//			endloop
//			clear (faces); //Each face is deleted and the collection cleared...

//	2. More unusual way (using a pointer to a collection):
//
//			FaceDictionary *faces = new FaceDictionary; 
//			As above but use "faces->" instead of "faces." or *faces instead of faces.
//			deleteFaceDictionary (faces); //deletes the entries and also faces itself...

#define loopDictionary(key,value,map,type) \
	{for (type##DictionaryIterator iterator = (map).begin (); iterator != (map).end (); ++iterator) { \
		const char *key = iterator->first.c_str (); \
		type *value = iterator->second;

#define declareDictionary(type) \
	typedef map <string, type *> type##Dictionary; \
	typedef type##Dictionary::iterator type##DictionaryIterator; \
	\
	inline void delete##type##DictionaryEntries (type##Dictionary &collection) { \
		loopDictionary (key, value, collection, type) \
			delete value; \
		endloop \
	} \
	inline void delete##type##Dictionary (type##Dictionary *collection) { \
		delete##type##DictionaryEntries (*collection); \
		delete collection; \
	} \
	\
	inline bool keyExists (type##Dictionary &collection, char *key) { \
		type##DictionaryIterator result = collection.find (key); \
		return result != collection.end (); \
	} \
	inline void clear (type##Dictionary &collection) { \
		delete##type##DictionaryEntries (collection); \
		collection.clear (); \
	}

//Create a special kind of StringDictionary for dealing with properties; i.e. 
//dictionaries that have string keys and string values...

//It eliminates the need to allocate or delete the values since they are 
//handled automatically by the collection (just like the keys are for the 
//standard types of dictionaries).

#define loopStringDictionary(key,value,map) \
	{for (StringDictionaryIterator iterator = (map).begin (); iterator != (map).end (); ++iterator) { \
		const char *key = iterator->first.c_str (); \
		const char *value = iterator->second.c_str ();


typedef map <string, string> StringDictionary; 
typedef StringDictionary::iterator StringDictionaryIterator; 
	
inline bool keyExists (StringDictionary &collection, char *key) { 
	StringDictionaryIterator result = collection.find (key); 
	return result != collection.end (); 
} 

//Example use:
//	StringDictionary test;
//	test ["a"] = "chair");
//	test ["b"] = "brick");
//	test ["c"] = "house");
//	loopStringDictionary (key, value, test)
//		if (stricmp (value, "brick") == 0) ::log ("\nBrick is at key %s.", key);
//		::log ("\nKey %s has vlue %s...", key, value);
//	endloop
//	const char *test1 = test ["a"].c_str (); //Retrieves "chair".
//	const char *test2 = test ["x"].c_str (); //Careful: Missing key creates a new key with value "".
//	if (keyExists (test, "y)) ... //Returns false without creating a new key...


//If you need a forward reference to a collection, you will need the following macros.
//An example would be if class Object itself needs to keep track of a collection of sub-objects.
//In that case, you would need to declare a collection of objects but you will have difficulty
//doing this before the class definition...

#define preDeclareCollection(classOrStruct,type) \
	classOrStruct type; \
	typedef vector <type *> type##Collection;

#define preDeclareDictionary(classOrStruct,type) \
	classOrStruct type; \
	typedef map <string, type *> type##Dictionary;


//Generic macros for reading universal files...
//Note: input.getline (line, 256, c) => picks up everything up to c exclusive and discards c.

#define SKIP_TO(character) input.getline (line, 256, character)
#define SKIP_TO_ENDLINE SKIP_TO ('\n')
#define SKIP_TO_COMMA SKIP_TO (',')
#define SKIP_TO_COLON SKIP_TO (':')
#define SKIP_TO_SEMICOLON SKIP_TO (';')
#define CLEAR_THE_LINE SKIP_TO_ENDLINE

//Useful utility...
void convertToLowercase (char *name);
bool isExtensionSupported (const char *allExtensions, const char *extension);
void logExtensions (bool logAllExtensions);

inline long minimum (long a, long b) {return a <= b ? a : b;}
inline double minimum (double a, double b) {return a <= b ? a : b;}
inline long maximum (long a, long b) {return a >= b ? a : b;}
inline double maximum (double a, double b) {return a >= b ? a : b;}

inline long clamp (long a, long min, long max) {return minimum (maximum (a, min), max);}
inline double clamp (double a, double min, double max) {return minimum (maximum (a, min), max);}
inline long clamp (int a, long min, long max) {return minimum (maximum (a, min), max);}


inline long activeTextureHandle () {
	GLint handle; glGetIntegerv (GL_TEXTURE_BINDING_2D, &handle); return handle;
}

inline long activeTextureUnit () {
	GLint unit; glGetIntegerv (GL_ACTIVE_TEXTURE_ARB, &unit); return unit - GL_TEXTURE0_ARB;
}

inline long activeShaderProgramHandle () {
	return glGetHandleARB (GL_PROGRAM_OBJECT_ARB); 
}

inline long numberOfTextureUnits () {
	int units; glGetIntegerv (GL_MAX_TEXTURE_UNITS_ARB, &units); return units;
}

#define doFirstTime(code) {static bool firstTime = true; if (firstTime) {firstTime = false; {code;}}}
#define doManyTimes(n,code) {static long counter = n; if (counter-- > 0) {code;}}

void logModelViewMatrix (const char *title);
void logPerspectiveMatrix (const char *title);
void logMatrices (const char *title);

inline void detectOpenGLError () {
	GLenum code = glGetError ();
	if (code != GL_NO_ERROR) {
		::log ("\nEncountered error %x \"%s\".", code, gluErrorString (code));
	}
}
inline void detectOpenGLError (const char *message) {
//return; //WILF DISABLE
	GLenum code = glGetError ();
	if (code != GL_NO_ERROR) {
		::halt ("\nEncountered error %x \"%s\" %s.", code, gluErrorString (code), message);
	} else ::log ("\nOpengl OK %s.", message);
}

#endif //utilitiesModule