
/* clib.cpp
   - c library string calls replacement

FIXME: Note: this file page is getting deprecated because I re-wrote all these
        for another project, fixing a number of bugs along the way.  besides, you should just use glibc, crt, or whatever.  
 */


#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "lib.h"

/* #ifdef _WIN32
# include <process.h> // for exit() ?
#endif
*/


char * va_string (char *fmt, ...)
{
    static char buf[0x10000];
    va_list args;

    memset(buf, 0, sizeof(buf));
    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);

    return &buf[0];
}


int C_strncmp (const char *s, const char *n, int sz)
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
}

/*
======================
 C_strncasecmp
  - case insensitive string compare
======================
*/
int C_strncasecmp (const char *s, const char *n, int sz)
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





/*
======================
  C_strncpy

  ensure against overruns, but also to make sure string is null 
   terminated no matter what
======================
*/
int C_strncpy ( char *to, const char *from, int sz )
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

    if (i >= sz)
        return -1;

    return sz;
}


/*
======================
  C_snprintf
======================
*/
int C_snprintf (char *buf, int sz, char *fmt, ...)
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

char * C_strstr( const char *haystack, const char *needle ) 
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

#if !defined( __INLINE_MEMSET__ ) && !defined( __SYSTEM_MEMSET__ )
void C_memset( void *loc, int val, int howmany )
{
    byte *b, *mem, *end;

    if ( howmany < 1 || !memloc )
        return;
   
    b = (byte)(0xFF & val);

    mem = (byte *) loc;
    end = (byte *)( (int)loc + howmany );

    while ( mem != end ) {
        *mem++ = b;
    }
}
#endif


