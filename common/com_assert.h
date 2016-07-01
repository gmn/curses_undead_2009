
#include <stdlib.h>
#include <stdio.h>
#include <process.h>



void _hidden_Assert( int, const char *, const char *, int );

#ifdef _DEBUG
#define Assert( v ) _hidden_Assert( ((v)), #v, __FILE__, __LINE__ )
#else
#define Assert( v )
#endif



