/*
================================================================ 
    cl_sound.h
================================================================ 
*/ 
#ifndef __CL_SOUND_H__
#define __CL_SOUND_H__


#define MAX_SNDPATH				0x400


#define	PAINTBUFFER_SIZE		4096					// this is in samples

#define SND_CHUNK_SIZE			1024					// samples
#define SND_CHUNK_SIZE_FLOAT	(SND_CHUNK_SIZE/2)		// floats
#define SND_CHUNK_SIZE_BYTE		(SND_CHUNK_SIZE*2)		// floats

#define MAX_CHANNELS    64
// MAX_SFX may be larger than MAX_SOUNDS because
// of custom player sounds
#define		MAX_SFX			4096
#define		LOOP_HASH		128

typedef int sfxHandle_t;

typedef struct {
	int			left;	// the final values will be clamped to +/- 0x00ffff00 and shifted down
	int			right;
} portable_samplepair_t;

typedef struct adpcm_state {
    short	sample;		/* Previous output value */
    char	index;		/* Index into stepsize table */
} adpcm_state_t;

typedef	struct sndBuffer_s {
	short					sndChunk[SND_CHUNK_SIZE];
	struct sndBuffer_s		*next;
    int						size;
	adpcm_state_t			adpcm;
} sndBuffer;

typedef struct soundDynamic_s {
    uint32 level;          // 0 - 255
    uint32 channel;
    uint32 distance;
    uint32 falloff;
    gbool moving;
} soundDynamic_t;

typedef struct sfx_s {
	sndBuffer		*soundData;
	gbool		defaultSound;			// couldn't be loaded, so use buzz
	gbool		inMemory;				// not in Memory
	gbool		soundCompressed;		// not in Memory
	int				soundCompressionMethod;	
	int 			soundLength;
	char 			soundName[MAX_SNDPATH];
	int				lastTimeUsed;
	struct sfx_s	*next;
    gbool       looping;
    int         loopcount; // 0: loop until stopped, >0: decrement & stop at 0
} sfx_t;

typedef struct {
	int			channels;
	int			samples;				// mono samples in buffer
	int			submission_chunk;		// don't mix less than this #
	int			samplebits;
	int			speed;
	byte		*buffer;
} dma_t;

typedef struct loopSound_s {
	vec3_t		origin;
	vec3_t		velocity;
	sfx_t		*sfx;
	int			mergeFrame;
	gbool	    active;
	gbool	    kill;
	gbool	    doppler;
	float		dopplerScale;
	float		oldDopplerScale;
	int			framenum;
} loopSound_t;

typedef struct
{
	int			allocTime;
	int			startSample;	// START_SAMPLE_IMMEDIATE = set immediately on next mix
	int			entnum;			// to allow overriding a specific sound
	int			entchannel;		// to allow overriding a specific sound
	int			leftvol;		// 0-255 volume after spatialization
	int			rightvol;		// 0-255 volume after spatialization
	int			master_vol;		// 0-255 volume before spatialization
	float		dopplerScale;
	float		oldDopplerScale;
	vec3_t		origin;			// only use if fixed_origin is set
	gbool	    has_origin;	// the sound is spatially determine
	sfx_t		*thesfx;		// sfx structure
    soundDynamic_t  dynamic;
	gbool	doppler;
} channel_t;

#define	WAV_FORMAT_PCM		1
#define START_SAMPLE_IMMEDIATE	0x7fffffff
#define MASTER_VOLUME_DEFAULT   127

#define MAX_RAW_SAMPLES 16384
#define MAX_RAW_SAMPLES_MASK    (MAX_RAW_SAMPLES-1)

typedef struct {
	int			format;
	int			rate;
	int			width;
	int			channels;
	int			samples;
	int			dataofs;		// chunk starts this many bytes from file start
} wavinfo_t;

#define SFX_DEF_CHAN        1
#define SFX_DEF_FREQ        22050
// 2 bytes
#define SFX_DEF_SAMPWIDTH   2

// PCM WAV HEADER BLOCKS
struct fileHeader {
	unsigned int riff;              // 'RIFF'
	unsigned int filesize;          // <file length - 8>
	unsigned int wave;              // 'WAVE'
	unsigned int format;            // 'fmt '
	unsigned int formatLength;      // 0x10 == 16
};

struct waveFormat {
	unsigned short formatTag;           // 1 == PCM 
	unsigned short channels;            // 1 = mono, 2 = stereo
	unsigned int samplesPerSec;         // eg, 44100, 22050, 11025
	unsigned int averageBytesPerSec;    // blockAlign * sampleRate
	unsigned short blockAlign;          // channels * bitsPerSample / 8
	unsigned short bitsPerSample;       // 8 or 16
};

struct chunkHeader {
	unsigned int type;              // 'data'
	unsigned int len;               // <length of the data block>
};

struct WAV_HEADER {
    fileHeader header;
    waveFormat format;
    chunkHeader data;
};

// sound channel defs
enum {
    CHAN_NORMAL,   // client normal, environmental determined game sounds
    CHAN_LOCAL,    // ui, hud & gui sounds, client local
    CHAN_GLOBAL,   // things heard no matter who you are, announcer, etc.
    CHAN_ANNOUNCER // ditto
};

class AudioManager_c {
public:
    // music commands
    char * current;
    void play( const char * );
    void pause( void );
    void stop( void );
    void list( void );
    void command( const char *, const char * );
    void genSongList( void ) ;
    void freeSongList( void ) ;
    AudioManager_c() : songs_len(0), songs_pp(0), current(0) { }
    ~AudioManager_c();
private:
    unsigned int songs_len;
    char ** songs_pp;
};
/*
music <no args>         prints what's playing, what second its at
music play filename     starts playing a different file
music stop              just stops it, could be same as pause, who cares
music pause             pauses the music track where it's at
music list              lists available tracks in music folder
*/

#endif /* !__CL_SOUND_H__ */

extern	dma_t	dma;

extern AudioManager_c audio;

