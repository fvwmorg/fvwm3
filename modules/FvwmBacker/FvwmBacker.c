/* FvwmBacker Module for Fvwm.
 *
 *  Copyright 1994,  Mike Finger (mfinger@mermaid.micro.umn.edu or
 *                               Mike_Finger@atk.com)
 *
 * The author makes not guarantees or warantees, either express or
 * implied.  Feel free to use any contained here for any purpose, as long
 * and this and any other applicible copyrights are kept intact.

 * The functions in this source file that are based on part of the FvwmIdent
 * module for Fvwm are noted by a small copyright atop that function, all others
 * are copyrighted by Mike Finger.  For those functions modified/used, here is
 *  the full, origonal copyright:
 *
 * Copyright 1994, Robert Nation and Nobutaka Suzuki.
 * No guarantees or warantees or anything
 * are provided or implied in any way whatsoever. Use this program at your
 * own risk. Permission to use this program for any purpose is given,
 * as long as the copyright is kept intact. */

/* Modified to directly manipulate the X server if a solid color
 * background is requested. To use this, usr "-solid <color_name>"
 * as the command to be executed.
 *
 * A. Davison
 * Septmber 1994.
 */
/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <stdio.h>
#include <signal.h>
#include <string.h>

#if HAVE_SYS_BSDTYPES_H
#include <sys/bsdtypes.h> /* Saul */
#endif /* Saul */

#include <X11/Xlib.h>
#include <X11/Intrinsic.h>

#include "libs/Module.h"
#include "libs/Colorset.h"
#include "FvwmBacker.h"

unsigned long BackerGetColor(char *color);

typedef struct
{
  int type; /* The command type.
	     * -1 = no command.
	     *  0 = command to be spawned
	     *  1 = a solid color to be set
	     *  2 = use background from colorset */
  char*	cmdStr; /* The command string (Type 0)   */
  unsigned long solidColor; /* A solid color after X parsing (Type 1) */
  int colorset; /* the colorset to be used */
} Command;

Command *commands;
int DeskCount=0;
int current_desk = 0;

int Fvwm_fd[2];
int fd_width;

char *Module;

/* X Display information. */

Display* 	dpy;
Window		root;
int			screen;

FILE*	logFile;

/* Comment this out if you don't want a logfile. */

/* #define LOGFILE "/tmp/FvwmBacker.log"*/


int main(int argc, char **argv)
{
  char *temp, *s;
  char* displayName = NULL;

  commands=NULL;

  /* Save the program name for error messages and config parsing */
  temp = argv[0];
  s=strrchr(argv[0], '/');
  if (s != NULL)
    temp = s + 1;

  Module=temp;

  if((argc != 6)&&(argc != 7))
  {
    fprintf(stderr, "%s Version %s should only be executed by fvwm!\n", Module,
	    VERSION);
    exit(1);
  }

  Fvwm_fd[0] = atoi(argv[1]);
  Fvwm_fd[1] = atoi(argv[2]);

  /* Grab the X display information now. */

  dpy = XOpenDisplay(displayName);
  if (!dpy)
  {
    fprintf(stderr, "%s:  unable to open display '%s'\n",
	    Module, XDisplayName (displayName));
    exit (2);
  }
  screen = DefaultScreen(dpy);
  root = RootWindow(dpy, screen);

  /* allocate default colorset */
  AllocColorset(0);

  /* Open a log file if necessary */
#ifdef LOGFILE
  logFile = fopen(LOGFILE,"a");
  fprintf(logFile,"Initialising FvwmBacker\n");
#endif

  signal (SIGPIPE, DeadPipe);

  /* Parse the config file */
  ParseConfig();

  fd_width = GetFdWidth();

  SetMessageMask(Fvwm_fd,
		 M_NEW_DESK|M_CONFIG_INFO|M_END_CONFIG_INFO|M_SENDCONFIG);

  /*
  ** we really only want the current desk, and window list sends it
  */
  SendInfo(Fvwm_fd,"Send_WindowList",0);

  /* tell fvwm we're running */
  SendFinishedStartupNotification(Fvwm_fd);

  /* Recieve all messages from Fvwm */
  EndLessLoop();

  /* Should never get here! */
  return 1;
}

/******************************************************************************
  EndLessLoop -  Read until we get killed, blocking when can't read
    Originally Loop() from FvwmIdent:
      Copyright 1994, Robert Nation and Nobutaka Suzuki.
******************************************************************************/
void EndLessLoop()
{
  while(1)
 {
    ReadFvwmPipe();
  }
}

/******************************************************************************
  ReadFvwmPipe - Read a single message from the pipe from Fvwm
******************************************************************************/
void ReadFvwmPipe()
{
  FvwmPacket* packet = ReadFvwmPacket(Fvwm_fd[1]);
  if ( packet == NULL )
    exit(0);
  else
    ProcessMessage( packet->type, packet->body );
}


void set_desk_background(int desk)
{
  switch (commands[desk].type)
  {
  case 1:
    /* Process a solid color request */
    XSetWindowBackground(dpy, root, commands[desk].solidColor);
    XClearWindow(dpy, root);
    XFlush(dpy);
#ifdef LOGFILE
    fprintf(logFile,"Color set.\n");
    fflush(logFile);
#endif
    break;
  case 2:
    SetWindowBackground(
      dpy, root, DisplayWidth(dpy, screen), DisplayHeight(dpy, screen),
      &Colorset[commands[desk].colorset % nColorsets],
      DefaultDepth(dpy, screen), DefaultGC(dpy, screen), True);
    XFlush(dpy);
    break;
  case 0:
  case -1:
  default:
    if(commands[desk].cmdStr != NULL)
    {
      SendFvwmPipe(commands[desk].cmdStr, (unsigned long)0);
    }
    break;
  } /* switch */
}

/******************************************************************************
  ProcessMessage - Process the message coming from Fvwm
    Skeleton based on processmessage() from FvwmIdent:
      Copyright 1994, Robert Nation and Nobutaka Suzuki.
******************************************************************************/
void ProcessMessage(unsigned long type,unsigned long *body)
{
  char *tline;
  int colorset = -1;

  switch (type)
  {
  case M_CONFIG_INFO:
    tline = (char*)&(body[3]);
    if (strncasecmp(tline, "colorset", 8) == 0)
    {
      colorset = LoadColorset(tline + 8);
      if (commands[current_desk].colorset == colorset)
      {
	set_desk_background(current_desk);
      }
    }
    break;

  case M_NEW_DESK:
    if (body[0]>DeskCount || commands[body[0]].type == -1)
    {
      return;
    }
#ifdef LOGFILE
    fprintf(logFile,"Desk: %d\n",body[0]);
    fprintf(logFile,"Command type: %d\n",commands[body[0]].type);
    if (commands[body[0]].type == 0)
      fprintf(logFile,"Command String: %s\n",commands[body[0]].cmdStr);
    else if (commands[body[0]].type == 1)
      fprintf(logFile,"Color Number: %d\n",commands[body[0]].solidColor);
    else if (commands[body[0]].type == -1)
      fprintf(logFile,"No Command\n");
    else if (commands[body[0]].type == 2)
      fprintf(logFile,"Use Colorset %d\n",commands[body[0]].colorset);
    else
    {
      fprintf(logFile,"Illegal command type !\n");
      exit(1);
    }
    fflush(logFile);
#endif
    set_desk_background(body[0]);
    current_desk = body[0];
    break;
  default:
    break;
  } /* switch */
}

/******************************************************************************
  SendFvwmPipe - Send a message back to fvwm
    Based on SendInfo() from FvwmIdent:
      Copyright 1994, Robert Nation and Nobutaka Suzuki.
******************************************************************************/
void SendFvwmPipe(char *message,unsigned long window)
{
  int w;
  char *hold,*temp,*temp_msg;
  hold=message;

  while(1)
  {
    temp=strchr(hold,',');
    if (temp!=NULL)
    {
      temp_msg=malloc(temp-hold+1);
      strncpy(temp_msg,hold,(temp-hold));
      temp_msg[(temp-hold)]='\0';
      hold=temp+1;
    } else temp_msg=hold;

    write(Fvwm_fd[0],&window, sizeof(unsigned long));

    w=strlen(temp_msg);
    write(Fvwm_fd[0],&w,sizeof(int));
    write(Fvwm_fd[0],temp_msg,w);

    /* keep going */
    w=1;
    write(Fvwm_fd[0],&w,sizeof(int));

    if(temp_msg!=hold) free(temp_msg);
    else break;
  }
}

/***********************************************************************
  Detected a broken pipe - time to exit
    Based on DeadPipe() from FvwmIdent:
      Copyright 1994, Robert Nation and Nobutaka Suzuki.
 **********************************************************************/
void DeadPipe(int nonsense)
{
  exit(1);
}

/******************************************************************************
  ParseConfig - Parse the configuration file fvwm to us to use
    Based on part of main() from FvwmIdent:
      Copyright 1994, Robert Nation and Nobutaka Suzuki.
******************************************************************************/
void ParseConfig()
{
  char line2[40];
  char *tline;

  sprintf(line2,"*%sDesk",Module);

  InitGetConfigLine(Fvwm_fd,line2);
  GetConfigLine(Fvwm_fd,&tline);

  while(tline != (char *)0)
  {
    if(strlen(tline)>1)
    {
      if(strncasecmp(tline,line2,strlen(line2))==0)
	AddCommand(&tline[strlen(line2)]);
      else if (strncasecmp(tline, "colorset", 8) == 0)
	LoadColorset(tline + 8);
    }
    GetConfigLine(Fvwm_fd,&tline);
  }
}

/******************************************************************************
  AddCommand - Add a command to the correct spot on the dynamic array.
******************************************************************************/
void AddCommand(char *string)
{
  char *temp;
  int num = 0;

  temp=string;
  while(isspace((unsigned char)*temp))
    temp++;
  num=atoi(temp);
  while(!isspace((unsigned char)*temp))
    temp++;
  while(isspace((unsigned char)*temp))
    temp++;
  if (DeskCount<1)
  {
    commands=(Command*)safemalloc((num+1)*sizeof(Command));
    while(DeskCount<num+1) commands[DeskCount++].type= -1;
  }
  else
  {
    if (num+1>DeskCount)
    {
      commands=(Command*)realloc(commands,(num+1)*sizeof(Command));
      while(DeskCount<num+1) commands[DeskCount++].type= -1;
    }
  }
/*  commands[num]=(Command*)safemalloc(sizeof(Command));
*/

  /* Now check the type of command... */
  /* strcpy(commands[num],temp);*/

  if (strncasecmp(temp,"-solid",6)==0)
  {
    char* color;
    char* tmp;
    /* Process a solid color request */

    color = &temp[7];
    while (isspace((unsigned char)*color))
      color++;
    tmp= color;
    while (!isspace((unsigned char)*tmp))
      tmp++;
    *tmp = 0;
    commands[num].type = 1;
    commands[num].solidColor = (!color || !*color) ?
      BlackPixel(dpy, screen) :
      BackerGetColor(color);
#ifdef LOGFILE
    fprintf(logFile,"Adding color: %s as number %d to desk %d\n",
	    color,commands[num].solidColor, num);
    fflush(logFile);
#endif
  }
  else if (strncasecmp(temp, "colorset", 8) == 0)
  {
    if (sscanf(temp + 8, "%d", &commands[num].colorset) < 1)
    {
      commands[num].colorset = 0;
    }
    commands[num].type = 2;
  }
  else
  {
#ifdef LOGFILE
    fprintf(logFile,"Adding command: %s to desk %d\n",temp, num);
    fflush(logFile);
#endif
    commands[num].type = 0;
    commands[num].cmdStr = (char *)safemalloc(strlen(temp)+1);
    strcpy(commands[num].cmdStr,temp);
  }
}
