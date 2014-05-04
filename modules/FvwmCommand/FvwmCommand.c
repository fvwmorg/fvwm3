/* -*-c-*- */
/*
 * Fvwm command input interface.
 *
 * Copyright 1998, Toshi Isogai.
 * Use this program at your own risk.
 * Permission to use this program for any purpose is given,
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
#include "libs/fvwmsignal.h"
#include "libs/fvwmlib.h"

#define MYNAME "FvwmCommand"

static int  Fdr, Fdw;  /* file discriptor for fifo */
static int  Fdrun;     /* file discriptor for run file */
static char *Fr_name = NULL;
static fd_set fdset;

static struct timeval Tv;
static int  Opt_reply; /* wait for replay */
static int  Opt_monitor;
static int  Opt_stdin;
static int  Opt_info;
static int  Opt_Serv;
static int  Opt_flags;

volatile sig_atomic_t  Bg;  /* FvwmCommand in background */


void err_msg( const char *msg );
void err_quit( const char *msg );
void sendit( char *cmd );
void receive( void );
static RETSIGTYPE sig_ttin(int);
static RETSIGTYPE sig_quit(int);
void usage(void);
int  read_f(int fd, char *p, int len);
void close_fifos(void);

void process_message( void ) ;
void list( unsigned long *body, char *) ;
void list_configure(unsigned long *body);
void list_focus_change(unsigned long *body) ;
void list_header(unsigned long *body, char *) ;
void list_icon_loc(unsigned long *body) ;
void list_iconify(unsigned long *body);
void list_mini_icon(unsigned long *body) ;
void list_new_desk(unsigned long *body) ;
void list_new_page(unsigned long *body) ;
void list_string (char *str);
void spawn_child( void );


/*
 *
 * send command to and receive message from the server
 *
 */
int main ( int argc, char *argv[])
{
  char cmd[MAX_MODULE_INPUT_TEXT_LEN + 1];
  char *f_stem, *fc_name, *fm_name;
  char *sf_stem;
  char *s;
  int  i;
  int  opt;
  int  ncnt;
  int  count;
  int  Rc;
  struct timeval tv2;
  extern char *optarg;
  extern int  optind;

#ifdef HAVE_SIGACTION
  {
    struct sigaction sigact;

    sigemptyset(&sigact.sa_mask);
#ifdef SA_RESTART
    sigact.sa_flags = SA_RESTART;
#else
    sigact.sa_flags = 0;
#endif
    sigact.sa_handler = sig_ttin;
    sigaction(SIGTTIN, &sigact, NULL);
    sigaction(SIGTTOU, &sigact, NULL);

    sigaddset(&sigact.sa_mask, SIGINT);
    sigaddset(&sigact.sa_mask, SIGHUP);
    sigaddset(&sigact.sa_mask, SIGQUIT);
    sigaddset(&sigact.sa_mask, SIGTERM);

#ifdef SA_INTERRUPT
    sigact.sa_flags = SA_INTERRUPT;
#else
    sigact.sa_flags = 0;
#endif
    sigact.sa_handler = sig_quit;
    sigaction(SIGINT, &sigact, NULL);
    sigaction(SIGHUP, &sigact, NULL);
    sigaction(SIGQUIT, &sigact, NULL);
    sigaction(SIGTERM, &sigact, NULL);
    sigaction(SIGPIPE, &sigact, NULL);
  }
#else
#ifdef USE_BSD_SIGNALS
  fvwmSetSignalMask(
	  sigmask(SIGINT) | sigmask(SIGHUP) | sigmask(SIGQUIT) |
	  sigmask(SIGTERM) | sigmask(SIGPIPE));
#endif
  signal(SIGINT, sig_quit);
  signal(SIGHUP, sig_quit);
  signal(SIGQUIT, sig_quit);
  signal(SIGTERM, sig_quit);
  signal(SIGPIPE, sig_quit);
  signal(SIGTTIN, sig_ttin);
  signal(SIGTTOU, sig_ttin);
#ifdef HAVE_SIGINTERRUPT
  siginterrupt(SIGINT, 1);
  siginterrupt(SIGHUP, 1);
  siginterrupt(SIGQUIT, 1);
  siginterrupt(SIGTERM, 1);
  siginterrupt(SIGPIPE, 1);
  siginterrupt(SIGTTIN, 0);
  siginterrupt(SIGTTOU, 0);
#endif
#endif

  Opt_reply = 0;
  Opt_info = -1;
  f_stem = NULL;
  sf_stem = NULL;
  Opt_monitor = 0;
  Opt_stdin = 0;
  Opt_Serv = 0;
  Opt_flags = 2;
  Tv.tv_sec = 0;
  Tv.tv_usec = 500000;
  Rc = 0;
  Bg = 0;


  while( (opt = getopt( argc, argv, "S:hvF:f:w:i:rmc" )) != EOF )
  {
    switch(opt)
    {
    case 'h':
      usage();
      exit(0);
      break;
    case 'f':
      f_stem = optarg;
      break;
    case 'F':
      Opt_flags = atoi (optarg);
      break;
    case 'S':
      sf_stem = optarg;
      Opt_Serv = 1;
      break;
    case 'i':
      Opt_info = atoi( optarg );
      break;
    case 'v':
      printf("%s %s\n", MYNAME, VERSION );
      exit(0);
    case 'w':
      Tv.tv_usec = atoi( optarg ) % 1000000;
      Tv.tv_sec = atoi( optarg ) / 1000000;
      break;
    case 'm':
      Opt_monitor = 1;
      break;
    case 'r':
      Opt_reply = 1;
      break;
    case 'c':
      Opt_stdin = 1;
      break;
    case '?':
      exit(3);
    }
  }

  if( f_stem == NULL )
  {
    if ((f_stem = fifos_get_default_name()) == NULL)
    {
       fprintf (stderr, "\n%s can't decide on fifo-name. "
	       "Make sure that $FVWM_USERDIR is set.\n",
	       MYNAME );
       exit(1);
    }
  }

  /* create 2 fifos */
  /* TA:  FIXME!  xasprintf() */
  fm_name = xmalloc( strlen(f_stem) + 2 );
  strcpy(fm_name,f_stem);
  strcat(fm_name, "M");
  fc_name = xmalloc( strlen(f_stem) + 2 );
  strcpy(fc_name,f_stem);
  strcat(fc_name, "C");
  s = xmalloc( strlen(f_stem) + 2 );
  strcpy(s,f_stem);
  strcat(s, "R");

  Fdrun = open(s, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, 0600);
  if (Fdrun < 0)
  {
    FILE *f;

    if ((f = fopen (s,"r" )) != NULL)
    {
      char *p;

      *cmd = 0;
      p = fgets(cmd, 20, f);
      (void)p;
      fclose(f);
      fprintf (stderr, "\nFvwmCommand lock file %sR is detected. "
	       "This may indicate another FvwmCommand is running. "
	       "It appears to be running under process ID:\n%s\n",
	       f_stem, (*cmd) ? cmd : "(unknown)" );
      fprintf (stderr, "You may either kill the process or run FvwmCommand "
	       "with another FIFO set using option -S and -f. "
	       "If the process doesn't exist, simply remove file:\n%sR\n\n",
	       f_stem);
      exit(1);
    }
    else
    {
      err_quit ("writing lock file");
    }
  }
  Fr_name = s;

  if( Opt_Serv )
  {
    int n;

    sprintf (cmd,"%s '%sS %s'", argv[0], MYNAME, sf_stem);
    n = system (cmd);
    (void)n;
  }

  {
    char buf[64];
    int n;

    sprintf(buf, "%d\n", (int)getpid());
    n = write(Fdrun, buf, strlen(buf));
    (void)n;
    close(Fdrun);
  }

  Fdr = Fdw = -1;
  count = 0;
  while ((Fdr=open (fm_name, O_RDONLY | O_NOFOLLOW)) < 0)
  {
    if (count++>5)
    {
      err_quit ("opening message fifo");
    }
    sleep(1);
  }
  count = 0;
  while ((Fdw=open (fc_name, O_WRONLY | O_NOFOLLOW)) < 0)
  {
    if (count++>2)
    {
      err_quit ("opening command fifo");
    }
    sleep(1);
  }

  i = optind;

  if (Opt_stdin)
  {
    while (fgets(cmd, MAX_MODULE_INPUT_TEXT_LEN - 1, stdin))
    {
      sendit(cmd);
    }
  }

  else if( Opt_monitor )
  {

    /* test if its stdin is closed for coprocess */
    tv2.tv_sec = 0;
    tv2.tv_usec = 5;
    FD_ZERO(&fdset);
    FD_SET(STDIN_FILENO, &fdset);
    ncnt = fvwmSelect(FD_SETSIZE, &fdset, 0, 0, &tv2);
    if( ncnt && (fgets( cmd, 1, stdin )==0 || cmd[0] == 0))
    {
      Bg = 1;
    }

    /* line buffer stdout for coprocess */
    setvbuf( stdout, NULL, _IOLBF, 0);

    /* send arguments first */
    for( ;i < argc; i++ )
    {
      strncpy( cmd, argv[i], MAX_MODULE_INPUT_TEXT_LEN - 1 );
      sendit( cmd );
    }


    while( !isTerminated )
    {
      FD_ZERO(&fdset);
      FD_SET(Fdr, &fdset);
      if( Bg == 0 )
      {
	FD_SET(STDIN_FILENO, &fdset);
      }
      ncnt = fvwmSelect(FD_SETSIZE, &fdset, 0, 0, NULL);

      /* message from fvwm */
      if (FD_ISSET(Fdr, &fdset))
      {
	process_message();
      }

      if( Bg == 0 )
      {
	/* command input */
	if( FD_ISSET(STDIN_FILENO, &fdset) )
	{
	  if( fgets( cmd, MAX_MODULE_INPUT_TEXT_LEN - 1, stdin ) == 0 )
	  {
	    if( Bg == 0 )
	    {
	      /* other than SIGTTIN */
	      break;
	    }
	    continue;
	  }
	  sendit( cmd );
	}
      }
    }
  }
  else
  {
    for( ;i < argc; i++ )
    {
      strncpy( cmd, argv[i], MAX_MODULE_INPUT_TEXT_LEN - 1 );
      sendit( cmd );
      if (Opt_info >= 0)
	receive();
    }
  }
  close_fifos();
  return Rc;
}

/*
 * send exit notice and close fifos
 */
void close_fifos (void)
{
  unlink (Fr_name);
  close(Fdr);
  close(Fdw);
}

/*
 *  signal handlers
 */
static RETSIGTYPE
sig_quit(int sig)
{
  close_fifos();
  fvwmSetTerminate(sig);
  SIGNAL_RETURN;
}

static RETSIGTYPE
sig_ttin(int dummy)
{
  (void)dummy;

  Bg = 1;
  signal(SIGTTIN, SIG_IGN);
  signal(SIGTTOU, SIG_IGN);
  SIGNAL_RETURN;
}

/*
 * print error message on stderr
 */
void err_quit( const char *msg )
{
  fprintf (stderr, "%s ", strerror(errno));
  err_msg(msg);
  close_fifos();
  exit(1);
}

void err_msg( const char *msg )
{
  fprintf( stderr, "%s error in %s\n", MYNAME , msg );
}

/*
 * add cr to the command and send it
 */
void sendit( char *cmd )
{
  int clen;

  if( cmd[0] != '\0'  )
  {
    clen = strlen(cmd);

    /* add cr */
    if( cmd[clen-1] != '\n' )
    {
      strcat(cmd, "\n");
      clen++;
    }
    if( clen != 1 )
    {
      int n;

      n = write( Fdw, cmd, clen );
      (void)n;
    }
  }
}

void receive(void)
{
  int  ncnt;
  struct timeval tv;

  if( Opt_reply )
  {
    /* wait indefinitely */
    FD_ZERO(&fdset);
    FD_SET( Fdr, &fdset);
    ncnt = select(FD_SETSIZE, SELECT_FD_SET_CAST &fdset, 0, 0, NULL);
  }

  while (1)
  {
    tv.tv_sec = Tv.tv_sec;
    tv.tv_usec = Tv.tv_usec;
    FD_ZERO(&fdset);
    FD_SET(Fdr, &fdset);

    ncnt = select(FD_SETSIZE, SELECT_FD_SET_CAST &fdset, 0, 0, &tv);

    if( ncnt < 0 )
    {
      err_quit("receive");
      break;
    }
    if( ncnt == 0 )
    {
      /* timeout */
      break;
    }
    if (FD_ISSET(Fdr, &fdset))
    {
      process_message();
    }
  }
}


/*
 * print usage
 */
void usage(void)
{
  fprintf (stderr, "Usage: %s [OPTION] [COMMAND]...\n", MYNAME);
  fprintf (stderr, "Send commands to fvwm via %sS\n\n", MYNAME);
  fprintf (stderr,
	   "  -c                  read and send commands from stdin\n"
	   "                      The COMMAND if any is ignored.\n");
  fprintf (stderr,
	   "  -S <file name>      "
	   "invoke another %s server with fifo <file name>\n",
	   MYNAME);
  fprintf (stderr,
	   "  -f <file name>      use fifo <file name> to connect to %sS\n",
	   MYNAME);
  fprintf (stderr,
	   "  -i <info level>     0 - error only\n" );
  fprintf (stderr,
	   "                      1 - above and config info (default)\n" );
  fprintf (stderr,
	   "                      2 - above and static info\n" );
  fprintf (stderr,
	   "                      3 - above and dynamic info\n" );
  fprintf (stderr,
	   "                     -1 - none (default, much faster)\n" );
  fprintf (stderr,
	   "  -F <flag info>      0 - no flag info\n");
  fprintf (stderr,
	   "                      2 - full flag info (default)\n");
  fprintf (stderr,
	   "  -m                  monitor fvwm message transaction\n");
  fprintf (stderr,
	   "  -r                  "
	   "wait for a reply (overrides waiting time)\n");
  fprintf (stderr,
	   "  -v                  print version number\n");
  fprintf (stderr,
	   "  -w <micro sec>      waiting time for the response from fvwm\n");
  fprintf (stderr, "\nDefault fifo names are ~/.%sC and ~/.%sM\n",
	   MYNAME, MYNAME);
  fprintf (stderr, "Default waiting time is 500,000 us\n");
}

/*
 * read fifo
 */
int read_f(int fd, char *p, int len)
{
  int i, n;
  for (i=0; i<len; )
  {
    n = read(fd, &p[i], len-i);
    if (n<0 && errno!=EAGAIN)
    {
      err_quit("reading message");
    }
    if (n==0)
    {
      /* eof */
      close_fifos();
      exit(0);
    }
    i += n;
  }
  return len;
}

/*
 * process message
 */
void process_message( void )
{
  unsigned long type, length, body[sizeof(struct ConfigWinPacket)];

  read_f( Fdr, (char*)&type, SOL);
  read_f( Fdr, (char*)&length, SOL);
  read_f( Fdr, (char*)body, length );

  if( type==M_ERROR )
  {
    fprintf( stderr,"%s", (char *)&body[3] );
  }
  else if( Opt_info >= 1 )
  {
    switch( type )
    {

    case M_WINDOW_NAME:
      list( body, "window");
      break;
    case M_ICON_NAME:
      list(body, "icon");
      break;
    case M_RES_CLASS:
      list(body, "class" );
      break;
    case M_RES_NAME:
      list(body, "resource");
      break;

    case M_END_WINDOWLIST:
      list_string("end windowlist");
      break;

    case M_ICON_FILE:
      list(body, "icon file");
      break;
    case M_ICON_LOCATION:
      list_icon_loc(body);
      break;

    case M_END_CONFIG_INFO:
      list_string("end configinfo");
      break;

    case M_DEFAULTICON:
      list(body, "default icon");
      break;

    case M_MINI_ICON:
      list_mini_icon( body );
      break;

    case M_CONFIG_INFO:
      printf( "%s\n", (char *)&body[3] );
      break;

    case MX_REPLY:
      list(body, "reply");
      break;

    default:
      if( Opt_info >=2 )
      {
	switch(type)
	{

	case M_CONFIGURE_WINDOW:
	  list_configure( body);
	  break;
	case M_STRING:
	  list(body, "string");
	  break;

	default:
	  if( Opt_info >= 3 )
	  {
	    switch( type )
	    {
	    case M_NEW_PAGE:
	      list_new_page(body);
	      break;
	    case M_NEW_DESK:
	      list_new_desk(body);
	      break;
	    case M_ADD_WINDOW:
	      list_header(body, "add");
	      list_configure( body);
	      break;
	    case M_RAISE_WINDOW:
	      list_header(body, "raise");
	      break;
	    case M_LOWER_WINDOW:
	      list_header(body, "lower");
	      break;
	    case M_FOCUS_CHANGE:
	      list_focus_change( body );
	      break;
	    case M_DESTROY_WINDOW:
	      list_header(body, "destroy");
	      break;
	    case M_ICONIFY:
	      list_iconify( body );
	      break;
	    case M_DEICONIFY:
	      list_header(body, "deiconify");
	      break;
	    case M_MAP:
	      list_header(body, "map");
	      break;
	    case M_WINDOWSHADE:
	      list_header(body, "windowshade");
	      break;
	    case M_DEWINDOWSHADE:
	      list_header(body, "dewindowshade");
	      break;
	    default:
	      printf("0x%lx type 0x%lx\n", body[0], type );
	    }
	  }
	}
      }
    }
  }
}


/*
 *
 *  print configuration info
 *
 */

void list_configure(unsigned long *body)
{
  struct ConfigWinPacket *cfgpacket = (void *)body;
  char *grav;

  printf( "0x%08lx %-20s x %ld, y %ld, width %ld, height %ld\n",
	  cfgpacket->w, "frame", cfgpacket->frame_x, cfgpacket->frame_y,
	  cfgpacket->frame_width, cfgpacket->frame_height );
  printf( "0x%08lx %-20s %ld\n" ,cfgpacket->w, "desktop", cfgpacket->desk);

  /* Oooh, you asked for it... */
  if (Opt_flags == 2)
  {
    printf( "Packet flags\n" );
    printf( "is_sticky_across_pages: %d\n",
	    IS_STICKY_ACROSS_PAGES( cfgpacket ) );
    printf( "is_sticky_across_desks: %d\n",
	    IS_STICKY_ACROSS_DESKS( cfgpacket ) );
    printf( "has_icon_font: %d\n",
	    HAS_ICON_FONT( cfgpacket ) );
    printf( "has_window_font: %d\n",
	    HAS_WINDOW_FONT( cfgpacket ) );
    printf( "do_circulate_skip: %d\n",
	    DO_SKIP_CIRCULATE( cfgpacket ) );
    printf( "do_circulate_skip_icon: %d\n",
	    DO_SKIP_ICON_CIRCULATE( cfgpacket ) );
    printf( "do_circulate_skip_shaded: %d\n",
	    DO_SKIP_SHADED_CIRCULATE( cfgpacket ) );
    printf( "do_grab_focus_when_created: %d\n",
	    FP_DO_GRAB_FOCUS( FW_FOCUS_POLICY(cfgpacket) ) );
    printf( "do_grab_focus_when_transient_created: %d\n",
	    FP_DO_GRAB_FOCUS_TRANSIENT( FW_FOCUS_POLICY(cfgpacket) ) );
    printf( "do_ignore_restack: %d\n",
	    DO_IGNORE_RESTACK( cfgpacket ) );
    printf( "do_lower_transient: %d\n",
	    DO_LOWER_TRANSIENT( cfgpacket ) );
    printf( "do_not_show_on_map: %d\n",
	    DO_NOT_SHOW_ON_MAP( cfgpacket ) );
    printf( "do_not_pass_click_focus_click: %d\n",
	    FP_DO_PASS_FOCUS_CLICK( FW_FOCUS_POLICY(cfgpacket) ) );
    printf( "do_raise_transient: %d\n",
	    DO_RAISE_TRANSIENT( cfgpacket ) );
    printf( "do_resize_opaque: %d\n",
	    DO_RESIZE_OPAQUE( cfgpacket ) );
    printf( "do_shrink_windowshade: %d\n",
	    DO_SHRINK_WINDOWSHADE( cfgpacket ) );
    printf( "do_stack_transient_parent: %d\n",
	    DO_STACK_TRANSIENT_PARENT( cfgpacket ) );
    printf( "do_window_list_skip: %d\n",
	    DO_SKIP_WINDOW_LIST( cfgpacket ) );
    printf( "has_depressable_border: %d\n",
	    HAS_DEPRESSABLE_BORDER( cfgpacket ) );
    printf( "has_mwm_border: %d\n",
	    HAS_MWM_BORDER( cfgpacket ) );
    printf( "has_mwm_buttons: %d\n",
	    HAS_MWM_BUTTONS( cfgpacket ) );
    printf( "has_mwm_override: %d\n",
	    HAS_MWM_OVERRIDE_HINTS( cfgpacket ) );
    printf( "has_no_icon_title: %d\n",
	    HAS_NO_ICON_TITLE( cfgpacket ) );
    printf( "has_override_size: %d\n",
	    HAS_OVERRIDE_SIZE_HINTS( cfgpacket ) );
    printf( "has_stippled_title: %d\n",
	    HAS_STIPPLED_TITLE( cfgpacket ) );
    printf( "is_fixed: %d\n",
	    IS_FIXED( cfgpacket ) );
    printf( "is_icon_sticky_across_pages: %d\n",
	    IS_ICON_STICKY_ACROSS_PAGES( cfgpacket ) );
    printf( "is_icon_sticky_across_desks: %d\n",
	    IS_ICON_STICKY_ACROSS_DESKS( cfgpacket ) );
    printf( "is_icon_suppressed: %d\n",
	    IS_ICON_SUPPRESSED( cfgpacket ) );
    printf( "is_lenient: %d\n",
	    FP_IS_LENIENT( FW_FOCUS_POLICY(cfgpacket) ) );
    printf( "does_wm_delete_window: %d\n",
	    WM_DELETES_WINDOW( cfgpacket ) );
    printf( "does_wm_take_focus: %d\n",
	    WM_TAKES_FOCUS( cfgpacket ) );
    printf( "do_iconify_after_map: %d\n",
	    DO_ICONIFY_AFTER_MAP( cfgpacket ) );
    printf( "do_reuse_destroyed: %d\n",
	    DO_REUSE_DESTROYED( cfgpacket ) );
    printf( "has_border: %d\n",
	    !HAS_NO_BORDER( cfgpacket ) );
    printf( "has_title: %d\n",
	    HAS_TITLE( cfgpacket ) );
    printf( "is_iconify_pending: %d\n",
	    IS_ICONIFY_PENDING( cfgpacket ) );
    printf( "is_fully_visible: %d\n",
	    IS_FULLY_VISIBLE( cfgpacket ) );
    printf( "is_iconified: %d\n",
	    IS_ICONIFIED( cfgpacket ) );
    printf( "is_iconfied_by_parent: %d\n",
	    IS_ICONIFIED_BY_PARENT( cfgpacket ) );
    printf( "is_icon_entered: %d\n",
	    IS_ICON_ENTERED( cfgpacket ) );
    printf( "is_icon_font_loaded: %d\n",
	    IS_ICON_FONT_LOADED( cfgpacket ) );
    printf( "is_icon_moved: %d\n",
	    IS_ICON_MOVED( cfgpacket ) );
    printf( "is_icon_ours: %d\n",
	    IS_ICON_OURS( cfgpacket ) );
    printf( "is_icon_shaped: %d\n",
	    IS_ICON_SHAPED( cfgpacket ) );
    printf( "is_icon_unmapped: %d\n",
	    IS_ICON_UNMAPPED( cfgpacket ) );
    printf( "is_mapped: %d\n",
	    IS_MAPPED( cfgpacket ) );
    printf( "is_map_pending: %d\n",
	    IS_MAP_PENDING( cfgpacket ) );
    printf( "is_maximized: %d\n",
	    IS_MAXIMIZED( cfgpacket ) );
    printf( "is_name_changed: %d\n",
	    IS_NAME_CHANGED( cfgpacket ) );
    printf( "is_partially_visible: %d\n",
	    IS_PARTIALLY_VISIBLE( cfgpacket ) );
    printf( "is_pixmap_ours: %d\n",
	    IS_PIXMAP_OURS( cfgpacket ) );
    printf( "is_size_inc_set: %d\n",
	    IS_SIZE_INC_SET( cfgpacket ) );
    printf( "is_transient: %d\n",
	    IS_TRANSIENT( cfgpacket ) );
    printf( "is_window_drawn_once: %d\n",
	    cfgpacket->flags.is_window_drawn_once );
    printf( "is_viewport_moved: %d\n",
	    IS_VIEWPORT_MOVED( cfgpacket ) );
    printf( "is_window_being_moved_opaque: %d\n",
	    IS_WINDOW_BEING_MOVED_OPAQUE( cfgpacket ) );
    printf( "is_window_font_loaded: %d\n",
	    IS_WINDOW_FONT_LOADED( cfgpacket ) );
    printf( "is_window_shaded %d\n",
	    IS_SHADED( cfgpacket ) );
    printf( "title_dir: %d\n",
	    GET_TITLE_DIR( cfgpacket ) );
  }

  printf( "0x%08lx %-20s %hd\n",
	  cfgpacket->w, "title height", cfgpacket->title_height);
  printf( "0x%08lx %-20s %hd\n",
	  cfgpacket->w, "border width", cfgpacket->border_width);
  printf( "0x%08lx %-20s width %ld, height %ld\n", cfgpacket->w, "base size",
	  cfgpacket->hints_base_width, cfgpacket->hints_base_height);
  printf( "0x%08lx %-20s width %ld, height %ld\n", cfgpacket->w,
	  "size increment", cfgpacket->hints_width_inc,
	  cfgpacket->hints_height_inc);
  printf( "0x%08lx %-20s width %ld, height %ld\n", cfgpacket->w, "min size",
	  cfgpacket->hints_min_width, cfgpacket->hints_min_height);
  printf( "0x%08lx %-20s width %ld, height %ld\n", cfgpacket->w, "max size",
	  cfgpacket->hints_max_width, cfgpacket->hints_max_height);

  switch(cfgpacket->hints_win_gravity)
  {
  case ForgetGravity:
    grav = "Forget";
    break;
  case NorthWestGravity:
    grav = "NorthWest";
    break;
  case NorthGravity:
    grav = "North";
    break;
  case NorthEastGravity:
    grav = "NorthEast";
    break;
  case WestGravity:
    grav = "West";
    break;
  case CenterGravity:
    grav = "Center";
    break;
  case EastGravity:
    grav = "East";
    break;
  case SouthWestGravity:
    grav = "SouthWest";
    break;
  case SouthGravity:
    grav = "South";
    break;
  case SouthEastGravity:
    grav = "SouthEast";
    break;
  case StaticGravity:
    grav = "Static";
    break;
  default:
    grav = "Unknown";
    break;
  }
  printf( "0x%08lx %-20s %s\n", cfgpacket->w, "gravity", grav);
  printf( "0x%08lx %-20s text 0x%lx, back 0x%lx\n", cfgpacket->w, "pixel",
	  cfgpacket->TextPixel, cfgpacket->BackPixel);
}

/*
 *
 * print info  icon location
 *
 */
void list_icon_loc(unsigned long *body)
{
  printf( "0x%08lx %-20s x %ld, y %ld, width %ld, height%ld\n",
	  body[0], "icon location", body[3], body[4], body[5], body[6] );
}

/*
 *
 * print info mini icon
 *
 */
void list_mini_icon(unsigned long *body)
{
  printf( "0x%08lx %-20s width %ld, height %ld, depth %ld\n",
	  body[0], "mini icon",body[5], body[6], body[7] );
}

/*
 *
 * print info  message body[3]
 *
 */
void list( unsigned long *body, char *text )
{
  printf( "0x%08lx %-20s %s\n", body[0], text, (char *)&body[3] );
}


/*
 *
 * print info  new page
 *
 */
void list_new_page(unsigned long *body)
{
  printf( "           %-20s x %ld, y %ld, desk %ld, max x %ld, max y %ld\n",
	  "new page",
	  body[0], body[1], body[2], body[3], body[4]);
}

/*
 *
 * print info  new desk
 *
 */
void list_new_desk(unsigned long *body)
{
  printf( "           %-20s %ld\n",  "new desk", body[0] );
}

/*
 *
 * print string
 *
 */
void list_string (char *str)
{
    printf( "%-20s\n", str );
}

/*
 *
 * print info header
 *
 */
void list_header(unsigned long *body, char *text)
{
  printf("0x%08lx %s\n", body[0], text);
}

/*
 *
 * print info focus change
 *
 */
void list_focus_change(unsigned long *body)
{
  printf( "0x%08lx %-20s highlight 0x%lx, foreground 0x%lx, background 0x%lx\n",
	  body[0], "focus change", body[3], body[4], body[5] );
}

/*
 *
 * print info iconify
 *
 */
void list_iconify(unsigned long *body)
{
  printf( "0x%08lx %-20s x %ld, y %ld, width %ld, hight %ld\n",
	  body[0], "iconify", body[3], body[4], body[5], body[6] );
}
