#ifndef FVWMSIGNAL_H
#define FVWMSIGNAL_H

/***************************************************************************
 * This module provides wrappers around system functions that could
 * potentially block, e.g. select(). These wrappers will check that
 * the "terminate" flag is still clear and then call the system
 * function in one atomic operation. This ensures that FVWM will not
 * block in a system function once it has received the signal to quit.
 *
 * This module was written by Chris Rankin,  rankinc@zipworld.com.au
 ***************************************************************************/

/***************************************************************************
 * This module is intended to use POSIX.1 signal semantics, since most
 * modern systems can reasonably be expected to support POSIX and since
 * the semantics of the old "signal()" function vary from system to system.
 * If POSIX.1 is not available then the module can provide BSD signal
 * semantics, which can be summarised as follows:
 *  - the signal handler will NOT uninstall itself once it has been called
 *  - a signal will be temporarily blocked from further delivery so long
 *    as its handler is running
 *  - certain system calls will be automatically restarted if interrupted
 *    by a signal
 */

#if !defined(HAVE_SIGACTION) \
    && defined(HAVE_SIGBLOCK) && defined(HAVE_SIGSETMASK)
#  define USE_BSD_SIGNALS
#endif

#ifdef USE_BSD_SIGNALS
#  define BSD_BLOCK_SIGNALS      int old_mask = sigblock( fvwmGetSignalMask() )
#  define BSB_BLOCK_ALL_SIGNALS  int old_mask = sigblock( ~0 )
#  define BSD_UNBLOCK_SIGNALS    sigsetmask( old_mask )
#else
#  define BSD_BLOCK_SIGNALS
#  define BSD_BLOCK_ALL_SIGNALS
#  define BSD_UNBLOCK_SIGNALS
#endif

#include <signal.h>
#include <sys/time.h>
#if HAVE_SYS_SELECT_H
#  include <sys/select.h>
#endif

/***************************************************************************
 * Global variables
 */
extern volatile sig_atomic_t isTerminated;
#ifdef FVWM_DEBUG_MSGS
extern volatile sig_atomic_t debug_term_signal;
#endif


/***************************************************************************
 * Module prototypes
 */
extern void fvwmSetTerminate(int sig);

#ifdef USE_BSD_SIGNALS
extern void fvwmSetSignalMask(int);
extern int fvwmGetSignalMask(void);
#endif

#ifdef 	HAVE_SELECT
extern int fvwmSelect(fd_set_size_t nfds,
                      fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
                      struct timeval *timeout);
#endif

#endif /* FVWMSIGNAL_H */

