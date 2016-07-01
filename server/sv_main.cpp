// sv_main.cpp
//

#include "../common/common.h"
#include "../game/g_move.h"   // cmdbuf
#include "../game/g_entity.h" // player
#include "../client/cl_public.h" // cls.state
#include "server.h"
#include <math.h>
#include "../map/m_area.h"

bool 	sv_paused = false;

class gvar_c *sv_fps = 0;

sv_frameSet_t sv_frames;

float sv_fps_actual = 0.f;
extern gvar_c *pl_runMult;
extern bool com_freeze_thinkers;

// FIXME: currently broken
void SV_FPS( void ) {
	static int last = 0, now = 0;
	now = Com_Millisecond();
	static int frames = 0;
	++frames;

	if ( now - last > 1000 ) {
		int diff = now - last;
		last = now;
		sv_fps_actual = (float)frames / (float)diff * 1000.f;
		frames = 0;
	}
}

/*
====================
 SV_Init
====================
*/
void SV_Init( void ) {
	sv_fps = Gvar_Get( "sv_fps", "60", 0 );
}

/*
====================
 SV_PrepareThinkers

	make first move for player and entities
====================
*/
void SV_PrepareThinkers( void ) {
	// begin all thinkers wish move 
	Entity_t ** epp = world.entList.data;
	const unsigned int elen = world.entList.length();
	Entity_t ** estart = epp;
	while ( epp - estart < elen ) {
		if ( (*epp)->entClass >= EC_THINKER )
			(*epp)->BeginMove();
		++epp;
	}

	// creates an unmoved wish to start off with
	player.BeginMove(); 
}

/*
====================
 SV_RunThinkers
====================
*/
void SV_RunThinkers( float frame_sz ) {

	// make move
	Entity_t ** epp = world.entList.data;
	const unsigned int elen = world.entList.length();
	Entity_t ** estart = epp;

	// think once for each
	while ( epp - estart < elen ) {
		(*epp++)->think();
	}
}

extern float r_angle;
extern gvar_c *platform_type;

/*
====================
 SV_PlayerMove
====================
*/
unsigned int SV_PlayerMove( float frame_sz ) {
	if ( player.pause_timer.check() )
		player.paused = false;

	// 
	player.think(); // animations, state and stuff

	if ( sv_paused )
		return 0; // could've paused in player.think()

	// evaluate player commands.  basically, add them all together, average
	//  their effect to construct the final vector move
	const unsigned int total = cmdbuf.size();
	if ( !total )
		return 0;

	const float one_over_total = 1.0f / total;
	float move[2] = { 0.f, 0.f };

	unsigned int cmdbuf_sz = cmdbuf.size();
	unsigned int shots = 0, termshot = 0;
	unsigned int buttons = 0;
	while ( cmdbuf.size() > 0 ) {
		usercmd_t cmd = cmdbuf.pop();
		move[0] += cmd.axis[0] / 127.0f;
		move[1] += cmd.axis[1] / 127.0f ;
		buttons |= cmd.buttons;
		/* have shots, got a cmd w/o shot */
		if ( shots && !(cmd.buttons & MOVE_SHOOT1) ) { 
			++termshot;
		}
		if ( cmd.buttons & MOVE_SHOOT1 ) {
			++shots;
		}
	}

	// update shooting condition
	player.shoot.notify( shots );

	// divide & average over total moves
	move[0] *= one_over_total;
	move[1] *= one_over_total;

	float ooN = 0.f;
	double denom = move[0] * move[0] + move[1] * move[1];
	if ( denom > 0.01 ) {
		ooN = 1.f / sqrt( denom );
		// scale by vector normal
		move[0] *= ooN;
		move[1] *= ooN;
	}

	// scale move by frame size
	move[0] *= frame_sz;
	move[1] *= frame_sz;

	// scale by player velocity
	move[0] *= pl_speed->value();
	move[1] *= pl_speed->value();

	if ( buttons & MOVE_RUN ) {
		move[0] *= pl_runMult->value();
		move[1] *= pl_runMult->value();
	}

/* moving these to cl_input. because we only store ONE flag for IMPULSE,
    where cl_input copies IMPULSE in for each key, which renders teh impulse
    meaningless 
	if ( (buttons & (UP_IMPULSE|DOWN_IMPULSE)) ) {
		move[0] *= 0.5f;
		move[1] *= 0.5f;
	} else if ( buttons & DOWN_IMPULSE ) { 
		move[0] *= 0.5f;
		move[1] *= 0.5f;
	} else if ( buttons & UP_IMPULSE ) {
		move[0] *= 0.25f;
		move[1] *= 0.25f;
	}
*/

// FIXME: just combine all these scaling factors together into one and 
// then as the very last step, multiply into the move.
// some day when speed becomes a factor

	// save desired move
    if ( platform_type->integer() == 2 ) {
        M_Rotate2d( move, -r_angle );
    }
	player.wishMove( move[0], move[1] );

	return buttons;
}

/*
====================
 SV_RunOneFrame
====================
*/
unsigned int SV_RunOneFrame( float frame_sz, int time ) {
	
	unsigned int buttons = 0;

	//
	// PLAYER
	//
	if ( !com_freeze_thinkers ) {
		buttons = SV_PlayerMove( frame_sz );
	}

	if ( sv_paused )
		return buttons;

	//
	// All Thinkers
	//
	SV_RunThinkers( frame_sz );

	return buttons;
}

/*
====================
 ThinkerCollisions
====================
*/
void ThinkerCollisions( void ) {
	
	/* tiles specifically check ent->wish. trig has nothing to do with it */

	// check for tile collisions
	Entity_t ** thinker = world.entList.data;
	Entity_t ** tstart = thinker;
	const unsigned int ttotal = world.entList.length();
	while ( thinker - tstart < ttotal ) 
	{
		(*thinker)->in_collision = 0;
		// don't worry about it if not collidable
		if ( !(*thinker) || !(*thinker)->collidable ) {
			++thinker; 	
			continue;
		}
		(*thinker)->boxen.reset();
		colModel_t **cmpp = world.colList.data;
		colModel_t **cm_start = cmpp;
		const unsigned int cm_len = world.colList.length();
		while ( cmpp - cm_start < cm_len ) {
			// if the thinker's wish intersects the tile's colModel
			if ((*cmpp)->boxIntersect( (*thinker)->wish ) ) {
				(*thinker)->in_collision |= 1; 
				(*thinker)->boxen.add( *cmpp );
			}
			++cmpp;
		}
		if ( (*thinker)->in_collision & 1 ) {
			(*thinker)->handleTileCollisions();
		}
		++thinker;
	}

	// reset proj_vecs before any entity colliision 
	thinker = world.entList.data;
	while ( thinker - tstart < ttotal ) {
		if ((*thinker) && 
			(*thinker)->collidable && 
			(*thinker)->entClass >= EC_THINKER ) { /* <-- should this be all? */
			(*thinker)->proj_vec.reset();
		}
		++thinker;
	}
	player.proj_vec.reset();

	/* entity collisions all check colModel first.  col->box is the bound
	around trig & clip.  handle() checks triggerable, trig, clipping, clip.
	handle() decides what to do */

	// player collision with all entities 
	thinker = world.entList.data;
	while ( thinker - tstart < ttotal ) {
		if ((*thinker) && 
			(*thinker)->collidable && 
			(*thinker)->col &&
			!(*thinker)->delete_me &&
			(*thinker)->col->collide( *player.col ) ) {
			Entity_t * player_p = &player;
			(*thinker)->handle( &player_p );
		}
		++thinker;
	}

	// entity collisions with each other -- ( n choose 2 )
	thinker = world.entList.data;
	Entity_t ** estart = world.entList.data;
	const unsigned int elen = world.entList.length();
	while ( thinker - tstart < ttotal ) 
	{
		if ( !(*thinker) || !(*thinker)->collidable || (*thinker)->delete_me ) {
			++thinker; 	
			continue;
		}
		Entity_t ** epp = thinker + 1 ;
		while ( epp - estart < elen ) {
			if ((*epp) && 
				(*epp)->collidable && 
				(*epp)->col && 
				!(*epp)->delete_me &&
				(*epp)->col->collide( *(*thinker)->col ) ) {
				(*thinker)->handle( epp );
			}
			++epp;
		}
		++thinker;
	}

	// finalize move
	thinker = world.entList.data;
	while ( thinker - tstart < ttotal ) {

		// finalize move
		if ( (*thinker)->entClass >= EC_THINKER )
			(*thinker)->FinishMove();

		// runtime deletion
		if ( (*thinker)->delete_me & 0x2 ) {
			(*thinker)->blockRemove();
			delete *thinker;
			*thinker = NULL;
			world.force_rebuild = true;
		}
		++thinker;
	}
}

/* 
====================
 PlayerCollision

	player move is constructed here
====================
*/
void PlayerCollision( int buttons ) {
	// no buttons, no reason to check anything.  any entities that come at
	//  the player, will be dealt with in the thinker loop
	if ( ! buttons ) 
		return ;

	// tile collisions
	player.boxen.reset();
	player.in_collision = 0;
	colModel_t **cmpp = world.colList.data;
	colModel_t **start = cmpp;
	const unsigned int len = world.colList.length();
	while ( cmpp - start < len ) {
		if ( (*cmpp)->boxIntersect( player.wish ) ) {
			player.in_collision |= 1; // mapTile collision
			player.boxen.add( *cmpp );
		}
		++cmpp;
	}
	if ( player.in_collision & 1 ) {
		player.handleTileCollisions();
	}

	// set USE KEY state for thinker section
	if ( buttons & MOVE_USE ) {
		if ( 0 == player.use ) {
			player.use = 1;
		} else if ( player.use > 64 ) {
			player.use = 0;
		} else {
			player.use = ( player.use << 1 ) & ~1;
		}
	} else {
		player.use = 0;
	}
}

/*
====================
 SV_Collisions
====================
*/
void SV_Collisions( int buttons ) {
	// player tile collisions
	PlayerCollision( buttons );

	//
	ThinkerCollisions();

	// after everything else, complete the player move
	P_FinishMove();
}


int		mapStartTime;
int		lastTime;
int 	leftOver;

/*
==================
SV_Frame
==================
*/
void SV_Frame( int msec ) {

	if ( sv_paused ) {
		return;
	}

	// no server at all until map is running
	if ( cls.state != CS_PLAYMAP ) {
		mapStartTime = 0;
		return;
	}

	// first frame of map
	if ( !mapStartTime ) {
		lastTime = mapStartTime = now();
		return;
	}

	int frameMsec = 1000 / sv_fps->integer() ;
	const float frame_sz = 1.f / sv_fps->integer();


	int time = now();
	if ( time - lastTime < frameMsec ) 
		return;
	
	leftOver = time - lastTime;

	int lastSave = lastTime;

	unsigned int buttons = 0;

	// start the moves
	SV_PrepareThinkers();

	int frame = 0;
	const int MAX_SV_FRAMES = 5;

	// run the game simulation frames
	while ( leftOver >= frameMsec && frame++ < MAX_SV_FRAMES ) {
		leftOver -= frameMsec;
		lastTime += frameMsec;

		// let everything in the world think and move
		buttons |= SV_RunOneFrame( frame_sz, time );
	
		if ( sv_paused )
			break; // player could have triggered a pause during the sv frame
	}

	// run collision detection routines over everybody, and do final move
	SV_Collisions( buttons );

	// record stats of the server frame 
	sv_frames.set( lastSave, player.poly.x, player.poly.y, buttons );

	SV_FPS();
}





float sv_frameSet_t::deltaTime() { 
	int last = current == 0 ? TOTAL_MASK : current - 1;
	if ( frames[last].time == 0 )
		return 0.f;
	return ( (float)now()-(float)frames[last].time ) / 1000.f / sv_fps->value();
}

float * SV_LerpTile( void ) {
	float *f = sv_frames.lerp2();
	static float a[2];
	main_viewport_t *v = M_GetMainViewport();
	a[0] = v->world.x - f[0];
	a[1] = v->world.y - f[1];
	return a;
}

float * SV_LerpPlayer( void ) {
	return sv_frames.lerp3();
}




/*
// returns an adjustment for a vertex , extrapolated viewport position vs. time
float * SV_LerpTile( void ) {
	
	// returns where the viewport should be now, extrapolated from the 
	// last 2 samples
	float *lvp = sv_frames.lerpView();
	//float *lvp = sv_frames.lerp2();
	main_viewport_t *vp = M_GetMainViewport();
	static float diff[2];
	diff[0] = lvp[0] - vp->world.x;
	diff[1] = lvp[1] - vp->world.y;
	return diff;
} */

void SV_Pause( void ) {
	sv_paused = true;
}

void SV_UnPause( void ) {
	lastTime = now();
	sv_paused = false;
}

void SV_TogglePause( void ) {
	if ( sv_paused ) {
		SV_UnPause();
	} else { 
		SV_Pause();
	}
}
