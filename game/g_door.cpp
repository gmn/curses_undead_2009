// g_door.cpp
//

/* FUCKING INTERLOPERS!
   I have no fucking idea how these get thrown in here, so I nix em outright */
#define __MAPEDIT_H__ 1 
#define __M_AREA_H__ 1

#include "g_entity.h"
#include "../common/common.h"
#include "../client/cl_console.h"

int Door_t::id = 0;
extern gvar_c * g_useRadius;

extern filehandle_t door_sound[];



// proper Door_t copy constructor
Door_t::Door_t( Door_t const &D ) : Portal_t( D ) 
{
    state = D.state;
    lock = D.lock;
    type = D.type;
    dir = D.dir;

    open_amt = D.open_amt;
    doortime = D.doortime;
    waittime = D.waittime;
    frac_mv = D.frac_mv;
    
    savepoly.set( D.savepoly );
    hitpoints = D.hitpoints;
    use_portal = D.use_portal;

    dtimer.reset();

    color = D.color;
}

// FIXME: for the time being I'm not sure if this is going to work. delete col
//  should now work, but don't know about mat, and anim. If they're broken,
//  they need to be fixed at their own copyCon level.
void Door_t::_my_destroy() {
	if ( mat ) 
		delete mat;
	mat = NULL;
	if ( col )
		delete col;  
	col = NULL;
	if ( anim )
		delete anim;
	anim = NULL;
}

/* Old 
void Door_t::_my_destroy() {
//	if ( mat ) 
//		delete mat;
	mat = NULL;

// ****NOW I know why this was doing this, I wasn't copying col in the copy constructor 

//	if ( col )
//		delete col;  // FIXME FUCKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKK
	// I CANT FUCKING FIGURE THIS FUCKING THE FUCK OUT>  IT JUST DOESNT MAKE ANY SUPER FUCKING SENSE FUCK
	col = NULL;
//	if ( anim )
//		delete anim;
	anim = NULL;
//	Entity_t::_my_destroy();
}
*/

bool Door_t::checkUse( Entity_t * e  ) {
	Player_t *p = dynamic_cast<Player_t*>(e);
	return p && p->use & 1;
}

bool Door_t::checkKey( Entity_t *e ) {
	Player_t *p = dynamic_cast<Player_t*>(e);
	if ( !p )
		return false;

	// not even a locking door
	if ( !(type & DOOR_LOCKABLE) ) {
		return false;
	}

	// check player's inventory for color key	
	// ....

	switch ( color ) {
	case DOOR_COLOR_RED:
	case DOOR_COLOR_BLUE:
	case DOOR_COLOR_GREEN:

	case DOOR_COLOR_SILVER:
	case DOOR_COLOR_GOLD:
		break;
	default:
		return false;
	}

	// checks if they are pressing the USE BUTTON, thus _using_ the key
	if ( checkUse( e ) )
		return true;

	return false;
}

// called by handler.  actually opens/closes the door
int Door_t::trigger( Entity_t **e ) {

	// LOCKS

	// can lock/unlock proximity door in any state. once it's closed
	// if locked, it stays closed until being unlocked
	if ( lock == DOOR_LOCKED ) {
		if ( checkKey( *e ) ) {
			// ..post unlocked door success sound 
			lock = DOOR_UNLOCKED;
			console.Printf( "player unlocked door" );
		} else {
			// ..generate a door-locked fail sound
		}

	} else if ( lock == DOOR_UNLOCKED && (type & DOOR_LOCKABLE) ) {
		if ( checkKey( *e ) ) {
			// ..post door locked sound
			lock = DOOR_LOCKED;
			console.Printf( "player locked door" ); 
		} else {
			// ..generate generic fail thud-sound
		}
	}


	// 2 broad types of doors, those that open automatically
	// and those that require input to open/close

	// AUTOMATIC DOORS
	if ( DOOR_PROXIMITY & type ) 
	{
		// CLOSED
		if ( DOOR_CLOSED == state ) {
			if ( DOOR_LOCKED == lock ) {
				console.Printf( "automatic door must be unlocked to open" );
				return RESULT_NORMAL;
			}
			//console.Printf( "automatic door opening automatically" );
			state = DOOR_OPENING;
			S_StartLocalSound( door_sound[(rand()>>5)&1] );
			return RESULT_NORMAL;
		}

		// OPEN
		if ( DOOR_OPEN == state ) {

			// it is open, if it's a portal, call portal's handler too
			if ( this->use_portal ) {
				Portal_t::_handle( e );
			}
	
			// entity in the way, postpone closing
			dtimer.set( waittime );
			return RESULT_NORMAL;
		}

		// CLOSING
		if ( DOOR_CLOSING == state ) {
			if ( lock == DOOR_UNLOCKED )
				state = DOOR_OPENING;
			return RESULT_NORMAL;
		}
	
		// OPENING
		if ( DOOR_OPENING == state ) {
			// automatic doors, no effect, 
			return RESULT_NORMAL;
		}
	} 

	// MANUAL DOORS
	else if ( DOOR_MANUAL & type ) 
	{
		// CLOSED
		if ( DOOR_CLOSED == state ) {
			if ( DOOR_LOCKED == lock ) {
				// ..post "trying to open locked door" sound 
				console.Printf( "player's attempt to open locked door failed" );
				return RESULT_NORMAL; 
			}
			// unlocked && trying to open
			if ( checkUse( *e ) ) {
				state = DOOR_OPENING;
				// ..post open-door sound
				S_StartLocalSound( door_sound[(rand()>>5)&1] );
				console.Printf( "player opened door" );
			}
			return RESULT_NORMAL;
		}

		// OPEN
		if ( DOOR_OPEN == state ) {

			// it is open and it's a portal, call portal's handler too
			if ( this->use_portal ) {
				Portal_t::_handle( e );
			}

			// set it to return to closing if USE'd, otherwise leave open
			if ( checkUse( *e ) ) {
				// ..post closing door sound (if any)
				S_StartLocalSound( door_sound[(rand()>>5)&1] );
				state = DOOR_CLOSING;
				console.Printf( "player closed door" );
			}
			return RESULT_NORMAL;
		}

		// CLOSING
		if ( DOOR_CLOSING == state ) {
			// once its closing, if it's locked it will finish closing
			// unless the user is quick enough to press Unlock and Use again
			// before it finishes
			if ( lock == DOOR_UNLOCKED && checkUse( *e ) ) {
				state = DOOR_OPENING;
				console.Printf( "player opened closing door" );
			}
			return RESULT_NORMAL;
		}
	
		// OPENING
		if ( DOOR_OPENING == state ) {
			if ( lock == DOOR_UNLOCKED && checkUse( *e ) ) {
				console.Printf( "player closed opening door" );
				state = DOOR_CLOSING;
			}
			return RESULT_NORMAL;
		}
	}

	// catch all
	return RESULT_NORMAL; // shouldn't get here, but just in case ..
}

void Door_t::ProjectileHandler( Projectile_t *P ) {
	if ( !P )
		return;
	if ( PROJ_EXPLODING == P->state )
		return;
	if ( AABB_collide( this->clip, P->wish ) ) {
		if ( !P->clipping )
			return;
		if ( this->clipping ) 
			P->explode();
/*		else
			P->delete_me = 1;
*/
	}
	// should teleport the projectile if the door is open, or just make it dissapear because
	// if the player goes into a room where gametics haven't been running, then there will be all these 
	// frozen projectiles that he'll walk through, getting blitzed.  well, if you ever figure out a 
	// way to run rooms that are nearby, then change it to a plain teleport, until then, make them
	// dissapear
}

void Door_t::_handle( Entity_t **epp ) {

	// filter to thinks that will set off/trigger a door
	switch ( (*epp)->entType ) {
	case ET_PLAYER:
	case ET_ROBOT:
    case ET_ZOMBIE:
		break; // handle them all the same
	case ET_PROJECTILE: return ProjectileHandler( dynamic_cast<Projectile_t*>(*epp) );
	default: 
		return;
	}

	// TRIGGER
	// if it's collidable, test it's wish with our trig
	int result = RESULT_NORMAL;
	if ( (*epp)->collidable && AABB_intersect( (*epp)->wish, trig ) ) {
		result = this->trigger( epp );
	}

	// trig result can void a collision.  (like in a teleport)
	if ( RESULT_NOCLIP == result )
		return;

	// CLIP
	// physical collision with intended move
	if ( (*epp)->clipping && this->clipping && AABB_intersect( (*epp)->wish, clip ) ) {
		(*epp)->handleEntityCollision( this );
	}
}

/* sets the visual */
void Door_t::setOpenAmt( float f ) {
	switch ( dir ) {
	case DOOR_UP:
		poly.y = savepoly.y + f * savepoly.h;
		poly.h = savepoly.h - f * savepoly.h;
		mat->t[2] = 1.0f - f;
		mat->t[3] = 1.0f - f;
		break;
	case DOOR_RIGHT:
		poly.x = savepoly.x + f * savepoly.w;
		poly.w = savepoly.w - f * savepoly.w;
		mat->s[1] = 1.0f - f;
		mat->s[2] = 1.0f - f;
		break;
	case DOOR_DOWN:
		poly.h = savepoly.h - f * savepoly.h;
		mat->t[0] = f;
		mat->t[1] = f;
		break;
	case DOOR_LEFT:
		poly.w = savepoly.w - f * savepoly.w;
		mat->s[0] = f;
		mat->s[3] = f;
		break;
	case DOOR_SPLIT:
		break;
	case DOOR_SWING:
		break;
	}
	switch ( dir ) {
	case DOOR_UP:
	case DOOR_RIGHT:
	case DOOR_DOWN:
	case DOOR_LEFT:
		//Entity_t::populateGeometry( 0 );
		Entity_t::populateGeometry( 4 );
	default:
		break;
	}
}

void Door_t::think( void ) {

	// we may be waiting for a portal to be handled
	if ( this->use_portal ) {
		Portal_t::think();
	}

	if ( DOOR_CLOSED == state ) {
		return;
	}

	if ( DOOR_OPEN == state ) {
		if ( type & DOOR_PROXIMITY && dtimer.check() && !(type & DOOR_PERMANENT) ) {
			//console.Printf( "automatic door closing automatically" );
			state = DOOR_CLOSING;
			S_StartLocalSound( door_sound[(rand()>>5)&1] );
			clipping = true;
		}

		// manual doors that are permanent, close automatically after being
		// opened manually
		if ( type & DOOR_MANUAL && dtimer.check() && (type & DOOR_PERMANENT) ) {
			state = DOOR_CLOSING;
			S_StartLocalSound( door_sound[(rand()>>5)&1] );
			clipping = true;
		}
		return;
	}

	if ( DOOR_OPENING == state ) {
		// check if all the way open
		if ( open_amt >= 1.0f ) {
			state = DOOR_OPEN;			
			open_amt = 1.0f;
			dtimer.set( waittime );
			clipping = false; // don't block entities when door is open
			setOpenAmt( open_amt );
			return;
		}
		open_amt += frac_mv;
		setOpenAmt( open_amt );
		return;
	}

	if ( DOOR_CLOSING == state ) {
		clipping = true;
		if ( open_amt <= 0.0f ) {
			state = DOOR_CLOSED;
			open_amt = 0.0f;
			setOpenAmt( open_amt );
			return;
		}
		open_amt -= frac_mv;
		setOpenAmt( open_amt );
		return ;
	}
}




