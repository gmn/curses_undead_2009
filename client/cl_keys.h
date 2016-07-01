
#ifndef __CL_KEYS_H__
#define __CL_KEYS_H__ 1

#if !defined ( __GBOOL__ )
# include "../common/common.h"
#endif

// keydefs_t - key mappings for the WM_KEYDOWN, WM_SYSKEYDOWN events
//              these are to map a keyboard's physical keys 1 to 1, 
typedef enum {
    KEY_NULLKEY = 0,

    KEY_KP_0,
    KEY_KP_1,
    KEY_KP_2,
    KEY_KP_3,
    KEY_KP_4,
    KEY_KP_5,
    KEY_KP_6,
    KEY_KP_7,
    KEY_KP_8,
    KEY_KP_9,
    KEY_KP_ASTERIX,
    KEY_KP_PLUS,
    KEY_KP_MINUS,
    KEY_KP_PERIOD,

    KEY_F1,
    KEY_F2,
    KEY_F3,
    KEY_F4,
    KEY_F5,
    KEY_F6,
    KEY_F7,
    KEY_F8,
    KEY_F9,
    KEY_F10,
    KEY_F11,
    KEY_F12,
    KEY_ESCAPE = 27,

    KEY_LCTRL,
    KEY_RCTRL,
    KEY_LALT,
    KEY_RALT,
    KEY_SPACEBAR  = 32,
    KEY_LSHIFT,
    KEY_RSHIFT,

    KEY_BACKSPACE,
    KEY_ENTER,
    KEY_CAPSLOCK,
    KEY_TAB,

    KEY_SINGLEQUOT = '\'',  // 39

    KEY_COMMA = ',',        // 44
    KEY_MINUS = '-',        // 45
    KEY_PERIOD = '.',       // 46
    KEY_FORWARDSLASH = '/', // 47

    KEY_0 = '0',            // 48
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    KEY_9,                  // 57

    KEY_SEMICOLON = ';',    // 59

    KEY_EQUAL = '=',        // 61

    KEY_PAUSE,
    KEY_NUMLOCK,
    KEY_SCROLLLOCK,
    KEY_HOME,
    KEY_END,
    KEY_PGUP,
    KEY_PGDOWN,
    KEY_INSERT,
    KEY_DELETE,

	KEY_UP,
	KEY_DOWN,
	KEY_LEFT,
	KEY_RIGHT,

    KEY_LBRACKET = '[',     // 91
    KEY_BACKSLASH = '\\',   // 92
    KEY_RBRACKET = ']',     // 93

	KEY_a = 'a',            // 97
	KEY_b,
	KEY_c,
	KEY_d,
	KEY_e,
	KEY_f,
	KEY_g,
	KEY_h,
	KEY_i,
	KEY_j,
	KEY_k,
	KEY_l,
	KEY_m,
	KEY_n,
	KEY_o,
	KEY_p,
	KEY_q,
	KEY_r,
	KEY_s,
	KEY_t,
	KEY_u,
	KEY_v,
	KEY_w,
	KEY_x,
	KEY_y,
	KEY_z,                  // 122

    KEY_MOUSE0,
    KEY_MOUSE1,
    KEY_MOUSE2,
    KEY_MOUSE3,
    KEY_MOUSE4,
    KEY_MOUSE5,
    KEY_MOUSE6,
    KEY_MOUSE7,
    KEY_MWHEELDOWN,
    KEY_MWHEELUP,

    KEY_CEILING_VAL
} keydefs_t;

#define MAXKEYS 256
#if KEY_CEILING_VAL > MAXKEYS
# error ERROR: Too many Keys defined in __FILE__
#endif

typedef struct {
	gbool   	down;
	char *		binding;
	short		impulse;
	int			repeats;
} gkey_t;

typedef struct actionButton_s {
    int         binding[2];     // keys bound to this action
	int			down[2];		// key nums holding it down
	unsigned	downtime;		// msec timestamp
	unsigned	msec;			// msec down this frame if both a down and up happened
	bool	    active;			// current state
	bool	    wasPressed;		// set when down, not cleared when up
    actionButton_s() {
        binding[0] = binding[1] = 0;
		down[0] = down[1] = 0;
		downtime = 0;
		msec = 0;
		active = 0;
		wasPressed = 0;
    }
} actionButton_t;


#endif __CL_KEYS_H__


extern gkey_t keys[MAXKEYS];
void CL_InitKeys( void );

int CL_stringToKey( const char *string );
const char *CL_keyToString( int );
actionButton_t * CL_stringToButton( const char * );
