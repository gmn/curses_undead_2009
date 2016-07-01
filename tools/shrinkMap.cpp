/* remove most of the white space from a map, leaving it all on one line, kind of like javascript obfustication, except I still leave one space between terms.  the advantage here, is to obfusticate the mapfile for distribution. */


#define _CRT_SECURE_NO_DEPRECATE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void err( const char * str ) {
    fprintf( stderr, "%s\n", str );
    exit(-1);
}

unsigned int trim_whitespace( char *s, unsigned int i, unsigned int sz ) { 
    unsigned int initial_i = i;
    while ( i < sz && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r' || s[i] == '\n') ) {
        ++i;
    }
    return i - initial_i;
}

int white( char c ) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

int is_comment( char *p ) {
    return *p == '/' && *(p+1) == '/' ;
}

// return length of this line to '\n'
unsigned int linelen( char * d, unsigned int pl , unsigned int sz ) {
    unsigned int i = 0;
    while ( pl < sz && *d != '\n' ) {
        ++d;
        i++;
        ++pl;
    }
    return i;
}

void remove_line( char * p, unsigned int * i, unsigned int data_sz ) {
    unsigned int len = linelen( p, *i, data_sz );
    if ( *p == '/' && *(p+1) == '/' ) {
        *i += len;
    }
}

int main( int argc, char ** argv ) {
    if ( argc != 2 ) {
        err( "usage shrinkMap <mapfile.map>" );
    }

    // read input into memory
    FILE * fp = fopen( argv[1], "rb" );
    if ( !fp ) {
        err( "could open input" );
    }
    char * in_data = (char *) malloc ( 1024 * 1024 );
    char * p = in_data;
    unsigned int n, data_sz = 0 ;
    do {
        n = fread( p, 1, 4096, fp );
        if ( n <= 0 ) break;
        data_sz += n;
        p += n;
    } while(1);
    fclose( fp );

    // create outfilename
    char oname[256];
    char tmp[256];
    strcpy( tmp, argv[1] );

    char * f = (char *) strstr( tmp, ".map" );
    if ( !f ) err( "fuck, I dont get it" );
    *f = '\0';
    sprintf( oname, "%s-shrunk.map", tmp );

    char buf[256];

    // open output
    fp = fopen( oname, "wb" );
    if ( !fp ) {
        sprintf( buf, "couldn't open %s for writing", oname );
        err( buf );
    }

    unsigned int i = 0;
    bool w = false;
    while ( i < data_sz ) {

        if ( !white( in_data[i] ) ) {
            // just remove comment
            if ( is_comment(&in_data[i]) ) 
            {
                remove_line( &in_data[i], &i, data_sz );
            } 
            else // print the line
            {
                unsigned int len = linelen( &in_data[i], i, data_sz );
                
                strncpy( buf, &in_data[i], len );
                buf[len] = '\0';

                fputs( buf, fp );
                i += len;
            }
            if ( i >= data_sz )
                break;
        }

        // skip all white chars
        w = false;
        while ( white( in_data[i] ) ) {
            i++;
            w = true;
            if ( i >= data_sz ) {
                w = false; break;
            }
        }

        if ( !white(in_data[i]) && !is_comment( &in_data[i] ) && w )
            fputc( ' ', fp );
    }
    
    fclose(fp);
    free( in_data );
    fprintf( stdout, "created %s\n", oname );
    return 0;
}
