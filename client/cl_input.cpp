/* 
 *
 * cl_input.cpp - joystick and mouse stuff 
 *
 * - client input passes here each frame (including keyboard input)
 *
 */

#include "../common/com_suppress.h"
#include "cl_local.h"
#include "../mapedit/mapedit.h" // ME_MouseFrame
#include "cl_console.h"
#include "../game/g_move.h"



actionButton_t act_forwards;
actionButton_t act_backwards;
actionButton_t act_left; // strafing only, no turning
actionButton_t act_right; 
actionButton_t act_use;
actionButton_t act_shoot1;
actionButton_t act_shoot2;
actionButton_t act_shoot3;
actionButton_t act_run;
actionButton_t act_alwaysRun;
actionButton_t act_talk;
actionButton_t act_map;
actionButton_t act_inventory;
actionButton_t act_buttons[ 4 ];
            // next weapon, prev weapon, next item, prev item

struct {
    actionButton_t *btn;
    char *name;
    Move_t move;
} buttonNames[] = {
    { &act_forwards, "act_forwards", MOVE_UP },
    { &act_backwards, "act_backwards", MOVE_DOWN },
    { &act_left, "act_left", MOVE_LEFT },
    { &act_right, "act_right", MOVE_RIGHT },
    { &act_use, "act_use", MOVE_USE },
    { &act_shoot1, "act_shoot1", MOVE_SHOOT1 },
    { &act_shoot2, "act_shoot2", MOVE_SHOOT2 },
    { &act_shoot3, "act_shoot3", MOVE_SHOOT3 },
    { &act_run, "act_run", MOVE_RUN },
    { &act_alwaysRun, "act_alwaysRun", MOVE_NONE },
    { &act_talk, "act_talk", MOVE_TALK },
    { &act_map, "act_map", MOVE_MAP },
    { &act_inventory, "act_inventory", MOVE_INV },
};
const unsigned int totalButtons = sizeof(buttonNames)/sizeof(buttonNames[0]);

actionButton_t * CL_stringToButton( const char *str ) {
    if ( !str || !str[0] )
        return NULL;
    for ( int i = 0; i < totalButtons; i++ ) {
        if ( !strcmp( str, buttonNames[i].name ) ) {
            return buttonNames[i].btn;
        }
    }
    return NULL;
}

void CL_BindKey( const char *btn, const char *key ) {
    actionButton_t *b = CL_stringToButton( btn );
    if ( !b ) {
        console.Printf( "button %s not found", btn );
        return;
    }
    if ( !key ) {
        if ( b->binding[0] && b->binding[1] )
            console.Printf( "%s bound to %s %s", btn, CL_keyToString(b->binding[0]), CL_keyToString(b->binding[1]) );
        else if ( b->binding[0] ) 
            console.Printf( "%s bound to %s", btn, CL_keyToString(b->binding[0]) );
        else
            console.Printf( "%s not bound", btn );
        return;
    }
    int n = CL_stringToKey(key);
    if ( !n ) {
        console.Printf("supplied key: %s not understood", key);
        return;
    }

    // 1 or more already set
    if ( b->binding[0] == n || b->binding[1] == n ) {
        console.Printf ("button %s already set to %s, %s", btn, CL_keyToString(b->binding[0]),CL_keyToString(b->binding[1]) );
        return;
    }
    
    // if the 1st is set, set the 2nd, else set the first
    if ( b->binding[0] != 0 ) {
        b->binding[1] = n;
    } else {
        b->binding[0] = n;
    }

    if ( b->binding[0] && b->binding[1] ) 
        console.Printf ("button %s is set to %s, %s", btn, CL_keyToString(b->binding[0]),CL_keyToString(b->binding[1]) );
    else if ( b->binding[0] ) 
        console.Printf ("button %s is set to %s",btn, CL_keyToString(b->binding[0]));
    else
        console.Printf ("button %s is set to %s", btn, CL_keyToString(b->binding[1]) );
}
void CL_UnBindKey( const char * btn ) {
    actionButton_t *b = CL_stringToButton( btn );
    if ( !b ) {
        console.Printf( "button %s not found", btn );
        return;
    }
    b->binding[0] = b->binding[1] = 0;
    console.Printf( "%s unbound", btn );
}

/*
void IN_KeyDown( kbutton_c *b ) {
	int		k;
	char	*c;
	
	c = Cmd_Argv(1);
	if ( c[0] ) {
		k = atoi(c);
	} else {
		k = -1;		// typed manually at the console for continuous down
	}

	if ( k == b->down[0] || k == b->down[1] ) {
		return;		// repeating key
	}
	
	if ( !b->down[0] ) {
		b->down[0] = k;
	} else if ( !b->down[1] ) {
		b->down[1] = k;
	} else {
		Com_Printf ("Three keys down for a button!\n");
		return;
	}
	
	if ( b->active ) {
		return;		// still down
	}

	// save timestamp for partial frame summing
	c = Cmd_Argv(2);
	b->downtime = atoi(c);

	b->active = qtrue;
	b->wasPressed = qtrue;
}
void IN_KeyUp( kbutton_t *b ) {
	int		k;
	char	*c;
	unsigned	uptime;

	c = Cmd_Argv(1);
	if ( c[0] ) {
		k = atoi(c);
	} else {
		// typed manually at the console, assume for unsticking, so clear all
		b->down[0] = b->down[1] = 0;
		b->active = qfalse;
		return;
	}

	if ( b->down[0] == k ) {
		b->down[0] = 0;
	} else if ( b->down[1] == k ) {
		b->down[1] = 0;
	} else {
		return;		// key up without coresponding down (menu pass through)
	}
	if ( b->down[0] || b->down[1] ) {
		return;		// some other key is still holding it down
	}

	b->active = qfalse;

	// save timestamp for partial frame summing
	c = Cmd_Argv(2);
	uptime = atoi(c);
	if ( uptime ) {
		b->msec += uptime - b->downtime;
	} else {
		b->msec += frame_msec / 2;
	}

	b->active = qfalse;
}
*/




/*
void IN_UpDown(void) {IN_KeyDown(&in_up);}
void IN_UpUp(void) {IN_KeyUp(&in_up);}
void IN_DownDown(void) {IN_KeyDown(&in_down);}
void IN_DownUp(void) {IN_KeyUp(&in_down);}
void IN_LeftDown(void) {IN_KeyDown(&in_left);}
void IN_LeftUp(void) {IN_KeyUp(&in_left);}
void IN_RightDown(void) {IN_KeyDown(&in_right);}
void IN_RightUp(void) {IN_KeyUp(&in_right);}
void IN_SpeedDown(void) {IN_KeyDown(&in_speed);}
void IN_SpeedUp(void) {IN_KeyUp(&in_speed);}
*/

mouseFrame_t *cl_mframe = NULL;
static int lastFrame = 0;
static const int timeForMagNotch = 0;
static int notch_time = 0;
const float MIN_MAG = 1.0f;
const float MAX_MAG = 13.0f ;

void CL_MouseEvent( void *vframe , unsigned int time ) 
{
	int last_x = 0, last_y = 0;

	mouseFrame_t *mframe = (mouseFrame_t *) vframe;

    cl_mframe = mframe;

    /* no VMs
     *
	if ( cls.keyCatchers & KEYCATCH_UI ) {
		VM_Call( uivm, UI_MOUSE_EVENT, dx, dy );
	} else if (cls.keyCatchers & KEYCATCH_GAME) {
		VM_Call (cgvm, CG_MOUSE_EVENT, dx, dy);
	} else {
		cl.mouseDx[cl.mouseIndex] += dx;
		cl.mouseDy[cl.mouseIndex] += dy;
	}
    */

	if ( cls.keyCatchers & KEYCATCH_MAPEDIT ) {
		ME_MouseFrame( mframe );
	}

	if ( cls.keyCatchers & KEYCATCH_CONSOLE ) {
		console.mouseHandler( mframe );
	}

    // let mouse wheel set global magnification
    // this is mostly a debug/design thing.  I don't think it'll stay
    //  in the game.  I'd like camera to zoom automatic
    if ( lastFrame != mframe->frameNum && mframe->z != 0 &&
		 !(cls.keyCatchers & KEYCATCH_CONSOLE) ) {
        const int t = now();
        lastFrame = mframe->frameNum;
        if ( !notch_time  ) {
            notch_time = t;
        }
        if ( t - notch_time > timeForMagNotch ) {
            notch_time = t;
            const float step = 1.0333f;
            float r0 = M_Ratio();
            float r1;

			r1 = ( mframe->z > 0 ) ? r0 / step : r0 * step;

            if ( r1 > MIN_MAG && r1 < MAX_MAG ) {
                float dr = ( r0 - r1 ) * 0.5f;
                float RES[2] = { M_Width() * dr, M_Height() * dr } ;
                M_MoveWorld( RES[0], RES[1] );
                M_AdjustUnit( M_Width() * r1, M_Height() * r1 );
                M_SetRatio( r1 );
            }
        }
    }

/*
		case MEC_ZOOM_IN:
			v = M_GetMainViewport();
			r0 = v->ratio;

			v->ratio /= 1.05f;
			v->iratio = 1.f / v->ratio;	// always 1/ratio

			dr = r0 - v->ratio;
			dr *= 0.5;
			RES[0] = v->res->width * dr;
			RES[1] = v->res->height * dr;

			v->world.x += RES[0];
			v->world.y += RES[1];

			v->unit_width = v->res->width * v->ratio;
			v->unit_height = v->res->height * v->ratio;

			break;
		case MEC_ZOOM_OUT:
			v = M_GetMainViewport();
			r0 = v->ratio;

			v->ratio *= 1.05f;
			v->iratio = 1.f / v->ratio;

			dr = r0 - v->ratio;
			dr *= 0.5;
			RES[0] = v->res->width * dr;
			RES[1] = v->res->height * dr;

			v->world.x += RES[0];
			v->world.y += RES[1];

			v->unit_width = v->res->width * v->ratio;
			v->unit_height = v->res->height * v->ratio;
*/
}




/*
=================
CL_MouseMove
=================
*/
void CL_MouseMove( usercmd_t *cmd ) {

//	mouseFrame_t *mf  =  Get_MouseFrame();



	/*
	float	mx, my;
	float	accelSensitivity;
	float	rate;

	// allow mouse smoothing
	if ( m_filter->integer ) {
		mx = ( cl.mouseDx[0] + cl.mouseDx[1] ) * 0.5;
		my = ( cl.mouseDy[0] + cl.mouseDy[1] ) * 0.5;
	} else {
		mx = cl.mouseDx[cl.mouseIndex];
		my = cl.mouseDy[cl.mouseIndex];
	}
	cl.mouseIndex ^= 1;
	cl.mouseDx[cl.mouseIndex] = 0;
	cl.mouseDy[cl.mouseIndex] = 0;

	rate = sqrt( mx * mx + my * my ) / (float)frame_msec;
	accelSensitivity = cl_sensitivity->value + rate * cl_mouseAccel->value;

	// scale by FOV
	accelSensitivity *= cl.cgameSensitivity;

	if ( rate && cl_showMouseRate->integer ) {
		Com_Printf( "%f : %f\n", rate, accelSensitivity );
	}

	mx *= accelSensitivity;
	my *= accelSensitivity;

	if (!mx && !my) {
		return;
	}

	// add mouse X/Y movement to cmd
	if ( in_strafe.active ) {
		cmd->rightmove = ClampChar( cmd->rightmove + m_side->value * mx );
	} else {
		cl.viewangles[YAW] -= m_yaw->value * mx;
	}

	if ( (in_mlooking || cl_freelook->integer) && !in_strafe.active ) {
		cl.viewangles[PITCH] += m_pitch->value * my;
	} else {
		cmd->forwardmove = ClampChar( cmd->forwardmove - m_forward->value * my );
	}
	*/

    if ( cl_mframe && cl_mframe->mb[0] ) {
	    cmd->buttons |= MOVE_SHOOT1;
    }
}


// do mice and joystick stuff
void Input_Frame( void ) {
	// direct Input mouse
    if ( di_mouse->integer() )
        Com_UpdateMouse();
}


/*
===============
CL_KeyState

Returns the fraction of the frame that the key was down
===============
*/
float CL_KeyState( actionButton_t *key ) {
	float		val;
	int			msec;

	msec = key->msec;
	key->msec = 0;

	if ( key->active ) {
		// still down
		if ( !key->downtime ) {
			msec = com_frameTime;
		} else {
			msec += com_frameTime - key->downtime;
		}
		key->downtime = com_frameTime;
	}

/*	val = (float)msec / frame_msec;
	if ( val < 0 ) {
		val = 0;
	}
	if ( val > 1 ) {
		val = 1;
	}
*/

	return val;
}



extern int cl_shift_down;
extern int cl_ctrl_down;
extern int cl_caps_down;
extern int cl_alt_down;

#if 0
static int shift_down = 0;
static int ctrl_down = 0;
static int alt_down = 0;

/*
====================
 CL_GameKeyHandler

	handler for game key presses.  key input is turned into usercmd_t here.

	FIXME: this is wrong, un-hooking for now
====================
*/
void CL_GameKeyHandler( int key, int down, unsigned int time, int anykeydown ) {
	
	// shift
	if ( key == KEY_LSHIFT || key == KEY_RSHIFT ) {
		shift_down = down;
	}
	// ctrl
	if ( key == KEY_LCTRL || key == KEY_RCTRL ) {
		ctrl_down = down;
	}
	// alt
	if ( key == KEY_LALT || key == KEY_RALT ) {
		alt_down = down;
	}

	// debug block print
	if ( key == KEY_TAB && down && com_developer->integer() ) {
		if ( cls.state == CS_PLAYMAP )
			controller.togglePrintBlocks();
	}
}
#endif // 





/* I just tthought of one reason why this doesn't work.  Because w can't
detect KEY UP events.  The only real way to do this is to create a button
type, and then you can have multiple keys mapped to a single button, and in 
order to make a move, you just check ALL buttons.  makes sense.  Its sort of a 
seeve.  concentrating a multitude of input into a simple conduit that we 
check because it's important to the game. */

void TogglePrintBlocks( void );

// Create Game Commands

void CL_KeyMove( usercmd_t *cmd ) {

    // check all the possible buttons 
    int i;
    int k1, k2;
    int move = MOVE_NONE;
    const int AXIS_AMT = 127;

    if ( !(cls.keyCatchers & KEYCATCH_GAME) )
        return;

    // keys get it before mouse, so they can do this
    cmd->axis[0] = 0; cmd->axis[1] = 0;

    for ( i = 0; i < totalButtons; i++ ) {
        // if bound to any key
		k1 = k2 = 0;
        if ( buttonNames[i].btn->binding[0] || buttonNames[i].btn->binding[1] ) {
			k1 = buttonNames[i].btn->binding[0];
			k2 = buttonNames[i].btn->binding[1];
            if ( !k1 || !keys[k1].down ) 
                k1 = 0;
            if ( !k2 || !keys[k2].down ) 
                k2 = 0;

            if ( !k1 && !k2 ) {
                continue;
            }

            move |= buttonNames[i].move;

            switch( buttonNames[i].move ) {
            case MOVE_UP:
                cmd->axis[1] = AXIS_AMT;
                if ( k1 && keys[k1].impulse & 3 ) {
                    cmd->axis[1] /= 2;
                } else if ( k1 && keys[k1].impulse & 1 ) {
                    cmd->axis[1] /= 2;
                } else if ( k1 && keys[k1].impulse & 2 ) {
                    cmd->axis[1] /= 4;
                } else if ( k2 && keys[k2].impulse & 3 ) {
                    cmd->axis[1] /= 2;
                } else if ( k2 && keys[k2].impulse & 1 ) {
                    cmd->axis[1] /= 2;
                } else if ( k2 && keys[k2].impulse & 2 ) {
                    cmd->axis[1] /= 4;
                }
                break;
            case MOVE_DOWN:
		        cmd->axis[1] = -AXIS_AMT;
                break;
            case MOVE_LEFT:
		        cmd->axis[0] = -AXIS_AMT;
                break;
            case MOVE_RIGHT:
		        cmd->axis[0] = AXIS_AMT;
                break;
            case MOVE_MAP:
                if ( k1 && keys[ k1 ].down && keys[ k1 ].impulse & 1 ) {
                    TogglePrintBlocks();
                } else
                if ( k2 && keys[ k2 ].down && keys[ k2 ].impulse & 1 ) {
                    TogglePrintBlocks();
                }
                break;
            }

            // adjust movements for impulses
            switch( buttonNames[i].move ) {
            case MOVE_UP:
            case MOVE_DOWN:
                if ( k1 && keys[k1].impulse & 3 ) {
                    cmd->axis[1] /= 2;
                } else if ( k1 && keys[k1].impulse & 1 ) {
                    cmd->axis[1] /= 2;
                } else if ( k1 && keys[k1].impulse & 2 ) {
                    cmd->axis[1] /= 4;
                } else if ( k2 && keys[k2].impulse & 3 ) {
                    cmd->axis[1] /= 2;
                } else if ( k2 && keys[k2].impulse & 1 ) {
                    cmd->axis[1] /= 2;
                } else if ( k2 && keys[k2].impulse & 2 ) {
                    cmd->axis[1] /= 4;
                }
                break;
            case MOVE_LEFT:
            case MOVE_RIGHT:
                if ( k1 && keys[k1].impulse & 3 ) {
                    cmd->axis[0] /= 2;
                } else if ( k1 && keys[k1].impulse & 1 ) {
                    cmd->axis[0] /= 2;
                } else if ( k1 && keys[k1].impulse & 2 ) {
                    cmd->axis[0] /= 4;
                } else if ( k2 && keys[k2].impulse & 3 ) {
                    cmd->axis[0] /= 2;
                } else if ( k2 && keys[k2].impulse & 1 ) {
                    cmd->axis[0] /= 2;
                } else if ( k2 && keys[k2].impulse & 2 ) {
                    cmd->axis[0] /= 4;
                }
                break;
            }

/*
            if ( k1 && keys[ k1 ].impulse & 1 ) {
                move |= DOWN_IMPULSE;
            }
            if ( k1 && keys[ k1 ].impulse & 2 ) {
                move |= UP_IMPULSE;
            }
            if ( k2 && keys[ k2 ].impulse & 1 ) {
                move |= DOWN_IMPULSE;
            }
            if ( k2 && keys[ k2 ].impulse & 2 ) {
                move |= UP_IMPULSE;
            }

	if ( (buttons & (UP_IMPULSE|DOWN_IMPULSE)) ) {
		move[0] *= 0.5f;
		move[1] *= 0.5f;
	} else if ( buttons & DOWN_IMPULSE ) { 
		move[0] *= 0.5f;
		move[1] *= 0.5f;
	} else if ( buttons & UP_IMPULSE ) {
		move[0] *= 0.25f;
		move[1] *= 0.25f;
	}
*/

            // unset impulses
            if ( k1 ) 
                keys[ k1 ].impulse = 0;
            if ( k2 ) 
                keys[ k2 ].impulse = 0;
        }
    }

    // pretty much wont work for key other than caps
    if ( cl_caps_down ) { 
        if ( act_alwaysRun.binding[0] == KEY_CAPSLOCK || 
            act_alwaysRun.binding[1] == KEY_CAPSLOCK ) {
			move |= MOVE_RUN;
		}
	} 

	cmd->buttons = move;
}

#if 0 
old version
void CL_KeyMove( usercmd_t *cmd ) {

	unsigned int move = MOVE_NONE;

	// up
	if ( keys[ KEY_UP ].down ) {
		move |= MOVE_UP;
		cmd->axis[1] = 127;
		if ( keys[ KEY_UP ].impulse & 1 ) {
			move |= DOWN_IMPULSE;
		}
		if ( keys[ KEY_UP ].impulse & 2 ) {
			move |= UP_IMPULSE;
		}
	}
	if ( keys[ KEY_w ].down ) {
		move |= MOVE_UP;
		cmd->axis[1] = 127;
		if ( keys[ KEY_w ].impulse & 1 ) {
			move |= DOWN_IMPULSE;
		}
		if ( keys[ KEY_w ].impulse & 2 ) {
			move |= UP_IMPULSE;
		}
	}

	// down
	if ( keys[ KEY_DOWN ].down ) {
		move |= MOVE_DOWN;
		cmd->axis[1] = -127;
		if ( keys[ KEY_DOWN ].impulse & 1 ) {
			move |= DOWN_IMPULSE;
		}
		if ( keys[ KEY_DOWN ].impulse & 2 ) {
			move |= UP_IMPULSE;
		}
	}
	if ( keys[ KEY_s ].down ) {
		move |= MOVE_DOWN;
		cmd->axis[1] = -127;
		if ( keys[ KEY_s ].impulse & 1 ) {
			move |= DOWN_IMPULSE;
		}
		if ( keys[ KEY_s ].impulse & 2 ) {
			move |= UP_IMPULSE;
		}
	}

	// left
	if ( keys[ KEY_LEFT ].down ) {
		move |= MOVE_LEFT;
		cmd->axis[0] = -127;
		if ( keys[ KEY_LEFT ].impulse & 1 ) {
			move |= DOWN_IMPULSE;
		}
		if ( keys[ KEY_LEFT ].impulse & 2 ) {
			move |= UP_IMPULSE;
		}
	}
	if ( keys[ KEY_a ].down ) {
		move |= MOVE_LEFT;
		cmd->axis[0] = -127;
		if ( keys[ KEY_a ].impulse & 1 ) {
			move |= DOWN_IMPULSE;
		}
		if ( keys[ KEY_a ].impulse & 2 ) {
			move |= UP_IMPULSE;
		}
	}

	// right
	if ( keys[ KEY_RIGHT ].down ) {
		move |= MOVE_RIGHT;
		cmd->axis[0] = +127;
		if ( keys[ KEY_RIGHT ].impulse & 1 ) {
			move |= DOWN_IMPULSE;
		}
		if ( keys[ KEY_RIGHT ].impulse & 2 ) {
			move |= UP_IMPULSE;
		}
	}
	if ( keys[ KEY_d ].down ) {
		move |= MOVE_RIGHT;
		cmd->axis[0] = +127;
		if ( keys[ KEY_d ].impulse & 1 ) {
			move |= DOWN_IMPULSE;
		}
		if ( keys[ KEY_d ].impulse & 2 ) {
			move |= UP_IMPULSE;
		}
	}

	// shoot
	if ( keys[ KEY_LCTRL ].down || keys[ KEY_RCTRL ].down ) {
		move |= MOVE_SHOOT;
	}

	// if always run is set, do run modifier
	
	// else if shift down, do run modifier
	if ( cl_shift_down || cl_caps_down ) {
		move |= MOVE_RUN;
	}

	// use
	if ( keys[ KEY_SPACEBAR ].down ) {
		move |= MOVE_USE;
	}

	//
	cmd->buttons = move;

	// consume these after cmd has been made
	keys[ KEY_UP ].impulse = 0;
	keys[ KEY_w ].impulse = 0;
	keys[ KEY_DOWN ].impulse = 0;
	keys[ KEY_s ].impulse = 0;
	keys[ KEY_LEFT ].impulse = 0;
	keys[ KEY_a ].impulse = 0;
	keys[ KEY_RIGHT ].impulse = 0;
	keys[ KEY_d ].impulse = 0;

    // print blocks / debug mini-map thing
    if ( keys[ KEY_m ].down && keys[ KEY_m ].impulse & 1 ) {
		if ( !(cls.keyCatchers & KEYCATCH_CONSOLE) ) {
            keys[ KEY_m ].impulse = 0;
            TogglePrintBlocks();
        }
    }
}
#endif












#if 0 
	const signed char amt = 127;
	float mult = 1.0f;
	signed char mv = (shift_down) ? amt * 2 : amt;
	mv = amt;

	// impulse modifier
	if ( !keys[key].down && keys[key].impulse & 1 ) {
		keys[key].impulse &= ~1;
		mv >>= 1;
	}

	bool up_impulse = ( keys[key].impulse & 2 ) >> 1;
	keys[key].impulse &= ~2;

	if ( !down && up_impulse ) {
		switch ( key ) {
		case KEY_UP:
		case KEY_w:
			cmd->axis[1] = mv / 2;
			break;
		case KEY_DOWN:
		case KEY_s:
			cmd->axis[1] = -mv / 2;
			break;
		case KEY_LEFT:
		case KEY_a:
			cmd->axis[0] = -mv / 2;
			break;
		case KEY_RIGHT:
		case KEY_d:
			cmd->axis[0] = -mv / 2;
			break;
		}
	} else if ( down ) {
		switch ( key ) {
		case KEY_UP:
		case KEY_w:
			cmd->axis[1] = mv;
			break;
		case KEY_DOWN:
		case KEY_s:
			cmd->axis[1] = -mv;
			break;
		case KEY_LEFT:
		case KEY_a:
			cmd->axis[0] = -mv;
			break;
		case KEY_RIGHT:
		case KEY_d:
			cmd->axis[0] = mv;
			break;
		}
	}
#endif 

/*
	int		movespeed;
	int		forward, side, up;

	//
	// adjust for speed key / running
	// the walking flag is to keep animations consistant
	// even during acceleration and develeration
	//
	if ( in_speed.active ^ cl_run->integer ) {
		movespeed = 127;
		cmd->buttons &= ~BUTTON_WALKING;
	} else {
		cmd->buttons |= BUTTON_WALKING;
		movespeed = 64;
	}

	forward = 0;
	side = 0;
	up = 0;
	if ( in_strafe.active ) {
		side += movespeed * CL_KeyState (&in_right);
		side -= movespeed * CL_KeyState (&in_left);
	}

	side += movespeed * CL_KeyState (&in_moveright);
	side -= movespeed * CL_KeyState (&in_moveleft);


	up += movespeed * CL_KeyState (&in_up);
	up -= movespeed * CL_KeyState (&in_down);

	forward += movespeed * CL_KeyState (&in_forward);
	forward -= movespeed * CL_KeyState (&in_back);

	cmd->forwardmove = ClampChar( forward );
	cmd->rightmove = ClampChar( side );
	cmd->upmove = ClampChar( up );
*/




static usercmd_t CL_CreateCmd( void ) {
    usercmd_t cmd( now() );
  
	CL_KeyMove( &cmd );     // sets cmd based on keystate

    CL_MouseMove( &cmd );

/*
    CL_AdjustAngles();
    CL_CmdButtons( &cmd );  // joystick buttons?
    CL_JoystickMove( &cmd );
    CL_FinishMove( &cmd );
*/
    return cmd;
}


/* 
====================
 CL_CreateNewCommands 

	called in CL_Frame by CL_SendCmd () 
====================
*/
void CL_CreateNewCommands( void ) 
{

	cmdbuf.add( CL_CreateCmd() );



#if 0
	int			cmdNum;

	// no need to create usercmds until we have a gamestate
	if ( cls.state < CS_READY ) {
		return;
	}

/*
	frame_msec = com_frameTime - old_com_frameTime;
	// if running less than 5fps, truncate the extra time to prevent
	// unexpected moves after a hitch
	if ( frame_msec > 200 ) {
		frame_msec = 200;
	}
	old_com_frameTime = com_frameTime;
*/

	// generate a command for this frame
	cls.cmdNumber++;
	cmdNum = cls.cmdNumber & CMD_MASK;
	cls.cmds[cmdNum] = CL_CreateCmd ();
#endif
}



/*
============
CL_InitInput 
============
*/
void CL_InitInput( void ) {
	/*
	Cmd_AddCommand ("centerview",IN_CenterView);
	Cmd_AddCommand ("+moveup",IN_UpDown);
	Cmd_AddCommand ("-moveup",IN_UpUp);
	Cmd_AddCommand ("+movedown",IN_DownDown);
	Cmd_AddCommand ("-movedown",IN_DownUp);
	Cmd_AddCommand ("+left",IN_LeftDown);
	Cmd_AddCommand ("-left",IN_LeftUp);
	Cmd_AddCommand ("+right",IN_RightDown);
	Cmd_AddCommand ("-right",IN_RightUp);

	Cmd_AddCommand ("+speed", IN_SpeedDown);
	Cmd_AddCommand ("-speed", IN_SpeedUp);

	cl_nodelta = Cvar_Get ("cl_nodelta", "0", 0);
	cl_debugMove = Cvar_Get ("cl_debugMove", "0", 0);
    */
}



