
#include "../map/m_area.h"
#include "mapedit.h"
#include "../client/cl_console.h"
#include "../renderer/r_floating.h"
#include "../renderer/r_ggl.h"
#include <gl/gl.h> 

#ifndef snprintf 
#define snprintf _snprintf
#endif

list_c<Area_t*> me_areas;
list_c<me_subArea_t*> me_subAreas;

static int client_id = 0;
timer_c area_timer;
static int area_client_id;

static node_c<Area_t*> * curArea = NULL;


int ME_CreateAreaFromSelection( const char *name ) {
	node_c<mapNode_t*> *node = selected.gethead();
	if ( !node ) 
		return -1;
	
	if ( !client_id ) {
		client_id = floating.requestClientID();
	}

	// gets centroid and bound box
	float *c = ME_GetCentroid();

	Area_t *area = Area_t::New();

	// set name
	if ( name ) {
		// user supplied
		strncpy( area->name, name, AREA_NAME_SIZE );
	} else {
		// auto generated
		snprintf( area->name, AREA_NAME_SIZE, "area_%03u", me_areas.size() + 1 );
	}
	
	// Area dimensions
	area->p1[0] = c[ 2 ];
	area->p1[1] = c[ 3 ];
	area->p2[0] = c[ 4 ];
	area->p2[1] = c[ 5 ];

	// pad the dimensions by 1 large viewport 1600x1200px at standard zoom
	//const float pad[] = { 6.0f * 1600.0f, 6.0f * 1200.0f };
	// toning it down.
	//const float pad[] = { 2.0f * 1600.0f, 2.0f * 1200.0f };
	const float pad[] = { 3000.f, 3000.f }; // no pad
	area->p1[0] -= pad[0];
	area->p1[1] -= pad[1];
	area->p2[0] += pad[0];
	area->p2[1] += pad[1];

	//
	me_areas.add( area );

	console.Printf( "area: \"%s\" created successfully" , area->name );
	char buf[128];
	snprintf( buf, sizeof(buf), "area: \"%s\" created", area->name );
	floating.ClearClientID( client_id );
	floating.AddCoordText( 300.f, 40.f, buf, 14, 10000, client_id );
	return 0; // success
}

void ME_PrintAreaInfo( void ) {
	if ( !me_areas.isstarted() )
		return;
	node_c<Area_t*> *n = me_areas.gethead();
	if ( n ) { 
		console.pushMsg( "" );
		console.Printf( "%u areas", me_areas.size() );
		console.pushMsg( "----------------------------------" );
	
		while ( n ) {
			Area_t *a = n->val;
			console.Printf( "\"%s\": %f %f %f %f", a->name, a->p1[0], a->p1[1], a->p2[0], a->p2[1] );
			int i = 0;
			while ( i < a->backgrounds.length() ) {
				console.Printf( "background: %u", (unsigned int)a->backgrounds.data[i++] );
			}
			n = n->next;
		}
	}

	node_c<me_subArea_t*> * m = me_subAreas.gethead();
	if ( !m )
		return;
	console.pushMsg( "" );
	console.Printf( "%u subAreas", me_subAreas.size() );
	console.pushMsg( "----------------------------------" );
	while ( m ) {
		me_subArea_t *e = m->val;
		console.Printf( "\"%d\": %i %i %i %i. (%u tiles)", e->id, 
				e->x, e->y, e->w, e->h, e->tiles.length() );
		m = m->next;
	}
}

int ME_CreateSubAreaFromSelection( void ) {
	static int id_local = 0;
	node_c<mapNode_t*> *node = selected.gethead();
	if ( !node ) 
		return -1;
	
	if ( !client_id ) {
		client_id = floating.requestClientID();
	}

	// gets centroid and bound box
	float *c = ME_GetCentroid();

	me_subArea_t *sub = new me_subArea_t;

	// Area dimensions
	sub->x = (int) c[ 2 ];
	sub->y = (int) c[ 3 ];
	sub->w = (int) ( c[ 4 ] - c[ 2 ] );
	sub->h = (int) ( c[ 5 ] - c[ 3 ] );

	// give it an id
	sub->id = ++id_local;

	// create list of mapTile members with empty materials, copied UID & poly
	while ( node ) {
		// backgrounds are ignored
		if ( node->val->val->background ) {
			node = node->next;
			continue;
		}

		//mapTile_t *nmt = mapTile_t::New();

		/*
		mapTile_t *n = (mapTile_t *) V_Malloc( sizeof(mapTile_t) );
		n->reset();
		*/
		mapTile_t *n = new mapTile_t;
		mapTile_t *mt_real = node->val->val;

		// uid
		n->uid = mt_real->uid;
		// poly
		n->poly = mt_real->poly; 
		// layer
		n->layer = mt_real->layer;

		sub->tiles.add( n );
		node = node->next;
	}

	//
	me_subAreas.add( sub );

	console.Printf( "subArea: \"%d\" created successfully with %u tiles" , sub->id, sub->tiles.length() );
	char buf[128];
	snprintf( buf, sizeof(buf), "subArea: \"%d\" created with %u tiles", sub->id, sub->tiles.length() );
	floating.ClearClientID( client_id );
	floating.AddCoordText( 300.f, 40.f, buf, 14, 10000, client_id );
	return 0; // success
}

// find the the first area containing the selected tile, and change it's name
int ME_SetName( const char *name ) {
	node_c<mapNode_t*> *node = selected.gethead();
	if ( !node )
		return -1;
 	if ( !name ) 
		return -2;
	

	poly_t *p = &node->val->val->poly;
	float v[4];
	p->toAABB( v );
	int b[4] = { (int)v[0], (int)v[1], (int)v[2], (int)v[3] };
	int c[4];
	
	node_c<Area_t*> *nA = me_areas.gethead();
	if ( !nA ) {
		return -3;
	}
	char buf[128];
	while ( nA ) {
		c[0] = nA->val->p1[0];
		c[1] = nA->val->p1[1];
		c[2] = nA->val->p2[0];
		c[3] = nA->val->p2[1];

		// b is completely inside of c
		if ( 	( b[0] >= c[0] && b[0] <= c[2] ) &&
				( b[1] >= c[1] && b[1] <= c[3] ) &&
				( b[2] >= c[0] && b[2] <= c[2] ) &&
				( b[3] >= c[1] && b[3] <= c[3] ) ) {
			// Found Area, set the name
			strncpy( nA->val->name, name, AREA_NAME_SIZE );
			console.Printf( "Area name changed to: \"%s\"", name );
			snprintf( buf, sizeof(buf), "Area name changed to: \"%s\"", name );
			floating.ClearClientID( client_id );
			floating.AddCoordText( 300.f, 40.f, buf, 14, 10000, client_id );
			return 0;
		}

		nA = nA->next;
	}
	
	// Area not found
	return -4;
}

#define GL_DRAW_QUAD_4V( v ) { \
        gglBegin( GL_QUADS ); \
        gglVertex3f( (v)[0], (v)[1], 0.0f ); \
        gglVertex3f( (v)[0], (v)[3], 0.0f ); \
        gglVertex3f( (v)[2], (v)[3], 0.0f ); \
        gglVertex3f( (v)[2], (v)[1], 0.0f ); \
		gglEnd(); }

#define GL_DRAW_OUTLINE_4V( v ) { \
	    gglBegin( GL_LINE_STRIP ); \
        gglVertex3f( (v)[0], (v)[1], 0.0f ); \
        gglVertex3f( (v)[0], (v)[3], 0.0f ); \
        gglVertex3f( (v)[2], (v)[3], 0.0f ); \
        gglVertex3f( (v)[2], (v)[1], 0.0f ); \
        gglVertex3f( (v)[0], (v)[1], 0.0f ); \
		gglEnd(); }

/*
====================
 ME_DrawAreas
====================
*/
void ME_DrawAreas( void ) {
	if ( area_timer.check() )
		return;

	CL_ModelViewInit2D();
    main_viewport_t *v = M_GetMainViewport();
    gglScalef( v->iratio, v->iratio, 0.f );
    gglTranslatef( -v->world.x, -v->world.y, 0.f );
  	gglDisable( GL_TEXTURE_2D );

	gglLineWidth( 5.0f );
	node_c<Area_t*> *n = me_areas.gethead();
	while ( n ) {
		Area_t *a = n->val;
		float v[4] = { a->p1[0], a->p1[1], a->p2[0], a->p2[1] };
		if ( curArea == n ) 
			gglColor4f( 0.0f, 0.0f, 1.0f, 0.5f );
		else
			gglColor4f( 0.8f, 0.0f, 0.8f, 0.4f );
		GL_DRAW_QUAD_4V( v );
		gglColor4f( 0.4f, 0.0f, 0.4f, 0.3f );
		GL_DRAW_OUTLINE_4V( v );
		n = n->next;
	}
	gglLineWidth( 1.0f );


}



/*
====================
 ME_Area_Command
	""			- no arg: prints current, total # of areas
	create		- 
	x y w h		- after adjusts axis, sets timed display
	delete		- 
	info		- total #, detail on current, whether any tiles are not in area
	info all	- detail on all
	show		- 
	help
	current 	- gets or sets current area by name
	name 		- gets/sets name of current
	bgadd		- add backgrounds to this area by UID
====================
*/
#define PARM(S) ( !strcmp( #S, cmd ) )
#define IFPARM(S) if PARM(S)
#define ELIFPARM(S) else if PARM(S)
#define EXIST(S) ( (S) && (S)[0] ) 
#define CP console.Printf

void ME_AreaHelp( void ) {
	CP( "%-20s %s", "area", "prints Area info" );
	CP( "%-20s %s", "area create [name]", "create a new Area from selected tiles" );
	CP( "%-20s %s", "area <x|y|w|h>", "adjust bounding box of current Area" );
	CP( "%-20s %s", "area delete", "deletes current Area" );
	CP( "%-20s %s", "area info", "prints info about current area" );
	CP( "%-20s %s", "area info all", "prints info about all areas" );
	CP( "%-20s %s", "area show", "highlights area on map" );
	CP( "%-20s %s", "area help", "prints this list" );
	CP( "%-20s %s", "area current [name]", "gets or sets current by name" );
	CP( "%-20s %s", "area name [name]", "gets/sets name of current" );
	CP( "%-20s %s", "area next", "changes current to current->next" );
	CP( "%-20s %s", "area bgadd/bgdel [UID]", "add or remove a background from the current area" );
}

static bool _autoCur( void ) {
	if ( !curArea ) {
		curArea = me_areas.gethead();
	} else {
		curArea = curArea->next ? curArea->next : me_areas.gethead();
	}
	return curArea != NULL;
}

void ME_Area_Command( const char *cmd, const char *arg1, const char *arg2, const char *arg3 ) {

	if ( !cmd || !cmd[0] ) {
		ME_PrintAreaInfo();
		CP( "type \"area help\" for usage" );
		return;
	}

	IFPARM( info ) {
		// check for 'info all'
		return ME_PrintAreaInfo();
	}

	IFPARM( create ) {
		if ( 0 != ME_CreateAreaFromSelection( arg1 ) ) {
			console.Printf( "error creating Area \"%s\"", arg1 );
		} else {
			curArea = me_areas.gettail();
		}
	}

	// sets current by name
	ELIFPARM( current ) {
		if EXIST( arg1 ) {
			node_c<Area_t*> *n = me_areas.gethead();
			while( n ) {
				if ( !strcmp( n->val->name, arg1 ) ) {
					curArea = n;
					CP( "current Area changed to %s", arg1 );
					break;
				}
				n = n->next;
			}
			if ( !n ) 
				CP( "Area %s not found", arg1 );
		} else {
			if ( !curArea ) {
				CP( "no current Area" );
			} else {
				CP( "current Area: %s", curArea->val->name );
			}
		}
	}

	// sets/gets current name
	ELIFPARM( name ) {
		if EXIST( arg1 ) {
			if ( !curArea && !_autoCur() ) {
				CP( "can't select name.  no current Area selected" );
			} else {
				strncpy( curArea->val->name, arg1, AREA_NAME_SIZE );
				CP( "current Area name changed to: %s", arg1 );
			}
		} else {
			CP( "current Area name is: %s", curArea->val->name );
		}
	}

	// help
	ELIFPARM( help ) {
		ME_AreaHelp();
	}

	// x y w h 
	else if ( PARM( x ) || PARM( y ) || PARM( w ) || PARM( h ) || 
		PARM( X ) || PARM( Y ) || PARM( W ) || PARM( H ) ) {

		int dim = tolower( cmd[0] );

		if ( !curArea && !_autoCur() ) {
			CP( "please select an Area to manipulate" );
		} else if ( !arg1 || !arg1[0] ) {
			switch( dim ) {
			case 'x': CP( "%c: %f", dim, curArea->val->p1[0] ); break;
			case 'y': CP( "%c: %f", dim, curArea->val->p1[1] ); break;
			case 'w': CP( "%c: %f", dim, curArea->val->p2[0] ); break;
			case 'h': CP( "%c: %f", dim, curArea->val->p2[1] ); break;
			}
		} else {
			float parm = atof( arg1 );
			switch( dim ) {
			case 'x': curArea->val->p1[0] += parm ; break;
			case 'y': curArea->val->p1[1] += parm ; break;
			case 'w': curArea->val->p2[0] += parm ; break;
			case 'h': curArea->val->p2[1] += parm ; break;
			}
			CP( "%c shifted by %f", dim, parm );
		}
	}

	// delete
	ELIFPARM( delete ) {
		if ( !curArea ) {
			CP( "no current Area to delete!" );
		} else {
			node_c<Area_t*> *tmp = curArea->next ? curArea->next : me_areas.gethead();
			CP( "Area %s removed.  new current is %s", curArea->val->name, (tmp) ? tmp->val->name : "(null)" ) ;
			me_areas.popnode( curArea );
			curArea = tmp;
		}
	}

	// show
	ELIFPARM( show ) {
		area_timer.set( 10000 );
		if ( !me_areas.gethead() ) {
			CP( "No areas exist. You should make one. Type 'area help'" );
		} else {

			// floating text labels
			if ( !area_client_id ) {
				client_id = floating.requestClientID();
			}
			node_c<Area_t*> *n = me_areas.gethead();
			while ( n ) {
				Area_t *A = n->val;
				floating.AddAreaText( A, n->val->name, 5.f, A->p2[1]-A->p1[1]-24.f, 12, 10000, area_client_id );
				n = n->next;
			}
		}

	}

	// next
	ELIFPARM( next ) {
		if ( !curArea && !_autoCur() ) {
			curArea = me_areas.gethead();
			if ( !curArea ) {
				CP( "No Areas" );
			} else {
				CP( "current Area is %s", curArea->val->name );
			}
		} else {
			curArea = curArea->next ? curArea->next : me_areas.gethead();
			CP( "current Area is %s", curArea->val->name );
		}
	}

	ELIFPARM( bgadd ) {
retry1:
		if ( !curArea ) {
			if ( me_areas.size() == 1 && _autoCur() ) {
				goto retry1;
			}
			CP( "no current area" );
		} else if ( !arg1 && !arg1[0] ) {
			CP( "supply a UID of a background to add to Area" );
		} else {
			// add pointers of background tiles to A->background
			int _uid = atoi( arg1 );

			mapNode_t *bg = NULL;
			do {
				bg = tiles->tileByID( _uid, bg ) ;
			} while ( bg && !bg->val->background );

			if ( !bg ) {
				CP( "UID %u background does not exist", _uid );
			} else {
				if ( !curArea->val->backgrounds.isstarted() )
					curArea->val->backgrounds.init();
				curArea->val->backgrounds.add( (dispTile_t*)_uid );
				CP( "Added background %u to Area %s", _uid, curArea->val->name );
			}
		}
	}

	ELIFPARM( bgdel ) {
retry2:
		if ( !curArea ) {
			if ( me_areas.size() == 1 && _autoCur() ) {
				goto retry2;
			}
			CP( "no current area" );
		} else if ( !arg1 && !arg1[0] ) {
			CP( "supply a UID of a background to DELETE from Area %s", curArea->val->name );
		} else {
			// find UID and nuke it
			int _uid = atoi( arg1 );
			if ( curArea->val->backgrounds.length() > 0 ) {
				const uint len = curArea->val->backgrounds.length();
				int index = -1;
				for ( unsigned int i = 0; i < len; i++ ) {
					if ( (unsigned int)curArea->val->backgrounds.data[i] == _uid ) {
						index = i;
						break;
					}
				}
				if ( index < 0 )
					CP ( "UID %u not found in curArea %s", _uid, curArea->val->name );
				else {
					curArea->val->backgrounds.removeIndex( index );
					CP( "removed background %u from Area %s", _uid, curArea->val->name );
				}
			} else {
				CP ( "UID %u not found in curArea %s", _uid, curArea->val->name );
			}
		}
	}

	else {
		ME_AreaHelp();
	}
}



#undef PARM
#undef IFPARM
#undef ELIFPARM
#undef EXIST
#undef CP
