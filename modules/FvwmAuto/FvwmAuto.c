/* This module, and the entire FvwmAuto program, and the concept for
 * interfacing this module to the Window Manager, are all original work
 * by Robert Nation
 *
 * Copyright 1994, Robert Nation. No guarantees or warantees or anything
 * are provided or implied in any way whatsoever. Use this program at your
 * own risk. Permission to use this program for any purpose is given,
 * as long as the copyright is kept intact.
 * 
 * reworked by A.Kadlec (albrecht@auto.tuwien.ac.at) 09/96
 * to support an arbitrary enter_fn & leave_fn (command line arguments)
 * completely reworked, while I was there.
 * 
 * Modified by John Aughey (jha@cs.purdue.edu) 11/96
 * to not perform any action when entering the root window
 *
 * Minor patch by C. Hines 12/96.
 *
 */

#define TRUE 1
#define FALSE 

#include "../../configure.h"
#ifdef ISC
#include <sys/bsdtypes.h> /* Saul */
#endif 

#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>
#if defined ___AIX || defined _AIX || defined __QNX__ || defined ___AIXV3 || defined AIXV3 || defined _SEQUENT_
#include <sys/select.h>
#endif
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include "../../fvwm/module.h"
#include "../../libs/fvwmlib.h"     
#include "../../version.h"

#ifdef __hpux
# define HPUX_CAST (int *)
#else
# define HPUX_CAST
#endif

/***********************************************************************
 *
 *  Procedure:
 *	SIGPIPE handler - SIGPIPE means fvwm is dying
 *
 ***********************************************************************/
void DeadPipe(int nonsense)
{
    exit(0);
}

#ifdef BROKEN_SUN_HEADERS
#include "../../fvwm/sun_headers.h"
#endif

#ifdef NEEDS_ALPHA_HEADER
#include "../../fvwm/alpha_header.h"
#endif /* NEEDS_ALPHA_HEADER */

/***********************************************************************
 *
 *  Procedure:
 *	main - start of module
 *
 ***********************************************************************/
void main(int argc, char **argv)
{
    char      	   *enter_fn="Raise",	/* default */
                   *leave_fn=NULL,
                   mask_mesg[80];
    unsigned long  header[HEADER_SIZE], 
                   *body,
                   last_win = 0,	/* last window handled */
                   focus_win = 0;	/* current focus */
    int            fd_width,
                   fd[2],
                   timeout,sec,usec;
    struct timeval value,
                   *delay;
    fd_set	   in_fdset;

    if(argc < 7 || argc > 9)
    {
	fprintf(stderr,"FvwmAuto can use one to three arguments.\n");
	exit(1);
    }

    /* Dead pipes mean fvwm died */
    signal (SIGPIPE, DeadPipe);  

    fd[0] = atoi(argv[1]);
    fd[1] = atoi(argv[2]);

    if ((timeout = atoi(argv[6])))
    {
	sec = timeout / 1000;
	usec = (timeout % 1000) * 1000;
	delay=&value;
    } else 
	delay=NULL;

    if (argv[7])			/* if specified */
    {
	if (*argv[7] && !StrEquals(argv[7],"NOP")) /* not empty */
	    enter_fn=argv[7];		/* override default */
	else
	    enter_fn=NULL;		/* nop */
	
	if (argv[8] && *argv[8] && !StrEquals(argv[8],"NOP"))	
					/* leave function specified */
	    leave_fn=argv[8];
    }	

#ifdef DEBUG
    fprintf(stderr,"[FvwmAuto]: timeout: %d EnterFn: >%s< LeaveFn: >%s<\n",timeout,enter_fn,leave_fn);
#endif

    fd_width = GetFdWidth();
    sprintf(mask_mesg,"SET_MASK %lu\n",(unsigned long)(M_FOCUS_CHANGE));
    SendInfo(fd,mask_mesg,0);

    while(1)
    {
	FD_ZERO(&in_fdset);
	FD_SET(fd[1],&in_fdset);

	if (delay)		/* fill in struct - modified by select() */
	{
	    delay->tv_sec = sec;
	    delay->tv_usec = usec;
	}
	select(fd_width, HPUX_CAST &in_fdset, 0, 0, 
	       (focus_win == last_win) ? NULL : delay);
#ifdef DEBUG
	    fprintf(stderr,"[FvwmAuto]: after select:  focus_win: 0x%08lx, last_win: 0x%08lx\n",focus_win, last_win);
	    fprintf(stderr,"[FvwmAuto]: after select:  delay: 0x%08lx, delay struct: %d.%06d sec\n",delay,delay->tv_sec,delay->tv_usec);
#endif

	if (FD_ISSET(fd[1], &in_fdset) && 
	    ReadFvwmPacket(fd[1],header, &body) > 0)
	{
	    focus_win = body[0];
	    free(body);
#ifdef DEBUG
	    fprintf(stderr,"[FvwmAuto]: M_FOCUS_CHANGE to 0x%08lx\n",focus_win);
#endif
	}
	if (((FD_ISSET(fd[1], &in_fdset)==0) == (delay!=NULL)) &&
					/* new message and timeout==0  or */
					/* no message and timeout>0 */
	    focus_win!=last_win)	/* there's sth. to do */
	{
	    if (last_win && leave_fn)	/* if last_win isn't the root */
	    {
		SendInfo(fd,leave_fn,last_win);
#ifdef DEBUG
		fprintf(stderr,"[FvwmAuto]: executing %s on window 0x%08lx\n",leave_fn,focus_win);
#endif
	    }
	    if (focus_win && enter_fn)	/* if focus_win isn't the root */
	    {
		SendInfo(fd,enter_fn,focus_win);
#ifdef DEBUG
		fprintf(stderr,"[FvwmAuto]: executing %s on window 0x%08lx\n",enter_fn,focus_win);
#endif
	    }
	    last_win = focus_win;	/* switch to wait mode again */
	}
    }
}


