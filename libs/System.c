/*
** System.c: code for dealing with various OS system call variants
*/

#include "config.h"
#include "fvwmlib.h"


#if HAVE_UNAME
#include <sys/utsname.h>
#endif


/*
** just in case...
*/
#ifndef FD_SETSIZE
#define FD_SETSIZE 2048
#endif


int GetFdWidth(void)
{
#if HAVE_SYSCONF
    return min(sysconf(_SC_OPEN_MAX),FD_SETSIZE);
#else
    return min(getdtablesize(),FD_SETSIZE);
#endif
}

/* return a string indicating the OS type (i.e. "Linux", "SINIX-D", ... ) */
int getostype(char *buf, int max)
{
#if HAVE_UNAME
    struct utsname sysname;

    if ( uname( &sysname ) >= 0 ) {
	strncat( buf, sysname.sysname, max );
	return 0;
    }
#endif
    strcpy (buf,"");
    return -1;
}


/**
 * Set a colon-separated path, with environment variable expansions,
 * and expand '+' to be the value of the previous path.
 **/
void setPath( char** p_path, const char* newpath, int free_old_path )
{
    char* oldpath = *p_path;
    int oldlen = strlen( oldpath );
    char* stripped_path = stripcpy( newpath );
    int found_plus = strchr( newpath, '+' ) != NULL;

    /** Leave room for the old path, if we find a '+' in newpath **/
    *p_path = envDupExpand( stripped_path, found_plus ? oldlen : 0 );
    free( stripped_path );
    
    if ( found_plus ) {
	char* p = strchr( *p_path, '+' );
	memmove( p+oldlen, p+1, strlen(p+1) );

	/* copy oldlen+1 bytes to include the trailing NUL */
	strncpy( p, oldpath, oldlen+1 ); 
    }

    if ( free_old_path )
	free( oldpath );
}


/****************************************************************************
 *
 * Find the specified file somewhere along the given path.
 *
 * There is a possible race condition here:  We check the file and later
 * do something with it.  By then, the file might not be accessible.
 * Oh well.
 *
 ****************************************************************************/
char* searchPath( const char* pathlist, const char* filename, 
		  const char* suffix, int type )
{
    char *path;
    int l;

    if ( filename == NULL || *filename == 0 )
	return NULL;

    l = (pathlist) ? strlen(pathlist) : 0;
    l += (suffix) ? strlen(suffix) : 0;
    
    /* +1 for extra / and +1 for null termination */
    path = safemalloc( strlen(filename) + l + 2 );
    *path = '\0';

    if (*filename == '/' || pathlist == NULL || *pathlist == '\0')
    {
	/* No search if filename begins with a slash */
	/* No search if pathlist is empty */
	strcpy( path, filename );
	return path;
    }

    /* Search each element of the pathlist for the file */
    while ((pathlist)&&(*pathlist))
    {
	char* dir_end = strchr(pathlist, ':');
	if (dir_end != NULL)
	{
	    strncpy(path, pathlist, dir_end - pathlist);
	    path[dir_end - pathlist] = 0;
	}
	else
	    strcpy(path, pathlist);

	strcat(path, "/");
	strcat(path, filename);
	if (access(path, type) == 0)
	    return path;

	if ( suffix && *suffix != 0 ) {
	    strcat( path, suffix );
	    if (access(path, type) == 0)
		return path;
	}
	
	/* Point to next element of the path */
	if(dir_end == NULL)
	    break;
	else
	    pathlist = dir_end + 1;
    }
    /* Hmm, couldn't find the file.  Return NULL */
    free(path);
    return NULL;
}
