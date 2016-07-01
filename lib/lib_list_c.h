
#ifndef __DATASTRUCT_H__
#define __DATASTRUCT_H__

//#include "../common/common.h"
#include "../common/com_types.h"

#include "lib_float64.h"
#include <string.h>

#ifdef _DEBUG
    #include <assert.h>
#endif

#include "../common/com_object.h"

/*
==============================================================================
 * memPool class, uses poolPage, 
 *
 * this is perhaps faster than fastPool_c
 *  one design trait that I tried to satisfy here that isn't in the others
 *  is a reset() time of O(1).
 * there are some loops, but they loop over pages, not per element.  so 
 *  in practice, the time that it takes for these loops will be minimal
==============================================================================
*/ 
static volatile const int PoolPageSize = 1024;
volatile static const unsigned int pageMagic = 0x58f13ffe;

template <typename type>
class poolPage {
private:
	unsigned int magic;
public:
	int pageSize;
    int inuse;                  // count how many used
    type *page;
    int returnEnd;              // last good index into returnList
    int returnStart;            // first good index into returnList
    unsigned int *returnList;
    poolPage<type> *next, *prev;

    // init is basic O(1). for use only on a page's initialization/1st use
    void init( int sz =PoolPageSize ) {
		if ( magic == pageMagic )
			return;
        next = prev = NULL;
		pageSize = sz;
		page = (type*) V_Malloc( sizeof(type) * pageSize );

		// dont alloc returnList until we need it
		//returnList = (unsigned int *) V_Malloc(sizeof(unsigned int) * pageSize);
		returnList = NULL;
		magic = pageMagic;
    }

    // reset is a O(1) op, but doesn't reset things that are crucial for
    //  record keeping
    void reset( int sz =PoolPageSize ) {
        returnEnd = returnStart = -1;
        inuse = 0;
		init( sz ) ;
    }

    void erase( void ) {
        memset( page, 0, sizeof(type) * pageSize );
        // dont worry about returnList
    }

    void returnOne( unsigned int n ) {
        // none returned at all
        if ( returnEnd == -1 ) {
            returnEnd = returnStart = 0;
			if ( !returnList )
				returnList = (unsigned int *) V_Malloc(
											sizeof(unsigned int) * pageSize
											);
            returnList[0] = n;
            return;
        }

        int i = (returnEnd + 1) % pageSize;

        // haven't hit returnStart, tack return onto End
        if ( i != returnStart ) {
            returnList[i] = n;
            returnEnd = i;
            return;
        }

        // returnEnd == returnStart, the returns rolled over,
        //  the entire list is free 
        reset();
    } 

    // all are in use, see if there are any in returnList that we 
    //  can use.
    int checkForReturned( void ) {
        if ( returnStart != -1 ) {
            // one open slot left in returnList
            if ( returnStart == returnEnd ) {
                int n = returnStart;
                returnStart = returnEnd = -1;
                return returnList[n];
            }
			int ind = returnStart;
            returnStart = ++returnStart % pageSize;
            return returnList[ ind ];
        }
        return -1;
    }
};

/*
======================================
 memPool
======================================
*/
volatile static const uint PoolMagic = 0x8675309A;
volatile static const int MaxPageSize = 1024 * 1024 * 16;

template <typename type>
class memPool {
private:
    uint magic;
public:
    poolPage<type> page;
    int pageIndex;  // index of the current page 
    int numPages;
	int currentPageSize;
	int CreatedBaseSize;

    void reset( gbool clear =gfalse, uint sz =PoolPageSize ) 
    {
		// first call to reset()
        if ( magic != PoolMagic ) {
            magic = PoolMagic;
            numPages = 1;
			// pages double in size, first one starts at PoolPageSize or what
			//  was requested. on reset this goes back to CreatedBaseSize
			CreatedBaseSize = sz;
			currentPageSize = sz;
        } 
		// all subsequent calls to reset()
		else 
		{
			// the size of our default page
			currentPageSize = CreatedBaseSize;
		}

        pageIndex = 0;

        // reset only first page.  if there are other pages, they are
        //  reset when they become the current page again
        // note: a page's size is only determined when it is first created,
        //  afterwards the size argument is ignored
        page.reset( currentPageSize );

        if ( clear ) {
            poolPage<type> *p = &page;
            while ( p->next ) {
                p = p->next;
                p->prev->erase();
            }
            p->erase();
        }
    }

	// synonym for reset
    void init( gbool clear =gfalse, uint sz =PoolPageSize ) { reset(clear,sz); }
    // synonym for init
    void start( gbool clear =gfalse, uint sz =PoolPageSize ){ reset(clear,sz); }

	

    poolPage<type> * newpage( void ) 
    {
        // get current page
        poolPage<type> *p = getPage( pageIndex );

        // if we're at the end of our pages...
        if ( pageIndex == numPages - 1 ) {

			// ..create another page
            p->next = (poolPage<type> *) V_Malloc( sizeof(poolPage<type>) );
            ++numPages;
            p->next->next = NULL;

			// heuristic: each new page is twice the size of the last
			if ( currentPageSize < MaxPageSize ) 
				currentPageSize *= 2;
        } 
		else 
		// get the pagesize from a page we already created
		{
			if ( p->next )
				currentPageSize = p->next->pageSize;
		}

		// this step serves to initialize a page, if it was created in the
		//  step above, it also resets pages that we have already allocated
        ++pageIndex;

        if ( p->next ) {
			// passing in size the first time sets the page's size
            p->next->reset( currentPageSize );
            p->next->prev = p;
        }

        return p->next;
    }

    type *getone ( void ) {
        poolPage<type> *p;

        // check we are not off the end of the poolPage
        if ( !( p = getPage( pageIndex ) ) )
            return NULL;
        if ( p->pageSize == p->inuse ) {
            // see if there were any returned that we can reuse them
            int i = p->checkForReturned();
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
        poolPage<type> *p = &page;
        while ( 1 ) {
            int diff; 
            if ( (diff = (int)(ret - p->page)) < p->pageSize ) {
                p->returnOne( diff ) ;
                return;
            }
            // hit the end and didn't find it
            if ( !p->next )
                return;
            p = p->next;
        }
    }

	poolPage<type> * getPage( int n ) {
		poolPage<type> *p = &page;
        if ( n < 0 )
            return NULL;
        if ( n == 0 ) 
            return &page;
        
        do {
            if ( !p->next )
                return NULL;
            p = p->next;
        } while ( --n );

        return p;
	}

	// relinquishes all pages except the base page
	void destroy( void ) {
		poolPage<type> *p;
		poolPage<type> *tmp;
		for ( p = page.next ;  p  ; p = tmp ) {
			if ( !V_isStarted() )
				return;
			tmp = p->next;
			if ( p->returnList ) {
				V_Free( p->returnList );
				p->returnList = NULL;
			}
			V_Free( p->page ) ;
			p->page = NULL;
			V_Free( p );
			p = NULL;
		}
		numPages = 1;
		pageIndex = 0;
		currentPageSize = CreatedBaseSize;
	}
};



/*
==============================================================================

    list_c 

 - linkedlist class, specifically designed to be used with memPool.  



lists will keep a change history which is incremented everytime calls that 
can affect the contents of a list are made.  (pop(), add() or remove(), etc)   
each valuator function will keep its own
value for changenum.  if changenum and the master changes differ,
the mean,max,total,...,etc.  will be re-computed,stored and returned.

this way, after you have manipulated the list as much as you like, you can
call the entire set of valuators in succession and they will be able to
re-use each others saved calculations to increase general speed.

_size is the one thing that will always be kept up-to-date 100% of the time.

A list_c doesn't mess with the pool at all because a particular pool might
be shared with one or more other data structures who rely on its itegrity 
remaining un-tampered with.  (unless it is internally allocated)

==============================================================================
*/

volatile static const unsigned listMagic = 0xF1DE7171;

template <typename type>
class node_c {
public:
    type val;
    node_c *next, *prev;
};

template <typename type>
class list_c {

private:

    unsigned int magic;
    gbool pool_internal;
	type null_type;

protected:

    memPool<node_c<type> > *pool;
    node_c<type> *head;
    node_c<type> *tail;

    unsigned int _size;          // count of how many elts

public:    
	list_c() : magic(0), pool_internal(gfalse), pool(0), head(0), tail(0), _size(0) {}

    unsigned int size( void ) { return _size; }

    void reset( void ) { 
        tail = head = NULL; 
        _size       = 0; 
        if ( pool && pool_internal ) 
            pool->reset();
    }

    void start( memPool<node_c<type> >* pp =NULL) {
        if ( listMagic == magic ) {
            reset();
            return;
        }
        if ( pp ) { 
            pool = pp;
            pool_internal = gfalse;
        } else {
            //pool = LIST_MALLOC( pool, memPool<node_c<type> > );
            pool = (memPool<node_c<type> > *) V_Malloc( sizeof(memPool<node_c<type> >) );
            pool_internal = gtrue;
        }
	    memset( &null_type, 0, sizeof(null_type) );
        reset();
        magic = listMagic;
    }
    void init( memPool<node_c<type> >* pp =NULL) {
        start( pp );
    }

	bool isstarted( void ) {
		return magic == listMagic;
	}

    node_c<type> *gethead( void ) { return head; }
    node_c<type> *gettail( void ) { return tail; }

    // adds onto the end (synonym for push)
    void add( type v ) {
        node_c<type> *n = pool->getone();
        n->val = v;
        n->next = NULL;

        if ( head ) {
            n->prev = tail;
            tail->next = n;
            tail = n;
        } else {
            head = tail = n;
            n->prev = NULL;
        }
        ++_size;
    }
    
    // pop specific node by passing node_c pointer
    type popnode( node_c<type> *node =NULL ) 
    {
        if ( !head || !tail ) 
            return null_type;

        if ( !node ) 
            return null_type;

        // first node in ll
        if ( node == head  ) {
            // ..and the only one in list
            if ( node == tail ) {
                head = tail = NULL;
            } 
            // ..or else more than one
            else { 
                head = node->next;
                if ( head ) {
                    head->prev = NULL;
				}
            }
        // last in ll and more than one in list
        } else if ( node == tail ) {
            tail->prev->next = NULL;
			tail = tail->prev;
        // in middle, with at least one or more on either side
        } else {
            node->next->prev = node->prev;
            node->prev->next = node->next;
        }

        type v = node->val;
        // recycle it
        pool->returnone( node );
        --_size;
        return v;
    }

    // pop first node with matching value
    bool popval( type value )
    {
        if ( !head || !tail ) 
            return false;

		node_c<type> *save = NULL;

        // first node in ll
        if ( value == head->val ) {
			save = head;
            // ..and the only one in list
            if ( head == tail ) {
                head = tail = NULL;
            } 
            // ..or else there is more than one in the list
            else { 
                head = head->next;
                if ( head ) {
                    head->prev = NULL;
				}
            }
        // last in ll and more than one in list
        } else if ( value == tail->val ) {
			save = tail;
            tail->prev->next = NULL;
			tail = tail->prev;

        // in middle, with at least one or more on either side
        } else {
			save = head->next;
			while ( save && save->val != value ) {
				save = save->next;
			}
			if ( !save )
				return false;

            save->next->prev = save->prev;
            save->prev->next = save->next;
        }

        // recycle it
        pool->returnone( save );
        --_size;
        return true;
    }

	// pop specific node by passing index (counted from head) // O(n/2)
	type popindex( unsigned int index ) {
        if ( !head || !tail ) {
            return null_type;
        }
		if ( index > _size-1 ) {
            return null_type;
		}

        // first node in ll
        if ( 0 == index  ) {
			return popnode( head );
		}

		if ( _size-1 == index ) {
			return popnode( tail );
		}

		node_c<type> *node;

		unsigned int i;
		if ( index < _size/2 ) {
			node = head;
			for ( i = 0; node && i < index; i++ ) {
				node = node->next;
			}
		} else {
			node = tail;
			for ( i = _size-1; node && i > index; i-- ) {
				node = node->prev;
			}
		}

#ifdef _DEBUG
		assert( index == i );
		assert( node );
#endif

		return popnode( node );
	}


    // pops the first val (pop off the beginning, synonym for shift)
    type pop( void )
    {
        if ( !head || !tail ) {
            return (type)0;
        }

        node_c<type> *p = NULL;
        // more than one, do this..
        if ( head->next ) {
            p = head->next;
            p->prev = NULL;
        }
        type v = head->val;
        pool->returnone( head );
        head = p;
        if ( !p )
            tail = NULL;
        
        --_size;
        return v;
    }

    // returns the first val (shift off the beginning)
    type shift( void ) {
        return pop();
    }

    // adds onto the end (synonym for add)
    void push( type v ) {
        add(v);
    }

    // adds onto the beginning.  
    void unshift( type v ) {
        node_c<type> *n = pool->getone();
        n->val = v;
        n->prev = NULL;

        if ( head ) {
            n->next = head;
            head->prev = n;
            head = n;
        } else {
            head = tail = n;
            n->next = NULL;
        }
        ++_size;
    }
    
    // peek at the last one put in
    type peek_last( void ) {
        return tail->val;
    }

    // peek at the first one put in
    type peek_first( void ) {
        return head->val;
    }
    
    // O(n/2), use w/ care
    type peek ( unsigned int index ) {
        if ( index > _size-1 || !head )
            return null_type;
        node_c<type> *p;
        unsigned int i;
        if ( index < _size/2 ) {
            p = head; i = 0;
            while ( i < index && p ) 
                p = p->next, ++i;
        } else {
            p = tail; i = _size;
            while ( --i > index && p ) 
                p = p->prev;
        }
        if ( i == index && p ) 
            return p->val;
        return null_type;
    }

	node_c<type> *peek_node( unsigned int index ) {
		if ( index > _size-1 || !head )
			return NULL;
        node_c<type> *p;
        unsigned int i;
        if ( index < _size/2 ) {
            p = head; i = 0;
            while ( i < index && p ) 
                p = p->next, ++i;
        } else {
            p = tail; i = _size;
            while ( --i > index && p ) 
                p = p->prev;
        }
        if ( i == index && p ) 
            return p;
        return NULL;
	}

	// must call init() after calling a list_c<type>::destroy()
	void destroy( void ) {
		if ( magic != listMagic )
			return;
		reset();
		// only destroy pools that are ours
		if ( pool_internal ) {
			pool->destroy();
			V_Free( pool );
			pool = NULL;
			magic = 0; // we want a clean slate 
		}
	}
};


/*
==============================================================================

    vec_c

==============================================================================
*/

volatile static const uint vecMagic = 0xFAF007ED;

template<typename type>
class vec_c {
public:

    type *vec;

    int isstarted( void ) { return magic == vecMagic; }
    void init( size_t sz =256 ) {
        if ( magic != vecMagic ) {
            vec = NULL; 
            _size = 0;
            magic = vecMagic;
        }

        if ( sz > _size ) {
            _size = sz;
            if ( vec ) {
                V_Free(vec);
            }
            vec = (type *)V_Malloc( sizeof(type)*_size );
        }
    }
    void reset( size_t sz =256 ) {
        init(sz);
    }
    void start( size_t sz =256 ) {
        reset(sz);
    }
    void destroy() { 
        if ( magic != vecMagic ) {
            return init(0);
        }
        if (vec) {  
            V_Free(vec); vec=NULL; _size=0; 
        } 
    }    

    size_t size() { return _size; }

	void zero_out( void );

protected:
    size_t _size;          // count of how many elts

private:
    uint magic;
};  // vec_c


/*
====================
 vec_c::zero_out
====================
*/
template <typename type>
void vec_c<type>::zero_out( void ) {
	uint s = sizeof(type) * _size;
	if ( s > 0 ) { 
		memset( vec, 0, s );
	}
}



///// MOving buffer_c to lib_buffer.h.  some wierd shit is going on with dependency errors.  Very fucked up.  Something must be causing it that I don't understand.
#include "lib_buffer.h"


/*
============================================

 hash_c -- simple hash table

============================================
*/

#define UNIQUE_STRING_SIZE	256

template<typename type>
class hash_c : public Auto_t {
private:

	//vec_c<hashNode_t *> table;

	virtual void _my_init( void ) {
		table.init( HASH_SIZE );
	}
	virtual void _my_reset( void ) {
		table.zero_out();
		_size = 0;
	}
	virtual void _my_destroy( void ) {
		table.destroy();
	}

	unsigned int _size;

public:

	struct hashNode_t : public Allocator_t {
		type  			val;
		long 			hashval;
		char 			unique[ UNIQUE_STRING_SIZE ];
		hashNode_t		*next, *prev;
		int				len;

		hashNode_t() { reset(); }
		
		void reset() {
			val = 0;
			hashval = 0;
			unique[ 0 ] = 0;
			next = 0;
			prev = 0;
			len = 0;
		}
	};

	unsigned int size() const { return _size; }
	void decrementSize( void ) { --_size; }

	static const unsigned int HASH_SIZE = 256;

	static long GenHash( const char *, unsigned int len );

	void Insert( type, const char *, unsigned int len );
	type FindFirst( const char *unique, unsigned int len );
	type FindAll( const char *unique, unsigned int len, int * );
	type Remove( const char *unique, unsigned int len );

	// FIXME: currently not implemented correctly
	void Insert( type ); // uses pointer as unique string
	type Find( type ); // tells whether we have it
	type Remove( type ); // 

	vec_c<hashNode_t *>& GetTable( void ) { return table; }

	static int hashCompare( const char *, const char *, unsigned int );

	void DeleteByTableReference( type, const char *, unsigned int );

private:

	vec_c<hashNode_t *> table;
};

template <typename type>
void hash_c<type>::Insert( type tp ) { 
	char *c = (char *) tp;	
	char str[5];
	int i = 0;
	str[ i++ ] = *c++;
	str[ i++ ] = *c++;
	str[ i++ ] = *c++;
	str[ i++ ] = *c++;
	str[ 4 ] = 0;
	Insert( tp, str, 4 );
}

template <typename type>
void hash_c<type>::Insert( type tp, const char *str, unsigned int len ) {
	Assert( len < UNIQUE_STRING_SIZE - 1 );

	hashNode_t *n = new hashNode_t;
	n->val = tp;
	n->hashval = GenHash( str, len );

	int i = 0;
	while ( i < len ) {
		n->unique[ i ] = str [ i ] ;
		++i;
	}
	n->unique[ len ] = '\0';
	n->len = len;
	
	if ( table.vec[ n->hashval ] ) {
		hashNode_t *p = table.vec[ n->hashval ];
		while ( p->next ) {
			p = p->next;
		}
		p->next = n;
		n->prev = p;
	} else {
		table.vec[ n->hashval ] = n;
	}

	++_size;
}

template <typename type>
long hash_c<type>::GenHash( const char *str, unsigned int len ) {
	int		i;
	long	hash;
	char	letter;

	hash = 0;
	i = 0;
	while ( i < len ) {
		hash += (long)( str[i] ) * ( i + 119 );
		i++;
	}
	hash &= (HASH_SIZE-1);
	return hash;
}

// same as strcmp, ignores nulls, just compares until len
template <typename type>
inline int hash_c<type>::hashCompare( const char *s, const char *n, unsigned int sz ) {
	int i = 0;
	while ( i < sz ) {
		if ( *s != *n ) {
			return *s - *n;
		}
		++s; ++n; ++i;
	}
	return 0;
}

template <typename type>
type hash_c<type>::FindFirst ( const char *unique, unsigned int len ) {
	long hashval = GenHash( unique, len ) ;
	hashNode_t *p = table.vec[ hashval ];
	while ( p && hashCompare( p->unique, unique, len ) ) {
		p = p->next;
	}
	if ( !p )
		return NULL;
	return p->val;
}

/* one starts this function by passing in an int pointer with value set to 0.  the 
coherency is maintained by this value shared between the caller and this func.
this func look for matches.  Each one it find , the int is incremented.  Finally,
if the int is non-zero, and no more are found the int is set to a negative val and NULL is
returned, telling the caller that there are no more */

template <typename type>
type hash_c<type>::FindAll( const char *unique, unsigned int len, int *index ) {

	if ( *index < 0 || !unique )
		return NULL;

	long hashval = GenHash( unique, len ) ;
	hashNode_t *p = table.vec[ hashval ];
	int i = *index;

	do {

		// go through non-matches looking for a match
		while ( p && hashCompare( p->unique, unique, len ) ) {
			p = p->next;
		}
		if ( !p ) {
			*index = -1; // finished looking
			return NULL;
		}

		// we have a match, skip already done matches, if any
		if ( --i >= 0 ) {
			p = p->next;
		}

		// check for null again
		if ( !p ) { 
			*index = -1; // finished looking
			return NULL;
		}

	} while ( i >= 0 );

	*index = *index + 1;

	return p->val;
}

template <typename type>
type hash_c<type>::Remove( const char *unique, unsigned int len ) {
	long hashval = GenHash( unique, len ) ;
	hashNode_t *p = table.vec[ hashval ];
	while ( p && hashCompare( p->unique, unique, len ) ) {
		p = p->next;
	}
	if ( !p )
		return NULL;

	if ( p->next ) {
		p->next->prev = p->prev;
	}
	if ( p->prev ) {
		p->prev->next = p->next;
	}

	if ( p == table.vec[ hashval ] ) {
		table.vec[ hashval ] = p->next;
	}

	type tp = p->val;
	delete p;
	--_size;
	return tp ;
}

template <typename type>
type hash_c<type>::Find( type tp ) {
	char *c = (char *) tp;	
	char str[5];
	int i = 0;
	str[ i++ ] = *c++;
	str[ i++ ] = *c++;
	str[ i++ ] = *c++;
	str[ i++ ] = *c++;
	str[ 4 ] = 0;
	return Find( str, 4 );
}

template <typename type>
type hash_c<type>::Remove( type tp ) {
	char *c = (char *) tp;	
	char str[5];
	int i = 0;
	str[ i++ ] = *c++;
	str[ i++ ] = *c++;
	str[ i++ ] = *c++;
	str[ i++ ] = *c++;
	str[ 4 ] = 0;
	return Remove( str, 4 );
}

// delete more specific occurrence
template <typename type>
void hash_c<type>::DeleteByTableReference( type tp, const char *unique, unsigned int len ) {

	long hashval = GenHash( unique, len ) ;
	hashNode_t *p = table.vec[ hashval ];

	while ( 1 ) {
		while ( p && hashCompare( p->unique, unique, len ) ) {
			p = p->next;
		}
		if ( !p )
			return;
	
		// not it
		if ( tp != p->val ) {
			p = p->next;
		} else {
			break;
		}
	}

	if ( p->next ) {
		p->next->prev = p->prev;
	}
	if ( p->prev ) {
		p->prev->next = p->next;
	}
	if ( p == table.vec[ hashval ] ) {
		table.vec[ hashval ] = p->next;
	}

	delete p;
	--_size;
}

/*
============================================
 Auto Starter versions of:
	- memPool			AutoMemPool
	- list_c			AutoList
	- vec_c				
	- buffer_c
	- hash_c

	explanation: I should have built these this way from the 
	beginning, but I didn't.  So instead, this is patching them
	to bring them up to muster

	someday, I'll figure out a way to do this, or just re-write them so that
	all of the actual class extend Auto_t, but not now
	
============================================
class AutoMemPool_c : public memPool, public Auto_t {
private:
	_my_reset() {
	}
	_my_init() {
	}
	_my_destroy() {
	}
public:
};
*/

#endif // __DATASTRUCT_H__
