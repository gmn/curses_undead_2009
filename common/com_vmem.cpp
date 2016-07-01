/******************************************************************************
 * com_vmem.cpp  -  memory management module
 * 
 * adapted from id Software's Doom Z_Malloc stuff.   which I believe was 
 * adapted from Apple's NEXtStep
 *****************************************************************************/

#define BEGIN_STAMP 0x6D656D76
#define END_STAMP 0x4D454D56

#include "com_vmem.h"
#include <stdlib.h>
#include "common.h"
#include "../client/cl_console.h"
#include <stdarg.h>

// initializer trick (force assembly)
memblock_t __memblock_default_s =  __memblock_default_d;
memzone_t __memzone_default_s = __memzone_default_d;

// pointers to malloc'd page headers
page_s *basePage = NULL;

// global client_id
static int client_id = 0;
static int highClientId = 0;

/* minimum size for leftover mem in a page, before it creates a new page */   
#define MINFRAGMENT  64	


/*
====================
 Z_Push

 new page created to house already allocated memzone
====================
*/
static void Z_Push( memzone_t *newMemZone ) {
    page_t *newPage 		= (page_t*) malloc(sizeof(*newPage));
    newPage->memzone 		= newMemZone;
    newPage->next 			= NULL;
    newPage->vmemused 		= 0;
    newPage->vmempeek 		= 0;
	newPage->client_id 		= client_id; /* sets to global client_id */

	/* set the page in the first block */
	newPage->memzone->blocks->page = newPage; 

	// attach
    page_s *pp = basePage;
    while ( pp->next ) {
        pp = pp->next;
    }
    pp->next = newPage;
}

/*
====================
 V_ZoneInit
====================
*/
static void V_ZoneInit( memzone_t **zpp, int size =bigPageBytes ) 
{
	/*
	 * get the chunk from malloc
	 */
	(*zpp) = (memzone_t *) malloc ( size );
	Assert( *zpp != NULL && "shit, malloc returned 0 bytes!" );

	/*
	 * initialize 
	 */
	**zpp = __memzone_default_s;
	(*zpp)->size = size;

	/* 
	 * block is the next address past memzone 
	 */
	memblock_t *block = (memblock_t *) ( (byte *)(*zpp) + sizeof(memzone_t) );
	*block = __memblock_default_s;

	/*
	 * init memzone_t::block pointers
	 */
	(*zpp)->blocks = block;
	block->next = NULL; /* I favor linked lists with null ends */
	block->prev = NULL;

	// NULL indicates a free block
	block->stamp = 0;
	block->endStamp = 0;
	block->page = NULL; // dont know yet
	(*zpp)->rover = block;

    /* the block is the size of ALL the remaining malloc'd memory */
	block->size = (*zpp)->size - sizeof(memzone_t);
}


/*
====================
 V_NewPage
====================
*/
static void V_NewPage( int requestedSize =bigPageBytes ) {
    memzone_t *newzone;
    V_ZoneInit( &newzone, requestedSize );
    Z_Push( newzone );
}

/*
====================
 V_Init

	start the first page
====================
*/
void V_Init ( void ) {
    basePage = (struct page_s*) malloc( sizeof(*basePage) );
    V_ZoneInit( &basePage->memzone );
	basePage->memzone->blocks->page = basePage;
    basePage->next = NULL;
    basePage->vmemused = 0;
    basePage->vmempeek = 0;
	basePage->client_id = client_id;
}

/*
==================== 
 V_Free
==================== 
*/
void V_Free ( void *ptr )
{
	// not started, already shutdown, or bad arg
	if ( !basePage || !ptr ) {
		return; 
	}

	/* block literally is the block that is one memblock before the ptr */
	/* each chunk of mem has its own header memblock_t that immediately 
     *  preceeds it */
    memblock_t *block = (memblock_t *) ( (byte *)ptr - sizeof(memblock_t) );

    if ( 	block->stamp != BEGIN_STAMP || 
			block->endStamp != END_STAMP ) {

		Err_Fatal( "V_Free: freed a pointer without verification\n");
		Assert( !"memory violation error" );
		return;
	}

	// lookup the page that the block is in by memzone boundary
	page_t *page = basePage;
	while ( page ) {
		if (((unsigned int)ptr >= (unsigned int)page->memzone ) && 
			((unsigned int)ptr < 
			((unsigned int)page->memzone + (unsigned int)page->memzone->size)) )
			break;
		page = page->next;
	}
	if ( !page ) {
		Err_Fatal( "V_Free: couldn't find memory page!\n" );
		return;
	}

	// confirm block's page
	Assert( block->page == page && "block page should match" ); 

    // found page. mark block as free
    block->stamp = 0;
	block->endStamp = 0;

	// update page's memused
    page->vmemused -= block->size;
	if ( page->vmemused < 0 )	
		page->vmemused = 0;

	// connect the newly freed chunk into any surrounding chunks that 
	// may also be free
	memblock_t * prev = block->prev;
	memblock_t ** rover = &page->memzone->rover;
	while ( prev && !prev->stamp ) {
		// adjust the rover if it was pointing here
		if ( *rover == block ) {
			*rover = prev;
		}

		// merge with previous free block
		prev->size += block->size;
		prev->next = block->next;
		if ( block->next ) 
			block->next->prev = prev;
		block = prev;

		// try the next
		prev = prev->prev;
	}

	memblock_t *next = block->next;
	while ( next && !next->stamp ) {
		// merge with next free block
		block->size += next->size;
		block->next = next->next;
		if ( next->next ) 
			next->next->prev = block;
		next = next->next;
	}

	// set the rover to this block
	page->memzone->rover = block;

	// check all the blocks are connected in this page
#if 0
#ifdef _DEBUG
	int tally = 0;
	memblock_t *rov = page->memzone->blocks;
	memblock_t *prev_rov;
	while ( rov ) {
		tally += rov->size;
		if ( rov->next ) {
			//tally += (byte*)rov->next - (byte*)rov;
		} else {
//			tally += (byte*)page->memzone->blocks + page->memzone->size - (byte*)rov;
		}
		prev_rov = rov;
		rov = rov->next;
	}
	Assert( tally + sizeof(memzone_t) == page->memzone->size && "all the blocks are not present and accounted for cap'n" );
#endif
#endif


#if 0 /* disabling this until I can profile more */ 
	// this page is completely empty.
	// count # of pages for this client id, then remove all empty pages but 1 
	if ( block->size == page->memzone->size - sizeof(memzone_t) ) {
		int sid = page->client_id;
		page_t *pp = basePage;
		int count = 0;
		while ( pp ) {
			if ( pp->client_id == sid ) {
				++count;
			}
			pp = pp->next;
		}

		if ( count > 1 ) {
			while ( pp ) {
				if ( 1 == count ) 
					break;
				// the right client and it's freed all the way
				if ( pp->client_id == sid &&  
					pp->memzone->size - sizeof(memzone_t) == 
					pp->memzone->blocks->size ) {

					page_t *tmp = pp->next;
        			free( pp->memzone );
        			free( pp );
					pp = tmp;
					--count;
					continue;
				}
				pp = pp->next;
			}
		}
	}
#endif 
}


/*
====================
V_Malloc
====================
*/
void *V_Malloc ( size_t request_size )
{
	// Auto Init, so that it can be used in default constructors by cpp
	if ( !basePage ) {
		V_Init();
		Assert( basePage != NULL && "vmem auto init failed" );
	}

	if ( (signed)request_size > bigPageBytes ) {
		int size = (request_size + 15) & ~15;
		V_NewPage( size + sizeof(memzone_t) + sizeof(memblock_t) );
	}

	// copy size so we can save original
    size_t size = request_size;

	/* 16 byte aligned */
	size = (size + 15) & ~15;

	/* scan through the block list looking for the first free block of 
		sufficient size, throwing out any un-used blocks along the way */

	// add in size of block header
	size += sizeof(memblock_t);

	// for each page, starting with first, 
	page_t *page = basePage;
	memblock_t *block = NULL;
	while ( page ) 
	{
		// skip if its a client_id other than ours
		if ( page->client_id != client_id ) {
			page = page->next;
			continue;
		}

		// for each block in that page, 
		// start at rover
		block = page->memzone->rover; 
		memblock_t *term = page->memzone->rover ? block->prev : NULL;
		bool found_it = false;
		while ( block ) 
		{
			// find the first unused block of sufficient size and return it
			if ( block->stamp == BEGIN_STAMP && block->endStamp == END_STAMP ) {
				block = block->next;
				continue;
			}
			// not the size we are looking for
			if ( block->size < size ) {
				block = block->next;
				continue;
			}

			found_it = true;
			break;
		}

		// if it hits the end and we haven't found one, 
		// start at the beginning again
		if ( !block && !found_it ) {
			block = page->memzone->blocks;
			// this way if term doesn't show up because of some segmentation
			// or buffer overflows, the list search will still terminate 
			while ( block && block != term ) {
				// find the first unused block of sufficient size and return it
				if ( block->stamp != 0 ) {
					block = block->next;
					continue;
				}
				// not the size we are looking for
				if ( block->size < size ) {
					block = block->next;
					continue;
				}
				found_it = true;
				break;
			}
		}

		if ( found_it ) 
			break;

		page = page->next;
	}
	
	// no blocks found that fit those requirements.  start a whole new page	
	if ( !block || !page ) {
        V_NewPage();
        return V_Malloc( request_size );
	}

	/* if we got here, we have a block big enough */

	// leftover determines if we have room for more blocks in this page
	int leftover = block->size - size;

	// there is enough left, create the next block
	if ( leftover >= MINFRAGMENT )
	{
		// there will be a free block after the allocated block
		memblock_t *newblock = (memblock_t *) ( ((byte *)block) + size );
		*newblock = __memblock_default_s;

		/* newblock comes after the one we are mallocing right now */
		newblock->size = leftover;

		newblock->stamp = 0; 		/* NULL indicates free block */
		newblock->endStamp = 0;
		newblock->page = page;

		/* connect */
		newblock->prev = block;
		newblock->next = block->next;
		if ( block->next ) 
			newblock->next->prev = newblock;
		block->next = newblock;

		/* start looking at newblock next time */
		page->memzone->rover = newblock;
	} 
	else
	{
		page->memzone->rover = page->memzone->blocks;
	}

	block->page = page;
	block->size = size;
	block->stamp = BEGIN_STAMP;
	block->endStamp = END_STAMP;

	// update page's memused
    page->vmemused += block->size;

	// update page's mempeek, so we can measure peek page allocation.
	// a perfect result at full allocation would be (vmempeek == bigPageBytes)
    if ( page->vmemused > page->vmempeek ) {
        page->vmempeek = page->vmemused;
	}

	// check all the blocks are connected in this page and add up
#if 0
#ifdef _DEBUG
	int tally = 0;
	memblock_t *rov = page->memzone->blocks;
	memblock_t *prev_rov = NULL;
	while ( rov ) {
		tally += rov->size;
		if ( rov->next ) {
			//tally += (byte*)rov->next - (byte*)rov;
		} else {
//			tally += (byte*)page->memzone->blocks + page->memzone->size - (byte*)rov;
		}
		prev_rov = rov;
		rov = rov->next;
	}
	Assert( tally + sizeof(memzone_t) == page->memzone->size && "all the blocks are not present and accounted for cap'n" );
#endif
#endif

	/* client actually gets the memory immediately following the memblock_t */
	return (void *)( (byte *)block + sizeof(memblock_t) );
}

/*
====================
 V_Shutdown
====================
*/
void V_Shutdown ( void )
{
    page_s *p;
    page_s *tmp;
    for ( p = basePage; p ; p = tmp ) {
        tmp = p->next;
        free( p->memzone );
        free( p );
    }
	basePage = NULL;
}

/*
====================
 V_MemInUse
====================
*/
int V_MemInUse ( void )
{
    unsigned int inuse = 0;
    for ( page_s *p = basePage; p ; p = p->next ) {
        inuse += p->vmemused;
    }
    return inuse;
}

/*
====================
 V_MemPeek
====================
*/
int V_MemPeek ( void ) 
{
    unsigned int peek = 0;
    for ( page_s *p = basePage; p ; p = p->next ) {
        peek += p->vmempeek;
    }
    return peek;
}

/*
====================
 V_NumPages
====================
*/
int V_NumPages( void ) {
    unsigned int pages = 0;
    for ( page_s *p = basePage; p ; p = p->next ) {
		pages++;
    }
    return pages;
}

/*
====================
 V_NumBlocks
====================
*/
int V_NumBlocks( void ) {
    unsigned int blocks = 0;
    for ( page_s *p = basePage; p ; p = p->next ) {
		memblock_t *mb = p->memzone->blocks;
		while ( mb ) {
			++blocks;
			mb = mb->next;
		}
    }
    return blocks;
}

/*
====================
 V_isStarted
====================
*/
int V_isStarted( void ) { 	
	return basePage != NULL ? 1 : 0 ; 
}

/*
====================
 V_SetCurrentZone

	sets global client_id.  subsequent calls to V_Malloc will auto allocate
	a new page for the global client_id if one has not already been created
====================
*/
void V_SetCurrentZone( int set_client_id, int *ioPrevClientID ) {
	// return current
	if ( ioPrevClientID )
		*ioPrevClientID = client_id;

	// mark high
	if ( set_client_id > highClientId ) {
		highClientId = set_client_id;
	}

	// set new
	client_id = set_client_id;
}

/*
====================
 V_ZoneWipe

	wipe all by client id, returns # of pages that exist w/ that id

	saveAllPages defaults to 0, but can be fed a value to preserve pages
====================
*/
int V_ZoneWipe( int _client_id, int saveAllPages ) {
	if ( !basePage )
		return 0;
	page_t *page = basePage;
	int count = 0;
	while ( page ) {
		if ( page->client_id == _client_id ) {
			++count;
			memzone_t *zpp = page->memzone ;
			memblock_t *block = (memblock_t *) ( (byte *)(zpp) + sizeof(memzone_t) );
			*block = __memblock_default_s;
			zpp->blocks = block;
			block->next = NULL; 
			block->prev = NULL;
			block->stamp = 0;
			block->endStamp = 0;
			block->size = zpp->size - sizeof(memzone_t);

			// zeros give piece of mind
			memset( (void*)((byte*)block+sizeof(memblock_t)), 0, block->size - sizeof(memblock_t) );
		}
		page = page->next;
	}

	if ( count == 1 || saveAllPages ) {
		return count;
	}

	/* YEAH, DEFINITELY NOT READY FOR THIS FUCKER YET

	// release extra pages but keep one 
	page = basePage;
	while ( page ) {
		if ( count == 1 ) 
			break;
		if ( page->client_id == _client_id ) {
			page_t *tmp = page->next;
			if ( page == basePage ) {
				basePage = tmp;
			}
			free( page->memzone );
        	free( page );
			page = tmp;
			--count;
			continue;
		} 
		page = page->next;
	} */

	return count;
}

/*
====================
 destroy one completely
====================
*/
void V_ZoneDestroy( int _client_id ) {
}

/*
====================
 V_RequestExclusiveZone

	creates and returns a new client id.  also creates a page for that id
	and sets it to current.  ioClientID returns id that was set before this
	call. (if not null)
====================
*/
int V_RequestExclusiveZone( int newPageSize, int *ioPrevClientID ) {
	if ( !basePage ) 
		V_Init();

	page_t *page = basePage;
	int newClientId = 1;
	while ( page ) {
		if ( page->client_id >= newClientId ) {
			newClientId = page->client_id + 1;
		}
		page = page->next;
	}

	V_SetCurrentZone( newClientId, ioPrevClientID );

	// get the size needed for the whole page
	size_t size_rounded = ( newPageSize + 15 ) & ~15;
	size_t size_real = size_rounded + sizeof(memzone_t) + sizeof(memblock_t);

	V_NewPage( size_real );
	
	return newClientId;
}

/* 
====================
 V_AllocExclusivePage

makes a one time allocation to an exclusive page that has a unique client_id.
the client_id is discarded.
====================
*/
void * V_AllocExclusivePage( size_t size ) {
	int original_client_id;
	void *buffer = NULL;
	size = ( size + 15 ) & ~15;
	size_t sz = size + sizeof(memzone_t) + sizeof(memblock_t);
	V_RequestExclusiveZone( sz, &original_client_id );
	buffer = V_Malloc( size );
	V_SetCurrentZone( original_client_id );
	return buffer;
}



//////////////////////////////////////////////////////////////////////////////
//
//  Memory profiler stuff.  definitely cowboy code.  fast and loose w/ bugs
//
//////////////////////////////////////////////////////////////////////////////

//#define V_BUFSZ 0x20000
#define V_BUFSZ 0x8000
#define V_END (V_BUFSZ-1)
#define SMBUF_SZ 0x1000
static int profiler_id = -1;
//char * _buf = NULL, *c;
//char * _smbuf = NULL;
//#define PROF_TOTAL_MEM (V_BUFSZ+SMBUF_SZ)

char _buf[ 0x8000 ], *c;


char * getend ( char * start , int max ) {
	if ( max < 0 || max > V_BUFSZ )
		return NULL;
	char * p = start;
	while ( *p ) {
		++p;
		if ( p - start >= max ) 
			return NULL;
	}
	return p;
}


char * put( const char *fmt, ... ) {
    char _smbuf[ SMBUF_SZ ];
	/*
	if ( !_smbuf ) {
		int orig_id;
		if ( -1 == profiler_id ) {
			profiler_id = V_RequestExclusiveZone( PROF_TOTAL_MEM, &orig_id );
		}
		V_SetCurrentZone( profiler_id, &orig_id );
		_smbuf = (char*) V_Malloc( SMBUF_SZ );
		V_SetCurrentZone( orig_id );
	} */
    va_list args;
    memset( _smbuf, 0, SMBUF_SZ );
    va_start(args, fmt);
    vsprintf(_smbuf, fmt, args);
    va_end(args);
	sprintf( c, "%s", _smbuf );
	c = getend( c, V_BUFSZ-(c-_buf) );
	return c;
}

#define CP console.Printf
void V_ConsoleReport( void ) {
	int num = 1;
	/* had problems with this stuff.  I think reason being is because the buffers are
	so large, it overloaded
	the console.Draw function which was making hundreds of thousands of glBegin()/End()
	calls for each silly character of text.  The back buffer management in the draw call
	needs to be improved

	if ( !_buf ) {
		int orig_id;
		if ( -1 == profiler_id ) {
			profiler_id = V_RequestExclusiveZone( PROF_TOTAL_MEM, &orig_id );
		}
		V_SetCurrentZone( profiler_id, &orig_id );
		_buf = (char *) V_Malloc( V_BUFSZ );
		V_SetCurrentZone( orig_id );
	}
	*/
	c = _buf;
	int totalMem = 0;
	int totalUnused = 0;
    for ( page_s *p = basePage; p ; p = p->next ) {

		if ( !put( "page %d :: client_id %d :: %d bytes :: ", num, p->client_id, p->memzone->size ) ) {
			_buf[ V_END ]=0;
			CP("%s",_buf);
			c = _buf;
		}
		++num;

		// count blocks, count empty, count in use
		memblock_t *b = p->memzone->blocks;
		int count = 0;
		while ( b ) {
			++count;
			b = b->next;
		}
		if ( !put( "%d blocks >> ", count ) ) {
			_buf[ V_END ]=0;
			CP("%s",_buf);
			c = _buf;
		}


		count = 0;
		b = p->memzone->blocks;
		int inuse = 0;
		int vacant = 0;
		memblock_t *last_block;
		while ( b ) {
			if ( b->stamp ) {
				/*
				if ( !put( "%d:inuse:%d ", count, b->size ) ) {
					_buf[ V_END ]=0;
					CP("%s",_buf);
					c = _buf;
				} */				

				++inuse;
			} else {
				/*
				if ( !put( "%d:vacant:%d ", count, b->size ) ) {
					_buf[ V_END ]=0;
					CP("%s",_buf);
					c = _buf;
				} */
				++vacant;
				totalUnused += b->size;
			}
			++count;
			last_block = b;
			b = b->next;
		}
			
		if ( !put( " last block (%s) # %d, %d bytes ", !last_block->stamp ? "available" : "inuse",  count, last_block->size ) ) {
			_buf[ V_END ]=0;
			CP("%s",_buf);
			c = _buf;
		}	

		if ( !put( " #### %d in use, %d vacant #### ", inuse, vacant ) ) {
			_buf[ V_END ]=0;
			CP("%s",_buf);
			c = _buf;
		}

		CP( "%s", _buf );
		c = _buf;

		totalMem += p->memzone->size;
	}
	CP( "%d total system memory in use" , totalMem );
	CP( "%d total unused", totalUnused );
}


