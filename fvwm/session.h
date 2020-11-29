/* -*-c-*- */

/* Description:
 *       exports from session.c shall go into this file
 *
 * Created:
 *       4 April 1999 - Steve Robbins <steve@nyongwa.montreal.qc.ca>
 */

#ifndef SESSION_H
#define SESSION_H
#include "fvwm.h"

/*
**  Load and save the 'global', ie not window-related, state of fvwm
**  into a file.
*/
void LoadGlobalState(char *filename);

/*
**  Turn off SM for new windows
*/
void DisableRestoringState(void);

/*
**  Load and save window states.
*/
void LoadWindowStates (char *filename);

/*
** Save state to the named file, and if running under SM,
** make the SM properly restart fvwm.
*/
void RestartInSession (char *filename, Bool isNative, Bool doPreserveState);

/*
**  Fill in the FvwmWindow struct with information saved from
**  the last session. This expects the fields
**    t->w
**    t->name
**    t->class
**    t->tmpflags.NameChanged
**  to have meaningful values. The shade and maximize flags are set
**  if the window should start out as shaded or maximized, respecively.
**  The dimensions returned in x, y, w, h should be used when the
**  window is to be maximized.
*/
typedef struct
{
	int shade_dir;
	unsigned do_shade : 1;
	unsigned used_title_dir_for_shading : 1;
	unsigned do_max : 1;
} mwtsm_state_args;

Bool MatchWinToSM(
	FvwmWindow *ewin, mwtsm_state_args *ret_state_args,
	initial_window_options_t *win_opts);

void SetClientID(char *client_id);

/*
**  Try to open a connection to the session manager. If non-NULL,
**  reuse the client_id.
*/
void SessionInit(void);

/*
**  The file number of the session manager connection or -1
**  if no session manager was found.
*/
extern int sm_fd;

/*
**  Process messages received from the session manager. Call this
**  from the main event loop when there is input waiting sm_fd.
*/
void ProcessICEMsgs(void);

/*
 * Fvwm Function implementation
 */
Bool quitSession(void);
Bool saveSession(void);
Bool saveQuitSession(void);

#endif
