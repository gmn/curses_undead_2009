/***********************************************************************
 *
 *  cl_keys.cpp
 *
 ***********************************************************************/

#include "cl_keys.h"
#include "cl_console.h"
#include "../mapedit/mapedit.h" // ME_KeyEvent
#include "cl_local.h" // CL_GameKeyHandler

gkey_t keys [ MAXKEYS ];
static int cl_anykeydown = 0;


int cl_shift_down = 0;
int cl_ctrl_down = 0;
int cl_caps_down = 0;
int cl_alt_down = 0;

// keyNames
struct {
    int num;
    char *name;
} keyNames[] = { 
    { KEY_KP_0, "KEY_KP_0" },
    { KEY_KP_1, "KEY_KP_1" },
    { KEY_KP_2, "KEY_KP_2" },
    { KEY_KP_3, "KEY_KP_3" },
    { KEY_KP_4, "KEY_KP_4" },
    { KEY_KP_5, "KEY_KP_5" },
    { KEY_KP_6, "KEY_KP_6" },
    { KEY_KP_7, "KEY_KP_7" },
    { KEY_KP_8, "KEY_KP_8" },
    { KEY_KP_9, "KEY_KP_9" },
    { KEY_KP_ASTERIX, "KEY_KP_ASTERIX" },
    { KEY_KP_PLUS, "KEY_KP_PLUS" },
    { KEY_KP_MINUS, "KEY_KP_MINUS" },
    { KEY_KP_PERIOD, "KEY_KP_PERIOD" },

    { KEY_F1, "KEY_F1" },
    { KEY_F2, "KEY_F2" },
    { KEY_F3, "KEY_F3" },
    { KEY_F4, "KEY_F4" },
    { KEY_F5, "KEY_F5" },
    { KEY_F6, "KEY_F6" },
    { KEY_F7, "KEY_F7" },
    { KEY_F8, "KEY_F8" },
    { KEY_F9, "KEY_F9" },
    { KEY_F10, "KEY_F10" },
    { KEY_F11, "KEY_F11" },
    { KEY_F12, "KEY_F12" },
    { KEY_ESCAPE, "KEY_ESCAPE" },

    { KEY_LCTRL, "KEY_LCTRL" },
    { KEY_RCTRL, "KEY_RCTRL" },
    { KEY_LALT, "KEY_LALT" },
    { KEY_RALT, "KEY_RALT" },
    { KEY_SPACEBAR, "KEY_SPACEBAR" },
    { KEY_LSHIFT, "KEY_LSHIFT" },
    { KEY_RSHIFT, "KEY_RSHIFT" },

    { KEY_BACKSPACE, "KEY_BACKSPACE" },
    { KEY_ENTER, "KEY_ENTER" },
    { KEY_CAPSLOCK, "KEY_CAPSLOCK" },
    { KEY_TAB, "KEY_TAB" },

    { KEY_SINGLEQUOT, "KEY_SINGLEQUOT" },

    { KEY_COMMA, "KEY_COMMA" },
    { KEY_MINUS, "KEY_MINUS" },
    { KEY_PERIOD, "KEY_PERIOD" },
    { KEY_FORWARDSLASH, "KEY_FORWARDSLASH" },

    { KEY_0, "KEY_0" },
    { KEY_1, "KEY_1" },
    { KEY_2, "KEY_2" },
    { KEY_3, "KEY_3" },
    { KEY_4, "KEY_4" },
    { KEY_5, "KEY_5" },
    { KEY_6, "KEY_6" },
    { KEY_7, "KEY_7" },
    { KEY_8, "KEY_8" },
    { KEY_9, "KEY_9" },

    { KEY_SEMICOLON, "KEY_SEMICOLON" },

    { KEY_EQUAL, "KEY_EQUAL" },

    { KEY_PAUSE, "KEY_PAUSE" },
    { KEY_NUMLOCK, "KEY_NUMLOCK" },
    { KEY_SCROLLLOCK, "KEY_SCROLLLOCK" },
    { KEY_HOME, "KEY_HOME" },
    { KEY_END, "KEY_END" },
    { KEY_PGUP, "KEY_PGUP" },
    { KEY_PGDOWN, "KEY_PGDOWN" },
    { KEY_INSERT, "KEY_INSERT" },
    { KEY_DELETE, "KEY_DELETE" },

	{ KEY_UP, "KEY_UP" },
	{ KEY_DOWN, "KEY_DOWN" },
	{ KEY_LEFT, "KEY_LEFT" },
	{ KEY_RIGHT, "KEY_RIGHT" },

    { KEY_LBRACKET, "KEY_LBRACKET" },
    { KEY_BACKSLASH, "KEY_BACKSLASH" },
    { KEY_RBRACKET, "KEY_RBRACKET" },

	{ KEY_a, "KEY_a" },
	{ KEY_b, "KEY_b" },
	{ KEY_c, "KEY_c" },
	{ KEY_d, "KEY_d" },
	{ KEY_e, "KEY_e" },
	{ KEY_f, "KEY_f" },
	{ KEY_g, "KEY_g" },
	{ KEY_h, "KEY_h" },
	{ KEY_i, "KEY_i" },
	{ KEY_j, "KEY_j" },
	{ KEY_k, "KEY_k" },
	{ KEY_l, "KEY_l" },
	{ KEY_m, "KEY_m" },
	{ KEY_n, "KEY_n" },
	{ KEY_o, "KEY_o" },
	{ KEY_p, "KEY_p" },
	{ KEY_q, "KEY_q" },
	{ KEY_r, "KEY_r" },
	{ KEY_s, "KEY_s" },
	{ KEY_t, "KEY_t" },
	{ KEY_u, "KEY_u" },
	{ KEY_v, "KEY_v" },
	{ KEY_w, "KEY_w" },
	{ KEY_x, "KEY_x" },
	{ KEY_y, "KEY_y" },
	{ KEY_z, "KEY_z" },

    { KEY_MOUSE0, "KEY_MOUSE0" },
    { KEY_MOUSE1, "KEY_MOUSE1" },
    { KEY_MOUSE2, "KEY_MOUSE2" },
    { KEY_MOUSE3, "KEY_MOUSE3" },
    { KEY_MOUSE4, "KEY_MOUSE4" },
    { KEY_MOUSE5, "KEY_MOUSE5" },
    { KEY_MOUSE6, "KEY_MOUSE6" },
    { KEY_MOUSE7, "KEY_MOUSE7" },
    { KEY_MWHEELDOWN, "KEY_MWHEELDOWN" },
    { KEY_MWHEELUP, "KEY_MWHEELUP" },
}; // keyNames
const unsigned int TotalKeyNames = sizeof(keyNames)/sizeof(keyNames[0]);

int O_strncasecmp (const char *s, const char *n, int sz);

int CL_stringToKey( const char *string ) {
    if ( !string || !string[0] )
        return 0; // not a key

    for ( int i = 0; i < TotalKeyNames; i++ ) {
        if ( !O_strncasecmp( string, keyNames[i].name, 25 ) ) {
            return keyNames[i].num;
        }
    }
    return 0;
}

const char *CL_keyToString( int key ) {
    for ( int i = 0; i < TotalKeyNames; i++ ) {
		if ( key == keyNames[i].num ) {
            return const_cast<const char *>(keyNames[i].name);
        }
    }
    return NULL;
}

/*
====================
 CL_SetModifiers

	handler for game key presses.  key input is turned into usercmd_t here.
====================
*/
void CL_SetModifiers( int key, int down ) {
	// shift
	if ( key == KEY_LSHIFT || key == KEY_RSHIFT ) {
		cl_shift_down = down;
	}
	// ctrl
	if ( key == KEY_LCTRL || key == KEY_RCTRL ) {
		cl_ctrl_down = down;
	}
	// alt
	if ( key == KEY_LALT || key == KEY_RALT ) {
		cl_alt_down = down;
	}
	// capslock
	if ( key == KEY_CAPSLOCK && down ) {
		cl_caps_down = !cl_caps_down;
	}
}


/*
===================
CL_KeyEvent

Called by the system for both key up and key down events
===================
*/
void CL_KeyEvent (int key, int down, unsigned int time) {
	char	*kb;
	char	cmd[1024];
    gbool   keydown;

	// update impulses.  
	// down impulse
	if ( down && !keys[ key ].down ) {
		keys[ key ].impulse |= 0x1;
	// up impulse
	} else if ( keys[ key ].down && !down ) {
		keys[ key ].impulse |= 0x2;
	}

	// set the state
	keys[key].down = ( down ) ? gtrue : gfalse;

	if ( down )     // key down
    { 
		if ( 1 == ++keys[key].repeats ) {
            ++cl_anykeydown;
		}
	} 
    else            // key up
    {                       
		keys[key].repeats = 0;
		if ( --cl_anykeydown < 0 ) {
			cl_anykeydown = 0;
		}
	}

	// shift, control, alt, capslock
	CL_SetModifiers( key, down );

	// handle ESC specially.  It can never be unbound
	if ( key == KEY_ESCAPE ) {
		if ( !down )
			return;
		/* decide what to do with ESCAPE key depending on context
		switch( )
		*/
		// can't pause while console is exposed
		if ( !(cls.keyCatchers & KEYCATCH_CONSOLE) ) 
			CL_TogglePause();
	}

	// only certain key up commands even matter
	if ( !down ) {
	}

	// if in menu mode, most keys bring up the menu 
	// if ( cls.state & ...
	// 		M_ToggleMenu()

	// also, hardcode ~ key to console
	if ( '`' == key && down ) {
		console.Toggle();
	}

	// distribute the key down event to the apropriate handler
	if ( cls.keyCatchers & KEYCATCH_CONSOLE ) {
		console.keyHandler( key, down );
	} 

	/*
	else if ( cls.keyCatchers & KEYCATCH_UI ) {
		UI_Key( key, down );
	} 
	*/

	/*
	else if ( cls.keyCatchers & KEYCATCH_MESSAGE ) {
		Message_Key( key );
	} 
	*/

	else if ( cls.keyCatchers & KEYCATCH_MAPEDIT ) {
		ME_KeyEvent( key, down, time, cl_anykeydown );
	}

}


#if 0
#ifdef __linux__
  if (key == K_ENTER)
  {
    if (down)
    {
      if (keys[K_ALT].down)
      {
        Key_ClearStates();
        if (Cvar_VariableValue("r_fullscreen") == 0)
        {
          Com_Printf("Switching to fullscreen rendering\n");
          Cvar_Set("r_fullscreen", "1");
        }
        else
        {
          Com_Printf("Switching to windowed rendering\n");
          Cvar_Set("r_fullscreen", "0");
        }
        Cbuf_ExecuteText( EXEC_APPEND, "vid_restart\n");
        return;
      }
    }
  }
#endif
#endif

  /* cant just check for '~', I might have mapped over it

	// console key is hardcoded, so the user can never unbind it
	if (key == '`' || key == '~') {
		if (!down) {
			return;
		}
        //Con_ToggleConsole_f ();
		return;
	}
    */

/* 
	// keys can still be used for bound actions
	if ( down && ( key < 128 || key == K_MOUSE1 ) && ( clc.demoplaying || cls.state == CA_CINEMATIC ) && !cls.keyCatchers) {

		if (Cvar_VariableValue ("com_cameraMode") == 0) {
			Cvar_Set ("nextdemo","");
			key = K_ESCAPE;
		}
	}
*/

/* maybe I need to hardcode escape key too ?

	// escape is always handled special
	if ( key == KEY_ESCAPE && down ) {
		if ( cls.keyCatchers & KEYCATCH_MESSAGE ) {
			// clear message mode
			Message_Key( key );
			return;
		}

		// escape always gets out of GAME stuff
		if (cls.keyCatchers & KEYCATCH_GAME) {
			cls.keyCatchers &= ~KEYCATCH_GAME;
			VM_Call (cgvm, CG_EVENT_HANDLING, GAME_EVENT_NONE);
			return;
		}

		if ( !( cls.keyCatchers & KEYCATCH_UI ) ) {
			if ( cls.state == CA_ACTIVE && !clc.demoplaying ) {
				VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_INGAME );
			}
			else {
				CL_Disconnect_f();
				S_StopAllSounds();
				VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_MAIN );
			}
			return;
		}

		VM_Call( uivm, UI_KEY_EVENT, key, down );
		return;
	}
*/

/*
	//
	// key up events only perform actions if the game key binding is
	// a button command (leading + sign).  These will be processed even in
	// console mode and menu mode, to keep the character from continuing 
	// an action started before a mode switch.
	//
	if (!down) {
		kb = keys[key].binding;

		CL_AddKeyUpCommands( key, kb );

		if ( cls.keyCatchers & KEYCATCH_UI && uivm ) {
			VM_Call( uivm, UI_KEY_EVENT, key, down );
		} else if ( cls.keyCatchers & KEYCATCH_GAME && cgvm ) {
			VM_Call( cgvm, CG_KEY_EVENT, key, down );
		} 

		return;
	}
*/



/*
    // send the bound action
    kb = keys[key].binding;
    if ( !kb ) {
        if (key >= 200) {
            Com_Printf ( "%s is unbound, use controls menu to set.\n", "whatever" );
//					, Key_KeynumToString( key ) );
        }
    } else if (kb[0] == '+') {	
        int i;
        char button[1024], *buttonPtr;
        buttonPtr = button;
        for ( i = 0; ; i++ ) {
            if ( kb[i] == ';' || !kb[i] ) {
                *buttonPtr = '\0';
                if ( button[0] == '+') {
                    // button commands add keynum and time as parms 
                    //  so that multiple
                    //  sources can be discriminated and subframe corrected
                    C_snprintf( cmd, sizeof(cmd), "%s %i %i\n", 
                            button, key, time );
                    Cbuf_AddText( cmd );
                } else {
                    // down-only command
                    Cbuf_AddText( button );
                    Cbuf_AddText( "\n" );
                }
                buttonPtr = button;
                while ( (kb[i] <= ' ' || kb[i] == ';') && kb[i] != 0 ) {
                    i++;
                }
            }
            *buttonPtr++ = kb[i];
            if ( !kb[i] ) {
                break;
            }
        }
    } else {
        // down-only command
        Cbuf_AddText( kb );
        Cbuf_AddText( "\n" );
    }
}
*/


/*
===================
CL_CharEvent

Normal keyboard characters, already shifted / capslocked / etc
===================
*/
void CL_CharEvent( int key ) {
	// the console key should never be used as a char
	if ( key == '`' || key == '~' ) {
		return;
	}

#if 0
	// distribute the key down event to the apropriate handler
	if ( cls.keyCatchers & KEYCATCH_CONSOLE )
	{
		Field_CharEvent( &g_consoleField, key );
	}
	else if ( cls.keyCatchers & KEYCATCH_UI )
	{
		VM_Call( uivm, UI_KEY_EVENT, key | K_CHAR_FLAG, qtrue );
	}
	else if ( cls.keyCatchers & KEYCATCH_MESSAGE ) 
	{
		Field_CharEvent( &chatField, key );
	}
	else if ( cls.state == CA_DISCONNECTED )
	{
		Field_CharEvent( &g_consoleField, key );
	}
#endif

}

void CL_InitKeys( void ) 
{
	// old way
    for ( int i = 0 ; i < MAXKEYS; i++ ) {
        keys[i].down = gfalse;
    }

	// just smeer the whole thing
	memset( keys, 0, sizeof(keys) );
}

