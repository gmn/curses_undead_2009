
/***********************************************************************
 *
 *  Win_event.cpp - windows keyboard input and window events
 *
 ***********************************************************************/

#define _WIN32_WINNT 0x0500   // for Mouse Wheel support

#include "win_local.h"

#include "../client/cl_keys.h"
#include "../common/common.h"

#include "wgl_driver.h"


#define	MAX_QUED_EVENTS		256
#define	MASK_QUED_EVENTS	( MAX_QUED_EVENTS - 1 )

sysEvent_c	        eventQue[MAX_QUED_EVENTS];
int			        eventHead, eventTail;





/*
================
Sys_QueEvent

A time of 0 will get the current time
Ptr should either be null, or point to a block of data that can
be freed by the game later.
================
*/
void Sys_QueEvent( int time, sysEventType_t type, int value, int value2, int ptrLength, void *ptr ) {
	sysEvent_c	*ev;

	ev = &eventQue[ eventHead & MASK_QUED_EVENTS ];
	if ( eventHead - eventTail >= MAX_QUED_EVENTS ) {
		Com_Printf("Sys_QueEvent: overflow\n");
		if ( ev->evPtr ) {
			V_Free( ev->evPtr );
		}
		eventTail++;
	}

	eventHead++;

	if ( time == 0 ) {
		time = Com_Millisecond ();
	}

	ev->evTime = time;
	ev->evType = type;
	ev->evValue = value;
	ev->evValue2 = value2;
	ev->evPtrLength = ptrLength;
	ev->evPtr = ptr;
}

/*
================
Sys_GetEvent
================
*/
sysEvent_c Sys_GetEvent( void ) {
    MSG			msg;
	sysEvent_c	ev;
	char		*s;

//	msg_t		netmsg;
//	netadr_t	adr;

	// return if we have data
	if ( eventHead > eventTail ) {
		eventTail++;
		return eventQue[ ( eventTail - 1 ) & MASK_QUED_EVENTS ];
	}

	// pump the message loop
	while (PeekMessage (&msg, NULL, 0, 0, PM_NOREMOVE)) 
    {
        // failure or window close event
		if ( !GetMessage (&msg, NULL, 0, 0) || ( msg.message == WM_QUIT ) ) {
			Com_Quit_f();
		}

		// save the msg time, because wndprocs don't have access to the timestamp
		gld.sysMsgTime = msg.time;

		TranslateMessage (&msg);
      	DispatchMessage (&msg);
	}

    /*
	// check for console commands
	s = Sys_ConsoleInput();
	if ( s ) {
		char	*b;
		int		len;

		len = strlen( s ) + 1;
		b = Z_Malloc( len );
		Q_strncpyz( b, s, len-1 );
		Sys_QueEvent( 0, SE_CONSOLE, 0, 0, len, b );
	}
    */

    /*
	// check for network packets
	MSG_Init( &netmsg, sys_packetReceived, sizeof( sys_packetReceived ) );
	if ( Sys_GetPacket ( &adr, &netmsg ) ) {
		netadr_t		*buf;
		int				len;

		// copy out to a seperate buffer for qeueing
		// the readcount stepahead is for SOCKS support
		len = sizeof( netadr_t ) + netmsg.cursize - netmsg.readcount;
		buf = Z_Malloc( len );
		*buf = adr;
		memcpy( buf+1, &netmsg.data[netmsg.readcount], netmsg.cursize - netmsg.readcount );
		Sys_QueEvent( 0, SE_PACKET, 0, 0, len, buf );
	}
    */

	// return if we have data
	if ( eventHead > eventTail ) {
		eventTail++;
		return eventQue[ ( eventTail - 1 ) & MASK_QUED_EVENTS ];
	}

	// create an empty event to return
	//C_memset( &ev, 0, sizeof( ev ) );
	memset( &ev, 0, sizeof( ev ) );
	ev.evTime = timeGetTime();

	return ev;
}



static byte scantokey[128] = 
{
    //0      1       2       3       4       5       6       7
    //8      9       A       B       C       D       E       F
/* 0 */
     0, KEY_ESCAPE, '1',    '2',    '3',    '4',    '5',    '6',    
    '7',    '8',    '9',    '0',    '-',    '=', KEY_BACKSPACE, KEY_TAB,
/* 1 */
    'q',    'w',    'e',    'r',    't',    'y',    'u',    'i',
    'o',    'p',    '[',    ']', KEY_ENTER, KEY_LCTRL, 'a',  's',
/* 2 */
    'd',    'f',    'g',    'h',    'j',    'k',    'l',    ';',
    '\'',   '`', KEY_LSHIFT,'\\',   'z',    'x',    'c',    'v',
/* 3 */
    'b',    'n',    'm',    ',',    '.',    '/', KEY_RSHIFT, KEY_KP_ASTERIX,
    KEY_LALT,' ',KEY_CAPSLOCK,KEY_F1,KEY_F2,KEY_F3,KEY_F4, KEY_F5,
/* 4 */
    KEY_F6,KEY_F7,KEY_F8,KEY_F9,KEY_F10,KEY_PAUSE,KEY_SCROLLLOCK,KEY_KP_7,
    KEY_KP_8,KEY_KP_9,KEY_KP_MINUS,KEY_KP_4,KEY_KP_5,KEY_KP_6,KEY_KP_PLUS,KEY_KP_1,
/* 5 */  
    KEY_KP_2,KEY_KP_3,KEY_KP_0,KEY_KP_PERIOD,0, 0,      0,   KEY_F11,
    KEY_F12,    0,      0,      0,      0,      0,      0,      0, 
/* 6 */
    0,          0,      0,      0,      0,      0,      0,      0,
    0,          0,      0,      0,      0,      0,      0,      0,
/* 7 */
    0,          0,      0,      0,      0,      0,      0,      0,
    0,          0,      0,      0,      0,      0,      0,      0
};
    


/*
==================== 
 LParamToGkey()
 - translates from windows key-mappings to gkeys.  there should be a 
    unique identifier for every key on a 105 key ibm extended keyboard
==================== 
*/ 
static int LParamToGkey( int lparam )
{
    int scancode;

    scancode = ( lparam >> 16 ) & 255;

    if ( scancode > 127 )
        return 0;

    // ibm extended keyboard keys
    if ( lparam & ( 1 << 24 ) )
    {
        switch ( scancode ) {
        case 29: return KEY_RCTRL;
        case 56: return KEY_RALT;
        case 69: return KEY_NUMLOCK;
        case 71: return KEY_HOME;
        case 72: return KEY_UP;
        case 73: return KEY_PGUP;
        case 75: return KEY_LEFT;
        case 77: return KEY_RIGHT;
        case 79: return KEY_END;
        case 80: return KEY_DOWN;
        case 81: return KEY_PGDOWN;
        case 82: return KEY_INSERT;
        case 83: return KEY_DELETE;
        default: break;
        }
    } 

    return scantokey[scancode];
}




/*      OLD
====================
WndProc 
- Callback for window, handles system events associated with it
====================
*/
/*
LRESULT CALLBACK WndProc(	HWND	hWnd,
							UINT	uMsg,
							WPARAM	wParam,
							LPARAM	lParam)
{
    sysEvent_t this_ev;

	switch (uMsg)						
	{
        case WM_CREATE:
        {
        //    if ( !gld.hWnd )
        //        gld.hWnd = hWnd;  
            return 0;
        }
		case WM_ACTIVATE:			
		{
            // ?
			return 0;		
		}

		case WM_SYSCOMMAND:	
		{
			switch (wParam)	
			{
				case SC_SCREENSAVE:	
				case SC_MONITORPOWER:	
				return 0;		
			}
			break;		
		}

		case WM_CLOSE:
		{
			PostQuitMessage(0);	
            //sys_shutdown_safe ();
			return 0;		
		}

		case WM_KEYDOWN:
		{
            this_ev.evType  = SE_KEY;
            this_ev.evValue = Win_GetKey (wParam);
            this_ev.evValue2 = 1;
			return 0;			
		}

		case WM_KEYUP:		
		{
            this_ev.evType  = SE_KEY;
            this_ev.evValue = Win_GetKey (wParam);
            this_ev.evValue2 = 1;
            //E_PostEvent(&this_ev);
			return 0;			
		}

		case WM_SIZE:		
		{
			//GL_ResizeScene(LOWORD(lParam),HIWORD(lParam));  
			return 0;		
		}
	}

	// pass unhandled messages To DefWindowProc
	return DefWindowProc( hWnd, uMsg, wParam, lParam );
}
*/

void IN_Win32Mouse( LPARAM lParam, WPARAM wParam, UINT uMsg ) {
    const int totalFrames = 64;
	static mouseFrame_t localFrames[totalFrames], *m;
    static int which;

    which = (which+1) & (totalFrames-1);
    m = &localFrames[ which ];
	m->reset();

	// direct input mouse overrides win32 mouse messages
	if ( di_mouse->integer() )
		return;

	switch ( uMsg ) {
	case WM_MOUSEWHEEL:
		m->z = (short) HIWORD ( wParam );		// 120 or -120
        Sys_QueEvent ( gld.sysMsgTime, SE_MOUSE, uMsg, 0, 1, (void *)m );
		return;
	case WM_LBUTTONDOWN:
		m->mb[0] = 0x80;
		m->flags = SE_MOUSE_CLICK;
		break;
	case WM_LBUTTONUP:
		break;
	case WM_RBUTTONDOWN:
		m->mb[1] = 0x80;
		break;
	case WM_RBUTTONUP:
		break;
	case WM_MBUTTONDOWN:
		m->mb[2] = 0x80;
		break;
	case WM_MBUTTONUP:
		break;
	case WM_MOUSEMOVE:
		m->flags = SE_MOUSE_MOVE;
		break;
	}

	
	// when sending a WM_MOUSEMOVE, a button may also be down
	if (wParam & MK_LBUTTON)
		m->mb[0] = 0x80;

	if (wParam & MK_RBUTTON)
		m->mb[1] = 0x80;

	if (wParam & MK_MBUTTON)
		m->mb[2] = 0x80;


	m->x = LOWORD( lParam );
	m->y = HIWORD( lParam );

	Sys_QueEvent ( gld.sysMsgTime, SE_MOUSE, uMsg, 0, 1, (void *)m );
}


/*
====================
 WndProc

- Callback for window, handles system events associated with it
====================
*/
//extern gvar_c *in_mouse;
//extern gvar_c *in_logitechbug;
LONG WINAPI WndProc (
    HWND    hWnd,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam)
{
	static gbool flip = gtrue;
	int zDelta, i;
 


	// http://msdn.microsoft.com/library/default.asp?url=/library/en-us/winui/winui/windowsuserinterface/userinput/mouseinput/aboutmouseinput.asp
	// Windows 95, Windows NT 3.51 - uses MSH_MOUSEWHEEL
	// only relevant for non-DI input
	//
	// NOTE: not sure how reliable this is anymore, might trigger double wheel events
    /*
	if (in_mouse->integer != 1)
	{
		if ( uMsg == MSH_MOUSEWHEEL )
		{
			if ( ( ( int ) wParam ) > 0 )
			{
				Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, K_MWHEELUP, qtrue, 0, NULL );
				Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, K_MWHEELUP, qfalse, 0, NULL );
			}
			else
			{
				Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, K_MWHEELDOWN, qtrue, 0, NULL );
				Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, K_MWHEELDOWN, qfalse, 0, NULL );
			}
			return DefWindowProc (hWnd, uMsg, wParam, lParam);
		}
	}
    */
	
    
	switch (uMsg)
	{
/*
	case WM_MOUSEWHEEL:
		// http://msdn.microsoft.com/library/default.asp?url=/library/en-us/winui/winui/windowsuserinterface/userinput/mouseinput/aboutmouseinput.asp
		// Windows 98/Me, Windows NT 4.0 and later - uses WM_MOUSEWHEEL
		// only relevant for non-DI input and when console is toggled in window mode
		//   if console is toggled in window mode (KEYCATCH_CONSOLE) then mouse is released and DI doesn't see any mouse wheel
        
		if (in_mouse->integer != 1 || (!r_fullscreen->integer && (cls.keyCatchers & KEYCATCH_CONSOLE)))
		{
			// 120 increments, might be 240 and multiples if wheel goes too fast
			// NOTE Logitech: logitech drivers are screwed and send the message twice?
			//   could add a cvar to interpret the message as successive press/release events
			zDelta = ( short ) HIWORD( wParam ) / 120;
			if ( zDelta > 0 )
			{
				for(i=0; i<zDelta; i++)
				{
					if (!in_logitechbug->integer)
					{
						Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, K_MWHEELUP, qtrue, 0, NULL );
						Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, K_MWHEELUP, qfalse, 0, NULL );
					}
					else
					{
						Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, K_MWHEELUP, flip, 0, NULL );
						flip = !flip;
					}
				}
			}
			else
			{
				for(i=0; i<-zDelta; i++)
				{
					if (!in_logitechbug->integer)
					{
						Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, K_MWHEELDOWN, qtrue, 0, NULL );
						Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, K_MWHEELDOWN, qfalse, 0, NULL );
					}
					else
					{
						Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, K_MWHEELDOWN, flip, 0, NULL );
						flip = !flip;
					}
				}
			}
			// when an application processes the WM_MOUSEWHEEL message, it must return zero
			return 0;
		}
        
		break;

	case WM_CREATE:

		g_wv.hWnd = hWnd;

		vid_xpos = Cvar_Get ("vid_xpos", "3", CVAR_ARCHIVE);
		vid_ypos = Cvar_Get ("vid_ypos", "22", CVAR_ARCHIVE);
		r_fullscreen = Cvar_Get ("r_fullscreen", "1", CVAR_ARCHIVE | CVAR_LATCH );

		MSH_MOUSEWHEEL = RegisterWindowMessage("MSWHEEL_ROLLMSG"); 

		if ( r_fullscreen->integer )
		{
			WIN_DisableAltTab();
		}
		else
		{
			WIN_EnableAltTab();
		}

		break;
        */
#if 0
	case WM_DISPLAYCHANGE:
		Com_DPrintf( "WM_DISPLAYCHANGE\n" );
		// we need to force a vid_restart if the user has changed
		// their desktop resolution while the game is running,
		// but don't do anything if the message is a result of
		// our own calling of ChangeDisplaySettings
		if ( com_insideVidInit ) {
			break;		// we did this on purpose
		}
		// something else forced a mode change, so restart all our gl stuff
		Cbuf_AddText( "vid_restart\n" );
		break;
#endif

/*
	case WM_DESTROY:
		// let sound and input know about this?
		g_wv.hWnd = NULL;
		if ( r_fullscreen->integer )
		{
			WIN_EnableAltTab();
		}
		break;

	case WM_CLOSE:
		Cbuf_ExecuteText( EXEC_APPEND, "quit" );
		break;

	case WM_ACTIVATE:
		{
			int	fActive, fMinimized;

			fActive = LOWORD(wParam);
			fMinimized = (BOOL) HIWORD(wParam);

			VID_AppActivate( fActive != WA_INACTIVE, fMinimized);
			SNDDMA_Activate();
		}
		break;

	case WM_MOVE:
		{
			int		xPos, yPos;
			RECT r;
			int		style;

			if (!r_fullscreen->integer )
			{
				xPos = (short) LOWORD(lParam);    // horizontal position 
				yPos = (short) HIWORD(lParam);    // vertical position 

				r.left   = 0;
				r.top    = 0;
				r.right  = 1;
				r.bottom = 1;

				style = GetWindowLong( hWnd, GWL_STYLE );
				AdjustWindowRect( &r, style, FALSE );

				Cvar_SetValue( "vid_xpos", xPos + r.left);
				Cvar_SetValue( "vid_ypos", yPos + r.top);
				vid_xpos->modified = qfalse;
				vid_ypos->modified = qfalse;
				if ( g_wv.activeApp )
				{
					IN_Activate (qtrue);
				}
			}
		}
		break;

// this is complicated because Win32 seems to pack multiple mouse events into
// one update sometimes, so we always check all states and look for events
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MOUSEMOVE:
		{
			int	temp;

			temp = 0;

			if (wParam & MK_LBUTTON)
				temp |= 1;

			if (wParam & MK_RBUTTON)
				temp |= 2;

			if (wParam & MK_MBUTTON)
				temp |= 4;

			IN_MouseEvent (temp);
		}
		break;

	case WM_SYSCOMMAND:
		if ( wParam == SC_SCREENSAVE )
			return 0;
		break;
*/
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MOUSEMOVE:
	case WM_MOUSEWHEEL:

		IN_Win32Mouse( lParam, wParam, uMsg );

        //SetCapture( gld.hWnd );
        //return 0; // eat all mouse messages
		break;
	case WM_DESTROY:
        Com_HastyShutdown();
        break;
    case WM_SYSKEYDOWN:
		Sys_QueEvent( gld.sysMsgTime, SE_KEY, LParamToGkey(lParam), 1, 0, NULL );
        break; // pass msg to OS
    case WM_KEYDOWN:
		Sys_QueEvent( gld.sysMsgTime, SE_KEY, LParamToGkey(lParam), 1, 0, NULL );
        return 0; // eat msg
    case WM_SYSKEYUP:
        Sys_QueEvent( gld.sysMsgTime, SE_KEY, LParamToGkey(lParam), 0, 0, NULL );
        break; // pass msg to OS
    case WM_KEYUP:
        Sys_QueEvent( gld.sysMsgTime, SE_KEY, LParamToGkey(lParam), 0, 0, NULL );
        return 0; // eat msg

	case WM_CHAR:
        Sys_QueEvent( gld.sysMsgTime, SE_CHAR, wParam, 0, 0, NULL );
        return 0; // eat msg

	case WM_SETFOCUS:
		return 0;
	case WM_KILLFOCUS:
		return 0;

    }

    return DefWindowProc( hWnd, uMsg, wParam, lParam );
}



WNDPROC Win_GetWndProc ( void ) {
    return WndProc;
}


