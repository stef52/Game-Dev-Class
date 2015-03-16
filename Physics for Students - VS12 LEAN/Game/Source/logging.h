
#ifndef loggingModule
#define loggingModule 

//*****************************************************************************************//
//                                      Includes                                           //
//*****************************************************************************************//

#include <stdio.h> //For file manipulation.
#include <windows.h> //For va_start, ShowCursor.
#include <stdlib.h> //For _fullpath.
#include <mmsystem.h> //For time
#include <stdlib.h> //For _fullpath or realpath	

//*****************************************************************************************//
//                                      Externals                                          //
//*****************************************************************************************//

void clearLog ();
double timeNow ();
void log (const char *message, ...);
void prompt (const char *message, ...);
void quit (const char *message, ...);
void halt (const char *message, ...);
char *asString (const char *message, ...);

#endif //loggingModule