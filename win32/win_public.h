//
// win_public.h
//
// public win32 system defines, public interface: other modules must use 
// calls specified here to access the win32 system or perish for their 
// insolence 
//


#include "../common/common.h"


// win_event.cpp
void Win_InitVideo ( void );
void Win_ShutdownVideo ( void );
void Win_CheckEventQueue ( void );
int  Win_GetKey ( int );
void Win_ToggleFullscreen ( int, int );
void Win_SwapScreenBuffers ( void );
void Win_SetFullscreen ( gbool );
void Win_InitSubSystems ( void );

// win_gl.cpp
void WGL_KillWindow ( void );
int  WGL_CreateWindow ( char* , int , int , int , gbool );
void WGL_Init ( void );

// win_main.cpp
sysEvent_c Sys_GetEvent( void ); 
void Sys_Error ( const char * , ... );

// win_input.cpp
void Sys_UpdateDIMouse( void );
void Sys_InitDIMouse( void );
void Sys_DestroyDIMouse( void );
mouseFrame_t *Get_MouseFrame( void );

// win_snd.cpp
void Sys_Snd_Shutdown( void );
gbool Sys_Snd_Init( void );
void Sys_Snd_Submit( void );
void Sys_Snd_BeginPainting( void );
int Sys_Snd_GetSamplePos( void );

/* move datatypes to common, have win_filesys.cpp import
 *  them into it from common

// win_filesys.cpp
dirlist_t *D_NewDirlist(void);
void T_GetDirlist (int , char **, dirlist_t *);
void D_AddDirent_dirlist (dirlist_t *);
dirent_t * D_AddDirent_dirent (dirent_t *);
void D_ReadDirlistContents (dirlist_t *);
*/

// win_filesys.cpp
void Sys_mkdir( const char * );
char *Sys_getcwd( void );
char *Sys_GetDateTime ( void );

// win_readdir.cpp
void Win_RecursiveReadDirectory( const char *dir, class buffer_c<char*> * files );


