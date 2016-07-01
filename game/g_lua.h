#ifndef __G_LUA_H__
#define __G_LUA_H__



#include "../common/com_object.h"
#include "../lib/lib_buffer.h"
#include "../client/cl_console.h"
#include <stdio.h>
#ifndef snprintf
#define snprintf _snprintf
#endif
#include "../lib/lib.h"


class Entity_t;

void G_AttachBindings( void );

typedef struct lua_State lua_State;

class Script_t;

/*
============================================================

                        LuaManager_t

============================================================
*/

// master lua state, VM
class LuaManager_t : public Allocator_t { 
public:
    lua_State *         masterState;
    Script_t *          head;
    int                 _numScripts;

    struct scriptNames_s { 
        char * path;
        char token[255];
    }; 
    scriptNames_s *     scriptNames;



    void                init( void ) ;
    LuaManager_t( void );
    ~LuaManager_t() ;

    void                setupScriptNames( void );       // get a list of available scripts

    Script_t *          startScript( Entity_t *, const char * );    // run a script by name. currently, the only interface to start a script

    void                update( float );                // update the whole lot

    int                 numScripts() { return _numScripts; }

    void                unlinkScript( Script_t * ) ;

    void                RunScriptFrame( float );        // synonym for update

    void                listScripts( void ) ;

    Script_t *          findScript( const char * , int =0 );

    void                printLoaded( void );

    void                killScript( Script_t * );

    // used for fetching, list of available scripts
    buffer_c<char *>    dirList;               
private:
};

extern LuaManager_t lua;



/*
============================================================

                         Script_t

============================================================
*/

enum ScriptState_t
{
    LSS_WAITFRAME,
    LSS_WAITTIME,
    LSS_RUNNING,
    LSS_NOTLOADED,
    LSS_DONE
};

// one of these is created for each running script
class Script_t : public Allocator_t { 
public:
    char                name[64];

    LuaManager_t *      manager;

    Script_t *          next;

    ScriptState_t       state;   

    lua_State *         scriptState;

    float               wakeUpTime;         // time to wake up

    int                 waitFrame;          // number of frames to wait

    float               time;               // gets correct time on update

    Entity_t *          ent;                // can be NULL if it's a map, other


    void                init();
    Script_t();
    Script_t( LuaManager_t * );
    ~Script_t();

    void                resume( float );

    void                update( float );

    void                kill( void ); 

    void                callScriptInit( void );

    void                callScriptUpdate( float dt );

    void                callFunc( const char *, int, int =0 );
    
    void                printLuaVar( const char * );
};


void G_ListScripts( void ) ;
void G_LoadScript( const char * );
void G_RunScript( const char * ); // calls update method
void G_PrintLoaded( void );
void G_CallFunction( const char * f, const char *, const char *, const char *, const char * );
void G_PrintValue( const char * t );
void G_LuaClose( void );


#endif // __G_LUA_H__



/*  
    Functions to be supplied in lua script types

    node:
        init(me)
        update(me, dt)
        activate(me)

    map:
        init(me)
    
    entity:
        init(me)
        update(me, dt)
    
*/
