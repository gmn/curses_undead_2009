// g_loader.cpp
//

/* I have no idea how these files are getting included!!!! */
#define __G_ENTITY_H__ 1
#define __MAPEDIT_H__ 1
#define __M_AREA_H__ 1

#include "g_loader.h"
#include "../client/cl_console.h"

/* 
====================
 MainGame

 main game object.  manages game flow between menus, levels, cut-scenes, etc..

====================
*/
Game_t MainGame;

// just makes a command w/ new string memory, doesn't verify is valid
gamecmd_t::gamecmd_t( const char *c, const char *a, int cmd_num ) {

	Assert( c != NULL );

	unsigned int sz = strlen( c );
	cmd_string = (char*) V_Malloc( sz + 1 );
	strcpy( cmd_string, c );
	*(cmd_string + sz) = 0;

	if ( a ) {
		sz = strlen ( a ) ;
		arg = (char*) V_Malloc( sz + 1 );
		strcpy( arg, a );
		*(arg + sz) = 0;
	} else {
		arg = NULL;
	}

	cmd = cmd_num;
}


/*
table of possible commands
*/
gamecmdTable_t gamecmdTable[] = {

	// takes an arg, executes a menu routine
	{ "menu", 		2 }, 

	// plays a scripted sequence or movie, takes an arg
	{ "play", 		2 },

	// loads a map
	{ "map", 		2 },

	// starts the map editor, no arg
	{ "editor", 	1 },

	// goes back to beginning of gamecmds
	{ "repeat",		1 },

};

volatile static const unsigned int total_gamecmds = sizeof(gamecmdTable) / sizeof(gamecmdTable[0]);


// returns cmdnum for good cmd, negative for fail
int Game_t::VerifyCmd( const char *cmd, const char *arg ) {
	for ( int i = 0; i < total_gamecmds; i++ ) {
		// found it!
		if ( !strcmp( cmd, gamecmdTable[ i ].cmd_string ) ) {
			// check arg requirement
			if ( gamecmdTable[ i ].numArgs > 1 && !arg ) {
				return -1; // fail
			}
			return i;
		}
	}
}

/*
====================
 CreateValidCommand
====================
*/
int Game_t::CreateValidCommand( const char *cmd, const char *arg, gamecmd_t **gc ) {
	int cnum = -1;
	if ( (cnum = VerifyCmd( cmd, arg )) == -1 ) {
		return -1;
	}
	*gc = new gamecmd_t( cmd, arg, cnum );
	return cnum;
}



/*
====================
 Game_t::AddGameCommand

	adds 1 command to list of gamecmds on to the end
====================
*/
int Game_t::AddGameCommand( const char * cmd, const char *arg ) {
	gamecmd_t *gc;
	int cnum;
	if ( (cnum = CreateValidCommand( cmd, arg, &gc )) < 0 ) {
		// invalid command 
		return -1;
	}
	gamecmds.add( gc );
	return cnum; 
}

/*
====================
 PushGameCommand

  inserts a gamecommand into the desired slot
====================
*/
// 0 is the first, very first command, 1 is the second, ..., n-1 the last
int Game_t::PushGameCommand( const char * cmd, const char * arg, int slot ) {

	// get size of current gamecmds
	int sz = gamecmds.length();
	if ( sz == slot ) {
		return AddGameCommand( cmd, arg );
	}

	// asked for too far, could pad, but with what?
	if ( slot > sz ) {
		return -1;
	}

	// get/create command
	gamecmd_t *gc;
	int cnum;
	if ( (cnum = CreateValidCommand( cmd, arg, &gc )) < 0 ) {
		return -1; // invalid command 
	}

	int move_seg_length = sz - slot;
	gamecmds.add( gc ); // add one, to increment internal size

	// now move manually
	memmove( &gamecmds.data[slot]+1, &gamecmds.data[slot], move_seg_length );
	gamecmds.data[slot] = gc;

	return cnum;
}


/*
====================
 Game_t::CreateGameCommands

	creates game commands from array of cmds, presumably one per line
====================
*/
unsigned int Game_t::CreateGameCommands( buffer_c<char*>& gameDef ) {
	int cmd = 0;
	char *line; 
	vec_c<char> buf;

	while ( cmd < gameDef.length() ) {

		// get line
		line = gameDef.data[ cmd ];

		unsigned int len = strlen( line );
		const char *end = line + len;

		// skip white space, if any
		while ( *line == ' ' || *line == '\t' ) {
			// at end of line
			if ( line >= end ) {
				goto next_line;
			}

			// must not be off end, eat next char
			++line;
		}

		// look for '//' comment, if there is one, skip whole line, continue
		if ( *line == '/' && *(line+1) == '/' ) {
			goto next_line;
		}

		//
		// verify command
		//
		buf.reset( len + 10 );
		buf.zero_out();

		// copy the line, sans whitespace
		strcpy( buf.vec, line );

		// find first space, put null there
		line = buf.vec;
		while ( line - buf.vec < len + 1 ) {
			// single , null terminated command
			if ( !*line ) {
				*++line = '\0';
				break;
			}

			// found it, bing!
			// trailing spaces
			if ( line - buf.vec <= len && (*line == ' ' || *line == '\t' || *line == '\n') ) {
				*line++ = 0; // set null, goto next char

				// eat any other white space
				while ( *line && line - buf.vec < len + 1 && ( *line == ' ' || *line == '\t' || *line == '\n' ) ) {
					++line;
				}

				// either first char of next word, or NULL
				break;
			}
			++line;
		}

		/*
		// if we're not at the end of the line yet, maybe it's
		// the first char of 2nd arg?  find end, and make sure to \0-out trailing
		//  whitespace or newline
		if ( line - buf.vec < len + 1 ) {
			char *_sav = line;

			// skip white space
			while ( line - buf.vec < len + 1 && *line != ' ' && *line != '\t' && *line != '\n' ) {
				++line;
			}

			// we're still not at the end.  we've probably got an argument 
			if ( line - buf.vec < len + 1 ) {
				line = _sav;
			}

			// either hit a null or a whitespace char, either way, re-null it
//			*line = '\0';
		}
		*/

		// **any trailing args are ignored

		// if fails, post warning to log, skip it & continue
		gamecmd_t *c;
		if ( CreateValidCommand( buf.vec, line, &c ) < 0 ) {
			console.Printf( "game.def: cmd \"%s %s\" not valid. skipping cmd", buf.vec, line );
			console.dumpcon( "console.DUMP" );
			goto next_line;
		}

		gamecmds.add( c );
		if ( 0 == cmd && c->cmd == GC_EDITOR ) {
			com_editor->setInt( 1 );
		}

		// NEXT LINE!
next_line:
		++cmd;
	}

	buf.destroy();

	// return number of currently stored commands
	return gamecmds.length();
}

/*
====================
 Game_t::RunNextCommand

	you dont have to run the first command, it'll already be loaded

	then again, maybe you don't run any commands in this object.  this object
	 just keeps track of the current cmd.  the client/server actually runs them
====================
*/
void Game_t::RunNextCommand( void ) {

	++cmd_num;	
	++cur_cmd;

	// automatically repeats if it falls off the end
	if ( cur_cmd >= gamecmds.length() ) {
		cur_cmd = 0;
	}

	// run it
	switch ( gamecmds.data[ cur_cmd ]->cmd ) {
	case GC_MENU:
		break;
	case GC_PLAY:
		break;
	case GC_MAP:
		break;
	case GC_EDITOR: // editor is a final command, will not return
		break;
	case GC_REPEAT:
		cur_cmd = 0;
		break;
	}
}

/*
====================
 Game_t::ClearCmds
====================
*/
void Game_t::ClearCmds( void ) {
	gamecmds.zero_out();
}

/*
====================
 Game_t::ResetGame
====================
*/
void Game_t::ResetGame( void ) {
	cur_cmd = 0;
}

