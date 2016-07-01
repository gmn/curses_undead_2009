
/****************************************************************************** 
 *
 * cl_draw.cpp -- helper functions called by cg_draw, cg_scoreboard, cg_info, 
 *
 *****************************************************************************/
 
#include "cl_local.h"
#include "../common/com_geometry.h"
#include "../renderer/r_ggl.h"

// simplistic square macro
#ifndef GL_SQUARE
#define GL_SQUARE(x,y,z,s) {\
		gglBegin(GL_QUADS); \
        gglVertex3f((GLfloat)(x),(GLfloat)(y)+(s),(GLfloat)(z)); \
        gglVertex3f((GLfloat)(x)+(s),(GLfloat)(y)+(s),(GLfloat)(z)); \
        gglVertex3f((GLfloat)(x)+(s),(GLfloat)(y),(GLfloat)(z)); \
        gglVertex3f((GLfloat)(x),(GLfloat)(y),(GLfloat)(z)); \
		gglEnd(); }
#endif


// draw a while outline around a poly
#define POLY_OUTLINE(x,y,w,h) { \
	gglPushAttrib( GL_CURRENT_BIT ); \
	gglColor3f( 1.0f, 1.0f, 1.0f ); \
	gglBegin( GL_LINE_STRIP ); \
	gglVertex3i( x, y, 0 ); \
	gglVertex3i( x, y+h, 0 ); \
	gglVertex3i( x+w, y+h, 0 ); \
	gglVertex3i( x+w, y, 0 ); \
	gglVertex3i( x, y, 0 ); \
	gglEnd(); \
	gglPopAttrib(); }





/*
================
CL_AdjustFrom640

Adjusted for resolution and screen aspect ratio
================
*/
void CL_AdjustFrom640( float *x, float *y, float *w, float *h ) {
	// scale for screen sizes
	*x *= cls.screenXscale;
	*y *= cls.screenYscale;
	*w *= cls.screenXscale;
	*h *= cls.screenYscale;
}




#if 0

/*
================
CL_FillRect

Coordinates are 640*480 virtual values
=================
*/
void CL_FillRect( float x, float y, float width, float height, const float *color ) {
	trap_R_SetColor( color );

	CL_AdjustFrom640( &x, &y, &width, &height );
	trap_R_DrawStretchPic( x, y, width, height, 0, 0, 0, 0, cgs.media.whiteShader );

	trap_R_SetColor( NULL );
}

/*
================
CL_DrawSides

Coords are virtual 640x480
================
*/
void CL_DrawSides(float x, float y, float w, float h, float size) {
	CL_AdjustFrom640( &x, &y, &w, &h );
	size *= cgs.screenXScale;
	trap_R_DrawStretchPic( x, y, size, h, 0, 0, 0, 0, cgs.media.whiteShader );
	trap_R_DrawStretchPic( x + w - size, y, size, h, 0, 0, 0, 0, cgs.media.whiteShader );
}

void CL_DrawTopBottom(float x, float y, float w, float h, float size) {
	CL_AdjustFrom640( &x, &y, &w, &h );
	size *= cgs.screenYScale;
	trap_R_DrawStretchPic( x, y, w, size, 0, 0, 0, 0, cgs.media.whiteShader );
	trap_R_DrawStretchPic( x, y + h - size, w, size, 0, 0, 0, 0, cgs.media.whiteShader );
}
/*
================
UI_DrawRect

Coordinates are 640*480 virtual values
=================
*/
void CL_DrawRect( float x, float y, float width, float height, float size, const float *color ) {
	trap_R_SetColor( color );

  CL_DrawTopBottom(x, y, width, height, size);
  CL_DrawSides(x, y, width, height, size);

	trap_R_SetColor( NULL );
}



/*
================
CL_DrawPic

Coordinates are 640*480 virtual values
=================
*/
void CL_DrawPic( float x, float y, float width, float height, qhandle_t hShader ) {
	CL_AdjustFrom640( &x, &y, &width, &height );
	trap_R_DrawStretchPic( x, y, width, height, 0, 0, 1, 1, hShader );
}

#endif

void CL_ModelViewInit2D( void ) {
	gglMatrixMode( GL_MODELVIEW );
    gglLoadIdentity ();
    gglRasterPos3i( 0, 0, 0 );

	// from OpenGL faq: setup Modelview Matrix for exact pixelization
	glTranslatef ( 0.375, 0.375, 0. );
}


// ok, virtual size is 640x480, and multipliers are used when drawing 
//  stretch pic for any size that is 4/3.  work out something smarter later.
//  just get this fucker on screen for now.
void CL_InitGL2D ( void ) 
{
	main_viewport_t *v = M_GetMainViewport();

	float w = v->res->width;
	float h = v->res->height;

    // viewport
	gglViewport( 0, 0, w, h );

    // scissor
	gglEnable( GL_SCISSOR_TEST );
	gglScissor( 0, 0, w , 0.75 * h );
	gglBegin( GL_QUADS ); gglVertex3f( 0,0,0 ); gglVertex3f( 0,h,0 );
	gglVertex3f( w,h,0 ); gglVertex3f( w,0,0 ); gglEnd();
	// enable & disable scissor once so GL knows to allocate the buffer 
	gglDisable( GL_SCISSOR_TEST );

    // projection
	gglMatrixMode( GL_PROJECTION );
    gglLoadIdentity ();
	
    gglOrtho( 0, w, 0, h, v->near_plane, v->far_plane );


	// model view
	CL_ModelViewInit2D();


    // color
    gglClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
    gglShadeModel( GL_FLAT );

    // depth
    gglDisable( GL_DEPTH_TEST );
    gglDepthFunc( GL_LESS );

    // blend
    gglEnable( GL_BLEND );
    gglBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    // stencil

    // alpha
	gglDisable( GL_ALPHA_TEST );
	gglAlphaFunc( GL_LESS, 1.0 );

    // smoothing
	/*
    glHint( GL_LINE_SMOOTH_HINT, GL_NICEST );
    glHint( GL_POLYGON_SMOOTH_HINT, GL_NICEST );
    glHint( GL_POINT_SMOOTH_HINT, GL_NICEST );
	glEnable( GL_POINT_SMOOTH );
	glEnable( GL_LINE_SMOOTH );
	*/
}

void CL_DrawStretchPic( float x, float y, float w, float h, 
					  float s1, float t1, float s2, float t2, 
                      image_t *img ) {

    gglBindTexture( GL_TEXTURE_2D, img->texhandle );

  	gglEnable( GL_TEXTURE_2D );
	gglTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

	gglBegin( GL_QUADS );

	gglTexCoord2f( s1, t1 );
    gglVertex3f( x, y, 0 );

	gglTexCoord2f( s1, t2 );
    gglVertex3f( x, y+h, 0 );
	
	gglTexCoord2f( s2, t2 );
    gglVertex3f( x+w, y+h, 0 );
	
	gglTexCoord2f( s2, t1 );
    gglVertex3f( x+w, y, 0 );
	
	gglEnd();

	gglDisable( GL_TEXTURE_2D );
}

image_t *fontImage = NULL;
image_t *fontImage2 = NULL;

/*
===============
CL_DrawChar

Coordinates and size in 640*480 virtual screen size
===============
* /
void CL_DrawChar( int x, int y, int width, int height, int ch ) {
	int row, col;
	float frow, fcol;
	float size, vsize;
	float	ax, ay, aw, ah;

	ch &= 255;

	if ( ch == ' ' ) {
		return;
	}

	ax = (float) x;
	ay = (float) y;
	aw = (float) width;
	ah = (float) height;
	CL_AdjustFrom640( &ax, &ay, &aw, &ah );


	// drop the lower-case letters down so that their bottoms line up with Upper and everything else
	if ( ch >= 97 && ch <= 122 ) {
		ay -= 0.125f * (float)height;
	}

	if ( ch > 126 ) { // only goes this far
		return;
	}

	int ind = ch - 33;

	row = ind / 16;
	col = ind % 16;

	frow = 0.9375f-row*0.0625f;
	fcol = col*0.0625f;
	size = 0.0625f;
	vsize = 0.058594f; // cut off the top 1/16th because the font texture isnt aligned perfect

	CL_DrawStretchPic( ax, ay, aw, ah,
					   fcol, frow, 
					   fcol + size, frow + vsize, 
					   fontImage );
} */

/* the old way, depends on how the char texture is laid out */
void CL_DrawChar( int x, int y, int width, int height, int ch ) {
	int row, col;
	float frow, fcol;
	float size, vsize;
	float	ax, ay, aw, ah;

	ch &= 255;

	if ( ch == ' ' ) {
		return;
	}

	ax = (float) x;
	ay = (float) y;
	aw = (float) width;
	ah = (float) height;
	CL_AdjustFrom640( &ax, &ay, &aw, &ah );

	// i already computed 
	// dont adjust location
	ay = y; ax = x;
	
	if ( ch == 64 ) { // '@' isnt a valid character
		return;
	} 
	if ( ch > 126 ) { // only goes this far
		return;
	}
	if ( ch > 64 ) { 
		--ch;
	} 
	int ind = ch - 32;

	row = ind / 16;
	col = ind % 16;

	frow = 0.9375f-row*0.0625f;
	fcol = col*0.0625f;
	size = 0.0625f;
	vsize = 0.058594f; // cut off the top 1/16th because the font texture isnt aligned perfect

	CL_DrawStretchPic( ax, ay, aw, ah,
					   fcol, frow, 
					   fcol + size, frow + vsize, 
					   fontImage2 );
}


int CL_DrawInt( int x, int y, int w, int h, int number, int spacing ) {
	int printchar[20];
	int plen = 0;

	if ( !number ) {
		CL_DrawChar( x, y, w, h, '0' );
		return x;
	}
	
	int abs = number & 0x7FFFFFFF;

	if ( number >> 31 ) {
		CL_DrawChar( x, y, w, h, '-' );
		x += spacing;
		abs = number * -1;
	}

	while ( abs > 0 ) {
		int digit = abs % 10;
		printchar[plen++] = '0' + digit;
		abs /= 10;
	}

	while ( --plen >= 0 ) {
		CL_DrawChar( x, y, w, h, printchar[plen] );
		x += spacing;
	}

	return x;
}

/*
================
 CL_DrawString
 x,y : start point
 w,h : dimension of character
 string : 
 spacing : optional spacing adjustment
================
*/
//void CL_DrawString( int x, int y, int w, int h, const char * string, int spacing =0 ) {

//extern "C" void F_Printf( int x, int y, int fnum, const char *fmt, ... );

void CL_DrawString( int x, int y, int w, int h, const char * string, int wpad, int hpad ) {
	int S = w + wpad;
	int i = 0, l = 0;
	int Px = x, Py = y;


//	F_Printf( x, y+h, FONT_VERA_MONO, "%s", string );
//	F_Printf( x, y+h, FONT_VERA_MONO_BOLD, "%s", string );
//	F_Printf( x, y+h, FONT_VERA_BOLD, "%s", string );
//	F_Printf( x, y+h, FONT_VERA, "%s", string );
//	return;


	while ( string[i] ) {
		CL_DrawChar( Px, Py, w, h, string[i] );
		if ( string[i] == '\n' ) {
			Py -= h + hpad;
			Px = x;
		} else {
			Px += S;
		}
		++i;
	}
}

void CL_BlendDraw( int x, int y, int h, int w, image_t *img ) {
}

void CL_ClipDraw( int x, int y, int h, int w, image_t *img ) {
}

void CL_BlendClipDraw( int x, int y, int h, int w, image_t *img ) {
}

void CL_InitGL( void ) {
	CL_InitGL2D();
}

void CL_Draw_Init( void ) {
	// load font image to cls.media.fontImage
	//IMG_compileImageFile ( "zpak/gfx/mainchar.tga" , &fontImage );
	//IMG_compileImageFile ( "zpak/gfx/font1.tga" , &fontImage2 );
	//IMG_compileImageFile ( "zpak/gfx/uberfont.tga" , &fontImage );

	material_t *tmp;
	tmp = materials.FindByName( "gfx/font1.tga" );
	if ( tmp ) {
		fontImage2 = tmp->img;
	}
	tmp = materials.FindByName( "gfx/uberfont.tga" );
	if ( tmp ) {
		fontImage = tmp->img;
	}
	


	CL_InitGL2D();
}



void CL_TextTest( void ) {
	gglLoadIdentity ();             
	gglClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    // yellow
    gglColor4f( 1.0, 1.0, 0.0, 1.0 ); 
	CL_DrawStretchPic( 1, 257, 128, 128, 0.f, 0.5f, 1, 1, fontImage );
	POLY_OUTLINE( 1, 257, 128, 128 );

	// green
	gglColor4f( 0.0, 1.0, 0.0, 1.0 );
	CL_DrawStretchPic( 0, 0, 256, 256, 0, 0, 1, 1, fontImage );
	POLY_OUTLINE( 0, 0, 256, 256 );

	// purple
	gglColor4f( 1.0f, 0.0f, 1.0f, 1.0f );


	int B = 20;

	CL_DrawChar( 20+1*B, 400, B, B, 'H' );
	CL_DrawChar( 20+2*B, 400, B, B, 'a' );
	CL_DrawChar( 20+3*B, 400, B, B, 'p' );
	CL_DrawChar( 20+4*B, 400, B, B, 'p' );
	CL_DrawChar( 20+5*B, 400, B, B, 'y' );

	gglColor4f( 0.0, 1.0, 0.0, 1.0 );
	CL_DrawChar( 20+7*B, 400, B, B, 'N' );
	CL_DrawChar( 20+8*B, 400, B, B, 'e' );
	CL_DrawChar( 20+9*B, 400, B, B, 'w' );

    gglColor4f( 1.0, 1.0, 0.0, 1.0 ); 
	CL_DrawChar( 20+11*B, 400, B, B, 'Y' );
	CL_DrawChar( 20+12*B, 400, B, B, 'e' );
	CL_DrawChar( 20+13*B, 400, B, B, 'a' );
	CL_DrawChar( 20+14*B, 400, B, B, 'r' );
	CL_DrawChar( 20+15*B, 400, B, B, 's' );
	CL_DrawChar( 20+16*B, 400, B, B, '!' );

	gglColor4f( 1.0f, 0.0f, 0.0f, 1.0f );
	int i, x, y;
	for ( i = 33; i < 127; i++ ) {
		x = i - 33;

		y = x / 22;
		x %= 22;

		CL_DrawChar( 180+x*B, 360-y*20, B, B, i );
		POLY_OUTLINE( 180+x*B, 360-y*20, B, B );
	}	

	gglColor4f( 0.15f, 0.3f, 0.92f, 1.0f );
	CL_DrawChar( 275, 10, 210, 210, 'g' ); 
	POLY_OUTLINE( 275, 10, 210, 210 ); 
}



void CL_DrawFrame( void ) {
	if ( com_texttest->integer() ) {
		CL_TextTest();
	}
}

// main game draw frame
void G_DrawFrame( void ) {
}
