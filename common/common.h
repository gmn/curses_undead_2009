/***********************************************************************
 *  common.h
 ***********************************************************************/

#include "com_suppress.h"
#include "com_types.h"


#include <stdlib.h> // for size_t and other stdlib stuff
#include <assert.h>
#include "com_assert.h"

#ifndef __COMMON_H__
#define __COMMON_H__

#include "com_color.h"


// macros
#define BIT(x) (1<<(x))
#define ISPOWEROF2(x) (( ((x)<2) || ((x)&((x)-1)) ) ? 0 : 1)
#define ABS(a) (((a) < 0) ? -(a) : (a))
#define DotProduct(x,y)			((x)[0]*(y)[0]+(x)[1]*(y)[1]+(x)[2]*(y)[2])
#define VectorSubtract(a,b,c)	((c)[0]=(a)[0]-(b)[0],(c)[1]=(a)[1]-(b)[1],(c)[2]=(a)[2]-(b)[2])
#define VectorAdd(a,b,c)		((c)[0]=(a)[0]+(b)[0],(c)[1]=(a)[1]+(b)[1],(c)[2]=(a)[2]+(b)[2])
#define VectorCopy(a,b)			((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2])
#define	VectorScale(v, s, o)	((o)[0]=(v)[0]*(s),(o)[1]=(v)[1]*(s),(o)[2]=(v)[2]*(s))


#if (defined(WIN32) || defined(_WIN32))
# if (defined(UNICODE) || defined(_UNICODE))
#  define W(x) (gwchar_t *)L##x
# else
#  define W(x) (char *)x
# endif
#else
# error CODE NOT PORTABLE YET!!
#endif


// Default Locations
#define IMGDIR      "data/"GAME_MODULE_NAME"/img"
#define FONTDIR     "data/"GAME_MODULE_NAME"/font"
#define MAPDIR      "data/"GAME_MODULE_NAME"/maps"


//--------------------
#define MAX_OSPATH          0x1000
#define MAX_FSPATH          0x400
#define FILENAME_SIZE       256
#define FULLPATH_SIZE       0x1000
#define MAX_FILES           4096
#define MAX_PATH_LEN        FULLPATH_SIZE


// virtual screen parameters
#define VSCREENWIDTH     640
#define VSCREENHEIGHT    480


#define LOGIC_FPS 35


#ifndef M_PI
# define M_PI 3.1415926535898
#endif


//===================================================================

// game specific defines
#include "../game/g_defs.h"

// memory, malloc
#include "com_vmem.h"

// c-like string functions
// #include "../lib/lib.h"

// win32 interface, not common enough
// #include "../win32/win_public.h"

// client
#include "../client/cl_public.h"

// sound 
#include "../client/cl_sound.h"

//===================================================================


// ISO C ensures that enum starts at 0 and increments by 1, does ISO C++?
typedef enum {
	SE_NONE = 0,	    // evTime is still valid
	SE_KEY,		        // evValue is a key code, evValue2 is the down flag
	SE_CHAR,	        // evValue is an ascii char
	SE_MOUSE,	        // evValue and evValue2 are reletive signed x / y moves
	SE_JOYSTICK_AXIS,	// evValue is an axis number and evValue2 is the current state (-127 to 127)
	SE_CONSOLE,	        // evPtr is a char*
	SE_PACKET,	        // evPtr is a netadr_t followed by data bytes to evPtrLength

	SE_MOUSE_CLICK,
	SE_MOUSE_MOVE,
} sysEventType_t;

/*
==================== 
 sysEvent_t
==================== 
*/ 
class sysEvent_c {
public:
	int				evTime;
	sysEventType_t	evType;
	int				evValue, evValue2;
	int				evPtrLength;	// bytes of data pointed to by evPtr, for journaling
	void			*evPtr;			// this must be manually freed if not NULL
};




// an error is either FATAL or WARNING, defaults to WARNING, it shows up
//  3 different ways: logfile, console, msgbox, and can be or'd to print to
//  all three, defaults to LOGFILE
typedef enum {
    ERR_LOG     = 1,
    ERR_CONSOLE = 2,
    ERR_MSGBOX  = 4,
    ERR_WARNING = 8,
    ERR_FATAL   = 16,
} com_error_t;

void Com_Printf( char *, ... );

#define COM_LOG_NAME "common.log"
#define LOG_OPEN_TAG 0xF1E2D3C4
// make a class for the auto-start,auto-close functionality
class com_logging_c {
public:
    int log_file_opened;
    int times_warned;
    FILE *com_file_p;
    com_logging_c() {

    }
    ~com_logging_c() {

    }
    int start( void ) {
        if ( log_file_opened != LOG_OPEN_TAG ) {
            times_warned = 0;
			fopen_s( &com_file_p, COM_LOG_NAME, "a" );
            log_file_opened = LOG_OPEN_TAG;
            log = Com_Printf;
			return 1;
        }
		return 0;
    }
    void stop ( void ) {
        if ( log_file_opened == LOG_OPEN_TAG ) {
            fprintf(com_file_p, "\nclosing logfile\ntimes warned: %d\n", 
                times_warned);
            fclose (com_file_p);
            log_file_opened = 0;
        }
    }
    void (*log) ( char *fmt, ... );
};

// new
#include "com_gvar.h"

typedef struct gvarHash_s {
} gvarHash_t;

/**********************************************************************/

#include "com_mouseFrameWhatTheFuck.h"


/* directions: these always seem to come in handy */
typedef enum {
	DIR_NONE = 			0x0,
	DIR_UP = 			0x1,	
	DIR_DOWN = 			0x2,
	DIR_LEFT = 			0x4,
	DIR_RIGHT = 		0x8,
	DIR_DOWN_LEFT = 	0x6,
	DIR_UP_LEFT = 		0x5,
	DIR_DOWN_RIGHT = 	0xA,
	DIR_UP_RIGHT = 		0x9,
    DIR_UP_AND_DOWN =   0x800,
    DIR_LEFT_AND_RIGHT = 0x2000,
} direction_t;


#define MAX_FILE_HANDLES    128

#define fileHash_t_DEFINE fileHash_t 

class fileHandleData_c {
public:
    FILE *fp;               // always NULL when NOT OPEN
    gbool iszip;
    gbool writeable;
    int filesize;
    char name[MAX_FSPATH];
    int lastAccess;         // updated everytime opened, read or written
    int fileofs;            // byte offset from beginning of file
    unsigned long hash;

//    fileHash_t_DEFINE *filehash;
};

#define FILE_HASH_SIZE  37
#define FILE_HASHPOOL_SIZE  MAX_FILE_HANDLES
typedef struct fileHash_s { 
    char name[MAX_FSPATH];
    struct fileHash_s *next;
    fileHandleData_c *data;
    int allocTime;
} fileHash_t;

// moved to com_types
//typedef int filehandle_t;

enum {
    FSEEK_SET = 11,
    FSEEK_CUR = 22,
    FSEEK_END = 33,
};

// com_gvar.cpp
void Gvar_Init( void );
void Gvar_Reset( void );
void Gvar_Free( gvar_c * );
//gvar_c *Gvar_Get( const char *, const char * =NULL, uint =0 );
gvar_c *Gvar_Get( const char *, const char *, uint =0, const char * =0 );
void Gvar_ConPrint( void );
gvar_c *Gvar_Find( const char *name ) ;


// set the ! operator for the gbool type
inline gbool operator! (gbool in) { 
	return (gbool)(!((int)in)); 
}
/***********************************************************************/

extern gvar_c *com_plix;
//extern gvar_c *com_mapedit;
extern gvar_c *com_texttest;

extern gvar_c *di_mouse;

extern gvar_c *fs_basepath; // game base path, full path
extern gvar_c *fs_game;		// folder of current mod
extern gvar_c *fs_gamepath; // fs_basepath/fs_game
extern gvar_c *fs_devpath;	// optional additional directory (fullpath) to 
							// search for resources in

extern gvar_c *com_developer;

extern gvar_c *freetype_test;

extern gvar_c *com_resolution;
extern gvar_c *com_reject;
extern gvar_c *com_showfps;

extern gvar_c *com_editor;
extern gvar_c *_lockConsole;
extern gvar_c *godmode;

#endif /* __COMMON_H__ */




/***********************************************************************
 *  prototypes
 ***********************************************************************/

// com_main.cpp 
const char * Com_GetArgv ( int i );
int Com_Main ( const char * );
gbool operator! (gbool);
void Com_Quit_f ( void );
int Com_EventLoop( void );

// com_files.cpp
void FS_InitialStartup ( void );
int FS_SlurpFile( const char *path, void **data );
void FS_ResetFileHashTable( void );
int FS_FOpenReadOnly( const char *, filehandle_t * );
int FS_FClose( filehandle_t );
int FS_Read( void *, int, filehandle_t );
int FS_Seek( filehandle_t, int, uint );
int FS_Rewind( filehandle_t );
int FS_Tell( filehandle_t );
int FS_Write( void *, int, filehandle_t );
int FS_FOpenWritable( const char *, filehandle_t * );
void FS_Shutdown( void );
int FS_ReadAll( void *, filehandle_t );
int FS_GetFileSize( filehandle_t );


// com_system.cpp
char *Com_GetDateTime( void ); 
void Com_Error ( int, const char *, ... );
void Error( const char *, ... );
void Com_InitOSSubSystems ( void );
void Com_InitTimers ( void );
int  Com_Millisecond ( void );
#ifndef __INLINE_NOW__
#define __INLINE_NOW__
inline int now() { return Com_Millisecond(); }
#endif
int  Com_GetTics ( void );
char * Com_GetTimeStamp ( void );
int  Com_FileSize ( const char * ); 
int  Com_MsgBox ( gwchar_t *, gwchar_t * );
int  Com_ConfirmBox ( gwchar_t *, gwchar_t * );
void Com_SysQuit ( void );
//int  Com_GetKey ( int );
void Com_EndFrame ( void );
void Com_Sleep ( int );
void Com_HastyShutdown ( void );
void Com_UpdateMouse( void );
void Com_InitMouse( void );
void ZeroOnInit( void );
void RunInitQueue( void );
char ** GetDirectoryList( const char *, unsigned int * );
int Com_CopyFile( const char *, const char * );
unsigned char * get_filestream( const char *filename, int *f_size );
void close_filestream( unsigned char * mm );
size_t get_filelen( FILE *fp );

// com_error.cpp
void O_StopLog( void );
void O_StartLog( const char * );
void O_WriteLog( const char * );
void O_Printf( const char *, ... );
void Err_Fatal( const char *, ... );
void Err_Warning( const char *, ... );



/***********************************************************************
 *  extern accessors
 ***********************************************************************/
extern gbool c_shutdown_initiated ;

extern	int		com_frameTime;
extern	int		com_frameMsec;


