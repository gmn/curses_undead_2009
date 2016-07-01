#ifndef __SERVER_H__
#define __SERVER_H__

#include <math.h> // sqrtf
#include <string.h> // memset
#include "../map/mapdef.h"

int now( void );

struct sv_frame_t {
	int num;
	int time;
	unsigned int buttons;
	float origin[2];
	float viewport[2];
};

struct sv_frameSet_t {
	static const unsigned int TOTAL = 256;
	static const unsigned int TOTAL_MASK = 255;

	sv_frame_t frames[ TOTAL ];
	int current; // index into our finite set of last TOTAL frames
	int count; // total server frames run

	void next () { current = ( current + 1 ) & TOTAL_MASK; }
	void set ( int _time ) { 
		next();
		frames[ current ].time = _time;
		frames[ current ].num = ++count;
		main_viewport_t *v = M_GetMainViewport();
		frames[ current ].viewport[ 0 ] = v->world.x;
		frames[ current ].viewport[ 1 ] = v->world.y;
	}
	void set ( int _time, float _x, float _y, unsigned int _btns =0 ) {
		set( _time );
		frames[ current ].origin[0] = _x;
		frames[ current ].origin[1] = _y;
		frames[ current ].buttons = _btns;
	}
	sv_frameSet_t() {
		memset( frames, 0, sizeof(frames) );
		current = -1;
		count = 0;
		main_viewport_t *v = M_GetMainViewport();
		frames[ TOTAL_MASK ].viewport[ 0 ] = v->world.x;
		frames[ TOTAL_MASK ].viewport[ 1 ] = v->world.y;
	}
	
	float deltaTime();

	bool originChanged() { 
		int last = current == 0 ? TOTAL_MASK : current - 1;
		int beforelast = last == 0 ? TOTAL_MASK : last - 1;
		return ( frames[ last ].origin[0] != frames[ beforelast ].origin[0] || 
		 		frames[ last ].origin[1] != frames[ beforelast ].origin[1] );
	}

	sv_frame_t & getLast( void ) {
		int last = current == 0 ? TOTAL_MASK : current - 1;
		return frames[ last ];
	}

	unsigned int lastBtns( void ) {
		return frames[ current == 0 ? TOTAL_MASK : current - 1 ].buttons;
	}

		/* y  =  y0 + ( x - x0 ) * ( y1 - y0 ) / ( x1 - x0 ) */
		/* y  =	 y0 + ( y1 - y0 ) * ( x - x0 ) / ( x1 - x0 )  */
		/* y is the axis, x is time:
  		( x1 - x0 ) / ( t1 - t0 ) * ( now() - t0 ) + x0 */

	float * lerpView( void ) {
		static float axis[2];
		int last = current == 0 ? TOTAL_MASK : current - 1;
		int before = last == 0 ? TOTAL_MASK : last - 1;

		/* y1 - y0 */
		axis[0] = frames[ last ].viewport[0] - frames[ before ].viewport[0];
		axis[1] = frames[ last ].viewport[1] - frames[ before ].viewport[1];
		//axis[0] = frames[ current ].viewport[0] - frames[ last ].viewport[0];
		//axis[1] = frames[ current ].viewport[1] - frames[ last ].viewport[1];

		/* x - x0 */
		float dnow = now() - frames[ before ].time;
		//float dnow = now() - frames[ last ].time;
		/* x1 - x0 */
		float dtime = frames[ last ].time - frames[ before ].time;
		//float dtime = frames[ current ].time - frames[ last ].time;

		float lerp = dnow / dtime;

		// where viewport should be right now
		axis[0] = lerp * axis[0] + frames[before].viewport[0];
		axis[1] = lerp * axis[1] + frames[before].viewport[1];
		//axis[0] = lerp * axis[0] + frames[last].viewport[0];
		//axis[1] = lerp * axis[1] + frames[last].viewport[1];
		return axis;
	}

	/* at the point that I draw, I have current and last.  I want to draw
	somewhere between them.  based on the ratio of time between current
	and last, and the ratio of time between now and current */
	float *lerp2( void ) {
		int last = current == 0 ? TOTAL_MASK : current - 1;
		int cur = current;
		float c_m_l = frames[ current ].time - frames[ last ].time;
		float n_m_c = now() - frames[ current ].time;
		float ratio[2] ;
		ratio[0] = frames[cur].viewport[0] - frames[last].viewport[0];
		ratio[1] = frames[cur].viewport[1] - frames[last].viewport[1];
		ratio[0] /= c_m_l;
		ratio[1] /= c_m_l;
		ratio[0] *= n_m_c;
		ratio[1] *= n_m_c;
		static float vp_real[2];
		vp_real[0] = ratio[0] + frames[last].viewport[0];
		vp_real[1] = ratio[1] + frames[last].viewport[1];
		if ( current == -1 ) { // before we have any frames
			vp_real[0] = 0.f;
			vp_real[1] = 0.f;
		}
		// I have a viewport, that is a fraction between current and last,
		// based on the time it is now, and the time that the viewport last
		// updated (or server frame ran).  what I want is, to take tiles
		// that are being drawn by the current most viewport, and actually
		// adjust the viewport backwards to a time between the the last known
		// and the one before it.  You know, it also occurs to me that this
		// would be faster if you just did a GL translate command, instead of
		// adding in to each vertex in user space.  because GL does that 
		// anyway.  might as well, update the figure.
		return vp_real;
	}

	// player origin based
	float *lerp3 ( void ) {
		int last = current == 0 ? TOTAL_MASK : current - 1;
		int cur = current;
		float c_m_l = frames[ current ].time - frames[ last ].time;
		float n_m_c = now() - frames[ current ].time;
		static float ratio[2] ;
		ratio[0] = frames[cur].origin[0] - frames[last].origin[0];
		ratio[1] = frames[cur].origin[1] - frames[last].origin[1];
		ratio[0] /= c_m_l;
		ratio[1] /= c_m_l;
		ratio[0] *= n_m_c;
		ratio[1] *= n_m_c;
		//static float origin_real[2];
		//origin_real[0] = ratio[0] + frames[last].origin[0];
		//origin_real[1] = ratio[1] + frames[last].origin[1];
		return ratio;
	}

	// tile again, adding in polar coordinate fix
	float * lerp4( void ) {
		int last = current == 0 ? TOTAL_MASK : current - 1;
		int cur = current;
		float c_m_l = frames[ current ].time - frames[ last ].time;
		float n_m_c = now() - frames[ current ].time;
		float ratio[2] ;
		ratio[0] = frames[cur].viewport[0] - frames[last].viewport[0];
		ratio[1] = frames[cur].viewport[1] - frames[last].viewport[1];
		float denom = sqrtf( ratio[0] * ratio[0] + ratio[1] * ratio[1] );
		if ( denom > 0.01f ) {
			ratio[0] /= denom;
			ratio[1] /= denom;
		}
		ratio[0] /= c_m_l;
		ratio[1] /= c_m_l;
		ratio[0] *= n_m_c;
		ratio[1] *= n_m_c;
		static float vp_real[2];
		vp_real[0] = ratio[0] + frames[last].viewport[0];
		vp_real[1] = ratio[1] + frames[last].viewport[1];
		return vp_real;
	}
};

extern sv_frameSet_t sv_frames; 
extern float sv_fps_actual;

void SV_Frame( int );
inline bool SV_PlayerMoved( void ) {
	return sv_frames.originChanged();
}
void SV_Init( void );

// extrapolates tile coordinate based on viewport movement the last 2 frames
float * SV_LerpTile( void ) ;
float * SV_LerpPlayer( void ) ;

void SV_Pause( void ) ;
void SV_UnPause( void ) ;
void SV_TogglePause( void ) ;

#endif // __SERVER_H__
