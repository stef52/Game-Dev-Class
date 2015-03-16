
//*****************************************************************************************//
//                                      Includes                                           //
//*****************************************************************************************//

#include "logging.h"

//*****************************************************************************************//
//                                       Timing                                            //
//*****************************************************************************************//

double timeNow () {
	//Returns how much time has elapsed since the first call of this function... Accurate to a microsecond...
	static INT64 countsPerSecond; static INT64 oldTime; static bool firstTime = true;
	if (firstTime) {firstTime = false; QueryPerformanceCounter ((LARGE_INTEGER *) &oldTime); QueryPerformanceFrequency ((LARGE_INTEGER *) &countsPerSecond);}
	
	INT64 newTime; QueryPerformanceCounter ((LARGE_INTEGER *) &newTime);
	INT64 elapsedCounts = newTime - oldTime; 
	
	double seconds = (double) elapsedCounts / (double) countsPerSecond; //count / (counts / second) = seconds
	return seconds;
} //Time in milliseconds.

//*****************************************************************************************//
//                                   Random Numbers                                        //
//*****************************************************************************************//

float randomPlusOrMinus (float range) {
	double zeroToOne = (rand () & RAND_MAX) / (double) RAND_MAX;
	//Successively compute a number between 0..1 => -0.5..+0.5 => -1.0..+1.0 => -range..+range...
	return (zeroToOne - 0.5) * 2.0 * range;
}

float randomUpTo (float limit) {
	double zeroToOne = (rand () & RAND_MAX) / (double) RAND_MAX;
	return zeroToOne * limit;
}


//*****************************************************************************************//
//                                Debugging Facilities                                     //
//*****************************************************************************************//

void clearLog () { 
	static char logFileName [255] = "";
	if (*((char *) &logFileName) == '\0') _fullpath ((char *) &logFileName, ".\\log", sizeof (logFileName));
	FILE *file = fopen ((char *) &logFileName, "w"); if (file == NULL) return;
	fclose (file);					 
}

#define setupStaticBuffer() \
	static char buffer [500]; va_list parameters; \
	va_start (parameters, message); \
	vsprintf (buffer, message, parameters); \
	va_end (parameters) 

void log (const char *message, ...) { 
	//Example use: log ("\nInteger %d float %4.2f hex %8.8x.", 10, 1.2, 16);
	setupStaticBuffer ();
	static char logFileName [255] = "";
	if (*((char *) &logFileName) == '\0') 
		_fullpath ((char *) &logFileName, ".\\log", sizeof (logFileName));
	//MessageBox (NULL, buffer, "      Message      ", MB_OK);
	FILE *file = fopen ((char *) &logFileName, "a");
	if (file == NULL) return;
	fprintf (file, "%s", buffer);
	fclose (file);					 
}

void prompt (const char *message, ...) { 
	//Use like log.
	setupStaticBuffer ();
	MessageBox (NULL, buffer, "      Message      ", MB_OK);
}

void quit (const char *message, ...) { 
	//Use like log.
	setupStaticBuffer ();
	::log ("\n%s", buffer);
	MessageBox (NULL, buffer, "      Message      ", MB_OK);
	exit (0);
}

void halt (const char *message, ...) { 
	//Use like log.
	setupStaticBuffer ();
	::log ("\n%s", buffer);
	MessageBox (NULL, buffer, "      Message      ", MB_OK);
	exit (0);
}
 
char *asString (const char *message, ...) { 
	//Use like log.
	setupStaticBuffer ();
	return &buffer [0]; //Careful: Two asStrings in a row use the same buffer.
}
