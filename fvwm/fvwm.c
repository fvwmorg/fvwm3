/* -*-c-*- */
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

/* ---------------------------- included header files ----------------------- */

#include <config.h>

#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_GETPWUID
#  include <pwd.h>
#endif
#if HAVE_SYS_SYSTEMINFO_H
/* Solaris has sysinfo instead of gethostname.  */
#include <sys/systeminfo.h>
#endif

#include <X11/Xproto.h>

#include "libs/fvwmlib.h"
#include "libs/FScreen.h"
#include "libs/FShape.h"
#include "libs/PictureBase.h"
#include "libs/PictureUtils.h"
#include "libs/FRenderInit.h"
#include "fvwm.h"
#include "externs.h"
#include "cursor.h"
#include "functions.h"
#include "misc.h"
#include "screen.h"
#include "builtins.h"
#include "module_interface.h"
#include "colorset.h"
#include "events.h"
#include "eventhandler.h"
#include "eventmask.h"
#include "icccm2.h"
#include "gnome.h"
#include "ewmh.h"
#include "add_window.h"
#include "fvwmsignal.h"
#include "stack.h"
#include "virtual.h"
#include "session.h"
#include "read.h"
#include "focus.h"
#include "update.h"
#include "move_resize.h"
#include "frame.h"
#include "menus.h"

/* ---------------------------- local definitions --------------------------- */

#define MAXHOSTNAME 255
#define MAX_CFG_CMDS 10
#define MAX_ARG_SIZE 25

#define g_width 2
#define g_height 2
#define l_g_width 4
#define l_g_height 2
#define s_g_width 4
#define s_g_height 4

/* ---------------------------- local macros -------------------------------- */

/* ---------------------------- imports ------------------------------------- */

extern int last_event_type;

/* ---------------------------- included code files ------------------------- */

/* ---------------------------- local types --------------------------------- */

typedef enum
{
	FVWM_RUNNING = 0,
	FVWM_DONE,
	FVWM_RESTART
} fvwm_run_state_t;

/* ---------------------------- forward declarations ------------------------ */

/* ---------------------------- local variables ----------------------------- */

static char *config_commands[MAX_CFG_CMDS];
static int num_config_commands=0;

/* assorted gray bitmaps for decorative borders */
static char g_bits[] =
{0x02, 0x01};
static char l_g_bits[] =
{0x08, 0x02};
static char s_g_bits[] =
{0x01, 0x02, 0x04, 0x08};

static char *home_dir;

static volatile sig_atomic_t fvwmRunState = FVWM_RUNNING;

static const char *initFunctionNames[4] =
{
	"InitFunction",
	"RestartFunction",
	"ExitFunction",
	"Nop"
};

/* ---------------------------- exported variables (globals) ---------------- */

int master_pid;                 /* process number of 1st fvwm process */

ScreenInfo Scr;                 /* structures for the screen */
Display *dpy = NULL;            /* which display are we talking to */

Bool fFvwmInStartup = True;     /* Set to False when startup has finished */
Bool DoingCommandLine = False;  /* Set True before each cmd line arg */

XContext FvwmContext;           /* context for fvwm windows */
XContext MenuContext;           /* context for fvwm menus */

int JunkX = 0, JunkY = 0;
Window JunkRoot, JunkChild;             /* junk window */
unsigned int JunkWidth, JunkHeight, JunkBW, JunkDepth, JunkMask;

Bool debugging = False;
Bool debugging_stack_ring = False;

Window bad_window = None;

char **g_argv;
int g_argc;

char *state_filename = NULL;
char *restart_state_filename = NULL;  /* $HOME/.fs-restart */

Bool Restarting = False;
int fd_width, x_fd;
char *display_name = NULL;
char *fvwm_userdir;
char const *Fvwm_VersionInfo;
char const *Fvwm_LicenseInfo;
char const *Fvwm_SupportInfo;

Atom _XA_MIT_PRIORITY_COLORS;
Atom _XA_WM_CHANGE_STATE;
Atom _XA_WM_STATE;
Atom _XA_WM_COLORMAP_WINDOWS;
Atom _XA_WM_TAKE_FOCUS;
Atom _XA_WM_DELETE_WINDOW;
Atom _XA_WM_DESKTOP;
Atom _XA_MwmAtom;
Atom _XA_MOTIF_WM;

Atom _XA_OL_WIN_ATTR;
Atom _XA_OL_WT_BASE;
Atom _XA_OL_WT_CMD;
Atom _XA_OL_WT_HELP;
Atom _XA_OL_WT_NOTICE;
Atom _XA_OL_WT_OTHER;
Atom _XA_OL_DECOR_ADD;
Atom _XA_OL_DECOR_DEL;
Atom _XA_OL_DECOR_CLOSE;
Atom _XA_OL_DECOR_RESIZE;
Atom _XA_OL_DECOR_HEADER;
Atom _XA_OL_DECOR_ICON_NAME;

Atom _XA_WM_WINDOW_ROLE;
Atom _XA_WM_CLIENT_LEADER;
Atom _XA_SM_CLIENT_ID;

Atom _XA_XROOTPMAP_ID;
Atom _XA_XSETROOT_ID;

/* ---------------------------- local functions ----------------------------- */

static void SaveDesktopState()
{
	FvwmWindow *t;
	unsigned long data[1];

	for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
	{
		data[0] = (unsigned long) t->Desk;
		XChangeProperty(
			dpy, FW_W(t), _XA_WM_DESKTOP, _XA_WM_DESKTOP, 32,
			PropModeReplace, (unsigned char *)data, 1);
	}
	data[0] = (unsigned long) Scr.CurrentDesk;
	XChangeProperty(
		dpy, Scr.Root, _XA_WM_DESKTOP, _XA_WM_DESKTOP, 32,
		PropModeReplace, (unsigned char *) data, 1);
	XFlush(dpy);

	return;
}

static void InternUsefulAtoms (void)
{
	/*
	 * Create priority colors if necessary.
	 */
	_XA_MIT_PRIORITY_COLORS = XInternAtom(
		dpy, "_MIT_PRIORITY_COLORS", False);
	_XA_WM_CHANGE_STATE = XInternAtom(dpy, "WM_CHANGE_STATE", False);
	_XA_WM_STATE = XInternAtom(dpy, "WM_STATE", False);
	_XA_WM_COLORMAP_WINDOWS = XInternAtom(
		dpy, "WM_COLORMAP_WINDOWS", False);
	_XA_WM_PROTOCOLS = XInternAtom(dpy, "WM_PROTOCOLS", False);
	_XA_WM_TAKE_FOCUS = XInternAtom(dpy, "WM_TAKE_FOCUS", False);
	_XA_WM_DELETE_WINDOW = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	_XA_WM_DESKTOP = XInternAtom(dpy, "WM_DESKTOP", False);
	_XA_MwmAtom=XInternAtom(dpy, "_MOTIF_WM_HINTS",False);
	_XA_MOTIF_WM=XInternAtom(dpy, "_MOTIF_WM_INFO",False);

	_XA_OL_WIN_ATTR=XInternAtom(dpy, "_OL_WIN_ATTR",False);
	_XA_OL_WT_BASE=XInternAtom(dpy, "_OL_WT_BASE",False);
	_XA_OL_WT_CMD=XInternAtom(dpy, "_OL_WT_CMD",False);
	_XA_OL_WT_HELP=XInternAtom(dpy, "_OL_WT_HELP",False);
	_XA_OL_WT_NOTICE=XInternAtom(dpy, "_OL_WT_NOTICE",False);
	_XA_OL_WT_OTHER=XInternAtom(dpy, "_OL_WT_OTHER",False);
	_XA_OL_DECOR_ADD=XInternAtom(dpy, "_OL_DECOR_ADD",False);
	_XA_OL_DECOR_DEL=XInternAtom(dpy, "_OL_DECOR_DEL",False);
	_XA_OL_DECOR_CLOSE=XInternAtom(dpy, "_OL_DECOR_CLOSE",False);
	_XA_OL_DECOR_RESIZE=XInternAtom(dpy, "_OL_DECOR_RESIZE",False);
	_XA_OL_DECOR_HEADER=XInternAtom(dpy, "_OL_DECOR_HEADER",False);
	_XA_OL_DECOR_ICON_NAME=XInternAtom(dpy, "_OL_DECOR_ICON_NAME",False);

	_XA_WM_WINDOW_ROLE=XInternAtom(dpy, "WM_WINDOW_ROLE",False);
	_XA_WM_CLIENT_LEADER=XInternAtom(dpy, "WM_CLIENT_LEADER",False);
	_XA_SM_CLIENT_ID=XInternAtom(dpy, "SM_CLIENT_ID",False);

	_XA_XROOTPMAP_ID=XInternAtom(dpy, "_XROOTPMAP_ID",False);
	_XA_XSETROOT_ID=XInternAtom(dpy, "_XSETROOT_ID",False);

	return;
}

/* exit handler that will try to release any grabs */
static void catch_exit(void)
{
	if (dpy != NULL)
	{
		/* Don't care if this is called from an X error handler. We
		 * *have* to try this, whatever happens. XFree 4.0 may freeze
		 * if we don't do this. */
		XUngrabServer(dpy);
		XUngrabPointer(dpy, CurrentTime);
		XUngrabKeyboard(dpy, CurrentTime);
	}

	return;
}

/*************************************************************************
 * Restart on a signal
 ************************************************************************/
static RETSIGTYPE
Restart(int sig)
{
	fvwmRunState = FVWM_RESTART;

	/* This function might not return - it could "long-jump"
	 * right out, so we need to do everything we need to do
	 * BEFORE we call it ... */
	fvwmSetTerminate(sig);

	return;
}

static RETSIGTYPE
SigDone(int sig)
{
	fvwmRunState = FVWM_DONE;

	/* This function might not return - it could "long-jump"
	 * right out, so we need to do everything we need to do
	 * BEFORE we call it ... */
	fvwmSetTerminate(sig);

	return;
}

/*
 * parseCommandArgs - parses a given command string into a given limited
 * argument array suitable for execv*. The parsing is similar to shell's.
 * Returns:
 *   positive number of parsed arguments - on success,
 *   0 - on empty command (only spaces),
 *   negative - on no command or parsing error.
 *
 * Any character can be quoted with a backslash (even inside single quotes).
 * Every command argument is separated by a space/tab/new-line from both sizes
 * or is at the start/end of the command. Sequential spaces are ignored.
 * An argument can be enclosed into single quotes (no further expanding)
 * or double quotes (expending environmental variables $VAR or ${VAR}).
 * The character '~' is expanded into user home directory (if not in quotes).
 *
 * In the current implementation, parsed arguments are stored in one
 * large static string pointed by returned argv[0], so they will be lost
 * on the next function call. This can be changed using dynamic allocation,
 * in this case the caller must free the string pointed by argv[0].
 */
static int parseCommandArgs(
	const char *command, char **argv, int maxArgc, const char **errorMsg)
{
	/* It is impossible to guess the exact length because of expanding */
#define MAX_TOTAL_ARG_LEN 256
	/* char *argString = safemalloc(MAX_TOTAL_ARG_LEN); */
	static char argString[MAX_TOTAL_ARG_LEN];
	int totalArgLen = 0;
	int errorCode = 0;
	int argc;
	char *aptr = argString;
	const char *cptr = command;

#define theChar (*cptr)
#define advChar (cptr++)
#define topChar (*cptr     == '\\'? *(cptr+1): *cptr)
#define popChar (*(cptr++) == '\\'? *(cptr++): *(cptr-1))
#define canAddArgChar (totalArgLen < MAX_TOTAL_ARG_LEN-1)
#define addArgChar(ch) (++totalArgLen, *(aptr++) = ch)
#define canAddArgStr(str) (totalArgLen < MAX_TOTAL_ARG_LEN-strlen(str))
#define addArgStr(str) \
{\
	const char *tmp = str;\
	while (*tmp)\
	{\
		addArgChar(*(tmp++));\
	}\
}

	*errorMsg = "";
	if (!command)
	{
		*errorMsg = "No command";
		return -1;
	}
	for (argc = 0; argc < maxArgc - 1; argc++)
	{
		int sQuote = 0;
		argv[argc] = aptr;
		while (isspace(theChar))
		{
			advChar;
		}
		if (theChar == '\0')
		{
			break;
		}
		while ((sQuote || !isspace(theChar)) &&
		       theChar != '\0' && canAddArgChar)
		{
			if (theChar == '"')
			{
				if (sQuote)
				{
					sQuote = 0;
				}
				else
				{
					sQuote = 1;
				}
				advChar;
			}
			else if (!sQuote && theChar == '\'')
			{
				advChar;
				while (theChar != '\'' && theChar != '\0' &&
				       canAddArgChar)
				{
					addArgChar(popChar);
				}
				if (theChar == '\'')
				{
					advChar;
				}
				else if (!canAddArgChar)
				{
					break;
				}
				else
				{
					*errorMsg = "No closing single quote";
					errorCode = -3;
					break;
				}
			}
			else if (!sQuote && theChar == '~')
			{
				if (!canAddArgStr(home_dir))
				{
					break;
				}
				addArgStr(home_dir);
				advChar;
			}
			else if (theChar == '$')
			{
				int beg, len;
				const char *str = getFirstEnv(cptr, &beg, &len);

				if (!str || beg)
				{
					addArgChar(theChar);
					advChar;
					continue;
				}
				if (!canAddArgStr(str))
				{
					break;
				}
				addArgStr(str);
				cptr += len;
			}
			else
			{
				if (addArgChar(popChar) == '\0')
				{
					break;
				}
			}
		}
		if (*(aptr-1) == '\0')
		{
			*errorMsg = "Unexpected last backslash";
			errorCode = -2;
			break;
		}
		if (errorCode)
		{
			break;
		}
		if (theChar == '~' || theChar == '$' || !canAddArgChar)
		{
			*errorMsg = "The command is too long";
			errorCode = -argc - 100;
			break;
		}
		if (sQuote)
		{
			*errorMsg = "No closing double quote";
			errorCode = -4;
			break;
		}
		addArgChar('\0');
	}
#undef theChar
#undef advChar
#undef topChar
#undef popChar
#undef canAddArgChar
#undef addArgChar
#undef canAddArgStr
#undef addArgStr
	argv[argc] = NULL;
	if (argc == 0 && !errorCode)
	{
		*errorMsg = "Void command";
	}

	return errorCode ? errorCode : argc;
}

/***********************************************************************
 *
 *  Procedure:
 *      Done - tells FVWM to clean up and exit
 *
 ***********************************************************************/
/* if restart is true, command must not be NULL... */
void Done(int restart, char *command)
{
	const char *exitFuncName;

	/* XFree freeze hack */
	XUngrabPointer(dpy, CurrentTime);
	XUngrabKeyboard(dpy, CurrentTime);
	XUngrabServer(dpy);
	if (!restart)
	{
		MoveViewport(0,0,False);
	}
	/* migo (03/Jul/1999): execute [Session]ExitFunction */
	exitFuncName = getInitFunctionName(2);
	if (functions_is_complex_function(exitFuncName))
	{
		const exec_context_t *exc;
		exec_context_changes_t ecc;

		char *action = safestrdup(
			CatString2("Function ", exitFuncName));
		ecc.type = EXCT_EXIT;
		ecc.w.wcontext = C_ROOT;
		exc = exc_create_context(&ecc, ECC_TYPE | ECC_WCONTEXT);
		execute_function(NULL, exc, action, 0);
		exc_destroy_context(exc);
		free(action);
	}
	if (!restart)
	{
		Reborder();
	}
	EWMH_ExitStuff();
	if (restart)
	{
		Bool doPreserveState = True;
		SaveDesktopState();

		if (command)
		{
			while (isspace(command[0]))
			{
				command++;
			}
			if (strncmp(command, "--dont-preserve-state", 21) == 0)
			{
				doPreserveState = False;
				command += 21;
				while (isspace(command[0])) command++;
			}
		}
		if (command[0] == '\0')
		{
			command = NULL; /* native restart */
		}

		/* won't return under SM on Restart without parameters */
		RestartInSession(
			restart_state_filename, command == NULL,
			doPreserveState);

		/* RBW - 06/08/1999 - without this, windows will wander to
		 * other pages on a Restart/Recapture because Restart gets the
		 * window position information out of sync. There may be a
		 * better way to do this (i.e., adjust the Restart code), but
		 * this works for now. */
		MoveViewport(0,0,False);
		Reborder();

		/* Really make sure that the connection is closed and cleared!
		 */
		CloseICCCM2();
		catch_exit();
		XCloseDisplay(dpy);
		dpy = NULL;

		/* really need to destroy all windows, explicitly, not sleep,
		 * but this is adequate for now */
		sleep(1);

		if (command)
		{
			char *my_argv[MAX_ARG_SIZE];
			const char *errorMsg;
			int n = parseCommandArgs(
				command, my_argv, MAX_ARG_SIZE, &errorMsg);

			if (n <= 0)
			{
				fvwm_msg(
					ERR, "Done",
					"Restart command parsing error in"
					" (%s): [%s]", command, errorMsg);
			}
			else if (StrEquals(my_argv[0], "--pass-args"))
			{
				if (n != 2)
				{
					fvwm_msg(
						ERR, "Done",
						"Restart --pass-args: single"
						" name expected. (restarting"
						" '%s' instead)", g_argv[0]);

				}
				else
				{
					int i;
					my_argv[0] = my_argv[1];
					for (i = 1; i < g_argc &&
						     i < MAX_ARG_SIZE - 1; i++)
					{
						my_argv[i] = g_argv[i];
					}
					my_argv[i] = NULL;

					execvp(my_argv[0], my_argv);
					fvwm_msg(
						ERR, "Done",
						"Call of '%s' failed!"
						" (restarting '%s' instead)",
						my_argv[0], g_argv[0]);
					perror("  system error description");
				}

			}
			else
			{
				char *str = NULL;

				/* Warn against an old 'Restart fvwm2' usage */
				if (n == 1 && strcmp(my_argv[0], "fvwm2") == 0)
				{
					str = "fvwm2";
				}
				/* If we are at it, warn against a 'Restart
				 * fvwm' usage as well */
				else if (n == 1 &&
					 strcmp(my_argv[0], "fvwm") == 0)
				{
					str = "fvwm";
				}
				if (str)
				{
					fvwm_msg(
						WARN, "Done",
						"`Restart %s' might not do"
						" what you want, see the man"
						" page.\n\tUse Restart without"
						" parameters if you mean to"
						" restart the same WM.", str);
				}
				execvp(my_argv[0], my_argv);
				fvwm_msg(
					ERR, "Done", "Call of '%s' failed!"
					" (restarting '%s' instead)",
					my_argv[0], g_argv[0]);
				perror("  system error description");
			}
		}

		execvp(g_argv[0], g_argv);    /* that _should_ work */
		fvwm_msg(ERR, "Done", "Call of '%s' failed!", g_argv[0]);
		perror("  system error description");
	}
	else
	{
		CloseICCCM2();
		catch_exit();
		XCloseDisplay(dpy);
		dpy = NULL;
	}

	/* dv (15-Jan-2000): This must be done after calling CloseICCCM2()!
	 * Otherwise fvwm ignores map requests while it still has
	 * SubstructureRedirect selected on the root window ==> windows end up
	 * in nirvana. This explicitly happened with windows unswallowed by
	 * FvwmButtons. */
	ClosePipes();

	exit(0);
}

/*************************************************************************
 * Reap child processes, preventing them from becoming zombies.
 * We do this asynchronously within the SIGCHLD handler so that
 * "it just happens".
 ************************************************************************/
static RETSIGTYPE
ReapChildren(int sig)
{
	(void)sig;

	BSD_BLOCK_SIGNALS;
	/*
	 * This is a signal handler, AND SO MUST BE REENTRANT!
	 * Now the wait() functions are safe here, but please don't
	 * add anything unless you're SURE that the new functions
	 * (plus EVERYTHING they call) are also reentrant. There
	 * are very few functions which are truly safe.
	 */
#if HAVE_WAITPID
	while (waitpid(-1, NULL, WNOHANG) > 0)
	{
		/* nothing to do here */
	}
#elif HAVE_WAIT3
	while (wait3(NULL, WNOHANG, NULL) > 0)
	{
		/* nothing to do here */
	}
#else
# error One of waitpid or wait3 is needed.
#endif
	BSD_UNBLOCK_SIGNALS;

	return;
}

/***********************************************************************
 *
 *  Procedure:
 *      InstallSignals: install the signal handlers, using whatever
 *         means we have at our disposal. The more POSIXy, the better
 *
 ************************************************************************/
static void
InstallSignals(void)
{
#ifdef HAVE_SIGACTION
	struct sigaction  sigact;

	/*
	 * All signals whose handlers call fvwmSetTerminate()
	 * must be mutually exclusive - we mustn't receive one
	 * while processing any of the others ...
	 */
	sigemptyset(&sigact.sa_mask);
	sigaddset(&sigact.sa_mask, SIGINT);
	sigaddset(&sigact.sa_mask, SIGHUP);
	sigaddset(&sigact.sa_mask, SIGQUIT);
	sigaddset(&sigact.sa_mask, SIGTERM);
	sigaddset(&sigact.sa_mask, SIGUSR1);

#ifdef SA_RESTART
	sigact.sa_flags = SA_RESTART;
#else
	sigact.sa_flags = 0;
#endif
	sigact.sa_handler = DeadPipe;
	sigaction(SIGPIPE, &sigact, NULL);

	sigact.sa_handler = Restart;
	sigaction(SIGUSR1, &sigact, NULL);

	sigact.sa_handler = SigDone;
	sigaction(SIGINT,  &sigact, NULL);
	sigaction(SIGHUP,  &sigact, NULL);
	sigaction(SIGQUIT, &sigact, NULL);
	sigaction(SIGTERM, &sigact, NULL);

	/*
	 * Reap all zombies automatically! This signal handler will
	 * only be called if a child process dies, not if someone
	 * sends a child a STOP signal. Note that none of our "terminate"
	 * signals can be delivered until the SIGCHLD handler completes,
	 * and this is a Good Thing because the terminate handlers
	 * might exit abruptly via "siglongjmp". This could potentially
	 * leave SIGCHLD handler with unfinished business ...
	 *
	 * NOTE: We could still receive SIGPIPE signals within the
	 *       SIGCHLD handler, but the SIGPIPE handler has the
	 *       SA_RESTART flag set and so should not affect our
	 *       "wait" system call.
	 */
	sigact.sa_flags |= SA_NOCLDSTOP;
	sigact.sa_handler = ReapChildren;
	sigaction(SIGCHLD, &sigact, NULL);
#else
#ifdef USE_BSD_SIGNALS
	fvwmSetSignalMask( sigmask(SIGUSR1) |
			   sigmask(SIGINT)  |
			   sigmask(SIGHUP)  |
			   sigmask(SIGQUIT) |
			   sigmask(SIGTERM) );
#endif

	/*
	 * We don't have sigaction(), so fall back on
	 * less reliable methods ...
	 */
	signal(SIGPIPE, DeadPipe);
	signal(SIGUSR1, Restart);
#ifdef HAVE_SIGINTERRUPT
	siginterrupt(SIGUSR1, 0);
#endif

	signal(SIGINT, SigDone);
#ifdef HAVE_SIGINTERRUPT
	siginterrupt(SIGINT, 0);
#endif
	signal(SIGHUP, SigDone);
#ifdef HAVE_SIGINTERRUPT
	siginterrupt(SIGHUP, 0);
#endif
	signal(SIGQUIT, SigDone);
#ifdef HAVE_SIGINTERRUPT
	siginterrupt(SIGQUIT, 0);
#endif
	signal(SIGTERM, SigDone);
#ifdef HAVE_SIGINTERRUPT
	siginterrupt(SIGTERM, 0);
#endif
	signal(SIGCHLD, ReapChildren);
#ifdef HAVE_SIGINTERRUPT
	siginterrupt(SIGCHLD, 0);
#endif
#endif

	/* When FVWM restarts, the SIGCHLD handler is automatically reset
	 * to the default handler. This means that Zombies left over from
	 * the previous instance of FVWM could still be roaming the process
	 * table if they exited while the default handler was in place.
	 * We fix this by invoking the SIGCHLD handler NOW, so that they
	 * may finally rest in peace. */
	ReapChildren(0);

	return;
}

/***********************************************************************
 *
 *  LoadDefaultLeftButton -- loads default left button # into
 *              assumes associated button memory is already free
 *
 ************************************************************************/
static void LoadDefaultLeftButton(DecorFace *df, int i)
{
	struct vector_coords *v = &df->u.vector;
	int j = 0;

	memset(&df->style, 0, sizeof(df->style));
	DFS_FACE_TYPE(df->style) = DefaultVectorButton;
	switch (i % 5)
	{
	case 0:
	case 4:
		v->num = 5;
		v->x = safemalloc(sizeof(char) * v->num);
		v->y = safemalloc(sizeof(char) * v->num);
		v->xoff = safemalloc(sizeof(char) * v->num);
		v->yoff = safemalloc(sizeof(char) * v->num);
		v->c = safecalloc(v->num, sizeof(char));
		v->x[0] = 22;
		v->y[0] = 39;
		v->c[0] = 1;
		v->x[1] = 78;
		v->y[1] = 39;
		v->c[1] = 1;
		v->x[2] = 78;
		v->y[2] = 61;
		v->x[3] = 22;
		v->y[3] = 61;
		v->x[4] = 22;
		v->y[4] = 39;
		v->c[4] = 1;
		break;
	case 1:
		v->num = 5;
		v->x = safemalloc(sizeof(char) * v->num);
		v->y = safemalloc(sizeof(char) * v->num);
		v->xoff = safemalloc(sizeof(char) * v->num);
		v->yoff = safemalloc(sizeof(char) * v->num);
		v->c = safecalloc(v->num, sizeof(char));
		v->x[0] = 32;
		v->y[0] = 45;
		v->x[1] = 68;
		v->y[1] = 45;
		v->x[2] = 68;
		v->y[2] = 55;
		v->c[2] = 1;
		v->x[3] = 32;
		v->y[3] = 55;
		v->c[3] = 1;
		v->x[4] = 32;
		v->y[4] = 45;
		break;
	case 2:
		v->num = 5;
		v->x = safemalloc(sizeof(char) * v->num);
		v->y = safemalloc(sizeof(char) * v->num);
		v->xoff = safemalloc(sizeof(char) * v->num);
		v->yoff = safemalloc(sizeof(char) * v->num);
		v->c = safecalloc(v->num, sizeof(char));
		v->x[0] = 49;
		v->y[0] = 49;
		v->c[0] = 1;
		v->x[1] = 51;
		v->y[1] = 49;
		v->c[1] = 1;
		v->x[2] = 51;
		v->y[2] = 51;
		v->x[3] = 49;
		v->y[3] = 51;
		v->x[4] = 49;
		v->y[4] = 49;
		v->c[4] = 1;
		break;
	case 3:
		v->num = 5;
		v->x = safemalloc(sizeof(char) * v->num);
		v->y = safemalloc(sizeof(char) * v->num);
		v->xoff = safemalloc(sizeof(char) * v->num);
		v->yoff = safemalloc(sizeof(char) * v->num);
		v->c = safecalloc(v->num, sizeof(char));
		v->x[0] = 32;
		v->y[0] = 45;
		v->c[0] = 1;
		v->x[1] = 68;
		v->y[1] = 45;
		v->c[1] = 1;
		v->x[2] = 68;
		v->y[2] = 55;
		v->x[3] = 32;
		v->y[3] = 55;
		v->x[4] = 32;
		v->y[4] = 45;
		v->c[4] = 1;
		break;
	}
	/* set offsets to 0, for all buttons */
	for(j = 0 ; j < v->num ; j++)
	{
	  v->xoff[j] = 0;
	  v->yoff[j] = 0;
	}

	return;
}

/***********************************************************************
 *
 *  LoadDefaultRightButton -- loads default left button # into
 *              assumes associated button memory is already free
 *
 ************************************************************************/
static void LoadDefaultRightButton(DecorFace *df, int i)
{
	struct vector_coords *v = &df->u.vector;
	int j = 0;

	memset(&df->style, 0, sizeof(df->style));
	DFS_FACE_TYPE(df->style) = DefaultVectorButton;
	switch (i % 5)
	{
	case 0:
	case 3:
		v->num = 5;
		v->x = safemalloc(sizeof(char) * v->num);
		v->y = safemalloc(sizeof(char) * v->num);
		v->xoff = safemalloc(sizeof(char) * v->num);
		v->yoff = safemalloc(sizeof(char) * v->num);
		v->c = safecalloc(v->num, sizeof(char));
		v->x[0] = 25;
		v->y[0] = 25;
		v->c[0] = 1;
		v->x[1] = 75;
		v->y[1] = 25;
		v->c[1] = 1;
		v->x[2] = 75;
		v->y[2] = 75;
		v->x[3] = 25;
		v->y[3] = 75;
		v->x[4] = 25;
		v->y[4] = 25;
		v->c[4] = 1;
		break;
	case 1:
		v->num = 5;
		v->x = safemalloc(sizeof(char) * v->num);
		v->y = safemalloc(sizeof(char) * v->num);
		v->xoff = safemalloc(sizeof(char) * v->num);
		v->yoff = safemalloc(sizeof(char) * v->num);
		v->c = safecalloc(v->num, sizeof(char));
		v->x[0] = 39;
		v->y[0] = 39;
		v->c[0] = 1;
		v->x[1] = 61;
		v->y[1] = 39;
		v->c[1] = 1;
		v->x[2] = 61;
		v->y[2] = 61;
		v->x[3] = 39;
		v->y[3] = 61;
		v->x[4] = 39;
		v->y[4] = 39;
		v->c[4] = 1;
		break;
	case 2:
		v->num = 5;
		v->x = safemalloc(sizeof(char) * v->num);
		v->y = safemalloc(sizeof(char) * v->num);
		v->xoff = safemalloc(sizeof(char) * v->num);
		v->yoff = safemalloc(sizeof(char) * v->num);
		v->c = safecalloc(v->num, sizeof(char));
		v->x[0] = 49;
		v->y[0] = 49;
		v->c[0] = 1;
		v->x[1] = 51;
		v->y[1] = 49;
		v->c[1] = 1;
		v->x[2] = 51;
		v->y[2] = 51;
		v->x[3] = 49;
		v->y[3] = 51;
		v->x[4] = 49;
		v->y[4] = 49;
		v->c[4] = 1;
		break;
	case 4:
		v->num = 5;
		v->x = safemalloc(sizeof(char) * v->num);
		v->y = safemalloc(sizeof(char) * v->num);
		v->xoff = safemalloc(sizeof(char) * v->num);
		v->yoff = safemalloc(sizeof(char) * v->num);
		v->c = safecalloc(v->num, sizeof(char));
		v->x[0] = 36;
		v->y[0] = 36;
		v->c[0] = 1;
		v->x[1] = 64;
		v->y[1] = 36;
		v->c[1] = 1;
		v->x[2] = 64;
		v->y[2] = 64;
		v->x[3] = 36;
		v->y[3] = 64;
		v->x[4] = 36;
		v->y[4] = 36;
		v->c[4] = 1;
		break;
	}
	/* set offsets to 0, for all buttons */
	for(j = 0 ; j < v->num ; j++)
	{
	  v->xoff[j] = 0;
	  v->yoff[j] = 0;
	}

	return;
}

/***********************************************************************
 *
 *  Procedure:
 *      CreateGCs - open fonts and create all the needed GC's.  I only
 *                  want to do this once, hence the first_time flag.
 *
 ***********************************************************************/
static void CreateGCs(void)
{
	XGCValues gcv;
	unsigned long gcm;
	XColor c;

	/* create scratch GC's */
	gcm = GCFunction|GCLineWidth;
	gcv.function = GXcopy;
	gcv.line_width = 0;

	Scr.ScratchGC1 = fvwmlib_XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);
	Scr.ScratchGC2 = fvwmlib_XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);
	Scr.ScratchGC3 = fvwmlib_XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);
	Scr.ScratchGC4 = fvwmlib_XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);
	Scr.TitleGC = fvwmlib_XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);
	Scr.TileGC = fvwmlib_XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);
	Scr.TransMaskGC = fvwmlib_XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);
	c.pixel = GetColor(DEFAULT_FORE_COLOR);
	Scr.ScratchMonoPixmap = XCreatePixmap(dpy, Scr.Root, 1, 1, 1);
	Scr.MonoGC = fvwmlib_XCreateGC(dpy, Scr.ScratchMonoPixmap, gcm, &gcv);

	return;
}

/***********************************************************************
 *
 *  Procedure:
 *      InitVariables - initialize fvwm variables
 *
 ************************************************************************/
static void InitVariables(void)
{
	FvwmContext = XUniqueContext();
	MenuContext = XUniqueContext();

	/* initialize some lists */
	Scr.AllBindings = NULL;
	Scr.functions = NULL;
	menus_init();
	Scr.last_added_item.type = ADDED_NONE;
	Scr.DefaultIcon = NULL;
	Scr.DefaultColorset = -1;
	Scr.StdGC = 0;
	Scr.StdReliefGC = 0;
	Scr.StdShadowGC = 0;
	Scr.XorGC = 0;
	/* zero all flags */
	memset(&Scr.flags, 0, sizeof(Scr.flags));
	/* create graphics contexts */
	CreateGCs();

	FW_W(&Scr.FvwmRoot) = Scr.Root;
	Scr.FvwmRoot.next = 0;
	init_stack_and_layers();
	Scr.root_pushes = 0;
	Scr.fvwm_pushes = 0;
	Scr.pushed_window = &Scr.FvwmRoot;
	Scr.FvwmRoot.number_cmap_windows = 0;
	Scr.FvwmRoot.attr_backup.colormap = Pcmap;

	Scr.MyDisplayWidth = DisplayWidth(dpy, Scr.screen);
	Scr.MyDisplayHeight = DisplayHeight(dpy, Scr.screen);
	Scr.BusyCursor = BUSY_NONE;
	Scr.Hilite = NULL;
	Scr.DefaultFont = NULL;
	Scr.VxMax = 2*Scr.MyDisplayWidth;
	Scr.VyMax = 2*Scr.MyDisplayHeight;
	Scr.Vx = 0;
	Scr.Vy = 0;
	Scr.SizeWindow = None;

	/* Sets the current desktop number to zero */
	/* Multiple desks are available even in non-virtual
	 * compilations */
	Scr.CurrentDesk = 0;
	Scr.EdgeScrollX = Scr.EdgeScrollY = DEFAULT_EDGE_SCROLL;
	Scr.ScrollResistance = DEFAULT_SCROLL_RESISTANCE;
	Scr.MoveResistance = DEFAULT_MOVE_RESISTANCE;
	Scr.XiMoveResistance = DEFAULT_XIMOVE_RESISTANCE;
	Scr.SnapAttraction = DEFAULT_SNAP_ATTRACTION;
	Scr.SnapMode = DEFAULT_SNAP_ATTRACTION_MODE;
	Scr.SnapGridX = DEFAULT_SNAP_GRID_X;
	Scr.SnapGridY = DEFAULT_SNAP_GRID_Y;
	Scr.OpaqueSize = DEFAULT_OPAQUE_MOVE_SIZE;
	Scr.MoveThreshold = DEFAULT_MOVE_THRESHOLD;
	/* ClickTime is set to the positive value upon entering the
	 * event loop. */
	Scr.ClickTime = -DEFAULT_CLICKTIME;
	Scr.ColormapFocus = COLORMAP_FOLLOWS_MOUSE;

	/* set major operating modes */
	Scr.NumBoxes = 0;
	Scr.cascade_x = 0;
	Scr.cascade_y = 0;
	/* the last Cascade placed window or NULL, we don't want NULL
	 * initially */
	Scr.cascade_window = &Scr.FvwmRoot;
	Scr.buttons2grab = 0;
	/* initialisation of the head of the desktops info */
	Scr.Desktops = (DesktopsInfo *)safemalloc(sizeof(DesktopsInfo));
	Scr.Desktops->name = NULL;
	Scr.Desktops->desk = 0; /* not desk 0 */
	Scr.Desktops->ewmh_dyn_working_area.x =
		Scr.Desktops->ewmh_working_area.x = 0;
	Scr.Desktops->ewmh_dyn_working_area.y =
		Scr.Desktops->ewmh_working_area.y = 0;
	Scr.Desktops->ewmh_dyn_working_area.width =
		Scr.Desktops->ewmh_working_area.width = Scr.MyDisplayWidth;
	Scr.Desktops->ewmh_dyn_working_area.height =
		Scr.Desktops->ewmh_working_area.height = Scr.MyDisplayHeight;
	Scr.Desktops->next = NULL;
	/* ewmh desktop */
	Scr.EwmhDesktop = NULL;
	InitFvwmDecor(&Scr.DefaultDecor);
#ifdef USEDECOR
	Scr.DefaultDecor.tag = "Default";
#endif
	/* Initialize RaiseHackNeeded by identifying X servers
	   possibly running under NT. This is probably not an
	   ideal solution, since eg NCD also produces X servers
	   which do not run under NT.

	   "Hummingbird Communications Ltd."
	   is the ServerVendor string of the Exceed X server under NT,

	   "Network Computing Devices Inc."
	   is the ServerVendor string of the PCXware X server under Windows.

	   "WRQ, Inc."
	   is the ServerVendor string of the Reflection X server under Windows.
	*/
	Scr.bo.RaiseHackNeeded =
		(strcmp (
			ServerVendor (dpy),
			"Hummingbird Communications Ltd.") == 0) ||
		(strcmp (
			ServerVendor (dpy),
			"Network Computing Devices Inc.") == 0) ||
		(strcmp (ServerVendor (dpy), "WRQ, Inc.") == 0);

	Scr.bo.ModalityIsEvil = 0;
	Scr.bo.DisableConfigureNotify = 0;
	Scr.bo.InstallRootCmap = 0;
	Scr.bo.FlickeringQtDialogsWorkaround = 1;
	Scr.bo.EWMHIconicStateWorkaround = 0;

	Scr.gs.EmulateMWM = DEFAULT_EMULATE_MWM;
	Scr.gs.EmulateWIN = DEFAULT_EMULATE_WIN;
	Scr.gs.use_active_down_buttons = DEFAULT_USE_ACTIVE_DOWN_BUTTONS;
	Scr.gs.use_inactive_buttons = DEFAULT_USE_INACTIVE_BUTTONS;
	Scr.gs.use_inactive_down_buttons = DEFAULT_USE_INACTIVE_DOWN_BUTTONS;
	/* Not the right place for this, should only be called once
	 * somewhere .. */

	/* EdgeCommands - no edge commands by default */
	Scr.PanFrameTop.command    = NULL;
	Scr.PanFrameBottom.command = NULL;
	Scr.PanFrameRight.command  = NULL;
	Scr.PanFrameLeft.command   = NULL;
	Scr.flags.is_pointer_on_this_screen = !!FQueryPointer(
		dpy, Scr.Root, &JunkRoot, &JunkChild, &JunkX, &JunkY, &JunkX,
		&JunkY, &JunkMask);

	/* make sure colorset 0 exists */
	alloc_colorset(0);

	return;
}

static void usage(void)
{
	fprintf(stderr, "\nfvwm %s Usage:\n\n", VERSION);
	fprintf(stderr, "  %s"
		" [-d dpy]"
		" [-debug]"
		" [-f config_file]"
		" [-cmd config_cmd]"
		" [-s]"
		" [--version]"
		" [-h]"
		" [-replace]"
		" [-clientId id]"
		" [-restore file]"
		" [-visualId id]"
		" [-visual class]"
		" [-color-limit]"
		"\n\n",g_argv[0]);
	exit(1);
}

static void setVersionInfo(void)
{
	char version_str[256];
	char license_str[512];
	char support_str[512] = "";
	int support_len;

	/* Set version information string */
	sprintf(version_str, "fvwm %s%s compiled on %s at %s",
		VERSION, VERSIONINFO, __DATE__, __TIME__);
	Fvwm_VersionInfo = safestrdup(version_str);

	sprintf(license_str,
		"fvwm comes with NO WARRANTY, to the extent permitted by law. "
		"You may\nredistribute copies of fvwm under "
		"the terms of the GNU General Public License.\n"
		"For more information about these matters, see the file "
		"named COPYING.");
	Fvwm_LicenseInfo = safestrdup(license_str);

#ifdef HAVE_READLINE
	strcat(support_str, " ReadLine,");
#endif
#ifdef HAVE_RPLAY
	strcat(support_str, " RPlay,");
#endif
#ifdef HAVE_STROKE
	strcat(support_str, " Stroke,");
#endif
#ifdef XPM
	strcat(support_str, " XPM,");
#endif
#ifdef HAVE_PNG
	strcat(support_str, " PNG,");
#endif
#ifdef GNOME
	strcat(support_str, " GNOME WM hints,");
#endif
#ifdef HAVE_EWMH
	strcat(support_str, " EWMH hints,");
#endif
	if (FHaveShapeExtension)
		strcat(support_str, " Shape,");
#ifdef SESSION
	strcat(support_str, " SM,");
#endif
#ifdef HAVE_BIDI
	strcat(support_str, " Bidi text,");
#endif
#ifdef HAVE_XINERAMA
	strcat(support_str, " Xinerama,");
#endif
#ifdef HAVE_XFT
	strcat(support_str, " XFT,");
#endif

	support_len = strlen(support_str);
	if (support_len > 0)
	{
		/* strip last comma */
		support_str[support_len - 1] = '\0';
		Fvwm_SupportInfo = safestrdup(
			CatString2("with support for:", support_str));
	}
	else
	{
		Fvwm_SupportInfo = "with no optional feature support";
	}

	return;
}

/* Sets some initial style values & such */
static void SetRCDefaults(void)
{
	int i;
	/* set up default colors, fonts, etc */
	char *defaults[] = {
		"XORValue 0",
		"DefaultFont",
		"DefaultColors black grey",
		DEFAULT_MENU_STYLE,
		"TitleStyle Centered -- Raised",
		"Style * Color lightgrey/dimgrey",
		"Style * HilightFore black, HilightBack grey",
		"AddToMenu MenuFvwmRoot \"Builtin Menu\" Title",
		"+ \"&1. XTerm\" Exec xterm",
		"+ \"&2. Setup Form\" Module FvwmForm FvwmForm-Setup",
		"+ \"&3. Setup 95 Script\" Module FvwmScript"
		" FvwmScript-Setup95",
		"+ \"&4. Issue FVWM commands\" Module FvwmConsole",
		"+ \"&R. Restart FVWM\" Restart",
		"+ \"&X. Exit FVWM\" Quit",
		"Mouse 0 R N Menu MenuFvwmRoot",
		"Read "FVWM_DATADIR"/ConfigFvwmDefaults",
		NULL
	};

	for (i = 0; defaults[i]; i++)
	{
		const exec_context_t *exc;
		exec_context_changes_t ecc;

		ecc.type = EXCT_INIT;
		ecc.w.wcontext = C_ROOT;
		exc = exc_create_context(&ecc, ECC_TYPE | ECC_WCONTEXT);
		execute_function(NULL, exc, defaults[i], 0);
		exc_destroy_context(exc);
	}

	return;
}

static int CatchRedirectError(Display *dpy, XErrorEvent *event)
{
	fvwm_msg(ERR, "CatchRedirectError", "another WM is running");
	exit(1);

	/* to make insure happy */
	return 0;
}

/* CatchFatal - Shuts down if the server connection is lost */
static int CatchFatal(Display *dpy)
{
	/* No action is taken because usually this action is caused by someone
	   using "xlogout" to be able to switch between multiple window managers
	*/
	ClosePipes();
	exit(1);

	/* to make insure happy */
	return 0;
}

/* FvwmErrorHandler - displays info on internal errors */
static int FvwmErrorHandler(Display *dpy, XErrorEvent *event)
{
	if (event->error_code == BadWindow)
	{
		bad_window = event->resourceid;
		return 0;
	}
	/* some errors are acceptable, mostly they're caused by
	 * trying to update a lost  window or free'ing another modules colors */
	if (event->error_code == BadWindow ||
	   event->request_code == X_GetGeometry ||
	   event->error_code == BadDrawable ||
	   event->request_code == X_ConfigureWindow ||
	   event->request_code == X_SetInputFocus||
	   event->request_code == X_GrabButton ||
	   event->request_code == X_ChangeWindowAttributes ||
	   event->request_code == X_InstallColormap ||
	   event->request_code == X_FreePixmap ||
	   event->request_code == X_FreeColors)
	{
		return 0;
	}
	fvwm_msg(ERR, "FvwmErrorHandler", "*** internal error ***");
	fvwm_msg(ERR, "FvwmErrorHandler", "Request %d, Error %d, EventType: %d",
		 event->request_code,
		 event->error_code,
		 last_event_type);

	return 0;
}

/* ---------------------------- interface functions ------------------------- */

/* Does initial window captures and runs init/restart function */
void StartupStuff(void)
{
#define startFuncName "StartFunction"
	const char *initFuncName;
	const exec_context_t *exc;
	exec_context_changes_t ecc;

	ecc.type = EXCT_INIT;
	ecc.w.wcontext = C_ROOT;
	exc = exc_create_context(&ecc, ECC_TYPE | ECC_W);
	CaptureAllWindows(exc, False);
	/* Turn off the SM stuff after the initial capture so that new windows
	 * will not be matched by accident. */
	if (Restarting)
	{
		DisableRestoringState();
	}
	/* Have to do this here too because preprocessor modules have not run
	 * to the end when HandleEvents is entered from the main loop. */
	checkPanFrames();

	fFvwmInStartup = False;

	/* Make sure the geometry window uses the current font */
	resize_geometry_window();

	/* Make sure we have the correct click time now. */
	if (Scr.ClickTime < 0)
	{
		Scr.ClickTime = -Scr.ClickTime;
	}

	/* It is safe to Ungrab here: if not and one of the init functions not
	 * finish we've got a complete freeze ! */
	UngrabEm(GRAB_STARTUP);
	XUngrabPointer(dpy, CurrentTime);

	/* migo (04-Sep-1999): execute StartFunction */
	if (functions_is_complex_function(startFuncName))
	{
		char *action = "Function " startFuncName;

		execute_function(NULL, exc, action, 0);
	}

	/* migo (03-Jul-1999): execute [Session]{Init|Restart}Function */
	initFuncName = getInitFunctionName(Restarting == True);
	if (functions_is_complex_function(initFuncName))
	{
		char *action = safestrdup(
			CatString2("Function ", initFuncName));

		execute_function(NULL, exc, action, 0);
		free(action);
	}

	/*
	  This should be done after the initialization is finished, since
	  it directly changes the global state.
	*/
	LoadGlobalState(state_filename);

	/*
	** migo (20-Jun-1999): Remove state file after usage.
	** migo (09-Jul-1999): but only on restart, otherwise it can be reused.
	*/
	if (Restarting)
	{
		unlink(state_filename);
	}
	exc_destroy_context(exc);

	return;
}

/***********************************************************************
 *
 *  LoadDefaultButton -- loads default button # into button structure
 *              assumes associated button memory is already free
 *
 ************************************************************************/
void LoadDefaultButton(DecorFace *df, int i)
{
	if (i & 1)
	{
		LoadDefaultRightButton(df, i / 2);
	}
	else
	{
		LoadDefaultLeftButton(df, i / 2);
	}

	return;
}

/***********************************************************************
 *
 *  ResetOrDestroyAllButtons -- resets all buttons to defaults
 *                              destroys existing buttons
 *
 ************************************************************************/
void DestroyAllButtons(FvwmDecor *decor)
{
	TitleButton *tbp;
	DecorFace *face;
	int i;
	int j;

	for (tbp = decor->buttons, i = 0; i < NUMBER_OF_BUTTONS; i++, tbp++)
	{
		for (j = 0, face = TB_STATE(*tbp); j < BS_MaxButtonState;
		     j++, face++)
		{
			FreeDecorFace(dpy, face);
		}
	}

	return;
}

void ResetAllButtons(FvwmDecor *decor)
{
	TitleButton *tbp;
	DecorFace *face;
	int i;
	int j;

	for (tbp = decor->buttons, i = 0; i < NUMBER_OF_BUTTONS; i++, tbp++)
	{
		memset(&TB_FLAGS(*tbp), 0, sizeof(TB_FLAGS(*tbp)));
		TB_JUSTIFICATION(*tbp) = JUST_CENTER;
		for (face = TB_STATE(*tbp), j = 0; j < BS_MaxButtonState;
		     j++, face++)
		{
			LoadDefaultButton(face, i);
		}
	}

	/* standard MWM decoration hint assignments (veliaa@rpi.edu)
	   [Menu]  - Title Bar - [Minimize] [Maximize] */
	TB_MWM_DECOR_FLAGS(decor->buttons[0]) |= MWM_DECOR_MENU;
	TB_MWM_DECOR_FLAGS(decor->buttons[1]) |= MWM_DECOR_MAXIMIZE;
	TB_MWM_DECOR_FLAGS(decor->buttons[3]) |= MWM_DECOR_MINIMIZE;

	return;
}

void SetMWM_INFO(Window window)
{
	struct mwminfo
	{
		long flags;
		Window win;
	}  motif_wm_info;
	static char set_yorn='n';

	if (set_yorn=='y')
	{
		return;
	}

	if (Scr.bo.ModalityIsEvil)
	{
		/* Set Motif WM_INFO atom to make motif relinquish
		 * broken handling of modal dialogs */
		motif_wm_info.flags = 2;
		motif_wm_info.win = window;
		XChangeProperty(
			dpy,Scr.Root, _XA_MOTIF_WM, _XA_MOTIF_WM,32,
			PropModeReplace, (unsigned char *)&motif_wm_info, 2);
		set_yorn='y';
	}

	return;
}

/*
 * setInitFunctionName - sets one of the init, restart or exit function names
 * getInitFunctionName - gets one of the init, restart or exit function names
 *
 * First parameter defines a function type: 0 - init, 1 - restart, 2 - exit.
 */
void setInitFunctionName(int n, const char *name)
{
	initFunctionNames[n >= 0 && n < 3? n: 3] = name;

	return;
}

const char *getInitFunctionName(int n)
{
	return initFunctionNames[n >= 0 && n < 3? n: 3];
}

/***********************************************************************
 *
 *  Procedure:
 *      main - start of fvwm
 *
 ***********************************************************************/

int main(int argc, char **argv)
{
	unsigned long valuemask;
	XSetWindowAttributes attributes;
	int i;
	int len;
	char *display_string;
	char *opt_color_limit = NULL;
	char message[255];
	Bool do_force_single_screen = False;
	Bool replace_wm = False;
	Bool option_error = False;
	int visualClass = -1;
	int visualId = -1;
	const exec_context_t *exc;
	exec_context_changes_t ecc;

	DBUG("main", "Entered, about to parse args");

	memset(&Scr, 0, sizeof(Scr));
	/* for use on restart */
	g_argv = (char **)safemalloc((argc + 4) * sizeof(char *));
	g_argc = argc;
	for (i = 0; i < argc; i++)
	{
		g_argv[i] = argv[i];
	}
	g_argv[g_argc] = NULL;
	setVersionInfo();
	FlocaleInit(LC_CTYPE, "", "", "FVWM");

	/* Put the default module directory into the environment so it can be
	 * used later by the config file, etc.  */
	putenv("FVWM_MODULEDIR=" FVWM_MODULEDIR);

	/* Figure out user's home directory */
	home_dir = getenv("HOME");
#ifdef HAVE_GETPWUID
	if (home_dir == NULL)
	{
		struct passwd* pw = getpwuid(getuid());
		if (pw != NULL)
		{
			home_dir = safestrdup(pw->pw_dir);
		}
	}
#endif
	if (home_dir == NULL)
	{
		home_dir = "/"; /* give up and use root dir */
	}

	/* Figure out where to read and write user's data files. */
	fvwm_userdir = getenv("FVWM_USERDIR");
	if (fvwm_userdir == NULL)
	{
		fvwm_userdir = safestrdup(CatString2(home_dir, "/.fvwm"));
		/* Put the user directory into the environment so it can be used
		 * later everywhere. */
		putenv(safestrdup(CatString2("FVWM_USERDIR=", fvwm_userdir)));
	}

	/* Create FVWM_USERDIR directory if needed */
	if (access(fvwm_userdir, F_OK) != 0)
	{
		mkdir(fvwm_userdir, 0777);
	}
	if (access(fvwm_userdir, W_OK) != 0)
	{
		fvwm_msg(
			ERR, "main", "No write permissions in `%s/'.\n",
			fvwm_userdir);
	}

	for (i = 1; i < argc; i++)
	{
		if (StrEquals(argv[i], "-debug_stack_ring") ||
		    StrEquals(argv[i], "--debug_stack_ring"))
		{
			debugging_stack_ring = True;
		}
		else if (StrEquals(argv[i], "-debug") ||
			 StrEquals(argv[i], "--debug"))
		{
			debugging = True;
		}
		else if (StrEquals(argv[i], "-clientId") ||
			 StrEquals(argv[i], "--clientId"))
		{
			if (++i >= argc)
			{
				usage();
			}
			SetClientID(argv[i]);
		}
		else if (StrEquals(argv[i], "-restore") ||
			 StrEquals(argv[i], "--restore"))
		{
			if (++i >= argc)
			{
				usage();
			}
			state_filename = argv[i];
		}
		else if (StrEquals(argv[i], "-s"))
		{
			do_force_single_screen = True;
		}
		else if (StrEquals(argv[i], "-d"))
		{
			if (++i >= argc)
			{
				usage();
			}
			display_name = argv[i];
		}
		else if (StrEquals(argv[i], "-f"))
		{
			if (++i >= argc)
			{
				usage();
			}
			if (num_config_commands < MAX_CFG_CMDS)
			{
				config_commands[num_config_commands] =
					(char *)malloc(6+strlen(argv[i]));
				strcpy(config_commands[num_config_commands],
				       "Read ");
				strcat(config_commands[num_config_commands],
				       argv[i]);
				num_config_commands++;
			}
			else
			{
				fvwm_msg(
					ERR, "main",
					"only %d -f and -cmd parms allowed!",
					MAX_CFG_CMDS);
			}
		}
		else if (StrEquals(argv[i], "-cmd") ||
			 StrEquals(argv[i], "--cmd"))
		{
			if (++i >= argc)
				usage();
			if (num_config_commands < MAX_CFG_CMDS)
			{
				config_commands[num_config_commands] =
					safestrdup(argv[i]);
				num_config_commands++;
			}
			else
			{
				fvwm_msg(
					ERR, "main",
					"only %d -f and -cmd parms allowed!",
					MAX_CFG_CMDS);
			}
		}
		else if (StrEquals(argv[i], "-h"))
		{
			usage();
			exit(0);
		}
		else if (StrEquals(argv[i], "-blackout"))
		{
			/* obsolete option */
			fvwm_msg(
				WARN, "main",
				"The -blackout option is obsolete, it may be "
				"removed in the future.");
		}
		else if (StrEquals(argv[i], "-replace") ||
			 StrEquals(argv[i], "--replace"))
		{
			replace_wm = True;
		}
		/* check for visualId before visual to remove ambiguity */
		else if (StrEquals(argv[i], "-visualId") ||
			 StrEquals(argv[i], "--visualId"))
		{
			visualClass = -1;
			if (++i >= argc)
			{
				usage();
			}
			if (sscanf(argv[i], "0x%x", &visualId) == 0)
			{
				if (sscanf(argv[i], "%d", &visualId) == 0)
				{
					usage();
				}
			}
		}
		else if (StrEquals(argv[i], "-visual") ||
			 StrEquals(argv[i], "--visual"))
		{
			visualId = None;
			if (++i >= argc)
			{
				usage();
			}
			if (strncasecmp(argv[i], "staticg", 7) == 0)
			{
				visualClass = StaticGray;
			}
			else if (strncasecmp(argv[i], "g", 1) == 0)
			{
				visualClass = GrayScale;
			}
			else if (strncasecmp(argv[i], "staticc", 7) == 0)
			{
				visualClass = StaticColor;
			}
			else if (strncasecmp(argv[i], "p", 1) == 0)
			{
				visualClass = PseudoColor;
			}
			else if (strncasecmp(argv[i], "t", 1) == 0)
			{
				visualClass = TrueColor;
			}
			else if (strncasecmp(argv[i], "d", 1) == 0)
			{
				visualClass = DirectColor;
			}
			else
			{
				usage();
			}
		}
		else if (StrEquals(argv[i], "-color-limit") ||
			 StrEquals(argv[i], "--color-limit"))
		{
			if (++i >= argc)
			{
				usage();
			}
			CopyString(&opt_color_limit,argv[i]);
		}
		else if (StrEquals(argv[i], "-version") ||
			 StrEquals(argv[i], "--version"))
		{
			printf("%s\n%s\n\n%s\n", Fvwm_VersionInfo,
			       Fvwm_SupportInfo, Fvwm_LicenseInfo);
			exit(0);
		}
		else
		{
			fvwm_msg(ERR, "main", "Unknown option: `%s'", argv[i]);
			option_error = TRUE;
		}
	}

	DBUG("main", "Done parsing args");

	if (option_error)
	{
		usage();
	}

	DBUG("main", "Installing signal handlers");
	InstallSignals();

	if (!(Pdpy = dpy = XOpenDisplay(display_name)))
	{
		fvwm_msg(
			ERR, "main", "can't open display %s",
			XDisplayName(display_name));
		exit (1);
	}
	atexit(catch_exit);
	Scr.screen= DefaultScreen(dpy);
	Scr.NumberOfScreens = ScreenCount(dpy);

	master_pid = getpid();

	if (!do_force_single_screen)
	{
		int myscreen = 0;
		char *cp;

		strcpy(message, XDisplayString(dpy));
		for (i=0;i<Scr.NumberOfScreens;i++)
		{
			if (i != Scr.screen && fork() == 0)
			{
				myscreen = i;
				/* Truncate the string 'whatever:n.n' to
				 * 'whatever:n', and then append the screen
				 * number. */
				cp = strchr(message, ':');
				if (cp != NULL)
				{
					cp = strchr(cp, '.');
					if (cp != NULL)
					{
						/* truncate at display part */
						*cp = '\0';
					}
				}
				sprintf(message + strlen(message), ".%d",
					myscreen);
				Pdpy = dpy = XOpenDisplay(message);
				Scr.screen = myscreen;
				Scr.NumberOfScreens = ScreenCount(dpy);

				break;
			}
		}

		g_argv[argc++] = "-s";
		g_argv[argc] = NULL;
	}
	FScreenInit(dpy);
	x_fd = XConnectionNumber(dpy);
	fd_width = GetFdWidth();

#ifdef HAVE_X11_FD
	if (fcntl(x_fd, F_SETFD, 1) == -1)
	{
		fvwm_msg(ERR, "main", "close-on-exec failed");
		exit (1);
	}
#endif

	/* Add a DISPLAY entry to the environment, incase we were started
	 * with fvwm -display term:0.0 */
	len = strlen(XDisplayString(dpy));
	display_string = safemalloc(len+10);
	sprintf(display_string, "DISPLAY=%s",XDisplayString(dpy));
	putenv(display_string);
	/* Add a HOSTDISPLAY environment variable, which is the same as
	 * DISPLAY, unless display = :0.0 or unix:0.0, in which case the full
	 * host name will be used for ease in networking.
	 *
	 * Note: Can't free the rdisplay_string after putenv, because it
	 * becomes part of the environment! */
	if (strncmp(display_string, "DISPLAY=:",9)==0)
	{
		char client[MAXHOSTNAME], *rdisplay_string;
		gethostname(client,MAXHOSTNAME);
		rdisplay_string = safemalloc(len+14 + strlen(client));
		sprintf(rdisplay_string, "HOSTDISPLAY=%s:%s", client,
			&display_string[9]);
		putenv(rdisplay_string);
	}
	else if (strncmp(display_string, "DISPLAY=unix:",13)==0)
	{
		char client[MAXHOSTNAME], *rdisplay_string;
		gethostname(client,MAXHOSTNAME);
		rdisplay_string = safemalloc(len+14 + strlen(client));
		sprintf(rdisplay_string, "HOSTDISPLAY=%s:%s",client,
			&display_string[13]);
		putenv(rdisplay_string);
	}
	else
	{
		char *rdisplay_string;
		rdisplay_string = safemalloc(len+14);
		sprintf(rdisplay_string, "HOSTDISPLAY=%s",XDisplayString(dpy));
		putenv(rdisplay_string);
	}

	Scr.Root = RootWindow(dpy, Scr.screen);
	if (Scr.Root == None)
	{
		fvwm_msg(
			ERR, "main", "Screen %d is not a valid screen",
			(int)Scr.screen);
		exit(1);
	}

	if (visualClass != -1)
	{
		/* grab the best visual of the required class */
		XVisualInfo template, *vizinfo;
		int total, i;

		Pdepth = 0;
		template.screen = Scr.screen;
		template.class = visualClass;
		vizinfo = XGetVisualInfo(
			dpy, VisualScreenMask | VisualClassMask, &template,
			&total);
		if (total)
		{
			for (i = 0; i < total; i++)
			{
				if (vizinfo[i].depth > Pdepth)
				{
					Pvisual = vizinfo[i].visual;
					Pdepth = vizinfo[i].depth;
				}
			}
			XFree(vizinfo);
			/* have to have a colormap for non-default visual
			 * windows */
			if (Pvisual->class == DirectColor)
			{
				Pcmap = XCreateColormap(
					dpy, Scr.Root, Pvisual, AllocAll);
			}
			else
			{
				Pcmap = XCreateColormap(
					dpy, Scr.Root, Pvisual, AllocNone);
			}
			Pdefault = False;
		}
		else
		{
			fvwm_msg(
				ERR, "main", "Cannot find visual class %d",
				visualClass);
			visualClass = -1;
		}
	}
	else if (visualId != -1)
	{
		/* grab visual id asked for */
		XVisualInfo template, *vizinfo;
		int total;

		Pdepth = 0;
		template.screen = Scr.screen;
		template.visualid = visualId;
		vizinfo = XGetVisualInfo(
			dpy, VisualScreenMask | VisualIDMask, &template,
			&total);
		if (total)
		{
			/* visualID's are unique so there will only be one */
			Pvisual = vizinfo[0].visual;
			Pdepth = vizinfo[0].depth;
			XFree(vizinfo);
			/* have to have a colormap for non-default visual
			 * windows */
			if (Pvisual->class == DirectColor)
			{
				Pcmap = XCreateColormap(
					dpy, Scr.Root, Pvisual, AllocAll);
			}
			else
			{
				Pcmap = XCreateColormap(
					dpy, Scr.Root, Pvisual, AllocNone);
			}
			Pdefault = False;
		}
		else
		{
			fvwm_msg(
				ERR, "main", "VisualId 0x%x is not valid ",
				visualId);
			visualId = -1;
		}
	}

	/* use default visuals if none found so far */
	if (visualClass == -1 && visualId == -1)
	{
		Colormap *cmap_list = NULL;
		int num, i;

		Pdepth = 0;

		/* Detection of a card with 2 hardware colormaps (8+24) which use
		 * depth 8 for the default. We can use our own depth 24 cmap
		 * without affecting other applications. */
		if (DefaultDepth(dpy, Scr.screen) <= 8 &&
		    (cmap_list = XListInstalledColormaps(dpy, Scr.Root, &num))
		    && num > 1)
		{
			XVisualInfo template, *vizinfo;
			int total;

			template.screen = Scr.screen;
			template.class = TrueColor;
			vizinfo = XGetVisualInfo(
				dpy, VisualScreenMask|VisualClassMask,
				&template, &total);
			for(i = 0; i<total; i++)
			{
				if (Pdepth < vizinfo[i].depth &&
				    vizinfo[i].depth > 8)
				{
					Pvisual = vizinfo[i].visual;
					Pdepth = vizinfo[i].depth;
				}
			}
			XFree(vizinfo);
			if (Pdepth > 0)
			{
				Pcmap = XCreateColormap(
					dpy, Scr.Root, Pvisual, AllocNone);
				Pdefault = False;
			}
		}
		if (cmap_list)
		{
			XFree(cmap_list);
		}
		if (Pdepth == 0)
		{
			Pvisual = DefaultVisual(dpy, Scr.screen);
			Pdepth = DefaultDepth(dpy, Scr.screen);
			Pcmap = DefaultColormap(dpy, Scr.screen);
			Pdefault = True;
		}
	}

	PictureSetupWhiteAndBlack();

	/* make a copy of the visual stuff so that XorPixmap can swap with root
	 */
	PictureSaveFvwmVisual();

	Scr.ColorLimit = 0;
	PUseDynamicColors = 0;
	Scr.ColorLimit = PictureInitColors(
		PICTURE_CALLED_BY_FVWM, True, opt_color_limit, True, True);
	if (opt_color_limit)
	{
		free(opt_color_limit);
	}

	FShapeInit(dpy);
	FRenderInit(dpy);
	Scr.pscreen = XScreenOfDisplay(dpy, Scr.screen);
	Scr.use_backing_store = DoesBackingStore(Scr.pscreen);
	Scr.flags.do_save_under = DoesSaveUnders(Scr.pscreen);

	InternUsefulAtoms();

	/* Make sure property priority colors is empty */
	XChangeProperty(dpy, Scr.Root, _XA_MIT_PRIORITY_COLORS,
			XA_CARDINAL, 32, PropModeReplace, NULL, 0);

	Scr.FvwmCursors = CreateCursors(dpy);
	/* create a window which will accept the keyboard focus when no other
	 * windows have it */
	/* do this before any RC parsing as some GC's are created from this
	 * window rather than the root window */
	attributes.event_mask = XEVMASK_NOFOCUSW;
	attributes.override_redirect = True;
	attributes.colormap = Pcmap;
	attributes.cursor = Scr.FvwmCursors[CRS_DEFAULT];
	attributes.background_pixmap = None;
	attributes.border_pixel = 0;
	Scr.NoFocusWin=XCreateWindow(
		dpy, Scr.Root, -10, -10, 10, 10, 0, Pdepth, InputOutput,
		Pvisual, CWEventMask | CWOverrideRedirect | CWColormap |
		CWBackPixmap | CWBorderPixel | CWCursor, &attributes);
	XMapWindow(dpy, Scr.NoFocusWin);
	SetMWM_INFO(Scr.NoFocusWin);
	FOCUS_SET(Scr.NoFocusWin);

	frame_init();
	XFlush(dpy);
	if (debugging)
	{
		sync_server(1);
	}

	SetupICCCM2(replace_wm);
	XSetErrorHandler(CatchRedirectError);
	XSetIOErrorHandler(CatchFatal);
	XSelectInput(dpy, Scr.Root, XEVMASK_ROOTW);
	XFlush(dpy);

	XSetErrorHandler(FvwmErrorHandler);

	{
		/* do not grab the pointer earlier because if fvwm exits with
		 * the pointer grabbed while a different display is visible,
		 * XFree 4.0 freezes. */
		Cursor cursor = XCreateFontCursor(dpy, XC_watch);
		XGrabPointer(
			dpy, Scr.Root, 0, 0, GrabModeAsync, GrabModeAsync,
			None, cursor, CurrentTime);
	}
	{
		Atom atype;
		int aformat;
		unsigned long nitems, bytes_remain;
		unsigned char *prop;

		if (XGetWindowProperty(
			    dpy, Scr.Root, _XA_WM_DESKTOP, 0L, 1L, True,
			    _XA_WM_DESKTOP, &atype, &aformat, &nitems,
			    &bytes_remain, &prop) == Success)
		{
			if (prop != NULL)
			{
				Restarting = True;
				/* do_force_single_screen = True; */
			}
		}
	}
	restart_state_filename = safestrdup(
		CatString3(fvwm_userdir, "/.fs-restart-",
			   getenv("HOSTDISPLAY")));
	if (!state_filename && Restarting)
	{
		state_filename = restart_state_filename;
	}

	/* This should be done early enough to have the window states loaded
	 * before the first call to AddWindow. */
	LoadWindowStates(state_filename);

	InitVariables();
	if (visualClass != -1 || visualId != -1)
	{
		/* this is so that menus use the (non-default) fvwm colormap */
		FW_W(&Scr.FvwmRoot) = Scr.NoFocusWin;
		Scr.FvwmRoot.number_cmap_windows = 1;
		Scr.FvwmRoot.cmap_windows = &Scr.NoFocusWin;
	}
	InitEventHandlerJumpTable();
	initModules();

	Scr.gray_bitmap =
		XCreateBitmapFromData(dpy,Scr.Root,g_bits, g_width,g_height);

	EWMH_Init();

	DBUG("main", "Setting up rc file defaults...");
	SetRCDefaults();
	flush_window_updates();
	simplify_style_list();

	DBUG("main", "Running config_commands...");
	ecc.type = EXCT_INIT;
	ecc.w.wcontext = C_ROOT;
	exc = exc_create_context(&ecc, ECC_TYPE | ECC_WCONTEXT);
	if (num_config_commands > 0)
	{
		int i;
		for (i=0;i<num_config_commands;i++)
		{
			DoingCommandLine = True;
			execute_function(NULL, exc, config_commands[i], 0);
			free(config_commands[i]);
		}
		DoingCommandLine = False;
	}
	else
	{
		/* Run startup command file in 5 places: FVWM_USERDIR,
		 * FVWM_DATADIR, and for compatibility: ~/.fvwm2rc,
		 * $sysconfdir/system.fvwm2rc */
		if (!run_command_file(
			    CatString3(fvwm_userdir, "/", FVWMRC), exc) &&
		    !run_command_file(
			    CatString3(home_dir, "/", FVWMRC), exc) &&
		    !run_command_file(
			    CatString3(FVWM_DATADIR, "/", FVWMRC), exc) &&
		    !run_command_file(
			    CatString3(FVWM_DATADIR, "/system", FVWMRC), exc) &&
		    !run_command_file(
			    CatString3(FVWM_CONFDIR, "/system", FVWMRC), exc))
		{
			fvwm_msg(
				ERR, "main", "Cannot read startup file, tried: "
				"\n\t%s/%s\n\t%s/%s\n\t%s/%s\n\t%s/system"
				"%s\n\t%s/system%s", fvwm_userdir, FVWMRC,
				home_dir, FVWMRC, FVWM_DATADIR, FVWMRC,
				FVWM_DATADIR, FVWMRC, FVWM_CONFDIR, FVWMRC);
		}
	}
	exc_destroy_context(exc);

	DBUG("main", "Done running config_commands");

	if (Pdepth<2)
	{
		Scr.gray_pixmap = XCreatePixmapFromBitmapData(
			dpy, Scr.NoFocusWin, g_bits, g_width, g_height,
			PictureBlackPixel(), PictureWhitePixel(), Pdepth);
		Scr.light_gray_pixmap = XCreatePixmapFromBitmapData(
			dpy, Scr.NoFocusWin, l_g_bits, l_g_width, l_g_height,
			PictureBlackPixel(), PictureWhitePixel(), Pdepth);
		Scr.sticky_gray_pixmap = XCreatePixmapFromBitmapData(
			dpy, Scr.NoFocusWin, s_g_bits, s_g_width, s_g_height,
			PictureBlackPixel(), PictureWhitePixel(), Pdepth);
	}

	attributes.background_pixel = Scr.StdBack;
	attributes.colormap = Pcmap;
	attributes.border_pixel = 0;
	valuemask = CWBackPixel | CWColormap | CWBorderPixel;

	Scr.SizeWindow = XCreateWindow(
		dpy, Scr.Root, 0, 0, 1, 1, (unsigned int)0, Pdepth,
		InputOutput, Pvisual, valuemask, &attributes);
	resize_geometry_window();
	initPanFrames();
	MyXGrabServer(dpy);
	checkPanFrames();
	MyXUngrabServer(dpy);
	CoerceEnterNotifyOnCurrentWindow();
	SessionInit();
	GNOME_Init();
	DBUG("main", "Entering HandleEvents loop...");

	HandleEvents();
	switch (fvwmRunState)
	{
	case FVWM_DONE:
		Done(0, NULL);     /* does not return */

	case FVWM_RESTART:
		Done(1, "");       /* does not return */

	default:
		DBUG("main", "Unknown FVWM run-state");
	}

	exit(0);
}
