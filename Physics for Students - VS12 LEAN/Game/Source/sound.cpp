//*****************************************************************************************//
//                                         Sound                                           //
//*****************************************************************************************//

#include "sound.h"
#include <windows.h>
#include <mmsystem.h>

bool Sound::playOnce () const {
	return PlaySound (filename, NULL, SND_FILENAME | SND_ASYNC) == TRUE;
}

bool Sound::playLoop () const {
	return PlaySound (filename, NULL, SND_FILENAME | SND_ASYNC | SND_LOOP) == TRUE;
}

bool Sound::stop () const {
	return PlaySound (NULL, NULL, SND_ASYNC) == TRUE;
}
