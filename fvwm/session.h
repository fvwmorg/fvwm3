/* File: session.h
 *
 * Description:
 *       exports from session.c shall go into this file
 *
 * Created:
 *       4 April 1999 - Steve Robbins <steve@nyongwa.montreal.qc.ca>
 */

#ifndef SESSION_H
#define SESSION_H


/*
**  Load and save the 'global', ie not window-related, state of FVWM 
**  into a file.
*/
void LoadGlobalState(char *filename);
int SaveGlobalState(FILE *f);


/*
**  Load and save window states.  
*/
void LoadWindowStates(char *filename);
int SaveWindowStates(FILE *f);


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
void MatchWinToSM(FvwmWindow *t, int *x, int *y, int *w, int *h, 
                  int *shade, int *maximize);


/*
**  Try to open a connection to the session manager. If non-NULL,
**  reuse the client_id.
 */
void SessionInit(char *client_id);


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

#endif

