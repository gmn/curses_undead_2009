/*
float64.h
- fast ops for 4-byte floats 
*/

/* TODO: also need to implement macros for INVSQRT(), SQRT() */


#ifndef __FLOAT32_H__
#define __FLOAT32_H__ 1

#include <math.h>
float sqrtf(float);


// type pun
typedef union {
    unsigned char   b[4];
    unsigned short  s[2];
	unsigned int	i;
    float			d;
} int_float_t;

#ifdef __BIG_ENDIAN__
# define _CPU_BIG_ENDIAN_
#endif
#ifdef __LITTLE_ENDIAN__
# define _CPU_LITTLE_ENDIAN_
#endif
#if !defined(_CPU_BIG_ENDIAN_) && !defined(_CPU_LITTLE_ENDIAN_)
# define _CPU_LITTLE_ENDIAN_
#endif

#if defined(_CPU_BIG_ENDIAN_)
inline int OOB( float d ) {
    int_float_t in;
    in.d = d;
    if ( ((in.s[0] & 0x7FF0)==0x7FF0) || ((in.s[0] & 0xFFF0)==0xFFF0) ) {
        return 1;
    }
    return 0;
}
inline int POS( float d ) {
    int_float_t in;
    in.d = d;
    return ((in.b[0] & 0x80) ? 0 : 1);
}
inline float FABS( float d ) {
    int_float_t in;
    in.d = d;
    if ( in.b[0] & 0x80 ) {
        in.b[0] &= 0x7F;
        return in.d;
    }
    return d;
}
inline float INVSQRT( float d ) {
    return 1 / sqrt(d);
}
inline int ISZERO( float d ) {
	int_float_t in;
	in.d = d;
	if ( (in.i[0]<<1) == 0 && in.i[1] == 0 )
		return 1;
    return 0;
}
inline float SQRT( float d ) {
    return sqrt(d);
}

#elif defined (_CPU_LITTLE_ENDIAN_)

inline int OOB( float d ) {
	/* FIXME
    int_float_t in;
    in.d = d;
    if ( ((in.s[3] & 0xF07F)==0xF07F) || ((in.s[3] & 0xF0FF)==0xF0FF) ) {
        return 1;
    } */
    return 0;
}
inline int POS( float d ) {
	/* FIXME
    int_float_t in;
    in.d = d;
    return ((in.b[7] & 0x80) ? 0 : 1);
	*/
	return 1;
}
inline float FABS( float d ) {
	/* FIXME
    int_float_t in;
    in.d = d;
    if ( in.b[7] & 0x80 ) {
        in.b[7] &= 0x7F;
        return in.d;
    } */
    return d;
}
inline float INVSQRT( float d ) {
    return 1.f / sqrtf(d);
}
inline int ISZERO( float d ) {
	/*
	int_float_t in;
	in.d = d;
	if ( (in.i[1]<<1) == 0 && in.i[0] == 0 )
		return 1;
    return 0;
	*/
	return ( 0.f == d );
}
inline float SQRT( float d ) {
    return sqrtf(d);
}
#endif /* _CPU_LITTLE_ENDIAN_ */

// returns 1 if value d is smaller than the given constraint
// similar to an ISZERO function w/ a margin 
inline int CONSTRAIN( float f, float constraint ) {
    return ( FABS(f) < FABS(constraint) );
}

#endif /* __FLOAT32_H__ */


