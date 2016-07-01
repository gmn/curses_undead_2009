#ifndef __G_ENTITY_H__
#define __G_ENTITY_H__


//#undef __COM_GEOMETRY_H__
#include "../common/com_geometry.h"


// get rid of it if possible
//#include "g_enthelper.h" // shoot_s, ... GOES FIRST!

#include "../lib/lib_list_c.h" // buffer_c ..
#include "../lib/lib_buffer.h" // buffer_c ..

//#include "../common/common.h" // gvar_c
#include "../common/com_gvar.h" // gvar_c
#include "../common/com_object.h"
#include "../map/mapdef.h"
#include "../renderer/r_lerp.h" // lerpFrame_c

//#include "../map/m_area.h" // added this for Area_t, but causing fuckup




extern gvar_c *sv_fps;
extern gvar_c *g_fireRate;
extern gvar_c *g_zombieLogicTic;

/* Priority Entities 
--------------------
x	Spawn_t
x	Player_t
x	Portal_t
x	Door_t
x	Robot_t
	Turret_t
	Meep_t

	Zombie_t
	Projectile_t
	Item_t
	Trigger_t
	Snake_t
--------------------
*/

/*

Entities ( All types, for now )

- Entity_t

- Trigger_t		causes an event resulting from collision, usually invisible
- Portal_t		causes a player to move to a new area, may spawn a confirmation
				dialog
- Spawn_t		an Invisible entity whose job it is to simply possess 
				information about a spawn point that we are spawning IN TO. 
				(not from), so really, it doesn't collide or anything.  only
				useful in setting up spawn points for maploads
- Door_t		can be locked, unlocked, closed, opened. when closed, a door
				acts as a simple barrier
				when opened acts as a portal OR a trigger, in that it
				can trigger a subArea, or portal to a new Area.  Or simply
				just open and allow passage.
- Item_t		Food, Tool, Weapon, Key, Money, Valuable, Medical, cellphone, 
				in most cases you automatically pick the thing up.  Your
				inventory may be full, in which event a simple dialogbox pops
				up informing you of this fact and nothing happens, the item
				left where it sits.  Perhaps you only pick things up when the
				USE key is pressed in their presence.
- Projectile_t	Bullets, hurled items.  pretty straight forward.  Fly in a 
				straight line, may be an animation_t, when they hit a target
				may spawn a sound+Animator+ cause damage.  (all 3 are events)
- NPC_t			can be Used (start dialog screen), are thinkers, may be hostile,
				can be traded with.
- Furniture_t	can be used (open drawer, open cabinet, ..), can contain items.
				can be locked, which means keys can be used on them.  
- Computer_t	can be used (start dialog screen)
- Decal_t		just draws.  Can be timed or permanent.  Also, can be delayed,
				meaning, can be set to appear 5 seconds from now in the future.
- Animator_t	Explosions as result of projectile hitting something, 
				snow flakes, toxic particles, ???, ...
- DialogBox_t	Block of text inside a bordered box, which is spawned in a 
				certain area.  Usually timed, so that when time runs out it 
				vanishes.  type2: floatingText numbers pop up above the head, 
				like +1 in CaveStory.  could be combined with dialogBox, just 
				with a noBackground parameter set.
- Player_t		...
- Robot_t
- Snake_t
- Zombie_t
*/


// gun handling
struct shoot_s {
	int msPerShot;
	unsigned int last;
	bool shooting;
	shoot_s() : msPerShot(800), last(0), shooting(0) {}
	shoot_s( int m ) : msPerShot(m), last(0), shooting(0) {}
	// must be consistently updated to keep firing
	void notify( unsigned int ); 
	bool check(); // are we firing this frame
	void reset() { last = 0; shooting = 0; }
} ;


// later starter classes.  aren't initialized (allocated) if they aren't used
class _boxen_t : public buffer_c<colModel_t*>, public Allocator_t {
	bool started;
public:
	_boxen_t() : started(0) {}
	void add( colModel_t *b ) {
		if ( !started ) {
			init( 4 );
		}
		started = true;
		buffer_c<colModel_t*>::add( b );
	}
};

class _pvec_t : public buffer_c<struct twoflt_t>, public Allocator_t {
private:
	bool started;
public:
	_pvec_t() : started(0) {}		
	
	void add( float x, float y ) {
		twoflt_t tmp( x, y );
		if ( !started ) {
			init( 4 );
		}
		started = true;
		buffer_c<twoflt_t>::add( tmp );
	}
};





/*
=============================================================================

	Entity_t

=============================================================================
*/
enum EntityType_t {
	ET_ENTITY,
	ET_TRIGGER,
	ET_PORTAL,
	ET_SPAWN,
	ET_DOOR,
	ET_ITEM,
	ET_PROJECTILE,
	ET_NPC,
	ET_FURNITURE,
	ET_COMPUTER,
	ET_DECAL,
	ET_ANIMATOR,
	ET_DIALOGBOX,
	ET_PLAYER,
	ET_ROBOT,
	ET_SNAKE,
	ET_ZOMBIE,
	ET_TURRET,
	ET_MEEP,
	TOTAL_ENTITY_TYPES
};

struct entityTypeList_t {
	EntityType_t type;
	const char * code; 
	const char * token;
};

extern entityTypeList_t entityTypes[];


/* 
these are how entity draw orders are determined.  Furniture draws before
decals.  Items draw after decals and furniture but before thinkers, etc..
Thinkers are characters & projectiles. Effects are explosions, floating text,
*/
enum EntityClass_t {
	EC_NODRAW,		// trigger, portal
	EC_FURNITURE,
	EC_DECAL,
	EC_ITEM,
	EC_THINKER,
	EC_EFFECT,
	TOTAL_ENTITY_CLASSES
};

struct entityClassList_t {
	EntityClass_t entClass;
	const char * code;
	const char * token;
};

extern entityClassList_t entityClasses[];

/* collision handler hierarchy (between Entity_t)
	OUT + OUT			: no effect, objects of the same type. 
						: won't happen in practice
	IN + OUT 			: The easy case, just pass it to IN
	IN + IN	(uneven)	: ranked case, the IN w/ the higher rank takes it
	IN + IN (even)   	: whichever gets it, handles it
*/
enum {
	OUT_TYPE, 	// always gets passed on to the other, unless the other is OUT,
				// then the event gets dropped
	IN_TYPE1,	// turret
	IN_TYPE2,	// projectile
	IN_TYPE3,	// robot
    IN_TYPE4,   // zombie
	IN_TYPE5,	// player
	IN_TYPE6, 	//
	IN_TYPE7,   
	IN_TYPE8,
	IN_TYPE9,   // portal, door
};
/* IN hieraarchy: Portal & Door >> Player >> Robot >> Projectile .. */

enum {
	RESULT_NORMAL,
	RESULT_NOCLIP,	// result of trigger says skip collision
};

#define ENTITY_NAME_SIZE 32

/*
============================================
 Entity_t

	core entity class.  Not Abstract!  but should be inherited from to
	create specialized entities.  the advantage to setting a custom
	'void (*think)(void)' function instead of a pointer is that the think 
	function has access to class data members
============================================
*/
class Entity_t : public Auto_t {
protected:
	// subclasses must override these if they contain additional members
	// that reside in heap memory
	virtual void _my_init();
	virtual void _my_reset();
	virtual void _my_destroy();
	static int id;
public:
	char							name[ ENTITY_NAME_SIZE ];
	EntityType_t					entType;
	EntityClass_t 					entClass;
	int								ioType;

	// visual
	class animSet_t *						anim;
	class material_t *					mat;
    class animation_t *             curAnim;
    class animation_t *             lastAnim;

	// geometric
	struct poly_t 							poly;
	AABB_t							aabb;
	OBB_t							obb;
	AABB_t							wish; // requested player move

	// meta-collision
	bool 							collidable; 			
	colModel_t *					col;

	// trigger
	bool							triggerable;
	AABB_t							trig;

	// physical
	bool							clipping;
	AABB_t							clip;

	// where are we
	class Block_t *						block;


	///////////////////////////////////////////////////////////////////
	// Internal shit ( not copied on copy-constructor )
	///////////////////////////////////////////////////////////////////
	bool							paused;
	timer_c							pause_timer;

	//
	vec2_t							prevCoord;
	vec2_t							lastDrawn; // for drawing extrapolation

	colModel_t 						col_save;

	// 
	int								in_collision;

	// timer
	timer_c 						timer;

	// collected return projections from mapTile collisions
	class _pvec_t 					proj_vec;			
	// saved collision boxes from all MapTiles hit in a move (and ents)
	class _boxen_t 					boxen;

	// gun timer management
	shoot_s 						shoot;

	// last recorded move made. sets facing direction 
	unsigned int 					facing;
	unsigned int					facingLast; // last direction we have info for
	byte 							delete_me; // mark for deletion at end of turn

	lerpFrame_c						lerp; // last 2 x,y locations. used for drawing interpolation


	// default constructor sets a predictabe state.  allocation is caused by 
	//  direct user request
	Entity_t() : entType(ET_ENTITY), entClass(EC_NODRAW), anim(0), mat(0), col(new colModel_t()), col_save(), proj_vec(), boxen(), shoot() { init(); }
	Entity_t( Entity_t const & ) ;
	virtual ~Entity_t() { if ( V_isStarted() ) { destroy(); delete col; } }

	// thinker ability
	virtual void 					think( void ) {} /* noop unless extended */ 

    virtual void                    startDelete() {
	    delete_me |= 0x3; // mark for deletion
    }
	
	// master collision handler system
	/* routs direction of call to _handle */
	void							handle( Entity_t ** ) ;
	/* specific handler, supplied by entity */
	virtual void					_handle( Entity_t ** ) {} 

	// trigger event  
	virtual int						trigger( Entity_t ** ) { return 0; }


	// function uses RTTI to copy ents on a per-type basis
	static Entity_t * CopyEntity( Entity_t * );
	Entity_t * Copy( void ) { return CopyEntity( this ); }

	void AutoName( const char *, int * =NULL );


	// defaults to setting all by poly
	virtual void populateGeometry( int colmod =7 ) {
		poly.toAABB( aabb );
		poly.toOBB( obb );
		if ( col && colmod & 1 ) {
			COPY4( col->box, aabb );
		}
		if ( colmod & 2 ) {
			COPY4( trig, aabb );
		}
		if ( colmod & 4 ) {
			COPY4( clip, aabb ) ;
		}
	}
	void propagateDimension( int colmod =1 ) { populateGeometry( colmod ); } 
	void propagateDimensions( int colmod =1 ) { populateGeometry( colmod ); } 


	void getDrawCoord( float *v, float frac, int =1 ); // default to internal
	
	void move( float , float );
	void setCoord( float, float );

	// experimental
	void undoMove( void );
	void bounceBack( float f =2.0f );

	// respond to a tile collision. might result in changed location
	virtual void handleTileCollisions( void ) ;

	material_t *lastMat;
	material_t *getMat();

	// entities extend this function, caller calls getMat()
	virtual material_t *_getMat() { return mat; }
	
	virtual void setupDrawSurfs() { mat = NULL; anim = NULL; }

	// the way we are trying to move, but also corrected
	void wishMove( float dx, float dy );

	int getID() { return id; }

	Block_t * blockLookup( void * =NULL ); // Area_t*
	Block_t * blockLookupByName( const char * =NULL );

	void blockRemove( void );

	void shiftClip( float *);
	void shiftTrig( float * );

	void BeginMove( void );
	virtual void FinishMove( void );
	
	void connect( Block_t * ); 

	void pause( int ); // someone told us to pause

	// sets colMod to wrap trig & clip
	void updateColBoxes( void );
		
	void handleEntityCollision( Entity_t * );

	void adjustFromEntityCollisions( void );

	virtual void fireWeapon( void ); // default weapon for enitty

	void setFacing( float * );
};


/*
====================================================================

	Spawn_t

====================================================================
*/
enum {
	ST_AREA, 				// can be any number of these
	ST_WORLD, 				// only 1 of these per mapFile/World
};
enum { // states
	SPAWN_NORMAL,
	SPAWN_BLOCKING,
};

struct SpawnTypeList_t {
	int type;
	const char * code; 
	const char * token;
};

class Spawn_t : public Entity_t {
private:
	void _my_reset() {
		Entity_t::_my_reset();
		entType = ET_SPAWN;
		entClass = EC_NODRAW;
	}
	static int id;
public:
	Spawn_t() {
		entType = ET_SPAWN;
		entClass = EC_NODRAW;
		main_viewport_t *v = M_GetMainViewport();
		view[0] = v->world.x;
		view[1] = v->world.y;
		view[2] = v->res->width;
		view[3] = v->res->height;
		zoom = v->ratio;
		type= ST_AREA;
		AutoName( "SPAWN", &Spawn_t::id );
		collidable = false;
		triggerable = false;
		clipping = false;
		ioType = OUT_TYPE;
		this->pl_speed[0] = 0; 
		set_view = false;
		state = SPAWN_NORMAL;
		timer.reset();
	}

	int type;
	bool set_view;
	int state;

	// sprite spawn direction is given by poly.angle

	// saved zoom
	float zoom;

	// saved viewport
	float view[4];

	// suggested player speed (for scaling), floating point given as string
	char pl_speed[ 32 ];

	// for use externally, a caller can tell this spawn to block the action
	// where he's at.  particularly, if a portal from a different area, needs
	// something to happen in an area they are sending someone to
	void setBlockWait( int );

	// polling
	void think( void );
};
// end Spawn_t


/*
==============================================================================

	Portal_t

==============================================================================
*/
#define PORTAL_NAME_SIZE 32

enum PortalType_t {
/* can be FAST or FREEZE or WAIT or FREEZEWAIT. */ 
	PRTL_FAST 		= 1,	// load next area right away
	PRTL_FREEZE		= 2,	// freeze cur scene for time t, then teleport
	PRTL_WAIT		= 4,	// load area and wait until timer
	PRTL_FREEZEWAIT = (PRTL_FREEZE|PRTL_WAIT),
	PRTL_MASK_LOW 	= 7,

/* any of those can also be EXPIRE or ONETIME */
	PRTL_EXPIRE		= 8,	// lasts only so long, then goes away
	PRTL_ONETIME	= 16,	// goes away after first use

	// state flags
	PRTL_SLEEPING,				// nothing, returns
	PRTL_FROZEN,				// began a freeze
	PRTL_LOADING,				// map load frame
	PRTL_WAITING,				// wait after load

	PRTL_DONE,
};


#ifndef AREA_NAME_SIZE
#define AREA_NAME_SIZE 32
#endif

class Portal_t : public Entity_t {
private:

	void _my_reset() {
		Entity_t::_my_reset();
		entType = ET_PORTAL;
		entClass = EC_NODRAW;
	}
	static int id;
public:
	char				area[ AREA_NAME_SIZE ]; 	// target area
	char				spawn[ ENTITY_NAME_SIZE ]; 	// target spawn
	int					type;
	int					state;	
	int					freeze;
	int					wait;
	Entity_t *			caught;
	bool				teleport_active;
	Spawn_t *			spawnDest;				// set this in teleport
	void *				areaDest;

	Portal_t() {
		entType = ET_PORTAL;
		entClass = EC_NODRAW;
		ioType = IN_TYPE9;
		collidable = true;
		triggerable = true;
		clipping = false;

		strcpy( area, "_none_specified_" );
		strcpy( spawn, "_none_specified_" );
		type = PRTL_FREEZE;
		state = PRTL_SLEEPING;
		freeze = 666;
		wait = 777;
		AutoName( "Portal", &Portal_t::id );
		caught = NULL;
		teleport_active = false;
		spawnDest = NULL;
		areaDest = NULL;
	}

    Portal_t( Portal_t const & P ) : Entity_t( P ) {
	    strncpy( area, P.area, AREA_NAME_SIZE ); 	
        strncpy( spawn, P.spawn, ENTITY_NAME_SIZE ); 	
        type = P.type;
        state = P.state;
        freeze = P.freeze;
        wait = P.wait;
        caught = P.caught;
        teleport_active = P.teleport_active;
        spawnDest = P.spawnDest;
        areaDest = P.areaDest;
    }

	virtual int trigger( Entity_t ** );
	virtual void think( void ) ; // polls the pause &/or delay timers
	void _handle( Entity_t ** );

	int teleportEntity( Entity_t * );
	void sleep( void );
	void activate( void );
};
// end Portal_t


/*
=============================================================================

	Projectile_t

=============================================================================
*/
void G_ProjectileInits( void );

enum {
	// states
	PROJ_DONE,
	PROJ_EXPLODING,
	PROJ_MOVING,
	PROJ_FLYING,
	// types
	PROJ_LASER,
	PROJ_FIREBALL,
	PROJ_TURRETBALL,
};
class Projectile_t : public Entity_t {
private:

	void _my_reset() {
		Entity_t::_my_reset();
		entType = ET_PROJECTILE;
		entClass = EC_EFFECT;
	}

	// velocity
	// direction
	// visual
	// does it spawn a timedAnimator (explosion) when it hits something,
	//  or a Decal? 
	// sound info when it hits something
	// to hit damage info

	static int laser_id;
	static int turretball_id;
public:
	static int client_id; 	// gets client_id from V_Malloc once, creates
							// our own page for projectiles only

	void virginize() {
		entType = ET_PROJECTILE;
		entClass = EC_EFFECT;
		collidable = true;
		triggerable = false;
		clipping = true;
		ioType = IN_TYPE2; // handles walls, robots & players handle it
		mat = NULL;
		anim = NULL;
		G_ProjectileInits();
		state = PROJ_FLYING;
		explode_time = 400;
        seenOpenSpace = false;
	}

	Projectile_t() { 
		virginize() ; 
		AutoName( "Projectile" );
	}
	Projectile_t( int _type, int, float * _direction, float * _startpoint );
    ~Projectile_t() {}    

	int type; 					// determines visual, damage and velocity
	float dir[2];
	int damage;
	int vel; 					// velocity
	int state;
	int explode_time;
    bool seenOpenSpace;         // isn't clipping until it's seen open space

	void think( void );
	void _handle( Entity_t ** );
	void handleTileCollisions( void ); // projectiles behave differently
	material_t * _getMat();

	// mem page stuff
	static int origMemID;
	static void setMemPage( void ); 
	static void retMemPage( void );

	void explode( void ); // tell bullet to start to explode

	void GeneralClippingHandler( Entity_t ** );

    virtual void startDelete() {
	    state = PROJ_DONE;
	    delete_me |= 0x3; // mark for deletion
    }
};
// end Projectile_t




/*
==============================================================================

	Door_t

==============================================================================
*/

enum {
	// state
	DOOR_CLOSED,
	DOOR_OPENING,
	DOOR_CLOSING,
	DOOR_OPEN,

	// lock
	DOOR_LOCKED,
	DOOR_UNLOCKED,

	// type
	DOOR_PROXIMITY		= 8,			// opens by proximity
	DOOR_MANUAL			= 16,			// must be used to open
	DOOR_LOCKABLE 		= 32,			// has to have this to be locked
	DOOR_PERMANENT 		= 64,			// once opened stays open

	// proximity, but once its open, it stays open
	DOOR_PROXPERM = (DOOR_PROXIMITY|DOOR_PERMANENT),
	// man perm is door that must be opened manually, but closes automatically
	DOOR_MANPERM = (DOOR_MANUAL|DOOR_PERMANENT),


	// color 
	DOOR_COLOR_NONE		= 100,
	DOOR_COLOR_RED,
	DOOR_COLOR_BLUE,
	DOOR_COLOR_GREEN,
	DOOR_COLOR_SILVER,
	DOOR_COLOR_GOLD,

	// dir
	DOOR_UP,
	DOOR_RIGHT,
	DOOR_DOWN,
	DOOR_LEFT,
	DOOR_SPLIT, // not imp
	DOOR_SWING, // not imp

};


class Door_t : public Portal_t {
private:

	void _my_reset() {
		Entity_t::_my_reset();
		entType = ET_DOOR;
		entClass = EC_FURNITURE;
	}

	void _my_destroy();

	static int id;

	// can be activated by the activate key changing its open/close state

	// can be locked, preventing change of open/close state

	// can be activated by a key, changing it's locked state

	// if open, it becomes a portal, and entity collision causes portal action
	//  (move to new spot)

public:
	Door_t() {
		entType = ET_DOOR;
		entClass = EC_FURNITURE;
		state = DOOR_CLOSED;
		lock = DOOR_UNLOCKED;
		type = DOOR_PROXIMITY;

		// make a full copy of the material because we'll be messing w/ the
		// tex coords
		mat = materials.FindByName( "gfx/ent_notex.tga" );
		mat = mat ? new material_t( *mat ) : new material_t();

		anim = NULL;
		open_amt = state == DOOR_CLOSED ? 0.0f : 1.0f;
		ioType = IN_TYPE6;
		AutoName( "Door" , &Door_t::id );
		collidable = true;
		triggerable = true;
		clipping = true;
		waittime = 3000;
		doortime = 1000;
		dir = DOOR_LEFT;
		frac_mv = doortime / 1000.f / sv_fps->value(); //amt to move per sv frame
		timer.reset();
		dtimer.reset();
		hitpoints = 100;
		use_portal = false;
		color = DOOR_COLOR_NONE;
	}


    Door_t( Door_t const & ) ;
	~Door_t() { /*this->_my_destroy();*/ }


	// Door State
	int state; // open, opening, closed, closing
	int lock; // locked, unlocked
	int type; // [ proximity, manual, PROXPERM ] 
	int dir; // up, right, left, down

	// manage open/close animation
	float open_amt;
	int doortime;	// time it takes to open (ms)
	int waittime;	// time automatic doors take to start closing again (ms)
	float frac_mv;  // amount to open/close per server frame

	poly_t savepoly; // original poly location

	int hitpoints;  // some doors will eventually break up from zombie abuse
	bool use_portal;

	timer_c dtimer; // door gets its own timer because Portal needs its
	
	int color;

	// called by entity collision or USE_ACTION.  If door is manual, it
	// requires a use action, else proximity is good enough
	int trigger( Entity_t ** ) ; 
	void _handle( Entity_t ** ) ;

	void think( void ); // the polling function

	void setOpenAmt( float );

	// overriding so we catch original location to savepoly
	void populateGeometry( int colmod =7 ) {
		Entity_t::populateGeometry( colmod );
		COPY_POLY( &savepoly, &poly );
	}

	// checks what keys the player has on him
	bool checkKey( Entity_t * );
	// checks player's current use state
	bool checkUse( Entity_t * );


	void ProjectileHandler( Projectile_t * );
};
// end Door_t


/*
==============================================================================

 Turret_t

==============================================================================
*/
enum {
	// turning or sitting still?
	TURRET_DORMANT,
	TURRET_CWISE,
	TURRET_CCWISE,
};

class Turret_t : public Entity_t {
private:
	void _my_reset() {
		Entity_t::_my_reset();
		entType = ET_TURRET;
		entClass = EC_THINKER;
	}
	static int id;
public:
	Turret_t() {
		entType = ET_TURRET;
		entClass = EC_THINKER;
		collidable = true;
		triggerable = false;
		clipping = false;
		ioType = IN_TYPE1; // handles projectiles only, player & robot have
							// no effect
		AutoName( "Turret", &Turret_t::id );
		anim = NULL;
		mat = NULL;

		playerSeen = false;
		scanRadius = 4096;
		shoot.msPerShot = 2400;
		turn_timer.reset();
		turn_time = 800; // time it takes to turn 45 degrees

		angle = poly.angle; // maybe we can do this?
		state = TURRET_DORMANT;
		threshold = 5.0f; // must be at least this many degrees off of player
							// to move
		facing = facingLast = DirFromAngle();
		shoot_window = 10.f; // how many degrees off of dead center we have to be before we can shoot
	}
	bool playerSeen; // if player seen it turns to face player and fires
	float scanRadius;

	static animSet_t * turretAnimSet;
	timer_c turn_timer;
	int turn_time;

	float angle;
	int state;
	float threshold;
	
	float player_dir[2];

	void think( void );
	void ProjectileHandler( Projectile_t * );
	void _handle( Entity_t ** );
	static void buildAnims( void );
	material_t * _getMat( void ) ;

	void FinishMove( void ) ; // need our own since we don't move
	int DirFromAngle();
	float shoot_window;
};
// end Turret_t


/*
==================================================================

 Robot_t

==================================================================
*/
extern animSet_t *robotAnimSet;

struct randomDirection_t {
	vec2_t v; // direction currently moving
	float dist; // how far we're going to move in that direction
	float sofar;
	void begin( float *_v ) { 
		sofar = 0.f;
		v[0] = _v[0]; v[1] = _v[1]; 
	}
	int check() {  
		if ( sqrtf(v[0]*v[0]+v[1]*v[1]) < 0.9f )
			return 1;
		return sofar >= dist; 
	} 
	void newdir();
	void newAxisDir();
	randomDirection_t() { 
		dist = 1024; sofar = 1024; 
	}
};

struct f4_t { float v[4]; };


// states
enum {
	ROBOT_SENTRY,		// stay in a certain area, 
	ROBOT_TRACKING,
	ROBOT_DIRECTIONAL, // random movement in 8 directions
	
	// states above ROBOT_DIRECTIONAL void collision handling, so put any
	// new thinking states above it.  ones below serve other functions

	ROBOT_PAUSED,

	// stages of blowing up
	ROBOT_START_EXPLODING,
	ROBOT_EXPLODING,
	ROBOT_DONE,

	// sentry duty states
	ROBOT_SNOOZE,
	ROBOT_ATTACKING,
	ROBOT_LOST_HUNTED,
	ROBOT_PACING,

	ROBOT_LATERAL,
	ROBOT_VERTICAL,
	ROBOT_POSITIVE_INCLINE,
	ROBOT_NEGATIVE_INCLINE,
};


struct sentry_t {
	int wait;
	timer_c timer;
	byte dir;
	int steps;
	int state;
	int type;
	sentry_t() {
		wait = 0;
		timer.reset();
		dir = 1;
		steps = 0;
		state = ROBOT_SNOOZE;
		type = ROBOT_LATERAL;
	}
};


void G_InitGame( void );

/* 
====================
	Robot_t 
====================
*/
class Robot_t : public Entity_t {
private:
	void _my_reset() {
		Entity_t::_my_reset();
		entType = ET_ROBOT;
		entClass = EC_THINKER;
	}
	static int id;
public:
	Robot_t() : ScanRadius(3300), sentry() 
	{
		entType = ET_ROBOT;
		entClass = EC_THINKER;
		collidable = true;
		triggerable = false;
		clipping = true;
		ioType = IN_TYPE3;
		AutoName( "Robot", &Robot_t::id );
		playerSeen = true;
		hunted = NULL;
		state = ROBOT_SENTRY;
		dir.dist = 4096;
		angry = false;
		hitpoints = 32;
		explode_time = 900;
		hullRemovalTime = 90000; // 0 leaves destroyed remnants indefinitely
		mat = NULL;
		mat = materials.FindByName( "gfx/SPRITE/ROBOT/robotR00_L.tga" );
		anim = NULL;
		// makes sure the primary robot animation is built so we can get a copy 
		// of it
		G_InitGame(); 
		anim = new animSet_t( *robotAnimSet );

		player_dist = 1000000.0f;
		angry_timer.reset();

		shoot.msPerShot = 1000;
		projHit = 0;
		damage = 1; // damage it does from touch
	}
    Robot_t(Robot_t const &) ;

	randomDirection_t dir;

	int state ;
	bool playerSeen;
	Entity_t * hunted;
	bool angry;
	int hitpoints;
	int explode_time;
	int hullRemovalTime;
	
	static animSet_t * robotAnimSet;

	static void buildAnims( void );

	void think( void );
	void _handle( Entity_t ** );
	int trigger( Entity_t ** );

	void RobotHandler( Robot_t * );
	void ProjectileHandler( Projectile_t * );

	void GatherSelfTogether( int ) ; 

	material_t * _getMat() ;

	void GeneralClippingHandler( Entity_t ** ); //
	bool SentryDuty( float * );
	void Scan( void );

	sentry_t sentry;
	const int ScanRadius;
	float player_dist;
	timer_c angry_timer;
	float lastTraj[2];
	int projHit;
	float projTraj[2]; // direc

	// line of sight aabb list
	buffer_c<f4_t> los;

	int damage;
};
// end Robot_t

#if 0
/*
=============================================================================

 Item_t

=============================================================================
*/
enum {
	ITEM_NOTYPE,

	// iclass -- broad item class
	ITEM_FOOD,		
	ITEM_TOOL,		
	ITEM_WEAPON,	
	ITEM_KEY,			

	// itype  --  specific type 
	ITEM_CELLPHONE,
	ITEM_TURKEY_LEG,
	ITEM_MEDKIT1,
	ITEM_MEDKIT2,
	ITEM_FLASHLIGHT,
	ITEM_5_GALLON_WATER_JUG,
	ITEM_SLEEPING_BAG,
	ITEM_CHAINSAW,
	ITEM_ALUMINUM_BAT,
	ITEM_ROCK,
	ITEM_BOAT_KEYS,
	ITEM_BEEF_JERKY,
	ITEM_SMOKED_SALMON,
	ITEM_TENT,
	ITEM_DIGITAL_WATCH,

	ITEM_KEY_RED,
	ITEM_KEY_BLUE,
	ITEM_KEY_GREEN,
	ITEM_KEY_SILVER,
	ITEM_KEY_GOLD,
};


class Item_t : public Entity_t {
private:

	static int id;

	void _my_reset() {
		Entity_t::_my_reset();
		entType = ET_ITEM;
		entClass = EC_ITEM;
	}
	
	// tells which item it is
	// index into a big linear lookup table
	// struct items_s { char name[32]; ... } items[] = { { "baseball" }, { "beef jerky" }, };

	// but items also need to have attributes.  do they give you health, are
	// they a key, a weapon.  I spose that there can be about 6 different 
	// classes of items: food, weapon, tool, 

	// need sub-system just to create each Item type, store image, and meta-
	//  data, and then gen a master list of items, or create an item factory
	//  method capable of taking the code as argument, creating the Entity
	
public:

	void basicShit() {
		entType = ET_ITEM;
		entClass = EC_ITEM;
		ioType = OUT_TYPE;
		collidable = true;
		clipping = true;
		AutoName( "Item", &Item_t::id );
		
		// isn't fully typed until running through copy-ctor
		itype = ITEM_NOTYPE;
		iclass = ITEM_NOTYPE;

		anim = NULL;
		if ( !mat ) {
			mat = materials.FindByName( "gfx/MISC/item.tga" ); 
		}
	}
	Item_t() { basicShit(); }
	Item_t( Item_t const & );

	int iclass; // broad type
	int itype; // specific type

	// if it's health bareing.  0 is not-applicable
	int health;
	// if it can be used multiple times, how many?  0 is infinite
	int uses; 
};
// end Item_t
#endif






/* 
==================================================================

	NPC_t

	generic talking non-player-character.  Dialog and scripted actions can 
	be loaded from meta-file, therefor this class can be repurposed for many 
	different characters of similar complexity.  NPCs are also interactible
	to some degree.  You can "activate" them, and they will enter dialog and
	say something to you.  Some NPCs may just say "go away" and then the screen
	returns to the normal one. 
==================================================================
*/
class NPC_t : public Entity_t {
private:

	void _my_reset() {
		Entity_t::_my_reset();
		entType = ET_NPC;
		entClass = EC_THINKER;
	}

public:
	NPC_t() {
		entType = ET_NPC;
		entClass = EC_THINKER;
	}
};


/* 
=============================================================================

	Trigger_t 

=============================================================================
*/
class Trigger_t : public Entity_t {
private:
	void _my_reset() {
		Entity_t::_my_reset();
		AutoName( "Trigger" );
		entType = ET_TRIGGER;
		entClass = EC_NODRAW;
	}

	// invisible
	// only uses colModel and collisionHandler to spawn an event
	// could conceivably move around, but what for?

	// ** need to figure out Event_t mechanism.  in simplest modular form
	// a trigger would just create an Event and stuff it in a queue somewhere
	// to be processed down the line
public:

	Trigger_t() {
		entType = ET_TRIGGER;
		entClass = EC_NODRAW;
	}
};






// muzzle flash, explosion, things that spawn, animate and go away quickly
class Animator_t : public Entity_t {
private:

	void _my_reset() {
		Entity_t::_my_reset();
		entType = ET_ANIMATOR;
		entClass = EC_EFFECT;
	}

	// collidable: may have blast damage radius

	// animates over a finite amount of time, expiring at the end
public:

	Animator_t() {
		entType = ET_ANIMATOR;
		entClass = EC_EFFECT;
	}
};

// decals just sit there
class Decal_t : public Entity_t {
private:

	void _my_reset() {
		Entity_t::_my_reset();
		entType = ET_DECAL;
		entClass = EC_DECAL;
	}

	// have visual and spatial info

	// render over dispTiles (of course)

	// dont collide 
	// dont move
	// dont animate

	// .. empty class .. (just needs basic Entity_t)
public:

	Decal_t() {
		entType = ET_DECAL;
		entClass = EC_DECAL;
	}
};

class Furniture_t : public Entity_t {
private:

	void _my_reset() {
		Entity_t::_my_reset();
		entType = ET_FURNITURE;
		entClass = EC_FURNITURE;
	}

public:
	Furniture_t() {
		entType = ET_FURNITURE;
		entClass = EC_FURNITURE;
	}
};

class Computer_t : public Entity_t {
private:

	void _my_reset() {
		Entity_t::_my_reset();
		entType = ET_COMPUTER;
		entClass = EC_THINKER;
	}

public:
	Computer_t() {
		entType = ET_COMPUTER;
		entClass = EC_THINKER;
	}
};

class Meep_t : public Entity_t {
private:

	void _my_reset() {
		Entity_t::_my_reset();
		entType = ET_MEEP;
		entClass = EC_THINKER;
	}

public:
	Meep_t() {
		entType = ET_MEEP;
		entClass = EC_THINKER;
	}
};




/*
====================
====================
*/
class Dialogbox_t : public Entity_t {
private:

	void _my_reset() {
		Entity_t::_my_reset();
		entType = ET_DIALOGBOX;
		entClass = EC_EFFECT;
	}

public:
	Dialogbox_t() {
		entType = ET_DIALOGBOX;
		entClass = EC_EFFECT;
	}
};


/*
=============================================================================

  Zombie_t

=============================================================================
*/
enum {
    Z_STANDING,
    Z_STANDING_PENSIVE,
    Z_TURNING,
    Z_WALKING,
    Z_CHASING,
    Z_CHASING_BLIND, 
    Z_ATTACKING,
    Z_START_DYING,
    Z_DYING, 
    Z_DEAD
};
enum {
    Z_UP = 1,           // (2^0)
    Z_RIGHT = 2,
    Z_DOWN = 4,
    Z_LEFT = 8,         // (2^3)
};
class Zombie_t : public Entity_t {
private:
	void _my_reset() {
		Entity_t::_my_reset();
		entType = ET_ZOMBIE;
		entClass = EC_THINKER;
	}
	static int id;
	static animSet_t * zombieAnimSet;

	static void buildAnims( void );

    // set every time one is created 
    void baseInit( void ) {
        // base class
		AutoName( "Zombie", &Zombie_t::id );
		entType = ET_ZOMBIE;
		entClass = EC_THINKER;
		ioType = IN_TYPE4;
		collidable = true;
		triggerable = false;
		clipping = true;
		playerSeen = false;
        buildAnims();
		anim = new animSet_t( *zombieAnimSet );
        mat = NULL;
    }
    void thisInit() {
        // this class
        state = (rand()%100) > 50 ? Z_STANDING : Z_WALKING;
        stateTimer.set();

        int R = rand() % 4;
        walkDir = 1<<R;
        walkTimer.set();
        setFacingFromWalkDir();

        logicTimer.set( 1000 / g_zombieLogicTic->integer() ); 

        playerSeen = false;
        hunted = NULL;
        hitpoints = 18; // bat hits for 3, 3x6 hits to kill 
        hitdamage = 5; // does 5 damage on hit
        switch ( walkDir ) {
        case Z_UP:
            mat = anim->getMat( "up" );
            break;
        case Z_RIGHT:
            mat = anim->getMat( "right" );
            break;
        case Z_DOWN:
            mat = anim->getMat( "down" );
            break;
        case Z_LEFT:
            mat = anim->getMat( "left" );
            break;
        }
        turnTimer.reset();
        attackStart = false;
        wasAttacking = false;
        needTurn = false;
        playerLastSeenXY[0] = 0.f;
        playerLastSeenXY[1] = 0.f;
    }
public:

	Zombie_t() {
        baseInit();
        thisInit();
	}

    Zombie_t( Zombie_t const & Z ) : Entity_t ( Z ) {
        state               = Z.state;
        stateTimer.set( Z.stateTimer );
        walkDir             = Z.walkDir;
        walkTimer.set( Z.walkTimer );

        logicTimer.set( Z.logicTimer ); 

        playerSeen          = Z.playerSeen;
        hunted              = Z.hunted;
        hitpoints           = Z.hitpoints;
        hitdamage           = Z.hitdamage;
        turnTimer.set( Z.turnTimer ); // puts a pause between turns to prevent
                                // quickly occilating between two directions
        attackStart         = Z.attackStart;
        wasAttacking        = Z.wasAttacking;
        needTurn            = Z.needTurn;
        playerLastSeenXY[0] = Z.playerLastSeenXY[0];
        playerLastSeenXY[1] = Z.playerLastSeenXY[1];
    }

    
    // data
	int state ;
    timer_c stateTimer;
    int walkDir;
    timer_c walkTimer;
    timer_c logicTimer;

	bool playerSeen;
	Entity_t * hunted;
	int hitpoints;
    int hitdamage;
    timer_c turnTimer;
    bool attackStart;

	buffer_c<f4_t> los; // line of sight aabb list
	bool wasAttacking;  // useful in coordinating animation
    bool needTurn;
    float playerLastSeenXY[2];

    // methods
	void think( void );
	void _handle( Entity_t ** );
	int trigger( Entity_t ** );

	void ProjectileHandler( Projectile_t * );
    void ZombieHandler( Zombie_t * );

	material_t * _getMat() ;

    void handleState( void );
    void startToWalk();
    bool doRandomTurn();
    void doMove( float, float );
    void setFacingFromWalkDir();
    void ScanForPlayer( void );
    void startTurning();
    int reachedPlayerLastSeen( void );
};
// end Zombie_t


/*
*/
class Snake_t : public Entity_t {
private:
	void _my_reset() {
		Entity_t::_my_reset();
		entType = ET_SNAKE;
		entClass = EC_THINKER;
	}
public:
	Snake_t() {
		entType = ET_SNAKE;
		entClass = EC_THINKER;
	}
};


class Item_t;

/*
==============================================================================

	Player_t

==============================================================================
*/
enum {
	PLAYER_NORMAL,
	PLAYER_START_DYING,
	PLAYER_DYING,
	PLAYER_DEAD,
	PLAYER_RESPAWNING,
};

/* the main player.  This of course does not go in this file */
class Player_t : public Entity_t {

	void _my_reset() {
		Entity_t::_my_reset();
		entType = ET_PLAYER;
		entClass = EC_THINKER;
	}

public:


	int waitTime;
	int use;
	int hitpoints;

	int state ;
	int death_time;

	// STATS 
	int exp; // experience points
	int level; // d&d style level
	int stamina; // drains from exertion
	int score;
	int money;

	list_c<Item_t*> items;
	timer_c shock_timer;
	int shock_delay;

	Player_t(); 
	~Player_t() {} 
	void SetFromEntity( Player_t const & ); // use to set local player from 
											// data found in map load

	void setupDrawSurfs( void );
	material_t *_getMat(  void );

	void _handle( Entity_t ** );
	void think( void );

	void RobotHandler( Robot_t * );
	void ProjectileHandler( Projectile_t * );
	void ItemHandler( Item_t * );
    void ZombieHandler( Zombie_t * );
	bool isDead() { return hitpoints <= 0; }

    int pickupItem( Item_t * );
    void adjustHitpoints( int );

}; 
// end Player_t

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

void G_ConPrintEntityTypes( void );
int  G_CreateEntity( const char * );
Entity_t * G_NewEntFromType( EntityType_t type_num );
Entity_t * G_NewEntFromToken( const char * );
EntityClass_t G_EntClassFromToken( const char *clas );
EntityType_t G_EntTypeFromToken( const char *tok );
void G_EntCommand( const char *, const char *, const char *, const char * );

extern Player_t player;
extern gvar_c *pl_speed;

void P_FinishMove( void );
float * G_CalcBoxCollision( float *, float * );

void P_Shoot( int, int, float *, float * ); // projectile caller

#endif // __G_ENTITY_H__
