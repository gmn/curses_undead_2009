

/*
==================== 
 win_snd.cpp
==================== 
*/

#include "win_local.h"
#include "wgl_driver.h"

#include "../client/cl_sound.h"
#include "../lib/lib.h"


/////////////////////////////////////////////////////////////////////////////
// local vars
/////////////////////////////////////////////////////////////////////////////
#define SECONDARY_BUFFER_SIZE	0x10000
static HINSTANCE hInstDS;
static LPDIRECTSOUND pDS = NULL;
static LPDIRECTSOUNDBUFFER pDSBuf, pDSPBuf;

extern channel_t    s_channels[MAX_CHANNELS];
extern channel_t    loop_channels[MAX_CHANNELS];
extern int	        numLoopChannels;

static DWORD    gSndBufSize;
static int      sample16;
static DWORD    locksize;

gbool           dsound_init;


/////////////////////////////////////////////////////////////////////////////
// functions
/////////////////////////////////////////////////////////////////////////////


static const char *DSoundError( int error ) {
	switch ( error ) {
	case DSERR_BUFFERLOST:
		return "DSERR_BUFFERLOST";
	case DSERR_INVALIDCALL:
		return "DSERR_INVALIDCALLS";
	case DSERR_INVALIDPARAM:
		return "DSERR_INVALIDPARAM";
	case DSERR_PRIOLEVELNEEDED:
		return "DSERR_PRIOLEVELNEEDED";
	}

	return "unknown";
}

/*
==============
S_GetSamplePos

return the current sample position (in mono samples read)
inside the recirculating dma buffer, so the mixing code will know
how many sample are required to fill it up.
===============
*/
int Sys_Snd_GetSamplePos( void ) {
	MMTIME	mmtime;
	int		s;
	DWORD	dwWrite;
    HRESULT h;

	if ( !dsound_init ) {
		return 0;
	}

	mmtime.wType = TIME_SAMPLES;
	//IDirectSoundBuffer_GetCurrentPosition(pDSBuf, &mmtime.u.sample, &dwWrite);
	h = pDSBuf->GetCurrentPosition( &mmtime.u.sample, &dwWrite );

	s = mmtime.u.sample;

	s >>= sample16;

	s &= ( dma.samples-1 );

	return s;
}

void Sys_Snd_BeginPainting( void ) {
	int		reps;
	DWORD	dwSize2;
	DWORD	*pbuf, *pbuf2;
	HRESULT	hresult;
	DWORD	dwStatus;

	if ( !pDSBuf ) {
		return;
	}

	// if the buffer was lost or stopped, restore it and/or restart it
	if ( IDirectSoundBuffer_GetStatus(pDSBuf, &dwStatus) != DS_OK ) {
		Com_Printf ("Couldn't get sound buffer status\n");
	}
	
	if (dwStatus & DSBSTATUS_BUFFERLOST)
		IDirectSoundBuffer_Restore (pDSBuf);
	
	if (!(dwStatus & DSBSTATUS_PLAYING))
		IDirectSoundBuffer_Play(pDSBuf, 0, 0, DSBPLAY_LOOPING);

	// lock the dsound buffer

	reps = 0;
	dma.buffer = NULL;

	while ((hresult = IDirectSoundBuffer_Lock(pDSBuf, 0, gSndBufSize, (LPVOID *)&pbuf, &locksize, 
								   (LPVOID *)&pbuf2, &dwSize2, 0)) != DS_OK)
	{
		if (hresult != DSERR_BUFFERLOST)
		{
			Com_Printf( "SNDDMA_BeginPainting: Lock failed with error '%s'\n", DSoundError( hresult ) );
			Sys_Snd_Shutdown ();
			return;
		}
		else
		{
			IDirectSoundBuffer_Restore( pDSBuf );
		}

		if (++reps > 2)
			return;
	}
	dma.buffer = (unsigned char *)pbuf;
}

void Sys_Snd_Submit( void ) {
    // unlock the dsound buffer
	if ( pDSBuf ) {
		IDirectSoundBuffer_Unlock(pDSBuf, dma.buffer, locksize, NULL, 0);
	}
}

static gbool Sys_Snd_InitDS( void ) 
{

	HRESULT			hr;
	DSBUFFERDESC	dsbufd;
	DSBCAPS			dsbcaps;
	WAVEFORMATEX	format;
	int				useDirectSound8;
    WAVEFORMATEX    wfmtex;
	//DWORD			fullscreenflag;


    Com_Printf("Initializing DirectSound..\n");

    useDirectSound8 = 1;


    // Create IDirectSound using the primary sound device
    if( FAILED( hr = DirectSoundCreate8( NULL, (LPDIRECTSOUND8 *)&pDS, NULL ) ) ) {
        Com_Printf("failed on DS8 init.  trying DS3..\n");
        if ( FAILED( hr = DirectSoundCreate( NULL, (LPDIRECTSOUND *)&pDS, NULL ) ) ) {
            Com_Printf("failed on DS init.\n");
            Sys_Snd_Shutdown();
            return gfalse;
        }
    }

	hr = IDirectSound_Initialize( pDS, NULL );

	Com_Printf( "ok\n" );

	/*
	if ( gld.fullscreen ) {
		Com_Printf("...setting DSSCL_PRIORITY coop level: " );
		fullscreenflag = DSSCL_PRIORITY;
	} else {
		Com_Printf("...setting DSSCL_NORMAL coop level: " );
		fullscreenflag = DSSCL_NORMAL;
	} */

//	if ( DS_OK != IDirectSound_SetCooperativeLevel( pDS, gld.hWnd, fullscreenflag ) )	{
	//hr = pDS->SetCooperativeLevel( gld.hWnd, fullscreenflag );
	hr = pDS->SetCooperativeLevel( gld.hWnd, DSSCL_PRIORITY );
	if ( FAILED( hr ) )	{
		Com_Printf ("failed\n");
		Sys_Snd_Shutdown ();
		return gfalse;
	}
	Com_Printf("ok\n" );


	// create the secondary buffer we'll actually work with
	dma.channels = 2;
	dma.samplebits = 16;
    dma.speed = 22050;


    // waveformatEx
    C_memset( &wfmtex, 0, sizeof(wfmtex) );
    wfmtex.wFormatTag       =   WAVE_FORMAT_PCM;
    wfmtex.nChannels        =   dma.channels;
    wfmtex.wBitsPerSample   =   dma.samplebits;
    wfmtex.nSamplesPerSec   =   dma.speed;
    // how many bytes per sample: nchan * bits/8
    wfmtex.nBlockAlign      =   dma.channels * dma.samplebits / 8;
    wfmtex.cbSize           =   0;
    wfmtex.nAvgBytesPerSec  =   wfmtex.nSamplesPerSec * wfmtex.nBlockAlign;


    // ds buffer descriptor
	C_memset (&dsbufd, 0, sizeof(dsbufd));
	dsbufd.dwSize = sizeof(DSBUFFERDESC);
	// try hardware first
	dsbufd.dwFlags = DSBCAPS_LOCHARDWARE;
	if (useDirectSound8) {
		dsbufd.dwFlags |= DSBCAPS_GETCURRENTPOSITION2;
	}
	dsbufd.dwBufferBytes = SECONDARY_BUFFER_SIZE;
	//dsbuf.lpwfxFormat = &format;
	dsbufd.lpwfxFormat = &wfmtex;
	
    // note: streaming buffer doesn't specify a flag

    // ds buffer caps
	C_memset(&dsbcaps, 0, sizeof(dsbcaps));
	dsbcaps.dwSize = sizeof(dsbcaps);
	
	Com_Printf( "creating secondary streaming buffer: \n" );
	if (DS_OK == IDirectSound_CreateSoundBuffer(pDS, &dsbufd, &pDSBuf, NULL)) {
		Com_Printf( "...in hardware.  ok\n" );
	}
	else {
        Com_Printf("hardware buffer failed, trying software\n");
		// Couldn't get hardware, fallback to software.
		dsbufd.dwFlags = DSBCAPS_LOCSOFTWARE;
		if (useDirectSound8) {
			dsbufd.dwFlags |= DSBCAPS_GETCURRENTPOSITION2;
		}
		if (DS_OK != IDirectSound_CreateSoundBuffer(pDS, &dsbufd, &pDSBuf, NULL)) {
			Com_Printf( "failed\n" );
			Sys_Snd_Shutdown ();
			return gfalse;
		}
		Com_Printf( "software ok\n" );
	}
		
	// Make sure mixer is active
	if ( DS_OK != IDirectSoundBuffer_Play(pDSBuf, 0, 0, DSBPLAY_LOOPING) ) {
		Com_Printf ("*** Looped sound play failed ***\n");
		Sys_Snd_Shutdown ();
		return gfalse;
	}

	// get the returned buffer size
	if ( DS_OK != IDirectSoundBuffer_GetCaps (pDSBuf, &dsbcaps) ) {
		Com_Printf ("*** GetCaps failed ***\n");
		Sys_Snd_Shutdown ();
		return gfalse;
	}
	
	gSndBufSize = dsbcaps.dwBufferBytes;

	dma.channels            = wfmtex.nChannels;
	dma.samplebits          = wfmtex.wBitsPerSample;
	dma.speed               = wfmtex.nSamplesPerSec;
	dma.samples             = gSndBufSize/(dma.samplebits/8);
	dma.submission_chunk    = 1;
	dma.buffer              = NULL;	 // must be locked first

	sample16 = (dma.samplebits/8) - 1;

	Sys_Snd_BeginPainting();

	if (dma.buffer) {
		C_memset(dma.buffer, 0, dma.samples * dma.samplebits/8);
    }

	Sys_Snd_Submit();

	return gtrue;
}


gbool Sys_Snd_Init( void ) 
{

	// Change the registry key under which our settings are stored.
	//SetRegistryKey(_T("The Lobsters"));

    // specify the concurrency model in windows
//    CoInitializeEx( NULL, COINIT_APARTMENTTHREADED );

    dsound_init = gfalse;
    if( Sys_Snd_InitDS() ) {
	    dsound_init = gtrue;
        return gtrue;
    }

    Com_Printf("Sys_Snd_Init failed\n");
    Sys_Snd_Shutdown();
    return gfalse;
}

static void Sys_Snd_ShutdownDS( void ) {
	Com_Printf( "Shutting down OS sound component\n" );

	if ( pDS ) {
		Com_Printf( "Destroying DS buffers\n" );
		if ( pDS )
		{
			Com_Printf( "...setting NORMAL coop level\n" );
			IDirectSound_SetCooperativeLevel( pDS, gld.hWnd, DSSCL_NORMAL );
		}

		if ( pDSBuf )
		{
			Com_Printf( "...stopping and releasing sound buffer\n" );
			IDirectSoundBuffer_Stop( pDSBuf );
			IDirectSoundBuffer_Release( pDSBuf );
		}

		// only release primary buffer if it's not also the mixing buffer 
        //  we just released
		if ( pDSPBuf && ( pDSBuf != pDSPBuf ) )
		{
			Com_Printf( "...releasing primary buffer\n" );
			IDirectSoundBuffer_Release( pDSPBuf );
		}
		pDSBuf = NULL;
		pDSPBuf = NULL;

		dma.buffer = NULL;

		Com_Printf( "...releasing DS object\n" );
		IDirectSound_Release( pDS );
	}

    /*
	if ( hInstDS ) {
		Com_Printf( "...freeing DSOUND.DLL\n" );
		FreeLibrary( hInstDS );
		hInstDS = NULL;
	} */

	pDS = NULL;
	pDSBuf = NULL;
	pDSPBuf = NULL;
	dsound_init = gfalse;
	C_memset ((void *)&dma, 0, sizeof (dma));
}

void Sys_Snd_Shutdown( void ) {
    Sys_Snd_ShutdownDS();
 //   CoUnitialize();
}



