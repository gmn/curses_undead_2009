
#include "mapdef.h"
#include "../common/common.h"
#include "../common/com_assert.h"
#include "../client/cl_console.h"

screen_resolution_t resolutions_set[] = {
{ "640 x 480", 		640, 480, 		640.f / 480.f },
{ "800 x 600", 		800, 600, 		800.f / 600.f },
{ "1024 x 768", 	1024, 768, 		1024.f / 768.f },
{ "1280 x 1024", 	1280, 1024, 	1280.f / 1024.f },
{ "1440 x 900", 	1440, 900, 		1440.f / 900.f },
{ "1600 x 1200", 	1600, 1200, 	1600.f / 1200.f },
//{ "1920 x 
// FIXME: others...
};


#define VID_RESOLUTIONS (sizeof(resolutions_set)/sizeof(resolutions_set[0]))

// the main game viewport
main_viewport_t main_viewport;

main_viewport_t * M_GetMainViewport( void ) {
	return &main_viewport;
}

void viewport_t::init( void ) {
	zoom = 1.0f;
	aspect_ratio = 4.0f/3.0f;

	meter_width = WINDOW_METER_WIDTH;
	meter_height = meter_width / aspect_ratio;

	ratio = meter_width / aspect_ratio;
	units_per_meter = UNITS_PER_METER;
	pixels_per_meter = units_per_meter / ratio;

	unit_width = meter_width * units_per_meter;
	unit_height = unit_width / aspect_ratio;

	screen.x = 0.f;
	screen.y = 0.f;
};

// init for the main viewport
void main_viewport_t::init( void ) {
	zoom = 1.0f;

	// just lazy defaulting to 800x600 right now
	//resnum = DEFAULT_RESNUM;;
	resnum = com_resolution->integer();

	Assert( resnum >= 0 && resnum < VID_RESOLUTIONS );

	resolutions = resolutions_set;
	res = &resolutions[ resnum ];
	total_res = VID_RESOLUTIONS;

	aspect_ratio = res->aspect_ratio;

	meter_width = WINDOW_METER_WIDTH;
	meter_height = meter_width / aspect_ratio;

	units_per_meter = UNITS_PER_METER;

	unit_width = meter_width * units_per_meter;
	unit_height = unit_width / aspect_ratio;

	// ratios from screen coordinates to world coordinates and vice-versa, respectively
	ratio = unit_width / res->width;
	iratio = 1.0f / ratio;
	original_ratio = ratio;	

	pixels_per_meter = units_per_meter / ratio;


	world.x = unit_width / -2.0f;
	world.y = unit_height / -2.0f;

	screen.x = 0.f;
	screen.y = 0.f;

	near_plane = NEAR_LAYER_MAX;
	far_plane = FAR_LAYER_MAX;
}

void M_Init( void ) {

	// initialize the main viewport
	main_viewport.init();

	// now that we have viewport, construct comment for console variable
	char buf[8000];
	memset( buf, 0, 8000 );
	char *p = buf;
	main_viewport_t * v = M_GetMainViewport();
	for ( int i = 0; i < v->total_res; i++ ) {
		sprintf( p, "%d: %s", i, v->resolutions[ i ].name );
		p = buf + strlen( buf );
		if ( i < v->total_res - 1 )
			*p++ = ',';
		*p++ = ' ';
	}
	*p-- = 0;
	*p-- = 0;
	buf[7999] = 0;

	com_resolution->setComment( buf );
}

void M_ConSetViewport( const char *x, const char *y, const char *z ) {
	if ( !x )
		return;
	main_viewport.world.x = atof( x );
	if ( y ) 
		main_viewport.world.y = atof( y );
	if ( z ) {
		main_viewport.ratio = atof( z );
		main_viewport.iratio = 1.0f / main_viewport.ratio;
	}
	console.Printf( "Viewport Set to: (%.2f, %.2f) x %.2f", main_viewport.world.x, main_viewport.world.y, main_viewport.ratio );
}

/* COORDINATE SYSTEM CONVERSION EQUATION

SCREEN = ( POINT - WORLD ) * SCALE ;

POINT = SCREEN / SCALE + WORLD ;

where WORLD is the vector offset of the bottom left of the viewport to the origin in world
coordinates.

*/

/* Moved to the header and inlined
void M_WorldToScreen( float *v ) {
	// subtract out vector of viewport corner to origin and scale it
	v[0] = ( v[0] - main_viewport.world.x ) * main_viewport.iratio;
	v[1] = ( v[1] - main_viewport.world.y ) * main_viewport.iratio;
}

void M_ScreenToWorld( float *v ) {
	v[0] = v[0] * main_viewport.ratio + main_viewport.world.x;
	v[1] = v[1] * main_viewport.ratio + main_viewport.world.y;
}

int M_Height( void ) {
	return main_viewport.resolutions[ main_viewport.resnum ].height;
}

int M_Width( void ) {
	return main_viewport.resolutions[ main_viewport.resnum ].width;
}
*/

