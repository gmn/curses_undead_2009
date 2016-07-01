
// me_tilelist.cpp

#include "mapedit.h"
#include <string.h>
#include "../common/com_geometry.h"
#include "../client/cl_console.h"
#include "../map/mapdef.h"
#include "../renderer/r_floating.h"

#ifndef snprintf
#define snprintf _snprintf
#endif


//
// tilelist_t
//
/*
====================
 tilelist_t::newMaterialTile

 bad, use copyConstructor
====================
*/
mapTile_t * tilelist_t::newMaterialTile( void ) {
	mapTile_t * tile = mapTile_t::NewMaterialTile();
	return tile;
}
/*
====================
 tilelist_t::newMaterialTileXY
====================
*/
mapTile_t * tilelist_t::newMaterialTileXY(	float x, float y, float w, float h, float a, char * name ) {
	mapTile_t * tile = mapTile_t::NewMaterialTile();

	material_t *m = materials.FindByName( name );
	if ( m )
		memcpy( tile->mat, m, sizeof(material_t) );
	
	tile->poly.set( x, y, w, h, a );

	return tile;
}

void tilelist_t::returnTile( mapTile_t *tile ) {
}

void tilelist_t::Destroy( void ) {
	node_c<mapTile_t*> * node = head;
	while ( node ) {
		node->val->Destroy();
		V_Free( node->val );
		node = node->next;
	}
	reset();
}

// copies this list, to another, passed in as arg
void tilelist_t::copyToList( tilelist_t *list ) {
	node_c<mapTile_t*> * node = head;
	if ( !node )
		return;

	list->Destroy(); // just in case, clear the target list 
	
	while ( node ) {
		mapTile_t *copy = node->val->Copy();
		list->add( copy );
		node = node->next;
	}
}

void tilelist_t::syncColModels( void ) {
	node_c<mapTile_t*> * node = head;
	if ( !node )
		return;
	while ( node ) {
		node->val->sync();
		node = node->next;
	}
}
void tilelist_t::syncColModelsNoBlock( void ) {
	node_c<mapTile_t*> * node = head;
	if ( !node )
		return;
	while ( node ) {
		node->val->syncNoBlock();
		node = node->next;
	}
}

// shitty bubblesort for the linked list (sorting by layer, least to most)
void tilelist_t::crapsort( void ) {

	if ( !head )
		return ;

	node_c<mapTile_t *> *rov, *bof, *start, *end;

	start = head;
	end = tail;

	while ( start && start->next && end && start != end ) {

		rov = head;
		bof = ( rov ) ? rov->next : NULL;

		while ( rov && bof ) {
			if ( rov->val->layer > bof->val->layer ) {

				rov->next = bof->next;
				bof->prev = rov->prev;

				if ( rov->prev )
					rov->prev->next = bof;
				else
					head = bof;
	
				if ( bof->next )
					bof->next->prev = rov;
				else
					tail = rov;
	
				bof->next = rov;
				rov->prev = bof;
	
				// increment 
				bof = rov->next;
			} else {
				// increment 
				rov = rov->next;
				bof = ( rov ) ? rov->next : NULL;
			}
			
		} // end while()

		// shorten the searchable area by 1 each pass
		//if ( end )
		//	end = end->prev;
		if ( start )
			start = start->next;

	} // end while()

	needsort = 0;
}

mapNode_t * tilelist_t::tileByID( unsigned int _uid, mapNode_t *start ) {
	node_c<mapTile_t*> *n = start ? start->next : head;
	while ( n ) {
		if ( n->val->uid == _uid ) 
			return n;
		n = n->next;
	}
	return NULL;
}


/*
=============================================================================

	activeMapTile_t

=============================================================================
*/
/*
===================
 activeMapTile_t::checkNode
===================
*/
node_c<mapNode_t*> * activeMapTile_t::checkNode( mapNode_t *node ) {
	node_c<mapNode_t*> *rov = gethead();
	while ( rov ) {
		if ( rov->val == node )
			return rov;
		rov = rov->next;
	}
	return NULL;
}

/*
=====================
 activeMapTile_t::clone
=====================
*/
void activeMapTile_t::cloneToTileList( tilelist_t *tileList ) {
	node_c<mapNode_t*> *node = gethead();
	if ( !node )
		return ;

	tileList->Destroy();

	while ( node ) {
		mapTile_t *copy = node->val->val->Copy();
		tileList->add( copy );
		node = node->next;
	}
}


void activeMapTile_t::cloneToSet( activeMapTile_t *nodeSet ) {
	node_c<mapNode_t*> *node = gethead();
	nodeSet->reset();
	while ( node ) {
		nodeSet->add( node->val );
		node = node->next;
	}
}

/*
=====================
 activeMapTile_t::setTexture
=====================
*/
void activeMapTile_t::setTexture( material_t *mat_p, unsigned int k, unsigned int tot ) {

	node_c<mapNode_t*> *node = this->head;

	if ( !client_id ) 
		client_id = floating.requestClientID();

	while ( node ) {

		if ( !node->val || !node->val->val ) {
			node = node->next;
			continue;
		}

		if ( !node->val->val->mat ) {
			/*
			node->val->val->mat = (material_t*) V_Malloc( sizeof(material_t) );
			node->val->val->mat->reset();
			*/
			node->val->val->mat = new material_t;
		}
        if ( mat_p )
    		memcpy( node->val->val->mat, mat_p, sizeof(material_t) );

		floating.ClearUIDAndClientID( node->val->val->uid, client_id );

		char buf[128];
		snprintf( buf, sizeof(buf), "%u/%u", k, tot );
		floating.AddTileText( node->val->val, buf, 40, 200, 18, 1, client_id );

        if ( mat_p ) {
	    	image_t *img = node->val->val->mat->img;
	    	if ( img ) {
	    		floating.AddTileText( node->val->val, img->name, 5.f, node->val->val->poly.h-24.f, 12, 1, client_id );
	    	}
        } else {
	    		floating.AddTileText( node->val->val, "NULL Material", 5.f, node->val->val->poly.h-24.f, 12, 1, client_id );
        }

		//node->val->val->mat->img = mat_p->img;  
		// since we're just setting texture as the function is called,
		// leave the rest of the current material alone (unless we're allocing
		// it for the very first time

		node = node->next;
	}
}

/*
=====================
 activeMapTile_t::deltaMoveXY
=====================
*/
void activeMapTile_t::deltaMoveXY( float dx, float dy ) {

	node_c<mapNode_t*> *node = this->head;

	while ( node ) {

		if ( !node->val || !node->val->val ) {
			node = node->next;
			continue;
		}

		node->val->val->poly.x += dx;
		node->val->val->poly.y += dy;

		node = node->next;
	}
}

/*
=====================
 activeMapTile_t::selectAll

 select all tiles
=====================
*/
void activeMapTile_t::selectAll( tilelist_t *tlist ) {

	mapNode_t *tile = tlist->gethead();

	this->reset();

	while ( tile ) {
		this->add( tile );
		tile = tile->next;
	}
}

/*
=====================
 activeMapTile_t::MustSnap
=====================
*/
bool activeMapTile_t::MustSnap( void ) {
	node_c<mapNode_t*> *rov = head;
	while ( rov ) {
		if ( rov->val->val->snap )
			return true;
		rov = rov->next;
	}
	return false;
}

/*
=====================
 activeMapTile_t::snapOffAll
=====================
*/
void activeMapTile_t::snapOffAll( void ) {
	node_c<mapNode_t*> *rov = head;
	while ( rov ) {
		rov->val->val->snap = false;
		rov = rov->next;
	}
}

void activeMapTile_t::syncColModels( void ) {
	node_c<mapNode_t*> *rov = head;
	while ( rov ) {
		rov->val->val->sync();
		rov = rov->next;
	}
}
void activeMapTile_t::syncColModelsNoBlock( void ) {
	node_c<mapNode_t*> *rov = head;
	while ( rov ) {
		rov->val->val->syncNoBlock();
		rov = rov->next;
	}
}
void activeMapTile_t::dragColModels( float dx, float dy ) {
	node_c<mapNode_t*> *rov = head;
	while ( rov ) {
		rov->val->val->dragColModel( dx, dy );
		rov = rov->next;
	}
}

bool activeMapTile_t::hasLock( void ) {
	node_c<mapNode_t*> *rov = head;
	bool is_locked = false;
	while ( rov ) {
		if ( rov->val->val->lock ) {
			is_locked = true;
			break;
		}
		rov = rov->next;
	}
	return is_locked;
}

/*
============================================================================= 

	paletteManager_c

============================================================================= 
*/
void paletteManager_c::_my_destroy( void ) {
	node_c<palette_t *> *node = palettes.gethead();
	while ( node ) {
		if ( node->val ) {
			delete node->val;
			node->val = NULL;
		}
		node = node->next;
	}
	palettes.destroy();
}

int paletteManager_c::Next( void ) { 
	node_c<palette_t*> *pnode = current->next;
	if ( !pnode ) {
		pnode = palettes.gethead();
	}
	SetCurrent( pnode );
	return 0;
}

int paletteManager_c::Prev( void ) { 
	node_c<palette_t*> *pnode = current->prev;
	if ( !pnode ) {
		pnode = palettes.gettail();
	}
	SetCurrent( pnode );
	return 0;
}

int paletteManager_c::ChangeTo( int req ) {
	if ( req < 1 || req > (int) palettes.size() )
		return -1;
	node_c<palette_t*> *node = palettes.gethead();
	while ( --req > 0 && node ) {
		node = node->next;
	}
	if ( !node )
		return -1;
	SetCurrent( node );
	return 0;
}

/* 4 new palette cases: 
	typeing palette w/ & w/o arg, saving the default map, and typing map */

// called by map and palette command
void paletteManager_c::NewPalette( const char *name ) {

	// NOTE: you probably tried to run a palette with the game running
	Assert( current != NULL );

	palette_t *p = current->val;
	palette_t *q = new palette_t;

	if ( !name || !name[0] ) {
		sprintf( q->name, "palette_%02d", ++total_palettes );
	} else {
		strcpy( q->name, name );
	}

	q->tilelist = (tilelist_t*) V_Malloc( sizeof(tilelist_t) );
	q->tilelist->init();
	q->entlist  = (entlist_t *) V_Malloc( sizeof(entlist_t) );
	q->entlist->init();

	// re-save state data for current palette.  It may have changed.
	SaveState();
	
	// copy the attributes of the existing editor context
	q->global_snap	 	= p->global_snap;
	q->gridres 		 	= p->gridres;
	q->drawgrid			= p->drawgrid;
	q->zoom				= p->zoom;
	q->x				= p->x;
	q->y				= p->y;
	q->vis 				= p->vis;
	rotateMode 			= 0;
	rotationType 		= 0;

	palettes.add( q );
	SetCurrent( palettes.gettail() );
}

/*
====================
 paletteManager_c::SetCurrent

 all context switches should come through here, (except for a couple special
	cases)
====================
*/
void paletteManager_c::SetCurrent( node_c<palette_t*> *node ) {
	if ( !node ) 
		return;

	// store metadata of current
	SaveState();			

	// save current, in case we delete it, we need to know where to go back to
	last = current;

	// change current
	current = node;
	
	// change tiles pointer
	tiles = node->val->tilelist;

	// change me_ents
	me_ents = node->val->entlist;

	// load new state 
	LoadState();

	me_activeMap->set( "me_activeMap", current->val->name, 0 );

	int i = 1;
	node_c<palette_t*> *n = palettes.gethead();
	while ( n != current && n->next ) { n = n->next; ++i; }

	console.Printf( "switched to: F%d: \"%s\"", i, current->val->name );

	// client id used to do per client clear() functions.  That way I only clear just what's mine
	static int client_id = 0;
	if ( !client_id ) {
		client_id = floating.requestClientID();
	}

	char buf[256];
	snprintf( buf, 256, "%d: %s", i, current->val->name );
	floating.ClearClientID( client_id );
	floating.AddCoordText( 10, M_Height() - 30, buf, 20, 1, client_id );
}



// called on every state change, to save current state before replacing it
//  with new state.  If there isn't a current palette_t , SaveState will
//  create it.  Unique Identifier is mapname.  empty string counts as a valid
//  name.  Although there CAN NOT be more than one map of the same name, ever,
//  and that also counts for empty string.  Otherwise, looks for a palette
//  with a matching string, finds it and saves all state data to it.
void paletteManager_c::SaveState( void ) {
	// have to create default Map palette if we don't already have it
	if ( palettes.size() == 0 || !current ) {
		palette_t *p = new palette_t;
		++total_maps ;
		strcpy( p->name, "_default" );
		me_activeMap->set( "me_activeMap", "_default", 0 );
		palettes.add( p );
		current = palettes.gettail();
		current->val->tilelist = tiles;
		current->val->entlist = me_ents;
		Assert( tiles != NULL );
	}
	
	main_viewport_t *v  = M_GetMainViewport();
	palette_t * p 		= current->val;

	p->global_snap		= snapToGrid; 
	p->gridres 			= subLineResolution;
	p->drawgrid			= drawGrid;
	p->zoom 			= v->ratio;
	p->x 				= v->world.x;
	p->y 				= v->world.y;
	p->vis 				= visibility;
}

void paletteManager_c::LoadState( void ) {
	if ( !current ) {
		return SaveState();
	}

	// certain global states get reset, like rotationMode and rotationType
	rotateMode = 0;
	rotationType = 0;
	selected.reset();
	entSelected.reset();

	main_viewport_t *v 	= M_GetMainViewport();
	palette_t *p 		= current->val;

	snapToGrid			= p->global_snap;
	subLineResolution 	= p->gridres;
	drawGrid			= p->drawgrid;
	v->ratio			= p->zoom;
	v->iratio			= 1.0f / v->ratio;
	v->world.x			= p->x;
	v->world.y			= p->y;
	visibility			= p->vis;
}

int paletteManager_c::DeleteCurrent( void ) {
	Assert( current != NULL );

	// can't delete the last one, there is _always_ one 
	if ( palettes.size() == 1 )
		return -1;

	palette_t *p = palettes.popnode( current );

	delete p;

	if ( current == last ) {
		last = current = palettes.gethead();
		if ( !current ) {
			SaveState();
			return 0;
		}
	}

	SetCurrent( last );
	return 0;
}

void paletteManager_c::Info( void ) {
	console.pushMsg( "" );
	console.Printf( "%u palettes loaded", palettes.size() );
	node_c<palette_t*> *p = palettes.gethead();
	// call save to update info in current palette
	if ( p ) {
		SaveState();
	}
	int i = 1;
	while ( p ) {
		console.Printf( "%s%d: \"%s\"  [%u tiles] [%u entities] (x, y)(%f, %f) mag: %f", (current==p)?"*F":"F", i++, p->val->name, p->val->tilelist->size(), p->val->entlist->size(), p->val->x, p->val->y, p->val->zoom );
		p = p->next;
	}
}

void paletteManager_c::ChangeCurrentName( const char *_name ) {
	Assert( current != NULL );
	strcpy( current->val->name, _name );
	me_activeMap->set( "me_activeMap", _name, 0 );
}

/*
=============================================================================

	palette_t

=============================================================================
*/
palette_t * palette_t::Copy( void ) {
	return NULL;
}


/*
============================================================================

	entNodeList_t

============================================================================
*/
/*
*/
node_c<entNode_t *> * entNodeList_t::checkNode( entNode_t *nE ) {
	node_c<node_c<Entity_t*> *> *rov = gethead();
	while ( rov ) {
		if ( rov->val == nE ) {
			return rov;
		}
		rov = rov->next;
	}
	return NULL;
}


/*
*/
void entNodeList_t::drag( float dx, float dy ) {
	node_c<entNode_t*> *n = this->head;
	while ( n ) {
		if ( !n->val || !n->val->val ) {
			n = n->next;
			continue;
		}
		n->val->val->poly.x += dx;
		n->val->val->poly.y += dy;
		n->val->val->aabb[0] += dx;
		n->val->val->aabb[1] += dy;
		n->val->val->aabb[2] += dx;
		n->val->val->aabb[3] += dy;
		float *O = n->val->val->obb;
		for ( int i = 0; i < 8; i+=2 ) {
			O[ i + 0 ] += dx;
			O[ i + 1 ] += dy;
		}
		if ( n->val->val->col ) {
			n->val->val->col->box[0] += dx;
			n->val->val->col->box[1] += dy;
			n->val->val->col->box[2] += dx;
			n->val->val->col->box[3] += dy;
		}
		n->val->val->trig[0] += dx;
		n->val->val->trig[1] += dy;
		n->val->val->trig[2] += dx;
		n->val->val->trig[3] += dy;
		n->val->val->clip[0] += dx;
		n->val->val->clip[1] += dy;
		n->val->val->clip[2] += dx;
		n->val->val->clip[3] += dy;
		n = n->next;
	}
}

/*
=====================
 entNodeList_t::setTexture
=====================
*/
void entNodeList_t::setTexture( material_t *mat_p, unsigned int k, unsigned int tot ) {

	node_c<entNode_t*> *node = this->head;

	if ( !client_id ) 
		client_id = floating.requestClientID();

	while ( node ) {

		// door gets its own material 
		if ( node->val->val->entType == ET_DOOR ) {
			if ( !node->val->val->mat ) {
				node->val->val->mat = new material_t;
			}
			memcpy( node->val->val->mat, mat_p, sizeof(material_t) );
		// everybody else gets a pointer to one in the commons
		} else {
			node->val->val->mat = mat_p;
		}

		floating.ClearUIDAndClientID( (unsigned int)node->val->val, client_id );

		char buf[128];
		snprintf( buf, sizeof(buf), "%u/%u", k, tot );
		floating.AddEntText( node->val->val, buf, 40, 200, 18, 1, client_id );
		image_t *img = node->val->val->mat->img;
		if ( img ) {
			floating.AddEntText( node->val->val, img->name, 5.f, node->val->val->poly.h-24.f, 12, 1, client_id );
		}

		node = node->next;
	}
}

/*
====================
 entNodeList_t::clone

	populate a list of entity pointers from the contents of this list
====================
*/
void entNodeList_t::cloneToList( entPointerList_t *list ) {
	node_c<entNode_t*> *n = this->head;
	if ( !n )
		return;
	list->reset(); // note: will just delete the pointers, memory leak-ish
	while( n ) {
		Entity_t * copy = Entity_t::CopyEntity( n->val->val );
		list->add( copy );
		n = n->next;
	}
}


/*
====================
 entPointerList_t::transferFromList

	empties one list (argument), storing its contents in another (this one)
====================
*/
void entPointerList_t::transferFromList( entPointerList_t & other ) {
	Entity_t *e ;
	if ( !other.gethead() ) {
		return;
	}
	// destroy this list first, if there's any in it
	while ( (e = pop()) ) {
		delete e;
	}
	while( (e = other.pop()) ) {
		add( e );
	}
}


