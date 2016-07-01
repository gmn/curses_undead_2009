
#include <windows.h>
//#include <io.h>
#include "../common/common.h"
#include "../win32/win_public.h" // Sys_Error

#include "../map/mapdef.h"

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
    unsigned int    sysMsgTime;

	void setHInstance( HINSTANCE h_in ) {
		hInstance = h_in;
	}

    //void start( HINSTANCE h_in ) {
    void start( void ) {
        fullscreen  = gfalse;
/*
        width       = VSCREENWIDTH;
        height      = VSCREENHEIGHT;
*/
        width       = M_Width();
        height      = M_Height();

        hDC         = NULL;
        hGLRC       = NULL;
        hWnd        = NULL;

        WndProc     = NULL;
        pfd         = NULL;
        pixelFormatSet = gfalse;

 //       hInstance   = h_in;
        classRegistered = gfalse;
        hInstOpenGL = NULL;
        
        log_fp      = NULL;
        islogging   = gfalse;

        Error       = Sys_Error;

        //C_wstrncpyDOWN ( window_title, GAME_WINDOW_TITLE, 128 );
        C_strncpy ( window_title, "Zombie Curses", 128 );
        C_strncpy ( logfilename, "wgldriver_log", 64 );

		SetLogging ( gfalse );
    }

    void stop( void ) { 
        SetLogging ( gfalse );
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

    // error
    void (*Error) ( const char *, ... );

    char window_title[128];

    
private:
    FILE *log_fp;
    gbool islogging;
    char logfilename[64];
    friend void Sys_Error ( const char *, ... );
    friend int C_wstrncpyDOWN ( char *, const gwchar_t *, int );
    friend int C_strncpy ( char *, const char *, int );

    //
};

// access
extern wgldriver_t gld;


