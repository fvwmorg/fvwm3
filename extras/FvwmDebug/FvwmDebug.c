/* This module, and the entire FvwmDebug program, and the concept for
 * interfacing this module to the Window Manager, are all original work
 * by Robert Nation
 *
 * Copyright 1994, Robert Nation. No guarantees or warantees or anything
 * are provided or implied in any way whatsoever. Use this program at your
 * own risk. Permission to use this program for any purpose is given,
 * as long as the copyright is kept intact. */

#include "config.h"

#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include "../../fvwm/module.h"

#include "FvwmDebug.h"

char *MyName;
int fd_width;
int fd[2];

/*
** spawn_xtee - code to execute xtee from a running executable &
** redirect stdout & stderr to it.  Currently sends both to same xtee,
** but you rewrite this to spawn off 2 xtee's - one for each stream.
*/

pid_t spawn_xtee(void)
{
  pid_t pid;
  int PIPE[2];
  char *argarray[256];

  setvbuf(stdout,NULL,_IOLBF,0); /* line buffered */

  if (pipe(PIPE))
  {
    perror("spawn_xtee");
    fprintf(stderr, "ERROR ERRATA -- Failed to create pipe for xtee.\n");
    return 0;
  }

  argarray[0] = "xtee";
  argarray[1] = "-nostdout";
  argarray[2] = NULL;

  if (!(pid = fork())) /* child */
  {
    dup2(PIPE[0], STDIN_FILENO);
    close(PIPE[0]);
    close(PIPE[1]);
    execvp("xtee",argarray);
    exit(1); /* shouldn't get here... */
  }
  else /* parent */
  {
    if (ReapChildrenPid(pid) != pid)
    {
      dup2(PIPE[1], STDOUT_FILENO);
      dup2(PIPE[1], STDERR_FILENO);
      close(PIPE[0]);
      close(PIPE[1]);
    }
  }

  return pid;
} /* spawn_xtee */

/***********************************************************************
 *
 *  Procedure:
 *	main - start of module
 *
 ***********************************************************************/
int main(int argc, char **argv)
{
  char *temp, *s;

  /* Save our program  name - for error messages */
  temp = argv[0];
  s=strrchr(argv[0], '/');
  if (s != NULL)
    temp = s + 1;

  MyName = safemalloc(strlen(temp)+2);
  strcpy(MyName,"*");
  strcat(MyName, temp);

  if((argc != 6)&&(argc != 7))
    {
      fprintf(stderr,"%s Version %s should only be executed by fvwm!\n",MyName,
	      VERSION);
      exit(1);
    }

  /* Dead pipe == Fvwm died */
  signal (SIGPIPE, DeadPipe);

  fd[0] = atoi(argv[1]);
  fd[1] = atoi(argv[2]);

#if 0
  spawn_xtee();
#endif

  /* Data passed in command line */
  fprintf(stderr,"Application Window 0x%s\n",argv[4]);
  fprintf(stderr,"Application Context %s\n",argv[5]);

  fd_width = GetFdWidth();

  /* Create a list of all windows */
  /* Request a list of all windows,
   * wait for ConfigureWindow packets */
  SendInfo(fd,"Send_WindowList",0);

  Loop(fd);
  return 0;
}


/***********************************************************************
 *
 *  Procedure:
 *	Loop - wait for data to process
 *
 ***********************************************************************/
void Loop(int *fd)
{
  unsigned long header[3], *body;

  while(1)
    {
      if(ReadFvwmPacket(fd[1],header,&body) > 0)
	{
	  process_message(header[1],body);
	  free(body);
	}
    }
}


/***********************************************************************
 *
 *  Procedure:
 *	Process message - examines packet types, and takes appropriate action
 *
 ***********************************************************************/
void process_message(unsigned long type,unsigned long *body)
{
  switch(type)
    {
    case M_ADD_WINDOW:
      list_add(body);
    case M_CONFIGURE_WINDOW:
      list_configure(body);
      break;
    case M_DESTROY_WINDOW:
      list_destroy(body);
      break;
    case M_FOCUS_CHANGE:
      list_focus(body);
      break;
    case M_NEW_PAGE:
      list_new_page(body);
      break;
    case M_NEW_DESK:
      list_new_desk(body);
      break;
    case M_RAISE_WINDOW:
      list_raise(body);
      break;
    case M_LOWER_WINDOW:
      list_lower(body);
      break;
    case M_ICONIFY:
      list_iconify(body);
      break;
    case M_MAP:
      list_map(body);
      break;
    case M_ICON_LOCATION:
      list_icon_loc(body);
      break;
    case M_DEICONIFY:
      list_deiconify(body);
      break;
    case M_WINDOW_NAME:
      list_window_name(body);
      break;
    case M_ICON_NAME:
      list_icon_name(body);
      break;
    case M_RES_CLASS:
      list_class(body);
      break;
    case M_RES_NAME:
      list_res_name(body);
      break;
    case M_END_WINDOWLIST:
      list_end();
      break;
    case M_DEFAULTICON:
    case M_ICON_FILE:
    default:
      list_unknown(body);
      break;
    }
}



/***********************************************************************
 *
 *  Procedure:
 *	SIGPIPE handler - SIGPIPE means fvwm is dying
 *
 ***********************************************************************/
void DeadPipe(int nonsense)
{
  fprintf(stderr,"FvwmDebug: DeadPipe\n");
  exit(0);
}

/***********************************************************************
 *
 *  Procedure:
 *	list_add - displays packet contents to stderr
 *
 ***********************************************************************/
void list_add(unsigned long *body)
{
  fprintf(stderr,"Add Window\n");
  fprintf(stderr,"\t ID %lx\n",body[0]);
  fprintf(stderr,"\t frame ID %lx\n",body[1]);
  fprintf(stderr,"\t fvwm ptr %lx\n",body[2]);
  fprintf(stderr,"\t frame x %ld\n",(long)body[3]);
  fprintf(stderr,"\t frame y %ld\n",(long)body[4]);
  fprintf(stderr,"\t frame w %ld\n",(long)body[5]);
  fprintf(stderr,"\t frame h %ld\n",(long)body[6]);
  fprintf(stderr,"\t desk %ld\n",(long)body[7]);
  fprintf(stderr,"\t flags %lx\n",body[8]);
  fprintf(stderr,"\t title height %ld\n",(long)body[9]);
  fprintf(stderr,"\t border width %ld\n",(long)body[10]);
  fprintf(stderr,"\t window base width %ld\n",(long)body[11]);
  fprintf(stderr,"\t window base height %ld\n",(long)body[12]);
  fprintf(stderr,"\t window resize width increment %ld\n",(long)body[13]);
  fprintf(stderr,"\t window resize height increment %ld\n",(long)body[14]);
  fprintf(stderr,"\t window min width %ld\n",(long)body[15]);
  fprintf(stderr,"\t window min height %ld\n",(long)body[16]);
  fprintf(stderr,"\t window max %ld\n",(long)body[17]);
  fprintf(stderr,"\t window max %ld\n",(long)body[18]);
  fprintf(stderr,"\t icon label window %lx\n",body[19]);
  fprintf(stderr,"\t icon pixmap window %lx\n",body[20]);
  fprintf(stderr,"\t window gravity %lx\n",body[21]);
}

/***********************************************************************
 *
 *  Procedure:
 *	list_configure - displays packet contents to stderr
 *
 ***********************************************************************/
void list_configure(unsigned long *body)
{
  fprintf(stderr,"Configure Window\n");
  fprintf(stderr,"\t ID %lx\n",body[0]);
  fprintf(stderr,"\t frame ID %lx\n",body[1]);
  fprintf(stderr,"\t fvwm ptr %lx\n",body[2]);
  fprintf(stderr,"\t frame x %ld\n",(long)body[3]);
  fprintf(stderr,"\t frame y %ld\n",(long)body[4]);
  fprintf(stderr,"\t frame w %ld\n",(long)body[5]);
  fprintf(stderr,"\t frame h %ld\n",(long)body[6]);
  fprintf(stderr,"\t desk %ld\n",(long)body[7]);
  fprintf(stderr,"\t flags %lx\n",body[8]);
  fprintf(stderr,"\t title height %ld\n",(long)body[9]);
  fprintf(stderr,"\t border width %ld\n",(long)body[10]);
  fprintf(stderr,"\t window base width %ld\n",(long)body[11]);
  fprintf(stderr,"\t window base height %ld\n",(long)body[12]);
  fprintf(stderr,"\t window resize width increment %ld\n",(long)body[13]);
  fprintf(stderr,"\t window resize height increment %ld\n",(long)body[14]);
  fprintf(stderr,"\t window min width %ld\n",(long)body[15]);
  fprintf(stderr,"\t window min height %ld\n",(long)body[16]);
  fprintf(stderr,"\t window max %ld\n",(long)body[17]);
  fprintf(stderr,"\t window max %ld\n",(long)body[18]);
  fprintf(stderr,"\t icon label window %lx\n",body[19]);
  fprintf(stderr,"\t icon pixmap window %lx\n",body[20]);
  fprintf(stderr,"\t window gravity %lx\n",body[21]);
}

/***********************************************************************
 *
 *  Procedure:
 *	list_destroy - displays packet contents to stderr
 *
 ***********************************************************************/
void list_destroy(unsigned long *body)
{
  fprintf(stderr,"destroy\n");
  fprintf(stderr,"\t ID %lx\n",body[0]);
  fprintf(stderr,"\t frame ID %lx\n",body[1]);
  fprintf(stderr,"\t fvwm ptr %lx\n",body[2]);
}

/***********************************************************************
 *
 *  Procedure:
 *	list_focus - displays packet contents to stderr
 *
 ***********************************************************************/
void list_focus(unsigned long *body)
{
  fprintf(stderr,"focus\n");
  fprintf(stderr,"\t ID %lx\n",body[0]);
  fprintf(stderr,"\t frame ID %lx\n",body[1]);
  fprintf(stderr,"\t fvwm ptr %lx\n",body[2]);

}

/***********************************************************************
 *
 *  Procedure:
 *	list_new_page - displays packet contents to stderr
 *
 ***********************************************************************/
void list_new_page(unsigned long *body)
{
  fprintf(stderr,"new page\n");
  fprintf(stderr,"\t x %ld\n",(long)body[0]);
  fprintf(stderr,"\t y %ld\n",(long)body[1]);
  fprintf(stderr,"\t desk %ld\n",(long)body[2]);
}

/***********************************************************************
 *
 *  Procedure:
 *	list_new_desk - displays packet contents to stderr
 *
 ***********************************************************************/
void list_new_desk(unsigned long *body)
{
  fprintf(stderr,"new desk\n");
  fprintf(stderr,"\t desk %ld\n",(long)body[0]);
}

/***********************************************************************
 *
 *  Procedure:
 *	list_raise - displays packet contents to stderr
 *
 ***********************************************************************/
void list_raise(unsigned long *body)
{
  fprintf(stderr,"raise\n");
  fprintf(stderr,"\t ID %lx\n",body[0]);
  fprintf(stderr,"\t frame ID %lx\n",body[1]);
  fprintf(stderr,"\t fvwm ptr %lx\n",body[2]);
}


/***********************************************************************
 *
 *  Procedure:
 *	list_lower - displays packet contents to stderr
 *
 ***********************************************************************/
void list_lower(unsigned long *body)
{
  fprintf(stderr,"lower\n");
  fprintf(stderr,"\t ID %lx\n",body[0]);
  fprintf(stderr,"\t frame ID %lx\n",body[1]);
  fprintf(stderr,"\t fvwm ptr %lx\n",body[2]);
}


/***********************************************************************
 *
 *  Procedure:
 *	list_unknow - handles an unrecognized packet.
 *
 ***********************************************************************/
void list_unknown(unsigned long *body)
{
  fprintf(stderr,"Unknown packet type\n");
}

/***********************************************************************
 *
 *  Procedure:
 *	list_iconify - displays packet contents to stderr
 *
 ***********************************************************************/
void list_iconify(unsigned long *body)
{
  fprintf(stderr,"iconify\n");
  fprintf(stderr,"\t ID %lx\n",body[0]);
  fprintf(stderr,"\t frame ID %lx\n",body[1]);
  fprintf(stderr,"\t fvwm ptr %lx\n",body[2]);
  fprintf(stderr,"\t icon x %ld\n",(long)body[3]);
  fprintf(stderr,"\t icon y %ld\n",(long)body[4]);
  fprintf(stderr,"\t icon w %ld\n",(long)body[5]);
  fprintf(stderr,"\t icon h %ld\n",(long)body[6]);
}


/***********************************************************************
 *
 *  Procedure:
 *	list_map - displays packet contents to stderr
 *
 ***********************************************************************/
void list_map(unsigned long *body)
{
  fprintf(stderr,"map\n");
  fprintf(stderr,"\t ID %lx\n",body[0]);
  fprintf(stderr,"\t frame ID %lx\n",body[1]);
  fprintf(stderr,"\t fvwm ptr %lx\n",body[2]);
}


/***********************************************************************
 *
 *  Procedure:
 *	list_icon_loc - displays packet contents to stderr
 *
 ***********************************************************************/
void list_icon_loc(unsigned long *body)
{
  fprintf(stderr,"icon location\n");
  fprintf(stderr,"\t ID %lx\n",body[0]);
  fprintf(stderr,"\t frame ID %lx\n",body[1]);
  fprintf(stderr,"\t fvwm ptr %lx\n",body[2]);
  fprintf(stderr,"\t icon x %ld\n",(long)body[3]);
  fprintf(stderr,"\t icon y %ld\n",(long)body[4]);
  fprintf(stderr,"\t icon w %ld\n",(long)body[5]);
  fprintf(stderr,"\t icon h %ld\n",(long)body[6]);
}



/***********************************************************************
 *
 *  Procedure:
 *	list_deiconify - displays packet contents to stderr
 *
 ***********************************************************************/

void list_deiconify(unsigned long *body)
{
  fprintf(stderr,"de-iconify\n");
  fprintf(stderr,"\t ID %lx\n",body[0]);
  fprintf(stderr,"\t frame ID %lx\n",body[1]);
  fprintf(stderr,"\t fvwm ptr %lx\n",body[2]);
}

/***********************************************************************
 *
 *  Procedure:
 *	list_window_name - displays packet contents to stderr
 *
 ***********************************************************************/

void list_window_name(unsigned long *body)
{
  fprintf(stderr,"window name\n");
  fprintf(stderr,"\t ID %lx\n",body[0]);
  fprintf(stderr,"\t frame ID %lx\n",body[1]);
  fprintf(stderr,"\t fvwm ptr %lx\n",body[2]);
  fprintf(stderr,"\t window name %s\n",(char *)(&body[3]));

}


/***********************************************************************
 *
 *  Procedure:
 *	list_icon_name - displays packet contents to stderr
 *
 ***********************************************************************/
void list_icon_name(unsigned long *body)
{
  fprintf(stderr,"icon name\n");
  fprintf(stderr,"\t ID %lx\n",body[0]);
  fprintf(stderr,"\t frame ID %lx\n",body[1]);
  fprintf(stderr,"\t fvwm ptr %lx\n",body[2]);
  fprintf(stderr,"\t icon name %s\n",(char *)(&body[3]));
}



/***********************************************************************
 *
 *  Procedure:
 *	list_class - displays packet contents to stderr
 *
 ***********************************************************************/
void list_class(unsigned long *body)
{
  fprintf(stderr,"window class\n");
  fprintf(stderr,"\t ID %lx\n",body[0]);
  fprintf(stderr,"\t frame ID %lx\n",body[1]);
  fprintf(stderr,"\t fvwm ptr %lx\n",body[2]);
  fprintf(stderr,"\t window class %s\n",(char *)(&body[3]));
}


/***********************************************************************
 *
 *  Procedure:
 *	list_res_name - displays packet contents to stderr
 *
 ***********************************************************************/
void list_res_name(unsigned long *body)
{
  fprintf(stderr,"class resource name\n");
  fprintf(stderr,"\t ID %lx\n",body[0]);
  fprintf(stderr,"\t frame ID %lx\n",body[1]);
  fprintf(stderr,"\t fvwm ptr %lx\n",body[2]);
  fprintf(stderr,"\t resource name %s\n",(char *)(&body[3]));
}

/***********************************************************************
 *
 *  Procedure:
 *	list_end - displays packet contents to stderr
 *
 ***********************************************************************/
void list_end(void)
{
  fprintf(stderr,"Send_WindowList End\n");
}

