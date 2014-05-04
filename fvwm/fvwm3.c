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
 * along with this program; if not, see: <http://www.gnu.org/licenses/>
 */

/* ---------------------------- included header files ---------------------- */

#include "config.h"

#include <stdio.h>
#include <signal.h>
#include <errno.h>
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
#include <X11/Xatom.h>

#include "libs/fvwmlib.h"
#include "libs/envvar.h"
#include "libs/Strings.h"
#include "libs/System.h"
#include "libs/Grab.h"
#include "libs/ColorUtils.h"
#include "libs/Graphics.h"
#include "libs/FScreen.h"
#include "libs/FShape.h"
#include "libs/PictureBase.h"
#include "libs/PictureUtils.h"
#include "libs/Fsvg.h"
#include "libs/FRenderInit.h"
#include "libs/charmap.h"
#include "libs/wcontext.h"
#include "fvwm.h"
#include "externs.h"
#include "cursor.h"
#include "functions.h"
#include "misc.h"
#include "screen.h"
#include "builtins.h"
#include "module_list.h"
#include "colorset.h"
#include "events.h"
#include "eventhandler.h"
#include "eventmask.h"
#include "icccm2.h"
#include "ewmh.h"
#include "add_window.h"
#include "libs/fvwmsignal.h"
#include "stack.h"
#include "virtual.h"
#include "session.h"
#include "read.h"
#include "focus.h"
#include "update.h"
#include "move_resize.h"
#include "frame.h"
#include "menus.h"
#include "menubindings.h"
#include "libs/FGettext.h"

/* ---------------------------- local definitions -------------------------- */

#define MAXHOSTNAME 255
#define MAX_CFG_CMDS 10
#define MAX_ARG_SIZE 25

#define g_width 2
#define g_height 2
#define l_g_width 4
#define l_g_height 2
#define s_g_width 4
#define s_g_height 4

/* ---------------------------- local macros ------------------------------- */

/* ---------------------------- imports ------------------------------------ */

extern int last_event_type;

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */

typedef enum
{
	FVWM_RUNNING = 0,
	FVWM_DONE,
	FVWM_RESTART
} fvwm_run_state_t;

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- local variables ---------------------------- */

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

static const char *init_function_names[4] =
{
	"InitFunction",
	"RestartFunction",
	"ExitFunction",
	"Nop"
};

/* ---------------------------- exported variables (globals) --------------- */

int master_pid;                 /* process number of 1st fvwm process */

ScreenInfo Scr;                 /* structures for the screen */
Display *dpy = NULL;            /* which display are we talking to */

Bool fFvwmInStartup = True;     /* Set to False when startup has finished */
Bool DoingCommandLine = False;  /* Set True before each cmd line arg */

XContext FvwmContext;           /* context for fvwm windows */
XContext MenuContext;           /* context for fvwm menus */

int JunkX = 0, JunkY = 0;
Window JunkRoot, JunkChild;             /* junk window */
int JunkWidth, JunkHeight, JunkBW, JunkDepth;
unsigned int JunkMask;

Bool debugging = False;
Bool debugging_stack_ring = False;

Window bad_window = None;

char **g_argv;
int g_argc;

char *state_filename = NULL;
char *restart_state_filename = NULL;  /* $HOME/.fs-restart */

Bool Restarting = False;
int x_fd;
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
Atom _XA_WINDOW_ROLE;
Atom _XA_WM_CLIENT_LEADER;
Atom _XA_SM_CLIENT_ID;

Atom _XA_XROOTPMAP_ID;
Atom _XA_XSETROOT_ID;

/* ---------------------------- local functions ---------------------------- */

static void SaveDesktopState(void)
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
	/* Create priority colors if necessary. */
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
	_XA_WINDOW_ROLE=XInternAtom(dpy, "WINDOW_ROLE",False);
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

/*
 * Restart on a signal
 */
static RETSIGTYPE
Restart(int sig)
{
	fvwmRunState = FVWM_RESTART;

	/* This function might not return - it could "long-jump"
	 * right out, so we need to do everything we need to do
	 * BEFORE we call it ... */
	fvwmSetTerminate(sig);

	SIGNAL_RETURN;
}

static RETSIGTYPE
SigDone(int sig)
{
	fvwmRunState = FVWM_DONE;

	/* This function might not return - it could "long-jump"
	 * right out, so we need to do everything we need to do
	 * BEFORE we call it ... */
	fvwmSetTerminate(sig);

	SIGNAL_RETURN;
}

/*
 * parse_command_args - parses a given command string into a given limited
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
static int parse_command_args(
	const char *command, char **argv, int max_argc, const char **error_msg)
{
	/* It is impossible to guess the exact length because of expanding */
#define MAX_TOTAL_ARG_LEN 256
	/* char *arg_string = safemalloc(MAX_TOTAL_ARG_LEN); */
	static char arg_string[MAX_TOTAL_ARG_LEN];
	int total_arg_len = 0;
	int error_code = 0;
	int argc;
	char *aptr = arg_string;
	const char *cptr = command;

#define the_char (*cptr)
#define adv_char (cptr++)
#define top_char (*cptr     == '\\' ? *(cptr + 1) : *cptr)
#define pop_char (*(cptr++) == '\\' ? *(cptr++) : *(cptr - 1))
#define can_add_arg_char (total_arg_len < MAX_TOTAL_ARG_LEN-1)
#define add_arg_char(ch) (++total_arg_len, *(aptr++) = ch)
#define can_add_arg_str(str) (total_arg_len < MAX_TOTAL_ARG_LEN - strlen(str))
#define add_arg_str(str) \
{\
	const char *tmp = str;\
	while (*tmp)\
	{\
		add_arg_char(*(tmp++));\
	}\
}

	*error_msg = "";
	if (!command)
	{
		*error_msg = "No command";
		return -1;
	}
	for (argc = 0; argc < max_argc - 1; argc++)
	{
		int s_quote = 0;
		argv[argc] = aptr;
		while (isspace(the_char))
		{
			adv_char;
		}
		if (the_char == '\0')
		{
			break;
		}
		while ((s_quote || !isspace(the_char)) &&
		       the_char != '\0' && can_add_arg_char)
		{
			if (the_char == '"')
			{
				if (s_quote)
				{
					s_quote = 0;
				}
				else
				{
					s_quote = 1;
				}
				adv_char;
			}
			else if (!s_quote && the_char == '\'')
			{
				adv_char;
				while (the_char != '\'' && the_char != '\0' &&
				       can_add_arg_char)
				{
					add_arg_char(pop_char);
				}
				if (the_char == '\'')
				{
					adv_char;
				}
				else if (!can_add_arg_char)
				{
					break;
				}
				else
				{
					*error_msg = "No closing single quote";
					error_code = -3;
					break;
				}
			}
			else if (!s_quote && the_char == '~')
			{
				if (!can_add_arg_str(home_dir))
				{
					break;
				}
				add_arg_str(home_dir);
				adv_char;
			}
			else if (the_char == '$')
			{
				int beg, len;
				const char *str = getFirstEnv(cptr, &beg, &len);

				if (!str || beg)
				{
					add_arg_char(the_char);
					adv_char;
					continue;
				}
				if (!can_add_arg_str(str))
				{
					break;
				}
				add_arg_str(str);
				cptr += len;
			}
			else
			{
				if (add_arg_char(pop_char) == '\0')
				{
					break;
				}
			}
		}
		if (*(aptr-1) == '\0')
		{
			*error_msg = "Unexpected last backslash";
			error_code = -2;
			break;
		}
		if (error_code)
		{
			break;
		}
		if (the_char == '~' || the_char == '$' || !can_add_arg_char)
		{
			*error_msg = "The command is too long";
			error_code = -argc - 100;
			break;
		}
		if (s_quote)
		{
			*error_msg = "No closing double quote";
			error_code = -4;
			break;
		}
		add_arg_char('\0');
	}
#undef the_char
#undef adv_char
#undef top_char
#undef pop_char
#undef can_add_arg_char
#undef add_arg_char
#undef can_add_arg_str
#undef add_arg_str
	argv[argc] = NULL;
	if (argc == 0 && !error_code)
	{
		*error_msg = "Void command";
	}

	return error_code ? error_code : argc;
}

/*
 *
 */
static
char *get_display_name(char *display_name, int screen_num)
{
	char *msg;
	char *new_dn;
	char *cp;
	char string_screen_num[32];

	CopyString(&msg, display_name);
	cp = strchr(msg, ':');
	if (cp != NULL)
	{
		cp = strchr(cp, '.');
		if (cp != NULL)
		{
			/* truncate at display part */
			*cp = '\0';
		}
	}
	sprintf(string_screen_num, ".%d", screen_num);
	/* TA:  FIXME!  Use asprintF() */
	new_dn = xmalloc(strlen(msg) + strlen(string_screen_num) + 1);
	*new_dn = '\0';
	strcat(new_dn, msg);
	strcat(new_dn, string_screen_num);
	free(msg);

	return new_dn;
}

/*
 *
 *  Procedure:
 *      Done - tells fvwm to clean up and exit
 *
 */
/* if restart is true, command must not be NULL... */
void Done(int restart, char *command)
{
	const char *exit_func_name;

	if (!restart)
	{
		MoveViewport(0,0,False);
	}
	/* migo (03/Jul/1999): execute [Session]ExitFunction */
	exit_func_name = get_init_function_name(2);
	if (functions_is_complex_function(exit_func_name))
	{
		const exec_context_t *exc;
		exec_context_changes_t ecc;

		char *action = xstrdup(CatString2("Function ", exit_func_name));
		ecc.type = restart ? EXCT_TORESTART : EXCT_QUIT;
		ecc.w.wcontext = C_ROOT;
		exc = exc_create_context(&ecc, ECC_TYPE | ECC_WCONTEXT);
		execute_function(NULL, exc, action, 0);
		exc_destroy_context(exc);
		free(action);
	}
	/* XFree freeze hack */
	XUngrabPointer(dpy, CurrentTime);
	XUngrabKeyboard(dpy, CurrentTime);
	XUngrabServer(dpy);
	if (!restart)
	{
		Reborder();
	}
	EWMH_ExitStuff();
	if (restart)
	{
		Bool do_preserve_state = True;
		SaveDesktopState();

		if (command)
		{
			while (isspace(command[0]))
			{
				command++;
			}
			if (strncmp(command, "--dont-preserve-state", 21) == 0)
			{
				do_preserve_state = False;
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
			do_preserve_state);

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
			const char *error_msg;
			int n = parse_command_args(
				command, my_argv, MAX_ARG_SIZE, &error_msg);

			if (n <= 0)
			{
				fvwm_msg(
					ERR, "Done",
					"Restart command parsing error in"
					" (%s): [%s]", command, error_msg);
			}
			else if (strcmp(my_argv[0], "--pass-args") == 0)
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
	module_kill_all();

	exit(0);
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

	/* Unblock these signals so that we can process them again. */
	sigprocmask(SIG_UNBLOCK, &sigact.sa_mask, NULL);

	/* Reap all zombies automatically! This signal handler will only be
	 * called if a child process dies, not if someone sends a child a STOP
	 * signal. Note that none of our "terminate" signals can be delivered
	 * until the SIGCHLD handler completes, and this is a Good Thing
	 * because the terminate handlers might exit abruptly via "siglongjmp".
	 * This could potentially leave SIGCHLD handler with unfinished
	 * business ...
	 *
	 * NOTE:  We could still receive SIGPIPE signals within the SIGCHLD
	 * handler, but the SIGPIPE handler has the SA_RESTART flag set and so
	 * should not affect our "wait" system call. */
	sigact.sa_flags |= SA_NOCLDSTOP;
	sigact.sa_handler = fvwmReapChildren;
	sigaction(SIGCHLD, &sigact, NULL);
#else
#ifdef USE_BSD_SIGNALS
	fvwmSetSignalMask(
		sigmask(SIGUSR1) | sigmask(SIGINT) | sigmask(SIGHUP) |
		sigmask(SIGQUIT) | sigmask(SIGTERM) );
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
	signal(SIGCHLD, fvwmReapChildren);
#ifdef HAVE_SIGINTERRUPT
	siginterrupt(SIGCHLD, 0);
#endif
#endif

	/* When fvwm restarts, the SIGCHLD handler is automatically reset
	 * to the default handler. This means that Zombies left over from
	 * the previous instance of fvwm could still be roaming the process
	 * table if they exited while the default handler was in place.
	 * We fix this by invoking the SIGCHLD handler NOW, so that they
	 * may finally rest in peace. */
	fvwmReapChildren(0);

	return;
}

void fvmm_deinstall_signals(void)
{
	signal(SIGCHLD, SIG_DFL);
	signal(SIGHUP, SIG_DFL);
	signal(SIGINT, SIG_DFL);
	signal(SIGPIPE, SIG_DFL);
	signal(SIGQUIT, SIG_DFL);
	signal(SIGTERM, SIG_DFL);
	signal(SIGUSR1, SIG_DFL);

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
		v->x = xmalloc(sizeof(char) * v->num);
		v->y = xmalloc(sizeof(char) * v->num);
		v->xoff = xmalloc(sizeof(char) * v->num);
		v->yoff = xmalloc(sizeof(char) * v->num);
		v->c = xcalloc(v->num, sizeof(char));
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
		v->x = xmalloc(sizeof(char) * v->num);
		v->y = xmalloc(sizeof(char) * v->num);
		v->xoff = xmalloc(sizeof(char) * v->num);
		v->yoff = xmalloc(sizeof(char) * v->num);
		v->c = xcalloc(v->num, sizeof(char));
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
		v->x = xmalloc(sizeof(char) * v->num);
		v->y = xmalloc(sizeof(char) * v->num);
		v->xoff = xmalloc(sizeof(char) * v->num);
		v->yoff = xmalloc(sizeof(char) * v->num);
		v->c = xcalloc(v->num, sizeof(char));
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
		v->x = xmalloc(sizeof(char) * v->num);
		v->y = xmalloc(sizeof(char) * v->num);
		v->xoff = xmalloc(sizeof(char) * v->num);
		v->yoff = xmalloc(sizeof(char) * v->num);
		v->c = xcalloc(v->num, sizeof(char));
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
		v->x = xmalloc(sizeof(char) * v->num);
		v->y = xmalloc(sizeof(char) * v->num);
		v->xoff = xmalloc(sizeof(char) * v->num);
		v->yoff = xmalloc(sizeof(char) * v->num);
		v->c = xcalloc(v->num, sizeof(char));
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
		v->x = xmalloc(sizeof(char) * v->num);
		v->y = xmalloc(sizeof(char) * v->num);
		v->xoff = xmalloc(sizeof(char) * v->num);
		v->yoff = xmalloc(sizeof(char) * v->num);
		v->c = xcalloc(v->num, sizeof(char));
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
		v->x = xmalloc(sizeof(char) * v->num);
		v->y = xmalloc(sizeof(char) * v->num);
		v->xoff = xmalloc(sizeof(char) * v->num);
		v->yoff = xmalloc(sizeof(char) * v->num);
		v->c = xcalloc(v->num, sizeof(char));
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
		v->x = xmalloc(sizeof(char) * v->num);
		v->y = xmalloc(sizeof(char) * v->num);
		v->xoff = xmalloc(sizeof(char) * v->num);
		v->yoff = xmalloc(sizeof(char) * v->num);
		v->c = xcalloc(v->num, sizeof(char));
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

	/* create scratch GC's */
	gcm = GCFunction|GCLineWidth;
	gcv.function = GXcopy;
	gcv.line_width = 0;

	Scr.ScratchGC1 = fvwmlib_XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);
	Scr.ScratchGC2 = fvwmlib_XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);
	Scr.ScratchGC3 = fvwmlib_XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);
	Scr.ScratchGC4 = fvwmlib_XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);
	Scr.TitleGC = fvwmlib_XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);
	Scr.BordersGC = fvwmlib_XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);
	Scr.TransMaskGC = fvwmlib_XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);
	Scr.ScratchMonoPixmap = XCreatePixmap(dpy, Scr.Root, 1, 1, 1);
	Scr.MonoGC = fvwmlib_XCreateGC(dpy, Scr.ScratchMonoPixmap, gcm, &gcv);
	Scr.ScratchAlphaPixmap = XCreatePixmap(
		dpy, Scr.Root, 1, 1, FRenderGetAlphaDepth());
	Scr.AlphaGC = fvwmlib_XCreateGC(dpy, Scr.ScratchAlphaPixmap, gcm, &gcv);
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
	Scr.EdgeScrollX = DEFAULT_EDGE_SCROLL * Scr.MyDisplayWidth / 100;
	Scr.EdgeScrollY = DEFAULT_EDGE_SCROLL * Scr.MyDisplayHeight / 100;
	Scr.ScrollDelay = DEFAULT_SCROLL_DELAY;
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
	Scr.Desktops = xmalloc(sizeof(DesktopsInfo));
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
	Scr.DefaultDecor.tag = "Default";
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
	Scr.bo.is_raise_hack_needed =
		(strcmp (
			ServerVendor (dpy),
			"Hummingbird Communications Ltd.") == 0) ||
		(strcmp (
			ServerVendor (dpy),
			"Network Computing Devices Inc.") == 0) ||
		(strcmp (ServerVendor (dpy), "WRQ, Inc.") == 0);

	Scr.bo.is_modality_evil = 0;
	Scr.bo.do_disable_configure_notify = 0;
	Scr.bo.do_install_root_cmap = 0;
	Scr.bo.do_enable_flickering_qt_dialogs_workaround = 1;
	Scr.bo.do_enable_qt_drag_n_drop_workaround = 0;
	Scr.bo.do_enable_ewmh_iconic_state_workaround = 0;

	Scr.gs.do_emulate_mwm = DEFAULT_EMULATE_MWM;
	Scr.gs.do_emulate_win = DEFAULT_EMULATE_WIN;
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
	/* EdgeLeaveCommands - no edge leave commands by default */
	Scr.PanFrameTop.command_leave    = NULL;
	Scr.PanFrameBottom.command_leave = NULL;
	Scr.PanFrameRight.command_leave  = NULL;
	Scr.PanFrameLeft.command_leave   = NULL;
	Scr.flags.is_pointer_on_this_screen = !!FQueryPointer(
		dpy, Scr.Root, &JunkRoot, &JunkChild, &JunkX, &JunkY, &JunkX,
		&JunkY, &JunkMask);

	/* make sure colorset 0 exists */
	alloc_colorset(0);

	return;
}

static void usage(int is_verbose)
{
	fprintf(stderr, "usage: %s", g_argv[0]);
	fprintf(stderr,
		" [-d display]"
		" [-f cfgfile]"
		" [-c cmd]"
		" [-s [screen_num]]"
		" [-I vis-id | -C vis-class]"
		" [-l colors"
		" [-L|A|S|P] ...]"
		" [-r]"
		" [OTHER OPTIONS] ..."
		"\n");
	if (!is_verbose)
	{
		fprintf(
			stderr, "Try '%s --help' for more information.\n",
			g_argv[0]);
		return;
	}
	fprintf(stderr,
		" -A:           allocate palette\n"
		" -c cmd:       preprocess configuration file with <cmd>\n"
		" -C vis-class: use visual class <vis-class>\n"
		" -d display:   run fvwm on <display>\n"
		" -D:           enable debug oputput\n"
		" -f cfgfile:   read configuration from <cfgfile>\n"
		" -F file:      used internally for session management\n"
		" -h, -?:       print this help message\n"
		" -i client-id: used internally for session management\n"
		" -I vis-id:    use visual <vis-id>\n"
		" -l colors:    try to use no more than <colors> colors\n"
		" -L:           strict color limit\n"
		" -P:           visual palette\n"
		" -r:           replace running window manager\n"
		" -s [screen]:  manage a single screen\n"
		" -S:           static palette\n"
		" -V:           print version information\n"
		);
	fprintf(
		stderr, "Try 'man %s' for more information.\n",
		PACKAGE);

		return;
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
	Fvwm_VersionInfo = xstrdup(version_str);

	sprintf(license_str,
		"fvwm comes with NO WARRANTY, to the extent permitted by law. "
		"You may\nredistribute copies of fvwm under "
		"the terms of the GNU General Public License.\n"
		"For more information about these matters, see the file "
		"named COPYING.");
	Fvwm_LicenseInfo = xstrdup(license_str);

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
#ifdef HAVE_RSVG
	strcat(support_str, " SVG,");
#endif
	if (FHaveShapeExtension)
		strcat(support_str, " Shape,");
#ifdef HAVE_XSHM
	strcat(support_str, " XShm,");
#endif
#ifdef SESSION
	strcat(support_str, " SM,");
#endif
#ifdef HAVE_BIDI
	strcat(support_str, " Bidi text,");
#endif
#ifdef HAVE_XINERAMA
	strcat(support_str, " Xinerama,");
#endif
#ifdef HAVE_XRENDER
	strcat(support_str, " XRender,");
#endif
#ifdef HAVE_XCURSOR
	strcat(support_str, " XCursor,");
#endif
#ifdef HAVE_XFT
	strcat(support_str, " XFT,");
#endif
#ifdef HAVE_NLS
	strcat(support_str, " NLS,");
#endif

	support_len = strlen(support_str);
	if (support_len > 0)
	{
		/* strip last comma */
		support_str[support_len - 1] = '\0';
		Fvwm_SupportInfo = xstrdup(
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
#define RC_DEFAULTS_COMPLETE ((char *)-1)
	int i;
	/* set up default colors, fonts, etc */
	const char *defaults[][3] = {
		{ "XORValue 0", "", "" },
		{ "ImagePath "FVWM_DATADIR"/default-config/images", "", "" },
		{ "SetEnv FVWM_DATADIR "FVWM_DATADIR"", "", "" },
		/* The below is historical -- before we had a default
		 * configuration which defines these and more.
		 */
		{ "DefaultFont", "", "" },
		{ "DefaultColors black grey", "", "" },
		{ DEFAULT_MENU_STYLE, "", "" },
		{ "TitleStyle Centered -- Raised", "", "" },
		{ "Style * Color lightgrey/dimgrey", "", "" },
		{ "Style * HilightFore black, HilightBack grey", "", "" },
		{ "DestroyFunc FvwmMakeMissingDesktopMenu", "", "" },
		{ "AddToFunc   FvwmMakeMissingDesktopMenu I PipeRead 'fvwm-menu-desktop --enable-mini-icons --fvwm-icons'", "", "" },
		{
			"AddToMenu MenuFvwmRoot \"",
			_("Builtin Menu"),
			"\" Title"
		},
                { "+ MissingSubmenuFunction FvwmMakeMissingDesktopMenu","",""},
		{ "+ \"&1. XTerm\" Exec xterm", "", ""},
		{
			"+ \"&2. ",
			_("Issue fvwm commands"),
			"\" Module FvwmConsole"
		},
		{
			"+ \"&D. ",
			_("Desktop Menu"),
			"\" Popup FvwmMenu"
		},
		{
			"+ \"&R. ",
			_("Restart fvwm"),
			"\" Restart"
		},
		{
			"+ \"&X. ",
			_("Exit fvwm"),
			"\" Quit"
		},
		{ "Mouse 1 R A Menu MenuFvwmRoot", "", "" },
		/* default menu navigation */
		{ "Key Escape M A MenuClose", "", "" },
		{ "Key Return M A MenuSelectItem", "", "" },
		{ "Key Left M A MenuCursorLeft", "", "" },
		{ "Key Right M A MenuCursorRight", "", "" },
		{ "Key Up M A MenuMoveCursor -1", "", "" },
		{ "Key Down M A MenuMoveCursor 1", "", "" },
		{ "Mouse 1 MI A MenuSelectItem", "", "" },
		/* don't add anything below */
		{ RC_DEFAULTS_COMPLETE, "", "" },
		{ "Read "FVWM_DATADIR"/ConfigFvwmDefaults", "", "" },
		{ NULL, NULL, NULL }
	};

	for (i = 0; defaults[i][0] != NULL; i++)
	{
		const exec_context_t *exc;
		exec_context_changes_t ecc;
		char *cmd;

		if (defaults[i][0] == RC_DEFAULTS_COMPLETE)
		{
			menu_bindings_startup_complete();
			continue;
		}
		ecc.type = Restarting ? EXCT_RESTART : EXCT_INIT;
		ecc.w.wcontext = C_ROOT;
		exc = exc_create_context(&ecc, ECC_TYPE | ECC_WCONTEXT);
		cmd = CatString3(
			defaults[i][0], defaults[i][1], defaults[i][2]);
		execute_function(NULL, exc, cmd, 0);
		exc_destroy_context(exc);
	}
#undef RC_DEFAULTS_COMPLETE

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
	module_kill_all();
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

/* ---------------------------- interface functions ------------------------ */

/* Does initial window captures and runs init/restart function */
void StartupStuff(void)
{
#define start_func_name "StartFunction"
	const char *init_func_name;
	const exec_context_t *exc;
	exec_context_changes_t ecc;

	ecc.type = Restarting ? EXCT_RESTART : EXCT_INIT;
	ecc.w.wcontext = C_ROOT;
	exc = exc_create_context(&ecc, ECC_TYPE | ECC_WCONTEXT);
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

# if 0
	/* It is safe to ungrab here: if not, and one of the init functions
	 * does not finish, we've got a complete freeze! */
	/* DV (15-Jul-2004): No, it is not safe to ungrab.  If another
	 * application grabs the pointer before execute_function gets it, the
	 * start functions are not executed.  And the pointer is grabbed
	 * during function execution anyway, so releasing it here buys us
	 * nothing. */
	UngrabEm(GRAB_STARTUP);
	XUngrabPointer(dpy, CurrentTime);
#endif

	/* migo (04-Sep-1999): execute StartFunction */
	if (functions_is_complex_function(start_func_name))
	{
		char *action = "Function " start_func_name;

		execute_function(NULL, exc, action, 0);
	}

	/* migo (03-Jul-1999): execute [Session]{Init|Restart}Function */
	init_func_name = get_init_function_name(Restarting == True);
	if (functions_is_complex_function(init_func_name))
	{
		char *action = xstrdup(
			CatString2("Function ", init_func_name));

		execute_function(NULL, exc, action, 0);
		free(action);
	}
	/* see comment above */
	UngrabEm(GRAB_STARTUP);
	XUngrabPointer(dpy, CurrentTime);

	/* This should be done after the initialization is finished, since
	 * it directly changes the global state. */
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

	/* TA:  20091212:  If we get here, we're done restarting, so reset the
	 * flag back to False!
	 */
	Restarting = False;

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

	for (tbp = decor->buttons, i = 0; i < NUMBER_OF_TITLE_BUTTONS;
	     i++, tbp++)
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

	DestroyAllButtons(decor);
	for (tbp = decor->buttons, i = 0; i < NUMBER_OF_TITLE_BUTTONS;
	     i++, tbp++)
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
		long props[2];
		/* prop[0]: flags */
		/* prop[1]: win */
	}  motif_wm_info;
	static char set_yorn='n';

	if (set_yorn=='y')
	{
		return;
	}

	if (Scr.bo.is_modality_evil)
	{
		/* Set Motif WM_INFO atom to make motif relinquish
		 * broken handling of modal dialogs */
		motif_wm_info.props[0] = 2;
		motif_wm_info.props[1] = window;
		XChangeProperty(
			dpy,Scr.Root, _XA_MOTIF_WM, _XA_MOTIF_WM,32,
			PropModeReplace, (unsigned char *)&motif_wm_info, 2);
		set_yorn='y';
	}

	return;
}

/*
 * set_init_function_name - sets one of the init, restart or exit function names
 * get_init_function_name - gets one of the init, restart or exit function names
 *
 * First parameter defines a function type: 0 - init, 1 - restart, 2 - exit.
 */
void set_init_function_name(int n, const char *name)
{
	init_function_names[n >= 0 && n < 3? n: 3] = name;

	return;
}

const char *get_init_function_name(int n)
{
	return init_function_names[n >= 0 && n < 3? n: 3];
}

#ifndef _PATH_DEVNULL
#	define _PATH_DEVNULL "/dev/null"
#endif
static void reopen_fd(int fd, char* mode, FILE *of)
{
	struct stat sbuf;
	FILE *f;
	int rc;

	errno = 0;
	rc = fstat(fd, &sbuf);
	if (rc == 0)
	{
		return;
	}
	else if (errno != EBADF)
	{
		exit(77);
	}
	f = freopen(_PATH_DEVNULL, mode, of);
	if (f == 0 || fileno(f) != fd)
	{
		exit(88);
	}

	return;
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
	Bool do_force_single_screen = False;
	int single_screen_num = -1;
	Bool replace_wm = False;
	int visualClass = -1;
	int visualId = -1;
	PictureColorLimitOption colorLimitop = {-1, -1, -1, -1, -1};
	const exec_context_t *exc;
	exec_context_changes_t ecc;

	DBUG("main", "Entered, about to parse args");

	fvwmlib_init_max_fd();
	/* Tell the FEvent module an event type that is not used by fvwm. */
	fev_init_invalid_event_type(KeymapNotify);

	/* close open fds */
	for (i = 3; i < fvwmlib_max_fd; i++)
	{
		close(i);
	}
	/* reopen stdin, stdout and stderr if necessary */
	reopen_fd(0, "rb", stdin);
	reopen_fd(1, "wb", stdout);
	reopen_fd(2, "wb", stderr);

	memset(&Scr, 0, sizeof(Scr));
	/* for use on restart */
	g_argv = xmalloc((argc + 4) * sizeof(char *));
	g_argc = argc;
	for (i = 0; i < argc; i++)
	{
		g_argv[i] = argv[i];
	}
	g_argv[g_argc] = NULL;

	FlocaleInit(LC_CTYPE, "", "", "fvwm");
	FGettextInit("fvwm", LOCALEDIR, "fvwm");

	setVersionInfo();
	/* Put the default module directory into the environment so it can be
	 * used later by the config file, etc.  */
	flib_putenv("FVWM_MODULEDIR", "FVWM_MODULEDIR=" FVWM_MODULEDIR);

	/* Figure out user's home directory */
	home_dir = getenv("HOME");
#ifdef HAVE_GETPWUID
	if (home_dir == NULL)
	{
		struct passwd* pw = getpwuid(getuid());
		if (pw != NULL)
		{
			home_dir = xstrdup(pw->pw_dir);
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
		char *s;

		fvwm_userdir = xstrdup(CatString2(home_dir, "/.fvwm"));
		/* Put the user directory into the environment so it can be used
		 * later everywhere. */
		s = xstrdup(CatString2("FVWM_USERDIR=", fvwm_userdir));
		flib_putenv("FVWM_USERDIR", s);
		free(s);
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
		if (strcmp(argv[i], "-debug_stack_ring") == 0 ||
		    strcmp(argv[i], "--debug-stack-ring") == 0)
		{
			debugging_stack_ring = True;
		}
		else if (strcmp(argv[i], "-D") == 0 ||
			 strcmp(argv[i], "-debug") == 0 ||
			 strcmp(argv[i], "--debug") == 0)
		{
			debugging = True;
		}
		else if (strcmp(argv[i], "-i") == 0 ||
			 strcmp(argv[i], "-clientid") == 0 ||
			 strcmp(argv[i], "--clientid") == 0 ||
			 strcmp(argv[i], "-clientId") == 0 ||
			 strcmp(argv[i], "--clientId") == 0)
		{
			if (++i >= argc)
			{
				usage(0);
				exit(1);
			}
			SetClientID(argv[i]);
		}
		else if (strcmp(argv[i], "-F") == 0 ||
			 strcmp(argv[i], "-restore") == 0 ||
			 strcmp(argv[i], "--restore") == 0)
		{
			if (++i >= argc)
			{
				usage(0);
				exit(1);
			}
			state_filename = argv[i];
		}
		else if (strcmp(argv[i], "-s") == 0 ||
			 strcmp(argv[i], "-single-screen") == 0 ||
			 strcmp(argv[i], "--single-screen") == 0)
		{
			do_force_single_screen = True;
			if (i+1 < argc && argv[i+1][0] != '-')
			{
				i++;
				if (sscanf(argv[i], "%d", &single_screen_num) ==
				    0)
				{
					usage(0);
					exit(1);
				}
			}
		}
		else if (strcmp(argv[i], "-d") == 0 ||
			 strcmp(argv[i], "-display") == 0 ||
			 strcmp(argv[i], "--display") == 0)
		{
			if (++i >= argc)
			{
				usage(0);
				exit(1);
			}
			display_name = argv[i];
		}
		else if (strcmp(argv[i], "-f") == 0)
		{
			if (++i >= argc)
			{
				usage(0);
				exit(1);
			}
			if (num_config_commands < MAX_CFG_CMDS)
			{
				config_commands[num_config_commands] =
					(char *)malloc(6+strlen(argv[i]));
				strcpy(config_commands[num_config_commands],
				       "Read ");
				strcat(config_commands[num_config_commands],
				       argv[i]);

				/* Check to see if the file requested exists.
				 * If it doesn't, use the default config
				 * instead.
				 */

				if (access(argv[i], F_OK) != 0) {
					free(config_commands[num_config_commands]);
					config_commands[num_config_commands] =
						xstrdup("Read " FVWM_DATADIR "/default-config/config");
				}
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
		else if (strcmp(argv[i], "-c") == 0 ||
			 strcmp(argv[i], "-cmd") == 0 ||
			 strcmp(argv[i], "--cmd") == 0)
		{
			if (++i >= argc)
			{
				usage(0);
				exit(1);
			}
			if (num_config_commands < MAX_CFG_CMDS)
			{
				config_commands[num_config_commands] =
					xstrdup(argv[i]);
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
		else if (strcmp(argv[i], "-h") == 0 ||
			 strcmp(argv[i], "-?") == 0 ||
			 strcmp(argv[i], "--help") == 0)
		{
			usage(1);
			exit(0);
		}
		else if (strcmp(argv[i], "-blackout") == 0)
		{
			/* obsolete option */
			fvwm_msg(
				WARN, "main",
				"The -blackout option is obsolete, it will be "
				"removed in 3.0.");
		}
		else if (strcmp(argv[i], "-r") == 0 ||
			 strcmp(argv[i], "-replace") == 0 ||
			 strcmp(argv[i], "--replace") == 0)
		{
			replace_wm = True;
		}
		/* check for visualId before visual to remove ambiguity */
		else if (strcmp(argv[i], "-I") == 0 ||
			 strcmp(argv[i], "-visualid") == 0 ||
			 strcmp(argv[i], "--visualid") == 0 ||
			 strcmp(argv[i], "-visualId") == 0 ||
			 strcmp(argv[i], "--visualId") == 0)
		{
			visualClass = -1;
			if (++i >= argc)
			{
				usage(0);
				exit(1);
			}
			if (sscanf(argv[i], "0x%x", &visualId) == 0)
			{
				if (sscanf(argv[i], "%d", &visualId) == 0)
				{
					usage(0);
					exit(1);
				}
			}
		}
		else if (strcmp(argv[i], "-C") == 0 ||
			 strcmp(argv[i], "-visual") == 0 ||
			 strcmp(argv[i], "--visual") == 0)
		{
			visualId = None;
			if (++i >= argc)
			{
				usage(0);
				exit(1);
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
				usage(0);
				exit(1);
			}
		}
		else if (strcmp(argv[i], "-l") == 0 ||
			 strcmp(argv[i], "-color-limit") == 0 ||
			 strcmp(argv[i], "--color-limit") == 0)
		{
			if (++i >= argc)
			{
				usage(0);
				exit(1);
			}
			colorLimitop.color_limit = atoi(argv[i]);
		}
		else if (strcmp(argv[i], "-L") == 0 ||
			 strcmp(argv[i], "-strict-color-limit") == 0 ||
			 strcmp(argv[i], "--strict-color-limit") == 0)
		{
			colorLimitop.strict = True;
		}
		else if (strcmp(argv[i], "-A") == 0 ||
			 strcmp(argv[i], "-allocate-palette") == 0 ||
			 strcmp(argv[i], "--allocate-palette") == 0)
		{
			colorLimitop.allocate = True;
		}
		else if (strcmp(argv[i], "-S") == 0 ||
			 strcmp(argv[i], "-static-palette") == 0 ||
			 strcmp(argv[i], "--static-palette") == 0)
		{
			colorLimitop.not_dynamic = True;
		}
		else if (strcmp(argv[i], "-P") == 0 ||
			 strcmp(argv[i], "-visual-palette") == 0 ||
			 strcmp(argv[i], "--visual-palette") == 0)
		{
			colorLimitop.use_named_table = True;
		}
		else if (strcmp(argv[i], "-V") == 0 ||
			 strcmp(argv[i], "-version") == 0 ||
			 strcmp(argv[i], "--version") == 0)
		{
			printf("%s\n%s\n\n%s\n", Fvwm_VersionInfo,
			       Fvwm_SupportInfo, Fvwm_LicenseInfo);
			exit(0);
		}
		else
		{
			usage(0);
			fprintf(stderr, "invalid option -- %s\n", argv[i]);
			exit(1);
		}
	}

	DBUG("main", "Done parsing args");

	DBUG("main", "Installing signal handlers");
	InstallSignals();

	if (single_screen_num >= 0)
	{
		char *dn = NULL;

		if (display_name)
		{
			dn = display_name;
		}
		if (!dn)
		{
			dn = getenv("DISPLAY");
		}
		if (!dn)
		{
			/* should never happen ? */
			if (!(dpy = XOpenDisplay(dn)))
			{
				fvwm_msg(
					ERR, "main", "can't open display %s"
					"to get the default display",
					XDisplayName(dn));
			}
			else
			{
				dn = XDisplayString(dpy);
			}
		}
		if (dn == NULL)
		{
			fvwm_msg(
				ERR,
				"main", "couldn't find default display (%s)",
				XDisplayName(dn));
		}
		else
		{
			char *new_dn;

			new_dn = get_display_name(dn, single_screen_num);
			if (dpy && strcmp(new_dn, dn) == 0)
			{
				/* allready opened */
				Pdpy = dpy;
			}
			else if (dpy)
			{
				XCloseDisplay(dpy);
				dpy = NULL;
			}
			if (!dpy && !(Pdpy = dpy = XOpenDisplay(new_dn)))
			{
				fvwm_msg(
					ERR, "main",
					"can't open display %s, single screen "
					"number %d maybe not correct",
					new_dn, single_screen_num);
			}
			Scr.screen = single_screen_num;
			Scr.NumberOfScreens = ScreenCount(dpy);
			free(new_dn);
		}
	}

	if (!dpy)
	{
		if(!(Pdpy = dpy = XOpenDisplay(display_name)))
		{
			fvwm_msg(
				ERR, "main", "can't open display %s",
				XDisplayName(display_name));
			exit (1);
		}
		Scr.screen= DefaultScreen(dpy);
		Scr.NumberOfScreens = ScreenCount(dpy);
	}

	atexit(catch_exit);
	master_pid = getpid();

	if (!do_force_single_screen)
	{
		int myscreen = 0;
		char *new_dn;
		char *dn;

		dn = XDisplayString(dpy);
		for (i=0;i<Scr.NumberOfScreens;i++)
		{
			if (i != Scr.screen && fork() == 0)
			{
				myscreen = i;
				new_dn = get_display_name(dn, myscreen);
				Pdpy = dpy = XOpenDisplay(new_dn);
				Scr.screen = myscreen;
				Scr.NumberOfScreens = ScreenCount(dpy);
				free(new_dn);

				break;
			}
		}

		g_argv[argc++] = "-s";
		g_argv[argc] = NULL;
	}
	FScreenInit(dpy);
	x_fd = XConnectionNumber(dpy);

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
	display_string = xmalloc(len+10);
	sprintf(display_string, "DISPLAY=%s",XDisplayString(dpy));
	flib_putenv("DISPLAY", display_string);
	/* Add a HOSTDISPLAY environment variable, which is the same as
	 * DISPLAY, unless display = :0.0 or unix:0.0, in which case the full
	 * host name will be used for ease in networking.
	 */
	if (strncmp(display_string, "DISPLAY=:",9)==0)
	{
		char client[MAXHOSTNAME], *rdisplay_string;
		gethostname(client,MAXHOSTNAME);
		/* TA:  FIXME!  xasprintf() */
		rdisplay_string = xmalloc(len+14 + strlen(client));
		sprintf(rdisplay_string, "HOSTDISPLAY=%s:%s", client,
			&display_string[9]);
		flib_putenv("HOSTDISPLAY", rdisplay_string);
		free(rdisplay_string);
	}
	else if (strncmp(display_string, "DISPLAY=unix:",13)==0)
	{
		char client[MAXHOSTNAME], *rdisplay_string;
		gethostname(client,MAXHOSTNAME);
		/* TA:  FIXME!  xasprintf() */
		rdisplay_string = xmalloc(len+14 + strlen(client));
		sprintf(rdisplay_string, "HOSTDISPLAY=%s:%s", client,
			&display_string[13]);
		flib_putenv("HOSTDISPLAY", rdisplay_string);
		free(rdisplay_string);
	}
	else
	{
		char *rdisplay_string;
		/* TA:  FIXME!  xasprintf() */
		rdisplay_string = xmalloc(len+14);
		sprintf(rdisplay_string, "HOSTDISPLAY=%s",XDisplayString(dpy));
		flib_putenv("HOSTDISPLAY", rdisplay_string);
		free(rdisplay_string);
	}
	free(display_string);

	Scr.Root = RootWindow(dpy, Scr.screen);
	if (Scr.Root == None)
	{
		fvwm_msg(
			ERR, "main", "Screen %d is not a valid screen",
			(int)Scr.screen);
		exit(1);
	}

	{
		XVisualInfo template, *vinfo = NULL;
		int total, i;

		Pdepth = 0;
		Pdefault = False;
		total = 0;
		template.screen = Scr.screen;
		if (visualClass != -1)
		{
			template.class = visualClass;
			vinfo = XGetVisualInfo(dpy,
					VisualScreenMask|VisualClassMask,
					&template, &total);
			if (!total)
			{
				fvwm_msg(ERR, "main",
					"Cannot find visual class %d",
					visualClass);
			}
		}
		else if (visualId != -1)
		{
			template.visualid = visualId;
			vinfo = XGetVisualInfo(dpy,
					VisualScreenMask|VisualIDMask,
					&template, &total);
			if (!total)
			{
				fvwm_msg(ERR, "main",
					"VisualId 0x%x is not valid ",
					visualId);
			}
		}

		/* visualID's are unique so there will only be one.
		   Select the visualClass with the biggest depth */
		for (i = 0; i < total; i++)
		{
			if (vinfo[i].depth > Pdepth)
			{
				Pvisual = vinfo[i].visual;
				Pdepth = vinfo[i].depth;
			}
		}
		if (vinfo)
		{
			XFree(vinfo);
		}

		/* Detection of a card with 2 hardware colormaps (8+24) which
		 * use depth 8 for the default. We can use our own depth 24
		 * cmap without affecting other applications. */
		if (Pdepth == 0 && DefaultDepth(dpy, Scr.screen) <= 8)
		{
			template.class = TrueColor;
			vinfo = XGetVisualInfo(
				dpy, VisualScreenMask|VisualClassMask,
				&template, &total);

			for(i = 0; i<total; i++)
			{
				if (Pdepth < vinfo[i].depth &&
				    vinfo[i].depth > 8)
				{
					Pvisual = vinfo[i].visual;
					Pdepth = vinfo[i].depth;
				}
			}
			if (vinfo)
			{
				XFree(vinfo);
			}
		}

		/* have to have a colormap for non-default visual windows */
		if (Pdepth > 0)
		{
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
		}
		/* use default visuals if none found so far */
		else
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
		PICTURE_CALLED_BY_FVWM, True, &colorLimitop, True, True);

#ifdef Frsvg_init
	Frsvg_init();
#endif
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
	XDefineCursor(dpy, Scr.Root, Scr.FvwmCursors[CRS_ROOT]);
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
	FOCUS_SET(Scr.NoFocusWin, NULL);

	frame_init();
	XFlush(dpy);
	if (debugging)
	{
		sync_server(1);
	}

	SetupICCCM2(replace_wm);
	XSetIOErrorHandler(CatchFatal);
	{
		/* We need to catch any errors of XSelectInput on the root
		 * window here.  The event mask contains
		 * SubstructureRedirectMask which can be acquired by exactly
		 * one client (window manager).  Synchronizing is necessary
		 * here because Neither XSetErrorHandler nor XSelectInput
		 * generate any protocol requests.
		 */
		XSync(dpy, 0);
		XSetErrorHandler(CatchRedirectError);
		XSelectInput(dpy, Scr.Root, XEVMASK_ROOTW);
		XSync(dpy, 0);
		XSetErrorHandler(FvwmErrorHandler);
	}
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
	restart_state_filename = xstrdup(
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

	Scr.gray_bitmap =
		XCreateBitmapFromData(dpy,Scr.Root,g_bits, g_width,g_height);

	EWMH_Init();

	DBUG("main", "Setting up rc file defaults...");
	SetRCDefaults();
	flush_window_updates();
	simplify_style_list();

	DBUG("main", "Running config_commands...");
	ecc.type = Restarting ? EXCT_RESTART : EXCT_INIT;
	ecc.w.wcontext = C_ROOT;
	exc = exc_create_context(&ecc, ECC_TYPE | ECC_WCONTEXT);
	if (num_config_commands > 0)
	{
		int i;
		for (i = 0; i < num_config_commands; i++)
		{
			DoingCommandLine = True;
			execute_function(NULL, exc, config_commands[i], 0);
			free(config_commands[i]);
		}
		DoingCommandLine = False;
	}
	else
	{
		/* Run startup command file in these places (default prefix):
		 *   ~/.fvwm/config
		 *   /usr/local/share/fvwm/config
		 * and for compatibility:
		 *   ~/.fvwm/.fvwm2rc
		 *   /usr/local/share/fvwm/system.fvwm2rc
		 * and for compatibility to be discontinued:
		 *   ~/.fvwm2rc,
		 *   /usr/local/share/fvwm/.fvwm2rc
		 *   /usr/local/etc/system.fvwm2rc
		 */
		if (
			!run_command_file(CatString3(
				fvwm_userdir, "/", FVWM_CONFIG), exc) &&
			!run_command_file(CatString3(
				FVWM_DATADIR, "/", FVWM_CONFIG), exc) &&
			!run_command_file(CatString3(
				fvwm_userdir, "/", FVWM2RC), exc) &&
			!run_command_file(CatString3(
				home_dir, "/", FVWM2RC), exc) &&
			!run_command_file(CatString3(
				FVWM_DATADIR, "/", FVWM2RC), exc) &&
			!run_command_file(CatString3(
				FVWM_DATADIR, "/system", FVWM2RC), exc) &&
			!run_command_file(CatString3(
				FVWM_CONFDIR, "/system", FVWM2RC), exc) &&
			!run_command_file(CatString3(
				FVWM_DATADIR, "/default-config/", "config"), exc))
		{
			fvwm_msg(
				ERR, "main", "Cannot read startup config file,"
				" tried: \n\t%s/%s\n\t%s/%s\n\t%s/%s\n\t"
				"%s/%s\n\t%s/%s\n\t%s/system%s\n\t%s/system%s",
				fvwm_userdir, FVWM_CONFIG,
				FVWM_DATADIR, FVWM_CONFIG,
				fvwm_userdir, FVWM2RC,
				home_dir, FVWM2RC,
				FVWM_DATADIR, FVWM2RC,
				FVWM_DATADIR, FVWM2RC,
				FVWM_CONFDIR, FVWM2RC);
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
		dpy, Scr.Root, 0, 0, 1, 1, 0, Pdepth,
		InputOutput, Pvisual, valuemask, &attributes);
	resize_geometry_window();
	initPanFrames();
	MyXGrabServer(dpy);
	checkPanFrames();
	MyXUngrabServer(dpy);
	CoerceEnterNotifyOnCurrentWindow();
	SessionInit();
	DBUG("main", "Entering HandleEvents loop...");

	HandleEvents();
	switch (fvwmRunState)
	{
	case FVWM_DONE:
		Done(0, NULL);     /* does not return */

	case FVWM_RESTART:
		Done(1, "");       /* does not return */

	default:
		DBUG("main", "Unknown fvwm run-state");
	}

	exit(0);
}
