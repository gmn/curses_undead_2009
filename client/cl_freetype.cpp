
//

#include <windows.h>



#include <ft2build.h>
#include <freetype/freetype.h>
#include <freetype/ftglyph.h>
#include <freetype/ftoutln.h>
#include <freetype/fttrigon.h>

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include "../renderer/r_ggl.h"

#include "../common/common.h" 
#include "../common/com_vmem.h"
#include "../common/com_object.h"

// font textures are created at F_POINT_SZ which should be 14 or larger to 
//  get a decent resolution.  Then default scale is the default size that tex
//  will be drawn at, unless specified otherwise.

static const float DEFAULT_SCALE = 11.0f / F_POINT_SZ;

static float scale = DEFAULT_SCALE;

void F_SetScale( float f ) { 
	scale = f; 
}
void F_ScaleDefault( void ) { 
	scale = DEFAULT_SCALE; 
}


// linkage wrapper for freetype library calls
extern "C" {

static bool is_init = false;



byte * pix_buffer = NULL;
#define PIX_BUFFER_SIZE 50000

#define TOTAL_FONTS 10
const int total_fonts = TOTAL_FONTS;
int font_dlist[TOTAL_FONTS];






struct glyph_t {
	int	top, rows, width, height, left, advance, letter;
	unsigned int texhandle;
	int tex_width, tex_height;
};

class fontCont_s : public Allocator_t {
private:
	static const unsigned int _TOTAL_GLYPH = 96;
	static const unsigned int _NAME_LEN = 128;
public:
	glyph_t 		glyph[ _TOTAL_GLYPH ];
	int 			sz;
	char 			name[ _NAME_LEN ];

	void reset( void ) {
		name[ 0 ] = '\0';
		memset( glyph, 0, sizeof(glyph) );
		sz = 0;
	}
	fontCont_s( void ) {
		reset();
	}
	fontCont_s( const char *str, int h ) {
		setName( str );
		reset();
		sz = h;
	}
	void setName( const char *n ) {
		strncpy( this->name, n, _NAME_LEN );
		this->name[ _NAME_LEN - 1 ] = '\0';
	}
	void copyTexhandles( unsigned int * texhandle ) {
		for ( unsigned int i = 0; i < _TOTAL_GLYPH; i++ ) {
			glyph[ i ].texhandle = texhandle[ i ];
		}
	}
};


buffer_c<fontCont_s *> fonts;



void F_MakeCharTexture( FT_Face face, int ch, glyph_t *g ) {
    FT_Glyph 			glyph;
    FT_BitmapGlyph 		bitmap_glyph;
	FT_Bitmap *			bitmap; 
    int 				width, height, i, j;
    float 				x, y;

	// Load the Glyph for our character.
	if ( FT_Load_Glyph( face, FT_Get_Char_Index( face, ch ), FT_LOAD_DEFAULT ) )
		Err_Warning( "FT_Load_Glyph failed" );

	// Move the face's glyph into a Glyph object.
    if ( FT_Get_Glyph( face->glyph, &glyph ) )
		Err_Warning( "FT_Get_Glyph failed" );

	// Convert the glyph to a bitmap.
	FT_Glyph_To_Bitmap( &glyph, ft_render_mode_normal, 0, 1 );
    bitmap_glyph = (FT_BitmapGlyph) glyph;

	bitmap = &bitmap_glyph->bitmap;

    width = 1;
    while ( width < bitmap->width ) 
		width <<= 1;

    height = 1;
    while ( height < bitmap->rows ) 
		height <<= 1;
    
	for ( j = 0; j < height; j++ ) {
		for ( i = 0; i < width; i++ ) {
			Assert( 2*(i+j*width)+1 < PIX_BUFFER_SIZE );
			pix_buffer[ 2*(i+j*width) ] = 
			pix_buffer[ 2*(i+j*width)+1 ] = 
				( i >= bitmap->width || j >= bitmap->rows ) ? 
					0 : bitmap->buffer[ j * bitmap->width + i ];
		}
	}

	// Now we just setup some texture paramaters.
    gglBindTexture   ( GL_TEXTURE_2D, g->texhandle );

	// this turns off bilinear and trilinear filtering
//	gglTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
//	gglTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	// bilinear filtering
	gglTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	gglTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ); 

	// Here we actually create the texture itself, notice
	// that we are using GL_LUMINANCE_ALPHA to indicate that
	// we are using 2 channel data.
    gglTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, width, height,
		  0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, pix_buffer );

	// save these 
	g->top 		= bitmap_glyph->top;
	g->rows		= bitmap->rows;
	g->width	= bitmap->width;
	g->left		= bitmap_glyph->left;
	g->advance 	= face->glyph->advance.x >> 6; // convert from 64ths of a pixel
	g->letter 	= ch;
	g->tex_width = width;
	g->tex_height = height;
}

void F_MakeFontTextures( FT_Face face, fontCont_s *fc_p ) {
	for ( int ch = 32, b = 0; ch < 128; ch++, b++ ) {
		F_MakeCharTexture( face, ch, &fc_p->glyph[ b ] ); 
	}
}

/*
====================
 F_MakeFontDisplayList
====================
*/
void F_MakeFontDisplaylist( FT_Face face, char ch, int list_base, GLuint * tex_base )
{
    FT_Glyph 			glyph;
    FT_BitmapGlyph 		bitmap_glyph;
	FT_Bitmap *			bitmap; 
    int 				width, height, i, j;
    float 				tx, ty;


	//Load the Glyph for our character.
	if ( FT_Load_Glyph( face, FT_Get_Char_Index( face, ch ), FT_LOAD_DEFAULT ) )
		Err_Warning( "FT_Load_Glyph failed" );

	//Move the face's glyph into a Glyph object.
    if ( FT_Get_Glyph( face->glyph, &glyph ) )
		Err_Warning( "FT_Get_Glyph failed" );

	//Convert the glyph to a bitmap.
	FT_Glyph_To_Bitmap( &glyph, ft_render_mode_normal, 0, 1 );
    bitmap_glyph = (FT_BitmapGlyph) glyph;

	bitmap = &bitmap_glyph->bitmap;

    width = 1;
    while ( width < bitmap->width ) 
		width <<= 1;

    height = 1;
    while ( height < bitmap->rows ) 
		height <<= 1;
    
	for ( j = 0; j < height; j++ ) {
		for ( i = 0; i < width; i++ ) {
			Assert( 2*(i+j*width)+1 < PIX_BUFFER_SIZE );
			pix_buffer[ 2*(i+j*width) ] = 
			pix_buffer[ 2*(i+j*width)+1 ] = 
				( i >= bitmap->width || j >= bitmap->rows ) ? 
					0 : bitmap->buffer[ j * bitmap->width + i ];
		}
	}

	//Now we just setup some texture paramaters.
    gglBindTexture   ( GL_TEXTURE_2D, tex_base[ ch - 32 ] );
/*
	gglTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	gglTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ); */

	// this combination turns off bilinear and trilinear filtering
	gglTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	gglTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );

	//Here we actually create the texture itself, notice
	//that we are using GL_LUMINANCE_ALPHA to indicate that
	//we are using 2 channel data.
    gglTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, width, height,
		  0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, pix_buffer );

	//
	// create a display list
	//
	gglNewList( list_base + ch, GL_COMPILE );

	gglBindTexture( GL_TEXTURE_2D, tex_base[ ch - 32 ] );

	//first we need to move over a little so that
	//the character has the right amount of space
	//between it and the one before it.
	gglTranslatef( (GLfloat)bitmap_glyph->left, 0, 0 );

	//Now we move down a little in the case that the
	//bitmap extends past the bottom of the line 
	//(this is only true for characters like 'g' or 'y'.
	gglPushMatrix();
	gglTranslatef( 0, (GLfloat)(bitmap_glyph->top - bitmap->rows), 0 );

	//Now we need to account for the fact that many of
	//our textures are filled with empty padding space.
	//We figure what portion of the texture is used by 
	//the actual character and store that information in 
	//the x and y variables, then when we draw the
	//quad, we will only reference the parts of the texture
	//that we contain the character itself.
	tx = (float)bitmap->width / (float)width;
	ty = (float)bitmap->rows  / (float)height;

	//Here we draw the texturemaped quads.
	//The bitmap that we got from FreeType was not 
	//oriented quite like we would like it to be,
	//so we need to link the texture to the quad
	//so that the result will be properly aligned.

	// handle TAB char ( '\t', '\n', '\r\n', ' ' ) specially


	gglBegin(GL_QUADS);
	gglTexCoord2f( 0.f, 0.f ); 
	gglVertex2f( 0.f, bitmap->rows );

	gglTexCoord2f( 0.f, ty ); 
	gglVertex2f( 0.f, 0.f );

	gglTexCoord2f( tx, ty ); 
	gglVertex2f( (GLfloat)bitmap->width, 0.f );

	gglTexCoord2f( tx, 0.f ); 
	gglVertex2f( (GLfloat)bitmap->width, (GLfloat)bitmap->rows );
	gglEnd();

	gglPopMatrix();
	gglTranslatef((GLfloat)(face->glyph->advance.x >> 6), 0, 0);

	//increment the raster position as if we were a bitmap font.
	//(only needed if you want to calculate text length)
	gglBitmap(0,0,0,0,face->glyph->advance.x >> 6,0,NULL);

	//Finnish the display list
	//
	gglEndList();
}


/*
====================
 F_InitOneFont

    returns 1 on success, 0 on failure
====================
*/
int F_InitOneFont( int h, int *dlist, const char *path )
{
    FT_Library  	library;
    FT_Face 		face;

    int 			i, err; 
	const int 		h64 = h * 64;
	GLuint 			textures[ 96 ];

    if ( FT_Init_FreeType( &library ) ) {
        //Err_Warning( "unable to init Freetype!" );
        Error( "Unable to init Freetype!" ); // fatal error
        return 0;
    }

    err = FT_New_Face ( library, path, 0, &face );

    if ( FT_Err_Unknown_File_Format == err ) {
        // the font file could be opened and read by appears
        //  that its format is unsupported
        Err_Warning( "unknown font file format" );
		return 0;
    } else if ( err ) {
        // another error code means that the font file could
        //  not be opened or read, or is just broken, or peice of shit.
        Err_Warning( "couldn't open freetype font.  Code: %d", err );
        return 0;
    }

	// Freetype measures font size in terms of 1/64th pixel.  
	FT_Set_Char_Size( face, h64, h64, 96, 96 );

	fontCont_s *fc_p = new fontCont_s( path, h );

	// generate the texnums, one for each font character
	gglGenTextures( 96, textures );

	fc_p->copyTexhandles( textures );

	// make 1 ggl texture for each character
	F_MakeFontTextures( face, fc_p );

	// add to the gglobal list
	fonts.add( fc_p );

	FT_Done_Face( face );
	FT_Done_FreeType( library );

    return 1;
}


/*
====================
 F_InitFreetype

 startup the whole shabang
====================
*/
void F_InitFreetype(void) 
{
	// gglobal inits
    pix_buffer = (byte * ) V_Malloc( PIX_BUFFER_SIZE );
    memset ( pix_buffer, 0, PIX_BUFFER_SIZE );

	fonts.init( 32 );

    int t = 0; 

	// init the separate fonts
    t |= F_InitOneFont( F_POINT_SZ, &font_dlist[0], "zpak/gfx/font/Vera.ttf" );
    t |= F_InitOneFont( F_POINT_SZ, &font_dlist[1], "zpak/gfx/font/VeraBd.ttf" );
    t |= F_InitOneFont( F_POINT_SZ, &font_dlist[2], "zpak/gfx/font/VeraBI.ttf" );
    t |= F_InitOneFont( F_POINT_SZ, &font_dlist[3], "zpak/gfx/font/VeraIt.ttf" );
    t |= F_InitOneFont( F_POINT_SZ, &font_dlist[4], "zpak/gfx/font/VeraMoBd.ttf" );
    t |= F_InitOneFont( F_POINT_SZ, &font_dlist[5], "zpak/gfx/font/VeraMoBI.ttf" );
    t |= F_InitOneFont( F_POINT_SZ, &font_dlist[6], "zpak/gfx/font/VeraMoIt.ttf" );
    t |= F_InitOneFont( F_POINT_SZ, &font_dlist[7], "zpak/gfx/font/VeraMono.ttf" );
    t |= F_InitOneFont( F_POINT_SZ, &font_dlist[8], "zpak/gfx/font/VeraSe.ttf" );
    t |= F_InitOneFont( F_POINT_SZ, &font_dlist[9], "zpak/gfx/font/VeraSeBd.ttf" );

    if ( !t ) {
        Error( "Missing Fonts! Can not execute!" ); 
    }

	V_Free( pix_buffer );
}

} /* extern "C" */


#define TEXT_SIZE 8000
/*
==================================
  F_Printf (fontnum, text)

==================================
 */
#if 0
float F_Printf( int x, int y, int fnum, const char *fmt, ... )
{
    char text[ TEXT_SIZE ];
    va_list ap;
	float modelview_matrix[16];	
    size_t sz;

/*
    float r, g, b, a;
	r = ( color >> 24 ) / 255.0f;
	g = ( ( color >> 16 ) & 0xFF ) / 255.0f;
	b = ( ( color >> 8  ) & 0xFF ) / 255.0f;
	a = ( color & 255 ) / 255.0f; 
*/
	if ( !is_init ) {
		F_InitFreetype();
		is_init = true;
	}


    memset ( text, 0, sizeof(text) );

    if ( fmt == NULL ) {
        *text = 0;
    } else {
        va_start( ap, fmt );
        vsprintf( text, fmt, ap );
        va_end( ap );
    }

    gglPushAttrib( GL_ALL_ATTRIB_BITS );

	gglMatrixMode( GL_MODELVIEW );
	gglDisable( GL_LIGHTING );
	gglEnable( GL_TEXTURE_2D );
    //gglColor4f( r, g, b, a ); // leave color with the caller

	Assert( fnum < TOTAL_FONTS );
    if ( fnum >= TOTAL_FONTS ) 
        fnum = 0; 

	gglListBase( font_dlist[fnum] );

	// save the modelview on starting
	gglGetFloatv( GL_MODELVIEW_MATRIX, modelview_matrix );

	// final length of the formatted text
	sz = strlen( text );
	Assert( sz < TEXT_SIZE );


	//This is where the text display actually happens.
	//For each line of text we reset the modelview matrix
	//so that the line's text will start in the correct position.
	//Notice that we need to reset the matrix, rather than just translating
	//down by h. This is because when each character is
	//drawn it modifies the current matrix so that the next character
	//will be drawn immediatly after it.  

	gglPushMatrix();
	gglLoadIdentity();
    //gglColor4f( r, g, b, a );
	gglTranslatef( (GLfloat)x, (GLfloat)(y - F_POINT_SZ), 0);
    gglMultMatrixf( modelview_matrix );

	//  The commented out raster position stuff can be useful if you need to
	//  know the length of the text that you are creating.
	//  If you decide to use it make sure to also uncomment the gglBitmap command
	//  in make_dlist().

	gglRasterPos2f(0,0);

    gglCallLists( sz, GL_UNSIGNED_BYTE, text );

	// get screen length of text
	float rpos[4];
	gglGetFloatv( GL_CURRENT_RASTER_POSITION , rpos );
	float len = rpos[0] - x;
	len = rpos[0] + 10; // note: for this to work, we need to know the width
						// of the last character, another reason to NOT use
						// display lists

    gglPopMatrix(); // FIXME: double check that popAttrib doesn't pop the matrix already..
	gglPopAttrib();		

	return len;
}
#endif





/*
====================

 the modelview matrix and rasterPos are handled outside of this function.
 all this does is bind the texture, blit it, and advance the appropriate amount
====================
*/
static void _printOneChar( glyph_t *g ) {

	gglBindTexture( GL_TEXTURE_2D, g->texhandle );

	// first we need to move over a little so that
	// the character has the right amount of space
	// between it and the one before it.
	gglTranslatef( g->left * scale, 0, 0 );

	// Now we move down a little in the case that the
	// bitmap extends past the bottom of the line 
	// (this is only true for characters like 'g' or 'y'.
	gglPushMatrix();
	gglTranslatef( 0, ( g->top - g->rows ) * scale, 0 );

	//Now we need to account for the fact that many of
	//our textures are filled with empty padding space.
	//We figure what portion of the texture is used by 
	//the actual character and store that information in 
	//the x and y variables, then when we draw the
	//quad, we will only reference the parts of the texture
	//that we contain the character itself.
	float tx = (float)g->width / (float)g->tex_width;
	float ty = (float)g->rows  / (float)g->tex_height;

	//Here we draw the texturemaped quads.
	//The bitmap that we got from FreeType was not 
	//oriented quite like we would like it to be,
	//so we need to link the texture to the quad
	//so that the result will be properly aligned.

	// handle TAB char ( '\t', '\n', '\r\n', ' ' ) specially

	float height = g->rows * scale;
	float width = g->width * scale;


	gglBegin( GL_QUADS );
	gglTexCoord2f( 0.f, 0.f ); 
	gglVertex2i( 0,	height );

	gglTexCoord2f( 0.f, ty ); 
	gglVertex2i( 0,	0 );

	gglTexCoord2f( tx, ty ); 
	gglVertex2i( width, 0 );

	gglTexCoord2f( tx, 0.f ); 
	gglVertex2i( width, height );
	gglEnd();

	gglPopMatrix();

	gglTranslatef( (GLfloat) g->advance * scale, 0.f, 0.f );
}




float F_Printf( int x, int y, int fnum, const char *fmt, ... ) {
    char text[ TEXT_SIZE ];
    va_list ap;
	float modelview_matrix[16];	
    size_t sz;

	if ( !is_init ) {
		F_InitFreetype();
		is_init = true;
	}

	if ( fnum >= (int)fonts.size() ) {
		Err_Warning( "requested font out of range" );
		return 0.f;
	}
	fontCont_s *fc = fonts.data[ fnum ];

    memset ( text, 0, sizeof(text) );

    if ( fmt == NULL ) {
        *text = 0;
    } else {
        va_start( ap, fmt );
        vsprintf( text, fmt, ap );
        va_end( ap );
    }
	text[ TEXT_SIZE - 1 ] = '\0';


	gglEnable( GL_TEXTURE_2D );


	// use the matrix & rasterPos to push around the characters
    gglPushAttrib( GL_ALL_ATTRIB_BITS );
	gglPushMatrix();
	gglLoadIdentity (); 
	gglTranslatef( x, y - fc->sz * scale, 0.f );


	// save the x translation before
	float mat[16], vmat[16];
	gglGetFloatv( GL_MODELVIEW_MATRIX, mat );
	float x_before = mat[ 12 ];
	float x_after;
	float longest_row = 0.f;


	char *letter = text;

	int lines = 0;

	while ( *letter ) {
		if ( *letter < 33 ) {
			if ( *letter == '\t' ) {
				glyph_t *g = &fc->glyph[ 'C' - 32 ];
				GLfloat t4 = (GLfloat) g->advance * 4.f * scale; 
				gglTranslatef( t4, 0.f, 0.f );
			} else if ( *letter == '\n' ) {
				lines++;

				// save the row length if it's the longest so far
				gglGetFloatv( GL_MODELVIEW_MATRIX, vmat );
				if ( vmat[ 12 ] - x_before > longest_row )
					longest_row = vmat[ 12 ] - x_before;

				// load the original model, and go down lines
				gglLoadMatrixf( mat );
				float down = -1.0f * ( fc->sz + 7 ) * lines * scale;
				gglTranslatef( 0.f, down, 0.f );
			} else if ( *letter == ' ' ) {
				glyph_t *g = &fc->glyph[ 'P' - 32 ];
				GLfloat t = (GLfloat) g->advance * scale; 
				gglTranslatef( t, 0.f, 0.f );
			}

			letter = letter + 1;
			continue;
		}

		Assert( *letter < 128 );

		_printOneChar( &fc->glyph[ *letter++ - 32 ] );
	}

	// get current modelview matrix w/ all the x translation in it 
	gglGetFloatv( GL_MODELVIEW_MATRIX, vmat );
	x_after = vmat[ 12 ];

	if ( x_after - x_before > longest_row )
		longest_row = x_after - x_before;

	// reset the matrix & rasterPos
	gglPopMatrix();
	gglPopAttrib();

	gglDisable( GL_TEXTURE_2D );

	return longest_row;
}




void FreetypeTest( void ) {} /* {

	if ( !is_init ) {
		F_InitFreetype();
		is_init = true;
	}

	int h = 18, H=460;
	
	F_Printf ( 10, 460, 0, 0xAAAAFFFF, "Hello World!" );
	F_Printf( 10, H-=h, 0, ~0, "This is the GNU Vera font" );
	F_Printf( 10, H-=h, 1, ~0, "This one here is VeraBd" );
	F_Printf( 10, H-=h, 2, ~0, "This is the GNU VeraBI font" );
	F_Printf( 10, H-=h, 3, ~0, "This is the GNU VeraIT font" );
	F_Printf( 10, H-=h, 4, ~0, "This is the GNU VeraMoBd font" );
	F_Printf( 10, H-=h, 5, ~0, "This is the GNU VeraMoBI font" );
	F_Printf( 10, H-=h, 6, ~0, "This is the GNU VeraMoIT font" );
	F_Printf( 10, H-=h, 7, ~0, "This is the GNU VeraMono font" );
	F_Printf( 10, H-=h, 8, ~0, "This is the GNU VeraMonoSe font" );
	F_Printf( 10, H-=h, 9, ~0, "This is the GNU VeraMonoSeBd font" );
	F_Printf( 10, H-=h, 10, ~0, "This is the BATTLE3 font from Uplink" );
	F_Printf( 10, H-=h, 11, ~0, "This is the \"dungeon\" font, also from Uplink" );

}*/



