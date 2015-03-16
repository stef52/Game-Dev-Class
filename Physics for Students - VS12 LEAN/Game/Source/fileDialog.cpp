
//*****************************************************************************************//
//                                      Includes                                           //
//*****************************************************************************************//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <windows.h>
#include <commdlg.h>
#include <direct.h>
#include "includes.all"
#include "fileDialog.h"

//*****************************************************************************************//
//                                     File Dialogs                                        //
//*****************************************************************************************//

//*****************************************************************************************//
//                                       Private                                           //
//*****************************************************************************************//

char lastMapDirectory [_MAX_DIR] = {'.','.','\\','M','A','P','S','\0'};
char lastWorldDirectory [_MAX_DIR] = {'.','.','\\','W','O','R','L','D','S','\0'};
char lastTextureDirectory [_MAX_DIR] = {'.','.','\\','T','E','X','T','U','R','E','S','\0'};
char lastDirectory [_MAX_DIR] = {'.','.','\\','M','A','P','S','\0'};

char *directories [4] = {lastMapDirectory, lastWorldDirectory, lastTextureDirectory, lastDirectory};
char lastFilename [MAX_PATH]; 

void setupInternalFilter (char *internalFilter, char *userFilter) {
	//Filter format example: "File (*.bmp;*.tga)|*.bmp;*.tga|All (*.*)|*.*||"
	strcpy (internalFilter, userFilter);
	for (long index = strlen (internalFilter); index >= 0; index--) {
		if (internalFilter [index] == '|') internalFilter [index] = '\0';
	}
}

void setupQueryStructure (OPENFILENAME &queryStructure, char *filter, char *initialFileName) {
	static char filenameBuffer [MAX_PATH]; strcpy (filenameBuffer, initialFileName);
	static char fileFilter [1024]; setupInternalFilter (&fileFilter [0], filter);
	if (lastDirectory [0] == '\0') {_getcwd (lastDirectory, MAX_PATH);}

	queryStructure.lStructSize = sizeof (OPENFILENAME);     
	queryStructure.hwndOwner = NULL;     
	queryStructure.hInstance = GetModuleHandle (NULL);         
	queryStructure.lpstrFilter = fileFilter;     
	queryStructure.lpstrCustomFilter = NULL;     
	queryStructure.nMaxCustFilter = 0;    
	queryStructure.nFilterIndex = 0;     
	queryStructure.lpstrFile = filenameBuffer;     
	queryStructure.nMaxFile = MAX_PATH;    
	queryStructure.lpstrFileTitle = NULL;     
	queryStructure.nMaxFileTitle = MAX_PATH;     
	queryStructure.lpstrInitialDir = lastDirectory;    
	queryStructure.lpstrTitle = NULL;     
	queryStructure.Flags = OFN_NOCHANGEDIR;     
	queryStructure.nFileOffset = 0;     
	queryStructure.nFileExtension = 0;     
	queryStructure.lpstrDefExt = NULL;     
	queryStructure.lCustData = 0;     
	queryStructure.lpfnHook = NULL;     
	queryStructure.lpTemplateName = NULL;     
}

char *promptForReadFile (DirectoryChoice choice, char *filter, char *initialFileName = "") {
	useDirectory (choice);
	OPENFILENAME query; setupQueryStructure (query, filter, initialFileName);      
	if (GetOpenFileName (&query)) {         
		strcpy (lastFilename, query.lpstrFile);   
		memorizeDirectoryUsed (choice); 
		return lastFilename;
	}
	return NULL;
}

char *promptForWriteFile (DirectoryChoice choice, char *filter, char *initialFileName = "") {
	useDirectory (choice);
	OPENFILENAME query; setupQueryStructure (query, filter, initialFileName);      
	if (GetSaveFileName (&query)) {       
		strcpy (lastFilename, query.lpstrFile);  
		memorizeDirectoryUsed (choice); 
		return lastFilename;
	}
	return NULL;
}

//*****************************************************************************************//
//                                       Public                                            //
//*****************************************************************************************//

void useDirectory (DirectoryChoice choice) {
	strcpy (lastDirectory, directories [choice]);
}
void memorizeDirectoryUsed (DirectoryChoice choice) {
	strcpy (directories [choice], lastDirectory);
}

char *promptForUniRead () {return promptForReadFile (UniChoice, "Uni File (*.uni)|*.uni||");}
char *promptForUniWrite () {return promptForWriteFile (UniChoice, "Uni File (*.uni)|*.uni||");}

char *promptForWorldRead () {return promptForReadFile (WorldChoice, "World File (*.wrl)|*.wrl||");}
char *promptForWorldWrite () {return promptForWriteFile (WorldChoice, "World File (*.wrl)|*.wrl||");}

char *promptForTextureRead () {return promptForReadFile (TextureChoice, "Texture File (*.*)|*.*||");}
char *promptForTextureWrite () {return promptForWriteFile (TextureChoice, "Texture File (*.*)|*.*||");}

char *promptForAnyRead () {return promptForReadFile (AnyChoice, "File (*.*)|*.*||");}
char *promptForAnyWrite () {return promptForWriteFile (AnyChoice, "File (*.*)|*.*||");}