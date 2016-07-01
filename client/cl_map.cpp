// cl_map.cpp
//

//#undef __COM_GEOMETRY_H__
//#include "../game/g_entity.h"
//#include "../mapedit/mapedit.h"


#include "../map/m_area.h"


#include "../map/mapdef.h"
#include "cl_local.h"
#include "../server/server.h" // SV_TogglePause

#include "../common/com_geometry.h"


bool cl_paused = false;

// cached data from the currently loaded mapFile
mapFile_t clientMapFile;
filehandle_t music_handle;

void CL_SetLocalPlayer( Entity_t *ent ) {
	Player_t *p = dynamic_cast<Player_t*>( ent );
	if ( !p )
		return;
	player.SetFromEntity( *p );
}

void CL_PopulateMapFile( mapFile_t & M, const char *name ) {
	
	mapFile_t *m = &M;

	// 
	m->reset();

	// name
	strcpy( m->name, name );

	// tiles
	if ( tiles ) {
		node_c<mapTile_t*> *t = tiles->gethead();
		while ( t ) {
			m->mapTiles.add( new mapTile_t( *t->val ) );
			t = t->next;
		}
	}

	// me_ents 
	if ( me_ents ) {
		node_c<Entity_t*> *e = me_ents->gethead();
		while ( e ) {
			// look for player
			//if ( typeid(Player_t*) == typeid(e->val) ) { 
			if ( e->val->entType == ET_PLAYER ) {
				CL_SetLocalPlayer( e->val );
			} else
				m->entities.add ( e->val->Copy() ) ;
			e = e->next;
		}
	}

	// me_areas
	node_c<Area_t*> *a = me_areas.gethead();
	while ( a ) {
		m->areas.add( new Area_t( *a->val, &world.entPool ) );
		a = a->next;
	}

	// me_subareas
	node_c<me_subArea_t*> *s = me_subAreas.gethead();
	while ( s ) {
		m->subAreas.add( new me_subArea_t( *s->val ) );
		s = s->next;
	}
}

/*
====================
====================
*/
void CenterViewportOnPlayer( void ) {
	float v[2];
	v[0] = player.poly.x + 0.5f * player.poly.w;
	v[1] = player.poly.y + 0.5f * player.poly.h;
	v[0] += M_Width() * -0.5f * M_Ratio();
	v[1] += M_Height() * -0.5f * M_Ratio();
	M_GetMainViewport()->SetView( v[0], v[1] );
}

void G_InitPlayer( void );
void G_InitGame( void );

/* 
====================
 CL_LoadMap

	called by the client upon the load of a new map.  Will populate the 
	mapFile structure

	returns: -1 for map not found
====================
*/
int CL_LoadMap( const char *mapname ) {

	// read map in
	int me_ret = ME_ReadMapForClient( mapname );

	// got nothing
	if ( me_ret < 0 ) {
		ME_DestroyMapEditLocal();
		return me_ret;
	}

	// populate world.mapFile from mapedit module
	CL_PopulateMapFile( clientMapFile, mapname );

	// clear the mapeditor module
	ME_DestroyMapEditLocal();

	// build out the engine's internal structure of the world from a mapFile_t 
	world.BuildWorld( clientMapFile );

	// camera, curBlock, curArea, 
	controller.setupGameState();

	G_InitPlayer();
	G_InitGame();

	console.Printf( "loading map \"%s\"", mapname );

	//S_StartBackGroundTrack( "zpak/music/teentown.wav", &music_handle, gtrue );
	//S_StartBackGroundTrack( "temple.ogg", &music_handle, gtrue );

    // we don't hardcode music playing
	//S_StartBackGroundTrack( "zpak/music/nestor1.ogg", &music_handle, gtrue );

	return me_ret;
}



/*
====================
 CL_ReloadMap

	suspect this is an obvious area for expansion here, to add support for
	caching multiple mapFiles at once, an array of mapFile_t perhaps, searchable
	by name
====================
*/
int CL_ReloadMap( void ) {
	console.Printf( "restarting map: \"%s\"", clientMapFile.name );
	world.rebuildPristine( clientMapFile );
	controller.setupGameState();
	G_InitPlayer();
	G_InitGame();
	player.BeginMove(); // get a fresh wish

    // not here anymore
	//S_StartBackGroundTrack( "zpak/music/teentown.wav", &music_handle, gtrue );
	// not working 
	//S_ReStartBackGroundTrack( music_handle, gtrue ); 
	return 1;
}


void CL_Pause( void ) {
	cl_paused = true;
	SV_Pause();
	S_Mute();
}

void CL_UnPause( void ) {
	cl_paused = false;
	SV_UnPause();
	S_UnMute();
}

void CL_TogglePause( void ) {
	if ( cl_paused ) {
		CL_UnPause();
	} else {
		CL_Pause();
	}
}

void CL_PauseWithMusic( void ) {
	cl_paused = true;
	SV_Pause();
}
