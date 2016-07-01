#include "lib.h"
#include "lib_parser.h"
#include "../client/cl_console.h"
#include "../mapedit/mapedit.h"
#include "../map/mapdef.h"
#include "../common/com_vmem.h"


#ifdef _WIN32
#define snprintf _snprintf
#endif

int CL_ReloadMap( void );
void CL_BindKey( const char *, const char * );
void CL_UnBindKey( const char * );

// g_lua.cpp
void G_ListScripts( void );
void G_LoadScript( const char * );
void G_RunScript( const char * );
void G_PrintLoaded( void );
void G_CallFunction( const char * f, const char * a1, const char * a2, const char * a3, const char * a4 );


cmd_lut_t commands[] = {
{ TOK_NONE, "", 0, 0, "Emptyness: The Void", NULL },
{ TOK_SAVE, "save", 1, 0, "save [fileName] (saves current map)", NULL },
{ TOK_QUIT, "quit", 0, 0, "quit (quit)", NULL },
{ TOK_DUMP, "dump", 1, 0, "dump [filenameToSaveAs] (dump console to textfile)", NULL },
{ TOK_FILE, "file", 1, 1, "file <textfileToLoad> (loads a textfile into con)", NULL },
{ TOK_MAP, "map", 1, 1, "map <mapToLoad> (loads a map)", NULL },
{ TOK_LISTCMDS, "listcmds", 1, 0, "listcmds (lists available commands)", NULL },
{ TOK_LISTGVARS, "listgvars", 0, 0, "listgvars (lists gvars)", NULL },
{ TOK_CLEAR, "clear", 0, 0, "clears console", NULL },
{ TOK_HELP, "help", 0, 0, "prints help", NULL },
{ TOK_EXIT, "exit", 0, 0, "gives advice", NULL },
{ TOK_SET, "set", 99, 0, "set <gvar> <value> set/create console var", NULL },
{ TOK_SETA, "seta", 2, 2, "seta <gvar> <value> set/create console var and archive", NULL },
{ TOK_UID, "uid", 0, 0, "displays list of uids of selected tiles", NULL },
{ TOK_LAYERINFO, "layerinfo", 0, 0, "prints the layer # of the selected tiles", NULL },
{ TOK_LOCKINFO, "lockinfo", 0, 0, "prints the locks of the selected tiles", NULL },
{ TOK_TILEINFO, "tileinfo", 0, 0, "displays information about all tiles" },
{ TOK_UNLOCK_ALL, "unlock_all", 0, 0, "unlocks all tiles" },
{ TOK_LOCK_ALL, "lock_all", 0, 0, "locks all tiles" },
{ TOK_RESCAN_ASSETS, "rescan_assets", 0, 0, "scans the assets directory, looking for new assets to make materials out of and add to the material palette", NULL },
{ TOK_VID_RESTART, "vid_restart", 0, 0, "restart the video system" , NULL }, 
{ TOK_CREATE_COLOR_MATERIAL, "create_color_material", 4, 4, "create_color_material <float> <float> <float> <float> (red, green, blue, alpha)", NULL },
{ TOK_MTLINFO, "mtlinfo", 1, 0, "mtlinfo [mtl_id] : prints prints info about material w/ mtl_id if one is provided, otherwise about selected material(s)", NULL },
{ TOK_COMBINE, "combine", 0, 0, "combines the colModels of two or more selected tiles.  only does very rough approximation of max width and height, so be careful using it." , NULL },
{ TOK_BIND, "bind", 2, 0, "map a key to a command", NULL },
{ TOK_UNBIND, "unbind", 1, 1, "unbind a kay", NULL },
{ TOK_PALETTE, "palette", 1, 0, "create a new palette/map", NULL },
{ TOK_RM, "rm", 0, 0, "remove current palette/map", NULL },
{ TOK_PALETTEINFO, "paletteinfo", 0, 0, "print info on loaded palettes", NULL },
{ TOK_HISTORY, "history", 0, 0, "prints the command history" , NULL },
{ TOK_BACKGROUND, "background", 0, 0, "turns the selected tile into a background", NULL },
{ TOK_UNSET_BACKGROUNDS, "unset_backgrounds", 0, 0, "unsets all backgrounds so that they can be maninpulated again", NULL },
{ TOK_CREATE_COLOR_MASK, "create_color_mask", 4, 4, "create_colormask <float> <float> <float> <float> - make a colormask material out of a currently selected textured mapTile", NULL },
{ TOK_DUP, "dup", 0, 0, "duplicate single selected material (useful if it's not already loaded into the materials list)", NULL },
{ TOK_CREATE_TEXTURE_MASK, "create_texture_mask", 0, 0, "combines two textures, if one is textureStatic and one is colorMask, into a textureMask", NULL },
{ TOK_SET_COLOR, "set_color", 4, 4, "set_color <f> <f> <f> <f> - set the color of a material, may change it's type" , NULL },
{ TOK_RELOAD, "reload", 0, 0, "reload texture directory", NULL },
{ TOK_MAG, "mag", 1, 0, "prints or sets viewport magnification", NULL },
{ TOK_ZOOM, "zoom", 1, 0, "prints or sets viewport magnification", NULL },
{ TOK_VIEW, "view", 3, 0, "view <X> <Y> <ZOOM> - prints or sets the viewport", NULL },
{ TOK_RESYNC, "resync", 0, 0, "re-set colModels (of selected) to syncronize to tile geometry in selected tiles", NULL },
{ TOK_CREATE_AREA, "create_area", 1, 0, "create_area [name].  create an Area out of selected tiles.", NULL },  
{ TOK_AREA_INFO, "area_info", 0, 0, "print info about all Area_t and subArea_t" , NULL },
{ TOK_SET_NAME, "set_name", 1, 1, "set the name of the selected area", NULL },
{ TOK_CREATE_SUB_AREA, "create_sub_area", 0, 0, "create a subArea from selected", NULL },
{ TOK_DELETE_AREA, "delete_area", 1, 1, "delete_area <name>", NULL },
{ TOK_DELETE_SUB_AREA, "delete_sub_area", 1, 1, "delete_sub_area <name>", NULL },
{ TOK_LIST_ENTITY_TYPES, "listentitytypes", 0, 0, "shows all possible entity macro types", NULL },
{ TOK_LIST_ENTITIES, "listentities", 0, 0, "output listing of all entities in world", NULL },
{ TOK_ENTITY, "entity", 1, 1, "entity <entityType>  (create an entity)", NULL },
{ TOK_ENT, "ent", 4, 0, "ent <command> [para1][parm2][parm2]  (modify fields in entities.  type 'ent help' for a list of commands and their usage" },
{ TOK_NUDGE, "nudge", 2, 2, "nudge [x|y|w|h] <float>  -- adjust selected colModels" },
{ TOK_AREA, "area" , 4, 0, "[|create|name|x|y|w|h|delete|info|info all|help|current] <parms..> various commands for manipulating areas.  Type 'area help' for complete instructions" },
{ TOK_BGINFO, "bginfo", 0, 0, "Prints MTL, UID and location of all backgrounds" },
{ TOK_MEM, "mem", 0, 0, "prints info on memory usage" },
{ TOK_UNSETBG, "unsetbg", 1, 1, "unsetbg <UID>   unset 1 background by uid" },
{ TOK_FTC, "ftc", 0, 0, "ftc -- fast, two argument calculator" },
{ TOK_RESTART, "restart", 0, 0, "restart the current map (in game)" },
{ TOK_PAUSE, "pause", 0, 0, "pause/unpause the game" },
{ TOK_MUSIC, "music", 2, 0, "'music help' for usage" },
{ TOK_LISTSCRIPTS, "listscripts", 0, 0, "prints a list of lua scripts available to load" },
{ TOK_LOADSCRIPT, "loadscript", 1, 1, "loads a script" },
{ TOK_RUN, "run", 1, 1, "runs a script" },
{ TOK_SCRIPTS, "scripts", 0, 0, "prints scripts loaded" },
};

const unsigned int TOTAL_COMMANDS = sizeof(commands)/sizeof(commands[0]);


// FIXME: not finished , not implemented
// list of commands that should be disabled when not running the map editor
cmd_t mapEditOnly[] = {
	TOK_UID,
	TOK_LAYERINFO,
	TOK_LOCKINFO,
	TOK_TILEINFO,
	TOK_UNLOCK_ALL,
	TOK_LOCK_ALL,
	TOK_CREATE_COLOR_MATERIAL,
	TOK_MTLINFO, 
	TOK_COMBINE, 
	TOK_BIND,
	TOK_PALETTE,
	TOK_RM,
	TOK_PALETTEINFO,
	TOK_BACKGROUND,
	TOK_UNSET_BACKGROUNDS,
	TOK_CREATE_COLOR_MASK,
	TOK_DUP,
	TOK_CREATE_TEXTURE_MASK,
	TOK_SET_COLOR,
	TOK_RESYNC,
	TOK_CREATE_AREA,
	TOK_AREA_INFO,
	TOK_SET_NAME,
	TOK_CREATE_SUB_AREA,
	TOK_DELETE_AREA,
	TOK_DELETE_SUB_AREA,
	TOK_LIST_ENTITY_TYPES,
	TOK_LIST_ENTITIES,
	TOK_ENTITY,
};

// list of commands that are only to be used during cgame
cmd_t cgameOnly[] = {
	TOK_LIST_ENTITIES,
};






int parser_t::findCmd( const char *str ) {
	size_t clen = strlen( str ) + 1;
	for ( unsigned int i = 1 ; i < TOTAL_COMMANDS; i++ ) {
		if ( !O_strncasecmp( str, commands[i].string, clen ) ) {
			return i;
		}
	}
	return 0;
}

int parser_t::tokenize( const char *str ) {

	// first thing to do is to cut string

	size_t len = strlen( str ) + 1;
	size_t buflen = ( len + 32 ) & ~31;

	argBuf.reset( buflen );

	strncpy ( argBuf.vec, str, len );

	words.start();

	// replace whitespace 
	int i = 0;
	while ( i < len && argBuf.vec[i] ) {
		if ( argBuf.vec[i] == ' ' || 
			argBuf.vec[i] == '\n' || 
			argBuf.vec[i] == '\r' || 
			argBuf.vec[i] == '\t' ) 
		{
			argBuf.vec[i] = '\0';
		}
		++i;
	}

	// skip to first word
	i = 0;
	while ( i < len-1 && argBuf.vec[i] == '\0' ) ++i;
	if ( i == len-1 )
		return 0;

	// add tokens
	do {
		words.add ( &argBuf.vec[i] );
		// advance past remaining characters of token
		while ( i < len-1 && argBuf.vec[i] != '\0' ) ++i;
		if ( i >= len-1 )
			break;
		// advance past trailing nullspace, if any
		while ( i < len-1 && argBuf.vec[i] == '\0' ) ++i;
		if ( i >= len-1 )
			break;
	} while(1);

	return words.size() > 0;
}

static void PARSER_memUsage( void ) {
	console.Printf( "default page size  : %d ", bigPageBytes );
	console.Printf( "in use (bytes)     : %d ", V_MemInUse() );
	console.Printf( "peek usage         : %d ", V_MemPeek() );
	console.Printf( "pages              : %d ", V_NumPages() );
	console.Printf( "blocks             : %d ", V_NumBlocks() );
	V_ConsoleReport();
}

void parser_t::Console_Gvar_Set( void ) {
	if ( !args[1] ) {
		console.Printf( "set: supply an argument to set/get gvar" );
		return;
	}
    gvar_c *g = Gvar_Find( args[1] );
    if ( !g ) {
        console.Printf( "Gvar: %s does not exist. supply a value to set it.", args[1] );
        return;
    }

	if ( !args[2] ) {
		console.Printf( "%s \"%s\"", g->name(), g->string() );
		return;
	}

    char buf[ 500 ];    
    strcpy( buf, args[2] );
    for ( int i = 3; i < TOKSIZE; i++ ) {
        if ( !args[i] )
            break;
		strcat( buf, " " );
        strcat( buf, args[i] );
    }

    g->setString( buf );
	console.Printf( "%s \"%s\"", g->name(), g->string() );
}

/*
=============================================================================================
 parser_t::runCmds
=============================================================================================
*/
// turns words list into an array of arguments and runs commands off of them if the parameters 
//  match exactly
int parser_t::runCmds( char *ret_line, unsigned int sz ) {

	size_t len = strlen( argBuf.vec ) + 1;

	node_c<char*> * node = words.gethead();
	int cmd = 0;

	memset( args, 0, sizeof(char *) * TOKSIZE );

	// to be considered a command, the very first token must be a command, not 2nd, 3rd, etc..
	// a command must be the first token
	if ( (cmd = findCmd( node->val )) ) {
		num_toks = 0;
		toks[ num_toks ] = cmd;
		args[ num_toks ] = commands[ cmd ].string;
		node = node->next;
		while ( node ) {
			args[ ++num_toks ] = node->val; 
			node = node->next;
		}
	} else {
		args[ 0 ] = NULL; // is only a command if args[0] is !NULL
	}


	// verify that the command has the exact correct number of arguments
	if ( cmd ) {
		int arg_count = 0;
		while ( args[ 1 + arg_count ] ) 
			++arg_count;
	
		// not a command for 1 of 2 criteria
		if ( ( commands[ cmd ].mandatoryArgs == 0 && arg_count > commands[ cmd ].possibleArgs ) ||
			 ( commands[ cmd ].mandatoryArgs > 0 && arg_count != commands[ cmd ].mandatoryArgs ) ) {
			// print help
			len = strlen( commands[ cmd ].help ) + 1;
			argBuf.reset( len + 1 + 7 );
			//strcpy( argBuf.vec, commands[ cmd ].help );
			sprintf( argBuf.vec, "usage: %s", commands[ cmd ].help );
			args[ 0 ] = NULL;

			// if the user types too many tokens after a command word, just consider it innocuous text
			if ( commands[ cmd ].mandatoryArgs == 0 && arg_count > commands[ cmd ].possibleArgs ) {
				cmd = 0;
			}
		}
	} else {
		// no command
		*argBuf.vec = 0;
		return 0;
	}


//	words.destroy();
	//return cmd;

	char dummy_buf[2000];
	if ( 0 == sz ) {
		ret_line = dummy_buf;
	}

	if ( !isCmd() ) {
		*ret_line = '\0';
		size_t len = strlen( argBuf.vec ) + 1;
		if ( len <= sz ) 
			strcpy( ret_line, argBuf.vec );
		return 1; // prints usage
	}

	int retval = 0;
	*ret_line = '\0';

	char *fname = NULL;
	char buf[128];

	const char *map_p = NULL;

	switch ( toks[ 0 ] ) {
	case TOK_SAVE: 	// saves current map.  if me_activeMap->string() is not set
					// or is zero length
					// it spawns the "Save As" dialog, otherwise it just
					// writes the file.
		if ( args[1] ) {
			ME_SaveMapCheckExtension( args[1] );
			map_p = me_activeMap->string();
			snprintf( ret_line, sz, "saving \"%s\"", map_p );
		} else {	
			map_p = me_activeMap->string();
			if ( !map_p || !map_p[0] ) {
				ME_SpawnSaveDialog();
				console.Toggle(); // close the con
				*ret_line = '\0';
			} else {
				ME_SaveMapCheckExtension( map_p );
				snprintf( ret_line, sz, "saving \"%s\"", map_p );
			}
		}
		retval = 1;
		break;
	case TOK_QUIT:
		Com_HastyShutdown();

		break;
	case TOK_DUMP:
		if ( !args[1] ) {
			snprintf( buf, 128, "%s", "console.out" );
		}
		fname = ( args[1] ) ? args[1] : buf ;
		console.dumpcon( fname );
		snprintf( ret_line, sz, "dumped console to \"%s\"", fname );
		retval = 1;
		break;
	case TOK_FILE:
		if ( console.load( args[1] ) ) {
//			snprintf( ret_line, sz, "loaded \"%s\"", args[1] );
			*ret_line = 0;
		} else 
			snprintf( ret_line, sz, "\"%s\" not found", args[1] );
		retval = 1;
		break;
	case TOK_MAP:
		if ( ME_LoadMapCheckExtension( args[1] ) ) {
			snprintf( ret_line, sz, "loaded map: \"%s\"", me_activeMap->string() );
			console.Toggle();
		} else {
			snprintf( ret_line, sz, "map: \"%s\" not found", args[1] );
		}
		retval = 1;
		break;
	case TOK_LISTCMDS:
		console.pushMsg( "" );
		console.pushMsg( "ZombieCurses Console Commands" );
		console.pushMsg( "-----------------------------------------------------" );
		retval = 1;
		if ( args[1] && args[1][0] == '-' && args[1][1] == 'r' ) {
			for ( unsigned int i = 0; i < TOTAL_COMMANDS; i++ ) 
				console.Printf( "%s :   %s", commands[i].string, commands[i].help );
			break;
		}

		for ( unsigned int i = TOTAL_COMMANDS-1; i > 0; i-- ) {
			console.Printf( "%s :   %s", commands[i].string, commands[i].help );
		}
		break;
	case TOK_LISTGVARS:
		Gvar_ConPrint();
		retval = 1;
		break;
	case TOK_CLEAR:
		console.Clear();
		retval = 1;
		break;
	case TOK_HELP:
		console.pushMsg( "" );
		console.Printf( "ZombieCurses console.  Type 'listcmds' for a list of commands" );
		retval = 1;
		break;
    case TOK_EXIT:
        console.pushMsg( "try typing \"quit\" instead" );
		retval = 1;
		break;
	case TOK_SET:
        Console_Gvar_Set();
		retval = 1;
		break;
	case TOK_SETA:
		strcpy( ret_line, "command not implemented" );
		retval = 1;
		break;
	case TOK_UID:
		if ( ME_ConPrintUIDs() )
			retval = 1;
		break;
	case TOK_LAYERINFO:
		if ( ME_ConPrintLayers() )
			retval = 1;
		break;
	case TOK_LOCKINFO:
		if ( ME_ConPrintLocks() )
			retval = 1;
		break;
	case TOK_TILEINFO:
		ME_ConPrintTileInfo();
		retval = 1;
		break;
	case TOK_UNLOCK_ALL:
		ME_SetLockAll( 0 );
		retval = 1;
		break;
	case TOK_LOCK_ALL:
		ME_SetLockAll( 1 );
		retval = 1;
		break;
	case TOK_CREATE_COLOR_MATERIAL:
		ME_CreateColorMaterial( atof(args[1]), atof(args[2]), atof(args[3]), atof(args[4]) );
		retval = 1;
		break;
	case TOK_MTLINFO:
		if ( args[1] ) {
		}
		ME_ConPrintMaterialInfo( -1 );
		retval = 1;
		break;
	case TOK_COMBINE:
		ME_CombineColModels();
		retval = 1;
		break;
	case TOK_BIND:
        CL_BindKey( args[1], args[2] );
		retval = 1;
		break;
    case TOK_UNBIND:
        CL_UnBindKey( args[1] );
		retval = 1;
		break;
	case TOK_PALETTE:
		palette.NewPalette( args[1] );
		retval = 1;
		break;
	case TOK_RM:
//		if ( ME_SpawnDeletePaletteDialog() )
//			retval = 1;
		strcpy( buf, me_activeMap->string() );
		buf[127] = 0;
		if ( 0 == palette.DeleteCurrent() )
			console.Printf( "deleting current palette: \"%s\"", buf );
		retval = 1;
		break;
	case TOK_PALETTEINFO:
		palette.Info();
		retval = 1;
		break;
	case TOK_HISTORY:
		console.PrintHistory();
		retval = 1;
		break;
	case TOK_BACKGROUND:
		ME_SetBackgrounds();
		retval = 1;
		break;
	case TOK_UNSET_BACKGROUNDS:
		ME_UnsetAllBackgrounds();
		retval = 1;
		break;
	case TOK_CREATE_COLOR_MASK:
		if ( !ME_CreateColorMaskMaterial( atof(args[1]), atof(args[2]), atof(args[3]), atof(args[4]) ) ) {
			strcpy( ret_line, "Please select just one tile with a texture to make a colorMask from" );
		}
		retval = 1;
		break;
	case TOK_DUP:
		if ( !ME_DupMaterial() ) {
			strcpy( ret_line, "select just one material to duplicate" );
		}
		retval = 1;
		break;
	case TOK_CREATE_TEXTURE_MASK:
		if ( !ME_CreateMaskedTexture() ) {
			strcpy( ret_line, "select exactly one texture and one mask to create a texture mask" );
		}
		retval = 1;
		break;
	case TOK_SET_COLOR:
		if ( !ME_SetMaterialColor( 	atof( args[1]), 
									atof( args[2]),
									atof( args[3]),
									atof( args[4])) ) {
			strcpy( ret_line, "please select a material to set the color on" );
		}
		retval = 1;
		break;
	case TOK_RELOAD:
		materials.ReLoadTextures();
		sprintf( ret_line, "textures re-loaded" );
		retval = 1;
		break;
	case TOK_MAG:
	case TOK_ZOOM:
		if ( args[1] ) {
			console.Printf( "Viewport Magnification: was: %.2f, changed to: %s", M_Ratio(), args[1] );
			M_SetRatio( atof(args[1]) );
		} else {
			console.Printf( "Viewport Magnification: %.2f", M_Ratio() );
		}
		retval = 1;
		break;
	case TOK_VIEW:
		if ( !args[1] ) {
			main_viewport_t *v = M_GetMainViewport();
			console.Printf( "Viewport: (%.2f, %2.f) x %.2f", v->world.x, v->world.y, v->ratio );
		} else {
			M_ConSetViewport( args[1], args[2], args[3] );
		}
		retval = 1;
		break;
	case TOK_CREATE_AREA:
		if ( ME_CreateAreaFromSelection( args[1] ) ) {
			strcpy( ret_line, "please select tiles to be member of Area_t" );
		} 
		retval = 1;
		break;
	case TOK_AREA_INFO:
		ME_PrintAreaInfo();
		retval = 1;
		break;
	case TOK_SET_NAME: // only Areas get user specified names
		if ( ( retval = ME_SetName( args[1] ) ) ) {
			switch( retval ) {
			case -1: strcpy( ret_line, "no tiles selected" ); break; 
			case -2: strcpy( ret_line, "please supply a name" ); break;
			case -3: strcpy( ret_line, "please create an area with the 'create_area' command , then set it's name" ); break;
			case -4: strcpy( ret_line, "selected tiles not found within created areas" ); break;
			}
		}
		break;
	case TOK_CREATE_SUB_AREA:
		if ( ME_CreateSubAreaFromSelection() ) {
			strcpy( ret_line, "please select tiles to be member of the sub Area" );
		} 
		retval = 1;
		break;
	case TOK_DELETE_AREA:
		break;
	case TOK_DELETE_SUB_AREA:
		break;
	case TOK_LIST_ENTITY_TYPES:
		G_ConPrintEntityTypes();
		retval = 1;
		break;
	case TOK_LIST_ENTITIES:
		break;
	case TOK_ENTITY:
		if ( G_CreateEntity( args[1] ) < 0 ) 
			strcpy( ret_line, "please supply a valid entityType. (try 'listentitytypes' cmd)" );
		else 
			sprintf( ret_line, "entity \"%s\" created successfully", args[1] );
		retval = 1;
		break;
	case TOK_ENT:
		G_EntCommand( args[1], args[2], args[3], args[4] );
		retval = 1;
		break;
	case TOK_NUDGE:
		CM_Nudge_Command( args[1], atof( args[2] ) );
		retval = 1;
		break;
	case TOK_AREA:
		ME_Area_Command( args[1], args[2], args[3], args[4] ) ;
		retval = 1;
		break;
	case TOK_BGINFO:
		ME_BackgroundInfo();
		retval = 1;
		break;
	case TOK_MEM:
		PARSER_memUsage();
		retval = 1;
		break;
	case TOK_UNSETBG:
		ME_UnsetBackground( args[1] );
		retval = 1;
		break;
	case TOK_FTC:
		//LIB_ftc_command( args[1] );
		retval = 1;
		break;
	case TOK_RESTART:
		CL_ReloadMap();
		console.Toggle();
		retval = 1;
		break;
	case TOK_PAUSE:
		CL_TogglePause();
		retval = 1;
		console.Printf( "%s game", cl_paused ? "pausing" : "unpausing" );
		break;
    case TOK_MUSIC:
        audio.command( args[1], args[2] );
		retval = 1;
        break;
    case TOK_LISTSCRIPTS:
        G_ListScripts();
		retval = 1;
        break;
    case TOK_LOADSCRIPT:
        G_LoadScript( args[1] );
		retval = 1;
        break;
    case TOK_RUN:
        G_RunScript( args[1] );
		retval = 1;
        break;
    case TOK_SCRIPTS:
        G_PrintLoaded();
        retval = 1;
        break;
	}

	return retval;
}


