/******************************************************************************
 *
 * 	vmem.h  -  memory module header
 *
 *****************************************************************************/

#ifndef __VMEM_H__
#define __VMEM_H__ 1

#include <stdlib.h> 

// gmn081114: changing from 8 to 4
#define BIG_PAGE_MEGABYTES  4

const int bigPageBytes = (1048576 * BIG_PAGE_MEGABYTES);


/*
 * header for each page of memory.  Each page has one zone.
 */
typedef struct page_s {
    struct memzone_s *		memzone;
    unsigned int 	vmemused;
    unsigned int 	vmempeek;
	int				client_id; /* clients own pages. 0 is default client */
    struct page_s *	next;
} page_t;
#define __page_default_d { 0, 0, 0, 0, 0 }

/*
 * header for each block of memory.  Blocks are created upon user request.
 */
typedef struct memblock_s {
	int					stamp;
	size_t 				size;	/* size of entire block including header */
	struct memblock_s	*next;
	struct memblock_s	*prev;
	page_t				*page;	/* page of which it is member */
	int					endStamp;
} memblock_t;
#define __memblock_default_d  { 0, 0, 0, 0, 0, 0 }

/*
 * header for each zone of memory.  Each zone has 1 to many blocks.
 */
typedef struct memzone_s {
	int				size;
	memblock_t *	blocks;
	memblock_t *	rover; // should always point to first available block
} memzone_t;
#define __memzone_default_d { 0, 0, 0 }




// general
void  V_Init ( void );
void  V_Free ( void *p );
void* V_Malloc ( size_t );
void  V_Shutdown ( void );
int   V_MemInUse ( void );
int   V_MemPeek ( void );
int   V_NumPages( void );
int   V_NumBlocks( void );
int   V_isStarted( void );

// client_id exclusive memory zones
// returns new client_id on the stack, and prevClientId as pointer
int   V_RequestExclusiveZone( int =bigPageBytes, int * =NULL ); 

// returns prevClientID as pointer
void  V_SetCurrentZone( int, int * =NULL );
int   V_ZoneWipe( int, int =0 ) ;

// makes one allocation to a one-time, unique page
void * V_AllocExclusivePage( size_t );

// prints a console report of mem-usage
void V_ConsoleReport( void );

#endif /* !__VMEM_H__ */

