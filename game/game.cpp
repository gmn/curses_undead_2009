// game.cpp
//

#include "game.h"
#include "g_entity.h"
#include "../common/common.h"
#include "../client/cl_console.h"
#include <stdlib.h>
#include <time.h>


// sound handles
filehandle_t laser_sound[4];
filehandle_t robot_crash;
filehandle_t robot_hit;
filehandle_t player_hit;
filehandle_t wilhelm_scream;
filehandle_t robot_shock;
filehandle_t door_sound[8];
filehandle_t success_sound[8];
filehandle_t explosion_sound[2];
filehandle_t tap_foot;
filehandle_t turretball_sound[10];
gvar_c *g_tbsound;


//
float * G_CalcBoxCollision( float *src_box, float *targ_box ) {

	float sc[2], pc[2];
	// src_box is player/protagonist/thing moving
	AABB_getCenter( pc, src_box );
	// targ_box is mapTile/thing stationary being collided into
	AABB_getCenter( sc, targ_box );

	// half-widths
	float p_hw[2] ={ fabs ( pc[0] - src_box[0] ), fabs ( pc[1] - src_box[1] )};
	float s_hw[2] ={ fabs ( sc[0] - targ_box[0] ), fabs ( sc[1] - targ_box[1])};

	static float proj[2];
	proj[0] = p_hw[0] + s_hw[0] - fabs( sc[0] - pc[0] );
	proj[1] = p_hw[1] + s_hw[1] - fabs( sc[1] - pc[1] );

	// delta distance between box centers
	float dx = pc[0] - sc[0];
	float dy = pc[1] - sc[1];

	// calculate projection vector
	// project in X
	if ( proj[0] < proj[1] ) {
		// left
		if ( dx < 0 ) {
			proj[0] *= -1.f;
			proj[1] = 0.f;
		// right
		} else {
			proj[1] = 0.f;
		}
	// project in Y
	} else {
		// down
		if ( dy < 0 ) {
			proj[0] = 0.f;
			proj[1] *= -1.f;
		} else {
			proj[0] = 0.f;
		}
	}
	return proj;
}


// helper function
static filehandle_t GETSND ( int n, const char *S ) { 
	filehandle_t fht;
	char sbuf[1024];
	sprintf( sbuf, "%s/%s", fs_gamepath->string(), S );
	fht = S_RegisterSound( sbuf ); 
	if ( fht < 0 ) { 
		Com_Printf( "couldn't load %s \n", S ); 		
		console.Printf( "couldn't load %s \n", S ); 
	} 
	return fht;
}

extern gvar_c * g_robotSpeed;
extern gvar_c * g_scanRadius;
extern gvar_c * g_minAdvance;
extern gvar_c * g_robotLOS;
extern gvar_c * g_robotRunMult;

/*
====================
 G_InitGame

	called once at startup to do data setups
====================
*/
void G_InitGame( void ) {
	static bool started = 0;
	if ( started )
		return;
	srand( _getpid() * time(0) );
	g_robotSpeed = Gvar_Get( "g_robotSpeed", "330", 0 );
	g_scanRadius = Gvar_Get( "g_scanRadius", "2300", 0 );
	g_minAdvance = Gvar_Get( "g_minAdvance", "500", 0 );
	g_robotLOS = Gvar_Get( "g_robotLOS", "1", 0, "enable line of sight obstruction checking" );
	g_robotRunMult = Gvar_Get( "g_robotRunMult", "2.0", 0 );


    // FIXME: these shouldn't be here, but should be put in each classes self respective constructor. 

	// type specific animation sets
	Robot_t::buildAnims();
	Turret_t::buildAnims();

	started = 1;

	//
	// sounds
	//

/*
	laser_sound = S_RegisterSound( "zpak/sound/melst_b4.wav" );
	if ( laser_sound < 0 ) {
		Com_Printf( "couldn't load melst_b4.wav\n");
		console.Printf( "couldn't load melst_b4.wav\n");

	}
*/


/* 
 * these should be specified in script
 * 
 * which means that you need a set of specifiers... not sure. just can't
 * have game crash because it is trying to load a hardcoded, but non-essential
 * file. instead, check for sound, if its not there, dont load it and keep
 * going
 *

	
	laser_sound[0] = GETSND( 0, "sound/FREESOUND/laser00.wav" );
	laser_sound[1] = GETSND( 0, "sound/FREESOUND/laser01.wav" );
	robot_crash = GETSND( 2, "sound/FREESOUND/robot_crash.wav" );
	robot_hit = GETSND( 0, "sound/FREESOUND/robot_hit.wav" );
	player_hit = GETSND( 0, "sound/FREESOUND/player_hit.wav" );
	robot_shock = GETSND( 0, "sound/FREESOUND/robot_shock.wav" );
	wilhelm_scream = GETSND( 0, "sound/Wilhelm.wav" );
	door_sound[0] = GETSND( 0, "sound/door01.wav" );
	door_sound[1] = GETSND( 0, "sound/door02.wav" );
	success_sound[0] = GETSND( 0, "sound/success01.wav" );
	success_sound[1] = GETSND( 0, "sound/success02.wav" );
	success_sound[2] = GETSND( 0, "sound/success03.wav" );
	explosion_sound[0] = GETSND( 0, "sound/explode01.wav" );
	tap_foot = GETSND( 0, "sound/tap_foot.wav" );

	//g_laserSnd = Gvar_Get( "g_laserSnd", "0", 0 );

	g_tbsound = Gvar_Get( "g_tbsound", "0", 0 );
	turretball_sound[0] = GETSND( 0, "sound/turret_shot.wav" );
*/

	//C_strncpy( track, "zpak/sound/01 - LANYARD LOOP.wav", 128 );
	//C_strncpy( track, "zpak/sound/Charlie Parker - Be Bop.wav", 128 );
	//S_StartBackGroundTrack( track );
	//S_StartBackGroundTrack( filename, &note, gtrue, 1 );

	//Assert( laser_sound >= 0 );
    //if ( note < 0 ) 
     //   return;

//	S_StartLocalSound( laser_sound );
}

