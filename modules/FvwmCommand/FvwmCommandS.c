/* $Id$
 * $Source$
 *
 * Fvwm command input interface.
 *
 * Copyright 1997, Toshi Isogai. No guarantees or warantees or anything
 * are provided. Use this program at your own risk. Permission to use
 * this program for any purpose is given,
 * as long as the copyright is kept intact.
 *
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "FvwmCommand.h"
#include "libs/fvwmlib.h"
#include "libs/fvwmsignal.h"

#define MYNAME   "FvwmCommandS"
#define MAXHOSTNAME 255

int Fd[2]; /* pipes to fvwm */
int FfdC; /* command fifo file discriptors */
int FfdM; /* message fifo file discriptors */

char *FfdC_name, *FfdM_name; /* fifo names */

ino_t FfdC_ino, FfdM_ino; /* fifo inode numbers */

char client[MAXHOSTNAME];
char hostname[32];

int open_fifos(const char *f_stem);
void close_fifos(void);
void close_pipes(void);
void err_msg(const char *msg);
void err_quit(const char *msg) __attribute__((noreturn));
void process_message(unsigned long type,unsigned long *body);
void relay_packet(unsigned long, unsigned long, unsigned long *);
void server(char *);
static RETSIGTYPE sig_handler(int);
int write_f(int fd, char *p, int len);

int main(int argc, char *argv[])
{
  char *fifoname;

  if (argc < FARGS)
  {
    fprintf(stderr,"%s Version %s should only be executed by fvwm!\n",
	    MYNAME, MYVERSION);
    exit(1);
  }

  if (argc == FARGS + 1)
  {
    fifoname = argv[FARGS];
  }
  else
  {
    fifoname = NULL;
  }

#ifdef HAVE_SIGACTION
  {
    struct sigaction sigact;

    sigemptyset(&sigact.sa_mask);
    sigaddset(&sigact.sa_mask, SIGINT);
    sigaddset(&sigact.sa_mask, SIGHUP);
    sigaddset(&sigact.sa_mask, SIGQUIT);
    sigaddset(&sigact.sa_mask, SIGTERM);
    sigaddset(&sigact.sa_mask, SIGPIPE);

#ifdef SA_INTERRUPT
    sigact.sa_flags = SA_INTERRUPT;
#else
    sigact.sa_flags = 0;
#endif
    sigact.sa_handler = sig_handler;
    sigaction(SIGINT, &sigact, NULL);
    sigaction(SIGHUP, &sigact, NULL);
    sigaction(SIGQUIT, &sigact, NULL);
    sigaction(SIGTERM, &sigact, NULL);
    sigaction(SIGPIPE, &sigact, NULL);
  }
#else
#ifdef USE_BSD_SIGNALS
  fvwmSetSignalMask(sigmask(SIGINT) | sigmask(SIGHUP) | sigmask(SIGQUIT)
                    | sigmask(SIGTERM) | sigmask(SIGPIPE));
#endif
  signal(SIGPIPE, sig_handler);
  signal(SIGINT, sig_handler);
  signal(SIGQUIT, sig_handler);
  signal(SIGHUP, sig_handler);
  signal(SIGTERM, sig_handler);
#ifdef HAVE_SIGINTERRUPT
  siginterrupt(SIGINT, 1);
  siginterrupt(SIGHUP, 1);
  siginterrupt(SIGQUIT, 1);
  siginterrupt(SIGTERM, 1);
  siginterrupt(SIGPIPE, 1);
#endif
#endif

  Fd[0] = atoi(argv[1]);
  Fd[1] = atoi(argv[2]);

  /* tell fvwm we're running */
  SendFinishedStartupNotification(Fd);

  server(fifoname);
  return 1;
}

/*
 * Signal handler
 */
static RETSIGTYPE
sig_handler(int signo)
{
  close_pipes();
  fvwmSetTerminate(signo);
}

/*
 * setup server and communicate with fvwm and the client
 */
void server (char *name)
{
  char *home;
  char *f_stem;
  int  len;
  fd_set fdset;
  char buf[MAX_MODULE_INPUT_TEXT_LEN + 1];  /* command receiving buffer */
  char cmd[MAX_MODULE_INPUT_TEXT_LEN + 1];
  int  ix,cix;

  if (name == NULL)
  {
    char *dpy_name;

    /* default name */
    home = getenv("HOME");
    if  (home == NULL)
      home = "";
    f_stem = safemalloc(strlen(home) + strlen(F_NAME) + MAXHOSTNAME + 4);
    strcpy(f_stem, home);
    if (f_stem[strlen(f_stem)-1] != '/')
    {
      strcat(f_stem, "/");
    }
    strcat(f_stem, F_NAME);

    /* Make it unique */
    strcat(f_stem, "-");
    gethostname(hostname,32);
    dpy_name = getenv("DISPLAY");
    if (!dpy_name)
      dpy_name = ":0";
    if (strncmp(dpy_name, "unix:", 5) == 0)
      dpy_name += 4;
    if (!dpy_name[0] || ':' == dpy_name[0])
      strcat(f_stem, hostname );  /* Put hostname before dpy if not there */
    strcat(f_stem, client);
  }
  else
  {
    f_stem = name;
  }

  if (open_fifos(f_stem) < 0)
  {
    exit (-1);
  }
  SendText(Fd, "NOP", 0); /* tell fvwm that we are here */

  cix = 0;

  while (!isTerminated)
  {
    FD_ZERO(&fdset);
    FD_SET(FfdC, &fdset);
    FD_SET(Fd[1], &fdset);

    if (fvwmSelect(FD_SETSIZE, &fdset, 0, 0, NULL) < 0)
      if (errno == EINTR)
        continue;

    if (FD_ISSET(Fd[1], &fdset))
    {
      FvwmPacket* packet = ReadFvwmPacket(Fd[1]);

      if (packet == NULL)
      {
        close_pipes();
        exit(0);
      }
      process_message(packet->type, packet->body);
    }

    if (FD_ISSET(FfdC, &fdset))
    {
      len = read(FfdC, buf, MAX_MODULE_INPUT_TEXT_LEN);
      if (len == 0)
        continue;
      else if (len < 0)
      {
        if (errno != EAGAIN && errno != EINTR)
          err_quit("reading fifo");
        continue;
      }

      /* in case of multiple long lines */
      for (ix = 0; ix < len; ix++)
      {
        cmd[cix] = buf[ix];
        if (cmd[cix] == '\n')
	{
          cmd[cix+1] = '\0';
          cix = 0;
	  SendText(Fd,cmd,0);
        }
	else if (cix >= MAX_MODULE_INPUT_TEXT_LEN)
	{
	  err_msg("command too long");
	  cix = 0;
	}
	else
	{
	  cix++;
	}
      } /* for */
    } /* FD_ISSET */
  } /* while */
}

/*
 * close  fifos and pipes
 */
void close_pipes(void)
{
  static char is_closed = 0;

  /* prevent that this is executed twice */
  if (!is_closed)
  {
    is_closed = 1;
    close(Fd[0]);
    close(Fd[1]);
    close_fifos();
  }
}

void close_fifos(void)
{
  struct stat stat_buf;

  /* only unlink this file if it is the same as FfdC */
  /* I know there's a race here between the stat and the unlink, if you know
   * of a way to unlink from a fildes use it here */
  if ((stat(FfdC_name, &stat_buf) == 0) && (stat_buf.st_ino == FfdC_ino))
    unlink(FfdC_name);
  /* only unlink this file if it is the same as FfdM */
  if ((stat(FfdM_name, &stat_buf) == 0) && (stat_buf.st_ino == FfdM_ino))
    unlink(FfdM_name);
  /* construct the name of the lock file and remove it */
  FfdM_name[strlen(FfdM_name)-1] = 'R';
  unlink(FfdM_name);
  close(FfdM);
  close(FfdC);
  FfdC = -1;
  FfdM = -1;
}

/*
 * open fifos
 */
int open_fifos(const char *f_stem)
{
  struct stat stat_buf;

  /* create 2 fifos */
  FfdC_name = malloc(strlen(f_stem) + 2);
  if (FfdC_name == NULL)
  {
    err_msg( "allocating command" );
    return -1;
  }
  FfdM_name = malloc(strlen(f_stem) + 2);
  if (FfdM_name == NULL)
  {
    err_msg("allocating message");
    return -1;
  }
  strcpy(FfdC_name, f_stem);
  strcpy(FfdM_name, f_stem);
  strcat(FfdC_name, "C");
  strcat(FfdM_name, "M");

  /* check to see if another FvwmCommandS is running by trying to talk to it */
  FfdC = open(FfdC_name, O_RDWR | O_NONBLOCK);
  /* remove the fifo's if they exist */
  unlink(FfdM_name);
  unlink(FfdC_name);
  /* If a previous FvwmCommandS is lingering tell it to go away
   * It will check that it owns the fifo's before removing them so no
   * there is no need to wait before creating our own */
  if (FfdC > 0)
  {
    write_f(FfdC, "killme\n", 9);
    close(FfdC);
  }

  /* now we can create the fifo's knowing that they don't already exist
   * and any lingering FvwmCommandS will not share them or remove them */
  if (mkfifo(FfdM_name, S_IRUSR | S_IWUSR) < 0)
  {
    err_msg(FfdM_name);
    return -1;
  }
  if (mkfifo(FfdC_name, S_IRUSR | S_IWUSR) < 0)
  {
    err_msg(FfdC_name);
    return -1;
  }

  if ((FfdM = open(FfdM_name, O_RDWR | O_NONBLOCK)) < 0)
  {
    err_msg("opening message fifo");
    return -1;
  }
  if ((FfdC = open(FfdC_name, O_RDWR | O_NONBLOCK)) < 0)
  {
    err_msg("opening command fifo");
    return -1;
  }

  /* get the inode numbers for use when quiting */
  if (fstat(FfdC, &stat_buf) != 0)
  {
    err_msg("stat()ing command fifo");
    return -1;
  }
  else
    FfdC_ino = stat_buf.st_ino;
  if (fstat(FfdM, &stat_buf) != 0)
  {
    err_msg("stat()ing message fifo");
    return -1;
  }
  else
    FfdM_ino = stat_buf.st_ino;

  return 0;
}


/*
 * Process window list messages
 */

void process_message(unsigned long type,unsigned long *body)
{
  int msglen;

  switch (type)
  {
  case M_ADD_WINDOW:
  case M_CONFIGURE_WINDOW:
    relay_packet(type, sizeof(struct ConfigWinPacket) * SOL, body);
    break;

  case M_ICON_LOCATION:
  case M_ICONIFY:
    relay_packet(type, 7 * SOL, body);
    break;

  case M_MINI_ICON:
    relay_packet(type, 6 * SOL, body);
    break;

  case M_NEW_PAGE:
    relay_packet(type, 5 * SOL, body);
    break;

  case M_NEW_DESK:
    relay_packet(type, 1 * SOL, body);
    break;

  case M_END_WINDOWLIST:
  case M_END_CONFIG_INFO:
    relay_packet(type, 0 * SOL, body);
    break;

  case M_WINDOW_NAME:
  case M_ICON_NAME:
  case M_RES_CLASS:
  case M_RES_NAME:
  case M_ICON_FILE:
  case M_DEFAULTICON:
  case M_ERROR:
  case M_STRING:
  case M_CONFIG_INFO:
    msglen = strlen((char *)&body[3]);
    relay_packet(type, msglen + 1 + 3 * SOL, body);
    break;

  default:
    relay_packet(type, 4 * SOL, body);
  }
}



/*
 * print error message on stderr and exit
 */

void err_msg(const char *msg)
{
  fprintf(stderr, "%s server error in %s, %s\n", MYNAME, msg, strerror(errno));
}

void err_quit(const char *msg)
{
  err_msg(msg);
  close_pipes();
  exit(1);
}

/*
 * relay packet to front-end
 */
void relay_packet(unsigned long type, unsigned long length,
                  unsigned long *body)
{
  write_f(FfdM, (char*)&type, SOL);
  write_f(FfdM, (char*)&length, SOL);
  write_f(FfdM, (char*)body, length);
}

/*
 * write to fifo
 */
int write_f(int fd, char *p, int len)
{
  int i, n;
  struct timeval tv;
  int again;

  again = 0;
  tv.tv_sec = 0;
  tv.tv_usec = 50000;

  for (i = 0; i < len; )
  {
    n = write(fd, &p[i], len - i);
    if (n < 0)
    {
      if(errno == EINTR)
      {
	continue;
      }
      else if (errno == EAGAIN)
      {
	if (again++ < 5)
	{
	  select(0, 0, 0, 0, &tv);
	  continue;
	}
	return -1; /* give up this message */
      }
      err_quit("writing fifo");
    }
    i += n;
  }
  return 0;
}
