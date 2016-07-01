
#include <iostream>

using std::cout;
using std::cerr;


#define BIGARRAY_PAGES 1024
#define BIGARRAY_PLATESIZE 4096

#include "lib.h"





gstack::gstack ( void ) {
    total = 0;
    head.next = &head;
    head.prev = &head;
}

gstack::~gstack ( void ) {
    while ( pop() )
        ;
}

int gstack::getSize ( void ) {
    return total;
}

/*
====================
 gstack::push(void *)
 - push onto the top
====================
*/
void gstack::push ( void *val ) {
    struct elt_s *e = new struct elt_s;

    e->value = val;

    // first insert / empty stack
    if (head.next == &head)
    {
        e->next = &head;
        e->prev = &head;
        head.next = e;
        head.prev = e;
    }
    else 
    {
        e->next = head.next;
        e->prev = &head;
        head.next->prev = e;
        head.next = e;
    }

    ++total;
}

/*
==================== 
 gstack::pop()
 - 
==================== 
*/
void * gstack::pop ( void ) 
{
    struct elt_s *h;
    void * v;

    // stack zero size
    if ( head.next == &head ) 
        return NULL;

    h = head.next;
    v = h->value;
    head.next->next->prev = &head;
    head.next = head.next->next;

    delete h;
    --total;

    return v;
}

// not a big fan of the O(n) , use with caution
void * gstack::operator[] ( int i ) const {
    elt_s * s = head.prev;
    int count = 0;
    while ( s != &head && count <= i ) {
        if ( count == i ) {
            return s->value;
        }
        s = s->prev;
        ++count;
    }
    return NULL;
}

void * gstack::popVal ( void *v )
{
	struct elt_s *t, *n;
	
	if ( head.next == &head )
		return NULL;

	for ( t = head.next; t != &head; t = t->next )
	{
		if ( t->value == v )
		{
			t->next->prev = t->prev;
			t->prev->next = t->next; 
			delete t;
			--total;
			return t;
		}
	}
	return NULL;
}


#if 0
gbool gstack::insert_after (int after, void * value) {
    struct elt_s *b;
    int here = 0;

    // FIXME::
    // if requested after is larger than what weve got
    //  should we insert and return false?, or 
    //  should we not insert and return false?
    
    // just insert anyway for now
    if ( head == NULL ) 
    {
        push( value );
        return true;
    }

    // if after not reached, just tack onto end

    b = head;
    while ( here < after && b != NULL ) 
    {
        b = b->next;
        if ( b == NULL )
        {
        }

        ++here;
        if ( here < after 

    }
    if (b == NULL) 
        return 0;

    return 0;
}
#endif



/* you could store all the internal data in gstack as unsigned long int
 * and let the c++ compiler convert it by offering overloaded function
 * definitions, and then do casts on the overloaded info, and keep track
 * of what type it was so that when you do a pop() you have the datatype
 * enum, and return it ,.... yeah, but how would your code know which 
 * thing to anticipate?  It wouldn't.  */

/*
int main (int argc, char *argv[]) 
{
    gstack gs;

    int n;

    gs.push ( (void *) 5 );
    gs.push ( (void *) 10 );
    gs.push ( (void *) 15 );
    gs.push ( (void *) 50 );

    for (int i = 0; i < 4; i++)
        cout << "gs[" << i << "] => " << (int) gs[i] << "\n";

    while (n = reinterpret_cast<int>(gs.pop())) {
        cout << n << "\n";
    }
}
*/


