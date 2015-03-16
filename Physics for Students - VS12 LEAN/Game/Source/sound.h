//*****************************************************************************************//
//                                         Sound                                           //
//*****************************************************************************************//

#ifndef soundModule
#define soundModule

//#include <afxwin.h> // TO DO: revise these files
//#include <afxdlgs.h>
//#include "AudioStream.h"

class Sound {

private:
	const char *filename;
public:
	Sound (const char *const filename) {this->filename = filename;}
	~Sound () {}
	bool playOnce () const;
	bool playLoop () const;
	bool stop () const;
};

#endif //soundModule

