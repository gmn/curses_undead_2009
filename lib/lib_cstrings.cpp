/******************************************************************************
 * part of zombiecurses 
 *
 * Copyright (C) 2007 Greg Naughton
 *
 * cstrings.cpp - common c string functions
 *****************************************************************************/

/* TODO: O_strchr, O_strrchr, O_strlen */


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "lib.h"

/*
====================
 va - general purpose var args
====================
*/
const char * va (char *fmt, ...)
{
    static char buf[0x10000];
    va_list args;

    memset(buf, 0, sizeof(buf));
    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);

    return &buf[0];
}

/*
====================
 O_strncmp
  only check up to sz chars, if it goes that far
====================
*/
int O_strncmp ( const char *s, const char *n, int sz )
{
	int i = 0;
	while ( *s && *n && i < sz ) {
		if ( *s != *n ) {
			return *s - *n;
		}
		++s; ++n; ++i;
		if ( *s == '\0' || *n == '\0' ) {
			return *s - *n;
		}
	}
	if ( i == sz )
		return 0;

	return *s - *n;
}
/*
int O_strncmp (const char *s, const char *n, int sz)
{
    int diff = 0;
    int i = 0, se = -1, ne = -1;
    while (*s || *n)
    {
        if (i > sz)
            break;

        if (se != -1 && ne != -1)
            break;

        if (!*s) 
            se = i;
        if (!*n) 
            ne = i;

        if (se > -1 && ne > -1)
            return diff;

        if (se > -1)
            diff += -(*n);
        else if (ne > -1)
            diff += -(*s);
        else
            diff += *s - *n;

        if (se == -1)
            ++s; 
        if (ne == -1)
            ++n;
        ++i;
    }
    return diff;
} */

/* 
static inline int TOLOWER( int x ) {
	if ( x >= 65 && x <= 90 ) {
		return x + 'a' - 'A'; 
	}
	return x;
}
static inline int TOUPPER( int x ) {
	if ( x >= 97 && x <= 122 ) {
		return x - 'a' + 'A'; 
	}
	return x;
} */

#define TO_LOWER( x ) \
	(( (x) >= 65 && (x) <= 90 ) ?  (x) + 'a' - 'A' : (x) )
#define TO_UPPER( x ) \
	(( (x) >= 97 && (x) <= 122 ) ? (x) - 'a' + 'A' : (x) )

// converts a whole string, in place
char * g_toupper( char * s ) {
    char * p = s;
    while ( *p ) {
        int n = *p;
        *p++ = TO_UPPER( n );
    }
    return s;
}

char * g_tolower( char * s ) {
    char * p = s;
    while ( *p ) {
        int n = *p;
        *p++ = TO_LOWER( n );
    }
    return s;
}

// splits a string using ch delimiter.  Resultant strings omit the specified
//  character. ie, "Some toes" returns char * R[3] = { "some", "toes", 0 };
//  strings array returned is NULL terminated
//  array of strings, toks returns how many.  Result must be freed.  Correct
//  way is to pass to O_StrSplitFree when finished.  
char ** O_StrSplit( const char *str, int ch, int *toks ) {
    const char * p = str, *e;
    #define SHAVE() while ( *p && *p == ch ) ++p 
    #define SKIP() e = p; while ( *e && *e != ch ) ++e

    char ** pp = NULL;
    *toks = 0;

    do {
        SHAVE();
        if ( *p ) {
            e = p;
            SKIP();
            if ( p != e ) 
                *++toks;
        }
        p = e;
        if ( !*p )
            break;
    } while(1);

    if ( *toks <= 0 ) {
        *toks = 0;
        return NULL;
    }

    pp = (char**) V_Malloc( sizeof(char*) * (*toks+1) );
    memset( pp, 0, sizeof(char*) * (*toks+1) );

    p = str;
    int count = 0;
    do {
        SHAVE();
        if ( *p ) {
            e = p;
            SKIP();
            if ( p != e ) {
                pp[count++] = (char*) V_Malloc( e-p+1 );
                int i = 0;
                while ( p != e ) {
                    pp[count][i++] = *p++;
                }
                pp[count][i] = '\0';
            }
        }
        p = e;
        if ( !*p )
            break;
    } while(1);   
    #undef SHAVE()
    #undef SKIP()
    
    Assert( *toks == count );
    return pp;
}

void O_StrSplitFree( char ** pp ) {
    if ( !pp )
        return;
    int i = 0;
    while ( pp[i] ) {
        V_Free( pp[i++] );
    }
    V_Free( pp );
}


/*
static inline int TOLOWER( int x ) {
	if ( x >= 65 && x <= 90 ) {
		return x + 'a' - 'A'; 
	}
	return x;
} */


/*
======================
 O_strncasecmp
  - case insensitive string compare
======================
*/
int O_strncasecmp ( const char *s, const char *n, int sz )
{
	int i = 0;
	while ( *s && *n && i < sz ) {
		if ( TO_LOWER(*s) != TO_LOWER(*n) )
		{
			return TO_LOWER(*s) - TO_LOWER(*n);
		}
		++s; ++n; ++i;
		if ( *s == '\0' || *n == '\0' ) {
			return TO_LOWER(*s) - TO_LOWER(*n);
		}
	}
	if ( i == sz )
		return 0;

	return TO_LOWER(*s) - TO_LOWER(*n);
}

/* 
======================
 O_pathicmp
  - case insensitive string compare, for system paths.  allows reversible PATH_SEP matches
======================
*/
int O_pathicmp ( const char *s, const char *n ) {
	int i = 0;
	while ( *s && *n ) {
		if ( *s == '/' && *n == '\\' ) 
		{			
		} 
		else if ( *s == '\\' && *n == '/' ) 
		{
		}
		else if ( TO_LOWER(*s) != TO_LOWER(*n) )
		{
			return TO_LOWER(*s) - TO_LOWER(*n);
		}
		++s; ++n; 
		if ( *s == '\0' || *n == '\0' ) {
			return TO_LOWER(*s) - TO_LOWER(*n);
		}
	}

	return TO_LOWER(*s) - TO_LOWER(*n);
}

/*
========================
 O_strcasestr

	find a substring in a larger string, case insensitive
========================
*/
const char * O_strcasestr( const char *haystack, const char *needle ) {
	char hay[1024];
	char pin[1024];

	strcpy( hay, haystack );
	strcpy( pin, needle );

	g_tolower( hay );
	g_tolower( pin );

	char *p = NULL;
	if ( (p = strstr( hay, pin )) )
		return p - hay + haystack;

	return NULL;
}



#if 0
int O_strncasecmp (const char *s, const char *n, int sz)
{
    int i;
    int t, tot;

    // 'a' - 'A' ==> 97 - 65 ==> 32
    // 'z' - 'Z' ==> 122 - 90
    

    tot = i = 0;


    while (i < sz)
    {

        if (n[i] == 0) {
            if (s[i] == 0)
                break;
            else
                tot -= s[i];
        } else if (s[i] == 0) {
                tot += n[i];
        } 
        else 
        {
            t = n[i] - s[i];

            // both are char and are same case
            if ((n[i] >= 97 && n[i] <= 122  && s[i] >= 97 && s[i] <= 122) ||
                (n[i] >= 65 && n[i] <= 90   && s[i] >= 65 && s[i] <= 90))
            {
                tot += t;
            } 
            else 
            {
                // n is char & lower
                if (n[i] >= 97 && n[i] <= 122) 
                {
                    // s is char & upper
                    if (s[i] >= 65 && s[i] <= 90) 
                    {
                        tot += n[i] - (s[i] + 32);
                    }
                    // s is not char (s cant be lower)
                    else
                    {
                        tot += t;
                    }
                } 
                // n is char & is upper
                else if (n[i] >= 65 && n[i] <= 90) 
                {
                    // s is char & lower
                    if (s[i] >= 97 && s[i] <= 122) 
                    {
                        tot += n[i] - (s[i] - 32);
                    }
                    // s is not char (s cant be upper)
                    else
                    {
                        tot += t;
                    }
                }
                else
                {
                    tot += t;
                }
            }
        }

        ++i;
    }

    return tot;
}
#endif

/*
======================
  O_strncpy

  ensure against overruns, but also to make sure string is null 
   terminated no matter what
  returns chars copied _not_ including trailing null;
======================
*/
int O_strncpy ( char *to, const char *from, int sz )
{
    int i = 0;
    int nullset = 0;

//    while (1)
    for (;;)
    {
        if (i >= sz)
            break;

        to[i] = from[i];

        if (from[i] == '\0') {
            nullset = 1;
            break;
        }

        ++i;
    }

    if (!nullset)
        to[sz-1] = '\0';

/*
    if (i >= sz)
        return -1;
*/

    return i;
}

/*
======================
  O_snprintf
======================
*/
int O_snprintf (char *buf, int sz, char *fmt, ...)
{
	va_list args;
	char *p, *sval;
	int ival;
	double dval;
    char tmp[256];
    int i;
    int stoploop = 1;

    char *buf_p = buf;

    // the maximum pointer position exclusive
    int maxp = (int)buf + sz;


	/* make args point to first unamed arg */
	va_start( args, fmt );

    /* loop through format string */
    /* while not at the end of the string and 
     * src buffer not full */
	for (p = fmt; *p && (int)buf_p < maxp; p++) 
    {

        if (*p != '%') {
            *buf_p++ = *p;
			continue;
        }

		switch (*++p) {
		case 'd':
			ival = va_arg (args, int);
			sprintf(tmp, "%d", ival);
            i = 0;
            stoploop = 1;
            while (tmp[i] && stoploop) 
            {
                *buf_p++ = tmp[i++];
                if ((int)buf_p >= maxp)
                    stoploop = 0;
            }
            if (!stoploop)
                continue;
            
			break;
		case 'f':
			dval = va_arg (args, double);
			sprintf(tmp, "%f", dval);
            i = 0;
            stoploop = 1;
            while (tmp[i] && stoploop) 
            {
                *buf_p++ = tmp[i++];
                if ((int)buf_p >= maxp)
                    stoploop = 0;
            }
            if (!stoploop)
                continue;

			break;
		case 's':
			for (sval = va_arg (args, char *); *sval && stoploop; sval++) {
                *buf_p++ = *sval;
                if ((int)buf_p >= maxp)
                    stoploop = 0;
            }
            if (!stoploop)
                continue;

			break;
		default:
            *buf_p++ = *p;
			break;
		}
	}

	va_end(args);

    if ((int)buf_p > maxp) {
        //sys_error_warn ("overrun in C_snprintf");
        buf[sz-1] = '\0';
        return -1;
    } else if ((int)buf_p == maxp) 
        buf[sz-1] = '\0';
    else 
        *buf_p = '\0';

    return (int)buf_p - (int)buf;
}

/*
====================
 O_strstr
====================
*/
char * O_strstr( const char *haystack, const char *needle ) 
{
    const char *hp;
    const char *hp2;
    const char *np;
	int nlen;

    if ( !needle || !haystack )
        return NULL;

    nlen = strlen ( needle );
	hp = haystack;

    while ( *hp ) 
    { 
        np = needle;
        hp2 = hp;
        while ( *np && *np == *hp2 ) {
			++hp2;
            ++np;
            if ( np - needle  >= nlen ) {
                return (char *)hp;
            }
        }
        ++hp;
    }
    return NULL;
}

/*
====================
 FUNC_memset
    - part of O_memset
====================
*/
#if !defined( __INLINE_MEMSET__ ) && !defined( __SYSTEM_MEMSET__ )
void FUNC_memset( void *loc, int val, int howmany )
{
    byte set, *mem, *end;

    if ( howmany < 1 || !loc )
        return;
   
    set = (byte)(0xFF & val);
    mem = (byte *) loc;
    end = (byte *)( (int)loc + howmany );

    while ( mem != end ) {
        *mem++ = set;
    }
}
#endif

/*
====================
 O_FindMemChunk
====================
*/
// returns a pointer to the first byte in the supplied chunk or null
// does not match the trailing zero in chunk
const byte * O_FindMemChunk( const void *_buf, int buflen, const void *_chunk, int chunklen ) {
    const byte *bp, *cp, *bp2;
    const byte *buf = (byte *)_buf;
    const byte *chunk = (byte *)_chunk;
    bp = (byte *) buf;
    while ( bp - buf < buflen ) {
        bp2 = bp;
        cp = (byte *)chunk;
        while ( *cp && *cp == *bp2 ) {
            ++cp; ++bp2;
            if ( cp - chunk >= chunklen )
                return bp;
        }
        ++bp;
    }
    return NULL;
}

char * O_strrchr( const char *buf, char s )
{
    int len;
    const char *p;

    len = strlen(buf);
    if ( len < 1 )
        return NULL;
    p = &buf[len-1];

    do {
        if ( *p == s )
            return (char *)p;
        --p;
    } while ( p >= buf ) ;
    
    return NULL;
}

/*
====================
 O_strlen
    - it may be better to write as array indices than the cockier 
      pointer arithmetic...
====================
*/
/*
int O_strlen( const char *s )
{
    const char *t = s;
    if ( !s )
        return 0;
    while ( *t )
        ++t;
    return (int)(t - s);
} */
int O_strlen( const char *s )
{
    register int i = 0;
    if ( !s )
        return 0;
    while ( s[i] )
        ++i;
    return i;
}

// looks for both kinds of path seperator
const char * strip_path( const char *s ) {
	const char *w = strrchr ( s, '\\' );
	const char *u = strrchr ( s, '/' );

	if ( !w && !u )
		return s;

	if ( w && u )
		return s;

	if ( w && !u ) {
		return (const char *)(w+1);
	}

	if ( !w && u ) {
		return (const char *)(u+1);
	}

	return s;
}

// returns a different buffer than passed in
const char * strip_extension( const char *s ) {
    static char *buf;
    static int  len;
    if ( !s || !s[0] ) {
        if ( buf )
            buf[0] = 0;
        return buf;
    }
    // late init, never gets called, never uses memory
    if ( !buf || strlen(s) + 1 > len ) {
        if ( buf )
            V_Free( buf );
        len = strlen(s) + 1;
        buf = (char*) V_Malloc( len );
    }

    const char *p = strrchr( s, '.' );
    if ( !p )
        return s;
    memcpy( buf, s, p - s );
    buf[p-s] = '\0';
    return buf;
}

// returns a different buffer than passed in
const char * homogenize( const char * s ) {
    const char *nopath = strip_path( s );
    const char *no_ext = strip_extension( nopath );
    return no_ext;
}

const char *Com_GetGamePath( void );
const char *Com_GetGameName( void );



// similar, but this one only strips fs_gamepath off, leaving us with the 
//  relative path
const char * strip_gamepath( const char *src ) {
	const char *gamepath = Com_GetGamePath();
	const char *game = Com_GetGameName();
	if ( !gamepath || !src )
		return NULL;

	unsigned int plen = strlen ( gamepath );
	unsigned int glen = strlen ( game );

	const char *e = strstr( src, gamepath );

	// no path, try game only
	if ( !e ) {
		const char *f = strstr( src, game );
		// no game, then keep what we already have
		if ( !f ) {
			return src;
		}

		f += glen;
		if ( *f == '/' || *f == '\\' )
			++f;
		return f;
	}

	e += plen;
	if ( *e == '/' || *e == '\\' )
		++e;
	return e;
}
