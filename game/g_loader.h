// g_loader.h
//

#ifndef __G_LOADER_H__
#define __G_LOADER_H__

#include "../common/com_object.h"
#include "../lib/lib_list_c.h"

// the main loader: reads "game.def".  should read in command, either

// first 2 main commands:
// map <mapname> 	
// editor 			

// more commands:
// menu
// mainmenu

/* 

a game.def for a regular game might look like:

menu menu1
play intro
map map_e1m1
play segway12
map map_e1m2
play segway13
map map_e1m3
map map_e1m4
play e1_victory
repeat

so right there we have 5 commands, 3 take arguments.  

commands: menu, play, map, editor, repeat

realistically, we're only going to need that many, perhaps a couple more,
so we can just code each one on a per-need basis.  

or you could make a tag and goto , if you even need that.  repeat assumes
you repeat the whole thing, so after e1_victory, it goes back to the menu
or you could have it:  'play credits'  then 'repeat'

*/

/*
so I already have a linked list of char *lines, in com_main.  Now I need to 
turn them into discreet actions, actions which enter main loops.

*/

// just an initializer value. does not limit size of cmd list 
#define STATIC_GAME_CMDS	32



struct gamecmdTable_t {
	char *cmd_string;
	int numArgs;
};

// labels for valid commands
enum { GC_MENU, GC_PLAY, GC_MAP, GC_EDITOR, GC_REPEAT };

struct gamecmd_t : public Allocator_t  {
	int 		cmd;
	char *		cmd_string;
	char *		arg;
	gamecmd_t() : cmd(-1000), cmd_string(0), arg(0) {}
	gamecmd_t( const char *c, const char *a, int ); 
};

class Game_t : public Allocator_t {
private:	

	// the current command number.  even if repeat is called, this does not
	// get reset and continues to ascend 
	int 					cmd_num;
	buffer_c<gamecmd_t*> 	gamecmds;
	unsigned int			cur_cmd;
	
public:
	
	// create cmds from array of char*, presumably 1 cmd per line
	unsigned int CreateGameCommands( buffer_c<char*>& ) ;

	// increment current and run it
	void RunNextCommand( void ) ;

	// add one cmd when creating cmdlist
	int AddGameCommand( const char *cmd, const char *arg =NULL ) ;
	int PushGameCommand( const char *cmd, const char *arg =NULL, int slot =0 ) ;

	void ClearCmds( void );
	void ResetGame( void ); // starts over w/o altering command list

	Game_t() : cmd_num(0), cur_cmd(0) { gamecmds.init( STATIC_GAME_CMDS ); }
	~Game_t() { gamecmds.destroy(); }

	static int VerifyCmd( const char *, const char * );

	// creates command if args check out, else returns -1
	static int CreateValidCommand( const char *, const char *, gamecmd_t ** );

	const gamecmd_t * CurCmd() const { return const_cast<const gamecmd_t*>( gamecmds.data[ cur_cmd ] ); }

	int CurCmdNum() { return cmd_num; }
};



#endif // __G_LOADER_H__
