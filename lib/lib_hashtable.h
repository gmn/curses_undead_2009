/*
====================
 hashtable.h
====================
*/

#ifndef __HASHTABLE_H__
#define __HASHTABLE_H__


#include <iostream>
#include <cstring>
#include <cstdio>

#include "lib.h"

enum {  
    HASHTABLE_NULL,
    HASHTABLE_INT,
    HASHTABLE_FLOAT,
    HASHTABLE_STRING
}; 



class hashtable {
    private:
        const static int default_hashsize = 1024;

        class hashnode {
            public:
            const static int stringsz = 128;
            hashnode ( ) { }

            void set (int , void *, void *, unsigned int); 

            int type;
            hashnode *next;
            void * val;

            struct {
                char s[stringsz];
                int i;
                float f;
            } key;    

            unsigned int hashedkey;
        }; 

        unsigned int hashsize;
        unsigned int mask;

        // array of hashnodes
        hashnode** table;

        // stores alloc'd hashes in order of insertion for quick lookup
        //  used by destructor and print
        gstack hashlist;

        int numHashed;

        // internal functions
        void tableInsert ( hashnode * );
        gbool removeNode ( unsigned int , int , void * );
        hashnode * getNode ( void * );


    public:
        hashtable( void );
        hashtable( int s );
        ~hashtable ( void );
                
        /*
        void clear( void ) {
            ~hashtable();
            hashtable();
        } 
        */
        
        void insert( const char *,  void * ); 
        void insert( int ,          void * ); 
        void insert( float ,        void * ); 
		void insert( double,		void * );

        void *lookup ( const char * ); 
        void *lookup ( int ); 
        void *lookup ( float ); 
        void *lookup ( double ); 

        // remove by key
        gbool remove ( const char * );
        gbool remove ( int );
        gbool remove ( float );
		gbool remove ( double );

        friend unsigned int stringHash ( const char * );
        friend unsigned int tenHash ( void * );

        void printContents ( void );
};


#endif /* __HASHTABLE_H__ */
