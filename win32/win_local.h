
//
//  private win32 implementation details
//

// block irrelevant compiler warnings
#include "../common/com_suppress.h"

#include "../common/common.h"



#ifndef __WIN_LOCAL_H__
#define __WIN_LOCAL_H__ 1


// we're still on directInput version 8, 
//  define this before including dinput.h
#define DIRECTINPUT_VERSION 0x0800

#define DIRECTSOUND_VERSION 0x0800


#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <direct.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <process.h> // exit()
#include <gl/gl.h>
#include "../renderer/glext.h"
#include <io.h>
#include <dsound.h>
#include <dinput.h>
#include "../renderer/r_public.h"

#include "win_public.h"

// 
//#include "wgl_driver.h"

//#include <windowsx.h> 
//#include <ddraw.h>    


#define COLORBITS 32


/*
class wgldriver_t {
public:
    gbool fullscreen;
    int width;
    int height;

    HDC	    	hDC;		// Private GDI Device Context
    HGLRC		hGLRC;		// Permanent Rendering Context
    HWND		hWnd;		// Holds Our Window Handle

    WNDPROC     WndProc;
    PIXELFORMATDESCRIPTOR *pfd;
    gbool       pixelFormatSet;

    HINSTANCE	hInstance;		// Holds The Instance Of The Application
    gbool       classRegistered;
	HINSTANCE	hInstOpenGL;

    OSVERSIONINFO osversion;

    // default constructor
    wgldriver_t() {
        fullscreen  = gfalse;
        width       = VSCREENWIDTH;
        height      = VSCREENHEIGHT;

        hDC         = NULL;
        hGLRC       = NULL;
        hWnd        = NULL;

        WndProc     = NULL;
        pfd         = NULL;
        pixelFormatSet = gfalse;

        hInstance   = 0;
        classRegistered = gfalse;
        hInstOpenGL = NULL;
        
        log_fp      = NULL;
        islogging   = gfalse;

        Error       = Sys_Error;

        C_wstrncpyDOWN ( window_title, GAME_WINDOW_TITLE, 128 );
        C_strncpy ( logfilename, "wgldriver_log", 64 );
    }

    ~wgldriver_t () {
        SetLogging ( gfalse );
    }

    //
    // logging
    //

    // logging defaults to false if not set
    void SetLogging ( gbool );
    // ignore call if logging is off
    void logMsg ( const char * );
//    void openLogfile ( void );
//    void closeLogfile ( void );

    // error
    void (*Error) ( const char *, ... );

    static char window_title[128];

    
private:
    FILE *log_fp;
    gbool islogging;
    static char logfilename[64];
    friend void Sys_Error ( const char *, ... );
    friend int C_wstrncpyDOWN ( char *, const gwchar_t *, int );
    friend int C_strncpy ( char *, const char *, int );

    //
};
*/

// wgldriver_t function defs
/*
void wgldriver_t::logMsg ( const char *msg )
{
    if ( log_fp && islogging )
        fprintf ( log_fp, "%s\n", msg );
}


void wgldriver_t::SetLogging ( gbool set )
{
    if ( !islogging ) 
    {
        if ( set ) 
        {
            if ( !log_fp && (( log_fp = fopen( logfilename, "a" )) == NULL ) ) {
                Error ( "couldn't open logfile for writing" );
                return;
            }
            islogging = gtrue;
        }
    }
    else //
    {
        if ( !set ) 
        {
            if ( log_fp ) {
                logMsg ( "shutting down wgldriver logging" );
                fclose ( log_fp );
                log_fp = NULL;
            }
            islogging = gfalse;
        }
    }
}
*/





/////////////////////////////////////////////////////////////////
// win_filesys.cpp
/////////////////////////////////////////////////////////////////
#define FT_UNKNOWN      (0)
#define FT_NORMAL       _A_NORMAL
#define FT_FOLDER       _A_SUBDIR
#define FT_DIRECTORY    FT_FOLDER
#define FT_HIDDEN       _A_HIDDEN
#define FT_SYSTEM       _A_SYSTEM
#define FT_RDONLY       _A_RDONLY


// 
// struct dirent is a list of directories
//  or list of contents of one directory
typedef struct dirent_s {
    char    name[FILENAME_SIZE];              // name of this file
    int     type;                           // type of this file
    int     size;                           // size of this file in bytes
    char    fullname[FULLPATH_SIZE];          // full path name

    struct dirent_s *next_p;
    struct dirent_s *prev_p;
    struct dirent_s *contents_first_p;
    struct dirent_s *contents_current_p;
    int num_files;  // num files read into this dirent's contents_*_p
    int num_dirent; // num of allocated dirent structs under contents_first_p
} dirent_t;

#define __dirent_default  { "", 0, 0, "", NULL, NULL, NULL, NULL, 0, 0 }

//
// struct dirlist is a headnode to a list of directories
typedef struct dirlist_s {
    int total;
    char cwd[FULLPATH_SIZE];
    dirent_t * first_p;
    dirent_t * current_p;
} dirlist_t;

#define __dirlist_default { 0, "", NULL, NULL } 
/////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// wininput_c: struct for mouse handling stuff

#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } } 
#define NUM_MOUSE_FRAMES 16
#define MOUSE_FRAME_MASK (NUM_MOUSE_FRAMES-1)
#define MOUSE_INIT_MAGIC 0xF8675309
#define DEFAULT_MOUSE_BUTTONS   4

class wininput_c {
public:
    int mousex, mousey, mousez;

    mouseFrame_t mframe[NUM_MOUSE_FRAMES];

    LPDIRECTINPUTDEVICE8    lpDIMouse;
    IDirectInput8*		    lpDI; 
    HINSTANCE               hInst_DI;
    HWND                    hWnd_DI;

    int numButtons;

    void start( void ) {
        lpDIMouse = NULL;
        lpDI = NULL;
        hInst_DI = 0L;
        hWnd_DI = 0L;

        endFrame = 0;
        numFrames = 0;
        started = MOUSE_INIT_MAGIC;
        numButtons = DEFAULT_MOUSE_BUTTONS;
    }
    wininput_c() {
        start();
    }

    void shutdown( void ) {
        if( lpDIMouse ) 
            lpDIMouse->Unacquire();
        SAFE_RELEASE( lpDIMouse );
        started = 0;
    }
    ~wininput_c() {
        shutdown();
    }

    void setFrame4(uint32,int,int,int,byte,byte,byte,byte);
    void setFrame8(uint32,int,int,int,byte,byte,byte,byte,byte,byte,byte,byte);
    mouseFrame_t * getFrame( void );
	mouseFrame_t * getPrevFrame( void );

	int getNumFrames( void ) const { return numFrames; }
	int getEndFrame( void ) const { return endFrame; }

private:
    // endframe left set to last frame acquired
    int numFrames;
    int endFrame;
    int started;
};

extern wininput_c winp;
//
/////////////////////////////////////////////////////////////////////////////




#endif /* __WIN_LOCAL_H__ */



/********************************************************************** 
 * function prototypes, and other miscellaneous stuff goes here.
 * basically anything that you dont have to worry about defining
 * twice, and would like to make sure gets defined all the time.      
 **********************************************************************/


// misc:
// block deprecated libc warnings if we have trouble with these
// #define _CRT_SECURE_NO_DEPRECATE




//#pragma warning ( disable : 4113 4133 4047 )
//#pragma warning ( disable : 4244 4668 4820 4255 )


// libs 
// #pragma comment(lib, "OpenGL32.lib")
// #pragma comment(lib, "glu32.lib")

//
// win32 function defs, internally visible
//

// win_ggl.cpp
gbool GGL_Init ( const gwchar_t * );
extern "C" void GGL_Shutdown ( void );

// win_event.cpp
WNDPROC Win_GetWndProc( void );

// win_main.cpp
void Sys_QueEvent(  int time, sysEventType_t type, int value, 
                    int value2, int ptrLength, void *ptr );



