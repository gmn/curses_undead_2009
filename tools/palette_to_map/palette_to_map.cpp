#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define _WIN 1

#ifndef _WIN
    #include <sys/types.h>
    #include <sys/stat.h>
#else
    #include <windows.h>
    #include <stdio.h>
    #include <conio.h> 
#endif

#include <assert.h>

char *save = NULL;

void warn( const char * str ) {
    fprintf( stderr, "%s\n", str );
}

void err( const char * str ) {
    fprintf( stderr, "%s\n", str );
    if ( save )
        free(save);
    exit(-1);
}

/*
I can't figure out windows at all
int Com_FileSize (const char *filename)
{
    int sz;
	struct _finddata_t c_file;
    intptr_t hFile;

    if ( (hFile = _findfirst (filename, &c_file )) == -1L )
        return -1;
    sz = c_file.size;
    _findclose(hFile);

    return sz;
}*/

char * get_gimp_palette( const char *filename, unsigned int *sz ) {
    FILE * fp = fopen( filename, "rb" );
    if ( !fp ) 
        return NULL;

    char * data = (char *) malloc ( 1024 * 1024 );
    char * p = data;
    int n, read = 0 ;
    do {
        n = fread( p, 1, 4096, fp );
        if ( n <= 0 ) break;
        read += n;
        p += n;
    } while(1);
    fclose( fp );
    *sz = read;
    save = data;
    return data;
}

void printTile( const char *line, int len, FILE *fp ) {
    static int uid = 0;

    int r, g, b, i, j;

    sscanf( line, "%d %d %d", &r, &g, &b );
    float c[3] = { r / 255.0f, g  / 255.0f, b / 255.0f };

    i = uid % 16;
    j = uid / 16;

    fprintf( fp, "MapTile\n{\n\tLayer\n\t{\n\t\t0\n\t}\n" );
    fprintf( fp, "\tBackground\n\t{\n\t\t0\n\t}\n\tUID\n\t{\n\t\t%d", uid );
    fprintf( fp, "\n\t}\n\tMaterial\n\t{\n\t\tcolor ( %f %f %f %f )\n\t}\n\tPoly\n\t", c[0], c[1], c[2], 1.000f );

    //        -2048 --> +2048, ", 256.0, 256.0, 0.000
    fprintf( fp, "{\n\t\t( %.6f %.6f %.6f %.6f %.6f )\n\t}\n}\n", 
//                    -2048.f + i * 256.f, 
//                    -2048.f + j * 256.f,
                    -2048.f + i * 256.f, 
                    1792.f - j * 256.f,
                    256.000,
                    256.000,
                    0.000 );
    ++uid;
}

void header( FILE *fp ) {
    fprintf( fp, "Version 1\n\n" );
    fprintf( fp, "script map_palette.map.lua\n\n" );
}


unsigned int trim_whitespace( char ** d, unsigned int pl, unsigned int sz ) { 
    unsigned int i = 0;
    while ( pl < sz && (**d == ' ' || **d == '\t' || **d == '\r' || **d == '\n') ) {
        ++(*d);
        ++i;
        ++pl;
    }
    return i;
}

unsigned int eatline( char ** d ) { 
    unsigned int i = 0;
    while ( **d != '\n' ) {
        ++i;
        *++d;
    }
    *++d; ++i;
    return i;
}

// return length of this line to '\n'
int linelen( char * d, unsigned int pl , unsigned int sz ) {
    int i = 0;
    while ( pl < sz && *d != '\n' ) {
        ++d;
        i++;
        ++pl;
    }
    return i;
}



void remove_comment( char ** p, unsigned int * i, int * len, unsigned int data_sz ) {
    if ( **p == '/' && *(*p+1) == '/' ) {
        *i += *len;
        *p += *len;
    } else if ( **p == '#' ) {
        *i += *len;
        *p += *len;
    }
    *i += trim_whitespace( p, *i, data_sz );
    *len = linelen( *p, *i, data_sz );
}

int main(int argc, char **argv) {

    if ( argc < 2 ) {
        err( "usage: gimp_palette_convert <*.map> <output>" );
    }

    unsigned int data_sz = 0;
    char * data = NULL;
    if ( !(data=get_gimp_palette( argv[1], &data_sz)) ) {
        char buf[100];
        sprintf( buf, "couldn't get palette: \"%s\"\n", argv[1] );
        err( buf );
    }

    // what to call output
    char outfile[256];
    if ( argc > 2 ) {
        strcpy( outfile, argv[2] );
    } else {    
        sprintf( outfile, "%s.map", argv[1] );
    }

    FILE *fp = fopen( outfile, "wb" );
    header( fp );

    printf( "data size of input file: %u\n", data_sz );

    char * p = data;

    unsigned int i = 0;
    while ( i < data_sz ) {

        // trim any white space
        i += trim_whitespace( &p, i, data_sz );
        if ( i >= data_sz )
            break;

        // get length of line to newline        
        int len = linelen( p, i, data_sz );

        // check if line is a comment, if so remove it
        remove_comment( &p, &i, &len, data_sz );
        if ( i >= data_sz )
            break;

        // get a line
        char line[256];
        strncpy( line, p, len );
        line[len] = '\0';

        // translate line values into 1 MapTile definition and print it
        printTile( line, len, fp );

        // advance past end of line, to first character past the '\n'
        //i += eatline( &data );
        p += len;
        i += len;
    }

    fclose( fp );
    free( data );
    printf( "creating: %s\n", outfile );

    return 0;
}
