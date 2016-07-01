// g_projectile.cpp
//

#define __MAPEDIT_H__
#define __M_AREA_H__
#include "g_entity.h"
//#include "../common/common.h"

static animSet_t *pAnims = NULL;
gvar_c *g_fireRate = NULL;
gvar_c *g_laserVel = NULL;
extern gvar_c *platform_type;
extern float r_angle;
/*
animSet_t *anim_fireball = NULL;
animSet_t *anim_laser_hit = NULL;
animSet_t *anim_muzzle_flare = NULL;
*/

// how much memory for the whole projectile page
static const unsigned int PROJECTILE_BYTES = 150000;


int Projectile_t::laser_id = 0;
int Projectile_t::turretball_id = 0;
int Projectile_t::client_id = -1;
int Projectile_t::origMemID = -1;

extern filehandle_t explosion_sound[];



static bool started = false;
void G_ProjectileInits( void ) {
	if ( started )
		return;

/* shots should scaled as a ratio to size of the thinker firing them */

	pAnims = new animSet_t();
	animation_t * A = pAnims->startAnim();
	A->init( 4, "laser_hit" );
	A->frames[0] = materials.FindByName( "gfx/effects/explosion_small00.tga" );
	A->frames[1] = materials.FindByName( "gfx/effects/explosion_small01.tga" );
	A->frames[2] = materials.FindByName( "gfx/effects/explosion_small02.tga" );
	A->frames[3] = materials.FindByName( "gfx/effects/explosion_small03.tga" );

	A = pAnims->startAnim();
	A->init( 4, "turret_ball" );
	A->frames[0] = materials.FindByName( "gfx/effects/turretball00.tga" );
	A->frames[1] = materials.FindByName( "gfx/effects/turretball01.tga" );
	A->frames[2] = materials.FindByName( "gfx/effects/turretball02.tga" );
	A->frames[3] = materials.FindByName( "gfx/effects/turretball03.tga" );

    // create color material
    material_t *m = new material_t();
    strcpy( m->name, "color_red" );
    m->type = MTL_COLOR;
    m->color[0] = 1.0f; 
    m->color[1] = 0.f;
    m->color[2] = 0.f;
    m->color[3] = 1.0f; 
    materials.add( m );

	started = true;
}

Projectile_t::Projectile_t( int _type, int side, float *_direction, float *_startpoint ) : Entity_t() 
{
	virginize() ; 
	type = _type;
	dir[0] = _direction[0];
	dir[1] = _direction[1];
	float * s = _startpoint;

	switch( type ) {
	case PROJ_LASER:
		damage = 4;
		vel = g_laserVel->integer();
		AutoName( "LaserBolt", &Projectile_t::laser_id );
		//mat = materials.FindByName( "gfx/SPRITE/LASER/laser_w.tga" );
		mat = materials.FindByName( "color_red" );
		anim = new animSet_t( *pAnims );
		break;
	case PROJ_FIREBALL:
		damage = 10;
		vel = 3000;
		//AutoName( "Fireball", &Projectile_t::id );
		mat = NULL;
		anim = new animSet_t( *pAnims );
		break;
	case PROJ_TURRETBALL:
		damage = 10;
		vel = 1000;
		AutoName( "Turretball", &Projectile_t::turretball_id );
		mat = NULL;
		anim = new animSet_t( *pAnims );
		break;
	}

	poly.w = 180.0f; 
	poly.h = poly.w * 0.25f; 
	poly.angle = 0.f;

	float hh = 0.5f * poly.h;
	float hw = 0.5f * poly.w;

	// the poly size for rectangular lasers
	switch ( side ) {
	case MOVE_LEFT:
		poly.x = s[0] - poly.w;
		poly.y = s[1] - hh;
		break;
	case MOVE_RIGHT:
		poly.x = s[0] ;
		poly.y = s[1] - hh;
		break;
	case MOVE_UP:
		poly.x = s[0] - hw;
		poly.y = s[1];
		poly.angle = 90.0f;
		break;
	case MOVE_DOWN:
		poly.x = s[0] - hw;
		poly.y = s[1] - poly.h;
		poly.angle = 90.0f;
		break;
	case 3: // up+right
		poly.x = s[0];
		poly.y = s[1];
		poly.angle = -45.0f;
		break;
	case 9: // up+left
		poly.x = s[0] - poly.w;
		poly.y = s[1];
		poly.angle = 45.0f;
		break;
	case 6: // down+right
		poly.x = s[0];
		poly.y = s[1] - poly.h;
		poly.angle = 45.0f;
		break;
	case 12: // down+left
		poly.x = s[0] - poly.w;
		poly.y = s[1] - poly.h;
		poly.angle = -45.0f;
		break;
	default:
        if ( platform_type->integer() == 2 ) {
		    poly.x = s[0] - hw;
		    poly.y = s[1];
		    poly.angle = -r_angle + 90.0f;
        }
		//Assert( 0 && "better setup location or it's goin' in the void!" );
	}
	
	// these are rectangular however
	if ( type == PROJ_TURRETBALL ) {
		poly.h = poly.w;
	}

	// set everything
	propagateDimensions( -1 );

	// hook it in to the matrix
	connect( this->blockLookup() );
}

void Projectile_t::think(  void ) {
	if ( PROJ_DONE == state )
		return ; // shouldn't reach
	if ( PROJ_EXPLODING == state ) {
		if ( timer.check() ) { // done exploding
			state = PROJ_DONE;
			delete_me |= 0x3; // mark for deletion
		}
		return;
	}

	float v[2] = { dir[0], dir[1] };
	float mv_amt = (float)vel / sv_fps->value();
	v[0] *= mv_amt;
	v[1] *= mv_amt;
	wishMove( v[0], v[1] );
}

void Projectile_t::GeneralClippingHandler( Entity_t **epp ) {
	if ( PROJ_EXPLODING == state )
		return;
	if ( this->clipping && (*epp)->clipping && AABB_collide( wish, (*epp)->clip ) ) {
		this->explode();
	}
}

void Projectile_t::_handle( Entity_t **epp ){
	switch( (*epp)->entType ) {
	case ET_PLAYER:
	case ET_ROBOT: // these have higher ioType, so should be handled elsewhere
		return;
	//case ET_DOOR: return GeneralClippingHandler(epp); // also has higher IO_TYPE
		break;
	}
}

void Projectile_t::explode( void ) {
	if ( PROJ_EXPLODING == state || PROJ_DONE == state )
		return;
	// start exploding
	state = PROJ_EXPLODING;
	timer.set( explode_time );
	// fix poly, make square w/ even sides
	poly.h = poly.w;
	propagateDimensions( -1 );
	S_StartLocalSound( explosion_sound[0] );
}

void Projectile_t::handleTileCollisions( void ) { // ovverride baby
	if ( PROJ_EXPLODING == state )
		return;

	explode();
}

material_t *Projectile_t::_getMat( void ) {
	switch ( type ) {
	case PROJ_TURRETBALL:
		return anim->getMat( "turret_ball" );
	}
	if ( PROJ_EXPLODING == state ) {
		return anim->getMat( "laser_hit" );
	}
	return mat;
}

void Projectile_t::setMemPage( void ) {

	// first time
	if ( client_id == -1 ) 
	{
		client_id = V_RequestExclusiveZone( PROJECTILE_BYTES, &origMemID );
	} 
	// subsequent
	else 
	{
		int tmp;
		V_SetCurrentZone( client_id, &tmp );

		if ( tmp != origMemID ) {
			origMemID = tmp;
		}
	}
}

void Projectile_t::retMemPage( void ) {
	if ( client_id == -1 || origMemID == -1 )
		return;
	int tmp;
	V_SetCurrentZone( origMemID, &tmp );
}

/*
====================
 P_Shoot

	projectile creation goes between 2 calls to set specific memory page
	for projectiles only. 
====================
*/
void P_Shoot( int type, int side, float *dir, float *v ) {
	Projectile_t::setMemPage();
	Entity_t *shot = new Projectile_t( type, side, dir, v );
	Projectile_t::retMemPage();
}


