#ifndef __LIB_PARSER_H__
#define __LIB_PARSER_H__


void cmd_save( void *, void *, void * ); 

enum cmd_t {
	TOK_NONE,
	TOK_SAVE,			// saves the current map
	TOK_QUIT,			// quits the app
	TOK_DUMP,			// dumps the console to console.out
	TOK_FILE,			// opens a text file
	TOK_MAP,			// loads map
	TOK_LISTCMDS,
	TOK_LISTGVARS,
	TOK_CLEAR,
	TOK_HELP,
    TOK_EXIT,
	TOK_SET,
	TOK_SETA,
	TOK_UID,
	TOK_LAYERINFO,
	TOK_LOCKINFO,
	TOK_TILEINFO,
	TOK_UNLOCK_ALL,
	TOK_LOCK_ALL,
	TOK_RESCAN_ASSETS,	// scans the assets directory, looking for new assets 	
	TOK_VID_RESTART,
	TOK_CREATE_COLOR_MATERIAL,
	TOK_MTLINFO, 
	TOK_COMBINE, 
	TOK_BIND,
    TOK_UNBIND,
	TOK_PALETTE,
	TOK_RM,
	TOK_PALETTEINFO,
	TOK_HISTORY,
	TOK_BACKGROUND,
	TOK_UNSET_BACKGROUNDS,
	TOK_CREATE_COLOR_MASK,
	TOK_DUP,
	TOK_CREATE_TEXTURE_MASK,
	TOK_SET_COLOR,
	TOK_RELOAD,			// re-loads texture directory
	TOK_MAG,			// prints or sets magnification
	TOK_ZOOM,			// alias for mag
	TOK_VIEW,			// get or set the whole viewport
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
	TOK_ENT,
	TOK_NUDGE,
	TOK_AREA,
	TOK_BGINFO,
	TOK_MEM,
	TOK_UNSETBG,
	TOK_FTC,
	TOK_RESTART,
	TOK_PAUSE,
    TOK_MUSIC,
    TOK_LISTSCRIPTS,
    TOK_LOADSCRIPT,
    TOK_RUN,
    TOK_SCRIPTS,
};

struct cmd_lut_t {
	int tok;
	char *string;
	int possibleArgs;
	int mandatoryArgs;
	char *help;
	void(*func)(void *, void *, void *);
};



extern const unsigned int TOTAL_COMMANDS;


class parser_t {
private:
	static const int TOKSIZE = 100;
	int toks[ TOKSIZE ];
	char * args[ TOKSIZE ];
	int num_toks;
	int findCmd( const char * );
	class vec_c<char> argBuf;

	list_c<char *> words;

public:
	void reset( void ) {
		toks[0] = 0;
		args[0] = NULL;
		num_toks = 0;
	}
	// returns total number of tokens found
	int tokenize( const char * );
	parser_t() {}
	parser_t ( const char *s ) { tokenize( s ); }

	void searchGvars( void );

	bool isCmd( void ) { return args[ 0 ] != NULL; } 

	int runCmds( char *, unsigned int ) ;

	unsigned int numToks( void ) { return words.size(); }
	const char *getToken( int i ) { return (const char * )this->words.peek( i ); }

    void Console_Gvar_Set( void );
};




#endif /* __LIB_PARSER_H__ */
