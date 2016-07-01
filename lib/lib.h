#ifndef __LIB_H__
#define __LIB_H__

#include <iostream>
#include <stdlib.h>

int Com_Millisecond( void );

//
// lib_strf.cpp
// 
// 8-bit char string functions
char * va_string (char *, ...);
int C_strncmp (const char *s, const char *n, int sz);
int C_strncasecmp (const char *s, const char *n, int sz);
int C_strncpy (char *to, const char *from, int sz);
int C_snprintf (char *buf, int sz, char *fmt, ...);
char * C_strstr( const char *, const char * );

// NOTE: i just wanted to write a duff's device.  
#undef __INLINE_MEMSET__ 
#define __SYSTEM_MEMSET__


#ifdef __INLINE_MEMSET__
#define C_memset(memloc,val,howmany)                \
do {                                                \
    byte __b, *__mem, *__end, __j;                  \
    __b = (byte)(0xFF & val);                       \
    __mem = (byte *) memloc;                        \
    __end = (byte *)( (int)memloc + howmany );      \
    __j = (byte)(( (int)__end-(int)__mem ) % 8);    \
    if ( howmany < 1 ) __j = 0xFF;                  \
    switch (__j) {                                  \
        case 0: for(;;) { *__mem++ = __b;           \
        case 7:           *__mem++ = __b;           \
        case 6:           *__mem++ = __b;           \
        case 5:           *__mem++ = __b;           \
        case 4:           *__mem++ = __b;           \
        case 3:           *__mem++ = __b;           \
        case 2:           *__mem++ = __b;           \
        case 1:           *__mem++ = __b;           \
                          if (__mem == __end)       \
                              break;                \
                }                                   \
        case 0xFF: break;                           \
    }                                               \
} while(0)
#endif

#ifdef __SYSTEM_MEMSET__
#define C_memset memset
#endif

#if !defined(__INLINE_MEMSET__) && !defined(__SYSTEM_MEMSET__)
void C_memset( void *, int, int );
#endif


#include "../common/com_types.h"


//
// lib_wstrf.cpp
//
// 16-bit wide char string functions
int C_wstrlen ( const gwchar_t *s );
int C_wstrncmp (const gwchar_t *s, const gwchar_t *n, int sz);
int C_wstrncasecmp (const gwchar_t *s, const gwchar_t *n, int sz);
int C_wstrcpy (gwchar_t *to, const gwchar_t *from);
int C_wstrncpy (gwchar_t *to, const gwchar_t *from, int sz);
int C_wsnprintf (gwchar_t *buf, int sz, char *fmt, ...);
int C_wstrncpyUP ( gwchar_t *dst, const char *src, int max );
int C_wstrncpyDOWN ( char *dst, const gwchar_t *src, int max );



//
// lib_stack.cpp
//
class gstack {
    private:
        int total;

        struct elt_s {
            void * value;
            struct elt_s * next;
            struct elt_s * prev;

            /* illegal copy constructor 
            elt_s (elt_s n) {
                prev = next = NULL;
                value = n.value;
                if (n) {
                    next = n.next;
                    prev = n.prev;
                } 
            }
            */
        };

        // head never equals anything, just hangs onto the top
        struct elt_s head;

    public:
        gstack ( void );
        ~gstack ( void );

        void push ( void * );

        // returns NULL when there's nothing left
        void * pop ( void );

        int getSize ( void );

        void * operator[] ( int ) const;
		void * popVal ( void * );

        //void insert_after ( int, void * );
        //void * peek ( int ) const;
};

/////////////////////////////////////////////////////////////////////////////
//  lib_linkedlist.cpp
/////////////////////////////////////////////////////////////////////////////
// #include "lib_linkedlist.h"

/////////////////////////////////////////////////////////////////////////////
//  lib_hashtable.cpp
/////////////////////////////////////////////////////////////////////////////
#include "lib_hashtable.h"

//////////////////////////////////////////////////////////////////////
// lib_autostring.cpp
//////////////////////////////////////////////////////////////////////
// autostring class
//
//
// my own string class.  stores data internally.  starts at the length
//  specified by AUTOSTRING_BASE_LENGTH_BYTES.  can be read, or overwritten.
//  all data internally is stored in gwchar_t format which is typedef unsigned
//  short.  if they are overwritten with a new str value that is longer than
//  the currently allocated memory, the memory is freed and a new chunk twice
//  the size is fetched, and the string is written into that.
//
// the goal of this class is that it is meant to be simple and to only 
//  support the functionalities that I find I commonly need or use.  also,
//  it supports taking string input as simple quoted strings that a compiler
//  understands and casting them into 16-bit strings, future language 
//  extensions should be much easier.
//


// 127 double-byte chars and a terminator
#define AUTOSTRING_BASE_LENGTH_BYTES 256

class autostring {

public:
    // explicit initialization instead of using the automatic
    //  constructor mechanism
    void init( void );
    void init( const char * );
    void init( const gwchar_t * );
    // explicit shutdown and memory return
    void shutdown( void );

    // stores new string into, gets strlen of incoming and doubles the 
    //  length of the internal buffer if necessary 
    void set( const char * );
    void set( const gwchar_t * );

    // sets w/o setting the modified flag or counter
    // FIXME: should this be private?  do I care?
    void setNoMod( const char * );
    void setNoMod( const gwchar_t * );

    // returns pointer to string data. its write-able. who cares. be careful
    gwchar_t * getp ( void ) const { return __str; }
    // ok, here's a const one
    const gwchar_t * getcp ( void ) const { return (const gwchar_t *)__str; }

    // len
    int len( void ) const { return __strlength; }
    // computes the length of the string argument in characters, 
    //  not including the null terminator, so someword.len() of "length" is 6.
    // returns same value ( this is not the same as bytes! ) no matter 
    //  whether the string is wide-character or 1-byte-per-character
    int len( const char *s ) const {
        int i = 0;
        while (s[i] != '\0') i++;
        return i; }
    int len( const gwchar_t *s ) const {
        int i = 0;
        while (s[i] != '\x0000') i++;
        return i; }

    // compare the string internally stored with one passed in
    // return 0 if identical
    int cmp( const char * ) const;
    int cmp( const gwchar_t * ) const;
    int cmp( const autostring * ) const;

    // case independant string comparison
    int icmp( const char * ) const;
    int icmp( const gwchar_t * ) const;
    int icmp( const autostring * ) const;

    // strstr: does this string contain any occurrenes of the string 
    //  argument.  
    gbool strstr ( const char * ) const;
    gbool strstr ( const gwchar_t * ) const;
    gbool strstr ( const autostring * ) const;

    // stristr: same as above, needle haystack except that it is a case-
    //  indepent match we're looking for, so "ZeUs" matches "zEuS" 
    gbool stristr ( const char * ) const;
    gbool stristr ( const gwchar_t * ) const;
    gbool stristr ( const autostring * ) const;

    void operator= ( const char *s ) { set( s ); }
    void operator= ( const gwchar_t *s ) { set( s ); }
    gbool operator== ( const autostring *n ) {
        if (!cmp(n->getcp()))
            return gtrue;
        return gfalse;
    } 
    gbool operator== ( const autostring &n ) {
        if (!cmp(n.getcp()))
            return gtrue;
        return gfalse;
    } 

    // returns an allocated copy of the internal string.  must be freed
    //  by the caller.  fixme: should have a const on the end, but the 
    //  compiler threw a stupid-ass error that said that converting the 'this'
    //  pointer from a 'const autostring' to a 'autostring &', conversion
    //  loses its qualifier.  fucking stupid ass rules.  
    gwchar_t * toString( void );

    // copy constructor
    autostring* clone( void );

    // substr: ( string, start, length )
    autostring* substr( int, int =0 );
    autostring* substr( autostring& , int , int =0 );

    void reset( void );

    int pow2above( int n ) { 
        int i = 1;
        while ( i < n ) i <<= 1;
        return i; }

    // constructor/destructor
    autostring();
    autostring( const char * );
    autostring( const gwchar_t * );
    ~autostring();

    // allocator constructors
    autostring( void *(*getter)(size_t) );
    autostring( const char *, void *(*getter)(size_t) );
    autostring( const gwchar_t *, void *(*getter)(size_t) );


private:

    // ____THE_STRING____
    gwchar_t *__str;

    // does all the real work, lengths should already be verified
    int _as_wstrncpy  ( gwchar_t *, const gwchar_t *, int );
    int _as_wstrncpyUP( gwchar_t *, const char *, int );

    // this resizes the memory, frees what's attached to string and 
    //  re-mallocs the value passed in, doesn't save copies of anything
    // returns the new chunk size
    int _resize ( int = 0 );

protected:

    void basicInit( int sz );

    // internal allocator pointers
    void *(*getmem)(size_t);
    void (*freemem)(void *);

    int     __strlength;
    int     __chunksize;  // the size of str in bytes, not number characters
    gbool   __modified;
    int     __timesModified;

    // zeros out entire chunk
    void zero ( void );
};


// autostring class related operators
std::wostream& operator<< ( std::wostream& os, const autostring& p ); 
std::wostream& operator<< ( std::wostream& os, const autostring *p );
 
#define freeAutoStringP(m)          \
            do {                    \
            if ( m ) {              \
                m->shutdown();      \
                V_Free(m);          \
                m = NULL;           \
            }                       \
            } while(0)

//
/////////////////////////////////////////////////////////////////////////////                        
#include "lib_timer.h" // I'm getting bloody fucking sick of this


#include "lib_image.h"
//#include "lib_lists.h"
#include "lib_list_c.h"

// lib_cstrings.cpp
const char * va ( char * , ... );
int O_strncmp ( const char *, const char *, int );
int O_strncasecmp( const char *, const char *, int );
int O_pathicmp( const char *, const char * );
int O_strncpy ( char *, const char *, int );
int O_snprintf ( char *, int, char *, ... );
char * O_strstr ( const char *, const char * );
const byte * O_FindMemChunk( const void *, int, const void *, int );
char * O_strrchr ( const char *, char );
int O_strlen ( const char * );
const char * O_strcasestr( const char *haystack, const char *needle );

const char * strip_path( const char *s );
const char * strip_extension( const char *s );
const char * strip_gamepath( const char *s );
char * homogenize( char * s );

char ** O_StrSplit( const char *, int, int * );
void O_StrSplitFree( char ** ); 
char * g_toupper( char * ); 
char * g_tolower( char * ); 


// lib_math.cpp
#include <math.h> // sqrtf
void M_TranslateRotate2d( float *, float *, float );
void M_Translate2d( float *, float * );
void M_Rotate2d( float *, float );
void M_MultVec4( float *, float *, float * );
inline float M_DotProduct2d( float *a, float *b ) {
	return a[0] * b[0] + a[1] * b[1];
}
inline float M_Magnitude2d( float *a ) {
	return sqrtf( a[0] * a[0] + a[1] * a[1] );
}
// FIXME: note, so much faster if you write the macro for INVSQRT and muliply
inline float M_Normalize2d( float *a ) {
	float n = sqrtf( a[0] * a[0] + a[1] * a[1] );
	a[0] /= n;
	a[1] /= n;
	return n;
}
inline void VecMult2d( float *a, float *b, float *c ) {
	a[0] = b[0] * c[0];
	a[1] = b[1] * c[1];
}
inline void VecMultScalar2d( float *a, float *b, float s ) {
	a[0] = b[0] * s;
	a[1] = b[1] * s;
}
void M_CrossProd2d( float *, float *, float * );
void M_CrossProd3d( float *, float *, float * );
void M_TranslateRotate2DVertSet( float *, float *, float );
float M_GetAngleDegrees( float *r1, float *r2 );
float M_GetAngleRadians( float *r1, float *r2 );
void M_ClampAngle360( float * );

// macros for doubles and floats
#include "lib_float32.h" // <-- finish this!
#include "lib_float64.h"

// ftc.c
void LIB_ftc_command( const char * );

#endif /* __LIB_H__ */


