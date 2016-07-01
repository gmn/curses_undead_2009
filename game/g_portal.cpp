// g_portal.cpp
//

#define __MAPEDIT_H__ 1

#include "../map/m_area.h"
#include "g_entity.h"
#include "../client/cl_console.h"

int Portal_t::id = 0;

extern bool r_inteleport;
bool com_freeze_thinkers = false;

// portals specify spawns
int Portal_t::teleportEntity( Entity_t *ent ) {
	// filter types that can pass through Portal
	switch ( ent->entType ) {
	case ET_PLAYER:
	case ET_ZOMBIE:
	case ET_ROBOT:
	case ET_PROJECTILE:
		break;
	default:
		return RESULT_NORMAL;
	}

	// find area
	int i;
	Area_t *A = NULL;

	if ( (A=(Area_t*)areaDest) ) 
		goto alreadyHaveArea;
	
	for ( i = 0; i < world.areas.length(); i++ ) {
		if ( !strcmp( area, world.areas.data[i]->name ) ) {
			A = world.areas.data[i];
			break;
		}
	}
	if ( world.areas.length() == i || !A ) {
		console.Printf( "Teleport error: area \"%s\" not found", this->area );
		return RESULT_NORMAL;
	}

	// save it
	this->areaDest = (void*)A;
alreadyHaveArea:

	// find spawn
	Spawn_t *S = NULL;
	node_c<Entity_t*> *node = A->entities.gethead();

	if ( (S=this->spawnDest) ) 
		goto alreadyHaveSpawn;
	
	while ( node ) {
		if ( node->val->entType == ET_SPAWN && !strcmp( spawn, node->val->name ) ) {
			if ( (S = dynamic_cast<Spawn_t*>( node->val )) )
				break;
		}
		node = node->next;
	}
	if ( !node || !S ) {
		console.Printf( "Teleport error: spawn \"%s\" not found", this->spawn );
		return RESULT_NORMAL;
	}

	// save 
	this->spawnDest = S;
alreadyHaveSpawn:

	// get spawn's block
	Block_t * teleBlock = S->blockLookup( A );
	if ( !teleBlock ) {
		console.Printf( "warning: no spawn block on teleport.aborting" );
		return RESULT_NORMAL;
	}


	// we're going to go through with it
	console.Printf( "%s teleported from (%.1f, %.1f) --> (%.1f, %.1f)", ent->name, ent->poly.x, ent->poly.y, S->poly.x, S->poly.y );

	// not player, change block that it is a member of
	if ( ET_PLAYER != ent->entType && teleBlock != ent->block ) {
		// remove from current block and insert into new one
		ent->blockRemove();
		ent->connect( teleBlock );
	}

	// update the wish intensions to the teleport location, or else it 
	// bounces back  ?
	ent->wishMove( S->poly.x - ent->poly.x, S->poly.y - ent->poly.y );

	// player specific
	if ( ET_PLAYER == ent->entType ) {

		controller.notifyTeleport( S->poly.x - ent->poly.x, S->poly.y - ent->poly.y, S, A );
	
		// tell renderer to chill out for this frame
		r_inteleport = true;

		// tell the spawn how long to block-wait for
		if ( (type & PRTL_WAIT) ) {
			spawnDest->setBlockWait( this->wait );
		}

		player.block = teleBlock;
	}

	// move entity to spawn location
	ent->setCoord( S->poly.x, S->poly.y );


	// instruct entity how long to pause for
	if ( (type & PRTL_WAIT) ) {
		ent->pause( wait );
	}

	// turn portal back off again
	sleep();

	return RESULT_NOCLIP;
}

/* 
====================
 Portal_t::trigger

	called everytime something walks over it's trig box
====================
*/
int Portal_t::trigger( Entity_t ** epp ) {

	// if it's sleeping, let stuff in
	if ( PRTL_SLEEPING != state )
		return RESULT_NOCLIP;

	if ( teleport_active )
		return RESULT_NOCLIP;

	// catch which entity set it off
	caught = *epp;
	if ( !caught )
		return RESULT_NOCLIP;

	// projectiles disappear when they hit a teleporter
	if ( caught->entType == ET_PROJECTILE ) {
		caught->startDelete();
		return RESULT_NOCLIP;
	}

	// fast teleport 
	if ( (type & PRTL_FAST) ) {
		sleep(); // turn local portal off
		return teleportEntity( *epp );
	}
	
	//
	if ( type & PRTL_FREEZE ) {
		state = PRTL_FROZEN;
		timer.set( freeze );
		activate();
		return RESULT_NORMAL;
	}

	// unrecognized type, disable whatever it is
	sleep();
	return RESULT_NORMAL;
}

/*
====================
 Portal_t::_handle
====================
*/
void Portal_t::_handle( Entity_t **epp ) {

	// filter to thinks that will set off/trigger a door
	switch ( (*epp)->entType ) {
	case ET_PLAYER:
	case ET_ROBOT:
	case ET_PROJECTILE:
		break; // handle them all the same
	default: 
		return;
	}

	// TRIGGER
	// if it's collidable, test it's wish with our clip
	int result = RESULT_NORMAL;
	if ( (*epp)->collidable && AABB_intersect( (*epp)->wish, trig ) ) {
		result = Portal_t::trigger( epp ); // named because Door extends Portal 
	}
}

void Portal_t::sleep( void ) {
	if ( this->caught->entType == ET_PLAYER ) {
		::com_freeze_thinkers = false;
	}
	// go back to waiting for something to happen
	this->state = PRTL_SLEEPING;
	// off
	this->teleport_active = false;
}

void Portal_t::activate( void ) {
	if ( this->caught->entType == ET_PLAYER ) {
		::com_freeze_thinkers = true;
	}
	this->teleport_active = true;
}

void Portal_t::think( void ) {
	if ( PRTL_SLEEPING == state ) 
		return;

	if ( PRTL_FROZEN == state ) {
		if ( timer.check() ) {
			teleportEntity( caught );
		}
		return;
	}

	// fell through?
	sleep();
}
