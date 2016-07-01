// g_player.cpp
//

#include "../map/m_area.h"
#include "g_entity.h"
#include "g_item.h"

#include "../common/common.h"
#include "../server/server.h"
#include "../client/cl_console.h"
#include "../renderer/r_floating.h"
#include "../renderer/r_effects.h"

gvar_c *pl_speed = NULL;
gvar_c *com_reject = NULL;
gvar_c *pl_runMult = NULL;
extern gvar_c *platform_type;

//
Player_t player;

int CL_ReloadMap( void );

extern floatingText_c floating;

extern filehandle_t player_hit;
extern filehandle_t wilhelm_scream;
extern filehandle_t robot_shock;
extern filehandle_t success_sound[];
extern filehandle_t tap_foot;

//extern filehandle_t laser_sound[];

static void resetStats( void ) {
	player.hitpoints = 24;
	player.exp = 0;
	player.level = 1;
	player.stamina = 100;
	player.score = 0;
	player.money = 0;
}

/*
====================
 Player_t::Player_t
====================
*/
Player_t::Player_t() {
	entType = ET_PLAYER;
	entClass = EC_THINKER;


	pl_speed = Gvar_Get( "pl_speed", "1100.0", 0, "1100 is good walking speed at a 512x512 game w/ mag ~8" );

	// good value seems to be between 1.3 & 2.0. if the value is too low, the
	// player gets stuck when walking against a wall.  if the value is too 
	// high, the entity bounces in a visibly noticeable way from the wall
	com_reject = Gvar_Get( "com_reject", "1.3", 0, "scaling factor for collision rejection" );

	strcpy( name, "Main_Player" );

	waitTime = 60000; //

	ioType = IN_TYPE5;

	collidable = true;
	triggerable = true;
	clipping = true;

	pl_runMult = Gvar_Get( "pl_runMult", "1.6", 0, "run multiplier" );

	shoot.msPerShot = 400; // g_fireRate->integer();

	use = 0;
	hitpoints = 24;
	state = PLAYER_NORMAL;
	death_time = 1500;

	items.init();
	resetStats();

	anim = NULL;
	mat = NULL;
}

/*
====================
 G_PlayerStartState
====================
*/
static void G_PlayerStartState( void ) {
	player.entType = ET_PLAYER;
	player.entClass = EC_THINKER;
	player.waitTime = 60000; //
	player.collidable = true;
	player.triggerable = true;
	player.clipping = true;
	player.shoot.msPerShot = 400; 
	player.use = 0;
	player.hitpoints = 24;
	player.state = PLAYER_NORMAL;
	player.death_time = 700;
	player.items.reset();
	resetStats();
	player.shock_timer.reset();
	player.shock_delay = 400;
}

/*
====================
 Player_t::SetFromEntity
====================
*/
void Player_t::SetFromEntity( Player_t const & E ) {
//	if ( E.anim ) {
//		anim = new animSet_t( *E.anim );
//	}

//	if ( E.mat ) {
//		mat = new material_t( *E.mat );
//	else
//		mat = NULL;

	poly.set( E.poly );
	COPY4( aabb, E.aabb );
	COPY8( obb, E.obb );
	collidable = E.collidable;

	if ( E.col ) {
		col = new colModel_t( *E.col );
	} else 
		col = NULL;

	COPY4( trig, E.trig );
	COPY4( clip, E.clip );

	// other player stuff ??
	hitpoints = E.hitpoints;
	state = PLAYER_NORMAL;

	items.reset();

	resetStats();

//	node_c<Item_t*> *i = E.items.gethead();
//	while ( i ) 
}

void Player_t::setupDrawSurfs ( void ) {
	if ( anim ) // already have it
		return;
	anim = new animSet_t();
	animation_t * A = NULL;

    // movement - gun
    A = anim->startAnim();
    A->init( 4, "down_w_gun" );
	A->frames[0] = materials.FindByName( "gfx/player/pdown01-gun.tga" );
	A->frames[1] = materials.FindByName( "gfx/player/pdown02-gun.tga" );
	A->frames[2] = materials.FindByName( "gfx/player/pdown03-gun.tga" );
	A->frames[3] = materials.FindByName( "gfx/player/pdown04-gun.tga" );
    
    A = anim->startAnim();
    A->init( 4, "right_w_gun" );
	A->frames[0] = materials.FindByName( "gfx/player/pright01-gun.tga" );
	A->frames[1] = materials.FindByName( "gfx/player/pright02-gun.tga" );
	A->frames[2] = materials.FindByName( "gfx/player/pright03-gun.tga" );
	A->frames[3] = materials.FindByName( "gfx/player/pright04-gun.tga" );

    A = anim->startAnim();
    A->init( 4, "left_w_gun" );
	A->frames[0] = materials.FindByName( "gfx/player/pleft01-gun.tga" );
	A->frames[1] = materials.FindByName( "gfx/player/pleft02-gun.tga" );
	A->frames[2] = materials.FindByName( "gfx/player/pleft03-gun.tga" );
	A->frames[3] = materials.FindByName( "gfx/player/pleft04-gun.tga" );

    // movement - bat
    A = anim->startAnim();
    A->init( 4, "down_w_bat" );
	A->frames[0] = materials.FindByName( "gfx/player/pdown01-bat.tga" );
	A->frames[1] = materials.FindByName( "gfx/player/pdown02-bat.tga" );
	A->frames[2] = materials.FindByName( "gfx/player/pdown03-bat.tga" );
	A->frames[3] = materials.FindByName( "gfx/player/pdown04-bat.tga" );
    
    A = anim->startAnim();
    A->init( 4, "right_w_bat" );
	A->frames[0] = materials.FindByName( "gfx/player/pright01-bat.tga" );
	A->frames[1] = materials.FindByName( "gfx/player/pright02-bat.tga" );
	A->frames[2] = materials.FindByName( "gfx/player/pright03-bat.tga" );
	A->frames[3] = materials.FindByName( "gfx/player/pright04-bat.tga" );

    A = anim->startAnim();
    A->init( 4, "left_w_bat" );
	A->frames[0] = materials.FindByName( "gfx/player/pleft01-bat.tga" );
	A->frames[1] = materials.FindByName( "gfx/player/pleft02-bat.tga" );
	A->frames[2] = materials.FindByName( "gfx/player/pleft03-bat.tga" );
	A->frames[3] = materials.FindByName( "gfx/player/pleft04-bat.tga" );

    A = anim->startAnim();
    A->init( 4, "up_w_bat" );
	A->frames[0] = materials.FindByName( "gfx/player/pup01-bat.tga" );
	A->frames[1] = materials.FindByName( "gfx/player/pup02-bat.tga" );
	A->frames[2] = materials.FindByName( "gfx/player/pup03-bat.tga" );
	A->frames[3] = materials.FindByName( "gfx/player/pup04-bat.tga" );

    // movement - no items
    A = anim->startAnim();
    A->init( 4, "up" );
	A->frames[0] = materials.FindByName( "gfx/player/pup01.tga" );
	A->frames[1] = materials.FindByName( "gfx/player/pup02.tga" );
	A->frames[2] = materials.FindByName( "gfx/player/pup03.tga" );
	A->frames[3] = materials.FindByName( "gfx/player/pup04.tga" );

    A = anim->startAnim();
    A->init( 4, "down" );
	A->frames[0] = materials.FindByName( "gfx/player/pdown01.tga" );
	A->frames[1] = materials.FindByName( "gfx/player/pdown02.tga" );
	A->frames[2] = materials.FindByName( "gfx/player/pdown03.tga" );
	A->frames[3] = materials.FindByName( "gfx/player/pdown04.tga" );

    A = anim->startAnim();
    A->init( 4, "right" );
	A->frames[0] = materials.FindByName( "gfx/player/pright01.tga" );
	A->frames[1] = materials.FindByName( "gfx/player/pright02.tga" );
	A->frames[2] = materials.FindByName( "gfx/player/pright03.tga" );
	A->frames[3] = materials.FindByName( "gfx/player/pright04.tga" );

    A = anim->startAnim();
    A->init( 4, "left" );
	A->frames[0] = materials.FindByName( "gfx/player/pleft01.tga" );
	A->frames[1] = materials.FindByName( "gfx/player/pleft02.tga" );
	A->frames[2] = materials.FindByName( "gfx/player/pleft03.tga" );
	A->frames[3] = materials.FindByName( "gfx/player/pleft04.tga" );

    // use
    A = anim->startAnim();
    A->init( 2, "use_up" );
    A->setMSPF( 250 );
	A->frames[0] = materials.FindByName( "gfx/player/pup01.tga" );
	A->frames[1] = materials.FindByName( "gfx/player/pup-use.tga" );

    A = anim->startAnim();
    A->init( 2, "use_right" );
    A->setMSPF( 250 );
	A->frames[0] = materials.FindByName( "gfx/player/pright01.tga" );
	A->frames[1] = materials.FindByName( "gfx/player/pright-use.tga" );

    A = anim->startAnim();
    A->init( 2, "use_left" );
    A->setMSPF( 250 );
	A->frames[0] = materials.FindByName( "gfx/player/pleft01.tga" );
	A->frames[1] = materials.FindByName( "gfx/player/pleft-use.tga" );

    A = anim->startAnim();
    A->init( 2, "use_down" );
    A->setMSPF( 250 );
	A->frames[0] = materials.FindByName( "gfx/player/pdown01.tga" );
	A->frames[1] = materials.FindByName( "gfx/player/pdown-use.tga" );

    A = anim->startAnim();
    A->init( 2, "use_right_gun" );
    A->setMSPF( 250 );
	A->frames[0] = materials.FindByName( "gfx/player/pright03-gun.tga" );
	A->frames[1] = materials.FindByName( "gfx/player/pright-use-gun.tga" );

    A = anim->startAnim();
    A->init( 2, "use_left_gun" );
    A->setMSPF( 250 );
	A->frames[0] = materials.FindByName( "gfx/player/pleft03-gun.tga" );
	A->frames[1] = materials.FindByName( "gfx/player/pleft-use-gun.tga" );

    A = anim->startAnim();
    A->init( 2, "use_down_gun" );
    A->setMSPF( 250 );
	A->frames[0] = materials.FindByName( "gfx/player/pdown01-gun.tga" );
	A->frames[1] = materials.FindByName( "gfx/player/pdown-use-gun.tga" );

    A = anim->startAnim();
    A->init( 2, "use_right_bat" );
    A->setMSPF( 250 );
	A->frames[0] = materials.FindByName( "gfx/player/pright01-bat.tga" );
	A->frames[1] = materials.FindByName( "gfx/player/pright-use-bat.tga" );

    A = anim->startAnim();
    A->init( 2, "use_left_bat" );
    A->setMSPF( 250 );
	A->frames[0] = materials.FindByName( "gfx/player/pleft01-bat.tga" );
	A->frames[1] = materials.FindByName( "gfx/player/pleft-use-bat.tga" );

    A = anim->startAnim();
    A->init( 2, "use_down_bat" );
    A->setMSPF( 250 );
	A->frames[0] = materials.FindByName( "gfx/player/pdown01-bat.tga" );
	A->frames[1] = materials.FindByName( "gfx/player/pdown-use-bat.tga" );

    A = anim->startAnim();
    A->init( 2, "use_up_bat" );
    A->setMSPF( 250 );
	A->frames[0] = materials.FindByName( "gfx/player/pup01-bat.tga" );
	A->frames[1] = materials.FindByName( "gfx/player/pup-use-bat.tga" );


    // swing bat
    A = anim->startAnim();
    A->init( 4, "swingbat_down" );
    A->setMSPF( 100 );
    A->direction = ANIM_BACK_AND_FORTH;
	A->frames[0] = materials.FindByName( "gfx/player/pdown01-bat.tga" );
	A->frames[1] = materials.FindByName( "gfx/player/pdown01-swingbat.tga" );
	A->frames[2] = materials.FindByName( "gfx/player/pdown02-swingbat.tga" );
	A->frames[3] = materials.FindByName( "gfx/player/pdown03-swingbat.tga" );
    
    A = anim->startAnim();
    A->init( 4, "swingbat_right" );
    A->setMSPF( 100 );
    A->direction = ANIM_BACK_AND_FORTH;
	A->frames[0] = materials.FindByName( "gfx/player/pright01-bat.tga" );
	A->frames[1] = materials.FindByName( "gfx/player/pright01-swingbat.tga" );
	A->frames[2] = materials.FindByName( "gfx/player/pright02-swingbat.tga" );
	A->frames[3] = materials.FindByName( "gfx/player/pright03-swingbat.tga" );

    A = anim->startAnim();
    A->init( 4, "swingbat_left" );
    A->setMSPF( 100 );
    A->direction = ANIM_BACK_AND_FORTH;
	A->frames[0] = materials.FindByName( "gfx/player/pleft01-bat.tga" );
	A->frames[1] = materials.FindByName( "gfx/player/pleft01-swingbat.tga" );
	A->frames[2] = materials.FindByName( "gfx/player/pleft02-swingbat.tga" );
	A->frames[3] = materials.FindByName( "gfx/player/pleft03-swingbat.tga" );

    A = anim->startAnim();
    A->init( 4, "swingbat_up" );
    A->setMSPF( 100 );
    A->direction = ANIM_BACK_AND_FORTH;
	A->frames[0] = materials.FindByName( "gfx/player/pup01-bat.tga" );
	A->frames[1] = materials.FindByName( "gfx/player/pup01-swingbat.tga" );
	A->frames[2] = materials.FindByName( "gfx/player/pup02-swingbat.tga" );
	A->frames[3] = materials.FindByName( "gfx/player/pup03-swingbat.tga" );

    // dying
    A = anim->startAnim();
    A->init( 8, "dying" );
    A->setMSPF( 150 );
	A->frames[0] = materials.FindByName( "gfx/player/pdying01.tga" );
	A->frames[1] = materials.FindByName( "gfx/player/pdying02.tga" );
	A->frames[2] = materials.FindByName( "gfx/player/pdying03.tga" );
	A->frames[3] = materials.FindByName( "gfx/player/pdying04.tga" );
	A->frames[4] = materials.FindByName( "gfx/player/pdying05.tga" );
	A->frames[5] = materials.FindByName( "gfx/player/pdying06.tga" );
	A->frames[6] = materials.FindByName( "gfx/player/pdying07.tga" );
	A->frames[7] = materials.FindByName( "gfx/player/pdying08.tga" );
}

material_t * Player_t::_getMat( void ) {
	if ( !anim && !mat )
		return NULL;
	if ( paused )
		return mat;
    #define SETM( t ) mat = anim->getMat( t )

    // set facingLast 
	if ( facing & MOVE_LEFT ) {
		facingLast = MOVE_LEFT;
	} else if ( facing & MOVE_RIGHT ) {
		facingLast = MOVE_RIGHT;
	} else if ( facing & MOVE_UP ) {
		facingLast = MOVE_UP;
	} else if ( facing & MOVE_DOWN ) {
		facingLast = MOVE_DOWN;
    }

	if ( isDead() ) {
		if ( PLAYER_START_DYING == state ) {
			mat = anim->getMat( "dying", 0 );
		} else if ( PLAYER_DYING == state ) { 
			mat = anim->getMat( "dying" );
		} else {
			mat = anim->getMat( "dying", 7 );
		}
        return mat;
    }

    if ( facing & MOVE_DOWN ) {
        mat = anim->getMat( "swingbat_down" );
	} else if ( facing & MOVE_UP ) {
        mat = anim->getMat( "swingbat_up" );
	} else if ( facing & MOVE_RIGHT ) {
        mat = anim->getMat( "swingbat_right" );
	} else if ( facing & MOVE_LEFT ) {
        mat = anim->getMat( "swingbat_left" );
    }

    // movement - none, bat, gun
    // use - none, bat, gun
    // swingbat
    int n = now();
    int which = n % 35000;
    if ( which < 5000 ) {
        if ( facing & MOVE_DOWN ) {
            SETM( "down" );
    	} else if ( facing & MOVE_UP ) {
            SETM( "up" );
    	} else if ( facing & MOVE_RIGHT ) {
            SETM( "right" );
    	} else if ( facing & MOVE_LEFT ) {
            SETM( "left" );
        }
    } else if ( which < 10000 ) {
        if ( facing & MOVE_DOWN ) {
            SETM( "down_w_bat" );
    	} else if ( facing & MOVE_UP ) {
            SETM( "up_w_bat" );
    	} else if ( facing & MOVE_RIGHT ) {
            SETM( "right_w_bat" );
    	} else if ( facing & MOVE_LEFT ) {
            SETM( "left_w_bat" );
        }
    } else if ( which < 15000 ) {
        if ( facing & MOVE_DOWN ) {
            SETM( "down_w_gun" );
    	} else if ( facing & MOVE_UP ) {
            SETM( "up" );
    	} else if ( facing & MOVE_RIGHT ) {
            SETM( "right_w_gun" );
    	} else if ( facing & MOVE_LEFT ) {
            SETM( "left_w_gun" );
        }
    } else if ( which < 20000 ) {
        if ( facing & MOVE_DOWN ) {
            SETM( "use_down" );
    	} else if ( facing & MOVE_UP ) {
            SETM( "use_up" );
    	} else if ( facing & MOVE_RIGHT ) {
            SETM( "use_right" );
    	} else if ( facing & MOVE_LEFT ) {
            SETM( "use_left" );
        }
    } else if ( which < 25000 ) {
        if ( facing & MOVE_DOWN ) {
            SETM( "use_down_bat" );
    	} else if ( facing & MOVE_UP ) {
            SETM( "use_up_bat" );
    	} else if ( facing & MOVE_RIGHT ) {
            SETM( "use_right_bat" );
    	} else if ( facing & MOVE_LEFT ) {
            SETM( "use_left_bat" );
        }
    } else if ( which < 30000 ) {
        if ( facing & MOVE_DOWN ) {
            SETM( "use_down_gun" );
    	} else if ( facing & MOVE_UP ) {
            SETM( "use_up" );
    	} else if ( facing & MOVE_RIGHT ) {
            SETM( "use_right_gun" );
    	} else if ( facing & MOVE_LEFT ) {
            SETM( "use_left_gun" );
        }
    } else {
        if ( facing & MOVE_DOWN ) {
            mat = anim->getMat( "swingbat_down" );
    	} else if ( facing & MOVE_UP ) {
            mat = anim->getMat( "swingbat_up" );
    	} else if ( facing & MOVE_RIGHT ) {
            mat = anim->getMat( "swingbat_right" );
    	} else if ( facing & MOVE_LEFT ) {
            mat = anim->getMat( "swingbat_left" );
        }
    }


    #undef SETM
    return mat;
}

#if 0
/*
====================
 Player_t::setupDrawSurfs
====================
*/
void Player_t::setupDrawSurfs ( void ) {
	if ( anim ) // already have it
		return;
	anim = new animSet_t();
	animation_t * A = NULL;

	A = anim->startAnim();
	A->init( 4, "left" );
	A->frames[0] = materials.FindByName( "gfx/SPRITE/HOODY/hoodyblk00_L.tga" );
	A->frames[1] = materials.FindByName( "gfx/SPRITE/HOODY/hoodyblk01_L.tga" );
	A->frames[2] = materials.FindByName( "gfx/SPRITE/HOODY/hoodyblk02_L.tga" );
	A->frames[3] = materials.FindByName( "gfx/SPRITE/HOODY/hoodyblk03_L.tga" );
	A = anim->startAnim();
	A->init( 4, "right" );
	A->frames[0] = materials.FindByName( "gfx/SPRITE/HOODY/hoodyblk00_R.tga" );
	A->frames[1] = materials.FindByName( "gfx/SPRITE/HOODY/hoodyblk01_R.tga" );
	A->frames[2] = materials.FindByName( "gfx/SPRITE/HOODY/hoodyblk02_R.tga" );
	A->frames[3] = materials.FindByName( "gfx/SPRITE/HOODY/hoodyblk03_R.tga" );
	A = anim->startAnim();
	A->init( 4, "up" );
	A->frames[0] = materials.FindByName( "gfx/SPRITE/HOODY/hoodyblk00_U.tga" );
	A->frames[1] = materials.FindByName( "gfx/SPRITE/HOODY/hoodyblk01_U.tga" );
	A->frames[2] = materials.FindByName( "gfx/SPRITE/HOODY/hoodyblk02_U.tga" );
	A->frames[3] = materials.FindByName( "gfx/SPRITE/HOODY/hoodyblk03_U.tga" );
	A = anim->startAnim();
	A->init( 4, "down" );
	A->frames[0] = materials.FindByName( "gfx/SPRITE/HOODY/hoodyblk00_D.tga" );
	A->frames[1] = materials.FindByName( "gfx/SPRITE/HOODY/hoodyblk01_D.tga" );
	A->frames[2] = materials.FindByName( "gfx/SPRITE/HOODY/hoodyblk02_D.tga" );
	A->frames[3] = materials.FindByName( "gfx/SPRITE/HOODY/hoodyblk03_D.tga" );
	A = anim->startAnim();
	A->init( 1, "standingLeft" );
	A->frames[0] = materials.FindByName( "gfx/SPRITE/HOODY/hoodyblk_SL.tga" );
	A = anim->startAnim();
	A->init( 1, "standingRight" );
	A->frames[0] = materials.FindByName( "gfx/SPRITE/HOODY/hoodyblk_SR.tga" );
	A = anim->startAnim();
	A->init( 1, "standingUp" );
	A->frames[0] = materials.FindByName( "gfx/SPRITE/HOODY/hoodyblk_SU.tga" );
	A = anim->startAnim();
	A->init( 1, "standingDown" );
	A->frames[0] = materials.FindByName( "gfx/SPRITE/HOODY/hoodyblk_SD.tga" );
	A = anim->startAnim();
	A->init( 12, "tap_foot" );
	A->frames[0] = materials.FindByName("gfx/SPRITE/HOODY/hoodyblk00_TAP.tga" );
	A->frames[1] = materials.FindByName("gfx/SPRITE/HOODY/hoodyblk00_TAP.tga" );
	A->frames[2] = materials.FindByName("gfx/SPRITE/HOODY/hoodyblk00_TAP.tga" );
	A->frames[3] = materials.FindByName("gfx/SPRITE/HOODY/hoodyblk00_TAP.tga" );
	A->frames[4] = materials.FindByName("gfx/SPRITE/HOODY/hoodyblk01_TAP.tga" );
	A->frames[5] = materials.FindByName("gfx/SPRITE/HOODY/hoodyblk00_TAP.tga" );
	A->frames[6] = materials.FindByName("gfx/SPRITE/HOODY/hoodyblk01_TAP.tga" );
	A->frames[7] = materials.FindByName("gfx/SPRITE/HOODY/hoodyblk00_TAP.tga" );
	A->frames[8] = materials.FindByName("gfx/SPRITE/HOODY/hoodyblk01_TAP.tga" );
	A->frames[9] = materials.FindByName("gfx/SPRITE/HOODY/hoodyblk00_TAP.tga" );
	A->frames[10] = materials.FindByName("gfx/SPRITE/HOODY/hoodyblk00_TAP.tga");
	A->frames[11] = materials.FindByName("gfx/SPRITE/HOODY/hoodyblk00_TAP.tga");
	A->setMSPF( 200 );

	A = anim->startAnim();
	A->init( 7, "death" );
	A->frames[0]=materials.FindByName("gfx/SPRITE/HOODY/hoodyblkDETH01_R.tga" );
	A->frames[1]=materials.FindByName("gfx/SPRITE/HOODY/hoodyblkDETH02_R.tga" );
	A->frames[2]=materials.FindByName("gfx/SPRITE/HOODY/hoodyblkDETH03_R.tga" );
	A->frames[3]=materials.FindByName("gfx/SPRITE/HOODY/hoodyblkDETH04_R.tga" );
	A->frames[4]=materials.FindByName("gfx/SPRITE/HOODY/hoodyblkDETH05_R.tga" );
	A->frames[5]=materials.FindByName("gfx/SPRITE/HOODY/hoodyblkDETH06_R.tga" );
	A->frames[6]=materials.FindByName("gfx/SPRITE/HOODY/hoodyblkDETH07_R.tga" );

	// default starting sprite
	mat = anim->getMat( "standingDown" );
}

/*
====================
 Player_t::_getMat
====================
*/
material_t * Player_t::_getMat( void ) {

	if ( !anim && !mat )
		return NULL;

	if ( paused )
		return mat;

	// player chooses animation based on state: 
	// 	which direction?  am I shooting?  am I standing?  am I dead? 

	static unsigned int waiting = 0;
	static int tap;

	if ( 0 == waiting ) {
		waiting = now();
	} else if ( shoot.shooting ) {
		waiting = now();
	} else if ( isDead() ) {
		waiting = now();
	}

    if ( platform_type->integer() == 2 ) {
        if ( facing ) {
            facing = MOVE_UP;
        }
    }

	if ( isDead() ) {
		if ( PLAYER_START_DYING == state ) {
			mat = anim->getMat( "death", 0 );
		} else if ( PLAYER_DYING == state ) { 
			mat = anim->getMat( "death" );
		} else {
			mat = anim->getMat( "death", 6 );
		}
	} else if ( facing & MOVE_LEFT ) {
		facingLast = MOVE_LEFT;
		waiting = now();
		mat = anim->getMat( "left" );
	} else if ( facing & MOVE_RIGHT ) {
		facingLast = MOVE_RIGHT;
		waiting = now();
		mat = anim->getMat( "right" );
	} else if ( facing & MOVE_UP ) {
		facingLast = MOVE_UP;
		waiting = now();
		mat = anim->getMat( "up" );
	} else if ( facing & MOVE_DOWN ) {
		facingLast = MOVE_DOWN;
		waiting = now();
		mat = anim->getMat( "down" );

	// not moving, 
	} else {

		if ( now() - waiting > waitTime && !shoot.shooting ) {
			mat = anim->getMat( "tap_foot" ); 
			int fn = anim->frameNo();
			if ( fn == 5 || fn == 7 || fn == 9 ) {
				if ( tap )
					S_StartLocalSound( tap_foot );
				tap = 0;
			} else {
				tap = 1;
			}
		} else if ( now() - waiting < 2000 ) {
			return mat;
		} else {
			switch( facingLast ) {
			case MOVE_UP:
				mat = anim->getMat( "standingUp" );
				break;
			case MOVE_LEFT:
				mat = anim->getMat( "standingLeft" ); 
				break;
			case MOVE_RIGHT:
				mat = anim->getMat( "standingRight" ); 
				break;
			default:
				mat = anim->getMat( "standingDown" ); 
				break;
			}
		}
	}

	return mat;
}
#endif

/*
====================
 G_SpawnPlayer
====================
*/
// called at the beginning of a map
void G_SpawnPlayer ( void ) {
	Area_t *A = NULL;

	if ( world.curArea ) {
		A = world.curArea;
	} else
		return;

	// find the world Spawn
	node_c<Entity_t*> *nE = A->entities.gethead();
	Spawn_t *s = NULL;
	while( nE ) {
		s = dynamic_cast<Spawn_t*>( nE->val );
		if ( s && s->type == ST_WORLD ) {
			break;
		}
		nE = nE->next;
	}

	// set the player pos & size to match spawn poly
	if ( s && s->type == ST_WORLD ) {
		
		if ( s->set_view ) {
			M_GetMainViewport()->SetView( s->view[0], s->view[1], s->zoom );
		}

		// we want to set poly size and location to match spawn, 
		// but scale the colModel instead of resetting it

		// we're going from CM2 to CM1.  we have CM2, and B1 & B2, 
		float B1[4];
		float B2[4];
		float S[4];
		float CM2[4], CM1[4];

		poly_t *sp = &s->poly;
		sp->toAABB( B1 );
		player.poly.toAABB( B2 );

		poly_t psav;
		COPY_POLY( &psav, &player.poly );

		// set player location and width to the spawn's
		player.poly.x = sp->x;
		player.poly.y = sp->y;
		player.poly.w = sp->w;
		player.poly.h = sp->h;
		player.poly.angle = 0;

		// set pl_speed gvar from world spawn
		if ( s->pl_speed[0] ) {
			pl_speed->set( "pl_speed", s->pl_speed, 0 );
		}

		// test for empty colModel, in that case, provide one, a match to poly
		if ( player.col ) {
			float *b = player.col->box ;
			if ( ISZERO( b[0] ) && ISZERO( b[1] ) && ISZERO( b[2] ) && ISZERO( b[3] ) ) {
				player.populateGeometry(1);
				return ;
			}
		}  

		// else, just populate geometry
		player.populateGeometry(0);

		// scale player colModel to spawn size
		if ( player.col ) {

			// widths of both
			float B2w[2] = { B2[2] - B2[0], B2[3] - B2[1] };
			float B1w[2] = { B1[2] - B1[0], B1[3] - B1[1] };

			COPY4( CM2, player.col->box );

			// 2, 2d scale vectors
			S[0] = ( CM2[0] - B2[0] ) / B2w[0];
			S[1] = ( CM2[1] - B2[1] ) / B2w[1];
			S[2] = ( CM2[2] - B2[2] ) / B2w[0];
			S[3] = ( CM2[3] - B2[3] ) / B2w[1];
	
			CM1[0] = B1[0] + S[0] * B1w[0];
			CM1[1] = B1[1] + S[1] * B1w[1];
			CM1[2] = B1[2] + S[2] * B1w[0];
			CM1[3] = B1[3] + S[3] * B1w[1];

			float *cb = player.col->box;
			cb[0] = CM1[0];
			cb[1] = CM1[1];
			cb[2] = CM1[2];
			cb[3] = CM1[3];
		}

		// shift 
		float sh[2] = { player.poly.x - psav.x, player.poly.y - psav.y };
		player.shiftClip( sh );
		player.shiftTrig( sh );

	}
}

/*
====================
 G_InitPlayer
====================
*/
// put all those here
void G_InitPlayer( void ) {

	// override anything already there
	player.setupDrawSurfs();

	if ( !player.col ) {
		player.col = new colModel_t ( player.poly );
	}

	G_SpawnPlayer();

	player.shoot.msPerShot = g_fireRate->integer();

	G_PlayerStartState();
}

/*
====================
 Player_t::RobotHandler
====================
*/
void Player_t::RobotHandler( Robot_t *R ) {
	if ( !R )
		return;

	// player already mortally wounded
	if ( state > PLAYER_NORMAL )
		return;

	// robot is in a non-interacting state
	if ( R->state > ROBOT_DIRECTIONAL )
		return;
	
	// check the clips
	if ( !R->clipping || !this->clipping || !AABB_intersect( R->wish, wish ) ) {
		return;
	}

	// check if we are already in contact with this robot
	if ( !shock_timer.check() )
		return;

	// set the timer
	shock_timer.set( this->shock_delay );
	
	// start the sound
	// FIXME: check if the sound is already running, .. erm, you'll have to poll the shock_timer, if it's
	// on and the sound stops running, start it again. hrm.  
	S_StartLocalSound( robot_shock );

	// ok we clip, now..

	// take damage
	if ( (this->hitpoints -= R->damage) <= 0 ) {
		// full damage
		state = PLAYER_START_DYING;
		delete_me |= 0x1;
		timer.set( death_time );
		console.Printf( "Player killed by Robot" );
		S_StartLocalSound( wilhelm_scream );
	}
	else {
		console.Printf( "Player collided with Robot, %d hitpoints left", hitpoints );
	}

	float *cor = G_CalcBoxCollision( wish, R->wish );
	float sav[2] = { cor[0], cor[1] };

	// throw bot backwards inertia from hit
	//cor = G_CalcBoxCollision( R->wish, wish );
	//R->wishMove( cor[0] * 1.0f, cor[1] * 1.0f );
	
	// throw player backwards inertia from hit
	//wishMove( sav[0] * 2.0f, sav[1] * 2.0f );
	// if you do a wish move here it makes the player jump too far.  
	// note to self, you need a better movement system.
}

/*
====================
 Player_t::ProjectileHandler
====================
*/
void Player_t::ProjectileHandler( Projectile_t *P ) {
	if ( !P )
		return;

	// already mortally wounded
	if ( state > PLAYER_NORMAL )
		return;

	// make sure the projectile hasn't already hit us (is it exploding?)
	if ( P->state == PROJ_EXPLODING )
		return;
	
	// check the clips
	if ( !P->clipping || !this->clipping || !AABB_intersect( P->wish, wish ) ) {
		return;
	}

	// ok we clip, now..

	// take damage
	if ( (this->hitpoints -= P->damage) <= 0 ) {
		// full damage
		state = PLAYER_START_DYING;
		delete_me |= 0x1;
		timer.set( death_time );
		console.Printf( "Player killed by projectile" );
		S_StartLocalSound( wilhelm_scream );
	}
	else { 
		S_StartLocalSound( player_hit );
		console.Printf( "Player hit by projectile, %d hitpoints left", hitpoints );
	}

	// throw player backwards inertia from hit
//	float *cor = G_CalcBoxCollision( wish, P->wish );
	//float *cor = G_CalcBoxCollision( wish, P->clip );
	//wishMove( cor[0] * 0.5f, cor[1] * 0.5f );
	// fuck that, it makes it too hard to shoot

	// tell the projectile to explode
	P->explode();
}

#define C(X) ( !strcmp( X, I->mat ) )

static int client_id;

/*
====================
 Player_t::ItemHandler
====================
*/
void Player_t::ItemHandler( Item_t * I ) {
	if ( !I )
		return;

	I->blockRemove();
	I->collidable = false; // remove from further interactions 
	world.force_rebuild = true; // rebuild display lists w/o it in it

	// some powerups we use right away
	
	// some powerups we pick up.  I'm thinking in RPGs that you pick up all
	// ITEMS, and then use them , either through menus or HotKeys. for now,
	// we'll pick them all up.  most likely there may be 1 or 2 things, like
	// vials and armor shards that you run into for powerup, everything else
	// you pick up and manage and use (return key) '[' & ']' to cycle.

	items.add ( I );

	if ( !client_id ) {
		client_id = floating.requestClientID();
	}
	floating.AddCoordText( 200.f, M_Height() - 200.f, I->name, 30, 3000, client_id );
	console.Printf( "Player got %s", I->name );

	// LATER: do a switch for different sounds for different items.
	switch ( I->item_id ) {
/*	case ITEM_TURKEY_LEG:
		S_StartLocalSound( success_sound[((rand()>>3)&1)+1] );
		break; */
	default:
		S_StartLocalSound( success_sound[0] );
		break;
	}
}

/*
====================
 Player_t::_handle
====================
*/
// handle a collision with an unknown entity
void Player_t::_handle( Entity_t **ent ) {

	switch ( (*ent)->entType ) {
	case ET_ROBOT: return RobotHandler( dynamic_cast<Robot_t*>(*ent) ); 
	case ET_PROJECTILE: return ProjectileHandler( dynamic_cast<Projectile_t*>(*ent) );
	case ET_ITEM: return ItemHandler( dynamic_cast<Item_t*>(*ent) );
    case ET_ZOMBIE: return ZombieHandler( dynamic_cast<Zombie_t*>(*ent) );
	default:
		break;
	}
}
	
#if 0
	// check extern entity trigger  with player's clip intention 
	bool triggered = (*ent)->triggerable && AABB_intersect( (*ent)->trig, wish );
	int result = RESULT_NORMAL;
	if ( triggered ) {
		Entity_t *player_p = this; 
		result = (*ent)->trigger( &player_p );
	}

	// trig result can void a collision.  (like in a teleport)
	if ( RESULT_NOCLIP == result )
		return;

	// physical collision with intended move
	if ( (*ent)->clipping && AABB_intersect( (*ent)->clip, wish ) ) {
		float *correction = G_CalcBoxCollision( wish, (*ent)->clip );
		wish[0] += correction[0];
		wish[1] += correction[1];
	}
#endif


/*
====================
 P_FinishMove
====================
*/
void P_FinishMove( void ) {

	if ( player.isDead() ) {
		// always update the lerpFrames
		player.lerp.update( player.aabb[0], player.aabb[1] );
		// the controller updates the camera
		controller.move( 0.f, 0.f );
		return;
	}

	// respond/adjust to entity collisions
	player.adjustFromEntityCollisions();

	float mv[2] = {	player.wish[0] - player.clip[0], 
					player.wish[1] - player.clip[1] };

	// keep track of which way we are facing
	//player.setFacing( mv ); 
	// MOVED to wishMove

	// are we firing weapon this frame?
	if ( player.shoot.check() ) {
		player.fireWeapon();
	}

	// tell the controller to check the blocks and make the move if possible
	controller.move( mv[0], mv[1] );

	// save our newly computed location frame
	player.lerp.update( player.aabb[0], player.aabb[1] );

	// reset 
	AABB_zero( player.wish );
}

/*
====================
 Player_t::think
====================
*/
// timed stuff, state stuff
void Player_t::think( void ) {
	if ( PLAYER_NORMAL == state )
		return;

	// death animations
	switch ( state ) {
	case PLAYER_START_DYING:
		state = PLAYER_DYING;
		break;
	case PLAYER_DYING:
		if ( timer.delta() > 600 ) {
			state = PLAYER_DEAD;
			timer.set( 500 ); // extra time to just lay there
		}
		break;
	case PLAYER_DEAD:
		if ( timer.check() ) {
			effects.startFade( FX_FADEOUT, 3000, 2000 );
			timer.set( 3000 + 2000 );
			state = PLAYER_RESPAWNING;
			// still need to pause robot from shooting again
			// this stops background track as well
			S_StopAllSounds(); 
			// try this
			S_Mute();
			S_UnMute(); // this works, but you still have to stop the robots
						// from shooting
			S_StartBackGroundTrack( "zpak/music/nestor4.wav", NULL, gtrue );
			CL_PauseWithMusic(); // stops thinkers, lets track play
		}
		break;
	case PLAYER_RESPAWNING:
		if ( timer.check() ) {
			//S_UnMute(); happens in CL_UnPause
			CL_UnPause();
			CL_ReloadMap();
			// well, now need to restart background track
		}

		// obviously need to save stats, reload properly after ...
		// not to mention implement a working, global-wide game pause
		break;
	}
}

void Player_t::ZombieHandler( Zombie_t *Z ) {
	if ( !Z )
        return;

	// player already mortally wounded
	if ( state > PLAYER_NORMAL )
        return;

	// if either isn't clipping or we're not touching at all..
	if ( !Z->clipping || !this->clipping || !AABB_intersect( Z->wish, wish ) ) {
        return;
	}

    // we can walk over him if he's dead
    if ( Z->state >= Z_START_DYING ) {
        return;
    }

	// zombie is not chasing/attacking, we can touch it w/o damage; just clip
	if ( Z->state < Z_CHASING ) { 
		// our correction
		float *correction = G_CalcBoxCollision( wish, Z->wish );

		// copy it
		float ours[2] = { correction[0], correction[1] };

		// their correction
		correction = G_CalcBoxCollision( Z->wish, wish ) ;

		// move both out of the incident area
		Z->wishMove( correction[0], correction[1] );
		this->wishMove( ours[0], ours[1] );
        return;

        // FIXME: what were these?
        //Z->handleEntityCollision( this );
        //this->handleEntityCollision( Z );
    }

	// this stops us from taking damage every frame, just take damage on first
    //  contact, then wait shock_delay to take more
    if ( !shock_timer.check() )
        return;

	// set the timer
    shock_timer.set( this->shock_delay );
	
    //S_StartLocalSound( player_hit );

    // ok we get hit now..

    // take damage
    if ( !godmode->integer() ) {
        this->hitpoints -= Z->hitdamage;
    }

    if ( hitpoints <= 0 ) {
        // full damage
        state = PLAYER_START_DYING;
        delete_me |= 0x1;
        timer.set( death_time );
        console.Printf( "Player killed by Zombie" );
        S_StartLocalSound( wilhelm_scream );
    }
    else {
        // FIXME: ultimately don't report this to console, just death; remove
        //  when we actually have a HUD
        if ( !godmode->integer() ) 
            console.Printf( "Zombie attacked Player. %d hitpoints left", hitpoints );
        else
            console.Printf( "Zombie attacked Player: no effect! [godmode]" );

        // knock the player back
        float *correction = G_CalcBoxCollision( wish, Z->wish );
        this->wishMove( correction[0], correction[1] );
    }
}

/*
====================
 
    can be + or - (something give life, enemies take it)
====================
*/
void Player_t::adjustHitpoints( int hp ) {
    // make adjustment
    // checks hp vs. ceiling (given by level & bonuses)
    // checks if <= 0, if it is 0, start death procedure
}

/*
====================
 pickupItem 
====================
*/
int Player_t::pickupItem( Item_t * I ) {
    // check to see if new item addition will put us over weight capacity
        // print message, return 0

    // check to see if all our item slots are full
        // print message, return 0

    // are some items off-limits? can't be carried for any reason?
        // print message, return 0

    // otherwise, pickup item, print message, return 1

    return 0;
}

