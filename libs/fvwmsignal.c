/***************************************************************************
 * fvwmsignal.c
 * Written by Chris Rankin,  rankinc@zipworld.com.au
 *
 ***************************************************************************/

#include "config.h"

#include <setjmp.h>
#include <errno.h>
#include "fvwmsignal.h"

#define true   1
#define false  0


typedef enum { SIG_INIT=0, SIG_DONE } SIG_STATUS;

volatile sig_atomic_t isTerminated = false;
#ifdef FVWM_DEBUG_MSGS
volatile sig_atomic_t debug_term_signal = 0;
#endif

static volatile sig_atomic_t canJump = false;
static sigjmp_buf deadJump;

#ifdef USE_BSD_SIGNALS
static int term_sigs;


/************************************************************************
 * fvwmSetSignalMask - store the set of mutually exclusive signals
 * away for future reference. This prevents different signals from
 * trying to access the same static data at the same time.
 *
 * NOTE: We don't need this if we have POSIX.1 since we can install
 *       a signal mask automatically using sigaction()
 */
void
fvwmSetSignalMask(int sigmask)
{
  term_sigs = sigmask;
}


/************************************************************************
 * fvwmGetSignalMask - get the set of signals that will terminate FVWM
 *
 * NOTE: We don't need this if we have POSIX.1 since we can install
 *       a signal mask automatically using sigaction()
 */
int
fvwmGetSignalMask(void)
{
  return term_sigs;
}

#endif


/************************************************************************
 * fvwmSetTerminate - set the "end-of-execution" flag.
 * This function should ONLY be called at the end of a
 * signal handler. It is an integral part of the mechanism
 * to stop system calls from blocking once the process is
 * finished.
 *
 * NOTE: This is NOT a signal handler function in its own right!
 */
void
fvwmSetTerminate(int sig)
{
  BSD_BLOCK_SIGNALS;

  isTerminated = true;
#ifdef DEBUG
  debug_term_signal = sig;
#endif

  if (canJump)
  {
    canJump = false;

    /*
     * This non-local jump is safe ONLY because we haven't called
     * any non-reentrant functions in the short period where the
     * "canJump" variable is true.
     *
     * NOTE: No need to restore the signal mask, since siglongjmp
     *       is designed to do that for us.
     */
    siglongjmp(deadJump, SIG_DONE);
  }

  BSD_UNBLOCK_SIGNALS;
}


#ifdef HAVE_SELECT
/************************************************************************
 * fvwmSelect - wrapper around the select() system call.
 * This system call may block indefinitely. We don't want
 * to block at all if the "terminate" flag is set - we
 * just want it to fail as quickly as possible.
 */
int
fvwmSelect(fd_set_size_t nfds,
	   fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
	   struct timeval *timeout)
{
  volatile int iRet = -1;  /* This variable MUST NOT be in a register */

  /*
   * Yes, we trash errno here, but you're only supposed to check
   * errno immediately after a function fails anyway. If we fail,
   * then it's because we received a signal. If we succeed, we
   * shouldn't be checking errno. And if somebody calls us expecting
   * us to preserve errno then that's their bug.
   *
   * NOTE: We mustn't call any function that might trash errno
   *       ourselves, except select() itself of course. I believe
   *       that sigsetjmp() does NOT trash errno.
   */
  errno = EINTR;

  /*
   * Now initialise the non-local jump. Between here and the end of
   * the routine (more-or-less) we must NOT call any non-reentrant
   * functions! This is because we might need to abandon them half
   * way through execution and return here!
   */
  if ( sigsetjmp(deadJump, 1) == SIG_INIT )
  {
    /*
     * Activate the non-local jump. Between now and when we turn the
     * jump off again, we must NOT call any non-reentrant functions
     * because we could be interrupted halfway through ...
     */
    canJump = true;

    /*
     * If we have already been told to terminate then we will not
     * execute the select() because the flag will be set. If a
     * "terminate" signal arrives between testing the flag and
     * calling select() then we will jump back to the non-local
     * jump point ...
     */
    if ( !isTerminated )
    {
      /*
       * The "die" signal will interrupt this system call:
       * that IS the whole point, after all :-)
       */
      iRet = select(nfds,
		    SELECT_FD_SET_CAST readfds,
		    SELECT_FD_SET_CAST writefds,
		    SELECT_FD_SET_CAST exceptfds,
		    timeout);
    }

    /*
     * The non-local jump is about to go out of scope,
     * so we must deactivate it. Note that the return-
     * value from select() will be safely stored in the
     * local variable before the jump is disabled.
     */
    canJump = false;
  }

  return iRet;
}

#endif  /* HAVE_SELECT */

