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
/*
 * fvwmsignal.c
 * Written by Chris Rankin,  rankinc@zipworld.com.au
 *
 */

#include "config.h"

#include <sys/wait.h>
#include <setjmp.h>
#include <errno.h>
#include "fvwmsignal.h"

#define true   1
#define false  0


typedef enum { SIG_INIT=0, SIG_DONE } SIG_STATUS;

volatile sig_atomic_t isTerminated = false;

static volatile sig_atomic_t canJump = false;
#ifdef HAVE_SIGSETJMP
static sigjmp_buf deadJump;
#define SIGSETJMP(env, savesigs) sigsetjmp(env, savesigs)
#else
static jmp_buf deadJump;
#define SIGSETJMP(env, savesigs) setjmp(env)
#endif
#if defined(HAVE_SIGLONGJMP) && defined(HAVE_SIGSETJMP)
#define SIGLONGJMP(env, val) siglongjmp(env, val)
#else
#define SIGLONGJMP(env, val) longjmp(env, val)
#endif
/*
 * Reap child processes, preventing them from becoming zombies.
 * We do this asynchronously within the SIGCHLD handler so that
 * "it just happens".
 */
RETSIGTYPE
fvwmReapChildren(int sig)
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

	SIGNAL_RETURN;
}

#ifdef USE_BSD_SIGNALS
static int term_sigs;

/*
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


/*
 * fvwmGetSignalMask - get the set of signals that will terminate fvwm
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


/*
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
		SIGLONGJMP(deadJump, SIG_DONE);
	}

	BSD_UNBLOCK_SIGNALS;
}


#ifdef HAVE_SELECT
/*
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

#ifdef C_ALLOCA
	/* If we're using the C version of alloca, see if anything needs to be
	 * freed up.
	 */
	alloca(0);
#endif

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
	if ( SIGSETJMP(deadJump, 1) == SIG_INIT )
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
