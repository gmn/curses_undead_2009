
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "../lib/lib.h" 
#include "cl_console.h"
#include "../map/mapdef.h"
#include "../renderer/r_ggl.h"
#include "../client/cl_keys.h"
#include "../client/cl_public.h"
#include "../lib/lib_parser.h"







#if defined(_WIN32) && !defined(snprintf)
#define snprintf _snprintf
#endif



console_t console;



void console_t::on( void ) {
	is_exposed = true;
	state = CON_OPEN;
	anim_height = height;

	if ( !keyCatchSave ) {
		keyCatchSave = cls.keyCatchers;
		cls.keyCatchers = KEYCATCH_CONSOLE;
	}
}

void console_t::off( void ) {
	if ( keyCatchSave ) {
		cls.keyCatchers = keyCatchSave;
		keyCatchSave = 0;
	}

	is_exposed = false;
	anim_height = 0;
	state = CON_CLOSED;
}

/*
====================
 console_t::Toggle
====================
*/
void console_t::Toggle( void ) {
/* if anim_height > 0, then is_exposed must be set.  the state can be anything except CON_CLOSED.  once anim_height reaches
zero, then CON_CLOSED is set */

    if ( _lockConsole->integer() )
        return;

	// if its totally closed, start expansion
	if ( state == CON_CLOSED && !is_exposed ) {
		timer.set( TRANSITION_TIME ); 
		on();
		state = CON_EXPANDING;
		anim_height = 0;
		next_tick = frac_tick;
		return;
	}

	// if its completely expanded, begin retraction
	if ( state == CON_OPEN && is_exposed ) {
		state = CON_RETRACTING;
		timer.set( TRANSITION_TIME );
		is_exposed = true;
		anim_height = height;
		next_tick = frac_tick;
		return;
	}

	// is in the process of closing
	if ( state == CON_RETRACTING && is_exposed ) {
		state = CON_EXPANDING;
		is_exposed = true;
		timer.set( timer.delta() );
		next_tick = frac_tick * ((float)timer.delta()/(float)TRANSITION_TIME);
		return;
	}

	// is in the process of opening
	if ( state == CON_EXPANDING && is_exposed ) {
		state = CON_RETRACTING;
		is_exposed = true;
		timer.set( timer.delta() );
		next_tick = frac_tick * ((float)timer.delta()/(float)TRANSITION_TIME);
		return;
	}
}

// 

void console_t::pollDynamicSize( void ) {

	if ( state == CON_CLOSED || state == CON_OPEN ) 
		return;

	float delta = (float) timer.delta();

	/* we have a timer thats going to tic off 1000 milliseconds.  our computed pixel height is 288 given the current resolution
	of 640x480.  the frac_time = 3.4777.  every time we're one multiple of frac_time past where we were last time tic the anim_height */

	/* if the anim_height reaches 0, or height, then adjust the state */

	// haven't reached a marker, nothing to do
	if ( delta <= next_tick ) {
		return;
	}

	// overshot the end of the transition period
	if ( delta >= TRANSITION_TIME ) {
		if ( CON_EXPANDING == state ) {
			on();
		}
		if ( CON_RETRACTING == state ) {
			off();
		}
		return;
	}

	float d = delta;

	while ( d > next_tick ) {
		d -= frac_tick;

		if ( state == CON_EXPANDING ) {
			anim_height++;
			if ( anim_height >= height ) {
				on();
				return;
			}
		} else if ( state == CON_RETRACTING ) {
			anim_height--;
			if ( anim_height <= 0 ) {
				off();
				return;
			}
		} else {
			break;
		}
	}

	// increment the marker until its in the future
	while ( next_tick < delta ) {
		next_tick += frac_tick;
	}

	// iterate the vertical size of the console by time.  there are 4 states. OPEN, CLOSED, EXPANDING, RETRACTING
	// There is a target exposed size.  hitting toggle when it is closed, or less than fully open, 
}

/*
====================
 console_t::Draw
====================
*/
void console_t::Draw( void ) {

	if ( !is_exposed )
		return;

	pollDynamicSize();

	// could have changed
	if ( !is_exposed )
		return;


	int screen_w = M_Width();
	int screen_h = M_Height();

	// set GL scissor test
	gglEnable( GL_SCISSOR_TEST );
	gglScissor( 0, screen_h-anim_height, screen_w, anim_height );  

	// save user font color and draw the background
	float c[4]; gglGetFloatv( GL_CURRENT_COLOR , c ) ;

	gglColor4f( 0.1f, 0.1f, 0.1f, 1.0f );
	gglBegin( GL_QUADS ); 
    gglVertex2i( 0, screen_h-anim_height );
	gglVertex2i( screen_w, screen_h-anim_height );
	gglVertex2i( screen_w, screen_h );
    gglVertex2i( 0, screen_h );
	gglEnd(); 

	int line_delta = view.h + view.hpad; 

	// border of the text entry area
	gglBegin( GL_LINES );
	int _h = screen_h - ( anim_height - line_delta - input_area_pad - 2 * border_width );
	gglColor4f( 0.5f, 0.5f, 0.5f, 1.0f );
	gglVertex2i( 0,			_h-1 );
	gglVertex2i( screen_w,	_h-1 );
	gglColor4f( 0.05f, 0.05f, 0.05f, 1.0f );
	gglVertex2i( 0,			_h );
	gglVertex2i( screen_w,	_h );
	gglColor4f( 0.7f, 0.7f, 0.7f, 1.0f );
	gglVertex2i( 0,			_h+1 );
	gglVertex2i( screen_w,	_h+1 );

	_h = screen_h - ( anim_height - border_width );
	gglColor4f( 0.5f, 0.5f, 0.5f, 1.0f );
	gglVertex2i( 0,			_h-1 );
	gglVertex2i( screen_w,	_h-1 );
	gglColor4f( 0.1f, 0.1f, 0.1f, 1.0f );
	gglVertex2i( 0,			_h );
	gglVertex2i( screen_w,	_h );
	gglColor4f( 0.7f, 0.7f, 0.7f, 1.0f );
	gglVertex2i( 0,			_h+1 );
	gglVertex2i( screen_w,	_h+1 );
	gglEnd();

	float	_h2 = screen_h - ( anim_height - border_width - input_area_pad - line_delta + 2 );

	// the current line of text
	gglColor4f( 0.2f, 0.5f, 0.9f, 1.0f );
	float len = 0.f;
	if ( current_line[0] ) {
		len = F_Printf( 10, _h2-2, font, "%s", current_line );
	}

	// draw cursor 
	float ms = (float)( Com_Millisecond() % 1200 );
	float g = cosf( ms * 6.3 / 1200.0f ) / 2.0f + 0.5f;
	g *= 0.65f;
	gglColor4f( 0.85f, 0.85f, 0.0f, g );
			_h	= screen_h - ( anim_height - border_width - input_area_pad - 1 );
	gglBegin( GL_QUADS );
	gglVertex2f( 11 + len, _h );
	gglVertex2f( 11 + len, _h2 );
	gglVertex2f( 11 + len + 8, _h2 );
	gglVertex2f( 11 + len + 8, _h );
	gglEnd();


	_h = ( anim_height - line_delta - input_area_pad - 2 * border_width - 5 );
	gglScissor( 0, screen_h - _h, screen_w, _h );  

	// reset the user color
	gglColor4fv( c );

	int line_initial = (int) ( screen_h - ( anim_height - 2 * line_delta - 2 * input_area_pad - 2 * border_width ) );
	int line_y = line_initial;
	int line_x = view.x + view.w + view.wpad;

	node_c<char*> *line = lines.gettail();

	int limit = 20; // draw lines max

	// temp buffers only initialized for long lines
	buffer_c<char*> sep_lines;
	sep_lines.init( 32 );
	buffer_c<char> sbuf;

	
	int LINE_BREAK_LEN = 70;
	// HACK ALERT: ok, I'm going to choose the line break & line buffer values based on empirical 
	//  guesses for each display resolution
	int W = M_Width();
	switch( W ) {
	case 640:	LINE_BREAK_LEN = 70;	limit = 20;		break;
	case 800:	LINE_BREAK_LEN = 90;	limit = 23;		break;
	case 1024:	LINE_BREAK_LEN = 102;	limit = 26;		break;
	case 1280:	LINE_BREAK_LEN = 120;	limit = 32;		break;
	case 1440:	LINE_BREAK_LEN = 130;	limit = 36;		break;
	case 1600:	LINE_BREAK_LEN = 140;	limit = 40;		break;
	}


	// increase the limit if we're scrolling
	limit += scroll / 18;


	while ( line && limit-- > 0 ) {
		// F_Printf takes the top-left pixel, of the area that the line of text occupies

		// but we are printing in opengl space, so 0,0 is the lower-left corner, not the top-left,
		//  like we would like.  

		// 60 chars in a line.  for lines longer than 60, print indented 
		int slen = strlen( line->val );

		// if text of this line longer than row width
		if ( slen > LINE_BREAK_LEN ) {
			// make copy
			sbuf.init( ( ((slen+1)+31) & ~31 ) * 2 ); 
			sbuf.data[0] = 0;
			sbuf.copy_in( line->val, slen );
			sbuf.data[ slen ] = 0;

			// restart
			sep_lines.reset();

			int abc = 123;
			int abcd = 123;
			// create ordered set of pointers to separate lines

			char *beg = sbuf.data;
			char *stop = sbuf.data + slen;

			// trim spaces off of beginning of line
			while ( *beg == ' ' && (unsigned int)beg < (unsigned int)stop ) 
				++beg;

			int times = 0;
			while ( (unsigned int)beg < (unsigned int)stop ) {
				// start at end of line, one linelength long and go backwards 
				//  scanning for the first appropriate place to break the line.

				// may be invalid, don't dereference just yet
				char *end = beg + ( LINE_BREAK_LEN - 2 );
				
				// ??
				if ( times == 0 )
					end += 2;

				// loop counter
				++times;

				// if end goes off of line, bring it back into the array
				if ( (unsigned int)end > (unsigned int)stop )
					end = stop - 1;

				while ( *end && *--end != ' ' && end != beg && (unsigned int)end > (unsigned int)sbuf.data ) 
					;
				// hit beginning of line looking for a space
				if ( end == beg ) {
					// set it back to desired line length
					end = beg + ( LINE_BREAK_LEN - 2 );
					// verify the bounds
					if ( (unsigned int)end > (unsigned int)stop )
						end = stop - 1;
				}

				// before we write it, just to make sure
				Assert( end >= sbuf.data && end < stop ); 

				if ( *end == ' ' ) {
					*end = '\0';
				} else if ( *end == '\0' ) {
					// do nothing
				} else {
					// copy whole line starting at end over to the right 1 place
					//  so that the char on *end is now on *(end+1).  even 
					//  copy the NULL char at end 
					// put a null in *end;
					char *p = end;
					//++stop;
					while ( p != stop ) 
						*(p+1) = *p++;
					// null on end
					*++end = '\0';
				}
	
				// save beg
				sep_lines.add( beg );
				
				// place beg for the next pass
				beg = end + 1;
				while ( (unsigned int)beg < (unsigned int)stop && *beg == ' ' ) {
					++beg;
				}
			}

			// highlight 
			gglColor4f( 1.f, 1.f, 0.0f, 1.0f );

			//node_c<char*> *parts = sep_lines.gettail();
			//node_c<char*> *head = sep_lines.gethead();

			unsigned int brokenLines = sep_lines.length();
			while ( --brokenLines >= 1 ) {
				F_Printf( line_x , line_y - scroll , font, "  %s", sep_lines.data[ brokenLines ] );
				line_y += line_delta;
			}
			F_Printf( line_x, line_y - scroll , font, "%s", sep_lines.data[0] );
			line_y += line_delta;
			
/*
			// lower lines: indented
			while ( parts && parts != head ) {
				beg = parts->val;
				F_Printf( line_x , line_y - scroll , font, "  %s", beg );
				line_y += line_delta;
				parts = parts->prev;
			}

			// top-line: slightly longer, un-indented 
			if ( parts ) {
				beg = parts->val;
				F_Printf( line_x, line_y - scroll , font, "%s", beg );
				line_y += line_delta;
			}
*/

			//limit -= sep_lines.size() - 1;
			limit -= sep_lines.length() - 1;

			// reset the user color
			gglColor4fv( c );


		// Line Length < LINE_BREAK_LEN
		} else {
			F_Printf( line_x , line_y - scroll , font, "%s", line->val );
			line_y += line_delta;
		}

		line = line->prev;
	}

	sbuf.destroy();
	sep_lines.destroy();

	// unset GL scissor test
	gglDisable( GL_SCISSOR_TEST );
}

// need another way to get lines buffer


void console_t::pushMsg( const char *msg ) {
	if ( !msg ) {
		lines.add( NULL );
		return;
	}
	size_t len = strlen( msg ) + 1;
	char *line = (char*) V_Malloc( len );
	strncpy( line, msg, len-1 );
	line[len-1] = '\0';
	lines.add( line );
}

void console_t::reset( void ) 
{
	//main_viewport_t *v = M_GetMainViewport();

	// 60% of vertical screen size;
	//height = 0.6f * v->res[v->resnum].height;

	height = 0.6f * M_Height();

	// initial state is closed
	anim_height = 0;
	is_exposed = false;
	state = CON_CLOSED;
	
	view.x = 8.0f;
	view.y = 0.0f;
	
	view.w = 0.0f;	// freetype does the font spacing
	view.h = 12.0f; // pixel height of font

	view.hpad = 6.0f;
	view.wpad = 0.0f;
	view.xpad = view.ypad = 20.0f;
	lines.start();

	frac_tick = (float)TRANSITION_TIME / (float)height;
	next_tick = frac_tick;

	border_width = 3;
	input_area_pad = 4;

	current_line[ 0 ] = 0;
	current_char = 0;

	keyCatchSave = 0;
	shift = 0;
	capslock = false;

/* dont have these any more
	font = FONT_VERA_MONO;
	font = FONT_VERA_MONO_BOLD;
	font = FONT_VERA_SE_BOLD;
	font = FONT_VERA_SE;
	font = FONT_UNI53;
	font = FONT_UNI63;
	font = FONT_UNI64;
	font = FONT_UNI54;
*/
	font = FONT_VERA;
	font = FONT_VERA_BOLD;

	scroll = 0;

	histLine = 0; // which line showing, 0 for current_line, -1..-INF for nodes
	history.init( 1024 );
}

void console_t::destroy( void ) {
	history.destroy();
	lines.destroy();
}

// typical output of 'mem' command is 120,000 bytes ~0x20000  
#define STUPID_FIXED_BUFFER_SIZE 0x8000

#define BIGGER_BUFSZ 0x40000

static char * text = NULL;

// FIXME: these var arg functions are a buffer overflow waiting to happen!
void console_t::Printf( const char *fmt, ... ) {
	//char text[ STUPID_FIXED_BUFFER_SIZE ];

	if ( !text ) {
		// get one exclusive page for console.Printf
		text = (char *) V_AllocExclusivePage( BIGGER_BUFSZ );
	}

	va_list ap;
	va_start( ap, fmt );
	vsprintf( text, fmt, ap );
	va_end( ap );
	//text[ STUPID_FIXED_BUFFER_SIZE-1 ] = '\0';
	pushMsg( text );
}

int console_t::checkCmd( void ) {
	if ( !current_line[0] )
		return 0;
	
	parser_t parser;
	
	if ( !parser.tokenize( current_line ) )
		// got no tokens 
		return 0;

	char ret_line[1000];

	if ( !parser.runCmds( ret_line, sizeof(ret_line) ) ) {
		// no runnable commands
		return 0;
	}

	if ( ret_line[0] )
		pushMsg( ret_line );

	return 1;
}

// just typing in a gvar prints out it's value
int console_t::checkGvar( void ) {
	if ( !current_line[0] )
		return 0;

	parser_t parser;	
	if ( !parser.tokenize( current_line ) )
		return 0;

	if ( parser.numToks() != 1 )
		return 0;

	const char * word = parser.getToken( 0 );

	gvar_c *g = NULL;
	if ( (g=Gvar_Find( word )) ) {
		console.Printf( "%s: \"%s\"", g->name(), g->string() );
		return 1;
	}

	return 0;
}

void console_t::keyHandler( int key, int down ) {
	if ( !down ) {
		if ( key == KEY_LSHIFT ) 
			shift &= ~1;
		if ( key == KEY_RSHIFT )
			shift &= ~2;
		return;
	}

	if ( key == KEY_LSHIFT ) {
		shift |= 1;
		return;
	}
	if ( key == KEY_RSHIFT ) {
		shift |= 2;
		return;
	}
	if ( key == KEY_CAPSLOCK ) {
		capslock = !capslock;
		return;
	}

	if ( key == KEY_BACKSPACE ) {
		if ( --current_char < 0 )
			current_char = 0;
		current_line[ current_char ] = 0;
		return;
	}
	else if ( key == KEY_ENTER ) {
		if ( !checkCmd() && !checkGvar() ) {
			pushMsg( current_line );
		}
		SaveLine();
		current_line[0] = 0;
		current_char = 0;
		return;
	}

	// a-z
	if ( key >= 'a' && key <= 'z' ) {
		if ( shift || capslock )
			current_line[ current_char++ ] = key - ( 'a' - 'A' );
		else
			current_line[ current_char++ ] = key;
		current_line[ current_char ] = '\0';
		return;
	}

	if ( key == '`' )
		return;

/*
	if ( key == KEY_UP ) {
		scroll += SCROLL_AMOUNT;
		return;
	} else if ( key == KEY_DOWN ) {	
		scroll -= SCROLL_AMOUNT;
		if ( scroll < 0 )
			scroll = 0;
		return;
*/
	if ( key == KEY_UP ) {
		HistoryBack();
		return;
	} else if ( key == KEY_DOWN ) {	
		HistoryNext();
		return;

	} else if ( key == KEY_ESCAPE ) {
		current_char = 0;
		memset( current_line, 0, sizeof(current_line) );
	

	} else if ( key == KEY_PGUP ) {
		int line_delta = view.h + view.hpad; 
		scroll += ( anim_height - line_delta - input_area_pad - 2 * border_width );
		return;
	} else if ( key == KEY_PGDOWN ) {
		int line_delta = view.h + view.hpad; 
		scroll -= ( anim_height - line_delta - input_area_pad - 2 * border_width );
		if ( scroll < 0 )
			scroll = 0;
		return;
	}

	if ( shift ) {
		// 1-0 , '-', '='
		switch ( key ) {
		case '0': current_line[ current_char++ ] = ')'; break;
		case '1': current_line[ current_char++ ] = '!'; break;
		case '2': current_line[ current_char++ ] = '@'; break;
		case '3': current_line[ current_char++ ] = '#'; break;
		case '4': current_line[ current_char++ ] = '$'; break;
		case '5': current_line[ current_char++ ] = '%'; break;
		case '6': current_line[ current_char++ ] = '^'; break;
		case '7': current_line[ current_char++ ] = '&'; break;
		case '8': current_line[ current_char++ ] = '*'; break;
		case '9': current_line[ current_char++ ] = '('; break;
		case '-': current_line[ current_char++ ] = '_'; break;
		case '=': current_line[ current_char++ ] = '+'; break;

		case '[': current_line[ current_char++ ] = '{'; break;
		case ']': current_line[ current_char++ ] = '}'; break;
		case '\\': current_line[ current_char++ ] = '|'; break;
		case ';': current_line[ current_char++ ] = ':'; break;
		case '\'': current_line[ current_char++ ] = '"'; break;
		case ',': current_line[ current_char++ ] = '<'; break;
		case '.': current_line[ current_char++ ] = '>'; break;
		case '/': current_line[ current_char++ ] = '?'; break;
		case ' ': current_line[ current_char++ ] = ' '; break;
		}
	} else
		current_line[ current_char++ ] = key;

	current_line[ current_char ] = '\0';
}

void console_t::dump( const char * f ) {
	dumpcon( f );
}

void console_t::dumpcon( const char *filename ) {

	char defname[ 512 ] = "console.DUMP";
	if ( !filename ) {
		filename = defname;
	}

	// check if there is already a file of that filename,
	// if so, move it to filename.1 etc..
	char buf[512];
	snprintf( buf, 512, "%s.bak" );
	Com_CopyFile( filename, buf );

	FILE *fp = fopen( (char*)filename, "wb+" );
	if ( !fp ) {
		Printf( "error: couldn't open %s for writing", filename );
		return;
	}
	
	node_c<char*> *line = lines.gethead();
	while ( line ) {
		fprintf( fp, "%s\n", line->val );
		line = line->next;
	}
	
	fclose( fp );
}

void console_t::Clear( void ) {
	lines.reset();
	return;

	int vert = this->height;
	while ( vert > 0 ) {
		pushMsg( "" );
		vert -= 20;
	}
}

bool console_t::load( const char *file ) {
	FILE * fp = fopen ( file, "rb" );
	if ( !fp )
		return false;

	int sz = Com_FileSize( file );
	if ( sz > 0 ) {
		lines.reset();
	}

	char *buf = (char*) V_Malloc( ((sz+1) + 32) & ~31 );
	fread( buf, 1, sz, fp );
	fclose( fp );
	buf[sz] = 0;

	char *line_p = buf;
	int i = 0;
	while ( i < sz && buf[i] ) {

		// get next newline or end of file
		while ( buf[i] && buf[i] != '\n' ) {
			++i;
		}
		
		// save line before newline
		if ( i < sz && buf[i] == '\n' ) {
			buf[i] = '\0';
			pushMsg( line_p );
		// save last line
		} else if ( i == sz ) {
			buf[sz] = 0;
			pushMsg( line_p );
			break;
		}

		// start looking for start of next line
		++i;
		while ( i < sz && buf[ i ] == '\n' ) {
			++i;
		}
		line_p = &buf[i];
	}
	
	V_Free( buf );
	return true;
}

void console_t::mouseHandler( mouseFrame_t *mf ) {
	if ( mf->z > 0 )
		scroll += 3 * SCROLL_AMOUNT;
	else if ( mf->z < 0 ) {
		scroll -= 3 * SCROLL_AMOUNT;
		if ( scroll < 0 )
			scroll = 0;
	}
}

void console_t::SaveLine( void ) {
	int len = strlen( current_line ) + 1;
	char * line = (char * ) V_Malloc( len );
	strcpy( line, current_line );
	line[ len - 1 ] = 0;
	history.push( line );
	histLine = 0;
}

void console_t::HistoryBack( void ) {
	if ( history.length() == 0 )
		return;
	histLine -= 1;
	if ( -histLine > history.length() ) {
		histLine = -1 * (int)history.length();
	}

	int index = history.length() + histLine;
	Assert( index >= 0 && index < history.length() );
	strcpy( current_line, history.data[ index ] );
	current_char = strlen( current_line );
}
void console_t::HistoryNext( void ) {
	if ( histLine >= 0 )
		return;

	histLine += 1;

	if ( histLine != 0 ) {
		int index = history.length() + histLine;
		Assert( index >= 0 && index < history.length() );
		strcpy( current_line, history.data[ index ] );
	}
	current_char = strlen( current_line );
}

void console_t::PrintHistory( void ) {
	int ofst = -1 * (int)history.length();
	int length = history.length(); 
	if ( length == 0 )
		return;
	console.pushMsg( "" );
	while ( ofst < 0 ) {
		console.Printf( "%-3d: %s", ofst, history.data[ length + ofst ] );
		++ofst;
	}
}

void CL_StartCon( void ) {
	console.init();
}

void CL_DrawString( int x, int y, int w, int h, const char * string, int wpad, int hpad );

// draws last 5 lines in upper left of game screen
void console_t::drawScreenBuffer( void ) {

	node_c<char*> *line = lines.gettail();
	if ( !line )
		return;

	int _total = 6;
	float h = M_Height() - 106.f;

	// push bottom lines upwards
	while ( _total >= lines.size() ) {
		_total--;
		h += 20.f;
	}

	while ( _total-- && line ) {
		F_Printf( 8, h, font, "%s", line->val );	
		//CL_DrawString( 8, h - 20, 16, 12, line->val, 4, 4 );
		h += 20.f;
		line = line->prev;
	}
}
