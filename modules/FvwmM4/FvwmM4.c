/* This module, and the entire FvwmM4 program, and the concept for
 * interfacing this module to the Window Manager, are all original work
 * by Robert Nation
 *
 * Copyright 1994, Robert Nation
 *  No guarantees or warantees or anything
 * are provided or implied in any way whatsoever. Use this program at your
 * own risk. Permission to use this program for any purpose is given,
 * as long as the copyright is kept intact. */

#define TRUE 1
#define FALSE  0

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
#include <pwd.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>          

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>

#include "../../fvwm/module.h"

#include "FvwmM4.h"
#include "../../libs/fvwmlib.h"
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/extensions/shape.h>
#include <X11/Xmu/SysUtil.h>
#define Resolution(pixels, mm) ((((pixels) * 100000 / (mm)) + 50) / 100)

char *MyName;
int fd[2];

int ScreenWidth, ScreenHeight;
int Mscreen;

long Vx, Vy;
static char *MkDef(char *name, char *def);
static char *MkNum(char *name,int def);
static char *m4_defs(Display *display, const char *host, char *m4_options, char *config_file);
#define MAXHOSTNAME 255
#define EXTRA 50

int  m4_enable;                 /* use m4? */
int  m4_prefix;                 /* Do GNU m4 prefixing (-P) */
char m4_options[BUFSIZ];        /* Command line options to m4 */
char m4_outfile[BUFSIZ] = "";   /* The output filename for m4 */
char *m4_prog = "m4";           /* Name of the m4 program */
int  m4_default_quotes;         /* Use default m4 quotes */
char *m4_startquote = "`";         /* Left quote characters for m4 */
char *m4_endquote = "'";           /* Right quote characters for m4 */      

/***********************************************************************
 *
 *  Procedure:

 *	main - start of module
 *
 ***********************************************************************/
void main(int argc, char **argv)
{
  Display *dpy;			/* which display are we talking to */
  char *temp, *s;
  char *display_name = NULL;
  char *filename;
  char *tmp_file, read_string[80],delete_string[80];
  int i,m4_debug = 0;

  m4_enable = TRUE;
  m4_prefix = FALSE;
  strcpy(m4_options,"");
  m4_default_quotes = 1;
  
  /* Record the program name for error messages */
  temp = argv[0];

  s=strrchr(argv[0], '/');
  if (s != NULL)
    temp = s + 1;

  MyName = safemalloc(strlen(temp)+2);
  strcpy(MyName,"*");
  strcat(MyName, temp);

  if(argc < 6)
    {
      fprintf(stderr,"%s Version %s should only be executed by fvwm!\n",MyName,
	      VERSION);
      fprintf(stderr,"Wanted argc == 6. Got %d\n",argc);
      exit(1);
    }

  /* Open the X display */
  if (!(dpy = XOpenDisplay(display_name))) 
    {
      fprintf(stderr,"%s: can't open display %s", MyName,
	      XDisplayName(display_name));
      exit (1);
    }

  
  Mscreen= DefaultScreen(dpy);
  ScreenHeight = DisplayHeight(dpy,Mscreen);
  ScreenWidth = DisplayWidth(dpy,Mscreen);

  /* We should exit if our fvwm pipes die */
  signal (SIGPIPE, DeadPipe);  
  
  fd[0] = atoi(argv[1]);
  fd[1] = atoi(argv[2]);

  for(i=6;i<argc;i++)
    {
      if(mystrcasecmp(argv[i],"-m4-prefix") == 0)
	{
	  m4_prefix = TRUE;
	}
      else if(mystrcasecmp(argv[i],"-m4opt") == 0)
	{
	  /* leaving this in just in case-- any option starting with '-'
 	     will get passed on to m4 anyway */
	  strcat(m4_options, argv[++i]);
	  strcat(m4_options, " ");	    
	}
      else if(mystrcasecmp(argv[i],"-m4-squote") == 0)
	{
	  m4_startquote = argv[++i];	  
	  m4_default_quotes = 0;
	}
      else if(mystrcasecmp(argv[i],"-m4-equote") == 0)
	{
	  m4_endquote = argv[++i];
	  m4_default_quotes = 0;
	}
      else if (mystrcasecmp(argv[i], "-m4prog") == 0)
	{
	  m4_prog = argv[++i];
	}
      else if(mystrcasecmp(argv[i], "-outfile") == 0)
	{
	  strcpy(m4_outfile,argv[++i]);
	}
      else if(mystrcasecmp(argv[i], "-debug") == 0)
	{
	  m4_debug = 1;
	}
      else if (mystrncasecmp(argv[i],"-",1) == 0)
	{
	  /* pass on any other arguments starting with '-' to m4 */
	  strcat(m4_options, argv[i]);
	  strcat(m4_options, " ");	    
        }
      else
	filename = argv[i];
    }

  for(i=0;i<strlen(filename);i++)
    if((filename[i] == '\n')||(filename[i] == '\r'))
      {
	filename[i] = 0;
      }

  if (!(dpy = XOpenDisplay(display_name))) 
    {
      fprintf(stderr,"FvwmM4: can't open display %s",
	      XDisplayName(display_name));
      exit (1);
    }
  
  tmp_file = m4_defs(dpy, display_name,m4_options, filename);

  sprintf(read_string,"read %s\n",tmp_file);
  SendInfo(fd,read_string,0);

  /* For a debugging version, we may wish to omit this part. */
  /* I'll let some m4 advocates clean this up */
  if(!m4_debug)
    {
      sprintf(delete_string,"exec rm %s\n",tmp_file);
      SendInfo(fd,delete_string,0);
    }
}



static char *m4_defs(Display *display, const char *host, char *m4_options, char *config_file)
{
  Screen *screen;
  Visual *visual;
  char client[MAXHOSTNAME], server[MAXHOSTNAME], *colon;
  char ostype[BUFSIZ];
  char options[BUFSIZ];
  static char tmp_name[BUFSIZ];
  struct hostent *hostname;
  char *vc;			/* Visual Class */
  FILE *tmpf;
  int fd;
  struct passwd *pwent;
  /* Generate a temporary filename.  Honor the TMPDIR environment variable,
     if set. Hope nobody deletes this file! */

  if (strlen(m4_outfile) == 0) {
    if ((vc=getenv("TMPDIR"))) {
      strcpy(tmp_name, vc);
    } else {
      strcpy(tmp_name, "/tmp");
    }
    strcat(tmp_name, "/fvwmrcXXXXXX");
    mktemp(tmp_name);
  } else {
    strcpy(tmp_name,m4_outfile);
  }

  if (*tmp_name == '\0')
  {
    perror("mktemp failed in m4_defs");
    exit(0377);
  }

  /*
  ** check to make sure it doesn't exist already, to prevent security hole
  */
  if ((fd = open(tmp_name, O_WRONLY|O_EXCL|O_CREAT, 0600)) < 0)
  {
    perror("exclusive open for output file failed in m4_defs");
    exit(0377);
  }
  close(fd);

  /*
   * Create the appropriate command line to run m4, and
   * open a pipe to the command.
   */

  if(m4_prefix)
    sprintf(options, "%s --prefix-builtins %s > %s\n",
	    m4_prog,
	    m4_options, tmp_name);
  else
    sprintf(options, "%s  %s > %s\n",
	    m4_prog,
	    m4_options, tmp_name);
  tmpf = popen(options, "w");
  if (tmpf == NULL) {
    perror("Cannot open pipe to m4");
    exit(0377);
  }
    
  mygethostname(client,MAXHOSTNAME);
  
  mygetostype  (ostype, sizeof ostype);
  
  /* Change the quoting characters, if specified */
  
  if (!m4_default_quotes)
  {
    fprintf(tmpf, "changequote(%s, %s)dnl\n", m4_startquote, m4_endquote);
  }
  
  hostname = gethostbyname(client);
  strcpy(server, XDisplayName(host));
  colon = strchr(server, ':');
  if (colon != NULL) *colon = '\0';
  if ((server[0] == '\0') || (!strcmp(server, "unix")))
    strcpy(server, client);	/* must be connected to :0 or unix:0 */
  
  /* TWM_TYPE is fvwm, for completeness */
  
  fputs(MkDef("TWM_TYPE", "fvwm"), tmpf);
  
  /* The machine running the X server */
  fputs(MkDef("SERVERHOST", server), tmpf);
  /* The machine running the window manager process */
  fputs(MkDef("CLIENTHOST", client), tmpf);
  if (hostname)
    fputs(MkDef("HOSTNAME", (char *)hostname->h_name), tmpf);
  else
    fputs(MkDef("HOSTNAME", (char *)client), tmpf);
  
  fputs(MkDef("OSTYPE", ostype), tmpf);
  
  pwent=getpwuid(geteuid());
  fputs(MkDef("USER", pwent->pw_name), tmpf);
  
  fputs(MkDef("HOME", getenv("HOME")), tmpf);
  fputs(MkNum("VERSION", ProtocolVersion(display)), tmpf);
  fputs(MkNum("REVISION", ProtocolRevision(display)), tmpf);
  fputs(MkDef("VENDOR", ServerVendor(display)), tmpf);
  fputs(MkNum("RELEASE", VendorRelease(display)), tmpf);
  screen = ScreenOfDisplay(display, Mscreen);
  visual = DefaultVisualOfScreen(screen);
  fputs(MkNum("WIDTH", DisplayWidth(display,Mscreen)), tmpf);
  fputs(MkNum("HEIGHT", DisplayHeight(display,Mscreen)), tmpf);
  
  fputs(MkNum("X_RESOLUTION",Resolution(screen->width,screen->mwidth)),tmpf);
  fputs(MkNum("Y_RESOLUTION",Resolution(screen->height,screen->mheight)),tmpf);
  fputs(MkNum("PLANES",DisplayPlanes(display, Mscreen)), tmpf);
  
  fputs(MkNum("BITS_PER_RGB", visual->bits_per_rgb), tmpf);
  fputs(MkNum("SCREEN", Mscreen), tmpf);
  
  switch(visual->class) 
  {
    case(StaticGray):
      vc = "StaticGray";
      break;
    case(GrayScale):
      vc = "GrayScale";
      break;
    case(StaticColor):
      vc = "StaticColor";
      break;
    case(PseudoColor):
      vc = "PseudoColor";
      break;
    case(TrueColor):
      vc = "TrueColor";
      break;
    case(DirectColor):
      vc = "DirectColor";
      break;
    default:
      vc = "NonStandard";
      break;
  }

  fputs(MkDef("CLASS", vc), tmpf);
  if (visual->class != StaticGray && visual->class != GrayScale) 
    fputs(MkDef("COLOR", "Yes"), tmpf);
  else 
    fputs(MkDef("COLOR", "No"), tmpf);
  fputs(MkDef("FVWM_VERSION", VERSION), tmpf);

  /* Add options together */
  *options = '\0';
#ifdef	SHAPE
  strcat(options, "SHAPE ");
#endif
#ifdef	XPM
  strcat(options, "XPM ");
#endif

  strcat(options, "M4 ");

#ifdef	NO_SAVEUNDERS
  strcat(options, "NO_SAVEUNDERS ");
#endif

  fputs(MkDef("OPTIONS", options), tmpf);

  fputs(MkDef("FVWM_MODULEDIR", FVWM_MODULEDIR), tmpf);
    
  /*
   * At this point, we've sent the definitions to m4.  Just include
   * the fvwmrc file now.
   */
    
  fprintf(tmpf, "%sinclude(%s%s%s)\n",
          (m4_prefix) ? "m4_": "",
          m4_startquote,
          config_file,
          m4_endquote);
  
  pclose(tmpf);
  return(tmp_name);
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



static char *MkDef(char *name, char *def)
{
    static char *cp = NULL;
    static int maxsize = 0;
    int n;
    
    /* The char * storage only lasts for 1 call... */

    /* Get space to hold everything, if needed */
    
    n = EXTRA + strlen(name) + strlen(def);
    if (n > maxsize) {
	maxsize = n;
	if (cp == NULL) {
	    cp = malloc(n);
	} else {
	    cp = realloc(cp, n);
	}
    }

    if (cp == NULL) {
	perror("MkDef can't allocate enough space for a macro definition");
	exit(0377);
    }

    /* Create the macro definition, using the appropriate prefix, if any */

    if (m4_prefix)
      {
	strcpy(cp, "m4_define(");
      }
    else
      strcpy(cp, "define(");
    
    strcat(cp, name);

    /* Tack on "," and 2 sets of starting quotes */
    strcat(cp, ",");
    strcat(cp, m4_startquote);
    strcat(cp, m4_startquote);

    /* The definition itself */
    strcat(cp, def);

    /* Add 2 sets of closing quotes */
    strcat(cp, m4_endquote);
    strcat(cp, m4_endquote);

    /* End the definition, appropriately */
    strcat(cp, ")");
    if (m4_prefix)
      {
	strcat(cp, "m4_");
      }

    strcat(cp, "dnl\n");
     
   return(cp);
}

static char *MkNum(char *name,int def)
{
    char num[20];
    
    sprintf(num, "%d", def);
    return(MkDef(name, num));
}
