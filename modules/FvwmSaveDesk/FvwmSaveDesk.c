/* This module, and the entire FvwmSaveDesktop program, and the concept for
 * interfacing this module to the Window Manager, are all original work
 * by Robert Nation and Mr. Per Persson <pp@solace.mh.se>
 *
 * The concept to write an function definition instead of an new.xinitrc
 * is from Carsten Paeth <calle@calle.in-berlin.de>
 *
 * Copyright 1994, Robert Nation and Mr. Per Persson.
 *  No guarantees or warantees or anything
 * are provided or implied in any way whatsoever. Use this program at your
 * own risk. Permission to use this program for any purpose is given,
 * as long as the copyright is kept intact.
 *
 * Copyright 1995, Carsten Paeth.
 *  No guarantees or warantees or anything
 * are provided or implied in any way whatsoever. Use this program at your
 * own risk. Permission to use this program for any purpose is given,
 * as long as the copyright is kept intact.
 *
 */

#define TRUE 1
#define FALSE 0

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
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>

#include "../../fvwm/module.h"

#include "FvwmSaveDesk.h"

char *MyName;
int fd[2];

struct list *list_root = NULL;

Display *dpy;			/* which display are we talking to */
int ScreenWidth, ScreenHeight;
int screen;

long Vx, Vy;

long CurDesk = 1;   /* actual Desktop while being called */

/***********************************************************************
 *
 *  Procedure:
 *	main - start of module
 *
 ***********************************************************************/
int main(int argc, char **argv)
{
  char *temp, *s;
  char *display_name = NULL;

  /* Record the program name for error messages */
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

  /* Open the X display */
  if (!(dpy = XOpenDisplay(display_name)))
    {
      fprintf(stderr,"%s: can't open display %s", MyName,
	      XDisplayName(display_name));
      exit (1);
    }
  screen= DefaultScreen(dpy);
  ScreenHeight = DisplayHeight(dpy,screen);
  ScreenWidth = DisplayWidth(dpy,screen);

  /* We should exit if our fvwm pipes die */
  signal (SIGPIPE, DeadPipe);

  fd[0] = atoi(argv[1]);
  fd[1] = atoi(argv[2]);

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
  unsigned long header[HEADER_SIZE], *body;
  int count;

  while(1)
    {
      /* read a packet */
      if((count = ReadFvwmPacket(fd[1],header,&body)) > 0)
	{
	  /* dispense with the new packet */
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
    case M_CONFIGURE_WINDOW:
      if(!find_window(body[0]))
	add_window(body[0],body);
      break;
    case M_WINDOW_NAME:
      {
	struct list *l;
	if ((l = find_window(body[0])) != 0) {
	   l->name = (char *)safemalloc(strlen((char *)&body[3])+1);
	   strcpy(l->name,(char *)&body[3]);
	}
      }
      break;
    case M_NEW_PAGE:
      list_new_page(body);
      break;
    case M_NEW_DESK:
      CurDesk = (long)body[0];
      break;
    case M_END_WINDOWLIST:
      do_save();
      break;
    default:
      break;
    }
}



/***********************************************************************
 *
 *  Procedure:
 *	find_window - find a window in the current window list
 *
 ***********************************************************************/
struct list *find_window(unsigned long id)
{
  struct list *l;

  if(list_root == NULL)
    return NULL;

  for(l = list_root; l!= NULL; l= l->next)
    {
      if(l->id == id)
	return l;
    }
  return NULL;
}



/***********************************************************************
 *
 *  Procedure:
 *	add_window - add a new window in the current window list
 *
 ***********************************************************************/
void add_window(unsigned long new_win, unsigned long *body)
{
  struct list *t;

  if(new_win == 0)
    return;

  t = (struct list *)safemalloc(sizeof(struct list));
  t->id = new_win;
  t->next = list_root;
  t->frame_height = (int)body[6];
  t->frame_width = (int)body[5];
  t->base_width = (int)body[11];
  t->base_height = (int)body[12];
  t->width_inc  = (int)body[13];
  t->height_inc  = (int)body[14];
  t->frame_x  = (int)body[3];
  t->frame_y  = (int)body[4];;
  t->title_height  = (int)body[9];;
  t->boundary_width  = (int)body[10];
  t->flags = (unsigned long)body[8];
  t->gravity = body[21];
  t->desk = body[7];
  t->name = 0;
  list_root = t;
}



/***********************************************************************
 *
 *  Procedure:
 *	list_new_page - capture new-page info
 *
 ***********************************************************************/
void list_new_page(unsigned long *body)
{
  Vx = (long)body[0];
  Vy = (long)body[1];
}
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


/***********************************************************************
 *
 *  Procedure:
 *	writes a command line argument to file "out"
 *      checks for qoutes and stuff
 *
 ***********************************************************************/
void write_string(FILE *out, char *line)
{
  int len,space = 0, qoute = 0,i;

  len = strlen(line);

  for(i=0;i<len;i++)
    {
      if(isspace(line[i]))
	space = 1;
      if(line[i]=='\"')
	qoute = 1;
    }
  if(space == 1)
    fprintf(out,"\"");
  if(qoute == 0)
    fprintf(out,"%s",line);
  else
    {
      for(i=0;i<len;i++)
	{
	  if(line[i]=='\"')
	    fprintf(out,"\\\"");
	  else
	    fprintf(out,"%c",line[i]);
	}
    }
  if(space == 1)
    fprintf(out,"\"");
  fprintf(out," ");

}

void do_save_command(FILE *out, struct list *t, int *curdesk,
                                int emit_wait, int *isfirstline)
{
   char tname[200],loc[30];
   char **command_list;
   int dwidth,dheight,xtermline = 0;
   int x1,x2,y1,y2,i,command_count;
   long tVx, tVy;

   tname[0]=0;

   x1 = t->frame_x;
   x2 = ScreenWidth - x1 - t->frame_width - 2;
   if(x2 < 0)
     x2 = 0;
   y1 = t->frame_y;
   y2 = ScreenHeight - y1 -  t->frame_height - 2;
   if(y2 < 0)
     y2 = 0;
   dheight = t->frame_height - t->title_height - 2*t->boundary_width;
   dwidth = t->frame_width - 2*t->boundary_width;
   dwidth -= t->base_width ;
   dheight -= t->base_height ;
   dwidth /= t->width_inc;
   dheight /= t->height_inc;

   if ( t->flags & STICKY )
     {
       tVx = 0;
       tVy = 0;
     }
   else
     {
       tVx = Vx;
       tVy = Vy;
     }
   sprintf(tname,"%dx%d",dwidth,dheight);
   if ((t->gravity == EastGravity) ||
       (t->gravity == NorthEastGravity) ||
       (t->gravity == SouthEastGravity))
     sprintf(loc,"-%d",x2);
   else
     sprintf(loc,"+%d",x1+(int)tVx);
   strcat(tname, loc);

   if((t->gravity == SouthGravity)||
      (t->gravity == SouthEastGravity)||
      (t->gravity == SouthWestGravity))
     sprintf(loc,"-%d",y2);
   else
     sprintf(loc,"+%d",y1+(int)tVy);
   strcat(tname, loc);

   if ( XGetCommand( dpy, t->id, &command_list, &command_count ) )
     {
       if (*curdesk != t->desk)
	 {
	    fprintf( out, "%s\t\"I\" Desk 0 %ld\n", *isfirstline ? "" : "+", t->desk);
	    fflush ( out );
	    if (*isfirstline) *isfirstline = 0;
	    *curdesk = t->desk;
	 }

       fprintf( out, "%s\t\t\"I\" Exec ",  *isfirstline ? "" : "+");
       if (*isfirstline) *isfirstline = 0;
       fflush ( out );
       for (i=0; i < command_count; i++)
	 {
	   if ( strncmp( "-geo", command_list[i], 4) == 0)
	     {
	       i++;
	       continue;
	     }
	   if ( strncmp( "-ic", command_list[i], 3) == 0)
	     continue;
	   if ( strncmp( "-display", command_list[i], 8) == 0)
	     {
	       i++;
	       continue;
	     }
	   write_string(out,command_list[i]);
	   fflush ( out );
	   if(strstr(command_list[i], "xterm"))
	     {
	       fprintf( out, "-geometry %s ", tname );
	       if ( t->flags & ICONIFIED )
		 fprintf(out, "-ic ");
	       xtermline = 1;
	       fflush ( out );
	     }
	 }
       if ( command_count > 0 )
	 {
	  if ( xtermline == 0 )
	    {
	      if ( t->flags & ICONIFIED )
		fprintf(out, "-ic ");
	      fprintf( out, "-geometry %s &\n", tname);
	    }
	  else
	    {
	      fprintf( out, "&\n");
	    }
       }
       if (emit_wait) {
	  if (t->name)
	     fprintf( out, "+\t\t\"I\" Wait %s\n", t->name);
	  else fprintf( out, "+\t\t\"I\" Wait %s\n", command_list[0]);
	  fflush( out );
       }
       XFreeStringList( command_list );
       xtermline = 0;
     }
}


/***********************************************************************
 *
 *  Procedure:
 *	checks to see if we are supposed to take some action now,
 *      finds time for next action to be performed.
 *
 ***********************************************************************/
void do_save(void)
{
  struct list *t;
  char fnbuf[200];
  FILE *out;
  int maxdesk = 0;
  int actdesk = -1;
  int curdesk;
  int isfirstline = 1;

  for (t = list_root; t != NULL; t = t->next)
     if (t->desk > maxdesk)
	maxdesk = t->desk;

  sprintf(fnbuf, "%s/.fvwm2desk", getenv( "HOME" ) );
  out = fopen( fnbuf, "w" );

  fprintf( out, "AddToFunc InitFunction");
  fflush ( out );

  /*
   * Generate all Desks except 'CurDesk'
   */
  for (curdesk = 0; curdesk <= maxdesk; curdesk++)
    {
        for (t = list_root; t != NULL; t = t->next)
	   {
	       if (t->desk != CurDesk && curdesk == t->desk)
                  do_save_command(out, t, &actdesk, 1, &isfirstline);
	   }
    }
  /*
   * Generate 'CurDesk'
   */
  for (t = list_root; t != NULL; t = t->next)
     {
	 if (t->desk == CurDesk)
	    do_save_command(out, t, &actdesk, 0, &isfirstline);
     }

  if (actdesk != CurDesk)
     fprintf( out, "+\t\"I\" Desk 0 %ld\n", CurDesk);

  fflush( out );
  fclose( out );
  exit(0);

}

