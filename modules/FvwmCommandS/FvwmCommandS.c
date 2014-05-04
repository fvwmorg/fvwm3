/* -*-c-*- */
/*
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
 * along with this program; if not, see: <http://www.gnu.org/licenses/>
 */

#include "config.h"

#include "FvwmCommand.h"
#include "libs/fvwmlib.h"
#include "libs/fvwmsignal.h"
#include "libs/Strings.h"

#define MYNAME   "FvwmCommandS"

static int Fd[2]; /* pipes to fvwm */
static int FfdC; /* command fifo file discriptors */
static int FfdM; /* message fifo file discriptors */
static struct stat stat_buf;

static char *FfdC_name = NULL, *FfdM_name = NULL; /* fifo names */

static ino_t FfdC_ino, FfdM_ino; /* fifo inode numbers */

int open_fifos(const char *f_stem);
void close_fifos(void);
void close_pipes(void);
void err_msg(const char *msg);
void err_quit(const char *msg) __attribute__((noreturn));
void process_message(unsigned long type,unsigned long *body);
void relay_packet(unsigned long, unsigned long, unsigned long *);
void server(char *);
static RETSIGTYPE sig_handler(int);
static char *bugger_off = "killme\n";

/* a queue of messages for FvwmCommand to receive */
typedef struct Q
{
  unsigned long length;
  unsigned long sent;
  char *body;
  struct Q *next;
} Q;
/* head and tail of the queue */
static Q *Qstart = NULL;
static Q *Qlast = NULL;
/* flag to indicate new queue items available */
static Bool queueing = False;

int main(int argc, char *argv[])
{
  char *fifoname;

  if (argc < FARGS)
  {
    fprintf(stderr, "%s version %s should only be executed by fvwm!\n",
	    MYNAME, VERSION);
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

#ifdef SA_RESTART
    sigact.sa_flags = SA_RESTART;
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
  siginterrupt(SIGINT, 0);
  siginterrupt(SIGHUP, 0);
  siginterrupt(SIGQUIT, 0);
  siginterrupt(SIGTERM, 0);
  siginterrupt(SIGPIPE, 0);
#endif
#endif

  Fd[0] = atoi(argv[1]);
  Fd[1] = atoi(argv[2]);

  server(fifoname);
  close_pipes();
  return 1;
}

/*
 * Signal handler
 */
static RETSIGTYPE
sig_handler(int signo)
{
  fvwmSetTerminate(signo);
  SIGNAL_RETURN;
}

/*
 * setup server and communicate with fvwm and the client
 */
void server (char *name)
{
  char *f_stem;
  int  len;
  fd_set fdrset, fdwset;
  char buf[MAX_MODULE_INPUT_TEXT_LEN + 1];  /* command receiving buffer */
  char cmd[MAX_MODULE_INPUT_TEXT_LEN + 1];
  int  ix,cix;
  struct timeval tv;

  tv.tv_sec = 10;
  tv.tv_usec = 0;

  if (name == NULL)
  {
     if ((f_stem = fifos_get_default_name()) == NULL)
     {
	exit(-1);
     }
  }
  else
  {
    f_stem = name;
  }

  if (open_fifos(f_stem) < 0)
  {
    exit (-1);
  }

  cix = 0;

  /*Accept reply-messages */
  SetMessageMask(Fd, MX_REPLY);
  /* tell fvwm we're running */
  SendFinishedStartupNotification(Fd);

  while (!isTerminated)
  {
    int ret;

    FD_ZERO(&fdrset);
    FD_ZERO(&fdwset);
    FD_SET(FfdC, &fdrset);
    FD_SET(Fd[1], &fdrset);
    if (queueing)
      FD_SET(FfdM, &fdwset);

    ret = fvwmSelect(FD_SETSIZE, &fdrset, &fdwset, 0, queueing ? &tv : NULL);

    if (ret < 0)
      continue;

    if (queueing && ret == 0) {
      /* a timeout has occurred, this means the pipe to FvwmCommand is full
       * dump any messages in the queue that have not been partially sent */
      Q *q1, *q2;

#ifdef PARANOMIA
      /* do nothing if there are no messages (should never happen) */
      if (!Qstart) {
	queueing = False;
	continue;
      }
#endif
      q1 = Qstart->next;

      /* if the first message has been partially sent don't remove it */
      if (!Qstart->sent) {
	free(Qstart->body);
	free(Qstart);
	Qstart = NULL;
      } else
	fprintf(stderr, "FvwmCommandS: leaving a partially sent message in the queue\n");

      /* now remove the rest of the message queue */
      while ((q2 = q1) != NULL) {
	q1 = q1->next;
	free(q2->body);
	free(q2);
      }

      /* there is either one message left (partially complete) or none */
      Qlast = Qstart;
      queueing = False;
      continue;
    }

    if (FD_ISSET(Fd[1], &fdrset))
    {
      FvwmPacket* packet = ReadFvwmPacket(Fd[1]);

      if (packet == NULL)
      {
	break;
      }
      process_message(packet->type, packet->body);
    }

    if (FD_ISSET(FfdC, &fdrset))
    {
      /*
       * This descriptor is non-blocking, hence we should never
       * block here! Also, the select() call should guarantee that
       * there is something here to read, and the signal handling
       * should be restarting interrupted reads. Together, these
       * should severely restrict the types of errors that we can see.
       */
      len = read(FfdC, buf, MAX_MODULE_INPUT_TEXT_LEN);
      if (len <= 0)
      {
	if ((len < 0) && (errno == EAGAIN))
	{
	  /*
	   * This probably won't happen - the select() has told us that
	   * there's something to read. The only possible thing that could
	   * cause this is if somebody else read the data first ... (who?)
	   */
	  continue;
	}

	/*
	 * Any other error, such as an invalid descriptor (?) or someone
	 * closing the remote end of our pipe, and we give up!
	 */
	err_quit("reading fifo");
      }

      /* in case of multiple long lines */
      for (ix = 0; ix < len; ix++)
      {
	cmd[cix] = buf[ix];
	if (cmd[cix] == '\n')
	{
	  cmd[cix] = '\0';
	  cix = 0;
	  if (StrHasPrefix(bugger_off, cmd))
	    /* fvwm will close our pipes when it has processed this */
	    SendQuitNotification(Fd);
	  else
	    SendText(Fd, cmd, 0);
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

    if (queueing && FD_ISSET(FfdM, &fdwset))
    {
      int sent;
      Q *q = Qstart;

      /* send one packet to FvwmCommand, try to send it all but cope
       * with partial success */
      sent = write(FfdM, q->body + q->sent, q->length - q->sent);
      if (sent == q->length - q->sent) {
	Qstart = q->next;
	free(q->body);
	free(q);
	if (Qstart == NULL) {
	  Qlast = NULL;
	  queueing = False;
	}
      } else if (sent >= 0)
	q->sent += sent;
    }

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
  FfdC = open(FfdC_name, O_RDWR | O_NONBLOCK | O_NOFOLLOW);
  /* remove the fifo's if they exist */
  unlink(FfdM_name);
  unlink(FfdC_name);
  /* If a previous FvwmCommandS is lingering tell it to go away
   * It will check that it owns the fifo's before removing them so no
   * there is no need to wait before creating our own */
  if (FfdC > 0)
  {
    int n;

    n = write(FfdC, bugger_off, strlen(bugger_off));
    (void)n;
    close(FfdC);
  }

  /* now we can create the fifos knowing that they don't already exist
   * and any lingering FvwmCommandS will not share them or remove them */
  if (mkfifo(FfdM_name, FVWM_S_IRUSR | FVWM_S_IWUSR) < 0)
  {
    err_msg(FfdM_name);
    return -1;
  }
  if (mkfifo(FfdC_name, FVWM_S_IRUSR | FVWM_S_IWUSR) < 0)
  {
    err_msg(FfdC_name);
    return -1;
  }

  if ((FfdM = open(FfdM_name, O_RDWR | O_NONBLOCK | O_NOFOLLOW)) < 0)
  {
    err_msg("opening message fifo");
    return -1;
  }
  if ((FfdC = open(FfdC_name, O_RDWR | O_NONBLOCK | O_NOFOLLOW)) < 0)
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
  {
    FfdC_ino = stat_buf.st_ino;
    if (!(S_IFIFO & stat_buf.st_mode))
    {
      err_msg("command fifo is not a fifo");
      return -1;
    }
    if (stat_buf.st_mode & (S_IRWXG | S_IRWXO))
    {
      err_msg("command fifo is too permissive");
      return -1;
    }
  }
  if (fstat(FfdM, &stat_buf) != 0)
  {
    err_msg("stat()ing message fifo");
    return -1;
  }
  else
  {
    FfdM_ino = stat_buf.st_ino;
    if (!(S_IFIFO & stat_buf.st_mode))
    {
      err_msg("message fifo is not a fifo");
      return -1;
    }
    if (stat_buf.st_mode & (S_IRWXG | S_IRWXO))
    {
      err_msg("message fifo is too permissive");
      return -1;
    }
  }

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
  case MX_REPLY:
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
 * this is now implemented by simply adding the packet to a queue
 * and letting the main select loop handle sending from the queue to
 * the front end. In this way FvwmCommandS is always responsive to commands
 * from the input pipes. (it will also die a lot faster when fvwm quits)
 */
void relay_packet(unsigned long type, unsigned long length,
		  unsigned long *body)
{
  Q *new;

  if (!body)
	  return;

  new = xmalloc(sizeof(Q));

  new->length = length + 2 * SOL;
  new->sent = 0L;
  new->body = xmalloc(new->length);
  memcpy(new->body, &type, SOL);
  memcpy(new->body + SOL, &length, SOL);
  memcpy(new->body + 2 * SOL, body, length);
  new->next = NULL;

  if (Qlast)
    Qlast->next = new;
  Qlast = new;
  if (!Qstart)
    Qstart = Qlast;
  queueing = True;
}
