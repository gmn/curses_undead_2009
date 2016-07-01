
#include "lib.h"
#include "../common/com_vmem.h"

#include <iostream>
#include <stdlib.h>


////////////////////////////////////////////////////////////////////////
//
// G Classes - my interlocked class hierarchy
//
// hashtable class (depend: linked-list,  )
// doubly linked-list class
// autostring class
// vmem allocator class
//      adding memory partitioning to the memory manager so that I can
//      group different class of memory users into different spaces.  should
//      name the spaces, and provide different allocator pointers, for the
//      different partitions, although one V_Free should work, and be smart
//      enough to figure out where the memory belonged to w/o having to be 
//      told
//      
// gvar class (depend: hashtable, autostring)
//
// types: see top (gwchar_t, gbool, byte)
//
/////////////////////////////////////////////////////////////////////////

// explicit constructor
void autostring::init( void ) {
    getmem = V_Malloc;
    freemem = V_Free;
    basicInit( 0 );
    zero();
}
void autostring::init( const char *in ) {
    getmem = V_Malloc;
    freemem = V_Free;
    basicInit( len( in ) + 1 );
    setNoMod( in );
}
void autostring::init( const gwchar_t *in ) {
    getmem = V_Malloc;
    freemem = V_Free;
    basicInit( len( in ) + 1 );
    setNoMod( in );
}
// explicit destructr
void autostring::shutdown ( void ) {
    freemem( __str );
}

// auto constructor
autostring::autostring( void ) {
    getmem = V_Malloc;
    freemem = V_Free;
    basicInit(0);
    zero();
}
autostring::autostring( const char *in ) {
    getmem = V_Malloc;
    freemem = V_Free;
    basicInit( len( in ) + 1 );
    setNoMod( in );
}
autostring::autostring( const gwchar_t *in ) {
    getmem = V_Malloc;
    freemem = V_Free;
    basicInit( len( in ) + 1 );
    setNoMod( in );
}
autostring::autostring( void*(*getter)(size_t) ) {
    getmem = getter;
    freemem = V_Free;
    basicInit(0);
    zero();
}
autostring::autostring( const char *in, void*(*getter)(size_t) ) {
    getmem = getter;
    freemem = V_Free;
    basicInit( len( in ) + 1 );
    setNoMod( in );
}
autostring::autostring( const gwchar_t *in, void*(*getter)(size_t) ) {
//std::wcerr << "in constructor\n";
    getmem = getter;
    freemem = V_Free;
    basicInit( len( in ) + 1 );
    setNoMod( in );
}
// ~autostring
autostring::~autostring() {
    if ( __str && freemem ) {
        freemem ( __str );
        __str = NULL;
    }
}

// basicInit
// sz is in units of 'characters', including the null, so at least a
//  number of sz characters must be allocated for
void autostring::basicInit( int sz ) {
//std::wcerr << "in basicInit\n";
    __strlength = 0;
    __chunksize = AUTOSTRING_BASE_LENGTH_BYTES;

    if ( sz * sizeof(gwchar_t) > __chunksize )
        __chunksize = pow2above( sz * sizeof(gwchar_t) );

    __modified = gfalse;
    __timesModified = 0;
    __str = (gwchar_t *) getmem( __chunksize );
}

// setNoMod
void autostring::setNoMod( const char *in ) {
    int b;

    b = len( in );

    // resize and set chunksize if needed
    if ( b * 2 + 2 > __chunksize ) {
        __chunksize = _resize( pow2above( b * 2 + 2 ) );
    }

    __strlength = b;

    _as_wstrncpyUP( __str, in, __chunksize );
}
void autostring::setNoMod( const gwchar_t *in ) {
    int n, b;
    int m;

    b = n = len( in );
    n *= 2;

    // resize and set chunksize if needed
    if ( n + 2 > __chunksize ) {
        m = pow2above ( n + 2 );
        _resize( m );
        __chunksize = m;
    }

    __strlength = b;

    // pass in chunksize, so if all else fails, there will be a null
    //  terminator at the last character of the chunk
    _as_wstrncpy( __str, in, __chunksize );
}

// argument in bytes, not characters
int autostring::_resize( int newchunksize )
{
    if ( newchunksize < AUTOSTRING_BASE_LENGTH_BYTES )
        newchunksize = AUTOSTRING_BASE_LENGTH_BYTES;
    freemem( (void *) __str );
    __str = (gwchar_t *) getmem ( newchunksize );
    return newchunksize;
}

// as_wstrncpy : private function, used by class only.  do not copy more than 
//  sz characters, ensure string is null terminated
int autostring::_as_wstrncpy ( gwchar_t *dst, const gwchar_t *src, int sz )
{
    int i = 0;
    int nullset = 0;

    for (;;)
    {
		if ( i >= sz ) 
			break;

        dst[i] = src[i];

        if (src[i] == '\x0000') {
            nullset = 1;
            ++i;
            break;
        }
        ++i;
    }

    // overwrite the last char if hit the end w/o writing a null terminator
	if (i > sz && !nullset) {
        dst[sz-1] = '\x0000';
		nullset = gtrue;
	}

	// set null 1 character after length specified by sz
	if (!nullset) {
		dst[i] = '\x0000';
	}

    if (i > sz)
        return -1;

    return i;
}
int autostring::_as_wstrncpyUP ( gwchar_t *dst, const char *src, int sz )
{
    int i = 0;
    int nullset = 0;

    for (;;)
    {
		if ( i >= sz )
			break;

        dst[i] = (gwchar_t) src[i];

        if (src[i] == '\0') {
            nullset = 1;
            ++i;
            break;
        }
        ++i;
    }

	// overwrite the last char if hit the end w/o writing a null terminator
	if (i > sz && !nullset) {
        dst[sz-1] = '\0';
		nullset = gtrue;
	}

	// set null 1 character after length specified by sz
	if (!nullset) {
		dst[i] = '\0';
	}

    if (i > sz)
        return -1;

    return i;
}

// set
void autostring::set ( const char * newstr )
{
    __timesModified++;
    __modified = gtrue;
    setNoMod( newstr );
}
void autostring::set ( const gwchar_t * newstr )
{
    __timesModified++;
    __modified = gtrue;
    setNoMod( newstr );
}

// toString - returns a gwchar_t string of the contents.
//  must be freed by caller
gwchar_t *autostring::toString ( void ) 
{
    gwchar_t *tmp;
    tmp = (gwchar_t *) getmem( len()+2 );
    _as_wstrncpy ( tmp, __str, len() );
    return tmp;
}

// zero : FIXME : use C_memset
void autostring::zero( void ) 
{
    int i = 0;

    // FIXME:
    /*
	int align4 = ( __chunksize + 3 ) & ~3;
	align4 /= 4;
    */

    unsigned long long _align = __chunksize / sizeof(unsigned long long);

    while ( i < _align )
        ((unsigned long long *)__str)[i++] = 0;
}

// clone : creates a clone of this string and returns pointer to it
autostring* autostring::clone( void )
{
    autostring *p       = (autostring *) getmem ( sizeof(autostring) );

    p->getmem = this->getmem;
    p->freemem = this->freemem;

    // set up the chunksize, allocates it, 
    p->__chunksize = pow2above( (len()+1)*sizeof(gwchar_t) );
    if ( p->__chunksize < AUTOSTRING_BASE_LENGTH_BYTES )
        p->__chunksize = AUTOSTRING_BASE_LENGTH_BYTES;
    p->__str = (gwchar_t *) p->getmem( p->__chunksize );

    p->__modified         = this->__modified;
    p->__timesModified    = this->__timesModified;

    // sets strlen and copies string data
    p->setNoMod( getcp() );
    
    return p;
}

autostring* autostring::substr( int start, int length )
{
    return substr( *this, start, length );
}

// substr
autostring* autostring::substr( autostring& asr, int start, int length ) 
{
    autostring *p = (autostring *) asr.getmem ( sizeof(autostring) );

    p->getmem = this->getmem;
    p->freemem = this->freemem;
    p->__modified = gfalse;
    p->__timesModified = 0;

    
    // substring length < 0 means ignore length to copy and goto end of 
    //  input string from start
    if ( length < 0 ) {
        if ( start < 0 ) 
            start = 0;
        if ( start > asr.len() - 1 ) {
            goto __default_substring;
        }
        p->__strlength = asr.len() - start;

    // however, length == 0 is taken seriously and the resultant string has
    //  zero length and is set to defaults
    // if start is off the right length boundary of the input string return
    //  an empty string as well
    } else if ( length == 0 || start > asr.len()-1 ) {
    __default_substring:
        p->__strlength = 0;
        p->__chunksize = AUTOSTRING_BASE_LENGTH_BYTES;
        p->__str = (gwchar_t *) p->getmem( p->__chunksize );
        p->zero();
        return p;

    // start must be less than end of input string
    // length must be greater than zero
    } else {
        if ( start < 0 ) 
            start = 0;

        if ( length + start > asr.len() ) {
            p->__strlength = asr.len() - start;
        } else {
            p->__strlength = length;
        }
    }

    p->__chunksize = pow2above( p->__strlength );
    if ( p->__chunksize < AUTOSTRING_BASE_LENGTH_BYTES )
        p->__chunksize = AUTOSTRING_BASE_LENGTH_BYTES;
    p->__str = (gwchar_t *) p->getmem( p->__chunksize );

	p->zero();

	gwchar_t *_offset_start = asr.getp();
	_offset_start += start;
    _as_wstrncpy(   p->__str, 
                    (const gwchar_t *)_offset_start, 
                    p->__strlength );

    return p;
}

// cmp
int autostring::cmp ( const char *s ) const
{
    const gwchar_t * n = (const gwchar_t *) __str;
    int diff = 0;

    while (*s || *n) {
        diff += (int)*n - (int)((gwchar_t)*s);

        if (*s) ++s;
        if (*n) ++n;
    }
    return diff;
}
int autostring::cmp ( const gwchar_t *s ) const
{
    const gwchar_t * n = (const gwchar_t *) __str;
    int diff = 0;

    while (*s || *n) {
        diff += (int)*n - (int)*s;

        if (*s) ++s;
        if (*n) ++n;
    }
    return diff;
}
int autostring::cmp ( const autostring *asp ) const
{
    const gwchar_t * n = (const gwchar_t *) __str;
    const gwchar_t * s = asp->getcp();
    int diff = 0;

    while (*s || *n) {
        diff += (int)*n - (int)*s;

        if (*s) ++s;
        if (*n) ++n;
    }
    return diff;
}

// icmp : FIXME
int autostring::icmp ( const char *s ) const 
{
    const gwchar_t * n = (const gwchar_t *) __str;
    int diff = 0;
    return diff;
}
int autostring::icmp ( const gwchar_t *s ) const 
{
    const gwchar_t * n = (const gwchar_t *) __str;
    int diff = 0;
    return diff;
}
int autostring::icmp ( const autostring *asp ) const 
{
    const gwchar_t * n = (const gwchar_t *) __str;
    const gwchar_t * s = asp->getcp();
    int diff = 0;
    return diff;
}

// autostring class specific operators 
std::wostream& operator<< ( std::wostream& os, const autostring& p ) {
    return os << p.getcp();
}
std::wostream& operator<< ( std::wostream& os, const autostring *p ) { 
    return os << p->getcp(); 
} 

// TODO : implement these
// strstr
// stristr

// reset
void autostring::reset( void ) {
    __chunksize = _resize( 0 );    
    __strlength = 0;
    __modified = gfalse;
    __timesModified = 0;
    zero();
}
 


//
//
//  end autostring
//////////////////////////////////////////////////////////////////////



