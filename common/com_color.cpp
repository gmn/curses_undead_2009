// com_color.cpp
//

#include "com_color.h"
#include <stdlib.h>
#include <string.h>

color_c color;

colorSet_t colors[] = {
	{ C_BLACK,		"C_BLACK", 		{ 0.0f, 0.0f, 0.0f, 1.0f } },
	{ C_WHITE,		"C_WHITE", 		{ 1.0f, 1.0f, 1.0f, 1.0f } },
	{ C_RED,   		"C_RED", 		{ 1.0f, 0.0f, 0.0f, 1.0f } },
	{ C_GREEN, 		"C_GREEN", 		{ 0.0f, 1.0f, 0.0f, 1.0f } },
	{ C_BLUE,   	"C_BLUE", 		{ 0.0f, 0.0f, 1.0f, 1.0f } },
	{ C_YELLOW,   	"C_YELLOW", 	{ 1.0f, 1.0f, 0.0f, 1.0f } },
	{ C_PURPLE,   	"C_PURPLE",		{ 0.6f, 0.0f, 0.6f, 1.0f } },
	{ C_VIOLET,   	"C_VIOLET", 	{ 1.0f, 0.0f, 1.0f, 1.0f } },
	{ C_ORANGE,   	"C_ORANGE", 	{ 1.0f, 0.4f, 0.0f, 1.0f } },
	{ C_AQUA,   	"C_AQUA", 		{ 0.0f, 0.6f, 0.8f, 1.0f } },
	{ C_NAVY,   	"C_NAVY", 		{ 0.0f, 0.2f, 0.6f, 1.0f } },
	{ C_DARKGREEN, 	"C_DARKGREEN", 	{ 0.0f, 0.4f, 0.0f, 1.0f } },
	{ C_HOTPINK,   	"C_HOTPINK", 	{ 1.0f, 0.2f, 0.6f, 1.0f } },
	{ C_DEEPPURPLE,	"C_DEEPPURPLE",	{ 0.4f, 0.0f, 0.4f, 1.0f } },
	{ C_LIME,   	"C_LIME", 		{ 0.2f, 0.6f, 0.0f, 1.0f } },
	{ C_BURGUNDY,  	"C_BURGUNDY", 	{ 0.6f, 0.0f, 0.2f, 1.0f } },
	{ C_INDIGO, 	"C_INDIGO", 	{ 0.2f, 0.0f, 0.8f, 1.0f } },
	{ C_BEIGE, 		"C_BEIGE", 		{ 0.6f, 0.4f, 0.2f, 1.0f } },
};

const int TOTAL_COLORS = sizeof(colors) / sizeof(colors[0]) ;


static int TOLOWER( int x ) {
	if ( x >= 65 && x <= 90 ) {
		return x + 'a' - 'A'; 
	}
	return x;
}

int color_c::GetColorName4v( float *v, const char * s ) {
	for ( int i = 0; i < TOTAL_COLORS; i++ ) {
		// found a first letter prospect
		if ( TOLOWER( colors[ i ].name[ 2 ] ) == TOLOWER( *s ) ) {
			colorSet_t *c = &colors[i];
			int j = 2, k = 0;
			while ( TOLOWER( c->name[ j ] ) && TOLOWER( s[ k ] ) ) {
				if ( TOLOWER( c->name[ j ] ) != TOLOWER( s[ k ] ) ) {
					c = NULL;
					break;
				}
				++j; ++k;
			}
			if ( c ) {
				v[ 0 ] = c->fc[ 0 ];
				v[ 1 ] = c->fc[ 1 ];
				v[ 2 ] = c->fc[ 2 ];
				v[ 3 ] = c->fc[ 3 ];
				return 1;
			}
		}
	}
	return 0;
}

colorSet_t * color_c::colors = NULL;

void color_c::StringToColor4v( float *v, const char *s ) {
	v[ 0 ] = v[ 1 ] = v[ 2 ] = v[ 3 ] = 1.0f;

	int i = 0, j = 0, k = 0;

	while( s[i] == ' ' ) 
		++i;
	// check if string is a color name
	if ( TOLOWER(s[i]) >= 'a' && TOLOWER(s[i]) <= 'z' ) {
		if ( GetColorName4v( v, &s[i] ) ) {
			return;
		}
	}

	char buf[64];
	while ( k < 4 ) {
		// inits
		j = 0;
		memset( buf, 0, sizeof( buf ) );

		// trip whitespace off input string
		while( s[i] == ' ' ) 
			++i;
	
		if ( !s[ i ] )
			break;

		// get everything thats not a space
		while ( s[ i ] != ' ' && s[ i ] != '\0' ) {
			buf[ j ] = s[ i ];
			++i; ++j;
		}
		buf [ j ] = '\0';

		// make a float
		float f = atof( buf );
		if ( f > 1.0f ) f = 1.0f;
		if ( f < 0.0f ) f = 0.0f;

		v[ k++ ] = f;
	}
}

int color_c::GetColorNamei( unsigned int *ip, const char * s ) {
	return 0;
}

void color_c::StringToUInt( unsigned int *ip, const char * s ) {
	*ip = 0xFFFFFFFF;
	unsigned char *bp = (unsigned char *) ip;

	int i = 0, j = 0, k = 0;

	// strip whitespace off input string
	while( s[i] == ' ' ) 
		++i;
	// check if string is a color name
	if ( TOLOWER(s[i]) >= 'a' && TOLOWER(s[i]) <= 'z' ) {
		if ( GetColorNamei( ip, &s[i] ) ) {
			return;
		}
	}

	char buf[64];
	while ( k < 4 ) {
		// inits
		j = 0;
		memset( buf, 0, sizeof( buf ) );

		while( s[i] == ' ' ) 
			++i;
	
		if ( !s[ i ] )
			break;

		// get everything thats not a space
		while ( s[ i ] != ' ' && s[ i ] != '\0' ) {
			buf[ j ] = s[ i ];
			++i; ++j;
		}
		buf [ j ] = '\0';

		// make a float
		float f = atof( buf );
		if ( f > 1.0f ) f = 1.0f;
		if ( f < 0.0f ) f = 0.0f;
		bp[ k++ ] = (unsigned char) ( f * 255.0f );
	}
}

float * color_c::TokenToFloat( unsigned int token ) {
	int i = 0;
	while ( i < TOTAL_COLORS ) {
		if ( colors[ i ].color == token ) 
			return colors[ i ].fc;
		++i;
	}
	return colors[ 0 ].fc;
}

