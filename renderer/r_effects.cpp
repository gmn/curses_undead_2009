// r_effects.cpp
//

#include "r_ggl.h"
#include <gl/gl.h>
#include "../common/com_vmem.h"
#include "../map/mapdef.h"
#include "../lib/lib.h"
#include "../common/common.h"
#include "r_effects.h"

#define ONE  1.0f
#define ZERO 0.0f


effects_c effects;

/*
==============================================================================

 vidFader_c

==============================================================================
*/
// start
void vidFader_c::start( int _mode, int howLong ) {

	startTime = 0; // will get first time we draw
	startTime = now();

	length = howLong;	

	fracPerMsec = 1.0f / (float) length;

	// capture screen
	gglReadPixels( 0, 0, M_Width(), M_Height(), GL_RGBA, GL_UNSIGNED_BYTE, effects.screenBuffer );
	
	mode = _mode;
}

// drawFadeout
void vidFader_c::drawFadeout( void ) {

	if ( !startTime )
		startTime = now();

	ellapsed = now() - startTime;

	// get alpha value and clamp it
	float f_alpha = 1.0f - ellapsed * fracPerMsec;
	int int_alpha = (int) ( f_alpha * 255.0f ) ;
	byte byte_alpha = 0;
	if ( int_alpha > 255 ) {
		byte_alpha = 255;
	} else if ( int_alpha < 0 ) {
		byte_alpha = 0;
	} else {
		byte_alpha = (byte) int_alpha;
	}

	// write a uniform alpha into entire image
	for ( int i = 3; i < effects.screenBuf_sz; i+=4 ) {
		effects.screenBuffer[ i ] = byte_alpha;
	}

	// blit over background color
	effects.black();
	effects.blit( 0 );
}

// draw
int vidFader_c::draw( void ) {
	switch ( mode ) {
	case FADE_FADEIN:
		break;
	case FADE_FADEOUT:
		drawFadeout();
		break;
	}

	if ( ellapsed >= length ) {
		return FINISHED;
	}
	return 0;
}


/*
==============================================================================

 effects_c

==============================================================================
*/
// draw
void effects_c::draw ( void ) {
	int ret = 0;
	switch ( mode ) {
	case FX_FADEIN:
	case FX_FADEOUT:
		ret = fader.draw();
		break;
	case FX_BLACK:
		black();
		if ( timer.check() ) {
			mode = FX_NONE;
		}
		break;
	case FX_SPLIT:
		break;
	case FX_TEAR:
		break;
	}

	// effect finished
	if ( ret == FINISHED ) {
		if ( blacktime > 0 ) {
			mode = FX_BLACK;
			timer.set( blacktime );
		} else {
			mode = FX_NONE;
		}
	}
}

// startBuffer
void effects_c::startBuffer( void ) {
	if ( screenBuffer ) {
		return;
	}
	width = (GLsizei) M_Width();
	height = (GLsizei) M_Height();
	screenBuf_sz = PIXEL_BYTES * M_Width() * M_Height() ;
	screenBuffer = ( byte * ) V_Malloc( screenBuf_sz );

	// create gl texture unit
	gglEnable( GL_TEXTURE_2D );
	gglGenTextures( 1, &textures[0] );
	gglBindTexture( GL_TEXTURE_2D, textures[0] );
//	gglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
//	gglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	gglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, modes[tex_mode->integer()].minimize );
    gglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, modes[tex_mode->integer()].maximize );
	gglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S , GL_CLAMP );
	gglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T , GL_CLAMP );
	//gglTexImage2D ( GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, screenBuffer );
}

// blit
void effects_c::blit( int which ) {
	gglEnable( GL_TEXTURE_2D );
	gglBindTexture ( GL_TEXTURE_2D, textures[ which ] );
//	gglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	gglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
	gglTexImage2D( GL_TEXTURE_2D, 
				0,  				// target mipMap level. 0 is base image 
				GL_RGBA, 			// # of color components in the texture
				width, 		
				height, 
				0, 					// width of the border
				GL_RGBA, 			// format of the pixel data
				GL_UNSIGNED_BYTE, 	// data type of the pixel data
				screenBuffer );		// where the data is

	// use standard draw calls
	gglBegin( GL_QUADS );
	gglTexCoord2f( ZERO, ZERO );
	gglVertex2i( 0, 0 );
	gglTexCoord2f( ONE, ZERO );
	gglVertex2i( width, 0 );
	gglTexCoord2f( ONE, ONE );
	gglVertex2i( width, height );
	gglTexCoord2f( ZERO, ONE );
	gglVertex2i( 0, height );
	gglEnd();
}

void effects_c::black( void ) {
	gglDisable( GL_TEXTURE_2D );
	gglClearColor( _color[0], _color[1], _color[2], _color[3] );
	gglClear( GL_COLOR_BUFFER_BIT );
}
