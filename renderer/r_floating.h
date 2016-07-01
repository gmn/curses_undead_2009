// r_floating.h
//
#ifndef __R_FLOATING_H__
#define __R_FLOATING_H__

#include "../client/cl_public.h"
#include "../common/com_object.h"
#include "../mapedit/mapedit.h"

#include "../map/m_area.h"
#include "../game/g_entity.h"

/*

two render paths:
	- per mapTile text
	- viewport Coordinate text

floating text can fade or be static.  Each peice of floating text has 
a timer.  if the flag is unset, then the text is static, else it is timed,
and once it's timer has run out, it is removed.  It's rgb intensity is 
scaled linearly by the time it is since it was create over the entire
duration.  

( now - start ) / duration == [ 0, 1.0 ]

*/

//
// floatingText_c
//

#define FLOATING_TEXT_SIZE	256

struct ftext_t : public Allocator_t {
	timer_c timer;
	char text[ FLOATING_TEXT_SIZE ];
	float x, y;
	int tile_uid;
	float scale;
	int font;
	int client_id;
	ftext_t() {
		timer.reset();
		text[0] = 0;
		x = y = 0.f ;
		tile_uid = -1;
		scale = 0.f;
		font = FONT_VERA;
		client_id = 0;
	}
};

class floatingText_c : public Auto_t {

private:

	// class Auto must implement these
	virtual void _my_init( void ) {
		tile_text.init();
		coord_text.init();
	}
	virtual void _my_reset( void ) {
		currentSize = F_POINT_SZ;	// 1.0f scale
		tile_text.reset();
		coord_text.reset();
		currentFont = FONT_VERA;
		trailOff = 3600;
	}
	virtual void _my_destroy( void ) {
		tile_text.destroy();
		coord_text.destroy();
	}

	hash_c<ftext_t*> tile_text;
	list_c<ftext_t*> coord_text;

	// do font size as a state machine.  It just stays the same if you
	//  don't touch it.  but if you need a certain size you have to call it
	//  and set the new size
	int currentSize;
	int currentFont;

	int trailOff;

	enum {
		FLOATING_PERMANENT,
		FLOATING_INTRO,
		FLOATING_TRAILOFF
	};

	static int clientID;

protected:

public:
	
	int requestClientID( void ) { return ++clientID; }

	void setSize( int n ) { currentSize = n; }
	int getSize( void ) const { return currentSize; }

	void AddTileText( mapTile_t *t, const char *str, float, float, int sz =0, int duration =0, int =0 ) ;
	void AddEntText( Entity_t *t, const char *str, float, float, int sz =0, int duration =0, int =0 ) ;
	void AddAreaText( Area_t *t, const char *str, float, float, int sz =0, int duration =0, int =0 ) ;
	void AddCoordText( float x, float y, const char *str, int sz =0, int duration =0, int =0 );
	void DrawTiles( tilelist_t * );
	void DrawEnts( entlist_t * );
	void DrawAreas( list_c<Area_t*> * );
	void DrawCoords( void );
	void Draw( tilelist_t *t ) { DrawTiles( t ); DrawCoords(); }
	void Draw( tilelist_t *t, entlist_t *e ) { DrawTiles( t ); DrawEnts( e ); DrawCoords(); }
	void Draw( tilelist_t *t, entlist_t *e, list_c<Area_t*> *a ) { DrawTiles( t ); DrawEnts( e ); DrawAreas(a); DrawCoords(); }

	// any that has same client id removed
	void ClearClientID( int );
	// first found with uid removed
	void ClearUID( int );
	// first found with both matching clientID and uid removed
	void ClearUIDAndClientID( int, int );

	static void genUniqueString( mapTile_t *tile, const char *str, char *buf, int *len );
};

#endif // __R_FLOATING_H__
