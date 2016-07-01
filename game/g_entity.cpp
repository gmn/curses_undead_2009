// g_entity.cpp
//


#include "../map/m_area.h"

#include "g_entity.h"
#include "g_item.h"

//#include "../mapedit/mapedit.h"
//#include "../common/com_geometry.h"


#include "../client/cl_console.h"
#include <string.h>
#include "../map/mapdef.h"
#include "../client/cl_public.h"

#ifndef snprintf
	#define snprintf _snprintf
#endif

extern filehandle_t laser_sound[];
extern filehandle_t turretball_sound[];
extern gvar_c *g_tbsound;
extern gvar_c *platform_type;
extern float r_angle;

entityTypeList_t entityTypes[] = {
	{ ET_ENTITY, "ET_ENTITY", "entity" },
	{ ET_TRIGGER, "ET_TRIGGER", "trigger" },
	{ ET_PORTAL, "ET_PORTAL", "portal" },
	{ ET_SPAWN, "ET_SPAWN", "spawn" },
	{ ET_DOOR, "ET_DOOR", "door" },
	{ ET_ITEM, "ET_ITEM", "item" },
	{ ET_PROJECTILE, "ET_PROJECTILE", "projectile" },
	{ ET_NPC, "ET_NPC", "npc" },
	{ ET_FURNITURE, "ET_FURNITURE", "furniture" },
	{ ET_COMPUTER, "ET_COMPUTER", "computer" },
	{ ET_DECAL, "ET_DECAL", "decal" },
	{ ET_ANIMATOR, "ET_ANIMATOR", "animator" },
	{ ET_DIALOGBOX, "ET_DIALOGBOX", "dialogbox" },
	{ ET_PLAYER, "ET_PLAYER", "player" },
	{ ET_ROBOT, "ET_ROBOT", "robot" },
	{ ET_SNAKE, "ET_SNAKE", "snake" },
	{ ET_ZOMBIE, "ET_ZOMBIE", "zombie" },
	{ ET_TURRET, "ET_TURRET", "turret" },
	{ ET_MEEP, "ET_MEEP", "meep" },
};

const unsigned int ENTITY_TYPELIST_SIZE = (sizeof(entityTypes)/sizeof(entityTypes[0]));

entityClassList_t entityClasses[] = {
	{ EC_NODRAW, "EC_NODRAW", "nodraw" },
	{ EC_FURNITURE, "EC_FURNITURE", "furniture" },
	{ EC_DECAL, "EC_DECAL", "decal" },
	{ EC_ITEM, "EC_ITEM", "item" },
	{ EC_THINKER, "EC_THINKER", "thinker" },
	{ EC_EFFECT, "EC_EFFECT", "effect" },
};

const unsigned int ENTITY_CLASSLIST_SIZE = (sizeof(entityClasses)/sizeof(entityClasses[0]));

/*
========================================
 shoot_s
========================================
*/
// # of shot commands this frame
void shoot_s::notify( unsigned int shots ) {
	if ( 0 == shots ) {
		shooting = false;
	} else {
		shooting = true;
	}
}
bool shoot_s::check( void ) {
	if ( !shooting )
		return false;
	int time = now();
	if ( time - last > msPerShot ) {
		last = time;
		return true;
	}
	return false;
}



// {{{
/*
==============================================================================

	Entity_t

==============================================================================
*/

int Entity_t::id = 0;

void Entity_t::AutoName( const char *tag , int *_id ) {
	snprintf( name, ENTITY_NAME_SIZE, "%s_%04d", tag, (_id) ? ++(*_id) : ++id );
}

// copy constructor
Entity_t::Entity_t( Entity_t const& E ) 
	: anim(0), mat(0), col(0), proj_vec(), boxen(), shoot(E.shoot), lerp(E.lerp)
{
	strcpy( name, E.name );
	entType = E.entType;
	entClass = E.entClass;
	ioType = E.ioType;

	anim = NULL;
	if ( E.anim ) {
		anim = new animSet_t( *E.anim );
	}

	mat = NULL;
	if ( E.mat ) {
		//mat = new material_t( *E.mat );
		// default action is to re-use pointers to stock materials
		mat = E.mat; 
	}
	lastMat = mat;

	poly 			= E.poly;
	COPY4( aabb, E.aabb );
	COPY8( obb, E.obb );
	AABB_zero( wish );

	collidable 		= E.collidable;

	// get a new one either way
	col = ( E.col ) ? new colModel_t( *E.col ) : new colModel_t();

	triggerable = E.triggerable;
	COPY4( trig, E.trig );
	clipping = E.clipping;
	COPY4( clip, E.clip );

	// take it if he has it
	block = E.block;

	// states
	paused = false;
	pause_timer.reset();
	delete_me = 0;
	facing = E.facing;
	facingLast = E.facingLast;
}

// 
void Entity_t::_my_init() {
	if ( col ) {
		col->init();
	}
}

// 
void Entity_t::_my_reset() {
	entType = ET_ENTITY;
	entClass = EC_NODRAW;

	if ( anim ) { anim->reset(); }
	mat = NULL;

	poly.zero();
	AABB_zero( aabb );
	OBB_zero( obb );
	AABB_zero( wish );

	collidable = true;
	if ( col ) { col->reset(); }

	triggerable = true;
	AABB_zero( trig );
	clipping = true;
	AABB_zero( clip );

	block = NULL;

	paused = false;
	pause_timer.reset();

	col_save.reset();

	proj_vec.reset();
	boxen.reset();

	shoot.reset();

	timer.reset();

//	AutoName( "ENTITY" );
	prevCoord[0] = 0.f;
	prevCoord[1] = 0.f;
	lastDrawn[0] = 0.f;
	lastDrawn[1] = 0.f;
	in_collision = 0;
	delete_me = 0;
	facing = facingLast = MOVE_DOWN;
}

//
void Entity_t::_my_destroy() {
	if ( anim ) {
		delete anim;
	}
	anim = NULL;

	// some entities will wan't to delete their material because they 
	// created their own copy; Door_t does this
	// but the default action is NOT to
	mat = NULL;

	if ( col ) {
//		delete col;
	}
//	col = NULL;
	/* since col was allocated in ctor, maybe it will get baff'd in the dtor,
	   cross your fingers */

	col_save.destroy();

	boxen.destroy();
	proj_vec.destroy();
}



/*
====================
 static utility function to copy Entity by type, so that new is invoked, 
  for the correct type

 caveat: all entity types must be registered here
====================
*/
Entity_t * Entity_t::CopyEntity( Entity_t *ent ) {
	// Portal_t
	if ( typeid(Portal_t) == typeid( *ent ) ) {
		return new Portal_t( *dynamic_cast<Portal_t*>(ent) );
	// NPC_t
	} else if ( typeid(NPC_t) == typeid( *ent ) ) {
		return new NPC_t( *dynamic_cast<NPC_t*>(ent) );
	// Player_t
	} else if ( typeid(Player_t) == typeid( *ent ) ) {
		return new Player_t( *dynamic_cast<Player_t*>(ent) );
	// Trigger_t
	} else if ( typeid(Trigger_t) == typeid( *ent ) ) {
		return new Trigger_t( *dynamic_cast<Trigger_t*>(ent) );
	// Projectile_t
	} else if ( typeid(Projectile_t) == typeid( *ent ) ) {
		return new Projectile_t( *dynamic_cast<Projectile_t*>(ent) );
	// Door_t
	} else if ( typeid(Door_t) == typeid( *ent ) ) {
		return new Door_t( *dynamic_cast<Door_t*>(ent) );
	// Item_t
	} else if ( typeid(Item_t) == typeid( *ent ) ) {
		return new Item_t( *dynamic_cast<Item_t*>(ent) );
	// Animator_t
	} else if ( typeid(Animator_t) == typeid( *ent ) ) {
		return new Animator_t( *dynamic_cast<Animator_t*>(ent) );
	// Decal_t
	} else if ( typeid(Decal_t) == typeid( *ent ) ) {
		return new Decal_t( *dynamic_cast<Decal_t*>(ent) );
	// Furniture_t
	} else if ( typeid(Furniture_t) == typeid( *ent ) ) {
		return new Furniture_t( *dynamic_cast<Furniture_t*>(ent) );
	// Entity_t
	} else if ( typeid(Entity_t) == typeid( *ent ) ) {
		return new Entity_t( *ent );
	// Spawn_t
	} else if ( typeid(Spawn_t) == typeid( *ent ) ) {
		return new Spawn_t( *dynamic_cast<Spawn_t*>(ent) );
	// Computer_t
	} else if ( typeid(Computer_t) == typeid( *ent ) ) {
		return new Computer_t( *dynamic_cast<Computer_t*>(ent) );
	// Dialogbox_t
	} else if ( typeid(Dialogbox_t) == typeid( *ent ) ) {
		return new Dialogbox_t( *dynamic_cast<Dialogbox_t*>(ent) );
	// Robot_t
	} else if ( typeid(Robot_t) == typeid( *ent ) ) {
		return new Robot_t( *dynamic_cast<Robot_t*>(ent) );
	// Snake_t
	} else if ( typeid(Snake_t) == typeid( *ent ) ) {
		return new Snake_t( *dynamic_cast<Snake_t*>(ent) );
	// Zombie_t
	} else if ( typeid(Zombie_t) == typeid( *ent ) ) {
		return new Zombie_t( *dynamic_cast<Zombie_t*>(ent) );
	// Turret_t
	} else if ( typeid(Turret_t) == typeid( *ent ) ) {
		return new Turret_t( *dynamic_cast<Turret_t*>(ent) );
	// Meep_t
	} else if ( typeid(Meep_t) == typeid( *ent ) ) {
		return new Meep_t( *dynamic_cast<Meep_t*>(ent) );
	}
	return NULL;
}

/* DEPRECATED, so is lastDrawn */
// extrapolates current coordinate, by frac
void Entity_t::getDrawCoord( float * v, float frac, int _internal ) {
	if ( _internal ) {
		COPY8( v, obb );
	}
	float delta[2] = { poly.x, poly.y };
	delta[0] = frac * ( poly.x - lastDrawn[0] );
	delta[1] = frac * ( poly.y - lastDrawn[1] );
	if ( _internal ) {
		lastDrawn[0] = poly.x;
		lastDrawn[1] = poly.y;
	}
	v[0] += delta[0];
	v[1] += delta[1];
	v[2] += delta[0];
	v[3] += delta[1];
	v[4] += delta[0];
	v[5] += delta[1];
	v[6] += delta[0];
	v[7] += delta[1];
}

// set any coordinate, ie teleport
void Entity_t::setCoord( float nx, float ny ) {
	if ( paused )
		return;
	prevCoord[0] = poly.x;
	prevCoord[1] = poly.y;
	float delta[2] = { nx - poly.x , ny - poly.y };
	poly.x = nx;
	poly.y = ny;
	populateGeometry(0);
	if ( col ) {
		col_save.set( *col );
		col->drag( delta[0], delta[1] );
	}
	trig[0] += delta[0];
	trig[1] += delta[1];
	trig[2] += delta[0];
	trig[3] += delta[1];
	clip[0] += delta[0];
	clip[1] += delta[1];
	clip[2] += delta[0];
	clip[3] += delta[1];
}

// move by delta amount
void Entity_t::move( float dx, float dy ) {
	if ( paused )
		return;
	if ( ISZERO( dx ) && ISZERO( dy ) )
		return;
	prevCoord[0] = poly.x;
	prevCoord[1] = poly.y;
	poly.x += dx;
	poly.y += dy;
	populateGeometry(0); // sets aabb & obb from poly.x/y
	if ( col ) {
		col_save.set( *col );
		col->drag( dx, dy );
	}
	trig[0] += dx;
	trig[1] += dy;
	trig[2] += dx;
	trig[3] += dy;
	clip[0] += dx;
	clip[1] += dy;
	clip[2] += dx;
	clip[3] += dy;
}


void Entity_t::undoMove( void ) {
	Assert("shouldn't be using right now" == NULL );
	in_collision &= ~1;
	float m[2] = { prevCoord[0] - poly.x, prevCoord[1] - poly.y };
	poly.x = prevCoord[0];
	poly.y = prevCoord[1];
	if ( col )
		col->drag( m[0], m[1] );
	populateGeometry();
}

void Entity_t::bounceBack( float f  ) {
	Assert("shouldn't be using right now" == NULL );
	in_collision &= ~1;
	float m[2] = { prevCoord[0] - poly.x, prevCoord[1] - poly.y };
	m[0] *= f;
	m[1] *= f;
	poly.x += m[0];
	poly.y += m[1];
	if ( col )
		col->drag( m[0], m[1] );
	populateGeometry();
}

float * G_CalcBoxCollision( float *, float * ); // in g_player.cpp

void Entity_t::handleTileCollisions( void ) {

    if ( 0 == boxen.length() )
        return;

    // get move vector 
    vec2_t mvec = { wish[0] - prevCoord[0], wish[1] - prevCoord[1] };
    if ( ISZERO( mvec[0] ) && ISZERO( mvec[1] ) )
        return;

    // find all projection vectors ( responses to tile collisions )
    proj_vec.reset();
    float projTotal[2] = { 0.f, 0.f };
    int index = -1;
    while ( ++index < boxen.length() ) {
        float *p = G_CalcBoxCollision( wish, boxen.data[ index ]->box );
        proj_vec.add( p[0], p[1] );
        if ( fabs(p[0]) > fabs(projTotal[0]) )
            projTotal[0] = p[0] * com_reject->value();
        if ( fabs(p[1]) > fabs(projTotal[1]) )
            projTotal[1] = p[1] * com_reject->value();
    }

	// adjust wish by adding projTotal to it.  
	wishMove( projTotal[0], projTotal[1] );
}

// can fail, but better not
Block_t *Entity_t::blockLookup( void * vA ) {
	if ( block )
		return block;
	Area_t *A = vA ? (Area_t*)vA : NULL;
	// must find area
	if ( !A ) {
		const unsigned int len = world.areas.length();
		for ( register int i = 0; i < len; i++ ) {
			if ( AABB_intersect( world.areas.data[i]->boundary, aabb ) ) {
				A = world.areas.data[i];
				break;
			}
		}
	// sanity check the area anyway
	} else {
		if ( !AABB_collision( A->boundary, aabb ) ) {
			A = NULL;
		}
	}

	if ( !A ) {
		// FIND AREA FAIL
		// FIXME: fallback on linear search
		console.Printf( "Entity_t::blockLookup: FIND AREA FAIL" );
		return NULL;
	}

	int block_index[2] = { 
				( poly.x - A->p1[0] ) / A->block_dim[0], 
				( poly.y - A->p1[1] ) / A->block_dim[1] 
						};

	int array_index = block_index[0] + A->bx * block_index[1];

	if ( array_index >= A->blocks.length() ) {
		// FIND_BLOCK_FAIL
		console.Printf( "Entity_t::blockLookup: FIND BLOCK FAIL" );
		return NULL;	
	}

	// the block
	Block_t *B = A->blocks.data[ array_index ];

	// confirm that we found it
	Block_t *found = NULL;
	for ( int i = 0 ; i < A->blocks.length(); i++ ) {
		Block_t **bpp = A->blocks.data;
		Block_t **beg = bpp;
		unsigned int len = A->blocks.length();
		while ( bpp - beg < len ) {
			node_c<Entity_t*> *e = (*bpp)->entities[ entClass ].gethead();
			while ( e ) {
				if ( e->val == this ) {
					found = *bpp;
					break;
				}
				e = e->next;
			}
			if ( found )
				break;
			++bpp;
		}
		if ( found )
			break;
	}
	if ( found != B ) {
		// comment incorrect.  this function may be called by an ent who isn't yet inside
		//  of a block, therefor this check will fail 
//		console.Printf( "Entity_t::blockLookup: FIND BLOCK CONFIRMATION FAIL. taking any we can find." );
		if ( found ) 
			return found;
		else if ( B )
			return B;
		else
			return NULL;
	}
	return B;
}

Block_t *Entity_t::blockLookupByName( const char *areaName ) {
	// go through areas linearly, or use hash if supplied with a name
	return NULL;

}

/*
// remove itself from block it is a member of 
void Entity_t::blockRemove( void ) {
	Block_t *b = NULL;
	if ( block ) {
		bool success = block->entities[entClass].popval( this );
		//Assert( success && "just making sure that list_c::popval works" );
	} else if ( (b = blockLookup()) ) {
		bool success = b->entities[entClass].popval( this );
		//Assert( success && "just making sure that list_c::popval works" );
	} else {
		Assert( ! "failed blockLookup()" );
	}
}
*/

void Entity_t::shiftClip( float *s ) {
	clip[0] += s[0];
	clip[1] += s[1];
	clip[2] += s[0];
	clip[3] += s[1];
}

void Entity_t::shiftTrig( float *s ) {
	trig[0] += s[0];
	trig[1] += s[1];
	trig[2] += s[0];
	trig[3] += s[1];
}

/*
====================
 Entity_t::wishMove

	first wishMove is the entity's intent, therefor it also set's facing.
	subsequent wishMoves, move the entity w/o changing facing
====================
*/
void Entity_t::wishMove( float dx, float dy ) {
	wish[0] += dx; wish[1] += dy; wish[2] += dx; wish[3] += dy;
	if ( !facing ) { // facing is erased in BeginMove
		float v[2] = { dx, dy };
		setFacing( v );
	}
}

/*
====================
 Entity_t::BeginMove
====================
*/
void Entity_t::BeginMove( void ) {
	COPY4( wish, clip ); 
	facing = 0;
}

/*
====================
 Entity_t::FinishMove
====================
*/
void Entity_t::FinishMove( void ) {
	if ( paused )
		goto finishMoveEnd;

	// respond/adjust to entity collisions
	adjustFromEntityCollisions();

	// null wish?  shouldn't be
	if (ISZERO(wish[0]) &&
		ISZERO(wish[1]) &&
		ISZERO(wish[2]) &&
		ISZERO(wish[3]) )
		goto finishMoveEnd;

	// create delta move
	float mv[2] = {	wish[0] - clip[0], wish[1] - clip[1] };

	// no move?  
	if ( ISZERO(mv[0]) && ISZERO(mv[1]) ) {
		AABB_zero( wish );
		goto finishMoveEnd;
	}

	// regular move can't be outside Area & world.curArea would not be our 
	// area if we are moving or vice-versa
	if ( 	wish[0] < world.curArea->boundary[0] || 
			wish[1] < world.curArea->boundary[1] || 
			wish[2] >= world.curArea->boundary[2] || 
			wish[3] >= world.curArea->boundary[3] ) {
		goto finishMoveEnd;
	}

	// set facing direction (for animation and shooting)
	// MOVED to wishMove instead, because we dont want facing affected
	// by external forces effect upon our wish
	//setFacing( mv );

	// perhaps we are shooting?
	if ( shoot.check() ) {
		fireWeapon();
	}

	// will this move change the entities block?  if so, remove it from 
	// the last and insert it into the new block.

	// do a block lookup if we need it
	if ( !block ) {
		block = this->blockLookup();
	}
	Assert( block != NULL && "block lookup failed in Ent::FinishMove" );
	
	// get ranges of block we're currently in
	float ranges[4] = { 	block->p1[0], 
							block->p1[1], 
							block->p2[0], 
							block->p2[1] };

	// save it in case it changes
	Block_t *blockWasIn = block;

	// left
	Block_t *B = NULL;
	if ( wish[0] < ranges[0] ) {
		// down
		if ( wish[1] < ranges[1] ) {
			block = controller.find( block, C_DL );
		// up
		} else if ( wish[1] >= ranges[3] ) {
			block = controller.find( block, C_UL );
		// just left	
		} else {
			block = controller.find( block, C_L );
		}
	// right
	} else if ( wish[0] >= ranges[2] ) {
		// down
		if ( wish[1] < ranges[1] ) {
			block = controller.find( block, C_DR );
		// up
		} else if ( wish[1] >= ranges[3] ) {
			block = controller.find( block, C_UR );
		// just right	
		} else {
			block = controller.find( block, C_R );
		}
	// up
	} else if ( wish[1] >= ranges[3] ) {
		block = controller.find( block, C_U );
	// down
	} else if ( wish[1] < ranges[1] ) {
		block = controller.find( block, C_D );
	}

	// clear the old state
	AABB_zero( wish );
	
	Assert( block != NULL && "verify controller.find() always does the right thing" );

	// changed block, update blocks and runtime lists
	if ( block != blockWasIn ) {
		bool foundThatShit = blockWasIn->entities[ entClass ].popval( this );
		Assert( foundThatShit && "still checking that popval works good" );
		block->entities[ entClass ].add( this );
		world.force_rebuild = true;
	}

	// do the move
	move( mv[0], mv[1] );

finishMoveEnd:

	// save our drawSurface location to lerp from
	lerp.update( aabb[0], aabb[1] );
}

/*
====================
 Entity_t::handle
	routing part of the entity event handler
====================
*/
void Entity_t::handle( Entity_t ** epp ) {
	// OUT + OUT -- dropped
	// IN + IN	-- higher rank handles handles call, if equal, just handle it
	// IN + OUT -- always pass OUT to be handled by IN

	if ( OUT_TYPE == this->ioType ) {
		// both OUT, drop event
		if ( (*epp)->ioType == OUT_TYPE )
			return;
		// else pass event to the callee
		Entity_t * tmp = this;
		return (*epp)->_handle( &tmp );
	}

	// callee has higher IN_TYPE
	if ( this->ioType < (*epp)->ioType ) {
		Entity_t * tmp = this;
		return (*epp)->_handle( &tmp );
	}

	// we're handling it
	this->_handle( epp );
}

void Entity_t::connect( Block_t * B ) {
//	Assert( B && "shouldn't be NULL" );
	if ( !B )
		return;
	B->entities[entClass].add( this );
	this->block = B;
}

void Entity_t::pause( int ptime ) {
	paused = true;
	pause_timer.set( ptime );

	// needs to check if paused in:  FinishMove, move, setCoord, ..
}

void Entity_t::updateColBoxes( void ) {
	if ( !collidable )
		return;
	if ( triggerable & clipping ) {
		col->box[0] = ( trig[0] < clip[0] ) ? trig[0] : clip[0];
		col->box[1] = ( trig[1] < clip[1] ) ? trig[1] : clip[1];
		col->box[2] = ( trig[2] > clip[2] ) ? trig[2] : clip[2];
		col->box[3] = ( trig[3] > clip[3] ) ? trig[3] : clip[3];
		return;
	}
	if ( triggerable ) {
		col->box[0] = trig[0];
		col->box[1] = trig[1];
		col->box[2] = trig[2];
		col->box[3] = trig[3];
		return;
	}
	// clipping
	col->box[0] = clip[0];
	col->box[1] = clip[1];
	col->box[2] = clip[2];
	col->box[3] = clip[3];
}

/* barrier is the entity that _WE_ hit.  That's why it's our wish against
 	their clip */
void Entity_t::handleEntityCollision( Entity_t *barrier ) {
	// just save them here.  They'll be combined and run together in FinishMove
	float *fix = G_CalcBoxCollision( this->wish, barrier->clip );
	proj_vec.add( fix[0], fix[1] );
	//this->in_collision |= 1;
	this->in_collision |= 2;
}

void Entity_t::adjustFromEntityCollisions( void ) {
	// respond/adjust to entity collisions
	float projTotal[2] = { 0.f, 0.f };
	for ( int i = 0; i < proj_vec.length(); i++ ) {
		float * p = proj_vec.data[ i ].p;
        if ( fabs(p[0]) > fabs(projTotal[0]) )
            projTotal[0] = p[0] * com_reject->value();
        if ( fabs(p[1]) > fabs(projTotal[1]) )
            projTotal[1] = p[1] * com_reject->value();
	}
	wishMove( projTotal[0], projTotal[1] );
}

void Entity_t::fireWeapon( void ) {

	// figure out starting location
	float s[2] = { 0, 0 };
	int direction = facing ? facing : facingLast;

	// get starting point offset from character model
	switch( direction ) {
	case MOVE_LEFT:
		s[0] = poly.x-poly.w*0.0f; s[1] = poly.y + 0.5f * poly.h;
		break;
	case MOVE_RIGHT:
		s[0] = aabb[2]+poly.w*0.0f; s[1] = poly.y + 0.5f * poly.h;
		break;
	case MOVE_UP:
		s[0] = poly.x + 0.5f * poly.w; s[1] = aabb[3] + poly.h*0.2f;
		break;
	case MOVE_DOWN:
		s[0] = poly.x + 0.5f * poly.w; s[1] = aabb[1] - poly.h*0.2f;
		break;
	case 3: // up+right
		s[0] = aabb[2]+poly.w*0.0f; 
		s[1] = aabb[3]+poly.h*0.0f;
		break;
	case 9: // up+left
		s[0] = aabb[0]-poly.w*0.0f; 
		s[1] = aabb[3]+poly.h*0.0f;
		break;
	case 6: // down+right
		s[0] = aabb[2]+poly.w*0.0f; 
		s[1] = aabb[1]-poly.h*0.0f;
		break;
	case 12: // down+left
		s[0] = aabb[0]-poly.w*0.0f; 
		s[1] = aabb[1]-poly.h*0.0f;
		break;
	}

	// get angle of velocity
	float v[2] = { 0, 0 };
	switch ( direction ) {
	case MOVE_RIGHT:
		v[0] = 1.f;
		break;
	case MOVE_LEFT:
		v[0] = -1.f;
		break;
	case MOVE_UP:
		v[1] = 1.f;
		break;
	case MOVE_DOWN:
		v[1] = -1.f;
		break;
	case MOVE_UP_RIGHT:
		v[0] = v[1] = 0.707107f;
		break;
	case MOVE_UP_LEFT:
		v[0] = -0.707107f;
		v[1] = 0.707107f;
		break;
	case MOVE_DOWN_RIGHT:
		v[0] = 0.707107f;
		v[1] = -0.707107f;
		break;
	case MOVE_DOWN_LEFT:
		v[0] = -0.707107f;
		v[1] = -0.707107f;
		break;
	}

    if ( platform_type->integer() == 2 && entType == ET_PLAYER ) {
        v[0] = 0.f;
        v[1] = 1.f;
        M_Rotate2d( v, -r_angle ); // dir travelling
        float c[2];
        poly.getCenter( c );
        s[0] = c[0]; s[1] = c[1]; 
        s[1] += poly.h;
        M_TranslateRotate2d( s, c, -r_angle ); // starting pos
        direction = MOVE_UP;
        direction = MOVE_NONE;
    }

	// different projectile depending on who shot it
	switch ( entType ) {
	case ET_PLAYER:
		P_Shoot( PROJ_LASER, direction, v, s );
		S_StartLocalSound( laser_sound[0] );
		break;
	case ET_ROBOT:
		P_Shoot( PROJ_LASER, direction, v, s );
		S_StartLocalSound( laser_sound[1] );
		break;
	case ET_TURRET:
		{ 
			Turret_t *T = dynamic_cast<Turret_t*>(this);
			if ( T ) {
				P_Shoot( PROJ_TURRETBALL, direction, T->player_dir, s );
				S_StartLocalSound( turretball_sound[0] );
			}
		}
		break;
	}

	world.force_rebuild = true;
}

/*
====================
 Entity_t::setFacing
====================
*/
void Entity_t::setFacing( float *w ) {
	float v[2] = { w[0], w[1] };

	// correction
	if ( !ISZERO(v[1]) && fabsf(v[0]/v[1]) > 2.f ) {
		v[1] = 0.f;
	} else if ( !ISZERO(v[0]) && fabsf(v[1]/v[0]) > 2.f ) {
		v[0] = 0.f;
	}

	int dir = 0;
	if ( v[0] > 0.0f ) 
		dir |= MOVE_RIGHT;
	else if ( v[0] < -0.1f )
		dir |= MOVE_LEFT;
	if ( v[1] > 0.1f ) 
		dir |= MOVE_UP;
	else if ( v[1] < -0.1f )
		dir |= MOVE_DOWN;

	if ( in_collision )
		return;
	
	if ( facing != 0 )
		facingLast = facing;

	facing = dir;
}

void Entity_t::blockRemove( void ) {
	Block_t *b;
	bool success = false;
	if ( block ) {
		success = block->entities[entClass].popval( this );
	} 
	if ( (!success||!block) && (b=blockLookup()) ) {
		success = b->entities[entClass].popval( this );
	} 
	Assert( success && "entity failed block removal" );
}

/*
====================
 getMat

 non-virtual interface wrapper to virtual _getMat() 
====================
*/
material_t * Entity_t::getMat( void ) { 
	if ( cl_paused ) {
		return lastMat;
	}
	lastMat = _getMat();
	return lastMat; 
}


//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
// }}}  // end  Entity_t  class
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
//============================================================================== 



/*
====================
 G_ConPrintEntityTypes
====================
*/
void G_ConPrintEntityTypes( void ) {
	Assert( ENTITY_TYPELIST_SIZE == TOTAL_ENTITY_TYPES );
	console.Printf( "%-24s %s", "type", "token" );
	console.Printf( "-------------------------------------------------" );
	for ( int i = 0; i < ENTITY_TYPELIST_SIZE; i++ ) {
		console.Printf( "%-24s %s", entityTypes[i].code, entityTypes[i].token );
	}
}

/* 
====================
 G_NewEntFromType

	utility func, creates ent and returns it, by type
====================
*/
Entity_t * G_NewEntFromType( EntityType_t type_num ) {
	Entity_t *ent = NULL;
	switch ( type_num ) {
	case ET_ENTITY: 		ent = new Entity_t(); break;
	case ET_TRIGGER: 		ent = new Trigger_t(); break;
	case ET_PORTAL:			ent = new Portal_t(); break;
	case ET_SPAWN:			ent = new Spawn_t(); break;
	case ET_DOOR:			ent = new Door_t(); break;
	case ET_ITEM:			ent = new Item_t(); break;
	case ET_PROJECTILE:		ent = new Projectile_t(); break;
	case ET_NPC:			ent = new NPC_t(); break;
	case ET_FURNITURE:		ent = new Furniture_t(); break;
	case ET_COMPUTER:		ent = new Computer_t(); break;
	case ET_DECAL:			ent = new Decal_t(); break;
	case ET_ANIMATOR:		ent = new Animator_t(); break;
	case ET_DIALOGBOX:		ent = new Dialogbox_t(); break;
	case ET_PLAYER:			ent = new Player_t(); break;
	case ET_ROBOT:			ent = new Robot_t(); break;
	case ET_SNAKE:			ent = new Snake_t(); break;
	case ET_ZOMBIE:			ent = new Zombie_t(); break;
	case ET_MEEP:			ent = new Meep_t(); break;
	case ET_TURRET:			ent = new Turret_t(); break;
	}
	return ent;
}

/*
====================
 G_NewEntFromToken
====================
*/
Entity_t * G_NewEntFromToken( const char *token ) {
	int type_num = 0;
	for ( ; type_num < ENTITY_TYPELIST_SIZE; type_num++ ) {
		if ( !strcmp( token, entityTypes[type_num].token ) ) 
			break;
	}
	if ( type_num == ENTITY_TYPELIST_SIZE ) {
		return NULL;
	}
	return G_NewEntFromType( (EntityType_t)type_num );
}

/*
====================
 G_CreateEntity

	returns the type enum if found, else -1 for not found
====================
*/
int G_CreateEntity( const char *type ) {
	// find the type
	int type_num = 0;
	for ( ; type_num < ENTITY_TYPELIST_SIZE; type_num++ ) {
		if ( !strcmp( type, entityTypes[type_num].token ) ) {
			break;
		}
	}

	if ( type_num == ENTITY_TYPELIST_SIZE ) {
		return -1;
	}

	Entity_t *ent = G_NewEntFromType( (EntityType_t)type_num );
	me_ents->addNew( ent );

	// make new ent, sole selected
	selected.reset();
	entSelected.reset();
	entSelected.add( me_ents->gettail() );

	return type_num;
}

/*
====================
 G_EntTypeFromToken
====================
*/
EntityType_t G_EntTypeFromToken( const char *tok ) {
	for ( int i = 0; i < TOTAL_ENTITY_TYPES; i++ ) {
		if ( !strcmp( tok, entityTypes[i].token ) )
			return (EntityType_t)i;
	}
	return (EntityType_t)-1;
}

/*
====================
 G_EntClassFromToken
====================
*/
EntityClass_t G_EntClassFromToken( const char *clas ) {
	for ( int i = 0; i < TOTAL_ENTITY_CLASSES; i++ ) {
		if ( !strcmp( clas, entityClasses[i].token ) )
			return (EntityClass_t) i;
	}
	return (EntityClass_t)-1;
}

/*
====================
 _genericBoxCmd
====================
*/
enum {
	CLIP_CMD,
	TRIG_CMD,
};
static void _genericBoxCmd( int clipOrTrig, const char *_axis, const char *amt ) {
	int axis = tolower( _axis[0] );
	if ( axis != 'x' && axis != 'y' && axis != 'w' && axis != 'h' ) {
		console.Printf( "unknown axis supplied" );
		return;
	}
	float delta = atof( amt );
	
	node_c<entNode_t*> *node = entSelected.gethead();
	int did = 0;
	while ( node ) {	
		if ( !node->val->val->col ) {
			node = node->next;
			continue;
		}
		float *which = (clipOrTrig == CLIP_CMD) ? node->val->val->clip : node->val->val->trig;
		switch( axis ) {
		case 'x': which[0] += delta; ++did; break; 
		case 'y': which[1] += delta; ++did; break; 
		case 'w': which[2] += delta; ++did; break; 
		case 'h': which[3] += delta; ++did; break; 
		default:
			break;
		}
		node->val->val->updateColBoxes(); // make sure colMod still wraps clip & trig
		node = node->next;
	}
	if ( did ) {
		console.Printf( "moved %d entity's %s %f in the %c axis", did, 
			(clipOrTrig==CLIP_CMD) ? "clipping box" : "trigger box",
			delta,
			axis );
	}
}

/*
====================
 G_EntClipCmd
====================
*/
void G_EntClipCmd( const char *axis, const char *amt ) {
	_genericBoxCmd( CLIP_CMD, axis, amt );
}

/*
====================
 G_EntTrigCmd
====================
*/
void G_EntTrigCmd( const char *axis, const char *amt ) {
	_genericBoxCmd( TRIG_CMD, axis, amt );
}

/*
====================
 G_PortalModify
====================
*/
void G_PortalModify( const char *arg1, const char *arg2, const char *arg3 ) {

	node_c<entNode_t*> *E = entSelected.gethead();
	if ( !E ) {
		console.Printf( "no entities selected" );
		return;
	}
	
	int cmd = 0;
	if ( !strcmp( arg1, "area" ) ) {
		cmd = 1;
	} else if ( !strcmp( arg1, "spawn" ) ) {
		cmd = 2;
	} else if ( !strcmp( arg1, "type" ) ) {
		cmd = 3;
	} else if ( !strcmp( arg1, "freeze" ) ) {
		cmd = 4;
	} else if ( !strcmp( arg1, "wait" ) ) {
		cmd = 5;
	} else {
		console.Printf( "Portal: unrecognized command" );
		return ;
	}

	// sanity check
	int parm2 ;
	if ( cmd > 0 && cmd < 4 ) {
		if ( !arg2 || !arg2[0] ) {
			console.Printf( "please provide parm for portal cmd: %s", arg1 );
			return;
		}
	} else {
		parm2 = atoi( arg2 );
	}

	int count = 0;
	Portal_t *P;
	while ( E ) {
		if ( !(P = dynamic_cast<Portal_t*>(E->val->val)) ) {
			console.Printf( "skipping.. not portal" );
			E = E->next;
			continue;
		}
		++count;
		switch ( cmd ) {
		case 1:
			console.Printf( "changed %s's area: %s to %s", P->name, P->area, arg2 );
			strcpy( P->area, arg2 );
			break;
		case 2:
			console.Printf( "changed %s's spawn: %s to %s", P->name, P->spawn, arg2 );
			strcpy( P->spawn, arg2 );
			break;
		case 3:
			if ( !strcmp( "fast", arg2 ) ) {
				P->type = PRTL_FAST;
			} else if ( !strcmp( "freeze", arg2 ) ) {
				P->type = PRTL_FREEZE;
			} else if ( !strcmp( "wait", arg2 ) ) {
				P->type = PRTL_WAIT;
			} else if ( !strcmp( "freeze_wait", arg2 ) ) {
				P->type = PRTL_FREEZEWAIT;
			} else {
				console.Printf( "Portal type makes no sense. skipping." );
				--count;
			}
			break;
		case 4:
			P->freeze = parm2;
			break;
		case 5:
			P->wait = parm2;
			break;
		default:
			console.Printf( "parm unknown. skipping." );
			--count;
			break;
		}
		E = E->next;
	}
	console.Printf( "updated %d portals", count );
}

/*
====================
 G_DoorModify
====================
*/
void G_DoorModify( const char *arg1, const char *arg2, const char *arg3 ) {

	node_c<entNode_t*> *E = entSelected.gethead();
	if ( !E ) {
		console.Printf( "no entities selected" );
		return;
	}
	
	int cmd = 0;
	if ( !strcmp( arg1, "state" ) ) {
		cmd = 1;
	} else if ( !strcmp( arg1, "lock" ) ) {
		cmd = 2;
	} else if ( !strcmp( arg1, "type" ) ) {
		cmd = 3;
	} else if ( !strcmp( arg1, "dir" ) ) {
		cmd = 4;
	} else if ( !strcmp( arg1, "doortime" ) ) {
		cmd = 5;
	} else if ( !strcmp( arg1, "waittime" ) ) {
		cmd = 6;
	} else if ( !strcmp( arg1, "hitpoints" ) ) {
		cmd = 7;
	} else if ( !strcmp( arg1, "portal" ) ) {
		cmd = 8;
	} else if ( !strcmp( arg1, "color" ) ) {
		cmd = 9;
	} else {
		console.Printf( "Door: unrecognized command" );
		return ;
	}

	// sanity check
	int parm2 ;
	if ( cmd > 0 && cmd < 5 ) {
		if ( !arg2 || !arg2[0] ) {
			console.Printf( "please provide parm for door type: %s", arg1 );
			return;
		}
	} else {
		parm2 = atoi( arg2 );
	}

	int count = 0;
	Door_t *D;
	while ( E ) {
		if ( !(D = dynamic_cast<Door_t*>(E->val->val)) ) {
			E = E->next;
			continue;
		}
		++count;
		switch ( cmd ) {
		case 1:
			if ( !strcmp( "open", arg2 ) ) {
				D->state = DOOR_OPEN;
			} else if ( !strcmp( "closed", arg2 ) ) {
				D->state = DOOR_CLOSED;
			} else if ( !strcmp( "opening", arg2 ) ) {
				D->state = DOOR_OPENING;
			} else if ( !strcmp( "closing", arg2 ) ) {
				D->state = DOOR_CLOSING;
			} else {
				console.Printf( "parm \"%s\" makes no sense. skipping", arg2 );
				--count;
			}
			break;
		case 2:
			if ( !strcmp( "locked", arg2 ) ) {
				D->lock = DOOR_LOCKED;
				D->type |= DOOR_LOCKABLE;
			} else if ( !strcmp( "unlocked", arg2 ) ) {
				D->lock = DOOR_UNLOCKED;
			} else {
				console.Printf( "\"%s\" not valid lock state, skipping", arg2 );
				--count;
			}
			break;
		case 3:
			if ( !strcmp( "proximity", arg2 ) ) {
				D->type &= ~DOOR_MANUAL;
				D->type |= DOOR_PROXIMITY;
			} else if ( !strcmp( "manual", arg2 ) ) {
				D->type &= ~DOOR_PROXIMITY;
				D->type |= DOOR_MANUAL;
			} else if ( !strcmp( "proxperm", arg2 ) ) {
				D->type &= ~DOOR_MANUAL;
				D->type |= DOOR_PROXPERM;
			} else if ( !strcmp( "manperm", arg2 ) ) {
				D->type &= ~DOOR_PROXIMITY;
				D->type |= DOOR_MANPERM;
			} else if ( !strcmp( "lockable", arg2 ) ) {
				D->type ^= DOOR_LOCKABLE;
			} else {
				console.Printf( "\"%s\" not valid door type, skipping.", arg2 );
				--count;
			}
			break;
		case 4:
			if ( !strcmp( "up", arg2 ) ) {
				D->dir = DOOR_UP;
			} else if ( !strcmp( "down", arg2 ) ) {
				D->dir = DOOR_DOWN;
			} else if ( !strcmp( "left", arg2 ) ) {
				D->dir = DOOR_LEFT;
			} else if ( !strcmp( "right", arg2 ) ) {
				D->dir = DOOR_RIGHT;
			} else {
				console.Printf( "Door direction makes no sense. skipping." );
				--count;
			}
			break;
		case 5:
			D->doortime = parm2;
			break;
		case 6:
			D->waittime = parm2;
			break;
		case 7:
			D->hitpoints = parm2;
			break;
		case 8:
			D->use_portal = *arg2=='0' || *arg2=='f' || *arg2=='F' ? false : true;
			break;
		case 9:
			if ( !strcmp( "red", arg2 ) ) {
				D->color = DOOR_COLOR_RED;
				D->type |= DOOR_LOCKABLE;
			} else if ( !strcmp( "blue", arg2 ) ) {
				D->color = DOOR_COLOR_BLUE;
				D->type |= DOOR_LOCKABLE;
			} else if ( !strcmp( "green", arg2 ) ) {
				D->color = DOOR_COLOR_GREEN;
				D->type |= DOOR_LOCKABLE;
			} else if ( !strcmp( "silver", arg2 ) ) {
				D->color = DOOR_COLOR_SILVER;
				D->type |= DOOR_LOCKABLE;
			} else if ( !strcmp( "gold", arg2 ) ) {
				D->color = DOOR_COLOR_GOLD;
				D->type |= DOOR_LOCKABLE;
			} else {
				console.Printf( "door color \"%s\" not understood", arg2 );
				--count;
			}
			
			break;
		default:
			console.Printf( "Door parm \"%s\" unknown, skipping", arg1 );
			--count;
			break;
		}
		E = E->next;
	}
	console.Printf( "updated %d doors. (cmd: '%s %s')", count, arg1, arg2 );
}

#ifndef CP
#define CP console.Printf
#endif

/*
====================
 G_EntCmdHelp
====================
*/
void G_EntCmdHelp( void ) {
	console.Printf( "entity commands are a subset of console commands, all beginning with the token \"ent\"" );
	console.Printf( "available commands are:" );

	console.Printf( "%-20s %s", "ent help", "prints this" );
	console.Printf( "%-20s %s", "ent info", "prints info about selected entities" );
	console.Printf( "%-20s %s", "ent info all", "prints info about all entities" );
	console.Printf( "%-20s %s", "ent name <name>", "set name of a single selected entity" );
	console.Printf( "%-20s %s", "ent saveview", "saves current viewport to Spawn_t entity" );
	console.Printf( "%-20s %s", "ent class <newClass>", "change class of all selected entities" );
	console.Printf( "%-20s %s", "ent spawntype", "toggles spawn type of any selected ET_SPAWN entities" );
	console.Printf( "%-20s %s", "ent angle", "set entities angle of rotation (degrees)" );
	console.Printf( "%-20s %s", "ent height", "set entities height" );
	console.Printf( "%-20s %s", "ent width", "set entities width" );
	console.Printf( "%-20s %s", "ent <token>", "where token is one of the available entity types.  See 'listentitytypes'" );
		
	console.Printf( "%-20s %s", "ent X/Y <value>", "move entity by <value> on the X or Y axis" );
	console.Printf( "%-20s %s", "ent clip [x|y|w|h] <float>", "adjust clipping box in selected" );
	console.Printf( "%-20s %s", "ent trig [x|y|w|h] <float>", "adjust trigger box in selected" );
	console.Printf( "%-20s %s", "ent pl_speed <value>", "save the player speed to set in the player at a spawn point, when the player spawns there (optional)" );
	console.Printf( "%-20s %s", "ent door [state|lock|type|dir|doortime|waittime|hitpoints|portal] <parm>", "  USAGE:   states: open, closed, opening, closing. lock: unlocked, locked. type: proximity, manual, proxperm, manperm, lockable.  dir: up, down, left, right. doortime: int, waittime: int.  color: red, blue, green, silver, gold." ); 
	console.Printf( "%-20s %s", "ent portal [area|spawn|type|freeze|wait] <parm>", "  USAGE:   area <name>, spawn <name>, type: [fast, freeze, wait, freeze_wait], freeze: int, wait: int" );
	console.Printf( "%-20s %s", "ent sync", "resync the colMod, trig, clip with the ent poly" );
}

/*
====================
 G_PrintEntInfo
====================
*/
void G_PrintEntInfo( Entity_t * E ) {

	console.Printf( "" );

	// generic info
	console.Printf( "%-15s %s", "name", E->name );
	console.Printf( "%-15s %s", "type", entityTypes[E->entType].token );
	console.Printf( "%-15s %s", "class", entityClasses[E->entClass].token );
	console.Printf( "%-15s %s", "io type", !E->ioType ? "OUT_TYPE" : "IN_TYPE" );

	// material or animation
	char *mn = ( E->mat ) ? mn = E->mat->name : NULL;
	console.Printf( "%-15s %s", "material", mn );
	if ( E->anim ) {
		uint L = E->anim->length();
		console.Printf( "%-15s %u animations", "animation", L );
		int i = 0;
		while ( i < L ) {
			console.Printf( "%-15s %s", "", E->anim->data[i++].name );
		}
	} else {
		console.Printf( "%-15s %s", "animation", "" );
	}

	poly_t *p = &E->poly;
	console.Printf( "%-15s (%.1f %.1f %.1f %.1f %.1f)", "poly", p->x, p->y, p->w, p->h, p->angle );
	/*char * ct = ( E->col && E->col->type == CM_OBB ) ? "CM_OBB" : ( E->col && E->col->type == CM_AABB ) ? "CM_AABB" : "COLMOD_NONE";
	console.Printf( "%-15s %s", "col", ct );
	*/
	console.Printf( "%-15s %s", "collidable", E->collidable ? "true" : "false");
	if ( E->col ) {
		float *b = E->col->box;
		console.Printf( "%-15s %.2f %.2f %.2f %.2f", "colModel", b[0], b[1], b[2], b[3] );
	} else
		console.Printf( "%-15s %s", "colModel", "" );

	if ( E->triggerable ) 
		console.Printf( "%-15s %s %.2f %.2f %.2f %.2f", "triggerable", "true", E->trig[0], E->trig[1], E->trig[2], E->trig[3] ); 
	else
		console.Printf( "%-15s %s", "triggerable", "false" );
	
	if ( E->clipping ) 
		console.Printf( "%-15s %s %.2f %.2f %.2f %.2f", "clipping", "true", E->clip[0], E->clip[1], E->clip[2], E->clip[3] );
	else
		console.Printf( "%-15s %s", "clipping", "false" );
	

	char * s = NULL;
	Spawn_t *S = NULL;
	Door_t *D = NULL;
	Portal_t *P = NULL;
	int Mask = 0;

	// type specific info
	switch( E->entType ) {
	case ET_DOOR:
		D = dynamic_cast<Door_t*>( E );
		if ( !D ) {
			console.Printf( "Error: entity with type: \"door\" failed dynamic cast!!" );
			console.dumpcon( "console.DUMP" );
			break;
		}
		console.Printf( "++++++++++++ DOOR  ++++++++++++" );
		s = NULL;
		switch( D->state ) {
		case DOOR_CLOSED: 	s = "DOOR_CLOSED"; break;
		case DOOR_OPENING: 	s = "DOOR_OPENING"; break;
		case DOOR_CLOSING: 	s = "DOOR_CLOSING"; break;
		case DOOR_OPEN: 	s = "DOOR_OPEN"; break; }
		console.Printf( "state: %s", s );

		console.Printf( "lockable %s", D->type & DOOR_LOCKABLE ? "yes" : "no" );
		s = ( D->lock == DOOR_LOCKED ) ? "DOOR_LOCKED" : ( D->lock == DOOR_UNLOCKED ) ? "DOOR_UNLOCKED" : "unknown" ;
		console.Printf( "lock: %s", s );
		
		s = (D->type & DOOR_PROXPERM)==DOOR_PROXPERM ? "DOOR_PROXPERM" : (D->type & DOOR_MANPERM)==DOOR_MANPERM ? "DOOR_MANPERM" : ( D->type & DOOR_PROXIMITY ) ? "DOOR_PROXIMITY" : ( D->type & DOOR_MANUAL ) ? "DOOR_MANUAL" : "unknown" ;
		console.Printf( "type: %s", s );

		console.Printf( "waittime: %d", D->waittime );
		console.Printf( "doortime: %d", D->doortime );
		
		s = ( D->dir == DOOR_UP ) ? "UP" : ( D->dir == DOOR_RIGHT ) ? "RIGHT" : ( D->dir == DOOR_DOWN ) ? "DOWN" : ( D->dir == DOOR_LEFT ) ? "LEFT" : "unknown";
		console.Printf( "doorfunc direction: %s", s );
		console.Printf( "hitpoints: %d", D->hitpoints );
		console.Printf( "use portal: %s", D->use_portal ? "true" : "false" );
		switch ( D->color ) {
		case DOOR_COLOR_RED: s = "RED"; break;
		case DOOR_COLOR_GREEN: s = "GREEN"; break;
		case DOOR_COLOR_BLUE: s = "BLUE"; break;
		case DOOR_COLOR_SILVER: s = "SILVER"; break;
		case DOOR_COLOR_GOLD: s = "GOLD"; break;
		default: s = "UNKNONW"; break;
		}
		console.Printf( "lock color code: %s", s );
		console.Printf( "+++++++++++++++++++++++++++++++" );		
		
		if ( D->use_portal && dynamic_cast<Portal_t*>( E ) ) {
			goto CASE_PORTAL;
		}
		break;
	case ET_ROBOT:
		break;
	case ET_SNAKE:
		break;
	case ET_ZOMBIE:
		break;
	case ET_SPAWN:
		S = dynamic_cast<Spawn_t*>( E );
		if ( !S ) {
			console.Printf( "Error: entity with type: \"spawn\" failed dynamic cast!!" );
			console.dumpcon( "console.DUMP" );
			break;
		}
		console.Printf( "++++++++++++ SPAWN ++++++++++++" );
		console.Printf( "Spawn Type: %s", S->type == ST_WORLD ? "ST_WORLD" : "ST_AREA" );
		if ( S->set_view )
			console.Printf( "Spawn Viewport: %.1f %.1f %.1f %.1f Zoom: %.1f", S->view[0], S->view[1], S->view[2], S->view[3], S->zoom );
		else 
			console.Printf( "default viewport" );
		console.Printf( "Player Speed: %s", S->pl_speed );
		console.Printf( "+++++++++++++++++++++++++++++++" );
		break;
	case ET_PORTAL:
CASE_PORTAL:
		P = dynamic_cast<Portal_t*>( E );
		if ( !P ) {
			console.Printf( "Error: entity with type: \"portal\" failed dynamic cast!!" );
			console.dumpcon( "console.DUMP" );
			break;
		}
		console.Printf( "++++++++++++ PORTAL ++++++++++++" );
		CP( "area %s", P->area );
		CP( "spawn %s", P->spawn );
		Mask = P->type & PRTL_MASK_LOW;
		CP( "type %s", Mask == PRTL_FAST ? "fast" : Mask == PRTL_FREEZE ? "freeze" : Mask == PRTL_WAIT ? "wait" : Mask == PRTL_FREEZEWAIT ? "freeze_wait" : "unknown" );
		CP( "freeze %d", P->freeze );
		CP( "wait %d", P->wait );
		console.Printf( "++++++++++++++++++++++++++++++++" );
		break;
	}
}

/*
====================
 G_EntCommand
====================
*/
void G_EntCommand( const char *cmd, const char *arg1, const char *arg2, const char *arg3 ) {

	// print help
	if ( !cmd || !cmd[0] || !strcmp( cmd, "help" ) ) {
		G_EntCmdHelp();
	}
	
	// ent info					// prints list of info of selected ents
	else if ( !strcmp( "info", cmd )  ) {
		// ent info all				// prints list of info of All ents
		if ( arg1 && arg1[0] && !strcmp( "all", arg1 ) ) {
			entNode_t *node = me_ents->gethead();
			if ( !node ) {
				console.Printf("no entities" );
			} else {
				while( node ) {
					G_PrintEntInfo( node->val );
					node = node->next;
				}
			}
		} else {
			node_c<entNode_t*> *node = entSelected.gethead();
			if ( !node ) {
				console.Printf( "no entities selected" );
			}
			while ( node ) {	
				G_PrintEntInfo( node->val->val );
				node = node->next;
			}
		}
	}

	// ent name <new_name>			// sets the ent name
	else if ( !strcmp( "name", cmd ) ) {
		if ( entSelected.size() != 1 ) {
			console.Printf( "exactly ONE entity must be selected to set the name" );
		}
		else if ( arg1 && arg1[0] ) {
			node_c<entNode_t*> *N = entSelected.gethead();
			char _old[256];
			strcpy( _old, N->val->val->name );
			strcpy( N->val->val->name, arg1 );
			console.Printf( "%s name changed to: \"%s\"", _old, arg1 );
		} else {
			console.Printf( "%-20s %s", "ent name <name>", "set name of a single selected entity" );
		}
	}

	// ent saveview
	else if ( !strcmp( "saveview", cmd ) ) {
		if ( entSelected.size() != 1 ) {
			console.Printf( "%-20s %s", "ent view|viewport", "saves current viewport to a single selected spawn entity" );
		} 
		else {
			Entity_t *e = entSelected.gethead()->val->val;
			Spawn_t *s = dynamic_cast<Spawn_t*>( e );
			if ( s ) {
				main_viewport_t *v = M_GetMainViewport();
				s->view[0] = v->world.x;
				s->view[1] = v->world.y;
				s->view[2] = v->res->width;
				s->view[3] = v->res->height;
				s->zoom = v->ratio;
				s->set_view = true;
				console.Printf( "Spawn viewport set: %.1f %.1f %.1f %.1f Zoom: %.1f", s->view[0], s->view[1], s->view[2], s->view[3], s->zoom );
			} else {
				console.Printf( "Spawn viewport not set" );
			}
		}
	}

	// ent class 
	else if ( !strcmp( "class", cmd ) ) {
		if ( arg1 && arg1[0] ) {
			bool found = false;
			EntityClass_t ftype = EC_NODRAW;
			int i = 0;
			for ( i = 0; i < ENTITY_CLASSLIST_SIZE; i++ ) {
				if ( !strcmp( entityClasses[i].token, arg1 ) ) {
					ftype = entityClasses[i].entClass;
					found = true;
					break;
				}
			}
			if ( !found ) {
				console.Printf( "not a valid entClass.  try 'listentclasses'" );
			} else {
				node_c<entNode_t*> *e = entSelected.gethead();
				while ( e ) {
					e->val->val->entClass = ftype;
					e = e->next;
				}
				console.Printf( "All selected entities changed to class: %s", entityClasses[i].token );
			}
		} else if ( entSelected.size() == 0 ) {
			console.Printf( "no entities selected" );
		} else {
			console.Printf( "please supply an arg" );
		}
	}

	// ent spawntype
	else if ( !strcmp( "spawntype", cmd ) ) {
		node_c<entNode_t*> *e = entSelected.gethead();
		uint howmany = entSelected.size();
		if ( 0 == howmany ) {
			console.Printf( "no Spawn entities selected" );
		}
		while ( e ) {
			Spawn_t *s = dynamic_cast<Spawn_t*> ( e->val->val );
			if ( s ) {
				if ( s->type == ST_AREA && howmany > 1 ) {
					console.Printf( "can't change multiple to ST_WORLD" );
				} else { 
					s->type ^= 1;
					console.Printf( "changed to: %s", s->type == ST_WORLD ? "ST_WORLD" : "ST_AREA" );
				}
			} else {
				console.Printf( "entity not a spawn entity, or none selected" );
			}
			e = e->next;
		}
	}

	// ent angle
	else if ( !strcmp( "angle", cmd ) ) {
		if ( arg1 && arg1[0] ) {
			float _angle = atof( arg1 );
			node_c<entNode_t*> *e = entSelected.gethead();
			if ( !e ) {
				console.Printf( "no entities selected" );
			} else {
				int count = 0;
				while ( e ) {
					e->val->val->poly.angle = _angle;
					e->val->val->populateGeometry();
					e = e->next;
					count++;
				}
				console.Printf( "updated %d entities to angle %f", count, _angle );
			}
		} else {
			console.Printf( "please provide angle" );
		}
	}

	// ent width/height
	else if ( !strcmp( "width", cmd ) || !strcmp( "height", cmd ) ) {
		int H = strcmp( "height", cmd ) ? 0 : 1;
		if ( arg1 && arg1[0] ) {
			float _w = (float) atof( arg1 );
			node_c<entNode_t*> *e = entSelected.gethead();
			if ( !e ) {
				console.Printf( "no entities selected" );
			} else {
				int count = 0;
				while ( e ) {
					if (H) 
						e->val->val->poly.h = _w;
					else
						e->val->val->poly.w = _w;
					e->val->val->populateGeometry();
					e = e->next;
					count++;
				}
				console.Printf( "updated %d entities to %s %f", count, (H)?"height":"width", _w );
			}
		} else {
			if ( entSelected.size() == 1 ) {
				poly_t *p = &entSelected.gethead()->val->val->poly;
				console.Printf( "%s %f", (H)?"height":"width",(H)?p->h:p->w );
			} else {
				console.Printf( "please provide angle" );
			}
		}
	}

	// ent X/Y
	else if ( 	!strcmp( "X", cmd ) || !strcmp( "x", cmd ) || 
				!strcmp( "Y", cmd ) || !strcmp( "y", cmd ) ) {
		if ( arg1 && arg1[0] ) {
			int which = tolower( cmd[0] );
			float delta = (float) atof( arg1 );
			node_c<entNode_t*> *e = entSelected.gethead();
			if ( !e ) {
				console.Printf( "no entities selected" );
			} else {
				int c = 0;
				while( e ) {
					if ( which == 'x' ) {
						e->val->val->poly.x += delta;
					} else {
						e->val->val->poly.y += delta;
					}
					e->val->val->populateGeometry();
					e = e->next;
					++c;
				}
				console.Printf( "shifted %d entities %s value by %f", c, which == 'x' ? "X" : "Y", delta );
			}
		} else {
			console.Printf( "please provide a parameter to move by" );
		}
	}

	// ent clip [x|y|w|h] <value>
	else if ( !strcmp( "clip", cmd ) ) {
		if ( !arg1 || !arg1[0] || !arg2 || !arg2[0] ) {
			console.Printf("please supply a parameter and amount to move clip model by");
		} else {
			G_EntClipCmd( arg1, arg2 );
		}
	}

	// ent trig [x|y|w|h] <value>
	else if ( !strcmp( "trig", cmd ) ) {
		if ( !arg1 || !arg1[0] || !arg2 || !arg2[0] ) {
			console.Printf("please supply a parameter and amount to adjust trigger");
		} else {
			G_EntTrigCmd( arg1, arg2 );
		}
	}

	// ent pl_speed <value>
	else if ( !strcmp( "pl_speed", cmd ) ) {
		if ( arg1, arg1[0] ) {
			node_c<entNode_t*> *e = entSelected.gethead();
			uint howmany = entSelected.size();
			if ( 0 == howmany ) {
				console.Printf( "no entities selected" );
			} else if ( howmany > 1 ) {
				console.Printf( "select just one Spawn point to set speed" );
			} else {
				Spawn_t *s = dynamic_cast<Spawn_t*> ( e->val->val );
				if ( s ) {
					strncpy( s->pl_speed, arg1, 31 );
					s->pl_speed[31] = '\0';
					console.Printf( "stored player speed: %s in spawn point", s->pl_speed ); 
				} else {
					console.Printf( "selected entity is not spawn" );
				}
			}
		} else {
			console.Printf( "supply a speed to load for the spawned player" );
		}
	}

	// ent door ...
	else if ( !strcmp( "door" , cmd ) && arg1 && arg1[0] ) {
		G_DoorModify( arg1, arg2, arg3 );
	}
	
	// ent portal ...
	else if ( !strcmp( "portal", cmd ) && arg1 && arg1[0] ) {
		G_PortalModify(arg1, arg2, arg3 );
	}

	// ent sync
	else if ( !strcmp( "sync", cmd ) ) {
		node_c<entNode_t*> *e = entSelected.gethead();
		uint howmany = entSelected.size();
		if ( 0 == howmany ) {
			console.Printf( "no entities to sync selected" );
		} else {
			int _count = 0;
			while ( e ) {
				++_count;
				e->val->val->populateGeometry();
				e = e->next;
			}
			console.Printf( "syncing col, clip, & trig of %d entities", _count );
		}
	}

	// ent <token>					// shorthand for entity
	else {
		for ( int i = 0; i < ENTITY_TYPELIST_SIZE; i++ ) {
			if ( !strcmp( entityTypes[i].token, cmd ) ) {
				G_CreateEntity( cmd );
				console.Printf( "entity: %s created", cmd );
				break;
			}
		}
	}
}

//==============================================================================
//==============================================================================
//==============================================================================

