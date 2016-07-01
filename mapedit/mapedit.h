#ifndef __MAPEDIT_H__
#define __MAPEDIT_H__

#define CURSOR_WIDTH 26
#define CURSOR_HALF ( CURSOR_WIDTH / 2 )
#define ME_LOGIC_FPS 45

// per tick, world units
#define VIEWPORT_MOVE		200

#define WORLD_GRID_BOUND	50000000

#include "../common/com_geometry.h" // poly

#include "../common/com_mouseFrameWhatTheFuck.h"

#include "../common/common.h"
#include "../game/g_entity.h"
#include "../common/com_object.h"
//#include "../renderer/r_floating.h"


// visibility sets what you can see.  and what you can interact with is
//  the same as what you can see.
enum me_vis_t {
	ME_VIS_NONE					= 0,
	ME_VIS_COLMODEL				= BIT(0),
	ME_VIS_MATERIAL				= BIT(1),
	ME_VIS_ENTITIES				= BIT(2),
	ME_VIS_FLOATING_TEXT		= BIT(3),
	ME_VIS_ALL					= ~0
};


enum mapeditCmd_t {
	MEC_DESELECT_ALL,
	MEC_NEWPOLY,
	MEC_GRABPOLY,					// mouse click over poly 
	MEC_RELEASEPOLY,
	MEC_DRAGPOLY, 					// click 'n' drag
	MEC_MOUSEOVER_POLY_EDGE,			// change mode to stretch
	MEC_GRAB_POLY_EDGE,
	MEC_STRETCH_POLY_EDGE,
	MEC_CYCLE,
	MEC_ZERO_ACTIVE_ANGLE,
	MEC_DELETE_ACTIVE,
	MEC_COPY,
	MEC_PASTE,
	MEC_TEXTURE,
	MEC_ZOOM_IN,
	MEC_ZOOM_OUT,
	MEC_TOGGLE_GRID,
	MEC_INC_GRIDRES,
	MEC_DEC_GRIDRES,
	MEC_ADVANCE_TEXTURE,
	MEC_PREVIOUS_TEXTURE,
	MEC_RESNAP,
	MEC_CCWISE,
	MEC_CWISE,
	MEC_ADD_REMOVE_COLMODEL,
	MEC_VISIBILITY,
	MEC_TOGGLE_TILE_SNAP,
	MEC_PREV_25_TEXTURE,
	MEC_NEXT_25_TEXTURE,
};

enum meInputMode_t {
	MODE_MENU,
	MODE_POLY_NORMAL,
	MODE_POLY_STRETCH,
	MODE_POLY_DRAG,
	MODE_POLY_DROP,
	MODE_POLY_ROTATE,
	MODE_VIEWPORT_DRAG,
	MODE_DRAG_SELECT,
	MODE_SPECIAL_SELECT,	
};

enum meInput_t {
	ME_IN_NONE					= 0,
	ME_IN_DESELECT_ALL				= 1<<0,
	ME_IN_SELECT_FACE 			= 1<<1,
	ME_IN_SELECT_EDGE			= 1<<2,
	ME_IN_NEW_POLY				= 1<<3,
	ME_IN_TEXTURE_MENU			= 1<<4,
	ME_IN_VIEW_LEFT				= 1<<5,
	ME_IN_VIEW_RIGHT			= 1<<6,
	ME_IN_VIEW_UP				= 1<<7,
	ME_IN_VIEW_DOWN				= 1<<8,
	ME_IN_VIEW_MOVE				= ME_IN_VIEW_UP | ME_IN_VIEW_DOWN | ME_IN_VIEW_LEFT | ME_IN_VIEW_RIGHT,
	ME_IN_CYCLE_POLYS			= 1<<9,
	ME_IN_ZERO_ANGLE			= 1<<10,
	ME_IN_DELETE				= 1<<11,
	ME_IN_COPY					= 1<<12,
	ME_IN_PASTE					= 1<<13,
	ME_IN_TEXTURE				= 1<<14,
	ME_IN_CMDQUE_COMMANDS	= ME_IN_DESELECT_ALL | ME_IN_ZERO_ANGLE | ME_IN_CYCLE_POLYS | ME_IN_NEW_POLY | ME_IN_DELETE | ME_IN_COPY | ME_IN_PASTE | ME_IN_TEXTURE,
	ME_IN_ZOOM_IN				= 1<<15,
	ME_IN_ZOOM_OUT				= 1<<16,
	ME_IN_TOGGLE_GRID			= 1<<17,
	ME_IN_INC_GRIDRES			= 1<<18,
	ME_IN_DEC_GRIDRES			= 1<<19,
	ME_IN_TOGGLE_ROTATE_MODE    = 1<<20,
	ME_IN_SELECT_ALL            = 1<<21,
	
	ME_IN_NEXT_TEXTURE			= 1<<22,
	ME_IN_PREV_TEXTURE			= 1<<23,
	ME_IN_RESNAP				= 1<<24,
	ME_IN_CCWISE				= 1<<25,
	ME_IN_CWISE					= 1<<26,

	ME_IN_ADD_REMOVE_COLMODEL	= 1<<27,

	ME_IN_VISIBILITY			= 1<<28,

	ME_IN_TOGGLE_TILE_SNAP		= 1<<29,
	ME_IN_PREV_25_TEXTURE		= 1<<30,
	ME_IN_NEXT_25_TEXTURE		= 1<<31,

	// FINISHED.  STOP USING THESE NOW
};

enum meStretchMode_t {
	ME_SM_NONE					= 0,
	ME_SM_CORNER				= 1,
	ME_SM_TOP					= 1<<1,
	ME_SM_BOT					= 1<<2,
	ME_SM_LEFT					= 1<<3,
	ME_SM_RIGHT					= 1<<4,
	ME_SM_VERTICAL				= 1<<5,
	ME_SM_HORIZONTAL			= 1<<6,

	ME_SM_BL_CORNER				= ME_SM_BOT | ME_SM_LEFT,
	ME_SM_BR_CORNER				= ME_SM_BOT | ME_SM_RIGHT,
	ME_SM_TL_CORNER				= ME_SM_TOP | ME_SM_LEFT,
	ME_SM_TR_CORNER				= ME_SM_TOP | ME_SM_RIGHT,

/*
	ME_SM_LEFT_INWARD				= 1<<10,
	ME_SM_LEFT_OUTWARD				= 1<<11,
	ME_SM_RIGHT_INWARD				= 1<<12,
	ME_SM_RIGHT_OUTWARD				= 1<<13,
	ME_SM_TOP_INWARD				= 1<<14,
	ME_SM_TOP_OUTWARD				= 1<<15,
	ME_SM_BOT_INWARD				= 1<<16,
	ME_SM_BOT_OUTWARD				= 1<<17,
	
	ME_SM_BL_CORNER				= ME_SM_CORNER | ME_SM_BOT | ME_SM_LEFT,
	ME_SM_BR_CORNER				= ME_SM_CORNER | ME_SM_BOT | ME_SM_RIGHT,
	ME_SM_TL_CORNER				= ME_SM_CORNER | ME_SM_TOP | ME_SM_LEFT,
	ME_SM_TR_CORNER				= ME_SM_CORNER | ME_SM_TOP | ME_SM_RIGHT,

	ME_SM_LEFT_VERTICAL			= ME_SM_VERTICAL | ME_SM_LEFT,
	ME_SM_RIGHT_VERTICAL		= ME_SM_VERTICAL | ME_SM_RIGHT,
	ME_SM_TOP_HORIZONTAL		= ME_SM_HORIZONTAL | ME_SM_TOP,
	ME_SM_BOT_HORIZONTAL		= ME_SM_HORIZONTAL | ME_SM_BOT
*/
};

enum {
	ROT_NONE,
	ROT_ONE,
	ROT_MANY,
};

enum {
	STRETCH_ONE,
	STRETCH_MANY,
};

class me_mouse_t {
private:
    uint magic;
    static const uint mMagic = 0xDEF1CA73;
public:
    int x, y, z;
    byte lmb_down, mmb_down, rmb_down, b4_down;
	byte lmb_click, mmb_click, rmb_click, b4_click;
    void start( void ) {
        x = 640 / 2;
        y = 480 / 2;
		z = 0;
        lmb_down = mmb_down = rmb_down = b4_down = 0;
		lmb_click = mmb_click = rmb_click = b4_click = 0;
        magic = mMagic;
    }
    int istarted( void ) {return magic == mMagic; }
};

//? did this to shut fucking compiler up, but then it complained worse below for list_c<poly_t>
// honestly, I'm at my wits end with this. It shouldn't be so hard. what's worse, the error
// originates when it compiles cl_map, which doesn't even reference mapedit.h, so I can't 
// figure out which file calls what to get to here, because the error would certainly be far easier
// to fix, back up the chain
//struct poly_t;
class mapTile_t;

struct actionState_t {
	struct poly_t *poly; // allow just for now
	int mode;
	actionState_t() : poly(0),mode(0),x0(0.f),y0(0.f),mx0(0.f),my0(0.f),xofst(0),yofst(0) {}
	float x0, y0;
	float mx0, my0;
	float angle;
	node_c<mapTile_t *> *node;
	int xofst, yofst;
	float c[2];
};

/*
struct gridline_t {
	float u[2], v[2];
	unsigned int c;	// color
	float R( void ) const { return (c>>24)/255.0f; }
	float G( void ) const { return (c<<8>>24)/255.0f; }
	float B( void ) const { return (c<<16>>24)/255.0f; }
	float A( void ) const { return (c<<24>>24)/255.0f; }
};
*/

/* moved to com_geom
struct gridline_t {
	float u[2];
	unsigned int c;
	float v[2];
	unsigned int c2;
};
*/


/*
struct drawBuffer_t {
	vec_c<float> vertex;
	vec_c<float> color;
	vec_c<float> texCoord;
}; */



extern me_mouse_t me_mouse;

extern list_c<poly_t> me_polys;
extern list_c<poly_t> deleted_polys;

extern list_c<mapeditCmd_t> cmdque;
extern int stretchModeHighlight;
extern int stretchMode;
//extern poly_t *stretchPoly;

extern actionState_t active;
extern actionState_t highlight;
extern actionState_t stretch;
extern actionState_t dragSelect;

extern gbool drawGrid;

extern int res_x;
extern int res_y;

//extern drawBuffer_t draw;


typedef node_c<mapTile_t*> mapNode_t;
typedef node_c<Entity_t*>  entNode_t;

/*
====================
 tilelist_t
====================
*/
// helper class as wrapper for mapTile_t* list
struct tilelist_t : public list_c<mapTile_t *>, public Allocator_t {

	// shared pool for all tiles
	memPool<node_c<mapTile_t *> > pool;

	// place to hold onto deleted tiles
	list_c<mapTile_t *> deleted;

	void init( void ) {
		pool.start();
		deleted.init( &pool );
		list_c<mapTile_t *>::init( &pool );
		nextDeleted = 0;
		totalDeleted = 0;
		needsort = 0;
	}

	void reset( void ) {
		deleted.reset();
		list_c<mapTile_t *>::reset();
		pool.reset();
		nextDeleted = 0;
		totalDeleted = 0;
		needsort = 0;
	}
	
	// override base method
	void add( mapTile_t *tile ) {
		needsort = 1;
		list_c<mapTile_t*>::add( tile );
	}

	static mapTile_t * newMaterialTile( void );
	static mapTile_t * newMaterialTileXY( float,float,float,float,float,char* );
	void returnTile( mapTile_t * );

	static const int DELETED_LENGTH = 100; // 100 undos
	int deletedAmounts[DELETED_LENGTH];
	int nextDeleted;
	int totalDeleted;

	void storeDeletedCount( int count ) {
		deletedAmounts[ nextDeleted ] = count;
		nextDeleted = (++nextDeleted) % DELETED_LENGTH;
		++totalDeleted;
	}

	void Destroy( void );
	void copyToList( tilelist_t * );

	void syncColModels( void ) ;
	void syncColModelsNoBlock( void ) ;

	int needsort;
	void crapsort( void ) ;
	void sort( void ) { crapsort(); }

	mapNode_t * tileByID( unsigned int _uid, mapNode_t *start =NULL );
};




/*
====================
 activeMapTile_t

 for storing pointers to active mapTile_t nodes. class houses the selected set
====================
*/
class activeMapTile_t : public list_c<mapNode_t *> {

private:
	int client_id;

public:

	// shared pool for all tiles
	memPool<node_c<mapNode_t *> > pool;

	void init( void ) {
		pool.start();
		list_c<mapNode_t *>::init( &pool );
		client_id = 0;
	}

	void reset( void ) {
		list_c<mapNode_t *>::reset();
		pool.reset();
	}

	// have to figure out a way to start using these
	activeMapTile_t() {
//		init();
	}

	// are anyActive
	bool anyActive( void ) { return this->size() > 0; }

	node_c<mapNode_t*> * checkNode( mapNode_t *node );
	node_c<mapNode_t*> * findNode( mapNode_t *node ) { return checkNode( node ); }

	void cloneToSet( activeMapTile_t * );
	void cloneToTileList( tilelist_t * );

	void setTexture( material_t *, unsigned int, unsigned int ) ;

	void setJustOne( mapNode_t *node ) {
		reset();
		add ( node ) ;
	}

	void addIfNot( mapNode_t *node ) {
		if ( !checkNode( node ) ) 
			add( node );
	}

	void deltaMoveXY ( float, float ) ;

	void selectAll( tilelist_t * );

	bool MustSnap( void );
	void snapOffAll( void );

	void syncColModels( void ) ;
	void syncColModelsNoBlock( void ) ;
	void dragColModels( float, float ) ;

	bool hasLock( void );

	// override base add
	void add( mapNode_t *node ) { 
		if ( !node->val->background ) {
			list_c<mapNode_t*>::add( node );
		}
	}
	// even though we overrode the default ::add, keep a secret one for special uses anyway
	void add_noblock( mapNode_t *node ) {
		list_c<mapNode_t*>::add( node ) ;
	}
};


void ME_InitPoly( poly_t * );
/*
====================
 class entlist_t
====================
*/
class entlist_t : public list_c<Entity_t*>, public Allocator_t {
public:
	void destroy( void ) {
		if ( this->size() != 0 ) {
			node_c<Entity_t *> *e = gethead();
			while ( e ) {
				V_Free( e->val );
				e = e->next;
			}
		}
		list_c<Entity_t*>::destroy();
	}

	// when we add a new entity, it gets auto coordinates here to place it 
	//  somewhere on the screen
	void addNew( Entity_t *ent ) {
		ME_InitPoly( &ent->poly );
		ent->populateGeometry();
		list_c<Entity_t*>::add ( ent );
	}

	node_c<Entity_t*> * findNode( Entity_t *e ) {
		entNode_t * n = head;
		while ( n ) {
			if ( n->val == e ) {
				return n;
			}
			n = n->next;	
		}
		return NULL;
	}
};


enum {
	P_MAP,
	P_PALETTE
};

class palette_t : public Allocator_t {
private:
	static int nextUID;
public:
	tilelist_t *	tilelist;
	entlist_t *		entlist;	

	bool			global_snap;
	int 			gridres;
	gbool			drawgrid;
	float 			zoom;
	float			x, y;
	me_vis_t 		vis;
	char 			name[ 256 ];
	
	void reset( void ) {
		tilelist = 0;
		entlist = 0;

		global_snap = true;
		drawgrid = gtrue;
		gridres = 0;

		zoom = 11110.0f;
		x = -512.0f;
		y = -512.0f;
		vis = ME_VIS_ALL;

		name[ 0 ] = '\0';
	}

	palette_t() { reset(); }
	palette_t( tilelist_t *_tl ) { reset(); tilelist = _tl; }
	palette_t( tilelist_t *_tl, entlist_t *_el ) {
		reset(); tilelist = _tl; entlist = _el;
	}

	palette_t * Copy( void );

	~palette_t() {
		Destroy();
	}
	void Destroy( void ) {
		if ( tilelist ) {
			tilelist->Destroy();
// calls to delete must match calls to new.  my older classes didn't use new
//			delete tilelist;
			tilelist = NULL;
		}
		if ( entlist ) {
			entlist->destroy();
			entlist = NULL;
		}
	}
};

/*
*/
class paletteManager_c : public Auto_t {
private:

	// class Auto must implement these
	virtual void _my_init( void ) {
		palettes.init();
		SaveState(); // gets the current map
	}
	virtual void _my_reset( void ) {
		current = 0;
		last = 0;
		palettes.reset();
		total_palettes = 0;
		total_maps = 0;
	}
	virtual void _my_destroy( void ); 

public:
	node_c<palette_t*> *current, *last; 
	list_c<palette_t *> 	palettes;

	int total_palettes;
	int total_maps;

	int Next( void );
	int Prev( void );
	int ChangeTo( int );
	void NewPalette( const char * ); // called by the 'palette' cmd
	void SetCurrent( node_c<palette_t*> * );
	void SaveState( void ) ; // gets gridLevel, zoom, x, y, visLevel
	void LoadState( void );
	int DeleteCurrent( void );

	/*
	paletteManager_c() { 
		init();
	} 
	virtual ~paletteManager_c() {
		destroy();
	} */

	void Info( void );
	void ChangeCurrentName( const char * );
};




class entPointerList_t : public list_c<Entity_t*>, public Allocator_t {
public:
	void transferFromList( entPointerList_t & );
};

class entNodeList_t : public list_c<node_c<Entity_t*>*> , public Allocator_t {
public:

	node_c<entNode_t *> * checkNode( entNode_t * );
	void drag( float , float );
	void setTexture( material_t *, unsigned int, unsigned int );
	int client_id;
	entNodeList_t() : client_id(0) {}
	void cloneToList( entPointerList_t * );
};


extern entNodeList_t entSelected;
extern entPointerList_t entCopy;
extern entPointerList_t entTmp;

extern entlist_t * me_ents;

extern paletteManager_c palette;


// second rev, material based
extern tilelist_t *tiles;
extern activeMapTile_t selected;
 
extern tilelist_t copied;
extern tilelist_t copyTmp;

extern class menu_t menu;


#define __me_def_poly_t { { 200.0f, 280.0f, 0.0f }, { 280.0f, 280.0f, 0.0f }, { 280.0f, 360.0f, 0.0f }, { 200.0f, 360.0f, 0.0f }, 80.0f, 80.0f }

extern gvar_c *me_activeMap;
extern gvar_c *me_newTileSize;

extern int current_texture;

extern actionState_t rotate;
extern int rotateMode;
extern int rotationType;

extern meInputMode_t inputMode;
extern int subLineResolution;

extern me_vis_t visibility; 
extern bool snapToGrid;

extern class floatingText_c floating;

extern list_c<class Area_t*> me_areas;
extern list_c<class me_subArea_t*> me_subAreas;
extern material_t *ent_notex;




// me_main, me_input && me_save
void ME_DrawFrame( void );
void ME_MakeCommand( void );
void ME_ExecuteCommands( void );
void ME_KeyEvent( int , int, unsigned int, int );
void ME_MouseFrame( mouseFrame_t * );
int ME_Tics ( void );
int ME_ConPrintUIDs( void );
int ME_ConPrintLayers( void );
int ME_ConPrintLocks( void );
void ME_ConPrintTileInfo( void );
void ME_DrawConsole( void );

void ME_IncSubline( void );
void ME_DecSubline( void );
void ME_SetSubline( int );

void ME_SetLockAll( int );
void ME_CreateColorMaterial( float, float, float, float );
void ME_ConPrintMaterialInfo( int );
void ME_SetBackgrounds( void );
void ME_UnsetAllBackgrounds( void );
void ME_Init( void );
void ME_InitGvars( void );
bool ME_CreateColorMaskMaterial( float, float, float, float );
bool ME_DupMaterial( void );
bool ME_CreateMaskedTexture( void );
bool ME_SetMaterialColor( float, float ,float ,float  );
float * ME_GetCentroid( void );
void ME_TrashLocalData( void );
void ME_DestroyMapEditLocal( void );
void ME_CombineColModels( void );
void ME_BackgroundInfo( void );
void ME_UnsetBackground( const char * );

// interface funcs to tie in to menus
void newPoly_f ( void );
void zeroAngle_f( void ) ;
void deletePoly_f( void );
void copy_f ( void );
void paste_f ( void );
void zoomIn_f ( void ) ;
void zoomOut_f ( void );
void toggleGrid_f ( void );
void quit_f( void );
void nextTexture_f ( void );
void prevTexture_f ( void );
void next25Texture_f ( void );
void prev25Texture_f ( void );
void deselectAll_f ( void );
void selectAll_f ( void ) ;
void grid_1 ( void );
void grid_2 ( void );
void grid_3 ( void );
void grid_4 ( void );
void grid_5 ( void );
void grid_6 ( void );
void grid_7 ( void );
void incGrid_f ( void );
void decGrid_f ( void );
void save_f( void );
void load_f ( void );
void newFile_f ( void );
void resnap_f ( void );
void rot90_f( void ) ;
void rot90cc_f ( void );
void toggleCon_f ( void );
void toggleColModel_f( void );
void changeVis_f( void );
void toggleSnap_f( void );
void lock_f( void ) ;
void incLayer_f( void );
void decLayer_f( void );
void toggleRotate_f( void );
void cycleSelected_f( void );
void nextPalette_f( void );
void prevPalette_f( void );
void grid_0( void );
void grid_m1( void );
void grid_m2( void );

// me_save.cpp
void ME_SaveMap( const char * );
int  ME_LoadMap( const char * );
void ME_SpawnSaveDialog( void );
void ME_SpawnLoadDialog( void );
bool ME_SaveMapCheckExtension( const char * );
bool ME_LoadMapCheckExtension( const char * );
int  ME_SpawnDeletePaletteDialog( void );
int  ME_ReadMapForClient( const char * );

// me_area.cpp
int ME_CreateAreaFromSelection( const char * );
void ME_PrintAreaInfo( void );
int ME_CreateSubAreaFromSelection( void );
int ME_SetName( const char * );
void ME_Area_Command( const char *, const char *, const char *, const char * );

#endif // __MAPEDIT_H__
