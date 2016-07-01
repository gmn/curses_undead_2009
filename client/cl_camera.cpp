// cl_camera.cpp
//

#include "../map/mapdef.h"

//#include "../common/common.h"

#include "cl_local.h"

#include "../server/server.h"


#include "../game/g_entity.h" // player

#include "../lib/lib.h" // math


Camera_t camera;
gvar_c * cam_autoAdjust;
gvar_c * cam_radius;

/*
====================
 Camera_t::init
====================
*/
void Camera_t::init( void ) {
	// get main viewport X Y, W, H
	main_viewport_t *v = M_GetMainViewport();
	x = &v->world.x;
	y = &v->world.y;
	r = &v->ratio;
	ir = &v->iratio;
	w = M_Width();
	h = M_Height();

    cam_radius = Gvar_Get( "cam_radius", "0", 0 );
    cam_radius->setValue( 0.5f * h * *r * 0.15f );
	//radius = 0.5f * h * *r * ( 0.24f /* <-- */ );

	ms_per_frame = 1000 / 60;
	window_creep_amt = 20.f;
	drift_compensate = 40.0f;

	timer.reset();
	wait_til_creep = 1600; // milliseconds
	
	last[0] = 0.f;
	last[1] = 0.f;
	last_excess = 0.f;
	cam_autoAdjust = Gvar_Get( "cam_autoAdjust", "0", 0 );

	started = true;
}

/*
====================
 Camera_t::distFromCenter
====================
*/
float Camera_t::distFromCenter( float _x, float _y ) {

	// find center of viewport
	float cx = *x + 0.5f * w * *r;
	float cy = *y + 0.5f * h * *r;
	
	// player distance from it
	float direction[2];
	direction[0] = _x - cx;
	direction[1] = _y - cy;

	return M_Magnitude2d( direction );
}

/*
====================
 Camera_t::
====================
*/
void Camera_t::update( void ) {

	if ( !started )
		return;

	// give player initial origin
	if ( lastUpdate == 0 ) {
		sv_frame_t & sf = sv_frames.getLast();
		last[0] = sf.origin[0];
		last[1] = sf.origin[1];
		timer.set( wait_til_creep );
	}

	// don't update too much, but at a constant framerate
	int time = now();
	if ( time - lastUpdate < ms_per_frame ) {
		return;
	}

	lastUpdate = time;

	// find center of viewport
	float cx = *x + 0.5f * w * *r;
	float cy = *y + 0.5f * h * *r;
	
	// player distance from it
	float direction[2];
	direction[0] = player.poly.x - cx;
	direction[1] = player.poly.y - cy;

	// update direction based on size of player ( by bbox )
	direction[0] += 0.5f * ( player.col->box[2] - player.col->box[0] );
	direction[1] += 0.5f * ( player.col->box[3] - player.col->box[1] );
	
	player_dist = M_Normalize2d( direction );

	// times up, do viewport auto drifting 
	if ( timer.check() && player_dist > window_creep_amt ) {
		if ( cam_autoAdjust->integer() ) {
			//*x += direction[0] * window_creep_amt; 
			//*y += direction[1] * window_creep_amt;
			controller.cameraMove( direction[0] * window_creep_amt, direction[1] * window_creep_amt );
		}

	// player moved, reset the waiting timer
	} else if ( SV_PlayerMoved() ) {
		timer.set( wait_til_creep );
		sv_frame_t & sf = sv_frames.getLast();
		last[0] = sf.origin[0];
		last[1] = sf.origin[1];

	// when centered, continually reset timer
	} else if ( player_dist <= window_creep_amt ) {
		timer.set( wait_til_creep );
	}
}

/*
====================
 Camera_t::
====================
*/
// start a camera shake
void Camera_t::shake( float intensity, float seconds ) {
}

/*
====================
 Camera_t::
====================
*/
void Camera_t::update( unsigned int etype, float dx, float dy ) {
	poly_t *p = NULL;	
	float *c = NULL;
	float mv_dist, dist;

	if ( ISZERO( dx ) && ISZERO( dy ) ) {
		etype = 0; // void the move
	}

	switch( etype ) {
	case CE_PMOVE:
			p = &player.poly;
			c = player.col->box;
			dist = distFromCenter( p->x + 0.5f * (c[2]-c[0]) , p->y + 0.5f * (c[3]-c[1]) );
			mv_dist = distFromCenter( p->x + 0.5f * (c[2]-c[0]) + dx, p->y + 0.5f * (c[3]-c[1]) + dy );
			// only move viewport if the player moving away from center
			// and is at the bounds of the radius
			if ( mv_dist >= cam_radius->value() && mv_dist >= dist ) {
				*x += dx;
				*y += dy;
			}
		break;
	case CE_PTILECOL:
			p = &player.poly;
			c = player.col->box;
			dist = distFromCenter( p->x + 0.5f * (c[2]-c[0]) , p->y + 0.5f * (c[3]-c[1]) );
			mv_dist = distFromCenter( p->x + 0.5f * (c[2]-c[0]) + dx, p->y + 0.5f * (c[3]-c[1]) + dy );
			if ( mv_dist >= cam_radius->value() && mv_dist >= dist ) {
				*x += dx;
				*y += dy;
			}
		break;
	}

	// always save the viewport after an alteration
	lerp.update( *x, *y );
}


