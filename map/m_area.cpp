// m_area.cpp
//

#include "../mapedit/mapedit.h"
#include "../lib/lib_list_c.h"
#include "../client/cl_console.h"
#include "m_area.h"
#include "../renderer/r_public.h"

extern buffer_c<char*> gameDef;

World_t world;

/*
================================================================================

	mapFile_t

================================================================================
*/
/* autostarters for mapFile_t */
void mapFile_t::_my_reset( void ) {
	name[ 0 ] = 0;
	mapTiles.delete_reset();
	entities.delete_reset();
	// areas' backgrounds aren't converted yet, which means we _have_ to deal with them ourselves to be safe
	for ( int i = 0; i < areas.length(); i++ ) {
		areas.data[i]->backgrounds.destroy();
	}
	areas.delete_reset();
	subAreas.delete_reset();
}
void mapFile_t::_my_init( void ) {
	mapTiles.init( 2048 );
	entities.init( 2048 / 4 );
	areas.init( 32 );
	subAreas.init( 32 );
}
void mapFile_t::_my_destroy( void ) {
	for ( int i = 0; i < mapTiles.length(); i++ ) {
		delete mapTiles.data[i];
		mapTiles.data[i] = NULL;
	}
	mapTiles.destroy(); 
	
	for ( int i = 0; i < entities.length(); i++ ) {
		delete entities.data[i];
		entities.data[i] = NULL;
	}
	entities.destroy();
	
	for ( int i = 0; i < areas.length(); i++ ) {
		areas.data[i]->backgrounds.destroy();
		delete areas.data[i];
		areas.data[i] = NULL;
	}
	areas.destroy();

	for ( int i = 0; i < subAreas.length(); i++ ) {
		delete subAreas.data[i];
		subAreas.data[i] = NULL;
	}
	subAreas.destroy();
}


/*
================================================================================

	World_t

================================================================================
*/
/*
====================
 CreateDefaultArea

	used to create 1 area for all tiles when one isn't provided
====================
*/
void World_t::CreateDefaultArea( mapFile_t const & mapFile ) {
	float 	low[2] = { 0.f, 0.f }, 
			high[2] = { 0.f, 0.f };

	if ( 0 == mapFile.mapTiles.length() )
		return;

	int i = 0;
	mapTile_t *tile = mapFile.mapTiles.data[ i++ ];
	
	low[ 0 ] = tile->poly.x;
	low[ 1 ] = tile->poly.y;
	high[ 0 ] = tile->poly.x + tile->poly.w;
	high[ 1 ] = tile->poly.y + tile->poly.h;

	for ( ; i < mapFile.mapTiles.length(); i++ ) {
		tile = mapFile.mapTiles.data[ i ];
		// backgrounds don't figure in to Area size.  They can be any size
		//  at discretion of the map creator
		if ( tile->background ) {
			continue;
		}

		if ( tile->poly.x < low[ 0 ] ) {
			low[ 0 ] = tile->poly.x;
		}
		if ( tile->poly.y < low[ 1 ] ) {
			low[ 1 ] = tile->poly.y;
		}
		if ( tile->poly.x + tile->poly.w > high[ 0 ] ) {
			high[ 0 ] = tile->poly.x + tile->poly.w;
		}
		if ( tile->poly.y + tile->poly.h > high[ 1 ] ) {
			high[ 1 ] = tile->poly.y + tile->poly.h;
		}
	}
	Entity_t *e;
	for ( i = 0; i < mapFile.entities.length(); i++ ) {
		e = mapFile.entities.data[ i ];
		poly_t *p = &e->poly;
		if ( e->poly.x < low[ 0 ] ) {
			low[ 0 ] = e->poly.x;
		}
		if ( e->poly.y < low[ 1 ] ) {
			low[ 1 ] = e->poly.y;
		}
		if ( p->x + p->w > high[ 0 ] ) {
			high[ 0 ] = p->x + p->w;
		}
		if ( p->y + p->h > high[ 1 ] ) {
			high[ 1 ] = p->y + p->h;
		}
	}

	Area_t *A = new Area_t( &entPool );
	
	// maybe it's better to make a deliberately shitty auto-generated area
	// so the mapper is reminded there is another way..  Or perhaps its just
	// good to do everything as excellently as possible thats automatic. 
	// yeah, that.
	float pad[2] = { 1600 * 6.0f, 1200 * 6.0f };
	A->p1[ 0 ] = low[ 0 ] 	- pad[0];
	A->p1[ 1 ] = low[ 1 ]	- pad[1];
	A->p2[ 0 ] = high[ 0 ] 	+ pad[0];
	A->p2[ 1 ] = high[ 1 ] 	+ pad[1];
	sprintf( A->name, "%s_default_area--auto_generated", mapFile.name );

	areas.add( A );
	this->default_area = true;
}

/*
======================
 BuildWorldFromMapFile

transfer globals from World.mapFile and populate Area_t->...
======================
*/
void World_t::BuildWorldFromMapFile( mapFile_t const &mapFile ) 
{
	// Area_t
	// detect Areas, if there is no Area, create 1 for all the mapTiles
	const mapFile_t & mf = mapFile;
	if ( 0 == mf.areas.length() ) {
		CreateDefaultArea( mapFile );
	} else {
		// copy in areas there are
		for ( int i = 0; i < mf.areas.length(); i++ ) {
			areas.add( new Area_t( *mf.areas.data[ i ], &entPool ) );

			// just copy over the already created and initialized area from
			// mapFile.  only, make sure to destroy it afterwards, so they
			// don't get freed later.
			// areas.add( mf.areas.data[ i ] );
		}
	}

	// copy subArea_t
	list_c<subArea_t*> 		sub_tmp;
	sub_tmp.init();
	for ( int i = 0; i < mf.subAreas.length(); i++ ) {
		sub_tmp.add( new subArea_t( *mf.subAreas.data[i]) );
	}

	// copy dispTile, colModel
	list_c<dispTile_t*> 	dis_tmp;
	list_c<colModel_t*> 	col_tmp;
	dis_tmp.init();
	col_tmp.init();

	// create deap copies of dispTiles & colModels 
	for ( int i = 0; i < mapFile.mapTiles.length(); i++ ) {
		mapTile_t *T = mapFile.mapTiles.data[ i ];

		if ( T->col ) {
			// some colModels wind up 0,0,0,0. reject them.
			if (  ( !ISZERO(T->col->box[0]) ||	
					!ISZERO(T->col->box[1]) ||	
					!ISZERO(T->col->box[2]) || 
					!ISZERO(T->col->box[3]) ) &&
				// some wind up being a single point x,y,x,y; also reject
				  ( T->col->box[0] != T->col->box[2] ||
					T->col->box[1] != T->col->box[3] ) ) 
				col_tmp.add( new colModel_t( *T->col ) );
		}

		// skip backgrounds, we get them at the end
		if ( !T->background ) {
			// dispTile constructor can take mapTile_t&
			dis_tmp.add( new dispTile_t( *T ) );
		}
	}

	// copy Entity_t
	list_c<Entity_t*>		etmp;
	etmp.init();

	// create list of entities copied over from mapFile
	for ( int i = 0; i < mapFile.entities.length(); i++ ) {
		Entity_t *ent = mapFile.entities.data[ i ];
		etmp.add( ent->Copy() ) ;
	}

	// FOR EACH AREA
	for ( int i = 0; i < areas.length(); i++ ) 
	{
		Area_t *A = areas.data[ i ];

		// subAreas
		//	FIXME: unfinished!

		// dispTiles
		// foreach mapTile if the tile is inside the Area, 
		// 	put dispTile & colModel, check if background  
		node_c<dispTile_t*> *Dnode = dis_tmp.gethead();
		while ( Dnode ) {
			dispTile_t * D = Dnode->val;
			// IS INSIDE AREA
			if ( 	D->poly.x >= A->p1[ 0 ] && D->poly.x <= A->p2[ 0 ] && 
					D->poly.y >= A->p1[ 1 ] && D->poly.y <= A->p2[ 1 ] ) 
			{
				// add to area
				A->dispTiles.add( D ); 
				// pop from tile list
				node_c<dispTile_t*> *tmp = Dnode->next;
				dis_tmp.popnode( Dnode );
				Dnode = tmp;
			} else {
				Dnode = Dnode->next;
			}
		}

		// colModels
		node_c<colModel_t*> *C = col_tmp.gethead();
		while ( C ) {
			AABB_t aabb = { A->p1[0], A->p1[1], A->p2[0], A->p2[1] };
			if ( C->val->boxIntersect( aabb ) ) {
				A->colModels.add( C->val );
				node_c<colModel_t*> *Cnext = C->next;
				col_tmp.popnode( C );
				C = Cnext;
				continue;
			}
			C = C->next;
		}

		// Entities
		// foreach entity, if inside of Area, put it
		node_c<Entity_t*> *En = etmp.gethead();
		while ( En ) {
			Entity_t *E = En->val;
			if ( 	E->poly.x >= A->p1[0] && E->poly.x <= A->p2[0] &&
					E->poly.y >= A->p1[1] && E->poly.y <= A->p2[1] ) 
			{
				node_c<Entity_t*> *e_next = En->next;
				etmp.popnode( En );
				A->entities.add( E );
				En = e_next;
			} else {
				En = En->next;
			}
		}
	}

	// 
	sub_tmp.destroy();
	dis_tmp.destroy();
	col_tmp.destroy();
	etmp.destroy();

	/* SAVE, Maybe later
	// populate Area HashTable
	for ( int i = 0; i < areas.length(); i++ ) {
		Area_t *A = areas.data[ i ];
		//ahash.Insert( A, A->name, AREA_NAME_SIZE-1 ); //cant do this because
		// we have no assurance of what garbage characters may lurk after the
		// name. it might make a hash lookup would be completely impossible
		ahash.Insert( A, A->name, strlen(A->name) );
	} 
	*/

	// backgrounds are special:
	// if a default area was created or if there is only 1 area: all backgrounds go in it
	if ( default_area || areas.length() == 1 ) {
		for ( int i = 0; i < mapFile.mapTiles.length(); i++ ) {
			mapTile_t *T = mapFile.mapTiles.data[ i ];
			if ( T->background ) {
				areas.data[0]->backgrounds.add( new dispTile_t( *T ) );
			}
		}
	// otherwise we get background's Area membership from mapFile
	// we smuggled over the dispTile UIDs of the backgrounds in Area_t::backgrounds
	} else {
		// for each area, 
		for ( int i = 0; i < areas.length(); i++ ) {
			Area_t *A = areas.data[ i ];
			// for each background uid
			for ( int j = 0; j < A->backgrounds.length(); j++ ) {

				// get the UID
				unsigned int _uid = (unsigned int) A->backgrounds.data[ j ];

				// find the tile in mapFile
				mapTile_t *T = NULL; 
				for ( int i = 0; i < mapFile.mapTiles.length(); i++ ) {
					T = mapFile.mapTiles.data[ i ];
					if ( T->background && T->uid == _uid ) {
						break;
					}
				}

				// didn't find it, post an error
				if ( mapFile.mapTiles.length() == i || !T ) {
					console.Printf( "couldn't find backgroud uid in tiles list" );
					console.dumpcon( "console.DUMP" );
					Err_Warning( "couldn't find backgroud uid in tiles list" );
					A->backgrounds.data[ j ] = NULL; // it's gonna crash for sure!
					continue;
				}
				
				// found it, now replace whats in A->backgrounds w/ a good copy
				A->backgrounds.data[ j ] = new dispTile_t( *T );
			}
		}
	}
}

/*
====================
 SetupViewport

if we can find a world spawn, set the viewport location and zoom from it
called in initial Area build
====================
*/
void World_t::SetupViewport( Area_t *A ) {
	node_c<Entity_t*> *nE = A->entities.gethead();
	Spawn_t *s = NULL;
	while( nE ) {
		s = dynamic_cast<Spawn_t*>( nE->val );
		if ( s && s->type == ST_WORLD ) {
			break;
		}
		nE = nE->next;
	}
	// also set the player pos & size to match spawn poly
	if ( s && s->type == ST_WORLD ) {
		if ( s->set_view ) {
			M_GetMainViewport()->SetView( s->view[0], s->view[1], s->zoom );
		}
	}
}


/*
====================
 World_t::BuildBlocklist

	literally builds the blocks for an area.  initializes them, populates them,
	connects them.

	** NOTE:
	- removes:	Area_t::dispTiles, Area_t::colModels 
	- leaves:	Area_t::backgrounds, Area_t::entities
====================
*/
void World_t::BuildBlocklist( Area_t *A ) {
	SetupViewport( A );
	float vp[2] = { M_Width() * M_Ratio(), M_Height() * M_Ratio() };

	A->block_dim[0] = BLOCK_DIM;
	A->block_dim[1] = BLOCK_DIM;

	const float fudge = 128.0f;

	if ( vp[0] > A->block_dim[0] - fudge ) {
		A->block_dim[0] = vp[0] + fudge;
	}
	if ( vp[1] > A->block_dim[1] - fudge ) {
		A->block_dim[1] = vp[1] + fudge;
	}

	float x = A->p2[0] - A->p1[0];
	float y = A->p2[1] - A->p1[1];

	A->bx = x / A->block_dim[0] + 1;
	A->by = y / A->block_dim[1] + 1;

	const int dim = A->bx * A->by;

	A->blocks.init( dim );
	A->blocks.zero_out();

	// create each block
	for ( int i = 0; i < dim; i++ ) {
		A->blocks.add( NewBlock() );
	}

	// set the location & dimension of each block
	x = A->p1[0];
	y = A->p1[1];
	for ( int j = 0; j < A->by; j++ ) {
		x = A->p1[0];
		for ( int i = 0; i < A->bx; i++ ) {
			A->blocks.data[ j * A->bx + i ]->p1[0] = x;
			A->blocks.data[ j * A->bx + i ]->p1[1] = y;
			A->blocks.data[ j * A->bx + i ]->p2[0] = x + A->block_dim[0];
			A->blocks.data[ j * A->bx + i ]->p2[1] = y + A->block_dim[1];
			x += A->block_dim[0];;
		}
		y += A->block_dim[1];
	}

	/* 
	 * set the boundary size 
	 */
	// just the same as the Area AABB now
	A->boundary[0] = A->p1[0];
	A->boundary[1] = A->p1[1];
	A->boundary[2] = A->p2[0];
	A->boundary[3] = A->p2[1];
	
	/*
	 *
	 * populate the blocks
	 *
	 */
	
	// dispTiles
	for ( int j = 0; j < A->dispTiles.length(); j++ ) {
		// foreach block
		bool found = false;
		for ( int i = 0; i < A->blocks.length() && !found; i++ ) {
			Block_t *B = A->blocks.data[i];
			AABB_t aabb = { B->p1[0], B->p1[1], B->p2[0], B->p2[1] };
			OBB_t obb;
			AABB_TO_OBB( obb, aabb );

			// intersect the box with the tile
			if ( OBB_intersection( obb, A->dispTiles.data[j]->obb ) ) {
				// found it, save a pointer to it
				B->dispTiles.add( A->dispTiles.data[j] );
				found = true;
			}
		}
	}

	// colModels
	for ( int j = 0; j < A->colModels.length(); j++ ) {
		colModel_t *cm = A->colModels.data[ j ];
		bool found = false;
		// foreach block
		for ( int i = 0; !found && i < A->blocks.length(); i++ ) {
			Block_t *B = A->blocks.data[i];
			AABB_t aabb = { B->p1[0], B->p1[1], B->p2[0], B->p2[1] };
			// if colModel is in Block
			if ( cm->boxIntersect( aabb ) ) {
				// found its block
				B->colModels.add( A->colModels.data[j] );
				found = true;
			}
		}
	}
	
	// entities
	node_c<Entity_t*> *nE = A->entities.gethead();
	while ( nE ) {
		bool found = false;
		// foreach block
		for ( int i = 0; !found && i < A->blocks.length(); i++ ) {
			Block_t *B = A->blocks.data[i];
			AABB_t block_aabb = { B->p1[0], B->p1[1], B->p2[0], B->p2[1] };
			// if Ent's poly is in block's AABB
			if ( AABB_intersection( nE->val->aabb, block_aabb ) ) {
				Assert( nE->val->entClass < TOTAL_ENTITY_CLASSES );
				B->entities[ nE->val->entClass ].add( nE->val );
				nE->val->block = B; // set initial block in each entity
				found = true;
			}
		}

		nE = nE->next;
	}

	//
	// destroy Area_t arrays, we no longer need them
	//
	A->dispTiles.destroy();
	A->colModels.destroy();
	//  ..we keep entities..
	// ..we keep backgrounds..

	//
	// connect up directional pointers
	//
/*
 * block connecting pattern

	starting points are diagonal, starting at the ll-corner, advancing to
	 the upper-right corner.  From each starting point we trace out UP &
	 RIGHT connecting each set of pointers we find between two inhabited 
	 blocks.  Make sure to connect-up, not only the block we're on, but 
	 the block's pointer with which it is connecting to.  We do this in
	 all 4 directions for each block

. . . . .  . . . . .  x . . . .  x . . . .  x . . . .  x x . . .  x x . . .
. . . . .  . . . . .  x . . . .  x . . . .  x . . . .  x x . . .  x x . . .
. . . . .  . . . . .  x . . . .  x . . . .  x . . . .  x x . . .  x x z . .
. . . . .  . . . . .  x . . . .  x z . . .  x o o o o  x o o o o  x o o o o
z . . . .  o o o o o  o o o o o  o o o o o  o o o o o  o o o o o  o o o o o
*/

	// if a block is touching an occupied block on one of its 4 sides, link them

	Block_t * b , * q;

	int i = 0, j = 0;
	while ( i < A->bx && j < A->by ) {
		
		// go right (getting starting block as well
		int r = i;
		int s = j;
		int rt, st;
		while ( r < A->bx ) {

			b = A->blocks.data[ A->bx * s + r ];

			// check blocks in all 4 directions, getting the block_p by indexing
			//  into the array w/ i & j
			
			// if a connecting block is inhabited, connect the two matching
			// 	connecting pointers UP+DOWN, RIGHT+LEFT, LEFT+RIGHT, DOWN+UP

			// up 
			rt = r;
			st = s + 1;
			if ( rt >= 0 && rt < A->bx && st >= 0 && st < A->by) {
				q = A->blocks.data[ A->bx * st + rt ];
				b->up = q;
				q->down = b;
			}
			
			// down
			rt = r;
			st = s - 1;
			if ( rt >= 0 && rt < A->bx && st >= 0 && st < A->by) {
				q = A->blocks.data[ A->bx * st + rt ];
				b->down = q;
				q->up = b;
			}

			// left 
			rt = r - 1;
			st = s;
			if ( rt >= 0 && rt < A->bx && st >= 0 && st < A->by) {
				q = A->blocks.data[ A->bx * st + rt ];
				b->left = q;
				q->right = b;
			}

			// right
			rt = r + 1;
			st = s;
			if ( rt >= 0 && rt < A->bx && st >= 0 && st < A->by) {
				q = A->blocks.data[ A->bx * st + rt ];
				b->right = q;
				q->left = b;
			}
			
			++r; // go right one block
		}

		// go up, from one above the starting block
		r = i;
		s = j + 1;
		while ( s < A->by ) {

			b = A->blocks.data[ A->bx * s + r ];

			// up 
			rt = r;
			st = s + 1;
			if ( rt >= 0 && rt < A->bx && st >= 0 && st < A->by) {
				q = A->blocks.data[ A->bx * st + rt ];
				b->up = q;
				q->down = b;
			}
			
			// down
			rt = r;
			st = s - 1;
			if ( rt >= 0 && rt < A->bx && st >= 0 && st < A->by) {
				q = A->blocks.data[ A->bx * st + rt ];
				b->down = q;
				q->up = b;
			}

			// left 
			rt = r - 1;
			st = s;
			if ( rt >= 0 && rt < A->bx && st >= 0 && st < A->by) {
				q = A->blocks.data[ A->bx * st + rt ];
				b->left = q;
				q->right = b;
			}

			// right
			rt = r + 1;
			st = s;
			if ( rt >= 0 && rt < A->bx && st >= 0 && st < A->by) {
				q = A->blocks.data[ A->bx * st + rt ];
				b->right = q;
				q->left = b;
			}
			
			++s; // move up one row
		}

		// move starting block diagonally up & right
		++i;
		++j; 
	}


	// set block perimeters
	A->ent_perim = 3;
	A->draw_perim = 3;
	A->col_perim = 4;
}

void World_t::startMemClient( void ) {
	if ( client_id == -1 ) {
		client_id = V_RequestExclusiveZone( WORLD_MEMORY_SIZE, &idSave );
	} else {
		V_SetCurrentZone( client_id, &idSave );
	}
}
void World_t::restoreMemClient( void ) {
	Assert ( idSave != -1 && "eek, called before startMemClient!" );
	V_SetCurrentZone( idSave );
}

/*
====================
 BuildWorld

	called by CL_LoadMap
====================
*/
void World_t::BuildWorld( mapFile_t const & mapFile ) 
{
	// specify our memory area
	startMemClient();	

	// undo our structures
	this->destroy();
	
	// get virgin pages
	V_ZoneWipe( client_id );
	
	// re-connect our structures
	this->init();
	this->reset();

	// transfer data from mapFile into world
	BuildWorldFromMapFile( mapFile );

	// foreach Area_t as A : build Area blocklist
	for ( int i = 0; i < areas.length(); i++ ) {
		BuildBlocklist( areas.data[i] );
	}

	// put it back to the way it was
	restoreMemClient();
}


/*
====================
 World_t::rebuildPristine
	
	rebuilds world block lists to their initial condition from mapFile
	so far, just a synonym for BuildWorld.  I'll leave it lay around, maybe
	it will don an expanded use later
====================
*/
void World_t::rebuildPristine( mapFile_t const & mapFile ) {
	BuildWorld( mapFile );
}


/*
====================
 World_t :: Auto Starter Methods
====================
*/
void World_t::init( void ) { // allocate

	// memPools
	entPool.init();

	// short lists
	dispBlocks.init( 40 );
	entBlocks.init( 40 );
	colBlocks.init( 40 );
	dispList.init(); // big
	entList.init( 512 );
	colList.init(); // big

	// Area_t list
	areas.init( 16 );
}

void World_t::reset( void ) { // reset to virgin state

	curBlock = NULL;
	prevBlock = NULL;
	curArea = NULL;
	prevArea = NULL;

	// pool
	entPool.reset();

	// shortlist
	dispBlocks.reset();
	entBlocks.reset();
	colBlocks.reset();
	dispList.reset();
	entList.reset();
	colList.reset();

	// area
	areas.delete_reset();

	//ahash.reset();
	
	force_rebuild = true;

	// don't touch client_id!
}

void World_t::destroy( void ) { // deallocate

	entPool.destroy(); // just pointers

	// dispBlocks
	dispBlocks.destroy(); 
	// entBlocks
	entBlocks.destroy();
	// colBlocks
	colBlocks.destroy();
	// dispList
	dispList.destroy();
	// entList
	entList.destroy();
	// colList
	colList.destroy();

	// areas
	areas.delete_destroy();
}



// the block allocator is in world, because the memory pools are in world
Block_t * World_t::NewBlock() {
	Block_t *b = new Block_t();
	b->dispTiles.init( 64 );
	b->colModels.init( 64 );
	for ( int i = 0; i < TOTAL_ENTITY_CLASSES; i++ ) {
		b->entities[i].init( &entPool );
	}
	return b;
}

/*
====================
 World_t::BuildFrameLists

	called once per drawFrame, rebuilds drawing lists, and Entity lists
	that are run by the engine for thinking and collisions.  

====================
*/ 
void World_t::BuildFrameLists( void ) {
	
	Assert( curBlock != NULL );

	// test whether curBlock is different from prevBlock, if it is, or if its
	//  first run, rebuild shortlists
	if ( force_rebuild ) {
	} else if ( curBlock == prevBlock ) {
		return;
	}
	force_rebuild = false;
	
	dispBlocks.reset_keep_mem();
	entBlocks.reset_keep_mem();
	colBlocks.reset_keep_mem(); 

	// 
	// draw blocks
	// 

	// center block
	dispBlocks.add( curBlock ); 
	
	Block_t *b, *q;
	int i, j;
	const int draw_perim = curArea->draw_perim;

	// RIGHT
	b = curBlock;
	i = draw_perim;
	while ( b && b->right && i-- ) { 
		dispBlocks.add( b->right );
		
		// & UP
		q = b->right;
		j = draw_perim;
		while ( q && q->up && j-- ) {
			dispBlocks.add( q->up );
			q = q->up;
		}
		b = b->right;
	}

	// LEFT
	b = curBlock;
	i = draw_perim;
	while ( b && b->left && i-- ) { 
		dispBlocks.add( b->left );

		// & DOWN
		q = b->left;
		j = draw_perim;
		while ( q && q->down && j-- ) {
			dispBlocks.add( q->down );
			q = q->down;
		}
		b = b->left;
	}

	// UP
	b = curBlock;
	i = draw_perim;
	while ( b && b->up && i-- ) { 
		dispBlocks.add( b->up );

		// & LEFT
		q = b->up;
		j = draw_perim;
		while ( q && q->left && j-- ) {
			dispBlocks.add( q->left );
			q = q->left;
		}
		b = b->up;
	}

	// DOWN
	b = curBlock;
	i = draw_perim;
	while ( b && b->down && i-- ) { 
		dispBlocks.add( b->down );

		// & RIGHT
		q = b->down;
		j = draw_perim;
		while ( q && q->right && j-- ) {
			dispBlocks.add( q->right );
			q = q->right;
		}
		b = b->down;
	}

	// 
	// Ent Blocks
	// 

	// center
	entBlocks.add( curBlock );
	const int ent_perim = curArea->ent_perim;
	
	// right & up
	b = curBlock;
	i = ent_perim;
	while ( b && b->right && i-- ) { 
		entBlocks.add( b->right );
		q = b->right;
		j = ent_perim;
		while ( q && q->up && j-- ) {
			entBlocks.add( q->up );
			q = q->up;
		}
		b = b->right;
	}

	// left & down
	b = curBlock;
	i = ent_perim;
	while ( b && b->left && i-- ) { 
		entBlocks.add( b->left );
		q = b->left;
		j = ent_perim;
		while ( q && q->down && j-- ) {
			entBlocks.add( q->down );
			q = q->down;
		}
		b = b->left;
	}

	// up & left
	b = curBlock;
	i = ent_perim;
	while ( b && b->up && i-- ) { 
		entBlocks.add( b->up );
		q = b->up;
		j = ent_perim;
		while ( q && q->left && j-- ) {
			entBlocks.add( q->left );
			q = q->left;
		}
		b = b->up;
	}

	// down & right
	b = curBlock;
	i = ent_perim;
	while ( b && b->down && i-- ) { 
		entBlocks.add( b->down );
		q = b->down;
		j = ent_perim;
		while ( q && q->right && j-- ) {
			entBlocks.add( q->right );
			q = q->right;
		}
		b = b->down;
	}


	//
	// colModels
	//

	// center
	colBlocks.add( curBlock );
	const int col_perim = curArea->col_perim;
	
	// right & up
	b = curBlock;
	i = col_perim;
	while ( b && b->right && i-- ) { 
		colBlocks.add( b->right );
		q = b->right;
		j = col_perim;
		while ( q && q->up && j-- ) {
			colBlocks.add( q->up );
			q = q->up;
		}
		b = b->right;
	}

	// left & down
	b = curBlock;
	i = col_perim;
	while ( b && b->left && i-- ) { 
		colBlocks.add( b->left );
		q = b->left;
		j = col_perim;
		while ( q && q->down && j-- ) {
			colBlocks.add( q->down );
			q = q->down;
		}
		b = b->left;
	}

	// up & left
	b = curBlock;
	i = col_perim;
	while ( b && b->up && i-- ) { 
		colBlocks.add( b->up );
		q = b->up;
		j = col_perim;
		while ( q && q->left && j-- ) {
			colBlocks.add( q->left );
			q = q->left;
		}
		b = b->up;
	}

	// down & right
	b = curBlock;
	i = col_perim;
	while ( b && b->down && i-- ) { 
		colBlocks.add( b->down );
		q = b->down;
		j = col_perim;
		while ( q && q->right && j-- ) {
			colBlocks.add( q->right );
			q = q->right;
		}
		b = b->down;
	}


	// 3 net outcomes to this step: 
	// - colModels array ( doesn't need to be sorted )
	// - dispList array ( needs to be sorted by Layer # )
	// - entities array ( needs to be sorted by entityClass_t )

	// notes: 
	// 1-entities dont have layers. they're drawn in order of their class
	// 2-dispTiles will already be sorted by layer# in their respective blocks
	// 3-colmodels collect from the same blocks as Entities, since ents check
	//   collisions 

	
	dispList.reset_keep_mem();
	entList.reset_keep_mem();
	colList.reset_keep_mem();

	// 
	// entlist
	// 
	
	const int blen = entBlocks.length();

	// for every entity class
	for ( j = 0; j < TOTAL_ENTITY_CLASSES; j++ ) {

		// for every block
		for ( i = 0; i < blen; i++ ) {

			b = entBlocks.data[ i ];

			// get all the entities of that class IN that block
			node_c<Entity_t*> *ne = b->entities[ j ].gethead();
			while ( ne ) {
				entList.add( ne->val );
				ne = ne->next;
			}
		}
	}

	//
	// colModels
	//	
	
	// for every block
	const int clen = colBlocks.length();
	for ( i = 0; i < clen; i++ ) {
		b = colBlocks.data[ i ];
	
		// get all colModels
		colList.copy_in( b->colModels.data, b->colModels.length() );
	}
	
	//
	// dispTiles
	//
	
	const int dlen = dispBlocks.length();

	// find lowest & highest layer# of the blocks
	int low_layer = 0x7fffffff;
	for ( i = 0; i < dlen; i++ ) {
		if ( dispBlocks.data[i]->dispTiles.length() > 0 && dispBlocks.data[ i ]->dispTiles.data[0]->layer < low_layer ) {
			low_layer = dispBlocks.data[ i ]->dispTiles.data[0]->layer;
		}
	}

	int high_layer = 0x80 << 24;
	for ( i = 0; i < dlen; i++ ) {
		b = dispBlocks.data[ i ];
		if ( b->dispTiles.length() == 0 )
			continue;
		if ( b->dispTiles.data[ b->dispTiles.length()-1 ]->layer > high_layer ) {
			high_layer = b->dispTiles.data[ b->dispTiles.length()-1 ]->layer;
		}
	}

	// local layer indexes array
	static vec_c<int> ind;
	ind.reset( dlen );
	ind.zero_out();
	
	// from the lowest layer # to the highest
	for ( int layer = low_layer; layer <= high_layer; layer++ ) {

		// for every block
		for ( j = 0; j < dlen; j++ ) {
			b = dispBlocks.data[ j ];

			// start looking for layer on saved index into dispTiles
			int tile = ind.vec[ j ];
	
			// get every tile @ current layer-#
			int len = b->dispTiles.length();
			if ( tile == len )
				continue;

			// FIXME: replace the next two lines with a per dispTile check, that checks if
			//  it's even inside of the frustum


			// count tiles
			while ( tile < len && b->dispTiles.data[tile]->layer == layer ) 
				++tile;
			
			// copy in whatever we found
			if ( tile - ind.vec[j] > 0 )
				dispList.copy_in( &b->dispTiles.data[ ind.vec[j] ], tile - ind.vec[j] );

			// update block index to where to start next search
			ind.vec[ j ] = tile;
		}
	}

	//
	prevBlock = curBlock;
}

void R_PrintBlocks( buffer_c<Block_t*>& blocks, float blk[2], unsigned int bx, unsigned int by );
/*
====================
 World_t::printBlocks
====================
*/
void World_t::printBlocks( void ) { 
	if ( curArea ) {
		R_PrintBlocks( curArea->blocks, curArea->block_dim, curArea->bx, curArea->by );
	}
}





/*
================================================================================

	Area_t

================================================================================
*/
void Area_t::init( void ) {
	// blocks.init() ::> dont initialize blocks until we know how many
	subAreas.init( 32 );
	dispTiles.init();
	colModels.init();
	entities.init();
	backgrounds.init( 4 );
}
void Area_t::reset( void ) {
	p1[ 0 ] = p1[ 1 ] = p2 [ 0 ]  = p2[ 1 ] = 0.f;
	name [ 0 ] = 0 ;
	bx = by = 0;

	blocks.delete_reset();
	subAreas.delete_reset();
	dispTiles.delete_reset();
	colModels.delete_reset();

	entities.reset();
	backgrounds.delete_reset();
}
void Area_t::destroy( void ) {
	blocks.delete_destroy();
	subAreas.delete_destroy();
	dispTiles.delete_destroy();
	colModels.delete_destroy();

/*	node_c<Entity_t*> *e = entities.gethead();
	while ( e ) {
		if ( e->val )
			delete e->val ;
		e->val = NULL;
		e = e->next;
	} */

	// don't destroy individual entities, do this in Block_t instead.  
	// question: are there some entities that aren't stored in Blocks ??
	entities.destroy(); 
	backgrounds.delete_destroy();
}

Area_t::Area_t( Area_t const &A ) 
	: bx(A.bx), by(A.by)
{
	Assert( 0 && "make sure I'm not using this" && "left out dispTiles, colModels" );

	p1[0] = A.p1[0];
	p1[1] = A.p1[1]; 
	p2[0] = A.p2[0]; 
	p2[1] = A.p2[1];
	name[ 0 ] = name[ AREA_NAME_SIZE-1 ] = 0;
	strcpy( name, A.name );
	name[ AREA_NAME_SIZE-1 ] = 0;

	// blocks
	blocks.set_by_ref( A.blocks );
	for ( int i = 0; i < A.blocks.length(); i++ ) {
		blocks.data[ i ] = new Block_t( *A.blocks.data[ i ] );
	}

	// subAreas
	subAreas.set_by_ref( A.subAreas );
	for ( int i = 0; i < A.subAreas.length(); i++ ) {
		subAreas.data[ i ] = new subArea_t( *A.subAreas.data[ i ] );
	}

	// backgrounds
	backgrounds.set_by_ref( A.backgrounds );
	for ( int i = 0; i < A.backgrounds.length(); i++ ) {
		backgrounds.data[ i ] = new dispTile_t( *A.backgrounds.data[ i ] );
	}
}



/* this is NOT a copy constructor.  It is specially designed to copy in Areas
	that come from mapFile read.  unorthodox use of data structures. 
	see explanation below.
*/
Area_t::Area_t( Area_t const &A, memPool<node_c<Entity_t*> > *ep ) 
	: bx(A.bx), by(A.by)
{
	reset(); 
	subAreas.init( 32 );
	backgrounds.init( 4 );
	dispTiles.init();
	colModels.init();
	entities.init( ep );

	p1[0] = A.p1[0];
	p1[1] = A.p1[1]; 
	p2[0] = A.p2[0]; 
	p2[1] = A.p2[1];
	name[ 0 ] = name[ AREA_NAME_SIZE-1 ] = 0;
	strcpy( name, A.name );
	name[ AREA_NAME_SIZE-1 ] = 0;

	// subAreas
	subAreas.set_by_ref( A.subAreas );
	for ( int i = 0; i < A.subAreas.length(); i++ ) {
		subAreas.data[ i ] = new subArea_t( *A.subAreas.data[ i ] );
	}

	// backgrounds
	backgrounds.set_by_ref( A.backgrounds );
	for ( int i = 0; i < A.backgrounds.length(); i++ ) {
		// The special part is here.  backgrounds buffer is smuggling over
		// an array of INTs disguised as pointers.  We need to get those
		// ints a little later when we're building this Area, so just copy in
		// what we've got now w/o invoking anything.
		backgrounds.data[ i ] = A.backgrounds.data[ i ];
	}

	// ??????????
	// blocks
	// tiles
	// colModels
	// entities
}





/*
================================================================================

	me_subArea_t

================================================================================
*/
void me_subArea_t::_my_init() {
	tiles.init();
}
void me_subArea_t::_my_reset() {
	x = y = w = h = id = 0;
	tiles.reset();
}
void me_subArea_t::_my_destroy() {
	tiles.destroy();
}
me_subArea_t::me_subArea_t( me_subArea_t const& A ) {
	x	= A.x;
	y 	= A.y;
	w	= A.w;
	h	= A.h;
	id	= A.id;

	// this makes an exact copy, which shallow copies the pointers
	tiles.set_by_ref( A.tiles );
	// what we want is deap copies, new objects, new pointers, etc..
	// so overwrite the ones that were there with pointers to fresh, copied
	//  mapTiles
	for ( int i = 0; i < A.tiles.length(); i++ ) {
		tiles.data[ i ] = new mapTile_t( *A.tiles.data[ i ] ) ;
	}
}

/*
================================================================================

	subArea_t

================================================================================
*/
void subArea_t::_my_init() {
	tiles.init();
}
void subArea_t::_my_reset() {
	id_num = visibility = 0;
	tiles.reset();
}
void subArea_t::_my_destroy() {
	tiles.destroy();
}
subArea_t::subArea_t( subArea_t const& A ) {
	id_num = A.id_num;
	visibility = A.visibility;
	tiles.set_by_ref( A.tiles );
	for ( int i = 0; A.tiles.length(); i++ ) {
		tiles.data[ i ] = new dispTile_t( *A.tiles.data[ i ] );
	}
}
// type converting copycon
subArea_t::subArea_t( me_subArea_t const& A ) {
	id_num = A.id;
	visibility = 0;
}


/*
================================================================================

	dispTile_t

================================================================================
*/
dispTile_t::dispTile_t( dispTile_t const& D ) {
	for ( int i = 0; i < MAX_DISPTILE_MODES; i++ ) {
		// deep copy anim & material
		if ( D.anim[ i ] )
			anim[ i ] = new animSet_t( *D.anim[ i ] );
		if ( D.mat[ i ] )
			mat [ i ] = new material_t( *D.mat[ i ] );
	}
	drawing 	= D.drawing;
	poly 		= D.poly;
	layer 		= D.layer;
	poly.toOBB( obb );
	poly.toAABB( aabb );
	uid 		= D.uid;
}

dispTile_t::dispTile_t( mapTile_t const & M ) {
	// FIXME: do I want deap copy or shallow copy for this, I think deep
	anim[ 0 ] = M.anim;
	mat [ 0 ] = M.mat;
	if ( M.anim )
		anim[ 0 ] = new animSet_t( *M.anim );
	if ( M.mat ) 
		mat [ 0 ] = new material_t( *M.mat );

	for ( int i = 1; i < MAX_DISPTILE_MODES; i++ ) {
		anim[ i ] = NULL;
		mat [ i ] = NULL;
	}

	drawing 	= 0; /* lowest level only */
	poly 		= M.poly;
	layer 		= M.layer;
	poly.toOBB( obb );
	poly.toAABB( aabb );
	uid 		= M.uid;
}


/*
================================================================================

	Block_t 

================================================================================
*/
void Block_t::_my_init() {
	// Blocks share pools held by World or BlockManager, therefore, don't
	//  initialize here
//	up = down = left = right = NULL; // not memory owners, just connectors
}

void Block_t::_my_reset() {
	id = -1;
	p1[0] = p1[1] = p2[0] = p2[1] = 0.0f;
	up = down = left = right = NULL; // not memory owners, just connectors

	for ( int i = 0; i < TOTAL_ENTITY_CLASSES; i++ ) {
		node_c<Entity_t*> *e = entities[i].gethead();
		while ( e ) {
			if ( e->val ) 
				delete e->val;
			e->val = NULL;
			e = e->next;
		}
		entities[i].reset();
	}
	
	dispTiles.delete_reset();
	colModels.delete_reset();
	edgeFlag = 0;
}

void Block_t::_my_destroy() {
//	up = down = left = right = NULL; // not memory owners, just connectors
	for ( int i = 0; i < TOTAL_ENTITY_CLASSES; i++ ) {
		node_c<Entity_t*> *e = entities[i].gethead();
		while ( e ) {
			if ( e->val )
				delete e->val;
			e->val = NULL;
			e = e->next;
		}
		entities[i].destroy();
	}
	dispTiles.delete_destroy();
	colModels.delete_destroy();
}

Block_t::Block_t( Block_t const & B ) 
	: up(0), down(0), left(0), right(0)
{
	id = B.id;
	p1[0] = B.p1[0];
	p1[1] = B.p1[1];
	p2[0] = B.p2[0];
	p2[1] = B.p2[1];
	

//	node_c<Entity_t*> * node = B.entities.gethead();
//	while ( node ) {
//		node = node->next;
//	}

	// FIXME: UNFINISHED!  Don't need a copycon for anything, I think, so 
	//  leaving it unfinished
}

bool Block_t::isOccupied( void) {
	for ( int e = 0; e < TOTAL_ENTITY_CLASSES; e++ ) {
		if ( entities[e].size() > 0 )
			return true;
	}
	if ( dispTiles.length() > 0 ) 
		return true;
	if ( colModels.length() > 0 )
		return true;
			
	return false;
}

/*
==================== 
 M_GetBlockExport
    -creates an export of the contents of a 9 block area, around a radius,
    one of whose verteces lies somewhere in the center block 
    -ioLen returns the length of the array.  should be V_Freed 
	-limitation: doesn't look further 1 surrounding block, if the radius
    goes further than the one adjacent block, it is not considered.  This
    could be improved.
==================== 
*/
BlockExport_t * M_GetBlockExport( float *pt, float radius, int *ioLen ) {
	Area_t *A = world.curArea;
	if ( !A )
		return NULL;

	// check point is in current Area. non-current Areas do not run, 
    //  so no point in checkign them, UNLESS, in the future we want to run
    //  different areas, for example: running the scenario inside a room that
    //  we aren't located in currently, it wouldn't happen, well the 
    //  blockexport wouldn't work anyway
	if ( 	pt[0] < A->p1[0] || 
			pt[1] < A->p1[1] || 
			pt[0] >= A->p2[0] || 
			pt[1] >= A->p2[1] ) {
		return NULL;
	}

	AABB_t box;
	box[0] = pt[0] - radius;
	box[1] = pt[1] - radius;
	box[2] = pt[0] + radius;
	box[3] = pt[1] + radius;

	int block_index[2] = { 
				( pt[0] - A->p1[0] ) / A->block_dim[0], 
				( pt[1] - A->p1[1] ) / A->block_dim[1] 
						};

	int array_index = block_index[0] + A->bx * block_index[1];

	if ( array_index >= A->blocks.length() || array_index < 0 ) {
		// FIND_BLOCK_FAIL
		console.Printf( "M_GetBlockExport: FIND BLOCK FAIL" );
		return NULL;	
	}

	// the block
	Block_t *B = A->blocks.data[ array_index ];

	// empty buffer
	buffer_c<Block_t*> buf;
	buf.init(9);
	
	// add central block
	buf.add( B );

	// -x 
	if ( pt[0] < B->p1[0] ) {
		if ( B->left ) {
			buf.add( B->left );
		}
		// -y 
		if ( pt[1] < B->p1[1] ) {
			if ( B->down ) {
				buf.add( B->down );
				if ( B->down->left )
					buf.add( B->down->left );
			} else if ( B->left ) {
				if ( B->left->down ) {
					buf.add( B->left->down );
				}
			}
		}
		// +y
		if ( pt[3] < B->p2[1] ) {
			if ( B->up ) {
				buf.add( B->up );
				if ( B->up->left )
					buf.add( B->up->left );
			} else if ( B->left ) {
				if ( B->left->up ) {
					buf.add( B->left->up );
				}
			}
		} 
	// +x 
	} else if ( pt[2] > B->p2[0] ) {
		if ( B->right ) 
			buf.add( B->right );
		// -y 
		if ( pt[1] < B->p1[1] ) {
			if ( B->down ) {
				buf.add( B->down );
				if ( B->down->right )
					buf.add( B->down->right );
			} else if ( B->right ) {
				if ( B->right->down ) {
					buf.add( B->right->down );
				}
			}
		}
		// +y
		if ( pt[3] < B->p2[1] ) {
			if ( B->up ) {
				buf.add( B->up );
				if ( B->up->right )
					buf.add( B->up->right );
			} else if ( B->right ) {
				if ( B->right->up ) {
					buf.add( B->right->up );
				}
			}
		} 
	}

	BlockExport_t *be = (BlockExport_t*) V_Malloc( sizeof(BlockExport_t) * buf.length() );

	const unsigned int L = buf.length();
	for ( int i = 0; i < L; i++ ) {
		be[i].cmpp = buf.data[i]->colModels.data;
		be[i].cmln = buf.data[i]->colModels.length();
		be[i].elst = buf.data[i]->entities;
	}
	
	if ( ioLen ) {
		*ioLen = L;
	}

	buf.destroy();

	return be;
}

void TogglePrintBlocks( void ) {
    world.doPrintBlocks = ! world.doPrintBlocks;
}
