
// win_input.cpp - direct X windows mouse code


#include <windows.h>
#include <wchar.h> // vswprintf

#include <stdlib.h> // memset ?

#include "win_local.h"
#include "wgl_driver.h"

wininput_c winp;


/////////////////////////////////////////////////////
// stubs 
//
#include <process.h>
#include <stdarg.h>
void werror ( const wchar_t *wfmt, ... )
{
    va_list argp;
    wchar_t wmsg[1024];

	va_start( argp, wfmt );
	_vsnwprintf( wmsg, 1024, wfmt, argp );
//	vswprintf( wmsg, wfmt, argp);
	va_end( argp );
    
    MessageBox (NULL, wmsg, L"massive error buddy" , 0) ;
    exit(-1);
}
//
/////////////////////////////////////////////////////




    /*
predefined DirectInput global variables:

    * c_dfDIKeyboard
    * c_dfDIMouse
    * c_dfDIMouse2
    * c_dfDIJoystick
    * c_dfDIJoystick2

    */

/*
LPDIRECTINPUTDEVICE8    lpDIMouse;
IDirectInput8*		    lpDI; 
HINSTANCE               winp.hInst_DI = 0L;
HWND                    winp.hWnd_DI;
*/

//	IDirectInputDevice8* m_pKeyboard;
//	IDirectInputDevice8* lpdiMouse;

/* create proto
#if DIRECTINPUT_VERSION > 0x0700
HRESULT WINAPI DirectInput8Create(      
    HINSTANCE hinst,
    DWORD dwVersion,
    REFIID riidltf,
    LPVOID *ppvOut,
    LPUNKNOWN punkOuter
);
extern HRESULT WINAPI DirectInput8Create(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID *ppvOut, LPUNKNOWN punkOuter);
#endif
*/

/* CLARIFY, YOU only keep 16 frames of mouse data to compute velocity , not
 * as an input buffer.  this will probably go in the event buffer anyway.
 * */
void wininput_c::setFrame4( uint32 flag, int x, int y, int z, byte mb1, byte mb2, byte mb3, byte mb4 ) 
{
    if (started != MOUSE_INIT_MAGIC) {
        start();
        return;
    }

    endFrame = (++endFrame) & MOUSE_FRAME_MASK;
    ++numFrames;

	mframe[endFrame].frameNum = numFrames;
    mframe[endFrame].flags  = flag;
    mframe[endFrame].x      = x;
    mframe[endFrame].y      = y;
    mframe[endFrame].z      = z;
    mframe[endFrame].mb[0] = mb1;
    mframe[endFrame].mb[1] = mb2;
    mframe[endFrame].mb[2] = mb3;
    mframe[endFrame].mb[3] = mb4;
}
void wininput_c::setFrame8( uint32 flag, int x, int y, int z, byte mb1, byte mb2, byte mb3, byte mb4, byte mb5, byte mb6, byte mb7, byte mb8 ) 
{
    if (started != MOUSE_INIT_MAGIC) {
        start();
        return;
    }

    endFrame = (++endFrame) % MOUSE_FRAME_MASK;
    ++numFrames;

	mframe[endFrame].frameNum = numFrames;
    mframe[endFrame].flags  = flag;
    mframe[endFrame].x      = x;
    mframe[endFrame].y      = y;
    mframe[endFrame].z      = z;
    mframe[endFrame].mb[0] = mb1;
    mframe[endFrame].mb[1] = mb2;
    mframe[endFrame].mb[2] = mb3;
    mframe[endFrame].mb[3] = mb4;
    mframe[endFrame].mb[4] = mb5;
    mframe[endFrame].mb[5] = mb6;
    mframe[endFrame].mb[6] = mb7;
    mframe[endFrame].mb[7] = mb8;
}

mouseFrame_t * wininput_c::getFrame( void ) {
    static mouseFrame_t sf; 

    if (numFrames == 0) {
        memset( &sf, 0, sizeof(mouseFrame_t) );
        return (mouseFrame_t *) &sf;
    }

    return &mframe[endFrame];
}

mouseFrame_t * wininput_c::getPrevFrame( void ) {
    static mouseFrame_t sf; 

    if (numFrames == 0) {
        memset( &sf, 0, sizeof(mouseFrame_t) );
        return (mouseFrame_t *) &sf;
    }

	int frame = endFrame;
	if ( --frame < 0 ) {
		frame = NUM_MOUSE_FRAMES - 1;
	}

    return &mframe[frame];
}

// public function
mouseFrame_t *Get_MouseFrame( void ) {
	static mouseFrame_t nf;
	mouseFrame_t *sf, *pf; 

    if ( 0 == winp.getNumFrames() ) {
        memset( (void*)&nf, 0, sizeof(nf) );
        return (mouseFrame_t *)&nf;
    }

	sf = winp.getFrame();
	pf = winp.getPrevFrame();

	memcpy( &nf, sf, sizeof(nf) );

	// compute dx, dy and store in sf
	nf.dx = sf->x - pf->x;
	nf.dy = sf->y - pf->y;

    return &nf;
}



// pointer biatch
HRESULT (WINAPI *pDirectInput8Create)(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID *ppvOut, LPUNKNOWN punkOuter) = 0;
HRESULT (WINAPI *pDirectInput8Create_debug)(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID *ppvOut, LPUNKNOWN punkOuter) = 0;



void Sys_InitDIMouse ( void )
{
	//winp.hWnd			= FindWindow(NULL, windowName);
	winp.hWnd_DI    = gld.hWnd;

    HRESULT         hres;
    DWORD           dwCoopFlags;

    gbool bExclusive = gfalse;
    if( bExclusive )
        dwCoopFlags = DISCL_EXCLUSIVE;
    else
        dwCoopFlags = DISCL_NONEXCLUSIVE;

    gbool bForeground = gfalse;
    if( bForeground )
        dwCoopFlags |= DISCL_FOREGROUND;
    else
        dwCoopFlags |= DISCL_BACKGROUND;

    wprintf (L"initializing DirectInput Mouse ...\n");

    if (!winp.hInst_DI) {
        winp.hInst_DI = LoadLibrary(L"dinput8.dll");
        if (!winp.hInst_DI) {
            werror(L"failed loading dinput.dll\n");
        }
    }

	
    // try somethin crazy here
   	if (!pDirectInput8Create) {
		pDirectInput8Create = (HRESULT (__stdcall *)(HINSTANCE, DWORD ,REFIID, LPVOID *, LPUNKNOWN)) GetProcAddress(winp.hInst_DI,"DirectInput8Create");

		if (!pDirectInput8Create) {
			werror(L"Couldn't get DI proc addr\n");
		}
	} 

	pDirectInput8Create_debug = DirectInput8Create;

    if (DI_OK != DirectInput8Create( winp.hInst_DI, DIRECTINPUT_VERSION, IID_IDirectInput8, (void **)&winp.lpDI, NULL )) {
        werror (L"wtf");
    }

    // GUID_SysMouseEm2 - says on msdn??
    if (DI_OK != winp.lpDI->CreateDevice( GUID_SysMouse, &winp.lpDIMouse, NULL )) {
        werror(L"CreateDevice\n"); 
    }

    // This tells DirectInput that we will be passing a
    // DIMOUSESTATE2 structure to IDirectInputDevice::GetDeviceState.
    if ( FAILED (winp.lpDIMouse->SetDataFormat(&c_dfDIMouse2)) )
        werror(L"unable to set DI Mouse Data format\n");

    hres = winp.lpDIMouse->SetCooperativeLevel( winp.hWnd_DI, dwCoopFlags );

    if ( hres == DIERR_UNSUPPORTED && !bForeground && bExclusive ) {
        werror(L"background exclusive mouse is not allowed\n");
    }

    if (FAILED(hres)) {
        werror(L"set cooperative level failed\n");
    }

    winp.lpDIMouse->Acquire();

    //  update once  
    Sys_UpdateDIMouse();
}


// if there is any mouse data, one Sys_Event is que'd
void Sys_UpdateDIMouse ( void ) 
{
    uint32 flags = 0;
    gbool gotEvent = gfalse;
    HRESULT hres;
    DIMOUSESTATE2 dims2;      // DirectInput mouse state structure
    int limit;

	if (winp.lpDIMouse && GetForegroundWindow() == winp.hWnd_DI)
	{
		winp.lpDIMouse->Acquire();
	}
	else
	{
		if (winp.lpDIMouse) 
            winp.lpDIMouse->Unacquire();
		return; 
	}


    memset( &dims2, 0, sizeof(dims2) );

    hres = winp.lpDIMouse->GetDeviceState( sizeof(DIMOUSESTATE2), &dims2 );

    if( FAILED(hres) ) 
    {
        // DirectInput may be telling us that the input stream has been
        // interrupted.  We aren't tracking any state between polls, so
        // we don't have any special reset that needs to be done.
        // We just re-acquire and try again.
        
        // If input is lost then acquire and keep trying 
        hres = winp.lpDIMouse->Acquire();

/*		limit = 0;
        while( hres == DIERR_INPUTLOST && limit++ < 10 ) 
            hres = winp.lpDIMouse->Acquire();
*/
        while( hres == DIERR_INPUTLOST ) 
            hres = winp.lpDIMouse->Acquire();
        // hres may be DIERR_OTHERAPPHASPRIO or other errors.  This
        // may occur when the app is minimized or in the process of 
        // switching, so just try again later 
        return;
    }

    /* ?
    mousevx[mousepage] = dims2.lX;
	mousevy[mousepage] = dims2.lY;
	mousevz[mousepage] = dims2.lZ;
	mouseb [mousepage] = 0;

	for(int i = 0; i < 8; i ++)
	{
		if (dims2.rgbButtons[i] & 0x80)
			mouseb[mousepage] |= (1 << i);
	}
    return S_OK;
    */

	mouseFrame_t *f = winp.getFrame();


    // axial change
    if ( dims2.lX || dims2.lY || dims2.lZ ) {
        gotEvent = gtrue;
        flags |= MOUSE_AXIAL;
    }
	// reset scroll value is considered an event
	if ( f->z && !dims2.lZ ) {
		gotEvent = gtrue;
		flags |= MOUSE_AXIAL;
	}

    // button press
    if ( dims2.rgbButtons[0] || 
         dims2.rgbButtons[1] || 
         dims2.rgbButtons[2] || 
         dims2.rgbButtons[3] ) 
    {
        flags |= MOUSE_LBUTTONS;
        gotEvent = gtrue;
    } 
    if ( !gotEvent && winp.numButtons == 8 ) {
        if ( dims2.rgbButtons[4] || 
             dims2.rgbButtons[5] || 
             dims2.rgbButtons[6] || 
             dims2.rgbButtons[7] ) {
            gotEvent = gtrue;
            flags |= MOUSE_HBUTTONS;
        }
    }

	// button release
	if ( f->mb[0] || f->mb[1] || f->mb[2] || f->mb[3] ) {
		if ( !( dims2.rgbButtons[0] || 
				dims2.rgbButtons[1] || 
				dims2.rgbButtons[2] || 
				dims2.rgbButtons[3] ) )
		{
			flags |= MOUSE_LBUTTONS;
			gotEvent = gtrue;
		}
	} 
	if ( 8 == winp.numButtons && !gotEvent ) {
		if ( f->mb[4] || f->mb[5] || f->mb[6] || f->mb[7] ) {
			if ( !( dims2.rgbButtons[4] || 
					dims2.rgbButtons[5] || 
					dims2.rgbButtons[6] || 
					dims2.rgbButtons[7] ) )
			{
				flags |= MOUSE_LBUTTONS;
				gotEvent = gtrue;
			}
		} 
	}

    if (gotEvent) {
        if ( winp.numButtons == 4 ) {
            winp.setFrame4( flags,
                        dims2.lX,
                        dims2.lY,
                        dims2.lZ,
                        dims2.rgbButtons[0] & 0x80,
                        dims2.rgbButtons[1] & 0x80,
                        dims2.rgbButtons[2] & 0x80,
                        dims2.rgbButtons[3] & 0x80 );
        } else if ( winp.numButtons == 8 ) {
            winp.setFrame8( flags,
                        dims2.lX,
                        dims2.lY,
                        dims2.lZ,
                        dims2.rgbButtons[0] & 0x80,
                        dims2.rgbButtons[1] & 0x80,
                        dims2.rgbButtons[2] & 0x80,
                        dims2.rgbButtons[3] & 0x80,
                        dims2.rgbButtons[4] & 0x80,
                        dims2.rgbButtons[5] & 0x80,
                        dims2.rgbButtons[6] & 0x80,
                        dims2.rgbButtons[7] & 0x80 );
        }
        Sys_QueEvent ( Com_Millisecond(), SE_MOUSE, 0, 0, 1, (void *)winp.getFrame() );
    }
}

void Sys_DestroyDIMouse( void )
{
    // Unacquire the device one last time just in case 
    // the app tried to exit while the device is still acquired.
    if( winp.lpDIMouse ) 
        winp.lpDIMouse->Unacquire();
    
    // Release any DirectInput objects.
    SAFE_RELEASE( winp.lpDIMouse );

	// give win32 mouse control
	ClipCursor(NULL);
	ReleaseCapture();
	while( ShowCursor(TRUE) < 0 )
		;
}


