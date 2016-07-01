
/* gwclib.cpp
   - wide char c library string calls replacement
 */


#define _CRT_SECURE_NO_DEPRECATE


#include "../common/com_types.h"
#include "../common/com_suppress.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>


/*
 * strcpy, promoting 8-bit strings to gwchar_t strings
 *  where max pertains to the count of 8-bit chars in src
 *  and not abs size of either
*/
int C_wstrncpyUP ( gwchar_t *dst, const char *src, int max )
{
    int count = 0;

    if (max < 0)
        return -1;
    if (max == 0)
        return 0;

    do {
        if (count >= max)
            break;
        *(dst++) = (gwchar_t) *(src++);
        count++;
    } while (*src);

    if (count > max)
        return -1;

    // hit end of string, null terminate anyway
    if (count == max) {
        *--dst = '\x0000';
        return max;
    }

    *dst = '\x0000';
    return count;
}

int C_wstrncpyDOWN ( char *dst, const gwchar_t *src, int max )
{
    int count = 0;

    if (max < 0)
        return -1;
    if (max == 0)
        return 0;

    do {
        if (count >= max)
            break;
        *(dst++) = (char) *(src++);
        count++;
    } while (*src);

    if (count > max)
        return -1;

    // hit end of src string, null terminate dst anyway
    if (count == max) {
        *--dst = '\0';
        return max;
    }

    *dst = '\0';
    return count;
}


int C_wstrlen ( const gwchar_t *s )
{
    int i = 0;

    while (s[i] != '\x0000')
        i++;

    return i;
}

#if 0
int C_wstrncmp (const gwchar_t *s, const gwchar_t *n, int sz)
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
 C_wstrncasecmp
  - case insensitive string compare
======================
*/
int C_wstrncasecmp (const gwchar_t *s, const gwchar_t *n, int sz)
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
  C_wstrncpy

  ensure against overruns, but also to make sure string is null 
   terminated no matter what
  return n chars written, counting null
======================
*/
int C_wstrncpy (gwchar_t *to, const gwchar_t *from, int sz)
{
    int i = 0;
    int nullset = 0;

    for (;;)
    {
        if (i >= sz)
            break;

        to[i] = from[i];

        if (from[i] == '\x0000') {
            nullset = 1;
            ++i;
            break;
        }
        ++i;
    }

    if (!nullset)
        to[sz-1] = '\x0000';

    if (i >= sz)
        return -1;

    return i;
}


int C_wstrcpy ( gwchar_t *to, const gwchar_t *from )
{
    int i = 0;
    int sz;
    int nullset = 0;

    for (;;)
    {
        to[i] = from[i];

        if (from[i] == '\x0000') {
            nullset = 1;
            ++i;
            break;
        }
        ++i;
    }

    sz = C_wstrlen( from );

    if (!nullset) 
        to[sz-1] = '\x0000';

    if (i >= sz)
        return -1;

    return i;
}


#if 0
/*
======================
  C_wsnprintf
======================
*/
int C_wsnprintf (gwchar_t *buf, int sz, char *fmt, ...)
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

#endif



