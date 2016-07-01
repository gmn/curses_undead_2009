// com_geometry.cpp

#include "../lib/lib.h"
#include "../mapedit/mapedit.h" // entNode
#include "../client/cl_console.h"
#include "com_geometry.h"

// list of available constructed materials
class materialSet_t materials;


// FIXME: these need work.  Also need a parms list, so that map_save knows 
//  what to scan for
materialString_t matNames[] = {
	{ MTL_NONE, "NONE!!!" },
	{ MTL_COLOR, "color" },
	{ MTL_TEXTURE_STATIC, "textureStatic" },
	{ MTL_COLORMASK, "colorMask" },
	{ MTL_MASKED_TEXTURE, "maskedTexture" },
	{ MTL_MASKED_TEXTURE_COLOR, "maskedTextureColor" },
	{ MTL_MULTITEX, "multiTexture" },
	{ MTL_OSCILLATOR, "oscillator" },
};

materialString_t matSubTypes[] = {
	{ 1, "colorMask" },
	{ 2, "alphaMask" },
	{ 3, "texture" },
};

const int TOTAL_MATERIAL_TYPES = sizeof(matNames) / sizeof(matNames[0]);


/*
=============================================================================

	tile_t

============================================================================= 
*/
void tile_t::setArbRotation( float angle ) {
	this->poly.set_rot( angle );
	if ( this->col ) {
		this->col->rotate( angle );
	}
}
void tile_t::rot90( unsigned int cw_turns ) {

	//cw_turns %= 4;
	cw_turns &= 3;

	if ( 0 == cw_turns )
		return;

	// gmn-081109: going to setArb all the time because I need to be able to rotate textures
	//  reliably to generate maps that make sense (or else a tile will flip back to its
	//  original orientation on mapLoad making the map look stupid
	return setArbRotation( poly.angle + 90.f * cw_turns );


	//////////////////////////////////////////////////////////////////////////////////
	int zero_angle = ISZERO( poly.angle );

	// check angle?.. do different , arbitrary rotation?
	//setArbRotation( poly.angle + 90.f * cw_turns );

	// poly
	if ( 3 == cw_turns ) {
		poly.rotateNinetyCC();
	} else if ( 2 == cw_turns ) {
		poly.rotateNinetyCC();
		poly.rotateNinetyCC();
	} else if ( 1 == cw_turns ) {
		poly.rotateNinety();
	}

	// material
	if ( this->mat && zero_angle ) {
		if ( 3 == cw_turns ) {
			mat->rot90ccTexCoords();
		} else if ( 2 == cw_turns ) {
			mat->rot90TexCoords();
			mat->rot90TexCoords();
		} else if ( 1 == cw_turns ) {
			mat->rot90TexCoords();
		}
	}

	// colModel
	if ( this->col ) {
		if ( this->do_sync ) {
			col->sync( &poly );
		} else {
			// has an OBB, do an arb rotation
			if ( col->OBB ) { 
				col->rotate( 90.f * cw_turns );
			// has only an AABB, use 90 degree rot methods
			} else if ( col->AABB ) {
				col->rot90( cw_turns );
			}
		}
	}
}

void tile_t::sync( void ) {
	// if there is a colModel AND it is set to auto-syncronize with tile:
	if ( col && this->do_sync ) {
		col->sync( &poly );
	}
}
void tile_t::syncNoBlock( void ) {
	if ( col ) {
		col->sync( &poly );
	}
}
void tile_t::dragColModel( float dx, float dy ) {
	if ( col ) {
		col->drag( dx, dy );
	}
}
void tile_t::_my_init() {
}
void tile_t::_my_reset() {
	layer = 0;
	do_sync = true;
	if ( anim ) { anim->reset(); }
	if ( mat  ) { mat->reset(); }
	if ( col  ) { col->reset(); }
}
void tile_t::_my_destroy() {
	if ( anim ) { delete anim; anim = NULL; }
	if ( mat  ) { delete mat; mat = NULL; }
	if ( col  ) { delete col; col = NULL; }
}
tile_t::tile_t( tile_t const & T ) : anim(0), mat(0), col(0) {
	if ( T.anim ) { 
	//	anim = T.anim->Copy(); 
		anim = new animSet_t( *T.anim );
	}
	if ( T.mat  ) { 
	//	mat = T.mat->Copy(); 
		mat = new material_t( *T.mat );
	}
	if ( T.col  ) { 
	//	col = T.col->Copy(); 
		col = new colModel_t( *T.col );
	}
	poly = T.poly;
	layer = T.layer;
}


int M_MousePolyComparison( float *, float * );


/*
==============================================================================

	colModel_t

==============================================================================
*/
bool colModel_t::collide( colModel_t &offender ) {
	return boxIntersect( offender.box );
}

bool colModel_t::checkPoint( float p[2] ) {
	// 
	if ( p[0] < box[0] )
		return false;
	if ( p[0] > box[2] )
		return false;
	if ( p[1] < box[1] )
		return false;
	if ( p[1] > box[3] )
		return false;

	if ( CM_AABB ) {
		AABB_node *A = AABB;
		while ( A ) {
			if ( p[0] >= A->AABB[0] && p[0] <= A->AABB[2] &&
				p[1] >= A->AABB[1] && p[1] <= A->AABB[3] ) {
				return true;
			}
			A = A->next;
		}
	} else if ( CM_OBB ) {
		OBB_node *O = OBB;
		while ( O ) {
			if ( M_MousePolyComparison( p, O->OBB ) )
				return true;
			O = O->next;
		}
	}
	return false;
}

/*
====================
 colModel_t::toggle
====================
*/
bool colModel_t::toggle( poly_t *p ) {
	// is off, create col model
	if ( type == COLMOD_NONE ) {
		if ( ISZERO( p->angle ) ) {
			type = CM_AABB;
			AABB = AABB_node::New();
			p->toAABB( AABB->AABB );
			COPY4( box, AABB->AABB );
		} else {
			type = CM_OBB;
			OBB = OBB_node::New();
			p->toOBB( OBB->OBB );
			OBB_TO_AABB_ENCLOSURE( box, OBB->OBB );
		}
		count = 1;
		return true;
	// had one, turn off
	} else if ( type == CM_AABB || type == CM_OBB ) {
		this->Destroy();
	}
	return false;
}

/*
====================
 colModel_t::Copy()
====================
*/
colModel_t * colModel_t::Copy( void ) const {
	return new colModel_t( *this );
}

#if 0
colModel_t * colModel_t::Copy( void ) const {
	colModel_t *col = new colModel_t;
	col->count = this->count;
	//col->info = info->Copy();
	col->type = this->type;
	if ( type == CM_AABB ) {
		AABB_node *a = AABB, *tail;
		while ( a ) {
			AABB_node *b = (AABB_node*) V_Malloc(sizeof(AABB_node) );
			b->next = NULL;
			COPY4( b->AABB, a->AABB );

			if ( col->AABB ) {
				tail->next = b;
			} else {
				col->AABB = b;
			}
			tail = b;

			a = a->next;
		}
	} else if ( type == CM_OBB ) {
		OBB_node *o = OBB, *tail;
		while ( o ) {
			OBB_node *p = (OBB_node*) V_Malloc( sizeof(OBB_node) );
			p->next = NULL;
			COPY8( p->OBB, o->OBB );

			if ( col->OBB ) {
				tail->next = p;
			} else {
				col->OBB = p;
			}
			tail = p;

			o = o->next;
		}
	}
	return col;
}
#endif


/*
====================
 colModel_t::New
====================
*/
colModel_t * colModel_t::New( void ) {
	return new colModel_t();
}

/*
====================
 colModel_t::Destroy
====================
*/
void colModel_t::Destroy( void ) {
	if ( type == COLMOD_NONE )
		return;

	if ( type == CM_AABB ) {
		AABB_node *n = AABB, *tmp;
		for ( ; n ; n = tmp ) {
			tmp = n->next;
			V_Free( n );
		}
		AABB = NULL;
	} else if ( type == CM_OBB ) {
		OBB_node *m = OBB, *tmp_o;
		for ( ; m ; m = tmp_o ) {
			tmp_o = m->next;
			V_Free( m );
		}
		OBB = NULL;
	}
	count = 0;
	type = COLMOD_NONE;
	AABB_zero( box );
}

/*
====================
 colModel_t::rotate

	currently not using. it uses sync() instead
====================
*/
void colModel_t::rotate( float angle ) {
	// calling this function should automatically convert all AABB_node
	//  members to OBB_nodes

	OBB_node *obb = this->OBB;
	float center[2];

	//
	// FIXME: how do I get center?
	//
	return;

	while ( obb ) {
		M_TranslateRotate2DVertSet( obb->OBB, center, angle );
		obb = obb->next;
	}
}

/*
====================
 colModel_t::rot90

	FIXME: broken. currently only works for 1 AABB
====================
*/
void colModel_t::rot90( unsigned int cw_turns ) { 
	cw_turns &= 3;
	if ( !cw_turns )
		return;

	float angle = cw_turns * 90.f;
	float v[8];
	float c[2];

	if ( 1 == count && AABB ) {
		AABB_TO_OBB( v, AABB->AABB );
		AABB_getCenter( c, AABB->AABB );
		M_TranslateRotate2DVertSet( v, c, angle );
		while ( cw_turns-- != 0 )
			SWAP_VERT8_90( v );
		OBB_TO_AABB( AABB->AABB, v );
		COPY4( box, AABB->AABB );
		return;
	}

	//
	// FIXME: not finished yet
	//
	return; 

	

	AABB_node *aabb = this->AABB;

	while ( aabb ) {
		AABB_TO_OBB( v, aabb->AABB );

		float tx = v[0];
		float ty = v[1];
		v[0] = v[2];
		v[1] = v[3];
		v[2] = v[4];
		v[3] = v[5];
		v[4] = v[6];
		v[5] = v[7];
		v[6] = tx;
		v[7] = ty;

		OBB_TO_AABB( aabb->AABB, v );
		aabb = aabb->next;
	}
	OBB_node *obb = this->OBB;
	while ( obb ) {
		COPY8( v, obb->OBB );		
		float tx = v[0];
		float ty = v[1];
		v[0] = v[2];
		v[1] = v[3];
		v[2] = v[4];
		v[3] = v[5];
		v[4] = v[6];
		v[5] = v[7];
		v[6] = tx;
		v[7] = ty;
		COPY8( obb->OBB, v );
		
		obb = obb->next;
	}
}

/*
====================
 colModel_t::sync

 syncs a colModel geometry to a single box duplicating poly argument
====================
*/
void colModel_t::sync( poly_t *p ) {

	if ( type == COLMOD_NONE )
		return;

	// replace whatever's here
//	this->Destroy();

	// create an OBB, unset AABB
	if ( !ISZERO( p->angle ) ) {
		float v[8];
		POLY_TO_VERT8( v, p );
		float c[2];
		p->getCenter( c );
		M_TranslateRotate2DVertSet( v, c, p->angle );


		if ( OBB && OBB->OBB ) {
			COPY8( OBB->OBB, v );
		} else {
			pushOBB( v );
		}
		OBB_TO_AABB_ENCLOSURE( box, v );
		return;
	}

	AABB_t aabb;
	POLY_TO_AABB( aabb, p );
	if ( AABB && AABB->AABB ) {
		COPY4( AABB->AABB, aabb );
	} else {
		pushAABB( aabb );
	}
	COPY4( box, aabb );
}

void colModel_t::drag( float dx, float dy ) {
	if ( type == CM_AABB ) {
		AABB_node *A = AABB;
		while ( A ) {
			A->AABB[0] += dx;
			A->AABB[1] += dy;
			A->AABB[2] += dx;
			A->AABB[3] += dy;
			A = A->next;
		}
	} else if ( type == CM_OBB ) {
		OBB_node *O = OBB;
		while( O ) {
			O->OBB[0] += dx; O->OBB[1] += dy;
			O->OBB[2] += dx; O->OBB[3] += dy;
			O->OBB[4] += dx; O->OBB[5] += dy;
			O->OBB[6] += dx; O->OBB[7] += dy;
			O = O->next;
		}
	}
	
	box[0] += dx;
	box[1] += dy;
	box[2] += dx;
	box[3] += dy;
}

void colModel_t::pushAABB( AABB_t aabb ) {
	AABB_node *node = AABB_node::New( aabb );
	//AABB_node *node = new AABB_node( aabb );
	if ( AABB ) {
		AABB_node *n = AABB;
		while ( n->next ) {
			n = n->next;
		}
		n->next = node;
	} else {
		AABB = node;
	}
	type = CM_AABB;
	++count;
}

void colModel_t::pushOBB( OBB_t obb ) {
	OBB_node *node = OBB_node::New( obb );
	//OBB_node *node = new OBB_node( obb );
	if ( OBB ) {
		OBB_node *n = OBB;
		while ( n->next ) {
			n = n->next;
		}
		n->next = node;
	} else {
		OBB = node;
	}
	type = CM_OBB;
	++count;
}

void colModel_t::_my_reset() {
	type = COLMOD_NONE;
	count = 0;
	AABB_zero( box );
}

void colModel_t::_my_destroy() {
	AABB_node *tmp_a, *p;
	if ( !V_isStarted() )
		return;
	for ( p = AABB; p ; p = tmp_a ) {
		tmp_a = p->next;
		delete p;
	}
	AABB = NULL;
	OBB_node *tmp, *q;
	for ( q = OBB; q ; q = tmp ) {
		tmp = q->next;
		delete q;
	}
	OBB = NULL;
}

colModel_t::colModel_t( colModel_t const & C ) : AABB(0), OBB(0) {
	type = C.type;

	AABB_node *a = C.AABB;
	while ( a ) {
		pushAABB( a->AABB );
		a = a->next;
	}

	OBB_node *p = C.OBB;
	while ( p ) {
		pushOBB( p->OBB ) ;
		p = p->next;
	}

	count = C.count;
    // info = copy info
	COPY4( box, C.box );
}

void colModel_t::set( colModel_t const & cm ) {
	type = cm.type;
	AABB_node *a = cm.AABB;
	if ( type == CM_AABB ) {
		AABB_node *ours = AABB;
		while ( a ) {
			if ( ours && ours->AABB ) {
				COPY4( ours->AABB, a->AABB );
				ours = ours->next;
			} else 
				pushAABB( a->AABB );
			a = a->next;
		}
	} else {
		OBB_node *p = cm.OBB;
		OBB_node *ours = OBB;
		while ( p ) {
			if ( ours && ours->OBB ) {
				COPY8( ours->OBB, p->OBB );
				ours = ours->next;
			} else
				pushOBB( p->OBB ) ;
			p = p->next;
		}
	}
	count = cm.count;
	COPY4( box, cm.box );
}

/*
====================
 colModel_t::isInABB
====================
*/
// input is aabb, if any colModels are in aabb, returns true
bool colModel_t::isInAABB( float *B ) {
	AABB_node *a = AABB;
	while( a ) {
		if ( AABB_intersection( B, a->AABB ) ) 
			return true;
		a = a->next;
	}
	OBB_node *o = OBB;
	float v[8];
	if ( o ) {
		AABB_TO_OBB( v, B );
	}
	while ( o ) {
		if ( OBB_intersection( v, o->OBB ) )
			return true;
		o = o->next;
	}
	return false;
}

/*
====================
 colModel_t::collideAABB

	test for collision of AABB with this colModel_t
====================
*/
bool colModel_t::collideAABB( AABB_t perp ) {
	// first test that perp intersects with bounding box that is around 
	//  col model collection
	if ( AABB_intersection( box, perp ) ) {
		if ( type == CM_AABB ) {
			AABB_node *A = AABB;
			while ( A ) {
				if ( AABB_intersection( A->AABB, perp ) ) 
					return true;
				A = A->next;
			}
		} else if ( type == CM_OBB ) {
			OBB_node *o = OBB;
			float v[8];
			AABB_TO_OBB( v, perp );
			while ( o ) {
				if ( OBB_intersection( v, o->OBB ) )
					return true;
				o = o->next;
			}
		}
	}
	return false;
}

/*
====================
 colModel_t::collideOBB

	test for collision of AABB with this colModel_t
====================
*/
bool colModel_t::collideOBB( OBB_t perp ) {
	OBB_t obb;
	AABB_TO_OBB( obb, box );
	if ( OBB_intersection( obb, perp ) ) {
		if ( type == CM_AABB ) {
			AABB_node *A = AABB;
			while ( A ) {
				AABB_TO_OBB( obb, A->AABB );
				if ( OBB_intersection( obb, perp ) ) 
					return true;
				A = A->next;
			}
		} else if ( type == CM_OBB ) {
			OBB_node *o = OBB;
			while ( o ) {
				if ( OBB_intersection( perp, o->OBB ) )
					return true;
				o = o->next;
			}
		}
	}
	return false;
}

/*
====================
 colModel_t::boxIntersect
	
	just intersect AABB perp with the bound box
====================
*/
bool colModel_t::boxIntersect( AABB_t perp ) {
	return AABB_intersection( box, perp );
}

/*
====================
 constructor, takes a poly as arg
====================
*/
colModel_t::colModel_t( poly_t & p ) : AABB(0), OBB(0) {
	count = 1;
	if ( ISZERO( p.angle ) ) {
		type = CM_AABB;
		p.toAABB( box );
		pushAABB( box );
	} else {
		type = CM_OBB;
		float v[8];
		p.toOBB( v );
		pushOBB( v );
		OBB_TO_AABB_ENCLOSURE( box, OBB->OBB );
	}
}

// - takes lower case only
// - adjust box only
int colModel_t::adjust( int corner, float amt ) { 
	switch ( corner ) {
	case 'x': box[0] += amt; break;
	case 'y': box[1] += amt; break;
	case 'w': box[2] += amt; break;
	case 'h': box[3] += amt; break;
	default:
		return 0;
	}
	return corner;
}

/*
==============================================================================

	poly_t

==============================================================================
*/
void poly_t::toAABB( AABB_t v ) {
	POLY_TO_AABB( v, this );
}

void poly_t::toOBB( OBB_t v ) {
	float c[2];
	getCenter( c );
	POLY_TO_VERT8( v, this );
	M_TranslateRotate2DVertSet( v, c, angle ); 
}

void poly_t::toOBBSorted( OBB_t v ) {
	toOBB( v ) ;

	// sort by x
	for ( int j = 0; j < 8; j+= 2 ) {	
		for ( int i = 0; i < 8; i+= 2 ) {	
			if ( v[i] > v[j] ) {
				float tmp = v[i];
				v[i] = v[j];
				v[j] = tmp;
				tmp = v[i+1];
				v[i+1] = v[j+1];
				v[j+1] = tmp;
			}
		}
	}

	// fix y ordering
	if ( v[1] > v[3] ) {
		float tx = v[0];
		float ty = v[1];
		v[0] = v[2];
		v[1] = v[3];
		v[2] = tx;
		v[3] = ty;
	}
	if ( v[5] < v[7] ) {
		float tx = v[4];
		float ty = v[5];
		v[4] = v[6];
		v[5] = v[7];
		v[6] = tx;
		v[7] = ty;
	}
}

void poly_t::rotateNinety( void ) {
	if ( angle == 0.f ) {
		float v[8];

		angle = 90.0f;
		toOBB( v );
		angle = 0.0f;

		float tx = v[0];
		float ty = v[1];
		v[0] = v[2];
		v[1] = v[3];
		v[2] = v[4];
		v[3] = v[5];
		v[4] = v[6];
		v[5] = v[7];
		v[6] = tx;
		v[7] = ty;
		VERT8_TO_POLY( this, v );
	} else {
		angle += 90.0f;
	}
}

void poly_t::rotateNinetyCC( void ) {
	if ( angle == 0.f ) {
		float v[8];

		angle = 270.0f;
		toOBB ( v );
		angle = 0.f;

		float tx = v[0];
		float ty = v[1];
		v[0] = v[6];
		v[1] = v[7];
		v[6] = v[4];
		v[7] = v[5];
		v[4] = v[2];
		v[5] = v[3];
		v[2] = tx;
		v[3] = ty;
		VERT8_TO_POLY( this, v );
	} else {
		angle += -90.0f;
	}
}
 
/* 
==============================================================================

	material_t

==============================================================================
*/ 
/*
================
================
*/

material_t * material_t::NewTextureMaterial( const char *_name, image_t *_img ) {
	material_t *M = New();
	
	// actually important
    char buf[500];
    strcpy( buf, strip_path( _name ) );
	strcpy( M->name, strip_extension( buf ) );
    g_tolower( M->name );
	M->type = MTL_TEXTURE_STATIC;
	M->img = _img;
	M->s[0] = 0.0f;
	M->s[1] = 1.0f;
	M->s[2] = 1.0f;
	M->s[3] = 0.0f;
	M->t[0] = 0.0f;
	M->t[1] = 0.0f;
	M->t[2] = 1.0f;
	M->t[3] = 1.0f;

	M->sub = NULL;
	M->color[0] = M->color[1] = M->color[2] = M->color[3] = 0.0f;
	M->gl_attrib = 0;

	return M;
}

material_t * material_t::New( void ) {
	material_t *n = new material_t;
	return n;
}
material_t * material_t::Copy( void ) const {
	material_t *n = material_t::New();
	COPY4( n->color, color );
	n->gl_attrib = gl_attrib;
	n->gl_info = gl_info;
	n->h = h;
	n->img = img;
	n->mask = mask;
	strcpy ( n->name, name );
	COPY4( n->s, s );
	n->sub = sub;
	COPY4( n->t, t );
	n->type = type;
	n->w = w;
	return n;
}
void material_t::Destroy( void ) {
}
 

void material_t::_my_reset( void ) {
	name[0] = 0;
	h = w = 0;
	color[0] = color[1] = color[2] = color[3] = 1.f;
	gl_attrib = 0;
	type = MTL_TEXTURE_STATIC;
	s[0] = 0.f; t[0] = 0.f;
	s[1] = 1.f; t[1] = 0.f;
	s[2] = 1.f; t[2] = 1.f;
	s[3] = 0.f; t[3] = 1.f;
}

material_t::material_t( material_t const & M ) : img(0), sub(0), mask(0) {
	name[ 0 ] = name[ name_sz - 1 ] = 0;
	strncpy ( name, M.name, name_sz );
	h = M.h; 
	w = M.w;
	COPY4( color, M.color );
	gl_attrib = M.gl_attrib;
	type = M.type;

	img = M.img;
	mask = M.mask;
	sub = M.sub;

	COPY4( s, M.s );
	COPY4( t, M.t );
	gl_info = M.gl_info;
}



/*
==============================================================================

	mapTile_t

==============================================================================
*/

int mapTile_t::nextMapTileUID = 0;

mapTile_t * mapTile_t::New( colModel_t *cpycol ) {
/* ohhh, this no longer works, because it is inheriting from a virtual class, where reset() is a
	virtual method, which means that the vtable must be built first by calling the default constructor
	which is not obviously called because of this raw allocation here.

	mapTile_t *n = (mapTile_t *) V_Malloc( sizeof(mapTile_t) );
	n->reset();
*/
	mapTile_t *n = new mapTile_t();

	if ( cpycol ) {
		n->col = cpycol->Copy();
	} else {
		//n->col = colModel_t::New();
		n->col = NULL;
	}

	n->uid = ++nextMapTileUID;
	return n;
}
mapTile_t * mapTile_t::NewMaterialTile( void ) {
	mapTile_t *n = mapTile_t::New();
	n->mat = material_t::New();
	return n;
}

/*
================
mapTile_t::Copy

 copy constructor
================
*/
mapTile_t * mapTile_t::Copy( void ) {

	mapTile_t *n = mapTile_t::New( this->col );

	if ( anim ) {
		n->anim = new animSet_t( *this->anim );
	}
	if ( mat ) {
		n->mat = new material_t( *this->mat );
		//n->mat = mat->Copy();
	}

	n->uid = this->uid;
	n->snap = this->snap;
	n->lock = false; // copied tiles do not inherit lock
	n->do_sync = this->do_sync;
	n->poly = this->poly;
	n->layer = this->layer;
	return n;
}

void mapTile_t::Destroy( void ) {
	if ( anim ) {
		/*
		anim->destroy();
		V_Free( anim );
		*/
		delete anim;
		anim = NULL;
	}
	if ( mat ) {
		mat->Destroy();
		V_Free( mat );
		mat = NULL;
	}
	if ( col ) {
		col->Destroy();
		V_Free( col );
		col = NULL;
	}
}

void mapTile_t::_my_init( void ) {
	tile_t::init();
}
void mapTile_t::_my_reset( void ) {
	uid = -1;
	snap = true;
	lock = false;
	background = 0;
//	tile_t::reset();
}
void mapTile_t::_my_destroy( void ) {
	tile_t::_my_destroy();
}
mapTile_t::mapTile_t( mapTile_t const& M ) 
	: uid(M.uid), snap(M.snap), lock(M.lock), background(M.background), tile_t(M) {
}




/*
==============================================================================

	materialSet_t

==============================================================================
*/

/*
====================
 materialSet_t::FindByName
====================
*/
material_t * materialSet_t::FindByName( const char * name ) {
	for ( int i = 0; i < total; i++ ) {
        // first try the material name
        if ( !strcmp( name, mat.data[i]->name ) )
            return mat.data[i];

        // if there's an image try the image syspath
        if ( !mat.data[i]->img ) { 
			continue;
        }
		if ( !O_pathicmp( name, mat.data[i]->img->syspath ) ) {
			return mat.data[i];
		}
	}
	return NULL;
}

material_t * materialSet_t::PrevMaterial( void ) {
    if ( 0 == total )
        return NULL;
    current = ( current == 0 ) ? total - 1 : current - 1;
	return mat.data[ current ];
}

material_t * materialSet_t::NextMaterial( void ) {
    if ( 0 == total )
        return NULL; // can not modulus by 0
	current = ( current + 1 ) % total;
	return mat.data[ current ];
}

void materialSet_t::LoadTextureDirectory( void ) {

	if ( !isInit() ) {
		init();
	}

	image_t **images = NULL;
	unsigned int total_tex = 0;

    unsigned int asset_len = 0;

    char ** asset_names = GetDirectoryList( "zpak/gfx", &asset_len );

	if ( !asset_len || !asset_names ) { 
		Err_Fatal( "couldn't find data directory! Cannot run. Exiting.\n" );
        return;
	}

    // count how many files listed are actually loadable textures
    for ( unsigned int i = 0; i < asset_len; i++ ) {
        if ( O_strcasestr( asset_names[i], ".tga" ) || O_strcasestr( asset_names[i], ".bmp" ) )
            ++total_tex;
    }

    images = (image_t **) V_Malloc( sizeof(image_t *) * total_tex );    

    // setup materials
//	materials.init();

    // FIXME: code that verifies supported image extensions should go in 
    //  lib_image.cpp
    int j = 0;
    for ( unsigned int i = 0; i < asset_len; i++ ) {
        if ( O_strcasestr( asset_names[i], ".tga" ) || O_strcasestr( asset_names[i], ".bmp" ) ) {
            IMG_compileImageFile( asset_names[i], &images[j] );

            // build a texture material
			material_t *pointer = material_t::NewTextureMaterial( asset_names[i], images[j] );
			this->add( pointer );

            ++j;
        }
    }

    // free array of string pointers
    for ( unsigned int i = 0; i < asset_len; i++ ) {
        V_Free( asset_names[i] );
    }
    V_Free( asset_names );
}

void materialSet_t::ReLoadTextures( void ) {
	unsigned int total_tex = 0;
    unsigned int asset_len = 0;

    char ** asset_names = GetDirectoryList( "zpak/gfx", &asset_len );

	if ( !asset_len || !asset_names ) { 
		Err_Fatal( "couldn't find data directory! Cannot run. Exiting.\n" );
        return;
	}

    // count how many files listed are actually loadable textures
    for ( unsigned int i = 0; i < asset_len; i++ ) {
        if ( O_strcasestr( asset_names[i], ".tga" ) || O_strcasestr( asset_names[i], ".bmp" ) )
            ++total_tex;
    }

	image_t *image = NULL;
	material_t *existing = NULL;
	
	// foreach asset name
    for ( unsigned int i = 0; i < asset_len; i++ ) {

		// if it appears to be an image file
        if ( O_strcasestr( asset_names[i], ".tga" ) || O_strcasestr( asset_names[i], ".bmp" ) ) {

			// compile a new image from the asset name
            IMG_compileImageFile( asset_names[i], &image );

			// see if we already have it
			if ( (existing=FindByName( strip_gamepath( asset_names[i] ) ) ) ) {
				// in that case replace the data in the image_t pointer in 
				//  the existing material with the new data, and free the 
				//  newly created, redundant image_t
				if ( !existing->img ) {
					existing->img = image;
				} else {
					memcpy( existing->img, image, sizeof(image_t) );
					V_Free( image );
				}
			} else {

				// otherwise: build a new texture material
				material_t *mat_p = material_t::NewTextureMaterial( asset_names[i], image );
				// and add it to the set
				this->add( mat_p );

			}
        }
    }

    // free array of string pointers
    for ( unsigned int i = 0; i < asset_len; i++ ) {
        V_Free( asset_names[i] );
    }
    V_Free( asset_names );
}


/*
==============================================================================

	AABB_t

==============================================================================
*/

/*
====================
 AABB_intersection
====================
*/
bool AABB_intersection( AABB_t a, AABB_t b ) {
	if ( a[0] > b[2] ) // min > max 
		return false;
	if ( a[1] > b[3] )
		return false;
	if ( b[0] > a[2] )
		return false;
	if ( b[1] > a[3] )
		return false;
	return true;
}


/*
==============================================================================

	OBB_t

==============================================================================
*/
/*
====================
 OBB_intersection
====================
*/
bool OBB_intersection( OBB_t obb1, OBB_t obb2 ) {
	// 
	float c1[2], c2[2];
	OBB_getCenter( obb1, c1 );
	OBB_getCenter( obb2, c2 );
	//
	float mp1[8], mp2[8];
	OBB_getMidPoints( obb1, mp1 );
	OBB_getMidPoints( obb2, mp2 );
	// 
	float t[2] = { c2[0] - c1[0], c2[1] - c1[1] };
	float tu[2] = { c2[0] - c1[0], c2[1] - c1[1] };
	float td = M_Normalize2d( tu );

	// must get 2mp vec per OBB that are <=90 from t & -t
	//

	// get all midpoint vectors per figure
	float A[8], B[8];
	for ( int i = 0; i < 8; i++ ) {
		A[i] = mp1[i] - c1[i&1];
		B[i] = mp2[i] - c2[i&1];
	}

	float Amag[4], Bmag[4];
	int i = 0;	
	Amag[0] = M_Normalize2d( &A[0] );
	Amag[1] = M_Normalize2d( &A[2] );
	Amag[2] = M_Normalize2d( &A[4] );
	Amag[3] = M_Normalize2d( &A[6] );
	Bmag[0] = M_Normalize2d( &B[0] );
	Bmag[1] = M_Normalize2d( &B[2] );
	Bmag[2] = M_Normalize2d( &B[4] );
	Bmag[3] = M_Normalize2d( &B[6] );

	float A_theta[4], B_theta[4];
	A_theta[0] = M_DotProduct2d( &A[0], tu );
	A_theta[1] = M_DotProduct2d( &A[2], tu );
	A_theta[2] = M_DotProduct2d( &A[4], tu );
	A_theta[3] = M_DotProduct2d( &A[6], tu );
	B_theta[0] = M_DotProduct2d( &B[0], tu );
	B_theta[1] = M_DotProduct2d( &B[2], tu );
	B_theta[2] = M_DotProduct2d( &B[4], tu );
	B_theta[3] = M_DotProduct2d( &B[6], tu );

	// 2 angles A >= 0 & 2 angles B <= 0
	int a1 = 0, a2 = 0; 
	int b1 = 0, b2 = 0;

	int r[4] = { 0,1,2,3 }; 
	int s[4] = { 0,1,2,3 };
	for ( int j = 0; j < 4; j++ ) {
		for ( int i = 0; i < 4; i++ ) {
			if ( A_theta[i] <= A_theta[j] ) {
				float tmp = A_theta[i];
				A_theta[i] = A_theta[j];
				A_theta[j] = tmp;
				int gay = r[i];
				r[i] = r[j];
				r[j] = gay;
			}
			if ( B_theta[i] >= B_theta[j] ) {
				float tmp = B_theta[i];
				B_theta[i] = B_theta[j];
				B_theta[j] = tmp;
				int itmp = s[i];
				s[i] = s[j];
				s[j] = itmp;
			}
		}
	}
	a1 = r[0]; a2 = r[1]; b1 = s[0]; b2 = s[1];

	// say t & l are same thing, fuckit
	float l[2] = { tu[0], tu[1] };
	float nl[2] = { -tu[0], -tu[1] };

	float D = td;

	float dA = 	Amag[a1] * fabsf( M_DotProduct2d( &A[a1<<1], l ) ) + 
				Amag[a2] * fabsf( M_DotProduct2d( &A[a2<<1], l ) );
	float dB =	Bmag[b1] * fabsf( M_DotProduct2d( &B[b1<<1], nl ) ) +	
				Bmag[b2] * fabsf( M_DotProduct2d( &B[b2<<1], nl ) );

	return D <= dA + dB; // true == intersection
}


/*
==============================================================================

 animation_t

==============================================================================
*/
unsigned int animation_t::anim_id = 0;

material_t ** animation_t::copy_frames( void ) {
	material_t ** out = NULL;
	if ( total > 0 ) {
		out = (material_t**) V_Malloc ( sizeof(material_t*) * total );
		memcpy( out, frames, total * sizeof(material_t*) );
	}
	return out;
}

animation_t::animation_t( const animation_t &A ) : frames(0) {
	strncpy( name, A.name, NAME_SIZE - 1 );
	name[ NAME_SIZE - 1 ] = '\0';
	if ( A.total > 0 ) {
		frames = (material_t**) V_Malloc ( sizeof(material_t*) * A.total );
		for ( int i = 0; i < A.total; i++ ) {
			frames[i] = A.frames[i];
		}
	}
	total = A.total;
	mspf = A.mspf;
	fps = A.fps;
	direction = A.direction;
	index = 0;
}

/*
====================
 animation_t::destroy
====================
*/
void animation_t::destroy( void ) {
	if ( frames )
		V_Free( frames );
	frames = NULL;
}

/*
====================
 animation_t::init
====================
*/
void animation_t::init( unsigned int _frames, const char * _name, byte _dir ) {
	if ( _name ) {
		strncpy( name, _name, NAME_SIZE - 1 );
		name[ NAME_SIZE - 1 ] = '\0';
	} else {
		sprintf( name, "animation_%04u", ++anim_id );
	}
	frames = NULL;
	total = _frames;
	if ( total > 0 ) {
		frames = (material_t**) V_Malloc ( sizeof(material_t*) * total );
	}
	fps = 10.0f;
	mspf = 100.f; // fps = 1000 / mspf 	..  mspf = 1000 / fps
	direction = _dir;
	last = now();
	index = 0;
}

/*
====================
 animation_t::advance
====================
*/
material_t * animation_t::advance( int frame ) {
	// requesting to start at a specific number
	if ( -1 != frame ) {
		index = frame;
		last = now();
	}

	int time = now();
	if ( time - last > mspf ) {
		if ( ANIM_BACK_AND_FORTH & direction ) {
			// down
			if ( direction & 0x80 ) {
				if ( --index <= 0 ) {
					direction ^= 0x80;
					index = 0;
				}
			} else {
				if ( ++index > total - 2 ) {
					index = total - 1;
					direction ^= 0x80;
				}
			}
		} else if ( ANIM_LOOPING & direction ) {
			index = ++index % total;
		}
		last = time;
	}

	return frames[ index ];
}

/*
====================
 animSet_t::getMat
====================
*/
material_t * animSet_t::getMat( const char *setname, int frame ) {
	lastAccess = -1;
	if ( !setname || !setname[0] )
		return NULL;
	int i;
	for ( i = 0; i < length(); i++ ) {
		if ( !strcmp( setname, data[ i ].name ) ) {
			break;
		}
	}
	if ( length() == i )
		return NULL;
	
	lastAccess = i;
	return data[i].advance( frame );
}

/*
====================
 animSet_t::getAnim

    does not advance, just gets it
====================
*/
animation_t * animSet_t::getAnim( const char * aname ) {
	lastAccess = -1;
	if ( !aname || !aname[0] )
		return NULL;
	int i;
	for ( i = 0; i < length(); i++ ) {
		if ( !strcmp( aname, data[ i ].name ) ) {
			break;
		}
	}
	if ( length() == i )
		return NULL;
	
	lastAccess = i;
	return &data[i];
}

/*
====================
 animSet_t::animSet_t
====================
*/
animSet_t::animSet_t( animSet_t const & A ) : started(0) {
	const unsigned int len = A.length();
	if ( len > 0 ) {

		// only allocing for the used slots
		init( len ); 
		
		for ( int i = 0; i < len; i++ ) 
		{
			// copy all local data members
			buffer_c<animation_t>::add( A.data[ i ] ); 

			/* this one is a little confusing. buffer_c<animation_t> stores animations
				by value, not pointer, so when we call add()^, some mystery mechanism that I
				have read about, but never seen before, calls the ctor() and makes a temporary
				copy of A.data[i], including the malloc and copying of its frames array.
				(it would be nice if didn't do this. do we have any control?)
				then after it has memory copied all of those elements over to the buffer<> array
				inside the add() func
				it calls the dtor which frees frames. so what you see below is data[i].frames
				with an invalid address that just got freed, but in a copy, so it hasn't been
				set to NULL.  THEN, we get another copy of frames via a roll-your-own function
				because C++ is a stupid mess */

			// but we must get our own copy of the frame pointers array
			data[i].frames = A.data[i].copy_frames();
			data[i].last = 0; // reset the last frame drawn , after copy
		}
		
		started = true;
	}
}


/*
====================
 CM_Nudge_Command
====================
*/
// called by the mapEditor to adjust selected colModels
int CM_Nudge_Command( const char *str, float amt ) {
	if ( !str )
		return 0 ;
	char corner = tolower( *str );
	switch ( corner ) {
	case 'x': case 'y': case 'w': case 'h': 
		break;
	default:
		console.Printf( "unknown parameter" );
		return 0;
	}

	node_c<entNode_t*> *node = entSelected.gethead();
	int did = 0;
	while ( node ) {	
		if ( !node->val->val->col ) {
			node = node->next;
			continue;
		}
		switch( corner ) {
		case 'x': node->val->val->col->adjust( 'x', amt ); ++did; break;
		case 'y': node->val->val->col->adjust( 'y', amt ); ++did; break;
		case 'w': node->val->val->col->adjust( 'w', amt ); ++did; break;
		case 'h': node->val->val->col->adjust( 'h', amt ); ++did; break;
		default:
			break;
		}
		node = node->next;
	}
	if ( did ) {
		console.Printf( "nudged %d entities in %c", did, corner );
		return did;
	}

	node_c<mapNode_t*> *mnode = selected.gethead();
	while ( mnode ) {
		if ( !mnode->val->val->col ) {
			mnode = mnode->next;
			continue;
		}
		switch( corner ) {
		case 'x': mnode->val->val->col->adjust( 'x', amt ); ++did; break;
		case 'y': mnode->val->val->col->adjust( 'y', amt ); ++did; break;
		case 'w': mnode->val->val->col->adjust( 'w', amt ); ++did; break;
		case 'h': mnode->val->val->col->adjust( 'h', amt ); ++did; break;
		default:
			break;
		}
		mnode = mnode->next;
	}
	if ( did ) {
		console.Printf( "nudged %d mapTiles in %c", did, corner );
	}
	return did;
}

/*
====================
 M_GetMtl
   utility func to get a material by its name only, quick and simple
   doesn't matter which directory it's in, as long as it is in the path
====================
*/
material_t * M_GetMtl( const char * name ) {
    char buf[1024];
    strcpy( buf, name );
    g_tolower( buf );
	for ( int i = 0; i < materials.total; i++ ) {
        if ( strstr( buf, materials.mat.data[i]->name ) ) {
            return materials.mat.data[i];
        }
    }
    return NULL;
}
