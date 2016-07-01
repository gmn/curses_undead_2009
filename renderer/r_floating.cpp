// r_floating.cpp
//

#include "r_floating.h"
#include "../common/com_types.h"
#include "../client/cl_public.h"
#include "r_ggl.h"
#include "../map/mapdef.h"

//
// floatingText_c
//

int floatingText_c::clientID = 0;

/*
====================
 floatText_c::AddTileText

	add a new per-tile text
====================
*/ 
void floatingText_c::AddTileText( mapTile_t *tile, const char *str, float x, float y, int sz, int duration, int cl_id ) {
	ftext_t * ft = new ftext_t;
	strcpy( ft->text, str );
	int size = ( 0 == sz ) ? currentSize : sz ;
	ft->scale = (float)size / (float)F_POINT_SZ; 
	ft->x = x;
	ft->y = y;
	ft->tile_uid = tile->uid;
	ft->font = this->currentFont;
	ft->client_id = cl_id;
	if ( 0 == duration ) {
		ft->timer.reset();
	} else {
		ft->timer.set( duration, FLOATING_INTRO );
	}

	/*
	// concatenate tile pointer w/ str, to create a unique identifier
	char buf[256];
	int len;
	genUniqueString( tile, str, buf, &len );
	tile_text.Insert( ft, buf, len );
	*/
	tile_text.Insert( ft, (const char *)&tile, sizeof(mapTile_t*) );
}

/*
====================
 floatText_c::AddEntText

	add a new per-entity text
====================
*/ 
void floatingText_c::AddEntText( Entity_t *ent, const char *str, float x, float y, int sz, int duration, int cl_id ) {
	ftext_t * ft = new ftext_t;
	strcpy( ft->text, str );
	int size = ( 0 == sz ) ? currentSize : sz ;
	ft->scale = (float)size / (float)F_POINT_SZ; 
	ft->x = x;
	ft->y = y;
	ft->tile_uid = (unsigned int)ent;
	ft->font = this->currentFont;
	ft->client_id = cl_id;
	if ( 0 == duration ) {
		ft->timer.reset();
	} else {
		ft->timer.set( duration, FLOATING_INTRO );
	}
	tile_text.Insert( ft, (const char *)&ent, sizeof(Entity_t*) );
}

/*
====================
 floatText_c::AddAreaText
====================
*/ 
void floatingText_c::AddAreaText( Area_t *area, const char *str, float x, float y, int sz, int duration, int cl_id ) {
	ftext_t * ft = new ftext_t;
	strcpy( ft->text, str );
	int size = ( 0 == sz ) ? currentSize : sz ;
	ft->scale = (float)size / (float)F_POINT_SZ; 
	ft->x = x;
	ft->y = y;
	ft->tile_uid = (unsigned int)area;
	ft->font = this->currentFont;
	ft->client_id = cl_id;
	if ( 0 == duration ) {
		ft->timer.reset();
	} else {
		ft->timer.set( duration, FLOATING_INTRO );
	}
	tile_text.Insert( ft, (const char *)&area, sizeof(Area_t*) );
}

/*
====================
 floatText_c::genUniqueString
====================
*/ 
void floatingText_c::genUniqueString( mapTile_t *tile, const char *str, char *buf, int *len ) {
	char *p = buf;
	const char *c = (const char *) &tile;
	*p++ = *c++;
	*p++ = *c++;
	*p++ = *c++;
	*p++ = *c++;
	c = str;
	while ( *c ) {
		*p++ = *c++;
	}
	*len = p - buf;
}

/*
====================
 floatText_c::AddCoordText
====================
*/ 
void floatingText_c::AddCoordText( float x, float y, const char *str, int sz, int duration, int cl_id ) {
	ftext_t *ft = new ftext_t;
	strcpy( ft->text, str );
	int size = ( 0 == sz ) ? currentSize : sz ;
	ft->scale = (float)size / (float)F_POINT_SZ; 
	ft->x = x;
	ft->y = y;
	ft->font = this->currentFont;
	ft->client_id = cl_id;
	if ( 0 == duration ) {
		ft->timer.reset();
	} else {
		ft->timer.set( duration, FLOATING_INTRO );
	}
	coord_text.add( ft );
}

/*
====================
 floatText_c::DrawTiles
====================
*/ 
void floatingText_c::DrawTiles( tilelist_t *tilelist ) {
	if ( tile_text.size() == 0 )
		return;
	
	ftext_t *ft = NULL;
	node_c<mapTile_t *> *node = tilelist->gethead();
	while ( node ) {

		int i = 0;
		while ( (ft = tile_text.FindAll( (const char *)&node->val, sizeof(mapTile_t*), &i )) ) {

			Assert( ft->tile_uid == node->val->uid );
			
			// if the tile's text has expired, remove it, continue
			if ( ft->timer.flags && ft->timer.timeup() ) {
				if ( ft->timer.flags == FLOATING_TRAILOFF ) {
					tile_text.DeleteByTableReference( ft, (const char *)&node->val, sizeof(mapTile_t*) );
					--i; // re-adjust the findAll param
					continue; // look again
				}

				if ( ft->timer.flags == FLOATING_INTRO ) {
					ft->timer.set( trailOff, FLOATING_TRAILOFF );
				}
			}

			// with the x, y offsets into the tile, and the tile location
			// we have the text location,
			float xxyy[2] = { node->val->poly.x + ft->x, node->val->poly.y + ft->y };
			M_WorldToScreen( xxyy );

			// check to see if it is an expiring type, if so, determine
			//  linear fade value
			float ratio = 1.0f;
			if ( ft->timer.flags == FLOATING_TRAILOFF ) {
				ratio = 1.0f - ft->timer.ratio();
			}
			gglColor4f( 1.0f, 1.0f, 1.0f, ratio );

			// draw
			F_SetScale( ft->scale );
			F_Printf( xxyy[0], xxyy[1], ft->font, "%s", ft->text );
			F_ScaleDefault();
		}

		node = node->next;
	}

	gglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
}

/*
====================
 floatText_c::DrawEnts
====================
*/ 
void floatingText_c::DrawEnts( entlist_t *ents ) {
	if ( tile_text.size() == 0 )
		return;
	
	ftext_t *ft = NULL;
	node_c<Entity_t *> *node = ents->gethead();
	while ( node ) {

		int i = 0;
		while ( (ft = tile_text.FindAll( (const char *)&node->val, sizeof(Entity_t*), &i )) ) {

			Assert( ft->tile_uid == (unsigned int)node->val );
			
			// if the tile's text has expired, remove it, continue
			if ( ft->timer.flags && ft->timer.timeup() ) {
				if ( ft->timer.flags == FLOATING_TRAILOFF ) {
					tile_text.DeleteByTableReference( ft, (const char *)&node->val, sizeof(Entity_t*) );
					--i; // re-adjust the findAll param
					continue; // look again
				}

				if ( ft->timer.flags == FLOATING_INTRO ) {
					ft->timer.set( trailOff, FLOATING_TRAILOFF );
				}
			}

			// with the x, y offsets into the tile, and the tile location
			// we have the text location,
			float xxyy[2] = { node->val->poly.x + ft->x, node->val->poly.y + ft->y };
			M_WorldToScreen( xxyy );

			// check to see if it is an expiring type, if so, determine
			//  linear fade value
			float ratio = 1.0f;
			if ( ft->timer.flags == FLOATING_TRAILOFF ) {
				ratio = 1.0f - ft->timer.ratio();
			}
			gglColor4f( 1.0f, 1.0f, 1.0f, ratio );

			// draw
			F_SetScale( ft->scale );
			F_Printf( xxyy[0], xxyy[1], ft->font, "%s", ft->text );
			F_ScaleDefault();
		}

		node = node->next;
	}

	gglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
}

/*
====================
 floatText_c::DrawAreas
====================
*/ 
void floatingText_c::DrawAreas( list_c<Area_t*> *areas ) {
	if ( tile_text.size() == 0 )
		return;
	
	ftext_t *ft = NULL;
	node_c<Area_t *> *node = areas->gethead();
	while ( node ) {

		int i = 0;
		while ( (ft = tile_text.FindAll( (const char *)&node->val, sizeof(Area_t*), &i )) ) {

			Assert( ft->tile_uid == (unsigned int)node->val );
			
			// if the tile's text has expired, remove it, continue
			if ( ft->timer.flags && ft->timer.timeup() ) {
				if ( ft->timer.flags == FLOATING_TRAILOFF ) {
					tile_text.DeleteByTableReference( ft, (const char *)&node->val, sizeof(Area_t*) );
					--i; // re-adjust the findAll param
					continue; // look again
				}

				if ( ft->timer.flags == FLOATING_INTRO ) {
					ft->timer.set( trailOff, FLOATING_TRAILOFF );
				}
			}

			// with the x, y offsets into the tile, and the tile location
			// we have the text location,
			float xxyy[2] = { node->val->p1[0] + ft->x, node->val->p1[1] + ft->y };
			M_WorldToScreen( xxyy );

			// check to see if it is an expiring type, if so, determine
			//  linear fade value
			float ratio = 1.0f;
			if ( ft->timer.flags == FLOATING_TRAILOFF ) {
				ratio = 1.0f - ft->timer.ratio();
			}
			gglColor4f( 1.0f, 1.0f, 1.0f, ratio );

			// draw
			F_SetScale( ft->scale );
			F_Printf( xxyy[0], xxyy[1], ft->font, "%s", ft->text );
			F_ScaleDefault();
		}

		node = node->next;
	}

	gglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
}


/*
====================
 DrawCoords

====================
*/
void floatingText_c::DrawCoords( void ) {
	node_c<ftext_t*> *n = coord_text.gethead();
	while ( n ) {
		ftext_t *ft = n->val;

		if ( ft->timer.flags == FLOATING_TRAILOFF && ft->timer.timeup() ) {
			node_c<ftext_t*> *tmp = n->next;
			coord_text.popnode( n );
			n = tmp;
			continue;
		}
		if ( ft->timer.flags == FLOATING_INTRO && ft->timer.timeup() ) {
			ft->timer.set( trailOff, FLOATING_TRAILOFF );
		}

		float ratio = 1.0f;
		if ( ft->timer.flags == FLOATING_TRAILOFF ) {
			ratio = 1.0f - ft->timer.ratio();
		}
		gglColor4f( 1.0f, 1.0f, 1.0f, ratio );

		F_SetScale( ft->scale );
		F_Printf( ft->x, ft->y, ft->font, "%s", ft->text );
		F_ScaleDefault();

		n = n->next;
	}
}

/*
====================
 floatingText_c::ClearClientID
====================
*/
void floatingText_c::ClearClientID( int client_id ) {
	// all client_ids are non-zero
	if ( !client_id )
		return;

	// go through both lists and remove any ftext_t nodes with matching id
	node_c<ftext_t*> *n = coord_text.gethead();
	while ( n ) {
		if ( n->val->client_id == client_id ) {
			node_c<ftext_t*> *tmp_next = n->next;
			coord_text.popnode( n );
			n = tmp_next;
			continue;
		}
		n = n->next;
	}

	if ( tile_text.size() == 0 )
		return;

	vec_c<hash_c<ftext_t*>::hashNode_t*> &table = tile_text.GetTable();
	int i = 0;
	while ( i < tile_text.HASH_SIZE ) {
		if ( table.vec[ i ] ) {
			hash_c<ftext_t*>::hashNode_t *p = table.vec[ i ];
			while ( p ) {
				if ( p->val->client_id == client_id ) {
					hash_c<ftext_t*>::hashNode_t *tmp = p->next;
					tile_text.Remove( p->unique, p->len );
					p = tmp;
				} else {
					p = p->next;
				}
			}
		}
		++i;
	}
}

/*
====================
 ClearUID

 clears first text it finds with uid and returns
====================
*/
void floatingText_c::ClearUID( int uid ) {

	// go through both lists and remove any ftext_t nodes with matching id
	node_c<ftext_t*> *n = coord_text.gethead();
	while ( n ) {
		if ( n->val->tile_uid == uid ) {
			node_c<ftext_t*> *tmp_next = n->next;
			coord_text.popnode( n );
			n = tmp_next;
			continue;
		}
		n = n->next;
	}

	if ( tile_text.size() == 0 )
		return;

	vec_c<hash_c<ftext_t*>::hashNode_t*> &table = tile_text.GetTable();
	int i = 0;
	while ( i < tile_text.HASH_SIZE ) {
		if ( table.vec[ i ] ) {
			hash_c<ftext_t*>::hashNode_t *p = table.vec[ i ];
			while ( p ) {
				if ( p->val->tile_uid == uid ) {
					hash_c<ftext_t*>::hashNode_t *tmp = p->next;
					tile_text.Remove( p->unique, p->len );
					p = tmp;
				} else {
					p = p->next;
				}
			}
		}
		++i;
	}
}

// remove ALL matching both clientID and UID
void floatingText_c::ClearUIDAndClientID( int uid, int client_id ) {

	// go through both lists and remove any ftext_t nodes with matching id
	node_c<ftext_t*> *n = coord_text.gethead();
	while ( n ) {
		if ( n->val->tile_uid == uid && n->val->client_id == client_id ) {
			node_c<ftext_t*> *tmp_next = n->next;
			coord_text.popnode( n );
			n = tmp_next;
			continue;
		}
		n = n->next;
	}

	if ( tile_text.size() == 0 )
		return;

	vec_c<hash_c<ftext_t*>::hashNode_t*> &table = tile_text.GetTable();
	int i = 0;
	while ( i < tile_text.HASH_SIZE ) {
		if ( table.vec[ i ] ) {
			hash_c<ftext_t*>::hashNode_t *p = table.vec[ i ];
			while ( p ) {
				if ( p->val->tile_uid == uid && p->val->client_id == client_id ) {
					hash_c<ftext_t*>::hashNode_t *tmp = p->next;

					// manual remove
					if ( p->next )
						p->next->prev = p->prev;
					if ( p->prev )
						p->prev->next = p->next;
					if ( table.vec[ i ] == p ) {
						table.vec[ i ] = p->next ;
					}
					delete p;
					tile_text.decrementSize();
					p = tmp;
				} else {
					p = p->next;
				}
			}
		}
		++i;
	}
}
