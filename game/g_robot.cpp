// g_robot.cpp
//
#define __MAPEDIT_H__
#include "../map/m_area.h"
#include "g_entity.h"
#include "../common/common.h"
#include "../lib/lib.h"
#include "../client/cl_console.h"
#include <stdlib.h>
#ifdef _WIN32
	#include <process.h>
#endif
#include <math.h>
#include <time.h>

int Robot_t::id = 0;
animSet_t *Robot_t::robotAnimSet = NULL;

// control
gvar_c * g_robotSpeed;
gvar_c * g_scanRadius;
gvar_c * g_minAdvance;
gvar_c * g_robotLOS;
gvar_c * g_robotRunMult;

extern filehandle_t robot_hit;
extern filehandle_t robot_crash;

void randomDirection_t::newdir( void ) {
	int angle = rand();
	angle >>= 5;
	angle %= 360;

	v[0] = cos( angle * M_PI * 0.005555555f );
	v[1] = sin( angle * M_PI * 0.005555555f );
	begin( v );
}

#define _M_PI_ 3.14159265358979323

void randomDirection_t::newAxisDir( void ) {
	int angle = rand();
	angle >>= 5;
	angle %= 8;

	v[0] = cosf( angle * 45 * _M_PI_ * 0.005555555f );
	v[1] = sinf( angle * 45 * _M_PI_ * 0.005555555f );
	begin( v );
}


// nearest ent
struct _tmp_s {
	Entity_t *p;
	float dist;
	bool hunted_seen;
	_tmp_s() : p(0), dist(0.f), hunted_seen(0) {}
} nearest;

/*
====================
====================
*/
Robot_t::Robot_t(Robot_t const &R ) : Entity_t( R ), ScanRadius(3300) {
    ++id;
    
// FIXME: write this!
}

/*
====================
 Robot_t::Scan

	sets playerSeen if player is in range and not behind a wall
====================
*/
void Robot_t::Scan( void ) {

	// get robot center
	float rc[2];
	poly.getCenter( rc );

	// calculate player_dist 
	float pc[2];
	player.poly.getCenter( pc );
	player_dist = sqrtf( ( pc[0] - rc[0] ) * ( pc[0] - rc[0] ) +
						 ( pc[1] - rc[1] ) * ( pc[1] - rc[1] ) );

	// determine if we see him from dist
	if ( player_dist <= g_scanRadius->value() ) {
		playerSeen = true;
	} else {
		playerSeen = false;
	}
	
	if ( !g_robotLOS->integer() )
		return;

	if ( !playerSeen )
		return;

	// the player is in range, do a rough line-of-sight to look for walls
	
	// create aabbs from the player to the bot the size of the player poly
	// spaced by the shortest half length.

	float pw = player.clip[2] - player.clip[0];
	float ph = player.clip[3] - player.clip[1];
	float spacing = ( pw < ph ) ? pw : ph  ;

	// reset the list
	los.init( 30 );

	// vector to advance each test box
	float v[2];
	v[0] = ( rc[0] - pc[0] ) / player_dist * spacing;
	v[1] = ( rc[1] - pc[1] ) / player_dist * spacing;

	// how many
	int tot = (int)player_dist / (int)spacing + 1;

	// starting point at player ll-corner
	float 	x = player.clip[0], 
			y = player.clip[1];

	// construct aabbs
	int i = 0;
	while ( i++ < tot ) 
	{
		f4_t f;
		f.v[0] = x;
		f.v[1] = y;
		f.v[2] = x + pw;
		f.v[3] = y + ph;

		los.add ( f );

		x += v[0];
		y += v[1];
	}

	// save line-of-sight length
	const int los_len = los.length();

	// get a set of data we're looking through
	int be_len;
	BlockExport_t *be = M_GetBlockExport( rc, g_scanRadius->value(), &be_len );

	// for all the found lists
	for ( int i = 0; i < be_len ; i++ ) 
	{
		// test each colModel against the line-of-sight boxlist
		for ( unsigned int j = 0; j < be[i].cmln; j++ ) {
			for ( int k = 0; k < los_len; k++ ) {
				if ( AABB_intersect( be[i].cmpp[j]->box, los[k].v ) ) {
					// intersection with map geometry, player hidden
					playerSeen = false;
					break;
				}
			}
			if ( !playerSeen )
				break;
		}
		if ( !playerSeen )
			break;
	}

	V_Free( be );
}

#if 0
		// collect entities
		// for each entity class	
		node_c<Entity_t*> * e = be[i].elst[ EC_THINKER ].gethead();
		while ( e ) {
			// skip if it found itself or targeting something not a bot
			if ( e->val == this || e->val->entType != ET_ROBOT ) {
				e = e->next;
				continue;
			}
			// we don't want it if ..
			Robot_t *R = dynamic_cast<Robot_t*>(e->val);
			if ( R && R->state > ROBOT_DIRECTIONAL ) {
				e = e->next;
				continue;
			}
			if ( AABB_intersect( e->val->aabb, perim ) ) {
				if ( nearest.p ) {
					// get dist
					float *f = e->val->aabb;
					float dist = 
								( f[0] - c[0] ) * ( f[0] - c[0] ) + 
								( f[1] - c[1] ) * ( f[1] - c[1] ) ;
					dist = sqrtf( dist );
					if ( dist < nearest.dist ) {
						nearest.dist = dist;
						nearest.p = e->val;
					}
				} else {
					nearest.p = e->val;
				}

				// we've seen hunting
				if ( e->val == hunted ) {
					nearest.hunted_seen = true;
				}
			}
			e = e->next;
		}
	}
	// return nearest entity
}
#endif



/*
====================
 Robot_t::SentryDuty
====================
*/
bool Robot_t::SentryDuty( float *v ) {

	// do a scan
	Scan();

	// prepare the move 
	v[0] = v[1] = 0.f;

	// should we go on the attack
	bool plot_attack_course = false;
	bool do_move = false;

	switch ( sentry.state ) {
	case ROBOT_SNOOZE:
		if ( sentry.timer.check() ) {
			sentry.state = ROBOT_PACING;
			sentry.timer.set( 3000 ); // pace for at _least_ X seconds
		}
		shoot.notify( 0 );
		plot_attack_course = false;
		do_move = false;
		break;
	case ROBOT_ATTACKING:
		if ( !playerSeen ) {
			sentry.state = ROBOT_LOST_HUNTED;
			sentry.timer.set( 3000 ) ; 
		}

		// we are shooting
		shoot.notify( 1 );
		plot_attack_course = true;
		
		// we are too close, stop moving, but continue to shoot in player's 
		// direction
		if ( playerSeen && player_dist < g_minAdvance->value() ) {
			plot_attack_course = false;
			do_move = true;
			v[0] = lastTraj[0] * 0.07; // creep in player direction in 
			v[1] = lastTraj[1] * 0.07; // order to maintain bullet aim
		}
		angry = true;
		
		break;
	case ROBOT_LOST_HUNTED:
		if ( playerSeen ) {
			sentry.state = ROBOT_ATTACKING;
		} else if ( sentry.timer.check() ) {
			sentry.state = ROBOT_PACING;
			hunted = NULL;
		}

		// keep shooting
		shoot.notify( 1 );

		// continue on last heading
		v[0] = lastTraj[0];
		v[1] = lastTraj[1];

		do_move = true;
		angry = true;

		break;
	case ROBOT_PACING:
		angry = false;

		if ( playerSeen ) {
			sentry.state = ROBOT_ATTACKING;
			hunted = &::player;
			shoot.notify(1);
			plot_attack_course = true;

		// take a step, maybe snooze
		} else {
			do_move = true;

			switch ( sentry.type ) {
			case ROBOT_LATERAL:
				v[0] = sentry.dir ? 1.0f : -1.0f;
				v[1] = 0.0f;
				break;
			case ROBOT_VERTICAL:
				v[0] = 0.0f;
				v[1] = sentry.dir ? 1.0f : -1.0f;
				break;
			case ROBOT_POSITIVE_INCLINE:
				v[0] = sentry.dir ? 0.707107f : -0.707107f;
				v[1] = sentry.dir ? 0.707107f : -0.707107f ;
				break;
			case ROBOT_NEGATIVE_INCLINE:
				v[0] = sentry.dir ? -0.707107f : 0.707107f;
				v[1] = sentry.dir ? 0.707107f : -0.707107f ;
				break;
			}

			// save in case we get interupted
			lastTraj[0] = v[0];
			lastTraj[1] = v[1];

			if ( ++sentry.steps > 150 ) {
				sentry.steps = 0;
				sentry.dir ^= 0x1;
			}

			// not shooting, just pacing
			shoot.notify( 0 );

			// don't even get a chance to snooze until pace timer runs out
			if ( !sentry.timer.check() )
				break;;

			// maybe take a snooze
			int what = ( rand() % 1000 );
			if ( what >= 0 && what < 4 ) {
				sentry.state = ROBOT_SNOOZE;
				sentry.timer.set( 1000 + ( rand() % 7000 ) );
			}
		}
		break;
	}

	// move towards something we want to attack
	if ( plot_attack_course ) {
		if ( hunted ) {
			//v[0] = hunted->aabb[0] - aabb[0];
			//v[1] = hunted->aabb[1] - aabb[1];
			v[0] = player.aabb[0] - aabb[0];
			v[1] = player.aabb[1] - aabb[1];
			M_Normalize2d( v );
			lastTraj[0] = v[0];
			lastTraj[1] = v[1];
		} else {
			v[0] = lastTraj[0];
			v[1] = lastTraj[1];
		}
		do_move = true;
	}

	return do_move;
}



/*
====================
 Robot_t::think
====================
*/
void Robot_t::think( void ) {
	// pause timer set by teleport, or gamePaused
	if ( paused && pause_timer.check() )
		paused = false;

	bool do_move = false;

	float v[2] = { 0,0 };

	// turn off weapon trigger
	if ( ROBOT_SENTRY != state ) {
		shoot.notify( 0 ) ;
	}

	// hack, if we enter think with a collision, switch state to directional
	// move around randomly for a certain length of time and then change back
	// to sentry.  side-effect: in directional, robot doesn't scan.  
	// although he will go back to SENTRY if shot
	if ( in_collision && state == ROBOT_SENTRY ) {
		Scan();
		if ( !playerSeen ) {
			state = ROBOT_DIRECTIONAL;
		}
	}

	// hit by bullet
	if ( projHit && state <= ROBOT_DIRECTIONAL ) {
		sentry.state = ROBOT_LOST_HUNTED;
		sentry.timer.set( 3000 );
		lastTraj[0] = projTraj[0];
		lastTraj[1] = projTraj[1];
		projHit = false;
	}

	switch ( state ) {
	case ROBOT_DONE:
		if ( hullRemovalTime && timer.check() ) {
			delete_me |= 0x2;
		}
		break;
	case ROBOT_START_EXPLODING:
		state = ROBOT_EXPLODING;
		break;
	case ROBOT_EXPLODING:
		if ( timer.check() ) {
			state = ROBOT_DONE;
			if ( hullRemovalTime != 0 )
				timer.set( hullRemovalTime );
			else
				timer.reset();
		}
		break;
	case ROBOT_PAUSED:
		if ( !timer.check() ) {
			return;
		} 
		state = ROBOT_DIRECTIONAL;
		dir.newAxisDir();
		do_move = true;
		break;
	case ROBOT_DIRECTIONAL:
		if ( timer.check() ) {
			state = ROBOT_SENTRY;
			dir.newAxisDir();
			timer.set( 3000 );
		}

		// distance expired
		else if ( dir.check() || in_collision ) {
			dir.newAxisDir();
		}

		v[0] = dir.v[0];
		v[1] = dir.v[1];
		do_move = true;

		break;
	case ROBOT_SENTRY:
		if ( SentryDuty( v ) )
			do_move = true;
		break;
	}

	if ( do_move ) {
		float amt = 1.f / sv_fps->value() * g_robotSpeed->value(); 
		if ( state == ROBOT_SENTRY &&
				(sentry.state == ROBOT_ATTACKING || 
				sentry.state == ROBOT_LOST_HUNTED) )  {
			amt *= g_robotRunMult->value();
		}
		v[0] *= amt;
		v[1] *= amt;
		wishMove( v[0], v[1] );
		dir.sofar += amt;
	}
}


void Robot_t::ProjectileHandler( Projectile_t *P ) {
	if ( !P )
		return;

	// we dont care
	if ( state > ROBOT_DIRECTIONAL )
		return;

	// make sure the projectile hasn't already hit us (is it exploding?)
	if ( P->state == PROJ_EXPLODING )
		return;
	
	// check the clips
	if ( !P->clipping || !this->clipping || !AABB_intersect( P->wish, wish ) ) {
		return;
	}

	// ok we clip, now..
	S_StartLocalSound( robot_hit );


	// take damage
	if ( (this->hitpoints -= P->damage) <= 0 ) {
		// full damage
		state = ROBOT_START_EXPLODING;
		delete_me |= 0x1;
		timer.set( explode_time );
		console.Printf( "Robot destroyed" );
		S_StartLocalSound( robot_crash );
	}
	


	// throw bot backwards inertia from hit
//	float *cor = G_CalcBoxCollision( wish, P->wish );
	float *cor = G_CalcBoxCollision( wish, P->clip );
	wishMove( cor[0] * 0.5f, cor[1] * 0.5f );


	// triggger the projectile to explode
	P->explode();


	// we're pissed off , we took damage
	//angry = true;
	//angry_timer.set( 4000 );
	projTraj[0] = -cor[0];
	projTraj[1] = -cor[1];
	M_Normalize2d( projTraj );
	projHit = 1;


	// wake it up if sleeping
	if ( sentry.state == ROBOT_SNOOZE ) {
		sentry.state = ROBOT_PACING;
		sentry.timer.set( 3000 );
	}

	// if we didn't explode yet, nudge it to sentry so it can retalliate
	if ( ROBOT_START_EXPLODING != state )
		state = ROBOT_SENTRY;
}

// two robots collide.  cause a brief pause, and then chart a new course
// for each, preferably in reverse of the previous course 
void Robot_t::RobotHandler( Robot_t *robot ) {
	if ( !robot )
		return; 

	bool hit = false;

	// not now
	if ( state > ROBOT_DIRECTIONAL || robot->state > ROBOT_DIRECTIONAL )
		return;
	
	// we hit
	if ( robot->clipping && this->clipping && 
		AABB_intersect( robot->wish, this->wish ) ) {

		// our correction
		float *correction = G_CalcBoxCollision( wish, robot->wish );

		// copy it
		float ours[2] = { correction[0], correction[1] };

		// their correction
		correction = G_CalcBoxCollision( robot->wish, wish ) ;

		// move both out of the incident area
		robot->wishMove( correction[0], correction[1] );
		this->wishMove( ours[0], ours[1] );

		// encourage moving out of the way
		if ( robot->state == ROBOT_SENTRY && robot->sentry.state != ROBOT_ATTACKING )
			robot->state = ROBOT_DIRECTIONAL;
		if ( this->state == ROBOT_SENTRY && this->sentry.state != ROBOT_ATTACKING )
			this->state = ROBOT_DIRECTIONAL;

		hit = true;
	}

/*
	if ( hit ) {
		// tell each to pause & recover from hit
		robot->GatherSelfTogether( 500 );
		this->GatherSelfTogether( 500 );
	}
*/
}


void Robot_t::_handle( Entity_t **epp ) {
	switch ( (*epp)->entType ) {
	case ET_ROBOT: return RobotHandler( dynamic_cast<Robot_t*>(*epp) );
	case ET_PROJECTILE: return ProjectileHandler( dynamic_cast<Projectile_t*>(*epp));
	// robot+player interaction handled in player
	default:
		break;
	}
}




// might use later
int Robot_t::trigger( Entity_t **epp ) {
	return 0;
}


/* FIXME: this isn't right, but it will work for now */
void Robot_t::GatherSelfTogether( int msec ) {
    timer.set( msec );
    state = ROBOT_PAUSED;
}

/*
====================
 Robot_t::_getMat
====================
*/
material_t * Robot_t::_getMat() {
	if ( paused )
		return mat;
	if ( !anim )
		anim = robotAnimSet;

	if ( state == ROBOT_START_EXPLODING ) {
		mat = anim->getMat( "exploding" , 0 );
	
	} else if ( state == ROBOT_EXPLODING ) {
		mat = anim->getMat( "exploding" );

	} else if ( state == ROBOT_DONE ) {
		mat = anim->getMat( "exploding", 5 );

	} else if ( !angry ) {
		if ( facingLast & MOVE_LEFT )
			mat = anim->getMat( "green_left" );
		else if ( facingLast & MOVE_RIGHT )
			mat = anim->getMat( "green_right" );
		else
			mat = anim->getMat( "green_right" );
	} else {
		if ( facingLast & MOVE_LEFT )
			mat = anim->getMat( "red_left" );
		else if ( facingLast & MOVE_RIGHT )
			mat = anim->getMat( "red_right" );
		else
			mat = anim->getMat( "red_right" );
	}
	return mat;
}

/*
====================
====================
*/
void Robot_t::buildAnims( void ) {
	robotAnimSet = new animSet_t();
	animation_t *A = robotAnimSet->startAnim();
	A->init( 6, "red_left" );
	A->frames[0] = materials.FindByName( "gfx/SPRITE/ROBOT/robotR00_L.tga" );
	A->frames[1] = materials.FindByName( "gfx/SPRITE/ROBOT/robotR01_L.tga" );
	A->frames[2] = materials.FindByName( "gfx/SPRITE/ROBOT/robotR02_L.tga" );
	A->frames[3] = materials.FindByName( "gfx/SPRITE/ROBOT/robotR03_L.tga" );
	A->frames[4] = materials.FindByName( "gfx/SPRITE/ROBOT/robotR04_L.tga" );
	A->frames[5] = materials.FindByName( "gfx/SPRITE/ROBOT/robotR05_L.tga" );
	A = robotAnimSet->startAnim();
	A->init( 6, "red_right" );
	A->frames[0] = materials.FindByName( "gfx/SPRITE/ROBOT/robotR00_R.tga" );
	A->frames[1] = materials.FindByName( "gfx/SPRITE/ROBOT/robotR01_R.tga" );
	A->frames[2] = materials.FindByName( "gfx/SPRITE/ROBOT/robotR02_R.tga" );
	A->frames[3] = materials.FindByName( "gfx/SPRITE/ROBOT/robotR03_R.tga" );
	A->frames[4] = materials.FindByName( "gfx/SPRITE/ROBOT/robotR04_R.tga" );
	A->frames[5] = materials.FindByName( "gfx/SPRITE/ROBOT/robotR05_R.tga" );
	A = robotAnimSet->startAnim();
	A->init( 6, "green_left" );
	A->frames[0] = materials.FindByName( "gfx/SPRITE/ROBOT/robotG00_L.tga" );
	A->frames[1] = materials.FindByName( "gfx/SPRITE/ROBOT/robotG01_L.tga" );
	A->frames[2] = materials.FindByName( "gfx/SPRITE/ROBOT/robotG02_L.tga" );
	A->frames[3] = materials.FindByName( "gfx/SPRITE/ROBOT/robotG03_L.tga" );
	A->frames[4] = materials.FindByName( "gfx/SPRITE/ROBOT/robotG04_L.tga" );
	A->frames[5] = materials.FindByName( "gfx/SPRITE/ROBOT/robotG05_L.tga" );
	A = robotAnimSet->startAnim();
	A->init( 6, "green_right" );
	A->frames[0] = materials.FindByName( "gfx/SPRITE/ROBOT/robotG00_R.tga" );
	A->frames[1] = materials.FindByName( "gfx/SPRITE/ROBOT/robotG01_R.tga" );
	A->frames[2] = materials.FindByName( "gfx/SPRITE/ROBOT/robotG02_R.tga" );
	A->frames[3] = materials.FindByName( "gfx/SPRITE/ROBOT/robotG03_R.tga" );
	A->frames[4] = materials.FindByName( "gfx/SPRITE/ROBOT/robotG04_R.tga" );
	A->frames[5] = materials.FindByName( "gfx/SPRITE/ROBOT/robotG05_R.tga" );

	A = robotAnimSet->startAnim();
	A->init( 6, "exploding" );
	A->frames[0] = materials.FindByName( "gfx/SPRITE/ROBOT/robotEXP00.tga" );
	A->frames[1] = materials.FindByName( "gfx/SPRITE/ROBOT/robotEXP01.tga" );
	A->frames[2] = materials.FindByName( "gfx/SPRITE/ROBOT/robotEXP02.tga" );
	A->frames[3] = materials.FindByName( "gfx/SPRITE/ROBOT/robotEXP03.tga" );
	A->frames[4] = materials.FindByName( "gfx/SPRITE/ROBOT/robotEXP04.tga" );
	A->frames[5] = materials.FindByName( "gfx/SPRITE/ROBOT/robotEXP05.tga" );
	A->setMSPF( 150 ); // usually defaults to 100
}


