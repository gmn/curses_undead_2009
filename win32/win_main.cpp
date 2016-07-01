
/*
 * win_main.c - entry point
 */



#include "win_local.h"

#include "wgl_driver.h"

#include "../renderer/r_ggl.h"

#include "../lib/lib.h"



//
// interface calls for com_system.cpp
//

void Win_SetFullscreen(gbool fullscreenflag) 
{
    gld.fullscreen = fullscreenflag;
}


void Win_ShutdownVideo (void)
{
	//WGL_KillWindow();	
}


void Win_ToggleFullscreen (int w, int h)
{
}

extern gvar_c * r_vsync;

/*
==================== 
 Win_SwapScreenBuffers()
 - main buffer flip
==================== 
*/
void Win_SwapScreenBuffers(void)
{
    // enables VSYNC when passed arg=1, or disables VSYNC w/ arg=0
    if ( gwglSwapIntervalEXT )
        gwglSwapIntervalEXT( r_vsync->integer() );

	SwapBuffers( gld.hDC );
}






void Sys_Error ( const char *fmt, ... )
{
    va_list argp;
    char msg[4096];
    gwchar_t lmsg[8192];

	va_start (argp, fmt);
	vsprintf (msg, fmt, argp);
	va_end (argp);

    // reset sys timer
	timeEndPeriod( 1 );

    //
    // shutdown anything running
    //
    
    // input shutdown
	//Input_Shutdown();

    C_wstrncpyUP ( lmsg, msg, 4096 );
    MessageBox ( NULL, (wchar_t *)lmsg , L"Sys Error!", 0 ); 

    exit(1);
}

// FIXME: doit, doit now
int Sys_GetProcessorId ( void )
{
    return -1;
}

gwchar_t *Sys_GetCurrentUser ( void )
{
	static gwchar_t gwUserName[1024];
	unsigned long size = sizeof( gwUserName );

	if ( !GetUserName( (LPWSTR)gwUserName, &size ) )
		C_wstrcpy( gwUserName, W("player") );

	if ( !gwUserName[0] )
		C_wstrcpy( gwUserName, W("player") );

	return gwUserName;
}

int cpuid;
gwchar_t currentUser[64];

void Win_InitSubSystems ( void )
{
    // ID's Sys_Init() in win_main.c
    
    // get OS version
    gld.osversion.dwOSVersionInfoSize = sizeof( gld.osversion );

	if (!GetVersionEx (&gld.osversion))
		Sys_Error ("Couldn't get OS info");

    // detect CPU
    cpuid = Sys_GetProcessorId();

    C_wstrncpy ( currentUser, Sys_GetCurrentUser(), 64 );

    // Input_Init() (IN_Init in ID's win_input.c)
}




int WINAPI WinMain (HINSTANCE hInstance, 
                    HINSTANCE hPrevInstance,
                    PSTR szCmdLine,
                    int iCmdShow )
{
    int retval;

	gld.setHInstance( hInstance );
//    gld.start( hInstance );

    retval = Com_Main ( szCmdLine );

//    MessageBox ( NULL, (wchar_t *)Com_GetArgv(0), L"bootstrap", 0 ); 

    return retval;
}


