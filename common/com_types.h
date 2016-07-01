//=============================
//=============================
#ifndef __COM_TYPES_H__
#define __COM_TYPES_H__	1


//--------------------
#ifndef __BYTE__
#define __BYTE__		1
typedef unsigned char byte;
#endif
//--------------------


//--------------------
#ifndef __GBOOL__
#define __GBOOL__		1
// gbool: the one, true boolean
typedef enum { gfalse = 0, gtrue = 1 } gbool;
#endif // __GBOOL__
//--------------------


//--------------------
#ifndef __UINT__
#define __UINT__
typedef unsigned int uint32;
typedef unsigned int uint;
#endif
//--------------------


//--------------------
#ifndef __USHORT__
#define __USHORT__
typedef unsigned short ushort16;
typedef unsigned short ushort;
#endif
//--------------------

#ifndef __POINT__
#define __POINT__
typedef int point2_t[2];
typedef int point3_t[3];
typedef int point4_t[4];
typedef int point5_t[5];
#endif

#ifndef __VECTYPES__
#define __VECTYPES__
typedef float vec_t;
typedef vec_t vec2_t[2];
typedef vec_t vec3_t[3];
typedef vec_t vec4_t[4];
typedef vec_t vec5_t[5];
#endif


#ifndef __GWCHAR__
#define __GWCHAR__
typedef unsigned short gwchar_t;
#endif



#ifndef NULL
# ifdef __cplusplus
#  define NULL 0
# else
#  define NULL ((void *)0)
# endif
#endif


typedef int filehandle_t;

#ifndef PATH_SEP
#ifdef _WIN32
#define PATH_SEP '\\'
#else
#define PATH_SEP '/'
#endif
#endif


#endif //__COM_TYPES_H__
//=============================
//=============================



