// cl_ogg.cpp - stuff to decode ogg/vorbis background track for streaming
//

#include "../common/common.h"
#include "../common/com_types.h"    // byte

#include "../contrib/stb_vorbis.c"
#include <sys/types.h>
#include <sys/stat.h>

#include "cl_sound.h"               // wavinfo_t


byte *          s_bgdata = NULL; // master filestream handle
static byte *   data_p;         // should always point to next byte to decode
int             s_oggFileSize;

// internal
static stb_vorbis *stbv = NULL;
static int s_headerSize;

#if 0

********************************************
MOVED TO COM_SYSTEM.CPP
********************************************

#ifdef _WIN
#define _MMAP_WINDOWS
#endif

//==================== 
#define _MMAP_WINDOWS
//==================== 

#ifdef _MMAP_WINDOWS
    #include <windows.h>
    #include <stdio.h>
    #include <conio.h>
#endif
/*
====================
 get_filestream
 close_filestream
====================
*/
// FIXME: move this stuff to central location where it can be used by everybody 
#ifdef _MMAP_WINDOWS
HANDLE __fhandle = NULL, __fmap_obj = NULL;
unsigned char * get_filestream( const char *filename, int *f_size ) 
{
    __fhandle = CreateFileA(		filename,
                                    GENERIC_READ, 
                                    FILE_SHARE_READ,
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL );
    if ( __fhandle == INVALID_HANDLE_VALUE ) {
        fprintf( stderr, "couldn't open the damn file: %s\n", filename );
        return NULL;
    }

    __fmap_obj = CreateFileMapping( __fhandle,
                                                NULL, 
                                                PAGE_READONLY,
                                                0,
                                                0,
                                                NULL );
    if ( __fmap_obj == NULL ) {
        fprintf( stderr, "couldn't create file mapping: %s\n", filename );
        return NULL;
    }

	size_t fsize = GetFileSize( __fhandle, NULL );
	*f_size = fsize;
    
	LPVOID fileView = MapViewOfFile( __fmap_obj, FILE_MAP_READ, 0, 0, fsize );
    if ( fileView == NULL ) {
        fprintf( stderr, "couldn't MAP the damn file: %s\n", filename );
        return NULL;
    }

    return (unsigned char *) fileView;
}

void close_filestream( unsigned char * pBuf ) {
    UnmapViewOfFile(pBuf);
    CloseHandle(__fmap_obj);
    CloseHandle(__fhandle);
    __fmap_obj = NULL;
    __fhandle = NULL;
}
#else
static size_t __secret_size;
unsigned char * get_filestream( const char *filename, int *f_size ) {
	FILE *fp = NULL;
	fp = fopen( filename, "rb" );
	int fn = fileno(fp);
	struct stat st;

	if ( fstat(fn, &st) == -1 || st.st_size == 0 ) {
		fprintf( stderr, "fstat fail!\n" );
		return NULL; 
	}

    __secret_size = st.st_size;
    *f_size = st.st_size;

    // mmap the file stream
    unsigned char *mm = (unsigned char *) mmap(0, st.st_size, PROT_READ, MAP_SHARED, fn, 0);
    return mm;
}
void close_filestream( unsigned char * mm ) {
    if ( -1 == munmap(mm, __secret_size) )
		fprintf( stderr, "munmap indicates error\n" );
    __secret_size = 0;
}
#endif // _MMAP_WINDOWS

/*
====================
 GET_FILELEN
====================
*/
size_t get_filelen( FILE *fp ) {
    struct stat st;
    int f_num = fileno ( fp );
    fstat ( f_num, &st );
    return st.st_size;
}
#endif //

/*
========================================
 class runoff_c
========================================
*/
#define RUNOFF_SZ 40000
class runoff_c {
public:
    int size;
	unsigned char *beg;
    unsigned char *end;
	unsigned char buf[ RUNOFF_SZ ];
	runoff_c() {
		beg = buf;
        end = buf;
        size = 0;
	}

    /* FIXME: don't use this! untested */
	void copy_in( void * data_in, unsigned int in_len ) {
        
		if ( in_len > RUNOFF_SZ ) {
			fprintf( stderr, "DATA LOSS! amount copied into runoff buffer exceeds size.\n" );
		}
        if ( in_len + size > RUNOFF_SZ ) {
            fprintf( stderr, "Warning: inserting more data into runoff than it can hold. Some bytes will be lost\n" );
        }
        if ( size != 0 ) {
            if ( beg != buf ) {
                // move any contents to beginning of buffer
                memmove ( buf, beg, size );
                beg = buf;
            }
        }
        byte * p = buf + size;
        memcpy( p, data_in, in_len );
        size += in_len;
	}

    // returns a pointer to exactly sz data from the front
    // and advances the internal pointer if there is any more
    // data to be kept track of
    unsigned char *copy_out( unsigned int sz ) {
        if ( sz > RUNOFF_SZ ) {
            fprintf ( stderr, "requesting more than total size of Runoff Buffer.\n" );
        }
        if ( sz > size ) {
            fprintf ( stderr, "requesting more than size of contents of Runoff Buffer.\n" );
        }

        byte * p = beg;
        if ( sz >= size ) {
            beg = buf;
            end = buf;
            size = 0;
        } else {
            beg = beg + sz;
            size -= sz;
        }
        return p; 
    }
    void reset( void ) {
        beg = buf;
        end = buf;
        size = 0;
		memset( buf, 0, RUNOFF_SZ );
    }
};

static runoff_c runoff;

/*
====================
 ogg_decode_samples
    return how many samples retrieved
    outbuf contains decoded samps, always stereo
====================
*/
int ogg_decode_samples( unsigned char **    data, 
                        unsigned int        dlen,
                        short *             outbuf,
                        unsigned int        request_samps ) {

    int chan;
    float **outputs;
    int samples;

    const float scale = 32760.f;
    short x, y; 

    int read = 0;
    int extra = 0;
	int samps_decoded = 0;
    unsigned char *in_p = *data;
	int already = 0;

	
    // use any decoded samples found in runoff from last run if there are any
    if ( runoff.size ) {
		if ( runoff.size > request_samps * 4 ) {
			x = request_samps * 4;
		} else {
			x = runoff.size;
		}
        memcpy( outbuf, runoff.copy_out(x), x );
		already = 
        samps_decoded = x / 4;
		if ( samps_decoded >= request_samps )
			return samps_decoded;
    }

    // decode from file input
    //
    do {

        // decode a section of input
        int bytes_used = stb_vorbis_decode_frame_pushdata( 
                stbv,           // stb handle
                in_p,           // source buffer
                dlen,           // how many bytes data are in source buffer
                &chan,          // channels count
                &outputs,       // float arrays, 1 for each chan
                &samples );     // nbr of float-samples in each chan

		// end condition
		if ( bytes_used == 0 && samples == 0 && dlen < 10 ) {
			break;
		}

        // update data buffer position & length
        *data = (in_p += bytes_used);
        dlen -= bytes_used;		

		float *left, *right;
		if ( 0 != samples ) {
			left = outputs[0];
			right = (chan > 1) ? outputs[1] : outputs[0];
		}

        // fill runoff buffer with all decoded samples
        for ( int i = 0 ; i < samples; i++ ) {
            // convert to short
			float _L = left[i]  > 1.0f ? 1.f : left[i]  < -1.0f ? -1.0f : left[i];
			float _R = right[i] > 1.0f ? 1.f : right[i] < -1.0f ? -1.0f : right[i];
            ((short*)runoff.end)[ i * 2 + 0 ] = (int)( scale * _L );
            ((short*)runoff.end)[ i * 2 + 1 ] = (int)( scale * _R );
            if ( runoff.end + i * 4 - runoff.buf > RUNOFF_SZ ) {
                fprintf( stderr, "ERROR: Need bigger runoff buffer!!\n" );
                __asm { 
                    int 0x3; 
                }
                exit(-1);
            }
        }
        runoff.end += 4 * samples;
        runoff.size += 4 * samples;
        samps_decoded += samples;

    } while ( samps_decoded < request_samps );

    // copy requested amount, or whatever we have, to outbuf
    unsigned int samps = request_samps < samps_decoded ? request_samps : samps_decoded;
	unsigned int bytes = (samps - already) * 4;
	
    if ( bytes > 0 ) {
        memcpy( ((byte*)outbuf) + already * 4, runoff.copy_out(bytes), bytes );
    }

	return samps; 
}

/*
====================
 OGG_PRINT_INFO
====================
* /
void ogg_print_info( stb_vorbis *v ) {
    if ( v ) {
        stb_vorbis_info info = stb_vorbis_get_info(v);
        printf( "%d channels, %d samples/sec\n", info.channels, info.sample_rate);
        printf( "Predicted memory needed: %d (%d + %d)\n", info.setup_memory_required + info.temp_memory_required, info.setup_memory_required, info.temp_memory_required);
    }
}
*/

/*
====================
 OGG_START_FILESTREAM
====================
*/
int ogg_start_filestream( unsigned char *data ) {
    const int inc_amt = 1000;
    int try_bytes = inc_amt;
again: 
    int used = 0, error = 0;
    stbv = stb_vorbis_open_pushdata( data, try_bytes, &used, &error, NULL );
    if ( NULL == stbv ) {
        if ( error == VORBIS_need_more_data ) {
            try_bytes += inc_amt; 
            goto again;
        } else {
            fprintf ( stderr, "unknown stb error: %i\n", error );
            exit( -666 );
        }
    }
    return used;
}

/*
====================
 S_BackgroundOggOpen
====================
*/
void S_BackgroundOggOpen( const char *name ) {
    s_bgdata = get_filestream( name, &s_oggFileSize );
    s_headerSize = ogg_start_filestream ( s_bgdata ); 
    data_p = s_bgdata + s_headerSize;
    data_p = s_bgdata; 
}

/*
====================
 S_GetOggInfo
====================
*/
wavinfo_t S_GetOggInfo( void ) {
    stb_vorbis_info info = stb_vorbis_get_info( stbv );
    wavinfo_t wi;
    wi.format = 0; // ?
    wi.rate = info.sample_rate;
    wi.width = 2; // bytes per mono sample
    wi.channels = info.channels;
    wi.samples = 2000000000; // ? not sure if I can know this w/ ogg
    wi.dataofs = s_headerSize;
    return wi;
}

/*
====================
 S_StopOggBackgroundTrack
    safe to call whenever, even if not running
====================
*/
void S_StopOggBackgroundTrack( void ) {
    if ( stbv ) 
        stb_vorbis_close( stbv );
    stbv = NULL;
    if ( s_bgdata )
        close_filestream( s_bgdata );
    s_bgdata = NULL;
}

/*
====================
 S_OggGetSamples

    ioLoop - in: tells if looping, out: tells if looped or not
====================
*/
int S_OggGetSamples( byte *outbuf, unsigned int request_samps, gbool * ioLoop ) 
{
    unsigned int inlen = s_oggFileSize - (data_p - s_bgdata);
    int got_samps = ogg_decode_samples( &data_p, inlen, (short*)outbuf, request_samps );

	short * out2;
	unsigned int need;
	int get2;

    // read fell short
    if ( got_samps != request_samps ) {
        if ( *ioLoop ) {
doit_again:
			inlen = s_oggFileSize - (data_p - s_bgdata);
            out2 = (short*)(outbuf + got_samps * 4);
			need = request_samps - got_samps;
			get2 = ogg_decode_samples( &data_p, inlen, out2, need );
			got_samps += get2;
			if ( got_samps >= request_samps )
				return got_samps;
			if ( get2 > 0 )
				goto doit_again;

            stb_vorbis_flush_pushdata( stbv );
            runoff.reset();
            data_p = s_bgdata; 
            inlen = s_oggFileSize - (data_p - s_bgdata);
            out2 = (short*)(outbuf + got_samps * 4);
			need = request_samps - got_samps;
            get2 = ogg_decode_samples( &data_p, inlen, out2, need );
            got_samps += get2;
        } else {
            *ioLoop = gfalse;
        }
    } else {
        *ioLoop = gfalse;
    }

    return got_samps;
}


