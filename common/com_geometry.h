#ifndef __COM_GEOMETRY_H__
#define __COM_GEOMETRY_H__

//#include "../lib/lib_list_c.h"
#include "../lib/lib_buffer.h"
#include "../lib/lib_image.h"  // material_t, image_t, animation_t
#include "../lib/lib.h" // M_TranslateRotate, image_c, memset, ..
#include "../lib/lib_float32.h"
//#include "../game/g_animation.h" wishful thinking here 

//============================================================================
//============================================================================
//============================================================================
// helper macros, not to be used by outside code.  Why? Because they're wrong, that's why.
#define POLY_TO_VERT8( v, p ) {		\
	(v)[0] = (p)->x;				\
	(v)[1] = (p)->y;				\
	(v)[2] = (p)->x + (p)->w;		\
	(v)[3] = (p)->y;				\
	(v)[4] = (p)->x + (p)->w;		\
	(v)[5] = (p)->y + (p)->h;		\
	(v)[6] = (p)->x;				\
	(v)[7] = (p)->y + (p)->h;		}

#define VERT8_TO_POLY( p, v ) {		\
	(p)->x = (v)[0];				\
	(p)->y = (v)[1];				\
	(p)->w = (v)[2] - (v)[0];		\
	(p)->h = (v)[5] - (v)[3]; }

#define COPY4( v, p ) {				\
	(v)[0] = (p)[0];				\
	(v)[1] = (p)[1];				\
	(v)[2] = (p)[2];				\
	(v)[3] = (p)[3];				}

#define COPY8( v, p ) {				\
	(v)[0] = (p)[0];				\
	(v)[1] = (p)[1];				\
	(v)[2] = (p)[2];				\
	(v)[3] = (p)[3];				\
	(v)[4] = (p)[4];				\
	(v)[5] = (p)[5];				\
	(v)[6] = (p)[6];				\
	(v)[7] = (p)[7];				}

#define AABB_TO_OBB( v, a ) {		\
	(v)[0] = (a)[0];				\
	(v)[1] = (a)[1];				\
	(v)[2] = (a)[2];				\
	(v)[3] = (a)[1];				\
	(v)[4] = (a)[2];				\
	(v)[5] = (a)[3];				\
	(v)[6] = (a)[0];				\
	(v)[7] = (a)[3];				}

#define OBB_TO_AABB( a, v ) {		\
	(a)[0] = (v)[0];				\
	(a)[1] = (v)[1];				\
	(a)[2] = (v)[4];				\
	(a)[3] = (v)[5];				}

#define SWAP_VERT8_90( v ) {		\
	float __tx = v[2];				\
	float __ty = v[3];				\
	v[2] = v[0];					\
	v[3] = v[1];					\
	v[0] = v[6];					\
	v[1] = v[7];					\
	v[6] = v[4];					\
	v[7] = v[5];					\
	v[4] = __tx;					\
	v[5] = __ty;					}

#define COPY_POLY( p, p0 ) {		\
	(p)->x = (p0)->x;				\
	(p)->y = (p0)->y;				\
	(p)->w = (p0)->w;				\
	(p)->h = (p0)->h;				\
	(p)->angle = (p0)->angle;		}

#define VERT8_IS_EQUAL( v, w ) 		\
	(	(v)[0] == (w)[0] && 		\
		(v)[1] == (w)[1] &&			\
		(v)[2] == (w)[2] &&			\
		(v)[3] == (w)[3] &&			\
		(v)[4] == (w)[4] &&			\
		(v)[5] == (w)[5] &&			\
		(v)[6] == (w)[6] &&			\
		(v)[7] == (w)[7] )

#define POLY_IS_EQUAL( p1, p2 )		\
	(	(p1)->x == (p2)->x &&		\
		(p1)->y == (p2)->y &&		\
		(p1)->w == (p2)->w &&		\
		(p1)->h == (p2)->h &&		\
		(p1)->angle == (p2)->angle )

#define _AABB_TO_POLY( p, A )		\
	{	(p)->x = (A)[0];			\
		(p)->y = (A)[1];			\
		(p)->w = (A)[2] - (A)[0];	\
		(p)->h = (A)[3] - (A)[1];	\
		(p)->angle = 0.0f;			}

// add a 2d vector into an AABB
#define AABB_add( D, A, S ) 				\
	{ 	(D)[0] = (A)[0] + (S)[0];			\
		(D)[1] = (A)[1] + (S)[1];			\
		(D)[2] = (A)[0] + (S)[2];			\
		(D)[3] = (A)[1] + (S)[3];			}
	
#define AABB_mult( D, A, S ) 				\
	{ 	(D)[0] = (A)[0] * (S)[0];			\
		(D)[1] = (A)[1] * (S)[1];			\
		(D)[2] = (A)[0] * (S)[2];			\
		(D)[3] = (A)[1] * (S)[3];			}

//============================================================================
//============================================================================
//============================================================================




typedef float vert2_t[2];
typedef float vert3_t[3]; 

// AXIALLY ALIGNED BOUND BOX : 4 points, 2 vertices, the least and greatest, computationally speedy
typedef float AABB_t[4];

/* OBB_t definition, an OBB's points will be in order, counter clockwise, 
 	but will not necessarily be sorted */
typedef float OBB_t[8]; // i've been using v[8] "vert8" as a synonym for this

#ifndef __FUKKIN_DEF_BOX_T
#define __FUKKIN_DEF_BOX_T
struct box_t {
	float x, y;
	float w, h;
};
#endif // __FUKKIN_DEF_BOX_T


#ifndef __FUKKIN_POLY_T
#define __FUKKIN_POLY_T
/*
====================
 poly_t
====================
*/
struct poly_t : public box_t {
	float angle;
	// im writing this this way specifically because of how MLP really pissed
	//  me off last night by trumping up RAII, and insisting (what nerve) as
	//  if the c++ way were the only way to do things.  what nerve.  I cant
	//  wait to quit this company

	void set( float, float, float, float, float );
	void set( poly_t const & );
	void set_xy( float, float );
	void set_rot( float );
	void set_wh( float, float );
	void getCenter( float * );
	void set_xy_delta( float, float );
	void getMidPoints( float * );
	void getVerts( float * );
	void toOBB( OBB_t );
	void toOBBSorted( OBB_t );
	void toAABB( AABB_t );
	void rotateNinetyCC( void );
	void rotateNinety( void );
	void zero( void ) { x = y = w = h = angle = 0.f; }
};
inline void poly_t::set( float _x, float _y, float _w, float _h, float _angle ) {
	x = _x;
	y = _y;
	w = _w;
	h = _h;
	angle = _angle;
}
inline void poly_t::set( poly_t const & pp ) {
	COPY_POLY( this, &pp );
}
inline void poly_t::set_xy( float _x, float _y ) {
	x = _x;
	y = _y;
}
inline void poly_t::set_rot( float angle ) {
	this->angle = angle;
}
inline void poly_t::set_wh ( float _w, float _h ) {
	w = _w;
	h = _h;
}
inline void poly_t::getCenter( float *c ) {
	c[0] = x + w * 0.5f;
	c[1] = y + h * 0.5f;
}
inline void poly_t::set_xy_delta( float dx, float dy ) {
	x += dx;
	y += dy;
}

void M_TranslateRotate2DVertSet( float *, float *, float );

inline void poly_t::getMidPoints( float *v ) {
	v[0] = x + 0.5f * w;
	v[1] = y;
	v[2] = x + w;
	v[3] = y + 0.5f * h;
	v[4] = v[0];
	v[5] = y + h;
	v[6] = x;
	v[7] = v[3];

	float c[2];
	getCenter( c );
	M_TranslateRotate2DVertSet( v, c, angle );
}
inline void poly_t::getVerts( float * v ) {
	this->toOBB( v ) ;
}
#endif // __FUKKIN_POLY_T




inline void AABB_getCenter( float *c, AABB_t A ) {
	c[0] = A[0] + 0.5f * ( A[2] - A[0] );
	c[1] = A[1] + 0.5f * ( A[3] - A[1] );
}


// what to do when a collision occurs
// function pointers, report Collision, play sound, calculate Damage, stuff
//  like that.  all this really need to know is, which function pointer to
//  call for which collision type (between whom?)
class collisionInfo_t {
};

// wingdi.h defines this already for Color Mode, I'll define it for my own use, if not under win32
#ifndef CM_NONE
#define CM_NONE	0
#endif

enum colModelType {
	COLMOD_NONE, 
	CM_AABB, 
	CM_OBB,  
};

class AABB_node : public Allocator_t {
public:
	AABB_t AABB;
	AABB_node *next;

	AABB_node() : next(0) {}
	// 
	AABB_node( AABB_t const & A ) : next(0) {
		AABB[ 0 ] = A[ 0 ];
		AABB[ 1 ] = A[ 1 ];
		AABB[ 2 ] = A[ 2 ];
		AABB[ 3 ] = A[ 3 ];
	}

	// !!! deprecated **** causes error when colModel_t::destructor runs!!!
	static AABB_node * New( AABB_t in = NULL ) {
		AABB_node * a = new AABB_node;
		//AABB_node *a = (AABB_node*) V_Malloc( sizeof(AABB_node) );
		a->next = NULL;
		if ( in ) { a->AABB[0] = in[0];
					a->AABB[1] = in[1];
					a->AABB[2] = in[2];
					a->AABB[3] = in[3]; }
		return a;
	}
	AABB_node * Copy( void ) const ;
	void Destroy( void );
};

class OBB_node : public Allocator_t {
public:
	OBB_t OBB;
	OBB_node *next;

	//
	OBB_node( OBB_t const & A ) : next(0) {
		for ( int i = 0; i < 8; i++ ) {
			OBB[ i ] = A[ i ] ;
		}
	}

	OBB_node() : next(0) {}
	OBB_node( OBB_node const & A ) : next(0) { 
		for ( int i = 0; i < 8; i++ ) {
			OBB[ i ] = A.OBB[ i ] ;
		}
	}
	~OBB_node() {}

	// !!! deprecated **** causes error when colModel_t::destructor runs!!!
	static OBB_node * New( OBB_t in =NULL ) {
		//OBB_node *n = (OBB_node*) V_Malloc( sizeof(OBB_node) );
		OBB_node * n = new OBB_node;
		n->next = NULL;
		if ( in ) { n->OBB[0] = in[0]; n->OBB[1] = in[1];
					n->OBB[2] = in[2]; n->OBB[3] = in[3];
					n->OBB[4] = in[4]; n->OBB[5] = in[5];
					n->OBB[6] = in[6]; n->OBB[7] = in[7]; }
		return n;
	}
	OBB_node * Copy( void ) const ;
	void Destroy( void );
};

// collision geometry, collection of AABB or OBB boxes
class colModel_t : public Auto_t {
private:
	void _my_init() { /* noop */ }
	void _my_reset() ;
	void _my_destroy() ;
public:

	colModelType type;

	AABB_t box;						// Axialy aligned bound box around entire
									//  colModel, irrespective of it's type

	AABB_node *AABB;				// if type == CM_AABB
	OBB_node *OBB;					// if type == CM_OBB

	unsigned int count;				// how many boxes

	collisionInfo_t info;

	// returns if now on or off
	bool toggle( poly_t * ) ;

	colModel_t * Copy( void ) const ;
	static colModel_t * New( void );
	void Destroy( void );

	void rotate( float );
	void rot90( unsigned int =1 );

	void clearOBB( void ) ;
	void clearAABB( void );

	void pushAABB( AABB_t );
	void pushOBB( OBB_t );

	void sync( poly_t * );

	void drag( float, float );

	colModel_t() : AABB(0), OBB(0) { _my_reset(); }
	colModel_t( colModel_t const & );
	colModel_t( poly_t & );
	~colModel_t() { _my_destroy(); }

	// returns true if point found in colModel
	bool checkPoint( float * );

	// returns true if any of colModel is inside given AABB
	bool isInAABB( float * );
	bool collideAABB( AABB_t );
	bool collideOBB( OBB_t );
	bool boxIntersect( AABB_t );
	bool collide( colModel_t & ); // master handler

	// set this colModel from another
	void set( colModel_t const & );

	// adjust the colModel
	int adjust( int, float ); 
};

class brush_t : public poly_t {
public:	
	float s, t;
};


struct quadExpanded_t {
	float a[2];
	float c1[4];
	float b[2];
	float c2[4];
	float c[2];
	float c3[4];
	float d[2];
	float c4[4];
	void setColor( float r, float g, float b, float a ) {
		c1[0] = c2[0] = c3[0] = c4[0] = r;
		c1[1] = c2[1] = c3[1] = c4[1] = g;
		c1[2] = c2[2] = c3[2] = c4[2] = b;
		c1[3] = c2[3] = c3[3] = c4[3] = a;
	}
};

/*
====================
 quad_t

	datastructure that fits nicely in an OpenGL Vertex Array
	we convert a set of polys to an array of adjacent quads which in turn
	fits as an vertex array.
====================
*/
struct quad_t {
	float a[3];
	unsigned int c1;
	float b[3];
	unsigned int c2;
	float c[3];
	unsigned int c3;
	float d[3];
	unsigned int c4;
	void setColor4f( float r, float g, float b, float a ) {
		unsigned char ir = (unsigned char)(r * 255.0f);
		unsigned char ig = (unsigned char)(g * 255.0f);
		unsigned char ib = (unsigned char)(b * 255.0f);
		unsigned char ia = (unsigned char)(a * 255.0f);
		c1 = c2 = c3 = c4 = ia<<24|ib<<16|ig<<8|ir;
	}
	void setColori( unsigned int c ) {
		c1 = c2 = c3 = c4 = c;
	}
	void setCoords8v( float *v ) {
		a[0] = v[0];
		a[1] = v[1];
		b[0] = v[2];
		b[1] = v[3];
		c[0] = v[4];
		c[1] = v[5];
		d[0] = v[6];
		d[1] = v[7];
		a[2] = b[2] = c[2] = d[2] = 0.f;
	}
};

/* 
====================
 texquad_t
====================
*/
struct texquad_t {
	float a[3];
	float t1[2];
	float b[3];
	float t2[2];
	float c[3];
	float t3[2];
	float d[3];
	float t4[2];
	int texnum;
};

/*
====================
 line_t

 for vertex array blitting 
====================
*/
struct line_t {
	float u[2];
	unsigned int c;
	float v[2];
	unsigned int c2;
};

/*
====================
 point_t
	
	can use for linestrips, triangle strips, etc.
====================
*/
struct point_t {
	float u[2];
	unsigned int c;
};


// FIXME: put all the stuff right into the image_t that is used
// at gl compile time.
typedef struct glImgInfo_s {
    uint32 wrap_s;
    uint32 wrap_t;
    /* GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER, 
     * GL_MIRRORED_REPEAT, GL_CLAMP_TO_EDGE, GL_REPEAT, GL_CLAMP,
     * GL_GENERATE_MIPMAP */
} glImgInfo_t;

typedef enum {
	MTL_NONE,
    MTL_COLOR,    			// gl colored poly
    MTL_TEXTURE_STATIC,        // single texture
	MTL_COLORMASK,
	MTL_MASKED_TEXTURE,
	MTL_MASKED_TEXTURE_COLOR,
    MTL_MULTITEX,       // multiple textures (tex through a tex mask) 
    MTL_OSCILLATOR,		// (sine | square | triangle) wave color oscillator
	MTL_MASKED,			// color through an alpha mask
	MTL_MASKED_OSCILLATOR,	// oscillator color through a mask
} materialType_t;

struct materialString_t {
	int type;
	char *string;
};

extern materialString_t matNames[];
extern const int TOTAL_MATERIAL_TYPES;


/*
========================================
	material_t
========================================
*/
class material_t : public Auto_t {
private:
	void _my_init() { /* nothing */ }
	void _my_reset();
	void _my_destroy() { /* images managed elsewhere */ }

	static const unsigned int name_sz = 128;
public:
	char name[ name_sz ];  	// custom material name

    int h, w;       // height of displayed img ( pixels )

//    int s1, s2, t1, t2;		// offset into image, if sub-image (s,t,r,q)

    vec4_t color;   // GL color
    uint gl_attrib; // BLEND, CLIP, DEPTH_BUF, REPEATING, CLAMPED, or'd

    materialType_t type;       

    class image_t *img;	// NULL if MTL_COLOR | MTL_SPECIAL, !NULL for all others
	class image_t *sub;	// NULL unless MTL_MULTITEX
	class image_t *mask;

	float s[4], t[4];
	/*
    float s1, t1, s2, t2; // if shifted or special, also for moving one tex
                            // under another
							*/

	glImgInfo_t gl_info;


	void rot90TexCoords( void ) {
		float ts = s[0];
		float tt = t[0];
		s[0] = s[1];
		t[0] = t[1];
		s[1] = s[2];
		t[1] = t[2];
		s[2] = s[3];
		t[2] = t[3];
		s[3] = ts;
		t[3] = tt;
	}
	void rot90ccTexCoords( void ) {
		float ts = s[0];
		float tt = t[0];
		s[0] = s[3];
		t[0] = t[3];
		s[3] = s[2];
		t[3] = t[2];
		t[2] = t[1];
		s[2] = s[1];
		s[1] = ts;
		t[1] = tt;
	}

	material_t * Copy( void ) const;
	static material_t * New( void );
	void Destroy( void );

	static material_t * NewTextureMaterial( const char *, image_t * );

	material_t() : img(0), sub(0), mask(0) { _my_reset(); }
	material_t( material_t const & ); 
	~material_t() { _my_destroy(); }

};


/*
============================================
 animFrame_t , animation_t & animSet_t
============================================
*/

enum animType_t {
	ANIM_LOOPING 			= 1,
	ANIM_BACK_AND_FORTH 	= 2,
	ANIM_ONCE				= 3
};

/*
- an animation has:
	- Name
	- # of frames
	- FPS of frames  
	- the material name of each frame
	- bbox for entire animation (AABB)
	- Direction flag
		- Normal / Looping  0, 1, 2, 0, 1, 2, ..
		- ascend / descend  0, 1, 2, 1, 0, 1, 2, ..
*/

class animation_t : public Allocator_t {
private:
	static unsigned int anim_id;
	static const unsigned int NAME_SIZE = 16;
public:
	char name[ NAME_SIZE ];
	material_t **frames;	// list of animation frames

	int total;				// total number of frames held
	int index;				// current frame

	float fps;				// fps, real
	int mspf;				// millisecond per frame, whole number

	byte direction;			// low-bits determine looping or up-n-down,
							// high bit is set on way back down

	animation_t() : frames(0), index(0), total(0) {}
	animation_t( animation_t const & ) ;
	~animation_t() { destroy(); }

	void init( unsigned int _frames =32, const char * =NULL, byte =ANIM_LOOPING );
	void destroy ();
	void reset() {}

	int last;				// time of last frame change
	material_t * advance( int =-1 );

	void setFPS( float _f ) {
		fps = _f;
		mspf = 1000.0f / fps;
	}
	void setMSPF( int _i ) {
		mspf = _i;
		fps = 1000.0f / mspf;
	}
	material_t ** copy_frames();
};


/*
==================== 
 animSet_t
	an animSet is a group of animations.
==================== 
*/
class animSet_t : public buffer_c<animation_t>, public Allocator_t {
private:
	bool started;
	static const unsigned int INITIAL_SET_SIZE = 8;
	int lastAccess; // index of last animation accessed by getMat()
public:
	animSet_t() : started(0) {}
	animSet_t( const animSet_t & );

	void add( animation_t *anim ) {
		if ( !started ) {
			buffer_c<animation_t>::init( INITIAL_SET_SIZE );
			started = true;
		}
		// add the local members
		buffer_c<animation_t>::add( *anim );
		
		// copy each frame (frames are material pointers)
		animation_t *A = &data[length()-1];
		A->frames = anim->copy_frames();
	}
	// pre-allocate animations, return pointer to first
	animation_t * startAnim( unsigned int howmany =1 ) {
		if ( !started ) {
			buffer_c<animation_t>::init( INITIAL_SET_SIZE );
			started = true;
		}
		animation_t dummy;
		uint ofst = free_p - data;
		while ( howmany-- ) {
			buffer_c<animation_t>::add( dummy );
		//	data[ free_p - data - 1 ];
		}
		return &data[ ofst ];
	}

	// looks like I'm echoing C++'s behavior here.  could just use the new[] allocator
	// and let cpp doit.
	~animSet_t() {
		const unsigned int len = length();
		for ( unsigned int i = 0; i < len; i++ ) {
			data[ i ].destroy();
		}
		destroy();
	}
	
	// override operator to provide reference
	animation_t & operator[]( unsigned int index ) {
		return data[ index ];
	}

	// fetch animation by name. if no frame# provided, animation chooses frame
	material_t *getMat( const char *, int =-1 );

	// returns frame number the lastAccessed animation is currently on
	int frameNo() { if ( lastAccess >= 0 && lastAccess < length() ) return data[ lastAccess ].index; return 0; }

    // get handle to the animation
    animation_t *getAnim( const char * );
};






//
// tile_t
//

// the visual description that each maptile or entity possess
class tile_t : public Auto_t {
private:
protected:
	virtual void _my_init() ;
	virtual void _my_reset() ;
	virtual void _my_destroy() ;
public:
	animSet_t *anim;
	material_t *mat;	// whichever's not null: anim or mat.  can't be both
	colModel_t *col;	// there may or may not be a colModel
	poly_t poly; 		// dimension & location 
	int layer;			// which layer is the tile on?

	tile_t() : anim(0), mat(0), col(0) { _my_reset(); }
	tile_t( tile_t const & );
	virtual ~tile_t() { _my_destroy(); }

	void rot90( unsigned int =1 );
	void setArbRotation( float );

	bool do_sync;	// when disabled, tile will not auto-sync. It is 
					//  automatically disabled if colModel is modified 
					//  in colModel editing mode.  can be turned back on
					//  manually in the console w/ resync command.
	void sync( void ); // syncs col model to current display geometry
	void DeSynchronize( void ) { do_sync = false; }
	void syncNoBlock( void ); // same as sync, but doesn't care whether mode
								// is set or not.  Used by functions that
								// always sync, like Dragging
	void dragColModel( float, float );
};

//enttile_t := (animation_t | material_t) & colModel_t & (poly_t | brush_t)
class entTile_t : public tile_t {
};


//maptile_t := (animation_t | material_t) & colModel_t & (poly_t | brush_t)
class mapTile_t : public tile_t {
private:

	static int nextMapTileUID;

	void _my_init();
	void _my_reset();
	void _my_destroy();

public:
	int uid; 
	bool snap;
	bool lock;
	int background;

	mapTile_t() : uid(-1), snap(1), lock(0), background(0) { _my_reset(); }
	mapTile_t( mapTile_t const & );
	~mapTile_t() { _my_destroy(); }

	// generate a copy of this
	mapTile_t * Copy( void );
	static mapTile_t * New( colModel_t * =NULL );
	static mapTile_t * NewMaterialTile( void );
	void Destroy( void );

	void regenUID( void ) { this->uid = ++nextMapTileUID; }
};






//
// materialSet_t
//
class materialSet_t {

private:
	unsigned int magic;
	static const unsigned int MTLSET_MAGIC = 0x93A870AD;

public:

	buffer_c<material_t*> mat;

	unsigned int size( void ) { return mat.length(); }

    int total;
	int current;

	void reset( void ) {
		total = 0;
		current = 0;
	}

	void init( void ) {
		if ( magic == MTLSET_MAGIC ) {
			reset();
			return;
		}
		reset();
		mat.init();
		magic = MTLSET_MAGIC;
	}

	materialSet_t() { reset(); }
	
	material_t * FindByName( const char * );

	void add ( material_t *new_mat ) {
		if ( !new_mat )
			return;
		mat.push( new_mat );
		++total;
	}

	material_t * PrevMaterial( void ) ;
	material_t * NextMaterial( void ) ;

	int isInit( void ) { return magic == MTLSET_MAGIC; }

	void LoadTextureDirectory( void );
	void ReLoadTextures( void );
};


extern materialSet_t materials;
bool AABB_intersection( AABB_t , AABB_t );
bool OBB_intersection( OBB_t, OBB_t );

inline bool AABB_collide( AABB_t a, AABB_t b ) {
	return AABB_intersection( a, b );
}
inline bool AABB_collision( AABB_t a, AABB_t b ) {
	return AABB_intersection( a, b );
}
inline bool AABB_intersect( AABB_t a, AABB_t b ) {
	return AABB_intersection( a, b );
}


/*
====================
 OBB_center
====================
*/
inline void OBB_getCenter( OBB_t obb, float *c ) {
	// 1/2 catty corner vector
	c[0] = (obb[4]-obb[0])*0.5f;
	c[1] = (obb[5]-obb[1])*0.5f;
	c[0] += obb[0];
	c[1] += obb[1];
}

/*
====================
 OBB_getMidPoints
====================
*/
inline void OBB_getMidPoints( OBB_t obb, float *v ) {
	v[0] = 0.5f * ( obb[2] - obb[0] ) + obb[0];
	v[1] = 0.5f * ( obb[3] - obb[1] ) + obb[1];
	v[2] = 0.5f * ( obb[4] - obb[2] ) + obb[2];
	v[3] = 0.5f * ( obb[5] - obb[3] ) + obb[3];
	v[4] = 0.5f * ( obb[6] - obb[4] ) + obb[4];
	v[5] = 0.5f * ( obb[7] - obb[5] ) + obb[5];
	v[6] = 0.5f * ( obb[0] - obb[6] ) + obb[6];
	v[7] = 0.5f * ( obb[1] - obb[7] ) + obb[7];
}

/*
====================
 OBB_TO_POLY_NO_ROTATE
====================
*/
// FIXME: Very Broken!
inline void OBB_TO_POLY_NO_ROTATE( poly_t *p, float *w ) {
	// first sort the vertices
	float v[8];
	COPY8( v, w );
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

	(p)->x = (v)[0];				
	(p)->y = (v)[1];		
	(p)->w = (v)[2] - (v)[0];
	(p)->h = (v)[5] - (v)[3];
}

// TODO: get this figured out
inline void OBB_TO_POLY( poly_t *p, float *obb ) {}

class CollisionChecker_c {
public:
	static bool aabb_aabb( AABB_t, AABB_t );
	static bool aabb_poly( AABB_t, poly_t *p );
	static bool aabb_point( AABB_t, float * );
	static bool aabb_point_i( AABB_t, int * );

	static bool poly_poly( poly_t *, poly_t * );
	static bool poly_point( poly_t *, float * );
	static bool poly_point_i( poly_t *, int * );

	// returns octant number of click's clicks location in relation to poly
	static int poly_point_octant( poly_t *, float * );
	static int poly_point_octant_i( poly_t *, int * );

	static bool v8_v8( float * , float * );
	static bool v8_poly( float *, poly_t * );
	static bool v8_aabb( float *, AABB_t ); 
	static bool v8_point( float *, float * );
	static bool v8_point_i( float *, float * );
};
extern CollisionChecker_c check;

class GeometryConverter_c {
};
extern GeometryConverter_c convert;

//============================================================================
//============================================================================
//============================================================================
#define GET_IF_LESSER( a, b ) { if ( (b) < (a) ) (a) = (b); }
#define GET_IF_GREATER( a, b ) { if ( (b) > (a) ) (a) = (b); }

inline void POLY_TO_AABB_ROTATED( AABB_t AABB, poly_t *p ) {
	float v[8], c[2];
	POLY_TO_VERT8( v, p );
	p->getCenter( c );
	M_TranslateRotate2DVertSet( v, c, p->angle );
	AABB[0] = 1e20f;
	GET_IF_LESSER( AABB[0], v[0] );
	GET_IF_LESSER( AABB[0], v[2] );
	GET_IF_LESSER( AABB[0], v[4] );
	GET_IF_LESSER( AABB[0], v[6] );
	AABB[1] = 1e20f;
	GET_IF_LESSER( AABB[1], v[1] );
	GET_IF_LESSER( AABB[1], v[3] );
	GET_IF_LESSER( AABB[1], v[5] );
	GET_IF_LESSER( AABB[1], v[7] );
	AABB[2] = -1e20f;
	GET_IF_GREATER( AABB[2], v[0] );
	GET_IF_GREATER( AABB[2], v[2] );
	GET_IF_GREATER( AABB[2], v[4] );
	GET_IF_GREATER( AABB[2], v[6] );
	AABB[3] = -1e20f;
	GET_IF_GREATER( AABB[3], v[1] );
	GET_IF_GREATER( AABB[3], v[3] );
	GET_IF_GREATER( AABB[3], v[5] );
	GET_IF_GREATER( AABB[3], v[7] );
}

/*
====================
 POLY_TO_AABB

 way faster version of POLY_TO_AABB for polys w/ 0 angle
====================
*/
inline void POLY_TO_AABB( AABB_t AABB, poly_t *p ) {
	if ( !p ) return;
	if ( !ISZERO( p->angle ) ) {
		return POLY_TO_AABB_ROTATED( AABB, p );
	}
	AABB[0] = p->x;
	AABB[1] = p->y;
	AABB[2] = p->x + p->w;
	AABB[3] = p->y + p->h;
}


/*
====================
 AABB_TO_POLY
====================
*/
inline void AABB_TO_POLY( poly_t *p, AABB_t a ) {
	p->x = a[0];
	p->y = a[1];
	p->w = a[2] - a[0];
	p->h = a[3] - a[1];
	p->angle = 0.f;
}

/*
====================
 POINTS_TO_AABB

 2 points forming a box, not necessarily in the right order
====================
*/
inline void POINTS_TO_AABB( AABB_t b, AABB_t a ) {
	if ( a[0] < a[2] ) 
		b[0] = a[0], b[2] = a[2];
	else
		b[0] = a[2], b[2] = a[0];
	if ( a[1] < a[3] )
		b[1] = a[1], b[3] = a[3];
	else
		b[1] = a[3], b[3] = a[1];
}

inline void OBB_zero( OBB_t OBB ) {
	OBB[0] = 0.f;
	OBB[1] = 0.f;
	OBB[2] = 0.f;
	OBB[3] = 0.f;
	OBB[4] = 0.f;
	OBB[5] = 0.f;
	OBB[6] = 0.f;
	OBB[7] = 0.f;
}

inline void AABB_zero( AABB_t AABB ) {
	AABB[0] = 0.f;
	AABB[1] = 0.f;
	AABB[2] = 0.f;
	AABB[3] = 0.f;
}

// draws an AABB axialy aligned around OBB. will be larger volume
inline void OBB_TO_AABB_ENCLOSURE( AABB_t a, OBB_t o ) {
	// lowest x
	a[0] = o[0];
	if ( o[2] < a[0] )
		a[0] = o[2];
	if ( o[4] < a[0] )
		a[0] = o[4];
	if ( o[6] < a[0] )
		a[0] = o[6];

	// lowest y
	a[1] = o[1];
	if ( o[3] < a[1] )
		a[1] = o[3];
	if ( o[5] < a[1] )
		a[1] = o[5];
	if ( o[7] < a[1] )
		a[1] = o[7];

	// highest x
	a[2] = o[0];
	if ( o[2] > a[2] )
		a[2] = o[2];
	if ( o[4] > a[2] )
		a[2] = o[4];
	if ( o[6] > a[2] )
		a[2] = o[6];

	// highest y
	a[3] = o[1];
	if ( o[3] > a[3] )
		a[3] = o[3];
	if ( o[5] > a[3] )
		a[3] = o[5];
	if ( o[7] > a[3] )
		a[3] = o[7];
}

inline void AABB_addvec( AABB_t A, float *vec ) {
	A[0] += vec[0];
	A[1] += vec[1];
	A[2] += vec[2];
	A[3] += vec[3];
}
inline void OBB_addvec( OBB_t O, float *vec ) {
	O[0] += vec[0];
	O[1] += vec[1];
	O[2] += vec[0];
	O[3] += vec[1];
	O[4] += vec[0];
	O[5] += vec[1];
	O[6] += vec[0];
	O[7] += vec[1];
}
inline void OBB_add( OBB_t O, float *vec ) {
	OBB_addvec( O, vec );
}

inline float Dist( float *v, float *c ) {
	return sqrtf( (v[0]-c[0])*(v[0]-c[0]) + (v[1]-c[1])*(v[1]-c[1]) ); 
}

// adjust colModel. corner arguments include: X, Y, H, W lower & upper case
int CM_Nudge_Command( const char *str, float amt );

material_t * M_GetMtl( const char * name );

//============================================================================
//============================================================================
//============================================================================

#endif
