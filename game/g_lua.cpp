// g_lua.cpp
//
// lua bindings for: Camera, Entity_t, Effects, Controller

extern "C" {  
#include "../lua/lua.h"  
#include "../lua/lualib.h"  
#include "../lua/lauxlib.h"  
//#include "../lua/lstate.h" // struct lua_State
} 

#include "g_lua.h"
#include "../common/com_assert.h"
#include "../common/com_gvar.h"

class Player_t;

// singleton, main system handle 
LuaManager_t lua;

extern Player_t player;
extern gvar_c *lua_useThreads;


/////////////////////////////////////////////////////////////////////////////
//
// list of zombiecurses func prototypes here
//
void glua_stackDump( lua_State * L );
void glua_globalsDump( lua_State * L );
void glua_globalsDump2( lua_State * L );

/////////////////////////////////////////////////////////////////////////////
//
// list of luahook wrappers here
//

// get pointer to player
// rets: 1:Entity_t*
static int l_getPlayer( lua_State *L ) {
    lua_pushlightuserdata( L, &player );
    return 1;
}

// args: 1:string
static int l_loadMap( lua_State *L ) {
    return 0;
}

static int l_conPrint( lua_State * L ) {
    const char * s = "";
    if ( lua_isstring(L,-1) ) {
        s = lua_tostring( L, -1 );
    }
    console.Printf( "%s", s );
    lua_pop( L, 1 );
    return 0;
}

// args 1:ms
static int l_fadeOut( lua_State * L ) {
    return 0;
}

// args 1:ms
static int l_fadeIn( lua_State * L ) {
    return 0;
}

// args 1:zoom 2:ms(optional, defaults to 0)
static int l_setZoom( lua_State * L ) {
    return 0;
}

// rets 1:Camera_t*
static int l_getCam( lua_State * L ) {
    return 1;
}

// args 1:x 2:y 3:t 4:zoom (optional)
static int l_camMove( lua_State * L ) {
    return 0;
}

// args 1:string
static int l_playMusic( lua_State * L ) {
    return 0;
}

static int l_playSfx( lua_State * L ) {
    return 0;
}

static int l_clearSfx( lua_State * L ) {
    return 0;
}

// args 1:string
// rets 1:Entity_t*
static int l_getEnt( lua_State * L ) {
    return 1;
}

// create an empty ent
// rets 1:Entity_t*
static int l_newEnt( lua_State * L ) {
    return 1;
}

/*
============================================================================
    ent* functions
============================================================================
*/
// args 1:Entity_t* 2:alpha
static int l_entSetAlpha( lua_State * L ) {
    return 0;
}

// args: 1:Entity_t*, 2:Entity_t* target -- can be null
static int l_entSetTarget( lua_State * L ) {
    return 0;
}

// args: 1:Entity_t*
// rets: 1:Entity_t*
static int l_entGetTarget( lua_State * L ) {
    return 1;
}

// args 1:Entity_t*
static int l_entDie( lua_State * L ) {
    return 0;
}
// args 1:Entity_t*
// rets 1:poly.x
static int l_ent_x( lua_State *L ) {
    return 1;
}
// args 1:Entity_t*
// rets 1:poly.y
static int l_ent_y( lua_State *L ) {
    return 1;
}

// args 1:string
static int l_entSetHitSfx( lua_State *L ) {
    return 0;
}

// args 1:string
static int l_entSetHitParticleEffect( lua_State *L ) {
    return 0;
}

// args 1:string
static int l_entSetWeapon( lua_State *L ) {
    return 0;
}

// args 1:string
static int l_entSetScript( lua_State * L ) {
    return 0;
}

/*
	"moneye-head",					-- texture
	2,								-- health
	1,								-- manaballamount
	1,								-- exp
	1,								-- money
	28,								-- collideRadius (only used if hit entities is on)
	STATE_IDLE,						-- initState
	128,							-- sprite width	
	128,							-- sprite height
	1,								-- particle "explosion" type, maps to particleEffects.txt -1 = none
	1,								-- 0/1 hit other entities off/on (uses collideRadius)	
	4000,							-- updateCull -1: disabled, default: 4000
	1
*/
static int l_entBaseSetup( lua_State * L ) {
    return 1;
}

static int l_entGetNearestEnt( lua_State * L ) {
    return 1;
}

static int l_entGetState( lua_State * L ) {
    return 1;
}
static int l_entSetState( lua_State * L ) {
    return 1;
}

static int l_debug( lua_State * L ) {
//    console.Printf( "sizeof(lua_State) == %u", sizeof(lua_State) );
    lua_gettable( L, LUA_GLOBALSINDEX );
    return 1;
}

static int l_getGlobalTable( lua_State * L ) {
    lua_gettable( L, LUA_GLOBALSINDEX );
    return 1;
}

//
//
/////////////////////////////////////////////////////////////////////////////

/*
====================
 G_AttachBindings
====================
*/
// set the function hooks in the global lua namespace
void G_AttachBindings( lua_State * L )
{
    #ifdef SETHOOK
    #error SETHOOK was defined elsewhere, pick another macro
    #endif
    #define SETHOOK(sym) \
        lua_pushcfunction(L, l_##sym); \
        lua_setglobal(L, #sym)
    
    SETHOOK( getPlayer );
    SETHOOK( loadMap );
    SETHOOK( conPrint );
    SETHOOK( fadeOut );
    SETHOOK( fadeIn );
    SETHOOK( setZoom );
    SETHOOK( getCam );
    SETHOOK( camMove );
    SETHOOK( playMusic );
    SETHOOK( playSfx );
    SETHOOK( clearSfx );
    SETHOOK( getEnt );
    SETHOOK( newEnt );

    SETHOOK( entSetAlpha );
    SETHOOK( entSetTarget );
    SETHOOK( entGetTarget );
    SETHOOK( entDie );
    SETHOOK( ent_x );
    SETHOOK( ent_y );
    SETHOOK( entSetHitSfx );
    SETHOOK( entSetHitParticleEffect );
    SETHOOK( entSetWeapon );
    SETHOOK( entSetScript );
    SETHOOK( entBaseSetup );
    SETHOOK( entGetNearestEnt );
    SETHOOK( entGetState );
    SETHOOK( entSetState );
    SETHOOK( debug );
    SETHOOK( getGlobalTable );

    #undef SETHOOK
}

//////////////////////////////////////////////////////////////////////////////
//
// glua_*    lua helper C library functions
//
int glua_pushScriptPointer( lua_State * L ) {
    if ( !L )
        return -2;
	// these two leave the Script_t pointer on the stack
	lua_pushlightuserdata( L, L );
	lua_gettable(L, LUA_GLOBALSINDEX );
    if ( !lua_islightuserdata(L,-1) ) {
        console.Printf ( "lua: warning: Script_t* not save in globals" );
        return -1;
    }
    return 0;
}


// makes a copy of the table current at -1 and leaves it at -1
void glua_copyTable( lua_State * L ) {
    if ( !L )
        return;
    if ( !lua_istable(L,-1) ) {
        console.Printf( "warning: lua: called copyTable with no table on the stack" );
        return;
    }
    lua_newtable(L);
    
    /* first key */
    lua_pushnil(L);  
    // nil, nT, oT

    while ( lua_next(L, -3) != 0 ) {
        // val, key, nT, oT

        // copy key for the next iteration
        lua_pushvalue( L, -2 );
        // key, val, key, nT, oT

        lua_insert( L, -3 );
        // val, key, key, nT, oT

        lua_settable( L, -4 );
        // key, nT, oT
    }
    // newtable, oldtable, 
    lua_insert( L, -2 );
    // oldtable, newtable
    lua_pop( L, 1 );
    // newtable
}

void glua_clearEnv( lua_State * L ) {
    if ( !L )
        return;
    lua_newtable( L );
    lua_setfenv( L, -1 );
}

/*
====================
  glua_saveEnv

    makes a copy of the entire global namespace, and then saves it to 
    the registry with pointer as the key
====================
*/
void glua_saveEnv( lua_State * L ) {
    if ( !L )
        return;

	if ( glua_pushScriptPointer( L ) )
        return;
	// t[0] = Script_t*

	lua_pushthread( L );
	// t[0] = thread, t[1] = Script_t*

	lua_getfenv( L, -1 );
    // t[0] = _env, t[1] = thread, t[2] = Script_t*

    glua_copyTable( L );
    // _env_cpy, thread, script_t*

	lua_remove( L, -2 );
	// t[0] = _env_cpy, t[1] = Script_t*

	lua_settable( L, LUA_REGISTRYINDEX );
    // 
}

/*
====================
  glua_loadEnv

    sets the environment from a saved copy
====================
*/
void glua_loadEnv( lua_State * L ) {
    if ( !L )
        return;

    glua_clearEnv( L );

	if ( glua_pushScriptPointer( L ) )
        return;
	// t[0] = Script_t*

	lua_gettable( L, LUA_REGISTRYINDEX ); // leaves env table on top of stack
	// t[0] = _env
    if ( !lua_istable( L, -1 ) ) {
        console.Printf( "lua: warning: env table not found" );
        return;
    }

	lua_pushthread( L );
	// t[0] = thread, t[1] = _env

	lua_insert( L, -2 );
	// t[0] = _env, t[1] = thread

    // pops table from stack and saves it as thread's environment. the index of the thread 
    //  is the second value, the table should be on the top of the stack
	lua_setfenv( L, -2 ); 
	// t[0] = thread

	lua_pop( L, 1 );
	//
}


void glua_outputError( lua_State *L, const char * prefix )
{
    const char* msg = lua_tostring( L, -1 );
    if (msg == NULL)
        msg = "(error with no message)";
    lua_pop( L, 1 );
    if ( prefix ) 
        console.Printf( "%s: %s", prefix, msg );
    else
        console.Printf( "%s", msg );
    console.dump();
}

// just prints information about vals at top of stack, leaves them in place
void glua_toString( lua_State * L, int num =1, int index =-1 ) {
    int i = index;
    while ( num-- ) 
    {
        int t = lua_type(L, i);
        int top = lua_gettop(L);
        if ( top == 0 || abs(i) > top )
            return;

        switch (t) {
        case LUA_TNIL: 
            console.Printf( "%2i nil" ); 
            break;
        case LUA_TSTRING: 
            if ( lua_isstring(L,i) )
            console.Printf( "%2i `%s'", i, lua_tostring(L, i) ); 
            break;
        case LUA_TBOOLEAN: 
            if ( lua_isboolean(L,i) )
            console.Printf( "%2i %s", i, lua_toboolean(L, i) ? "true":"false");
            break;
        case LUA_TLIGHTUSERDATA: 
            if ( lua_isuserdata(L,i) )
            console.Printf( "%2i lightuserdata: %X", i, (unsigned int)lua_topointer( L, i )); 
            break; 
        case LUA_TNUMBER: 
            if ( lua_isnumber(L,i) )
            console.Printf( "%2i %g", i, lua_tonumber(L, i) ); 
            break;    
        case LUA_TTABLE:
            if ( lua_istable(L,i) )
            console.Printf( "%2i table: %s", i, "unknown" ); 
            break;
        case LUA_TFUNCTION:
        case LUA_TUSERDATA:
        case LUA_TTHREAD:
            console.Printf("%2i %s", i, lua_typename(L, t) );
            break;
        default:
            break;
        }
        --i;
    }
}

void glua_globalsDump( lua_State * L ) {
    lua_pushthread( L );
    // L

    lua_getfenv( L, -1 );
    // T, L

    lua_remove( L, -2 );
    // T

    lua_pushnil(L);  /* first key */
    // nil, T

    while ( lua_next(L, -3) != 0 ) {
        // val, key, T

        glua_toString( L, 1, -2 );
        glua_toString( L, 1, -1 );
        // val, key, T

        lua_pop( L, 1 );
        // key, T
    }

    lua_pop( L, 1 );
    //
}

// get _G. see if it yields anything differnt than getfenv
void glua_globalsDump2( lua_State * L ) {

    lua_getglobal( L, "_G" );
    // T

    lua_pushnil(L);  /* first key */
    // nil, T

    while ( lua_next(L, -3) != 0 ) {
        // val, key, T
        
        glua_toString( L, 1, -2 );
        glua_toString( L, 1, -1 );
        // val, key, T

        lua_pop( L, 1 );
        // key, T
    }

    lua_pop( L, 1 );
    //
}

void glua_stackDump( lua_State * L ) {
	console.Printf( "stackDump: " );
    int i;
    int top = lua_gettop(L);
    for (i = -1; i >= -top; i--) {  /* repeat for each level */
        if ( i == 0 ) break; // not sure of for behavior
        int t = lua_type(L, i);
        switch (t) {
        case LUA_TSTRING:  /* strings */
            console.Printf("%2i `%s'", i, lua_tostring(L, i));
            break;
        case LUA_TBOOLEAN:  /* booleans */
            console.Printf( "%2i %s", i, lua_toboolean(L, i) ? "true" : "false");
            break; 
        case LUA_TNUMBER:  /* numbers */
            console.Printf("%2i %g", i, lua_tonumber(L, i));
            break;    
        default:  /* other values */
            if ( t > LUA_TTHREAD || t < 0 )
                console.Printf("%2i %s", i, "CORRUPT STACK VALUE" );
            else
                console.Printf("%2i %s", i, lua_typename(L, t));
            break;
        }
    }
    console.Printf(" ");  /* end the listing */
}
//
//////////////////////////////////////////////////////////////////////////////

/*
============================================================

                      LuaManager_t

============================================================
*/
// since it touches so many other systems, initialize manually
void LuaManager_t::init( void ) {
    // not using threads, give each script his own lua_State
    if ( !lua_useThreads->integer() ) {
        setupScriptNames();
        return;
    }

    masterState = lua_open();
    if ( !masterState ) {
        console.Printf( "error: Lua failed to initialize" );
        console.dump();
        return;
    }
    luaL_openlibs( masterState );
    G_AttachBindings( masterState );
    setupScriptNames();

    // save a virgin copy of the fully initialized namespace before any scripts are loaded
    lua_pushlightuserdata( masterState, masterState );
    lua_pushlightuserdata( masterState, this );
    lua_settable( masterState, LUA_GLOBALSINDEX );
    glua_saveEnv( masterState );
}

//
LuaManager_t::LuaManager_t( void ) 
    : masterState(0), scriptNames(0) ,head(0), _numScripts(0) {
}

//
LuaManager_t::~LuaManager_t() { 
    if ( !V_isStarted() ) {
        if ( masterState )
            lua_close( masterState );
        masterState = NULL;
        return;
    }
    dirList.destroy();
    if ( scriptNames ) 
        V_Free( scriptNames ); 
    scriptNames = NULL;

    // destroy all children scripts
    Script_t * s = head;
    while( s ) {
        Script_t * next = s->next;
        delete s;
        s = next;
    }
    if ( masterState )
        lua_close( masterState );
    masterState = NULL;
}

const char * homogenize( const char * );
void Win_RecursiveReadDirectory( const char *, buffer_c<char*> * );

/*
==================
  setupScriptNames

    build a library of scripts available in game folder
==================
*/
void LuaManager_t::setupScriptNames( void ) {
    dirList.init();
    char s_dir[512];
    snprintf( s_dir, 511, "%s/scripts", fs_gamepath->string() ); 
    Win_RecursiveReadDirectory( s_dir, &dirList );

    scriptNames = (scriptNames_s *) V_Malloc( sizeof(scriptNames_s) * (dirList.length() + 1) );
    int i;
    for ( i = 0; i < dirList.length(); i++ ) {
        scriptNames[i].path = dirList.data[i];
        strncpy( scriptNames[i].token, homogenize( (const char *)scriptNames[i].path ), 254 );
        scriptNames[i].token[254] = '\0';
    }
    scriptNames[i].path = NULL;
    scriptNames[i].token[0] = '\0';
}

/*
====================
 LuaManager_t::startScript

    run a script by name. name must be an exact match with a token name
====================
*/
Script_t * LuaManager_t::startScript( Entity_t *_Ent, const char *name ) {
    int i;
    for ( i = 0; i < dirList.length(); i++ ) {
        // perfect match w/ token
        if ( !strcmp( scriptNames[i].token, name ) ) { 
            break;
        }
    }
    if ( dirList.length() == i ) {
        console.Printf( "script %s not run, couldn't find", name );
        return NULL;
    }

    //
    // got here: script located. create, attach and run it
    //

    glua_clearEnv( masterState );

    // create
    Script_t * s = new Script_t( this );

    // attach
    s->next = head;
    head = s;

    // connect Entity_t & name
    s->ent = _Ent;
    strcpy( s->name, name );

    // let script create and manage its own state
    // get default global namespace
    if ( lua_useThreads->integer() )
        glua_loadEnv( masterState );

    // run
    int status = luaL_loadfile( s->scriptState, scriptNames[i].path );  
    
    if ( 0 == status ) {
        ++_numScripts;
        s->resume( 0.f );
        if ( lua_useThreads->integer() ) {
            glua_saveEnv( s->scriptState );
        }
    } else {
        glua_outputError( s->scriptState, "LuaManager_t::startScript" );
    }
    return s;
}

/*
====================
 LuaManager_t::update

    manager doesn't deal in time, just does what its told
====================
*/
void LuaManager_t::update( float dt ) {
    Script_t* s = head;
    while (s) {
        s->update( dt );
        s = s->next;
    }
}

/*
====================
 unlinkScript
====================
*/
void LuaManager_t::unlinkScript( Script_t *script ) {
    Script_t * prev;

    // if script is at the head, simply unlink it
    if ( head == script ) {
        head = script->next;
        return;
    }

    // find previous link
    prev = head;
    while( prev )
    {
        if ( prev->next == script )
        {
            prev->next = script->next;
            --_numScripts;
            return;
        }
        prev = prev->next;
    }
}

/*
====================
 Frame
====================
*/
void LuaManager_t::RunScriptFrame( float dt )
{
/*
    // get elapsed seconds since last frame
    int newTime         = now();
    float elapsedSec    = (float)(newTime-oldTime) / 1000.0f;
    oldTime             = newTime;
*/

    // update the scripts
    this->update( dt );
}

void LuaManager_t::listScripts( void ) {
    if ( 0 == dirList.length() ) {
        console.Printf( "no scripts seen" );
        return;
    }
    console.Printf( "loadable scripts:" );
    for ( int i = 0; i < dirList.length(); i++ ) {
        console.Printf( "  %s" , scriptNames[i].token );
    }
}

//
Script_t * LuaManager_t::findScript( const char * tok, int useAlreadyLoaded ) {
    char buf[500];
    const char * p;

    // strip the lua extension if it's there
    if ( (p=O_strcasestr( tok, ".lua" )) ) {
        strncpy( buf, tok, p - tok );
        buf[p-tok] = '\0';
    } else {
        strcpy( buf, tok );
    }

    // check in scripts already loaded.
    Script_t * s = head;
    while ( s && useAlreadyLoaded ) {
        if ( !strcmp( s->name, buf ) )
            return s;
        s = s->next;
    }

    // if not found, check in dirList
    int i;
    for ( i = 0; i < dirList.length(); i++ ) {
        if ( !strcmp( scriptNames[i].token, buf ) ) { 
            break;
        }
    }
    
    // compared the whole list, didn't find
    if ( i == dirList.length() )
        return NULL;

    return startScript( NULL, scriptNames[i].token );
}

void LuaManager_t::printLoaded( void ) {
    if ( !head ) {
        console.Printf( "none loaded" );
        return;
    }
    console.Printf( "scripts loaded:" );
    Script_t * s = head;
    while ( s ) {
        console.Printf( "  %s", s->name );
        s = s->next;
    }
}

void LuaManager_t::killScript( Script_t *script ) {
    if ( !script )
        return;
    
    Script_t * s = head;
    while ( s ) {
        if ( s == script ) {
            s->kill(); // calls unlink and shutsdown script's lua if necessary
            delete s;
            return;
        }
        s = s->next;
    }
}

/*
==================================================================

                            Script_t

==================================================================
*/

void Script_t::init( void ) {
    manager         = NULL;
    next            = NULL;
    state           = LSS_NOTLOADED;
    scriptState     = NULL;
    wakeUpTime      = 0.f;
    waitFrame       = 0;
    time            = 0.f;
    ent             = NULL;
}

Script_t::Script_t() {
    Assert( 0 && "really shouldn't use this constructor" );
    init();
}

Script_t::Script_t( LuaManager_t *mngr ) {
    manager             = mngr;
    next                = NULL;
    state               = LSS_NOTLOADED;
    scriptState         = NULL;

    if ( lua_useThreads->integer() ) {
        // create a thread/state for this object
        scriptState = lua_newthread(manager->masterState);
        Assert( scriptState && manager && manager->masterState );
    }

    wakeUpTime          = 0.f;
    waitFrame           = 0;
    time                = 0;
    ent                 = NULL;
    
    // save a pointer to this object in the global-table
    // using the thread pointer as the key
/*
    lua_pushlightuserdata( manager->masterState, (void*)scriptState );
    lua_pushlightuserdata( manager->masterState, this );
    lua_settable( manager->masterState, LUA_GLOBALSINDEX );
*/
    if ( lua_useThreads->integer() ) {
        lua_pushlightuserdata( scriptState, scriptState );
        lua_pushlightuserdata( scriptState, this );
        lua_settable( scriptState, LUA_GLOBALSINDEX );
    }

    if ( !lua_useThreads->integer() ) {
        scriptState = lua_open();
        if ( !scriptState ) {
            console.Printf( "error: Script failed to initialize" );
            console.dump();
            return;
        }
        luaL_openlibs( scriptState );
        G_AttachBindings( scriptState );
    }
}

Script_t::~Script_t() {
    if ( lua_useThreads->integer() )
        return;
    if ( scriptState ) 
        lua_close( scriptState );
    scriptState = NULL;
}

void Script_t::resume( float dt ) {
    state = LSS_RUNNING;
   
    // scripts themselves can take arguments, run in infinite loops,
    //  be started, stopped, and yield return values.  We're not going to 
    //  use them this way, at least at first
    // lua_pushnumber( scriptState, dt );

    int status;
    if ( lua_useThreads->integer() ) {
        // Starts and resumes a coroutine in a given thread.
        status = lua_resume( scriptState, 0 );
    } else {
        status = lua_pcall( scriptState, 0, 0, 0 );
    }

    if ( status ) {
        glua_outputError( scriptState, "Script_t::resume" );
    }
}

void Script_t::update( float dt ) {
    time += dt;
    switch( state ) {
    case LSS_WAITTIME:
        if ( time >= wakeUpTime ) {
            resume( 0.0f );
            callScriptUpdate( dt );
        }
        break;
    case LSS_WAITFRAME:
        waitFrame--;
        if ( waitFrame <= 0 ) {
            resume( 0.0f );
            callScriptUpdate( dt );
            // see, depending on how I do animation, I could pass time
            // but after seeing Aquaria's code, I've decided to give animating
            // by dt a shot instead.  if I need to go back, I can just switch 
            // here what is being passed in
        }
        break;
    case LSS_NOTLOADED:
        break;
    case LSS_RUNNING:
        callScriptUpdate( dt );
        break;
    default:
        break;
    }
}

// 
void Script_t::kill( void ) {
    manager->unlinkScript( this );
    if ( lua_useThreads->integer() ) 
        return;
    lua_close( scriptState );
    scriptState = NULL;
}

void Script_t::callScriptInit( void ) {
    if ( lua_useThreads->integer() ) {
	    glua_loadEnv( scriptState ) ;
    }
    lua_pushlightuserdata( scriptState, (void*)ent );
    callFunc( "init", 1 );
}

void Script_t::callScriptUpdate( float dt ) {
    if ( lua_useThreads->integer() ) {
	    glua_loadEnv( scriptState ) ;
    }
    lua_pushlightuserdata( scriptState, (void*)ent );
    lua_pushnumber( scriptState, dt );
    callFunc( "update", 2 );
}

// funcName, numArgs
void Script_t::callFunc( const char * funcName, int numArgs, int numRets ) {
    // find the lua function and put it under the args
    lua_getglobal( scriptState, funcName );
    if ( lua_isnil(scriptState, -1) ) {
        console.Printf( "script %s: nothing to run, no call to update() found", name );
        lua_settop( scriptState, -numArgs-1 );
        return;
    }
    if ( numArgs )
        lua_insert( scriptState, -numArgs-1 );

    // now, call into Lua
    int status = lua_pcall( scriptState, numArgs, numRets, 0 );

    if ( status ) {
        glua_outputError( scriptState, "Script_t::callFunc" );
    }
}

void Script_t::printLuaVar( const char * var ) {
    if ( !var || !var[0] )
        return;
    glua_loadEnv( scriptState );
    lua_getglobal( scriptState, var );
    if ( lua_isnil(scriptState, -1) )
        return;
    console.Printf( "%s: %s\n", var, lua_tostring(scriptState, -1) );
    lua_pop(scriptState,1);
}





//////////////////////////////////////////////////////////////////////////////


void G_ListScripts( void ) {
    lua.listScripts();
}

// can run same script multiple times, so just do what we're told and load it
void G_LoadScript( const char * s ) {
    if ( !s || !s[0] )
        return;
    if ( !lua.startScript( NULL, s ) ) {
        console.Printf( "couldn't load: %s ", s ) ;
        return;
    }
    console.Printf( "%s loaded" , s ) ;
}

void G_RunScript( const char * s ) {
    if ( !s || !s[0] ) 
        return;
    Script_t * x = lua.findScript( s, 1 ); // runs any, so doesn't check whats already loaded
    if ( !x ) {
        console.Printf( "couldn't find script %s", s );
        return;
    }
    x->update( 0.f );
}

void G_PrintLoaded( void ) {
    lua.printLoaded();
}

void G_CallFunction( const char * f, const char * a1, const char * a2, const char * a3, const char * a4 ) {
}

void G_PrintValue( const char * t ) {
//    lua.printValue( t );
}

// call before vmem shutdown so we can close the script lua_States properly
void G_LuaClose( void ) {
    if ( !V_isStarted() ) {
        return; // all hope is lost ;-p
    }

    lua.dirList.destroy();
    if ( lua.scriptNames ) 
        V_Free( lua.scriptNames ); 
    lua.scriptNames = NULL;

    // destroy all children scripts
    Script_t * s = lua.head;
    while( s ) {
        Script_t * next = s->next;
        delete s;
        s = next;
    }
    if ( lua.masterState )
        lua_close( lua.masterState );
    lua.masterState = NULL;
}
