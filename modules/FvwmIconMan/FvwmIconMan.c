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

#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include "FvwmIconMan.h"
#include "readconfig.h"
#include "x.h"
#include "xmanager.h"

#include "libs/fvwmsignal.h"
#include "libs/Module.h"


static fd_set_size_t fd_width;

static char *IM_VERSION = "1.3";

static char const rcsid[] =
  "$Id$";

const char *MyName;

static RETSIGTYPE TerminateHandler(int);

char *copy_string(char **target, const char *src)
{
  int len = strlen(src);
  ConsoleDebug(CORE, "copy_string: 1: 0x%lx\n", (unsigned long)*target);

  if (*target)
    Free(*target);

  ConsoleDebug(CORE, "copy_string: 2\n");
  *target = (char *)safemalloc((len + 1) * sizeof(char));
  strcpy(*target, src);
  ConsoleDebug(CORE, "copy_string: 3\n");
  return *target;
}

#ifdef TRACE_MEMUSE

long MemUsed = 0;

void Free (void *p)
{
  struct malloc_header *head = (struct malloc_header *)p;

  if (p != NULL) {
    head--;
    if (head->magic != MALLOC_MAGIC) {
      fprintf(stderr, "Corrupted memory found in Free\n");
      abort();
      return;
    }
    if (head->len > MemUsed) {
      fprintf(stderr, "Free block too big\n");
      return;
    }
    MemUsed -= head->len;
    free(head);
  }
}

void PrintMemuse(void)
{
  ConsoleDebug(CORE, "Memory used: %d\n", MemUsed);
}

#else

void Free(void *p)
{
  if (p != NULL)
    free(p);
}

void PrintMemuse(void)
{
}

#endif


static RETSIGTYPE
TerminateHandler(int sig)
{
  fvwmSetTerminate(sig);
}


void
ShutMeDown(int flag)
{
  ConsoleDebug(CORE, "Bye Bye\n");
  exit(flag);
}

void
DeadPipe(int nothing)
{
  (void)nothing;
  ShutMeDown(0);
}

static void
main_loop (void)
{
  fd_set readset, saveset;

  FD_ZERO(&saveset);
  FD_SET(Fvwm_fd[1], &saveset);
  FD_SET(x_fd, &saveset);

  while( !isTerminated ) {
    /* Check the pipes for anything to read, and block if
     * there is nothing there yet ...
     */
    readset = saveset;
    if (fvwmSelect(fd_width, &readset, NULL, NULL, NULL) < 0) {
      ConsoleMessage("Internal error with select: errno=%s\n",
                     strerror(errno));
    }
    else {

      if (FD_ISSET(x_fd, &readset) || XPending(theDisplay)) {
        xevent_loop();
      }
      if (FD_ISSET(Fvwm_fd[1], &readset)) {
        ReadFvwmPipe();
      }

    }
  } /* while */
}

int
main(int argc, char **argv)
{
  const char *s;
  int i;

#ifdef ELECTRIC_FENCE
  extern int EF_PROTECT_BELOW, EF_PROTECT_FREE;

  EF_PROTECT_BELOW = 1;
  EF_PROTECT_FREE = 1;
#endif

#ifdef DEBUG_ATTACH
  {
    char buf[256];
    sprintf(buf, "%d", getpid());
    if (fork() == 0) {
      chdir("/home/bradym/src/FvwmIconMan");
      execl("/usr/local/bin/ddd", "/usr/local/bin/ddd", "FvwmIconMan",
	     buf, NULL);
    }
    else {
      int i, done = 0;
      for (i = 0; i < (1 << 27) && !done; i++) ;
    }
  }
#endif

#ifdef I18N_MB
  setlocale(LC_CTYPE, "");
#endif

  OpenConsole(OUTPUT_FILE);

#if 0
  ConsoleMessage("PID = %d\n", getpid());
  ConsoleMessage("Waiting for GDB to attach\n");
  sleep(10);
#endif

  init_globals();
  init_winlists();

  MyName = argv[0];
  s = strrchr(argv[0], '/');
  if (s != NULL)
    MyName = s + 1;

  if (argc == 7)
  {
    if (strcasecmp (argv[6], "Transient")==0)
      globals.transient = 1;
    else
    {
      MyName = argv[6];
    } 
  }
  ModuleLen = strlen(MyName) + 1;
  Module = safemalloc(ModuleLen+1);
  *Module = '*';
  strcpy (Module+1, MyName);

  if((argc != 6) && (argc != 7)) {
    fprintf(stderr,"%s Version %s should only be executed by fvwm!\n",Module,
      IM_VERSION);
    ShutMeDown(1);
  }
  Fvwm_fd[0] = atoi(argv[1]);
  Fvwm_fd[1] = atoi(argv[2]);
  init_display();
  init_boxes();

#ifdef HAVE_SIGACTION
  {
    struct sigaction  sigact;

    sigemptyset(&sigact.sa_mask);
    sigaddset(&sigact.sa_mask, SIGPIPE);
    sigaddset(&sigact.sa_mask, SIGINT);
    sigaddset(&sigact.sa_mask, SIGHUP);
    sigaddset(&sigact.sa_mask, SIGTERM);
    sigaddset(&sigact.sa_mask, SIGQUIT);
# ifdef SA_INTERRUPT
    sigact.sa_flags = SA_INTERRUPT;
# else
    sigact.sa_flags = 0;
# endif
    sigact.sa_handler = TerminateHandler;

    sigaction(SIGPIPE, &sigact, NULL);
    sigaction(SIGINT,  &sigact, NULL);
    sigaction(SIGHUP,  &sigact, NULL);
    sigaction(SIGTERM, &sigact, NULL);
    sigaction(SIGQUIT, &sigact, NULL);
  }
#else
  /* We don't have sigaction(), so fall back to less robust methods.  */
#ifdef USE_BSD_SIGNALS
  fvwmSetSignalMask( sigmask(SIGPIPE) |
                     sigmask(SIGINT) |
                     sigmask(SIGHUP) |
                     sigmask(SIGTERM) |
                     sigmask(SIGQUIT) );
#endif
  signal(SIGPIPE, TerminateHandler);
  signal(SIGINT,  TerminateHandler);
  signal(SIGHUP,  TerminateHandler);
  signal(SIGTERM, TerminateHandler);
  signal(SIGQUIT, TerminateHandler);
#ifdef HAVE_SIGINTERRUPT
  siginterrupt(SIGPIPE, 1);
  siginterrupt(SIGINT, 1);
  siginterrupt(SIGHUP, 1);
  siginterrupt(SIGTERM, 1);
  siginterrupt(SIGQUIT, 1);
#endif
#endif

  read_in_resources(argv[3]);

  for (i = 0; i < globals.num_managers; i++) {
    X_init_manager(i);
  }

  assert(globals.managers);
  fd_width = GetFdWidth();

  SetMessageMask(Fvwm_fd, M_CONFIGURE_WINDOW | M_RES_CLASS | M_RES_NAME |
                 M_ADD_WINDOW | M_DESTROY_WINDOW | M_ICON_NAME |
                 M_DEICONIFY | M_ICONIFY | M_END_WINDOWLIST |
                 M_NEW_DESK | M_NEW_PAGE | M_FOCUS_CHANGE | M_WINDOW_NAME |
                 M_CONFIG_INFO |
#ifdef MINI_ICONS
		 M_MINI_ICON |
#endif
		 M_STRING);

  SendInfo(Fvwm_fd, "Send_WindowList", 0);

  /* tell fvwm we're running */
  SendFinishedStartupNotification(Fvwm_fd);

  /* Lock on send only for iconify and deiconify (for NoIconAction) */
  SetSyncMask(Fvwm_fd, M_DEICONIFY | M_ICONIFY);

  main_loop();

#ifdef FVWM_DEBUG_MSGS
  if ( debug_term_signal )
  {
    fvwm_msg(DBG, "main", "Terminated by signal %d", debug_term_signal);
  }
#endif

  return 0;
}

