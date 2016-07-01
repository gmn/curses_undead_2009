/***********************************************************************        
 *
 *  win_filesys.cpp
 *
 ***********************************************************************/


#include "win_local.h"

#include <time.h>

/*
==================== 
 D_AddDirent_dirent
==================== 
*/ 

dirent_t * D_AddDirent_dirent (dirent_t *dirent)
{
    dirent_t _dedef  = __dirent_default;
    dirent_t *next_p, *new_p;

    new_p = (dirent_t *) V_Malloc(sizeof(dirent_t));
    *new_p = _dedef;

    if (dirent->contents_first_p == NULL)
    {
        dirent->contents_current_p = dirent->contents_first_p = new_p;
        new_p->next_p = new_p->prev_p = new_p;
    } 
    else
    {
        next_p = dirent->contents_current_p->next_p;

        dirent->contents_current_p->next_p->prev_p = new_p;
        dirent->contents_current_p->next_p = new_p;

        new_p->prev_p = dirent->contents_current_p;
        new_p->next_p = next_p;

        dirent->contents_current_p = new_p;
    }

    dirent->num_dirent++;
    return new_p;
}


/*
==================== 
 D_AddDirent_dirlist
==================== 
*/
void D_AddDirent_dirlist (dirlist_t *dirlist)
{
    dirent_t _dedef  = __dirent_default;
    dirent_t *next_p, *new_p;

    new_p = (dirent_t *) V_Malloc(sizeof(dirent_t));
    *new_p = _dedef;

    if (dirlist->first_p == NULL)
    {
        dirlist->current_p = dirlist->first_p = new_p;
        new_p->next_p = new_p->prev_p = new_p;
    } 
    else
    {
        next_p = dirlist->current_p->next_p;

        dirlist->current_p->next_p->prev_p = new_p;
        dirlist->current_p->next_p = new_p;

        new_p->prev_p = dirlist->current_p;
        new_p->next_p = next_p;

        dirlist->current_p = new_p;
    }

    dirlist->total++;
}

/*
==================== 
 T_GetDirlist 
==================== 
*/

void T_GetDirlist (int margc, char **margv, dirlist_t *dirlist)
{
    int i;
    char strbuf[256], dbuf[256];
	struct _finddata_t c_file;
    intptr_t hFile;
    int total_folders;
    char *cp;


    // getcwd - store to dirlist
    _getcwd(dirlist->cwd, FILENAME_SIZE);


	total_folders = 0;
    for (i = 1; i < margc; i++)
    {

        // parse input arg
		if ((margv[i][0] == 'C' || margv[i][0] == 'c') && margv[i][1] == ':') 
			strcpy (strbuf, margv[i]);
		else
        	sprintf(strbuf, ".\\%s", margv[i]);


        if ( (hFile = _findfirst (strbuf , &c_file )) != -1L ) 
        {
            if (c_file.attrib & _A_SUBDIR) 
            {
                //
                // is a directory, add slot to dirlist 
                //
                D_AddDirent_dirlist (dirlist);
                
                // 
                _fullpath(dbuf, margv[i], FULLPATH_SIZE);

                // passed in name is already a fullpath
                //  need to get foldername as well
                if (!strncmp(dbuf, margv[i], FULLPATH_SIZE))
                {
                    // separate folder name from basename
                    cp = strrchr(dbuf, '\\');
                    if (cp) {
                        strncpy(dirlist->current_p->name, ++cp, FILENAME_SIZE);
                        strncpy(dirlist->current_p->fullname, dbuf, FULLPATH_SIZE);
                    }
                    else
                        Sys_Error("folder path incorrect");
                }
                else
                {
                    // assume argv is folder name
                    strncpy(dirlist->current_p->name, margv[i], FILENAME_SIZE);
                    strncpy(dirlist->current_p->fullname, dbuf, FILENAME_SIZE);
                }

                printf("adding directory \"%s\"\n", margv[i]);
                ++total_folders;
            }
            else
                fprintf(stderr, "%s is not a directory, skipping\n", margv[i]);
        }
        else
        {
            fprintf(stderr, "%s not found\n", margv[i]);
        }
    }

    _findclose(hFile);
   
}



dirlist_t * D_NewDirlist (void)
{
    dirlist_t _dldef = __dirlist_default;
    dirlist_t *dp;
    dp = (dirlist_t *) V_Malloc(sizeof(dirlist_t));
    *dp = _dldef;
    return dp;
}


/*
====================
 D_ReadDirlistContents
====================
*/

void D_ReadDirlistContents (dirlist_t *dirlist)
{
    int folder_num;
    dirent_t *cur_p, *contents_p;
    char strbuf[FULLPATH_SIZE];
	struct _finddata_t c_file;
    intptr_t hFile;
    char *cp;

    cur_p = dirlist->first_p;

    for (folder_num = 0; folder_num < dirlist->total; folder_num++) 
    {
        sprintf (strbuf, "%s\\*", cur_p->fullname);
        if ( (hFile = _findfirst (strbuf, &c_file )) == -1L )
            printf ("No files found in %s", cur_p->fullname);
        else {
            do {

                if (stricmp(c_file.name, ".") && stricmp(c_file.name, ".."))
                {

                    // add a slot to current dirent
                    contents_p = D_AddDirent_dirent(cur_p);

                    // fullpath name
					_fullpath (contents_p->fullname, c_file.name, FULLPATH_SIZE);

                    // filename
                    cp = strrchr (contents_p->fullname, '\\');
                    if (cp++) 
                        strcpy (contents_p->name , cp);
                    else
                        strcpy (contents_p->name , contents_p->fullname);

                    // inc num_files in current dirent
                    ++cur_p->num_files;

                    // get current file size 
                    contents_p->size = c_file.size;

                    // get current file type
                    contents_p->type = c_file.attrib;
                }
            } 
            while ( _findnext( hFile, &c_file ) == 0 );

            cur_p = cur_p->next_p;
        } 

        _findclose(hFile);
    }
}

    


/*
==============
Sys_mkdir
==============
*/
void Sys_mkdir( const char *path ) {
	_mkdir ( path );
}

/*
==============
Sys_Cwd
==============
*/
char *Sys_getcwd( void ) {
	static char cwd[MAX_OSPATH];

	_getcwd( cwd, sizeof( cwd ) - 1 );
	cwd[MAX_OSPATH-1] = 0;

	return cwd;
}

/*
====================
 Sys_GetTimeStamp
====================
*/ 
char *Sys_GetDateTime ( void ) 
{
    time_t tt;
    tt = time(0);
    return asctime(localtime(&tt));
}

int Sys_FileLength( const char *name ) {
    int beg, end;
    if ( !name || !name[0] )
        return -1;
    FILE *fp = fopen( name, "r" );
    if ( !fp ) {
        return -1;
    }
    beg = ftell( fp );
    fseek( fp, 0, SEEK_END );
    end = ftell( fp );
    fclose( fp );
    return end - beg;
}
