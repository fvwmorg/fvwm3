/*
 * Dealing with variations on Operating Systems.
 * Most .c files should include this one!
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>

#if HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#if HAVE_UNAME
#include <sys/utsname.h>
#endif

#if HAVE_MALLOC_H
#include <malloc.h>
#endif

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif


/** Functions defined in System.c **/
int GetFdWidth(void);
int mygethostname(char *buf, int max);
int mygetostype(char *buf, int max);
