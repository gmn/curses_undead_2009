/*
 *  cl_public.h - client public interface
 */

#ifndef __CL_PUBLIC_H__
#define __CL_PUBLIC_H__

//#include "../lib/lib.h"
//#include "../lib/lib_list_c.h"

#include "../common/com_types.h"
#include "../lib/lib_image.h"
#include "cl_sound.h"
#include "../common/com_object.h"
#include "../renderer/r_public.h"

//#include "../map/m_area.h" // World_t
//#include "../renderer/r_public.h"

#include "../lib/lib_timer.h"
#include "../renderer/r_lerp.h"

// ==================================================================
//  datatypes
//==================================================================


enum clientState_t {
    CS_NULL,
    CS_JUST_STARTED,        // clientState just started
    CS_INITIALIZED,     	// client initialized
    CS_READY,           	// client ready

	CS_LOADMAP,				// in case map load time is long, we'll have a 
							//  load bar

	CS_PLAY_CUTSCENE,
	CS_EXECUTE_MENU,
	CS_PLAYMAP,				// regular game
	CS_EDITOR,				// map editor
};




#define TOTAL_CMDS  32
#define CMD_MASK    (TOTAL_CMDS-1)


struct clientMedia_t {
	int dummy, fuckass;
	//image_c fontImage;
};


/*
// gameState_t
struct gameState_t {
    int vidWidth, vidHeight;
    void start( float x, float y ) { vidWidth = x; vidHeight = y; }
}; */

enum clientKeyCatchers_t {
	KEYCATCH_NONE 			= 0,		
	KEYCATCH_UI				= 1<<0,
	KEYCATCH_GAME			= 1<<1,
	KEYCATCH_CONSOLE		= 1<<2,
	KEYCATCH_MESSAGE		= 1<<3,
	KEYCATCH_MAPEDIT		= 1<<4,
    KEYCATCH_MENU           = 1<<5,
};

/* font choices */
enum Font_t {
	FONT_VERA = 0,
	FONT_VERA_BOLD = 1,
	FONT_VERA_MONO = 7,
	FONT_VERA_MONO_BOLD = 4,
	FONT_VERA_SE = 8,
	FONT_VERA_SE_BOLD = 9,
	FONT_UNI53 = 10,
	FONT_UNI54 = 11,
	FONT_UNI63 = 12,
	FONT_UNI64 = 13,
};

#define F_POINT_SZ 14

// named user move
enum Move_t {
	MOVE_NONE		= 0,

	MOVE_UP			= 1 << 0,
	MOVE_RIGHT		= 1 << 1,
	MOVE_DOWN		= 1 << 2,
	MOVE_LEFT		= 1 << 3,
	MOVE_UP_RIGHT	= 3,
	MOVE_UP_LEFT	= 9, 
	MOVE_DOWN_RIGHT	= 6,
	MOVE_DOWN_LEFT  = 12,

	MOVE_SHOOT1     = 1 << 4,
	MOVE_SHOOT2     = 1 << 5,
	MOVE_SHOOT3     = 1 << 6,
	MOVE_STAB		= 1 << 7,
	MOVE_RUN		= 1 << 8,
	UP_IMPULSE 		= 1 << 9,
	DOWN_IMPULSE 	= 1 << 10,
	MOVE_USE		= 1 << 11,

    MOVE_TALK       = 1 << 12,
    MOVE_MAP        = 1 << 13,
    MOVE_INV        = 1 << 14,
};


/*
==============================================================================

 Camera_t

==============================================================================
*/
class Camera_t : public Allocator_t {
private:
	bool started;
public:
	float * x, * y; 	// viewport location
	float w, h;			// viewport dimension
	float radius;		// area allowed to move before forcing viewport move
	float *r, *ir; 		// zoom

	int lastUpdate;		//

	void init( void ); 
	void update( void );
	void shake( float, float =3.0f );

	Camera_t() : started(0), lastUpdate(0) {}

	float window_creep_amt;
	int ms_per_frame;

	timer_c timer;
	int wait_til_creep;

	float last[2];		// the players last known coordinates
	float last_excess;	// if they are outside of the radius, how much
	float drift_compensate; // compensation amount

	lerpFrame_c lerp;

	// other systems will notify camera of what they're up to
	void update( uint, float, float );

	float player_dist;
	
	float distFromCenter( float , float ) ;

};

extern Camera_t camera;

/*
==============================================================================

 Controller_t

==============================================================================
*/
#define C_U 	1
#define C_R		2
#define C_D		4
#define C_L  	8
#define C_UR 	(C_U|C_R)
#define C_DR 	(C_D|C_R)
#define C_UL	(C_U|C_L)
#define C_DL	(C_D|C_L)

class Controller_t : public Allocator_t {
private:
    static class World_t& world;
    static class Camera_t& camera;
    //static class Renderer_t& render;
    static class main_viewport_t& viewport;

    bool doPrintBlocks;
public:
    float x, y;
    float xMoveAmt, yMoveAmt;

    Controller_t() : x(1.f), y(1.f) {
        xMoveAmt = yMoveAmt = 65.0f;
        doPrintBlocks = false;
    }

    int move( float, float );

    void togglePrintBlocks();

    // camera, curBlock, starting x,y, ...
	void setupGameState();
	void checkCurBlock();

	class Block_t * find( class Block_t *, int );
	int cameraMove( float, float );

	struct tele_s {
		float dx, dy;
		void *spawn;
		void *area;
		bool on;
		tele_s() : on(0) {}
	} tele;

	void notifyTeleport( float, float, class Spawn_t *, class Area_t * );
	void teleportViewChange( void );
};

extern Controller_t controller;


/*
==============================================================================

 clientState_c

==============================================================================
*/
class clientState_c {
public:
    int 		keyCatchers; // bit flag 
	float 		screenXscale;
	float 		screenYscale;
    int 		state;

	// FIXME: remove!  not using this command array or indices 
	// I'm sick of copying id conventions.  It has the side effect of making
	// me BLIND.  It is much better to write intuitively, and design your
	// systems yourself.  Reading their code to learn from it and writing to
	// fit its idioms are two very different things.  one sets you free while
	// the latter enslaves you.
//	usercmd_t	cmds[TOTAL_CMDS];	// each mesage will send several old cmds
//	int			cmdNumber;			// incremented each frame, because multiple
                                    //  frames may need to be packed into a single packet
//	int 		oldCmdNum;

	struct 		clientMedia_t media;

	float 		width, height;

	int 		current_gamecmd;

    void start( void );

	clientState_c() {
		keyCatchers = 0;
		screenXscale = 0;
		screenYscale = 0;
		state = 0;
//		cmdNumber = 0;
		width = height = 1.f;
		current_gamecmd = -1;
	}
};


//
// cl_input
//
class kbutton_c {
public:
	int			down[2];		// key nums holding it down
	unsigned	downtime;		// msec timestamp
	unsigned	msec;			// msec down this frame if both a down and up happened
	gbool	active;			// current state
	gbool	wasPressed;		// set when down, not cleared when up
};




enum {
	CE_PMOVE,			// player moved
	CE_PTILECOL,		// player responding to tile collision
};






//==================================================================
//  vars
//==================================================================
extern clientState_c cls;
//extern parser_t parser;



//==================================================================
//  functions
//==================================================================
// cl_input.cpp
void CL_InitInput ( void );    
void CL_MouseEvent( void * , unsigned int time );
void CL_JoystickEvent( int axis, int value, unsigned int time );
void Input_Frame ( void );
void CL_CreateNewCommands( void ); 
int * CL_GetMouseSinceLastFrame( void );
void CL_GameKeyHandler( int, int , unsigned int, int );

// cl_main.cpp
void CL_StartClient ( void );
void CL_Init ( void );
void CL_Frame ( int msec ); 

// cl_keys.cpp
void CL_KeyEvent ( int key, int down, unsigned int time );
void CL_CharEvent( int key );

// cl_sound.cpp
void S_FreeOldestSound( void );
void S_InitRegistration( void );
int S_LoadSound( sfx_t *sfx );
sfx_t * S_FindName( const char *name );
void S_MemoryLoad( sfx_t *sfx );
int S_RegisterSound( const char *lpath );
void S_ClearSoundBuffer ( void );
void S_StopAllSounds(void);
void S_PrintSoundInfo( void );
void S_Init( void );
void S_Update( void );
gbool S_StartSound( vec3_t origin, soundDynamic_t *dyn, sfxHandle_t sfxHandle );
void S_StartLocalSound( sfxHandle_t sfxHandle );
void S_StopBackGroundTrack( void );
gbool S_UpdateBackGroundTrack( void );
//void S_StartBackGroundTrack( const char *name );
void S_StartBackGroundTrack( const char *, filehandle_t * =NULL, gbool =gfalse, int =0 );
void S_ReStartBackGroundTrack( filehandle_t , gbool =gfalse, int =0 );
void S_Shutdown ( void );
void S_UnMute( void );
void S_Mute( void );

// cl_draw.cpp
void CL_Draw_Init( void );
void CL_InitGL( void );
void CL_InitGL2D( void );
void CL_DrawFrame( void );
void G_DrawFrame( void );

/*
void CL_DrawStretchPic( float, float, float, float, 
						float, float, float, float, image_c * );
*/
void CL_DrawChar( int, int, int, int, int );
int CL_DrawInt( int, int, int, int, int, int =0 );
void CL_DrawString( int, int, int, int, const char *, int =0, int =4 );

void CL_ModelViewInit2D( void );

// cl_freetype.cpp
float F_Printf( int, int, int, const char *, ... );
void F_SetScale( float );
void F_ScaleDefault( void );

// cl_console.cpp
void CL_StartCon( void );

//
void CL_Pause( void );
void CL_UnPause( void );
void CL_TogglePause( void );
void CL_PauseWithMusic( void );

// client controls pause state
extern bool cl_paused;

#endif /* !__CL_PUBLIC_H__ */

