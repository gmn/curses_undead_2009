/***********************************************************************
 * zombiecurses
 *
 * Copyright (C) Greg Naughton 2007
 *
 * error.cpp - error stuff
 ***********************************************************************/

#include "common.h"

#include <stdio.h>
#include <stdarg.h>
#include <signal.h>
//#include <wchar.h>

#include "../client/cl_console.h"
#include "../lib/lib.h"



static void (*Olib_Shutdown_User)( void ) = NULL;
static int olib_times_warned = 0;
static FILE *olib_logfile_p = NULL;
static char olib_logfile_name[256];
#define DEF_LOGNAME "olib.log"
static int times_log_opened = 0;

unsigned int verbosity = 0;
static int olib_started = 0;


/*
void Olib_Shutdown( void )
{    
    // file handling
    if ( verbosity > 1 )
    O_Printf( "Shutting down the filesystem... FS_Shutdown()\n" );
    FS_Shutdown();

    // memory
    if ( verbosity > 1 )
    O_Printf( "Shutting down the memory handler... V_Shutdown()\n" );
    V_Shutdown();

    O_StopLog();

	olib_started = 0;

// let exit be handled by caller...
//    exit( 0 );
}

static void Shutdown_caller( void ) {
    if (Olib_Shutdown_User)
        Olib_Shutdown_User();
    Olib_Shutdown();
}

static void sig_usr( int sig ) {
    gbool shutdown = gfalse;
    switch( sig ) {
    case SIGUSR1: O_Printf("Olib: caught SIGUSR1\n"); shutdown = gtrue; break;
    case SIGUSR2: O_Printf("Olib: caught SIGUSR2\n"); shutdown = gtrue; break;
    case SIGINT: O_Printf("Olib: caught SIGINT\n"); shutdown = gtrue; break;
    case SIGTERM: O_Printf("Olib: caught SIGTERM\n"); shutdown = gtrue; break;
    default: Err_Warning("received signal %d\n", sig );
    }
    if ( shutdown ) {
        Shutdown_caller();
        exit(sig);
    }
}


void Olib_Startup( void )
{
    time_ms(); // set base time

    if ( verbosity > 1 )
        O_Printf( "Starting up the memory handler... V_Init()\n" );
    V_Init();

    // setup signal handlers to catch SIGKILL SIGQUIT etc. 
    if ( signal(SIGUSR1, sig_usr) == SIG_ERR )
        Err_Warning("can't catch SIGUSR1\n");
    if ( signal(SIGUSR2, sig_usr) == SIG_ERR )
        Err_Warning("can't catch SIGUSR2\n");
    if ( signal(SIGINT, sig_usr) == SIG_ERR )
        Err_Warning("can't catch SIGINT\n");
    if ( signal(SIGTERM, sig_usr) == SIG_ERR )
        Err_Warning("can't catch SIGTERM\n");

    if ( verbosity > 1 )
        O_Printf( "Starting up the file handler... FS_Startup()\n" );
    FS_Startup();

	olib_started = 1;
}

int Olib_IsStarted( void ) {
	return olib_started;
}
*/

// should have different behaviors for Debug and Release modes.  Release could print an error to log, while Debug could crash the program.
void _hidden_Assert( int die_if_false, const char *exp, const char *file, int line ) {
	if ( !die_if_false ) {
		console.Printf( "Assert failed with expression: \"%s\" at: %s, line: %d\n", exp, file, line );
		console.dumpcon( "console.DUMP" );

		__asm {
			int 0x3;
		}

		fprintf( stderr, "Assert failed with expression: \"%s\" at: %s, line: %d\n", exp, file, line );

		Com_Printf( "Assert failed with expression: \"%s\" at: %s, line: %d\n", exp, file, line );
		fflush( stderr );
		fflush( stdout );
		exit( -1 );
	}
}


// prints a warning to the log (if logging is enabled) as well as to stderr
// automatically prepends the prefix "Warning: " 
void Err_Warning( const char *fmt, ... )
{
    char buf[8192];
    char buf2[8192];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buf2, 8192, fmt, args);
    va_end(args);

#ifdef _WIN32
    _snprintf( buf, 8192, "warning: %s", buf2 );
#else
    snprintf( buf, 8192, "warning: %s", buf2 );
#endif

    // FIXME: bug in O_snprintf !!!!
    //O_snprintf( buf, 8192, "Warning(%i): %s", olib_times_warned, buf2 );
    O_WriteLog( buf );
    fprintf( stderr, "%s", buf );
    fflush(stderr);

    ++olib_times_warned;
}

// prints a message to the log (if logging is enabled), to stderr, and then
//  halts the program, (executes the shutdown sequence)
void Err_Fatal( const char *fmt, ... )
{
    char buf[8192];
    char buf2[8192];
    va_list args;
 
    va_start(args, fmt);
    vsnprintf(buf2, 8192, fmt, args);
    va_end(args);

    O_snprintf( buf, 8192, "Fatal Error: %s", buf2 );
    O_WriteLog( buf );
    fprintf( stderr, "%s", buf );

#if 0
#ifdef _DEBUG
	// causes a break into the debugger
	__asm__ {
		int 0x3;
	}
#endif
#endif

    if ( Olib_Shutdown_User ) {
        Olib_Shutdown_User();
    } else {

    }
    // FIXME: put shutdown here
    //Olib_Shutdown();
}

// prints a message to the log (if logging is enabled) as well as to stdout
void O_Printf( const char *fmt, ... ) {
	char *tmp_p;
    va_list args;

    va_start( args, fmt );
#ifdef _GNU_SOURCE
    vasprintf( &tmp_p, fmt, args );
#else
    char buf[8000];
    vsnprintf( buf, sizeof(buf), fmt, args );
    tmp_p = buf;
#endif
    va_end( args );

	if ( !tmp_p )
		return ;

    O_WriteLog( tmp_p );
    fprintf ( stdout, "%s", tmp_p );
    fflush( stdout );
#ifdef _GNU_SOURCE
	free( tmp_p );
#endif
}

/*
void O_wPrintf( const char *fmt, ... ) {
}
*/

/*
// if shutdownHandler is not set explicitly a big WARNING is generated
//  which says that the shutdownHandler was not set.  the default handler
//  will be called , which is just a call to exit(0)
void O_SetShutdownHandler( void (*shutdown_func)(void) )
{
    Olib_Shutdown_User = shutdown_func;
}
*/


// just writes whats passed in to the log
void O_WriteLog( const char *log )
{
    //char buf[8192];
    if ( olib_logfile_p )
    {
        // turning data printing off until I have time to write a function that
        //  formats time as YYMMDDHHMMSSMM
        //O_snprintf( buf, sizeof(buf), "%s: %s", GetTimeStamp(), log );
        //fprintf( olib_logfile_p, "%s", buf );
        //fwrite( buf, 1, strlen(buf), olib_logfile_p );

        fprintf( olib_logfile_p, "%s", log );
        fflush( olib_logfile_p );
    }
}

// logname can be null, if not, logname is set by it.  if no logname given 
//  a default name will be used.
void O_StartLog( const char *logname )
{
    char tmpname[256];
    if ( olib_logfile_p )
    {
        // if this is called after the log is already started, check
        //  to see if te new name requested is different than the current
        //  name. if so, close the log and reopen it with the new name.
        if ( O_strncmp( logname, olib_logfile_name, 256 ) )
        {
            O_StopLog();
            O_StartLog( logname );
        }
    } else {

        if ( logname )
            O_strncpy( tmpname, logname, 256 );
        else
            O_strncpy( tmpname, DEF_LOGNAME, 256 );

        olib_logfile_p = fopen( tmpname, "a+" ); 

        if ( !olib_logfile_p ) {
            Err_Warning ( "Failure to open Log file. \n" );
            return;
        }
        ++times_log_opened;
        if ( times_log_opened == 1 ) {
//            O_WriteLog( va("Logfile \"%s\" opened\n", tmpname) ); 
        } else if ( times_log_opened > 1 ) { 
            O_WriteLog( va("logging restarting as: \"%s\" \n", tmpname) ); 
        }
        O_strncpy( olib_logfile_name, tmpname, 256 );
    }
}

// file is flushed and closed
void O_StopLog( void )
{
    if ( !olib_logfile_p )
        return;
    //O_WriteLog( "Logfile is shutting down\n" );
    fflush( olib_logfile_p );
    fclose( olib_logfile_p );
    olib_logfile_p = NULL;
}

