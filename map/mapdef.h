/*
 *
 *  mapdef.h            -- defines everything about the map
 *
 */

#ifndef __MAPDEF_H__
#define __MAPDEF_H__

/*

brainstorm

256 sub-divisions per meter.  (The smallest amount that anything can move).
1 meter scale will be , you know, about the same size is one square of 
side-walk.  that sort of thing.

so if you are walking, you are covering about 1 meter per second, which is
256 small units.  running, double that amount, so you cover 2 meters per 
second running.  

walking: thats 256 units per second, there are 60 gametics per second?
which means that you can move 256 / 60 = 4.26666 per gametic.  of course
this fraction fucking blows, unless you just keep track of the entire thing
in floats.  sure , why not.

what about doors, drawers?  well a typical drawer would be about 1/3rd meter,
and move about the same rate as walking or a little slower.  sooo.. 

so, whats the slowest that anything will need to move, as a direct rate of
velocity?  something could move at a fractional amount of units per tic, but
we want to avoid that, in fact, we need to restrict that from happening in
order to ensure a certain safe area for a map to exist on.

do I want 1024 per meter?  1024 / sec.  yes, 1024 per meter.  now there is
no question that I have a full selection of whole number per tic move amounts. 
to make it even easier I should make the tic 64.  

2 ^ 6 / 2 ^ 10 = 2 ^ (10-6) = 2^4 = 16;

so thats 16 units per tic , with a high gametic of 64.  even halflife2 and 
those, go for more like 60.  

I keep thinking of the quake3 code that contructs a client movement where it
is 128, but if running it is 256.  so, perhaps I want to cut it up even more.

that would make it 4096 per meter.  make the gametic 60 / sec.   soo, walking
1 m/s would be 4096 / 60 = 68 units per tic.  this is pretty refined.  in
fact it is very refined.  think about it, you'd be walking 4096 every second.

yeah, back to 1024.  I like the fact that its 16 per 60th of a second.  that just kinda sounds nice.  it means that there is still a great deal of control over minutia there in case we ever need it, which we wont.  

ok, and its easy to remember 1024 per meter.  and it means that we our mapsize
limit is 4000 meters.  and I'm just going to figure out how to move coordinate
systems anyway.

*/


#define UNITS_PER_METER 		1024

/*

our map-squares that keep pointers to lists of entities for collision 
computation culling, well, we can just make each collission_square 3x3 meters.

*/


/*

the next thing we need is a conversion factor between screen coordinates, 
window size, and map coordinates.

*/


// at zoom = 1.0, this is what is seen in a 4/3 resolution
// (ie, the base viewport displays 6 meters across
#define WINDOW_METER_WIDTH 		6

// most gameboard polys created on DEFAULT_LAYER
#define DEFAULT_LAYER			1

// the size of a grid square.  after the map is created, it is carved up into
// an array of grid_squares, which are large squares that store pointers to 
// resources and entities.  this way, you only need to consider the grid 
// square and the grid squares around you for player interactions and collision
// detection
#define GRID_SQUARE_SIDE 		(WINDOW_METER_WIDTH*UNITS_PER_METER)


// resolution
class screen_resolution_t {
public:
	char *name;
	int width, height;
	float aspect_ratio; 		// w/h , eg. 4/3 = 1.3333 
};


// view 
class viewport_t {
public:

	// controls perspective zoom & ratio
	float zoom; 			// 1.0 starting

	float aspect_ratio;		// meter_width / meter_height

	// how many world meters are displayed in the view window @ zoom = 1.0
	float meter_width;  	
	float meter_height;

	// the ratio is the conversion factor between world units and screen units.
	// unit_width / ratio == screen_width in pixels
	// unit_height / ratio == screen_height in pixels
	// and vice-versa
	float ratio;			// meter_width / aspect_ratio

	// to remove the division step for faster conversion
	// world.x * iratio = screen.x, etc...
	float iratio;			// 1 / ratio

	float original_ratio;	// ratio.  this never changes, for resetting view zoom

	float units_per_meter;	// loaded from UNITS_PER_METER
	//float units_per_pixel; 	// unit_per_meter / ratio
	float pixels_per_meter; 	// unit_per_meter / ratio


	// unit measurements of viewport
	float unit_width;		// meter_width * units_per_meter
	float unit_height; 		// unit_width / aspect_ratio

	struct {
		float x, y;
	} screen;	// location of bottom-left corner of the viewport on the screen

	void init( void );			// setup internals
};

// some extra stuff for the main viewport
class main_viewport_t : public viewport_t {
public:

	void init( void );

	struct {
		float x, y;
	} world;	// location of bottom-left corner of the viewport in the world

	screen_resolution_t *res;	// current resolution
	screen_resolution_t *resolutions;	//  resolutions

	int resnum;
	int total_res;

	double near_plane, far_plane;

	void SetView( float x, float y, float w, float h, float z ) {
		world.x = x;
		world.y = y;
//		world.w = w;
//		world.h = h;
		ratio = z;
		iratio = 1.0f / ratio;
	}
	void SetView( float x, float y, float z ) {
		world.x = x;
		world.y = y;
		ratio = z;
		iratio = 1.0f / ratio;
	}
	void SetView( float x, float y ) {
		world.x = x;
		world.y = y;
	}
};

/*

whats the relation to the view_window and the screen_resolution?  what happens
if you can zoom in and zoom out?  well, first of all, obviously screen_res
never changes, but view_window does change.  view window would be, 

well, you could just, since we're doing a 2D game, keep the frustum Ortho,
which means, that elevation doesn't change perspective.  near and far
planes are important.   but near, far, elevation never change.  I think the
conversion factor is given by the ratio between screen_resolution.width &
view_window.width.  

I'm just about going to figure this out, then its all done.

*/

/* 

height will be the subbordinate value.  meaning, width and ratio are the first
values evaluated.  and that we can accomodate any resolution if we follow this
rule. 

- width first, then ratio to get height.  
w = 1280; w/r = h ==> 1024

- this gives us meter width and height, 
mw = 6
mw/r = 4.8

- use meters to get UNITS
uw = mw * 1024 = 6144
uw/r = 4915.200195

- window coordinates are 
w = 1280; h = 1024;

- unit to screen ratio is:
6144 / 1280 = 4.8
4915.200195 / 1024 = 4.8

sooooo, (meter width/ratio) also gives us UNITS per PIXEL.  this is crucial.  
because
everything I draw is coming from coordinates in world space, but being
translated by coordinates to view space.

zoom directly effects units per pixel.  

*/

/*

to get oriented with the feel of world space in screen space, I should write
a routine to print a grid and label it.  (I need more fonts)

sooo, if we have a 1280 screen which displays 6 meters in width, and we 
know that ratio is 4.8, then how many pixels wide is 1 meter.  
1 meter = 1024 / 4.8 = 213.3333  and 213 * 6 = 1278.  
ok, this is starting to work.

*/

/*

next thing we have to figure out is reference points.  viewport offset.

there is a map zero-zero point.  and we can move left and right, which is
basically just moving the viewport offset.  (I'm still thinking in mapedit)

also, you can more than one viewport, and the viewport can be odd sizess
that are not screen coords.  thats the main reason for abstracting it.

for instance, you may want to over letterbox it , or like I said, have
more than one, or look through a keyhole or something.  sooo,

we keep a main resolution.  which gives us the screen coord to map everything
to at the end of the pipe.

we have a viewport_t that we are rendering to currently, or are in the
context of rendering to.  so all coordinates get multiplied through the 
ratio and zoom of the current viewport

*/

/*

how does zoom work?  well, let zoom multiply directly into meters.  therefor,

zoom < 1 zooms IN, 
zoom > 1 zooms OUT.  
zoom can never be <= 0,

assert ( zoom > 0.f );

*/

// the main game viewport
extern main_viewport_t main_viewport;

#define ZOOM_LIMIT_IN 			0.2f
#define ZOOM_LIMIT_OUT			100.0f

#define NEAR_LAYER_MAX			-100.0
#define FAR_LAYER_MAX			100.0

// screen resolution defaults to 800x600 without any testing or pre-calculation
#define DEFAULT_RESNUM 			2

class zombie_map_t {

};

// mapdef.cpp
void M_Init( void );
main_viewport_t * M_GetMainViewport( void );
//void M_WorldToScreen( float * );
//void M_ScreenToWorld( float * );

//int M_Width( void );
//int M_Height( void );

inline float M_Ratio( void ) { return main_viewport.ratio; }
inline void M_SetRatio( float f ) { main_viewport.ratio = f; main_viewport.iratio = 1.0f / main_viewport.ratio; }

inline void M_WorldToScreen( float *v ) {
	// subtract out vector of viewport corner to origin and scale it
	v[0] = ( v[0] - main_viewport.world.x ) * main_viewport.iratio;
	v[1] = ( v[1] - main_viewport.world.y ) * main_viewport.iratio;
}

inline void M_ScreenToWorld( float *v ) {
	v[0] = v[0] * main_viewport.ratio + main_viewport.world.x;
	v[1] = v[1] * main_viewport.ratio + main_viewport.world.y;
}

inline int M_Height( void ) {
	return main_viewport.resolutions[ main_viewport.resnum ].height;
}

inline int M_Width( void ) {
	return main_viewport.resolutions[ main_viewport.resnum ].width;
}

void M_ConSetViewport( const char *, const char *, const char * );

inline void M_MoveWorld( float x, float y ) {
    main_viewport.world.x += x;
    main_viewport.world.y += y;
}

inline void M_AdjustUnit( float fW, float fH ) {
    main_viewport.unit_width += fW;
    main_viewport.unit_height += fH;
}

/*
====================
 map_t
====================
* /
class map_t {
public:

}; */

extern main_viewport_t main_viewport;

#endif // __MAPDEF_H__
