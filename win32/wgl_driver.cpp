

#include "win_local.h"

#include "wgl_driver.h"

//#include <windows.h>
//#include <io.h>


/* 
 * functions for the wgldriver_t class
 */

//void WGLD_LogMsg ( const char *msg )

// 
wgldriver_t gld;


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
    else // we are logging
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


