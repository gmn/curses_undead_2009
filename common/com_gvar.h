#ifndef __COM_GVAR_H__
#define __COM_GVAR_H__

#include <typeinfo.h>
#include "com_types.h"

/***********************************************************************
 *  GVAR system defines
 ***********************************************************************/
#define HOPEFULLY_UNIQUE_INIT_TAG 0x55378008
#define INIT_TAG HOPEFULLY_UNIQUE_INIT_TAG

typedef enum {
    GV_ARCHIVE	        =	1,	    // set to cause it to be saved to vars.rc
								    // used for system variables, not for player
								    // specific configurations
    GV_USERINFO	        =	2,      // sent to server on connect or change
    GV_SERVERINFO		=   4,	    // sent in response to front end requests
    GV_SYSTEMINFO		=   8,	    // these cvars will be duplicated on all clients
    GV_INIT			    =   16,	    // don't allow change from console at all,
								    // but can be set from the command line
//    GV_LATCH			=   32,	    // will only change when C code next does
								    // a Cvar_Get(), so it can't be changed
								    // without proper initialization.  modified
								    // will be set, even though the value hasn't
								    // changed yet
    GV_ROM			    =   64,	    // display only, cannot be set by user at all
    GV_USER_CREATED     =   128,	// created by a set command
    GV_TEMP			    =   256,	// can be set even when cheats are disabled, but is not archived
    GV_CHEAT			=   512,	// can not be changed if cheats are disabled
    GV_NORESTART		=   1024,	// do not clear when a cvar_restart is issued
    GV_AUTHORITATIVE    =   2048,    // also sets the reset string to the new value on a subsequent set()
} gvarFlag_t;

#define GV_NAME_SIZE    64
#define GV_STRING_SIZE  0x100
#define GV_HASH_SIZE    0x80
#define GV_POOL_SIZE    0x400

// nothing outside the Cvar_*() functions should modify these fields!
class gvar_c {

protected:

	char		_name        [GV_NAME_SIZE];
	char		_string      [GV_STRING_SIZE];
    char        _oldstring   [GV_STRING_SIZE];
	char		_resetString [GV_STRING_SIZE];	// gvar_restart will reset to this value
    char        _comment [GV_STRING_SIZE];

	//char		latchedString;		// for CVAR_LATCH vars
    
	uint		_flags;
	gbool	    _modified;			// set each time the cvar is changed
	int			_mcount;	        // incremented each time the cvar is changed
	float		_value;				// atof( string )
	int			_integer;			// atoi( string )
	//gvar_c      *_next;

public:

    // never use default constructor
//	gvar_c ( void ) { assert( typeid( this ) != typeid( gvar_c ) ); }
	gvar_c() {}
    virtual ~gvar_c( void );

    void set ( const char * , const char * , uint =0 ); 
	void setValue( float );
    void setString( const char * );
    void setInt( int );
    void reset ( void ) ;  //  sets to resetstring
    void setResetString( const char * );
	void setComment( const char * );

    const char *name( void )    const { return (const char *)_name; }
    const char *string( void )  const { return (const char *)_string; }
    const char *last( void )    const { return (const char *)_oldstring; }
    uint flags( void )          const { return _flags; }
    gbool modified( void )      const { return _modified; }
    int timesModified( void )   const { return _mcount; }
    float value( void )         const { return _value; }
    int integer( void )         const { return _integer; }
	const char *comment( void )  const { return _comment; }

    int     inuse;
    long    hash;
//    gvar_c  * next( void )      const { return _next; }

	gvar_c      *next;

	gbool checkName( void ) {
		if ( _name[0] )
			return gtrue;
		return gfalse;
	}

private:

}; // end gvar_c

gvar_c *Gvar_Find( const char *name ) ;

#endif // __COM_GVAR_H__
