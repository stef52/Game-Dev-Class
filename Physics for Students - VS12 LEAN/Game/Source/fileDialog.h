//*****************************************************************************************//
//                                      File Dialog                                        //
//*****************************************************************************************//

#ifndef fileDialogModule
#define fileDialogModule 

enum DirectoryChoice {UniChoice, WorldChoice, TextureChoice, AnyChoice};

void useDirectory (DirectoryChoice choice);
void memorizeDirectoryUsed (DirectoryChoice choice);

char *promptForUniRead ();
char *promptForUniWrite ();

char *promptForWorldRead ();
char *promptForWorldWrite ();

char *promptForTextureRead ();
char *promptForTextureWrite ();

char *promptForAnyRead ();
char *promptForAnyWrite ();

#endif //fileDialogModule