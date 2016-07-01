// g_zombie.cpp
//
#include "g_entity.h"
#include "../client/cl_console.h"
#include "../map/m_area.h"


gvar_c *g_zombieSpeed = Gvar_Get( "g_zombieSpeed", "310", 0 ); // worldcoord per Second
gvar_c *g_zombieAnimSpeed = Gvar_Get( "g_zombieAnimSpeed", "360", 0 ); // ms per frame
gvar_c *g_zombieLogicTic = Gvar_Get( "g_zombieLogicTic", "30", 0 ); // 
extern gvar_c *g_scanRadius; // in game.cpp
gvar_c *g_zombieLOS = Gvar_Get( "g_zombieLOS", "1" , 0 );
gvar_c *g_attackRadius = Gvar_Get( "g_attackRadius", "500", 0 );
gvar_c *g_zombieChasingMultiplier = Gvar_Get( "g_zombieChasingMultiplier", "2.4", 0 );


// static members
int             Zombie_t::id = 0;
animSet_t *     Zombie_t::zombieAnimSet = NULL;



/*
====================
 _getMat

set the direction by walkDir
====================
*/
material_t * Zombie_t::_getMat() 
{
    lastAnim = curAnim;
    lastMat = mat;

    if ( state == Z_TURNING && !wasAttacking ) {
        if ( walkDir & Z_LEFT ) {
            curAnim = anim->getAnim( "left" );
        } else if ( walkDir & Z_RIGHT ) {
            curAnim = anim->getAnim( "right" );
        } else if ( walkDir & Z_UP ) {
            curAnim = anim->getAnim( "up" );
        } else if ( walkDir & Z_DOWN ) {
            curAnim = anim->getAnim( "down" );
        }
    } else if ( state == Z_WALKING && !wasAttacking ) {
        if ( walkDir & Z_LEFT ) {
            curAnim = anim->getAnim( "left" );
        } else if ( walkDir & Z_RIGHT ) {
            curAnim = anim->getAnim( "right" );
        } else if ( walkDir & Z_UP ) {
            curAnim = anim->getAnim( "up" );
        } else if ( walkDir & Z_DOWN ) {
            curAnim = anim->getAnim( "down" );
        }
    } else if ( (state == Z_CHASING||state==Z_CHASING_BLIND) && !wasAttacking ) {
        if ( walkDir & Z_LEFT ) {
            curAnim = anim->getAnim( "chasing_left" );
        } else if ( walkDir & Z_RIGHT ) {
            curAnim = anim->getAnim( "chasing_right" );
        } else if ( walkDir & Z_UP ) {
            curAnim = anim->getAnim( "chasing_up" );
        } else if ( walkDir & Z_DOWN ) {
            curAnim = anim->getAnim( "chasing_down" );
        }
    } else if ( state == Z_ATTACKING || wasAttacking ) {
        if ( walkDir & Z_LEFT ) {
            curAnim = anim->getAnim( "attack_left" );
        } else if ( walkDir & Z_RIGHT ) {
            curAnim = anim->getAnim( "attack_right" );
        } else if ( walkDir & Z_UP ) {
            curAnim = anim->getAnim( "attack_up" );
        } else if ( walkDir & Z_DOWN ) {
            curAnim = anim->getAnim( "attack_down" );
        }
    } else if ( state == Z_START_DYING ) {
        if ( walkDir & (Z_LEFT|Z_RIGHT) ) {
            curAnim = anim->getAnim( "dying_horz" );
        } else {
            curAnim = anim->getAnim( "dying_vert" );
        }
    } else if ( state == Z_DYING ) {
        if ( walkDir & (Z_LEFT|Z_RIGHT) ) {
            curAnim = anim->getAnim( "dying_horz" );
        } else {
            curAnim = anim->getAnim( "dying_vert" );
        }
    } else {
        curAnim = lastAnim;
    }

    if ( !mat ) {
        mat = lastMat;
    } else {
        if ( curAnim ) {
            // just started attacking, start animation on the first frame
            if ( state == Z_ATTACKING && attackStart && !wasAttacking ) {
                mat = curAnim->advance( 0 );
            // just started dying
            } else if ( state == Z_START_DYING ) {
                mat = curAnim->advance( 0 );
            } else {
                if ( state != Z_STANDING && state != Z_TURNING )
                    mat = curAnim->advance();
            }
        }
    }

    attackStart = false;

    if ( wasAttacking && anim->frameNo() == 9 ) 
        wasAttacking = false;

    if ( state == Z_DYING || state == Z_DEAD ) {
        if ( anim && curAnim && anim->frameNo() == curAnim->total - 1 ) {
            mat = curAnim->advance( curAnim->total - 1 );
        }
    }

    return mat;
}


/*
====================
 think
====================
*/
void Zombie_t::think( void ) {

    float amt = 1.f / sv_fps->value() * g_zombieSpeed->value(); 

    handleState(); // state changes here

    if ( state < Z_WALKING ) {
        return;
    }

    if ( state >= Z_START_DYING ) {
        return;
    }

    if ( state == Z_CHASING || state == Z_CHASING_BLIND ) {
        amt *= g_zombieChasingMultiplier->value();
    } else if ( state == Z_ATTACKING ) {
        //amt *= 0.85;
    }

    // do chasing/attacking move
    if ( state > Z_WALKING ) {
        float v[2];
        if ( state == Z_CHASING_BLIND ) {
            v[0] = playerLastSeenXY[0] - aabb[0];
            v[1] = playerLastSeenXY[1] - aabb[1];
        } else {
            v[0] = player.aabb[0] - aabb[0];
            v[1] = player.aabb[1] - aabb[1];
        }
        M_Normalize2d( v );

        int v0gtv1 = fabs(v[0]) > fabs(v[1]) ? 1 : 0;
    
        if ( v0gtv1 ) {
            if ( v[0] > 0 ) {
                doMove( amt, 0 );
            } else {
                doMove( -amt, 0 );
            }
        } else {
            if ( v[1] > 0 ) {
                doMove( 0, amt );
            } else {
                doMove( 0, -amt );
            }
        }
        return;
    }

    // do walking (directional) move
    switch ( walkDir ) {
    case Z_UP:
        doMove( 0, amt );
        break;
    case Z_RIGHT:
        doMove( amt, 0 );
        break;
    case Z_DOWN:
        doMove( 0, -amt );
        break;
    case Z_LEFT:
        doMove( -amt, 0 );
        break;
    }
}

/*
====================
 _handle
====================
*/
void Zombie_t::_handle( Entity_t ** epp ) {
	switch ( (*epp)->entType ) {
	//case ET_ROBOT: return RobotHandler( dynamic_cast<Robot_t*>(*epp) );
	case ET_PROJECTILE: return ProjectileHandler( dynamic_cast<Projectile_t*>(*epp));
    case ET_ZOMBIE: return ZombieHandler( dynamic_cast<Zombie_t*>(*epp) );
	default:
		break;
	}
}

// could be just standing in a daze, this would activate them into moving
//  ...maybe, per-chance, it would activate them into moving
int Zombie_t::trigger( Entity_t ** E ) {
    return 1;
}


void Zombie_t::buildAnims( void ) {
    if ( zombieAnimSet ) {
        return;
    }

    int speed = g_zombieAnimSpeed->integer();
	zombieAnimSet = new animSet_t();

	animation_t *A = zombieAnimSet->startAnim();
	A->init( 16, "left" );
	A->setMSPF( speed ); 
	A->frames[0] = materials.FindByName( "gfx/zombie/zleft01.tga" );
	A->frames[1] = materials.FindByName( "gfx/zombie/zleft02.tga" );
	A->frames[2] = materials.FindByName( "gfx/zombie/zleft03.tga" );
	A->frames[3] = materials.FindByName( "gfx/zombie/zleft02.tga" );
	A->frames[4] = materials.FindByName( "gfx/zombie/zleft05.tga" );
	A->frames[5] = materials.FindByName( "gfx/zombie/zleft02.tga" );
	A->frames[6] = materials.FindByName( "gfx/zombie/zleft03.tga" );
	A->frames[7] = materials.FindByName( "gfx/zombie/zleft02.tga" );
	A->frames[8] = materials.FindByName( "gfx/zombie/zleft01.tga" );
	A->frames[9] = materials.FindByName( "gfx/zombie/zleft02.tga" );
	A->frames[10] = materials.FindByName( "gfx/zombie/zleft03.tga" );
	A->frames[11] = materials.FindByName( "gfx/zombie/zleft02.tga" );
	A->frames[12] = materials.FindByName( "gfx/zombie/zleft13.tga" );
	A->frames[13] = materials.FindByName( "gfx/zombie/zleft02.tga" );
	A->frames[14] = materials.FindByName( "gfx/zombie/zleft03.tga" );
	A->frames[15] = materials.FindByName( "gfx/zombie/zleft02.tga" );

	A = zombieAnimSet->startAnim();
	A->init( 16, "right" );
	A->setMSPF( speed ); 
	A->frames[0] = materials.FindByName( "gfx/zombie/zright01.tga" );
	A->frames[1] = materials.FindByName( "gfx/zombie/zright02.tga" );
	A->frames[2] = materials.FindByName( "gfx/zombie/zright03.tga" );
	A->frames[3] = materials.FindByName( "gfx/zombie/zright02.tga" );
	A->frames[4] = materials.FindByName( "gfx/zombie/zright05.tga" );
	A->frames[5] = materials.FindByName( "gfx/zombie/zright02.tga" );
	A->frames[6] = materials.FindByName( "gfx/zombie/zright03.tga" );
	A->frames[7] = materials.FindByName( "gfx/zombie/zright02.tga" );
	A->frames[8] = materials.FindByName( "gfx/zombie/zright01.tga" );
	A->frames[9] = materials.FindByName( "gfx/zombie/zright02.tga" );
	A->frames[10] = materials.FindByName( "gfx/zombie/zright03.tga" );
	A->frames[11] = materials.FindByName( "gfx/zombie/zright02.tga" );
	A->frames[12] = materials.FindByName( "gfx/zombie/zright13.tga" );
	A->frames[13] = materials.FindByName( "gfx/zombie/zright02.tga" );
	A->frames[14] = materials.FindByName( "gfx/zombie/zright03.tga" );
	A->frames[15] = materials.FindByName( "gfx/zombie/zright02.tga" );

	A = zombieAnimSet->startAnim();
	A->init( 16, "up" );
	A->setMSPF( speed ); 
	A->frames[0] = materials.FindByName( "gfx/zombie/zup01.tga" );
	A->frames[1] = materials.FindByName( "gfx/zombie/zup02.tga" );
	A->frames[2] = materials.FindByName( "gfx/zombie/zup03.tga" );
	A->frames[3] = materials.FindByName( "gfx/zombie/zup02.tga" );
	A->frames[4] = materials.FindByName( "gfx/zombie/zup05.tga" );
	A->frames[5] = materials.FindByName( "gfx/zombie/zup02.tga" );
	A->frames[6] = materials.FindByName( "gfx/zombie/zup03.tga" );
	A->frames[7] = materials.FindByName( "gfx/zombie/zup02.tga" );
	A->frames[8] = materials.FindByName( "gfx/zombie/zup01.tga" );
	A->frames[9] = materials.FindByName( "gfx/zombie/zup02.tga" );
	A->frames[10] = materials.FindByName( "gfx/zombie/zup03.tga" );
	A->frames[11] = materials.FindByName( "gfx/zombie/zup02.tga" );
	A->frames[12] = materials.FindByName( "gfx/zombie/zup13.tga" );
	A->frames[13] = materials.FindByName( "gfx/zombie/zup02.tga" );
	A->frames[14] = materials.FindByName( "gfx/zombie/zup03.tga" );
	A->frames[15] = materials.FindByName( "gfx/zombie/zup02.tga" );

	A = zombieAnimSet->startAnim();
	A->init( 16, "down" );
	A->setMSPF( speed ); 
	A->frames[0] = materials.FindByName( "gfx/zombie/zdown01.tga" );
	A->frames[1] = materials.FindByName( "gfx/zombie/zdown02.tga" );
	A->frames[2] = materials.FindByName( "gfx/zombie/zdown03.tga" );
	A->frames[3] = materials.FindByName( "gfx/zombie/zdown02.tga" );
	A->frames[4] = materials.FindByName( "gfx/zombie/zdown05.tga" );
	A->frames[5] = materials.FindByName( "gfx/zombie/zdown02.tga" );
	A->frames[6] = materials.FindByName( "gfx/zombie/zdown03.tga" );
	A->frames[7] = materials.FindByName( "gfx/zombie/zdown02.tga" );
	A->frames[8] = materials.FindByName( "gfx/zombie/zdown01.tga" );
	A->frames[9] = materials.FindByName( "gfx/zombie/zdown02.tga" );
	A->frames[10] = materials.FindByName( "gfx/zombie/zdown03.tga" );
	A->frames[11] = materials.FindByName( "gfx/zombie/zdown02.tga" );
	A->frames[12] = materials.FindByName( "gfx/zombie/zdown13.tga" );
	A->frames[13] = materials.FindByName( "gfx/zombie/zdown02.tga" );
	A->frames[14] = materials.FindByName( "gfx/zombie/zdown03.tga" );
	A->frames[15] = materials.FindByName( "gfx/zombie/zdown02.tga" );

    // chasing
	A = zombieAnimSet->startAnim();
	A->init( 2, "chasing_up" );
	A->setMSPF( speed ); 
	A->frames[0] = materials.FindByName( "gfx/zombie/zup05.tga" );
	A->frames[1] = materials.FindByName( "gfx/zombie/zup10.tga" );
	A = zombieAnimSet->startAnim();
	A->init( 2, "chasing_right" );
	A->setMSPF( speed ); 
	A->frames[0] = materials.FindByName( "gfx/zombie/zright05.tga" );
	A->frames[1] = materials.FindByName( "gfx/zombie/zright10.tga" );
	A = zombieAnimSet->startAnim();
	A->init( 2, "chasing_down" );
	A->setMSPF( speed ); 
	A->frames[0] = materials.FindByName( "gfx/zombie/zdown05.tga" );
	A->frames[1] = materials.FindByName( "gfx/zombie/zdown10.tga" );
	A = zombieAnimSet->startAnim();
	A->init( 2, "chasing_left" );
	A->setMSPF( speed ); 
	A->frames[0] = materials.FindByName( "gfx/zombie/zleft05.tga" );
	A->frames[1] = materials.FindByName( "gfx/zombie/zleft10.tga" );

    // attack
	A = zombieAnimSet->startAnim();
	A->init( 10, "attack_right" );
	A->setMSPF( 60 ); 
	A->frames[0] = materials.FindByName( "gfx/zombie/zatkright01.tga" );
	A->frames[1] = materials.FindByName( "gfx/zombie/zatkright02.tga" );
	A->frames[2] = materials.FindByName( "gfx/zombie/zatkright03.tga" );
	A->frames[3] = materials.FindByName( "gfx/zombie/zatkright04.tga" );
	A->frames[4] = materials.FindByName( "gfx/zombie/zatkright05.tga" );
	A->frames[5] = materials.FindByName( "gfx/zombie/zatkright06.tga" );
	A->frames[6] = materials.FindByName( "gfx/zombie/zatkright07.tga" );
	A->frames[7] = materials.FindByName( "gfx/zombie/zatkright08.tga" );
	A->frames[8] = materials.FindByName( "gfx/zombie/zatkright09.tga" );
	A->frames[9] = materials.FindByName( "gfx/zombie/zatkright09.tga" );
	A = zombieAnimSet->startAnim();
	A->init( 10, "attack_down" );
	A->setMSPF( 60 ); 
	A->frames[0] = materials.FindByName( "gfx/zombie/zatkdown01.tga" );
	A->frames[1] = materials.FindByName( "gfx/zombie/zatkdown02.tga" );
	A->frames[2] = materials.FindByName( "gfx/zombie/zatkdown03.tga" );
	A->frames[3] = materials.FindByName( "gfx/zombie/zatkdown04.tga" );
	A->frames[4] = materials.FindByName( "gfx/zombie/zatkdown05.tga" );
	A->frames[5] = materials.FindByName( "gfx/zombie/zatkdown06.tga" );
	A->frames[6] = materials.FindByName( "gfx/zombie/zatkdown07.tga" );
	A->frames[7] = materials.FindByName( "gfx/zombie/zatkdown08.tga" );
	A->frames[8] = materials.FindByName( "gfx/zombie/zatkdown09.tga" );
	A->frames[9] = materials.FindByName( "gfx/zombie/zatkdown09.tga" );
	A = zombieAnimSet->startAnim();
	A->init( 10, "attack_left" );
	A->setMSPF( 60 ); 
	A->frames[0] = materials.FindByName( "gfx/zombie/zatkleft01.tga" );
	A->frames[1] = materials.FindByName( "gfx/zombie/zatkleft02.tga" );
	A->frames[2] = materials.FindByName( "gfx/zombie/zatkleft03.tga" );
	A->frames[3] = materials.FindByName( "gfx/zombie/zatkleft04.tga" );
	A->frames[4] = materials.FindByName( "gfx/zombie/zatkleft05.tga" );
	A->frames[5] = materials.FindByName( "gfx/zombie/zatkleft06.tga" );
	A->frames[6] = materials.FindByName( "gfx/zombie/zatkleft07.tga" );
	A->frames[7] = materials.FindByName( "gfx/zombie/zatkleft08.tga" );
	A->frames[8] = materials.FindByName( "gfx/zombie/zatkleft09.tga" );
	A->frames[9] = materials.FindByName( "gfx/zombie/zatkleft09.tga" );
	A = zombieAnimSet->startAnim();
	A->init( 10, "attack_up" );
	A->setMSPF( 60 ); 
	A->frames[0] = materials.FindByName( "gfx/zombie/zatkup01.tga" );
	A->frames[1] = materials.FindByName( "gfx/zombie/zatkup02.tga" );
	A->frames[2] = materials.FindByName( "gfx/zombie/zatkup03.tga" );
	A->frames[3] = materials.FindByName( "gfx/zombie/zatkup04.tga" );
	A->frames[4] = materials.FindByName( "gfx/zombie/zatkup05.tga" );
	A->frames[5] = materials.FindByName( "gfx/zombie/zatkup06.tga" );
	A->frames[6] = materials.FindByName( "gfx/zombie/zatkup07.tga" );
	A->frames[7] = materials.FindByName( "gfx/zombie/zatkup08.tga" );
	A->frames[8] = materials.FindByName( "gfx/zombie/zatkup09.tga" );
	A->frames[9] = materials.FindByName( "gfx/zombie/zatkup09.tga" );

    // exploding, 

    // dying, 
	A = zombieAnimSet->startAnim();
    A->init( 6, "dying_horz" );
    A->setMSPF( 80 );
    A->frames[0] = materials.FindByName( "gfx/zombie/zdethH00.tga" );
    A->frames[1] = materials.FindByName( "gfx/zombie/zdethH01.tga" );
    A->frames[2] = materials.FindByName( "gfx/zombie/zdethH02.tga" );
    A->frames[3] = materials.FindByName( "gfx/zombie/zdethH03.tga" );
    A->frames[4] = materials.FindByName( "gfx/zombie/zdethH04.tga" );
    A->frames[5] = materials.FindByName( "gfx/zombie/zdethH05.tga" );
	A = zombieAnimSet->startAnim();
    A->init( 7, "dying_vert" );
    A->setMSPF( 80 );
    A->frames[0] = materials.FindByName( "gfx/zombie/zdethV00.tga" );
    A->frames[1] = materials.FindByName( "gfx/zombie/zdethV01.tga" );
    A->frames[2] = materials.FindByName( "gfx/zombie/zdethV02.tga" );
    A->frames[3] = materials.FindByName( "gfx/zombie/zdethV03.tga" );
    A->frames[4] = materials.FindByName( "gfx/zombie/zdethV04.tga" );
    A->frames[5] = materials.FindByName( "gfx/zombie/zdethV05.tga" );
    A->frames[6] = materials.FindByName( "gfx/zombie/zdethV06.tga" );

    // dead, 
}

void Zombie_t::setFacingFromWalkDir() {
    switch ( walkDir ) {
    case Z_UP: facing = MOVE_UP; break;
    case Z_RIGHT: facing = MOVE_RIGHT; break;
    case Z_DOWN: facing = MOVE_DOWN; break;
    case Z_LEFT: facing = MOVE_LEFT; break;
    }
}


void Zombie_t::startToWalk() {
    state = Z_WALKING;
    walkDir = 1 << (rand()%4);
    //setFacingFromWalkDir();
    walkTimer.set();
    stateTimer.set();
}

bool Zombie_t::doRandomTurn() {
    if ( !turnTimer.check() ) 
        return true;
    int d;
    do {
        d = 1 << (rand()%4);
    } while ( d == walkDir );
    walkDir = d;
    turnTimer.set( 500 + (rand()%500) );
    return false;
}

void Zombie_t::startTurning() {
    startToWalk();
    turnTimer.set( 500 + (rand()%500) );
    state = Z_TURNING;
    stateTimer.set();
}

// helps set wishMove
void Zombie_t::doMove( float x, float y ) {
    int x_gt_y = fabs(x) > fabs(y) ? 1 : 0;
    int newDir = x_gt_y ? ((x > 0) ? Z_RIGHT : Z_LEFT) : (y > 0) ? Z_UP : Z_DOWN;
    // move is a turn
    if ( newDir != walkDir ) {
        // allow the turn
        if ( turnTimer.check() ) {
            walkDir = newDir;
            turnTimer.set( 800 );
        } else {
            // cant turn yet, finagle it so they keep walking in the direction they were walking
            float z = x_gt_y ? fabs(x) : fabs(y);
            x = 0; y = 0;
            switch ( walkDir ) {
            case Z_RIGHT:   x = z;
                break;
            case Z_LEFT:    x = -z;
                break;
            case Z_UP:      y = z;
                break;
            case Z_DOWN:    y = -z;
                break;
            }
        }
    }
    wishMove( x, y );
}

int Zombie_t::reachedPlayerLastSeen( void ) {
    const float min_dist = 16.f;
    float v[2] ;
    v[0] = playerLastSeenXY[0] - aabb[0];
    v[1] = playerLastSeenXY[1] - aabb[1];
    return sqrtf(v[0] * v[0] + v[1] * v[1]) < min_dist;
}

/*
====================
 handleState
====================
*/
void Zombie_t::handleState() {

    // timing the logic frame only really matters for when we aren't dead
    if (  !logicTimer.check() && state < Z_START_DYING ) {
        return;
    }

    if ( state < Z_START_DYING )
        ScanForPlayer();

    int R = rand() % 101;

    switch ( state ) {
    case Z_STANDING:
        if ( stateTimer.time() > 4000 ) {
            if ( R > 95 ) {
                startToWalk();
            } else if ( R > 92 ) {
                state = Z_STANDING_PENSIVE;
                stateTimer.set();
            } else if ( R > 40 ) { // Z_TURNING
                startTurning();
            }
        }
        break;
    case Z_STANDING_PENSIVE:
        if ( stateTimer.time() > 2000 ) {
            startToWalk();
            if ( rand() % 2 ) {
                startTurning();
            }
        }
        break;
    case Z_TURNING:
        // FIXME: doesn't work. can't figure out why...
        if ( stateTimer.time() > 8000 && !needTurn ) { 
            if ( R > 95 ) {
                startToWalk();
            } else if ( R > 90 ) {
                state = Z_STANDING_PENSIVE;
                stateTimer.set();
            }
        } else if ( turnTimer.check() || needTurn ) {
            needTurn = doRandomTurn();
        }
        break;
    case Z_WALKING:
        if ( stateTimer.time() > 16000 ) {
            if ( R > 95 ) {
                stateTimer.set();
                state = Z_STANDING;
            }
        } else if ( in_collision && (in_collision & 2) != 2 ) {
            doRandomTurn();
            walkTimer.set( 3000 + (rand()%2200) );
        } else if ( walkTimer.check() ) {
            if ( R > 94 ) {
                doRandomTurn();
                walkTimer.set( 3000 + (rand()%2200) );
            }
        }
        break;
    case Z_CHASING:
        if ( !playerSeen ) {
            state = Z_CHASING_BLIND;
            stateTimer.set( 4000 );
        }
        break;
    case Z_CHASING_BLIND:
        if ( playerSeen ) { // found them again (oh wait this gets set in Scan)
            state = Z_CHASING;
            break;
        }
        if ( stateTimer.check() || reachedPlayerLastSeen() )
            startToWalk();
        break;
    case Z_ATTACKING:
        if ( !playerSeen ) {
            state = Z_CHASING_BLIND;
            stateTimer.set( 4000 );
        }
        break;
    case Z_START_DYING:
        state = Z_DYING;
        wasAttacking = false;
        attackStart = false; // FIXME: put these in a state tag Z_START_ATTACK
                            // instead
        break;
    case Z_DYING:
        // the animation tells us when we're done dying
        if ( anim && curAnim && anim->frameNo() == curAnim->total - 1 ) {
            state = Z_DEAD;
            delete_me |= 0x2;
    		console.Printf( "Zombie destroyed" );
        }
        wasAttacking = false;
        attackStart = false; 
        break;
    case Z_DEAD:

        break;
    default:
        state = Z_STANDING;
        stateTimer.set();
        break;
    }

    // set it even if dying
    if ( logicTimer.check() )
        logicTimer.set( 1000 / g_zombieLogicTic->integer() ) ;
}

// two zombie collide.  cause a brief pause, and then chart a new course
// for each, preferably in reverse of the previous course 
void Zombie_t::ZombieHandler( Zombie_t *Z ) {
	if ( !Z )
		return; 

	bool hit = false;

/*
	if ( state > Z_WALKING ) { 
        if ( Z->state > Z_WALKING ) {
            return;
        }
        
		// just move them out of the way
		float *correction = G_CalcBoxCollision( Z->wish, wish ) ;
		Z->wishMove( correction[0], correction[1] );
        return;
    }
    else if ( Z->state > Z_WALKING ) {
        if ( state > Z_WALKING ) {
            return;
        }
		// just move us out of the way
		float *correction = G_CalcBoxCollision( wish, Z->wish ) ;
		wishMove( correction[0], correction[1] );
        return;
    }
*/
	if ( state > Z_WALKING || Z->state > Z_WALKING ) {
        
		// allow some overlap if they are attacking, so they can really
        // pile on the player in large squishy throngs. Although, don't 
        // allow entire overlap (>50%) because then it loses visual sense

        // FIXME: how to determine 50% overlap?
		float *correction = G_CalcBoxCollision( Z->wish, wish ) ;
		Z->wishMove( correction[0], correction[1] );
        return;
    }
	
	// we hit
	if ( Z->clipping && this->clipping && AABB_intersect(Z->wish, this->wish))
    {
		// our correction
		float *correction = G_CalcBoxCollision( wish, Z->wish );

		// copy it
		float ours[2] = { correction[0], correction[1] };

		// their correction
		correction = G_CalcBoxCollision( Z->wish, wish ) ;

		// move both out of the incident area
		Z->wishMove( correction[0], correction[1] );
		this->wishMove( ours[0], ours[1] );
	}
}

void Zombie_t::ProjectileHandler( Projectile_t *P ) {
	if ( !P )
		return;

	// make sure the projectile hasn't already hit us (is it exploding?)
	if ( P->state == PROJ_EXPLODING )
		return;
	
	// check the clips
	if ( !P->clipping || !this->clipping || !AABB_intersect( P->wish, wish ) ) {
		return;
	}

	// ok we clip, now..
	//S_StartLocalSound( robot_hit );

	// take damage
	if ( (this->hitpoints -= P->damage) <= 0 ) {
		// full damage
		state = Z_START_DYING;
		delete_me |= 0x1;
		//S_StartLocalSound( robot_crash );
	}
	


	// throw bot backwards inertia from hit
	float *cor = G_CalcBoxCollision( wish, P->clip );
	wishMove( cor[0] * 0.5f, cor[1] * 0.5f );


	// trigger the projectile to explode
	P->explode();
}

/*
====================
 Zombie_t::ScanForPlayer

a successful scan changes state to Z_CHASING or Z_ATTACKING if close enough
====================
*/
void Zombie_t::ScanForPlayer( void ) {

	if ( !g_zombieLOS->integer() )
		return;

	// get zombie center
	float zc[2];
	poly.getCenter( zc );

	// calculate player_dist 
	float pc[2];
	player.poly.getCenter( pc );
	float player_dist = sqrtf( ( pc[0] - zc[0] ) * ( pc[0] - zc[0] ) +
						 ( pc[1] - zc[1] ) * ( pc[1] - zc[1] ) );

    bool withinRange = false;

	// determine if player is within scan range
	if ( player_dist <= g_scanRadius->value() ) {
		withinRange = true;
	}

	if ( !withinRange ) {
        playerSeen = false;
		return;
    }

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
	v[0] = ( zc[0] - pc[0] ) / player_dist * spacing;
	v[1] = ( zc[1] - pc[1] ) / player_dist * spacing;

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
	BlockExport_t *be = M_GetBlockExport( zc, g_scanRadius->value(), &be_len );

    // assume true, look for wall geometry to invalidate
    playerSeen = true;

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

    // state change here
    if ( playerSeen ) {
        playerLastSeenXY[0] = player.clip[0];
        playerLastSeenXY[1] = player.clip[1];
        if ( player_dist < g_attackRadius->value() ) {
            if ( state != Z_ATTACKING && !wasAttacking )
                attackStart = true;
            wasAttacking = true;
            state = Z_ATTACKING;
        } else {
            state = Z_CHASING;
        }
    }
}
