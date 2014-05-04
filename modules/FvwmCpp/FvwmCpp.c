/* -*-c-*- */
/* This module, and the entire FvwmCpp program, and the concept for
 * interfacing this module to the Window Manager, are all original work
 * by Robert Nation
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

#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "libs/ftime.h"
#include <unistd.h>
#include <ctype.h>
#include <pwd.h>
#include <sys/param.h>
#include <sys/types.h>
#include <netdb.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>

#include "libs/Module.h"
#include "libs/fvwm_sys_stat.h"

#include "FvwmCpp.h"
#include "libs/fvwmlib.h"
#include "libs/FShape.h"
#include "libs/PictureBase.h"
#include "libs/FSMlib.h"
#include "libs/Strings.h"
#include "libs/System.h"
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#define Resolution(pixels, mm) ((((pixels) * 2000 / (mm)) + 1) / 2)

char *MyName;
int fd[2];

long Vx, Vy;
static char *MkDef(char *name, char *def);
static char *MkNum(char *name,int def);
static char *cpp_defs(Display *display, const char *host, char *m4_options, char *config_file);
#define MAXHOSTNAME 255
#define EXTRA 20

char *cpp_prog = FVWM_CPP;          /* Name of the cpp program */

char cpp_options[BUFSIZ];
char cpp_outfile[BUFSIZ]="";


/*
 *
 *  Procedure:
 *      main - start of module
 *
 */
int main(int argc, char **argv)
{
  Display *dpy;                 /* which display are we talking to */
  char *temp, *s;
  char *display_name = NULL;
  char *filename = NULL;
  char *tmp_file;
  int i;
  int debug = 0;
  int lock = 0;
  int noread = 0;
  char *user_dir;

  /* Figure out the working directory and go to it */
  user_dir = getenv("FVWM_USERDIR");
  if (user_dir != NULL)
  {
    if (chdir(user_dir) < 0)
      fprintf(stderr, "%s: <<Warning>> chdir to %s failed in cpp_defs",
	      MyName, user_dir);
  }

  sprintf(cpp_options, "-I'%s' ", FVWM_DATADIR);

  /* Record the program name for error messages */
  temp = argv[0];

  s=strrchr(argv[0], '/');
  if (s != NULL)
    temp = s + 1;

  MyName = xmalloc(strlen(temp) + 2);
  strcpy(MyName,"*");
  strcat(MyName, temp);

  if(argc < 6)
    {
      fprintf(stderr,"%s Version %s should only be executed by fvwm!\n",MyName,
	      VERSION);
      exit(1);
    }

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
	  debug = 1;
	}
      else if(strcasecmp(argv[i], "-lock") == 0)
	{
	  lock = 1;
	}
      else if(strcasecmp(argv[i], "-noread") == 0)
	{
	  noread = 1;
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

  if (!filename)
  {
    fprintf(stderr, "%s: no file specified.\n", MyName);
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

  /* set up G */
  PictureInitCMap(dpy);

  /* tell fvwm we're running if -lock is not used */
  if (!lock)
    SendFinishedStartupNotification(fd);

  tmp_file = cpp_defs(dpy, display_name,cpp_options, filename);

  if (!noread)
  {
    char *read_string = CatString3("Read '", tmp_file, "'");
    SendText(fd, read_string, 0);
  }

  /* tell fvwm to continue if -lock is used */
  if (lock)
    SendFinishedStartupNotification(fd);

  /* For a debugging version, we may wish to omit this part. */
  /* I'll let some cpp advocates clean this up */
  if (!debug)
  {
    char *delete_string;
    char *delete_file = tmp_file;
    if (tmp_file[0] != '/' && user_dir != NULL)
    {
      delete_file = xstrdup(CatString3(user_dir, "/", tmp_file));
    }
    delete_string = CatString3("Exec exec /bin/rm '", delete_file, "'");
    SendText(fd, delete_string, 0);
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
	char *vc;                     /* Visual Class */
	FILE *tmpf;
	struct passwd *pwent;
	int fd;
	int Mscreen;

	/* Generate a temporary filename.  Honor the TMPDIR environment variable,
	   if set. Hope nobody deletes this file! */

	if (strlen(cpp_outfile) == 0)
	{
		if ((vc=getenv("TMPDIR")))
		{
			strcpy(tmp_name, vc);
		}
		else
		{
			strcpy(tmp_name, "/tmp");
		}
		strcat(tmp_name, "/fvwmrcXXXXXX");
		fd = fvwm_mkstemp(tmp_name);
		if (fd == -1)
		{
			fprintf(
				stderr,
				"[FvwmCpp][cpp_def] fvwm_mkstemp failed %s\n",
				tmp_name);
			exit(0377);
		}
	}
	else
	{
		strcpy(tmp_name,cpp_outfile);
		/*
		 * check to make sure it doesn't exist already, to prevent
		 * security hole
		 */
		/* first try to unlink it */
		unlink(tmp_name);
		fd = open(
			tmp_name, O_WRONLY|O_EXCL|O_CREAT,
			FVWM_S_IRUSR | FVWM_S_IWUSR);
		if (fd < 0)
		{
			fprintf(
				stderr,
				"[FvwmCpp][cpp_defs] error opening file %s\n",
				tmp_name);
			exit(0377);
		}
	}
	close(fd);

	/*
	 * Create the appropriate command line to run cpp, and
	 * open a pipe to the command.
	 */

	sprintf(options, "%s %s >%s\n", cpp_prog, cpp_options, tmp_name);
	tmpf = popen(options, "w");
	if (tmpf == NULL)
	{
		fprintf(
			stderr,
			"[FvwmCpp][cpp_defs] Cannot open pipe to cpp\n");
		exit(0377);
	}

	gethostname(client,MAXHOSTNAME);

	getostype  (ostype, sizeof ostype);

	hostname = gethostbyname(client);
	strcpy(server, XDisplayName(host));
	colon = strchr(server, ':');
	if (colon != NULL) *colon = '\0';
	if ((server[0] == '\0') || (!strcmp(server, "unix")))
		strcpy(server, client); /* must be connected to :0 or unix:0 */

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

	Mscreen= DefaultScreen(display);
	fputs(MkNum("SCREEN", Mscreen), tmpf);

	fputs(MkNum("WIDTH", DisplayWidth(display,Mscreen)), tmpf);
	fputs(MkNum("HEIGHT", DisplayHeight(display,Mscreen)), tmpf);

	screen = ScreenOfDisplay(display, Mscreen);
	fputs(MkNum("X_RESOLUTION",Resolution(screen->width,screen->mwidth)),
	      tmpf);
	fputs(MkNum("Y_RESOLUTION",Resolution(screen->height,screen->mheight)),
	      tmpf);
	fputs(MkNum("PLANES",DisplayPlanes(display, Mscreen)), tmpf);

	visual = DefaultVisualOfScreen(screen);
	fputs(MkNum("BITS_PER_RGB", visual->bits_per_rgb), tmpf);

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

	switch(Pvisual->class)
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
	fputs(MkDef("FVWM_CLASS", vc), tmpf);

	if (visual->class != StaticGray && visual->class != GrayScale)
		fputs(MkDef("COLOR", "Yes"), tmpf);
	else
		fputs(MkDef("COLOR", "No"), tmpf);

	if (Pvisual->class != StaticGray && Pvisual->class != GrayScale)
		fputs(MkDef("FVWM_COLOR", "Yes"), tmpf);
	else
		fputs(MkDef("FVWM_COLOR", "No"), tmpf);

	fputs(MkDef("FVWM_VERSION", VERSION), tmpf);

	/* Add options together */
	*options = '\0';
	if (FHaveShapeExtension)
		strcat(options, "SHAPE ");

	if (XpmSupport)
		strcat(options, "XPM ");

	strcat(options, "Cpp ");

	fputs(MkDef("OPTIONS", options), tmpf);

	fputs(MkDef("FVWM_MODULEDIR", FVWM_MODULEDIR), tmpf);
	fputs(MkDef("FVWM_DATADIR", FVWM_DATADIR), tmpf);

	if ((vc = getenv("FVWM_USERDIR")))
		fputs(MkDef("FVWM_USERDIR", vc), tmpf);

	if (SessionSupport && (vc = getenv("SESSION_MANAGER")))
		fputs(MkDef("SESSION_MANAGER", vc), tmpf);


	/*
	 * At this point, we've sent the definitions to cpp.  Just include
	 * the fvwmrc file now.
	 */

	fprintf(tmpf, "#include \"%s\"\n", config_file);

	pclose(tmpf);
	return(tmp_name);
}





/*
 *
 *  Procedure:
 *      SIGPIPE handler - SIGPIPE means fvwm is dying
 *
 */
RETSIGTYPE DeadPipe(int nonsense)
{
  exit(0);
  SIGNAL_RETURN;
}

static char *MkDef(char *name, char *def)
{
  char *cp = NULL;
  int n;

  /* Get space to hold everything, if needed */

  n = EXTRA + strlen(name) + strlen(def);
  cp = xmalloc(n);

  sprintf(cp, "#define %s %s\n",name,def);

  return(cp);
}

static char *MkNum(char *name,int def)
{
  char num[20];

  sprintf(num, "%d", def);

  return(MkDef(name, num));
}
