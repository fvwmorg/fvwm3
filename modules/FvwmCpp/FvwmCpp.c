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

#include "FvwmCpp.h"
#include "../../libs/fvwmlib.h"
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>
#include <X11/extensions/shape.h>
#include <X11/Xmu/SysUtil.h>
#define Resolution(pixels, mm)  ((((pixels) * 100000 / (mm)) + 50) / 100)

char *MyName;
int fd[2];

struct list *list_root = NULL;

int ScreenWidth, ScreenHeight;
int Mscreen;

long Vx, Vy;
static char *MkDef(char *name, char *def);
static char *MkNum(char *name,int def);
static char *cpp_defs(Display *display, const char *host, char *m4_options, char *config_file);
#define MAXHOSTNAME 255
#define EXTRA 20

char *cpp_prog = FVWM_CPP;          /* Name of the cpp program */

char cpp_options[BUFSIZ];
char cpp_outfile[BUFSIZ]="";

/***********************************************************************
 *
 *  Procedure:

 *	main - start of module
 *
 ***********************************************************************/
int main(int argc, char **argv)
{
  Display *dpy;			/* which display are we talking to */
  char *temp, *s;
  char *display_name = NULL;
  char *filename = NULL;
  char *tmp_file, read_string[80],delete_string[80];
  int i,cpp_debug = 0;

  strcpy(cpp_options,"");

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
      /* leaving this in just in case-- any option starting with '-'
         will get passed on to cpp anyway */
      if(strcasecmp(argv[i],"-cppopt") == 0)
	{
	  strcat(cpp_options, argv[++i]);
	  strcat(cpp_options, " ");
	}
      else if (strcasecmp(argv[i], "-cppprog") == 0)
	{
	  cpp_prog = argv[++i];
	}
      else if(strcasecmp(argv[i], "-outfile") == 0)
      {
        strcpy(cpp_outfile,argv[++i]);
      }
      else if(strcasecmp(argv[i], "-debug") == 0)
	{
	  cpp_debug = 1;
	}
      else if (strncasecmp(argv[i],"-",1) == 0)
      {
        /* pass on any other arguments starting with '-' to cpp */
        strcat(cpp_options, argv[i]);
        strcat(cpp_options, " ");
      }
      else
	filename = argv[i];
    }

  /* If we don't have a cpp, stop right now.  */
  if (!cpp_prog || cpp_prog[0] == '\0') {
    fprintf(stderr, "%s: no C preprocessor program specified\n", MyName);
    exit(1);
  }

  for(i=0;i<strlen(filename);i++)
    if((filename[i] == '\n')||(filename[i] == '\r'))
      {
	filename[i] = 0;
      }

  if (!(dpy = XOpenDisplay(display_name)))
    {
      fprintf(stderr,"%s: can't open display %s",
	      MyName, XDisplayName(display_name));
      exit (1);
    }

  tmp_file = cpp_defs(dpy, display_name,cpp_options, filename);

  sprintf(read_string,"read %s\n",tmp_file);
  SendInfo(fd,read_string,0);

  /* For a debugging version, we may wish to omit this part. */
  /* I'll let some cpp advocates clean this up */
  if(!cpp_debug)
    {
      sprintf(delete_string,"exec rm %s\n",tmp_file);
      SendInfo(fd,delete_string,0);
    }
  return 0;
}



static char *cpp_defs(Display *display, const char *host, char *cpp_options, char *config_file)
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
  struct passwd *pwent;
  int fd;
  /* Generate a temporary filename.  Honor the TMPDIR environment variable,
     if set. Hope nobody deletes this file! */

  if (strlen(cpp_outfile) == 0) {
    if ((vc=getenv("TMPDIR"))) {
      strcpy(tmp_name, vc);
    } else {
      strcpy(tmp_name, "/tmp");
    }
    strcat(tmp_name, "/fvwmrcXXXXXX");
    mktemp(tmp_name);
  } else {
    strcpy(tmp_name,cpp_outfile);
  }

  if (*tmp_name == '\0')
    {
      perror("mktemp failed in cpp_defs");
      exit(0377);
    }

  /*
  ** check to make sure it doesn't exist already, to prevent security hole
  */
  if ((fd = open(tmp_name, O_WRONLY|O_EXCL|O_CREAT, 0600)) < 0)
  {
    perror("exclusive open for output file failed in cpp_defs");
    exit(0377);
  }
  close(fd);

    /*
     * Create the appropriate command line to run cpp, and
     * open a pipe to the command.
     */

  sprintf(options, "%s %s >%s\n",
	  cpp_prog,
	  cpp_options, tmp_name);
  tmpf = popen(options, "w");
  if (tmpf == NULL) {
    perror("Cannot open pipe to cpp");
    exit(0377);
  }

  gethostname(client,MAXHOSTNAME);

  getostype  (ostype, sizeof ostype);

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

    strcat(options, "Cpp ");

#ifdef	NO_SAVEUNDERS
    strcat(options, "NO_SAVEUNDERS ");
#endif

    fputs(MkDef("OPTIONS", options), tmpf);

    fputs(MkDef("FVWM_MODULEDIR", FVWM_MODULEDIR), tmpf);
    fputs(MkDef("FVWM_CONFIGDIR", FVWM_CONFIGDIR), tmpf);

    /*
     * At this point, we've sent the definitions to cpp.  Just include
     * the fvwmrc file now.
     */

    fprintf(tmpf, "#include \"%s\"\n", config_file);

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
  char *cp = NULL;
  int n;

  /* Get space to hold everything, if needed */

  n = EXTRA + strlen(name) + strlen(def);
  cp = safemalloc(n);

  sprintf(cp, "#define %s %s\n",name,def);

  return(cp);
}

static char *MkNum(char *name,int def)
{
  char num[20];

  sprintf(num, "%d", def);

  return(MkDef(name, num));
}
