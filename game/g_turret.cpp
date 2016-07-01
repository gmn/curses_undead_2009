// g_sentry.cpp
//

#include "g_entity.h"
#include "../lib/lib.h"
#include <math.h>

int Turret_t::id = 0;
animSet_t * Turret_t::turretAnimSet = NULL;

void Turret_t::think( void ) {
	float player_dist = aabb[0] - player.aabb[0];
	player_dist *= player_dist;
	player_dist += (aabb[1] - player.aabb[1]) * (aabb[1]-player.aabb[1]) ;
	player_dist = sqrtf( player_dist );

	// always set the latest unit vector to the player's current location
	player_dir[0] = ( player.aabb[0] - aabb[0] ) / player_dist;
	player_dir[1] = ( player.aabb[1] - aabb[1] ) / player_dist;
	
	if ( player_dist >= scanRadius ) {
		// lay dormant
		shoot.notify( 0 );
		return;
	}

	// we have to be facing within 45 degrees of ray to player, else we must 
	// turn in the direction of player, slowing us down from shooting slightly.
	const double pi180 = M_PI * 0.0055555555555555;
	float facing_vector[2] = { cos( angle * pi180 ), sin( angle * pi180 ) };

	float right[2] = { 1.0f , 0.0f };
	// 
	float p_angle = M_GetAngleDegrees( right, player_dir );

	// i'm calling ccwise "positive" rotation
	if ( player_dir[1] < 0.f ) {
		p_angle = 360.0f - p_angle;
	}

	M_ClampAngle360( &angle );
	M_ClampAngle360( &p_angle );

	float relative_angle = p_angle - angle;
	M_ClampAngle360( &relative_angle );

	// shoot if we're close enough 
	if ( relative_angle < shoot_window || relative_angle > 360.f - shoot_window ) {
		shoot.notify( 1 ); 
	} else {
		shoot.notify( 0 );
	}

	// don't move as long as we are close enough
	if ( relative_angle < threshold || 360.f - relative_angle < threshold ) {
		return;
	}

	// keep turning til we are close to facing the player

	// how far do we move per server frame (based on turn_time)
	// FIXME: this can be pre-computed
	float secsToTurn360 = ((float)turn_time) * 8.f * 0.001f;
	float frames_per_full_circle = secsToTurn360 * sv_fps->value();
	float frames_per_8th_turn = frames_per_full_circle * 0.125f;
	float degrees_per_frame = 45.f / frames_per_8th_turn;

	if ( relative_angle < 180.0f ) 
		angle += degrees_per_frame;
	else
		angle -= degrees_per_frame;

	// sets facing
	wishMove( cos( angle * pi180 ), sin( angle * pi180 ) );
}


void Turret_t::ProjectileHandler( Projectile_t *P ) {
	if ( PROJ_DONE == P->state || PROJ_EXPLODING == P->state ) {
		return;
	}
	
}

void Turret_t::_handle( Entity_t **epp ) {
	if ( !epp || !*epp )
		return;
	switch ( (*epp)->entType ) {
	case ET_PROJECTILE: return ProjectileHandler( dynamic_cast<Projectile_t*>(*epp) );
	default:
		return;
	}
}

void Turret_t::buildAnims( void ) {
	turretAnimSet = new animSet_t();
	animation_t *A = turretAnimSet->startAnim();
	A->init( 1, "right" );
	A->frames[0] = materials.FindByName( "gfx/SPRITE/TURRET/cam01.tga" );
	A = turretAnimSet->startAnim();
	A->init( 1, "down_right" );
	A->frames[0] = materials.FindByName( "gfx/SPRITE/TURRET/cam02.tga" );
	A = turretAnimSet->startAnim();
	A->init( 1, "down" );
	A->frames[0] = materials.FindByName( "gfx/SPRITE/TURRET/cam03.tga" );
	A = turretAnimSet->startAnim();
	A->init( 1, "down_left" );
	A->frames[0] = materials.FindByName( "gfx/SPRITE/TURRET/cam04.tga" );
	A = turretAnimSet->startAnim();
	A->init( 1, "left" );
	A->frames[0] = materials.FindByName( "gfx/SPRITE/TURRET/cam05.tga" );
	A = turretAnimSet->startAnim();
	A->init( 1, "up_left" );
	A->frames[0] = materials.FindByName( "gfx/SPRITE/TURRET/cam06.tga" );
	A = turretAnimSet->startAnim();
	A->init( 1, "up" );
	A->frames[0] = materials.FindByName( "gfx/SPRITE/TURRET/cam07.tga" );
	A = turretAnimSet->startAnim();
	A->init( 1, "up_right" );
	A->frames[0] = materials.FindByName( "gfx/SPRITE/TURRET/cam08.tga" );
}

material_t * Turret_t::_getMat( void ) {
	if ( !anim ) {
		anim = new animSet_t( *Turret_t::turretAnimSet );
	}
	switch ( facing ) {
	case MOVE_LEFT: 		return anim->getMat( "left" );
	case MOVE_DOWN_LEFT: 	return anim->getMat( "down_left" );
	case MOVE_DOWN:			return anim->getMat( "down" );
	case MOVE_DOWN_RIGHT:	return anim->getMat( "down_right" );
	case MOVE_RIGHT:		return anim->getMat( "right" );
	case MOVE_UP_RIGHT:		return anim->getMat( "up_right" );
	case MOVE_UP:			return anim->getMat( "up" );
	case MOVE_UP_LEFT:		return anim->getMat( "up_left" );
	}
	return NULL;
}

int Turret_t::DirFromAngle( void ) {
	static const float Q[8] = { 22.5f, 67.5f, 112.5f, 157.5f, 202.5f, 247.5f, 292.5f, 337.5f };
	if ( angle >= Q[7] || (angle >= 0.f && angle < Q[0]) ) {
		return MOVE_RIGHT;
	} else if ( angle >= Q[0] && angle < Q[1] ) {
		return MOVE_UP | MOVE_RIGHT;
	} else if ( angle >= Q[1] && angle < Q[2] ) {
		return MOVE_UP;
	} else if ( angle >= Q[2] && angle < Q[3] ) {
		return MOVE_UP | MOVE_LEFT;
	} else if ( angle >= Q[3] && angle < Q[4] ) {
		return MOVE_LEFT;
	} else if ( angle >= Q[4] && angle < Q[5] ) {
		return MOVE_DOWN | MOVE_LEFT;
	} else if ( angle >= Q[5] && angle < Q[6] ) {
		return MOVE_DOWN;
	} else if ( angle >= Q[6] && angle < Q[7] ) {
		return MOVE_DOWN | MOVE_RIGHT;
	}
	return MOVE_RIGHT;
}

void Turret_t::FinishMove( void ) {

	if ( shoot.check() ) {
		fireWeapon();
	}

	M_ClampAngle360( &angle );

	facingLast = facing = DirFromAngle();
}

