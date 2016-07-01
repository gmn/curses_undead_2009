
/******************************************************************************
 * com_files.cpp
 * - file handling and local hash
 *****************************************************************************/


#include "common.h"
#include "../lib/lib.h"





static gbool filesystem_started = gfalse;

char gamename[128] = "zpak";
char c_cdpath[MAX_OSPATH];
char c_basepath[MAX_OSPATH];
char c_homepath[MAX_OSPATH];


#define MAX_FILE_HANDLE_MASK    (MAX_FILE_HANDLES-1)
static fileHandleData_c fileSlots[MAX_FILE_HANDLES];

static fileHash_t *fileHashTable[FILE_HASH_SIZE];
static fileHash_t fileHashPool[FILE_HASHPOOL_SIZE];
static fileHash_t *hashfreelist = NULL;
static int poolInUse = 0;

#define PATH_SEP    '/'

void FS_HashRemove( fileHandleData_c *p );


/*
====================
 FS_PoolGetOne
====================
*/
static fileHash_t * FS_PoolGetOne( void ) {
	fileHash_t *p;

again:
	if (hashfreelist == NULL) {
        int i;
        int oldtime = Com_Millisecond();
        int oldindex = -1;
        for ( i = 0; i < FILE_HASHPOOL_SIZE; i++ ) {
            if ( fileHashPool[i].allocTime < oldtime ) {
                oldtime = fileHashPool[i].allocTime;
                oldindex = i;
            }
        }
        if ( oldindex == -1 ) {
            Com_Printf( "WARNING: fileHashPool overrun!\n" );
            return NULL;
        }

        FS_HashRemove( fileHashPool[oldindex].data ) ;        
        goto again;
	}

	p = hashfreelist;
	hashfreelist = *(fileHash_t **)hashfreelist;
	p->allocTime = Com_Millisecond();
    p->next = NULL;
    ++poolInUse;
	return p;
}

/*
====================
 FS_PoolReturn
====================
*/
static void FS_PoolReturn( fileHash_t *p ) {
    if ( !poolInUse ) {
        return;
    }
    if ( --poolInUse < 0 ) poolInUse = 0;
    C_memset( p, 0, sizeof(*p) );
	*(fileHash_t **)p = hashfreelist;
	hashfreelist = (fileHash_t *)p;
}

/*
====================
 FS_HashFileName
====================
*/
static long FS_HashFileName( const char *fname, int hashSize ) {
	int		i;
	long	hash;
	char	letter;

	hash = 0;
	i = 0;
	while (fname[i] != '\0') {
		letter = tolower(fname[i]);             // case insensitive
		//if (letter =='.') break;				// don't include extension
		if (letter =='\\') letter = '/';		// damn path names
		if (letter == PATH_SEP) letter = '/';   // damn path names
		hash+=(long)(letter)*(i+119);
		i++;
	}
	hash = (hash ^ (hash >> 10) ^ (hash >> 20));
	hash &= (hashSize-1);
	return hash;
}

/*
====================
 FS_ResetFileHashTable
====================
*/
static void FS_ResetFileHashTable( void ) {
    fileHash_t *p;

    C_memset( fileHashTable, 0, sizeof(fileHashTable) );
    C_memset( fileHashPool, 0, sizeof(fileHashPool) );
    p = fileHashPool + FILE_HASHPOOL_SIZE;
    while ( --p != fileHashPool ) {
        *(fileHash_t **)p = p - 1;
    }
    *(fileHash_t **)p = NULL;
    hashfreelist = fileHashPool + FILE_HASHPOOL_SIZE - 1;
}

/*
====================
 FS_Start
====================
*/
static int FS_Startup( const char *game ) {
    FS_ResetFileHashTable();
    return 1;
}

//////////////////////////////////////////////////////////////////
// end local functions
//////////////////////////////////////////////////////////////////


/*
====================
 FS_InitialStartup

 - filesystem init, called once at beginning
====================
*/
void FS_InitialStartup( void )
{
    if ( filesystem_started )
        return;

    //
    // set base paths
    //
    // cd path
    // basepath
    // homepath


    if ( FS_Startup( gamename ) ) {
        filesystem_started = gtrue;
    }
}

/*
====================
 FS_SlurpFile

 opens, mallocs, reads in and closes file, returning size
 null data just gets filesize
 does not keep track of file handle
====================
*/
int FS_SlurpFile( const char *path, void **data ) 
{
    FILE *fp;
    int size, n;
    int beg, end;
    byte *p;

    fp = fopen( path, "rb" );
    if ( !fp ) 
        return 0;

    beg = ftell( fp );
    fseek ( fp, 0, SEEK_END );
    end = ftell( fp );
    fseek ( fp, 0, SEEK_SET );
    size = end - beg;

    if ( !data ) {
        fclose( fp );
        return size;
    }
    
    p = (byte *) V_Malloc(size + 128);
    *(byte **)data = p;

	while( !feof(fp) )
	{
        n = fread( p, 1, 8192, fp );
        p += n;
	}

    // ensure string safe
    *p = 0;
    fclose ( fp );
    return size;
}

/*
====================
 FS_CheckHash

 check for entry in hashtable
====================
*/
static fileHash_t * FS_CheckHash( const char *name, long hash ) {
    fileHash_t *fh;

    fh = fileHashTable[hash];
    while ( fh && C_strncmp( fh->name, name, MAX_FSPATH ) ) {
        fh = fh->next;
    }

    return fh;
}

/*
====================
 FS_HashInsert

 put a new entry into hashtable
====================
*/
static int FS_HashInsert( fileHandleData_c *np, long hash ) {
    fileHash_t *find;
    fileHash_t *slot = FS_PoolGetOne();
    if ( !slot )
        return 0;
    
    if ( !fileHashTable[hash] ) {
        fileHashTable[hash] = slot;
    } else {
        find = fileHashTable[hash];
        while( find->next ) {
            find = find->next;
        }
        find->next = slot;
    }
    slot->data = np;
    slot->allocTime = Com_Millisecond();
    C_strncpy( slot->name, np->name, MAX_FSPATH );
    return 1;
}


/*
====================
 FS_HashRemove
====================
*/
static void FS_HashRemove( fileHandleData_c *p ) {
    fileHash_t *search_p;
    fileHash_t *prev_p;
    unsigned long hash;

    if ( !p ) {
        return;
    }
    if ( p->fp ) {
        fclose( p->fp );
        p->fp = NULL;
    }

    //long hash = FS_HashFileName( p->name, FILE_HASH_SIZE );
    hash = p->hash;

    if ( !fileHashTable[hash] ) {
        Com_Printf("error removing from hashtable, wtf!?\n");
        return;
    }

    search_p = fileHashTable[hash];
    while ( search_p && search_p->data != p ) {
        search_p = search_p->next;
    }

    if ( !search_p || search_p->data != p )
        return;

    // found it , now get the one prior 
    
    prev_p = fileHashTable[hash];
    
    // at the beginning
    if ( prev_p == search_p ) {
        if ( prev_p->next ) {
            fileHashTable[hash] = search_p->next;
        } else {
            fileHashTable[hash] = NULL;
        }
    } else {
        while ( prev_p->next != search_p ) {
            prev_p = prev_p->next;
        }
        // this shouldn't ever happen
        if ( prev_p->next != search_p )
            return;
        prev_p->next = search_p->next;
    }
    FS_PoolReturn( search_p );
}

/*
====================
 FS_FOpenReadOnly

 opens file and maintains filehandle.  filehandles start at zero.  an erroneous
 interaction will return a filehandle < 0
====================
*/
int FS_FOpenReadOnly( const char *path, filehandle_t *hp ) {
    unsigned long hashval; 
    fileHandleData_c *tmpslot;
    FILE *file_p = NULL;
    int oldslot, oldtime;
    int i;
    int beginning, end;
    fileHash_t *fhp;

    if ( path == NULL ) {
        *hp = -1;
        return 0;
    }

    // check to see if file exists
    // if not return 0;
    file_p = fopen( path, "rb" );
    if ( file_p == 0 ) {
        *hp = -1;
        return 0;
    }

    // hash file name
    hashval = FS_HashFileName( path, MAX_FILE_HANDLES );

    // check if already in hashtable
    if ( (fhp = FS_CheckHash( path, hashval )) ) {
        tmpslot = fhp->data;
        // if it is in the hashtable, store the filehandle and return
        *hp = tmpslot - fileSlots;
        return 1;
    }

    // not in hashtable:
    // get open slot in fileSlots
    tmpslot = NULL;
    for ( i = 0; i < MAX_FILE_HANDLES && !tmpslot; i++ ) {
        if ( !fileSlots[i].fp ) {
            tmpslot = &fileSlots[i];
            break;
        }
    }

    // if no open slots left: close file with oldest 'lastAccess'
    // and use that slot
    if ( !tmpslot ) {
        oldtime = Com_Millisecond();
        for ( i = 0; i < MAX_FILE_HANDLES; i++ ) {
            if ( fileSlots[i].lastAccess < oldtime ) {
                oldtime = fileSlots[i].lastAccess;
                oldslot = i;
            }
        }
        
        // close existing, and remove from hash
        fclose( fileSlots[oldslot].fp );
        FS_HashRemove( &fileSlots[oldslot] );
        C_memset( (void *)&fileSlots[oldslot], 0, sizeof(fileSlots[oldslot]) );
        tmpslot = &fileSlots[oldslot];
    }
    

    // store meta-data & fp into slot
    tmpslot->lastAccess = Com_Millisecond();
    C_strncpy( tmpslot->name, path, MAX_FSPATH );
    tmpslot->fp = file_p;
    beginning = ftell( file_p );
    fseek( file_p, 0, SEEK_END );
    end = ftell( file_p );
    fseek( file_p, 0, SEEK_SET );
    tmpslot->filesize = end - beginning;
    tmpslot->iszip = ( C_strstr( tmpslot->name, ".zip" ) ) ? gtrue : gfalse; 
    tmpslot->writeable = gfalse;
    tmpslot->hash = hashval;
    
    // add the new slot to the hash table
    if ( !FS_HashInsert( tmpslot, hashval ) ) {
        // FIXME: make sure this never happens
        fclose( file_p );
        *hp = -1;
        return 0;
    }

    *hp = tmpslot - fileSlots;
    return 1;
}

/*
==================== 
 FS_DataFromHandle
==================== 
*/ 
static fileHandleData_c * FS_DataFromHandle( filehandle_t fh ) {
    fileHandleData_c *p;
    if ( fh >= MAX_FILE_HANDLES ) {
        Com_Printf("WARNING: requesting filehandle out of range!\n");
        return NULL;
    }
    p = &fileSlots[fh];
    p->lastAccess = Com_Millisecond();
    return p;
}

/*
====================
 FS_FClose
====================
*/
int FS_FClose( filehandle_t fh ) {
    fileHandleData_c *p = FS_DataFromHandle( fh );
    if ( !p ) {
        return 0;
    }
    fclose( p->fp );
    FS_HashRemove( p );
    C_memset( (void *)p, 0, sizeof(*p) );
    return 1;
}

/*
====================
 FS_Read
====================
*/
int FS_Read( void *dump, int readrequest, filehandle_t fh ) 
{
    fileHandleData_c *dp = FS_DataFromHandle( fh );
    byte *p;
    int n;
    int readcount;
	int readsize;
	int ireadrequest = readrequest;

    if ( !dp ) {
        return 0;
    }

    if ( !dp->fp ) {
        Com_Printf("WARNING: trying to read a closed file\n");
        return 0;
    }

    p = (byte *)dump;
    readcount = 0;

    // do low level read
    do {

		if ( readrequest > 4096 ) {
			readsize = 4096;
		} else {
			readsize = readrequest;
		}

        n = fread( (void *)p, 1, readsize, dp->fp );
        readcount += n;
        readrequest -= n;
        if ( n <= 0 || readrequest <= 0 )
            break;
        p += n;
    } while(1);

    // sanity check
    if ( readcount < ireadrequest ) {
		//Com_Printf( "Warning: read less than request: %s %i < %i \n", dp->name, readcount, ireadrequest );
    }
    if ( readrequest < 0 ) {
		//Com_Printf( "Warning: read more than requested: %s \n", dp->name );
    }

    // adjust fileofs by howmany read
    dp->fileofs += readcount;
    return readcount;
}

int FS_ReadAll( void *buf, filehandle_t fh ) {
	return FS_Read( buf, 0x7FFFFFF0, fh );
}

/*
====================
 FS_Seek
====================
*/
int FS_Seek( filehandle_t fh, int offs, uint whence ) {
    int a, b;
    fileHandleData_c *p = FS_DataFromHandle( fh );
    if ( !p ) {
        return -1;
    }

    switch (whence) {
    case FSEEK_SET:
        fseek( p->fp, offs, SEEK_SET );
        p->fileofs = offs;
        return p->fileofs;
    case FSEEK_CUR:
        fseek( p->fp, offs, SEEK_CUR );
        p->fileofs += offs;
        return p->fileofs;
    case FSEEK_END:
        fseek( p->fp, offs, SEEK_END );
        p->fileofs = p->filesize - 1 - offs;
        return p->fileofs;
    }
    return -1;
}

/*
====================
 FS_Rewind
====================
*/
int FS_Rewind( filehandle_t fh ) {
    return FS_Seek( fh, 0, FSEEK_SET );
}

/*
====================
 FS_Tell
====================
*/
int FS_Tell( filehandle_t fh ) {
    fileHandleData_c *p = FS_DataFromHandle( fh );
    if ( !p ) {
        return -1;
    }
    return p->fileofs;
}


/*
====================
 FS_Write
====================
*/
int FS_Write( void *data, int howManyBytes, filehandle_t fh ) {
    fileHandleData_c *p = FS_DataFromHandle( fh );
    if ( !p ) {
        return 0;
    }

    return 1;
}

void FS_Shutdown( void ) {
    int i;
    for ( i = 0; i < MAX_FILE_HANDLES; i++ ) {
        if ( fileSlots[i].fp ) {
            fclose( fileSlots[i].fp );
        }
    }
    C_memset( (void *)&fileSlots[0], 0, sizeof(fileSlots) );
    FS_ResetFileHashTable();
}

/*
===================
 FS_GetFileSize
 returns filesize recorded when the file was initially opened
===================
*/
int FS_GetFileSize( filehandle_t fh ) {
	fileHandleData_c *p = FS_DataFromHandle( fh );
	if ( !p ) {
		return -1;
	}
	return p->filesize;
}

#if 0
/*
====================
 FS_FOpenWrite
====================
*/
int FS_FOpenWritable( const char *path, filehandle_t *hp ) {

    // just re-write the read function so that you can pass a read/write
    // parameter to it, and then throw this one away.

    /*
    // store meta-data & fp into that slot
    tmpslot->lastAccess = Com_Millisecond();
    C_strncpy( tmpslot->name, path, MAX_FSPATH );
    tmpslot->fp = file_p;
    beginning = ftell( file_p );
    fseek( file_p, 0, SEEK_END );
    end = ftell( file_p );
    fseek( file_p, 0, SEEK_SET );
    tmpslot->filesize = end - beginning;
    tmpslot->iszip = ( !C_strstr ( tmpslot->name, ".zip" ) ) ? gtrue : gfalse; 
    tmpslot->writeable = gfalse;

    fileSlots[oldslot].lastAccess = Com_Millisecond();
    */
}

#endif
