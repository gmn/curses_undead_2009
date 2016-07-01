#ifndef __R_EFFECTS_H__
#define __R_EFFECTS_H__

#include "../common/com_object.h"
#include "../lib/lib.h"
#include "../common/common.h"
#include "../common/com_color.h"
#include "../common/com_geometry.h"

extern byte * screenBuffer;

#define FINISHED -666

enum {
	FADE_NONE,
	FADE_FADEIN,
	FADE_FADEOUT,
};

struct vidFader_c {
	int mode;
	int length, startTime, ellapsed;
	float fracPerMsec;

	void start( int, int );
	void drawFadeout();
	int draw();
};


enum {
	FX_NONE,
	FX_FADEIN,
	FX_FADEOUT, // capture screen and fade to black over period
	FX_BLACK, // black screen for a timed interval
	FX_SPLIT,
	FX_TEAR,
	//...
};

class effects_c : public Allocator_t {
private:
	int mode;
	vec4_t _color;

	static const unsigned int PIXEL_BYTES = 4;
	GLuint textures[ 2 ];

	GLsizei width, height;

	int blacktime;
	timer_c timer;

public:
	unsigned int screenBuf_sz;
	byte * screenBuffer ;

	vidFader_c fader;

	void startFade( int _mode, int length, int _blacktime =0, int clr =C_BLACK ) {
		startBuffer();
		mode = _mode;
		int fmode = _mode == FX_FADEIN ? FADE_FADEIN : FADE_FADEOUT ;
		fader.start( fmode, length );
		// can fade to color other than black if so specified
		float *c = ::color.TokenToFloat( clr );
		COPY4( this->_color, c );
		blacktime = _blacktime;
	}

	effects_c() : fader(), screenBuffer(0) {
		mode = FX_NONE; // no effect currently processing
		_color[0] = _color[1] = _color[2] = 0.f;
		_color[3] = 1.f; // black
		screenBuf_sz = 0;
	}

	bool active() { return mode != FX_NONE; }
	void draw( void ) ;
	void startBuffer( void ) ;
	void black( void );
	void blit( int );
};

extern effects_c effects;


#endif /* __R_EFFECTS_H__ */
