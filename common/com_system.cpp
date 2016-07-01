/***********************************************************************
 * system interface functions
 *
 * replace this file to port to another OS, or just carve up with a myriad
 * of #ifdefs
 ***********************************************************************/


#include "common.h"
#include "../lib/lib.h"
#include "../client/cl_console.h"

#if (defined(WIN32) || defined(_WIN32))
# include "../win32/win_public.h"
# include "../win32/wgl_driver.h"
# include <windows.h>
# include <process.h>
# include <io.h>
#include <direct.h> // getcwd
#else
# error Only WIN32 Implemented so far!
# if defined(LINUX)
#  if defined(SDL) 
#   define LINUX_SDL 
#  else
#   define LINUX_X11
#  endif
# elif defined(OSX)
#  define MAC_OSX
# else
# error OS not supported!
# endif
#endif


// os agnostic includes
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

//
// Windows section
//
#if (defined(WIN32) || defined(_WIN32))

#define _MMAP_WINDOWS

#ifdef _MMAP_WINDOWS
    #include <windows.h>
    #include <stdio.h>
    #include <conio.h>
#endif

static int starttime_ms;
static int currenttime_ms;
static gbool timer_initialized = gfalse;
extern com_logging_c com_logger;


    /*
    ERR_LOG = 1,
    ERR_CONSOLE = 2,
    ERR_MSGBOX  = 4,
    ERR_WARNING = 8,
    ERR_FATAL   = 16,
    */

char *Com_GetDateTime( void ) {
	return Sys_GetDateTime();
}

/* pops a message box and kills the works */
void Error( const char * fmt , ... ) { 
    static char buf[0x10000];
    va_list args;

    memset(buf, 0, sizeof(buf));
    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);

    Sys_Error( buf );
}

void Com_Error ( int flag, const char *fmt, ... )
{
    gbool do_warn = gfalse, 
          do_logfile = gfalse;
    va_list argp;
    char msg[4096];
    gwchar_t lmsg[4096];


	memset( msg, 0, sizeof(msg) );
	memset( lmsg, 0, sizeof(lmsg) );

	com_logger.start();


    // make sure it at least does the defaults if not flagged correctly
    if ( (flag & ERR_LOG) || (flag ^ ERR_MSGBOX) ) {
        do_logfile = gtrue;
    }
    if ( (flag & ERR_WARNING) || (flag ^ ERR_FATAL) ) {
        do_warn = gtrue;
    }


    // right now:
    // an err can report to MSGBOX or LOGFILE or Both.  defaults to just
    // LOGFILE, an err can be WARNING or FATAL, defaults to WARNING
    if ( do_logfile ) {
        va_start(argp, fmt);
		_vsnprintf( msg, 4096, fmt, argp );
		fprintf ( com_logger.com_file_p, "%s", msg );
		//fwrite ( msg, 1, 4096, com_logger.com_file_p );
	    va_end (argp);
    }
    if ( flag & ERR_MSGBOX ) {
		if ( !do_logfile ) {
		    va_start (argp, fmt);
			_vsnprintf (msg, 4096, fmt, argp);
			va_end (argp);
		}
        C_wstrncpyUP ( lmsg, msg, sizeof(msg) );
        MessageBox ( 0, (wchar_t *)lmsg , L"Sys Error!", 0 ); 
    }

	// causes a break into the debugger
#ifdef _DEBUG
	__asm {
		int 0x3;
	}
#endif

    if ( do_warn ) {
        com_logger.times_warned++;
    }
    if ( flag & ERR_FATAL ) {
        // reset sys timer
	    timeEndPeriod( 1 );


        //
        // shutdown anything running
        //
    
        // input shutdown
	    //Input_Shutdown();

        //exit(1);

        Com_HastyShutdown();
    }
}

void Com_InitOSSubSystems ( void ) {
	gld.start();
    Win_InitSubSystems();
}

// set base millisecond and timer resolution
void Com_InitTimers ( void )
{
	if ( timer_initialized ) {
        return;
    } else {
        if ( timeBeginPeriod(1) == TIMERR_NOCANDO )
            Sys_Error( "err init sys timer\n" );
		starttime_ms = timeGetTime();
		timer_initialized = gtrue;
	}
}

// ms since init timers first called
int Com_Millisecond ( void )
{
	currenttime_ms = timeGetTime() - starttime_ms;
	return currenttime_ms;
}

/*
====================
 Com_GetTics
 - there is one tick for each frame of gameplay,
====================
*/
int Com_GetTics (void)
{
    return (timeGetTime()-starttime_ms)*LOGIC_FPS/1000;
}

char *Com_GetTimeStamp (void) 
{
    time_t tt;
    tt = time(0);
    return asctime(localtime(&tt));
}

int Com_FileSize (const char *filename)
{
    int sz;
	struct _finddata_t c_file;
    intptr_t hFile;

    if ( (hFile = _findfirst (filename, &c_file )) == -1L )
        return -1;
    sz = c_file.size;
    _findclose(hFile);

    return sz;
}

// only use these in extreme cases, ie, really bad fatal error
int Com_MsgBox ( gwchar_t *msg, gwchar_t *title )
{
    return MessageBox (NULL, (LPCWSTR)msg, (LPCWSTR)title, MB_OK|MB_ICONEXCLAMATION);
}

int Com_ConfirmBox (gwchar_t *msg, gwchar_t *title)
{
    if (MessageBox(NULL, (LPCWSTR)msg, (LPCWSTR)title, MB_YESNO|MB_ICONQUESTION) == IDYES)
        return 1;
    return 0;
}

void Com_SysQuit (void)
{
    timeEndPeriod(1);
	//exit (EXIT_SUCCESS);
}


/********************************************************************** 
 * win32 interfaces 
 **********************************************************************/

/*
==================== 
==================== 
*/
void Com_ShutdownVideo (void)
{
//    win_ShutdownVideo ();
}

/*
==================== 
==================== 
*/
void Com_CheckEventQueue (void)
{
//    win_CheckEventQueue();
}

/*
==================== 
==================== 
*/
/* not so sure that this is how I'm doing it at all
 *  events come in another way and the key info is in there
 * CL_Frame sets client commands based on key state
int Com_GetKey (int k)
{
	int rc = -1;
    rc = Win_GetKey (k);
	return rc;
}
*/

/*
==================== 
for use before video is initialized
==================== 
*/
void Com_SetFullscreen (gbool fullscreenflag) {
}


/*
==================== 

- for use after video is initialized
==================== 
*/
void Com_ToggleFullscreen (int w, int h)
{
    // silence compiler
    w ^= h;
    h ^= w; 
    w ^= h;
}

/*
==================== 
 Com_EndFrame
==================== 
*/
void Com_EndFrame ( void )
{
    Win_SwapScreenBuffers();
}

void Com_Sleep ( int ms )
{
    Sleep( ms );
}

void CL_WriteCfg( void );
void G_LuaClose( void );

/*
==================== 
 Com_HashShutdown
==================== 
*/
void Com_HastyShutdown ( void ) 
{
	static int count = 0;

	Com_Printf( "\nHasty Shutdown requested" );

	if ( count++ )
		Com_Printf( " ...again(%d)", count );
	Com_Printf( "\n" );
	if ( c_shutdown_initiated )
		return;

    // 
    CL_WriteCfg();

	c_shutdown_initiated = gtrue;

	S_Shutdown();
    Com_Printf( "S_Shutdown()\n" );

	if ( di_mouse->integer() ) {
    	Sys_DestroyDIMouse();
    	Com_Printf( "Sys_DestroyDIMouse()\n" );
	}

    WGL_KillWindow();
    Com_Printf( "WGL_KillWindow()\n" );

    FS_Shutdown();
    Com_Printf( "FS_Shutdown()\n" );

    G_LuaClose();
    Com_Printf( "G_LuaClose()\n" );

    V_Shutdown();
    Com_Printf( "V_Shutdown()\n" );

    // reset sys timer
    timeEndPeriod( 1 );
    exit(1);
}

void Com_InitMouse( void ) {
	if ( !com_editor->integer() )
	    Sys_InitDIMouse();
}

void Com_UpdateMouse( void ) {
	if ( !com_editor->integer() )
		Sys_UpdateDIMouse();
}

// keeps a linked list of pointers to things that need to be initilized once at startup
struct zbuf_t {
	void *addr;
	uint size;
};

list_c<zbuf_t*> zbuflist;

void ZeroOnInit( void *buf, unsigned int size ) {
	if ( !zbuflist.isstarted() ) {
		zbuflist.reset();
	}
	zbuf_t *b = (zbuf_t *) V_Malloc( sizeof(zbuf_t) );
	b->addr = buf;
	b->size = size;
	zbuflist.add( b );
}

void RunInitQueue( void ) {
	node_c<zbuf_t*> *rov = zbuflist.gethead();
	while ( rov ) {
		if ( !rov->val ) {
			rov = rov->next;
			continue;
		}
		memset( rov->val->addr, 0, rov->val->size );
		rov = rov->next;
	}
}

/*
====================
 GetDirectoryList
====================
*/
char ** GetDirectoryList( const char *dir, unsigned int *len  ) {
	// start a buffer on the heap
	buffer_c<char *> * files = buffer_c<char*>::newBuffer();

	Win_RecursiveReadDirectory( dir, files );
	
	// get how many there are
	*len = files->length();

	// make sure the char* at the end is NULL
	files->add( NULL );

	// strip the data from the buffer,
	char ** strings = files->data;

	// return the buffer memory to the heap
	V_Free( files );

	// and return the array of strings, as 'char*' to make it easily exportable
	return strings;
}

/*
====================
 Com_CopyFile
====================
*/
int Com_CopyFile( const char *from, const char *to ) {
	FILE *ffp;
	FILE *tfp;
	ffp = fopen ( from, "rb" );
	if ( !ffp )
		return -1;
	tfp = fopen ( to , "wb" );
	if ( !tfp ) {
		fclose( ffp );
		return -2;
	}

	int sz = Com_FileSize( from );
	if ( 0 == sz ) {
		fclose ( tfp ); fclose ( ffp );
	}
	
	int i = 0, j = 0;;
	unsigned char buffer[8192];
	do {
        j = fread( buffer, 1, 8192, ffp );
		if ( j <= 0 )
			break;

		if ( fwrite( buffer, 1, j, tfp ) != j )
			break;

	} while ( 1 );

	fclose( tfp );
	fclose( ffp );

	return 0;
}

// helper func for files that don't #include common.h
const char * Com_GetGamePath( void ) {
	return fs_gamepath->string();
}
const char * Com_GetGameName( void ) {
	return fs_game->string();
}

double Sys_FloatTime (void)
{
#ifdef _WIN32

    static int starttime = 0;

    if ( ! starttime )
        starttime = clock();

    return (clock()-starttime)*1.0/1024;

#else

    struct timeval tp;
    struct timezone tzp; 
    static int      secbase; 
    
    gettimeofday(&tp, &tzp);  

    if (!secbase)
    {
        secbase = tp.tv_sec;
        return tp.tv_usec/1000000.0;
    }

    return (tp.tv_sec - secbase) + tp.tv_usec/1000000.0;

#endif
}

extern gvar_c * fs_basepath;
extern gvar_c * fs_game;
extern gvar_c * fs_gamepath;

void Sys_GetPaths( void ) {
    char buf[ 1024 ];
    char * c = getcwd( buf, 1024U );
    if ( !c ) {
        console.Printf( "getcwd() failed. in setting game paths func" );
        console.dumpcon();
        return;
    }

    fs_basepath->setString( buf );
    char buff[ 1024 ];
    sprintf( buff, "%s\\%s", buf, fs_game->string() );
    fs_gamepath->setString( buff );
}


/*
====================
 get_filestream
 close_filestream
====================
*/
#ifdef _MMAP_WINDOWS
HANDLE __fhandle = NULL, __fmap_obj = NULL;
unsigned char * get_filestream( const char *filename, int *f_size ) 
{
    __fhandle = CreateFileA(		filename,
                                    GENERIC_READ, 
                                    FILE_SHARE_READ,
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL );
    if ( __fhandle == INVALID_HANDLE_VALUE ) {
        fprintf( stderr, "couldn't open the damn file: %s\n", filename );
        return NULL;
    }

    __fmap_obj = CreateFileMapping( __fhandle,
                                                NULL, 
                                                PAGE_READONLY,
                                                0,
                                                0,
                                                NULL );
    if ( __fmap_obj == NULL ) {
        fprintf( stderr, "couldn't create file mapping: %s\n", filename );
        return NULL;
    }

	size_t fsize = GetFileSize( __fhandle, NULL );
	*f_size = fsize;
    
	LPVOID fileView = MapViewOfFile( __fmap_obj, FILE_MAP_READ, 0, 0, fsize );
    if ( fileView == NULL ) {
        fprintf( stderr, "couldn't MAP the damn file: %s\n", filename );
        return NULL;
    }

    return (unsigned char *) fileView;
}

void close_filestream( unsigned char * pBuf ) {
    UnmapViewOfFile(pBuf);
    CloseHandle(__fmap_obj);
    CloseHandle(__fhandle);
    __fmap_obj = NULL;
    __fhandle = NULL;
}
#else
static size_t __secret_size;
unsigned char * get_filestream( const char *filename, int *f_size ) {
	FILE *fp = NULL;
	fp = fopen( filename, "rb" );
	int fn = fileno(fp);
	struct stat st;

	if ( fstat(fn, &st) == -1 || st.st_size == 0 ) {
		fprintf( stderr, "fstat fail!\n" );
		return NULL; 
	}

    __secret_size = st.st_size;
    *f_size = st.st_size;

    // mmap the file stream
    unsigned char *mm = (unsigned char *) mmap(0, st.st_size, PROT_READ, MAP_SHARED, fn, 0);
    return mm;
}
void close_filestream( unsigned char * mm ) {
    if ( -1 == munmap(mm, __secret_size) )
		fprintf( stderr, "munmap indicates error\n" );
    __secret_size = 0;
}
#endif // _MMAP_WINDOWS

#include <sys/stat.h>

/*
====================
 GET_FILELEN
====================
*/
size_t get_filelen( FILE *fp ) {
    struct stat st;
    int f_num = fileno ( fp );
    fstat ( f_num, &st );
    return st.st_size;
}

// returns anything but 0 on error
int sys_atoi( const char *num, int *retNum ) {
    return -1;
}

#endif /* ifdef WIN32 */



