
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <time.h>
#include <direct.h>
#include "../lib/lib_list_c.h"

static void AddFileName( const char *dir, buffer_c<char *> * files ) {
	int len = strlen( dir ) + 1;
	len = ( len + 15 ) & ~15;
	char * s = (char *) V_Malloc( len );
	strcpy( s, dir );
	files->add( s );
}

void Win_RecursiveReadDirectory( const char *dir, buffer_c<char*> * files ) {
	struct _finddata_t c_file;
	intptr_t hFile;
	char buf[1024];
	char cwd[1024];
	char *e = NULL;

	// save this calls cwd
	_getcwd( cwd, 1024 );

	// expand out the path
	_fullpath( buf, dir, 1024 );

	// change to requested path
	_chdir ( buf );	

	// start file listing 
	if ( (hFile = _findfirst( "*", &c_file )) == -1L ) {
		_chdir( cwd );
		return;	
	}
	else
	{
		do {
			// skip builtin paths
			if ( !strcmp( c_file.name, "." ) || !strcmp( c_file.name, ".." ) )
				continue;

			// skip .svn directories
			if ( !strcmp( c_file.name, ".svn" ) )
				continue;

			_fullpath ( buf, c_file.name, 1024 );

			if ( c_file.attrib & _A_SUBDIR ) {
				Win_RecursiveReadDirectory( buf, files );
			} else {
				AddFileName( buf, files );
			}

      } while( _findnext( hFile, &c_file ) == 0 );

      _findclose( hFile );
   }

	_chdir( cwd );
}


