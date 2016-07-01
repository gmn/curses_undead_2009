
#include "mapedit.h"
#include "../common/common.h"
#include "me_menus.h"
#include "../client/cl_console.h"

#include "../map/m_area.h"

/*


 fs_basepath = full os path to the game directory: ie, "C:\Program Files\NestorSoft\ZombieCurses"
 fs_game = folder name of currently active game. same as game name: ie: "zpak"
 fs_gamepath = the concatenation of the two above: "C:\Program Files\NestorSoft\ZombieCurses\zpak"
 fs_devpath = an addition folder which can be searched for content.  it is thought of by the game the same way that the game thinks of fs_gamepath.  This allows the developer to muck about with new assets and make a mess in another directory, that is not the prime gamepath, so that the master game folder can remain untouchted, until at whatever time they deem it necessary to make a more perminent change.  Don't pollute the pool, etc..

 game folder hierarchy: this will be the folder structure that the engine will look for from either the gamepath, or devpath, for example:

 gfx/map1, 
 gfx/map2, 
 gfx/titlescreen, 
 gfx/special, 
 sounds, 
 maps, 
 scripts 

 Another thing: the way id does it is that all file assets that are used in the game are stored as their relative path to the gamepath.  so for instance a map might be called: method\dm\methdm1 , so it starts in the base folder, looks for a method folder, a dm, and so on.  I will do the same.  

 a second point, file extensions may be left off.  at least this is how John does it.  so, see that map file, a map file can only have 2or3 possible extensions, so he just codes them all, and takes the first thing he finds.  I dont have to do this.  but I will at least cover with and without the extension ".map"

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#ifdef _WIN32
#define snprintf _snprintf
#endif



static textEntryBox_t saveTextBox; 
static messageBox_t msgBox;



void ME_SaveMap( const char *fname ) {

	char buf[512], buf2[512];
	buf[0] = 0; buf2[0] = 0;
	snprintf( buf, 512, "%s\\maps\\%s", fs_gamepath->string(), fname );
	snprintf( buf2, 512, "%s\\maps\\%s.bak", fs_gamepath->string(), fname );

	// make a backup
	Com_CopyFile( buf, buf2 );

	FILE *fp = fopen ( buf, "wb" );

	if ( !fp ) {
		Com_Error( ~0, "Couldn't open %s for writing!!!\n", buf );
	}

	// Must be first
	fprintf( fp, "Version 1\n\n" );
	
	// metadata comment
	fprintf( fp, "////////////////////////////////////////////////////////\n" );
	fprintf( fp, "// Map file format for ZombieCurses\n" ); 
	fprintf( fp, "// Copyright (C) 2008 Greg Naughton\n" );
	fprintf( fp, "// mapname: \"%s\" \n", fname );
	fprintf( fp, "// generated: %s", Com_GetDateTime() );
	fprintf( fp, "////////////////////////////////////////////////////////\n\n" );

    fprintf( fp, "script map_%s.lua\n\n", fname );

	//
	// mapTile list
	//
	mapNode_t *tile = tiles->gethead();
	while ( tile ) 
	{
		Assert( tile->val != NULL );

		material_t *material = tile->val->mat;
		float * c = material->color;
		poly_t *p = &tile->val->poly;

		// mapTile decl
		fprintf( fp , "MapTile\n{\n\t" );
		// Layer
		fprintf( fp, "Layer\n\t{\n\t\t%d\n\t}\n\n", tile->val->layer );
		// Background
		fprintf( fp, "\tBackground\n\t{\n\t\t%d\n\t}\n\n", tile->val->background );
		// UID (needed by subAreas)
		fprintf( fp, "\tUID\n\t{\n\t\t%d\n\t}\n\n", tile->val->uid );

		// material
		if ( material ) {
			fprintf( fp , "\tMaterial\n\t{\n\t\t" );

			Assert( material->type < TOTAL_MATERIAL_TYPES );
			fprintf( fp, "%s ", matNames[ material->type ].string );
	
			switch ( material->type ) {
			case MTL_COLOR:
				fprintf( fp, "( %.3f %.3f %.3f %.3f ) ", c[0],c[1],c[2],c[3] );
				break;
			case MTL_TEXTURE_STATIC:
				//Assert( material->img != NULL );
				if ( material->img != NULL )
					fprintf( fp, "%s ", material->img->syspath );
				break;
			case MTL_COLORMASK:
				//Assert( material->img != NULL );
				fprintf( fp, "( %.3f %.3f %.3f %.3f ) ", c[0],c[1],c[2],c[3] );
				if ( material->img != NULL )
					fprintf( fp, "%s ", material->img->syspath );
				break;
			case MTL_MASKED_TEXTURE:
				//Assert( material->img != NULL );
				//Assert( material->mask != NULL );
				if ( material->img != NULL )
					fprintf( fp, "\n\t\ttexture %s", material->img->syspath );
				if ( material->mask != NULL )
					fprintf( fp, "\n\t\talphaMask %s ", material->mask->syspath );
				break;
			}

			fprintf ( fp, "\n\t}\n\n" );
		}

		// Poly
		fprintf( fp, "\tPoly\n\t{\n\t\t" );
		fprintf( fp , "( %f %f %f %f %f )\n\t}\n", p->x, p->y, p->w, p->h, p->angle ); 

		// ColModel
		if ( 	tile->val->col && 
				(!ISZERO(tile->val->col->box[0]) ||
				!ISZERO(tile->val->col->box[1]) ||
				!ISZERO(tile->val->col->box[2]) ||
				!ISZERO(tile->val->col->box[3])) 
			) {
			
			fprintf( fp, "\n\tColModel\n\t{\n\t\t" );
			fprintf( fp, "( %f %f %f %f )\n", 
				tile->val->col->box[0],
				tile->val->col->box[1],
				tile->val->col->box[2],
				tile->val->col->box[3] );
			fprintf( fp, "\t}\n" );
		}
/* deprecated
		colModel_t *col = tile->val->col;
		if ( col && col->type != COLMOD_NONE ) {
			fprintf( fp, "\n\tColModel\n\t{\n\t\t" );
			AABB_node *a = col->AABB;
			OBB_node *r = col->OBB;
			switch( col->type ) {
			case CM_AABB:
				fprintf( fp, "CM_AABB %d\n\n", col->count );
				while( a ) {
					fprintf( fp, "\t\t( %f %f %f %f )\n", 
						a->AABB[0], a->AABB[1], a->AABB[2], a->AABB[3] );
					a = a->next;
				} 
				break;
			case CM_OBB:
				fprintf( fp, "CM_OBB %d\n\n", col->count );
				while( r ) {
					fprintf( fp, "\t\t( %f %f %f %f %f %f %f %f )\n", 
						r->OBB[0], r->OBB[1], r->OBB[2], r->OBB[3],
						r->OBB[4], r->OBB[5], r->OBB[6], r->OBB[7] );
					r = r->next;
				} 
				break;
			}
			fprintf( fp, "\t}\n" );
		}
*/

		// Animation
		animSet_t *anim = tile->val->anim;
		if ( anim ) {
			fprintf( fp, "\n\tAnimation\n\t{\n" );
			fprintf( fp, "\t}\n" );
		}

		// closing bracket
		fprintf ( fp, "}\n\n" );

		// next MapTile
		tile = tile->next;
	}
	// end mapTile


	//
	// entity list
	//
	entNode_t *nE = me_ents->gethead();
	while ( nE ) {
		Entity_t *E = nE->val;
		// header 
		fprintf( fp, "Entity\n{\n\tname %s\n", E->name );
		fprintf( fp, "\ttype %s\n", entityTypes[ E->entType ].token );
		fprintf( fp, "\tclass %s\n\n", entityClasses[ E->entClass ].token );

		// Animation
		if ( E->anim ) {
			fprintf( fp , "\tAnimation\n\t{\n\t\t" );
			fprintf ( fp, "\n\t}\n\n" );
		}

		// Material
		if ( E->mat ) {
			float * c = E->mat->color;
			fprintf( fp , "\tMaterial\n\t{\n\t\t" );
			Assert( E->mat->type < TOTAL_MATERIAL_TYPES );
			fprintf( fp, "%s ", matNames[ E->mat->type ].string );
	
			switch ( E->mat->type ) {
			case MTL_COLOR:
				fprintf( fp, "( %.3f %.3f %.3f %.3f ) ", c[0],c[1],c[2],c[3] );
				break;
			case MTL_TEXTURE_STATIC:
				if ( E->mat->img != NULL )
					fprintf( fp, "%s ", E->mat->img->syspath );
				break;
			case MTL_COLORMASK:
				fprintf( fp, "( %.3f %.3f %.3f %.3f ) ", c[0],c[1],c[2],c[3] );
				if ( E->mat->img != NULL )
					fprintf( fp, "%s ", E->mat->img->syspath );
				break;
			case MTL_MASKED_TEXTURE:
				if ( E->mat->img != NULL )
					fprintf( fp, "\n\t\ttexture %s", E->mat->img->syspath );
				if ( E->mat->mask != NULL )
					fprintf( fp, "\n\t\talphaMask %s ", E->mat->mask->syspath );
				break;
			}
			fprintf ( fp, "\n\t}\n\n" );
		}

		// poly
		poly_t *p = &E->poly;
		fprintf( fp, "\tPoly\n\t{\n\t\t" );
		fprintf( fp, "( %f %f %f %f %f )\n\t}\n", p->x, p->y, p->w, p->h, p->angle ); 

		// Collidable
		fprintf( fp, "\tCollidable\n\t{\n\t\t%hhu\n\t}\n", E->collidable );

		// ColModel
		colModel_t *col = E->col;
		if ( col ) {
			fprintf( fp, "\n\tColModel\n\t{\n\t\t" );
			fprintf( fp, "( %f %f %f %f )\n", 
					col->box[0], col->box[1], col->box[2], col->box[3] );
			fprintf( fp, "\t}\n" );
		}
		// trigger, only writes if triggerable is set
		if ( E->collidable && E->triggerable ) {
			fprintf( fp, "\tTrigger\n\t{\n\t\t%f %f %f %f \n\t}\n",  
				E->trig[0], E->trig[1], E->trig[2], E->trig[3]
			);
		}
		// clipping, only writes if clipping is set
		if ( E->collidable && E->clipping ) {
			fprintf( fp, "\tClip\n\t{\n\t\t%f %f %f %f \n\t}\n",  
				E->clip[0], E->clip[1], E->clip[2], E->clip[3]
			);
		}
		
		

/* just using the bounding box, nothing else
		if ( col && col->type != COLMOD_NONE ) {
			fprintf( fp, "\n\tColModel\n\t{\n\t\t" );
			AABB_node *a = col->AABB;
			OBB_node *r = col->OBB;
			switch( col->type ) {
			case CM_AABB:
				fprintf( fp, "CM_AABB %d\n\n", col->count );
				while( a ) {
					fprintf( fp, "\t\t( %f %f %f %f )\n", 
						a->AABB[0], a->AABB[1], a->AABB[2], a->AABB[3] );
					a = a->next;
				} 
				break;
			case CM_OBB:
				fprintf( fp, "CM_OBB %d\n\n", col->count );
				while( r ) {
					fprintf( fp, "\t\t( %f %f %f %f %f %f %f %f )\n", 
						r->OBB[0], r->OBB[1], r->OBB[2], r->OBB[3],
						r->OBB[4], r->OBB[5], r->OBB[6], r->OBB[7] );
					r = r->next;
				} 
				break;
			}
			fprintf( fp, "\t}\n" );
		}
*/


		//
		//  #### SUB CLASSES ####
		//
		Spawn_t *s = NULL;
		Door_t *D = NULL;
		Portal_t *P = NULL;
		int Mask;
		switch ( entityTypes[ E->entType ].type ) {
		case ET_SPAWN:
			s = dynamic_cast<Spawn_t*>(E);
			if ( !s ) {
				console.Printf( "Spawn Type failed dynamic cast!  Wutup with that?" );
				console.dumpcon( "console.DUMP" );
				break;
			}
			fprintf( fp, "\tSpawn\n\t{\n\t\ttype %s\n", s->type == ST_AREA ? "ST_AREA" : "ST_WORLD" );
			// OPTIONAL PARM
			if ( s->set_view ) 
				fprintf( fp, "\t\tview %f %f %f %f %f\n" , s->view[0], s->view[1], s->view[2], s->view[3], s->zoom );
			// OPTIONAL PARM
			if ( s->pl_speed[0] )
				fprintf( fp, "\t\tspeed %s\n", s->pl_speed );
			fprintf( fp, "\t}\n" );
			break;
		case ET_PLAYER:
			break;
		case ET_ROBOT:
			break;

		case ET_DOOR:
			D = dynamic_cast<Door_t*>(E);
			if ( !D ) {
				console.Printf( "Door Type failed dynamic cast!  Wutup with that?" );
				console.dumpcon( "console.DUMP" );
				break;
			}
			fprintf( fp, "\tDoor\n\t{\n\t\tstate %s\n", D->state == DOOR_CLOSED ? "closed" : (D->state == DOOR_OPEN) ? "open" : "unknown" );
			fprintf( fp, "\t\tlock %s\n", D->lock == DOOR_UNLOCKED ? "unlocked" : "locked" );
			fprintf( fp, "\t\ttype %s\n", (D->type&DOOR_PROXPERM)==DOOR_PROXPERM ? "proxperm" : (D->type&DOOR_MANPERM)==DOOR_MANPERM ? "manperm" : D->type & DOOR_MANUAL ? "manual" : D->type & DOOR_PROXIMITY ? "proximity" : "unknown" );
			fprintf( fp, "\t\tdir  %s\n", D->dir == DOOR_UP ? "up" : D->dir == DOOR_RIGHT ? "right" : D->dir == DOOR_DOWN ? "down" : D->dir == DOOR_LEFT ? "left" : "unknown" );
			fprintf( fp, "\t\tdoortime %d\n", D->doortime );
			fprintf( fp, "\t\twaittime %d\n", D->waittime );
			fprintf( fp, "\t\thitpoints %d\n", D->hitpoints );
			fprintf( fp, "\t\tuse_portal %hhu\n", D->use_portal );
			{
			char * clr = NULL;
			switch ( D->color ) {
			case DOOR_COLOR_RED: clr = "red"; break;
			case DOOR_COLOR_BLUE: clr = "blue"; break;
			case DOOR_COLOR_GREEN: clr = "green"; break;
			case DOOR_COLOR_SILVER: clr = "silver"; break;
			case DOOR_COLOR_GOLD: clr = "gold"; break;
			default: break; }
			if ( clr )
				fprintf( fp, "\t\tcolor %s\n", clr );
			}
			fprintf( fp, "\t}\n" );

			// if it is also a portal, let it fall through
			if ( !D->use_portal )
				break;
		case ET_PORTAL:
			P = dynamic_cast<Portal_t*>(E);
			if ( !P ) {
				console.Printf( "Portal failed dynamic cast! Wutup with that?");
				console.dumpcon( "console.DUMP" );
				break;
			}
			fprintf( fp, "\tPortal\n\t{\n\t\tarea %s\n", P->area );
			fprintf( fp, "\t\tspawn %s\n", P->spawn );
			Mask = P->type & PRTL_MASK_LOW;
			fprintf( fp, "\t\ttype %s\n", Mask == PRTL_FAST ? "fast" : Mask == PRTL_FREEZE ? "freeze" : Mask == PRTL_WAIT ? "wait" : Mask == PRTL_FREEZEWAIT ? "freeze_wait" : "unknown" );
			// all portals are SLEEPING when the map is loaded
			//fprintf( fp, "\t\tstate %s\n", P->state == PRTL_SLEEPING
			if ( P->type & PRTL_FREEZE ) {
				fprintf( fp, "\t\tfreeze %u\n", P->freeze );
			}
			if ( P->type & PRTL_WAIT ) {
				fprintf( fp, "\t\twait %u\n", P->wait );
			}
			fprintf( fp, "\t}\n" );
			break;
		}

		fprintf( fp, "}\n\n" );
		nE = nE->next;
	}

	//
	// Area Definitions
	//
	node_c<Area_t*> *Anode = me_areas.gethead();
	while( Anode ) {
		Area_t *A = Anode->val;
		fprintf( fp, "Area\n{\n\t%s %f %f %f %f", A->name, A->p1[0], A->p1[1], A->p2[0], A->p2[1] ); 
		if ( A->backgrounds.length() > 0 ) 
			fprintf( fp, "\n\tuid " );
		for ( int a = 0 ; a < A->backgrounds.length(); a++ ) {
			fprintf( fp, "%u ", (unsigned int)A->backgrounds.data[ a ] );
		}
		fprintf( fp, "\n}\n" );
		Anode = Anode->next;
	}

	//
	// subArea definitions
	//
	node_c<me_subArea_t*> *Snode = me_subAreas.gethead();
	while( Snode ) {
		me_subArea_t *S = Snode->val;
		fprintf( fp, "subArea\n{\n\t%i %i %i %i %i \n", S->id, S->x, S->y, S->w, S->h ); 

		mapTile_t ** st = S->tiles.data;
		int i = 0;
		int total = S->tiles.length();
		while ( i < total ) {
			fprintf( fp, "\t%d ", (*st)->uid );
			if ( (*st)->mat ) { 
				material_t *mat = (*st)->mat;
				float *c = mat->color;

				// extra material info
				fprintf( fp , " Material\n\t{\n\t\t" );

				Assert( mat->type < TOTAL_MATERIAL_TYPES );
				fprintf( fp, "%s ", matNames[ mat->type ].string );
	
				switch ( mat->type ) {
				case MTL_COLOR:
					fprintf( fp, "( %.3f %.3f %.3f %.3f ) ", c[0],c[1],c[2],c[3] );
				break;
				case MTL_TEXTURE_STATIC:
					if ( mat->img != NULL )
						fprintf( fp, "%s ", mat->img->syspath );
					break;
				case MTL_COLORMASK:
					fprintf( fp, "( %.3f %.3f %.3f %.3f ) ", c[0],c[1],c[2],c[3] );
					if ( mat->img != NULL )
						fprintf( fp, "%s ", mat->img->syspath );
					break;
				case MTL_MASKED_TEXTURE:
					if ( mat->img != NULL )
						fprintf( fp, "\n\t\ttexture %s", mat->img->syspath );
					if ( mat->mask != NULL )
						fprintf( fp, "\n\t\talphaMask %s ", mat->mask->syspath );
					break;
				}

				fprintf ( fp, "\n\t} " );

			} // if material
			fprintf( fp, "\n" );

			++st; ++i;
		}

		fprintf( fp, "\n}\n\n" );
		Snode = Snode->next;
	}
	// end subArea


	fclose( fp );
}

// consumes comments and white space
static void strip_white_space (char *buf, int *in, int max)
{
    int i = *in;

	// consume all comments and whitespace until we come to a meaningful character
	while ( 1 ) {
		while ( buf[i] == '/' && buf[i+1] == '/' ) {
			while ( buf[i++] != '\n' && i < max-1 ) 
				;
		}
		if ( i >= max - 1 ) 
			break;

    	while ((buf[i] == ' '  || 
            	buf[i] == '\t' ||
            	buf[i] == '\n' ||
            	buf[i] == '\r' ) && i < max-1 )
        	++i;

		if ( i >= max - 1 ) 
			break;

		if ( 	buf[i] != '/' && buf[i] != ' ' && buf[i] != '\t' &&
				buf[i] != '\n' && buf[i] != '\r' )
			break;
		if ( 	buf[i] == '/' && buf[i+1] != '/' || 
				buf[i] != '/' && buf[i+1] == '/' )
			break;
	}

	*in = i;
}
#define STRIP_WHITE_SPACE() strip_white_space(data, &i, sz)

static int _nextMaterial( int *j, char * d, int sz ) {

	int i = *j;

	do {
		while ( i < sz && d[i] != 'M' )
			++i;

		if ( i >= sz ) {
			*j = i;
			return 0;
		}

		if (d[i+1] == 'a' &&
			d[i+2] == 't' &&
			d[i+3] == 'e' &&
			d[i+4] == 'r' &&
			d[i+5] == 'i' &&
			d[i+6] == 'a' &&
			d[i+7] == 'l' ) {
			*j = i + 8;
			return 1;
		}

	} while ( 1 ) ;

	*j = i;
	return 0;
}
#define NEXT_MATERIAL() _nextMaterial( &i, data, sz )

static float _getFloat( int *in, char *name, char *data, int sz ) {
	int i = *in, j = 0;
	STRIP_WHITE_SPACE();
	while ( ( i < sz ) && 
			( 	( data[i] >= '0' && data[i] <= '9' ) ||
				( data[i] == '.' ) ||
				( data[i] == 'e' ) ||
				( data[i] == 'f' ) ||
				( data[i] == '+' ) ||
				( data[i] == '-' ) ) ) {
		name[ j++ ] = data[ i++ ];
	}
				
	name[j] = 0;
	*in = i;
	return (float)atof( name );
}
#define GET_FLOAT() _getFloat( &i, name, data, sz )

static int _checkMapTile( int *j, char *data, int sz ) {
	int i = *j;
	if ( 	data[i+0] == 'M' &&
			data[i+1] == 'a' &&
			data[i+2] == 'p' &&
			data[i+3] == 'T' &&
			data[i+4] == 'i' &&
			data[i+5] == 'l' &&
			data[i+6] == 'e' ) {
			i += 7;

			STRIP_WHITE_SPACE();

			*j = i;
			// should be a bracket
			if ( data[i] != '{' )
				return 0;
			*j++;
			return 1;
		}
	return 0;
}
#define CHECK_MAPTILE() _checkMapTile( &i, data, sz )


// FIXME: the right way to do this is to know exactly what characters are valid in a float
//  at all times.  you have +-0-9e.0-9 so you check when its NOT one of those characters,
//  thats how you know its a float.  similarly, for this name, you check explicitly for
//  newline
static void _getName( int *in, char *name, char *data, int sz ) {
	int i = *in;
	STRIP_WHITE_SPACE();

	int j = 0;
	bool finished = false;

	do {
		name[j] = data[i];

		switch ( name[j] ) {
		case '\n':
		case ' ':
		case '\0':
		case '\t':
		case '\r':
			finished = true;
			break;
		default:
			++i; ++j;
			break;
		}
	} while ( !finished && i < sz );

	name[j] = '\0';
	*in = i;
}
#define GET_NAME() _getName( &i, name, data, sz )


static int TOLOWER( int x ) {
	if ( x >= 65 && x <= 90 ) {
		return x + 'a' - 'A'; 
	}
	return x;
}

static int _checkName_i( int *j, char *name, char *data, int sz ) {
	int i = *j, k = 0;
	while ( name[k] ) {
		if ( TOLOWER( name[k++] ) != TOLOWER( data[i++] ) ) {
			return 0;
		}
	}

	// still another character left in the data string, voids the match
	if ( data[i] != ' ' && data[i] != '\r' && data[i] != '\t' && data[i] != '\n' )
			return 0;

	return 1;
}
#define CHECK_NAME_i(name) _checkName_i( &i, name, data, sz )


static int _checkName( int *j, char *name, char *data, int sz ) {
	int i = *j, k = 0;
	while ( name[k] ) {
		if ( name[k++] != data[i++] ) {
			return 0;
		}
	}

	// still another character left in the data string, voids the match
	if ( data[i] != ' ' && data[i] != '\r' && data[i] != '\t' && data[i] != '\n' )
			return 0;

	return 1;
}
#define CHECK_NAME(name) _checkName( &i, name, data, sz )

static int _checkVersion( int *j, char *data, int sz ) {
	int i = *j;
	if (  data[i] != 'V' ||
		data[i+1] != 'e' ||
		data[i+2] != 'r' ||
		data[i+3] != 's' ||
		data[i+4] != 'i' ||
		data[i+5] != 'o' ||
		data[i+6] != 'n' )
		return -1;

	i += 6 + 1;
	STRIP_WHITE_SPACE();
	if ( data[i] > '9' || data[i] < '0' )
		return -1;

	char buf[32];
	memset( buf, 0, 32 );
	int k = 0;

	while ( data[i] >= '0' && data[i] <= '9' ) {
		buf[k++] = data[i++];
	}
		
	int version = atoi( buf );
	*j = i;
	return version;
}
#define CHECK_VERSION() _checkVersion( &i, data, sz )

int ME_ReadInArea( list_c<Area_t*> *areas, int *j, char *data, int sz ) {
	int i = *j;

	STRIP_WHITE_SPACE();
	// attempt to consume bracket '{'
	if ( data[i] != '{' ) {
		*j = i;
		console.Printf( "mapLoad: error: unexpected character" );
		return -1;
	}
	++i;

	Area_t *a = Area_t::New();

	// area is 1 line: STRING INT INT INT INT

	// name
	char name[256];
	memset( name, 0, sizeof(name) );
	GET_NAME();
	strncpy( a->name, name, AREA_NAME_SIZE );
	a->name[ AREA_NAME_SIZE - 1 ] = '\0';

	// dimension
	a->p1[0] = (int) GET_FLOAT();
	a->p1[1] = (int) GET_FLOAT();
	a->p2[0] = (int) GET_FLOAT();
	a->p2[1] = (int) GET_FLOAT();

	STRIP_WHITE_SPACE();

	// check for conditional background uids
	if ( CHECK_NAME_i("UID") ) {
		i+=3;
		a->backgrounds.init();
		do {
			STRIP_WHITE_SPACE();
			if ( data[i] == '}' )
				break;
			int _uid = GET_FLOAT();
			a->backgrounds.add( (dispTile_t*)_uid );
		} while(1);
	}

	areas->add( a );

	STRIP_WHITE_SPACE();
	// closing bracket
	if ( data[i] != '}' ) {
		*j = i;
		console.Printf( "mapLoad: error: unexpected character" );
		return -2;
	}
	++i;
	*j = i;
	return 0;
}

int ME_ReadInSubArea( list_c<me_subArea_t*> *subAreas, int *j, char *data, int sz ) {
	int i = *j;

	STRIP_WHITE_SPACE();
	// attempt to consume bracket '{'
	if ( data[i] != '{' ) {
		*j = i;
		console.Printf( "mapLoad: error: unexpected character" );
		return -1;
	}
	++i;

	me_subArea_t *s = new me_subArea_t;

	// subArea is 1 or more lines
	// 1st line is: INT INT INT INT INT
	// all following are: UID [Material]

	// id & dimension
	char name[64];
	s->id = (int) GET_FLOAT();
	s->x = (int) GET_FLOAT();
	s->y = (int) GET_FLOAT();
	s->w = (int) GET_FLOAT();
	s->h = (int) GET_FLOAT();

	while( data[i] != '}' ) {
		STRIP_WHITE_SPACE();
		mapTile_t *tile = mapTile_t::New();
		tile->uid = (int) GET_FLOAT();
		STRIP_WHITE_SPACE();

		// get substitution material
		if ( data[i] == 'M' ) {

			// FIXME: add this when you start using it

			while ( data[i] != '}' )
				++i; // just skip it
			++i;
		}

		s->tiles.add( tile );
	}


	subAreas->add( s );

	STRIP_WHITE_SPACE();
	// closing bracket
	if ( data[i] != '}' ) {
		*j = i;
		console.Printf( "mapLoad: error: unexpected character" );
		return -2;
	}
	++i;
	*j = i;
	return 0;
}

int ME_ReadInMapTile( list_c<mapTile_t*> *tlist, int *j, char *data, int sz ) {
	int i = *j;

	bool inBracket = false;
	bool inParent = false;

	const char * tokens[] = { "Layer", "Material", "Poly", "ColModel", "Animation", "Background", "UID" };
	const int TOKENSIZE = sizeof(tokens) / sizeof(tokens[0]);
	int cur_tok = -1;

	mapTile_t *tile = mapTile_t::New();
	int count = 0;
	int type = 0;
	char name[256];
	material_t *mat_p = NULL;
	bool exit_cond = false;
	int badchar = 0;


	STRIP_WHITE_SPACE();
	// attempt to consume MapTile bracket '{'
	if ( data[i] != '{' ) {
		*j = i;
		console.Printf( "mapLoad: error: missing opening bracket" );
		return -2;
	}
	++i;


	while ( i < sz ) 
	{
		STRIP_WHITE_SPACE();

		switch ( data[i] ) {
		case '{': inBracket = true; 	++i; STRIP_WHITE_SPACE(); break;
		case '}': 
			if ( !inBracket ) { // 2nd bracket is exiting condition
				exit_cond = true;
				break;
			}
			inBracket = false; 			++i; STRIP_WHITE_SPACE(); break;
		case '(': inParent = true; 		++i; STRIP_WHITE_SPACE(); break;
		case ')': inParent = false; 	++i; STRIP_WHITE_SPACE(); break;
		case 'L':
			if ( CHECK_NAME( "Layer" ) ) {
				i += 5;
				STRIP_WHITE_SPACE();
				cur_tok = 0;
			}
			break;
		case 'M':
			if ( CHECK_NAME( "Material" ) ) {
				i += strlen( "Material" );
				STRIP_WHITE_SPACE();
				cur_tok = 1;
			}
			break;
		case 'P':
			if ( CHECK_NAME( "Poly" ) ) {
				i += 4;
				STRIP_WHITE_SPACE();
				cur_tok = 2;
			}
			break;
		case 'C':
			if ( CHECK_NAME( "ColModel" ) ) {
				i += strlen( "ColModel" );
				STRIP_WHITE_SPACE();
				cur_tok = 3;
			}
			break;
		case 'A':
			if ( CHECK_NAME( "Animation" ) ) {
				i += strlen( "Animation" );
				STRIP_WHITE_SPACE();
				cur_tok = 4;
			}
			break;
		case 'B':
			if ( CHECK_NAME( "Background" ) ) {
				i += strlen( "Background" );
				STRIP_WHITE_SPACE();
				cur_tok = 5;
			}
			break;
		case 'U':
			if ( CHECK_NAME( "UID" ) ) {
				i += 3;
				STRIP_WHITE_SPACE();
				cur_tok = 6;
			}
			break;
		default:
			break;
		}

		if ( cur_tok > -1 && cur_tok < TOKENSIZE && inBracket ) {
			// Poly 
			if ( !inParent && cur_tok == 2 ) {
				STRIP_WHITE_SPACE();
			} else if ( inParent && cur_tok == 2 ) {
				tile->poly.x = GET_FLOAT();
				tile->poly.y = GET_FLOAT();
				tile->poly.w = GET_FLOAT();
				tile->poly.h = GET_FLOAT();
				tile->poly.angle = GET_FLOAT();
				cur_tok = -1;
			// ColModel type
			} else if ( !inParent && cur_tok == 3 ) {
/* deprecated (or put off til later)
				if ( CHECK_NAME( "CM_AABB" ) ) {
					i += strlen( "CM_AABB" );
					type = CM_AABB;						
				} else if ( CHECK_NAME( "CM_OBB" ) ) {
					i += strlen( "CM_OBB" );
					type = CM_OBB;
				}
				STRIP_WHITE_SPACE();
				count = (int) GET_FLOAT();
				STRIP_WHITE_SPACE();
*/
				if ( !tile->col ) {
					//tile->col = colModel_t::New();
					tile->col = new colModel_t();
				}
			// ColModel Rows
			} else if ( inParent && cur_tok == 3 ) {
				tile->col->box[0] = GET_FLOAT();
				tile->col->box[1] = GET_FLOAT();
				tile->col->box[2] = GET_FLOAT();
				tile->col->box[3] = GET_FLOAT();
/* deprecated colModel
				float v[8];
				if ( type ) {
					v[0] = GET_FLOAT();
					v[1] = GET_FLOAT();
					v[2] = GET_FLOAT();
					v[3] = GET_FLOAT();
				}
				if ( type == CM_OBB ) {
					v[4] = GET_FLOAT();
					v[5] = GET_FLOAT();
					v[6] = GET_FLOAT();
					v[7] = GET_FLOAT();
				}
				if ( type == CM_AABB ) {
					tile->col->pushAABB( v );
					COPY4( tile->col->box, v );
				} else if ( type == CM_OBB ) {
					tile->col->pushOBB( v );
					OBB_TO_AABB_ENCLOSURE( tile->col->box, v );
				}
*/
				cur_tok = -1;
			// Layer
			} else if ( cur_tok == 0 ) {
				tile->layer = (int) GET_FLOAT();
				cur_tok = -1;
			// Background
			} else if ( cur_tok == 5 ) {
				tile->background = (int) GET_FLOAT();
				cur_tok = -1;
			
			// Material type
			} else if ( cur_tok == 1 ) {
				type = -1;
				for ( int x = 0; x < TOTAL_MATERIAL_TYPES; x++ ) {
					if ( CHECK_NAME( matNames[ x ].string ) ) {
						i += strlen( matNames[ x ].string );
						type = matNames[ x ].type; break;
					}
				}
				if ( type > -1 && type <= TOTAL_MATERIAL_TYPES ) {
					tile->mat = material_t::New();
					tile->mat->type = (materialType_t) type;
				}
				STRIP_WHITE_SPACE();

				// Material data
				int j = 0;
				switch ( type ) {
				case MTL_COLOR:
					if ( data[i] != '(' )
						break;
					++i;
					STRIP_WHITE_SPACE();
					tile->mat->color[j++] = GET_FLOAT();
					tile->mat->color[j++] = GET_FLOAT();
					tile->mat->color[j++] = GET_FLOAT();
					tile->mat->color[j++] = GET_FLOAT();
					STRIP_WHITE_SPACE();
					Assert ( data[i] == ')' ) ;
					++i;

					break;
				case MTL_TEXTURE_STATIC:
					memset( name, 0, sizeof(name) );
					GET_NAME();

					mat_p = materials.FindByName( name );
					if ( mat_p ) {
						memcpy( tile->mat, mat_p, sizeof(material_t) );
					}
					break;
				case MTL_COLORMASK:
					if ( data[i] != '(' )
						break;
					++i;
					STRIP_WHITE_SPACE();
					tile->mat->color[j++] = GET_FLOAT();
					tile->mat->color[j++] = GET_FLOAT();
					tile->mat->color[j++] = GET_FLOAT();
					tile->mat->color[j++] = GET_FLOAT();
					STRIP_WHITE_SPACE();
					Assert ( data[i] == ')' ) ;
					++i;
					STRIP_WHITE_SPACE();
					memset( name, 0, sizeof(name) );
					GET_NAME();

					mat_p = materials.FindByName( name );
					if ( mat_p ) {
						tile->mat->img = mat_p->img;
					}
					tile->mat->type = (materialType_t) type;
					break;
				case MTL_MASKED_TEXTURE:
					STRIP_WHITE_SPACE();
					if ( CHECK_NAME_i( "texture" ) ) {
						i += 7;
						STRIP_WHITE_SPACE();
						memset( name, 0, sizeof(name) );
						GET_NAME();
						mat_p = materials.FindByName( name );
						if ( mat_p ) {
							tile->mat->img = mat_p->img;
						}
						STRIP_WHITE_SPACE();
						if ( CHECK_NAME_i( "alphaMask" ) ) {
							i += 9;
							STRIP_WHITE_SPACE();
							memset( name, 0, sizeof(name) );
							GET_NAME();
							mat_p = materials.FindByName( name );
							if ( mat_p ) {
								tile->mat->mask = mat_p->img;
							}
						}
					} else if ( CHECK_NAME_i( "alphaMask" ) ) {
						i += 9;
						STRIP_WHITE_SPACE();
						memset( name, 0, sizeof(name) );
						GET_NAME();
						mat_p = materials.FindByName( name );
						if ( mat_p ) {
							tile->mat->mask = mat_p->img;
						}
						STRIP_WHITE_SPACE();
						if ( CHECK_NAME_i( "texture" ) ) {
							i += 7;
							STRIP_WHITE_SPACE();
							memset( name, 0, sizeof(name) );
							GET_NAME();
							mat_p = materials.FindByName( name );
							if ( mat_p ) {
								tile->mat->img = mat_p->img;
							}
						}
					}
					if ( !tile->mat->img || !tile->mat->mask ) {
						console.Printf( "masked texture missing either texture or mask" );
						return -1;
					}
					break;
				default:
					break;
				} // material type
				mat_p = NULL;
				cur_tok = -1;
			// Animation
			} else if ( cur_tok == 4 ) {
				// ...
				cur_tok = -1;
			} else if ( cur_tok == 6 ) {
				STRIP_WHITE_SPACE();
				tile->uid = (int) GET_FLOAT();
				cur_tok = -1;
			} else {
				// failsafe in case we get stuck on an unexpected character
				if ( ++badchar > 100 ) {
					exit_cond = true;
					break;
				}
			} // if
		} // switch
		if ( exit_cond )
			break;
	} // while

	if ( badchar > 100 ) {
		console.Printf( "mapLoad: error: too many bad chars" );
		return -1;
	}

	tlist->add( tile );
	*j = i;

	return 0;
}

static int _getBracket( int *j, char *data, int open =0 ) {
	if ( open ) {
		if ( data[*j] != '{' )
			return 0;
	} else {
		if ( data[*j] != '}' )
			return 0;
	}
	(*j)++;
	return 1;
}
#define GET_BRACKET( x ) _getBracket( &i, data, (x) )

static int _getParen( int *j, char *data, int open =0 ) {
	if ( open ) {
		if ( data[*j] != '(' )
			return 0;
	} else {
		if ( data[*j] != ')' )
			return 0;
	}
	++(*j);
	return 1;
}
#define GET_PAREN( x ) _getParen( &i, data, (x) )

int ME_ReadInEntity( buffer_c<Entity_t*> *ents, int *j, char * data, int sz ) {
	int i = *j;

	bool inBracket = false;
	bool inParent = false;

	int count = 0;
	int type = 0;

	// for use with the GET_NAME() macro
	char name[256] = { 0 };

	char entName[256] = { 0 };
	char entType[32] = { 0 };
	char entClass[32] = { 0 };
	material_t *mat_p = NULL;

	bool exit_cond = false;
	int badchar = 0;

	// header: name, type, class
	// opening bracket
	STRIP_WHITE_SPACE();
	if ( data[i] != '{' ) {
		*j = i;
		console.Printf( "readEntity: error: missing opening bracket" );
		return -1;
	}
	++i;

	// name
	STRIP_WHITE_SPACE();
	if ( CHECK_NAME_i( "name" ) ) {
		i += 4;
		STRIP_WHITE_SPACE();
		GET_NAME();
		strcpy( entName, name );
		STRIP_WHITE_SPACE();
	} else {
		sprintf( entName, "" );
	}

	// type
	STRIP_WHITE_SPACE();
	if ( CHECK_NAME_i( "type" ) ) {
		i += 4;
		STRIP_WHITE_SPACE();
		GET_NAME();
		strcpy( entType, name );
		STRIP_WHITE_SPACE();
	} else {
		sprintf( entType, "" );
	}

	// class
	STRIP_WHITE_SPACE();
	if ( CHECK_NAME_i( "class" ) ) {
		i += 5;
		STRIP_WHITE_SPACE();
		GET_NAME();
		strcpy( entClass, name );
		STRIP_WHITE_SPACE();
	} else {
		sprintf( entClass, "" );
	}

	if ( !entName[0] && !entType[0] && !entClass[0] ) {
		*j = i;
		console.Printf( "readEntity: error: missing header information" );
		return -2;
	}

	// allocate
	Entity_t *ent = G_NewEntFromToken( entType );
	strcpy( ent->name, entName );
	ent->entType = G_EntTypeFromToken( entType );
	ent->entClass = G_EntClassFromToken( entClass );

	while ( i < sz ) 
	{
		STRIP_WHITE_SPACE();

		switch ( data[i] ) {
		case '}': 
			exit_cond = true;
			break;
		case 'A':
			if ( CHECK_NAME( "Animation" ) ) {
				i += 9;
				STRIP_WHITE_SPACE();
				if ( !GET_BRACKET(1) ) {
					console.Printf( "error: malformed Animation" );
					return -3;
				}
				STRIP_WHITE_SPACE();
				if ( !GET_BRACKET(0) ) {
					console.Printf( "error: malformed Animation" );
					return -3;
				}
				STRIP_WHITE_SPACE();
			}
			break;
		case 'D':
		{
			if ( CHECK_NAME( "Door" ) ) {
				i += 4;

				Door_t *D = dynamic_cast<Door_t*>(ent);
				Assert( D != NULL );

				STRIP_WHITE_SPACE();
				if ( !GET_BRACKET(1) ) {
					console.Printf( "Door missing bracket" );
					return -4;
				}
				STRIP_WHITE_SPACE();
	
				if ( !CHECK_NAME_i( "state" ) ) {
					console.Printf( "error: malformed door entity" );
					return -666;
				}
				i += 5;
				STRIP_WHITE_SPACE();
				GET_NAME();
				if ( !strcmp( "closed", name ) ) {
					D->state = DOOR_CLOSED;
				} else if ( !strcmp( "open", name ) ) {
					D->state = DOOR_OPEN;
				} else {
					console.Printf( "error: unknown door state" );
					return -666;
				}
				STRIP_WHITE_SPACE();

				if ( !CHECK_NAME_i( "lock" ) ) {
					console.Printf( "error: malformed door entity" );
					return -666;
				}
				i += 4;

				// a door that is locked or has color is implicitly lockable
				D->type = 0;

				STRIP_WHITE_SPACE();
				GET_NAME();
				if ( !strcmp( "unlocked", name ) ) {
					D->lock = DOOR_UNLOCKED;
				} else if ( !strcmp( "locked", name ) ) {
					D->lock = DOOR_LOCKED;
					D->type |= DOOR_LOCKABLE;
				} else {
					console.Printf( "error: unknown door lock state" );
					return -666;
				}
				STRIP_WHITE_SPACE();

				if ( !CHECK_NAME_i( "type" ) ) {
					console.Printf( "error: malformed door entity" );
					return -666;
				}
				i += 4;
				STRIP_WHITE_SPACE();
				GET_NAME();
				if ( !strcmp( "proximity", name ) ) {
					D->type |= DOOR_PROXIMITY;
				} else if ( !strcmp( "manual", name ) ) {
					D->type |= DOOR_MANUAL;
				} else if ( !strcmp( "proxperm", name ) ) {
					D->type |= DOOR_PROXPERM;
				} else if ( !strcmp( "manperm", name ) ) {
					D->type |= DOOR_MANPERM;
				} else {
					console.Printf( "error: unknown door type" );
					return -666;
				}
				STRIP_WHITE_SPACE();

				if ( !CHECK_NAME_i( "dir" ) ) {
					console.Printf( "error: malformed door entity" );
					return -666;
				}
				i += 3;
				STRIP_WHITE_SPACE();
				GET_NAME();
				if ( !strcmp( "left", name ) ) {
					D->dir = DOOR_LEFT;
				} else if ( !strcmp( "right", name ) ) {
					D->dir = DOOR_RIGHT;
				} else if ( !strcmp( "up", name ) ) {
					D->dir = DOOR_UP;
				} else if ( !strcmp( "down", name ) ) {
					D->dir = DOOR_DOWN;
				} else {
					console.Printf( "error: unknown door lock" );
					return -666;
				}
				STRIP_WHITE_SPACE();

				if ( !CHECK_NAME_i( "doortime" ) ) {
					console.Printf( "error: malformed door entity" );
					return -666;
				}
				i += 8;
				STRIP_WHITE_SPACE();
				D->doortime = GET_FLOAT();
				STRIP_WHITE_SPACE();

				if ( !CHECK_NAME_i( "waittime" ) ) {
					console.Printf( "error: malformed door entity" );
					return -666;
				}
				i += 8;
				STRIP_WHITE_SPACE();
				D->waittime = GET_FLOAT();
				STRIP_WHITE_SPACE();

				if ( CHECK_NAME_i( "hitpoints" ) ) {
					i += 9;
					STRIP_WHITE_SPACE();
					D->hitpoints = GET_FLOAT();
					STRIP_WHITE_SPACE();
				} else
					D->hitpoints = 0;

				if ( CHECK_NAME_i( "use_portal" ) ) {
					i += 10;
					STRIP_WHITE_SPACE();
					D->use_portal = data[i]=='0'||data[i]=='f'||data[i]=='F' ? false : true;
					GET_NAME();
					STRIP_WHITE_SPACE();
				} else
					D->use_portal = false;

				D->color = DOOR_COLOR_NONE;
				if ( CHECK_NAME_i( "color" ) ) {
					i += 5;
					STRIP_WHITE_SPACE();
					GET_NAME();
					STRIP_WHITE_SPACE();
					if ( !strcmp( "red", name ) ) {
						D->color = DOOR_COLOR_RED;
					} else if ( !strcmp( "blue", name ) ) {
						D->color = DOOR_COLOR_BLUE;
					} else if ( !strcmp( "green", name ) ) {
						D->color = DOOR_COLOR_GREEN;
					} else if ( !strcmp( "silver", name ) ) {
						D->color = DOOR_COLOR_SILVER;
					} else if ( !strcmp( "gold", name ) ) {
						D->color = DOOR_COLOR_GOLD;
					} else {
						console.Printf( "error: malformed door color" );
						return -666;
					}
				} 
				if ( D->color != DOOR_COLOR_NONE ) {
					D->type |= DOOR_LOCKABLE;
				}

				STRIP_WHITE_SPACE();
				if ( !GET_BRACKET(0) ) {
					console.Printf( "Door missing bracket" );
					return -4;
				}
				STRIP_WHITE_SPACE();
			} // name == Door
			break;
		}
		case 'M':
		{
			if ( CHECK_NAME( "Material" ) ) {
				i += strlen( "Material" );
				STRIP_WHITE_SPACE();
				if ( !GET_BRACKET(1) ) {
					console.Printf( "error: material format" );
					return -4;
				}
				STRIP_WHITE_SPACE();

				type = -1;
				for ( int x = 0; x < TOTAL_MATERIAL_TYPES; x++ ) {
					if ( CHECK_NAME( matNames[ x ].string ) ) {
						i += strlen( matNames[ x ].string );
						type = matNames[ x ].type; break;
					}
				}
				if ( TOTAL_MATERIAL_TYPES == type ) {
					console.Printf( "error: couldn't recognize material type" );
					return -666;
				}
				ent->mat = material_t::New();
				ent->mat->type = (materialType_t) type;
				STRIP_WHITE_SPACE();

				// Material data
				int j = 0;
				switch ( type ) {
				case MTL_COLOR:
					if ( data[i] != '(' )
						break;
					++i;
					STRIP_WHITE_SPACE();
					ent->mat->color[j++] = GET_FLOAT();
					ent->mat->color[j++] = GET_FLOAT();
					ent->mat->color[j++] = GET_FLOAT();
					ent->mat->color[j++] = GET_FLOAT();
					STRIP_WHITE_SPACE();
					Assert ( data[i] == ')' ) ;
					++i;

					break;
				case MTL_TEXTURE_STATIC:
					memset( name, 0, sizeof(name) );
					GET_NAME();

					mat_p = materials.FindByName( name );
					if ( mat_p ) {
						memcpy( ent->mat, mat_p, sizeof(material_t) );
					}
					break;
				case MTL_COLORMASK:
					if ( data[i] != '(' )
						break;
					++i;
					STRIP_WHITE_SPACE();
					ent->mat->color[j++] = GET_FLOAT();
					ent->mat->color[j++] = GET_FLOAT();
					ent->mat->color[j++] = GET_FLOAT();
					ent->mat->color[j++] = GET_FLOAT();
					STRIP_WHITE_SPACE();
					Assert ( data[i] == ')' ) ;
					++i;
					STRIP_WHITE_SPACE();
					memset( name, 0, sizeof(name) );
					GET_NAME();

					mat_p = materials.FindByName( name );
					if ( mat_p ) {
						ent->mat->img = mat_p->img;
					}
					ent->mat->type = (materialType_t) type;
					break;
				case MTL_MASKED_TEXTURE:
					STRIP_WHITE_SPACE();
					if ( CHECK_NAME_i( "texture" ) ) {
						i += 7;
						STRIP_WHITE_SPACE();
						memset( name, 0, sizeof(name) );
						GET_NAME();
						mat_p = materials.FindByName( name );
						if ( mat_p ) {
							ent->mat->img = mat_p->img;
						}
						STRIP_WHITE_SPACE();
						if ( CHECK_NAME_i( "alphaMask" ) ) {
							i += 9;
							STRIP_WHITE_SPACE();
							memset( name, 0, sizeof(name) );
							GET_NAME();
							mat_p = materials.FindByName( name );
							if ( mat_p ) {
								ent->mat->mask = mat_p->img;
							}
						}
					} else if ( CHECK_NAME_i( "alphaMask" ) ) {
						i += 9;
						STRIP_WHITE_SPACE();
						memset( name, 0, sizeof(name) );
						GET_NAME();
						mat_p = materials.FindByName( name );
						if ( mat_p ) {
							ent->mat->mask = mat_p->img;
						}
						STRIP_WHITE_SPACE();
						if ( CHECK_NAME_i( "texture" ) ) {
							i += 7;
							STRIP_WHITE_SPACE();
							memset( name, 0, sizeof(name) );
							GET_NAME();
							mat_p = materials.FindByName( name );
							if ( mat_p ) {
								ent->mat->img = mat_p->img;
							}
						}
					} // material types
					if ( !ent->mat->img || !ent->mat->mask ) {
						console.Printf( "masked texture missing either texture or mask" );
						return -1;
					}
					break;
				default:
					break;
				} // switch material types
				mat_p = NULL;

				STRIP_WHITE_SPACE();
				if ( !GET_BRACKET(0) ) {
					console.Printf( "error: material format" );
					return -4;
				}
				STRIP_WHITE_SPACE();
				break;
			}
			break;
		}
		case 'P':
		{
			if ( CHECK_NAME( "Poly" ) ) {
				i += 4;

				STRIP_WHITE_SPACE();
				if ( !GET_BRACKET(1) ) {
					console.Printf( "Poly missing bracket" );
					return -4;
				}
				STRIP_WHITE_SPACE();
				if ( !GET_PAREN(1) ) {
					console.Printf( "Poly missing parentheses" );
					return -4;
				}
				STRIP_WHITE_SPACE();

				ent->poly.x = GET_FLOAT();
				ent->poly.y = GET_FLOAT();
				ent->poly.w = GET_FLOAT();
				ent->poly.h = GET_FLOAT();
				ent->poly.angle = GET_FLOAT();

				STRIP_WHITE_SPACE();
				if ( !GET_PAREN(0) ) {
					console.Printf( "Poly missing parentheses" );
					return -4;
				}
				STRIP_WHITE_SPACE();
				if ( !GET_BRACKET(0) ) {
					console.Printf( "Poly missing bracket" );
					return -4;
				}
				STRIP_WHITE_SPACE();
			}

			else if ( CHECK_NAME_i( "Portal" ) ) {
				i += 6;

				Portal_t *P = dynamic_cast<Portal_t*>(ent);
				Assert( P != NULL );

				STRIP_WHITE_SPACE();
				if ( !GET_BRACKET(1) ) {
					console.Printf( "missing bracket" );
					return -4;
				}
				STRIP_WHITE_SPACE();

				if ( CHECK_NAME_i( "area" ) ) {
					i += 4;
				} else {
					Assert( !("Can't find area label") );
				}
				STRIP_WHITE_SPACE();
				GET_NAME();
				strcpy( P->area , name );

				STRIP_WHITE_SPACE();
				if ( CHECK_NAME_i( "spawn" ) ) {
					i += 5;
				} else {
					Assert( !("Can't find spawn label in Portal") );
				}
				STRIP_WHITE_SPACE();
				GET_NAME();
				strcpy( P->spawn , name );

				STRIP_WHITE_SPACE();
				if ( CHECK_NAME_i( "type" ) ) {
					i += 4;
				} else {
					Assert( !("Can't find type label in Portal") );
				}
				STRIP_WHITE_SPACE();
				GET_NAME();
				if ( !strcmp( name, "fast" ) ) {
					P->type = PRTL_FAST;
				} else if ( !strcmp( name, "freeze" ) ) {
					P->type = PRTL_FREEZE;
				} else if ( !strcmp( name, "wait" ) ) {
					P->type = PRTL_WAIT;
				} else if ( !strcmp( name, "freeze_wait" ) ) {
					P->type = PRTL_FREEZEWAIT;
				} else {
					Assert( !("Can't decipher type in Portal") );
				}
				STRIP_WHITE_SPACE();

				// optional parms
				if ( CHECK_NAME_i( "freeze" ) ) {
					i += 6;
					STRIP_WHITE_SPACE();
					P->freeze = GET_FLOAT();
					STRIP_WHITE_SPACE();
				}
				if ( CHECK_NAME_i( "wait" ) ) {
					i += 4;
					STRIP_WHITE_SPACE();
					P->wait = GET_FLOAT();
					STRIP_WHITE_SPACE();
				}

				STRIP_WHITE_SPACE();
				if ( !GET_BRACKET(0) ) {
					console.Printf( "missing bracket" );
					return -4;
				}
				STRIP_WHITE_SPACE();
			}
			break;
		}
		case 'C':
		{
			if ( CHECK_NAME( "ColModel" ) ) {
				i += strlen( "ColModel" );

				STRIP_WHITE_SPACE();
				if ( !GET_BRACKET(1) ) {
					console.Printf( "ColModel missing bracket" );
					return -4;
				}
				STRIP_WHITE_SPACE();
				if ( !GET_PAREN(1) ) {
					console.Printf( "ColModel missing parentheses" );
					return -4;
				}
				STRIP_WHITE_SPACE();

				if ( !ent->col ) {
					ent->col = new colModel_t();
				}
				ent->col->box[0] = GET_FLOAT();
				ent->col->box[1] = GET_FLOAT();
				ent->col->box[2] = GET_FLOAT();
				ent->col->box[3] = GET_FLOAT();

/* in a perfect world, I'd have time to do this.  maybe later
				if ( CHECK_NAME( "CM_AABB" ) ) {
					i += strlen( "CM_AABB" );
					type = CM_AABB;						
				} else if ( CHECK_NAME( "CM_OBB" ) ) {
					i += strlen( "CM_OBB" );
					type = CM_OBB;
				}
				STRIP_WHITE_SPACE();
				count = (int) GET_FLOAT();
				STRIP_WHITE_SPACE();

				if ( !ent->col ) {
					ent->col = colModel_t::New();
				}

				float v[8];
				if ( type ) {
					v[0] = GET_FLOAT();
					v[1] = GET_FLOAT();
					v[2] = GET_FLOAT();
					v[3] = GET_FLOAT();
				}
				if ( type == CM_OBB ) {
					v[4] = GET_FLOAT();
					v[5] = GET_FLOAT();
					v[6] = GET_FLOAT();
					v[7] = GET_FLOAT();
				}
				if ( type == CM_AABB ) {
					ent->col->pushAABB( v );
				} else if ( type == CM_OBB ) {
					ent->col->pushOBB( v );
				}
*/

				STRIP_WHITE_SPACE();
				if ( !GET_PAREN(0) ) {
					console.Printf( "ColModel missing parentheses" );
					return -4;
				}
				STRIP_WHITE_SPACE();
				if ( !GET_BRACKET(0) ) {
					console.Printf( "ColModel missing bracket" );
					return -4;
				}
				STRIP_WHITE_SPACE();

			} else if ( CHECK_NAME( "Collidable" ) ) {
				i += strlen( "Collidable" );

				STRIP_WHITE_SPACE();
				if ( !GET_BRACKET(1) ) {
					console.Printf( "Collidable missing bracket" );
					return -4;
				}
				STRIP_WHITE_SPACE();
				ent->collidable = GET_FLOAT();
				STRIP_WHITE_SPACE();
				if ( !GET_BRACKET(0) ) {
					console.Printf( "Collidable missing bracket" );
					return -4;
				}
				STRIP_WHITE_SPACE();
			} else if ( CHECK_NAME_i( "Clip" ) ) {
				i += 4;

				STRIP_WHITE_SPACE();
				if ( !GET_BRACKET(1) ) {
					console.Printf( "Clip missing bracket" );
					return -4;
				}
				STRIP_WHITE_SPACE();
				ent->clip[0] = GET_FLOAT();
				ent->clip[1] = GET_FLOAT();
				ent->clip[2] = GET_FLOAT();
				ent->clip[3] = GET_FLOAT();
				STRIP_WHITE_SPACE();
				if ( !GET_BRACKET(0) ) {
					console.Printf( "Clip missing bracket" );
					return -4;
				}
				STRIP_WHITE_SPACE();
			}
			break;
		}
		case 'S':
		{
			if ( CHECK_NAME( "Spawn" ) ) {
				i += 5;

				Spawn_t *s = dynamic_cast<Spawn_t*>(ent);
				Assert( s != NULL );

				STRIP_WHITE_SPACE();
				if ( !GET_BRACKET(1) ) {
					console.Printf( "Spawn missing bracket" );
					return -4;
				}
				STRIP_WHITE_SPACE();

				if ( CHECK_NAME_i( "type" ) ) {
					i += 4;
				} else if ( CHECK_NAME_i( "spawntype" ) ) {
					i += 9;
				} else {
					Assert( !("Can't find spawn type label") );
				}
				STRIP_WHITE_SPACE();

				GET_NAME();
				if ( !strcmp( name, "ST_WORLD" ) ) {
					s->type = ST_WORLD;
				} else if ( !strcmp( name, "ST_AREA" ) ) {
					s->type = ST_AREA;
				} else {
					Assert( !("still not with it" ) );
				}
				STRIP_WHITE_SPACE();

				// OPTIONAL PARM
				if ( CHECK_NAME_i( "view" ) ) {
					i += 4;
					STRIP_WHITE_SPACE();
					s->view[0] = GET_FLOAT();
					s->view[1] = GET_FLOAT();
					s->view[2] = GET_FLOAT();
					s->view[3] = GET_FLOAT();
					s->zoom = GET_FLOAT();
					STRIP_WHITE_SPACE();
					s->set_view = true;
				}

				// OPTIONAL PARM
				if ( CHECK_NAME_i( "speed" ) ) {
					i += 5;
					STRIP_WHITE_SPACE();
					GET_NAME();
					strncpy( s->pl_speed, name, 31 );
					s->pl_speed[31] = '\0';
					STRIP_WHITE_SPACE();
				}

				if ( !GET_BRACKET(0) ) {
					console.Printf( "Spawn missing bracket" );
					return -4;
				}
				STRIP_WHITE_SPACE();
			}
			break;
		}
		case 'T':
			if ( CHECK_NAME_i( "Trigger" ) ) {
				i += 7;
				STRIP_WHITE_SPACE();
				if ( !GET_BRACKET(1) ) {
					console.Printf( "Trigger missing bracket" );
					return -4;
				}
				STRIP_WHITE_SPACE();
				ent->trig[0] = GET_FLOAT();
				ent->trig[1] = GET_FLOAT();
				ent->trig[2] = GET_FLOAT();
				ent->trig[3] = GET_FLOAT();
				STRIP_WHITE_SPACE();
				if ( !GET_BRACKET(0) ) {
					console.Printf( "Trigger missing bracket" );
					return -4;
				}
				STRIP_WHITE_SPACE();
			}

		default:
			++badchar;
			break;
		} // switch

		if ( exit_cond )
			break;
		if ( badchar > 100 ) {
			console.Printf( "bad chars encountered in map" );
			return -777;
		}

	} // while

	// create a default colModel off of poly  if none were provided
	if ( !ent->col ) {
		ent->col = new colModel_t( ent->poly );
	}

	ent->populateGeometry( 8 /* don't touch col, clip, or trig */ );

	ents->add( ent );
	*j = i;
	return 0;
}

char me_mapScript[ 255 ];

int ME_LoadMap( const char *fname ) {

	char buf[512];
	buf[0] = 0;
	snprintf( buf, 512, "%s\\maps\\%s", fs_gamepath->string(), fname );

	FILE *fp = fopen ( buf, "rb" );
	if ( !fp ) {
		return -1;
	}

	int sz = Com_FileSize( buf );
	if ( !sz ) {
		fclose ( fp );
		return -2;
	}
	char *data = (char*) V_Malloc( ( sz + 64 ) & ~31 );

	fread ( data, 1, sz , fp );
	fclose( fp );

	// load map into a temporary list.  once verified transfer it into the
	//  main list
	list_c<mapTile_t*> tlist;
	tlist.init();
	list_c<Area_t*> areas;
	areas.init();
	list_c<me_subArea_t*> subAreas;
	subAreas.init();
	buffer_c<Entity_t*> ents;
	ents.init();

	int my_errno = 0;

	goto next;

exit_on_err:
	V_Free( data );
	tlist.destroy();
	areas.destroy();
	subAreas.destroy();
	ents.destroy();
	return my_errno;

next:

	// important variable! don't use for odd-jobs!
	int i = 0;

	float x, y, w, h, a;
	char name[512];

	int version = -1;

    me_mapScript[0] = 0;

	while ( i < sz ) {
	
		STRIP_WHITE_SPACE();
	
		switch( data[i] ) {
		// get Version
		case 'V':
			version = CHECK_VERSION();
			break;
		case 'M':
 			if ( CHECK_MAPTILE() ) {
				if ( 0 != ME_ReadInMapTile( &tlist, &i, data, sz ) ) {
					console.Printf( "error reading mapTile. abandoning mapLoad" );
					my_errno = -4;
					goto exit_on_err;
				}
			}
			break;
		case 'E':
			if ( CHECK_NAME_i( "Entity" ) ) {
				i += 6;
				if ( 0 != ME_ReadInEntity( &ents, &i, data, sz ) ) {
					console.Printf( "error reading entity. abandoning mapLoad" );
					my_errno = -7;
					goto exit_on_err;
				}
			}
			break;
		case 'A':
			if ( CHECK_NAME_i( "Area" ) ) {
				i += 4;
				if ( ME_ReadInArea( &areas, &i, data, sz ) ) {
					my_errno = -5;
					goto exit_on_err;
				}
			}
			break;
		case 's':
			if ( CHECK_NAME_i( "subArea" ) ) {
				i += 7;
				if ( ME_ReadInSubArea( &subAreas, &i, data, sz ) ) {
					my_errno = -6;
					goto exit_on_err;
				}
			} else if ( CHECK_NAME_i( "script" ) ) {
                i += 6;
                GET_NAME();
                strcpy( me_mapScript, name );
            }
			break;
		default:
			++i; // consume garbage char after trying everything else
			break;
		}
	}

	V_Free( data );

	// double check we have version
	if ( version <= 0 ) {
		console.Printf( "error: loading map \"%s\", couldn't get version", fname );
		my_errno = -3;
		goto exit_on_err;
	}

	// no data loaded 
	if ( tlist.size() == 0 && 0 == ents.size() ) {
		console.Printf( "loaded no data" );
		my_errno = -4;
		goto exit_on_err;
	}

	// NO Palettes when IN GAME !!
	if ( com_editor->integer() ) {
		// if it's not the default map OR we have modified the default map at all 
		if ( strcmp( me_activeMap->string(), "_default" ) || tiles->size() > 0 ) {
			// create a new palette to load the map into and keep the default map
			palette.NewPalette( fname );
		} else {
			// set active map ourselves
			palette.ChangeCurrentName( fname );
		}
	} else {
		me_activeMap->set( "me_activeMap", fname, 0 );
	}
	
	// clear map editor globals: current list and selected
	selected.reset();
	tiles->reset();
	me_areas.reset();
	me_subAreas.reset();
	me_ents->reset();

	// transfer list (look for backgrounds while doing)
	node_c<mapTile_t*> *tile = tlist.gethead();
	while ( tile ) {
		tiles->add( tile->val );
		if ( tile->val->background ) {
			selected.add_noblock( tiles->gettail() );
		}
		tile = tile->next;
	}
	console.Printf( "%s read: %u tiles loaded" , fname, tiles->size() );

	ME_SetBackgrounds(); // create labels for background tiles

	// transfer areas
	node_c<Area_t*> *at = areas.gethead();
	while( at ) {
		me_areas.add( at->val );
		at = at->next;
	}
	node_c<me_subArea_t*> *s = subAreas.gethead();
	while( s ) {
		me_subAreas.add( s->val );
		s = s->next;
	}

	// ents
	for ( int i = 0 ; i < ents.length(); i++ ) {
		me_ents->add( ents.data[i] );
	}

	my_errno = 0;
	goto exit_on_err;
}


static void ME_SaveCancel( void ) {
	menu.clearFloatingBoxes();
}



void ME_SpawnMsgBox( const char *msg, void(*f)(void) ) {
	msgBox.reset();
	strcpy( msgBox.label, msg );
	msgBox.flag = TE_BLOCKING;
	msgBox.setOKFunc( f );
	menu.registerFloating( &msgBox );
}

void ME_SaveMsgBoxOK( void ) {	
	menu.clearFloatingBoxes();
}

static bool ME_SaveOK( const char *name, int len, int nospawn =0 ) {
	if ( len > 0 ) {
		char buf[128];

		if ( name[len-1] == 'p' && name[len-2] == 'a' && name[len-3] == 'm' && name[len-4] == '.' ) {
			strcpy( buf, name );
		} else {
			snprintf( buf, 128, "%s.map", name );
		}
		ME_SaveMap( buf );

		// set the me_activeMap gvar
		palette.ChangeCurrentName( buf );

		// Spawn a MessageBox informing that the save of filename : %s was successful

		// when the OK on the messageBox is clicked, then call menu.clearFloatingBoxes()
		if ( !nospawn ) {
			char msg[256];
			snprintf( msg, 256, "%s Saved", buf );
			menu.clearFloatingBoxes();
			ME_SpawnMsgBox( msg, ME_SaveMsgBoxOK );
		}
		return true;
	}
	return false;
}

bool ME_SaveMapCheckExtension( const char *name ) {
	return ME_SaveOK( name, strlen( name ), 1 );
}

void ME_SpawnLoadDialog( void );

void ME_LoadError( void ) {
	menu.clearFloatingBoxes();
	ME_SpawnLoadDialog();
}

static bool ME_LoadOK( const char *name, int size, int nospawn =0 ) {
	char buf[256];
	if ( name[size-4] == '.' && name[size-3] == 'm' && name[size-2] == 'a' && name[size-1] == 'p' ) {
		strcpy( buf, name );
	} else {
		snprintf( buf, 256, "%s.map", name );
	}



	// success
	if ( 0 == ME_LoadMap( buf ) ) {
		menu.clearFloatingBoxes();
		me_activeMap->set( "me_activeMap", buf, 0 );
		return true;
	}


	menu.clearFloatingBoxes();

	if ( !nospawn ) {
		char buf2[256];
		snprintf( buf2, 256, "File Not Found: %s", buf );
		ME_SpawnMsgBox( buf2, ME_LoadError );
	}

	return false;
}

int CL_LoadMap( const char * );

/*
====================
 ME_LoadMapCheckExtension

	loads differently depending on whether in map editor or normal
====================
*/
bool ME_LoadMapCheckExtension( const char *name ) {
	if ( com_editor->integer() ) {
		return ME_LoadOK( name, strlen( name ), 1 );
	} else {
		return CL_LoadMap( name ) == 0;
	}
}

void ME_LoadCancel( void ) {
	menu.clearFloatingBoxes();
}



void ME_SpawnSaveDialog( void ) {
	//textEntryBox_t *tfield = (textEntryBox_t*) V_Malloc( sizeof(textEntryBox_t) ) ;
	//tfield( TE_BLOCKING );

	//saveTextBox = (textEntryBox_t*) V_Malloc( sizeof(textEntryBox_t) ) ;


	saveTextBox.reset();

	// set internals before registering 
	saveTextBox.flag = TE_BLOCKING;
	saveTextBox.setLabel( "Enter a name to save your map as:" );
	saveTextBox.setOKCancelFuncs( ME_SaveOK, ME_SaveCancel );

	// menu sets its location & dimension
	menu.registerFloating( &saveTextBox );
}

/*
*/
void ME_SpawnLoadDialog( void ) {
	saveTextBox.reset();
	saveTextBox.flag = TE_BLOCKING;
	saveTextBox.setLabel( "Load which file? " );
	saveTextBox.setOKCancelFuncs( ME_LoadOK, ME_LoadCancel );
	menu.registerFloating( &saveTextBox );
}

bool ME_DelPaletteOK( const char *name, int size, int nospawn =0 ) {
	palette.DeleteCurrent();
	// Spawn an OK box with a string: "\"name\" deleted"
	return true;
}
void ME_DelPaletteCancel( void ) {
	ME_LoadCancel();
}

int ME_SpawnDeletePaletteDialog( void ) {
	// shit, I need an OK/CANCEL box with static text lable

	// "are you sure you want to delete this palette: "name" ? "
	// "OK" "CANCEL"


	saveTextBox.reset();
	saveTextBox.flag = TE_BLOCKING;
	char buf[256];
	sprintf( buf, "%s", "Are you sure you want to delete the current palette: \"%s\"?", me_activeMap->string() );
	saveTextBox.setLabel( buf );
	saveTextBox.setOKCancelFuncs( ME_DelPaletteOK, ME_DelPaletteCancel );
	menu.registerFloating( &saveTextBox );
	return 1;
}

// wrapper for ME_LoadMap, that will automatically start local
//  data structures necessary to read in a map
int ME_ReadMapForClient( const char *mapname ) {
	if ( !tiles ) {
		tiles = (tilelist_t*) V_Malloc( sizeof(tilelist_t) );
		tiles->init(); 
	} else
		tiles->reset();

	if ( !me_ents ) {
		me_ents = (entlist_t*) V_Malloc( sizeof(entlist_t) );
		me_ents->init(); 
	} else
		me_ents->reset();

	if ( selected.isstarted() ) 
		selected.reset(); 
	else 
		selected.init();

	if ( me_areas.isstarted() ) 
		me_areas.reset(); 
	else 
		me_areas.init();

	if ( me_subAreas.isstarted() ) 
		me_subAreas.reset(); 
	else 
		me_subAreas.init();

	char buf[256];
	unsigned int size = strlen( mapname );
	if ( mapname[size-4] == '.' && mapname[size-3] == 'm' && mapname[size-2] == 'a' && mapname[size-1] == 'p' ) {
		strcpy( buf, mapname );
	} else {
		snprintf( buf, 256, "%s.map", mapname );
	}
				
	return ME_LoadMap( buf ); 
}
