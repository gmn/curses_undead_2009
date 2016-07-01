
#include <glew.h>

#include "../common/common.h"
#include "r_local.h"
#include "../map/m_area.h" // world
#include "../map/mapdef.h"
#include "../client/cl_console.h"
#include "../game/g_entity.h"
#include "r_floating.h"
#include "../server/server.h"
#include "r_effects.h"
#include "../client/cl_public.h"
#include <gl/gl.h>
#include "r_glmacro.h"


gvar_c *r_drawHiddenEnts = NULL;
gvar_c *ent_lerp = NULL;
gvar_c *tile_lerp = NULL;
gvar_c *r_drawColMod = NULL;
extern gvar_c *r_maxfps;
gvar_c *r_dim = NULL;
gvar_c *r_falloff = NULL;
gvar_c *r_globIllum = NULL;
bool r_inteleport = false;
gvar_c *r_drawLOS = NULL;
gvar_c *r_drawGameCon = NULL;
gvar_c *r_printBlocks = NULL;

extern gvar_c * platform_type ;

/*
====================
 R_LocalInits
====================
*/
static bool r_local_inits = false;
static void R_LocalInits( void ) {
	if ( r_local_inits )
		return;
	if ( !r_drawHiddenEnts ) {
		r_drawHiddenEnts = Gvar_Get( "r_drawHiddenEnts", "0", 0 );
	}

	if ( !ent_notex ) {
		ent_notex = materials.FindByName( "gfx/ent_notex.tga" );
	}

	ent_lerp = Gvar_Get( "ent_lerp", "0", 0 );
	tile_lerp = Gvar_Get( "tile_lerp", "0", 0 );
	r_drawColMod = Gvar_Get( "r_drawColMod", "0", 0 );

	r_local_inits = true;

	// if r_globIllum is ON, then it uses r_dim value, else it uses R_Falloff
	r_globIllum = Gvar_Get( "r_globIllum", "1" , 0 );
	r_dim = Gvar_Get( "r_dim", "1.0", 0 ); // off
	r_falloff = Gvar_Get( "r_falloff", "5000000", 0 );
	r_drawLOS = Gvar_Get( "r_drawLOS", "0", 0 );
    r_drawGameCon = Gvar_Get( "r_drawGameCon", "0", 0 );
    r_printBlocks = Gvar_Get( "r_printBlocks", "0", 0 );
}



/// FIXME: didn't end up using
//Renderer_t render;

/*
====================
 R_LerpEnt

	takes an Entity's lerpFrame_c& and constructs an interpolated 
    position between the last two computed frames. based on time has passed
    since last frame computed, except instead of extrapolating forwards from
    last computed frame, we use the time difference to interpolate between
    the last 2 computed frames, effectly drawing 1 frame in the past instead
    of a fractional frame into the future that hasn't been computed yet. which
    had a tendency to lead to drawing problems.
====================
*/
static float * R_LerpEnt( lerpFrame_c & lerpFrame ) {
	lframe_t & f0 = lerpFrame.getFrame( 0 );
	lframe_t & f1 = lerpFrame.getFrame( -1 );

	// interpolation factor into frame as a function of time
	// drawTime on the client side vs time to complete the frame on the server
	float dt_now = now() - f0.time;
	const float dt = f0.time - f1.time;
	const float dt_ratio = dt_now / dt;

	static float out[2]; // adjusted coords 

	// x
	float dx = f0[0] - f1[0];
	out[0] = f1[0] + dt_ratio * dx ;
	
	// y
	float dy = f0[1] - f1[1];
	out[1] = f1[1] + dt_ratio * dy ;

    // clamp the x,y values to be somewhere in-between the two computed frames
	
	// clamp x
	if ( f0[0] > f1[0] ) { // +
        if ( out[0] > f0[0] ) {
			out[0] = f0[0];
		} else if ( out[0] < f1[0] ) {
			out[0] = f1[0];
		} 
	} else if ( f0[0] < f1[0] ) { // -
        if ( out[0] < f0[0] ) {
			out[0] = f0[0];
		} else if ( out[0] > f1[0] ) {
			out[0] = f1[0];
		} 
	}
	// clamp y
	if ( f0[1] > f1[1] ) { // +
        if ( out[1] > f0[1] ) {
			out[1] = f0[1];
		} else if ( out[1] < f1[1] ) {
			out[1] = f1[1];
		} 
	} else if ( f0[1] < f1[1] ) { // -
        if ( out[1] < f0[1] ) {
			out[1] = f0[1];
		} else if ( out[1] > f1[1] ) {
			out[1] = f1[1];
		} 
	}
	return out;
}

/*
====================
 R_Falloff

	returns a light factor 1.0f --> 0.0f
	for dimming tiles as they get further from the player
====================
*/
float R_Falloff( float *v ) {
	if ( !r_falloff->integer() )
		return 1.0f;

	const float F = r_falloff->value();
	
	float x = 	(v[0] - player.poly.x ) * (v[0] - player.poly.x) +
				(v[1] - player.poly.y ) * (v[1] - player.poly.y) ;
	x = sqrtf( x );

	float light = expf( -1.0f * x * x / F );
	if ( light > 1.0f )
		light = 1.0f;
	if ( light < 0.0f )
		light = 0.0f;

	return light;
}

/*
====================
 R_DrawTileColModel
====================
*/
void R_DrawTileColModel( colModel_t *C ) {
	if ( !C )
		return;
    gglLineWidth( 2.0f );
	gglDisable( GL_TEXTURE_2D );
   	gglColor4f( 0.7f, 0.7f, 1.0f, 0.75f );
	GL_DRAW_OUTLINE_4V( C->box );
	gglEnable( GL_TEXTURE_2D );
    gglColor4f( 1.f, 1.f, 1.f, 1.f );
    gglLineWidth( 1.0f );
}

/*
==================== 
 R_DrawColModel
==================== 
*/
void R_DrawColModel( Entity_t *E ) {
	if ( !E )
		return;
	if ( !E->collidable )
		return;

    gglLineWidth( 2.0f );
	gglDisable( GL_TEXTURE_2D );

	if ( E->col ) { // light blue
    	gglColor4f( 0.7f, 0.7f, 1.0f, 0.75f );
		GL_DRAW_OUTLINE_4V( E->col->box );
	}
	if ( E->triggerable ) { // yellow
		gglColor4f( 1.f, 1.f, 0.f, 1.f );
		GL_DRAW_OUTLINE_4V( E->trig );
	}
	if ( E->clipping ) { // red
		gglColor4f( 1.f, 0.f, 0.f, 1.f );
		GL_DRAW_OUTLINE_4V( E->clip );
	}

	gglEnable( GL_TEXTURE_2D );
    gglColor4f( 1.f, 1.f, 1.f, 1.f );
    gglLineWidth( 1.0f );
}

#if 0
/*
==================== 
 if a map is loaded, it sets curBlock, and camera-xy. 
==================== 
*/
void Renderer_t::Init( void ) {
}

void Renderer_t::DrawFrame( void ) {
}
#endif

extern mouseFrame_t *cl_mframe;
GLfloat before_rot[16];
float r_angle = 0.f;

static void R_Rotate( void ) {
	if ( !cl_mframe )
		return;
	/* 
	cool auto-gyrate thing, looks like a cactus game
    float ms = (float)( Com_Millisecond() % 10000 );
    float g = cosf( ms * 6.3 / 10000.0f );
	static float r_angle = 20.f * g;
	*/

    // METHOD 1 :: Win32 Mouse 
	const float dim_x = M_Width();
	const float dim_y = M_Height();

	float pl[2];
	player.poly.getCenter( pl );
	M_WorldToScreen( pl );

    float x = cl_mframe->x;
	int ix = cl_mframe->x;
    const float PixTo90 = dim_x / 2.0f;
    float a = x - dim_x/2.0f;


	float trans_x = pl[0];
	float trans_y = pl[1];

	static int lastMouseFrame = 0;

    if ( cls.keyCatchers & KEYCATCH_GAME ) {
        // METHOD 2 :: DI Mouse
        if ( di_mouse->integer() ) {
    		// ONLY change angle if new mouse input
    		if ( lastMouseFrame != cl_mframe->frameNum ) {
    	        r_angle += x / PixTo90 * -90.f;
    			lastMouseFrame = cl_mframe->frameNum;
    		}
    	// WIN32 
        } else {
    		// sets to exact coordinates from 0,0 -> W,H (of window)
            r_angle = a / PixTo90 * 180.f;
        }
    }

    while ( r_angle > 360.000f ) {
        r_angle -= 360.0f;
    }
    while ( r_angle < -360.000f ) {
        r_angle += 360.0f;
    }

    gglTranslatef( trans_x, trans_y, 0.f );
    gglRotatef( r_angle, 0.f, 0.f, -1.f );
    gglTranslatef( -trans_x, -trans_y, 0.f );

}

void DEBUG_Print( void ) {
	CL_ModelViewInit2D();
	gglColor4f( 1.0f, 0, 0, 1 );
    main_viewport_t *v = M_GetMainViewport();
	if ( cl_mframe )
		F_Printf( 20, 20, 1, "%i %i, %.1f %.1f, zoom: %.1f", cl_mframe->x, cl_mframe->y, v->world.x, v->world.y, v->ratio );
}

/*
====================
 R_ResetForDrawing
====================
*/
static void R_ResetForDrawing( int rotate = 1 ) {

	CL_ModelViewInit2D();
    gglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

	if ( rotate == 1 )
		R_Rotate();

    main_viewport_t *v = M_GetMainViewport();
	gglScalef( v->iratio, v->iratio, 0.f );
	gglTranslatef( -v->world.x, -v->world.y, 0.f );

  	gglEnable( GL_TEXTURE_2D );
	gglTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
}

void R_SetForWorldDrawing( int rotate  = 1 ) {
    gglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

	if ( rotate )
		R_Rotate();

    main_viewport_t *v = M_GetMainViewport();
	gglScalef( v->iratio, v->iratio, 0.f );
	gglTranslatef( -v->world.x, -v->world.y, 0.f );

  	gglEnable( GL_TEXTURE_2D );
	gglTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
}

/*
====================
 R_PrintBlocks
====================
*/
void R_PrintBlocks( buffer_c<Block_t*>& blocks, float blk[2], unsigned int bx, unsigned int by ) { 
	// reset the modelview
    CL_ModelViewInit2D();

gglDisable( GL_BLEND ); 

	// get viewport size
	float vp[4];
	vp[2] = M_Width();
	vp[3] = M_Height();

	const int inset = 80;

	// max width, max height
	// move each corner inwards diagonally 80 pixels to get blockmap size
	int bmap[4];
	bmap[0] = vp[2] - inset * 2;
	bmap[1] = vp[3] - inset * 2;

	// divide ( bmap_w / A->bx ), ( bmap_h / A->by ) to get the H & W block 
	//  dimension.
	float block_dim[2];
	block_dim[ 0 ] = ((float)bmap[0]) / bx;
	block_dim[ 1 ] = ((float)bmap[1]) / by;

/* I took this out and it looks really cool.  Kinda skewed like the original
	Adventure come to think of it.

	// take lesser of the two
	if ( block_dim[0] < block_dim[1] ) {
		block_dim[1] = block_dim[0];
	} else {
		block_dim[0] = block_dim[1];
	}
*/

	// calculate new offsets & perimeter
	bmap[0] = ( M_Width() - block_dim[0] * bx ) / 2;
	bmap[1] = ( M_Height() - block_dim[1] * by ) / 2;
	bmap[2] = bmap[0] + block_dim[0] * bx;
	bmap[3] = bmap[1] + block_dim[1] * by;

	// draw 4 pixel white border around blockmap
	gglColor4f( 1.f, 1.f, 1.f, 1.f );
	gglLineWidth( 4.0f );
	GL_LINES_FANCYPANTS( bmap, 4, 2 );
	gglLineWidth( 1.0f );

	// draw each block: grey for !isOccupied(), blue-grey for Occupied(), 
	// reddish is for curBlock
	// also draw a darker, near-black 2px border for each
	for ( int j = 0; j < by; j++ ) {
		for ( int i = 0; i < bx; i++ ) {
			// color
			if ( blocks.data[j*bx+i] == world.curBlock ) {
				gglColor4f( 1.0f, 0.0f, 0.0f, 0.34f );
			} else if ( blocks.data[j*bx+i]->isOccupied() ) {
				gglColor4f( 0.1f, 0.2f, 0.4f, 0.6f );
			} else {
				gglColor4f( 0.3f, 0.3f, 0.3f, 0.6f );
			}

			// block
			float v[4];
			v[0] = bmap[0] + i * block_dim[0];
			v[1] = bmap[1] + j * block_dim[1];
			v[2] = bmap[0] + (i+1) * block_dim[0];
			v[3] = bmap[1] + (j+1) * block_dim[1];
			GL_DRAW_QUAD_4V( v );

			// outline
			gglColor4f( 0.1f, 0.1f, 0.1f, 0.7f );
			gglLineWidth( 2.0f );
			GL_DRAW_OUTLINE_4V( v );
			gglLineWidth( 1.0f );
		}
	}

	// draw connections between each block w/ a non-null directional pointers
	// ( as an arrow )
	gglColor4f( 0.7f, 0.7f, 0.7f, 0.7f );
	for ( int j = 0; j < by; j++ ) {
		for ( int i = 0; i < bx; i++ ) {
			float v[4];
			v[0] = bmap[0] + i * block_dim[0];
			v[1] = bmap[1] + j * block_dim[1];
			v[2] = bmap[0] + (i+1) * block_dim[0];
			v[3] = bmap[1] + (j+1) * block_dim[1];

			if ( blocks.data[j*bx+i]->up ) {
				UP_ARROW( v );
			}
			if ( blocks.data[j*bx+i]->down ) {
				DOWN_ARROW( v );
			}
			if ( blocks.data[j*bx+i]->left ) {
				LEFT_ARROW( v );
			}
			if ( blocks.data[j*bx+i]->right ) {
				RIGHT_ARROW( v );
			}
		}
	}

	gglPushMatrix();
	// translate up & over by inset
	gglTranslated( bmap[0], bmap[1], 0 );
	// scale up viewport size to ratio that blocks are scaled from real to
	//  screen representation
	gglScalef( ((float)block_dim[0])/blk[0], ((float)block_dim[1])/blk[1], 0.f );
	// translate ll-corner of blocks up to origin
	main_viewport_t *mvp = M_GetMainViewport();
	gglTranslatef(	-blocks.data[0]->p1[0], 
					-blocks.data[0]->p1[1], 
					0.f ); 


	// green
	gglColor4f( 0.1f, 0.4f, 0.1f, 0.5f );

	// go through blocks, draw all block's dispTiles, scaled appropriately
	for ( int i = 0; i < blocks.length(); i++ ) {
		const unsigned int len = blocks.data[i]->dispTiles.length();
		for ( unsigned int j = 0; j < len; j++ ) {
			GL_DRAW_QUAD_8V( blocks.data[i]->dispTiles.data[j]->obb );		
		}
	}

	gglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

	// draw all block's entities
	for ( int i = 0; i < blocks.length(); i++ ) {
		for ( unsigned int k = 0; k < TOTAL_ENTITY_CLASSES; k++ ) {
			node_c<Entity_t*> *e = blocks.data[i]->entities[k].gethead();
			while ( e ) {
				GL_DRAW_QUAD_8V( e->val->obb );		
				e = e->next;
			}
		}
	}

	// draw white outline of viewport
	gglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
    gglPushMatrix();
    gglTranslatef( player.obb[0], player.obb[1] , 0.f );
    gglRotatef( -r_angle, 0.f, 0.f, -1.f );
    gglTranslatef( -player.obb[0], -player.obb[1] , 0.f );
	vp[0] = mvp->world.x;
	vp[1] = mvp->world.y;
	vp[2] = vp[0] + M_Width() * mvp->ratio;
	vp[3] = vp[1] + M_Height() * mvp->ratio;
	GL_DRAW_OUTLINE_4i( vp );
    gglPopMatrix();

	// 
	F_Printf( 20, 20, 0, "%f %f", mvp->world.x, mvp->world.y );

	// draw the player
	gglColor4f( 1.0f, 0.f, 1.0f, 1.0f );
	GL_DRAW_QUAD_8V( player.obb );		

	gglPopMatrix();

gglEnable( GL_BLEND ); 
}

/*
====================
 R_DrawDispTiles

	draws backgrounds and dispTiles, backgrounds first
====================
*/
void R_DrawDispTiles( void ) {

	// get Area backgrounds from world 
	const unsigned int bg_len = world.curArea->backgrounds.length();

	// get dispTile lists from world
	const unsigned int dl_len = world.dispList.length();

	// combine totals for total tiles to be drawn
	const unsigned int tot_len = bg_len + dl_len;

	// draw all, in order
	for ( int i = 0; i < tot_len; i++ ) 
	{
		dispTile_t * T;
		// backgrounds
		if ( i < bg_len ) {
			T = world.curArea->backgrounds.data[ i ];
		// regular tiles
		} else {
			T = world.dispList.data[ i - bg_len ];
		}

		animSet_t *		anim = T->anim[ T->drawing ];
		material_t * 	mat  = T->mat[ T->drawing ];

        float v[8];
		COPY8( v, T->obb );

		if ( !mat ) 
			mat = ent_notex;

		/* lighting */
		float f = ( r_globIllum->integer() ) ? r_dim->value() : R_Falloff( v );
		gglColor4f( f, f, f, 1.0f );

		//
		switch ( mat->type ) {
		case MTL_COLOR:
			gglDisable( GL_TEXTURE_2D );
			gglColor4fv( mat->color );
			break;
		case MTL_TEXTURE_STATIC:
			if ( !mat->img )
				goto do_default2;
			gglEnable( GL_TEXTURE_2D );
			gglBindTexture( GL_TEXTURE_2D, mat->img->texhandle );
			break;
		case MTL_COLORMASK:
			if ( !mat->img )
				goto do_default2;
			gglEnable( GL_TEXTURE_2D );
			gglBindTexture( GL_TEXTURE_2D, mat->img->texhandle );
			gglColor4fv( mat->color );
			break;
	
		case MTL_MASKED_TEXTURE:
		case MTL_MASKED_TEXTURE_COLOR:
			if ( !mat->img || !mat->mask )
				goto do_default2;

			gglActiveTexture( GL_TEXTURE0 );
			gglEnable( GL_TEXTURE_2D );
			gglBindTexture( GL_TEXTURE_2D, mat->img->texhandle );
			gglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

			gglActiveTexture( GL_TEXTURE1 );
			gglEnable( GL_TEXTURE_2D );
			gglBindTexture( GL_TEXTURE_2D, mat->mask->texhandle );
			gglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

			if ( mat->type == MTL_MASKED_TEXTURE_COLOR )
				gglColor4fv( mat->color );

			gglBegin( GL_QUADS );
		    gglMultiTexCoord2f( GL_TEXTURE0, mat->s[0], mat->t[0] );
		    gglMultiTexCoord2f( GL_TEXTURE1, mat->s[0], mat->t[0] );
	        gglVertex3f( v[0], v[1], 0.f );
		    gglMultiTexCoord2f( GL_TEXTURE0, mat->s[1], mat->t[1] );
		    gglMultiTexCoord2f( GL_TEXTURE1, mat->s[1], mat->t[1] );
	        gglVertex3f( v[2], v[3], 0.f );
		    gglMultiTexCoord2f( GL_TEXTURE0, mat->s[2], mat->t[2] );
		    gglMultiTexCoord2f( GL_TEXTURE1, mat->s[2], mat->t[2] );
	        gglVertex3f( v[4], v[5], 0.f );
		    gglMultiTexCoord2f( GL_TEXTURE0, mat->s[3], mat->t[3] );
		    gglMultiTexCoord2f( GL_TEXTURE1, mat->s[3], mat->t[3] );
	        gglVertex3f( v[6], v[7], 0.f );
		    gglEnd();
	
    		gglActiveTexture( GL_TEXTURE1 );
    		gglDisable( GL_TEXTURE_2D );
    		gglActiveTexture( GL_TEXTURE0 );
    		gglDisable( GL_TEXTURE_2D );

			break;
		default:
do_default2:
			// FIXME: should have a "NoTex" pattern to put on unset tiles
			gglDisable( GL_TEXTURE_2D );
			gglColor4f( 0.2f, 0.4f, 0.6f, 1.0f );
			break;
		}

		// standard draw call
		if ( 	mat->type != MTL_MASKED_TEXTURE && 
				mat->type != MTL_MASKED_TEXTURE_COLOR ) {
			gglBegin( GL_QUADS );
	    	gglTexCoord2f( mat->s[0], mat->t[0] );
        	gglVertex3f( v[0], v[1], 0.f );
	    	gglTexCoord2f( mat->s[1], mat->t[1] );
	        gglVertex3f( v[2], v[3], 0.f );
		    gglTexCoord2f( mat->s[2], mat->t[2] );
   		    gglVertex3f( v[4], v[5], 0.f );
		    gglTexCoord2f( mat->s[3], mat->t[3] );
	        gglVertex3f( v[6], v[7], 0.f );
		    gglEnd();
		}
	}
} 

/*
====================
 R_DrawConsole
====================
*/
void R_DrawConsole( void ) {
	gglColor4f( 0.2f, 1.0f, 0.2f, 1.0f );
	CL_ModelViewInit2D();
	console.Draw();
}

/*
====================
 R_DrawEnts
====================
*/
static void R_DrawEnts( unsigned int entStart, unsigned int entEnd ) {

	unsigned int entIndex = entStart;
	const unsigned int totalEnts = entEnd;

	//
	// draw all entities in order
	//
	while ( entIndex < totalEnts ) {
		// skip NODRAW entities
		if ( EC_NODRAW == world.entList.data[entIndex]->entClass && 
											!r_drawHiddenEnts->integer() ) {
			if ( ++entIndex >= totalEnts ) 
				break;
			continue;
		}

		Entity_t *ent = world.entList.data[ entIndex ];

		float v[8];
		material_t * m = ent_notex;
		materialType_t type = MTL_TEXTURE_STATIC;

		//
		// drawing quad
		//

		// copy the drawing geometry
		COPY8( v, ent->obb );

		// adjust it only if it's a thinker
		if ( ent->entClass >= EC_THINKER && ent_lerp->integer() ) {
			// get adjusted coord
			float *l = R_LerpEnt( ent->lerp );
			// take the difference
			float diff[2] = { l[0] - ent->poly.x, l[1] - ent->poly.y };
			// and add it in
			OBB_add( v, diff );
		}

		//
		// get a material
		//
		m = ent->getMat();
		if ( !m ) 
			m = ent_notex;
		type = m->type;
		if ( !m ) {
			m = ent_notex;
			type = MTL_TEXTURE_STATIC;
		}
		if ( !m->img ) {
			m->img = ent_notex->img;
			type = MTL_TEXTURE_STATIC;
		}


		gglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

		switch ( type ) {
		case MTL_COLOR:
			gglDisable( GL_TEXTURE_2D );
			gglColor4fv( m->color );
			break;
		case MTL_TEXTURE_STATIC:
			gglEnable( GL_TEXTURE_2D );
			gglBindTexture( GL_TEXTURE_2D, m->img->texhandle );
			break;
		case MTL_COLORMASK:
			gglEnable( GL_TEXTURE_2D );
			gglBindTexture( GL_TEXTURE_2D, m->img->texhandle );
			gglColor4fv( m->color );
			break;
	
		case MTL_MASKED_TEXTURE:
		case MTL_MASKED_TEXTURE_COLOR:
			if ( !m->img || !m->mask )
				goto do_default2;

			gglActiveTexture( GL_TEXTURE0 );
			gglEnable( GL_TEXTURE_2D );
			gglBindTexture( GL_TEXTURE_2D, m->img->texhandle );
			gglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

			gglActiveTexture( GL_TEXTURE1 );
			gglEnable( GL_TEXTURE_2D );
			gglBindTexture( GL_TEXTURE_2D, m->mask->texhandle );
			gglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

			if ( type == MTL_MASKED_TEXTURE_COLOR )
				gglColor4fv( m->color );

			gglBegin( GL_QUADS );
		    gglMultiTexCoord2f( GL_TEXTURE0, m->s[0], m->t[0] );
		    gglMultiTexCoord2f( GL_TEXTURE1, m->s[0], m->t[0] );
	        gglVertex3f( v[0], v[1], 0.f );
		    gglMultiTexCoord2f( GL_TEXTURE0, m->s[1], m->t[1] );
		    gglMultiTexCoord2f( GL_TEXTURE1, m->s[1], m->t[1] );
	        gglVertex3f( v[2], v[3], 0.f );
		    gglMultiTexCoord2f( GL_TEXTURE0, m->s[2], m->t[2] );
		    gglMultiTexCoord2f( GL_TEXTURE1, m->s[2], m->t[2] );
	        gglVertex3f( v[4], v[5], 0.f );
		    gglMultiTexCoord2f( GL_TEXTURE0, m->s[3], m->t[3] );
		    gglMultiTexCoord2f( GL_TEXTURE1, m->s[3], m->t[3] );
	        gglVertex3f( v[6], v[7], 0.f );
		    gglEnd();
	
    		gglActiveTexture( GL_TEXTURE1 );
    		gglDisable( GL_TEXTURE_2D );
    		gglActiveTexture( GL_TEXTURE0 );
    		gglDisable( GL_TEXTURE_2D );

			break;
		default:
do_default2:
			gglDisable( GL_TEXTURE_2D );
			gglColor4f( 0.50f, 0.3f, 0.1f, 0.7f );
			break;
		}

		// standard draw call
		if ( type == MTL_NONE ) {
			gglBegin( GL_QUADS );
        	gglVertex3f( v[0], v[1], 0.f );
	        gglVertex3f( v[2], v[3], 0.f );
   		    gglVertex3f( v[4], v[5], 0.f );
	        gglVertex3f( v[6], v[7], 0.f );
		    gglEnd();
		} else if ( type != MTL_MASKED_TEXTURE && 
					type != MTL_MASKED_TEXTURE_COLOR ) {
			gglBegin( GL_QUADS );
	    	gglTexCoord2f( m->s[0], m->t[0] );
        	gglVertex3f( v[0], v[1], 0.f );
	    	gglTexCoord2f( m->s[1], m->t[1] );
	        gglVertex3f( v[2], v[3], 0.f );
		    gglTexCoord2f( m->s[2], m->t[2] );
   		    gglVertex3f( v[4], v[5], 0.f );
		    gglTexCoord2f( m->s[3], m->t[3] );
	        gglVertex3f( v[6], v[7], 0.f );
		    gglEnd();
		}

		// Text
		// - name
		// - entType
		// - entClass
    
		// draw each in entList
		if ( r_drawColMod->integer() ) {
			R_DrawColModel( world.entList.data[entIndex] );
		}

		//
		// entity specific stuff
		//

		// ROBOT
		Robot_t *R;
		if ( r_drawLOS->integer() ) {
			if ( (R = dynamic_cast<Robot_t*>(world.entList.data[entIndex])) ) {
				gglDisable( GL_TEXTURE_2D );
				gglColor4f( 1.f, 0.f, 0.f, 0.8f );
				for ( int k = 0; k < R->los.length(); k++ ) {
					GL_DRAW_QUAD_4V( R->los.data[ k ].v ) ;
				}
				gglEnable( GL_TEXTURE_2D );
			}
		}

        // Zombie
		Zombie_t *Z;
		if ( r_drawLOS->integer() ) {
			if ( (Z = dynamic_cast<Zombie_t*>(world.entList.data[entIndex])) ) {
				gglDisable( GL_TEXTURE_2D );
				gglColor4f( 1.f, 0.f, 0.f, 0.8f );
				for ( int k = 0; k < Z->los.length(); k++ ) {
					GL_DRAW_QUAD_4V( Z->los.data[ k ].v ) ;
				}
				gglEnable( GL_TEXTURE_2D );
			}
		}

		// next
		++entIndex;
    }

	// draw the player's colModel too if..
	if ( r_drawColMod->integer() ) {
		R_DrawColModel( &player );
	}
	gglDisable( GL_TEXTURE_2D );
}


/*
====================
 R_DrawBeforeEntities
====================
*/
static unsigned int r_effectsStart;
static unsigned int r_entsLength;
void R_DrawBeforeEntities( void ) {
	r_entsLength = world.entList.length();
	int index = r_entsLength - 1;
	while ( index >= 0 ) { 
		if ( EC_EFFECT != world.entList.data[ index ]->entClass ) {
			r_effectsStart = index + 1;
			break;
		}
		--index;
	}
	R_DrawEnts( 0, r_effectsStart );
}

/*
====================
 R_DrawAfterEntities
====================
*/
void R_DrawAfterEntities( void ) {
	if ( r_entsLength != 0 && r_effectsStart != r_entsLength )
		R_DrawEnts( r_effectsStart, r_entsLength );
}


/*
====================
 R_DrawPlayer
====================
*/
void R_DrawPlayer( const char *which ) {
	material_t *m = player.getMat();
	if ( !m ) m = ent_notex;
	if ( !m->img ) m->img = ent_notex->img;

	float v[8];
	COPY8( v, player.obb );

    if ( ent_lerp->integer() ) {
	    // get adjusted coord
	    float *l = R_LerpEnt( player.lerp );
	    // take the difference
	    float diff[2] = { l[0] - player.poly.x, l[1] - player.poly.y };
	    // and add it in
	    OBB_add( v, diff );
    }

	// adjust to screen if 
	if ( which[0] == 's' ) {
		M_WorldToScreen( &v[0] );
		M_WorldToScreen( &v[2] );
		M_WorldToScreen( &v[4] );
		M_WorldToScreen( &v[6] );
	}
	
	gglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
	gglEnable( GL_TEXTURE_2D );
	gglBindTexture( GL_TEXTURE_2D, m->img->texhandle );
	gglBegin( GL_QUADS );
	gglTexCoord2f( m->s[0], m->t[0] );
	gglVertex3f( v[0], v[1], 0.f );
	gglTexCoord2f( m->s[1], m->t[1] );
	gglVertex3f( v[2], v[3], 0.f );
	gglTexCoord2f( m->s[2], m->t[2] );
	gglVertex3f( v[4], v[5], 0.f );
	gglTexCoord2f( m->s[3], m->t[3] );
	gglVertex3f( v[6], v[7], 0.f );
	gglEnd();
}

static void R_DrawCompass( void ) {
    if ( platform_type->integer() != 2 )
        return;

    static material_t *ctex;
    if ( !ctex ) {
		ctex = materials.FindByName( "gfx/MISC/compass.tga" );
    }
    if ( !ctex ) 
        return;

	material_t *m = ctex;
    float v[8];
    poly_t p;
    float w = M_Width();
    float h = M_Height();
    p.x = 14.6f / 16.f * w;
    p.y = 11.6f / 13.f * h;
    p.w = 1.f / 16.0f * w;
    p.h = p.w;
	p.angle = 0.f;
	p.toOBB( v );
    float c[2];
    p.getCenter( c );

    gglTranslatef( c[0], c[1] , 0.f );
    gglRotatef( r_angle, 0.f, 0.f, -1.f );
    gglTranslatef( -c[0], -c[1] , 0.f );

	gglEnable( GL_TEXTURE_2D );
	gglBindTexture( GL_TEXTURE_2D, ctex->img->texhandle );
	gglBegin( GL_QUADS );
	gglTexCoord2f( m->s[0], m->t[0] );
	gglVertex3f( v[0], v[1], 0.f );
	gglTexCoord2f( m->s[1], m->t[1] );
	gglVertex3f( v[2], v[3], 0.f );
	gglTexCoord2f( m->s[2], m->t[2] );
	gglVertex3f( v[4], v[5], 0.f );
	gglTexCoord2f( m->s[3], m->t[3] );
	gglVertex3f( v[6], v[7], 0.f );
	gglEnd();
	gglDisable( GL_TEXTURE_2D );
}

/*
====================
 PollFPS
====================
*/
static float r_fps = 0.f;
static int r_frames = 0;
static void PollFPS( void ) {
	static int last = 0;
	int now = Com_Millisecond();

	++r_frames;

	if ( now == last )
		return;

	if ( now - last > 1000 ) {

		int diff = now - last;
		last = now;

		r_fps = (float)r_frames / (float)diff * 1000.f;
		r_frames = 0;
	}
}

/*
====================
 R_SkipFrame
====================
*/
int R_SkipFrame( void ) {
	if ( r_maxfps->integer() == 0 )
		return false;

	PollFPS();

	static int drop_inc = r_maxfps->integer();

	if ( r_fps > r_maxfps->value() ) {
		if ( --drop_inc < 1 )
			drop_inc = 1;

	} else if ( r_fps < r_maxfps->value() )
		drop_inc++;

	if ( drop_inc && !( r_frames % drop_inc ) ) { 
	//	--r_frames;
		return true;
	}

	return false;
}

/*
====================
 R_DrawMapTileColModels
====================
*/
void R_DrawMapTileColModels( void ) {
	colModel_t **cmpp = world.colList.data;
	const unsigned int len = world.colList.length();
	colModel_t **start = cmpp;
	colModel_t & pcm = *player.col;
	while ( cmpp - start < len ) {
		R_DrawTileColModel( *cmpp++ );
	}
}

/*
====================
 R_StartFrameLerp
====================
*/
void R_StartFrameLerp( void ) {
    // IF lerp is ON and we're NOT teleporting 
	if ( tile_lerp->integer() && !r_inteleport ) {
/*
		float *adj = R_LerpFrame( camera.lerp );
		gglPushMatrix();
		gglTranslatef( adj[0] - *camera.x, adj[1] - *camera.y, 0.f );
*/
        float *lerp_adjust = SV_LerpTile();
        gglPushMatrix();
        gglTranslatef( lerp_adjust[0], lerp_adjust[1], 0.f );
	}
}

/*
====================
 R_EndFrameLerp
====================
*/
void R_EndFrameLerp( void ) {
	if ( tile_lerp->integer() && !r_inteleport ) {
		gglPopMatrix();
	}
	r_inteleport = false;
}

/*
====================
 R_DrawGameFrame
====================
*/
static void R_DrawGameFrame( void ) {
//	if ( R_SkipFrame() )
//		return;

	R_LocalInits();

	// build the lists used for drawing, thinking and collision detection
	world.BuildFrameLists();

	// prepare
	gglClear( GL_COLOR_BUFFER_BIT );
	R_ResetForDrawing( platform_type->integer() - 1 );

	// translate the whole world, interpolating for camera movement
	R_StartFrameLerp();

	// backgrounds & dispTiles  
	R_DrawDispTiles();

	// tile colModels debug mode 
	if ( r_drawColMod->integer() ) {
		R_DrawMapTileColModels();
	}

	// Entities that draw before player
	R_DrawBeforeEntities();

    if ( platform_type->integer() == 2 )
	    R_ResetForDrawing(0);
	
	// player 
	R_DrawPlayer( "world" );

    if ( platform_type->integer() == 2 )
	    R_ResetForDrawing(1);

	// entities that draw on top
	R_DrawAfterEntities();

	//R_DrawAfterTiles();

	// pop the gl matrix
	R_EndFrameLerp();

	//DEBUG_Print();

	gglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
	gglDisable( GL_TEXTURE_2D );

	if ( world.doPrintBlocks || r_printBlocks->integer() ) {
		world.printBlocks(); // calls R_PrintBlocks w/ proper args
	}

    // compass
    CL_ModelViewInit2D();
    R_DrawCompass();


	// text
    CL_ModelViewInit2D();
	floating.DrawCoords();


	// in-game console
    if ( r_drawGameCon->integer() ) {
    //	gglColor4f( 1.0f, 0.5f, 0.1f, 1.0f );
    //	gglColor4f( 0.2f, 0.65, 0.1f, 1.0f );
    	gglColor4f( 0.0f, 0.65, 0.0f, 0.85f );
	    console.drawScreenBuffer();
    }

	// fadeout
	if ( effects.active() ) {
		effects.draw();
		// draw player over fade
		gglEnable( GL_TEXTURE_2D );
		gglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
		R_DrawPlayer( "screen" );
		gglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	}

	// console
	R_DrawConsole();
}

/*
====================
 R_DrawFrame
====================
*/
void R_DrawFrame( void ) {
	switch ( cls.state ) {
	case CS_PLAYMAP:
		R_DrawGameFrame();
		break;
	}
}


