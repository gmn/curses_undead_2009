// cl_controller.cpp

#include "../map/m_area.h"			// world_t
#include "cl_local.h" 				// camera_t
#include "../renderer/r_public.h"	// renderer_t
#include "../map/mapdef.h"

Controller_t controller;

extern World_t world;
extern Camera_t camera;

World_t & 			Controller_t::world = 		::world;
Camera_t & 			Controller_t::camera = 		::camera;
//Renderer_t & 		Controller_t::render = 		::render;
main_viewport_t & 	Controller_t::viewport = 	::main_viewport;



/*
====================
 Controller_t::find
====================
*/
// find block by telling it current block and spatial relationship
Block_t * Controller_t::find( Block_t *bp, int want ) {
	const int bx = world.curArea->bx;
	const int by = world.curArea->by;
	Block_t **bpp = world.curArea->blocks.data;
	int index = 0;
	const unsigned int length = world.curArea->blocks.length();
	for ( ; index < length; index++ ) {
		if ( bpp[ index ] == bp ) {
			break;
		}
	}
	if ( index == length ) {
		return NULL;
	}
	
	switch ( want ) {
	case C_U:
		if ( index + bx >= length )
			return NULL;
		return bpp[ index + bx ];
	case C_R:
		if ( index + 1 >= length )
			return NULL;
		return bpp[ index + 1 ];
	case C_D:
		if ( index - bx < 0 )
			return NULL;
		return bpp[ index - bx ];
	case C_L:
		if ( index - 1 < 0 )
			return NULL;
		return bpp[ index - 1 ];
	case C_UR:
		if ( index + bx + 1 >= length )
			return NULL;
		return bpp[ index + bx + 1 ];
	case C_DR:
		if ( index - bx + 1 < 0 )
			return NULL;
		return bpp[ index - bx + 1 ];
	case C_UL:
		if ( index + bx - 1 >= length )
			return NULL;
		return bpp[ index + bx - 1 ];
	case C_DL:
		if ( index - bx - 1 < 0 )
			return NULL;
		return bpp[ index - bx - 1 ];
	}
	return NULL;
}

/*
====================
 Controller_t::move
====================
*/
int Controller_t::move( float mv_x, float mv_y ) {

	bool camera_move = true;

	if ( !world.curArea || !world.curBlock ) {
		console.Printf( "no curArea, can't move!" );
		console.dumpcon( "console.DUMP" );
		goto skipPlayerMove;
	}

	// check if player teleported. give view the chance to update 
	if ( tele.on ) {
		teleportViewChange();
	}

	// no move
	if ( ISZERO(mv_x) && ISZERO(mv_y) )
		goto skipPlayerMove;

	//
	// validate viewport move
	//
	float viewwish[4] = { 
		main_viewport.world.x + mv_x, 
		main_viewport.world.y + mv_y, 
		main_viewport.world.x + M_Width() * M_Ratio() + mv_x,
		main_viewport.world.y + M_Height() * M_Ratio() + mv_y
	};

	// if the viewport already IS outside the boundary 
	if ( 	main_viewport.world.x < world.curArea->boundary[0] || 
			main_viewport.world.y < world.curArea->boundary[1] || 
			main_viewport.world.x + M_Width() * M_Ratio() >= world.curArea->boundary[2] || 
			main_viewport.world.y + M_Height() * M_Ratio() >= world.curArea->boundary[3] ) {

		// and the move brings it towards the inside, allow it, else forbid it
		if (viewwish[0] >= world.curArea->boundary[0] && 
			viewwish[1] >= world.curArea->boundary[1] && 
			viewwish[2] < world.curArea->boundary[2] && 
			viewwish[3] < world.curArea->boundary[3] ) {
			camera_move = true;
		} else {
			camera_move = false;
		}
	}
	// viewport isn't allowed to move outside the area boundary
	else if (viewwish[0] < world.curArea->boundary[0] || 
			viewwish[1] < world.curArea->boundary[1] || 
			viewwish[2] >= world.curArea->boundary[2] || 
			viewwish[3] >= world.curArea->boundary[3] ) {
		camera_move = false;
	}

	// player move inside area?
	if ( 	player.wish[0] < world.curArea->boundary[0] || 
			player.wish[1] < world.curArea->boundary[1] || 
			player.wish[2] >= world.curArea->boundary[2] || 
			player.wish[3] >= world.curArea->boundary[3] ) {
		goto skipPlayerMove;
	}
	
	// get ranges of curBlock
	float ranges[4] = { 	world.curBlock->p1[0], 
							world.curBlock->p1[1], 
							world.curBlock->p2[0], 
							world.curBlock->p2[1] };

	Block_t *curBlkSav = world.curBlock;	

	// left
	Block_t *B = NULL;
	if ( viewwish[0] < ranges[0] ) {
		// down
		if ( viewwish[1] < ranges[1] ) {
			world.curBlock = find( world.curBlock, C_DL );
		// up
		} else if ( viewwish[1] > ranges[3] ) {
			world.curBlock = find( world.curBlock, C_UL );
		// just left	
		} else {
			world.curBlock = find( world.curBlock, C_L );
		}
	// right
	} else if ( viewwish[0] > ranges[2] ) {
		// down
		if ( viewwish[1] < ranges[1] ) {
			world.curBlock = find( world.curBlock, C_DR );
		// up
		} else if ( viewwish[1] > ranges[3] ) {
			world.curBlock = find( world.curBlock, C_UR );
		// just right	
		} else {
			world.curBlock = find( world.curBlock, C_R );
		}
	// up
	} else if ( viewwish[1] > ranges[3] ) {
		world.curBlock = find( world.curBlock, C_U );
	// down
	} else if ( viewwish[1] < ranges[1] ) {
		world.curBlock = find( world.curBlock, C_D );
	}

	//
	if ( world.curBlock != curBlkSav ) {
		//world.prevBlock = curBlkSav;
		world.force_rebuild = true;
	}

	// this is probably safe because we've ensured that the player move is within
	// the area.  that should be good enough.  as long as you don't make careless maps.
	if ( !world.curBlock ) {
		world.curBlock = curBlkSav;
	}
//	Assert( world.curBlock != NULL );

	// keep it here too
	player.block = world.curBlock;

	// do the move
	player.move( mv_x, mv_y );

	goto dontSkipPlayerMove;

skipPlayerMove:
	camera_move = 0;

dontSkipPlayerMove:

	// notify camera 
	if ( camera_move ) {
		camera.update( CE_PMOVE, mv_x, mv_y );
	} else {
		// still have to update it 
		camera.update( 0, 0.f, 0.f );
	}
	return 1;
}

/*
====================
 Controller_t::checkCurBlock
====================
*/
void Controller_t::checkCurBlock( void ) {
//	player
}

/*
====================
 Controller_t::togglePrintBlocks
====================
*/
void Controller_t::togglePrintBlocks( void ) { 
	doPrintBlocks = !doPrintBlocks; 
	world.setPrintBlocks( doPrintBlocks );
}

/*
====================
 Controller_t::setupGameState
====================
*/
// - setup camera
// - set camera starting point
// - set current block
// - set current area
void Controller_t::setupGameState( void ) {

	/*
	the viewport will go in .. controller.  Camera controls viewport.
	we don't technically have to even use a viewport.  

	but I think using one is a good idea.  the problem is well, world.x, world.y, which
	are part of viewport.  then again, who cares.  if you don't like it, store pointers to
	them in camera.

	think about how you used to use it.  You would use, x, y, and ratio.  and that's about
	it.  but you would convert between screen & world a lot.  and this has to do with resolution,
	viewport width, all that crap.

	another thing, I need to put a starting spawn entity into map editor, which stores the 
	area, the x & y, and then put a routine here, to find the block those coords are member
	of and set that block  as curBlock, that area as curArea.

	alternately, if none is given, like right now, just find the first block that's got anything
	in it and set that.

	*/

	// Set World curArea
	Spawn_t *WorldSpawn = NULL;
	if ( !world.curArea ) {		
		Assert( world.areas.length() > 0 ) ;

		for ( int i = 0; i < world.areas.length() ; i++ ) {
			Area_t *A = world.areas.data[i];
			node_c<Entity_t*> *n = A->entities.gethead();
			Spawn_t *S = NULL;
			while ( n ) {
				if ( (S = dynamic_cast<Spawn_t*>(n->val)) && S->type == ST_WORLD ) {
					world.curArea = A;
					WorldSpawn = S;
					break;
				}
				n = n->next;
			}
			if ( world.curArea && WorldSpawn )
				break;
		}
		Assert( world.curArea && WorldSpawn && "couldn't find WorldSpawn to set curArea" );
	}

	// Set World curBlock
	if ( !world.curBlock && WorldSpawn ) {
		world.curBlock = WorldSpawn->blockLookup( (void*)world.curArea );
	} 

	// still don't have curBlock, just take first block with stuff in it
	if ( !world.curBlock ) {
		Block_t **bpp = world.curArea->blocks.data;
		int len = world.curArea->blocks.length();
		const Block_t **start = const_cast<const Block_t**>(bpp);

		while ( !(*bpp)->isOccupied() && bpp - start < len ) {
			++bpp;
		}

		if ( (*bpp)->isOccupied() ) {
			world.curBlock = *bpp;
			viewport.world.x = world.curBlock->p1[0];
			viewport.world.y = world.curBlock->p1[1];	
		} else {
			world.curBlock = NULL;
		}
	}



	if ( world.curBlock == NULL )
		Err_Fatal( "Map appeared to have nothing in it.  Dieing badly" );

	world.prevArea = NULL;
	world.prevBlock = NULL;

	// learns about the viewport
	camera.init();
}

// 
int Controller_t::cameraMove( float mv_x, float mv_y ) {
	float wish[2] = { main_viewport.world.x + mv_x, 
					main_viewport.world.y + mv_y };
	// can't be outside boundary
	if ( 	wish[0] < world.curArea->boundary[0] || 
			wish[1] < world.curArea->boundary[1] || 
			wish[0] > world.curArea->boundary[2] || 
			wish[1] > world.curArea->boundary[3] ) {
		return 0;
	}
	main_viewport.world.x += mv_x;
	main_viewport.world.y += mv_y;
	return 1;
}

/* inform the controller that the player caused a view change by telelporting */
void Controller_t::teleportViewChange( void ) {
	float dx = tele.dx;
	float dy = tele.dy;
	Spawn_t *S = (Spawn_t*) tele.spawn;
	Area_t *A = (Area_t*) tele.area;

	// World Spawn, set viewport and zoom
	if ( S->type == ST_WORLD && S->set_view ) {
		main_viewport.SetView( S->view[0], S->view[1], S->zoom );
	} 

	// spawn has it's own view
	else if ( S->set_view ) {
		main_viewport.SetView( S->view[0], S->view[1] );

	} else {
		float v[4];
		v[0] = S->poly.x + 0.5f * S->poly.w;
		v[1] = S->poly.y + 0.5f * S->poly.h;
		v[0] += M_Width() * -0.5f * M_Ratio();
		v[1] += M_Height() * -0.5f * M_Ratio();

		// top-right viewport edge (estimated)
		v[2] = v[0] + M_Width() * M_Ratio();
		v[3] = v[1] + M_Height() * M_Ratio();

		// check if off Area.. and adjust automatically
		if ( v[2] >= A->boundary[2] ) { 
			v[2] = A->boundary[2] - 1.0f;
			v[0] = v[2] - M_Width() * M_Ratio();
		}
		if ( v[3] >= A->boundary[3] ) {
			v[3] = A->boundary[3] - 1.0f;
			v[1] = v[3] - M_Height() * M_Ratio();
		}
		if ( v[0] < A->boundary[0] ) {
			v[0] = A->boundary[0];
			v[2] = v[0] + M_Width() * M_Ratio();
		}
		if ( v[1] < A->boundary[1] ) {
			v[1] = A->boundary[1];
			v[3] = v[1] + M_Height() * M_Ratio();
		}


//		Assert( v[0] >= A->boundary[0] && v[0] < A->boundary[2] );
//		Assert( v[1] >= A->boundary[1] && v[1] < A->boundary[3] );
//		Assert( v[2] >= A->boundary[0] && v[2] < A->boundary[2] );
//		Assert( v[3] >= A->boundary[1] && v[3] < A->boundary[3] );

		main_viewport.SetView( v[0], v[1] );
	}		

	// must force a block rebuild
	world.force_rebuild = true;
	tele.on = 0;

	// change world state
	Block_t *B = S->blockLookup( A );
	world.setCurBlock( B );
	world.setCurArea( A );
}

/*
====================
 Controller_t::notifyTeleport
====================
*/
void Controller_t::notifyTeleport( float dx, float dy, Spawn_t *S, Area_t *A ) {
	tele.dx = dx;
	tele.dy = dy;
	tele.spawn = (void*) S;
	tele.area = (void*) A;
	tele.on = 1;
}
