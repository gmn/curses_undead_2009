#ifndef __R_LERP_H__
#define __R_LERP_H__

#include "../common/common.h" // now()

/*
====================
 twoflt_t
====================
*/
struct twoflt_t {
	float p[2];
	void zero() { 
		p[0] = p[1] = 0.f; 
	}
	twoflt_t() { zero(); }
	twoflt_t( float a, float b ) {
		p[0] = a; p[1] = b;
	}
	float operator[] ( unsigned int i ) {
		if ( i > 1 ) return 0.f;
		return p[ i ];
	}
	void set( float x, float y ) { 
		p[0] = x; p[1] = y; 
	}
};

/*
====================
 lframe_t

	adds time, drawTime to twoflt_t
====================
*/
struct lframe_t : public twoflt_t {
	int time;
	int drawTime;
	// overrides set()
	void set( float x, float y ) {
		p[0] = x;
		p[1] = y;
 		time = now();
		drawTime = 0; // is set by drawer when it draws first time
	}
};

/* 
====================
 lerpFrame_c

	frame interpolation
====================
*/
struct lerpFrame_c {
	static const unsigned int total = 3;
	lframe_t frame[ total ];
	int last;
	lerpFrame_c() : last(-1) {}
	void update( float x, float y ) {
        if ( -1 == last ) { // first use ever, set all 3 frame equal
            frame[1].set( x, y );
            frame[2].set( x, y );
        }
		last = (last+1) % total;
		frame[ last ].set( x, y );
	}
	float * getLast() { return frame[ last ].p; }
	void setAll( float x, float y ) {
		for ( int i = 0; i < total; i++ ) {
			frame[ i ].set( x, y );
		}
	}
    // 0 is the last saved frame, -1 the previous, and -2 before the previous
	lframe_t & getFrame( int num = 0 ) {
		if ( 0 == num ) {
			return frame[ last ];
		}
		int get = last;
		int i = -1;
		do {
			get = get == 0 ? total - 1 : get - 1;
			if ( i == num ) {
				return frame[ get ];
			}
			if ( --i < 1 - total )
                break;
		} while(1);
		return frame[ get ];	
	}
};

#endif // __R_LERP_H__
