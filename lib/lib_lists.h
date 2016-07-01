/*
============================================================================= 
 lib_lists.h

 templated datastructs:

 * pool_c
 * queue_c
 * fastpool_c - / faster re-implementation of pool_c
 * memPool
 
============================================================================= 
*/

#ifndef __LIB_LISTS_H__
#define __LIB_LISTS_H__

#include <cassert>

#define POOL_MAGIC 0x8675309A
#define POOLPAGESIZE 1024


template<class type>
class node_c {
public:
    type    t;
    class node_c *next;
    class node_c *prev;
    class node_c *pnext;    // vals for use internally in the pool
    class node_c *pprev;
}; 


template<class type>
class container_c {
public:
    node_c<type> nodes[POOLPAGESIZE];
    container_c<type> *next;
};

/*
==================== 
    pool_c - variable size pool of whatever templated type you like.
             starts with 1024 of something that can be used and returned.
             it gets more mem if needed.  only grows dynamically, doesn't
             return memory again.
==================== 
*/

template<class type>
class pool_c {
public:

    pool_c( void ) {}
    virtual ~pool_c( void ) {}



    container_c<type>   *headcont;
    node_c<type>        *inuse;
    node_c<type>        *available;

    node_c<type> * getone( void ) {
        node_c<type> *p;
        if ( magic != POOL_MAGIC )
            reset();

        if ( available ) {
            // more than one
            if ( available->pnext != available ) {
                p = available->pnext;
                p->pprev->pnext = p->pnext;
                p->pnext->pprev = p->pprev;
                available = p->pnext;
                setInuse( p );
                return p;
            } else {        // just one
                p = available;
                available = NULL;
                // starts a new container, and fills up available
                makeContainerAvailable( addNewContainer() );
                setInuse( p );
                return p;
            }
        } else {
            makeContainerAvailable( addNewContainer() );
            p = available->pnext;
            p->pprev->pnext = p->pnext;
            p->pnext->pprev = p->pprev;
            available = p->pnext;
            setInuse( p );
            return p;
        }
    }

    void returnone( node_c<type> *nr ) {
        node_c<type> *p;
        if ( magic != POOL_MAGIC )
            reset();

        if ( !inuse )
            return;
        // only 1 inuse
        p = inuse;
        if ( p->pnext == p ) {
            if ( nr != p )
                return;
            inuse = NULL;
        } else { // 2 or more inuse
            
            // make sure nr is in inuse
            while ( p->pnext != inuse ) {
                if ( p->pnext == nr ) 
                    break;
                p = p->pnext;
            }
            if ( p->pnext != nr ) {
                return;
            }

            p = nr->pprev;
            nr->pprev->pnext = nr->pnext;
            nr->pnext->pprev = nr->pprev;

            if ( nr == inuse ) {
                inuse = p; 
            }
        }
        setAvailable( nr );
    }

    int isInuse( type *tr ) {
    }

    int isAvailable( type *tr ) {
    }

    void reset ( int pages=0 ) {
        inuse = NULL;
        available = NULL;

        if ( magic != POOL_MAGIC ) {
            headcont = NULL;
            addNewContainer();
            magic = POOL_MAGIC;
        }
        while ( pages-- > 1 ) { 
            addNewContainer();
        }
        resetAndRebuildAvailable();
    }


private:

    void setAvailable( node_c<type> *a ) {
        if ( available ) {
            a->pprev = available->pprev;
            a->pnext = available;
            available->pprev->pnext = a;
            available->pprev = a;
        } else {
            available = a->pprev = a->pnext = a;
        }
    }

    void setInuse( node_c<type> *p ) {
        if ( inuse ) {
            p->pprev = inuse->prev;
            p->pnext = inuse; 
            inuse->pprev->pnext = p;
            inuse->pprev = p;
        } else {
            inuse = p->pnext = p->pprev = p;
        }
    }

    // start a new container and attach
    container_c<type> * addNewContainer( void ) {
        if ( !headcont ) {
            headcont = (container_c<type> *) V_Malloc( sizeof(container_c<type>) );
            headcont->next = NULL;
            contcount = 1;
            return headcont;
        }

        container_c<type> *c = headcont;
        while ( c->next ) {
            c = c->next;
        }
        c->next = (container_c<type> *) V_Malloc( sizeof(container_c<type>) );
        c->next->next = NULL;
        ++contcount;
        return c->next;
    }
    
    // add one container to available 
    void makeContainerAvailable( container_c<type> *nc ) {
        for ( int i = 0 ; i < POOLPAGESIZE; i++ ) {
            setAvailable( &nc->nodes[i] );
        }
    }

    void resetAndRebuildAvailable( void ) {
        container_c<type> *cont;
        // detach them all
        available = NULL;
        cont = headcont;
        while ( cont ) {
            for ( int i = 0 ; i < POOLPAGESIZE; i++ ) {
                setAvailable( &cont->nodes[i] );
            }
            cont = cont->next;
        }
    }

    int contcount;  // number of containers of nodes held
    int magic;
}; /* pool_c */


/*
===================
queue_c
===================
*/
template<class type>
class queue_c {
public:

    pool_c<type> pool;
    node_c<type> *head;

    void push( type t ) 
    { 
        node_c<type> *p = pool.getone();
        p->t = t;
        if ( !head ) {
            p->next = p->prev = head = p;
            ++length;
            return;
        }
        p->prev = head->prev;
        p->next = head;
        head->prev->next = p;
        head->prev = p;
        head = p;
        ++length;
    }
    type pop( void ) {
        type t;
		if ( !head ) {
			if (--length < 0)
				length = 0;
            return ( type ) 0; 
		}

        if ( head->prev == head ) {
            node_c<type> *p = head;
            head = NULL;
            t = p->t;
            pool.returnone( p );
            --length;
            return t;
        }

        node_c<type> *ret = head->prev;
        ret->prev->next = head;
        head->prev = ret->prev;
        t = ret->t;
        pool.returnone( ret );
        --length;
        return t;
    }
    void clear( void ) {
        while (length) {
            pop();
        }
    }
    queue_c( void ) { head = NULL; length = 0; }
    ~queue_c( void ) {
        clear();
    }
    int isEmpty( void ) { return (length > 0) ? 0 : 1; }

    int peek( type t ) {
		if ( !head )
			return 0;
        node_c<type> *p = head;
        if ( p->t == t )
            return 1;
        while ( p->next != head ) {
            if ( p->t == t )
                return 1;
            p = p->next;
        }
        return 0;
    }

private:
    int length;
}; /*  queue_c  */




template <class type> 
class fastpool_c {
public:
    fastpool_c( void ) {}
    virtual ~fastpool_c( void ) {}

    container_c<type>   *headcont;
    container_c<type>   *tailcont;
    node_c<type>        *available;

    node_c<type> * getone( void ) {
        node_c<type> *v;

        if ( magic != POOL_MAGIC )
            reset();
again2:
        if ( available == NULL ) {
            makeContainerAvailable( addNewContainer() );
            goto again2;
        }

        v = available;
        available = *( node_c<type> ** )available;
        v->prev = v->next = NULL;
        return v;
    }

    void returnone( node_c<type> *nr ) { 
        if ( magic != POOL_MAGIC )
            reset();
        *( node_c<type> ** )nr = available;
        available = ( node_c<type> * )nr;
    }

    void reset ( int pages=0 ) {
        available = NULL;

        if ( magic != POOL_MAGIC ) {
            headcont = NULL;
            addNewContainer();
            magic = POOL_MAGIC;
        }
        while ( pages-- > 1 ) { 
            addNewContainer();
        }
        resetAndRebuildAvailable();
    }

private:
    int contcount;
    uint magic;

    container_c<type> * addNewContainer( void ) {
        if ( !headcont ) {
            tailcont = 
            headcont = (container_c<type> *) V_Malloc( sizeof(container_c<type>) );
            headcont->next = NULL;
            contcount = 1;
            return headcont;
        }

        tailcont->next = (container_c<type> *) V_Malloc( sizeof(container_c<type>) );
        tailcont->next->next = NULL;
        tailcont = tailcont->next;
        ++contcount;
        return tailcont;
    }

    // add one container to available 
    void makeContainerAvailable( container_c<type> *nc ) {
        node_c<type> *p = nc->nodes + POOLPAGESIZE;
        while ( --p != nc->nodes ) {
            *( node_c ** )p = p - 1;
        }

        if ( !available ) {
            *( node_c ** )p = NULL;
        } else {
            *( node_c ** )p = available;
        }
        available = nc->nodes + POOLPAGESIZE - 1;
    }

    void resetAndRebuildAvailable( void ) {
        container_c<type> *cont;
        
        // detach them all
        available = NULL;
        cont = headcont;
        while ( cont ) {
            node_c<type> *p = nc->nodes + POOLPAGESIZE;
            while ( --p != nc->nodes ) {
                *( node_c ** )p = p - 1;
            }
            *( node_c ** )p = available;
            available = nc->nodes + POOLPAGESIZE - 1;

            cont = cont->next;
        }
    }
};



/*
 * mempool class, uses poolPage, 
 *
 * this is perhaps faster than fastPool_c
 *  one design trait that I tried to satisfy here that isn't in the others
 *  is a reset() time of O(1).
 * there are some loops, but they loop over pages, not per element.  so 
 *  in practice, the time that it takes for these loops will be minimal
*/ 

static const int PoolPageSize = 1024;


template <class type>
class poolPage {
public:
    int inuse;                  // count how many used
    type page[PoolPageSize];
    int returnEnd;              // last good index into returnList
    int returnStart;            // first good index into returnList
    unsigned short returnList[PoolPageSize];
    poolPage<type> *next, *prev;


    // reset is a O(1) op, but doesn't reset things that are crucial for
    //  record keeping
    void reset( void ) {
        returnEnd = returnStart = -1;
        inuse = 0;
    }

    // init means basic O(1) setup for use (typically the first time use)
    void init( void ) {
        reset();
        next = prev = NULL;
    }
    void erase( void ) {
        C_memset( page, 0, sizeof(type) * PoolPageSize );
        // dont worry about returnList
    }

    void returnOne( int n ) {
        // none returned at all
        if ( returnEnd == -1 ) {
            returnEnd = returnStart = 0;
            returnList[0] = (unsigned short) n;
            return;
        }

        int i = (returnEnd + 1)%PoolPageSize;

        // haven't hit returnStart, tack return onto End
        if ( i != returnStart ) {
            returnList[i] = (unsigned short) n;
            returnEnd = i;
            return;
        }

        // returnEnd == returnStart, the returns rolled over,
        //  the entire list is free 
        reset();
    } 

    // all are in use, see if there are any in returnList that we 
    //  can use.
    unsigned short checkForReturned( void ) {
        if ( returnStart != -1 ) {
            // one open slot left in returnList
            if ( returnStart == returnEnd ) {
                int n = returnStart;
                returnStart = returnEnd = -1;
                return returnList[n];
            }
            ++returnStart;
            return returnList[ returnStart - 1 ];
        }
        return -1;
    }
};


const uint PoolMagic = 0x8675309;

template <class type>
class memPool {
private:
    uint magic;
public:
    poolPage<type> pool;
    int pIndex;  // current page index
    int numPages;

    void reset( gbool clear=gfalse ) 
    {
        if ( magic != PoolMagic ) {
            magic = PoolMagic;
            numPages = 1;
        }
        pIndex = 0;

        // reset only first page.  if there are other pages, they are
        //  reset when they become the current page again
        pool.reset();

        if ( clear ) {
            poolPage<type> *p = &pool;
            while ( p->next ) {
                p = p->next;
                p->prev->erase();
            }
            p->erase();
        }
    }

    poolPage<type> * newpage( void ) 
    {
        // get current page
        poolPage<type> *p = getPage( pIndex );

        // if we're at the end...
        if ( pIndex == numPages - 1 ) {
            p->next = V_Malloc( sizeof(poolPage<type>) );
            ++numPages;
            p->next->next = NULL;
        }

        // otherwise:
        // this way we can reset and re-use any pages that we have
        //  alread allocated, if reset was called
        ++pIndex;
        p->next->reset();
        p->next->prev = p;

        return p->next;
    }

    type *getone ( void ) {
        // check we are not off the end of the poolPage
        poolPage<type> *p;

        if ( !( p = getPage( pIndex ) ) )
            return NULL;
        if ( PoolPageSize == p->inuse ) {
            // see if there were any returned that we can reuse
            ushort i = p->checkForReturned();
            if ( i > -1 )
                return &p->page[i];
            // entire page in use, get new one
            p = newpage();
        }

        ++p->inuse;
        return &p->page[p->inuse-1];
    }

    void returnone( type *ret ) {
        // find which page
        poolPage<type> *p = &pool;
        while ( 1 ) {
            int diff; 
            if ( (diff = ret - p->page) < PoolPageSize ) {
                p->returnOne( diff ) ;
                return;
            }
            if ( !p->next )
                return;
            p = p->next;
        }
    }

	poolPage<type> * getPage( int n ) {
		poolPage<type> *p = &pool;
        if ( n < 0 )
            return NULL;
        if ( n == 0 ) 
            return &pool;
        
        do {
            if ( !p->next )
                return NULL;
            p = p->next;
        } while ( --n );

        return p;
	}
};


#endif /* !__LIB_LISTS_H__ */


