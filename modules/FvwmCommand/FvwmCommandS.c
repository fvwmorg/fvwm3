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
#include "../../libs/fvwmlib.h"
#include "../../libs/fvwmsignal.h"

#define MYNAME   "FvwmCommandS"
#define MAXHOSTNAME 255

int  Fd[2];  /* pipe to fvwm */
int  Ffdr;   /* command fifo file discriptors */
int  Ffdw;   /* message fifo file discriptors */
char *F_name, *Fc_name, *Fm_name; /* fifo name */
char Nounlink;   /* don't delete fifo when true */
char Connect;    /* client is connected */

char client[MAXHOSTNAME];
char hostname[32];

int  open_fifos(const char *f_stem);
void close_fifos(void);
void close_pipes(void);
void DeadPipe( int ) __attribute__((noreturn));
void err_msg(const char *msg);
void err_quit(const char *msg) __attribute__((noreturn));
void process_message(unsigned long type,unsigned long *body);
void relay_packet( unsigned long, unsigned long, unsigned long *);
void server( char * );
static RETSIGTYPE sig_handler( int );
int  write_f(int fd, char *p, int len);

int main(int argc, char *argv[])
{
  char *fifoname;

  if(argc < FARGS)    {
    fprintf(stderr,"%s Version %s should only be executed by fvwm!\n",
	    MYNAME, MYVERSION);
    exit(1);
  }

  if( argc == FARGS+1 ) {
    fifoname = argv[FARGS];
  }else{
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
  fvwmSetSignalMask( sigmask(SIGINT) | sigmask(SIGHUP) |
                     sigmask(SIGQUIT) | sigmask(SIGTERM) |
                     sigmask(SIGPIPE) );
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

  Nounlink = 0;

  /* tell fvwm we're running */
  SendFinishedStartupNotification(Fd);

  server( fifoname );
  return 1;
}

/*
 * NOT a signal handler (can't call fprintf in a signal handler)
 */
void DeadPipe( int dummy ) {
  (void)dummy;

  fprintf(stderr,"%s: dead pipe\n", MYNAME);
  close_pipes();
  exit(0);
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
void server ( char *name ) {
  char *home;
  char *f_stem;
  int  len;
  fd_set fdset;
  char buf[MAX_COMMAND_SIZE];  /* command receiving buffer */
  char cmd[MAX_COMMAND_SIZE];
  int  ix,cix;

  if( name == NULL ) {
    char *dpy_name;

    /* default name */
    home = getenv("HOME");
    if (!home)  home = "";
    f_stem = safemalloc( strlen(home) + strlen(F_NAME) + MAXHOSTNAME + 4);
    strcpy (f_stem, home);
    if (f_stem[strlen(f_stem)-1] != '/') {
      strcat (f_stem, "/");
    }
    strcat (f_stem, F_NAME);

    /* Make it unique */
    strcat (f_stem, "-");
    gethostname(hostname,32);
    dpy_name = getenv("DISPLAY");
    if (!dpy_name)
      dpy_name = ":0";
    if (strncmp(dpy_name, "unix:", 5) == 0)
      dpy_name += 4;
    if (!dpy_name[0]  ||  ':' == dpy_name[0])
      strcat( f_stem, hostname );  /* Put hostname before dpy if not there */
    strcat (f_stem, client);
  }else{
    f_stem = name;
  }

  if (open_fifos(f_stem) < 0) {
    exit (-1);
  }
  SendText(Fd," ",0); /* tell fvwm that we are here */

  cix = 0;

  while ( !isTerminated ){
    FD_ZERO(&fdset);
    FD_SET(Ffdr, &fdset);
    FD_SET(Fd[1], &fdset);

    if (fvwmSelect(FD_SETSIZE, &fdset, 0, 0, NULL) < 0) {
      if (errno == EINTR) {
        continue;
      }
    }

    if (FD_ISSET(Fd[1], &fdset)){
      FvwmPacket* packet = ReadFvwmPacket(Fd[1]);
      if ( packet == NULL ) {
        close_pipes();
        exit( 0 );
      }
	  process_message( packet->type, packet->body );
    }

    if (FD_ISSET(Ffdr, &fdset)){
      len = read( Ffdr, buf, MAX_COMMAND_SIZE-1 );
      if (len == 0) {
        continue;
      }
      if (len < 0) {
        if (errno != EAGAIN && errno != EINTR) {
          err_quit("reading fifo");
        }
      }

      Connect = 1;
      /* in case of multiple long lines */
      for (ix=0; ix<len; ix++) {
        cmd[cix] = buf[ix];
        if (cmd[cix] == '\n') {
          cmd[cix+1] = '\0';
          cix = 0;
          if (!strncmp (cmd, CMD_CONNECT, strlen(CMD_CONNECT))) {
            /* do nothing */
          } else if (!strcmp (cmd, CMD_EXIT)) {
            Connect = 0;
            break;
          } else {
            if (!strcmp (cmd, CMD_KILL_NOUNLINK)) {
              Nounlink = 1;
              strcpy (cmd, "killme" );
            }
            SendText (Fd,cmd,0);
          }
        }else{
          if (cix >= MAX_COMMAND_SIZE-1) {
            err_msg ("command too long");
            cix = 0;
          } else {
            cix++;
          }
        }
      }
    }
  }
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
    close (Fd[0]);
    close (Fd[1]);
    close_fifos();
    is_closed = 1;
  }
}

void close_fifos(void) {
  close (Ffdw);
  close (Ffdr);
  if (!Nounlink) {
    strcat (F_name,"C");
    unlink (F_name);
    F_name[strlen(F_name)-1] = 'M';
    unlink (F_name);
    F_name[strlen(F_name)-1] = 'R';
    unlink (F_name);
  }
  free (F_name);
  Ffdr = -1;
  Ffdw = -1;
}

/*
 * open fifos
 */
int open_fifos (const char *f_stem) {
  char *fc_name, *fm_name;

  /* create 2 fifos */
  fc_name = malloc( strlen(f_stem) + 2 );
  if (fc_name == NULL) {
    err_msg( "allocating command" );
    return -1;
  }
  fm_name = malloc( strlen(f_stem) + 2 );
  if (fm_name == NULL) {
    err_msg( "allocating message" );
    return -1;
  }
  strcpy(fc_name,f_stem);
  strcpy(fm_name,f_stem);
  strcat(fc_name, "C");
  strcat(fm_name, "M");

  if( (Ffdw = open(fc_name, O_RDWR | O_NONBLOCK ) ) > 0) {
    write_f( Ffdw, CMD_KILL_NOUNLINK, strlen(CMD_KILL_NOUNLINK) );
    close( Ffdw );
  }

  unlink( fm_name );
  unlink( fc_name );

  if( mkfifo( fm_name, S_IRUSR | S_IWUSR ) < 0 ) {
    err_msg( fm_name );
    return -1;
  }
  if( mkfifo( fc_name, S_IRUSR | S_IWUSR ) < 0 ) {
    err_msg( fc_name );
    return -1;
  }

  Ffdr = open(fc_name, O_RDWR | O_NONBLOCK | O_TRUNC);
  if (Ffdr < 0) {
    err_msg( "opening command fifo" );
    return -1;
  }
  free(fc_name);
  Ffdw = open(fm_name, O_RDWR | O_NONBLOCK | O_TRUNC);
  if (Ffdw < 0) {
    err_msg( "opening message fifo" );
    return -1;
  }
  free(fm_name);

  F_name = malloc (strlen (f_stem) + 2);
  if (F_name == NULL) {
    err_msg( "allocating name string" );
    return -1;
  }
  strcpy (F_name, f_stem);

  return 0;
}


/*
 * Process window list messages
 */

void process_message(unsigned long type,unsigned long *body){
  int  msglen;

  switch(type) {

  case M_CONFIGURE_WINDOW:
    relay_packet( type, 24*SOL, body );
    break;

  case M_WINDOW_NAME:
  case M_ICON_NAME:
  case M_RES_CLASS:
  case M_RES_NAME:
  case M_ICON_FILE:
  case M_DEFAULTICON:
    msglen = strlen( (char *)&body[3] );
    relay_packet( type, msglen+1+3*SOL, body );
    break;

  case M_END_WINDOWLIST:
  case M_END_CONFIG_INFO:
    relay_packet( type, 0*SOL, body );
    break;

  case M_ICON_LOCATION:
    relay_packet( type, 7*SOL, body );
    break;

  case M_ERROR:
  case M_STRING:
  case M_CONFIG_INFO:
    msglen = strlen( (char *)&body[3] );
    relay_packet( type, msglen+1+3*SOL, body );
    break;


  case M_MINI_ICON:
    relay_packet( type, 6*SOL, body );
    break;


  case M_NEW_PAGE:
    relay_packet( type, 5*SOL, body );
    break;

  case M_NEW_DESK:
    relay_packet( type, 1*SOL, body );
    break;

  case M_ADD_WINDOW:
    relay_packet( type, 24*SOL, body );
    break;

  case M_RAISE_WINDOW:
  case M_LOWER_WINDOW:
  case M_FOCUS_CHANGE:
  case M_DESTROY_WINDOW:
  case M_DEICONIFY:
  case M_MAP:
  case M_WINDOWSHADE:
  case M_DEWINDOWSHADE:
    relay_packet( type, 4*SOL, body );
    break;

  case M_ICONIFY:
    relay_packet( type, 7*SOL, body );

    break;

  default:
    relay_packet( type, 4*SOL, body );
  }
}



/*
 * print error message on stderr and exit
 */

void err_msg( const char *msg ) {
  fprintf( stderr, "%s server error in %s, %s\n",
	   MYNAME, msg, strerror(errno) );
}

void err_quit( const char *msg ) {
  err_msg(msg);
  close_pipes();
  exit(1);
}

/*
 * relay packet to front-end
 */
void relay_packet( unsigned long type,
		   unsigned long length, unsigned long *body) {
  write_f( Ffdw, (char*)&type, SOL );
  write_f( Ffdw, (char*)&length, SOL );
  write_f( Ffdw, (char*)body, length );
}

/*
 * write to fifo
 */
int write_f (int fd, char *p, int len) {
  int    i, n;
  struct timeval tv;
  int    again;
  static int giveup=0;

  again = 0;
  tv.tv_sec = 0;
  tv.tv_usec = 50000;

  for (i=0; i<len; )  {
    n = write (fd, &p[i], len-i);
    if (n<0) {
      if( errno==EINTR) {
	continue;
      } else if (errno==EAGAIN ) {
	if (again++ < 5) {
	  select(0, 0, 0, 0, &tv);
	  continue;
	}else{
	  if (giveup++ > 20) {
	    Connect = 0;
	  }
	  return -1; /* give up this message */
	}
      }
      err_quit ("writing fifo");
    }
    i += n;
  }
  giveup = 0;
  return 0;
}
