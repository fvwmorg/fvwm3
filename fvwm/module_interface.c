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

/****************************************************************************
 * This module is all original code
 * by Rob Nation
 * Copyright 1993, Robert Nation
 *     You may use this code for any purpose, as long as the original
 *     copyright remains in the source code and all documentation
 ****************************************************************************/

/***********************************************************************
 *
 * code for launching fvwm modules.
 *
 ***********************************************************************/
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <ctype.h>
#include <stdarg.h>
#include <errno.h>
#include <X11/keysym.h>

#include <fcntl.h>
#include <sys/time.h>
#include <sys/socket.h>

#include "fvwm.h"
#include "fvwmsignal.h"
#include "events.h"
#include "bindings.h"
#include "misc.h"
#include "screen.h"
#include "module_interface.h"
#include "libs/Colorset.h"

int npipes;
int *readPipes;
int *writePipes;
int *pipeOn;
char **pipeName;
/*  RBW - hack for gsfr new config args  */
unsigned long junkzero = 0;

unsigned long *PipeMask;
unsigned long *SyncMask;
struct queue_buff_struct **pipeQueue;

extern fd_set init_fdset;

static void DeleteQueueBuff(int module);
static void AddToQueue(int module, unsigned long *ptr, int size, int done);

void initModules(void)
{
  int i;

  npipes = GetFdWidth();

  writePipes = (int *)safemalloc(sizeof(int)*npipes);
  readPipes = (int *)safemalloc(sizeof(int)*npipes);
  pipeOn = (int *)safemalloc(sizeof(int)*npipes);
  PipeMask = (unsigned long *)safemalloc(sizeof(unsigned long)*npipes);
  SyncMask = (unsigned long *)safemalloc(sizeof(unsigned long)*npipes);
  pipeName = (char **)safemalloc(sizeof(char *)*npipes);
  pipeQueue=(struct queue_buff_struct **)
    safemalloc(sizeof(struct queue_buff_struct *)*npipes);

  for(i=0;i<npipes;i++)
    {
      writePipes[i]= -1;
      readPipes[i]= -1;
      pipeOn[i] = -1;
      PipeMask[i] = MAX_MASK;
      SyncMask[i] = 0; 
      pipeQueue[i] = (struct queue_buff_struct *)NULL;
      pipeName[i] = NULL;
    }
  DBUG("initModules", "Zeroing init module array\n");
  FD_ZERO(&init_fdset);
}

void ClosePipes(void)
{
  int i;
  for(i=0;i<npipes;i++)
    {
      if(writePipes[i]>0)
	{
	  close(writePipes[i]);
	  close(readPipes[i]);
	}
      if(pipeName[i] != NULL)
	{
	  free(pipeName[i]);
	  pipeName[i] = 0;
	}
      while(pipeQueue[i] != NULL)
	{
	  DeleteQueueBuff(i);
	}
    }
}

static int do_execute_module(F_CMD_ARGS, Bool desperate)
{
  int fvwm_to_app[2],app_to_fvwm[2];
  int i,val,nargs = 0;
  int ret_pipe;
  char *cptr;
  char *args[20];
  char *arg1 = NULL;
  char arg2[20];
  char arg3[20];
  char arg5[20];
  char arg6[20];
  extern char *ModulePath;
  extern char *fvwm_file;
  Window win;

  /* Olivier: Why ? */
  /*   if(eventp->type != KeyPress) */
  /*     UngrabEm(); */

  if(action == NULL)
    return -1;

  if(tmp_win)
    win = tmp_win->w;
  else
    win = None;

  /* If we execute a module, don't wait for buttons to come up,
   * that way, a pop-up menu could be implemented */
  *Module = 0;

  action = GetNextToken(action, &cptr);
  if (!cptr)
    return -1;

  arg1 = searchPath( ModulePath, cptr, EXECUTABLE_EXTENSION, X_OK );

  if(arg1 == NULL)
    {
      /* If this function is called in 'desparate' mode this means fvwm is
       * trying a module name as a last resort.  In this case the error message
       * is inappropriate because it was most likely a typo in a command, not
       * a module name. */
      if (!desperate)
      {
	fvwm_msg(ERR,"executeModule",
		 "No such module '%s' in ModulePath '%s'",cptr,ModulePath);
      }
      free(cptr);
      return -1;
    }

#ifdef REMOVE_EXECUTABLE_EXTENSION
  {
      char* p = arg1 + strlen( arg1 ) - strlen( EXECUTABLE_EXTENSION );
      if ( strcmp( p, EXECUTABLE_EXTENSION ) ==  0 )
	  *p = 0;
  }
#endif

  /* Look for an available pipe slot */
  i=0;
  while((i<npipes) && (writePipes[i] >=0))
    i++;
  if(i>=npipes)
    {
      fvwm_msg(ERR,"executeModule","Too many Accessories!");
      free(arg1);
      free(cptr);
      return -1;
    }
  ret_pipe = i;

  /* I want one-ended pipes, so I open two two-ended pipes,
   * and close one end of each. I need one ended pipes so that
   * I can detect when the module crashes/malfunctions */
  if(pipe(fvwm_to_app)!=0)
    {
      fvwm_msg(ERR,"executeModule","Failed to open pipe");
      free(arg1);
      free(cptr);
      return -1;
    }
  if(pipe(app_to_fvwm)!=0)
    {
      fvwm_msg(ERR,"executeModule","Failed to open pipe2");
      free(arg1);
      free(cptr);
      close(fvwm_to_app[0]);
      close(fvwm_to_app[1]);
      return -1;
    }

  pipeName[i] = stripcpy(cptr);
  free(cptr);
  sprintf(arg2,"%d",app_to_fvwm[1]);
  sprintf(arg3,"%d",fvwm_to_app[0]);
  sprintf(arg5,"%lx",(unsigned long)win);
  sprintf(arg6,"%lx",(unsigned long)context);
  args[0]=arg1;
  args[1]=arg2;
  args[2]=arg3;
  if(fvwm_file != NULL)
    args[3]=fvwm_file;
  else
    args[3]="none";
  args[4]=arg5;
  args[5]=arg6;
  nargs = 6;
  while((action != NULL)&&(nargs < 20)&&(args[nargs-1] != NULL))
    {
      args[nargs] = 0;
      action = GetNextToken(action,&args[nargs]);
      nargs++;
    }
  if(args[nargs-1] == NULL)
    nargs--;
  args[nargs] = 0;

  /* Try vfork instead of fork. The man page says that vfork is better! */
  /* Also, had to change exit to _exit() */
  /* Not everyone has vfork! */
  val = fork();
  if(val > 0)
    {
      /* This fork remains running fvwm */
      /* close appropriate descriptors from each pipe so
       * that fvwm will be able to tell when the app dies */
      close(app_to_fvwm[1]);
      close(fvwm_to_app[0]);

      /* add these pipes to fvwm's active pipe list */
      writePipes[i] = fvwm_to_app[1];
      readPipes[i] = app_to_fvwm[0];
      pipeOn[i] = -1;
      PipeMask[i] = MAX_MASK;
      SyncMask[i] = 0;
      free(arg1);
      pipeQueue[i] = NULL;
      if (DoingCommandLine) {
        /* add to the list of command line modules */
        DBUG("executeModule", "starting commandline module\n");
        FD_SET(i, &init_fdset);
      }

      /* make the PositiveWrite pipe non-blocking. Don't want to jam up
	 fvwm because of an uncooperative module */
#ifdef O_NONBLOCK
      fcntl(writePipes[i],F_SETFL,O_NONBLOCK); /* POSIX, better behavior */
#else
      fcntl(writePipes[i],F_SETFL,O_NDELAY); /* early SYSV, bad behavior */
#endif
      /* Mark the pipes close-on exec so other programs
       * won`t inherit them */
      if (fcntl(readPipes[i], F_SETFD, 1) == -1)
	fvwm_msg(ERR,"executeModule","module close-on-exec failed");
      if (fcntl(writePipes[i], F_SETFD, 1) == -1)
	fvwm_msg(ERR,"executeModule","module close-on-exec failed");
      for(i=6;i<nargs;i++)
	{
	  if(args[i] != 0)
	    free(args[i]);
	}
    }
  else if (val ==0)
    {
      /* this is  the child */
      /* this fork execs the module */
#ifdef FORK_CREATES_CHILD
      close(fvwm_to_app[1]);
      close(app_to_fvwm[0]);
#endif
      /* Only modules need to know these */
      putenv(CatString2("FVWM_USERHOME=", user_home_dir));
      if (!Pdefault) {
        char *visualid, *colormap;

        visualid = safemalloc(32);
	sprintf(visualid, "FVWM_VISUALID=%lx", XVisualIDFromVisual(Pvisual));
	putenv(visualid);
	colormap = safemalloc(32);
	sprintf(colormap, "FVWM_COLORMAP=%lx", Pcmap);
	putenv(colormap);
      }

      /** Why is this execvp??  We've already searched the module path! **/
      execvp(arg1,args);
      fvwm_msg(ERR,"executeModule","Execution of module failed: %s",arg1);
      perror("");
      close(app_to_fvwm[1]);
      close(fvwm_to_app[0]);
#ifdef FORK_CREATES_CHILD
      exit(1);
#endif
    }
  else
    {
      fvwm_msg(ERR,"executeModule","Fork failed");
      free(arg1);
      for(i=6;i<nargs;i++)
	{
	  if(args[i] != 0)
	    free(args[i]);
	}
      return -1;
    }

  return ret_pipe;
}

int executeModuleDesperate(F_CMD_ARGS)
{
  return do_execute_module(eventp, w, tmp_win, context, action, Module, True);
}

void executeModule(F_CMD_ARGS)
{
  do_execute_module(eventp, w, tmp_win, context, action, Module, False);
  return;
}

void executeModuleSync(F_CMD_ARGS)
{
  int sec = 0;
  char *next;
  char *token;
  char *expect = NULL;
  struct timeval timeout = {0, 1};
  struct timeval *timeoutP = &timeout; /* use 1 usec timeout in select */
  int pipe_slot;
  fd_set in_fdset;
  fd_set out_fdset;
  Window targetWindow;
  extern fd_set_size_t fd_width;
  time_t start_time;
  Bool done = False;
  Bool need_ungrab = False;
  char *escape = NULL;
  XEvent tmpevent;
  extern FvwmWindow *Tmp_win;

  if (!action)
    return;

  token = PeekToken(action, &next);
  if (StrEquals(token, "expect"))
  {
    token = PeekToken(next, &next);
    if (token)
    {
      expect = alloca(strlen(token) + 1);
      strcpy(expect, token);
    }
    action = next;
    token = PeekToken(action, &next);
  }
  if (token && StrEquals(token, "timeout"))
  {
    if (GetIntegerArguments(next, &next, &sec, 1) > 0 && sec > 0)
    {
      /* we have a delay, skip the number */
      action = next;
    }
    else
    {
      fvwm_msg(ERR, "executeModuleSync", "illegal timeout");
      return;
    }
  }

  if (!action)
  {
    /* no module name */
    return;
  }

  pipe_slot =
    do_execute_module(eventp, w, tmp_win, context, action, Module, False);
  if (pipe_slot == -1)
  {
    /* executing the module failed, just return */
    return;
  }

  /* Busy cursor stuff */
  if (Scr.BusyCursor & BUSY_MODULESYNCHRONOUS)
  {
    if (GrabEm(CRS_WAIT, GRAB_BUSY))
      need_ungrab = True;
  }

  /* wait for module input */
  start_time = time(NULL);

  while (!done)
  {
    FD_ZERO(&in_fdset);
    FD_ZERO(&out_fdset);
    if(readPipes[pipe_slot] >= 0)
      FD_SET(readPipes[pipe_slot], &in_fdset);
    if(pipeQueue[pipe_slot]!= NULL)
      FD_SET(writePipes[pipe_slot], &out_fdset);

    if (fvwmSelect(fd_width, &in_fdset, &out_fdset, 0, timeoutP) > 0)
    {
      if ((readPipes[pipe_slot] >= 0) &&
	  FD_ISSET(readPipes[pipe_slot], &in_fdset))
      {
	/* Check for module input. */
	if (read(readPipes[pipe_slot], &targetWindow, sizeof(Window)) > 0)
	{
	  if (HandleModuleInput(targetWindow, pipe_slot, expect) == 77)
	  {
	    /* we got the message we were waiting for */
	    done = True;
	  }
	}
	else
	{
	  KillModule(pipe_slot);
	  done = True;
	}
      }
      if ((writePipes[pipe_slot] >= 0) &&
	  FD_ISSET(writePipes[pipe_slot], &out_fdset))
      {
        FlushQueue(pipe_slot);
      }
    }
    usleep(1000);
    if (difftime(time(NULL), start_time) >= sec && sec)
    {
      /* timeout */
      done = True;
    }
    /* Check for "escape function" */
    if (XPending(dpy) && XCheckMaskEvent(dpy, KeyPressMask, &tmpevent))
    {
      escape = CheckBinding(Scr.AllBindings,
#ifdef HAVE_STROKE
			    0,
#endif /* HAVE_STROKE */
			    tmpevent.xkey.keycode,
			    tmpevent.xkey.state, GetUnusedModifiers(),
			    GetContext(Tmp_win, &tmpevent, &targetWindow),
			    KEY_BINDING);
      if (escape != NULL)
      {
	if (!strcasecmp(escape,"escapefunc"))
	  done = True;
      }
    }
  } /* while */
  if (need_ungrab) UngrabEm(GRAB_BUSY);
}


/* Changed message from module from 255 to 1000. dje */
int HandleModuleInput(Window w, int channel, char *expect)
{
  char text[MAX_MODULE_INPUT_TEXT_LEN];
  int size;
  int cont,n;
  extern FvwmWindow *ButtonWindow;

  /* Already read a (possibly NULL) window id from the pipe,
   * Now read an fvwm bultin command line */
  n = read(readPipes[channel], &size, sizeof(size));
  if(n < sizeof(size))
    {
      fvwm_msg(ERR, "HandleModuleInput",
               "Fail to read (Module: %i, read: %i, size: %i)",
               channel, n, sizeof(size));
      KillModule(channel);
      return 0;
    }

  if(size > sizeof(text))
    {
      fvwm_msg(ERR, "HandleModuleInput",
               "Module(%i) command is too big (%d), limit is %d",
               channel, size, sizeof(text));
      size=sizeof(text);
    }

  pipeOn[channel] = 1;

  n = read(readPipes[channel],text, size);
  if(n < size)
    {
      fvwm_msg(ERR, "HandleModuleInput",
               "Fail to read command (Module: %i, read: %i, size: %i)",
               channel, n, size);
      KillModule(channel);
      return 0;
    }
  text[n] = '\0';
  /* DB(("Module read[%d] (%dy): `%s'", n, size, text)); */

  n = read(readPipes[channel],&cont, sizeof(cont));
  /* DB(("Module read[%d] cont = %d", n, cont)); */
  if(n < sizeof(cont))
    {
      fvwm_msg(ERR, "HandleModuleInput",
               "Module %i, Size Problems (read: %d, size: %d)", 
	       channel, n, sizeof(cont));
      KillModule(channel);
      return 0;
    }
  if(cont == 0)
    {
      fvwm_msg(ERR, "HandleModuleInput",
               "Module %i, Size Problem: cont is zero", channel);
      KillModule(channel);
    }
  if(strlen(text)>0)
    {
      extern int Context;
      FvwmWindow *tmp_win;

      if(strncasecmp(text,"UNLOCK",6)==0) { /* synchronous response */
        return 66;
      }

      if (expect && strncasecmp(text, expect, strlen(expect)) == 0)
      {
	/* the module sent the expected string */
	return 77;
      }
      else if (!expect && strncasecmp(text, "FINISHED_STARTUP", 16) == 0)
      {
	/* module has finished startup */
	return 77;
      }

      /* perhaps the module would like us to kill it? */
      if(strncasecmp(text,"KillMe",6)==0)
      {
        KillModule(channel);
        return 0;
      }

      /* If a module does XUngrabPointer(), it can now get proper Popups */
      if(StrEquals(text, "popup"))
	Event.xany.type = ButtonPress;
      else
	Event.xany.type = ButtonRelease;
      Event.xany.window = w;

      if (XFindContext (dpy, w, FvwmContext, (caddr_t *) &tmp_win) == XCNOENT)
	{
	  tmp_win = NULL;
	  w = None;
	}

      if(tmp_win)
	{
	  if (!IS_ICONIFIED(tmp_win))
	  {
	    Event.xbutton.x_root = tmp_win->frame_g.x;
	    Event.xbutton.y_root = tmp_win->frame_g.y;
	  }
	  else
	  {
	    Event.xbutton.x_root = tmp_win->icon_x_loc;
	    Event.xbutton.y_root = tmp_win->icon_y_loc;
	  }
	}
      else
	{
          XQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild, &JunkX,&JunkY,
                        &Event.xbutton.x_root,&Event.xbutton.y_root,&JunkMask);
	}
      Event.xbutton.button = 1;
      Event.xbutton.x = 0;
      Event.xbutton.y = 0;
      Event.xbutton.subwindow = None;
      Context = GetContext(tmp_win,&Event,&w);
      ButtonWindow = tmp_win;
      ExecuteFunction(text,tmp_win,&Event,Context,channel,EXPAND_COMMAND);
      ButtonWindow = NULL;
    }
  return 0;
}


RETSIGTYPE DeadPipe(int sig)
{}


void KillModule(int channel)
{
  close(readPipes[channel]);
  close(writePipes[channel]);

  readPipes[channel] = -1;
  writePipes[channel] = -1;
  pipeOn[channel] = -1;
  while(pipeQueue[channel] != NULL)
    {
      DeleteQueueBuff(channel);
    }
  if(pipeName[channel] != NULL)
    {
      free(pipeName[channel]);
      pipeName[channel] = NULL;
    }

  if (fFvwmInStartup) {
    /* remove from list of command line modules */
    DBUG("killModule", "ending command line module\n");
    FD_CLR(channel, &init_fdset);
  }

  return;
}

static void KillModuleByName(char *name)
{
  int i = 0;

  if(name == NULL)
    return;

  while(i<npipes)
    {
      if((pipeName[i] != NULL)&&(matchWildcards(name,pipeName[i])))
	{
	  KillModule(i);
	}
      i++;
    }
  return;
}

void module_zapper(F_CMD_ARGS)
{
  char *module;

  GetNextToken(action,&module);
  if (!module)
    return;
  KillModuleByName(module);
  free(module);
}

static unsigned long *
make_vpacket(unsigned long *body, unsigned long event_type,
             unsigned long num, va_list ap)
{
  extern Time lastTimestamp;
  unsigned long *bp = body;

  *(bp++) = START_FLAG;
  *(bp++) = event_type;
  *(bp++) = num+FvwmPacketHeaderSize;
  *(bp++) = lastTimestamp;

  for (; num > 0; --num)
    *(bp++) = va_arg(ap, unsigned long);

  return body;
}



/* ===========================================================
    RBW - 04/16/1999 - new packet builder for GSFR --
    Arguments are pairs of lengths and argument data pointers.
   =========================================================== */
static unsigned long
make_new_vpacket(unsigned char *body, unsigned long event_type,
		 unsigned long num, va_list ap)
{
  extern Time lastTimestamp;
  unsigned long arglen;
  unsigned long bodylen = 0;
  unsigned long *bp = (unsigned long *)body;
  unsigned long *bp1 = bp;
  unsigned long plen = 0;

  *(bp++) = START_FLAG;
  *(bp++) = event_type;
  /*  Skip length field, we don't know it yet. */
  bp++;
  *(bp++) = lastTimestamp;

  for (; num > 0; --num)  {
      arglen = va_arg(ap, unsigned long);
      bodylen += arglen;
      if (bodylen < FvwmPacketMaxSize_byte) {
	register char *tmp = (char *)bp;
        memcpy(tmp, va_arg(ap, char *), arglen);
        tmp += arglen;
	bp = (unsigned long *)tmp;
      }
    }

  /*
      Round up to a long word boundary. Most of the module interface
      still thinks in terms of an array of long ints, so let's humor it.
  */
  plen = (unsigned long) ((char *)bp - (char *)bp1);
  plen = ((plen + (sizeof(long) - 1)) / sizeof(long)) * sizeof(long);
  *(((unsigned long*)bp1)+2) = (plen / (sizeof(unsigned long)));

  return plen;
}



void
SendPacket(int module, unsigned long event_type, unsigned long num_datum, ...)
{
  unsigned long body[FvwmPacketMaxSize];
  va_list ap;

  va_start(ap, num_datum);
  make_vpacket(body, event_type, num_datum, ap);
  va_end(ap);

  PositiveWrite(module, body, (num_datum+FvwmPacketHeaderSize)*sizeof(body[0]));
}

void
BroadcastPacket(unsigned long event_type, unsigned long num_datum, ...)
{
  unsigned long body[FvwmPacketMaxSize];
  va_list ap;
  int i;

  va_start(ap,num_datum);
  make_vpacket(body, event_type, num_datum, ap);
  va_end(ap);

  for (i=0; i<npipes; i++)
    PositiveWrite(i, body, (num_datum+FvwmPacketHeaderSize)*sizeof(body[0]));
}


/* ============================================================
    RBW - 04/16/1999 - new style packet senders for GSFR --
   ============================================================ */
static void SendNewPacket(int module, unsigned long event_type,
			  unsigned long num_datum, ...)
{
  unsigned char body[FvwmPacketMaxSize_byte];
  va_list ap;
  unsigned long plen;

  va_start(ap,num_datum);
  plen = make_new_vpacket(body, event_type, num_datum, ap);
  va_end(ap);

  PositiveWrite(module, (void *) &body, plen);
}

static void BroadcastNewPacket(unsigned long event_type,
			       unsigned long num_datum, ...)
{
  unsigned char body[FvwmPacketMaxSize_byte];
  va_list ap;
  int i;
  unsigned long plen;

  va_start(ap,num_datum);
  plen = make_new_vpacket(body, event_type, num_datum, ap);
  va_end(ap);

  for (i=0; i<npipes; i++)  {
    PositiveWrite(i, (void *) &body, plen);
  }
}


/* this is broken, the flags may not fit in a word */
#define CONFIGARGS(_t) 24,\
            (_t)->w,\
            (_t)->frame,\
            (unsigned long)(_t),\
            (_t)->frame_g.x,\
            (_t)->frame_g.y,\
            (_t)->frame_g.width,\
            (_t)->frame_g.height,\
            (_t)->Desk,\
            (_t)->flags,\
            (_t)->title_g.height,\
            (_t)->boundary_width,\
	    (_t)->hints.base_width,\
	    (_t)->hints.base_height,\
	    (_t)->hints.width_inc,\
	    (_t)->hints.height_inc,\
            (_t)->hints.min_width,\
            (_t)->hints.min_height,\
            (_t)->hints.max_width,\
            (_t)->hints.max_height,\
            (_t)->icon_w,\
            (_t)->icon_pixmap_w,\
            (_t)->hints.win_gravity,\
            (_t)->TextPixel,\
            (_t)->BackPixel

#ifndef DISABLE_MBC
#define OLDCONFIGARGS(_t) 24,\
            (_t)->w,\
            (_t)->frame,\
            (unsigned long)(_t),\
            (_t)->frame_g.x,\
            (_t)->frame_g.y,\
            (_t)->frame_g.width,\
            (_t)->frame_g.height,\
            (_t)->Desk,\
            old_flags,\
            (_t)->title_g.height,\
            (_t)->boundary_width,\
	    (_t)->hints.base_width,\
	    (_t)->hints.base_height,\
	    (_t)->hints.width_inc,\
	    (_t)->hints.height_inc,\
            (_t)->hints.min_width,\
            (_t)->hints.min_height,\
            (_t)->hints.max_width,\
            (_t)->hints.max_height,\
            (_t)->icon_w,\
            (_t)->icon_pixmap_w,\
            (_t)->hints.win_gravity,\
            (_t)->TextPixel,\
            (_t)->BackPixel

#define SETOLDFLAGS \
{ int i = 1; \
  old_flags |= DO_START_ICONIC(t)		? i : 0; i<<=1; \
  old_flags |= False /* OnTop */		? i : 0; i<<=1; \
  old_flags |= IS_STICKY(t)			? i : 0; i<<=1; \
  old_flags |= DO_SKIP_WINDOW_LIST(t)		? i : 0; i<<=1; \
  old_flags |= IS_ICON_SUPPRESSED(t)		? i : 0; i<<=1; \
  old_flags |= HAS_NO_ICON_TITLE(t)		? i : 0; i<<=1; \
  old_flags |= IS_LENIENT(t)			? i : 0; i<<=1; \
  old_flags |= IS_ICON_STICKY(t)		? i : 0; i<<=1; \
  old_flags |= DO_SKIP_ICON_CIRCULATE(t)	? i : 0; i<<=1; \
  old_flags |= DO_SKIP_CIRCULATE(t)		? i : 0; i<<=1; \
  old_flags |= HAS_CLICK_FOCUS(t)		? i : 0; i<<=1; \
  old_flags |= HAS_SLOPPY_FOCUS(t)		? i : 0; i<<=1; \
  old_flags |= !DO_NOT_SHOW_ON_MAP(t)		? i : 0; i<<=1; \
  old_flags |= HAS_BORDER(t)			? i : 0; i<<=1; \
  old_flags |= HAS_TITLE(t)			? i : 0; i<<=1; \
  old_flags |= IS_MAPPED(t)			? i : 0; i<<=1; \
  old_flags |= IS_ICONIFIED(t)			? i : 0; i<<=1; \
  old_flags |= IS_TRANSIENT(t)			? i : 0; i<<=1; \
  old_flags |= False /* Raised */		? i : 0; i<<=1; \
  old_flags |= IS_FULLY_VISIBLE(t)		? i : 0; i<<=1; \
  old_flags |= IS_ICON_OURS(t)			? i : 0; i<<=1; \
  old_flags |= IS_PIXMAP_OURS(t)		? i : 0; i<<=1; \
  old_flags |= IS_ICON_SHAPED(t)		? i : 0; i<<=1; \
  old_flags |= IS_MAXIMIZED(t)			? i : 0; i<<=1; \
  old_flags |= WM_TAKES_FOCUS(t)		? i : 0; i<<=1; \
  old_flags |= WM_DELETES_WINDOW(t)		? i : 0; i<<=1; \
  old_flags |= IS_ICON_MOVED(t)			? i : 0; i<<=1; \
  old_flags |= IS_ICON_UNMAPPED(t)		? i : 0; i<<=1; \
  old_flags |= IS_MAP_PENDING(t)		? i : 0; i<<=1; \
  old_flags |= HAS_MWM_OVERRIDE_HINTS(t)	? i : 0; i<<=1; \
  old_flags |= HAS_MWM_BUTTONS(t)		? i : 0; i<<=1; \
  old_flags |= HAS_MWM_BORDER(t)		? i : 0; }
#endif /* DISABLE_MBC */



/* ===============================================================
    RBW - 04/16/1999 - new version for GSFR --
        - args are now pairs:
          - length of arg data
          - pointer to arg data
        - number of arguments is the number of length/pointer pairs.
        - the 9th field, where flags used to be, is temporarily left
        as a dummy to preserve alignment of the other fields in the
        old packet: we should drop this before the next release.
   =============================================================== */
#define CONFIGARGSNEW(_t) 25,\
	    (unsigned long)(sizeof(unsigned long)),\
            &(*(_t))->w,\
	    (unsigned long)(sizeof(unsigned long)),\
            &(*(_t))->frame,\
	    (unsigned long)(sizeof(unsigned long)),\
            &(_t),\
	    (unsigned long)(sizeof(unsigned long)),\
            &(*(_t))->frame_g.x,\
	    (unsigned long)(sizeof(unsigned long)),\
            &(*(_t))->frame_g.y,\
	    (unsigned long)(sizeof(unsigned long)),\
            &(*(_t))->frame_g.width,\
	    (unsigned long)(sizeof(unsigned long)),\
            &(*(_t))->frame_g.height,\
	    (unsigned long)(sizeof(unsigned long)),\
            &(*(_t))->Desk,\
 \
/* RBW - temp hack to preserve old alignment */ \
	    (unsigned long)(sizeof(unsigned long)),\
            (void *) &junkzero,\
 \
	    (unsigned long)(sizeof(unsigned long)),\
            &(*(_t))->title_g.height,\
	    (unsigned long)(sizeof(unsigned long)),\
            &(*(_t))->boundary_width,\
	    (unsigned long)(sizeof(unsigned long)),\
	    &(*(_t))->hints.base_width,\
	    (unsigned long)(sizeof(unsigned long)),\
	    &(*(_t))->hints.base_height,\
	    (unsigned long)(sizeof(unsigned long)),\
	    &(*(_t))->hints.width_inc,\
	    (unsigned long)(sizeof(unsigned long)),\
	    &(*(_t))->hints.height_inc,\
	    (unsigned long)(sizeof(unsigned long)),\
            &(*(_t))->hints.min_width,\
	    (unsigned long)(sizeof(unsigned long)),\
            &(*(_t))->hints.min_height,\
	    (unsigned long)(sizeof(unsigned long)),\
            &(*(_t))->hints.max_width,\
	    (unsigned long)(sizeof(unsigned long)),\
            &(*(_t))->hints.max_height,\
	    (unsigned long)(sizeof(unsigned long)),\
            &(*(_t))->icon_w,\
	    (unsigned long)(sizeof(unsigned long)),\
            &(*(_t))->icon_pixmap_w,\
	    (unsigned long)(sizeof(unsigned long)),\
            &(*(_t))->hints.win_gravity,\
	    (unsigned long)(sizeof(unsigned long)),\
            &(*(_t))->TextPixel,\
	    (unsigned long)(sizeof(unsigned long)),\
            &(*(_t))->BackPixel,\
	    (unsigned long)(sizeof((*(_t))->flags)),\
            &(*(_t))->flags



void SendConfig(int module, unsigned long event_type, const FvwmWindow *t)
{
const FvwmWindow **t1 = &t;

/*  RBW-  SendPacket(module, event_type, CONFIGARGS(t)); */
  SendNewPacket(module, event_type, CONFIGARGSNEW(t1));
#ifndef DISABLE_MBC
  /* send out an old version of the packet to keep old mouldules happy */
  /* fixme: should test to see if module wants this packet first */
  {
    long old_flags = 0;
    SETOLDFLAGS
    SendPacket(module, event_type == M_ADD_WINDOW ?
                       M_OLD_ADD_WINDOW : M_OLD_CONFIGURE_WINDOW,
               OLDCONFIGARGS(t));
  }
#endif /* DISABLE_MBC */
}


void BroadcastConfig(unsigned long event_type, const FvwmWindow *t)
{
const FvwmWindow **t1 = &t;

/*  RBW-  BroadcastPacket(event_type, CONFIGARGS(t)); */
  BroadcastNewPacket(event_type, CONFIGARGSNEW(t1));
#ifndef DISABLE_MBC
  /* send out an old version of the packet to keep old mouldules happy */
  {
    long old_flags = 0;
    SETOLDFLAGS
    BroadcastPacket(event_type == M_ADD_WINDOW ?
                    M_OLD_ADD_WINDOW : M_OLD_CONFIGURE_WINDOW,
                    OLDCONFIGARGS(t));
  }
#endif /* DISABLE_MBC */
}


static unsigned long *
make_named_packet(int *len, unsigned long event_type, const char *name,
                  int num, ...)
{
  unsigned long *body;
  va_list ap;

  /* Packet is the header plus the items plus enough items to hold the name
     string.  */
  *len = FvwmPacketHeaderSize + num + (strlen(name) / sizeof(unsigned long)) + 1;

  body = (unsigned long *)safemalloc(*len * sizeof(unsigned long));
  body[*len-1] = 0; /* Zero out end of memory to avoid uninit memory access. */

  va_start(ap, num);
  make_vpacket(body, event_type, num, ap);
  va_end(ap);

  strcpy((char *)&body[FvwmPacketHeaderSize+num], name);
  body[2] = *len;

  /* DB(("Packet (%lu): %lu %lu %lu `%s'", *len,
       body[FvwmPacketHeaderSize], body[FvwmPacketHeaderSize+1], body[FvwmPacketHeaderSize+2], name)); */

  return (body);
}

void
SendName(int module, unsigned long event_type,
         unsigned long data1,unsigned long data2, unsigned long data3,
         const char *name)
{
  unsigned long *body;
  int l;

  if (name == NULL)
    return;

  body = make_named_packet(&l, event_type, name, 3, data1, data2, data3);
  PositiveWrite(module, body, l*sizeof(unsigned long));
  free(body);
}

void
BroadcastName(unsigned long event_type,
              unsigned long data1, unsigned long data2, unsigned long data3,
              const char *name)
{
  unsigned long *body;
  int i, l;

  if (name == NULL)
    return;

  body = make_named_packet(&l, event_type, name, 3, data1, data2, data3);

  for (i=0; i < npipes; i++)
    PositiveWrite(i, body, l*sizeof(unsigned long));

  free(body);
}

#ifdef MINI_ICONS
static void
SendMiniIcon(int module, unsigned long event_type,
             unsigned long data1, unsigned long data2,
             unsigned long data3, unsigned long data4,
             unsigned long data5, unsigned long data6,
             unsigned long data7, unsigned long data8,
             const char *name)
{
  unsigned long *body;
  int l;

  if ((name == NULL) || (event_type != M_MINI_ICON))
    return;

  body = make_named_packet(&l, event_type, name, 8, data1, data2, data3,
                           data4, data5, data6, data7, data8);
  PositiveWrite(module, body, l*sizeof(unsigned long));
  free(body);
}

void
BroadcastMiniIcon(unsigned long event_type,
                  unsigned long data1, unsigned long data2,
                  unsigned long data3, unsigned long data4,
                  unsigned long data5, unsigned long data6,
                  unsigned long data7, unsigned long data8,
                  const char *name)
{
  unsigned long *body;
  int i, l;

  body = make_named_packet(&l, event_type, name, 8, data1, data2, data3,
                           data4, data5, data6, data7, data8);

  for (i=0; i < npipes; i++)
    PositiveWrite(i, body, l*sizeof(unsigned long));

  free(body);
}
#endif /* MINI_ICONS */

/**********************************************************************
 * Reads a colorset command from a module and broadcasts it back out
 **********************************************************************/
void BroadcastColorset(int n)
{
  int i;
  char *buf;

  buf = DumpColorset(n, &Colorset[n]);
  for (i=0; i < npipes; i++)
  {
    /* just a quick check to save us lots of overhead */
    if (pipeOn[i] >= 0)
      SendName(i, M_CONFIG_INFO, 0, 0, 0, buf);
  }
}


/*
** send an arbitrary string to all instances of a module
*/
void SendStrToModule(XEvent *eventp,Window junk,FvwmWindow *tmp_win,
                     unsigned long context, char *action,int* Module)
{
  char *module,*str;
  unsigned long data0, data1, data2;
  int i;

  /* FIXME: Without this, popup menus can't be implemented properly in
   *  modules.  Olivier: Why ? */
  /* UngrabEm(); */
  if (!action)
    return;
  GetNextToken(action,&module);
  if (!module)
    return;
  str = strdup(action + strlen(module) + 1);

  if (tmp_win) {
    /* Modules may need to know which window this applies to */
    data0 = tmp_win->w;
    data1 = tmp_win->frame;
    data2 = (unsigned long)tmp_win;
  } else {
    data0 = 0;
    data1 = 0;
    data2 = 0;
  }
  for (i=0;i<npipes;i++)
  {
    if((pipeName[i] != NULL)&&(matchWildcards(module,pipeName[i])))
    {
      SendName(i,M_STRING,data0,data1,data2,str);
      FlushQueue(i);
    }
  }

  free(module);
  free(str);
}


/* This used to be marked "fvwm_inline".  I removed this
   when I added the lockonsend logic.  The routine seems to big to
   want to inline.  dje 9/4/98 */
extern int myxgrabcount;                /* defined in libs/Grab.c */
int PositiveWrite(int module, unsigned long *ptr, int size)
{
  if((pipeOn[module]<0)||(!((PipeMask[module]) & ptr[1])))
    return -1;

  /* a dirty hack to prevent FvwmAnimate triggering during Recapture */
  /* would be better to send RecaptureStart and RecaptureEnd messages. */
  /* If module is lock on send for iconify message and its an 
   * iconify event and server grabbed, then return */
  if ((SyncMask[module] & M_ICONIFY & ptr[1]) && (myxgrabcount != 0)) {
    return -1;
  }

  AddToQueue(module,ptr,size,0);

  /* dje, from afterstep, for FvwmAnimate, allows modules to sync with fvwm.
   * this is disabled when the server is grabbed, otherwise deadlocks happen. 
   * M_LOCKONSEND has been replaced by a separated mask which defines on 
   * which messages the fvwm-to-module communication need to be lock 
   * on send. olicha Nov 13, 1999 */
  if ((SyncMask[module] & ptr[1]) && !myxgrabcount) {
    Window targetWindow;
    int e;

    FlushQueue(module);
    fcntl(readPipes[module],F_SETFL,0);
    while ((e = read(readPipes[module],&targetWindow, sizeof(Window))) > 0) {
      if (HandleModuleInput(targetWindow, module, NULL) == 66) {
        break;
      }
    }
    if (e <= 0) {
      KillModule(module);
    }
    fcntl(readPipes[module],F_SETFL,O_NDELAY);
  }
  return size;
}


static void AddToQueue(int module, unsigned long *ptr, int size, int done)
{
  struct queue_buff_struct *c,*e;
  unsigned long *d;

  c = (struct queue_buff_struct *)safemalloc(sizeof(struct queue_buff_struct));
  c->next = NULL;
  c->size = size;
  c->done = done;
  d = (unsigned long *)safemalloc(size);
  c->data = d;
  memcpy((void*)d,(const void*)ptr,size);

  e = pipeQueue[module];
  if(e == NULL)
    {
      pipeQueue[module] = c;
      return;
    }
  while(e->next != NULL)
    e = e->next;
  e->next = c;
}

static void DeleteQueueBuff(int module)
{
  struct queue_buff_struct *a;

  if(pipeQueue[module] == NULL)
     return;
  a = pipeQueue[module];
  pipeQueue[module] = a->next;
  free(a->data);
  free(a);
  return;
}

void FlushQueue(int module)
{
  char *dptr;
  struct queue_buff_struct *d;
  int a;
  extern int errno;

  if((pipeOn[module] <= 0)||(pipeQueue[module] == NULL))
    return;

  while(pipeQueue[module] != NULL)
    {
      d = pipeQueue[module];
      dptr = (char *)d->data;
      while(d->done < d->size)
	{
	  a = write(writePipes[module],&dptr[d->done], d->size - d->done);
	  if(a >=0)
	    d->done += a;
	  /* the write returns EWOULDBLOCK or EAGAIN if the pipe is full.
	   * (This is non-blocking I/O). SunOS returns EWOULDBLOCK, OSF/1
	   * returns EAGAIN under these conditions. Hopefully other OSes
	   * return one of these values too. Solaris 2 doesn't seem to have
	   * a man page for write(2) (!) */
	  else if ((errno == EWOULDBLOCK)||(errno == EAGAIN)||(errno==EINTR))
	    {
	      return;
	    }
	  else
	    {
	      KillModule(module);
	      return;
	    }
	}
      DeleteQueueBuff(module);
    }
}

void FlushOutputQueues(void)
{
  int i;

  for(i = 0; i < npipes; i++) {
    if(writePipes[i] >= 0) {
      FlushQueue(i);
    }
  }
}

void send_list_func(XEvent *eventp, Window w, FvwmWindow *tmp_win,
                    unsigned long context, char *action, int *Module)
{
  FvwmWindow *t;

  if(*Module >= 0)
    {
      SendPacket(*Module, M_NEW_DESK, 1, Scr.CurrentDesk);
      SendPacket(*Module, M_NEW_PAGE, 5,
                 Scr.Vx, Scr.Vy, Scr.CurrentDesk, Scr.VxMax, Scr.VyMax);

      if(Scr.Hilite != NULL)
	SendPacket(*Module, M_FOCUS_CHANGE, 5,
                   Scr.Hilite->w,
                   Scr.Hilite->frame,
		   (unsigned long)True,
		   Scr.DefaultDecor.HiColors.fore,
		   Scr.DefaultDecor.HiColors.back);
      else
	SendPacket(*Module, M_FOCUS_CHANGE, 5,
                   0, 0, (unsigned long)True,
                   Scr.DefaultDecor.HiColors.fore,
                   Scr.DefaultDecor.HiColors.back);
      if (Scr.DefaultIcon != NULL)
	SendName(*Module, M_DEFAULTICON, 0, 0, 0, Scr.DefaultIcon);

      for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
	{
	  SendConfig(*Module,M_CONFIGURE_WINDOW,t);
	  SendName(*Module,M_WINDOW_NAME,t->w,t->frame,
		   (unsigned long)t,t->name);
	  SendName(*Module,M_ICON_NAME,t->w,t->frame,
		   (unsigned long)t,t->icon_name);

          if (t->icon_bitmap_file != NULL
              && t->icon_bitmap_file != Scr.DefaultIcon)
            SendName(*Module,M_ICON_FILE,t->w,t->frame,
                     (unsigned long)t,t->icon_bitmap_file);

	  SendName(*Module,M_RES_CLASS,t->w,t->frame,
		   (unsigned long)t,t->class.res_class);
	  SendName(*Module,M_RES_NAME,t->w,t->frame,
		   (unsigned long)t,t->class.res_name);

	  if((IS_ICONIFIED(t))&&(!IS_ICON_UNMAPPED(t)))
	    SendPacket(*Module, M_ICONIFY, 7, t->w, t->frame,
		       (unsigned long)t,
		       t->icon_x_loc, t->icon_y_loc,
		       t->icon_w_width, t->icon_w_height+t->icon_p_height);

	  if((IS_ICONIFIED(t))&&(IS_ICON_UNMAPPED(t)))
	    SendPacket(*Module, M_ICONIFY, 7, t->w, t->frame,
		       (unsigned long)t,
                       0, 0, 0, 0);
#ifdef MINI_ICONS
	  if (t->mini_icon != NULL)
            SendMiniIcon(*Module, M_MINI_ICON,
                         t->w, t->frame, (unsigned long)t,
                         t->mini_icon->width,
                         t->mini_icon->height,
                         t->mini_icon->depth,
                         t->mini_icon->picture,
                         t->mini_icon->mask,
                         t->mini_pixmap_file);
#endif
	}

      if(Scr.Hilite == NULL)
	  BroadcastPacket(M_FOCUS_CHANGE, 5,
                          0, 0, (unsigned long)True,
                          Scr.DefaultDecor.HiColors.fore,
                          Scr.DefaultDecor.HiColors.back);
      else
	  BroadcastPacket(M_FOCUS_CHANGE, 5,
                          Scr.Hilite->w,
                          Scr.Hilite->frame,
                          (unsigned long)True,
                          Scr.DefaultDecor.HiColors.fore,
                          Scr.DefaultDecor.HiColors.back);

      SendPacket(*Module, M_END_WINDOWLIST, 0);
    }
}

void set_mask_function(F_CMD_ARGS)
{
  int val = 0;

  GetIntegerArguments(action, NULL, &val, 1);
  PipeMask[*Module] = (unsigned long)val;
}

void setSyncMaskFunc(F_CMD_ARGS)
{
  int val = 0;

  GetIntegerArguments(action, NULL, &val, 1);
  SyncMask[*Module] = (unsigned long)val;
}
