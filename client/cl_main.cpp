

#include "../common/com_suppress.h"
#include "cl_local.h"
#include "cl_keys.h"
#include "../game/game.h" // GL_Demo_*
#include "../map/mapdef.h" // M_Init
#include "../mapedit/mapedit.h" // ME_DrawFrame

#include "../common/com_geometry.h"
#include "../renderer/r_public.h"



clientState_c   cls;
renderExport_t  re;


static void CL_InitRenderer ( void ) {
    re.init();
}

/*
====================
 CL_StartClient
====================
*/
void CL_StartClient ( void ) {


    CL_InitRenderer();

    Com_InitMouse();

	// starts the entire sound system
    S_Init();

    // sets up sound registration
    S_InitRegistration();

    //CL_InitUI();

	// Load Texture Directory
	materials.LoadTextureDirectory();

	// sets up open GL's 2D drawing context (needs textures loaded)
	CL_Draw_Init();

	//S_StartBackGroundTrack( "zpak/music/nestor1.wav" );
	//S_StartBackGroundTrack( "zpak/music/nestor3.wav" );
}


void ME_Init( void );

/*
====================
 CL_Init 

	- set client state
====================
*/
void CL_Init ( void ) 
{


    CL_InitKeys();

    // init input commands
    CL_InitInput();

	// set up the viewport and map parameters
	M_Init();
    
    // init data structure
    cls.start();
    re.start();

    // init console.  comes after viewport, because it needs screen resolution for it's size
    CL_StartCon();  

	//
	// do sub-system initializations
	//

	// mapedit system
	ME_Init();
}

void CL_WritePacket ( void ) {
}


void CL_SendCmd ( void ) {

	if ( cls.state == CS_EDITOR ) {
		return;
	}

    /*
	// don't send any message if not connected
	if ( cls.state < CA_CONNECTED ) {
		return;
	}

	// don't send commands if paused
	if ( com_sv_running->integer && sv_paused->integer && cl_paused->integer ) {
		return;
	}
    */

	// we create commands even if a demo is playing,
	CL_CreateNewCommands();

    /*
	// don't send a packet if the last packet was sent too recently
	if ( !CL_ReadyToSendPacket() ) {
		if ( cl_showSend->integer ) {
			Com_Printf( ". " );
		}
		return;
	}
    */
	CL_WritePacket();
}

void CL_CheckUserInfo( void ) {
}

void CL_CheckLocalState( void ) 
{

	// synchronize client state with gamecmd 
	if ( cls.current_gamecmd != MainGame.CurCmdNum() || cls.current_gamecmd < 0 ) {
		// A new gamecmd happened, what is it?  

		// if its a menu, play, or map , we need to set some shit up
		const gamecmd_t *gamecmd = MainGame.CurCmd();
		switch ( gamecmd->cmd ) {
		case GC_MENU:
			// .. nothing yet ..
			break;
		case GC_PLAY:
			// .. nothing yet ..
			break;
		case GC_MAP:
			// do a World::MapLoad
			cls.state = CS_LOADMAP;

			if ( 0 > CL_LoadMap( gamecmd->arg ) ) {
				console.Printf( "CL_LoadMap: failed loading map: \"%s\"", gamecmd->arg );
				console.dumpcon( "console.DUMP" );
				Err_Fatal( "CL_LoadMap: failed loading map: \"%s\"", gamecmd->arg ); 
			}

			// player initializations (some are map dependant ie. zoom effects speed)

			cls.keyCatchers = KEYCATCH_GAME;
			cls.state = CS_PLAYMAP;
			break;
		case GC_EDITOR:
			// new command is editor, set the gvar_c
			com_editor->set( "com_editor", "1", 0 );
			cls.keyCatchers = KEYCATCH_MAPEDIT;
			cls.state = CS_EDITOR;
			break;
		}

		cls.current_gamecmd = MainGame.CurCmdNum();
	}
}

// test out the sound code
void CL_DoSoundDemo( void ) {
    static timer_c dt;
	static char filename[128];
	static char track[128];
    static filehandle_t note = -1;

    if ( note < 0 ) {
		C_strncpy( filename, "zpak/sound/melst_b4.wav", sizeof(filename) );
        note = S_RegisterSound( filename );
        if ( note < 0 ) {
            Com_Printf("couldn't load melst_b4.wav\n");
            return;
        }
        C_strncpy( track, "zpak/sound/01 - LANYARD LOOP.wav", 128 );
        C_strncpy( track, "zpak/sound/Charlie Parker - Be Bop.wav", 128 );
        //S_StartBackGroundTrack( track );
        S_StartBackGroundTrack( filename, &note, gtrue, 1 );
    }
    if ( note < 0 ) 
        return;
/*
    if ( !dt.flags || dt.check() ) {
        dt.set( 3000 );
        S_StartLocalSound( note );
		Com_Printf( "playing: %s\t\t%d\n", filename, Com_Millisecond() );
    }
	*/
}

void CL_DrawFPS( void ) {
	gglColor4f( 1.0f, 1.0f, 1.0f, 0.6f );

	static int last = 0, now = 0;
	static int height = M_Height();
	static int width = M_Width();

	now = Com_Millisecond();

	static int frames = 0;
	++frames;

	static float fps = 0.f;
	if ( now - last > 1000 ) {

		int diff = now - last;
		last = now;

		fps = (float)frames / (float)diff * 1000.f;
		frames = 0;
	}

	F_Printf( width - 80 , height - 4 , 1, "fps: %.0f", fps );
}

extern float sv_fps_actual;
void CL_DrawFPS_SV( void ) {
	F_Printf( M_Width() - 200 , M_Height() - 4 , 1, "sv_fps: %.0f", sv_fps_actual );
}

void FreetypeTest( void );

void CL_UpdateScreen ( void ) {

/*
	// pick where we're routed to
	if ( freetype_test->integer() ) {
		FreetypeTest();
	} else if ( com_texttest->integer() ) {
		CL_DrawFrame();
	} else if ( com_mapedit->integer() ) {
		ME_DrawFrame();
	} else {
		G_DrawFrame();
	}
*/
	switch ( cls.state ) {
	case CS_LOADMAP: 
		break;
	case CS_PLAY_CUTSCENE:
		break;
	case CS_PLAYMAP:

		// update camera every draw frame because it does things on its own
		camera.update();

		R_DrawFrame();
		break;
	case CS_EDITOR:

		ME_DrawFrame();
		break;
	}

	if ( com_showfps->integer() ) {
		CL_DrawFPS();
		CL_DrawFPS_SV();
	}
}

void CL_Frame( int msec ) {

    CL_CheckLocalState();

	// see if we need to update any userinfo
	CL_CheckUserInfo();

	// send cmd intentions now
	// this is for GAME COMMANDS, like: FORWARDS, BACKWARDS, SHOOT, JUMP
	// not for keyboard input, like Console input
	CL_SendCmd();

	// update the screen
	CL_UpdateScreen();

	// update audio
	S_Update();
}

/*
====================
 clientState_c::start()
====================
*/
void clientState_c::start( void ) {
	this->width = M_Width();
	this->height = M_Height();

	// scale factor from virtual resolution 640x480 to actual XXXX,YYYY
	screenXscale  = this->width / VSCREENWIDTH;
	screenYscale = this->height / VSCREENHEIGHT;

	state = CS_JUST_STARTED;

	keyCatchers = 0;
	if ( com_editor->integer() ) {
		keyCatchers |= KEYCATCH_MAPEDIT;
	}
}
