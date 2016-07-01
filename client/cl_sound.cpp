/******************************************************************************
 *  cl_sound.cpp
 *
 *  - shamefully copied from id's snd_dma, snd_mix, and snd_mem.  I have
 *    never seen an implementation of a software mixer before, so I dont
 *    think google or your run-of-the-mill programming or dsp book in this 
 *    case were a viable alternative.
 *
 *    thanks to id & jc
 ******************************************************************************/ 
#include "cl_local.h"
#include "cl_sound.h"
#include "../win32/win_public.h"
#include <math.h>
#include "../lib/lib.h"

//=============================================================================
//  internal variables / datem
//=============================================================================

static filehandle_t s_backgroundFile = -1;
static wavinfo_t	s_backgroundInfo;

static gbool        s_backgroundLooping = gfalse;
static int          s_backgroundLoopCount = 0;

//static float musicVolume = 0.7f;

static int s_soundStarted;
static gbool s_soundMuted;
static gbool s_paused = gfalse;

channel_t   s_channels[MAX_CHANNELS];
//channel_t   loop_channels[MAX_CHANNELS];
//int			numLoopChannels;

dma_t		dma;

static int			listener_number;
static vec3_t		listener_origin;
static vec3_t		listener_axis[3];

int			s_soundtime;		// sample PAIRS
int   		s_paintedtime; 		// sample PAIRS


sfx_t		s_knownSfx[MAX_SFX];
int			s_numSfx = 0;


static	sfx_t		*sfxHash[LOOP_HASH];

gvar_c		*s_testsound;
gvar_c		*s_khz;
gvar_c		*s_show;
gvar_c		*s_musicVolume;
gvar_c		*s_separation;
gvar_c		*s_doppler;

gvar_c *		s_volume;
gvar_c *		s_mixahead;
gvar_c *		s_mixPreStep;

char s_backgroundFileName[256];

int  s_backgroundFileType;
enum { BGTYPE_WAV, BGTYPE_OGG };

// ogg stuff
wavinfo_t S_GetOggInfo( void );
int S_OggGetSamples( byte *, unsigned int, gbool * );
void S_StopOggBackgroundTrack( void );
void S_BackgroundOggOpen( const char * );

//static loopSound_t		loopSounds[MAX_GENTITIES];
static	channel_t		*freelist = NULL;

int						s_rawend;
portable_samplepair_t	s_rawsamples[MAX_RAW_SAMPLES];

static	sndBuffer	*sndmem_buffer = NULL;
static	sndBuffer	*sndmem_freelist = NULL;
static	int inUse = 0;
static	int totalInUse = 0;

//short *sfxScratchBuffer = NULL;
//sfx_t *sfxScratchPointer = NULL;
int	   sfxScratchIndex = 0;
gbool snd_setup = gfalse;

static portable_samplepair_t paintbuffer[PAINTBUFFER_SIZE];
static int snd_vol;

AudioManager_c audio;
    
//===========================================================================
// snd memory management
//===========================================================================

// this is fucking ridiculous.  fucking brilliant!
static void SND_free( sndBuffer *v ) {
	*(sndBuffer **)v = sndmem_freelist;
	sndmem_freelist = (sndBuffer*)v;
	inUse += sizeof(sndBuffer);
}

void S_FreeOldestSound( void ) {
	int	i, oldest, used;
	sfx_t	*sfx;
	sndBuffer *buf, *nbuf;

	oldest = Com_Millisecond ();
	used = 0;

	for (i = 1; i < s_numSfx; i++) {
		sfx = &s_knownSfx[i];
		if ( sfx->inMemory && sfx->lastTimeUsed < oldest ) {
			used = i;
			oldest = sfx->lastTimeUsed;
		}
	}

	sfx = &s_knownSfx[used];

	Com_Printf("S_FreeOldestSound: freeing sound %s\n", sfx->soundName);

    /* my way
    buf = sfx->soundData;
    if ( !buf->next ) {
        SND_Free( buf );
    } else {
        while ( buf->next ) {
            buf = buf->next;
            SND_free( buf->prev );
        }
        SND_free( buf );
    } */

    // mine is faster for a reasonably large set, but his is more elegant
	buf = sfx->soundData;
	while ( buf != NULL ) {
		nbuf = buf->next;
		SND_free( buf );
		buf = nbuf;
	} 

	sfx->inMemory = gfalse;
	sfx->soundData = NULL;
}


static sndBuffer* SND_malloc( void ) {
	sndBuffer *v;
redo:
	if (sndmem_freelist == NULL) {
		S_FreeOldestSound();
		goto redo;
	}

	inUse -= sizeof(sndBuffer);
	totalInUse += sizeof(sndBuffer);

	v = sndmem_freelist;
	sndmem_freelist = *(sndBuffer **)sndmem_freelist;
	v->next = NULL;
	return v;
}


static void SND_setup( void ) {
	sndBuffer *p, *q;
	//cvar_t	*cv;
	int scs;
    int soundMegs = 16;

    if ( snd_setup )
        return;

	//cv = Cvar_Get( "com_soundMegs", DEF_COMSOUNDMEGS, CVAR_LATCH | CVAR_ARCHIVE );


	//scs = (cv->integer*1536);   // 1.5k, wtf?
    
    scs = soundMegs * 1536;

	// allocate the stack based hunk allocator
	//sfxScratchBuffer    = (short *) malloc(SND_CHUNK_SIZE * sizeof(short) * 4);	
	//sfxScratchPointer   = NULL;

	inUse               = scs * sizeof(sndBuffer);
	sndmem_buffer       = (sndBuffer *) malloc( inUse );

	p = sndmem_buffer;
	q = p + scs;
	while (--q > p) {
		*(sndBuffer **)q = q-1;
    }
	
	*(sndBuffer **)q = NULL;
	sndmem_freelist = p + scs - 1;

	Com_Printf("Sound memory manager started\n");
    snd_setup = gtrue;
}

static void SND_shutdown( void ) {
    /*
    if ( sfxScratchBuffer ) {
        free( sfxScratchBuffer );
        sfxScratchBuffer = NULL;
    } */
    if ( sndmem_buffer ) {
        free( sndmem_buffer );
        sndmem_buffer = NULL;
    }
}
//===========================================================================
// mem end
//===========================================================================



/* aparently wavs do not HAVE TO HAVE all the 'data', 'info' and 'fmt ' chunks
 * so you have to scan for each one.  gay. 
wavinfo_t S_GetWavInfo( byte *data, int size ) 
{
    wavinfo_t info;
    C_memset( &info, 0, sizeof(info) );
    WAV_HEADER wheader;

    if ( !data ) 
        return info;

    memcpy( (void *)wheader, (void *)data, sizeof(wheader) );

    info.format     = wheader.format.formatTag;
    info.rate       = wheader.format.samplesPerSec; 
    info.width      = wheader.format.bitsPerSample / 8;
    info.channels   = wheader.formal.channels;
    info.samples    = wheader.data.len / wheader.format.blockAlign;
    info.dataofs    = 44;

    return info;
}
*/


// returns a pointer to the first byte in the string supplied in chunk
// or null
// does not match the trailing zero in chunk
static const byte * FindMemChunk( const byte *buf, int buflen, const char *chunk, int chunklen ) {
    const byte *bp, *cp, *bp2;
    bp = buf;
    while ( bp - buf < buflen ) {
        bp2 = bp;
        cp = (byte *) chunk;
        while ( *cp && *cp == *bp2 ) {
            ++cp; ++bp2;
            if ( cp - (byte *)chunk >= chunklen )
                return bp;
        }
        ++bp;
    }
    return NULL;
}

wavinfo_t S_GetWavInfo( const byte *data, int size )
{
    wavinfo_t info;
    memset( &info, 0, sizeof(info) );
    const byte * p;
    WAV_HEADER wh;

    if ( !data )
        return info;

    if ( !(p = FindMemChunk( data, size, "RIFF", 4 )) )
        Com_Error(ERR_FATAL, "error loading wav!\n");
    memcpy( (void *)&wh.header, (void *)p, sizeof(wh.header) );
    if ( !(p = FindMemChunk( data, size, "fmt ", 4 )) )
        Com_Error(ERR_FATAL, "error loading wav!\n");
    p += 8;
    memcpy( (void *)&wh.format, (void *)p, sizeof(wh.format) );
    if ( !(p = FindMemChunk( data, size, "data", 4 )) )
        Com_Error(ERR_FATAL, "error loading wav!\n");
    memcpy( (void *)&wh.data,   (void *)p, sizeof(wh.data) );

    info.format     = wh.format.formatTag;
    info.rate       = wh.format.samplesPerSec; 
    info.width      = wh.format.bitsPerSample / 8;
    info.channels   = wh.format.channels;
    info.samples    = wh.data.len / wh.format.blockAlign;
    info.dataofs    = p - data + 8;
    
    return info;
}

static void S_ChannelFree(channel_t *v) {
	v->thesfx = NULL;
	*(channel_t **)v = freelist;
	freelist = (channel_t*)v;
}

static channel_t* S_ChannelMalloc( void ) {
	channel_t *v;
	if (freelist == NULL) {
		return NULL;
	}
	v = freelist;
	freelist = *(channel_t **)freelist;
	v->allocTime = Com_Millisecond ();
	return v;
}

static void S_ChannelSetup( void ) {
	channel_t *p, *q;

	C_memset( s_channels, 0, sizeof( s_channels ) );

	p = s_channels;
	q = p + MAX_CHANNELS;
	while (--q > p) {
		*(channel_t **)q = q-1;
	}
	
	*(channel_t **)q = NULL;
	freelist = p + MAX_CHANNELS - 1;
	Com_Printf("Channel memory manager cleared & reset\n");
}


void S_InitRegistration( void ) {
	s_soundMuted = gfalse;		// we can play again

	if (s_numSfx == 0) {
		SND_setup();

		C_memset( s_knownSfx, 0, sizeof( s_knownSfx ) );
		C_memset( sfxHash, 0, sizeof(sfx_t *) * LOOP_HASH );

//		S_RegisterSound( "zpak/sound/melst_b4.wav" );
	}
}

void S_Shutdown( void ) {
    S_StopAllSounds();
    s_soundStarted = 0;

    // free sfx buffers
    SND_shutdown();
    Com_Printf( "freeing sound buffers\n" );

    // stop direct sound
    Sys_Snd_Shutdown();
}


// re-samples the raw samples AND moves them into sndBuffers at the same time
static void S_ResampleSfx( sfx_t *sfx, int inrate, int inwidth, byte *data ) {
	int		outcount;
	int		srcsample;
	float	stepscale;
	int		i;
	int		sample, samplefrac, fracstep;
	int			part;
	sndBuffer	*chunk;
	

    // infile of 44100 : 2
    // infile of 22050 : 1
    // infile of 11025 : 0.5
	stepscale = (float)inrate / dma.speed;	// this is usually 0.5, 1, or 2

    // output frames
	outcount = sfx->soundLength / stepscale;
	sfx->soundLength = outcount;

	samplefrac = 0;

    // 128, 256, 512
	fracstep = stepscale * 256;

    // null when loading new sfx
	chunk = sfx->soundData;

	for ( i = 0; i < outcount; i++ )
	{

		srcsample = samplefrac >> 8;
		samplefrac += fracstep;

		if ( inwidth == 2 ) {
            // short to short
			sample = ( ((short *)data)[srcsample] );
		} else {
            // pretty much a width of 1
            // subtracting 128 yanks the top bit, this is done because 8-bit 
            // pcm is unsigned.  samples are 0-255, whereas 16-bit is signed,
            // 2's compliment format.
            // then shift left 8, has half
            // the resolution of a 16-bit sample because the low byte is
            // always zero
			sample = (int)( (unsigned char)(data[srcsample]) - 128) << 8;
		}

        // every time it rolls over get a new chunk
		part = (i&(SND_CHUNK_SIZE-1));
		if ( part == 0 ) {
			sndBuffer *newchunk;
			newchunk = SND_malloc();
			if (chunk == NULL) {
				sfx->soundData = newchunk;
			} else {
				chunk->next = newchunk;
			}
			chunk = newchunk;
		}

		chunk->sndChunk[part] = sample;
	}
}


int S_LoadSound( sfx_t *sfx ) { 
    byte *  data;
    int     size;
    wavinfo_t   info;

    size = FS_SlurpFile( sfx->soundName, (void **)&data );
    if ( !data ) {
        return 0;
    }

    info = S_GetWavInfo ( data, size );

    // mono only
    if ( info.channels != SFX_DEF_CHAN ) {
        Com_Printf ( "%s is not mono!\n", sfx->soundName );
        V_Free( data );
        return 0;
    }

    if ( info.width == 1 ) {
        Com_Printf( "WARNING: %s is a 8 bit file\n", sfx->soundName ) ;
    }

    if ( info.rate != SFX_DEF_FREQ ) {
        Com_Printf( "warning: %s has freq: %d. system rate is: %d\n", sfx->soundName, info.rate, SFX_DEF_FREQ );
    }

    //samples = V_Malloc ( info.sample * sizeof(short) * 2 );
    sfx->lastTimeUsed = Com_Millisecond() + 1;

    // **removed pcm compression support
    
    sfx->soundCompressionMethod = 0;
    sfx->soundLength = info.samples;
    sfx->soundData = NULL;

    S_ResampleSfx( sfx, info.rate, info.width, data + info.dataofs );

    //V_Free( samples );
    V_Free( data );

    return 1;
}

static long S_HashSFXName( const char *name ) {
	int		i;
	long	hash;
	char	letter;

	hash = 0;
	i = 0;
	while (name[i] != '\0') {
		letter = tolower(name[i]);
		if ( letter == '.' ) break;				// don't include extension
		if ( letter == '\\' ) letter = '/';		// damn path names
		hash += (long) ( letter )*( i + 119 );
		i++;
	}
	hash &= (LOOP_HASH-1);
	return hash;
}


sfx_t * S_FindName( const char *name ) {
    int     i;
    int     hash;
    sfx_t   *sfx;

    if ( !name ) {
        Com_Error( ERR_FATAL, "S_FindName: NULL\n" );
    }

    if ( !name[0] ) {
        Com_Error( ERR_FATAL, "S_FindName: empty name\n" );
    }

    if ( strlen( name ) >= MAX_PATH_LEN ) {
        Com_Error( ERR_FATAL, "sound name too long: %s\n", name );
    }

    hash = S_HashSFXName( name ); 

    sfx = sfxHash[hash];

    // see if already loaded
    while ( sfx ) {
        if ( !C_strncasecmp( sfx->soundName, name, MAX_PATH_LEN ) ) {
            return sfx;
        }
        sfx = sfx->next;
    }

    // find a free sfx
    for ( i = 0; i < s_numSfx; i++ ) {
        if ( !s_knownSfx[i].soundName[0] ) {
            break;
        }
    }

    // there isn't a free one, put another on the end
    if ( i == s_numSfx ) {
        if ( s_numSfx == MAX_SFX ) {
            Com_Error ( ERR_FATAL, "S_FindName: out of sfx_t\n" );
        }
        s_numSfx++;
    }

    sfx = &s_knownSfx[i];
    C_memset( sfx, 0, sizeof(*sfx) );
    strcpy ( sfx->soundName, name );

    sfx->next = sfxHash[hash];
    sfxHash[hash] = sfx;

    return sfx;
}

void S_MemoryLoad( sfx_t *sfx ) { 
	// load the sound file
	if ( !S_LoadSound ( sfx ) ) {
		sfx->defaultSound = gtrue;
	}
	sfx->inMemory = gtrue;
}

int S_RegisterSound( const char *lpath ) {
    sfx_t *sfx;

    if (!s_soundStarted) {
        return 0;
    }

    if ( strlen( lpath ) >= MAX_PATH_LEN ) {
        Com_Printf( "soundfile path [%s] exceeds max path length\n", lpath );
        return 0;
    }

    sfx = S_FindName ( lpath ) ;

    if ( !sfx ) {
        Com_Printf( "error loading sound: %s\n", lpath );
        return 0;
    }

    // already loaded into mem
    if ( sfx->soundData ) {
        if ( sfx->defaultSound ) {
            Com_Printf( "warning: could not find %s - using default\n", sfx->soundName );
            return 0;
        }
        return sfx - s_knownSfx;
    }

    sfx->inMemory = gfalse;

    S_MemoryLoad( sfx );

    if ( sfx->defaultSound ) {
        Com_Printf( "warning: could not find %s - using default\n", sfx->soundName );
        return 0;
    }

    return sfx - s_knownSfx;
}


void S_ClearSoundBuffer ( void ) {
	int		clear;
		
	if ( !s_soundStarted )
		return;

	// stop looping sounds
//	C_memset(loopSounds, 0, MAX_GENTITIES*sizeof(loopSound_t));
//	C_memset(loop_channels, 0, MAX_CHANNELS*sizeof(channel_t));

//	numLoopChannels = 0;

    S_ChannelSetup();

    s_rawend = 0;

	if (dma.samplebits == 8)
		clear = 0x80;
	else
		clear = 0;

    Sys_Snd_BeginPainting();
	if (dma.buffer) {
		C_memset( dma.buffer, clear, dma.samples * dma.samplebits / 8 );
    }
    Sys_Snd_Submit();
}


void S_StopAllSounds(void) {
	if ( !s_soundStarted ) {
		return;
	}

	// stop the background music
	S_StopBackGroundTrack();

	S_ClearSoundBuffer ();
}


#if 0
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
void S_StopLoopingSound( int entityNum ) {
	loopSounds[entityNum].active = gfalse;
//	loopSounds[entityNum].sfx = 0;
	loopSounds[entityNum].kill = gfalse;
}

/*
==================
S_ClearLoopingSounds

==================
*/
void S_ClearLoopingSounds( qboolean killall ) {
	int i;
	for ( i = 0 ; i < MAX_GENTITIES ; i++) {
		if (killall || loopSounds[i].kill == gtrue || (loopSounds[i].sfx && loopSounds[i].sfx->soundLength == 0)) {
			loopSounds[i].kill = gfalse;
			S_StopLoopingSound(i);
		}
	}
	numLoopChannels = 0;
}

/*
==================
S_AddLoopingSound

Called during entity generation for a frame
Include velocity in case I get around to doing doppler...
==================
*/
void S_AddLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfxHandle ) {
	sfx_t *sfx;

	if ( !s_soundStarted || s_soundMuted ) {
		return;
	}

	if ( sfxHandle < 0 || sfxHandle >= s_numSfx ) {
		Com_Printf( S_COLOR_YELLOW, "S_AddLoopingSound: handle %i out of range\n", sfxHandle );
		return;
	}

	sfx = &s_knownSfx[ sfxHandle ];

	if (sfx->inMemory == gfalse) {
		S_memoryLoad(sfx);
	}

	if ( !sfx->soundLength ) {
		Com_Error( ERR_DROP, "%s has length 0", sfx->soundName );
	}

	VectorCopy( origin, loopSounds[entityNum].origin );
	VectorCopy( velocity, loopSounds[entityNum].velocity );
	loopSounds[entityNum].active = gtrue;
	loopSounds[entityNum].kill = gtrue;
	loopSounds[entityNum].doppler = gfalse;
	loopSounds[entityNum].oldDopplerScale = 1.0;
	loopSounds[entityNum].dopplerScale = 1.0;
	loopSounds[entityNum].sfx = sfx;

	if (s_doppler->integer && VectorLengthSquared(velocity)>0.0) {
		vec3_t	out;
		float	lena, lenb;

		loopSounds[entityNum].doppler = gtrue;
		lena = DistanceSquared(loopSounds[listener_number].origin, loopSounds[entityNum].origin);
		VectorAdd(loopSounds[entityNum].origin, loopSounds[entityNum].velocity, out);
		lenb = DistanceSquared(loopSounds[listener_number].origin, out);
		if ((loopSounds[entityNum].framenum+1) != cls.framecount) {
			loopSounds[entityNum].oldDopplerScale = 1.0;
		} else {
			loopSounds[entityNum].oldDopplerScale = loopSounds[entityNum].dopplerScale;
		}
		loopSounds[entityNum].dopplerScale = lenb/(lena*100);
		if (loopSounds[entityNum].dopplerScale<=1.0) {
			loopSounds[entityNum].doppler = gfalse;			// don't bother doing the math
		}
	}

	loopSounds[entityNum].framenum = cls.framecount;
}

/*
==================
S_AddLoopingSound

Called during entity generation for a frame
Include velocity in case I get around to doing doppler...
==================
*/
void S_AddRealLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfxHandle ) {
	sfx_t *sfx;

	if ( !s_soundStarted || s_soundMuted ) {
		return;
	}

	if ( sfxHandle < 0 || sfxHandle >= s_numSfx ) {
		Com_Printf( S_COLOR_YELLOW, "S_AddRealLoopingSound: handle %i out of range\n", sfxHandle );
		return;
	}

	sfx = &s_knownSfx[ sfxHandle ];

	if (sfx->inMemory == gfalse) {
		S_memoryLoad(sfx);
	}

	if ( !sfx->soundLength ) {
		Com_Error( ERR_DROP, "%s has length 0", sfx->soundName );
	}
	VectorCopy( origin, loopSounds[entityNum].origin );
	VectorCopy( velocity, loopSounds[entityNum].velocity );
	loopSounds[entityNum].sfx = sfx;
	loopSounds[entityNum].active = gtrue;
	loopSounds[entityNum].kill = gfalse;
	loopSounds[entityNum].doppler = gfalse;
}



/*
==================
S_AddLoopSounds

Spatialize all of the looping sounds.
All sounds are on the same cycle, so any duplicates can just
sum up the channel multipliers.
==================
*/
void S_AddLoopSounds (void) {
	int			i, j, time;
	int			left_total, right_total, left, right;
	channel_t	*ch;
	loopSound_t	*loop, *loop2;
	static int	loopFrame;


	numLoopChannels = 0;

	time = Com_Milliseconds();

	loopFrame++;
	for ( i = 0 ; i < MAX_GENTITIES ; i++) {
		loop = &loopSounds[i];
		if ( !loop->active || loop->mergeFrame == loopFrame ) {
			continue;	// already merged into an earlier sound
		}

		if (loop->kill) {
			S_SpatializeOrigin( loop->origin, 127, &left_total, &right_total);			// 3d
		} else {
			S_SpatializeOrigin( loop->origin, 90,  &left_total, &right_total);			// sphere
		}

		loop->sfx->lastTimeUsed = time;

		for (j=(i+1); j< MAX_GENTITIES ; j++) {
			loop2 = &loopSounds[j];
			if ( !loop2->active || loop2->doppler || loop2->sfx != loop->sfx) {
				continue;
			}
			loop2->mergeFrame = loopFrame;

			if (loop2->kill) {
				S_SpatializeOrigin( loop2->origin, 127, &left, &right);				// 3d
			} else {
				S_SpatializeOrigin( loop2->origin, 90,  &left, &right);				// sphere
			}

			loop2->sfx->lastTimeUsed = time;
			left_total += left;
			right_total += right;
		}
		if (left_total == 0 && right_total == 0) {
			continue;		// not audible
		}

		// allocate a channel
		ch = &loop_channels[numLoopChannels];
		
		if (left_total > 255) {
			left_total = 255;
		}
		if (right_total > 255) {
			right_total = 255;
		}
		
		ch->master_vol = 127;
		ch->leftvol = left_total;
		ch->rightvol = right_total;
		ch->thesfx = loop->sfx;
		ch->doppler = loop->doppler;
		ch->dopplerScale = loop->dopplerScale;
		ch->oldDopplerScale = loop->oldDopplerScale;
		numLoopChannels++;
		if (numLoopChannels == MAX_CHANNELS) {
			return;
		}
	}
}
/////////////////////////////////////////////////////////////////////////////
#endif




void S_PrintSoundInfo( void ) {
	Com_Printf("----- Sound Info -----\n" );
	if (!s_soundStarted) {
		Com_Printf ("sound system not started\n");
	} else {
		if ( s_soundMuted ) {
			Com_Printf ("sound system is muted\n");
		}

		Com_Printf("%5d stereo\n", dma.channels - 1);
		Com_Printf("%5d samples\n", dma.samples);
		Com_Printf("%5d samplebits\n", dma.samplebits);
		Com_Printf("%5d submission_chunk\n", dma.submission_chunk);
		Com_Printf("%5d speed\n", dma.speed);
		Com_Printf("0x%x dma buffer\n", dma.buffer);
		if ( s_backgroundFile >= 0 ) {
			Com_Printf("Background file: %s\n", s_backgroundFileName );
		} else {
			Com_Printf("No background file.\n" );
		}

	}
	Com_Printf("----------------------\n" );
}


void S_Init( void ) {
    gbool res = gfalse;

    s_mixahead      = Gvar_Get( "s_mixahead", "0.2", 0 );
    s_mixPreStep    = Gvar_Get( "s_mixPreStep", "0.05", 0 );
//    s_volume        = Gvar_Get( "s_volume", "0.8", 0 );
//	s_musicVolume	= Gvar_Get( "s_musicVolume", "0.55", 0 );

    res = Sys_Snd_Init();

    if ( res ) 
    {
		++s_soundStarted;
		s_soundMuted = gtrue;

        S_StopAllSounds();

        S_PrintSoundInfo();
    }
}

// - clear any sound effects that end before the current time, 
// - start any new sounds
// - return number of new sounds starting this frame
int S_ScanChannelStarts( void ) {
	channel_t		*ch;
	int				i;
	int		        newSamples = 0;

	ch = s_channels;
	for ( i = 0; i < MAX_CHANNELS ; i++, ch++ ) {
		if ( !ch->thesfx ) {
			continue;
		}
		// if this channel was just started this frame,
		// set the sample count to it begins mixing
		// into the very first sample
		if ( ch->startSample == START_SAMPLE_IMMEDIATE ) {
			ch->startSample = s_paintedtime;
			++newSamples;
			continue;
		}

		// if it is completely finished by now, clear it
		if ( ch->startSample + (ch->thesfx->soundLength) <= s_paintedtime ) {
			S_ChannelFree( ch );
		}
	}

	return newSamples;
}

void S_GetSoundTime( void )
{
	int		        samplepos;
	static	int		buffers = 0;
	static	int		oldsamplepos = 0;
	int		        fullsamples;
	
    // full samples take into account bitwidth as well as channels,
    // so 1 fullsample of 16-bit stereo is 4-bytes, and fullsamples
    // is the number of fullsamples in the whole dma buffer
	fullsamples = dma.samples / dma.channels;

	// it is possible to miscount buffers if it has wrapped twice between
	// calls to S_Update.  Oh well.
	samplepos = Sys_Snd_GetSamplePos();
	if (samplepos < oldsamplepos)
	{
		buffers++;					// buffer wrapped
		
		if ( s_paintedtime > 0x40000000 )
		{	// time to chop things off to avoid 32 bit limits
			buffers = 0;
			s_paintedtime = fullsamples;
			S_StopAllSounds ();
		}
	}
	oldsamplepos = samplepos;

	s_soundtime = buffers * fullsamples + samplepos / dma.channels;

	if ( dma.submission_chunk < 256 ) {
		s_paintedtime = s_soundtime + s_mixPreStep->value() * dma.speed;
	} else {
		s_paintedtime = s_soundtime + dma.submission_chunk;
	}

}


short	*snd_out;
int		snd_linear_count;
int		*snd_p;

/* from MSDN:
For functions declared with the naked attribute, the compiler generates code without prolog and epilog code. You can use this feature to write your own prolog/epilog code sequences using inline assembler code. Naked functions are particularly useful in writing virtual device drivers. Note that the naked attribute is only valid on x86, and is not available on x64 or Itanium.
*/
__declspec( naked ) void S_WriteLinearBlastStereo16 ( void )
{
	__asm {

 push edi
 push ebx
 mov ecx,ds:dword ptr[snd_linear_count]
 mov ebx,ds:dword ptr[snd_p]
 mov edi,ds:dword ptr[snd_out]
LWLBLoopTop:
 mov eax,ds:dword ptr[-8+ebx+ecx*4]
 sar eax,8
 cmp eax,07FFFh
 jg LClampHigh
 cmp eax,0FFFF8000h
 jnl LClampDone
 mov eax,0FFFF8000h
 jmp LClampDone
LClampHigh:
 mov eax,07FFFh
LClampDone:
 mov edx,ds:dword ptr[-4+ebx+ecx*4]
 sar edx,8
 cmp edx,07FFFh
 jg LClampHigh2
 cmp edx,0FFFF8000h
 jnl LClampDone2
 mov edx,0FFFF8000h
 jmp LClampDone2
LClampHigh2:
 mov edx,07FFFh
LClampDone2:
 shl edx,16
 and eax,0FFFFh
 or edx,eax
 mov ds:dword ptr[-4+edi+ecx*2],edx
 sub ecx,2
 jnz LWLBLoopTop
 pop ebx
 pop edi
 ret
	}
}

void S_TransferStereo16 (unsigned long *pbuf, int endtime)
{
	int		lpos;
	int		ls_paintedtime;

	
	snd_p = (int *) paintbuffer;
	ls_paintedtime = s_paintedtime;

	while (ls_paintedtime < endtime)
	{
	// handle recirculating buffer issues
		lpos = ls_paintedtime & ((dma.samples>>1)-1);

		snd_out = (short *) pbuf + (lpos<<1);

		snd_linear_count = (dma.samples>>1) - lpos;
		if (ls_paintedtime + snd_linear_count > endtime)
			snd_linear_count = endtime - ls_paintedtime;

		snd_linear_count <<= 1;

	    // write a linear blast of samples
		S_WriteLinearBlastStereo16 ();

		snd_p += snd_linear_count;
		ls_paintedtime += (snd_linear_count>>1);
	}
}


/*
===================
S_TransferPaintBuffer

- hand the mixed samples to the sound interface
===================
*/
void S_TransferPaintBuffer( int endtime )
{
	int 	out_idx;
	int 	count;
	int 	out_mask;
	int 	*p;
	int 	step;
	int		val;
	unsigned long *pbuf;

	pbuf = (unsigned long *)dma.buffer;


    
	if ( 0 ) {
		int		i;
		int		count;

		// write a fixed sine wave
		count = ( endtime - s_paintedtime );
		for (i=0 ; i<count ; i++)
			paintbuffer[i].left = paintbuffer[i].right = sin((s_paintedtime+i)*0.1)*20000*256;
	}


	if (dma.samplebits == 16 && dma.channels == 2)
	{	// optimized case
		S_TransferStereo16 (pbuf, endtime);
	}
	else
	{	// general case
		p = (int *) paintbuffer;
		count = (endtime - s_paintedtime) * dma.channels;
		out_mask = dma.samples - 1; 
		out_idx = s_paintedtime * dma.channels & out_mask;
		step = 3 - dma.channels;

		if (dma.samplebits == 16)
		{
			short *out = (short *) pbuf;
			while (count--)
			{
				val = *p >> 8;
				p+= step;
				if (val > 0x7fff)
					val = 0x7fff;
				else if (val < -32768)
					val = -32768;
				out[out_idx] = val;
				out_idx = (out_idx + 1) & out_mask;
			}
		}
		else if (dma.samplebits == 8)
		{
			unsigned char *out = (unsigned char *) pbuf;
			while (count--)
			{
				val = *p >> 8;
				p+= step;
				if (val > 0x7fff)
					val = 0x7fff;
				else if (val < -32768)
					val = -32768;
				out[out_idx] = (val>>8) + 128;
				out_idx = (out_idx + 1) & out_mask;
			}
		}
	}
}


/*
==================== 
 S_PaintChannelFrom16
==================== 
*/ 
static void S_PaintChannelFrom16( channel_t *ch, sfx_t *sc, int count, int sampleOffset, int bufferOffset ) {
	int						data, aoff, boff;
	int						leftvol, rightvol;
	int						i, j;
	portable_samplepair_t	*samp;
	sndBuffer				*chunk;
	short					*samples;
	float					ooff, fdata, fdiv, fleftvol, frightvol;


	samp = &paintbuffer[ bufferOffset ];

	chunk = sc->soundData;
	while ( sampleOffset >= SND_CHUNK_SIZE ) {
		chunk = chunk->next;
		sampleOffset -= SND_CHUNK_SIZE;
		if ( !chunk ) {
            if ( sc->looping ) {
                if ( sc->loopcount != 0 ) {
                    if ( --sc->loopcount <= 0 ) {
                        sc->looping = gfalse;
                    }
                }
            }
            if ( sc->looping ) {
                chunk = sc->soundData;
            } else {
                return;
            }
		}
	}
 
	leftvol = ch->leftvol*snd_vol;
	rightvol = ch->rightvol*snd_vol;
	samples = chunk->sndChunk;

	for ( i = 0 ; i < count ; i++ ) {
		data  = samples[sampleOffset++];
		samp[i].left += (data * leftvol) >> 8;
		samp[i].right += (data * rightvol) >> 8;

		if ( sampleOffset == SND_CHUNK_SIZE ) {
			chunk = chunk->next;
            if ( !chunk ) {
                if ( sc->looping ) {
                    if ( sc->loopcount != 0 ) {
                        if ( --sc->loopcount <= 0 ) {
                            sc->looping = gfalse;
                        }
                    }
                }
                if ( sc->looping ) {
                    chunk = sc->soundData;
                } else {
                    return;
                }
            } 
			samples = chunk->sndChunk;
			sampleOffset = 0;
		}
	}
}


/*
===================
 S_PaintChannels
===================
*/
    // s_paintedtime must be what has been painted so far
    // endtime is the millisecond limit of the last sample
void S_PaintChannels( int endtime ) {
	int 	i;
	int 	end;
	channel_t *ch;
	sfx_t	*sc;
	int		ltime, count;
	int		sampleOffset;


	//snd_vol = s_volume->value*255;
	//snd_vol = s_volume * 255;
	snd_vol = s_volume->value() * 255;


    // paint up to the allowed endtime
	while ( s_paintedtime < endtime ) 
    {
		// if paintbuffer is smaller than DMA buffer
		// we may need to fill it multiple times
		end = endtime;
		if ( endtime - s_paintedtime > PAINTBUFFER_SIZE ) {
			end = s_paintedtime + PAINTBUFFER_SIZE;
		}

		// clear the paint buffer to either music or zeros
		if ( s_rawend < s_paintedtime ) {
			if ( s_rawend ) {
				Com_Printf ("background sound underrun\n");
			}
			C_memset(paintbuffer, 0, (end - s_paintedtime) * sizeof(portable_samplepair_t));
		} else {
			// copy from the streaming sound source
			int		s;
			int		stop;

			stop = (end < s_rawend) ? end : s_rawend;

			for ( i = s_paintedtime ; i < stop ; i++ ) {
				s = i & MAX_RAW_SAMPLES_MASK;
				paintbuffer[i-s_paintedtime] = s_rawsamples[s];
			}

			for ( ; i < end ; i++ ) {
				paintbuffer[i-s_paintedtime].left =
				paintbuffer[i-s_paintedtime].right = 0;
			}
		}

		// paint in the channels.
		ch = s_channels;
		for ( i = 0; i < MAX_CHANNELS ; i++, ch++ ) {		
			if ( !ch->thesfx || (ch->leftvol<0.25 && ch->rightvol<0.25 )) {
				continue;
			}

			ltime = s_paintedtime;
			sc = ch->thesfx;

			sampleOffset = ltime - ch->startSample;
			count = end - ltime;
			if ( sampleOffset + count > sc->soundLength ) {
				count = sc->soundLength - sampleOffset;
			}

			if ( count > 0 ) {	
                S_PaintChannelFrom16 (ch, sc, count, sampleOffset, ltime - s_paintedtime);
			}
		}

		// transfer out according to DMA format
		S_TransferPaintBuffer( end );
		s_paintedtime = end;
	}
}


/*
==================== 
 S_Update_real
====================
*/
static void S_Update_real( void ) 
{
	unsigned        endtime;
	int				samps;
	static			float	lastTime = 0.0f;
	float			ma, op;
	float			thisTime, sane;
	static			int ot = -1;

	if ( !s_soundStarted || s_soundMuted ) {
		return;
	}

	thisTime = Com_Millisecond();

	// Updates s_soundtime
	S_GetSoundTime();

	if (s_soundtime == ot) {
		return;
	}
	ot = s_soundtime;

	// clear any sound effects that end before the current time,
	// and start any new sounds
	S_ScanChannelStarts();

	sane = thisTime - lastTime;
	if ( sane < 11 ) {
		sane = 11;			// 85hz
	}

	ma = s_mixahead->value() * dma.speed;
	op = s_mixPreStep->value() + sane*dma.speed*0.01;

	if (op < ma) {
		ma = op;
	}

	// mix ahead of current position
	endtime = s_soundtime + ma;

	// mix to an even submission block size
	endtime = (endtime + dma.submission_chunk-1)
		& ~(dma.submission_chunk-1);

	// never mix more than the complete buffer
	samps = dma.samples >> (dma.channels-1);
	if (endtime - s_soundtime > samps)
		endtime = s_soundtime + samps;

	Sys_Snd_BeginPainting ();

	S_PaintChannels ( endtime );

	Sys_Snd_Submit ();

	lastTime = thisTime;

}

void S_Update( void ) 
{
    if ( !s_soundStarted || s_soundMuted )
        return;

    // pause only effects music
    if ( !s_paused ) {
        S_UpdateBackGroundTrack();
    }

    S_Update_real();
}

void S_UnMute( void ) {
    s_soundMuted = gfalse;
}
void S_Mute( void ) {
	S_ClearSoundBuffer();
    s_soundMuted = gtrue;
}

/*
====================
S_StartSound

Volume, Distance, Falloff, channel, sfx now has 2 looping variables.
you can set for non-looping, a numbered amount of loops, or 0 for inf loops

Validates the parms and ques the sound up

====================
*/
gbool S_StartSound( vec3_t origin, soundDynamic_t *dyn, sfxHandle_t sfxHandle ) {
    channel_t	*ch;
    sfx_t		*sfx;
    int i, oldest, chosen, time;
    int inplay, allowed;

	if ( !s_soundStarted || s_soundMuted ) {
		return gfalse;
	}

	if ( sfxHandle < 0 || sfxHandle >= s_numSfx ) {
		Com_Printf( "S_StartSound: handle %i out of range\n", sfxHandle );
		return gfalse;
	}

	sfx = &s_knownSfx[ sfxHandle ];

	if ( sfx->inMemory == gfalse ) {
		S_MemoryLoad( sfx );
	}


	time = Com_Millisecond();


    
    /*
	ch = s_channels;
	inplay = 0;
	for ( i = 0; i < MAX_CHANNELS ; i++, ch++ ) {		

		if (ch[i].entnum == entityNum && ch[i].thesfx == sfx) {
			if (time - ch[i].allocTime < 50) {
				return;
			}
			inplay++;
		}
	}

	if (inplay > allowed) {
		return;
	}
    */

	sfx->lastTimeUsed = time;

	ch = S_ChannelMalloc();	

    if ( !ch )
        return gfalse;

    // All channels in use, reasons to free an old channel would be, 
    //  CHAN_ANNOUNCER, CHAN_GLOBAL or just to keep things cycling through
    //
    /* fuck all channels full for right now.  I'm going to have 128,
     * if they're all full then, fuckit, I shouldn't ever have 128 sounds
     * all playing at the same time anyway.
     *
     * this assumes of course that somewhere in the update code that channels
     * that have ended playing are freed
     *
	if ( !ch ) 
    {
		ch = s_channels;

		oldest = sfx->lastTimeUsed;

		chosen = -1;
		for ( i = 0 ; i < MAX_CHANNELS ; i++, ch++ ) {
			if ( ch->allocTime < oldest && ch->entchannel != CHAN_ANNOUNCER ) {
				oldest = ch->allocTime;
				chosen = i;
			}
		}

		if ( chosen == -1 ) {
			ch = s_channels;
			for ( i = 0 ; i < MAX_CHANNELS ; i++, ch++ ) {
				if ( ch->allocTime < oldest && ch->entchannel != CHAN_ANNOUNCER) {
					oldest = ch->allocTime;
					chosen = i;
				}
			}
			if (chosen == -1) {
            	for ( i = 0 ; i < MAX_CHANNELS ; i++, ch++ ) {
		            if (ch->allocTime<oldest) {
						oldest = ch->allocTime;
						chosen = i;
					}
				}

				if (chosen == -1) {
					Com_Printf("dropping sound\n");
					return;
				}
			}
		}
		ch = &s_channels[chosen];
		ch->allocTime = sfx->lastTimeUsed;
	}
    */

	if ( origin ) {
		VectorCopy ( origin, ch->origin );
		ch->has_origin = gtrue;
	} else {
		ch->has_origin = gfalse;
	}

	ch->master_vol = MASTER_VOLUME_DEFAULT;
	ch->entnum = 0;
	ch->thesfx = sfx;
	ch->startSample = START_SAMPLE_IMMEDIATE;
	ch->entchannel = dyn->channel;
	ch->leftvol = ch->master_vol;		// these will get calced at next spatialize
	ch->rightvol = ch->master_vol;		// unless the game isn't running
	ch->doppler = gfalse;

    ch->dynamic.level = dyn->level;
    ch->dynamic.channel = dyn->channel;
    ch->dynamic.distance = dyn->distance;
    ch->dynamic.falloff = dyn->falloff;
    ch->dynamic.moving = dyn->moving;

    return gtrue;
}


/*
==================
S_StartLocalSound
==================
*/
void S_StartLocalSound( sfxHandle_t sfxHandle ) {
    static soundDynamic_t dyn;

	if ( !s_soundStarted || s_soundMuted ) {
		return;
	}

	if ( sfxHandle < 0 || sfxHandle >= s_numSfx ) {
		Com_Printf( "S_StartLocalSound: handle %i out of range\n", sfxHandle );
		return;
	}

    dyn.level     = 127;
    dyn.channel   = CHAN_LOCAL;
    dyn.distance  = 100000000;
    dyn.falloff   = 100000000;
    dyn.moving    = gfalse;

	S_StartSound ( NULL, &dyn, sfxHandle );
}


/*
============
S_TransferRaw

Music streaming : fill s_rawsamples
============
*/
void S_TransferRaw( int samples, int rate, int width, int s_channels, const byte *data, float volume ) {
	int		i;
	int		src, dst;
	float	scale;
	int		intVolume;

	if ( !s_soundStarted || s_soundMuted ) {
		return;
	}

	intVolume = 256 * volume;       // 0 - 256

	if ( s_rawend < s_soundtime ) {
		Com_Printf( "S_TransferRaw: resetting minimum: %i < %i\n", s_rawend, s_soundtime );
		s_rawend = s_soundtime;
	}

	scale = (float)rate / dma.speed;

    // 16-bit stereo
	if (s_channels == 2 && width == 2)
	{
		if (scale == 1.0)
		{	// optimized case
			for (i = 0; i < samples; i++)
			{
				dst = s_rawend & MAX_RAW_SAMPLES_MASK;
				s_rawend++;
				s_rawsamples[dst].left = ((short *)data)[i*2] * intVolume;
				s_rawsamples[dst].right = ((short *)data)[i*2+1] * intVolume;
			}
		}
		else
		{
			for (i=0 ; ; i++)
			{
				src = i * scale;
				if (src >= samples)
					break;
				dst = s_rawend & MAX_RAW_SAMPLES_MASK;
				s_rawend++;
				s_rawsamples[dst].left = ((short *)data)[src*2] * intVolume;
				s_rawsamples[dst].right = ((short *)data)[src*2+1] * intVolume;
			}
		}
	}
    // 16-bit mono
	else if (s_channels == 1 && width == 2)
	{
		for (i=0 ; ; i++)
		{
			src = i*scale;
			if (src >= samples)
				break;
			dst = s_rawend & MAX_RAW_SAMPLES_MASK;
			s_rawend++;
			s_rawsamples[dst].left = ((short *)data)[src] * intVolume;
			s_rawsamples[dst].right = ((short *)data)[src] * intVolume;
		}
	}
    // 8-bit stereo
	else if (s_channels == 2 && width == 1)
	{
		intVolume *= 256;

		for (i=0 ; ; i++)
		{
			src = i*scale;
			if (src >= samples)
				break;
			dst = s_rawend & MAX_RAW_SAMPLES_MASK;
			s_rawend++;
			s_rawsamples[dst].left = ((char *)data)[src*2] * intVolume;
			s_rawsamples[dst].right = ((char *)data)[src*2+1] * intVolume;
		}
	}
    // 8-bit mono
	else if (s_channels == 1 && width == 1)
	{
		intVolume *= 256;

		for (i=0 ; ; i++)
		{
			src = i*scale;
			if (src >= samples)
				break;
			dst = s_rawend & MAX_RAW_SAMPLES_MASK;
			s_rawend++;
			s_rawsamples[dst].left = (((byte *)data)[src]-128) * intVolume;
			s_rawsamples[dst].right = (((byte *)data)[src]-128) * intVolume;
		}
	}

	if ( s_rawend > s_soundtime + MAX_RAW_SAMPLES ) {
		Com_Printf( "S_TransferRaw: overflowed %i > %i\n", s_rawend, s_soundtime );
	}
}

/*
=================
S_ByteSwapRawSamples

If raw data has been loaded in little endien binary form, this must be done.
If raw data was calculated, as with ADPCM, this should not be called.
=================
*/
/*
void S_ByteSwapRawSamples( int samples, int width, int s_channels, const byte *data ) {
	int		i;

	if ( width != 2 ) {
		return;
	}
	if ( LittleShort( 256 ) == 256 ) {
		return;
	}

	if ( s_channels == 2 ) {
		samples <<= 1;
	}
	for ( i = 0 ; i < samples ; i++ ) {
		((short *)data)[i] = LittleShort( ((short *)data)[i] );
	}
}
*/

void S_StopBackGroundTrack( void ) {
    if ( s_backgroundFile < 0 ) {
        return ;
    }
    if ( s_backgroundFileType == BGTYPE_WAV )
        FS_FClose( s_backgroundFile );
    s_backgroundFile = -1;
    s_backgroundFileName[0] = '\0';
    //s_rawend = 0;

    S_StopOggBackgroundTrack();
}

gbool S_UpdateBackGroundTrack( void ) {
    int         bufferSamples;
    int         fileSamples;
    byte        raw[30000];
    byte        *raw_p;
    int         fileBytes;
    int         r, s;

    if ( s_backgroundFile < 0 ) {
        return gfalse;
    }

    if ( s_musicVolume->value() <= 0 ) {
        return gfalse;
    }

    if ( s_rawend < s_soundtime ) {
        s_rawend = s_soundtime ;
    }

    // ogg-vorbis path
    if ( BGTYPE_OGG == s_backgroundFileType ) {
        while ( s_rawend < s_soundtime + MAX_RAW_SAMPLES ) {
            bufferSamples = MAX_RAW_SAMPLES - (s_rawend - s_soundtime);
            // this is the number of samples we need
            fileSamples = bufferSamples * s_backgroundInfo.rate / dma.speed;
            // not more than size of raw
            if ( fileSamples * 4 > sizeof(raw) ) 
                fileSamples = sizeof(raw) / 4;

            gbool ioLoop = s_backgroundLooping ;

            r = S_OggGetSamples( raw, fileSamples, &ioLoop );
            if ( ioLoop ) {
                if ( s_backgroundLoopCount > 0 ) {
                    if ( 0 == --s_backgroundLoopCount ) {
                        s_backgroundLooping = gfalse;
                    }
                }           
            }
			if ( r != fileSamples ) {
				// what? it will be unless it isn't looping
				if ( !s_backgroundLooping ) {
					S_StopBackGroundTrack();
				}
			}

            S_TransferRaw(  fileSamples, 
                            s_backgroundInfo.rate, 
                            s_backgroundInfo.width, 
                            s_backgroundInfo.channels, 
                            raw, 
                            s_musicVolume->value() );
        }
        return gtrue;
    }


    while ( s_rawend < s_soundtime + MAX_RAW_SAMPLES ) {
        //bufferSamples = MAX_RAW_SAMPLES - (s_rawend - s_soundtime);
        bufferSamples = MAX_RAW_SAMPLES - s_rawend + s_soundtime;

		// decide how much data needs to be read from the file
		fileSamples = bufferSamples * s_backgroundInfo.rate / dma.speed;

		// don't try and read past the end of the file
		if ( fileSamples > s_backgroundInfo.samples ) {
			fileSamples = s_backgroundInfo.samples;
		}

		// our max buffer size
		fileBytes = fileSamples * (s_backgroundInfo.width * s_backgroundInfo.channels);

        // dont read more than our tmp stack buffer
		if ( fileBytes > sizeof(raw) ) {
			fileBytes = sizeof(raw);
			fileSamples = fileBytes / (s_backgroundInfo.width * s_backgroundInfo.channels);
		}

		r = FS_Read( raw, fileBytes, s_backgroundFile );

        // incomplete read
		if ( r != fileBytes ) {
            if ( s_backgroundLooping ) {
                // 0=loop forever, >0=count
                if ( s_backgroundLoopCount > 0 ) {
                    if ( 0 == --s_backgroundLoopCount ) {
                        s_backgroundLooping = gfalse;
                    }
                }
			}

			if ( s_backgroundLooping ) { 
				FS_Seek( s_backgroundFile, s_backgroundInfo.dataofs, FSEEK_SET );
				raw_p = raw + r;
				s = FS_Read( raw_p, fileBytes - r, s_backgroundFile );

				if ( s != fileBytes - r ) {
					Com_Printf( "WARNING: restart loop track, read %d, requested %d\n", s, fileBytes - r );
				}
			} else  {
				s_backgroundLoopCount = 0;
				s_backgroundLooping = gfalse;

				// re-compute samples 
				fileBytes = r;
				fileSamples = fileBytes / (s_backgroundInfo.width * s_backgroundInfo.channels);
				S_StopBackGroundTrack();
				break;
			}
		}

		// byte swap if needed, 
        // NOTE: Implement when getting to NON-i386 archetectures
		//S_ByteSwapRawSamples( fileSamples, s_backgroundInfo.width, s_backgroundInfo.channels, raw );

		// add raw to s_raw buffer
		S_TransferRaw(  fileSamples, 
                        s_backgroundInfo.rate, 
			            s_backgroundInfo.width, 
                        s_backgroundInfo.channels, 
                        raw, 
                        s_musicVolume->value() );
    }
    return gtrue;
}


//void S_StartBackGroundTrack( const char *name, filehandle_t *fh=NULL, gbool loop=gfalse, int c=0 ) {
void S_StartBackGroundTrack( const char *name, filehandle_t *fh, gbool loop, int c ) {

	// there is a background file already running
	if ( s_backgroundFile > -1 ) {
		// client requested same file twice, start it over from beginning
		if ( ! strcmp( name, s_backgroundFileName ) ) {
    		FS_Seek( s_backgroundFile, s_backgroundInfo.dataofs, FSEEK_SET );
			return;
		}

		// client requested new file, close old one first
		S_StopBackGroundTrack();
	}

    int len;
    byte scratch[128];

    // NAME
    C_strncpy ( s_backgroundFileName, name, sizeof(s_backgroundFileName) );
    
    // FILE TYPE
    const char *p = name;
    while ( *p ) ++p;
    if ( p[-4] == '.' && p[-3] == 'o' && p[-2] == 'g' && p[-1] == 'g' ) 
        s_backgroundFileType = BGTYPE_OGG;
    else if ( p[-4] == '.' && p[-3] == 'O' && p[-2] == 'G' && p[-1] == 'G' ) 
        s_backgroundFileType = BGTYPE_OGG;
    else
        s_backgroundFileType = BGTYPE_WAV;

    // OGG VORBIS pathway
    if ( s_backgroundFileType == BGTYPE_OGG ) 
    {
        S_BackgroundOggOpen( name );
	    s_backgroundLooping = loop;
	    s_backgroundLoopCount = c;
        s_backgroundInfo = S_GetOggInfo();
        /* need to set this even though we don't use it */
        s_backgroundFile = 7; 
        return;
    }
	
	if ( fh )
		*fh = -1;

	FS_FOpenReadOnly( name, &s_backgroundFile );
	
	if ( s_backgroundFile < 0 ) {
		Com_Printf( "WARNING: couldn't open music file %s\n", name );
		return;
	}
	
	if ( fh )
		*fh = s_backgroundFile;
	
	s_backgroundLooping = loop;
	s_backgroundLoopCount = c;

    FS_Read( scratch, sizeof(scratch), s_backgroundFile );
    s_backgroundInfo = S_GetWavInfo( scratch, sizeof(scratch) );

    FS_Seek( s_backgroundFile, s_backgroundInfo.dataofs, FSEEK_SET );
}

// for starting background tracks once the file has already been loaded, 
// or changing the background track looping attributes
void S_ReStartBackGroundTrack( filehandle_t fh, gbool loop, int count ) {
	s_backgroundFile = fh;
	s_backgroundLooping = loop;
	s_backgroundLoopCount = count;
}

/*
================================================================
 AudioManager_c 
================================================================
*/
void AudioManager_c::play( const char * tune ) {
    if ( !tune )
        return;

    // get a list of files in music directory
    if ( !songs_pp ) {
        genSongList();
    }
    
    // we don't require an extension, so:
    // determine if there is a file extension in the argument 'tune'
    int len = strlen( tune );
    char * c = (char*) strrchr( tune, '.' );
    bool arg_has_ext = false;
    if ( c && c - tune == len - 4 ) {
        arg_has_ext = true;
    }

    // if there is, we look for a file that is a perfect match

    // if there isn't, then for each file in music directory:
        // get a pointer to the file extension, copy the file string, clipping
        //  off the extra path and extension stuff, then try to match it
        
    // if one matches call S_StartBackGroundTrack & exit, otherwise print
    //  error and play nothing.
    
    int found = -1;
    for ( int i = 0; i < songs_len; i++ ) {
        if ( arg_has_ext ) {
            if ( !strcmp( tune, songs_pp[i] ) ) {
                found = i;
                break;
            }
        } else {
            c = strrchr( songs_pp[i], '.' );
            if ( c ) { 
                char s = *c;
                *c = '\0';
                if ( !strcmp( tune, songs_pp[i] ) ) {
                    found = i;
                }
                *c = s;
            }
            if ( found != -1 )
                break;
        }
    }

    s_paused = gfalse; // play implicitly unpauses

    current = NULL;
    if ( found > -1 && found < songs_len ) {
        char buf[512];
        sprintf( buf, "zpak/music/%s", songs_pp[found] );
		filehandle_t whocares;
	    S_StartBackGroundTrack( buf, &whocares, gtrue );
        current = s_backgroundFileName;
        console.Printf( "playing: %s", current );
        return;
    } 
    console.Printf( "song not found" );
}

void AudioManager_c::pause( void ) {
    s_paused = !s_paused;
    console.Printf( "music is %s", s_paused ? "paused" : "now unpaused" );
}

void AudioManager_c::stop( void ) {
    S_StopBackGroundTrack();
    current = NULL;
    console.Printf( "music stopped" );
}

void AudioManager_c::list( void ) {
    if ( !songs_pp ) 
        genSongList();
    if ( 0 == songs_len ) {
        console.Printf( "No Music Found" );
        return;
    }
    console.Printf( "Loaded Songs:" );
    for ( unsigned int i = 0; i < songs_len; i++ ) {
        console.Printf( "  %s", songs_pp[i] );
    }
}

// console commands
void AudioManager_c::command( const char * cmd, const char *arg ) {
    // print the song currently playing
    if ( !cmd ) {
        current = s_backgroundFileName;
		if ( s_backgroundFile > -1 )
            if ( s_paused ) 
	            console.Printf( "current song: %s, volume is %.2f, PAUSED", current, s_musicVolume->value() );
            else
	            console.Printf( "currently playing: %s, volume is %.2f", current, s_musicVolume->value() );
		else
			console.Printf( "no song playing" );
        return;
    }

    // play <filename>
    if ( !strcmp( cmd, "play" ) ) {
        if ( !arg ) {
            console.Printf( "please provide a filename to play" );
            return;
        }
        return play( arg );
    }
    // pause
    if ( !strcmp( cmd, "pause" ) ) {
        return pause();
    }
    // stop
    if ( !strcmp( cmd, "stop" ) ) {
        return stop();
    }
    // list
    if ( !strcmp( cmd, "list" ) ) {
        return list();
    }
    // help
    if ( !strcmp( cmd, "help" ) ) {
        console.Printf( "music       - prints what's currently playing" ); 
        console.Printf( "music play <song> - plays a song" ); 
        console.Printf( "music pause - pauses/unpauses music" ); 
        console.Printf( "music stop  - stops music" ); 
        console.Printf( "music list  - lists all available songs" ); 
        console.Printf( "music help  - prints this" );
        console.Printf( "music volume <value> - set music volume" );
        return;
    }
    // volume
    if ( !strcmp( cmd, "volume" ) || !strcmp( cmd, "vol" ) ) {
        if ( !arg ) {
            console.Printf( "music volume is %.2f. Provide a value to set it", s_musicVolume->value() );
            return;
        }
        float val = atof( arg );
        s_musicVolume->setValue( val );
        console.Printf( "music volume now set to %.2f", s_musicVolume->value() );
        return;
    }
    
    // ?
    console.Printf( "unknown command: %s", cmd );
}

// get a list of files in music directory
void AudioManager_c::genSongList( void ) {
    songs_pp = GetDirectoryList( "zpak/music", &songs_len );

    char copy[ 1000 ];
    char *c, *p;

    // clip off the fullpath
    for ( unsigned int i = 0; i < songs_len; i++ ) {
        c = strrchr( songs_pp[i], '\\' );
        if ( !c ) {
            c = strrchr( songs_pp[i], '/' );
        }
        if ( c ) {
            p = c + 1;
        } else { 
            p = songs_pp[i];
        }
        int len = strlen( songs_pp[i] );
        memmove( songs_pp[i], p, len - (p - songs_pp[i]) );
        songs_pp[i][ len - (p - songs_pp[i]) ] = '\0';
    }
}

void AudioManager_c::freeSongList( void ) {
    // free array of string pointers
    for ( unsigned int i = 0; i < songs_len; i++ ) {
        V_Free( songs_pp[i] );
    }
    V_Free( songs_pp );
    songs_len = 0;
    songs_pp = NULL;
}

AudioManager_c::~AudioManager_c ( void ) {
    if ( songs_pp ) {
    //    freeSongList(); // who cares, let V_Shutdown deal with this
    }
	songs_pp = NULL;
}
