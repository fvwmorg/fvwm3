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
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <errno.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "fvwm.h"
#include "menus.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"
#include "module.h"


int npipes;
int *readPipes;
int *writePipes;
int *pipeOn;
char **pipeName;

unsigned long *PipeMask;
struct queue_buff_struct **pipeQueue;

inline int PositiveWrite(int module, unsigned long *ptr, int size);
void DeleteQueueBuff(int module);
void AddToQueue(int module, unsigned long *ptr, int size, int done);

void initModules(void)
{
  int i;

  npipes = GetFdWidth();

  writePipes = (int *)safemalloc(sizeof(int)*npipes);
  readPipes = (int *)safemalloc(sizeof(int)*npipes);
  pipeOn = (int *)safemalloc(sizeof(int)*npipes);
  PipeMask = (unsigned long *)safemalloc(sizeof(unsigned long)*npipes);
  pipeName = (char **)safemalloc(sizeof(char *)*npipes);
  pipeQueue=(struct queue_buff_struct **)
    safemalloc(sizeof(struct queue_buff_struct *)*npipes);

  for(i=0;i<npipes;i++)
    {
      writePipes[i]= -1;
      readPipes[i]= -1;
      pipeOn[i] = -1;
      PipeMask[i] = MAX_MASK;
      pipeQueue[i] = (struct queue_buff_struct *)NULL;
      pipeName[i] = NULL;
    }
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


void executeModule(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		     unsigned long context, char *action,int* Module)
{
  int fvwm_to_app[2],app_to_fvwm[2];
  int i,val,nargs = 0;
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

  if(eventp->type != KeyPress)
    UngrabEm();

  if(action == NULL)
    return;

  if(tmp_win)
    win = tmp_win->w;
  else
    win = None;

  /* If we execute a module, don't wait for buttons to come up,
   * that way, a pop-up menu could be implemented */
  *Module = 0;

  action = GetNextToken(action, &cptr);
  if (!cptr)
    return;

  arg1 = findIconFile(cptr,ModulePath,X_OK);
  if(arg1 == NULL)
    {
      fvwm_msg(ERR,"executeModule",
	       "No such module '%s' in ModulePath '%s'",cptr,ModulePath);
      free(cptr);
      return;
    }

  /* Look for an available pipe slot */
  i=0;
  while((i<npipes) && (writePipes[i] >=0))
    i++;
  if(i>=npipes)
    {
      fvwm_msg(ERR,"executeModule","Too many Accessories!");
      free(arg1);
      free(cptr);
      return;
    }

  /* I want one-ended pipes, so I open two two-ended pipes,
   * and close one end of each. I need one ended pipes so that
   * I can detect when the module crashes/malfunctions */
  if(pipe(fvwm_to_app)!=0)
    {
      fvwm_msg(ERR,"executeModule","Failed to open pipe");
      free(arg1);
      free(cptr);
      return;
    }
  if(pipe(app_to_fvwm)!=0)
    {
      fvwm_msg(ERR,"executeModule","Failed to open pipe2");
      free(arg1);
      free(cptr);
      close(fvwm_to_app[0]);
      close(fvwm_to_app[1]);
      return;
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
      free(arg1);
      pipeQueue[i] = NULL;

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
      close(fvwm_to_app[1]);
      close(app_to_fvwm[0]);

      execvp(arg1,args);
      fvwm_msg(ERR,"executeModule","Execution of module failed: %s",arg1);
      perror("");
      close(app_to_fvwm[1]);
      close(fvwm_to_app[0]);
      exit(1);
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
    }
  return;
}

/* Changed to return 66, Locking code AS dje */
int HandleModuleInput(Window w, int channel)
{
  char text[256];
  int size;
  int cont,n;

  /* Already read a (possibly NULL) window id from the pipe,
   * Now read an fvwm bultin command line */
  n = read(readPipes[channel], &size, sizeof(size));
  if(n < sizeof(size))
    {
      KillModule(channel,1);
      return 0;
    }

  if(size >255)
    {
      fvwm_msg(ERR, "HandleModuleInput",
               "Module command is too big (%d)", size);
      size=255;
    }

  pipeOn[channel] = 1;

  n = read(readPipes[channel],text, size);
  if(n < size)
    {
      KillModule(channel,2);
      return 0;
    }
  text[n] = '\0';
  /* DB(("Module read[%d] (%d): `%s'", n, size, text)); */

  n = read(readPipes[channel],&cont, sizeof(cont));
  /* DB(("Module read[%d] cont = %d", n, cont)); */
  if(n < sizeof(cont))
    {
      KillModule(channel,3);
      return 0;
    }
  if(cont == 0)
    {
      KillModule(channel,4);
    }
  if(strlen(text)>0)
    {
      extern int Context;
      FvwmWindow *tmp_win;

      if(strncasecmp(text,"UNLOCK",6)==0) { /* synchronous response */
        return 66;
      }

      /* perhaps the module would like us to kill it? */
      if(strncasecmp(text,"KillMe",6)==0)
      {
        KillModule(channel,12);
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
	  Event.xbutton.button = 1;
	  Event.xbutton.x_root = tmp_win->frame_x;
	  Event.xbutton.y_root = tmp_win->frame_y;
	  Event.xbutton.x = 0;
	  Event.xbutton.y = 0;
	  Event.xbutton.subwindow = None;
	}
      else
	{
	  Event.xbutton.button = 1;
	  Event.xbutton.x_root = 0;
	  Event.xbutton.y_root = 0;
	  Event.xbutton.x = 0;
	  Event.xbutton.y = 0;
	  Event.xbutton.subwindow = None;
	}
      Context = GetContext(tmp_win,&Event,&w);
      ExecuteFunction(text,tmp_win,&Event,Context ,channel);
    }
  return 0;
}


RETSIGTYPE DeadPipe(int nonsense)
{
}


void KillModule(int channel, int place)
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

  return;
}

void KillModuleByName(char *name)
{
  int i = 0;

  if(name == NULL)
    return;

  while(i<npipes)
    {
      if((pipeName[i] != NULL)&&(matchWildcards(name,pipeName[i])))
	{
	  KillModule(i,10);
	}
      i++;
    }
  return;
}


static unsigned long *
make_vpacket(unsigned long *body, unsigned long event_type,
             unsigned long num, va_list ap)
{
  extern Time lastTimestamp;
  unsigned long *bp = body;

  *(bp++) = START_FLAG;
  *(bp++) = event_type;
  *(bp++) = num+HEADER_SIZE;
  *(bp++) = lastTimestamp;

  for (; num > 0; --num)
    *(bp++) = va_arg(ap, unsigned long);

  return body;
}

void
SendPacket(int module, unsigned long event_type, unsigned long num_datum, ...)
{
  unsigned long body[MAX_BODY_SIZE+HEADER_SIZE];
  va_list ap;

  va_start(ap, num_datum);
  make_vpacket(body, event_type, num_datum, ap);
  va_end(ap);

  PositiveWrite(module, body, (num_datum+HEADER_SIZE)*sizeof(body[0]));
}

void
BroadcastPacket(unsigned long event_type, unsigned long num_datum, ...)
{
  unsigned long body[MAX_BODY_SIZE+HEADER_SIZE];
  va_list ap;
  int i;

  va_start(ap,num_datum);
  make_vpacket(body, event_type, num_datum, ap);
  va_end(ap);

  for (i=0; i<npipes; i++)
    PositiveWrite(i, body, (num_datum+HEADER_SIZE)*sizeof(body[0]));
}

#define CONFIGARGS(_t) 24,\
            (_t)->w,\
            (_t)->frame,\
            (unsigned long)(_t),\
            (_t)->frame_x,\
            (_t)->frame_y,\
            (_t)->frame_width,\
            (_t)->frame_height,\
            (_t)->Desk,\
            (_t)->flags,\
            (_t)->title_height,\
            (_t)->boundary_width,\
            ((_t)->hints.flags & PBaseSize) ? (_t)->hints.base_width : 0,\
            ((_t)->hints.flags & PBaseSize) ? (_t)->hints.base_height: 0,\
            ((_t)->hints.flags & PResizeInc)? (_t)->hints.width_inc  : 1,\
            ((_t)->hints.flags & PResizeInc)? (_t)->hints.height_inc : 1,\
            (_t)->hints.min_width,\
            (_t)->hints.min_height,\
            (_t)->hints.max_width,\
            (_t)->hints.max_height,\
            (_t)->icon_w,\
            (_t)->icon_pixmap_w,\
            (_t)->hints.win_gravity,\
            (_t)->TextPixel,\
            (_t)->BackPixel

void SendConfig(int module, unsigned long event_type, const FvwmWindow *t)
{
  SendPacket(module, event_type, CONFIGARGS(t));
}


void BroadcastConfig(unsigned long event_type, const FvwmWindow *t)
{
  BroadcastPacket(event_type, CONFIGARGS(t));
}

static unsigned long *
make_named_packet(int *len, unsigned long event_type, const char *name,
                  int num, ...)
{
  unsigned long *body;
  va_list ap;

  /* Packet is the header plus the items plus enough items to hold the name
     string.  */
  *len = HEADER_SIZE + num + (strlen(name) / sizeof(unsigned long)) + 1;

  body = (unsigned long *)safemalloc(*len * sizeof(unsigned long));
  body[*len-1] = 0; /* Zero out end of memory to avoid uninit memory access. */

  va_start(ap, num);
  make_vpacket(body, event_type, num, ap);
  va_end(ap);

  strcpy((char *)&body[HEADER_SIZE+num], name);
  body[2] = *len;

  /* DB(("Packet (%lu): %lu %lu %lu `%s'", *len,
       body[HEADER_SIZE], body[HEADER_SIZE+1], body[HEADER_SIZE+2], name)); */

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
void
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

/*
** send an arbitrary string to all instances of a module
*/
void SendStrToModule(XEvent *eventp,Window junk,FvwmWindow *tmp_win,
                     unsigned long context, char *action,int* Module)
{
  char *module,*str;
  int i;

  if (!action)
    return;
  GetNextToken(action,&module);
  if (!module)
    return;
  str = strdup(action + strlen(module) + 1);

  for (i=0;i<npipes;i++)
  {
    if((pipeName[i] != NULL)&&(matchWildcards(module,pipeName[i])))
    {
      SendName(i,M_STRING,0,0,0,str);
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

  /*
   * fvwm2  recapture has the x  server grabbed while  iconify events are
   * simulated.  Right now (01/24/99), the only module using lock on send
   * is FvwmAnimate which always  does  a grab of  its  own.  To avoid  a
   * deadlock, iconify events are not sent to a lock on send module while
   * the server is grabbed.   In the future, it might  make sense to send
   * the fact that the server is  grabbed to the  lock on send module, or
   * in some other way get finer control.
   */
  if ((PipeMask[module] & M_LOCKONSEND) /* module uses lock on send */
      && (myxgrabcount != 0)            /* and server grabbed */
      && (ptr[1] & M_ICONIFY)) {        /* and its an iconify event */
    return -1;                          /* don't send it */
  }
  AddToQueue(module,ptr,size,0);
  /* dje, from afterstep, for FvwmAnimate,
     allows the module to synchronize with fvwm.
     */
  if (PipeMask[module] & M_LOCKONSEND) {
    Window targetWindow;
    int e;

    FlushQueue(module);
    fcntl(readPipes[module],F_SETFL,0);
    while ((e = read(readPipes[module],&targetWindow, sizeof(Window))) > 0) {
      if (HandleModuleInput(targetWindow,module) == 66) {
        break;
      }
    }
    if (e <= 0) {
      KillModule(module,10);
    }
    fcntl(readPipes[module],F_SETFL,O_NDELAY);
  }
  return size;
}


void AddToQueue(int module, unsigned long *ptr, int size, int done)
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

void DeleteQueueBuff(int module)
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
	      KillModule(module,123);
	      return;
	    }
	}
      DeleteQueueBuff(module);
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
		   (unsigned long)Scr.Hilite,
		   Scr.DefaultDecor.HiColors.fore,
		   Scr.DefaultDecor.HiColors.back);
      else
	SendPacket(*Module, M_FOCUS_CHANGE, 5,
                   0, 0, 0,
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

	  if((t->flags & ICONIFIED)&&(!(t->flags & ICON_UNMAPPED)))
	    SendPacket(*Module, M_ICONIFY, 7, t->w, t->frame,
		       (unsigned long)t,
		       t->icon_x_loc, t->icon_y_loc,
		       t->icon_w_width, t->icon_w_height+t->icon_p_height);

	  if((t->flags & ICONIFIED) && (t->flags & ICON_UNMAPPED))
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
                          0, 0, 0,
                          Scr.DefaultDecor.HiColors.fore,
                          Scr.DefaultDecor.HiColors.back);
      else
	  BroadcastPacket(M_FOCUS_CHANGE, 5,
                          Scr.Hilite->w,
                          Scr.Hilite->frame,
                          (unsigned long)Scr.Hilite,
                          Scr.DefaultDecor.HiColors.fore,
                          Scr.DefaultDecor.HiColors.back);

      SendPacket(*Module, M_END_WINDOWLIST, 0);
    }
}
void set_mask_function(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		       unsigned long context, char *action,int* Module)
{
  int val = 0;

  GetIntegerArguments(action, NULL, &val, 1);
  PipeMask[*Module] = (unsigned long)val;
}
