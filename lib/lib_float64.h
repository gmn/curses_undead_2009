/*
float64.h
- fast ops for 64-bit floats (doubles)
*/

/* TODO: also need to implement macros for INVSQRT() and ISZERO() */

/** 
** +INF     7FF0000000000000     7F800000
** -INF     FFF0000000000000     FF800000
**  NaN     7FF0000000000001     7F800001
**                to               to
**          7FFFFFFFFFFFFFFF     7FFFFFFF
**                and              and
**          FFF0000000000001     FF800001
**                to               to
**          FFFFFFFFFFFFFFFF     FFFFFFFF
 */

#ifndef __FLOAT64_H__
#define __FLOAT64_H__ 1

#include <math.h>
double sqrt(double);




// type pun
typedef union {
    unsigned char   b[8];
    unsigned short  s[4];
	unsigned int	i[2];
    double          d;
} int_double_t;

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
inline int OOBd( double d ) {
    int_double_t in;
    in.d = d;
    if ( ((in.s[0] & 0x7FF0)==0x7FF0) || ((in.s[0] & 0xFFF0)==0xFFF0) ) {
        return 1;
    }
    return 0;
}
inline int POSd( double d ) {
    int_double_t in;
    in.d = d;
    return ((in.b[0] & 0x80) ? 0 : 1);
}
inline double FABSd( double d ) {
    int_double_t in;
    in.d = d;
    if ( in.b[0] & 0x80 ) {
        in.b[0] &= 0x7F;
        return in.d;
    }
    return d;
}
inline double INVSQRTd( double d ) {
    return 1 / sqrt(d);
}
inline int ISZEROd( double d ) {
	int_double_t in;
	in.d = d;
	return (in.i[0]<<1) == 0 && in.i[1] == 0 ;
}
inline double SQRTd( double d ) {
    return sqrt(d);
}

#elif defined (_CPU_LITTLE_ENDIAN_)

inline int OOBd( double d ) {
    int_double_t in;
    in.d = d;
    if ( ((in.s[3] & 0xF07F)==0xF07F) || ((in.s[3] & 0xF0FF)==0xF0FF) ) {
        return 1;
    }
    return 0;
}
inline int POSd( double d ) {
    int_double_t in;
    in.d = d;
    return ((in.b[7] & 0x80) ? 0 : 1);
}
inline double FABSd( double d ) {
    int_double_t in;
    in.d = d;
    if ( in.b[7] & 0x80 ) {
        in.b[7] &= 0x7F;
        return in.d;
    }
    return d;
}
inline double INVSQRTd( double d ) {
    return 1.0 / sqrt(d);
}
inline int ISZEROd( double d ) {
	int_double_t in;
	in.d = d;
	return (in.i[1]<<1) == 0 && in.i[0] == 0 ;
}
inline double SQRTd( double d ) {
    return sqrt(d);
}
#endif /* _CPU_LITTLE_ENDIAN_ */

// returns 1 if value d is smaller than the given constraint
// similar to an ISZERO function w/ a margin 
inline int CONSTRAINd( double d, double constraint ) {
    return ( FABSd(d) < FABSd(constraint) );
}

#endif /* __FLOAT64_H__ */


