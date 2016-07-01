/////////////////
// hashtable.cpp
/////////////////


#include "lib_hashtable.h"


/*
====================
 constructors/destructors
====================
*/
hashtable::hashtable ( void ) {
    hashsize = default_hashsize;
    mask = default_hashsize - 1;

    //table = new (hashsize * sizeof(hashnode *));
    table = new hashnode* [hashsize];
    memset (table, 0, sizeof(hashnode*) * hashsize);
    numHashed = 0;
}

hashtable::hashtable ( int s ) {
    if (s < 1) {
        std::cerr << "hashtable size too small!\n";
        //exit(-1);
        return;
    }
    hashsize = s;
    mask = s - 1;

    table = new hashnode* [hashsize];
    memset (table, 0, sizeof(hashnode *) * hashsize);
    numHashed = 0;
}

hashtable::~hashtable ( void ) {
	hashnode *h, *n;
	int i;
	int *cleared = new int [hashsize];
	memset (cleared, 0, sizeof(int) * hashsize);

	while ( i = (int) hashlist.pop() )
	{	
		if (cleared[i])
			continue;

		cleared[i]++;

		h = table[i];
		if ( !h ) 
			continue;
		n = h->next;
 
		while ( h ) {			
			delete h;
			h = n;
			if ( n )
				n = n->next;
		}
	}

    delete[] table;
	delete[] cleared;
}




// Returns a hash code based on an address
unsigned int tenHash( void *p )
{
    unsigned int val = reinterpret_cast<unsigned int>( p );
    return ( val ^ (val >> 10) ^ (val >> 20) ); 
}

#define SLATSIZE 1024

unsigned int stringHash ( const char *sp ) 
{
    unsigned int slatvals[SLATSIZE];
    int slat = 0;
    int shift = 0;
    int wraparound = 0;
    int i;
    const char *p = sp;
    unsigned int retval = 0;

    // combine, put 4 chars into one unsigned
    while (*p) 
    {  
        if (shift == 0)
            slatvals[slat] = 0;

        slatvals[slat] += *p << shift;

        shift += 8;
        if (shift > 24) {
            shift = 0; 
            ++slat;
        }

        // wrap around
        if (slat >= SLATSIZE) {
            ++wraparound;
            slat = 0;
        }

        ++p;
    }

    // hash the slats
    if ( wraparound ) 
        slat = SLATSIZE-1;

    for (i = 0; i <= slat ; i++) {
        retval ^= slatvals[i];
    }

    return retval;

}

void hashtable::tableInsert (hashnode *hn)
{
    hashnode *t;

    if (table[hn->hashedkey])
    {
        t = table[hn->hashedkey];
        while (t->next)
            t = t->next;
        t->next = hn;
    }
    else
    {
        table[hn->hashedkey] = hn;
    }
}

void hashtable::insert (int a, void *store) 
{
    unsigned int b = tenHash((void *)a);
    b &= mask;

    hashnode *hn = new hashnode();
    hn->set( HASHTABLE_INT, (void *)a, store, b );
    tableInsert ( hn );
    ++numHashed;

    hashlist.push((void *)b);
}

void hashtable::insert (float a, void *store) 
{
    float ftmp = a;
    unsigned int b = tenHash((void *)*((int *)&ftmp));
    b &= mask;

    hashnode *hn = new hashnode();
    hn->set( HASHTABLE_FLOAT, (void *)*((int *)&ftmp), store, b );
    tableInsert ( hn );
    ++numHashed;

    hashlist.push((void *)b);
}

void hashtable::insert (double a, void *store) {
	insert ((float) a, store);
}

void hashtable::insert (const char *a, void *store) {
    unsigned int b = stringHash(a);
    b &= mask;

    hashnode *hn = new hashnode();
    hn->set( HASHTABLE_STRING, (void *)a, store, b );
    tableInsert ( hn );
    ++numHashed;

    hashlist.push((void *)b);
}

gbool hashtable::removeNode ( unsigned int hashkey, int itype, void *ival )
{
	hashnode *t, *p, *n;
	gbool match = gfalse;
	void *vtmp = ival;

	t = table[hashkey];
	if ( !t )
		return gfalse;

	n = t->next;

	while ( t && !match ) {
		switch (itype) {
		case HASHTABLE_INT:
			if ((int)ival == t->key.i)
				match = gtrue;
			break;
		case HASHTABLE_FLOAT:
			if (*(float *)&vtmp == t->key.f)
				match = gtrue;
			break;
		case HASHTABLE_STRING:
			if (!C_strncmp((const char *)ival, t->key.s, t->stringsz))
				match = gtrue;
		default:
			break;
		}

		if (match) 
		{	
			// t at beginning of the list
			p = table[hashkey];
			if ( p == t ) {
				table[hashkey] = n;
				delete p;
				hashlist.popVal ((void *)hashkey);
			} else {
				// get a pointer right behind t
				while ( p->next != t )
					p = p->next;
				p->next = t->next;
				delete t;
				hashlist.popVal ((void *)hashkey);
			}
			return gtrue;
		} else {
			t = t->next;
			if ( t )
				n = t->next;
		}
	}
    return gfalse;
}

gbool hashtable::remove ( int key ) {
    unsigned int hkey = tenHash ( (void *)key );
	hkey &= mask;
    return hashtable::removeNode ( hkey, HASHTABLE_INT, (void *)key );
}

gbool hashtable::remove ( float key ) { 
    float ftmp = key;
    unsigned int hkey = tenHash ( (void *)*(int *)&ftmp );
	hkey &= mask;
    return hashtable::removeNode ( hkey, HASHTABLE_FLOAT, (void *)*(int *)&ftmp );
}

gbool hashtable::remove ( double key ) {
	return remove ( (float) key );
}

gbool hashtable::remove ( const char * key ) { 
    unsigned int hkey = stringHash ( key );
	hkey &= mask;
    return hashtable::removeNode ( hkey, HASHTABLE_STRING, (void *)key );
}

void *hashtable::lookup ( const char *s ) {
    hashnode *h;
    unsigned int hkey = stringHash ( s );
    hkey &= mask;
    h = table[hkey];

    while ( h ) { 
        if (!C_strncmp(s, table[hkey]->key.s, h->stringsz)) 
            return  h->val;
        h = h->next;
    }

    return NULL;
}

using std::cout;

void hashtable::printContents ( void ) {
    int h;
    int t;
    for (t = hashlist.getSize() - 1; t >= 0; t--) {
        h = (int) hashlist[t]; 

		if (!table[h])
			continue;

        cout << "original key: " ;
		switch (table[h]->type) {
		case HASHTABLE_INT:
			printf ("%d", table[h]->key.i);
			break;
		case HASHTABLE_FLOAT:
            cout << table[h]->key.f;
			break;
		case HASHTABLE_STRING:
			cout << "\"" << table[h]->key.s << "\"";
			break;
		default:
			break;
		}
        cout << "\n";

		cout << "hash : " << table[h]->hashedkey << "\n";
		cout << "type : " << table[h]->type  << "\n";

        cout << "stored pointer: " << std::hex << table[h]->val << "\n\n";
    }
}


void hashtable::hashnode::set (int itype, void *ikey, void *ival, unsigned int hk=-1) 
{
	void * vtmp;
    // get primitive type
    type = itype;

    // get original key
    switch (itype) {
    case HASHTABLE_INT:
        key.i = (int)ikey;
        break;
    case HASHTABLE_FLOAT:
		vtmp = ikey;
		key.f = *(float *)&vtmp;
        break;
    case HASHTABLE_STRING:
        C_strncpy ( key.s, (const char *)ikey, stringsz );
        break;
    default:
        type = HASHTABLE_NULL;
        break;
    }

    // get the data value we're storing for lookup
    val = ival;

    // get the hashed value, if any
    hashedkey = hk;

    next = NULL;
}

#if 0 
int main (int argc, char *argv[]) 
{    
    hashtable ht;

    ht.insert ("get one", (void *)1);
    ht.insert ("get two", (void *)2);
    ht.insert ("GET THREE", (void *)3);
    ht.insert (1.8080, (void *)4);
    ht.insert (55, (void *)66);
	ht.insert ("sucka on my big wang", (void *)-1);

	cout << "before\n";
    ht.printContents();
	ht.remove ("sucka on my big wang");
	ht.remove ( 55 );
	ht.remove ( 1.808 );
	cout << "after\n";
	ht.printContents();

    //cout << "lookup: \"get two\" gets: " << ht.lookup("get two") << "\n";

    return 0;
}
#endif
