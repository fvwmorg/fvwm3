/*
** System.c: code for dealing with various OS system call variants
*/

#include "config.h"

#include <unistd.h>

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
