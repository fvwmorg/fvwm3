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
#include <stdarg.h>
#include <errno.h>
#include <time.h>

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "libs/fvwmlib.h"
#include "libs/Colorset.h"
#include "libs/FScreen.h"
#include "fvwm.h"
#include "externs.h"
#include "cursor.h"
#include "functions.h"
#include "bindings.h"
#include "misc.h"
#include "screen.h"
#include "module_interface.h"
#include "read.h"
#include "events.h"
#include "fvwmsignal.h"

/*
 * Use POSIX behaviour if we can, otherwise use SysV instead
 */
#ifndef O_NONBLOCK
#  define O_NONBLOCK  O_NDELAY
#endif

int npipes;
int *readPipes;
int *writePipes;
int *pipeOn;
char **pipeName;
#ifndef WITHOUT_KILLMODULE_ALIAS_SUPPORT
char **pipeAlias;  /* as given in: Module FvwmPager MyAlias */
#endif

unsigned long *PipeMask;
unsigned long *SyncMask;
unsigned long *NoGrabMask;
struct queue_buff_struct **pipeQueue;

extern fd_set init_fdset;

static void DeleteMessageQueueBuff(int module);
static void AddToMessageQueue(int module, unsigned long *ptr, int size, int done);
static void AddToCommandQueue(Window w, int module, char * command);

void initModules(void)
{
  int i;

  npipes = GetFdWidth();

  writePipes = (int *)safemalloc(sizeof(int)*npipes);
  readPipes = (int *)safemalloc(sizeof(int)*npipes);
  pipeOn = (int *)safemalloc(sizeof(int)*npipes);
  PipeMask = (unsigned long *)safemalloc(sizeof(unsigned long)*npipes);
  SyncMask = (unsigned long *)safemalloc(sizeof(unsigned long)*npipes);
  NoGrabMask = (unsigned long *)safemalloc(sizeof(unsigned long)*npipes);
  pipeName = (char **)safemalloc(sizeof(char *)*npipes);
#ifndef WITHOUT_KILLMODULE_ALIAS_SUPPORT
  pipeAlias= (char **)safemalloc(sizeof(char *)*npipes);
#endif
  pipeQueue=(struct queue_buff_struct **)
    safemalloc(sizeof(struct queue_buff_struct *)*npipes);

  for(i=0;i<npipes;i++)
    {
      writePipes[i]= -1;
      readPipes[i]= -1;
      pipeOn[i] = -1;
      PipeMask[i] = MAX_MASK;
      SyncMask[i] = 0;
      NoGrabMask[i] = 0;
      pipeQueue[i] = (struct queue_buff_struct *)NULL;
      pipeName[i] = NULL;
#ifndef WITHOUT_KILLMODULE_ALIAS_SUPPORT
      pipeAlias[i] = NULL;
#endif
    }
  DBUG("initModules", "Zeroing init module array\n");
  FD_ZERO(&init_fdset);
}

void ClosePipes(void)
{
    int i;
    for(i=0; i<npipes; ++i)
    {
      if(writePipes[i] > 0)
      {
        close(writePipes[i]);
        close(readPipes[i]);
      }
      if(pipeName[i] != NULL)
      {
        free(pipeName[i]);
        pipeName[i] = 0;
      }
#ifndef WITHOUT_KILLMODULE_ALIAS_SUPPORT
      if(pipeAlias[i] != NULL)
      {
        free(pipeAlias[i]);
        pipeAlias[i] = NULL;
      }
#endif
      while(pipeQueue[i] != NULL)
      {
        DeleteMessageQueueBuff(i);
      }
    }
}

static int do_execute_module(F_CMD_ARGS, Bool desperate)
{
  int fvwm_to_app[2],app_to_fvwm[2];
  int i,val,nargs = 0;
  int ret_pipe;
  char *cptr;
  char **args;
  char *arg1 = NULL;
  char arg2[20];
  char arg3[20];
  char arg5[20];
  char arg6[20];
  char *token;
  extern char *ModulePath;
  Window win;

  args = (char **)safemalloc(7 * sizeof(char *));

  /* Olivier: Why ? */
  /*   if(eventp->type != KeyPress) */
  /*     UngrabEm(); */

  if(action == NULL)
  {
    free(args);
    return -1;
  }

  if(tmp_win)
    win = tmp_win->w;
  else
    win = None;

  /* If we execute a module, don't wait for buttons to come up,
   * that way, a pop-up menu could be implemented */
  *Module = 0;

  action = GetNextToken(action, &cptr);
  if (!cptr)
  {
    free(args);
    return -1;
  }

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
    free(args);
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
    free(args);
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
    free(args);
    return -1;
  }
  if(pipe(app_to_fvwm)!=0)
  {
    fvwm_msg(ERR,"executeModule","Failed to open pipe2");
    free(arg1);
    free(cptr);
    close(fvwm_to_app[0]);
    close(fvwm_to_app[1]);
    free(args);
    return -1;
  }

  pipeName[i] = stripcpy(cptr);
  free(cptr);
  sprintf(arg2,"%d",app_to_fvwm[1]);
  sprintf(arg3,"%d",fvwm_to_app[0]);
  sprintf(arg5,"%lx",(unsigned long)win);
  sprintf(arg6,"%lx",(unsigned long)context);
  args[0] = arg1;
  args[1] = arg2;
  args[2] = arg3;
  args[3] = (char *)get_current_read_file();
  if (!args[3])
    args[3] = "none";
  args[4] = arg5;
  args[5] = arg6;
  for (nargs = 6; action = GetNextToken(action, &token), token; nargs++)
  {
    args = (char **)saferealloc((void *)args, (nargs + 2) * sizeof(char *));
    args[nargs] = token;
#ifndef WITHOUT_KILLMODULE_ALIAS_SUPPORT
    if (pipeAlias[i] == NULL)
    {
      const char *ptr = skipModuleAliasToken(args[nargs]);
      if (ptr && *ptr == '\0')
        pipeAlias[i] = stripcpy(args[nargs]);
    }
#endif
  }
  args[nargs] = NULL;

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
    NoGrabMask[i] = 0;
    free(arg1);
    pipeQueue[i] = NULL;
    if (DoingCommandLine) {
      /* add to the list of command line modules */
      DBUG("executeModule", "starting commandline module\n");
      FD_SET(i, &init_fdset);
    }

    /* make the PositiveWrite pipe non-blocking. Don't want to jam up
       fvwm because of an uncooperative module */
    fcntl(writePipes[i], F_SETFL, O_NONBLOCK);
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
    free(args);
    return -1;
  }
  free(args);

  return ret_pipe;
}

int executeModuleDesperate(F_CMD_ARGS)
{
  return do_execute_module(F_PASS_ARGS, True);
}

void CMD_Module(F_CMD_ARGS)
{
  do_execute_module(F_PASS_ARGS, False);
  return;
}

void CMD_ModuleSynchronous(F_CMD_ARGS)
{
  extern char *ModuleFinished;		/* defined in libs/Module.c */
  int sec = 0;
  char *next;
  char *token;
  char *expect = ModuleFinished;
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

  pipe_slot = do_execute_module(F_PASS_ARGS, False);
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
    struct timeval timeout;
    int num_fd;

    /* A signal here could interrupt the select call. We would
     * then need to restart our select, unless the signal was
     * a "terminate" signal. Note that we need to reinitialise
     * all of select's parameters after it has returned. */
    do
    {
      FD_ZERO(&in_fdset);
      FD_ZERO(&out_fdset);
      if (readPipes[pipe_slot] >= 0)
        FD_SET(readPipes[pipe_slot], &in_fdset);
      if (pipeQueue[pipe_slot] != NULL)
        FD_SET(writePipes[pipe_slot], &out_fdset);

      timeout.tv_sec = 0;
      timeout.tv_usec = 1;  /* use 1 usec timeout in select */
      num_fd = fvwmSelect(fd_width, &in_fdset, &out_fdset, 0, &timeout);
    }
    while ( (num_fd < 0) && !isTerminated );

    /* Exit if we have received a "terminate" signal */
    if ( isTerminated ) break;

    if (num_fd > 0)
    {
      if ((readPipes[pipe_slot] >= 0) &&
           FD_ISSET(readPipes[pipe_slot], &in_fdset))
      {
        /* Check for module input. */
        if (read(readPipes[pipe_slot], &targetWindow, sizeof(Window)) > 0)
        {
          if (HandleModuleInput(targetWindow, pipe_slot, expect, False))
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
        FlushMessageQueue(pipe_slot);
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
                            STROKE_ARG(0)
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

  if (need_ungrab)
    UngrabEm(GRAB_BUSY);
}

/* read input from a module and either execute it or queue it
 * returns True and does NOP if the module input matches the expect string */
Bool HandleModuleInput(Window w, int module, char *expect, Bool queue)
{
  char text[MAX_MODULE_INPUT_TEXT_LEN];
  unsigned long size;
  unsigned long cont;
  int n;
  int channel = readPipes[module];

  /* Already read a (possibly NULL) window id from the pipe,
   * Now read an fvwm bultin command line */
  n = read(channel, &size, sizeof(size));
  if (n < sizeof(size))
    {
      fvwm_msg(ERR, "HandleModuleInput",
               "Fail to read (Module: %i, read: %i, size: %i)",
               module, n, sizeof(size));
      KillModule(module);
      return False;
    }

  if (size > sizeof(text))
    {
      fvwm_msg(ERR, "HandleModuleInput",
               "Module(%i) command is too big (%ld), limit is %d",
               module, size, sizeof(text));
      /* The rest of the output from this module is going to be scrambled
       * so let's kill it rather than risk interpreting garbage */
      KillModule(module);
      return False;
    }

  pipeOn[module] = 1;

  n = read(channel, text, size);
  if (n < size)
    {
      fvwm_msg(ERR, "HandleModuleInput",
               "Fail to read command (Module: %i, read: %i, size: %ld)",
               module, n, size);
      KillModule(module);
      return False;
    }
  text[n] = '\0';
  n = read(channel, &cont, sizeof(cont));
  if (n < sizeof(cont))
    {
      fvwm_msg(ERR, "HandleModuleInput",
                    "Module %i, Size Problems (read: %d, size: %d)",
                    module, n, sizeof(cont));
      KillModule(module);
      return False;
    }
  if (cont == 0)
    {
      /* this is documented as a valid way for a module to quit
       * so let's not complain
      fvwm_msg(ERR, "HandleModuleInput",
               "Module %i, Size Problem: cont is zero", module);
       */
      KillModule(module);
    }
  if (strlen(text)>0)
    {
      if (expect && (strncasecmp(text, expect, strlen(expect)) == 0))
        /* the module sent the expected string */
        return True;

      if (queue)
        AddToCommandQueue(w, module, text);
      else
        ExecuteModuleCommand(w, module, text);
    }
  return False;
}

/* run the command as if it cames from a button press or release */
void ExecuteModuleCommand(Window w, int module, char *text)
{
  extern FvwmWindow *ButtonWindow;
  extern int Context;
  FvwmWindow *tmp_win;

  if (XFindContext (dpy, w, FvwmContext, (caddr_t *) &tmp_win) == XCNOENT)
  {
    tmp_win = NULL;
    w = None;
  }

  /* Query the pointer, the pager-drag-out feature doesn't work properly.*/
  /* This is OK now that the Pager uses "Move pointer" */
  /* A real fix would be for the modules to pass the button press coords */
  XQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild, &JunkX,&JunkY,
		&Event.xbutton.x_root,&Event.xbutton.y_root,&JunkMask);
  /* If a module does XUngrabPointer(), it can now get proper Popups */
  if(StrEquals(text, "popup"))
    Event.xbutton.type = ButtonPress;
  else
    Event.xbutton.type = ButtonRelease;
  Event.xbutton.window = w;
  Event.xbutton.button = 1;
  Event.xbutton.x = 0;
  Event.xbutton.y = 0;
  Event.xbutton.subwindow = None;
  Context = GetContext(tmp_win,&Event,&w);
  ButtonWindow = tmp_win;
  old_execute_function(text, tmp_win, &Event, Context, module, 0, NULL);
  ButtonWindow = NULL;
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
      DeleteMessageQueueBuff(channel);
    }
  if(pipeName[channel] != NULL)
    {
      free(pipeName[channel]);
      pipeName[channel] = NULL;
    }
#ifndef WITHOUT_KILLMODULE_ALIAS_SUPPORT
  if(pipeAlias[channel] != NULL)
    {
      free(pipeAlias[channel]);
      pipeAlias[channel] = NULL;
    }
#endif
  if (fFvwmInStartup) {
    /* remove from list of command line modules */
    DBUG("killModule", "ending command line module\n");
    FD_CLR(channel, &init_fdset);
  }

  return;
}

static void KillModuleByName(char *name
#ifndef WITHOUT_KILLMODULE_ALIAS_SUPPORT
  , char *alias
#endif
)
{
  int i = 0;

  if(name == NULL)
    return;

  while(i<npipes)
    {
      if((pipeName[i] != NULL)&&(matchWildcards(name,pipeName[i]))
#ifndef WITHOUT_KILLMODULE_ALIAS_SUPPORT
          && (!alias || (pipeAlias[i] && matchWildcards(alias, pipeAlias[i])))
#endif
        )
	{
	  KillModule(i);
	}
      i++;
    }
  return;
}

void CMD_KillModule(F_CMD_ARGS)
{
  char *module;
#ifndef WITHOUT_KILLMODULE_ALIAS_SUPPORT
  char *alias = NULL;
#endif

  action = GetNextToken(action,&module);
  if (!module)
    return;

#ifndef WITHOUT_KILLMODULE_ALIAS_SUPPORT
  GetNextToken(action, &alias);
  KillModuleByName(module, alias);
#else
  KillModuleByName(module);
#endif
  free(module);
}

static unsigned long *
make_vpacket(unsigned long *body, unsigned long event_type,
             unsigned long num, va_list ap)
{
  extern Time lastTimestamp;
  unsigned long *bp = body;

  /* truncate long packets */
  if (num > FvwmPacketMaxSize)
    num = FvwmPacketMaxSize;
  *(bp++) = START_FLAG;
  *(bp++) = event_type;
  *(bp++) = num+FvwmPacketHeaderSize;
  *(bp++) = lastTimestamp;

  for (; num > 0; --num)
    *(bp++) = va_arg(ap, unsigned long);

  return body;
}



/*************************************************************
    RBW - 04/16/1999 - new packet builder for GSFR --
    Arguments are pairs of lengths and argument data pointers.
    RBW - 05/01/2000 -
    A length of zero means that an int is being passed which
    must be stored in the packet as an unsigned long. This is
    a special hack to accommodate the old CONFIGARGSNEW
    technique of sending the args for the M_CONFIGURE_WINDOW
    packet.
**************************************************************/
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
  int *tmpint;
  Bool expandint;

  *(bp++) = START_FLAG;
  *(bp++) = event_type;
  /*  Skip length field, we don't know it yet. */
  bp++;
  *(bp++) = lastTimestamp;

  for (; num > 0; --num)  {
      arglen = va_arg(ap, unsigned long);
      if (arglen != 0)  {
        expandint = False;
        }  else  {
        expandint = True;
	arglen = sizeof(unsigned long);
        }
      bodylen += arglen;
      if (bodylen < FvwmPacketMaxSize_byte) {
        if (! expandint)  {
	  register char *tmp = (char *)bp;
          memcpy(tmp, va_arg(ap, char *), arglen);
          tmp += arglen;
	  bp = (unsigned long *)tmp;
          }  else  {
          tmpint = va_arg(ap, int *);
          *bp = (unsigned long) *tmpint;
          bp++;
          }
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


/* ************************************************************
    RBW - 04/16/1999 - new style packet senders for GSFR --
   ************************************************************ */
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
            (_t)->colors.fore,\
            (_t)->colors.back

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
            (_t)->colors.fore,\
            (_t)->colors.back

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



/****************************************************************
    RBW - 04/16/1999 - new version for GSFR --
        - args are now pairs:
          - length of arg data
          - pointer to arg data
        - number of arguments is the number of length/pointer pairs.
        - the 9th field, where flags used to be, is temporarily left
        as a dummy to preserve alignment of the other fields in the
        old packet: we should drop this before the next release.
*****************************************************************/
#define CONFIGARGSNEW(_t) 25,\
	    (unsigned long)(sizeof(unsigned long)),\
            &(*(_t))->w,\
	    (unsigned long)(sizeof(unsigned long)),\
            &(*(_t))->frame,\
	    (unsigned long)(sizeof(unsigned long)),\
            &(_t),\
	    (unsigned long)(0),\
            &(*(_t))->frame_g.x,\
	    (unsigned long)(0),\
            &(*(_t))->frame_g.y,\
	    (unsigned long)(0),\
            &(*(_t))->frame_g.width,\
	    (unsigned long)(0),\
            &(*(_t))->frame_g.height,\
	    (unsigned long)(0),\
            &(*(_t))->Desk,\
	    (unsigned long)(0),\
            &(*(_t))->layer,\
	    (unsigned long)(0),\
            &(*(_t))->title_g.height,\
	    (unsigned long)(0),\
            &(*(_t))->boundary_width,\
	    (unsigned long)(0),\
	    &(*(_t))->hints.base_width,\
	    (unsigned long)(0),\
	    &(*(_t))->hints.base_height,\
	    (unsigned long)(0),\
	    &(*(_t))->hints.width_inc,\
	    (unsigned long)(0),\
	    &(*(_t))->hints.height_inc,\
	    (unsigned long)(0),\
            &(*(_t))->hints.min_width,\
	    (unsigned long)(0),\
            &(*(_t))->hints.min_height,\
	    (unsigned long)(0),\
            &(*(_t))->hints.max_width,\
	    (unsigned long)(0),\
            &(*(_t))->hints.max_height,\
	    (unsigned long)(0),\
            &(*(_t))->icon_w,\
	    (unsigned long)(sizeof(unsigned long)),\
            &(*(_t))->icon_pixmap_w,\
	    (unsigned long)(0),\
            &(*(_t))->hints.win_gravity,\
	    (unsigned long)(sizeof(unsigned long)),\
            &(*(_t))->colors.fore,\
	    (unsigned long)(sizeof(unsigned long)),\
            &(*(_t))->colors.back,\
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
   * string. */
  *len =
    FvwmPacketHeaderSize + num + (strlen(name) / sizeof(unsigned long)) + 1;
  /* truncate long packets */
  if (*len > FvwmPacketMaxSize_byte)
    *len = FvwmPacketMaxSize_byte;

  body = (unsigned long *)safemalloc(*len * sizeof(unsigned long));
  body[*len-1] = 0; /* Zero out end of memory to avoid uninit memory access. */

  va_start(ap, num);
  make_vpacket(body, event_type, num, ap);
  va_end(ap);

  strcpy((char *)&body[FvwmPacketHeaderSize+num], name);
  body[2] = *len;

#if 0
  DB(("Packet (%lu): %lu %lu %lu `%s'", *len,
      body[FvwmPacketHeaderSize], body[FvwmPacketHeaderSize+1],
      body[FvwmPacketHeaderSize+2], name));
#endif

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


/**********************************************************************
 * Broadcasts a string to all modules as M_CONFIG_INFO.
 **********************************************************************/
void BroadcastConfigInfoString(char *string)
{
  int i;

  for (i = 0; i < npipes; i++)
  {
    /* just a quick check to save us lots of overhead */
    if (pipeOn[i] >= 0)
      SendName(i, M_CONFIG_INFO, 0, 0, 0, string);
  }
}


/**********************************************************************
 * Broadcasts the state of Xinerama support to all modules as M_CONFIG_INFO.
 **********************************************************************/
void broadcast_xinerama_state(void)
{
  BroadcastConfigInfoString((char *)FScreenGetConfiguration());
  return;
}


/*
** send an arbitrary string to all instances of a module
*/
void CMD_SendToModule(F_CMD_ARGS)
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
  str = safestrdup(action + strlen(module) + 1);

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
    if ((pipeName[i] != NULL && matchWildcards(module,pipeName[i]))
#ifndef WITHOUT_KILLMODULE_ALIAS_SUPPORT
      || (pipeAlias[i] && matchWildcards(module, pipeAlias[i]))
#endif
    )
    {
      SendName(i,M_STRING,data0,data1,data2,str);
      FlushMessageQueue(i);
    }
  }

  free(module);
  free(str);
}


/* This used to be marked "fvwm_inline".  I removed this
   when I added the lockonsend logic.  The routine seems too big to
   want to inline.  dje 9/4/98 */
extern int myxgrabcount;                /* defined in libs/Grab.c */
extern char *ModuleUnlock;		/* defined in libs/Module.c */
void PositiveWrite(int module, unsigned long *ptr, int size)
{
  extern int moduleTimeout;

  if(ptr == NULL)
  {
   return;
  }
  if((pipeOn[module]<0)||(!((PipeMask[module]) & ptr[1])))
  {
    return;
  }

  /* a dirty hack to prevent FvwmAnimate triggering during Recapture */
  /* would be better to send RecaptureStart and RecaptureEnd messages. */
  /* If module is lock on send for iconify message and it's an
   * iconify event and server grabbed, then return */
  if ((NoGrabMask[module] & SyncMask[module] & ptr[1]) && (myxgrabcount != 0))
  {
    return;
  }

  AddToMessageQueue(module,ptr,size,0);

  /* dje, from afterstep, for FvwmAnimate, allows modules to sync with fvwm.
   * this is disabled when the server is grabbed, otherwise deadlocks happen.
   * M_LOCKONSEND has been replaced by a separated mask which defines on
   * which messages the fvwm-to-module communication need to be lock
   * on send. olicha Nov 13, 1999 */
  /* migo (19-Aug-2000): removed !myxgrabcount to sync M_DESTROY_WINDOW */
/*if ((SyncMask[module] & ptr[1]) && !myxgrabcount) */

  if ((SyncMask[module] & ptr[1]))
  {
    Window targetWindow;
    fd_set readSet;
    struct timeval timeout;
    int channel = readPipes[module];

    FlushMessageQueue(module);

    do
    {
      int rc = 0;
      /*
       * We give the read a long timeout; if the module fails to
       * respond within this time then it deserves to be KILLED!
       *
       * NOTE: rather than impose an arbitrary timeout on the user,
       *       we will make this a configuration parameter.
       */

      do
      {
        timeout.tv_sec = moduleTimeout;
        timeout.tv_usec = 0;
        FD_ZERO(&readSet);
        FD_SET(channel, &readSet);

        /* Wait for input to arrive on just one descriptor, with a timeout */
        /* (fvwmSelect <= 0) or read() returning wrong size is bad news */
        rc = fvwmSelect(channel + 1, &readSet, NULL, NULL, &timeout);
        /* retry if select() failed with EINTR */
      } while ((rc < 0) && !isTerminated && (errno == EINTR));

      if ( isTerminated ) break;

      if (rc <= 0 || read(channel, &targetWindow, sizeof(targetWindow))
           != sizeof(targetWindow))
      {
        /* Doh! Something has gone wrong - get rid of the offender!! */
        fvwm_msg(ERR, "PositiveWrite",
                 "Failed to read descriptor from '%s':\n"
                 "- data available=%c\n"
                 "- terminate signal=%c\n",
#ifndef WITHOUT_KILLMODULE_ALIAS_SUPPORT
                 pipeAlias[module]
                   ? CatString3(pipeName[module], " ", pipeAlias[module])
                   : pipeName[module],
#else
                 pipeName[module],
#endif
                 (FD_ISSET(channel, &readSet) ? 'Y' : 'N'),
                 isTerminated ? 'Y' : 'N');
        KillModule(module);
        break;
      }

      /* Execute all messages from the module until UNLOCK is received
       * N.B. This may cause recursion if a command results in a sync message
       * to another module, which in turn may send a command that results in
       * another sync message to this module.

       * Hippo: I don't think this will cause deadlocks, but the third time we
       * get here the first times UNLOCK will be read and then on returning up
       * the third level UNLOCK will be read at the first level. This could be
       * difficult to fix without turning queueing on.  Turning queueing on
       * may be bad because it can be useful for modules to be able to inject
       * commands from modules in a synchronous manner. e.g. FvwmIconMan
       * can tell FvwmAnimate to do an animation when a window is de-iconified
       * from the IconMan, queueing make s this happen too late.
       */
    }
    while (!HandleModuleInput(targetWindow, module, ModuleUnlock, False));
  }
}


static void AddToMessageQueue(int module, unsigned long *ptr, int size, int done)
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

static void DeleteMessageQueueBuff(int module)
{
  struct queue_buff_struct *a;

  if(pipeQueue[module] == NULL)
     return;
  a = pipeQueue[module];
  pipeQueue[module] = a->next;
  if (a->data != NULL)
    free(a->data);
  free(a);
  return;
}

void FlushMessageQueue(int module)
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
      DeleteMessageQueueBuff(module);
    }
}

void FlushAllMessageQueues(void)
{
  int i;

  for(i = 0; i < npipes; i++) {
    if(writePipes[i] >= 0) {
      FlushMessageQueue(i);
    }
  }
}

/* A queue of commands from the modules */
typedef struct CommandQueue
{
  struct CommandQueue *next;
  Window window;
  int module;
  char *command;
} CommandQueue;

static CommandQueue *CQstart = NULL;
static CommandQueue *CQlast = NULL;

/***********************************************************************
 *
 *  Procedure:
 *	AddToCommandQueue - add a module command to the command queue
 *
 ************************************************************************/

static void AddToCommandQueue(Window window, int module, char *command)
{
  CommandQueue *new;

  if (!command)
    return;

  new = (CommandQueue *)safemalloc(sizeof(CommandQueue));

  new->window = window;
  new->module = module;
  new->command = safestrdup(command);
  new->next = NULL;

  DBUG("AddToCommandQueue", command);

  if (CQlast)
    CQlast->next = new;
  CQlast = new;
  if (!CQstart)
    CQstart = CQlast;
}

/***********************************************************************
 *
 *  Procedure:
 *	EmptyCommandQueue - runs command from the module command queue
 *	This may be called recursively if a module command runs a function
 *	that does a Wait, so it must be re-entrant
 *
 ************************************************************************/
void ExecuteCommandQueue(void)
{
  CommandQueue *temp;

  while (CQstart)
  {
    /* remember the first command */
    temp = CQstart;
    /* remove it from the queue */
    CQstart = CQstart->next;
    /* fix the end pointer if the queue is now empty */
    if (CQstart == NULL)
      CQlast = NULL;
    /* run the first command (this may add stuff to the queue) */
    if (temp->command)
    {
      DBUG("EmptyCommandQueue", temp->command);
      ExecuteModuleCommand(temp->window, temp->module, temp->command);
      free(temp->command);
    }
    free(temp);
  }
}

void CMD_Send_WindowList(F_CMD_ARGS)
{
  FvwmWindow *t;

  if(*Module >= 0)
    {
      SendPacket(*Module, M_NEW_DESK, 1, Scr.CurrentDesk);
      SendPacket(*Module, M_NEW_PAGE, 5,
                 Scr.Vx, Scr.Vy, Scr.CurrentDesk, Scr.VxMax, Scr.VyMax);

      if(Scr.Hilite != NULL)
      {
	SendPacket(*Module, M_FOCUS_CHANGE, 5,
                   Scr.Hilite->w,
                   Scr.Hilite->frame,
		   (unsigned long)True,
		   Scr.Hilite->hicolors.fore,
		   Scr.Hilite->hicolors.back);
      }
      else
      {
	SendPacket(*Module, M_FOCUS_CHANGE, 5,
                   0, 0, (unsigned long)True,
                   GetColor(DEFAULT_FORE_COLOR),
                   GetColor(DEFAULT_BACK_COLOR));
      }
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
		       t->icon_g.x, t->icon_g.y,
		       t->icon_g.width, t->icon_g.height+t->icon_p_height);

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
      {
	  BroadcastPacket(M_FOCUS_CHANGE, 5,
                          0, 0, (unsigned long)True,
			  GetColor(DEFAULT_FORE_COLOR),
			  GetColor(DEFAULT_BACK_COLOR));
      }
      else
      {
	  BroadcastPacket(M_FOCUS_CHANGE, 5,
                          Scr.Hilite->w,
                          Scr.Hilite->frame,
                          (unsigned long)True,
                          Scr.Hilite->hicolors.fore,
                          Scr.Hilite->hicolors.back);
      }

      SendPacket(*Module, M_END_WINDOWLIST, 0);
    }
}

void CMD_set_mask(F_CMD_ARGS)
{
  int val = 0;

  GetIntegerArguments(action, NULL, &val, 1);
  PipeMask[*Module] = (unsigned long)val;
}

void CMD_set_sync_mask(F_CMD_ARGS)
{
  int val = 0;

  GetIntegerArguments(action, NULL, &val, 1);
  SyncMask[*Module] = (unsigned long)val;
}

void CMD_set_nograb_mask(F_CMD_ARGS)
{
  int val = 0;

  GetIntegerArguments(action, NULL, &val, 1);
  NoGrabMask[*Module] = (unsigned long)val;
}

/*
 * returns a pointer inside a string (just after the alias) if ok or NULL
 */
char *skipModuleAliasToken(const char *string)
{
#define is_valid_first_alias_char(ch) (isalpha(ch) || (ch) == '/')
#define is_valid_alias_char(ch) (is_valid_first_alias_char(ch) \
    || isalnum(ch) || (ch) == '-' || (ch) == '.' || (ch) == '/')

  if (is_valid_first_alias_char(*string))
  {
    int len = 1;
    string++;
    while (*string && is_valid_alias_char(*string))
    {
      if (++len > 250) return NULL;
      string++;
    }
    return (char *)string;
  }
  return NULL;
}
