#ifndef __G_ANIMATION_H__
#define __G_ANIMATION_H__

//#include "../common/com_geometry.h"
//#include "../common/com_object.h"

#if 0 // I want to move these here, but there are too many inter-
		// dependencies between things in com_geometry
enum animType_t {
	ANIM_LOOPING 			= 1,
	ANIM_ASCEND_DESCEND 	= 2,
	ANIM_ONCE				= 3
};

/*
============================================
 animFrame_t , animation_t & animSet_t
============================================
*/
struct animFrame_t : public Allocator_t {
	material_t *mat;
	animFrame_t *next;

	// ctor & dtor apply to just this one obj, not the chain
	animFrame_t() : mat(0), next(0) {}
	animFrame_t( material_t const & M ) : mat(0), next(0) {
		mat = new material_t( M );
	}
	animFrame_t( animFrame_t const & A ) : mat(0), next(0) { 
		if ( A.mat ) {
			mat = new material_t( *A.mat );
		}
	}
	~animFrame_t() { if ( mat ) { delete mat; mat = NULL; } }

	// static funcs apply to the chain
	static void pushMat( animFrame_t **head, material_t const & _mat ) {
		if ( !*head ) {
			*head = new animFrame_t( _mat );
			return;
		}
		animFrame_t *rov = *head;
		while ( rov->next ) {
			rov = rov->next;
		}
		rov->next = new animFrame_t( _mat );
	}

	static animFrame_t * CopySet( const animFrame_t * src ) {
		if ( !src ) 
			return NULL;
		animFrame_t * out = NULL, * rov = NULL;

		pushMat( &out, *src->mat );
		if ( !out || !src->next )
			return out;

		rov = out;
		src = src->next;
		while ( src ) {
			pushMat( &rov->next, *src->mat );
			src = src->next;
			rov = rov->next;
		}
		return out;
	}

	static void DestroySet( animFrame_t * set ) {
		animFrame_t * next;
		for ( ; set ; set = next ) {
			next = set->next;
			delete set;
		}
	}
};

class animation_t : public Auto_t {
private:
	void _my_init() { /* .. empty .. */ }
	void _my_reset();
	void _my_destroy();
public:
	animFrame_t *frames;	// list of animation frames
	animFrame_t *current;	// frame we're currently on

	int total;				// total number of frames held
	int tpframe;			// tics per frame
	int tic;				// tic we're on until frame switch: 

	animation_t() : frames(0), current(0) { _my_reset(); }
	animation_t( animation_t const & ) ;
	~animation_t() { _my_destroy(); }

	animation_t * Copy( void ) const;
	static animation_t * New ( void );
	void Destroy( void );

	void next() { 
		if ( !current ) {
			current = frames;
			return;
		}
		current = current->next;
		if ( !current )
			current = frames;
	}

	void checkAdvance( int _tics ) {
		/* .. */
	}
};

/*
==================== 
 animSet_t
	an animSet is a group of animations.
==================== 
*/
class animSet_t : public Auto_t {
};
#endif















#endif // __G_ANIMATION_H__
