/* $Id$
 * $Source$
 *
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "FvwmCommand.h"
#include "../../libs/fvwmlib.h"

#define MYNAME "FvwmCommand"
#define MAXHOSTNAME 255

int  Fdr, Fdw;  /* file discriptor for fifo */
FILE *Frun;     /* File contains pid */
char *Fr_name;
int  Pfd;
char *getline();
fd_set fdset;

struct timeval Tv;
int  Opt_reply; /* wait for replay */
int  Opt_monitor;
int  Opt_info;
int  Opt_Serv;
int  Opt_flags;
FILE *Fp;
int  Rc;  /* return code */
int  Bg;  /* FvwmCommand in background */

char client[MAXHOSTNAME];
char hostname[32];

void err_msg( char *msg );
void err_quit( char *msg );
void sendit( char *cmd );
void receive( void );
void sig_ttin ( int );
void sig_pipe ( int );
void sig_quit ( int );
void usage(void);
int  read_f (int fd, char *p, int len);
void close_fifos (void);

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


/*******************************************************
 *
 * send command to and receive message from the server
 *
 *******************************************************/
int main ( int argc, char *argv[]) {
  char cmd[MAX_COMMAND_SIZE];
  char *home;
  char *f_stem, *fc_name, *fm_name;
  char *sf_stem;
  int  i;
  int  opt;
  int  ncnt;
  int  count;
  struct timeval tv2;
  extern char *optarg;
  extern int  optind, opterr, optopt;

  signal (SIGINT, sig_quit);
  signal (SIGHUP, sig_quit);
  signal (SIGQUIT, sig_quit);
  signal (SIGTERM, sig_quit);
  signal (SIGTTIN, sig_ttin);
  signal (SIGTTOU, sig_ttin);

  Opt_reply = 0;
  Opt_info = -1;
  f_stem = NULL;
  sf_stem = NULL;
  Opt_monitor = 0;
  Opt_Serv = 0;
  Opt_flags = 2;
  Tv.tv_sec = 0;
  Tv.tv_usec = 500000;
  Rc = 0;
  Bg = 0;


  while( (opt = getopt( argc, argv, "S:hvF:f:w:i:rm" )) != EOF ) {
    switch(opt) {
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
      printf("%s %s\n", MYNAME, MYVERSION );
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
    case '?':
      exit(3);
    }
  }


  if( f_stem == NULL ) {
    home = getenv("HOME");
    f_stem = safemalloc( strlen(home) + strlen(F_NAME) + MAXHOSTNAME + 4);
    strcpy (f_stem, home);
    if (f_stem[strlen(f_stem)-1] != '/') {
      strcat (f_stem, "/");
    }
    strcat (f_stem, F_NAME);
    /* Make it unique */
    gethostname(hostname,32);
    strcpy( client, getenv("DISPLAY") );
    if (!client[0]  ||  ':' == client[0])
    {
      sprintf( client, "%s%s", hostname, client );
    }
    strcat (f_stem, "-");
    strcat (f_stem, client);
  }

  /* create 2 fifos */
  fm_name = safemalloc( strlen(f_stem) + 2 );
  strcpy(fm_name,f_stem);
  strcat(fm_name, "M");
  fc_name = safemalloc( strlen(f_stem) + 2 );
  strcpy(fc_name,f_stem);
  strcat(fc_name, "C");
  Fr_name = safemalloc( strlen(f_stem) + 2 );
  strcpy(Fr_name,f_stem);
  strcat(Fr_name, "R");

  if ((Frun = fopen (Fr_name,"r" )) !=NULL) {
    if (fgets (cmd, 20, Frun) != NULL) {
      fprintf (stderr, "\nFvwmCommand lock file %sR is detected. "
	       "This may indicate another FvwmCommand is running. "
	       "It appears to be running under process ID:\n%s\n",
	       f_stem, cmd );
      fprintf (stderr, "You may either kill the process or run FvwmCommand "
	       "with another FIFO set using option -S and -f. "
	       "If the process doesn't exist, simply remove file:\n%sR\n\n",
	       f_stem);
      exit(1);
    }
    fclose (Frun);
    unlink (Fr_name);
  }

  if( Opt_Serv ) {
    sprintf (cmd,"%s '%sS %s'", argv[0], MYNAME, sf_stem);
    system (cmd);
  }

  if ((Frun = fopen (Fr_name,"w" )) != NULL) {
    fprintf (Frun, "%d\n", (int) getpid());
    fclose (Frun);
  }else {
      err_quit ("writing lock file");
  }

  Fdr = Fdw = -1;
  count = 0;
  while ((Fdr=open (fm_name, O_RDONLY)) < 0) {
    if (count++>5) {
      err_quit ("opening message fifo");
    }
    sleep(1);
  }
  count = 0;
  while ((Fdw=open (fc_name, O_WRONLY)) < 0) {
    if (count++>2) {
      err_quit ("opening command fifo");
    }
    sleep(1);
  }

  strcpy (cmd, CMD_CONNECT);
  sendit (cmd);

  i = optind;
  if( Opt_monitor ) {

    /* test if its stdin is closed for coprocess */
    tv2.tv_sec = 0;
    tv2.tv_usec = 5;
    FD_ZERO(&fdset);
    FD_SET(STDIN_FILENO, &fdset);
    ncnt = select(FD_SETSIZE,SELECT_FD_SET_CAST &fdset, 0, 0, &tv2);
    if( ncnt && (fgets( cmd, 1, stdin )==0 || cmd[0] == 0)) {
      Bg = 1;
    }

    /* line buffer stdout for coprocess */
    setvbuf( stdout, NULL, _IOLBF, 0);

    /* send arguments first */
    for( ;i < argc; i++ ) {
      strncpy( cmd, argv[i], MAX_COMMAND_SIZE-2 );
      sendit( cmd );
    }


    while(1) {
      FD_ZERO(&fdset);
      FD_SET(Fdr, &fdset);
      if( Bg == 0 ) {
	FD_SET(STDIN_FILENO, &fdset);
      }
      ncnt = select(FD_SETSIZE,SELECT_FD_SET_CAST &fdset, 0, 0, NULL);

      /* message from fvwm */
      if (FD_ISSET(Fdr, &fdset)){
	process_message();
      }

      if( Bg == 0 ) {
      /* command input */
	if( FD_ISSET(STDIN_FILENO, &fdset) ) {
	  if( fgets( cmd, MAX_COMMAND_SIZE-2, stdin ) == 0 ) {
	    if( Bg == 0 ) {
	      /* other than SIGTTIN */
	      break;
	    }else{
	      continue;
	    }
	  }
	  sendit( cmd );
	}
      }
    }
  }else {
    for( ;i < argc; i++ ) {
      strncpy( cmd, argv[i], MAX_COMMAND_SIZE-2 );
      sendit( cmd );
      if (Opt_info >= 0)
	receive();
    }
  }
  close_fifos();
  exit( Rc );
}

/*
 * send exit notice and close fifos
 */
void close_fifos (void) {
  char cmd[10];

  if (Fdw >= 0) {
    strcpy (cmd, CMD_EXIT);
    sendit (cmd);
  }
  close(Fdr);
  close(Fdw);
  unlink (Fr_name);
}

/******************************************
 *  signal handlers
 ******************************************/
void sig_quit (int dummy) {
  close_fifos();
  err_msg("receiving signal\n" );
  exit(1);
}

void sig_ttin( int  dummy ) {
  Bg = 1;
  signal( SIGTTIN, SIG_IGN );
}

/************************************/
/* print error message on stderr */
/************************************/
void err_quit( char *msg ) {
  fprintf (stderr, "%s ", strerror(errno));
  err_msg(msg);
  close_fifos();
  exit(1);
}

void err_msg( char *msg ) {
  fprintf( stderr, "%s error in %s\n", MYNAME , msg );
}

/*************************************/
/* add cr to the command and send it */
/*************************************/
void sendit( char *cmd ) {
  int clen;

  if( cmd[0] != '\0'  ) {
    clen = strlen(cmd);

    /* add cr */
    if( cmd[clen-1] != '\n' ) {
      strcat(cmd, "\n");
      clen++;
    }
    if( clen != 1 ) {
      write( Fdw, cmd, clen );
    }
  }
}

void receive () {
  int  ncnt;
  struct timeval tv;

  if( Opt_reply ) {
    /* wait indefinitely */
    FD_ZERO(&fdset);
    FD_SET( Fdr, &fdset);
    ncnt = select(FD_SETSIZE, SELECT_FD_SET_CAST &fdset, 0, 0, NULL);
  }


  while (1){
    tv.tv_sec = Tv.tv_sec;
    tv.tv_usec = Tv.tv_usec;
    FD_ZERO(&fdset);
    FD_SET(Fdr, &fdset);

    ncnt = select(FD_SETSIZE, SELECT_FD_SET_CAST &fdset, 0, 0, &tv);

    if( ncnt < 0 ) {
      err_quit("receive");
      break;
    }
    if( ncnt == 0 ) {
      /* timeout */
      break;
    }
    if (FD_ISSET(Fdr, &fdset)) {
      process_message();
    }
  }
}


/*******************************
 * print usage
 *******************************/
void usage(void) {
  fprintf (stderr, "Usage: %s [OPTION] [COMMAND]...\n", MYNAME);
  fprintf (stderr, "Send commands to fvwm via %sS\n\n", MYNAME);
  fprintf (stderr,
	   "  -F <flag info>      0 - no flag info\n");
  fprintf (stderr,
	   "                      2 - full flag info (default)\n");
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
	   "  -m                  monitor fvwm message transaction\n");
  fprintf (stderr,
	   "  -r                  "
	   "wait for a reply (overrides waiting time)\n");
  fprintf (stderr,
	   "  -v                  print version number\n");
  fprintf (stderr,
	   "  -w <micro sec>      waiting time for the reponse from fvwm\n");
  fprintf (stderr, "\nDefault fifo names are ~/.%sC and ~/.%sM\n",
	   MYNAME, MYNAME);
  fprintf (stderr, "Default waiting time is 500,000 us\n");
}

/*
 * read fifo
 */
int read_f (int fd, char *p, int len) {
  int i, n;
  for (i=0; i<len; )  {
    n = read (fd, &p[i], len-i);
    if (n<0 && errno!=EAGAIN) {
      err_quit("reading message");
    }
    if (n==0) {
      /* eof */
      close_fifos();
      exit( Rc );
    }
    i += n;
  }
  return len;
}

/*
 * process message
 */
void process_message( void ) {
  unsigned long type, length, body[24*SOL];

  read_f( Fdr, (char*)&type, SOL);
  read_f( Fdr, (char*)&length, SOL);
  read_f( Fdr, (char*)body, length );

  if( type==M_ERROR ) {
    fprintf( stderr,"%s", (char *)&body[3] );
    Rc = 1;
  }else if( Opt_info >= 1 ) {

    switch( type ) {

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
      printf( "%s", (char *)&body[3] );
      break;

    default:
      if( Opt_info >=2 ) {

	switch(type) {

	case M_CONFIGURE_WINDOW:
	  list_configure( body);
	  break;
	case M_STRING:
	  list(body, "string");
	  break;

	default:
	  if( Opt_info >= 3 ) {
	    switch( type ) {
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


/**********************************
 *
 *  print configuration info
 *
 **********************************/

void list_configure(unsigned long *body) {
  unsigned long i;
  char *flag;
  char *grav;
  char *flagstr[32] = {
    "StartIconic",
    "OnTop",
    "Sticky",
    "WindowListSkip",
    "SuppressIcon",
    "NoiconTitle",
    "Lenience",
    "StickyIcon",
    "CirculateSkipIcon",
    "CirculateSkip",
    "ClickToFocus",
    "SloppyFocus",
    "SkipMapping",
    "Handles",
    "Title",
    "Mapped",
    "Iconified",
    "Transient",
    "Raised", /* ??? */
    "Visible",
    "IconOurs",
    "PixmapOurs",
    "ShapedIcon",
    "Maximized",
    "WmTakeFocus",
    "WmDeleteWindow",
    "IconMoved",
    "IconUnmapped",
    "MapPending",
    "HintOverride",
    "MWMButtons",
    "MWMBorders"
  };


  printf( "0x%08lx %-20s x %ld, y %ld, width %ld, height %ld\n",
	  body[0], "frame", body[3], body[4], body[5], body[6] );
  printf( "0x%08lx %-20s %ld\n" ,body[0], "desktop", body[7]);

  if (Opt_flags == 2) {
    for( i=0; i<=31; i++ ) {
      if( body[8] & (1<<i) ) {
	flag = "yes";
      }else{
	flag = "no";
      }
      printf( "0x%08lx %-20s %s\n", body[0], flagstr[i], flag );
    }
  }

  printf( "0x%08lx %-20s %ld\n",
	  body[0], "title height", body[9]);
  printf( "0x%08lx %-20s %ld\n",
	  body[0], "border width", body[10]);
  printf( "0x%08lx %-20s width %ld, height %ld\n",
	  body[0], "base size", body[11], body[12]);
  printf( "0x%08lx %-20s width %ld, height %ld\n",
	  body[0], "size increment", body[13], body[14]);
  printf( "0x%08lx %-20s width %ld, height %ld\n",
	  body[0], "min size", body[15], body[16]);
  printf( "0x%08lx %-20s width %ld, height %ld\n",
	  body[0], "max size", body[17], body[18]);


  switch(body[21]) {
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
  printf( "0x%08lx %-20s %s\n", body[0], "gravity", grav);

  printf( "0x%08lx %-20s text 0x%lx, back 0x%lx\n",
	  body[0], "pixel", body[22], body[23]);
}

/*************************************************************************
 *
 * print info  icon location
 *
 ************************************************************************/
void list_icon_loc(unsigned long *body) {

  printf( "0x%08lx %-20s x %ld, y %ld, width %ld, height%ld\n",
	  body[0], "icon location", body[3], body[4], body[5], body[6] );
}

/*************************************************************************
 *
 * print info mini icon
 *
 ************************************************************************/
void list_mini_icon(unsigned long *body) {

  printf( "0x%08lx %-20s width %ld, height %ld, depth %ld\n",
	  body[0], "mini icon",body[5], body[6], body[7] );
}

/*************************************************************************
 *
 * print info  message body[3]
 *
 ************************************************************************/
void list( unsigned long *body, char *text ) {

  printf( "0x%08lx %-20s %s\n", body[0], text, (char *)&body[3] );
}


/*************************************************************************
 *
 * print info  new page
 *
 ************************************************************************/
void list_new_page(unsigned long *body) {

  printf( "           %-20s x %ld, y %ld, desk %ld, max x %ld, max y %ld\n",
	  "new page",
	  body[0], body[0], body[2], body[3], body[4]);
}

/*************************************************************************
 *
 * print info  new desk
 *
 ************************************************************************/
void list_new_desk(unsigned long *body) {

  printf( "           %-20s %ld\n",  "new desk", body[0] );
}

/*************************************************************************
 *
 * print string
 *
 ************************************************************************/
void list_string (char *str) {
    printf( "%-20s\n", str );
}

/*************************************************************************
 *
 * print info header
 *
 ************************************************************************/
void list_header(unsigned long *body, char *text) {

  printf("0x%08lx %s\n", body[0], text);

}

/*************************************************************************
 *
 * print info focus change
 *
 ************************************************************************/
void list_focus_change(unsigned long *body) {

  printf( "0x%08lx %-20s highlight 0x%lx, foreground 0x%lx, background 0x%lx\n",
	  body[0], "focus change", body[3], body[4], body[5] );
}

/*************************************************************************
 *
 * print info iconify
 *
 ************************************************************************/
void list_iconify(unsigned long *body) {

  printf( "0x%08lx %-20s x %ld, y %ld, width %ld, hight %ld\n",
	  body[0], "iconify", body[3], body[4], body[5], body[6] );
}

