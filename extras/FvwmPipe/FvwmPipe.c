/* 
   FvwmPipe v0.1 copyright 1996 Matthias Ettrich
   (ettrich@informatik.uni-tuebingen.de)

   Simple hacked module to control fvwm from outside.
   When invoked this module creates a pipe "$HOME/.fvwmpipe.in".
   Whatever you write to this pipe will be passed through to fvwm.
   For example: 
         echo "restart" > $HOME/.fvwmpipe.in
	 (restarts fvwm)
   or
         echo "AddtoMenu \"StartMenu\" \"Emacs\" Exec emacs " > $HOME/.fvwmpipe.in
	 (adds emacs to the StartMenu)
   or
         echo "DestroyMenu \"StartMenu\" " > $HOME/.fvwmpipe.in
	 (destroys the StartMenu)

   The module makes it possible to write config-tools for fvwm
   in any language, for example TCL/TK.

   The code is somewhat based on the FvwmTalk module.
   */ 

/* This module, and the entire NoClutter program, and the concept for
 * interfacing this module to the Window Manager, are all original work
 * by Robert Nation
 *
 * Copyright 1994, Robert Nation. No guarantees or warantees or anything
 * are provided or implied in any way whatsoever. Use this program at your
 * own risk. Permission to use this program for any purpose is given,
 * as long as the copyright is kept intact. */


#define TRUE 1
#define FALSE 

#define UPDATE_ONLY 1
#define ALL 2
#define PROP_SIZE 1024

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

#include "../../libs/fvwmlib.h"     
#include "../../fvwm/module.h"
#include "../../version.h"

char *MyName;
int fd_width;
int fd[2];

/***********************************************************************
 *
 *  Procedure:
 *	SIGPIPE handler - SIGPIPE means fvwm is dying
 *
 ***********************************************************************/
void DeadPipe(int nonsense)
{
  fprintf(stderr,"FvwmPipe: dead pipe\n");
  exit(0);
}


void Loop(int *fvwmfd){
  int fd;
  int res;
  char s[256];
  int a,i;
  char* Home;
  int HomeLen;
  char* inpipename;
  fd_set fdset;

  Home = getenv("HOME");
  if (Home == NULL)
    Home = "./";
  HomeLen = strlen(Home);
  inpipename = safemalloc(HomeLen + 17);
  strcpy(inpipename,Home);
  strcat(inpipename,"/.fvwmpipe.in");

  fd = open(inpipename, O_RDWR|O_NONBLOCK);
  if (fd){
    /* send an empty message to end elder FvwmPipes */ 
    write (fd, "\n", 1);
    close(fd);
  }
    
  mkfifo(inpipename, 0600);
  fd = open(inpipename, O_RDONLY|O_NONBLOCK);
  if (fd<0){
    fprintf(stderr,"FvwmPipe: cannot create %s\n", inpipename);
    exit(0);
  }
  FD_ZERO(&fdset);
  FD_SET(fd, &fdset);
  FD_SET(fvwmfd[1], &fdset);
  while (1){
#ifdef __hpux
    select(fd_width,(int *)&fdset, 0, 0, NULL);
#else
    select(fd_width,&fdset, 0, 0, NULL);
#endif  
    if (FD_ISSET(fd, &fdset)){
      /* read from the fvwmpipe.in and send the stuff to fvwm */ 
      res = read(fd, s, 255);
      if(res>=0) {
	s[res] = '\0';
	a = 0;
	for (i=0; i<res;i++)
	  if (s[i] == '\n'){
	    s[i] = '\0';
	    SendText(fvwmfd,(s+a),0);
	    a = i+1;
	  }
      }
    }
    else {
      /* ignore what comes from fvwm but empty the pipe */ 
      do {
	res = read(fvwmfd[1], s, 99);
      } while (res >= 0);
    }
  }
}


void main(int argc, char **argv)
{
  char *temp, *s;
  FILE *file;
  
  /* Save the program name - its used for error messages and option parsing */
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

  /* Save the program name - its used for error messages and option parsing */
  temp = argv[0];

  s=strrchr(argv[0], '/');
  if (s != NULL)
    temp = s + 1;

  MyName = safemalloc(strlen(temp)+2);
  strcpy(MyName,"*");
  strcat(MyName, temp);

  /* Dead pipes mean fvwm died */
  signal (SIGPIPE, DeadPipe);  
  
  fd[0] = atoi(argv[1]);
  fd[1] = atoi(argv[2]);

  fd_width = GetFdWidth();
  Loop(fd);
}


