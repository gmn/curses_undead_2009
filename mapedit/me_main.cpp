
#include <glew.h>

#include "../common/common.h"
#include "../win32/win_public.h" // Get_MouseFrame
#include "../renderer/r_ggl.h"
#include <gl/gl.h> // for the constants and flags and extra stuff
#include <string.h> // memcpy
#include "../client/cl_public.h"
#include "../common/com_geometry.h"
#include "mapedit.h"
#include "../map/mapdef.h"
#include <math.h>
#include "../lib/lib.h"

#include "me_menus.h"

#include "../client/cl_console.h"
#include "../renderer/r_floating.h"


#define GL_DRAW_QUAD_8V( v ) { \
        gglBegin( GL_QUADS ); \
        gglVertex3f( (v)[0], (v)[1], 0.0f ); \
        gglVertex3f( (v)[2], (v)[3], 0.0f ); \
        gglVertex3f( (v)[4], (v)[5], 0.0f ); \
        gglVertex3f( (v)[6], (v)[7], 0.0f ); \
		gglEnd(); }

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

#define GL_DRAW_OUTLINE_8V( v ) { \
	    gglBegin( GL_LINE_STRIP ); \
        gglVertex3f( v[0], v[1], 0.0f ); \
        gglVertex3f( v[2], v[3], 0.0f ); \
        gglVertex3f( v[4], v[5], 0.0f ); \
        gglVertex3f( v[6], v[7], 0.0f ); \
        gglVertex3f( v[0], v[1], 0.0f ); \
		gglEnd(); }

#define GL_LINE_SQUARE( a ) { \
		gglBegin( GL_LINE_STRIP ); \
		gglVertex3f( -(a), -(a), 0.f ); \
		gglVertex3f(  (a), -(a), 0.f ); \
		gglVertex3f(  (a),  (a), 0.f ); \
		gglVertex3f( -(a),  (a), 0.f ); \
		gglVertex3f( -(a), -(a), 0.f ); \
		gglEnd(); }

#define GL_SQUARE( a ) { \
		gglBegin( GL_QUADS ); \
		gglVertex3f( -(a), -(a), 0.f ); \
		gglVertex3f(  (a), -(a), 0.f ); \
		gglVertex3f(  (a),  (a), 0.f ); \
		gglVertex3f( -(a),  (a), 0.f ); \
		gglEnd(); }


// 
paletteManager_c palette;

// second rev, material based
tilelist_t * tiles = NULL;


gvar_c *me_activeMap = NULL;
gvar_c *me_newTileSize = NULL;
gvar_c *me_selectMargin = NULL;
gvar_c *me_gridColor = NULL;
gvar_c *me_gridColor2 = NULL;
gvar_c *me_bgColor = NULL;


static unsigned int grid_color = 0x99CCCCCC;
static unsigned int grid_color2 = 0x99999999;


int me_start = 0;
int me_lastframe;
me_mouse_t me_mouse;
image_t *cursor_img = NULL;
image_t *mainchar = NULL;

/*  subline designation
	
	_name_				
	grid-2:		2		
	grid-1:		4			
	grid0:		8			
	grid1:		16			
	grid2:		32			
	grid3:		64			
	grid4:		128			
	grid5:		256			
	grid6:		512			
	grid7:		1024		<- max	
*/
int subLineResolution = 4; // default 
// computation :: 1024 / 2^(7-res) ==> 2^3+2^res ==> 2^(3+res) ==> 1 << ( 3 + res )
const int subMax = 7;
const int subMin = -2;

void CL_DrawStretchPic( float, float, float, float, 
                        float, float, float, float, image_t * );

material_t * ent_notex = NULL;
floatingText_c floating;


/// lines
buffer_c<line_t> gridlines;

/// vertex array list 
buffer_c<quad_t> quadlist;

/// Entities, all kinds
entlist_t * me_ents = NULL;



/*
============================================
	MENUS
============================================
*/
menu_t menu;

// menuColumn_t and objects that extend from it must be allocated as static 
//  global so that the default constructor can run and build the vtable
menuColumn_t    file_col;
menuColumn_t	edit_col;
menuColumn_t    grid_col;
menuColumn_t    sel_col;
menuColumn_t    cmd_col;
textArea_t      about_txt; 

menuBar_t       bar;
menuColumn_t	recent_col;

/*
====================
 ME_CreateMenus
====================
*/
void ME_CreateMenus( void ) {

    // create a box and set it as the column's trigger
    menuBox_t *file_box = menuBox_t::newBox( "File", NULL );
	menuBox_t *sel_box = menuBox_t::newBox( "Selection", NULL );
    menuBox_t *grid_box = menuBox_t::newBox( "Grid", NULL );
    menuBox_t *cmd_box = menuBox_t::newBox( "Commands", NULL );
    menuBox_t *about_box = menuBox_t::newBox( "About", NULL );

    // create a bar and add the trigger boxes to it
    bar.setLocation( 0.f, 0.f );
    bar.setPadding( 14.f, 5.f );
    bar.addBox( file_box );
    bar.addBox( sel_box );
    bar.addBox( grid_box );
    bar.addBox( cmd_box );
    bar.addBox( about_box );

    // configure Commands column
	cmd_col.reset();
    cmd_col.addTextBox( "New MapTile (Spacebar)", newPoly_f );
    cmd_col.addTextBox( "Toggle Rotate mode (R)", toggleRotate_f );
    cmd_col.addTextBox( "Cycle through selected Tiles (M)", cycleSelected_f );
    cmd_col.addTextBox( "Zero Angle (B)", zeroAngle_f );
	cmd_col.addTextBox( "Next Texture (T)", nextTexture_f );
	cmd_col.addTextBox( "Prev Texture (Shift+T)", prevTexture_f );
	cmd_col.addTextBox( "Next 25 Textures (G)", next25Texture_f );
	cmd_col.addTextBox( "Prev 25 Textures (Shift+G)", prev25Texture_f );
    cmd_col.addTextBox( "Zoom In (Z)", zoomIn_f );
    cmd_col.addTextBox( "Zoom Out (X)", zoomOut_f );
	cmd_col.addTextBox( "Toggle Grid (Y)", toggleGrid_f );
	cmd_col.addTextBox( "Re-Snap to Grid (S)", resnap_f );
	cmd_col.addTextBox( "Toggle-Snap (Shift+S)", toggleSnap_f );
	cmd_col.addTextBox( "rotate 90 degrees cwise ([)", rot90_f );
	cmd_col.addTextBox( "rotate 90 degrees ccwise (])", rot90cc_f );
	cmd_col.addTextBox( "add/remove Collision Model (C)", toggleColModel_f );
	cmd_col.addTextBox( "change visibility mode (V)", changeVis_f );
	cmd_col.addTextBox( "Lock/Unlock Selected Tile (Ctrl+L)", lock_f );
	cmd_col.addTextBox( "Increment Layer Number (L)", incLayer_f );
	cmd_col.addTextBox( "Decrement Layer Number (Shift+L)", decLayer_f );
	cmd_col.addTextBox( "Change to Next Palette (P)", nextPalette_f );
	cmd_col.addTextBox( "Change to Prev Palette (Shift+P)", prevPalette_f );
	cmd_col.addTextBox( "Select Any Palette (F1-F12)" );

    cmd_col.setPadding( 10.f, 4.f ); 
    cmd_col.connectTriggerBox( cmd_box );

    // configure File column and attach trigger box
	file_col.reset();
	file_col.addTextBox( "Show Console (~)", toggleCon_f );
	file_col.addTextBox( "New Map", newFile_f );
	file_col.addTextBox( "Load Map", load_f );
	file_col.addTextBox( "Save As (Ctrl+S)", save_f );
//	menuBox_t *recent_box = menuBox_t::newBox( "Recent" );
//	file_col.addBox ( recent_box );
	file_col.addTextBox( "Quit (Ctrl+Q)", quit_f );
	file_col.setPadding( 10.f, 4.f );
	file_col.connectTriggerBox( file_box );

/* SAVE
	// small recent exposed box in File column
	recent_col.reset();
	recent_col.setPadding( 10.f, 4.f );
	recent_col.addTextBox( "test1" ); // some empty text to give the box form
	recent_col.connectTriggerBox( recent_box );
*/

	// Grid column
	grid_col.reset();
	grid_col.setPadding( 10.f, 4.f );
    grid_col.addTextBox( "Increase Grid Resolution (+)", incGrid_f );
    grid_col.addTextBox( "Decrease Grid Resolution (-)", decGrid_f );
	grid_col.addTextBox( " Grid-2 -   2",		grid_m2 );
	grid_col.addTextBox( " Grid-1 -   4",		grid_m1 );
	grid_col.addTextBox( " Grid0 -    8",		grid_0 );
	grid_col.addTextBox( " Grid1 -   16 (1)", grid_1 );
	grid_col.addTextBox( " Grid2 -   32 (2)", grid_2 );
	grid_col.addTextBox( " Grid3 -   64 (3)", grid_3 );
	grid_col.addTextBox( ">Grid4 -  128 (4)", grid_4 );
	grid_col.addTextBox( " Grid5 -  256 (5)", grid_5 );
	grid_col.addTextBox( " Grid6 -  512 (6)", grid_6 );
	grid_col.addTextBox( " Grid7 - 1024 (7)", grid_7 );
	grid_col.connectTriggerBox( grid_box );

	// Selection Menu
	sel_col.reset();
	sel_col.setPadding( 10.f, 4.f );
	sel_col.addTextBox( "Deselect All (Escape)", deselectAll_f );
	sel_col.addTextBox( "Select All (Ctrl+A)", selectAll_f );
    sel_col.addTextBox( "Copy (Ctrl+C)", copy_f );
    sel_col.addTextBox( "Paste (Ctrl+V)", paste_f );
    sel_col.addTextBox( "Delete Selected Tile(s) (Backspace)", deletePoly_f );
	sel_col.connectTriggerBox( sel_box );

    // configure About column and attach trigger box
	about_txt.reset();
	about_txt.setPadding( 10.f, 10.f );
	about_txt.setText( "No-Frills Drop Menu System\nCopyright (c) 2008 Greg Naughton\ngregnaughton@yahoo.com" );
	about_txt.connectTriggerBox( about_box );

    // create the master menu object and add the bar to it
//	menu.setDimensions( 8.f, 10.f, 0.f, 2.f );
	menu.setDimensions( 12.f, 12.f, -2.f, 2.f );
    menu.addBar( &bar );

	menu.setGlobalBackgroundColorGreyScale( 0.30f, 0.90f );
	menu.setBarFontColor( 0.6f, 1.0f, 0.3f, 1.0f );
	//menu.setBarFontColor( 0.5f, 0.8f, 0.2f, 1.0f );
	menu.setColFontColor( 0.3f, 0.6f, 1.0f, 1.0f );
	//menu.setColFontColor( 0.2f, 0.5f, 0.8f, 1.0f );

    // tell it the screem dimensions so it can draw
	//main_viewport_t *v = M_GetMainViewport();	
	//menu.setScreenDim( v->res->width, v->res->height );

	menu.setScreenDim( M_Width(), M_Height() );
}



/*
================
 ME_DrawMenu
================
*/
void ME_DrawMenu( void ) {
    CL_ModelViewInit2D();

    menu.draw();
}


/*
====================
 ME_Tics
====================
*/
int ME_Tics ( void ) {
    static uint32 starttime_ms = 0;
    if (!starttime_ms) {
        starttime_ms = Com_Millisecond();
    }
    return (Com_Millisecond()-starttime_ms)* ME_LOGIC_FPS/1000;
}


void ME_DrawEntColModel( Entity_t *E ) {
	if ( !E )
		return;
	if ( !E->collidable )
		return;

    gglLineWidth( 2.0f );
	gglDisable( GL_TEXTURE_2D );

	if ( E->col ) {
    	gglColor4f( 0.7f, 0.7f, 1.0f, 0.75f );
		GL_DRAW_OUTLINE_4V( E->col->box );
	}
	if ( E->triggerable ) {
		gglColor4f( 1.f, 1.f, 0.f, 1.f );
		GL_DRAW_OUTLINE_4V( E->trig );
	}
	if ( E->clipping ) {
		gglColor4f( 0.f, 1.f, 0.f, 1.f );
		GL_DRAW_OUTLINE_4V( E->clip );
	}

	gglEnable( GL_TEXTURE_2D );
    gglColor4f( 1.f, 1.f, 1.f, 1.f );
    gglLineWidth( 1.0f );
}


void ME_DrawMouseHighlights( void ) {

    if ( !stretchModeHighlight || !stretch.node )
        return;

    //main_viewport_t *view = M_GetMainViewport();
    poly_t *p = &stretch.node->val->poly;


    float c[2] = { p->x + p->w / 2.f, p->y + p->h / 2.f };

    //
    // Fucking stupid fucking mouse cursor thing, just create a fucking poly highlight over the edge
    //  that is activated for stretching and be done with it.  god, what a fucking waste of time.
    //
    float v[8];
    float w = 45.f; // world
    switch ( stretchModeHighlight ) {
    case ME_SM_LEFT:
        v[0] = p->x - w;
        v[1] = p->y;
        v[2] = p->x + w;
        v[3] = p->y;
        v[4] = p->x + w;
        v[5] = p->y + p->h;
        v[6] = p->x - w;
        v[7] = p->y + p->h;
        break;
    case ME_SM_RIGHT:
        v[0] = p->x + p->w - w;
        v[1] = p->y;
        v[2] = p->x + p->w + w;
        v[3] = p->y;
        v[4] = p->x + p->w + w;
        v[5] = p->y + p->h;
        v[6] = p->x + p->w - w;
        v[7] = p->y + p->h;
        break;
    case ME_SM_TOP:
        v[0] = p->x;
        v[1] = p->y + p->h - w;
        v[2] = p->x + p->w;
        v[3] = p->y + p->h - w;
        v[4] = p->x + p->w;
        v[5] = p->y + p->h + w; 
        v[6] = p->x;
        v[7] = p->y + p->h + w;
        break;
    case ME_SM_BOT:
        v[0] = p->x;
        v[1] = p->y - w;
        v[2] = p->x + p->w;
        v[3] = p->y - w;
        v[4] = p->x + p->w;
        v[5] = p->y + w;
        v[6] = p->x;
        v[7] = p->y + w;
        break;
    }

    M_TranslateRotate2DVertSet( v, c, p->angle );
    M_WorldToScreen( v );
    M_WorldToScreen( &v[2] );
    M_WorldToScreen( &v[4] );
    M_WorldToScreen( &v[6] );

    gglEnable( GL_BLEND );
    gglBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    // faint yellow
    gglColor4f( 1.0f, 1.0f, 0.2f, 0.44f );

    gglBegin( GL_QUADS );
    gglVertex3f( v[0], v[1], 0.0f );
    gglVertex3f( v[2], v[3], 0.0f );
    gglVertex3f( v[4], v[5], 0.0f );
    gglVertex3f( v[6], v[7], 0.0f );
    gglEnd();

    // yellow
    gglColor4f( 1.0f, 1.0f, 0.0f, 1.0f );

    gglBegin( GL_LINE_STRIP );
    gglVertex3f( v[0], v[1], 0.0f );
    gglVertex3f( v[2], v[3], 0.0f );
    gglVertex3f( v[4], v[5], 0.0f );
    gglVertex3f( v[6], v[7], 0.0f );
    gglVertex3f( v[0], v[1], 0.0f );
    gglEnd();
}

#if 0
    const float m = 3.f, l = 8.f;
    float v[2] = { 0.f, 0.f };

    float wm[2] = { (float)me_mouse.x, (float)me_mouse.y };
    M_ScreenToWorld( wm );

    // first see if its a corner
    if ( stretchModeHighlight & ME_SM_CORNER ) {
        if ( stretchModeHighlight & ME_SM_LEFT ) {
            v[0] = stretchPoly->x;
        } else if ( stretchModeHighlight & ME_SM_RIGHT ) {
            v[0] = stretchPoly->x + stretchPoly->w;
        } else {
            return 0;
        }
        if ( stretchModeHighlight & ME_SM_TOP ) {
            v[1] = stretchPoly->y + stretchPoly->h;
        } else if ( stretchModeHighlight & ME_SM_BOT ) {
            v[1] = stretchPoly->y;
        } else {
            return 0;
        }

        M_TranslateRotate2d( v, c, p->angle );
        M_WorldToScreen( v );

        gglBegin( GL_QUADS );
        gglVertex3f( v[0]-m, v[1]-m, 0.0f );
        gglVertex3f( v[0]+m, v[1]-m, 0.0f );
        gglVertex3f( v[0]+m, v[1]+m, 0.0f );
        gglVertex3f( v[0]-m, v[1]+m, 0.0f );
        gglEnd();

    } else {

        // put the mouse into virtual, un-rotated coordinate system
        M_TranslateRotate2d( wm, c, -p->angle );

        // create 2 vectors:
        // - mouse to center of poly
        float mc[2] = { wm[0] - c[0], wm[1] - c[1] };
        // - side-median to center of poly & side-point
        float sm[2];
        float sp[2]; // also get point of the side median tip
        switch ( stretchModeHighlight ) {
        case ME_SM_LEFT:
            sp[0] = p->x; 
            sp[1] = c[1];
            break;
        case ME_SM_RIGHT:
            sp[0] = p->x + p->w; 
            sp[1] = c[1];
            break;
        case ME_SM_TOP:
            sp[0] = c[0]; 
            sp[1] = p->y + p->h;
            break;
        case ME_SM_BOT:
            sp[0] = c[0];
            sp[1] = p->y;
            break;
        default:
            return 0;
        }
        
        sm[0] = sp[0] - c[0];
        sm[1] = sp[1] - c[1];


        // angle between these two
        float theta = M_GetAngleRadians( mc, sm );

        // yields magnitude of vector that is along side of cube
        float mag_opp = tanf( theta ) * M_Magnitude2d( sm );

        // magnitude of hypoteneus vector. goes from center to desired edge
        //  point.
        //float mag_hyp = M_Magnitude2d( sm ) / cosf( theta ) ;

        // figure out which way it is pointing
        // if we know the direction and the magnitude, we have the point 
        // because the coordinate system is aligned, it removed one axis from
        //  the decision process
        // a magnitude in this case ( axis aligned ) is a vector 
        //  with one of its values set to zero
        // go ahead and get Vhyp
        float Vside[2];
        switch ( stretchModeHighlight ) {
        case ME_SM_LEFT:
        case ME_SM_RIGHT:
            Vside[0] = 0.f;
            if ( wm[1] < sp[1] )  
                Vside[1] = sp[1] - mag_opp;
            else 
                Vside[1] = sp[1] + mag_opp;
            break;
        case ME_SM_TOP:
        case ME_SM_BOT:
            Vside[1] = 0.f;
            if ( wm[0] < sp[0] )  
                Vside[0] = sp[0] - mag_opp;
            else 
                Vside[0] = sp[0] + mag_opp;
            break;
        }

        float Vhyp[2] = { sm[0] + Vside[0], sm[1] + Vside[1] };

        float Vgoal[2] = { c[0] + Vhyp[0], c[1] + Vhyp[1] };

        // rotate back into scene
        M_TranslateRotate2d( Vgoal, c, p->angle );

        v[0] = Vgoal[0];
        v[1] = Vgoal[1];

        int rot = 0;



        M_WorldToScreen( v );


        gglBegin( GL_QUADS );
        gglVertex3f( v[0]-m, v[1]-m, 0.0f );
        gglVertex3f( v[0]+m, v[1]-m, 0.0f );
        gglVertex3f( v[0]+m, v[1]+m, 0.0f );
        gglVertex3f( v[0]-m, v[1]+m, 0.0f );
        gglEnd();

return 1;

        gglTranslatef( v[0], v[1], 0.f );
        gglRotatef( (float)rot+p->angle, 0.f, 0.f, -1.f ) ;
        gglTranslatef( -v[0], -v[1], 0.f );

        gglBegin( GL_QUADS );
        gglVertex3f( v[0]-m+0, v[1]-l, 0.f );
        gglVertex3f( v[0]-m+1, v[1]-l, 0.f );
        gglVertex3f( v[0]-m+1, v[1]+l, 0.f );
        gglVertex3f( v[0]-m+0, v[1]+l, 0.f );

        gglVertex3f( v[0]+m+0, v[1]-l, 0.f );
        gglVertex3f( v[0]+m+1, v[1]-l, 0.f );
        gglVertex3f( v[0]+m+1, v[1]+l, 0.f );
        gglVertex3f( v[0]+m+0, v[1]+l, 0.f );
        gglEnd();
    }
    return 1;
}
#endif

/*
int ME_DrawMouseHighlights( void ) {

    if ( !stretchModeHighlight || !stretchPoly )
        return 0 ;
    return 1;

    main_viewport_t *view = M_GetMainViewport();

    poly_t *sp = stretchPoly;

    // yellow
    gglColor4f( 1.0f, 1.0f, 0.0f, 1.0f );
    
    const float m = 3.f, l = 7.f;

    float x, y, w, h;

    gglBegin( GL_QUADS );

    if ( stretchModeHighlight & ME_SM_CORNER ) {
        if ( stretchModeHighlight & ME_SM_LEFT ) {
            x = stretchPoly->x;
        } else {
            x = stretchPoly->x + stretchPoly->w;
        }
        if ( stretchModeHighlight & ME_SM_TOP ) {
            y = stretchPoly->y + stretchPoly->h;
        } else {
            y = stretchPoly->y;
        }

        // screen coords
        x -= view->world.x;
        y -= view->world.y;
        x *= view->iratio;
        y *= view->iratio;

        float v[2] = { x, y };
        // need to rotate them
        float c[2];
        c[0] = ( sp->x + sp->w / 2.f ) * view->iratio;
        c[1] = ( sp->y + sp->h / 2.f ) * view->iratio;
        M_TranslateRotate2d( v, c, -sp->angle );
        x = v[0]; y = v[1];

        gglVertex3f( x-m, y-m, 0.0f );
        gglVertex3f( x+m, y-m, 0.0f );
        gglVertex3f( x+m, y+m, 0.0f );
        gglVertex3f( x-m, y+m, 0.0f );
    } else {
        switch( stretchModeHighlight ) {
        case ME_SM_LEFT_VERTICAL:
            x = stretchPoly->x;
        case ME_SM_RIGHT_VERTICAL:
            if ( stretchModeHighlight & ME_SM_RIGHT )
                x = stretchPoly->x + stretchPoly->w;
            y = (float) me_mouse.y;
            
            x -= view->world.x;
            y -= view->world.y;
            x *= view->iratio;
            y *= view->iratio;

            gglVertex3f( x-m+0, y-l, 0.f );
            gglVertex3f( x-m+1, y-l, 0.f );
            gglVertex3f( x-m+1, y+l, 0.f );
            gglVertex3f( x-m+0, y+l, 0.f );

            gglVertex3f( x+m+0, y-l, 0.f );
            gglVertex3f( x+m+1, y-l, 0.f );
            gglVertex3f( x+m+1, y+l, 0.f );
            gglVertex3f( x+m+0, y+l, 0.f );
            break;
        case ME_SM_TOP_HORIZONTAL:
            y = stretchPoly->y + stretchPoly->h;
        case ME_SM_BOT_HORIZONTAL:
            if ( stretchModeHighlight & ME_SM_BOT )
                y = stretchPoly->y;
            x = (float) me_mouse.x;
            
            x -= view->world.x;
            y -= view->world.y;
            x *= view->iratio;
            y *= view->iratio;

            gglVertex3f( x-l, y-m+0, 0.f );
            gglVertex3f( x+l, y-m+0, 0.f );
            gglVertex3f( x+l, y-m+1, 0.f );
            gglVertex3f( x-l, y-m+1, 0.f );

            gglVertex3f( x-l, y+m+0, 0.f );
            gglVertex3f( x+l, y+m+0, 0.f );
            gglVertex3f( x+l, y+m+1, 0.f );
            gglVertex3f( x-l, y+m+1, 0.f );
            break;
        }
    }
    gglEnd();
    return 1;
}
*/


void ME_DrawMouseCursor( void ) {

    CL_ModelViewInit2D();

    // extra mouse-over highlights
    ME_DrawMouseHighlights();

    // draw the cursor
    //gglColor4f( 1.0f, 0.7f, 0.2f, 1.0f );
    gglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
    CL_DrawStretchPic( (float)(me_mouse.x-CURSOR_HALF), (float)(me_mouse.y-CURSOR_HALF), (float)CURSOR_WIDTH, (float)CURSOR_WIDTH, 0.f, 0.f, 1.f, 1.f, cursor_img );

/* debug shit we dont want to see
    // get the info for this frame
    mouseFrame_t *mframe = Get_MouseFrame();

    // print button number if pressed (check for all 8, not that I've ever seen over '3')
    gglColor4f( 1.0f, 0.0f, 1.0f, 1.0f );
    for ( uint i = 0; i < 8; i++ ) {
        if ( mframe->mb[i] & 0x80 ) {
            CL_DrawChar( 300, 330, 20, 20, '0' + i );
        }
    }

    // print deltas
    gglColor4f( 0.0f, 1.0f, 0.0f, 1.0f );
    int off = CL_DrawInt( 4, 460, 20, 20, mframe->x, 14 );
    CL_DrawChar( off+20, 460, 20, 20, ',' );
    CL_DrawInt( off+60, 460, 20, 20, -mframe->y, 14 );
*/

}


void ME_DrawAABB ( poly_t *p ) {
    gglPushAttrib( GL_CURRENT_BIT );
    gglPushMatrix();
    gglColor4f( 1.f, 1.f, 1.f, 1.f );
    
    // middle point of poly
    float m[2];
    m[0] = p->x + p->w / 2.f;
    m[1] = p->y + p->h / 2.f;

    // create 4 points, ccwise, bot-left point first
    float v[8];
    v[0] = p->x;
    v[1] = p->y;
    v[2] = p->x + p->w;
    v[3] = p->y;
    v[4] = p->x + p->w;
    v[5] = p->y + p->h;
    v[6] = p->x;
    v[7] = p->y + p->h;

//    float r[8] = {v[0],v[1],v[2],v[3],v[4],v[5],v[6],v[7]}; // save these
    
    // rotate to get vertices (reverse)
    M_TranslateRotate2d( v, m, -p->angle );
    M_TranslateRotate2d( &v[2], m, -p->angle );
    M_TranslateRotate2d( &v[4], m, -p->angle );
    M_TranslateRotate2d( &v[6], m, -p->angle );

    // outline poly in reverse rotation
    gglBegin( GL_LINE_STRIP );
    gglVertex3f( v[0], v[1], 0.f );
    gglVertex3f( v[2], v[3], 0.f );
    gglVertex3f( v[4], v[5], 0.f );
    gglVertex3f( v[6], v[7], 0.f );
    gglVertex3f( v[0], v[1], 0.f );
    gglEnd(); 

    glPopMatrix();
    glPushMatrix();

    /*
    // rotate to get vertices
    M_TranslateRotate2d( r, m, p->angle );
    M_TranslateRotate2d( &r[2], m, p->angle );
    M_TranslateRotate2d( &r[4], m, p->angle );
    M_TranslateRotate2d( &r[6], m, p->angle );

     looks bad because the pixels are drawn over have the time by the poly fill
    // outline the actual poly
    gglBegin( GL_LINE_STRIP );
    gglVertex3f( r[0], r[1], 0.f );
    gglVertex3f( r[2], r[3], 0.f );
    gglVertex3f( r[4], r[5], 0.f );
    gglVertex3f( r[6], r[7], 0.f );
    gglVertex3f( r[0], r[1], 0.f );
    gglEnd(); 
    */


    // set AABB points and draw
    float q[4];
    q[0] = 1000000000000.0f;
    q[1] = 1000000000000.0f;
    q[2] = -100000000000.0f;
    q[3] = -1000000000000.0f;
    for ( unsigned int i = 0; i < 8; i+=2 ) {
        // furthest left
        if ( v[i] < q[0] ) 
            q[0] = v[i];
        // furthest down
        if ( v[i+1] < q[1] ) 
            q[1] = v[i+1];
        // furthest right
        if ( v[i] > q[2] ) 
            q[2] = v[i];
        // furthest up
        if ( v[i+1] > q[3] ) 
            q[3] = v[i+1];
    }

    gglBegin( GL_LINE_STRIP );
    gglVertex3f( q[0], q[1], 0.f );
    gglVertex3f( q[2], q[1], 0.f );
    gglVertex3f( q[2], q[3], 0.f );
    gglVertex3f( q[0], q[3], 0.f );
    gglVertex3f( q[0], q[1], 0.f );
    gglEnd();

    
    gglPopMatrix();
    gglPopAttrib();
}


void ME_DrawHighlights8vImmediate( float *c, mapNode_t *node ) {

    float m = 50.f;
    float v[8];
    float w[8];

	if ( !node )
		return;

	// keep the amount that the select highlight over extends constant
	//  over different magnifications, zoom amounts
	m = me_selectMargin->value() * M_Ratio();

    // ACTIVE / SELECTED (RED + PINK)
	if ( selected.checkNode( node ) ) {

        POLY_TO_VERT8( v, &node->val->poly );

        v[0] -= m;
        v[1] -= m;
        v[2] += m;
        v[3] -= m;
        v[4] += m;
        v[5] += m;
        v[6] -= m;
        v[7] += m;
        
        COPY8( w, v );

        M_TranslateRotate2DVertSet( v, c, node->val->poly.angle );

		// different shading color if locked
		if ( node->val->lock )
	        gglColor4f( 0.0f, 0.7f, 0.9f, 0.3f );
		else
	        gglColor4f( 1.0f, 0.3f, 0.3f, 0.3f );

		GL_DRAW_QUAD_8V( v );
        
        COPY8( v, w );

        // do the border in immediate
        v[0] -= 1;
        v[1] -= 1;
        v[2] += 1;
        v[3] -= 1;
        v[4] += 1;
        v[5] += 1;
        v[6] -= 1;
        v[7] += 1;

        M_TranslateRotate2DVertSet( v, c, node->val->poly.angle );

        gglColor4f( 1.0f, 0.0f, 0.0f, 0.88f );
		GL_DRAW_OUTLINE_8V( v );
    }

	// TILE LOCKED and NOT SELECTED : faint blue green, no border
	else if ( node->val->lock ) {

        POLY_TO_VERT8( v, &node->val->poly );

/*
        v[0] -= m;
        v[1] -= m;
        v[2] += m;
        v[3] -= m;
        v[4] += m;
        v[5] += m;
        v[6] -= m;
        v[7] += m;
        
        COPY8( w, v );
*/

        M_TranslateRotate2DVertSet( v, c, node->val->poly.angle );

        gglColor4f( 0.0f, 0.7f, 0.9f, 0.2f );
		GL_DRAW_QUAD_8V( v );
 /*
        COPY8( v, w );

        // do the border in immediate
        v[0] -= 1;
        v[1] -= 1;
        v[2] += 1;
        v[3] -= 1;
        v[4] += 1;
        v[5] += 1;
        v[6] -= 1;
        v[7] += 1;

        M_TranslateRotate2DVertSet( v, c, node->val->poly.angle );

        gglColor4f( 0.0f, 0.5f, 0.8f, 0.88f );
		GL_DRAW_OUTLINE_8V( v );
*/
    }

    // mouseover highlight (FAINT WHITE / TRANLUCENT )
    //if ( p == highlight.poly ) {
    if ( highlight.node == node ) {

		POLY_TO_VERT8( v, &node->val->poly );

        v[0] -= m;
        v[1] -= m;
        v[2] += m;
        v[3] -= m;
        v[4] += m;
        v[5] += m;
        v[6] -= m;
        v[7] += m;

        COPY8( w, v );

        M_TranslateRotate2DVertSet( v, c, node->val->poly.angle );

        gglColor4f( 1.0f, 1.0f, 1.0f, 0.22f );
		GL_DRAW_QUAD_8V( v );
        
        COPY8( v, w );

        // do the border in immediate
        v[0] -= 1;
        v[1] -= 1;
        v[2] += 1;
        v[3] -= 1;
        v[4] += 1;
        v[5] += 1;
        v[6] -= 1;
        v[7] += 1;

        M_TranslateRotate2DVertSet( v, c, node->val->poly.angle );

        // white
        gglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
		GL_DRAW_OUTLINE_8V( v );
    }

	// draw rotateMode handles to show mode enabled
	if ( ( inputMode == MODE_POLY_ROTATE || rotateMode ) && selected.checkNode( node ) ) {
		if ( rotationType == ROT_ONE ) {
			POLY_TO_VERT8( v, &node->val->poly );
			poly_t *p = &node->val->poly;
			float s[2] = { node->val->poly.x + node->val->poly.w * 0.5f, p->y + p->h + 400.0f };
			float r[2] = { p->x + p->w + 400.0f, p->y + p->h * 0.5f };
			M_TranslateRotate2d( s, c, p->angle );
			M_TranslateRotate2d( r, c, p->angle );
			// Translate+Rotate
			gglColor4f( 1.0f, 0.f, 0.f, 1.0f );
			gglBegin( GL_LINES );
			gglVertex2fv( c );
			gglVertex2fv( s );
			gglColor4f( 0.0f, 0.f, 1.f, 1.0f );
			gglVertex2fv( c );
			gglVertex2fv( r );
			gglEnd();
		}
	}

    gglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
}

void ME_DrawGroupRotationHighlight( void ) {
    CL_ModelViewInit2D();
    main_viewport_t *v = M_GetMainViewport();
    gglScalef( v->iratio, v->iratio, 0.f );
    gglTranslatef( -v->world.x, -v->world.y, 0.f );

// draw white dashed line (stipple) around the large poly
	float w[8];
	if ( rotate.poly ) {
		rotate.poly->toOBB( w );

		gglEnable( GL_LINE_STIPPLE );
		gglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
		gglLineWidth( 1.0f );
		gglLineStipple( 1, 0xFF00 );
		GL_DRAW_OUTLINE_8V( w );

		gglDisable( GL_LINE_STIPPLE );

// draw the red and blue orientation lines like in single poly rotate
		float c[2];
		rotate.poly->getCenter( c );
		poly_t *p = rotate.poly;
		float s[2] = { p->x + p->w * 0.5f, p->y + p->h + 600.0f };
		float r[2] = { p->x + p->w + 600.0f, p->y + p->h * 0.5f };
			M_TranslateRotate2d( s, c, p->angle );
			M_TranslateRotate2d( r, c, p->angle );
			// Translate+Rotate
			gglColor4f( 1.0f, 0.f, 0.f, 1.0f );
			gglBegin( GL_LINES );
			gglVertex2fv( c );
			gglVertex2fv( s );
			gglColor4f( 0.0f, 0.f, 1.f, 1.0f );
			gglVertex2fv( c );
			gglVertex2fv( r );
			gglEnd();
	}
	

}

/*
void ME_DrawHighlights8v( float *c, poly_t *p ) {

    float m = 50.f;
    float v[8];
    float w[8];


    // ACTIVE / SELECTED (RED + PINK)
    if ( p == active.poly ) {

        POLY_TO_VERT8( v, p );

        v[0] -= m;
        v[1] -= m;
        v[2] += m;
        v[3] -= m;
        v[4] += m;
        v[5] += m;
        v[6] -= m;
        v[7] += m;
        
        COPY8( w, v );

        M_TranslateRotate2DVertSet( v, c, p->angle );

        quad_t q;
        q.setColor4f( 1.0f, 0.3f, 0.3f, 0.4f );
        q.setCoords4v( v );
        quadlist.add( q );
        
        COPY8( v, w );

        // do the border in immediate
        v[0] -= 1;
        v[1] -= 1;
        v[2] += 1;
        v[3] -= 1;
        v[4] += 1;
        v[5] += 1;
        v[6] -= 1;
        v[7] += 1;

        M_TranslateRotate2DVertSet( v, c, p->angle );

        gglColor4f( 1.0f, 0.0f, 0.0f, 0.88f );

        gglBegin( GL_LINE_STRIP );
        gglVertex3f( v[0], v[1], 0.0f );
        gglVertex3f( v[2], v[3], 0.0f );
        gglVertex3f( v[4], v[5], 0.0f );
        gglVertex3f( v[6], v[7], 0.0f );
        gglVertex3f( v[0], v[1], 0.0f );
        gglEnd();
    }

    // mouseover highlight (FAINT WHITE / TRANLUCENT )
    //if ( p == highlight.poly ) {
    if ( highlight.node && p == &highlight.node->val->poly ) {

        POLY_TO_VERT8( v, p );

        v[0] -= m;
        v[1] -= m;
        v[2] += m;
        v[3] -= m;
        v[4] += m;
        v[5] += m;
        v[6] -= m;
        v[7] += m;

        COPY8( w, v );

        M_TranslateRotate2DVertSet( v, c, p->angle );

        quad_t q;
        q.setColor4f( 1.0f, 1.0f, 1.0f, 0.22f );
        q.setCoords4v( v );
        quadlist.add( q );
        
        COPY8( v, w );

        // do the border in immediate
        v[0] -= 1;
        v[1] -= 1;
        v[2] += 1;
        v[3] -= 1;
        v[4] += 1;
        v[5] += 1;
        v[6] -= 1;
        v[7] += 1;

        M_TranslateRotate2DVertSet( v, c, p->angle );

        // white
        gglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

        gglBegin( GL_LINE_STRIP );
        gglVertex3f( v[0], v[1], 0.0f );
        gglVertex3f( v[2], v[3], 0.0f );
        gglVertex3f( v[4], v[5], 0.0f );
        gglVertex3f( v[6], v[7], 0.0f );
        gglVertex3f( v[0], v[1], 0.0f );
        gglEnd();
    }
}
*/

/*
void ME_DrawHighlights( poly_t * p ) {

    float v[8];
    float m = 50.f;

    // ACTIVE / SELECTED (RED + PINK)
    if ( p == active.poly ) {

        gglColor4f( 1.0f, 0.3f, 0.3f, 0.22f );
        
        POLY_TO_VERT8( v, p );

        v[0] -= m;
        v[1] -= m;
        v[2] += m;
        v[3] -= m;
        v[4] += m;
        v[5] += m;
        v[6] -= m;
        v[7] += m;
        
        gglBegin( GL_QUADS );
        gglVertex3f( v[0], v[1], 0.0f );
        gglVertex3f( v[2], v[3], 0.0f );
        gglVertex3f( v[4], v[5], 0.0f );
        gglVertex3f( v[6], v[7], 0.0f );
        gglEnd();

        v[0] -= 1;
        v[1] -= 1;
        v[2] += 1;
        v[3] -= 1;
        v[4] += 1;
        v[5] += 1;
        v[6] -= 1;
        v[7] += 1;

        gglColor4f( 1.0f, 0.0f, 0.0f, 0.88f );

        gglBegin( GL_LINE_STRIP );
        gglVertex3f( v[0], v[1], 0.0f );
        gglVertex3f( v[2], v[3], 0.0f );
        gglVertex3f( v[4], v[5], 0.0f );
        gglVertex3f( v[6], v[7], 0.0f );
        gglVertex3f( v[0], v[1], 0.0f );
        gglEnd();
    }


    // mouseover highlight (FAINT WHITE / TRANLUCENT )
    if ( highlight.node && p == &highlight.node->val->poly ) {
        POLY_TO_VERT8( v, p );

        v[0] -= m;
        v[1] -= m;
        v[2] += m;
        v[3] -= m;
        v[4] += m;
        v[5] += m;
        v[6] -= m;
        v[7] += m;

        // faint 
        gglColor4f( 1.0f, 1.0f, 1.0f, 0.22f );

        gglBegin( GL_QUADS );
        gglVertex3f( v[0], v[1], 0.0f );
        gglVertex3f( v[2], v[3], 0.0f );
        gglVertex3f( v[4], v[5], 0.0f );
        gglVertex3f( v[6], v[7], 0.0f );
        gglEnd();

        v[0] -= 1;
        v[1] -= 1;
        v[2] += 1;
        v[3] -= 1;
        v[4] += 1;
        v[5] += 1;
        v[6] -= 1;
        v[7] += 1;


        // white
        gglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

        gglBegin( GL_LINE_STRIP );
        gglVertex3f( v[0], v[1], 0.0f );
        gglVertex3f( v[2], v[3], 0.0f );
        gglVertex3f( v[4], v[5], 0.0f );
        gglVertex3f( v[6], v[7], 0.0f );
        gglVertex3f( v[0], v[1], 0.0f );
        gglEnd();

    }
    gglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
}
*/

void ME_DrawEntityHighlights( node_c<Entity_t*> *node ) {
	if ( !node )
		return;

	gglDisable( GL_TEXTURE_2D );

    const float m = me_selectMargin->value() * M_Ratio();
    float v[8];
	float w[8];
	float c[2];

    // ACTIVE / SELECTED (RED + PINK)
	if ( entSelected.checkNode( node ) ) {
		POLY_TO_VERT8( v, &node->val->poly )
        //COPY8( v, node->val->obb );
        v[0] -= m;
        v[1] -= m;
        v[2] += m;
        v[3] -= m;
        v[4] += m;
        v[5] += m;
        v[6] -= m;
        v[7] += m;
		node->val->poly.getCenter(  c ) ;
		COPY8( w, v );
		M_TranslateRotate2DVertSet( v, c, node->val->poly.angle );
        gglColor4f( 0.0f, 0.7f, 0.9f, 0.3f );
		GL_DRAW_QUAD_8V( v );

        // do the border in immediate
        w[0] -= 1;
        w[1] -= 1;
        w[2] += 1;
        w[3] -= 1;
        w[4] += 1;
        w[5] += 1;
        w[6] -= 1;
        w[7] += 1;
		M_TranslateRotate2DVertSet( w, c, node->val->poly.angle );
        gglColor4f( 1.0f, 0.0f, 0.0f, 0.88f );
		GL_DRAW_OUTLINE_8V( w );
    }

    gglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
	gglEnable( GL_TEXTURE_2D );
}


int current_texture = 0;

void ME_DrawTileColModels( void ) {

	node_c<mapTile_t *> *node = tiles->gethead();
	if ( !node )
		return;

    CL_ModelViewInit2D();
	gglColor4f( 0.7f, 0.7f, 1.0f, 0.75f );
	gglLineWidth( 2.0f );
	main_viewport_t *v = M_GetMainViewport();
    gglScalef( v->iratio, v->iratio, 0.f );
    gglTranslatef( -v->world.x, -v->world.y, 0.f );

	while ( node ) {
		if ( !node->val || !node->val->col ) {
			node = node->next;
			continue;
		}

		GL_DRAW_OUTLINE_4V( node->val->col->box );

/* maybe later
		if ( node->val->col->type == CM_NONE ) {
			node = node->next;
			continue;
		}
		else if ( CM_AABB == node->val->col->type ) {
			AABB_node *n = node->val->col->AABB;
			while ( n ) {
				float v[8];
				AABB_TO_OBB( v, n->AABB );
				GL_DRAW_OUTLINE_8V( v );
				n = n->next;
			}
		}
		else if ( CM_OBB == node->val->col->type ) {
			OBB_node *n = node->val->col->OBB ;
			while ( n ) {
				GL_DRAW_OUTLINE_8V( n->OBB );
				n = n->next;
			}
		}
*/

		node = node->next;
	}
	gglLineWidth( 1.0f );
}


void ME_DrawTiles( void ) {

	// arrange the lowest layers to draw first
	if ( tiles->size() && tiles->needsort )
		tiles->sort();

    node_c<mapTile_t *> *node = tiles->gethead();

    if ( !node )
        return;

    GLdouble mat[16];

    CL_ModelViewInit2D();

    gglGetDoublev( GL_MODELVIEW_MATRIX, mat );

    main_viewport_t *v = M_GetMainViewport();

    // scale everything (translate from world coords to screen coords)
    gglScalef( v->iratio, v->iratio, 0.f );

    // now transfer viewport position -1/2w, -1/2h , bring the low left corner
    //  to center
    // except that your provide world coordinates because everything is already
    //  being scaled by the modelview matrix w/ the glScale() call
    gglTranslatef( -v->world.x, -v->world.y, 0.f );

  	gglEnable( GL_TEXTURE_2D );
	gglTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

	// check material visibility is enabled
	if ( !(visibility & ME_VIS_MATERIAL) ) {
		node = NULL;
	}

	image_t * tex;
	image_t * mask;

    while ( node ) 
    {
		if ( node->val->background ) {
			node = node->next;
			continue;
		}

        float tx = node->val->poly.x + node->val->poly.w / 2.f;
        float ty = node->val->poly.y + node->val->poly.h / 2.f;

        float c[2] = { tx, ty };
        float v[8];

        // convert poly to raw vertex data
        POLY_TO_VERT8( v, &node->val->poly );

        // rotate it 
        M_TranslateRotate2DVertSet( v, c, node->val->poly.angle );

		material_t *m = node->val->mat;

		switch ( m->type ) {
		case MTL_COLOR:
			gglDisable( GL_TEXTURE_2D );
			gglColor4fv( node->val->mat->color );
			break;
		case MTL_TEXTURE_STATIC:
			if ( !node->val->mat->img )
				goto do_default2;
			gglEnable( GL_TEXTURE_2D );
			gglBindTexture( GL_TEXTURE_2D, node->val->mat->img->texhandle );
			break;
		case MTL_COLORMASK:
			if ( !node->val->mat->img )
				goto do_default2;
			gglEnable( GL_TEXTURE_2D );
			gglBindTexture( GL_TEXTURE_2D, node->val->mat->img->texhandle );
			gglColor4fv( node->val->mat->color );
			break;
	
		case MTL_MASKED_TEXTURE:
		case MTL_MASKED_TEXTURE_COLOR:
			tex = node->val->mat->img;
			mask = node->val->mat->mask;
			if ( !tex || !mask )
				goto do_default2;

			gglActiveTexture( GL_TEXTURE0 );
			gglEnable( GL_TEXTURE_2D );
			gglBindTexture( GL_TEXTURE_2D, tex->texhandle );
			gglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

			gglActiveTexture( GL_TEXTURE1 );
			gglEnable( GL_TEXTURE_2D );
			gglBindTexture( GL_TEXTURE_2D, mask->texhandle );
			gglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

			if ( m->type == MTL_MASKED_TEXTURE_COLOR )
				gglColor4fv( node->val->mat->color );

			gglBegin( GL_QUADS );
		    gglMultiTexCoord2f( GL_TEXTURE0, m->s[0], m->t[0] );
		    gglMultiTexCoord2f( GL_TEXTURE1, m->s[0], m->t[0] );
	        gglVertex3f( v[0], v[1], 0.f );
		    gglMultiTexCoord2f( GL_TEXTURE0, m->s[1], m->t[1] );
		    gglMultiTexCoord2f( GL_TEXTURE1, m->s[1], m->t[1] );
	        gglVertex3f( v[2], v[3], 0.f );
		    gglMultiTexCoord2f( GL_TEXTURE0, m->s[2], m->t[2] );
		    gglMultiTexCoord2f( GL_TEXTURE1, m->s[2], m->t[2] );
	        gglVertex3f( v[4], v[5], 0.f );
		    gglMultiTexCoord2f( GL_TEXTURE0, m->s[3], m->t[3] );
		    gglMultiTexCoord2f( GL_TEXTURE1, m->s[3], m->t[3] );
	        gglVertex3f( v[6], v[7], 0.f );
		    gglEnd();
	
    		gglActiveTexture( GL_TEXTURE1 );
    		gglDisable( GL_TEXTURE_2D );
    		gglActiveTexture( GL_TEXTURE0 );
    		gglDisable( GL_TEXTURE_2D );

			break;
		default:
do_default2:
			// FIXME: should have a "NoTex" pattern to put on unset tiles
			gglDisable( GL_TEXTURE_2D );
			gglColor4f( 0.2f, 0.4f, 0.6f, 1.0f );
			break;
		}

		// standard draw call
		if ( 	m->type != MTL_MASKED_TEXTURE && 
				m->type != MTL_MASKED_TEXTURE_COLOR ) {

			gglBegin( GL_QUADS );

	    	gglTexCoord2f( m->s[0], m->t[0] );
        	gglVertex3f( v[0], v[1], 0.f );

	    	gglTexCoord2f( m->s[1], m->t[1] );
	        gglVertex3f( v[2], v[3], 0.f );
		
		    gglTexCoord2f( m->s[2], m->t[2] );
   		    gglVertex3f( v[4], v[5], 0.f );
	
		    gglTexCoord2f( m->s[3], m->t[3] );
	        gglVertex3f( v[6], v[7], 0.f );
	
		    gglEnd();
		}


		//
        // highlights
		//
        POLY_TO_VERT8( v, &node->val->poly );
        M_TranslateRotate2DVertSet( v, c, node->val->poly.angle );

		gglDisable( GL_TEXTURE_2D );
        //ME_DrawHighlights( &n->val->poly );
        ME_DrawHighlights8vImmediate( c, node );
		gglEnable( GL_TEXTURE_2D );

		node = node->next;
    }

	gglDisable( GL_TEXTURE_2D );

	//
	// draw the collision models, if visible
	//
	if ( visibility & ME_VIS_COLMODEL ) {
		ME_DrawTileColModels();
	}

	if ( rotateMode && ( rotationType == ROT_MANY ) ) {
		ME_DrawGroupRotationHighlight();
	}
}

void ME_DrawEntityColModel( void ) {
}

void ME_DrawEntities( void ) {

    node_c<Entity_t *> *node = me_ents->gethead();

    if ( !node )
        return;

    CL_ModelViewInit2D();

    main_viewport_t *mvp = M_GetMainViewport();
    gglScalef( mvp->iratio, mvp->iratio, 0.f );
    gglTranslatef( -mvp->world.x, -mvp->world.y, 0.f );

  	gglEnable( GL_TEXTURE_2D );
	gglTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

	if ( !(visibility & ME_VIS_MATERIAL) && !(visibility & ME_VIS_COLMODEL) ) {
		node = NULL;
	}

	// FIXME: entities should be drawn in order of type

    while ( node ) 
    {
		// check material visibility is enabled
		if ( !(visibility & ME_VIS_MATERIAL) && (visibility & ME_VIS_COLMODEL) ) {
			ME_DrawEntColModel( node->val );
			node = node->next;
			continue;
		}

		float v[8];
		COPY8( v, node->val->obb );

		material_t *m = ent_notex;
		materialType_t type = MTL_TEXTURE_STATIC;
		if ( node->val->mat ) {
			m = node->val->mat;
			type = m->type;
		}

		if ( ! m->img ) {
			m->img = ent_notex->img;
		}

		gglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

		switch ( type ) {
		case MTL_COLOR:
			gglDisable( GL_TEXTURE_2D );
			gglColor4fv( m->color );
			break;
		case MTL_TEXTURE_STATIC:
			gglEnable( GL_TEXTURE_2D );
			gglBindTexture( GL_TEXTURE_2D, m->img->texhandle );
			break;
		case MTL_COLORMASK:
			gglEnable( GL_TEXTURE_2D );
			gglBindTexture( GL_TEXTURE_2D, m->img->texhandle );
			gglColor4fv( m->color );
			break;
	
		case MTL_MASKED_TEXTURE:
		case MTL_MASKED_TEXTURE_COLOR:
			if ( !m->img || !m->mask )
				goto do_default2;

			gglActiveTexture( GL_TEXTURE0 );
			gglEnable( GL_TEXTURE_2D );
			gglBindTexture( GL_TEXTURE_2D, m->img->texhandle );
			gglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

			gglActiveTexture( GL_TEXTURE1 );
			gglEnable( GL_TEXTURE_2D );
			gglBindTexture( GL_TEXTURE_2D, m->mask->texhandle );
			gglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

			if ( type == MTL_MASKED_TEXTURE_COLOR )
				gglColor4fv( m->color );

			gglBegin( GL_QUADS );
		    gglMultiTexCoord2f( GL_TEXTURE0, m->s[0], m->t[0] );
		    gglMultiTexCoord2f( GL_TEXTURE1, m->s[0], m->t[0] );
	        gglVertex3f( v[0], v[1], 0.f );
		    gglMultiTexCoord2f( GL_TEXTURE0, m->s[1], m->t[1] );
		    gglMultiTexCoord2f( GL_TEXTURE1, m->s[1], m->t[1] );
	        gglVertex3f( v[2], v[3], 0.f );
		    gglMultiTexCoord2f( GL_TEXTURE0, m->s[2], m->t[2] );
		    gglMultiTexCoord2f( GL_TEXTURE1, m->s[2], m->t[2] );
	        gglVertex3f( v[4], v[5], 0.f );
		    gglMultiTexCoord2f( GL_TEXTURE0, m->s[3], m->t[3] );
		    gglMultiTexCoord2f( GL_TEXTURE1, m->s[3], m->t[3] );
	        gglVertex3f( v[6], v[7], 0.f );
		    gglEnd();
	
    		gglActiveTexture( GL_TEXTURE1 );
    		gglDisable( GL_TEXTURE_2D );
    		gglActiveTexture( GL_TEXTURE0 );
    		gglDisable( GL_TEXTURE_2D );

			break;
		default:
do_default2:
			// FIXME: should have a "NoTex" pattern to put on unset tiles
			gglDisable( GL_TEXTURE_2D );
			gglColor4f( 0.50f, 0.3f, 0.1f, 0.7f );
			break;
		}

		// standard draw call
		if ( type == MTL_NONE ) {
			gglBegin( GL_QUADS );
        	gglVertex3f( v[0], v[1], 0.f );
	        gglVertex3f( v[2], v[3], 0.f );
   		    gglVertex3f( v[4], v[5], 0.f );
	        gglVertex3f( v[6], v[7], 0.f );
		    gglEnd();
		} else if ( type != MTL_MASKED_TEXTURE && type != MTL_MASKED_TEXTURE_COLOR ) {
			gglBegin( GL_QUADS );
	    	gglTexCoord2f( m->s[0], m->t[0] );
        	gglVertex3f( v[0], v[1], 0.f );
	    	gglTexCoord2f( m->s[1], m->t[1] );
	        gglVertex3f( v[2], v[3], 0.f );
		    gglTexCoord2f( m->s[2], m->t[2] );
   		    gglVertex3f( v[4], v[5], 0.f );
		    gglTexCoord2f( m->s[3], m->t[3] );
	        gglVertex3f( v[6], v[7], 0.f );
		    gglEnd();
		}

		// Text
		// - name
		// - entType
		// - entClass


		//
        // highlights
		//
		ME_DrawEntityHighlights( node );

        //ME_DrawHighlights8vImmediate( c, node );





   //     POLY_TO_VERT8( v, &node->val->poly );
   //     M_TranslateRotate2DVertSet( v, c, node->val->poly.angle );

        //ME_DrawHighlights( &n->val->poly );
        
        //ME_DrawHighlights8vImmediate( c, node );

		if ( visibility & ME_VIS_COLMODEL ) 
			ME_DrawEntColModel( node->val );
        
		node = node->next;
    }

	gglDisable( GL_TEXTURE_2D );

	//
	// draw the collision models, if visible
	//
//	if ( visibility & ME_VIS_COLMODEL ) {
//		ME_DrawEntityColModel();
//	}
}


/*
void ME_DrawTilesNoTex( void ) {

    // 
    node_c<mapTile_t *> *n = tiles->gethead();

    if ( !n )
        return;

    GLdouble mat[16];

    CL_ModelViewInit2D();

    gglGetDoublev( GL_MODELVIEW_MATRIX, mat );

    main_viewport_t *v = M_GetMainViewport();


    // scale everything (translate from world coords to screen coords)
    gglScalef( v->iratio, v->iratio, 0.f );


    // now transfer viewport position -1/2w, -1/2h , bring the low left corner
    //  to center
    // except that your provide world coordinates because everything is already
    //  being scaled by the modelview matrix w/ the glScale() call
    gglTranslatef( -v->world.x, -v->world.y, 0.f );

    gglGetDoublev( GL_MODELVIEW_MATRIX, mat );

    // empty vertex array
    quadlist.init();
    quadlist.reset_keep_mem();

    while ( n ) 
    {
        // draw bound box outline
        //ME_DrawAABB( &n->val );

        float tx = n->val->poly.x + n->val->poly.w / 2.f;
        float ty = n->val->poly.y + n->val->poly.h / 2.f;

        float c[2] = { tx, ty };
        float v[8];

        // convert poly to raw vertex data
        POLY_TO_VERT8( v, &n->val->poly );

        // rotate it 
        M_TranslateRotate2DVertSet( v, c, n->val->poly.angle );

        // set and store the quad, into the vertex array
        quad_t q;
        q.setColor4f( 0.2f, 0.4f, 0.6f, 1.0f );
        q.setCoords4v( v );
        quadlist.add( q );

        // grey box , bot-right corner
        POLY_TO_VERT8( v, &n->val->poly );
        v[0] = c[0];
        v[5] = c[1];
        v[6] = c[0];
        v[7] = c[1];

        M_TranslateRotate2DVertSet( v, c, n->val->poly.angle );

        q.setColor4f( 0.7f, 0.7f, 0.7f, 1.f );
        q.setCoords4v( v );
        quadlist.add( q );


        // highlights
        POLY_TO_VERT8( v, &n->val->poly );
        M_TranslateRotate2DVertSet( v, c, n->val->poly.angle );

        ME_DrawHighlights8v( c, &n->val->poly );

        n = n->next;
    }


    // 
    gglEnableClientState( GL_VERTEX_ARRAY );
    gglEnableClientState( GL_COLOR_ARRAY );
    uint stride = 3 * sizeof(float) + sizeof(unsigned int);
    gglVertexPointer( 3, GL_FLOAT, stride, quadlist.data );
    gglColorPointer( 4, GL_UNSIGNED_BYTE, stride, &((byte*)quadlist.data)[sizeof(float)*3] );
    gglDrawArrays( GL_QUADS, 0, quadlist.length() * 4 );
}
*/


#if 0
int lines[14] = { 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192 };



void ME_DrawGridOld( void ) {
    if ( !drawGrid )
        return;
    
    gridlines.init();
    gridlines.reset_keep_mem();

    // clear
    CL_ModelViewInit2D();

    main_viewport_t *v = M_GetMainViewport();
    gglScalef( v->iratio, v->iratio, 0.f );
    gglTranslatef( -v->world.x, -v->world.y, 0.f );

    /*
    things to think about here:

    only the grid lines that make sense are drawn.  You put grid lines on power-of-2 divisions,
    1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192   
    certain of those are going to within some parameter on the min-focus,
    max-focus side in order to be drawn.  the rest will not.  

    we need to:
    - find the viewport edges in the world coordinate system.  
    - determine which lines will be drawn out of possible lines
    - create two vertices in world space for each line and a color
    - convert these vertices to screen coordinates and draw
    - colors and color strengths are determined by pow-2 divisible boundary alignments, independant of where
        the viewport happens to be.  so, every 1024, starting at 0,0 is a very strong color, etc..
    - DRAW BOUNDARIES:
        upper bound: if more than 1 line is on screen.  draw.
        lower bound: stop drawing if lines have less than 3 pixels between them.
    */


    int x[2] = { (int)v->world.x, (int)v->world.x };
    int y[2] = { (int)v->world.y, (int)v->world.y };

    x[1] += (int)(v->res->width * v->ratio);
    y[1] += (int)(v->res->height * v->ratio);

    // going to set the viewport params slightly outside , because if you dont do this,
    //  the floating point errors are visible
    x[0] -= 1; x[1] += 1; y[0] -= 1; y[1] += 1;

    const int ob = -WORLD_GRID_BOUND-1;

    //
    // vertical
    //
    for ( int line = 0; line < 14; line++ ) 
    {
        int sz = lines[line];


        // get first line
        int i = x[0] & ~(0x80<<24);
        i %= sz;
        int first_line = i ? x[0] - i + sz : x[0];
        
        // get last line
        i = x[1] & ~(1<<31);
        i %= sz;
        int last_line = x[1] - i;

        // test if on screen
        int on_screen = 0;
        for ( int i = first_line; i <= last_line; i += sz ) {
            if ( i >= x[0] && i < x[1] ) {
                if ( ++on_screen > v->res->width / 3 ) {
                    on_screen = 0;
                    break; // detect small line divisions as early as we can
                            // theres probably an equation for this, lazy ass
                }
            }
        }

        // upper bound
        if ( on_screen < 2 )
            continue;

        // lower bound
        if ( on_screen > v->res->width / res_x )
            continue;

        
        // add the lines to a list_c<line_t>
        for ( int i = first_line; i <= last_line; i += sz ) {
            line_t t;
            t.u[0] = (0==i) ? 0.f : (float)i;
            t.u[1] = (float)y[0];
            t.v[0] = (0==i) ? 0.f : (float)i;
            t.v[1] = (float)y[1];

//            i &= ~(1<<31);
/*
            if ( i % (sz*8) == 0 ) 
                t.c = 0x88888877;
            else
                t.c = 0x66666655;
*/

            if ( i % (sz*8) == 0 ) {
                t.c = 0x77888888;
                t.c2 = t.c;
            } else {
                t.c = 0x55666666;
                t.c2 = t.c;
            }

            gridlines.add( t );
        }
    }

    //
    // horizontal
    //
    for ( int line = 0; line < 14; line++ ) 
    {
        int sz = lines[line];

        // get first line
        int i = y[0] & ~(0x80<<24);
        i %= sz;
        int first_line = i ? y[0] - i + sz : y[0];
        
        // get last line
        i = y[1] & ~(1<<31);
        i %= sz;
        int last_line = y[1] - i;

        // test if on screen
        int on_screen = 0;
        for ( int i = first_line; i <= last_line; i += sz ) {
            if ( i >= y[0] && i < y[1] ) {
                if ( ++on_screen > v->res->height / res_y ) {
                    on_screen = 0;
                    break;
                }
            }
        }

        // upper bound
        if ( on_screen < 2 )
            continue;

        // lower bound
        if ( on_screen > v->res->height / res_y )
            continue;
        
        // add the lines to a list_c<line_t>
        for ( int i = first_line; i <= last_line; i += sz ) {
            line_t t;
            t.u[0] = (float)x[0];
            t.u[1] = (float)i;
            t.v[0] = (float)x[1];
            t.v[1] = (float)i;

//            i &= ~(1<<31);
/*
            if ( i % (sz*8) == 0 ) 
                t.c = 0x88888877;
            else
                t.c = 0x66666655;
*/

            if ( i % (sz*8) == 0 ) {
                t.c = 0x77888888;
                t.c2 = t.c;
            } else {
                t.c = 0x55666666;
                t.c2 = t.c;
            }

            gridlines.add( t );
        }
    }

    //
    // the two strongest lines through the world-origin
    //
    line_t a, b;
    a.u[0] = (float)x[0];
    a.u[1] = 0.f;
    a.v[0] = (float)x[1];
    a.v[1] = 0.f;
    b.u[0] = 0.f;
    b.u[1] = (float)y[0];
    b.v[0] = 0.f;
    b.v[1] = (float)y[1];
    a.c2 = b.c2 = a.c = b.c = -1;
    gridlines.add( a );
    gridlines.add( b );

/* REPLACE

    // draw
    gglBegin( GL_LINES );

    node_c<line_t> *n = gridlines.gethead();
    while( n ) {
        gglColor4f( n->val.R(), n->val.G(), n->val.B(), n->val.A() );
        gglVertex3f( n->val.u[0], n->val.u[1], 0.f );
        gglVertex3f( n->val.v[0], n->val.v[1], 0.f );

        n = n->next;
    }
    gglEnd();

*/

/*
    note: change line_t, so that the color and vert values are 
    contiguous, interleaved types that can be dumped straight into GL.  then,
    at this point in the code, instead of making all the glBegin,glColor,
    glVertex, calls, just call glColorPointer, glDrawArray and be done with it.
*/

    // there are intertwined arrays and interleaved.  intertwined is where you do it yourself,
    //  and interleaved are calls provided by GL.  this is intertwined

    gglEnableClientState( GL_VERTEX_ARRAY );
    gglEnableClientState( GL_COLOR_ARRAY );
    uint s = 2 * sizeof(float) + sizeof(unsigned int);
    gglVertexPointer( 2, GL_FLOAT, s, gridlines.data );
    gglColorPointer( 4, GL_UNSIGNED_BYTE, s, &((byte*)gridlines.data)[8] );
    gglDrawArrays( GL_LINES, 0, gridlines.length() * 2 );
}
#endif

/* OK, well, there are only ever 2 different shades of grid lines drawn.
lines at 1024 which are of normal darkness, and the special line which is 
selected in the menu.

*/

// a hack to add an asterix to the text next to the active grid level in the drop-menu
void ME_SetSubline( int n ) {
	if ( inputMode == MODE_POLY_DRAG || inputMode == MODE_POLY_STRETCH ) {
		return;
	}

	subLineResolution = n;
	menuBox_t *box = grid_col.getBoxHead();
	if ( !box )
		return;

	int count = 0;

	const char MARKER = '>';

	// unset all the boxes text
	while ( box ) {
		if ( box->text[0] != ' ' && box->text[0] != MARKER ) {
			box = box->getNext();
			continue;
		}
		box->text[0] = ' ';

		if ( count++ == n+2 ) {
			box->text[0] = MARKER;
		}

		box = box->getNext();
	}
}

void ME_IncSubline( void ) {
	if ( inputMode == MODE_POLY_DRAG || inputMode == MODE_POLY_STRETCH )
		return;
	if ( ++subLineResolution > subMax )
		subLineResolution = subMax;
	ME_SetSubline( subLineResolution );
}

void ME_DecSubline( void ) {
	if ( inputMode == MODE_POLY_DRAG || inputMode == MODE_POLY_STRETCH )
		return;
	if ( --subLineResolution < subMin )
		subLineResolution = subMin;
	ME_SetSubline( subLineResolution );
}

void ME_DrawGrid( void ) {
    if ( !drawGrid )
        return;

    gridlines.init();
    gridlines.reset_keep_mem();

	// clear
    CL_ModelViewInit2D();

    main_viewport_t *v = M_GetMainViewport();
    gglScalef( v->iratio, v->iratio, 0.f );
    gglTranslatef( -v->world.x, -v->world.y, 0.f );

    int x[2] = { (int)v->world.x, (int)v->world.x };
    int y[2] = { (int)v->world.y, (int)v->world.y };

    x[1] += (int)(v->res->width * v->ratio);
    y[1] += (int)(v->res->height * v->ratio);

    x[0] -= 1; x[1] += 1; y[0] -= 1; y[1] += 1;


	// draw default grid lines at 1024, which are persistent

	int s[2], r[2];

	// get first, left to right 
	s[0] = x[0] + 1024 & ~1023;

	// draw vertical lines
	for ( int i = s[0]; i <= x[1]; i += 1024 ) {
		line_t t;
		t.u[0] = (float)i;
        t.u[1] = (float)y[0];
        t.v[0] = (float)i;
        t.v[1] = (float)y[1];
		t.c = t.c2 = ~0;
		t.c = t.c2 = grid_color;
		gridlines.add( t );
	}
	int v_sav = s[0];

	// get first bot to top
	r[0] = y[0] + 1024 & ~1023;

	// draw horizontal lines
	for ( int i = r[0]; i <= y[1]; i += 1024 ) {
		line_t t;
		t.u[0] = (float)x[0];
        t.u[1] = (float)i;
        t.v[0] = (float)x[1];
        t.v[1] = (float)i;
		t.c = t.c2 = ~0;
		t.c = t.c2 = grid_color;
		gridlines.add( t );
	}
	int h_sav = r[ 0 ];

	// sublines
	int res = 1 << ( 3 + subLineResolution );

	// get first, left to right 
	s[0] = x[0] + res & ~(res-1);

	// draw vertical lines
	for ( int i = s[0]; i <= x[1]; i += res ) {

		// don't draw a sub-line over a persistent line
		if ( ( ( i - v_sav ) % 1024 ) == 0 )
			continue;

		line_t t;
		t.u[0] = (float)i;
        t.u[1] = (float)y[0];
        t.v[0] = (float)i;
        t.v[1] = (float)y[1];
		t.c = t.c2 = grid_color2; // NODE, arrays are read color bytes in reverse order from what you see
		gridlines.add( t );
	}

	// get first bot to top
	r[0] = y[0] + res & ~(res-1);

	// draw horizontal lines
	for ( int i = r[0]; i <= y[1]; i += res ) {

		// don't draw a sub-line over a persistent line
		if ( ( ( i - h_sav ) % 1024 ) == 0 )
			continue;

		line_t t;
		t.u[0] = (float)x[0];
        t.u[1] = (float)i;
        t.v[0] = (float)x[1];
        t.v[1] = (float)i;
		t.c = t.c2 = ~0;
		t.c = t.c2 = grid_color2;
		gridlines.add( t );
	}

#ifdef _DEBUG
	assert( sizeof(line_t) == sizeof(float) * 4 + sizeof(unsigned int) * 2 );
#endif

    gglEnableClientState( GL_VERTEX_ARRAY );
    gglEnableClientState( GL_COLOR_ARRAY );
    const uint spacing = 2 * sizeof(float) + sizeof(unsigned int) ;
    gglVertexPointer( 2, GL_FLOAT, spacing, gridlines.data );
    gglColorPointer( 4, GL_UNSIGNED_BYTE, spacing, &((byte*)gridlines.data)[2*sizeof(float)] );
    gglDrawArrays( GL_LINES, 0, gridlines.length() * 2 );

	// [0,0] origin coordinate marker
	if ( x[0] < 0.f && x[1] > 0.f && y[0] < 0.f && y[1] > 0.f ) {
		// we scaled the whole world by iratio, so that we draw in world scale
		//  but it is shrunk down to screen scale.  we want a constant 16 in
		//  screen scale.  scale it back UP to world, where gl will 
		//  automatically shrink it down again later in the pipeline.
		float halfsz = 16.0f * v->ratio;
		gglColor4f( 1.0f, 0.0f, 0.6f, 0.8f );
		GL_LINE_SQUARE( halfsz );
		gglColor4f( 1.0f, 0.0f, 0.6f, 0.4f );
		GL_SQUARE( halfsz - 1.f );
		gglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
	}
}

#if 0
moving to cl_main
void LoadTextureDir( void ) 
{

    char ** images = NULL;

    unsigned int len = 0;

    images = GetDirectoryList( "zpak/gfx", &len );

    if ( !len ) 
        return;

    char tmp[1024];

    // count how many files listed are actually loadable textures
    for ( unsigned int i = 0; i < len; i++ ) {
        if ( O_strcasestr( images[i], ".tga" ) || O_strcasestr( images[i], ".bmp" ) )
            ++total_tex;
    }

    // get t-table
    textures = (image_t **) V_Malloc( sizeof(image_t *) * total_tex );    

    // setup materials
    materials.total = 0;
    materials.mat = (material_t *) V_Malloc( sizeof(material_t) * total_tex );

    // FIXME: code that verifies supported image extensions should go in 
    //  lib_image.cpp
    int j = 0;
    int m = 0;
    for ( unsigned int i = 0; i < len; i++ ) {
        if ( O_strcasestr( images[i], ".tga" ) || O_strcasestr( images[i], ".bmp" ) ) {
            IMG_compileImageFile( images[i], &textures[j] );

            // build a texture material
            materials.mat[m].BuildTextureMaterial( images[i], textures[j] );
            ++materials.total;

            ++j; ++m;
        }
    }

    // free array of string pointers
    for ( unsigned int i = 0; i < len; i++ ) {
        V_Free( images[i] );
    }
    V_Free( images );
}
#endif

/*
extern timer_c area_timer;

void ME_DrawAreas( void ) {
	if ( area_timer.check() )
		return;

	CL_ModelViewInit2D();
    main_viewport_t *v = M_GetMainViewport();
    gglScalef( v->iratio, v->iratio, 0.f );
    gglTranslatef( -v->world.x, -v->world.y, 0.f );
  	gglDisable( GL_TEXTURE_2D );

	node_c<Area_t*> *n = me_areas.gethead();
	while ( n ) {
		Area_t *a = n->val;
		float v[4] = { a->p1[0], a->p1[1], a->p2[0], a->p2[1] };
		gglColor4f( 0.8f, 0.0f, 0.8f, 0.6f );
		GL_DRAW_QUAD_4V( v );
		gglColor4f( 0.4f, 0.0f, 0.4f, 0.6f );
		GL_DRAW_OUTLINE_4V( v );
		n = n->next;
	}
}
*/


void ME_DrawBackground( void ) {
	node_c<mapTile_t *> *node = tiles->gethead();
	if ( !node )
		return;

	CL_ModelViewInit2D();

    main_viewport_t *v = M_GetMainViewport();
    gglScalef( v->iratio, v->iratio, 0.f );
    gglTranslatef( -v->world.x, -v->world.y, 0.f );

  	gglEnable( GL_TEXTURE_2D );
	gglTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

	while ( node ) {
		if ( !node->val->background ) {
			node = node->next;
			continue;
		}

        float v[8];

        node->val->poly.toOBB( v );

		material_t *m = node->val->mat;

		switch ( m->type ) {
		case MTL_COLOR:
			gglDisable( GL_TEXTURE_2D );
			gglColor4fv( node->val->mat->color );
			break;
		case MTL_TEXTURE_STATIC:
			if ( !node->val->mat->img )
				goto do_default;
			gglEnable( GL_TEXTURE_2D );
			gglBindTexture( GL_TEXTURE_2D, node->val->mat->img->texhandle );
			break;
		case MTL_COLORMASK:
			if ( !node->val->mat->img )
				goto do_default;
			gglEnable( GL_TEXTURE_2D );
			gglColor4fv( node->val->mat->color );
			gglBindTexture( GL_TEXTURE_2D, node->val->mat->img->texhandle );
			break;
		case MTL_MASKED_TEXTURE:
			if ( !node->val->mat->img )
				goto do_default;
			gglEnable( GL_TEXTURE_2D );
			gglBindTexture( GL_TEXTURE_2D, node->val->mat->img->texhandle );
			gglBegin( GL_QUADS );
		    gglTexCoord2f( m->s[0], m->t[0] );
	        gglVertex3f( v[0], v[1], 0.f );
		    gglTexCoord2f( m->s[1], m->t[1] );
	        gglVertex3f( v[2], v[3], 0.f );
		    gglTexCoord2f( m->s[2], m->t[2] );
	        gglVertex3f( v[4], v[5], 0.f );
		    gglTexCoord2f( m->s[3], m->t[3] );
	        gglVertex3f( v[6], v[7], 0.f );
		    gglEnd();
			if ( !node->val->mat->mask )
				goto do_default;
			gglBindTexture( GL_TEXTURE_2D, node->val->mat->mask->texhandle );
			break;

		default:
do_default:
			// FIXME: should have a "NoTex" pattern to put on unset tiles
			gglDisable( GL_TEXTURE_2D );
			gglColor4f( 0.2f, 0.4f, 0.6f, 1.0f );
			break;
		}

		gglBegin( GL_QUADS );

	    gglTexCoord2f( m->s[0], m->t[0] );
        gglVertex3f( v[0], v[1], 0.f );

	    gglTexCoord2f( m->s[1], m->t[1] );
        gglVertex3f( v[2], v[3], 0.f );
	
	    gglTexCoord2f( m->s[2], m->t[2] );
        gglVertex3f( v[4], v[5], 0.f );
	
	    gglTexCoord2f( m->s[3], m->t[3] );
        gglVertex3f( v[6], v[7], 0.f );
	
	    gglEnd();

		node = node->next;
	}

	gglDisable( GL_TEXTURE_2D );
}

void ME_DrawForeground( void ) {
/*
foreground might be like when you want to put a tinted transparency over the 
entire scene.  say to drench the scene in pinkish blood for a second, or
yellow, or green, to give the look of seeing through a toxic russian radiation
suit helmet or something.
*/
}

void ME_DrawCollisionModels( void ) {

}


void ME_InitGvars( void ) {
	me_activeMap = Gvar_Get( "me_activeMap", "", 0 );
	me_newTileSize = Gvar_Get( "me_newTileSize", "512", 0 );
	me_selectMargin = Gvar_Get( "me_selectMargin", "4.0", 0 );
	me_bgColor = Gvar_Get( "me_bgColor", "0.0 0.0 0.0 0.0", 0 );
	me_gridColor = Gvar_Get( "me_gridColor", "0.85 0.85 0.85 0.6", 0 );
	me_gridColor2 = Gvar_Get( "me_gridColor2", "0.5 0.5 0.5 0.6", 0 );
}

void ME_Init( void ) {
   	me_lastframe = me_start = ME_Tics();

   	me_mouse.start();
	
	tiles = new tilelist_t;
	tiles->init();


	selected.init();
	copied.init();
	copyTmp.init();

	cmdque.init();

	ME_CreateMenus();

	floating.init();

	me_areas.init();
	me_subAreas.init();

	me_ents = new entlist_t;
	me_ents->init();
	entSelected.init();
	entCopy.init();
	entTmp.init();

	// NOTE: you can't init palettes until after tiles & me_ents
	palette.init();
}

// returns all memory held by mapedit code module
static void ME_TrashLocalData( void ) {
	// FIXME: what about objects allocated to pointer, then stored in lists
	//  of pointers?  tiles?  
	
	// cmdque
	cmdque.destroy();

	// individual tiles & tiles list
	if ( tiles ) {
		tiles->destroy();
		V_Free( tiles );
		tiles = NULL;
	}

	// entities & entities list
	if ( me_ents ) {
		me_ents->destroy();
		V_Free( me_ents );
		me_ents = NULL;
	}


	// individual palettes & palettes list
	palette.destroy();

	// copied tiles list
	copied.destroy();

	// copyTmp tiles list
	copyTmp.destroy();

	// selected tiles list
	selected.destroy();

	// areas & areas list
	me_areas.destroy();

	// subAreas & subAreas list
	me_subAreas.destroy();

	// floating: using floating elsewhere

	// gridlines
	gridlines.destroy();

	// quadlist
	quadlist.destroy();
	
	//menus
	menu.destroy();


	entSelected.destroy();
	entCopy.destroy();

	// console: using elsewhere
}

bool late_init_not_done = true;
void ME_LateInit( void ) {
	if ( late_init_not_done ) {
		float c[4];
		color.StringToColor4v( c, me_bgColor->string() );
	    gglClearColor( c[0], c[1], c[2], c[3] );

		// bg and grid colors only initialized at startup, changing during 
		//  run won't change color.  must restart or do vid_restart
		color.StringToUInt( &grid_color, me_gridColor->string() );
		color.StringToUInt( &grid_color2, me_gridColor2->string() );

		ent_notex = materials.FindByName( "gfx/ent_notex.tga" );

		late_init_not_done = false;
	}
}


//extern "C" void F_Printf( int, int, int, const char *, ... );
//void F_Printf( int, int, int, const char *, ... );

void ME_DebugInfo( void ) {
	gglColor4f( 1.0f, 1.0f, 1.0f, 0.6f );
	CL_DrawInt( 6, 4, 10, 10, me_mouse.z, 10 );

	static int last = 0, now = 0;

	now = Com_Millisecond();

	static int frames = 0;
	++frames;

	static float fps = 0.f;
	if ( now - last > 1000 ) {

		int diff = now - last;
		last = now;

		fps = (float)frames / (float)diff * 1000.f;
		frames = 0;
	}

	F_Printf( 30, 15, 1, "fps: %.0f", fps );
}

void ME_DrawDragSelect( void ) {
	if ( inputMode != MODE_DRAG_SELECT && inputMode != MODE_SPECIAL_SELECT )
		return;
	float x = dragSelect.mx0;
	float y = dragSelect.my0;
	float x2 = dragSelect.x0;
	float y2 = dragSelect.y0;

	float g = 0.85f;
	gglColor4f( g, g, g, 1.0f );
	gglBegin( GL_LINE_STRIP );
	gglVertex2f( x, y );
	gglVertex2f( x2, y );
	gglVertex2f( x2, y2 );
	gglVertex2f( x, y2 );
	gglVertex2f( x, y );
	gglEnd();
}

void ME_DrawConsole( void ) {
    gglColor4f( 0.2f, 1.0f, 0.2f, 1.0f );
    CL_ModelViewInit2D();
	console.Draw();
}

void ME_DrawFloatingText( void ) {
    CL_ModelViewInit2D();
	floating.Draw( tiles, me_ents, &me_areas );
}

/*
====================
 ME_DestroyMapEditLocal

 frees all memory used by mapedit module that is not needed by the rest of
  the program
====================
*/
void ME_DestroyMapEditLocal( void ) {
	ME_TrashLocalData();
}

extern void ME_DrawAreas( void ) ;

void ME_DrawFrame( void ) {
	ME_LateInit();
    gglClear( GL_COLOR_BUFFER_BIT );
    gglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

    // if theres a Tic, update the mouse texture location and set local
    //  mouse state.  this is only the map editor, it doesn't have to be
    //  perfect
    if ( me_lastframe != me_start ) {
		// certain pop-up boxes can block input from reaching the rest of the map editor
		if ( !menu.blocking() ) {
			ME_MakeCommand();
			ME_ExecuteCommands();
		}
    }

    me_lastframe = ME_Tics();
	

    ME_DrawBackground();

	ME_DrawGrid();

    gglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

    ME_DrawTiles();

	ME_DrawEntities();

    ME_DrawForeground();

	ME_DrawAreas();

    ME_DrawMenu();

	ME_DrawDragSelect();

	ME_DrawCollisionModels();

	ME_DrawFloatingText();

	ME_DrawConsole();
}

