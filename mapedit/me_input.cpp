/*
me_input.cpp
*/


#include "../client/cl_public.h"
#include "../client/cl_keys.h" // keys array
#include "../win32/win_public.h" // Get_MouseFrame
#include "mapedit.h"
#include "../map/mapdef.h"
#include "../common/com_geometry.h"
#include "../client/cl_console.h"
#include "../renderer/r_floating.h"

#include "me_menus.h"

#include <math.h>

//#include "../renderer/gl.h"
#include "../renderer/r_ggl.h"
#define GL_GLEXT_PROTOTYPES
#include "../renderer/glext.h"
#include "../lib/lib_image.h"

#ifndef M_PI
# define M_PI	3.1415926535897932384
#endif

#ifndef snprintf
#define snprintf _snprintf
#endif

#define RAD_TO_DEG 	(180.0f/(float)M_PI)

meInputMode_t inputMode = MODE_POLY_NORMAL; // 
unsigned int input = ME_IN_NONE;



gbool drawGrid = gtrue;


actionState_t drag;

list_c<mapeditCmd_t> cmdque;

int stretchModeHighlight = ME_SM_NONE;


actionState_t stretch;
actionState_t rotate;
actionState_t vpDrag;
actionState_t dragSelect;

activeMapTile_t selected; 
tilelist_t copied, copyTmp;

entNodeList_t entSelected;
entPointerList_t entCopy, entTmp;

actionState_t highlight;


// 
bool shiftKeyModifier = false;
bool ctrlKeyModifier = false;
bool altKeyModifier = false;
bool capslockModifier = false;

#define COPY_VERT( a, b ) { a[0]=b[0];a[1]=b[1];a[2]=b[2]; }
#define COPY_POLY( a, b ) { *(poly_t*)&(a)[0]=*(poly_t*)&(b)[0]; }

int rotateMode = 0;
int rotationType = ROT_NONE;

bool snapToGrid = true;

me_vis_t visibility = ME_VIS_ALL;

/*
====================
 ME_GetWorldMouse

 FIXME: Mouse should really be a class, shouldn't it?
====================
*/
float * ME_GetWorldMouse( void ) {
	static float m[2];
	m[0] = (float)me_mouse.x; 
	m[1] = (float)me_mouse.y;
	M_ScreenToWorld( m );
	return m;
}

/*
====================
 ME_GetWorldCentroid

 returns centroid x,y of selected boxes, and outer edges
====================
*/
float * ME_GetWorldCentroid( void ) {
	static float c[6];
	
	node_c<mapNode_t*> *n = selected.gethead();
	if ( !n || !n->val || !n->val->val )
		return NULL;

	poly_t *p = &n->val->val->poly;	

	// more than 1 selected.  find outer boundaries on the x&y axes
	float xl = 1e20f;
	float xh = -1e20f;
	float yl = 1e20f;
	float yh = -1e20f;

	while ( n ) 
	{
		if ( !n->val || !n->val->val || n->val->val->background ) {
			n = n->next;
			continue;
		}

		p = &n->val->val->poly;

		float v[8];
		p->toOBB( v );

		// determine edge flags so you dont consider inside edges
		for ( int i = 0; i < 8; i += 2 ) {
			if ( v[i] < xl )
				xl = v[i];
			if ( v[i] > xh )
				xh = v[i];
			if ( v[i+1] < yl )
				yl = v[i+1];
			if ( v[i+1] > yh )
				yh = v[i+1];
		}

		n = n->next;
	}

	// take median of group, to get a centroid
	c[0] = xl + 0.5f * ( xh - xl );
	c[1] = yl + 0.5f * ( yh - yl );

	// also store an AABB of the overall
	c[2] = xl;
	c[3] = yl;
	c[4] = xh;
	c[5] = yh;

	return c;
}
float * ME_GetCentroid( void ) { return ME_GetWorldCentroid(); }

/*
====================
 ME_StretchPoly
====================
*/
/*
void ME_StretchPoly( mapNode_t *node ) {
	float v[8];

	if ( !node )
		return;

	POLY_TO_VERT8( v, (&node->val->poly) );

	poly_t *poly = &node->val->poly;

	M_WorldToScreen( v );
	M_WorldToScreen( &v[2] );
	M_WorldToScreen( &v[4] );
	M_WorldToScreen( &v[6] );

	// center, world units
	//float c[2] = { v[0] + stretch.poly->w / 2.f, v[1] + stretch.poly->h / 2.f };
	float c[2] = { v[0] + poly->w / 2.f, v[1] + poly->h / 2.f };

	// center, screen coordinates
	M_WorldToScreen( c );	

	// mouse click now and original mouse position (already in screen coord)
	float m0[2] = { stretch.mx0, stretch.my0 };
	float m1[2] = { (float)me_mouse.x, (float)me_mouse.y };

	// rotate mouse vectors by -angle of the poly rotation putting the mice
	//  coordinates in a virtual, un-rotated coordinate system
//	M_TranslateRotate2d( m0, c, -stretch.poly->angle );
//	M_TranslateRotate2d( m1, c, -stretch.poly->angle );

	M_TranslateRotate2d( m0, c, -poly->angle );
	M_TranslateRotate2d( m1, c, -poly->angle );

	float s[2] = { stretch.x0, stretch.y0 };
	M_WorldToScreen( s );

	// then just get x & y like we would before having to worry about rotation
	if ( stretch.mode & ME_SM_BOT ) 
		v[1] = v[3] = s[1] + ( m1[1] - m0[1] );
	else if ( stretch.mode & ME_SM_TOP ) 
		v[5] = v[7] = s[1] + ( m1[1] - m0[1] );

	if ( stretch.mode & ME_SM_LEFT ) 
		v[0] = v[6] = s[0] + ( m1[0] - m0[0] );
	else if ( stretch.mode & ME_SM_RIGHT ) 
		v[2] = v[4] = s[0] + ( m1[0] - m0[0] );

	// back to the world, Ma
	M_ScreenToWorld( v );
	M_ScreenToWorld( &v[2] );
	M_ScreenToWorld( &v[4] );
	M_ScreenToWorld( &v[6] );

	//VERT8_TO_POLY( stretch.poly, v );
	VERT8_TO_POLY( poly, v );
}
*/



/*
// returns poly_t that it hits, if any
poly_t * ME_ComputeMouseHit( float x, float y ) {

	node_c<mapTile_t *> *n = tiles->gethead();

	if ( !n )
		return NULL;

	// 
	// convert mouse click from screen to world coordinates
	//

	main_viewport_t *v = M_GetMainViewport();
	
	// vector from origin to screen ll-corner, in world units
	float Vs[2] = { v->world.x, v->world.y };

	// vector of click in world units, from ll-corner
	float Vm[2];
	Vm[0] = x * v->ratio;
	Vm[1] = y * v->ratio;

	// add the two vectors together,
	// mouse click world coord is the sum of those 2 vectors
	float Wx = Vs[0] + Vm[0];
	float Wy = Vs[1] + Vm[1];


	while ( n ) 
	{
		poly_t *p = &n->val->poly;

		//
		// create 4 poly face vectors
		//
	
		// need to create 4 rotated points(2d) using angle and AABB points

		// find middle point of poly
		float m[2];
		m[0] = p->x + p->w / 2.f;
		m[1] = p->y + p->h / 2.f;

		// ccwise, bot-left point first
		float v1[2], v2[2], v3[2], v4[2];
		v1[0] = p->x;
		v1[1] = p->y;
		v2[0] = p->x + p->w;
		v2[1] = p->y;
		v3[0] = p->x + p->w;
		v3[1] = p->y + p->h;
		v4[0] = p->x;
		v4[1] = p->y + p->h;
	
		M_TranslateRotate2d( v1, m, p->angle );
		M_TranslateRotate2d( v2, m, p->angle );
		M_TranslateRotate2d( v3, m, p->angle );
		M_TranslateRotate2d( v4, m, p->angle );
		
		// create the 4 side vectors
		float a[2], b[2], c[2], d[2];
		a[0] = v2[0] - v1[0];
		a[1] = v2[1] - v1[1];
		b[0] = v3[0] - v2[0];
		b[1] = v3[1] - v2[1];
		c[0] = v4[0] - v3[0];
		c[1] = v4[1] - v3[1];
		d[0] = v1[0] - v4[0];
		d[1] = v1[1] - v4[1];
	
		// create 4 vectors, from the 4 points to the mouse click
		v1[0] = Wx - v1[0];
		v1[1] = Wy - v1[1];
		v2[0] = Wx - v2[0];
		v2[1] = Wy - v2[1];
		v3[0] = Wx - v3[0];
		v3[1] = Wy - v3[1];
		v4[0] = Wx - v4[0];
		v4[1] = Wy - v4[1];
	
		// if all 4 dot products are > 0, the click lies within the poly
		if (M_DotProduct2d( v1, a ) > 0.f && 
			M_DotProduct2d( v2, b ) > 0.f && 
			M_DotProduct2d( v3, c ) > 0.f && 
			M_DotProduct2d( v4, d ) > 0.f ) {
			return p;
		}

		n = n->next;
	}
	return NULL;
}
*/

// returns poly_t that it hits, if any
node_c<mapTile_t*> * ME_ComputeMouseHit( float x, float y, mapNode_t *cont =NULL ) {

	// user can provide a starting point, otherwise, start at the beginning of the list
	mapNode_t *n = cont ? cont : tiles->gethead();

	if ( !n )
		return NULL;

	// 
	// convert mouse click from screen to world coordinates
	//

	main_viewport_t *v = M_GetMainViewport();
	
	// vector from origin to screen ll-corner, in world units
	float Vs[2] = { v->world.x, v->world.y };

	// vector of click in world units, from ll-corner
	float Vm[2];
	Vm[0] = x * v->ratio;
	Vm[1] = y * v->ratio;

	// add the two vectors together,
	// mouse click world coord is the sum of those 2 vectors
	float Wx = Vs[0] + Vm[0];
	float Wy = Vs[1] + Vm[1];


	while ( n ) 
	{
		poly_t *p = &n->val->poly;

		//
		// create 4 poly face vectors
		//
	
		// need to create 4 rotated points(2d) using angle and AABB points

		// find middle point of poly
		float m[2];
		m[0] = p->x + p->w / 2.f;
		m[1] = p->y + p->h / 2.f;

		// ccwise, bot-left point first
		float v1[2], v2[2], v3[2], v4[2];
		v1[0] = p->x;
		v1[1] = p->y;
		v2[0] = p->x + p->w;
		v2[1] = p->y;
		v3[0] = p->x + p->w;
		v3[1] = p->y + p->h;
		v4[0] = p->x;
		v4[1] = p->y + p->h;
	
		M_TranslateRotate2d( v1, m, p->angle );
		M_TranslateRotate2d( v2, m, p->angle );
		M_TranslateRotate2d( v3, m, p->angle );
		M_TranslateRotate2d( v4, m, p->angle );
		
		// create the 4 side vectors
		float a[2], b[2], c[2], d[2];
		a[0] = v2[0] - v1[0];
		a[1] = v2[1] - v1[1];
		b[0] = v3[0] - v2[0];
		b[1] = v3[1] - v2[1];
		c[0] = v4[0] - v3[0];
		c[1] = v4[1] - v3[1];
		d[0] = v1[0] - v4[0];
		d[1] = v1[1] - v4[1];
	
		// create 4 vectors, from the 4 points to the mouse click
		v1[0] = Wx - v1[0];
		v1[1] = Wy - v1[1];
		v2[0] = Wx - v2[0];
		v2[1] = Wy - v2[1];
		v3[0] = Wx - v3[0];
		v3[1] = Wy - v3[1];
		v4[0] = Wx - v4[0];
		v4[1] = Wy - v4[1];
	
		// if all 4 dot products are > 0, the click lies within the poly
		if (M_DotProduct2d( v1, a ) > 0.f && 
			M_DotProduct2d( v2, b ) > 0.f && 
			M_DotProduct2d( v3, c ) > 0.f && 
			M_DotProduct2d( v4, d ) > 0.f ) {
			return n;
		}

		n = n->next;
	}
	return NULL;
}

/*
========================

========================
*/
mapNode_t *ME_ComputeSelectedMouseHit( float x, float y ) {
	mapNode_t *local = NULL;

	do 
	{
		local = ME_ComputeMouseHit( x, y, local );

		if ( !local )
			return NULL;

		if ( selected.checkNode( local ) ) { 
			return local;
		}

		local = local->next;

	} while ( local );
	
	return local;
}

int M_MousePolyComparison( float *m, float *v );

/*
========================
 ME_SelectedEntMouseHit
========================
*/
entNode_t *ME_SelectedEntMouseHit( float x, float y ) {
	float m[2] = { x , y };
	M_ScreenToWorld( m );
	
	node_c<entNode_t*> *nE = entSelected.gethead();
	while ( nE ) {
		if ( M_MousePolyComparison( m, nE->val->val->obb ) ) {
			return nE->val;
		}
		nE = nE->next;
	}
	return NULL;	
}


/*
=======================
 M_MousePolyComparison

 returns 1 if a given m[2] point is found within the poly v[8]
=======================
*/
// m[2], v[8]
int M_MousePolyComparison( float *m, float *v ) {

	// create the 4 side vectors
	float a[2], b[2], c[2], d[2];
	a[0] = v[2] - v[0];
	a[1] = v[3] - v[1];
	b[0] = v[4] - v[2];
	b[1] = v[5] - v[3];
	c[0] = v[6] - v[4];
	c[1] = v[7] - v[5];
	d[0] = v[0] - v[6];
	d[1] = v[1] - v[7];
	
	float q[8];
	// create 4 vectors, from the 4 points to the mouse click
	q[0] = m[0] - v[0];
	q[1] = m[1] - v[1];
	q[2] = m[0] - v[2];
	q[3] = m[1] - v[3];
	q[4] = m[0] - v[4];
	q[5] = m[1] - v[5];
	q[6] = m[0] - v[6];
	q[7] = m[1] - v[7];
	
	// if all 4 dot products are > 0, the click lies within the poly
	if (M_DotProduct2d( &q[0], a ) > 0.f && 
		M_DotProduct2d( &q[2], b ) > 0.f && 
		M_DotProduct2d( &q[4], c ) > 0.f && 
		M_DotProduct2d( &q[6], d ) > 0.f ) {
		return 1;
	}

	return 0;
}

// Just compares vertices, not very useful when you want OBB collision.
bool M_CompareV8( float *v, float *w ) {
	if (M_MousePolyComparison( &v[0], w ) ||
		M_MousePolyComparison( &v[2], w ) ||
		M_MousePolyComparison( &v[4], w ) ||
		M_MousePolyComparison( &v[6], w ) ||
		M_MousePolyComparison( &w[0], v ) ||
		M_MousePolyComparison( &w[2], v ) ||
		M_MousePolyComparison( &w[4], v ) ||
		M_MousePolyComparison( &w[6], v ) )
		return true;
	return false;
}



void ME_StretchPoly( mapNode_t *node ) {

	if ( !node || !node->val )
		return;

	//main_viewport_t *vp = M_GetMainViewport();

	poly_t *p = &node->val->poly;
	float center[2];
	p->getCenter( center );

	float v[8];
	node->val->poly.toOBB( v );

	// magnitude of stretch 
	float *m = ME_GetWorldMouse();

	// turn off stretch mode, if mouse crosses to inside of the poly it is stretching
	if ( M_MousePolyComparison( m, v ) ) {
		// FIXME: instead this should be MOUSE_POLY_SUSPEND, the problem is, that the only way that stretching
		//  in this particular instance should be re-enabled automatically is if the mouse pointer goes back to
		//  the side in which it started in the first place.  the other 3 sides are disqualifiers, but that still
		//  shouldn't turn _off_ the stretch mode, it should just suspend any stretching from happening until the
		//  mouse appears back in teh proper quadrant again.  This should be remedied by setting a stretchMode 
		//  to sides, when the first one is detected.  and then checking the mouse against that one side right away
		//
		// note also that cornered stretching (two sides, 3 vertices) is not supported by the current configuration
		//  either
		//
		// however, our primary aim, to forbid inverting the polygon, is still achieved by this simple method.
		//inputMode = MODE_POLY_NORMAL;
		return;
	}

	// subline resolution
	int res = 1 << ( 3 + subLineResolution );

	float mi[2] = { stretch.mx0, stretch.my0 };
	mi[0] += stretch.xofst * res;
	mi[1] += stretch.yofst * res;

	float dx = m[0] - mi[0];
	float dy = m[1] - mi[1];

	float tx = 0.f, ty = 0.f;
	bool huzzah = false;
	if ( fabs(m[0] - mi[0]) >= res ) {
		if ( mi[0] > m[0] ) { // neg
			tx = (float)-res;
			stretch.xofst--;
		} else {
			tx = (float)+res;
			stretch.xofst++;
		}
		huzzah = true;
	}
	if ( fabs(m[1] - mi[1]) >= res ) {
		if ( mi[1] > m[1] ) { // neg
			ty = (float)-res;
			stretch.yofst--;
		} else {
			ty = (float)+res;
			stretch.yofst++;
		}
		huzzah = true;
	}

	if ( !huzzah )
		return;

	// see which side is closest to facing the vector of the initial click

	// create 4 vectors from center to side-midpoint
	float h[8], hi[8];
	p->getMidPoints( hi );
	COPY8( h, hi );
	h[0] -= center[0];
	h[1] -= center[1];
	h[2] -= center[0];
	h[3] -= center[1];
	h[4] -= center[0];
	h[5] -= center[1];
	h[6] -= center[0];
	h[7] -= center[1];

	// make unit vectors
	M_Normalize2d( &h[0] );
	M_Normalize2d( &h[2] );
	M_Normalize2d( &h[4] );
	M_Normalize2d( &h[6] );

	// 4 normalized vectors of mouse from midpoints of each side
	float um[8] = { m[0] - hi[0], 
					m[1] - hi[1], 
					m[0] - hi[2],
					m[1] - hi[3],
					m[0] - hi[4],
					m[1] - hi[5],
					m[0] - hi[6],
					m[1] - hi[7] };
	M_Normalize2d( &um[0] );
	M_Normalize2d( &um[2] );
	M_Normalize2d( &um[4] );
	M_Normalize2d( &um[6] );

	// the dot product of current mouse w/ each unit vector closest to 1 determines side
	float s0 = M_DotProduct2d( &um[0], &h[0] );
	float s1 = M_DotProduct2d( &um[2], &h[2] );
	float s2 = M_DotProduct2d( &um[4], &h[4] );
	float s3 = M_DotProduct2d( &um[6], &h[6] );

	// any dot products which are > 0 are a valid quadrant that the mouse is stretching.
	//  at most there will be 2. if there are 2, it is a corner case, and you may stretch
	//  both sides.
	char side, side1 = 0, side2 = 0;
	if ( s0 > 0.f ) 
		side1 = 1;
	if ( s1 > 0.f ) 
		if ( side1 ) {
			side2 = 2;
		} else {
			side1 = 2;
		}
	if ( s2 > 0.f ) 
		if ( side1 ) {
			side2 = 3;
		} else {
			side1 = 3;
		}
	if ( s3 > 0.f ) 
		if ( side1 ) {
			side2 = 4;
		} else {
			side1 = 4;
		}

	if ( side1 && !side2 ) {
		side = side1;
	} else if ( !side1 && side2 ) {
		side = side2;
	}

	// none for some reason
	if ( !side && !side1 && !side2 ) {
		return;
	}

	float dist1, dist2;

/* This doesn't work becuase the designator: side, is thinking in terms of a relative coordinate 
system that has been rotated by "angle" degrees.  this has already been taken into account when
computing hi, therefor, hi is in normal, un-rotated world-coordinates.  

so if I am rotated 90 clockwise, side 1, is the side on the left, 
*/

	// try changing the side according to inherent rotation:
	int adj_angle = (int) p->angle;
	while ( adj_angle < 0 ) {
		adj_angle += 360;
	}
	adj_angle %= 360;
	int adj = 0;
	if ( adj_angle > 269 ) {
		adj = 3;
		ty = tx;
	} else if ( adj_angle > 179 ) {
		if ( ++side > 4 )
			side = 1;
		if ( ++side > 4 )
			side = 1;
	} else if ( adj_angle > 89 ) { 
		adj = 1;
		ty = tx;
	}


	// single side case
	if ( side ) {
		float e[2];
		switch ( side ) {
		case 1: e[0] = hi[0]; e[1] = hi[1]; break;
		case 2: e[0] = hi[2]; e[1] = hi[3]; break;
		case 3: e[0] = hi[4]; e[1] = hi[5]; break;
		case 4: e[0] = hi[6]; e[1] = hi[7]; break;
		}
		dist1 = sqrtf( ( m[0] - e[0] ) * ( m[0] - e[0] ) + ( m[1] - e[1] ) * ( m[1] - e[1] ) );
		// opposite side
		switch ( side ) {
		case 3: e[0] = hi[0]; e[1] = hi[1]; break;
		case 4: e[0] = hi[2]; e[1] = hi[3]; break;
		case 1: e[0] = hi[4]; e[1] = hi[5]; break;
		case 2: e[0] = hi[6]; e[1] = hi[7]; break;
		}
		dist2 = sqrtf( ( m[0] - e[0] ) * ( m[0] - e[0] ) + ( m[1] - e[1] ) * ( m[1] - e[1] ) );
	}

	// corner case
	if ( !side && side1 && side2 ) {
		return ; // for now
	}

	// forbid stretching the poly smaller than the current grid resolution
	if ( fabsf(dist2 - dist1) <= res ) {
		switch ( side ) {
		case 1: if ( ty > 0.f ) return;
			break;
		case 2: if ( tx < 0.f ) return;
			break;
		case 3: if ( ty < 0.f ) return;
			break;
		case 4: if ( tx > 0.f ) return;
		}
	}

	// do the actual stretch to the poly
	switch ( side ) {
	case 1:
		if ( !adj )
			p->y += ty;
		else
			p->x -= tx;
		p->h -= ty;
		break;
	case 2:
		p->w += tx;
		break;
	case 3:
		if ( adj )
			p->x += tx;
		p->h += ty;
		break;
	case 4:
		p->x += tx;
		p->w -= tx;
		break;
	}
}


void ME_StretchSingleTile( poly_t *p ) {

	// magnitude of stretch 
	float *m = ME_GetWorldMouse();

	float m_init[2] = { stretch.mx0, stretch.my0 };
	float m_now[2] = { m[0], m[1] };

	// rotate m_init,m_now -angle around initial centroid
	float c_init[2] = { stretch.x0, stretch.y0 };
	
	// try current center instead of initial
	p->getCenter( c_init );

	M_TranslateRotate2d( m_init, c_init, -p->angle );
	M_TranslateRotate2d( m_now, c_init, -p->angle );

	// now we are in an un-rotated coordinate space with the poly, as if
	//  the poly's angle were set to 0

	// cease stretching if entered poly
	float u[8], v[8];
	p->toOBB( u );
	if ( M_MousePolyComparison( m, u ) ) {
		return;
	}

	// gets poly but ignores angle
	POLY_TO_VERT8( v, p );
	
	//
	// detect which side, from initial click
	//

	// 8 areas affect the behavior of the stretching action
	// 4 primary areas are within the projection of each side plane
	// 4 additional areas are combinations, inducing stretch on two sides


	// 1st, which side are we on
	int side = 0;
	if ( m_now[0] < v[0] ) {
		side |= ME_SM_LEFT;
	} else if ( m_now[0] > v[2] ) { 
		side |= ME_SM_RIGHT;
	}

	if ( m_now[1] > v[5] ) {
		side |= ME_SM_TOP;
	} else if ( m_now[1] < v[3] ) {
		side |= ME_SM_BOT;
	}


	// total offsets since stretch began
	int x_delta = (int) ( m_now[0] - m_init[0] );
	int y_delta = (int) ( m_now[1] - m_init[1] );
 
	// subline resolution
	int lineres = 1 << ( 3 + subLineResolution );

	// 
	int already_done_x = stretch.xofst * lineres;
	int already_done_y = stretch.yofst * lineres;

	// save initial poly for position correction
	poly_t Lr;
	COPY_POLY( &Lr, p );
	int move = 0;

	if ( side & ME_SM_LEFT ) {
		// outward
		if ( x_delta < already_done_x - lineres ) { 
			stretch.xofst--;
			p->x -= lineres;
			p->w += lineres;
			move |= ME_SM_LEFT;
		}
		// inward
		else if ( x_delta > already_done_x ) {
			if ( p->w >= lineres<<1 ) {
				++stretch.xofst;
				p->x += lineres;
				p->w -= lineres;
				move |= ME_SM_LEFT;
			}
		}
	} 
	else if ( side & ME_SM_RIGHT ) {
		if ( x_delta >= already_done_x + lineres ) {
			stretch.xofst++;
			p->w += lineres;
			move |= ME_SM_RIGHT;
		}
		else if ( x_delta < already_done_x ) {
			if ( p->w >= lineres<<1 ) {
				--stretch.xofst;
				p->w -= lineres;
				move |= ME_SM_RIGHT;
			}
		}
	}

	// UP & DOWN
	if ( side & ME_SM_BOT ) {
		if ( y_delta < already_done_y - lineres ) { 
			stretch.yofst--;
			p->y -= lineres;
			p->h += lineres;
			move |= ME_SM_BOT;
		}
		else if ( y_delta > already_done_y ) {
			if ( p->h >= lineres<<1 ) {
				++stretch.yofst;
				p->y += lineres;
				p->h -= lineres;
				move |= ME_SM_BOT;
			}
		}
	}
	else if ( side & ME_SM_TOP ) {
		if ( y_delta >= already_done_y + lineres ) {
			stretch.yofst++;
			p->h += lineres;
			move |= ME_SM_TOP;
		}
		else if ( y_delta < already_done_y ) {
			if ( p->h >= lineres<<1 ) {
				--stretch.yofst;
				p->h -= lineres;
				move |= ME_SM_TOP;
			}
		}		
	} 

	p->toOBB( v ); // poly after stretch

	// if there was a move, add in a correction, to keep the corner(s) fixed that weren't
	//  involved in the stretch
	switch ( move ) {
	case ME_SM_TOP:
		p->x += u[0] - v[0];
		p->y += u[1] - v[1];
		break;
	case ME_SM_BOT:
		p->x += u[4] - v[4];
		p->y += u[5] - v[5];
		break;
	case ME_SM_LEFT:
		p->x += u[4] - v[4];
		p->y += u[5] - v[5];
		break;
	case ME_SM_RIGHT:
		p->x += u[0] - v[0];
		p->y += u[1] - v[1];
		break;
	case ME_SM_BL_CORNER:
		p->x += u[4] - v[4];
		p->y += u[5] - v[5];
		break;
	case ME_SM_BR_CORNER:
		p->x += u[6] - v[6];
		p->y += u[7] - v[7];
		break;
	case ME_SM_TR_CORNER:
		p->x += u[0] - v[0];
		p->y += u[1] - v[1];
		break;
	case ME_SM_TL_CORNER:
		p->x += u[2] - v[2];
		p->y += u[3] - v[3];
		break;
	}

	// has modified tile, do sync
	switch ( move ) {
	case ME_SM_TOP:
	case ME_SM_BOT:
	case ME_SM_LEFT:
	case ME_SM_RIGHT:
	case ME_SM_BL_CORNER:
	case ME_SM_BR_CORNER:
	case ME_SM_TR_CORNER:
	case ME_SM_TL_CORNER:
		selected.syncColModels();
		break;
	}
}

void ME_StretchSelectedSet( void ) {
}

void ME_StretchMapTile ( void ) {

	node_c<mapNode_t*> *node = selected.gethead();
	if ( !node )
		return;
	poly_t *p = &node->val->val->poly;
	mapTile_t *m = node->val->val;
	
	poly_t p_before, p_after;
	float v[8];

	switch ( stretch.mode ) {
	case STRETCH_ONE:

		// if visibility is JUST colModel, substitute the colModel to be
		//  stretched.
		if ( visibility == ME_VIS_COLMODEL ) {
			if ( !m->col )
				return;

			// FIXME: what about multiple models for one poly ?
			// For now, you are forced to assume just one

			if ( m->col->type == CM_AABB ) {
				AABB_TO_POLY( &p_before, m->col->AABB->AABB );
			} else if ( m->col->type == CM_OBB ) {
				//FIXME: how to get a rotated poly from OBB, need to solve this 
				// OBB_TO_POLY( &p_before, m->col->OBB->OBB );
			}
			p = &p_before;
		}

		// do single stretch
		ME_StretchSingleTile( p ); 

		// save poly modifications to colModel
		if ( visibility == ME_VIS_COLMODEL ) {
			// 
			if ( m->col->type == CM_AABB ) {
				AABB_TO_POLY( &p_after, m->col->AABB->AABB );
				if ( !POLY_IS_EQUAL( &p_before, &p_after ) ) {
					m->DeSynchronize();
					p_before.toAABB( m->col->AABB->AABB );
				}
			} else if ( m->col->type == CM_OBB ) {
//				if ( 
					m->DeSynchronize();
					p_before.toOBB( m->col->OBB->OBB );
//				}
			}
		}

		break;
	case STRETCH_MANY:
		ME_StretchSelectedSet();
		break;
	
	}
}


/*
	node_c<mapNode_t*> *node = selected.gethead();
	while ( node ) {
		ME_StretchPoly( node->val );
		node = node->next;
	}

	selected.syncColModels();
*/


/*

OK, brainstorm.  SHIFT+CLICK, has several unique selection behaviors.  if you have one or more
selected, shift+click selects another that is under cursor.  secondly, say there are 4 under 
cursor, and you repetitively click, it should do 1, then 2, then 3, then 4, deselecting the
others each time, and on the 5th click it should do all 4, so thats just 1, just 2, just 3, 
just 4, all 4.  repeat.  How do you do this?  well, you could save the last clicked (because
the mouse is bound to move a tinybit between clicks, this is an inexact science, and the
next shift+click, you start looking at last->next.  this ensures, a different one each time,
and doesn't really care how many there are.  when last->next == NULL, just start from the top!

*/


/*
====================
 ME_SetActiveMapTiles

 setting the active poly on a single left mouse button click
====================
*/
static int ME_SetActiveMapTiles( int button ) {
	static mapNode_t *lastSelect = NULL;

	if ( rotateMode )
		return 0;

	if ( selected.size() > 0 && !shiftKeyModifier )
		return 0;

	float m[2] = { (float)me_mouse.x, (float)me_mouse.y };
	M_ScreenToWorld( m );

	mapNode_t *n = NULL, *endingTile = NULL;
	if ( shiftKeyModifier ) {
		n = ( lastSelect && lastSelect->prev ) ? lastSelect->prev : tiles->gettail();
		endingTile = ( lastSelect && lastSelect->prev ) ? lastSelect->prev : NULL;
	} else {
		n = tiles->gettail();
		endingTile = NULL;
	}

	if ( !n )
		return 0;

	// go through all the tiles
	do {

		float v[8];
		n->val->poly.toOBB( v );

		// check for mouse intersection w/ non-background tile
		if ( M_MousePolyComparison( m, v ) && !n->val->background ) {

			// none selected, go ahead and add
			if ( !selected.anyActive() ) {
				selected.add( n );

			// shiftKey enables special selection mode
			} else if ( shiftKeyModifier ) {

				node_c<mapNode_t*> *handle = NULL;

				// count how many are under cursor. if there is just one, merely toggle its state
				mapNode_t *q = tiles->gethead();
				int countUnderCursor = 0;
				while ( q ) {
					q->val->poly.toOBB( v );

					if ( M_MousePolyComparison( m, v ) && !q->val->background ) {
						++countUnderCursor;
					}
					q = q->next;
				}

				// multiple under cursor
				if ( countUnderCursor > 1 ) {

					if ( 0 == selected.size() ) {
						selected.add( n );
					}

					// multiple currently selected
					else if ( selected.size() >= 1 ) {

						// disable all under cursor and enable next one
						node_c<mapNode_t*> *s = selected.gethead();
						while ( s ) {
							s->val->val->poly.toOBB( v );
							if ( M_MousePolyComparison( m, v ) ) {
								node_c<mapNode_t*> *tmp = s->next;
								selected.popnode( s );
								s = tmp;
							} else {
								s = s->next;
							}
						}
						selected.add( n );
					}

				// just one under cursor, toggle selection
				} else if ( 1 == countUnderCursor ) {
					if ( (handle=selected.checkNode( n )) ) {
						selected.popnode( handle );
					} else { 
						selected.add( n );
					}
				}
			} // if single tile || special select
			lastSelect = n;
			break;

		} else { // if clicked in _this_ tile

			n = n->prev;
			if ( !n && shiftKeyModifier && lastSelect && lastSelect->prev ) {
				n = tiles->gettail();
			}
		}
	} while ( n != endingTile ) ;

	if ( !shiftKeyModifier ) {
		lastSelect = NULL; 
	}
}

/*
====================
 ME_SetActiveEntities

	mouse click routine for selecting entities
====================
*/
static int ME_SetActiveEntities( int button ) {
	if ( rotateMode )
		return 0;
	// require shift key to alter selected set if more than one
	if ( entSelected.size() > 0 && !shiftKeyModifier )
		return 0;
	float *m = ME_GetWorldMouse();
	node_c<Entity_t*> *n = me_ents->gethead();
	while ( n ) {

		// check for mouse intersection w/ Entity_t
		if ( M_MousePolyComparison( m, n->val->obb ) ) {

			// if none selected, add it
			if ( 0 == entSelected.size() ) {
				entSelected.add ( n );

			// shift key allows selection-toggling of multiple
			} else if ( shiftKeyModifier ) {
				node_c<node_c<Entity_t*>*> *handle = NULL;
				if ( (handle=entSelected.checkNode( n )) ) { 
					entSelected.popnode( handle );
				} else {
					entSelected.add( n );
				}
			}
			return 1;
		}

		n = n->next;
	}
	return 0;
}

/*
====================
 ME_SetActive
====================
*/
void ME_SetActive( int button ) {

	// already entities selected
	if ( entSelected.size() > 0 ) {
		ME_SetActiveEntities( button );
	
	} else if ( !entSelected.size() && !selected.size() ) {
		if ( !ME_SetActiveEntities( button ) ) {
			// didn't find any , try tiles
			ME_SetActiveMapTiles( button );
		}
	} else { 
		ME_SetActiveMapTiles( button );
	} 
}


/******************************************************************************

mapTile_t
	animation_t *anim;
	material_t *mat;	// whichever's not null.  can't be both
	colModel_t col;
	poly_t poly; 		// dimension & location 
	int layer;			// which layer is the tile on?

	material_t	
		name[128]
		type
		image_t *

*/

void ME_HoverHighlight( void ) 
{
	node_c<mapTile_t *> *node = tiles->gethead();
	if ( !node )
		return;

	float m[2] = { (float)me_mouse.x, (float)me_mouse.y };
	M_ScreenToWorld( m );

	//poly_t *found = NULL;
	node_c<mapTile_t *> *found = NULL;

	while ( node ) {

		float v[8];
		
		node->val->poly.toOBB( v );

		if ( M_MousePolyComparison( m, v ) ) {
			//found = &node->val->poly;
			found = node;
			break;
		}

		node = node->next;
	}

	if ( found ) {
		highlight.node = found;
	} else {
		highlight.node = NULL;
	}
}


/*
================
 ME_SetMouseOverHighlights
================
*/
int ME_SetMouseOverHighlights( float x, float y ) {


	ME_HoverHighlight();


	// only enter stretch highlight from nothing/normal state
	if ( inputMode != MODE_POLY_NORMAL )
		return 0;

	int mode = ME_SM_NONE;
	const float e = 40.f;	// margin of error to a hit (world-units)

	main_viewport_t *view = M_GetMainViewport();

	// vector from origin to screen ll-corner, in world units
	float Vs[2] = { view->world.x, view->world.y };

	// vector of click in world units, from ll-corner of viewport
	float Vm[2];
	Vm[0] = x * view->ratio;
	Vm[1] = y * view->ratio;

	// mouse world coord is the sum of those 2 vectors
	// add the two vectors together,
	float m[2] = { Vs[0] + Vm[0], Vs[1] + Vm[1] };

	node_c<mapTile_t *> *n = tiles->gethead();
	while ( n ) {

		poly_t *p = &n->val->poly;

		float c[2];

		// center of block in world space units
		c[0] = p->x + p->w / 2.f;
		c[1] = p->y + p->h / 2.f;

		// create 2 extra polys, one a little larger, one a little smaller
		//  (by the factor of 'e'), if the mouse is inside the larger w/o 
		//  being inside the smaller we have a mouse-over
		float A[8], B[8];

		A[0] = p->x - e;
		A[1] = p->y - e;
		A[2] = p->x + p->w + e;
		A[3] = p->y - e;
		A[4] = p->x + p->w + e;
		A[5] = p->y + p->h + e;
		A[6] = p->x - e;
		A[7] = p->y + p->h + e;

		B[0] = p->x + e;
		B[1] = p->y + e;
		B[2] = p->x + p->w - e;
		B[3] = p->y + e;
		B[4] = p->x + p->w - e;
		B[5] = p->y + p->h - e;
		B[6] = p->x + e;
		B[7] = p->y + p->h - e;

		M_TranslateRotate2DVertSet( A, c, p->angle );
		M_TranslateRotate2DVertSet( B, c, p->angle );
		
		if ( M_MousePolyComparison( m, A ) && !M_MousePolyComparison( m, B ) )
		{
			// now we need to figure out a mode

			// , but for now, just set it to anything

			mode = ME_SM_HORIZONTAL | ME_SM_BOT;
			mode = ME_SM_NONE;


			// create 4 polys for the 4 sides, the width 'e'
			// if any of the side polys overlap, that gives a corner

			// bottom
			A[0] = p->x - e;
			A[1] = p->y - e;
			A[2] = p->x + p->w + e;
			A[3] = p->y - e;
			A[4] = p->x + p->w + e;
			A[5] = p->y + e;
			A[6] = p->x - e;
			A[7] = p->y + e;
			M_TranslateRotate2DVertSet( A, c, p->angle );
			if ( M_MousePolyComparison( m, A ) )
				mode |= ME_SM_BOT;

			// right
			A[0] = p->x + p->w - e;
			A[1] = p->y - e;
			A[2] = p->x + p->w + e;
			A[3] = p->y - e;
			A[4] = p->x + p->w + e;
			A[5] = p->y + p->h + e;
			A[6] = p->x + p->w - e;
			A[7] = p->y + p->h + e;
			M_TranslateRotate2DVertSet( A, c, p->angle );
			if ( M_MousePolyComparison( m, A ) )
				mode |= ME_SM_RIGHT;

			// top
			A[0] = p->x - e;
			A[1] = p->y + p->h - e;
			A[2] = p->x + p->w + e;
			A[3] = p->y + p->h - e;
			A[4] = p->x + p->w + e;
			A[5] = p->y + p->h + e;
			A[6] = p->x - e;
			A[7] = p->y + p->h + e;
			M_TranslateRotate2DVertSet( A, c, p->angle );
			if ( M_MousePolyComparison( m, A ) )
				mode |= ME_SM_TOP;

			// left
			A[0] = p->x - e;
			A[1] = p->y - e;
			A[2] = p->x + e;
			A[3] = p->y - e;
			A[4] = p->x + e;
			A[5] = p->y + p->h + e;
			A[6] = p->x - e;
			A[7] = p->y + p->h + e;
			M_TranslateRotate2DVertSet( A, c, p->angle );
			if ( M_MousePolyComparison( m, A ) )
				mode |= ME_SM_LEFT;

			if ( 	mode == (ME_SM_BOT|ME_SM_LEFT) ||
					mode == (ME_SM_BOT|ME_SM_RIGHT) || 
					mode == (ME_SM_TOP|ME_SM_LEFT) || 
					mode == (ME_SM_TOP|ME_SM_RIGHT)  ) {
				mode |= ME_SM_CORNER;
			}
		}

		// set the mode based on spatial relation
		//mode = M_MousePolyComparison( m, v );

		// lookin for a highlight, dont have one
		if ( !stretchModeHighlight ) {
			if ( mode && selected.checkNode( n ) ) {
				stretchModeHighlight = mode;
				stretch.node = n;
				return 1;
			}
		}
		// already in highlight mode, 
		//  stay on the same poly and update the highlight
		//else if ( stretchPoly == p ) {
		else if ( stretch.node == n ) {
			if ( mode ) {
				stretchModeHighlight = mode;
				return 2;
			}
		}

		n = n->next;
	}

	// was in highlight, but now we're not
	if ( stretchModeHighlight ) {
		if ( !mode ) {
			stretchModeHighlight = mode;
			//stretchPoly = NULL;
			stretch.node = NULL;
			return -1;
		}
	}

	return 0;
}

/*
===================
 ME_InitPoly

 start a new poly at 1 meter square, center of the viewport
===================
*/
void ME_InitPoly( poly_t *p ) {
	// 
	main_viewport_t *v = M_GetMainViewport();
	// center of world
	float x = v->world.x + v->unit_width / 2.f;
	float y = v->world.y + v->unit_height / 2.f;

	// subline resolution
	int res = 1 << ( 3 + subLineResolution );

	// get first, left to right 
	float s = (float) ( ((int)x) + res & ~(res-1) );
	
	// first, top to bot
	float r = (float) ( ((int)y) + res & ~(res-1) ); 

	float xy_dim = me_newTileSize->value();
	
	p->set( s, r, xy_dim, xy_dim, 0.f /* no rotation */ );
}

void ME_ReSnapEnts( void ) {
	node_c<entNode_t*> *n = entSelected.gethead();
	if ( !n )
		return;
	
	// subline resolution
	int res = 1 << ( 3 + subLineResolution );

	float v[8];
	n->val->val->poly.toOBB( v );
	// find lower-left corner
	float x , y ;
	x = v[ 0 ];
	y = v[ 1 ];
	if ( v[2] < x ) {
		x = v[2];
		y = v[3];
	}
	if ( v[4] < x ) {
		x = v[4];
		y = v[5];
	}
	if ( v[6] < x ) {
		x = v[6];
		y = v[7];
	}
	if ( v[3] < y ) {
		y = v[3];
		x = v[2];
	}
	else if ( v[5] < y ) {
		y = v[5];
		x = v[4];
	}
	else if ( v[7] < y ) {
		y = v[7];
		x = v[6];
	}

	// get first, left to right 
	float dx = (float) ( ((int)x) + res & ~(res-1) ) - x;
	// first, top to bot
	float dy = (float) ( ((int)y) + res & ~(res-1) ) - y;

	while ( n ) {
		n->val->val->move( dx, dy );
		n = n->next;
	}
}

/*
====================
 ME_ReSnapSelected

 doesn't really care if multiple selected are not aligned in the same grid resolution,
 but if they are , you will have good results
====================
*/
void ME_ReSnapSelected( void ) {

	// subline resolution
	int res = 1 << ( 3 + subLineResolution );

	node_c<mapNode_t*> *node = selected.gethead();

	if ( !node || !node->val ) {
		if ( !entSelected.gethead() )
			return;
		return ME_ReSnapEnts();
	}

	if ( !node->val->val ) {
		while ( node && !node->val->val )
			node = node->next;
		if ( !node || !node->val->val )
			return;
	}

	float v[8];
	node->val->val->poly.toOBB( v );
	// find lower-left corner
	float x , y ;
	x = v[ 0 ];
	y = v[ 1 ];
	if ( v[2] < x ) {
		x = v[2];
		y = v[3];
	}
	if ( v[4] < x ) {
		x = v[4];
		y = v[5];
	}
	if ( v[6] < x ) {
		x = v[6];
		y = v[7];
	}
	if ( v[3] < y ) {
		y = v[3];
		x = v[2];
	}
	else if ( v[5] < y ) {
		y = v[5];
		x = v[4];
	}
	else if ( v[7] < y ) {
		y = v[7];
		x = v[6];
	}

	// get first, left to right 
	float dx = (float) ( ((int)x) + res & ~(res-1) );
	// first, top to bot
	float dy = (float) ( ((int)y) + res & ~(res-1) ); 

	dx -= x;
	dy -= y;

	while ( node ) {
		node->val->val->poly.set_xy_delta( dx, dy );
		node->val->val->dragColModel( dx, dy );
		node->val->val->snap = true;
		node = node->next;
	}
}

/*
====================
 ME_PolyOnScreen
====================
*/
int ME_PolyOnScreen( poly_t *p ) {
	main_viewport_t *w = M_GetMainViewport();
	
	float x1, x2, y1, y2;
			
	x1 = w->world.x;
	y1 = w->world.y;
	x2 = w->world.x + w->unit_width;
	y2 = w->world.y + w->unit_height;

	float v[8];
	POLY_TO_VERT8( v, p );

	float c[2] = { p->x + p->w / 2.f, p->y + p->h / 2.f };
	M_TranslateRotate2DVertSet( v, c, p->angle );

	for ( unsigned int i = 0; i < 8; i+=2 ) {
		// if any verteces on screen, good enough
		if ( v[i] > x1 && v[i] < x2 && v[1+i] > y1 && v[1+i] < y2 )
			return 1;
	}
	return 0;
}

/*
====================
 ME_ToggleGrid
====================
*/
void ME_ToggleGrid( void ) {
	drawGrid = !drawGrid;
}

void ME_RotateGroup90( unsigned int );

/*
====================
 ME_RotateNinetyCC
====================
*/
void ME_RotateNinetyCC( void ) {

	node_c<mapNode_t*> *node = selected.gethead();
	if ( !node )
		return;

	if ( 1 == selected.size() ) {
		node->val->val->rot90( 3 );
		return;
	}

	// rotate set
	ME_RotateGroup90( 3 );
}

/*
====================
 ME_RotateNinety
====================
*/
void ME_RotateNinety( void ) {

	node_c<mapNode_t*> *node = selected.gethead();
	if ( !node )
		return;
	
	if ( 1 == selected.size() ) {
		node->val->val->rot90();
		return;
	}

	// rotate set
	ME_RotateGroup90( 1 );
}

enum {
	LAST_COPY_NONE,
	LAST_COPY_ENTITY,
	LAST_COPY_MAPTILE,
};
static int lastCopy = LAST_COPY_NONE;

/*
====================
 ME_CopyToClipboard
====================
*/
void ME_CopyToClipboard( void ) {
	if ( !selected.anyActive() ) {

		// try entities
		if ( entSelected.size() > 0 ) {
			entSelected.cloneToList( &entCopy );
			lastCopy = LAST_COPY_ENTITY;
		}
		return;
	}

	// create an identical copy of selected tiles in copied
	selected.cloneToTileList( &copied );
	lastCopy = LAST_COPY_MAPTILE;
}

void ME_PasteEntities( void ) {
	if ( 0 == entSelected.size() )
		return;

	const float shiftAmt = 1024.0f;

	poly_t delta;
	ME_InitPoly( &delta );

	entTmp.reset();
	entSelected.reset();
	
	Entity_t *e;
	bool firstPass = true;
	while ( (e = entCopy.pop()) ) {
		// get delta on first pass
		if ( firstPass ) {
			delta.x = delta.x - e->poly.x;
			delta.y = delta.y - e->poly.y;
			firstPass = false;
		}
		// move by the delta
		e->move( delta.x, delta.y );
		// make a full copy	
		Entity_t *dup = Entity_t::CopyEntity( e );
		// put it in the tmp list
		entTmp.add( dup );
		// add the moved ent to the global ents list
		me_ents->add( e );
		// and make it selected
		entSelected.add( me_ents->gettail() );
	}

	// now transfer the newly copied list back into the clipboard
	entCopy.transferFromList( entTmp );

	// this will move the ents over a unit away from what they were copied from
	ME_ReSnapEnts();
}

/*
====================
 ME_PasteFromClipboard
====================
*/
void ME_PasteFromClipboard( void ) {
	// if not tiles, try entities
	if ( lastCopy == LAST_COPY_ENTITY && entSelected.size() > 0 )
		return ME_PasteEntities();
	if ( !copied.size() )
		return;

	const float shiftAmt = 1024.0f;

	// check if all are off-screen
	bool allOffScreen = true;
	node_c<mapTile_t*> *node = copied.gethead();
	while ( node ) {
		poly_t tmp;
		tmp.set_xy(	node->val->poly.x + shiftAmt,
					node->val->poly.y + shiftAmt );
		if ( ME_PolyOnScreen( &tmp ) ) {
			allOffScreen = false;
			break;
		}
		node = node->next;
	}

	// init delta point to center of viewport
	poly_t delta;
	if ( allOffScreen ) {
		ME_InitPoly( &delta );
	}

	// clear the temporary copy of copied
	copyTmp.Destroy();

	// clear selected
	selected.reset();

	bool firstPass = true;
	
	mapTile_t *tile;

	// while there are nodes left in copied
	// detach a node from copied
	while ( (tile = copied.pop()) ) {

		// use first copied tile poly as a reference
		if ( allOffScreen && firstPass ) {
			delta.x = delta.x - tile->poly.x;
			delta.y = delta.y - tile->poly.y;
			firstPass = false;
		}

		// set its new location
		if ( allOffScreen ) {
			poly_t *p = &tile->poly;
			tile->poly.set_xy( delta.x + p->x, delta.y + p->y );
			if ( tile->col )
				tile->col->drag( delta.x, delta.y );
		} else {
			tile->poly.set_xy( 	tile->poly.x + shiftAmt, 
								tile->poly.y + shiftAmt );
			if ( tile->col )
				tile->col->drag( shiftAmt, shiftAmt );
		}

		// make a copy of the tile and put it in the temp set
		mapTile_t *dup = tile->Copy();
		copyTmp.add( dup );

		// put the tile from the clipboard in the master set and get a uid (the one it has is the 
		//  copied UID)
		tile->regenUID();
		tiles->add( tile );

		// the new node is now selected
		selected.add( tiles->gettail() );

		console.Printf( "pasted tile #%d @ (%.1f,%.1f)", tile->uid, tile->poly.x, tile->poly.y );
	}

	// move copyTmp to copied
	copyTmp.copyToList( &copied );

	// sync colModels
	selected.syncColModels();
}

/*
====================
 ME_DragTilesNoSnap
====================
*/
void ME_DragTilesNoSnap( int do_ent =0 ) {
	// ratio must be converting distance from mouse to world
	float tx = ((float)me_mouse.x-drag.mx0) * M_Ratio(); 
	float ty = ((float)me_mouse.y-drag.my0) * M_Ratio();

	drag.mx0 = (float)me_mouse.x;
	drag.my0 = (float)me_mouse.y;

	if ( do_ent ) {
		entSelected.drag( tx, ty );
	} else {
		selected.deltaMoveXY( tx, ty );
		selected.dragColModels( tx, ty );
		// set all selected to keepSnap = false;
		selected.snapOffAll();
	}
}

/*
====================
 ME_DragTiles
====================
*/
void ME_DragTiles( void ) {

	if ( !snapToGrid || !selected.MustSnap() ) {
		ME_DragTilesNoSnap();
		return;
	}

	//main_viewport_t *v = M_GetMainViewport();

	// subline resolution
	int res = 1 << ( 3 + subLineResolution );

	float m[2] = { (float)me_mouse.x, (float)me_mouse.y };
	float m0[2] = { drag.mx0, drag.my0 };
	M_ScreenToWorld( m );
	M_ScreenToWorld( m0 );

	// adjust m0 by x0,y0 steps already made
	m0[0] += drag.x0 * res;
	m0[1] += drag.y0 * res;

	float tx = 0.f; 
	float ty = 0.f;

	bool hua = false;

	if ( fabs(m[0] - m0[0]) >= res ) {
		if ( m0[0] > m[0] ) { // neg
			tx = (float)-res;
			drag.x0 = drag.x0 - 1.0f;
		} else {
			tx = (float)+res;
			drag.x0 = drag.x0 + 1.0f;
		}
		hua = true;
	} 

	if ( fabs(m[1] - m0[1]) >= res ) {
		if ( m0[1] > m[1] ) { // neg
			ty = (float)-res;
			drag.y0 = drag.y0 - 1.0f;
		} else {
			ty = (float)+res;
			drag.y0 = drag.y0 + 1.0f;
		}
		hua = true;
	}

	if ( !hua )
		return;
	
	selected.deltaMoveXY( tx, ty );
	selected.dragColModels( tx, ty );
}

/*
====================
 ME_DragEnts
====================
*/
void ME_DragEnts( void ) {
	ME_DragTilesNoSnap( 1 );
}

/*
====================
 ME_DragStuff
====================
*/
void ME_DragStuff( void ) {
	if ( selected.size() ) {
		ME_DragTiles();
	} else if ( entSelected.size() ) {
		ME_DragEnts();
	}
}

/*
====================
 ME_RotateSingleTile
====================
*/
void ME_RotateSingleTile( void ) {

	// get center of square, & convert from world to screen coordinates
	float c[2];

/* this is how I used to do it

	main_viewport_t *v = M_GetMainViewport();

	// units in world space
	c[0] = rotate.poly->x + rotate.poly->w / 2.f; // 
	c[1] = rotate.poly->y + rotate.poly->h / 2.f;

	// get vector from origin of viewport to point
	c[0] -= v->world.x;
	c[1] -= v->world.y;

	// scale into screen coords 
	c[0] *= v->iratio;
	c[1] *= v->iratio;
*/

	rotate.node->val->poly.getCenter( c );
	M_WorldToScreen( c );

	// 2 mouse clicks, current and initial
	float m[3] = { (float)me_mouse.x - c[0], (float)me_mouse.y - c[1], 0.f };
	float m_orig[3] = { rotate.mx0 - c[0], rotate.my0 - c[1], 0.f };

	// create vectors perpendicular (90) from m_orig, by doing cross product
	//  w/ z-axis.  
	float up[3], down[3], z[3] = { 0.f, 0.f, -1.f };
	M_CrossProd3d( up, m_orig, z );
	z[2] = 1.f;
	M_CrossProd3d( down, m_orig, z );

	unsigned char quad = 0;

	// determine which quadrant we're in (coord sys relative to center of block
	//  and first mouse click)
	if ( M_DotProduct2d( m, m_orig ) > 0.f ) {	// 1 || 4
		// 1
		if ( M_DotProduct2d( m, up ) > 0.f ) {
			quad = 1;
		} else if ( M_DotProduct2d( m, down ) > 0.f ) {
			quad = 4;
		}
	} else {									// 2 || 3
		if ( M_DotProduct2d( m, up ) > 0.f ) {
			quad = 2;
		} else if ( M_DotProduct2d( m, down ) > 0.f ) {
			quad = 3;
		}
	}
	if ( 0 == quad ) {
		return; // fuckit
	}

	// dot product for angle between 2 clicks
	float dot = M_DotProduct2d( m, m_orig );
	
	// get normals for scale
	float a = M_Magnitude2d( m );
	float b = M_Magnitude2d( m_orig );
	
	// actual rotation 
	float rot = (float) acosf( dot / (a * b) ) * RAD_TO_DEG;

	// it goes 0-180-0, modify the angle on the way down 
	if ( quad == 1 || quad == 2 ) {
		rot = 360.0f - rot;
	}

	rotate.node->val->poly.set_rot( rot + rotate.angle );

	// once rotated, snap gets turned off
	selected.snapOffAll();
}

void ME_RotateTileGroup( float *supplied_mouse ) {



	// use M_TranslateRotate2DVertSet() to rotate each MapTile around the centroid point 
	//  of the large poly by the angle created by 
	//  m and m_initial.  Also rotate the large poly around this same point.
	// then rotate each poly around it's own center also by the same angle 
	// have to do all this in worldspace instead

	// - get angle between m & m_init

	// - copy the original lower-left point from the original polys in copyTmp.
	//   M_TranslateRotate the point by the new angle storing the new point in the corresponding
	//   poly in selected (it'll have the same uid)

	// - store the new angle in each poly too

	// - store the new angle in the large bounding poly


	// centroid
	float centroid[2];
	centroid[0] = rotate.x0;
	centroid[1] = rotate.y0;
	
	float *m = NULL;
	if ( supplied_mouse ) {
		// user supplied mouse provides custom rotation
		m = supplied_mouse;
	} else {
		// current mouse and initial in world coordinate
		m = ME_GetWorldMouse();
	}
	float m_init[2] = { rotate.mx0 , rotate.my0 };
	M_ScreenToWorld( m_init );

	// make vectors w/ respect to the centroid
	m[0] -= centroid[0];
	m[1] -= centroid[1];
	m_init[0] -= centroid[0];
	m_init[1] -= centroid[1];


	// angle
	float angle = M_GetAngleDegrees( m, m_init );


	// have to adjust the angle depending on quadrant

	// create vectors perpendicular (90) from m_orig, by doing cross product
	//  w/ z-axis.  
	float up[3], down[3], z[3] = { 0.f, 0.f, -1.f };
	M_CrossProd3d( up, m_init, z );
	z[2] = 1.f;
	M_CrossProd3d( down, m_init, z );

	unsigned char quad = 0;

	// determine which quadrant we're in (coord sys relative to center of block
	//  and first mouse click)
	if ( M_DotProduct2d( m, m_init ) > 0.f ) {	// 1 || 4
		// 1
		if ( M_DotProduct2d( m, up ) > 0.f ) {
			quad = 1;
		} else if ( M_DotProduct2d( m, down ) > 0.f ) {
			quad = 4;
		}
	} else {									// 2 || 3
		if ( M_DotProduct2d( m, up ) > 0.f ) {
			quad = 2;
		} else if ( M_DotProduct2d( m, down ) > 0.f ) {
			quad = 3;
		}
	}
	if ( 0 == quad ) {
		return; 
	}

	// it goes 0-180-0, modify the angle on the way down 
	if ( quad == 1 || quad == 2 ) {
		angle = 360.0f - angle;
	}


	// for each original poly
	mapNode_t *node = copyTmp.gethead();

	// the uids should be in order, so just hang on to the current one
	//  but just in case, it goes through them once looking for a match
	node_c<mapNode_t*> *sel = selected.gethead();
	node_c<mapNode_t*> *head = sel;

	while ( node ) {
		float poly_center[2];
		node->val->poly.getCenter( poly_center );

		float w = node->val->poly.w;
		float h = node->val->poly.h;

		M_TranslateRotate2d( poly_center, centroid, angle );

		int uid = node->val->uid;
		float ang0 = node->val->poly.angle;

		node_c<mapNode_t*> *start = sel;
		while ( sel && sel->val->val->uid != uid ) {
			sel = sel->next;
			if ( !sel )
				sel = head;
			// break after once through the list
			if ( sel == start )
				break;
		}
		// found poly, rotate its center around group centroid and set its angle
		if ( sel && sel->val->val->uid == uid ) {
			sel->val->val->poly.x = poly_center[0] - 0.5f * w;
			sel->val->val->poly.y = poly_center[1] - 0.5f * h;
			sel->val->val->poly.angle = ang0 + angle;
		}

		node = node->next;
	}

	// set angle of group poly
	if ( rotate.poly ) {
		rotate.poly->angle = angle;
	}
}

/*
====================
 ME_RotateTiles
====================
*/
void ME_RotateTiles( void ) {
	if ( rotationType == ROT_ONE ) {
		ME_RotateSingleTile();
	}
	else if ( rotationType == ROT_NONE ) {
		inputMode = MODE_POLY_NORMAL;
		rotateMode = 0;
	} else {
		ME_RotateTileGroup( NULL );
	}

	selected.syncColModels();
}

/*
====================
 ME_RotateGroup90
====================
*/
void ME_RotateGroup90( unsigned int cc_turns ) {
	cc_turns &= 3;
	if ( !cc_turns || !selected.anyActive() )
		return;

	// get centroid of group
	float *c_world = ME_GetCentroid();
	rotate.x0 = c_world[0];
	rotate.y0 = c_world[1];

	// create bounding poly
	if ( !rotate.poly ) {
		rotate.poly = (poly_t*) V_Malloc( sizeof(poly_t) );
	}

	float v[8], m = 64.f;
	AABB_TO_OBB( v, &c_world[2] );
	v[0] -= m; v[1] -= m; v[2] += m; v[3] -= m; v[4] += m; v[5] += m; v[6] -= m; v[7] += m;
	VERT8_TO_POLY( rotate.poly, v );
	rotate.poly->angle = 0.f;
	rotate.angle = 0.f;

	// create copy of selected before rotation begins
	selected.cloneToTileList( &copyTmp );

	// have to convert back to screen
	float c[2] = { c_world[0], c_world[1] };

	// initial click in screen coord
	M_WorldToScreen( c );
	rotate.mx0 = c[0] + 10.0f; // up right
	rotate.my0 = c[1] + 10.0f;

	// secondary click in world
	c[0] = c_world[0];
	c[1] = c_world[1];
	float vec[2];
	if ( 1 == cc_turns ) {
		vec[0] = c[0] + 10.0f; // down right
		vec[1] = c[1] + -10.0f;
	} else if ( 2 == cc_turns ) {
		vec[0] = c[0] + -10.0f; // down left
		vec[1] = c[1] + -10.0f;
	} else {
		vec[0] = c[0] + -10.0f; // up left
		vec[1] = c[1] + 10.0f;
	}

	ME_RotateTileGroup( vec );
	selected.syncColModels();
}

/*
====================
 ME_ViewportDrag
====================
*/
void ME_ViewportDrag( void ) {

	main_viewport_t *v = M_GetMainViewport();

	float dx = ((float)me_mouse.x-vpDrag.mx0) * v->ratio;
	float dy = ((float)me_mouse.y-vpDrag.my0) * v->ratio;

	v->world.x = vpDrag.x0 - dx;
	v->world.y = vpDrag.y0 - dy;
}

/*
====================
 ME_DrawSelectWindow
====================
*/
void ME_DrawSelectWindow ( void ) {
	dragSelect.x0 = (float)me_mouse.x;
	dragSelect.y0 = (float)me_mouse.y;
}

/*
====================
 ME_ToggleColModel
====================
*/
void ME_ToggleColModel( void ) {
	node_c<mapNode_t *> *node = selected.gethead();
	while ( node ) {
		Assert( node->val != NULL );
		Assert( node->val->val != NULL );

		mapTile_t *tile = node->val->val;
		colModel_t *col = tile->col;

		if ( !tile->col ) {
			tile->col = new colModel_t;
			col = tile->col;
		}

		bool on = col->toggle( &tile->poly );

		console.Printf( "colModel of tile at (%.1f,%.1f) is %s", tile->poly.x, tile->poly.y, (on) ? "ON" : "OFF" );

		node = node->next;
	}
}

void ME_ToggleVisibility( void ) {
	if ( (visibility & ME_VIS_ALL) == ME_VIS_ALL ) {
		visibility = ME_VIS_COLMODEL;
		console.Printf( "visibility: showing only colModels" );
	} else if ( visibility & ME_VIS_COLMODEL ) {
		visibility = ME_VIS_MATERIAL;
		console.Printf( "visibility: showing only materials" );
	} else if ( visibility & ME_VIS_MATERIAL ) {
		visibility = ME_VIS_ALL;
		console.Printf( "visibility: showing all" );
	} 
}

void ME_ToggleTileSnap( void ) {

	node_c<mapNode_t*> *node = selected.gethead();

	if ( !node )
		return;

	buffer_c<char> kbuf;
	kbuf.init();

	while ( node ) {
		Assert( node->val != NULL );
		Assert( node->val->val != NULL );

		node->val->val->snap = !node->val->val->snap;

		char buf[100];
		snprintf( buf, 100, "uid_%d => %s, ", node->val->val->uid, node->val->val->snap ? "snap" : "nosnap" );	
		int len = strlen( buf );
		kbuf.copy_in( buf, len );

		node = node->next;
	}

	int k = kbuf.length();
	kbuf.data[ --k ] = '\0';
	kbuf.data[ --k ] = '\0';

	console.Printf( "Snap Toggled: %s", kbuf.data );

	kbuf.destroy();
}

/*
====================
 ME_ExecuteCommands
====================
*/
void ME_ExecuteCommands( void ) {

	// special input modes that occur between the moment of a mouse-click
	//  and the release 

	// FIXME: should they necessarily block all other input from happening or
	//  does it even matter?  I guess if you want to be able to key things in
	//  while holding a mouse button down you should be mindful of this behavior
	
	////////////////////////////////////////////////////////////////////////
	//  SPECIAL SELECT  (Ctrl + Click and Drag)
	////////////////////////////////////////////////////////////////////////
	if ( inputMode == MODE_SPECIAL_SELECT ) {
		ME_DrawSelectWindow();
		return;
	}

	////////////////////////////////////////////////////////////////////////
	//  ROTATE
	////////////////////////////////////////////////////////////////////////
	if ( inputMode == MODE_POLY_ROTATE ) {
		if ( selected.hasLock() )
			return;
		ME_RotateTiles();
		return;
	}

	////////////////////////////////////////////////////////////////////////
	//	DRAG
	////////////////////////////////////////////////////////////////////////
	if ( inputMode == MODE_POLY_DRAG ) {
		if ( selected.hasLock() )
			return;
		ME_DragStuff();
		return;
	}

	////////////////////////////////////////////////////////////////////////
	//	STRETCH
	////////////////////////////////////////////////////////////////////////
	if ( inputMode == MODE_POLY_STRETCH ) {
		if ( selected.hasLock() )
			return;
		ME_StretchMapTile ();
		return;
	}

	////////////////////////////////////////////////////////////////////////
	//  VIEWPORT DRAG
	////////////////////////////////////////////////////////////////////////
	if ( inputMode == MODE_VIEWPORT_DRAG ) {
		ME_ViewportDrag();
		return;
	}

	////////////////////////////////////////////////////////////////////////
	//  DRAG SELECT
	////////////////////////////////////////////////////////////////////////
	if ( inputMode == MODE_DRAG_SELECT ) {
		ME_DrawSelectWindow();
		return;
	}

	
	float x_dir = +1.f, y_dir = +1.f;

	main_viewport_t *v = NULL;

	float r0, dr, RES[2];

	mapTile_t *tmp_tile;

	int count = 0;
	material_t *mat;
	static int client_id = 0;
	if ( !client_id ) 
		client_id = floating.requestClientID();

    while ( cmdque.size() > 0 ) {
        mapeditCmd_t cmd = cmdque.pop();
        switch( cmd ) {
        case MEC_DESELECT_ALL:
			selected.reset();
			entSelected.reset();
			rotateMode = 0;
            break;
        case MEC_NEWPOLY:

			// make a new one
			tmp_tile = tiles->newMaterialTile();

			// give it default location
			ME_InitPoly( &tmp_tile->poly );
			// enter it in the global group
			tiles->add( tmp_tile );

			// make newly added, the sole-selected
			selected.reset(); 
			selected.add( tiles->gettail() );
			entSelected.reset();

			console.Printf( "poly #%d created @ (%.1f,%.1f)", tmp_tile->uid, tmp_tile->poly.x, tmp_tile->poly.y );

            break;
		case MEC_RESNAP:
			if ( selected.hasLock() )
				break;
			ME_ReSnapSelected();
			break;
		case MEC_CYCLE:

			//
			// only able to cycle if there are NONE or ONE selected
			//

			// 1 ent OR none at all selected: cycle ents first, if there are any
			if ( 1 == entSelected.size() || ( !selected.anyActive() && entSelected.size() == 0 && me_ents->size() ) ) {
				entNode_t * e = me_ents->gethead();
				if ( e ) {
					if ( entSelected.size() == 1 ) {
						if ( me_ents->size() > 1 ) {
							entNode_t * was = entSelected.pop();
							entNode_t * found = me_ents->findNode( was->val );
							if ( found && found->next ) {
								entSelected.add( found->next );
							} else {
								entSelected.add( me_ents->gethead() );
							}
						}
					} else {
						entSelected.add( e );
					}
				}
			} else if ( !selected.anyActive() ) {
				node_c<mapTile_t*> *n = tiles->gethead();
				if ( n ) {
					selected.setJustOne( n );
				}
			} else if ( 1 == selected.size() && tiles->size() > 1 ) {
			
				node_c<node_c<mapTile_t *> *> *sn = selected.gethead();
				node_c<mapTile_t*> *n = sn->val;

				// at the end of selected, start over at the beginning
				if ( !n || !n->next ) {
					node_c<mapTile_t*> *_tile = tiles->gethead();
					if ( _tile ) {
						selected.setJustOne( _tile );
					}
				} else {
					selected.setJustOne( n->next );
				}
			}
			rotateMode = 0;
			break;
		case MEC_ZERO_ACTIVE_ANGLE:
			if ( selected.hasLock() )
				break;
			{
			// only do zeroing when just one poly selected
			node_c<mapNode_t *> *node = selected.gethead();
			int count = 0;
			while( node ) {
				if ( node && node->val && node->val->val ) {
					node->val->val->poly.angle = 0.0f;
					console.Printf( "zeroed angle of poly #%d", node->val->val->uid );
					++count;
				}
				node = node->next;
			}
			if ( selected.gethead() )
				console.Printf( "zeroed %d polys", count );
			else
				console.Printf( "no polys selected" );
			}
			break;
		case MEC_DELETE_ACTIVE:
			if ( selected.hasLock() )
				break;
			if ( selected.anyActive() ) {
				//
				// unset pointers that may be pointing to the defunct poly
				//
				if ( selected.checkNode( highlight.node ) ) {
					highlight.node = NULL;
				}
				if ( selected.checkNode( stretch.node ) ) {
					stretch.node = NULL;
				}

				count = 0;

				node_c<mapNode_t *> *node = selected.gethead();
				while ( node ) {
					tiles->deleted.add( node->val->val );
					mapTile_t *t = tiles->popnode( node->val );
					console.Printf( "deleted tile #%d @ (%.1f,%.1f)", t->uid, t->poly.x, t->poly.y );
					++count;
					node = node->next;
				}

				selected.reset();
				
				// put some metadata in tiles in case we want to undo the delete
				tiles->storeDeletedCount( count );
			} else if ( entSelected.size() > 0 ) {
				while ( entSelected.size() != 0 ) {
					Entity_t * ent = me_ents->popnode( entSelected.pop() );
					delete ent;
				}
			}

			break;
		case MEC_COPY:
			ME_CopyToClipboard();

			break;
		case MEC_PASTE:
			ME_PasteFromClipboard();

			break;
		case MEC_PREVIOUS_TEXTURE:
			if ( selected.hasLock() )
				break;

			mat = materials.PrevMaterial();
			selected.setTexture( mat, materials.current+1, materials.total );
			entSelected.setTexture( mat, materials.current+1, materials.total);
			break;
		case MEC_ADVANCE_TEXTURE:
			if ( selected.hasLock() )
				break;

			// set the texture on all the selected to the current Texture
			mat = materials.NextMaterial();
			selected.setTexture( mat, materials.current+1, materials.total );
			entSelected.setTexture( mat, materials.current+1, materials.total);
			break;
		case MEC_TEXTURE:
			// FIXME: note, its just supposed to set all selected, but for now leave it
			break;
		case MEC_ZOOM_IN:
			v = M_GetMainViewport();
			r0 = v->ratio;

			v->ratio /= 1.05f;
			v->iratio = 1.f / v->ratio;	// always 1/ratio

			dr = r0 - v->ratio;
			dr *= 0.5;
			RES[0] = v->res->width * dr;
			RES[1] = v->res->height * dr;

			v->world.x += RES[0];
			v->world.y += RES[1];

			v->unit_width = v->res->width * v->ratio;
			v->unit_height = v->res->height * v->ratio;

			break;
		case MEC_ZOOM_OUT:
			v = M_GetMainViewport();
			r0 = v->ratio;

			v->ratio *= 1.05f;
			v->iratio = 1.f / v->ratio;

			dr = r0 - v->ratio;
			dr *= 0.5;
			RES[0] = v->res->width * dr;
			RES[1] = v->res->height * dr;

			v->world.x += RES[0];
			v->world.y += RES[1];

			v->unit_width = v->res->width * v->ratio;
			v->unit_height = v->res->height * v->ratio;

			break;
		case MEC_TOGGLE_GRID:
			ME_ToggleGrid();
			break;

		case MEC_CCWISE:
			if ( selected.hasLock() )
				break;
			ME_RotateNinetyCC();
			break;
		case MEC_CWISE:
			if ( selected.hasLock() )
				break;
			ME_RotateNinety();
			break;
		case MEC_ADD_REMOVE_COLMODEL:
			if ( selected.hasLock() )
				break;
			ME_ToggleColModel();
			break;
		case MEC_VISIBILITY:
			ME_ToggleVisibility();
			break;
		case MEC_TOGGLE_TILE_SNAP:
			if ( selected.hasLock() )
				break;
			ME_ToggleTileSnap();
			break;
		case MEC_PREV_25_TEXTURE:
			materials.current -= 25; 
			if ( materials.current < 0 )
				materials.current += materials.total;
			mat = materials.mat.data[ materials.current ];
			selected.setTexture( mat, materials.current+1, materials.total );
			break;
		case MEC_NEXT_25_TEXTURE:
			materials.current = (materials.current + 25) % materials.total; 
			mat = materials.mat.data[ materials.current ];
			selected.setTexture( mat, materials.current+1, materials.total );
			break;
		}
    }
}



/*
====================
 ME_StartStretchMode
====================
*/
void ME_StartStretchMode( void ) {

	if ( !selected.anyActive() ) 
		return;

	stretch.mode = ( selected.size() > 1 ) ? STRETCH_MANY : STRETCH_ONE ;

	float *c = ME_GetWorldCentroid(); // centroid also provides the x&y minima & maxima of the set
	float *m = ME_GetWorldMouse();

	// save the mouse in world-coord
	stretch.mx0 = m[0];
	stretch.my0 = m[1];

	// save the centroid in world-coord
	stretch.x0 = c[0];
	stretch.y0 = c[1];

	stretch.xofst = 0;
	stretch.yofst = 0;

	stretch.c[0] = c[0];
	stretch.c[1] = c[1];

	inputMode = MODE_POLY_STRETCH;
}


/*
====================
====================
*/
void ME_StartDragState( node_c<mapTile_t*> *n, entNode_t *nE ) {

	// it has to be selected in order for you to drag it
	if ( !selected.checkNode( n ) && !entSelected.checkNode( nE ) ) 
		return;

	// all you need is to store, initial mouseClick coord and compare against release Coords
	drag.mx0 = (float)me_mouse.x;
	drag.my0 = (float)me_mouse.y;	

	drag.x0 = drag.y0 = 0; // use these for counters how many grid units we've crossed

	inputMode = MODE_POLY_DRAG;
}

/*
====================
ME_StartRotate
====================
*/
void ME_StartRotate( void ) {
	//
	// save initial mouse  
	//

	// mouse
	rotate.mx0 = (float)me_mouse.x;
	rotate.my0 = (float)me_mouse.y;

	inputMode = MODE_POLY_ROTATE;
}

// called right when rotate is toggled
void ME_SetupRotationType( void ) {

	if ( rotationType == ROT_ONE ) {
		node_c<mapNode_t*> *n = selected.gethead();
		rotate.angle = n->val->val->poly.angle;
		rotate.node = n->val;
	}
	else if ( rotationType == ROT_MANY ) {
		// get original centroid,
		float *c = ME_GetCentroid();
		rotate.x0 = c[0];
		rotate.y0 = c[1];

		// create bounding poly
		if ( !rotate.poly ) {
			rotate.poly = (poly_t*) V_Malloc( sizeof(poly_t) );
		}

		float v[8], m = 64.f;
		AABB_TO_OBB( v, &c[2] );
		v[0] -= m; v[1] -= m; v[2] += m; v[3] -= m; v[4] += m; v[5] += m; v[6] -= m; v[7] += m;
		VERT8_TO_POLY( rotate.poly, v );
		rotate.poly->angle = 0.f;
		rotate.angle = 0.f;

		// create copy of selected before rotation begins
		selected.cloneToTileList( &copyTmp );
	}
}

/*
====================
ME_StartDragSelect
====================
*/
void ME_StartDragSelect( void ) {
	dragSelect.mx0 = (float)me_mouse.x;
	dragSelect.my0 = (float)me_mouse.y;
	inputMode = MODE_DRAG_SELECT;
}

/*
====================
 ME_DragSelectRelease
====================
*/
void ME_DragSelectRelease( void ) {
// create a poly & AABB for the dragged selection window
// need a macro to get AABB for a poly
// compute the AABB intersections between the window and each poly
//  if any are found, further check each vertices via the ME_MousePolyComparison
//  if any, then add the box to selected IFF it is not already a member of 
//  selected

	// for now, only works for mapTiles, when no entities are active
	if ( entSelected.size() > 0 )
		return;

	float m[2], m0[2];
	poly_t win;

	m0[0] = dragSelect.mx0;
	m0[1] = dragSelect.my0;
	m[0] = dragSelect.x0;
	m[1] = dragSelect.y0;
	M_ScreenToWorld( m0 );
	M_ScreenToWorld( m );
	
	OBB_t win_obb;
	AABB_t tmp, win_aabb;

	tmp[0] = m0[0];
	tmp[1] = m0[1];
	tmp[2] = m[0];
	tmp[3] = m[1];

	// convert 2 unsorted vertices
	POINTS_TO_AABB( win_aabb, tmp ); 
	// now sorted
	AABB_TO_POLY( &win, win_aabb );
	
	win.toOBB( win_obb );

	AABB_t test_aabb;
	mapNode_t *tile = tiles->gethead();
	while ( tile ) {
		tile->val->poly.toAABB( test_aabb );
		if ( AABB_intersection( win_aabb, test_aabb ) ) {
			if ( tile->val->poly.angle == 0.f ) { 
				selected.addIfNot( tile );
			} else {
				OBB_t test_obb;
				tile->val->poly.toOBB( test_obb );
				if ( OBB_intersection( test_obb, win_obb ) ) {
					selected.addIfNot( tile );
				}
			}
		}
		tile = tile->next;
	}
}

/*
====================
 ME_SpecialSelectRelease
====================
*/
void ME_SpecialSelectRelease( void ) {
	ME_DragSelectRelease();
}

/*
====================
====================
*/
int ME_DoViewPortMove( void ) {
	static int tic = 0;

	if ( tic == ME_Tics() )
		return 0;

	tic = ME_Tics();

	main_viewport_t *view = M_GetMainViewport();
	if ( input & ME_IN_VIEW_LEFT ) {
		view->world.x -= VIEWPORT_MOVE;
	} else if ( input & ME_IN_VIEW_RIGHT ) {
		view->world.x += VIEWPORT_MOVE;
	}
	if ( input & ME_IN_VIEW_UP ) {
		view->world.y += VIEWPORT_MOVE;
	} else if ( input & ME_IN_VIEW_DOWN ) {
		view->world.y -= VIEWPORT_MOVE;
	}
	return 1;
}

/*
====================
 ME_StartViewportDrag
====================
*/
void ME_StartViewportDrag( void ) {
	vpDrag.mx0 = (float)me_mouse.x;
	vpDrag.my0 = (float)me_mouse.y;
	main_viewport_t *v = M_GetMainViewport();
	vpDrag.x0 = v->world.x;
	vpDrag.y0 = v->world.y;
	inputMode = MODE_VIEWPORT_DRAG;
}

/*
====================
 ME_StartSpecialSelect
====================
*/
void ME_StartSpecialSelect( void ) {
	dragSelect.mx0 = (float)me_mouse.x;
	dragSelect.my0 = (float)me_mouse.y;
	inputMode = MODE_SPECIAL_SELECT;
}

/*
====================
 ME_MakeCommand

 called on a tic
====================
*/
void ME_MakeCommand( void ) {

	node_c<mapTile_t*> *	anySelectedTilesHit = NULL;
	node_c<Entity_t*> * 	anySelectedEntsHit = NULL;

	// escape to menu
	if ( input & ME_IN_DESELECT_ALL ) {
		cmdque.add( MEC_DESELECT_ALL );
		input &= ~ME_IN_DESELECT_ALL;
	}

	// new poly
	if ( input & ME_IN_NEW_POLY ) {
		cmdque.add( MEC_NEWPOLY );
		input &= ~ME_IN_NEW_POLY;
	}

	// cycle through selected
	if ( input & ME_IN_CYCLE_POLYS ) {
		cmdque.add( MEC_CYCLE );
		input &= ~ME_IN_CYCLE_POLYS;
	}

	if ( input & ME_IN_ZERO_ANGLE ) {
		cmdque.add( MEC_ZERO_ACTIVE_ANGLE );
		input &= ~ME_IN_ZERO_ANGLE;
	}

	if ( input & ME_IN_DELETE ) {
		cmdque.add( MEC_DELETE_ACTIVE );
		input &= ~ME_IN_DELETE;
	}

	if ( input & ME_IN_COPY ) {
		cmdque.add( MEC_COPY );
		input &= ~ME_IN_COPY;
	}

	if ( input & ME_IN_PASTE ) {
		cmdque.add( MEC_PASTE );
		input &= ~ME_IN_PASTE;
	}

	if ( input & ME_IN_TEXTURE ) {
		cmdque.add( MEC_TEXTURE );
		input &= ~ME_IN_TEXTURE;
	}
	if ( input & ME_IN_NEXT_TEXTURE ) {
		cmdque.add( MEC_ADVANCE_TEXTURE );
		input &= ~ME_IN_NEXT_TEXTURE;
	}
	if ( input & ME_IN_PREV_TEXTURE ) {
		cmdque.add( MEC_PREVIOUS_TEXTURE );
		input &= ~ME_IN_PREV_TEXTURE;
	}

	if ( input & (ME_IN_ZOOM_IN| ME_IN_ZOOM_OUT) ) {
		uint x = ME_IN_ZOOM_IN|ME_IN_ZOOM_OUT;
		if ( (input & x) != x ) {
			if ( input & ME_IN_ZOOM_IN ) {
				cmdque.add( MEC_ZOOM_IN );
			} else if ( input & ME_IN_ZOOM_OUT ) {
				cmdque.add( MEC_ZOOM_OUT );
			}
		}
		input &= ~(ME_IN_ZOOM_IN|ME_IN_ZOOM_OUT) ;
	}

	if ( input & ME_IN_TOGGLE_GRID ) {
		cmdque.add( MEC_TOGGLE_GRID );
		input &= ~ME_IN_TOGGLE_GRID;
	}

	if ( input & ME_IN_TOGGLE_ROTATE_MODE ) {
		rotateMode ^= 1;
		input &= ~ME_IN_TOGGLE_ROTATE_MODE;
		char *type = NULL;
		if ( !selected.anyActive() ) {
			rotateMode = 0;
			rotationType = ROT_NONE;
		} 
		else if ( selected.size() > 1 ) {
			rotationType = ROT_MANY;
			type = "group mode";
		} else if ( selected.size() == 1 ) {
			rotationType = ROT_ONE;
			type = "single mapTile mode";
		}

		// also setup rotate.poly here
		if ( rotationType ) {
			ME_SetupRotationType();
		}

		console.Printf( "[%s] rotation is %s", type, ( rotateMode ) ? "ON" : "OFF" );
	}

	if ( input & ME_IN_SELECT_ALL ) {
		selected.selectAll( tiles );
		input &= ~ME_IN_SELECT_ALL;
	}

	if ( input & ME_IN_RESNAP ) {
		cmdque.add ( MEC_RESNAP ) ;
		input &= ~ME_IN_RESNAP;
	}

	if ( input & ME_IN_CCWISE ) {
		cmdque.add( MEC_CCWISE );
		input &= ~ME_IN_CCWISE;
	}

	if ( input & ME_IN_CWISE ) {
		cmdque.add( MEC_CWISE );
		input &= ~ME_IN_CWISE;
	}

	if ( input & ME_IN_ADD_REMOVE_COLMODEL ) {
		cmdque.add( MEC_ADD_REMOVE_COLMODEL );
		input &= ~ME_IN_ADD_REMOVE_COLMODEL;
	}

	if ( input & ME_IN_VISIBILITY ) {
		cmdque.add( MEC_VISIBILITY );
		input &= ~ME_IN_VISIBILITY;
	}

	if ( input & ME_IN_TOGGLE_TILE_SNAP ) {
		cmdque.add( MEC_TOGGLE_TILE_SNAP );
		input &= ~ME_IN_TOGGLE_TILE_SNAP;
	}

	if ( input & ME_IN_PREV_25_TEXTURE ) {
		cmdque.add( MEC_PREV_25_TEXTURE );
		input &= ~ME_IN_PREV_25_TEXTURE;
	}

	if ( input & ME_IN_NEXT_25_TEXTURE ) {
		cmdque.add( MEC_NEXT_25_TEXTURE );
		input &= ~ME_IN_NEXT_25_TEXTURE;
	}

	//============================================================================ 
	//============================================================================ 
	//============================================================================ 

	// setup mouse-over highlights
	//	ME_SetMouseOverHighlights( (float)me_mouse.x, (float)me_mouse.y );
	//	ME_HoverHighlight();

	// lmb release
	if ( me_mouse.lmb_down == 0 && (inputMode == MODE_POLY_DRAG || 
									inputMode == MODE_POLY_STRETCH ||
									inputMode == MODE_POLY_ROTATE) ) {
		inputMode = MODE_POLY_NORMAL;
		stretch.node = NULL;
	}
	else if ( me_mouse.lmb_down == 0 && inputMode == MODE_DRAG_SELECT ) {
		ME_DragSelectRelease();
		inputMode = MODE_POLY_NORMAL;
		return;
	}
	else if ( me_mouse.lmb_down == 0 && inputMode == MODE_SPECIAL_SELECT ) {
		ME_SpecialSelectRelease();
		inputMode = MODE_POLY_NORMAL;
		return;
	}

	// rmb release
	if ( me_mouse.rmb_down == 0 && inputMode == MODE_VIEWPORT_DRAG ) { 
		inputMode = MODE_POLY_NORMAL;
	}


	//============================================================================ 
	//============================================================================ 
	//============================================================================ 

	//
	// left-click
	//

	// special select 
	if ( me_mouse.lmb_click & 0x80 && ctrlKeyModifier ) {
		ME_StartSpecialSelect();
		me_mouse.lmb_click = 0;
	// other left-click functions
	} else if ( me_mouse.lmb_click & 0x80 ) {

		// alters active set
		ME_SetActive( 0 );

		anySelectedTilesHit = NULL;
		anySelectedTilesHit = ME_ComputeSelectedMouseHit( (float)me_mouse.x, (float)me_mouse.y );
		anySelectedEntsHit = ME_SelectedEntMouseHit( (float)me_mouse.x, (float)me_mouse.y );

		// rotateMode enabled, start a mapTile rotation
		if ( rotateMode && ( inputMode == MODE_POLY_NORMAL ) ) {
			ME_StartRotate();
			me_mouse.lmb_click = 0; // consume the click
		}

		// clicked within a selected tile
		else if ( anySelectedTilesHit || anySelectedEntsHit ) {
			
			// start dragging
			if ( inputMode == MODE_POLY_NORMAL ) {
				ME_StartDragState( anySelectedTilesHit, anySelectedEntsHit );
				me_mouse.lmb_click = 0; // consume the click
			}

		// clicked outside of all selected tiles, if any
		} else {

			// stretch mode
			if ( selected.anyActive() ) {
				if ( inputMode == MODE_POLY_NORMAL ) {
					ME_StartStretchMode();
					me_mouse.lmb_click = 0; // consume the click
				}

			// None Active defaults to windows-style box drag select
			} else if ( !entSelected.size() ) {
				ME_StartDragSelect();
				me_mouse.lmb_click = 0;
			}
		}

	} // left mouse click

	//
    // right-click
	//
	if ( me_mouse.rmb_click & 0x80 ) {

		//ME_SetActive( 1 );

		// viewport drag
		if ( inputMode == MODE_POLY_NORMAL ) {
			ME_StartViewportDrag();
			me_mouse.rmb_click = 0;
		}

	}  // right mouse click
	

	// keystroke viewport move : may be discontinued at some point ?
	if ( input & ME_IN_VIEW_MOVE ) {
		if ( ME_DoViewPortMove() )
			input &= ~ME_IN_VIEW_MOVE;
	}

	// mouse wheel
	if ( me_mouse.z != 0 ) {
		if ( me_mouse.z > 0 ) {
			while ( me_mouse.z > 0 ) {
				cmdque.add( MEC_ZOOM_IN );
				me_mouse.z -= 120;
			}
		} else if ( me_mouse.z < 0 ) {
			while ( me_mouse.z < 0 ) {
				cmdque.add( MEC_ZOOM_OUT );
				me_mouse.z += 120;
			}
		}
	}

	// clicked in an empty area, consume the clicks after everybody else's had 
	//  a shot at them
	me_mouse.lmb_click = 0;
	me_mouse.rmb_click = 0;	
	//me_mouse.z = 0;
}

void ME_IncrementLayer( int inc ) {

	static int client_id = 0;
	if ( !client_id ) {
		client_id = floating.requestClientID();
	}

	// FIXME: this isn't quite right.  because other client ids may be lingering on other tiles which are
	//  now un-selected.  we, ideally, don't want these cleared.  ahh, who cares for now
	//floating.ClearClientID( client_id );

	char buf[100];
	node_c<mapNode_t*> *node = selected.gethead();
	while ( node ) {
		Assert( node->val != NULL );
		Assert( node->val->val != NULL );
		node->val->val->layer += inc;
		mapTile_t *t = node->val->val;
		console.Printf( "layer on mapTile #%d changed to %d", t->uid,t->layer );

		// put number on tile
		floating.ClearUIDAndClientID( t->uid, client_id );
		snprintf( buf, sizeof(buf), "%d", node->val->val->layer );
		floating.AddTileText( node->val->val, buf, 60.f, 180.f, 20, 1, client_id );

		node = node->next;
	}
}

void ME_ToggleLockSelected( void ) {
	node_c<mapNode_t*> *node = selected.gethead();
	if ( !node )
		return;

	static int client_id = 0;
	if ( !client_id ) {
		client_id = floating.requestClientID();
	}

	// FIXME: again, a tile should keep it's lock label, when it's NOT the selected one that was
	//  toggled.  I'll be changing this
	//floating.ClearClientID( client_id );

	bool newLockState = ! selected.hasLock();

	while ( node ) {
		Assert( node->val != NULL );
		Assert( node->val->val != NULL );

		node->val->val->lock = newLockState;

		float h = node->val->val->poly.h;

		floating.ClearUIDAndClientID( node->val->val->uid, client_id );

		// label tile, lock labels are permanent
		if ( newLockState )  
			floating.AddTileText( node->val->val, "locked", 5.f, h-24.f, 12, 0, client_id );
		else
			floating.AddTileText( node->val->val, "un-locked", 5.f, h-24.f, 12, 1, client_id );

		node = node->next;
	}

	console.Printf( "all tiles: %s", newLockState ? "locked" : "unlocked" );
}

int ME_ConPrintLocks( void ) {
	node_c<mapNode_t*> *node = selected.gethead();
	if ( !node )
		return 0;

	buffer_c<char> buf;
	buf.init();

	while ( node ) {
		Assert( node->val != NULL );
		Assert( node->val->val != NULL );

		char one[100];

		snprintf( one, 100, "uid_%d => %s, ", node->val->val->uid, (node->val->val->lock) ? "locked" : "unlocked" );
		buf.copy_in( one, strlen(one) );
		node = node->next;
	}

	int k = buf.length();
	buf.data[ --k ] = '\0';
	buf.data[ --k ] = '\0';

	console.Printf( "%s", buf.data );
	buf.destroy();

	return 1;
}

void ME_StartAreaSelect( void ) {
/* 

1 - dim whole area by blitting one tinted tile as large foreground
		console.Printf( "Area Select Mode started" );
2 - ESC key exits Area_SELECTMODE
		- console.Printf( "Area Select Mode Voided" );
3 - click twice, once to start, once to finalize area

press key to start mode

click once for first corner
second time for second corner
box appears afterwards, dashed line, everything is still dimmed

how does area selection work?  

press Key to start mode, a tint tile, sort of black-ish green, is displayed
	over every tile, a floating title is permanently displayed in top-center
	of window.
ESC exits Area mode.
Clicking in open Space, drops a Marker, 
Pressing Escape after the first click removes the marker, but remain in
	AREA_SELECT_MODE
Clicking once, then Clicking twice, leaves a dashed-line square around the
	mode.  
Pressing Escape erases the whole square and no Area is created.  
floating.TextStatic( "Press Return to Create Area Box, ESC to cancel" );
Press Return creates the Area.

Post a 5sec floating message, "Area Created"

Areas are invisible unless we are in Area Mode, in which all Previously 
created areas are visible.  

Clicking within a selected Area, Selects It.

Once an Area is selected, Clicking in it, and hold down the mouse? for
3 seconds, and then the first point is created.  

Click a second time, and the subArea is created, 

++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
OK, Here is an Alternative UI strategy that may be easier:

Select a group of Tiles, type

'create_area'

And an area is created.  and All the selected tiles are copied to it, with 
empty Materials.  A floating message is posted.

Key 'i' toggles Area Mode, where the 'm' cycles through Areas, their names
displayed in the corner of the screen.

'area_info' - (prints a list of all areas and their info)

'set_name <name>' - sets the name of a selected area, if any

You are not allowed to MOVE tiles.  or stretch.  The only thing you are allowed
to do in an Area is create a subArea.  so You can drag select, un-select, 
all the selection modes.  Instead, pressing 'i' again, LEAVES AREA_MODE

say you:
	- press 'i' and enter Area Mode, Area boundaries are highlighted, 	
		stats printed in the left column down the screen, of each area
	- press 'm', the first area is highlighted, the others become invisible
		there is a border around it.  The messages that were in the left 
		column are replaced with the info of just the selected area.
	- you select 4 tiles in the selected Area, then type:

'create_sub_area'

and a sub Area is created.  Message is posted.

KEY_n, cycles through subAreas, within the selected Area.


In fact, I think I'll dispense with all the UI eye-candy and just do the
4 console commands.  You create a sub_area, then oh-fucking well.  I can 
print the info of it easy enough, but I'm not going to fuck with any display
info.  TOo much time.  I can get these 4 commands done right now though.

Later on, if this thing needs to be spruced up for public consumption, then
I will do UI touch-up.


'create_area <name>'
'area_info'
'set_name <name>' // if 1 or more tiles selected, see which area they belong to 
'create_sub_area' 

*/
}

/*
====================
 ME_KeyEvent

	called by keyboard input module
====================
*/
void ME_KeyEvent( int key, int down, unsigned int time, int anykeydown ) {
	// give the menu a crack at it
	if ( menu.keyboard( key, down ) )
		return;

	if ( down ) {
		switch( key ) {

		// NEW MAPTILE
		case KEY_SPACEBAR:
			input |= ME_IN_NEW_POLY;
			break;

		// DE-SELECT ALL
		case KEY_ESCAPE:
			input |= ME_IN_DESELECT_ALL;
			break;

		// MOVE VIEWPORT
		case KEY_LEFT:
			input |= ME_IN_VIEW_LEFT;
			break;
		case KEY_RIGHT:
			input |= ME_IN_VIEW_RIGHT;
			break;
		case KEY_UP:
			input |= ME_IN_VIEW_UP;
			break;
		case KEY_DOWN:
			input |= ME_IN_VIEW_DOWN;
			break;

		// AREA & SUB-AREA SELECT
		case KEY_i:
			ME_StartAreaSelect();
			break;

		case KEY_s:
			// SAVE
			if ( ctrlKeyModifier ) {
				ME_SpawnSaveDialog();
			// TOGGLE SNAP
			} else if ( shiftKeyModifier ) {
				input |= ME_IN_TOGGLE_TILE_SNAP;
			// RE-SNAP
			} else {
				input |= ME_IN_RESNAP;
			}
			break;

		case KEY_c:
			// COPY
			if ( ctrlKeyModifier ) {
				input |= ME_IN_COPY;

			// TOGGLE COL-MODEL
			} else {
				input |= ME_IN_ADD_REMOVE_COLMODEL;
			}
			break;

		// ZERO ANGLE
		case KEY_b:
			input |= ME_IN_ZERO_ANGLE;
			break;

		// DELETE
		case KEY_BACKSPACE:
		case KEY_DELETE:
			input |= ME_IN_DELETE;
			break;

		// NEXT TEXTURE
		case KEY_t:
			if ( shiftKeyModifier )
				input |= ME_IN_PREV_TEXTURE;
			else
				input |= ME_IN_NEXT_TEXTURE;
			break;
		// LARGER TEXTURE JUMPS
		case KEY_g:
			if ( shiftKeyModifier ) {
				input |= ME_IN_PREV_25_TEXTURE;
			} else if ( ctrlKeyModifier ) {
				materials.current = materials.total - 1;
				selected.setTexture( materials.mat.data[ materials.current ], materials.current+1, materials.total );
			} else {
				input |= ME_IN_NEXT_25_TEXTURE;
			}
			break;

		// ZOOM
		case KEY_z:
			input |= ME_IN_ZOOM_IN;
			break;
		case KEY_x:
			input |= ME_IN_ZOOM_OUT;
			break;

		// TOGGLE GRID
		case KEY_y:
			input |= ME_IN_TOGGLE_GRID;
			break;

		// GRID
		case KEY_EQUAL:
			ME_IncSubline(); 
			break;
		case KEY_MINUS:
			ME_DecSubline(); 
			break;
		
		// L
		case KEY_l:
			// TOGGLE LOCK
			if ( ctrlKeyModifier ) {
				ME_ToggleLockSelected();
				break;
			}
			if ( selected.hasLock() )
				break;
			if ( shiftKeyModifier ) {
				// DECREMENT LAYER
				ME_IncrementLayer( -1 );
			} else {
				// INCREMENT LAYER
				ME_IncrementLayer( +1 );
			}
			tiles->needsort = 1;
			break;

		// 

		// CYCLE SELECT
		case KEY_m:
			input |= ME_IN_CYCLE_POLYS;
			break;

		// NEXT/PREV PALETTE
		case KEY_p:
			if ( shiftKeyModifier ) {
				palette.Prev();
			} else {
				palette.Next();
			}
			break;

		// ROTATE MODE
		case KEY_r:
			input |= ME_IN_TOGGLE_ROTATE_MODE;
			break;

		// SELECT ALL
		case KEY_a:
			if ( ctrlKeyModifier ) {
				input |= ME_IN_SELECT_ALL;
			}
			break;

		// VIS 
		case KEY_v:
			if ( ctrlKeyModifier ) {
				// PASTE
				input |= ME_IN_PASTE;
			} else {
				// TOGGLE VISIBILITY
				input |= ME_IN_VISIBILITY;
			}
			break;

		// QUIT
		case KEY_q:
			if ( ctrlKeyModifier ) {
				quit_f ();
			}
			break;

		// GRID HOT KEYS
		case KEY_1: grid_1(); break;
		case KEY_2: grid_2(); break;
		case KEY_3: grid_3(); break;
		case KEY_4: grid_4(); break;
		case KEY_5: grid_5(); break;
		case KEY_6: grid_6(); break;
		case KEY_7: grid_7(); break;

		// 90 DEGREE ROTATION
		case KEY_LBRACKET: // ccwise
			input |= ME_IN_CCWISE;
			break;
		case KEY_RBRACKET:
			input |= ME_IN_CWISE;
			break;

		// PALETTE HOT KEYS
		case KEY_F1:
		case KEY_F2:
		case KEY_F3:
		case KEY_F4:
		case KEY_F5:
		case KEY_F6:
		case KEY_F7:
		case KEY_F8:
		case KEY_F9:
		case KEY_F10:
		case KEY_F11:
		case KEY_F12:
			palette.ChangeTo( key - KEY_F1 + 1 );
			break;


		// MODIFIERS
		case KEY_CAPSLOCK:
			capslockModifier = true;
			break;
		case KEY_LSHIFT:
		case KEY_RSHIFT:
			shiftKeyModifier = true;
			break;
		case KEY_LCTRL:
		case KEY_RCTRL:
			ctrlKeyModifier = true;
			break;
		case KEY_LALT:
		case KEY_RALT:
			altKeyModifier = true;
			break;
		}
	// KEY UP
	} else {
		// MODIFIERS
		switch ( key ) {
		case KEY_LSHIFT:
		case KEY_RSHIFT:
			shiftKeyModifier = false;
			break;
		case KEY_LCTRL:
		case KEY_RCTRL:
			ctrlKeyModifier = false;
			break;
		case KEY_LALT:
		case KEY_RALT:
			altKeyModifier = false;
			break;
		case KEY_CAPSLOCK:
			capslockModifier = false;
			break;
		}
	}
}

// called by input loop
void ME_MouseFrame( mouseFrame_t *mframe ) {

	if ( !mframe )
		return;

	static int now;
	static int before;
	static int lastFlag;

	now = Com_Millisecond();

	// not sure why, but win32 mouse is sending us multiple mouse events for the same millisec
	// I'm just going to throw away multiple clicks until I can come up with a better solution
	// but don't throw away scroll wheel input 
	if ( (now == before || now-1 == before) && lastFlag == SE_MOUSE_CLICK && mframe->flags == SE_MOUSE_CLICK && !mframe->z ) {
		return;
	}
	lastFlag = mframe->flags;

	me_mouse.x = mframe->x;
	me_mouse.y = M_Height() - mframe->y;

	int state = MOUSE_NORMAL;

	// left mouse button
	if ( mframe->mb[0] ) {
		if ( !(me_mouse.lmb_down & 0x80) ) {
			me_mouse.lmb_click = 0x80;
			state |= MOUSE_LMB_DOWN;
		}
		me_mouse.lmb_down = 0x80;
	} else {
		me_mouse.lmb_click = 0x00;
		me_mouse.lmb_down = 0x00;
		state |= MOUSE_LMB_UP;
	}

	// right mouse button
	if ( mframe->mb[1] ) {
		if ( !(me_mouse.rmb_down & 0x80) ) {
			me_mouse.rmb_click = 0x80;
			state |= MOUSE_RMB_DOWN;
		}
		me_mouse.rmb_down = 0x80;
	} else {
		me_mouse.rmb_click = 0x00;
		me_mouse.rmb_down = 0x00;
		state |= MOUSE_RMB_UP;
	}

	// middle mouse button
	if ( mframe->mb[2] ) {
		if ( !(me_mouse.mmb_down & 0x80) ) {
			me_mouse.mmb_click = 0x80;
			state |= MOUSE_MMB_DOWN;
		}
		me_mouse.mmb_down = 0x80;
	} else {
		me_mouse.mmb_click = 0x00;
		me_mouse.mmb_down = 0x00;
		state |= MOUSE_MMB_UP;
	}

    // mouse-wheel 
	me_mouse.z += mframe->z;

	menu.mouseInput( me_mouse.x, me_mouse.y, me_mouse.z, state );

	before = Com_Millisecond();
}

/**********************************************************************************************/

void newPoly_f( void ) {
	input |= ME_IN_NEW_POLY;
}

void zeroAngle_f( void ) {
	input |= ME_IN_ZERO_ANGLE;
}

void deletePoly_f( void ) {
	input |= ME_IN_DELETE;
}

void copy_f ( void ) {
	input |= ME_IN_COPY;
}

void paste_f ( void ) {
	input |= ME_IN_PASTE;
}

void zoomIn_f ( void ) {
	input |= ME_IN_ZOOM_IN;
}

void zoomOut_f ( void ) {
	input |= ME_IN_ZOOM_OUT;
}

void toggleGrid_f ( void ) {
	input |= ME_IN_TOGGLE_GRID;
}

void quit_f ( void ) {
	Com_HastyShutdown();
}

void nextTexture_f ( void ) {
	input |= ME_IN_NEXT_TEXTURE;
}

void prevTexture_f( void ) {
	input |= ME_IN_PREV_TEXTURE;
}

void next25Texture_f ( void ) {
	input |= ME_IN_NEXT_25_TEXTURE;
}

void prev25Texture_f( void ) {
	input |= ME_IN_PREV_25_TEXTURE;
}

void deselectAll_f ( void ) {
	input |= ME_IN_DESELECT_ALL;
}

void selectAll_f ( void ) {
	input |= ME_IN_SELECT_ALL;
}

void grid_1 ( void ) { ME_SetSubline( 1 ); }
void grid_2 ( void ) { ME_SetSubline( 2 ); }
void grid_3 ( void ) { ME_SetSubline( 3 ); }
void grid_4 ( void ) { ME_SetSubline( 4 ); }
void grid_5 ( void ) { ME_SetSubline( 5 ); }
void grid_6 ( void ) { ME_SetSubline( 6 ); }
void grid_7 ( void ) { ME_SetSubline( 7 ); }
void grid_0 ( void ) { ME_SetSubline( 0 ); }
void grid_m1 ( void ) { ME_SetSubline( -1 ); }
void grid_m2 ( void ) { ME_SetSubline( -2 ); }

void incGrid_f ( void ) {
	ME_IncSubline(); 
}
void decGrid_f ( void ) {
	ME_DecSubline();
}

void save_f ( void ) {
	ME_SpawnSaveDialog();
}

void load_f ( void ) {
	ME_SpawnLoadDialog();
}

void newFile_f ( void ) {
	tiles->reset();
}

void resnap_f ( void ) {
	input |= ME_IN_RESNAP;
}

void rot90_f ( void ) {
	input |= ME_IN_CWISE;
}

void rot90cc_f ( void ) {
	input |= ME_IN_CCWISE;
}

void toggleCon_f ( void ) {
	console.Toggle();
}

void toggleColModel_f( void ) {
	input |= ME_IN_ADD_REMOVE_COLMODEL;
}

void changeVis_f( void ) {
	input |= ME_IN_VISIBILITY;
}

void toggleSnap_f ( void ) {
	input |= ME_IN_TOGGLE_TILE_SNAP;
}

void lock_f ( void ) {
	ME_ToggleLockSelected();
}

void incLayer_f( void ) {
	ME_IncrementLayer( +1 );
}

void decLayer_f( void ) {
	ME_IncrementLayer( -1 );
}

void toggleRotate_f( void ) {
	input |= ME_IN_TOGGLE_ROTATE_MODE;
}

void cycleSelected_f( void ) {
	input |= ME_IN_CYCLE_POLYS;
}

void nextPalette_f( void ) {
	palette.Next();
}
void prevPalette_f( void ) {
	palette.Prev();
}

/**********************************************************************************************/


int ME_ConPrintUIDs( void ) {
	node_c<mapNode_t*> *node = selected.gethead();

	if ( !node )
		return 0;

	char buf[100];

	buffer_c<char> kbuf;
	kbuf.init();

	while ( node ) {
		Assert( node->val != NULL );
		Assert( node->val->val != NULL );

		snprintf( buf, 100, "%d, ", node->val->val->uid );
		int tlen = strlen( buf );
		Assert( tlen < 100 );
		kbuf.copy_in( buf, tlen );

		node = node->next;
	}

	int k = kbuf.length();
	kbuf.data[ --k ] = '\0';
	kbuf.data[ --k ] = '\0';

	console.Printf( "Selected UIDS: %s", kbuf.data );
	kbuf.destroy();

	static int client_id = 0;
	if ( !client_id ) {
		client_id = floating.requestClientID();
	}

	// floating text
	node = selected.gethead();
	while ( node ) {
		snprintf( buf, sizeof(buf), "uid_%u", node->val->val->uid );
		floating.AddTileText( node->val->val, buf, 12.f, 100.f, 12, 3000, client_id );
		node = node->next;
	}

	return 1;
}

int ME_ConPrintLayers( void ) {
	node_c<mapNode_t*> *node = selected.gethead();

	if ( !node )
		return 0;

	static int client_id = 0;
	if ( !client_id ) {
		client_id = floating.requestClientID();
	}

	buffer_c<char> kbuf;
	kbuf.init();

	while ( node ) {
		Assert( node->val != NULL );
		Assert( node->val->val != NULL );

		char buf[100];
		snprintf( buf, 100, "uid_%d => %d, ", node->val->val->uid, node->val->val->layer );	
		int len = strlen( buf );
		kbuf.copy_in( buf, len );

		floating.ClearUIDAndClientID( node->val->val->uid, client_id );
		snprintf( buf, sizeof(buf), "%d", node->val->val->layer );
		floating.AddTileText( node->val->val, buf, 60.f, 180.f, 20, 3000, client_id );

		node = node->next;
	}

	int k = kbuf.length();
	kbuf.data[ --k ] = '\0';
	kbuf.data[ --k ] = '\0';

	console.Printf( "Selected Layers: %s", kbuf.data );

	kbuf.destroy();

	return 1;
}

void ME_ConPrintTileInfo( void ) {
	if ( 0 == tiles->size() ) {
		console.Printf( "There are currently no mapTiles created.  You should make one by pressing <SPACEBAR>" );
		return;
	}

	console.pushMsg( "" );
	console.Printf( "Total Tiles: %u.  Total Selected: %u", tiles->size(), selected.size() );
	console.Printf( "---------------------------------------------" );

	node_c<mapNode_t*> *node = selected.gethead();
	while( node ) {
		console.Printf( "mapTile: uid_%u (%.2f, %.2f) width: %.2f height: %.2f rot: %f", 
			node->val->val->uid, 
			node->val->val->poly.x, 
			node->val->val->poly.y, 
			node->val->val->poly.w, 
			node->val->val->poly.h, 
			node->val->val->poly.angle 
			); 
		node = node->next;
	}
}

void ME_SetLockAll( int wantLock ) {
	node_c<mapTile_t *> *node = tiles->gethead();
	if ( !node )
		return;

	while ( node ) {
		node->val->lock = wantLock;
		node = node->next;
	}

	if ( wantLock ) {
		console.Printf( "locking all tiles" );
	} else {
		console.Printf( "unlocking all tiles" );
	}
}

void ME_CreateColorMaterial( float r, float g, float b, float a ) {
	material_t *mat = material_t::New();
	mat->type = MTL_COLOR;
	mat->color[ 0 ] = r;
	mat->color[ 1 ] = g;
	mat->color[ 2 ] = b;
	mat->color[ 3 ] = a;
	materials.add( mat );
	console.Printf( "color material: %.1f %.1f %.1f %.1f created and added @ #%u", r,g,b,a, materials.total );
}

void ME_ConPrintMaterialInfo( int mtl_id ) {

	console.pushMsg( "" );
	console.Printf( "%u total materials loaded", materials.size() );

	// no id, try selected
	if ( -1 == mtl_id ) {

		if ( 0 == selected.size() ) {
			console.Printf( "no materials selected" );
			return;
		}

		// FIXME: would be better to get the uids of all tiles that have a 
		//  particular texture or something, but that's too much work.  this
		//  just allows quick inspection
	
		node_c<mapNode_t*> * node = selected.gethead();
		while ( node ) {
			material_t *mat = node->val->val->mat;
			switch( mat->type ) {
			case MTL_COLOR:
				console.Printf( "uid_%u: MTL_COLOR: %.2f %.2f %.2f %.2f", node->val->val->uid, mat->color[0], mat->color[1], mat->color[2], mat->color[3] );
				break;
			case MTL_TEXTURE_STATIC:
				console.Printf( "uid_%u: MTL_TEXTURE_STATIC: \"%s\"", node->val->val->uid, mat->name );
				break;
			case MTL_COLORMASK:
				console.Printf( "colorMask material: (%.1f %.1f %.1f %.1f) \"%s\"", 
					mat->color[ 0 ], mat->color[ 1 ], mat->color[ 2 ], mat->color[ 3 ],
					(mat->img) ? mat->img->syspath : "null" );
				break;
			}

			node = node->next;
		}
	}

	// otherwise find id
}

static int bg_client_id = 0;

void ME_SetBackgrounds( void ) {

	if ( !bg_client_id ) {
		bg_client_id = floating.requestClientID();
	}

	node_c<mapNode_t*> *node = selected.gethead();

	char buf[128];
	while( node ) {
		node->val->val->background = true;
		sprintf( buf, "background %u", node->val->val->uid );
		floating.AddTileText( node->val->val, buf, 5.f, node->val->val->poly.h-24.f, 12, 0, bg_client_id );
		console.Printf( "mapTile (%.2f, %.2f) uid_%u is background", node->val->val->poly.x, node->val->val->poly.y, node->val->val->uid );
		node_c<mapNode_t*> *tmp = node->next;
		selected.popnode( node );
		node = tmp;
	}
}

void ME_UnsetBackground( const char *num ) {
	if ( !num || !num[0] ) {
		console.Printf( "please provide number" );
		return;
	}

	unsigned int _uid = atoi ( num );
	
	mapNode_t *n = tiles->gethead();
	while( n ) {
		if ( n->val->background && n->val->uid == _uid ) {
			n->val->background = false;
			break;
		}
		n = n->next;
	}
	if ( n ) {
		floating.ClearUIDAndClientID( _uid, bg_client_id );
		console.Printf( "unset background %u", _uid );
	} else {
		console.Printf( "couldn't find background tile %u ", _uid );
	}
}

void ME_UnsetAllBackgrounds( void ) {

	floating.ClearClientID( bg_client_id );

	mapNode_t *node = tiles->gethead();
	while( node ) {
		node->val->background = false;
		node = node->next;
	}

	console.Printf( "all backgrounds unset" );
	floating.AddCoordText( M_Width()*0.5-140.f, M_Height()-30.0f, "backgrounds reset", 18, 3000 );
}

bool ME_CreateColorMaskMaterial( float r, float g, float b, float a ) {
	node_c<mapNode_t*> *node = selected.gethead();
	if ( !node ) 
		return false;

	int count = 0;
	while( node ) {
		++count;
		if ( count > 1 )
			return false;
		node = node->next;	
	}

	node = selected.gethead();
	if ( !node->val->val->mat )
		return false;
	if ( !node->val->val->mat->img )
		return false;

	material_t *mat = node->val->val->mat->Copy();

	mat->type = MTL_COLORMASK;
	mat->color[ 0 ] = r;
	mat->color[ 1 ] = g;
	mat->color[ 2 ] = b;
	mat->color[ 3 ] = a;
	materials.add( mat );
	console.Printf( "ColorMask material: ( %.1f %.1f %.1f %.1f ) \"%s\" created and added @ #%u", r,g,b,a, ( mat->img ) ? mat->img->syspath : "null", materials.total );

	// give the current tile the new material
	memcpy( node->val->val->mat, mat, sizeof(material_t) );

	return true;
}

bool ME_DupMaterial( void ) {
	node_c<mapNode_t*> *node = selected.gethead();
	if ( !node || selected.size() != 1 ) 
		return false;

	material_t *mat = node->val->val->mat->Copy();
	materials.add( mat );
	//console.Printf( "duplicated material" );
	ME_ConPrintMaterialInfo( -1 );
	return true;
}

//#define DO_IT_YOURSELF

#if !defined ( DO_IT_YOURSELF )


bool ME_CreateMaskedTexture( void ) {
	if ( selected.size() != 2 )
		return false;
	
	node_c<mapNode_t*> *node = selected.gethead();
	material_t *tex = node->val->val->mat;
	material_t *msk = node->next->val->val->mat;

	if ( tex->type != MTL_TEXTURE_STATIC ) {
		tex = node->next->val->val->mat;
		msk = node->val->val->mat;
	}
	if ( tex->type != MTL_TEXTURE_STATIC || msk->type != MTL_COLORMASK ) {
		return false;
	}
	if ( ! msk->img || ! tex->img )
		return false;


	int num = 0;
	glGetIntegerv( GL_MAX_TEXTURE_UNITS, &num );

	console.Printf( "Max OpenGL Texture Units: %d", num );

	// CREATE GL MULTITEXTURE

/*
	gglActiveTexture( GL_TEXTURE0 );
	gglEnable( GL_TEXTURE_2D );
	gglBindTexture( GL_TEXTURE_2D, tex->img->texhandle );
	gglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

	gglActiveTexture( GL_TEXTURE1 );
	gglEnable( GL_TEXTURE_2D );
	gglBindTexture( GL_TEXTURE_2D, msk->img->texhandle );
	gglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

    gglActiveTexture( GL_TEXTURE1 );
    gglDisable( GL_TEXTURE_2D );
    gglActiveTexture( GL_TEXTURE0 );
    gglDisable( GL_TEXTURE_2D );
*/
	
	material_t *mTex = tex->Copy();
	mTex->mask = msk->img ;
	mTex->type = MTL_MASKED_TEXTURE;
	materials.add( mTex );
	console.Printf( "Masked Texture Material: Tex: \"%s\" Mask: \"%s\" created @ #%u", mTex->img->syspath, mTex->mask->syspath, materials.total );

	return true;
}

#else

bool ME_CreateMaskedTexture( void ) {
	if ( selected.size() != 2 )
		return false;
	
	node_c<mapNode_t*> *node = selected.gethead();

	material_t *tex = node->val->val->mat;
	material_t *msk = node->next->val->val->mat;

	if ( tex->type != MTL_TEXTURE_STATIC ) {
		tex = node->next->val->val->mat;
		msk = node->val->val->mat;
	}
	if ( tex->type != MTL_TEXTURE_STATIC || msk->type != MTL_COLORMASK ) {
		return false;
	}
	if ( ! msk->img || ! tex->img )
		return false;

	image_t *img = NULL;

	if ( 0 != IMG_CombineTextureMaskPow2( tex->img, msk->img, &img ) ) {
		return false;
	}
	Assert( img != NULL );

	material_t *mTex = tex->Copy();
	mTex->img = img ;
	mTex->mask = msk->img; // save pointer for this just for accounting
	mTex->type = MTL_MASKED_TEXTURE;
	materials.add( mTex );
	console.Printf( "Masked Texture Material: Tex: \"%s\" Mask: \"%s\" created @ #%u", mTex->img->syspath, mTex->mask->syspath, materials.total );

	return true;
}
#endif // DO_IT_YOURSELF

/* calling this against a material may change it's type.  

- textureMask is promoted to textureMaskColor

*/
bool ME_SetMaterialColor( float r, float g, float b, float a ) {
	node_c<mapNode_t*> *node = selected.gethead();
	if ( !node )
		return false;
	while ( node ) {
		material_t *m = node->val->val->mat;
		switch ( node->val->val->mat->type ) {
		case MTL_COLOR:
		case MTL_TEXTURE_STATIC:
		case MTL_COLORMASK:
		case MTL_MASKED_TEXTURE_COLOR:
			console.Printf( "uid_%u color changed to: %.2f %.2f %.2f %.2f", node->val->val->uid, r,g,b,a );
			m->color[0] = r; m->color[1] = g; m->color[2] = b; m->color[3] = a;
			break;
		case MTL_MASKED_TEXTURE:
			console.Printf( "uid_%u promoted from Masked_Texture to Masked_Texture_Color: %.2f %.2f %.2f %.2f", node->val->val->uid, r,g,b,a );
			m->color[0] = r; m->color[1] = g; m->color[2] = b; m->color[3] = a;
			m->type = MTL_MASKED_TEXTURE_COLOR;
			break;
    	case MTL_MULTITEX:
			m->color[0] = r; m->color[1] = g; m->color[2] = b; m->color[3] = a;
			break;
		}
		node = node->next;
	}
	return true;
}

/*
*/
void ME_CombineColModels( void ) {
	node_c<mapNode_t*> *node = selected.gethead();
	if ( !node ) {
		console.Printf( "no tiles selected" );
		return;
	}
	AABB_t master;
	COPY4( master, node->val->val->col->box );
	node = node->next;
	if ( !node ) {
		console.Printf( "combine one tile to itself has no effect" );
		return;
	}
	while ( node ) {
		if ( !node->val->val->col ) {
			node = node->next;
			continue;
		}
		float * box = node->val->val->col->box;

		if ( box[0] < master[0] ) {
			master[0] = box[0];
		}
		if ( box[1] < master[1] ) {
			master[1] = box[1];
		}
		if ( box[2] > master[2] ) {
			master[2] = box[2];
		}
		if ( box[3] > master[3] ) {
			master[3] = box[3];
		}
		node = node->next;
	}

	node = selected.gethead();
	mapTile_t *first = NULL;
	while ( node ) {
		if ( node->val->val->col ) {
			first = node->val->val;
			node = node->next;
			break;
		}
		node = node->next;
	}
	
	if ( !first ) {
		console.Printf( "tiles contain no colModels" );
		return;
	}
	if ( first && !node ) {
		console.Printf( "combine one tile to itself has no effect" );
		return;
	}

	// delete the rest
	while ( node ) {
		if ( node->val->val->col ) {
		 	delete node->val->val->col;
			node->val->val->col = NULL;
		}
		node = node->next;
	}

	// set the first
	COPY4( first->col->box, master );
	console.Printf( "colModel tiles combined to (%.2f %.2f %.2f %.2f) and attached to mapTile: %d", 
		first->col->box[0], 
		first->col->box[1], 
		first->col->box[2], 
		first->col->box[3], 
		first->uid );
}

// prints info about all backgrounds
void ME_BackgroundInfo( void ) {
	console.Printf( "bginfo not implemented.  Just read the UID off of the background and add it to the Area with 'bgadd'" );
}

