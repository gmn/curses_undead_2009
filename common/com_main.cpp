/*
 * com_main.cpp
 */ 

#include "common.h"
#include <stdlib.h>
#include <stdarg.h>
#include "../win32/win_public.h"
#include "../mapedit/mapedit.h"
#include "../map/mapdef.h"

#include <process.h> // exit()

#include "../client/cl_console.h"
#include "../game/game.h"

#include "../server/server.h"

#include "../client/cl_keys.h" // actionButton_t

#include "../game/g_lua.h"


#ifndef snprintf
#define snprintf _snprintf
#endif


#define MAX_PUSHED_EVENTS           1024
static int com_pushedEventsHead = 0;
static int com_pushedEventsTail = 0;
static sysEvent_c	com_pushedEvents[MAX_PUSHED_EVENTS];

gbool c_shutdown_initiated = gfalse;


int		com_frameTime;
int		com_frameMsec;

gvar_c *com_plix = NULL;
gvar_c *com_texttest = NULL;
int com_frametime;
gvar_c *r_maxfps = NULL;
gvar_c *com_dedicated = NULL;
gvar_c *com_developer = NULL;

gvar_c *di_mouse = NULL;

gvar_c *fs_basepath = NULL;
gvar_c *fs_game = NULL;
gvar_c *fs_gamepath = NULL;
gvar_c *fs_devpath = NULL;

gvar_c *freetype_test = NULL;

gvar_c *tex_mode = NULL;

gvar_c *v_fullscreen = NULL;
gvar_c *com_resolution = NULL;
gvar_c *com_showfps = NULL;

gvar_c *com_sleep = NULL;
gvar_c *com_editor = NULL;
extern gvar_c *g_fireRate;
extern gvar_c *g_laserVel;
gvar_c *g_useRadius = NULL;;
gvar_c *platform_type = NULL;
gvar_c *show_cursor = NULL;
gvar_c *r_vsync = NULL;
gvar_c *active_cfg = NULL;
gvar_c *pl_name = NULL;
extern gvar_c *s_volume;
extern gvar_c *s_musicVolume;
gvar_c *lua_useThreads = NULL;
gvar_c *_lockConsole = NULL;
gvar_c *godmode = NULL;



int			myargc = 0;
char **		myargv = NULL;
/*
====================
 G_CheckParm
 - checks for the existence of a cmdline arg, returning a pointer to it
    (good for getting the following argument, (ie --num-passes 2 )) 
    or else returns NULL.
 - case sensitive
====================
*/
const char ** G_CheckParm( const char *parm )
{
    int i;
    for ( i = 1; i < myargc; i++ ) {
        if ( !O_strncmp( parm, myargv[i], FULLPATH_SIZE ) ) {
            return (const char **) &myargv[i];
        }
    }
    return (const char **)0;
}
/*
====================
 G_CheckParm_i
 - same as check parm but case insensitive
====================
*/
const char ** G_CheckParm_i( const char *parm )
{
    int i;
    for ( i = 1; i < myargc; i++ ) {
        if ( !O_strncasecmp( parm, myargv[i], FULLPATH_SIZE ) ) {
            return (const char **) &myargv[i];
        }
    }
    return (const char **)0;
}



/* things in a cfg file

    - texture filtering level
    - screen resolution
    - VSYNC
    - v_fullscreen

    - sfx volume
    - music volume

    - character name
    - control bindings

    - ... any Gvars you want, really
*/
memPool<node_c<char*> > cfg_pool;
list_c<char*> cfg_file;
class savecon_c : public buffer_c<char*> {
public:
    void write( const char * fmt, ... ) {
        char buf[0x10000];
        va_list args;
        memset(buf, 0, sizeof(buf));
        va_start(args, fmt);
        vsprintf(buf, fmt, args);
        va_end(args);
    
        int len = strlen( buf );
        char * t_str = (char*) V_Malloc( len + 10 );
        strcpy( t_str, buf );
        add( t_str );
    }
    savecon_c() {
    }
    void destroyBuffer() {
        for ( int i = 0; i < length(); i++ ) {
            V_Free( data[i] );
        }
        destroy();
    }
    ~savecon_c() {
    }
};
savecon_c savecon; // this is tacky, but fuck...

void CL_ParseActiveCfg( void ) {
    FILE * fp;
    char buf[400];
    savecon.init( 32 );

    // let config provided at cmdline override default
    const char **pp = G_CheckParm_i( "active_cfg" );
    if ( pp ) {
        if ( !O_strncasecmp( "+set", *(pp-1), 100 ) ) {
            active_cfg->setString( *(pp+1) );
        }
    }

    if ( !active_cfg->string()[0] ) {
        active_cfg->setString( "player.cfg" );
    }

	cfg_pool.init(gfalse, 32 ); // small pool
    cfg_file.init(&cfg_pool);
    sprintf( buf, "%s/%s", fs_game->string(), active_cfg->string() );

    int write_cfg = 0, c;

    // not found, write default .cfg
    if ( !(fp = fopen( buf, "rb" )) ) {
        write_cfg = 1;
    } else {

        // make sure it looks valid
        while( (c=fgetc(fp)) != EOF ) {
            if ( c != ' ' || c != '\n' || c != '\r' || c != '\t' )
                break;
        }
        // got a non-whitespace char before end of file
        if ( c == EOF ) {
            fclose( fp );
            write_cfg = 1;
        }
    }

    if ( write_cfg ) {
        if ( !(fp = fopen( buf, "wb+" )) ) {
            Error( "couldn't write config %s, panicking", buf );
            /*
            console.Printf( "couldn't write config %s, panicking", buf );
            console.dump();
            */
            return;
        }
        #define P(S) fprintf(fp, "%s\n", S)
        P("tex_mode 0");
        P("com_resolution 2");
        P("r_vsync 1");
        P("v_fullscreen 1");
        P("s_musicVolume 0.55");
        P("s_volume 0.8"); // sfx volume
        P("pl_name \"defaultPlayer\"");
        P("act_forwards KEY_UP KEY_w");
        P("act_backwards KEY_DOWN KEY_s");
        P("act_left KEY_LEFT KEY_a");
        P("act_right KEY_RIGHT KEY_d");
        P("act_use KEY_SPACEBAR KEY_f");
        P("act_shoot1 KEY_MOUSE0 KEY_LCTRL");
        P("act_shoot2 KEY_MOUSE1 KEY_LALT");
        P("act_shoot3 KEY_MOUSE2" ); 
        P("act_run KEY_LSHIFT");
        P("act_alwaysRun KEY_CAPSLOCK");
        P("act_talk KEY_e" );
        P("act_map KEY_m");
        P("act_inventory KEY_TAB KEY_i");
        #undef P
        fclose( fp );
        fp = NULL;

        savecon.write( "player.cfg not found, creating default" );
    } else {
        savecon.write( "reading %s...", active_cfg->string() );
    }

    int file_sz;
    byte * file = get_filestream( buf, &file_sz );

    // read .cfg
    if ( 0 == file_sz || !file ) {
        Error( "couldn't read config: %s", buf );
        /*
        console.Printf( "couldn't read config: %s", buf );
        console.dump();*/
        return;
    }

    //console.Printf( "reading config: %s...", buf );

    char cmd[50], arg1[500], arg2[50];
    int i = 0;
    do {
        // scan a line, looking for eof or newline
        int r = i;
        while ( i < file_sz && file[i] != '\n' ) {
            ++i;
        }

        // got a line: read at least one char before newline
        if ( i > r ) {  
            strncpy( buf, (char*)&file[r], i - r );
            buf[ i - r ] = 0;
            cmd[0] = arg1[0] = arg2[0] = '\0';
            sscanf( buf, "%s %s %s", cmd, arg1, arg2 );
            // handle quoted string differently
            if ( arg1[0] == '"' ) {
                char * p = buf + strlen( cmd );
                while ( *p == ' ' ) ++p;
                strcpy( arg1, p );
                arg2[0] = 0;
            }
            if ( cmd[0] && arg1[0] ) {
                // action 
                actionButton_t *b = NULL;
                if ( (b = CL_stringToButton( cmd )) ) {
                    if ( !C_strncasecmp( arg1, "KEY", 3 ) ) {
                        b->binding[0] = CL_stringToKey( arg1 );
                    } else {
                        b->binding[0] = atoi( arg1 );
                    }
                    if ( arg2[0] && !C_strncasecmp( arg2, "KEY", 3 ) ) {
                        b->binding[1] = CL_stringToKey( arg2 );
                    } else {
                        b->binding[1] = atoi( arg2 );
                    }
                    /*
                    if ( arg2[0] ) 
                        console.Printf( "..setting action: %s = %s, %s", 
                                                    cmd, arg1, arg2 );
                    else
                        console.Printf( "..setting action: %s = %s", cmd,arg1); */
                    if ( arg2[0] )
                        savecon.write( "..setting action: %s = %s, %s", cmd, arg1, arg2 );
                    else
                        savecon.write( "..setting action: %s = %s", cmd,arg1); 
                // Gvar
                } else {
                    if ( com_editor->integer() ) {
                        // leave the mouse alone when in editor 
                        if ( !strcmp( cmd, "show_cursor" ) && !strcmp( cmd, "di_mouse" ) ) {
					        Gvar_Get( cmd, arg1, 0 );
                            //console.Printf( "..setting Gvar: %s = %s", cmd, arg1 );
                            savecon.write( "..setting Gvar: %s = %s", cmd, arg1 );
                        } else {
                            savecon.write( "**not setting %s in editor mode", cmd );
                        }
					} else {
					    Gvar_Get( cmd, arg1, 0 );
                        savecon.write( "..setting Gvar: %s = %s", cmd, arg1 );
					}
                }
                // keep track of these 
                int L = strlen(cmd) + 3;
                char * copy = (char *) V_Malloc( L );
                strcpy( copy, cmd );
                cfg_file.add( copy );
            }
        }
        while ( file[i] == '\n' ) ++i;
        while ( file[i] == '\r' ) ++i;
    } while ( i < file_sz );
    close_filestream( file );
    //console.Printf ("finished reading config");
    savecon.write("finished reading config");
}

void CL_WriteCfg( void ) {
    node_c<char *> *n = cfg_file.gethead();
    if ( !n ) {
        console.Printf( "error: list_c::cfg_file empty" );
        console.dump();
        return;
    }

    char buf[500];
    FILE *fp = NULL;
    sprintf( buf, "%s/%s", fs_game->string(), active_cfg->string() );
    if ( !(fp = fopen( buf, "wb+" )) ) {
        console.Printf( "couldn't write config %s, panicking", buf );
        console.dump();
        return;
    }

    #define P(C,A,B) \
        do { \
        if ( B ) { \
            fprintf(fp, "%s %s %s\n", C, A, B); \
        } else { \
            fprintf(fp, "%s %s\n", C, A); \
        } \
        } while(0)

    actionButton_t *b;
    while ( n ) {
        // button
        if ( (b = CL_stringToButton( n->val )) ) {
			const char * A = CL_keyToString(b->binding[0]);
			const char * B = CL_keyToString(b->binding[1]);
            P( n->val, A, B ); 
        // gvar
        } else {
            gvar_c *gv = Gvar_Find( n->val );
			const char *S = gv ? gv->string() : NULL;
            if ( !strcmp( n->val, "pl_name" ) )
                fprintf(fp, "%s \"%s\"\n", n->val, gv->string() );
            else
                fprintf(fp, "%s %s\n", n->val, gv->string() );
        }
        n = n->next;
    }
    #undef P
    fflush( fp );
    fclose( fp );
}

buffer_c<char*> gameDef;
int gameDefLines = 0;
int currentDef = 0;
bool game_command_given = false;

void Com_ReadGameDef( void ) {
	// pointers to strings
	gameDef.init( 128 );
	filehandle_t fh = 0;
	char path[512];
	char *buf = NULL;
	snprintf( path, sizeof(path)-1, "%s\\game.def", fs_game->string() );
	int sz = 0;
	if ( !(sz = FS_SlurpFile( path, (void**)&buf )) ) {
		//console.pushMsg( "couldn't find game.def!  bailing." );
		//console.dumpcon( "console.DUMP" );
		Err_Fatal( "couldn't find game.def! bailing." );
		Error( "couldn't find game.def! bailing." );
		Com_HastyShutdown();
		return;
	}
	if ( !buf ) {
		//console.pushMsg( "couldn't find game.def!  bailing." );
		//console.dumpcon( "console.DUMP" );
		Err_Fatal( "couldn't find game.def! bailing." );
		Error( "couldn't find game.def! bailing." );
		Com_HastyShutdown();
		return;
	}	
	goto start_loop;

	char *b, *e, tmp;

line_beg:
	tmp = *e;
	*e = '\0';
	gameDef.add( b );
	*e = tmp;
	b = e + 1;
	++e;
	++gameDefLines;
	if ( e - buf >= sz )
		goto done;
	goto line_end;

start_loop:
	// store each line of the game.def 
	e = b = buf;
	while ( e - buf < sz ) {
line_end:
		// msft endline '\r\n'
		if ( *e == '\r' ) {
			*e = '\0';
			if ( *(e+1) == '\n' ) {
				*++e = 0;
				goto line_beg;
			}
		}
		// found a line, save it
		if ( *e == '\n' || !*e ) {
			goto line_beg;
		}
		++e;
	}
done:
	Assert( gameDefLines == gameDef.length() );
}


void Com_Quit_f( void ) {
    // shut stuff down,
    // have a system to register things that require an
    //  explicit shutdown, then just send a signal to that system
    //  and let it keep track of shutdowns 
	Com_HastyShutdown();
}


// proper
#if 0
void Com_Shutdown( void ) 
{
    Com_Printf( "shutting down ... \n" );
    S_Shutdown();

    Sys_DestroyDIMouse();

    WGL_KillWindow();
    V_Shutdown();

    // reset sys timer
    timeEndPeriod( 1 );

    exit(1);
}
#endif


com_logging_c com_logger;
void Com_Printf( char *fmt, ... ) {
    return ; // ignore this for now

	char msg[4096];
    va_list argp;

	memset( msg, 0, sizeof(msg) );
	com_logger.start();

	va_start (argp, fmt);
	_vsnprintf( msg, 4096, fmt, argp );
	fprintf ( com_logger.com_file_p, "%s", msg );
	va_end (argp);
}



void Com_InitPushedEvent ( void ) 
{
  memset( com_pushedEvents, 0, sizeof(com_pushedEvents) );

  com_pushedEventsHead = 0;
  com_pushedEventsTail = 0;


}


/*
=================
Com_PushEvent
=================
*/
void Com_PushEvent( sysEvent_c *event ) {
	sysEvent_c		*ev;
	static gbool printedWarning = gfalse; 

	ev = &com_pushedEvents[ com_pushedEventsHead & (MAX_PUSHED_EVENTS-1) ];

	if ( com_pushedEventsHead - com_pushedEventsTail >= MAX_PUSHED_EVENTS ) {

		// don't print the warning constantly, or it can give time for more...
		if ( !printedWarning ) {
			printedWarning = gtrue;
			Com_Printf( "WARNING: Com_PushEvent overflow\n" );
		}

		if ( ev->evPtr ) {
			V_Free( ev->evPtr );
		}
		com_pushedEventsTail++;
	} else {
		printedWarning = gfalse;
	}

	*ev = *event;
	com_pushedEventsHead++;
}



/*
====================
 Cmdline Arguments

 purpose: turn windows cmdline string into standard C-style argc, argv
====================
*/

int Com_ParseArgv( const char *cmdline ) {

	// copy
	int bufsz = strlen( EXECUTABLE_NAME ) + 1 + strlen( cmdline ) + 1;
	char *buf = (char*) V_Malloc( bufsz );
	snprintf( buf, bufsz, "%s %s", EXECUTABLE_NAME, cmdline );
	buf[ bufsz - 1 ] = '\0';

    char *p = buf;
	int sawchar = 0;

	// count arguments
	while ( *p ) {
		if ( *p == '"' ) {
			++p;
			if ( *p != '"' ) {
				++sawchar;
				while ( *p && *p != '"' ) {
					++p;
				}
				if ( *p && *p == '"' ) {
					sawchar = 0;
					++myargc;
					*p++ = '\0';

					while ( *p && *p == ' ' )
						*p++ = '\0';

					// quoted string happened to be at the exact end
					if ( !*p ) {
						break;
					}
				}

			} else if ( *p == '"' ) {
				++p;
			}
		}

		// creep over characters, noting we saw them
		else if ( *p != ' ' )
			sawchar++;

		// found a space after seeing characters, or quoted string
		if ( *p == ' ' && sawchar ) {
			++myargc;
			sawchar = 0;
			// turn spaces into null and increment 
			while ( *p && *p == ' ' )
				*p++ = '\0';
		} else if ( p - buf < bufsz ) {
			++p;
		} else {
		}
		
		// found null at end of cmdline, after last word
		if ( !*p && sawchar ) {
			++myargc;
		}
	}

	// create array of pointers
	myargv = (char**) V_Malloc( myargc * sizeof(char**) );
	memset( myargv, 0, myargc * sizeof(char**) );

	int i = 0;
	int in_word = 0;
	int arg = 0;
	while ( i < bufsz ) {
		// found non-null char, mark arg
		if ( buf[i] != '\0' ) {
			// start of quoted string
			if ( buf[ i ] == '"' ) {
				++i;
				myargv[ arg++ ] = &buf[ i ];

				// eat up string until NULL
				while( buf[i] != '\0' ) {
					if ( ++i >= bufsz ) {
						break;
					}
				}

			} else {
				myargv[ arg++ ] = &buf[ i ];
				// and eat up rest of arg
				while( buf[i] != '\0' ) {
					if ( ++i >= bufsz ) {
						break;
					}
				}
			}
		} else
			++i;
		
		// eat null chars
		while ( i < bufsz && buf[i] == '\0' )
			++i;
	}

	Assert( arg == myargc );
	return 0;
}

#if 0 


int Com_ParseArgv ( const char *cmdline )
{
    const char *p; 
	const char *arg;
    int i, index;
	int sawchar = 0;
    char *buf;
    //gwchar_t *wbuf;
    int totalsz;

    myargc = 1;
    p = cmdline;

    // eat up prepend space
    while (*p == ' ')
        ++p;

    // space separated argv , null terminated
    for (;;)
    {
        // points to something
		if (*p != ' ' && *p != '\0') {
            ++p;
			sawchar++;
		}
        // points to ' ' or '\0'
        else
        {
            if (*p == ' ') {
                ++myargc;

                // eat up extra space til next arg
                while (*++p == ' ' )
                    ;
            } 
            else if (*p == '\0')
            {
				if (sawchar)
	                ++myargc;
                break;
            }
        }
    }


    // 
    // put pointers on front, buf w/ strings following, connect the
    //  pointers to them
    //
    
    //totalsz = sawchar + myargc-1 + C_wstrlen(EXECUTABLE_NAME);
    totalsz = sawchar + myargc-1 + strlen( EXECUTABLE_NAME );

	totalsz = strlen( EXECUTABLE_NAME ) + 1 + strlen( cmdline ) + 1;

    //myargv = (gwchar_t **) V_Malloc ( myargc * sizeof(gwchar_t *) );
    myargv = (char **) V_Malloc ( myargc * sizeof(char *) );

    //wbuf = (gwchar_t *) V_Malloc ( totalsz * (sizeof(gwchar_t)/sizeof(char)) );
    buf = (char *) V_Malloc ( totalsz * sizeof(char) );

    //index = (sizeof(gwchar_t)/sizeof(char));


    index = 0;
	//index += C_wstrncpy ( &wbuf[0], EXECUTABLE_NAME, C_wstrlen(EXECUTABLE_NAME) );
	strncpy ( &buf[0], EXECUTABLE_NAME, strlen(EXECUTABLE_NAME) );
	index += strlen( EXECUTABLE_NAME );
	
	myargv[0] = buf;
    myargv[1] = &buf[++index];

    p = arg = cmdline;

    // eat up any prepend spaces
    while ( *p == ' ' ) {
        ++arg;
        ++p;
    }

    i = 1;
	sawchar = 0;
    for(;;)
    {
		if (*p != ' ' && *p != '\0') {
            ++p;
			sawchar++;
		}
        else
        {
            // hit a space, end of arg
            if ( *p == ' ' ) {

                //index += C_wstrncpyUP( myargv[i], arg, p - arg + 1 );
				strncpy( myargv[i], arg, p - arg + 1 );
                index += ( p - arg + 1 );
                //myargv[i][p - arg + 1] = '\x0000';
                myargv[i][p - arg + 1] = '\0';

				if (i < myargc-1) 
	                myargv[++i] = &buf[++index];

                // eat up any extra space til next arg
                while ( *++p == ' ' )
                    ;
                arg = p;
            } 
            else if (*p == '\0')
            {
				if (sawchar)
                    strncpy( myargv[i++], arg, p - arg + 2 );
                    //C_wstrncpyUP( myargv[i++], arg, p - arg + 2 );
                break;
            }
        }
    }

    if (i == myargc)
        return 0;

    return -1;
}
#endif


const char * Com_GetArgv ( int i ) {
	if (i >= myargc)
		return (char *)NULL;
	return myargv[i];
}



// either get an event from the system or from a .. journal file?
// insert journalling interlocutor here when apropriate
sysEvent_c Com_GetRealEvent( void )
{
    sysEvent_c ev;

    // if ( com_journaling->on )
    //      do journal crap
    // else

    ev = Sys_GetEvent();

    return ev;
}

sysEvent_c Com_GetEvent( void )
{
	if ( com_pushedEventsHead > com_pushedEventsTail ) {
		com_pushedEventsTail++;
		return com_pushedEvents[ (com_pushedEventsTail-1) & (MAX_PUSHED_EVENTS-1) ];
	}
	return Com_GetRealEvent();
}

// Com_RunAndTimeServerPacket

//void Plix_KeyEvent (int key, int down, unsigned int time);

int Com_EventLoop( void ) 
{
	sysEvent_c	ev;
//	netadr_t	evFrom;
//	byte		bufData[MAX_MSGLEN];
//	msg_t		buf;

//	MSG_Init( &buf, bufData, sizeof( bufData ) );

	while ( 1 ) {
		ev = Com_GetEvent();

		// if no more events are available
		if ( ev.evType == SE_NONE ) 
        {
            /* nets later... 
             *
             *
			// manually send packet events for the loopback channel
			while ( NET_GetLoopPacket( NS_CLIENT, &evFrom, &buf ) ) {
				CL_PacketEvent( evFrom, &buf );
			}

			while ( NET_GetLoopPacket( NS_SERVER, &evFrom, &buf ) ) {
				// if the server just shut down, flush the events
				if ( com_sv_running->integer ) {
					Com_RunAndTimeServerPacket( &evFrom, &buf );
				}
			}
            */

			return ev.evTime;
		}


		switch ( ev.evType ) {
		default:
			Com_Error( ERR_FATAL, "Com_EventLoop: bad event type %i", 
                    ev.evType );
			break;
        case SE_NONE:
            break;
		case SE_KEY:
			CL_KeyEvent( ev.evValue, ev.evValue2, ev.evTime );
			break;
		case SE_CHAR:
			CL_CharEvent( ev.evValue );
			break;
		case SE_MOUSE:
			CL_MouseEvent( ev.evPtr, ev.evTime );
			break;
		case SE_JOYSTICK_AXIS:
			//CL_JoystickEvent( ev.evValue, ev.evValue2, ev.evTime );
			break;
		case SE_CONSOLE:
            /*
			Cbuf_AddText( (char *)ev.evPtr );
			Cbuf_AddText( "\n" );
            */
			break;
		case SE_PACKET:
			break;
            /*
			// this cvar allows simulation of connections that
			// drop a lot of packets.  Note that loopback connections
			// don't go through here at all.
			if ( com_dropsim->value > 0 ) {
				static int seed;

				if ( Q_random( &seed ) < com_dropsim->value ) {
					break;		// drop this packet
				}
			}

			evFrom = *(netadr_t *)ev.evPtr;
			buf.cursize = ev.evPtrLength - sizeof( evFrom );

			// we must copy the contents of the message out, because
			// the event buffers are only large enough to hold the
			// exact payload, but channel messages need to be large
			// enough to hold fragment reassembly
			if ( (unsigned)buf.cursize > buf.maxsize ) {
				Com_Printf("Com_EventLoop: oversize packet\n");
				continue;
			}
			Com_Memcpy( buf.data, (byte *)((netadr_t *)ev.evPtr + 1), buf.cursize );
			if ( com_sv_running->integer ) {
				Com_RunAndTimeServerPacket( &evFrom, &buf );
			} else {
				CL_PacketEvent( evFrom, &buf );
			}
			break;
            */
		}

		/*
		// free any block data
		if ( ev.evPtr ) {
			V_Free( ev.evPtr );
		}
		*/
	}

	return 0;	// never reached
}




unsigned int com_framenumber = 0;



static void Com_Frame ( void ) 
{
    int msec, minMsec;
    static int lastMsec = now();
    
	/*
	================================================================ 

		Server Section

	================================================================ 
	*/
    
	/* always true */ 
    if ( !com_dedicated->integer() && r_maxfps->integer() > 0 )
    {
        minMsec = 1000 / r_maxfps->integer();
    } else {
        minMsec = 1;
    }

    // catch up with events
    do {
        com_frametime = Com_EventLoop();
        msec = com_frametime - lastMsec;
    } while (msec < minMsec);
    
	/* command buf execute */
    // Cbuf_Execute();
    
	/* server frame */
    SV_Frame( msec );


	/*
	================================================================ 

		Client Section

	================================================================ 
	*/
    
    // not dedicated
    if (!com_dedicated->integer() ) {
        Com_EventLoop();
		/* execute command buf again, before running client frame  */
    	// Cbuf_Execute();
        CL_Frame( msec );
    }
    
    Com_EndFrame();

    com_framenumber++;
}

void Sys_GetPaths( void );

void Com_SetGvarDefaults( void ) {

	com_plix = Gvar_Get( "com_plix", "0", 0 );
	com_texttest = Gvar_Get( "com_texttest", "0", 0 );

	com_dedicated = Gvar_Get( "com_dedicated", "0", 0 );
	com_developer = Gvar_Get( "com_developer", "1", 0 );
	r_maxfps = Gvar_Get( "r_maxfps", "120", 0 );

	di_mouse = Gvar_Get( "di_mouse", "1", 0 );

	fs_basepath = Gvar_Get( "fs_basepath", "", 0 );
	fs_game = Gvar_Get( "fs_game", "zpak" , 0 );
	fs_gamepath = Gvar_Get( "fs_gamepath", "", 0 );
    Sys_GetPaths(); // sets fs_basepath, fs_gamepath

	//fs_devpath = NULL;

	freetype_test = Gvar_Get( "freetype_test", "0", 0 );


	// texture filtering mode
	tex_mode = Gvar_Get( "tex_mode", "0", 0, "texture filtering mode: 0:off, 1:linear no mipmap, 2:off+mipmap, 3:linear+mipmap, 4:off w/ mipmap trilinear, 5: bilinear w/ mipmap trilinear" );

	v_fullscreen = Gvar_Get( "v_fullscreen", "0", 0 );

	// default to 1024x768
	com_resolution = Gvar_Get( "com_resolution", "2", 0 );

	com_showfps = Gvar_Get( "com_showfps", "0", 0 );

	com_sleep = Gvar_Get( "com_sleep", "6", 0 );
	
	com_editor = Gvar_Get( "com_editor", "0", 0 );

	g_fireRate = Gvar_Get( "g_fireRate", "300", 0 );
	g_laserVel = Gvar_Get( "g_laserVel", "4500", 0 );

	g_useRadius = Gvar_Get( "g_useRadius", "160", 0 );

    // 1: 2d, right side up
    platform_type = Gvar_Get( "platform_type", "1", 0, "1: 2d right side up fixed. 2: mouse rotated screen orientation" );

    show_cursor = Gvar_Get( "show_cursor", "0", 0 );
    r_vsync = Gvar_Get( "r_vsync", "1", 0 );
    active_cfg = Gvar_Get( "active_cfg", "player.cfg", 0 );
    pl_name = Gvar_Get( "pl_name", "defaultPlayer", 0 );

    s_volume        = Gvar_Get( "s_volume", "0.8", 0 );
	s_musicVolume	= Gvar_Get( "s_musicVolume", "0.55", 0 );
    lua_useThreads  = Gvar_Get( "lua_useThreads", "1", 0 );
    _lockConsole    = Gvar_Get( "_lockConsole", "0", 0 );
    godmode         = Gvar_Get( "godmode", "0", 0 );
}

void Com_SetCmdLineGvars( void ) {
	const char **p = NULL;
	int i = 1;
	while ( i < myargc ) {
		if ( !O_strncasecmp( myargv[i], "+set", FULLPATH_SIZE ) ) {
			if ( i + 1 < myargc ) {
				const char *arg = myargv[ i + 1 ];
				// have arg, find value
				if ( i + 2 < myargc ) {
					const char *val = myargv[ i + 2 ];

					// have value, set in gvars
					Gvar_Get( arg, val, 0 );
					i += 1;
				}
				i += 1;
			}
			i += 1;
		} else {
			i += 1;
		}
	}
}

void Com_SetCmdLineSwitches( void ) {
	const char **p = NULL;
	if ( (p = G_CheckParm_i( "-nice" ) ) ) {
		v_fullscreen->set( "v_fullscreen", "1", 0 ); 
		com_resolution->set( "com_resolution", "5", 0 );
	}
	if ( G_CheckParm_i( "--fullscreen" ) || G_CheckParm_i( "-fs" ) ) {
		v_fullscreen->set( "v_fullscreen", "1", 0 ); 
	}

	// +set com_editor || +editor
	if ( G_CheckParm( "+editor" ) ) {
		com_editor->set( "com_editor", "1", 0 );
	}

}

static void Com_AssureGvarSaneState( int ask_editor ) {

    /* no, this was an invalid presumption, because now it breaks the editor cmd from game.def

    // the one thing that overrides game.def is when they change editor state
    if ( ask_editor != com_editor->integer() ) {
        com_editor->setInt( ask_editor );  
    } */

	// editor requires certain stuff
	if ( com_editor->integer() ) {
		MainGame.PushGameCommand( "editor", NULL, 0 ); 
		game_command_given = true;
        show_cursor->setInt( 1 );
        di_mouse->setInt( 0 );
	}
}

void write_saved_console_messages( void ) {
    if ( !savecon.isstarted() )
        return;
    for ( int i = 0; i < savecon.length(); i++ ) {
        console.pushMsg( savecon.data[i] );
    }
    savecon.destroyBuffer();
}

/* set gvars in order of:
    1 - 1st set defaults
    2 - then read player.cfg, allow to override
    3 - then apply cmdline parms, overriding config
    4 - read game.def 
    5 - assure sanity
*/
static void Com_SetGvars( void ) 
{
    // 1 - defaults already set in Com_Init

    // 1.5 - create fs_game->string() directory if not exist (but check cmdline)

    // 2 - player.cfg
    CL_ParseActiveCfg();

    // 3 - 
    // user provided gvar args, overrides game.def & player.cfg
    Com_SetCmdLineGvars();
    Com_SetCmdLineSwitches(); // shortcuts

    int do_editor = com_editor->integer();

    // 4 - 
    // if +editor, com_editor and active_map are not set, read game.def
    // note: also could set master luaScript from cmdline 
    if ( !com_editor->integer() && !me_activeMap->integer()) {
		Com_ReadGameDef();
        if ( MainGame.CreateGameCommands( gameDef ) ) {
            game_command_given = true;
        }
    }

    // 5 - 
    // after all possible methods to attain/set gvars have run
    Com_AssureGvarSaneState( do_editor );
}

static void Com_Init ( const char *argv )
{
    Com_InitTimers();  // gets base msec

	Com_Printf( "\n--------------------------------------------------\n" );
	Com_Printf( "  starting new session: %s", Sys_GetDateTime() );
	Com_Printf( "--------------------------------------------------\n" );

    // allocate & setup virtual memory
	V_Init();

	// set gvar's default values
	Com_SetGvarDefaults();

	// init these even if playing regular game because we use ME_ReadMap()
	//  in regular client code (at least right now)
	ME_InitGvars();

	// local server vars
	SV_Init();

    Com_ParseArgv ( argv );

    // reset file hash table
    FS_InitialStartup();

    //Cmd_Init();

    // Netchan_Init() // set it up but dont start it
    //SV_Init()

    // 
    Com_SetGvars();

    // setup client state
    CL_Init(); 
    write_saved_console_messages();    
    
    // all State initialization should be done before systems are started 

	Com_InitOSSubSystems(); // calls Win_InitSubsystem()

	// call before starting gl client
	if ( v_fullscreen->integer() ) {	
		Win_SetFullscreen( gtrue );
	}

	// start the client systems
    CL_StartClient(); 
    
    // start scripting system
    lua.init(); 
}



int Com_Main ( const char *argv )
{
	Com_Init( argv );
	Com_Printf( "\nCom Initialization Complete.\n" );

    // main game loop
    for(;;)
    {
        Input_Frame();

        Com_Frame();

		if ( c_shutdown_initiated )
			break;

		if ( com_sleep->integer() ) {
			Com_Sleep( com_sleep->integer() );
		}
    }
    return 0;
}



